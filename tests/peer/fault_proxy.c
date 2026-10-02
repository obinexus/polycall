/*
 * TCP fault-injection proxy for the peer fault tests (test helper only).
 *
 *   polycall_fault_proxy ENDPOINT_FILE TARGET_HOST:PORT MODE [DELAY_MS]
 *
 * Listens on 127.0.0.1:<ephemeral>, writes that endpoint to ENDPOINT_FILE,
 * and handles every connection according to MODE:
 *   forward    relay bytes both ways (a healthy link)
 *   delay      hold the client's request for DELAY_MS, then relay it and
 *              the answer (a slow link: the sender's deadline may expire
 *              while the message is still delivered)
 *   blackhole  accept, swallow everything, never answer (a partition)
 *   reset      accept and close immediately (a flapping link)
 * Runs until killed.
 */

#include "polycall.h"
#include "core/pc_sys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_host[96];
static uint16_t g_port;
static int g_mode;          /* 0 forward 1 delay 2 blackhole 3 reset */
static uint32_t g_delay;

typedef struct { pc_sock_t c; } conn_t;

/* relay until both sides are done or 30 s of silence */
static void pipe_both(pc_sock_t a, pc_sock_t b)
{
    char buf[16384];
    int a_open = 1, b_open = 1;
    uint64_t idle_deadline = pc_deadline_after(30000);
    while ((a_open || b_open) && pc_remaining_ms(idle_deadline) > 0) {
        size_t got = 0;
        int moved = 0;
        if (a_open && pc_wait_readable(a, 20) > 0) {
            if (pc_recv_some(a, buf, sizeof buf, pc_deadline_after(1000), &got) != POLYCALL_OK || got == 0) {
                a_open = 0;
                pc_sock_shutdown(b);
            } else {
                pc_send_all(b, buf, got, pc_deadline_after(5000));
                moved = 1;
            }
        }
        if (b_open && pc_wait_readable(b, 20) > 0) {
            if (pc_recv_some(b, buf, sizeof buf, pc_deadline_after(1000), &got) != POLYCALL_OK || got == 0) {
                b_open = 0;
                break;          /* upstream answered and closed: done */
            }
            pc_send_all(a, buf, got, pc_deadline_after(5000));
            moved = 1;
        }
        if (moved) idle_deadline = pc_deadline_after(30000);
    }
}

static void conn_main(void *arg)
{
    conn_t *ct = (conn_t *)arg;
    pc_sock_t c = ct->c, u = PC_BAD_SOCK;
    free(ct);
    if (g_mode == 3) {
        pc_sock_close(c);
        return;
    }
    if (g_mode == 2) {
        char sink[4096];
        size_t got;
        while (pc_recv_some(c, sink, sizeof sink, pc_deadline_after(60000), &got) == POLYCALL_OK && got) {
        }
        pc_sock_close(c);
        return;
    }
    if (g_mode == 1) {
        /* collect the request (until the client pauses), then hold it */
        char *req = malloc(4u << 20);
        size_t n = 0, got = 0;
        if (!req) { pc_sock_close(c); return; }
        while (n < (4u << 20) && pc_wait_readable(c, n ? 300 : 5000) > 0) {
            if (pc_recv_some(c, req + n, (4u << 20) - n, pc_deadline_after(1000), &got) != POLYCALL_OK || !got) break;
            n += got;
        }
        pc_sleep_ms(g_delay);
        if (pc_connect(g_host, g_port, 2000, &u, NULL, 0) == POLYCALL_OK) {
            pc_send_all(u, req, n, pc_deadline_after(5000));
            pipe_both(c, u);
            pc_sock_close(u);
        }
        free(req);
        pc_sock_close(c);
        return;
    }
    if (pc_connect(g_host, g_port, 2000, &u, NULL, 0) == POLYCALL_OK) {
        pipe_both(c, u);
        pc_sock_close(u);
    }
    pc_sock_close(c);
}

int main(int argc, char **argv)
{
    pc_sock_t ls;
    uint16_t bound = 0;
    char err[200];
    FILE *f;
    if (argc < 4) {
        fprintf(stderr, "usage: %s ENDPOINT_FILE TARGET_HOST:PORT forward|delay|blackhole|reset [DELAY_MS]\n", argv[0]);
        return 2;
    }
    if (pc_split_endpoint(argv[2], g_host, sizeof g_host, &g_port, 0) != POLYCALL_OK) {
        fprintf(stderr, "bad target %s\n", argv[2]);
        return 2;
    }
    if (!strcmp(argv[3], "forward")) g_mode = 0;
    else if (!strcmp(argv[3], "delay")) g_mode = 1;
    else if (!strcmp(argv[3], "blackhole")) g_mode = 2;
    else if (!strcmp(argv[3], "reset")) g_mode = 3;
    else { fprintf(stderr, "bad mode %s\n", argv[3]); return 2; }
    g_delay = argc > 4 ? (uint32_t)atoi(argv[4]) : 1000;
    pc_net_init();
    if (pc_listen("127.0.0.1", 0, 64, &ls, &bound, err, sizeof err) != POLYCALL_OK) {
        fprintf(stderr, "listen: %s\n", err);
        return 1;
    }
    f = fopen(argv[1], "w");
    if (!f) return 1;
    fprintf(f, "127.0.0.1:%u\n", (unsigned)bound);
    fclose(f);
    for (;;) {
        pc_sock_t c;
        pc_thread_t t;
        conn_t *ct;
        if (pc_accept(ls, 1000, &c) != POLYCALL_OK) continue;
        ct = malloc(sizeof *ct);
        if (!ct) { pc_sock_close(c); continue; }
        ct->c = c;
        if (pc_thread_start(&t, conn_main, ct) != 0) {
            free(ct);
            pc_sock_close(c);
            continue;
        }
#if defined(_WIN32)
        CloseHandle(t);
#else
        pthread_detach(t);
#endif
    }
}
