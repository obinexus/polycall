/* polycall_rpc v1 framing on top of the deadline-bounded socket layer. */

#include "rpc_wire.h"

#include "polycall.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pcr_net_init(void)
{
    (void)pc_net_init();
}

void pcr_net_shutdown(void)
{
    /* Winsock stays initialised for the process lifetime (pc_net_init is
     * once-only and thread-safe); the old per-call refcount raced when the
     * runtime and a client ran on different threads. */
}

void pcr_close(pcr_sock_t s)
{
    pc_sock_close(s);
}

static void put_u32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static uint32_t get_u32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static int map(int status)
{
    if (status == POLYCALL_OK) return 0;
    if (status == POLYCALL_E_TIMEOUT) return -3;
    return -1;
}

int pcr_send(pcr_sock_t s, uint8_t type, uint32_t corr,
             const char *payload, uint32_t length, uint32_t timeout_ms)
{
    unsigned char hdr[PCR_HEADER_LEN];
    uint64_t deadline = pc_deadline_after(timeout_ms ? timeout_ms : 5000);
    int rc;
    if (length > PCR_MAX_PAYLOAD) return -2;
    hdr[0] = PCR_MAGIC0; hdr[1] = PCR_MAGIC1; hdr[2] = PCR_MAGIC2; hdr[3] = PCR_MAGIC3;
    hdr[4] = type;
    hdr[5] = 0;
    hdr[6] = hdr[7] = 0;
    put_u32(hdr + 8, corr);
    put_u32(hdr + 12, length);
    rc = pc_send_all(s, hdr, PCR_HEADER_LEN, deadline);
    if (rc == POLYCALL_OK && length) {
        rc = pc_send_all(s, payload, length, deadline);
    }
    return map(rc);
}

int pcr_recv(pcr_sock_t s, pcr_frame_t *frame, uint32_t timeout_ms)
{
    unsigned char hdr[PCR_HEADER_LEN];
    uint64_t deadline = timeout_ms ? pc_deadline_after(timeout_ms) : PC_NO_DEADLINE;
    uint32_t len;
    int rc;

    memset(frame, 0, sizeof *frame);
    rc = pc_recv_exact(s, hdr, PCR_HEADER_LEN, deadline);
    if (rc != POLYCALL_OK) return map(rc);

    if (hdr[0] != PCR_MAGIC0 || hdr[1] != PCR_MAGIC1 ||
        hdr[2] != PCR_MAGIC2 || hdr[3] != PCR_MAGIC3) {
        return -2;
    }
    if (hdr[4] < PCR_T_REQUEST || hdr[4] > PCR_T_REPLY ||
        hdr[5] != 0 || hdr[6] != 0 || hdr[7] != 0) {
        return -2;
    }
    frame->type = hdr[4];
    frame->corr = get_u32(hdr + 8);
    len = get_u32(hdr + 12);
    if (len > PCR_MAX_PAYLOAD) return -2;

    frame->payload = malloc((size_t)len + 1);
    if (!frame->payload) return -1;
    if (len) {
        rc = pc_recv_exact(s, frame->payload, len, deadline);
        if (rc != POLYCALL_OK) {
            free(frame->payload);
            frame->payload = NULL;
            return map(rc);
        }
    }
    frame->payload[len] = '\0';
    frame->length = len;
    return 0;
}

void pcr_frame_free(pcr_frame_t *frame)
{
    if (frame && frame->payload) {
        free(frame->payload);
        frame->payload = NULL;
        frame->length = 0;
    }
}

int pcr_roundtrip(const char *host, uint16_t port, uint8_t type,
                  const char *payload, uint32_t length,
                  pcr_frame_t *reply, uint32_t deadline_ms)
{
    pcr_sock_t s = PCR_BAD_SOCKET;
    uint64_t deadline = pc_deadline_after(deadline_ms ? deadline_ms : 5000);
    int rc;

    memset(reply, 0, sizeof *reply);
    rc = pc_connect(host && *host ? host : "127.0.0.1", port,
                    pc_remaining_ms(deadline), &s, NULL, 0);
    if (rc != POLYCALL_OK) return map(rc);

    rc = pcr_send(s, type, 1, payload, length, pc_remaining_ms(deadline));
    if (rc == 0) {
        uint32_t left = pc_remaining_ms(deadline);
        rc = left ? pcr_recv(s, reply, left) : -3;
        /* corr 0 + REPLY is a server-initiated answer sent before our frame
         * was read (e.g. "server.busy" at the connection limit); anything
         * else must echo our correlation id */
        if (rc == 0 && reply->corr != 1 &&
            !(reply->corr == 0 && reply->type == PCR_T_REPLY)) {
            pcr_frame_free(reply);
            rc = -2;                 /* reply to some other request */
        }
    }
    pcr_close(s);
    return rc;
}
