#ifndef POLYCALL_INTERNAL_PEER_HTTP_H
#define POLYCALL_INTERNAL_PEER_HTTP_H

/*
 * Minimal, strict HTTP/1.1 for the polycall-peer/1 protocol. Internal.
 *
 * Deliberately small: one request per connection ("Connection: close"),
 * bodies only with Content-Length (Transfer-Encoding is refused, so there
 * is no request-smuggling ambiguity), hard limits on header and body size,
 * and absolute deadlines on every read and write.
 */

#include <stddef.h>
#include <stdint.h>

#include "../core/pc_sys.h"

#define PC_HTTP_MAX_HEADER  (16u * 1024u)
#define PC_HTTP_MAX_BODY    (2u * 1024u * 1024u)

typedef struct {
    char   method[8];
    char   path[256];
    char   authorization[512];   /* raw header value, "" when absent  */
    long   content_length;       /* -1 when absent                   */
    char  *body;                 /* malloc'd, NUL-terminated, or NULL */
    size_t body_len;
} pc_http_req_t;

/* Read one request. Returns 0 when *req is complete, otherwise the HTTP
 * status to answer with: 400 malformed, 408 timeout, 411 length required,
 * 413 too large, 414 path too long, 431 headers too large, 501 transfer
 * encoding, 505 version; or -1 when the client vanished (send nothing). */
int  pc_http_read_request(pc_sock_t s, uint32_t header_timeout_ms,
                          uint32_t body_timeout_ms, size_t max_body,
                          pc_http_req_t *req);
void pc_http_req_free(pc_http_req_t *req);

const char *pc_http_reason(int status);

/* Write one complete response (Connection: close). extra_headers, when not
 * NULL, is zero or more "Name: value\r\n" lines. */
int  pc_http_respond(pc_sock_t s, int status, const char *content_type,
                     const char *extra_headers, const char *body, size_t len,
                     uint32_t timeout_ms);

/* One request/response exchange. Returns a POLYCALL_* status for the
 * transport (OK even for a 4xx/5xx answer -- see *http_status). The
 * response body (<= max_resp bytes) is malloc'd into *resp (caller frees),
 * NUL-terminated. The socket is always closed before returning. */
int  pc_http_exchange(const char *host, uint16_t port, const char *method,
                      const char *path, const char *bearer_token,
                      const char *content_type, const void *body,
                      size_t body_len, uint32_t timeout_ms, size_t max_resp,
                      int *http_status, char **resp, size_t *resp_len,
                      char *err, size_t errcap);

/* constant-time comparison for secrets */
int  pc_secret_equal(const char *a, const char *b);

#endif /* POLYCALL_INTERNAL_PEER_HTTP_H */
