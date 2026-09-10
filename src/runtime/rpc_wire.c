/* polycall_rpc v1 framing + blocking socket helpers with partial-IO handling. */

#include "rpc_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <errno.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <poll.h>
#  include <sys/socket.h>
#  include <unistd.h>
#endif

static int g_net_refs = 0;

void pcr_net_init(void)
{
#if defined(_WIN32)
    if (g_net_refs++ == 0) {
        WSADATA w;
        WSAStartup(MAKEWORD(2, 2), &w);
    }
#else
    g_net_refs++;
#endif
}

void pcr_net_shutdown(void)
{
#if defined(_WIN32)
    if (g_net_refs > 0 && --g_net_refs == 0) {
        WSACleanup();
    }
#else
    if (g_net_refs > 0) g_net_refs--;
#endif
}

void pcr_close(pcr_sock_t s)
{
#if defined(_WIN32)
    if (s != PCR_BAD_SOCKET) closesocket(s);
#else
    if (s >= 0) close(s);
#endif
}

/* ---- big-endian helpers ------------------------------------------------ */

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

/* ---- raw send/recv, partial-IO tolerant ------------------------------ */

static int send_all(pcr_sock_t s, const unsigned char *buf, size_t len)
{
    size_t off = 0;
    while (off < len) {
#if defined(_WIN32)
        int n = send(s, (const char *)buf + off, (int)(len - off), 0);
        if (n == SOCKET_ERROR) return -1;
#else
        ssize_t n = send(s, buf + off, len - off, 0);
        if (n < 0) { if (errno == EINTR) continue; return -1; }
#endif
        if (n == 0) return -1;
        off += (size_t)n;
    }
    return 0;
}

static int wait_readable(pcr_sock_t s, uint32_t deadline_ms)
{
    if (deadline_ms == 0) return 1;
#if defined(_WIN32)
    {
        fd_set rf;
        struct timeval tv;
        FD_ZERO(&rf);
        FD_SET(s, &rf);
        tv.tv_sec = (long)(deadline_ms / 1000);
        tv.tv_usec = (long)((deadline_ms % 1000) * 1000);
        return select(0, &rf, NULL, NULL, &tv);   /* >0 ready, 0 timeout, <0 err */
    }
#else
    {
        struct pollfd p;
        p.fd = s;
        p.events = POLLIN;
        for (;;) {
            int r = poll(&p, 1, (int)deadline_ms);
            if (r < 0 && errno == EINTR) continue;
            return r;
        }
    }
#endif
}

static int recv_all(pcr_sock_t s, unsigned char *buf, size_t len,
                    uint32_t deadline_ms)
{
    size_t off = 0;
    while (off < len) {
        int w = wait_readable(s, deadline_ms);
        if (w == 0) return -3;      /* timeout */
        if (w < 0) return -1;
#if defined(_WIN32)
        {
            int n = recv(s, (char *)buf + off, (int)(len - off), 0);
            if (n == SOCKET_ERROR) return -1;
            if (n == 0) return -1;  /* peer closed */
            off += (size_t)n;
        }
#else
        {
            ssize_t n = recv(s, buf + off, len - off, 0);
            if (n < 0) { if (errno == EINTR) continue; return -1; }
            if (n == 0) return -1;
            off += (size_t)n;
        }
#endif
    }
    return 0;
}

/* ---- frame API ---------------------------------------------------- */

int pcr_send(pcr_sock_t s, uint8_t type, uint32_t corr,
             const char *payload, uint32_t length)
{
    unsigned char hdr[PCR_HEADER_LEN];
    if (length > PCR_MAX_PAYLOAD) return -1;
    hdr[0] = PCR_MAGIC0; hdr[1] = PCR_MAGIC1; hdr[2] = PCR_MAGIC2; hdr[3] = PCR_MAGIC3;
    hdr[4] = type;
    hdr[5] = 0;
    hdr[6] = hdr[7] = 0;
    put_u32(hdr + 8, corr);
    put_u32(hdr + 12, length);
    if (send_all(s, hdr, PCR_HEADER_LEN) != 0) return -1;
    if (length && send_all(s, (const unsigned char *)payload, length) != 0) return -1;
    return 0;
}

int pcr_recv(pcr_sock_t s, pcr_frame_t *frame, uint32_t deadline_ms)
{
    unsigned char hdr[PCR_HEADER_LEN];
    uint32_t len;
    int rc;

    memset(frame, 0, sizeof *frame);
    rc = recv_all(s, hdr, PCR_HEADER_LEN, deadline_ms);
    if (rc != 0) return rc;

    if (hdr[0] != PCR_MAGIC0 || hdr[1] != PCR_MAGIC1 ||
        hdr[2] != PCR_MAGIC2 || hdr[3] != PCR_MAGIC3) {
        return -2;
    }
    frame->type = hdr[4];
    frame->corr = get_u32(hdr + 8);
    len = get_u32(hdr + 12);
    if (len > PCR_MAX_PAYLOAD) return -2;

    frame->payload = malloc((size_t)len + 1);
    if (!frame->payload) return -1;
    if (len) {
        rc = recv_all(s, (unsigned char *)frame->payload, len, deadline_ms);
        if (rc != 0) { free(frame->payload); frame->payload = NULL; return rc; }
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
    char portstr[16];
    struct addrinfo hints, *res = NULL, *ai;
    pcr_sock_t s = PCR_BAD_SOCKET;
    int rc = -1;

    pcr_net_init();
    snprintf(portstr, sizeof portstr, "%u", (unsigned)port);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host && *host ? host : "127.0.0.1", portstr, &hints, &res) != 0) {
        pcr_net_shutdown();
        return -1;
    }
    for (ai = res; ai; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == PCR_BAD_SOCKET) continue;
        if (connect(s, ai->ai_addr, (int)ai->ai_addrlen) == 0) break;
        pcr_close(s);
        s = PCR_BAD_SOCKET;
    }
    freeaddrinfo(res);
    if (s == PCR_BAD_SOCKET) {
        pcr_net_shutdown();
        return -1;
    }

    if (pcr_send(s, type, 1, payload, length) == 0) {
        rc = pcr_recv(s, reply, deadline_ms ? deadline_ms : 5000);
    }
    pcr_close(s);
    pcr_net_shutdown();
    return rc;
}
