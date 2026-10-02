#ifndef POLYCALL_RPC_WIRE_H
#define POLYCALL_RPC_WIRE_H

/*
 * polycall_rpc v1 framing. Internal to the runtime; see docs/RPC.md.
 *
 *   magic  : 'P','C','R','1'          (4 bytes)
 *   type   : u8                        1 REQUEST 2 RESPONSE 3 CONTROL 4 REPLY
 *   flags  : u8                        (reserved, must be 0)
 *   rsvd   : u16                       (must be 0)
 *   corr   : u32 big-endian           request/response correlation id
 *   length : u32 big-endian           payload byte count (<= RPC_MAX_PAYLOAD)
 *   payload: UTF-8 JSON, `length` bytes
 *
 * No C struct or pointer is ever placed on the wire; the payload is always
 * JSON text. Every operation here is bounded by a deadline (see pc_sys.h):
 * a peer that stalls or trickles bytes costs one deadline, never a thread.
 *
 * Return codes: 0 ok, -1 I/O error or EOF, -2 protocol violation (bad
 * magic / type / reserved bits / oversize), -3 deadline exceeded.
 */

#include <stddef.h>
#include <stdint.h>

#include "../core/pc_sys.h"

typedef pc_sock_t pcr_sock_t;
#define PCR_BAD_SOCKET PC_BAD_SOCK

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

/* send one frame within timeout_ms (0 = 5000 ms default) */
int pcr_send(pcr_sock_t s, uint8_t type, uint32_t corr,
             const char *payload, uint32_t length, uint32_t timeout_ms);

/* read one complete frame within timeout_ms (0 = no deadline: only for a
 * caller that already knows the frame is arriving). Allocates
 * frame->payload. */
int pcr_recv(pcr_sock_t s, pcr_frame_t *frame, uint32_t timeout_ms);

void pcr_frame_free(pcr_frame_t *frame);

/* one-shot client helper: connect host:port, send a frame, recv one
 * frame, all within deadline_ms (0 = 5000 ms). Returns 0 (frame filled),
 * -1 connect/io failure, -2 protocol, -3 timeout. */
int pcr_roundtrip(const char *host, uint16_t port, uint8_t type,
                  const char *payload, uint32_t length,
                  pcr_frame_t *reply, uint32_t deadline_ms);

/* platform socket lifecycle; idempotent. */
void pcr_net_init(void);
void pcr_net_shutdown(void);
void pcr_close(pcr_sock_t s);

#endif /* POLYCALL_RPC_WIRE_H */
