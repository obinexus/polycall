#ifndef POLYCALL_RPC_WIRE_H
#define POLYCALL_RPC_WIRE_H

/*
 * polycall_rpc v1 framing. Internal to the runtime; see docs/RPC.md.
 *
 *   magic  : 'P','C','R','1'          (4 bytes)
 *   type   : u8                        1 REQUEST 2 RESPONSE 3 CONTROL 4 REPLY
 *   flags  : u8                        (reserved, 0)
 *   rsvd   : u16                       (0)
 *   corr   : u32 big-endian           request/response correlation id
 *   length : u32 big-endian           payload byte count (<= RPC_MAX_PAYLOAD)
 *   payload: UTF-8 JSON, `length` bytes
 *
 * No C struct or pointer is ever placed on the wire; the payload is always
 * JSON text. Partial reads/writes are handled by the send/recv helpers.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601   /* Windows 7: getaddrinfo / inet_pton */
#  endif
#  ifndef WINVER
#    define WINVER 0x0601
#  endif
#  include <winsock2.h>
typedef SOCKET pcr_sock_t;
#  define PCR_BAD_SOCKET INVALID_SOCKET
#else
typedef int pcr_sock_t;
#  define PCR_BAD_SOCKET (-1)
#endif

#define PCR_MAGIC0 'P'
#define PCR_MAGIC1 'C'
#define PCR_MAGIC2 'R'
#define PCR_MAGIC3 '1'
#define PCR_HEADER_LEN 16
#define PCR_MAX_PAYLOAD (1u << 20)   /* 1 MiB */

enum {
    PCR_T_REQUEST  = 1,
    PCR_T_RESPONSE = 2,
    PCR_T_CONTROL  = 3,
    PCR_T_REPLY    = 4
};

typedef struct {
    uint8_t  type;
    uint32_t corr;
    char    *payload;   /* malloc'd, NUL-terminated; caller frees */
    uint32_t length;
} pcr_frame_t;

/* blocking, handles partial IO; 0 on success, -1 on error/EOF */
int pcr_send(pcr_sock_t s, uint8_t type, uint32_t corr,
             const char *payload, uint32_t length);

/* reads one frame; allocates frame->payload. 0 ok, -1 error/EOF,
 * -2 protocol violation (bad magic / oversize). `deadline_ms` 0 = block. */
int pcr_recv(pcr_sock_t s, pcr_frame_t *frame, uint32_t deadline_ms);

void pcr_frame_free(pcr_frame_t *frame);

/* one-shot client helper: connect host:port, send a frame, recv one frame.
 * Returns 0 on success (frame filled), -1 connect/io failure, -3 timeout. */
int pcr_roundtrip(const char *host, uint16_t port, uint8_t type,
                  const char *payload, uint32_t length,
                  pcr_frame_t *reply, uint32_t deadline_ms);

/* platform socket lifecycle (WSAStartup / cleanup). idempotent. */
void pcr_net_init(void);
void pcr_net_shutdown(void);
void pcr_close(pcr_sock_t s);

#endif /* POLYCALL_RPC_WIRE_H */
