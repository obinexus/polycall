#include "http.h"

#include "polycall.h"
#include "../core/pc_buf.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- small helpers ---------------------------------------------------- */

static int ci_eq_n(const char *a, const char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) return 0;
        if (a[i] == '\0') return 1;
    }
    return 1;
}

static const char *find_crlfcrlf(const char *buf, size_t len)
{
    size_t i;
    if (len < 4) return NULL;
    for (i = 0; i + 3 < len; ++i) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return buf + i;
        }
    }
    return NULL;
}

/* strict non-negative decimal (no sign, no spaces inside, <= max) */
static int parse_len(const char *s, size_t n, long max, long *out)
{
    long v = 0;
    size_t i;
    if (n == 0 || n > 10) return -1;
    for (i = 0; i < n; ++i) {
        if (s[i] < '0' || s[i] > '9') return -1;
        v = v * 10 + (s[i] - '0');
        if (v > max) return -2;
    }
    *out = v;
    return 0;
}

int pc_secret_equal(const char *a, const char *b)
{
    size_t la = a ? strlen(a) : 0, lb = b ? strlen(b) : 0, i;
    unsigned char diff = (unsigned char)(la != lb);
    size_t n = la > lb ? la : lb;
    for (i = 0; i < n; ++i) {
        unsigned char ca = i < la ? (unsigned char)a[i] : 0;
        unsigned char cb = i < lb ? (unsigned char)b[i] : 0;
        diff |= (unsigned char)(ca ^ cb);
    }
    return diff == 0;
}

const char *pc_http_reason(int status)
{
    switch (status) {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 411: return "Length Required";
    case 413: return "Payload Too Large";
    case 414: return "URI Too Long";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    case 505: return "HTTP Version Not Supported";
    default:  return "Status";
    }
}

/* ---- server side ------------------------------------------------------ */

void pc_http_req_free(pc_http_req_t *req)
{
    if (!req) return;
    free(req->body);
    req->body = NULL;
    req->body_len = 0;
}

int pc_http_read_request(pc_sock_t s, uint32_t header_timeout_ms,
                         uint32_t body_timeout_ms, size_t max_body,
                         pc_http_req_t *req)
{
    char *hdr = NULL;
    size_t have = 0;
    const char *end = NULL;
    uint64_t deadline = pc_deadline_after(header_timeout_ms);
    const char *line, *next, *hend;
    int status = 0;
    int seen_cl = 0;

    memset(req, 0, sizeof *req);
    req->content_length = -1;

    hdr = malloc(PC_HTTP_MAX_HEADER + 1);
    if (!hdr) return 500;

    /* read until the blank line, never past the header limit */
    while (!end) {
        size_t got = 0;
        int rc;
        if (have >= PC_HTTP_MAX_HEADER) { status = 431; goto out; }
        rc = pc_recv_some(s, hdr + have, PC_HTTP_MAX_HEADER - have, deadline, &got);
        if (rc == POLYCALL_E_TIMEOUT) { status = have ? 408 : -1; goto out; }
        if (rc != POLYCALL_OK || got == 0) { status = -1; goto out; }
        have += got;
        hdr[have] = '\0';
        end = find_crlfcrlf(hdr, have);
    }

    /* request line: METHOD SP PATH SP HTTP/1.x CRLF */
    hend = end + 2;                         /* points at the final CRLF */
    line = hdr;
    if (memchr(hdr, '\0', (size_t)(end - hdr)) != NULL) {
        status = 400;                       /* NUL bytes never belong here */
        goto out;
    }
    next = strstr(line, "\r\n");
    if (!next || next > end) { status = 400; goto out; }
    {
        const char *sp1 = memchr(line, ' ', (size_t)(next - line));
        const char *sp2 = sp1 ? memchr(sp1 + 1, ' ', (size_t)(next - sp1 - 1)) : NULL;
        size_t ml, pl;
        if (!sp1 || !sp2) { status = 400; goto out; }
        ml = (size_t)(sp1 - line);
        pl = (size_t)(sp2 - sp1 - 1);
        if (ml == 0 || ml >= sizeof req->method) { status = 400; goto out; }
        if (pl == 0) { status = 400; goto out; }
        if (pl >= sizeof req->path) { status = 414; goto out; }
        memcpy(req->method, line, ml);
        memcpy(req->path, sp1 + 1, pl);
        if (req->path[0] != '/') { status = 400; goto out; }
        if ((size_t)(next - sp2 - 1) != 8 ||
            (strncmp(sp2 + 1, "HTTP/1.1", 8) != 0 && strncmp(sp2 + 1, "HTTP/1.0", 8) != 0)) {
            status = 505;
            goto out;
        }
    }

    /* header fields */
    for (line = next + 2; line < hend; line = next + 2) {
        const char *colon, *v, *ve;
        size_t nl;
        next = strstr(line, "\r\n");
        if (!next || next > hend) break;
        if (next == line) break;
        if (*line == ' ' || *line == '\t') { status = 400; goto out; } /* obs-fold */
        colon = memchr(line, ':', (size_t)(next - line));
        if (!colon || colon == line) { status = 400; goto out; }
        nl = (size_t)(colon - line);
        v = colon + 1;
        while (v < next && (*v == ' ' || *v == '\t')) ++v;
        ve = next;
        while (ve > v && (ve[-1] == ' ' || ve[-1] == '\t')) --ve;

        if (nl == 14 && ci_eq_n(line, "content-length", 14)) {
            long cl;
            int pr = parse_len(v, (size_t)(ve - v), (long)PC_HTTP_MAX_BODY * 4, &cl);
            if (pr == -2) { status = 413; goto out; }
            if (pr != 0) { status = 400; goto out; }
            if (seen_cl && cl != req->content_length) { status = 400; goto out; }
            seen_cl = 1;
            req->content_length = cl;
        } else if (nl == 17 && ci_eq_n(line, "transfer-encoding", 17)) {
            status = 501;
            goto out;
        } else if (nl == 13 && ci_eq_n(line, "authorization", 13)) {
            size_t al = (size_t)(ve - v);
            if (al >= sizeof req->authorization) { status = 431; goto out; }
            memcpy(req->authorization, v, al);
            req->authorization[al] = '\0';
        }
    }

    if (strcmp(req->method, "POST") == 0 && req->content_length < 0) {
        status = 411;
        goto out;
    }
    if (req->content_length > 0) {
        size_t cl = (size_t)req->content_length;
        size_t already = have - (size_t)(end + 4 - hdr);
        uint64_t bdl = pc_deadline_after(body_timeout_ms);
        if (cl > max_body) { status = 413; goto out; }
        if (already > cl) { status = 400; goto out; }     /* pipelined extra */
        req->body = malloc(cl + 1);
        if (!req->body) { status = 500; goto out; }
        memcpy(req->body, end + 4, already);
        if (already < cl) {
            int rc = pc_recv_exact(s, req->body + already, cl - already, bdl);
            if (rc == POLYCALL_E_TIMEOUT) { status = 408; goto out; }
            if (rc != POLYCALL_OK) { status = -1; goto out; }
        }
        req->body[cl] = '\0';
        req->body_len = cl;
    }
    status = 0;

out:
    free(hdr);
    if (status != 0) pc_http_req_free(req);
    return status;
}

int pc_http_respond(pc_sock_t s, int status, const char *content_type,
                    const char *extra_headers, const char *body, size_t len,
                    uint32_t timeout_ms)
{
    pc_buf_t b;
    int rc;
    pc_buf_init(&b, 0);
    pc_buf_appendf(&b, "HTTP/1.1 %d %s\r\nServer: polycall-peer/1\r\n"
                       "Connection: close\r\nCache-Control: no-store\r\n",
                   status, pc_http_reason(status));
    if (len || content_type) {
        pc_buf_appendf(&b, "Content-Type: %s\r\n",
                       content_type ? content_type : "application/json");
    }
    pc_buf_appendf(&b, "Content-Length: %lu\r\n", (unsigned long)len);
    if (extra_headers) pc_buf_append(&b, extra_headers);
    pc_buf_append(&b, "\r\n");
    if (len) pc_buf_append_n(&b, body, len);
    if (pc_buf_failed(&b)) {
        pc_buf_free(&b);
        return POLYCALL_E_NO_MEMORY;
    }
    rc = pc_send_all(s, b.data, b.len, pc_deadline_after(timeout_ms));
    pc_buf_free(&b);
    return rc;
}

/* ---- client side ------------------------------------------------------ */

int pc_http_exchange(const char *host, uint16_t port, const char *method,
                     const char *path, const char *bearer_token,
                     const char *content_type, const void *body,
                     size_t body_len, uint32_t timeout_ms, size_t max_resp,
                     int *http_status, char **resp, size_t *resp_len,
                     char *err, size_t errcap)
{
    pc_sock_t s = PC_BAD_SOCK;
    pc_buf_t req, in;
    uint64_t deadline = pc_deadline_after(timeout_ms);
    const char *end;
    int rc;
    long clen = -1;

    *http_status = 0;
    *resp = NULL;
    *resp_len = 0;

    rc = pc_connect(host, port, pc_remaining_ms(deadline), &s, err, errcap);
    if (rc != POLYCALL_OK) return rc;

    pc_buf_init(&req, 0);
    pc_buf_appendf(&req, "%s %s HTTP/1.1\r\nHost: %s:%u\r\n"
                         "User-Agent: polycall-peer/1\r\nConnection: close\r\n"
                         "Accept: application/json\r\n",
                   method, path, host, (unsigned)port);
    if (bearer_token && *bearer_token) {
        pc_buf_appendf(&req, "Authorization: Bearer %s\r\n", bearer_token);
    }
    if (body || strcmp(method, "POST") == 0) {
        pc_buf_appendf(&req, "Content-Type: %s\r\nContent-Length: %lu\r\n",
                       content_type ? content_type : "application/json",
                       (unsigned long)body_len);
    }
    pc_buf_append(&req, "\r\n");
    if (body_len) pc_buf_append_n(&req, body, body_len);
    if (pc_buf_failed(&req)) {
        pc_buf_free(&req);
        pc_sock_close(s);
        return POLYCALL_E_NO_MEMORY;
    }
    rc = pc_send_all(s, req.data, req.len, deadline);
    pc_buf_free(&req);
    if (rc != POLYCALL_OK) {
        pc_sock_close(s);
        if (err && errcap) {
            snprintf(err, errcap, "%s while sending to %s:%u",
                     rc == POLYCALL_E_TIMEOUT ? "timed out" : "connection lost",
                     host, (unsigned)port);
        }
        return rc;
    }

    /* read the whole response (Connection: close), bounded */
    pc_buf_init(&in, PC_HTTP_MAX_HEADER + max_resp);
    for (;;) {
        char chunk[4096];
        size_t got = 0;
        rc = pc_recv_some(s, chunk, sizeof chunk, deadline, &got);
        if (rc != POLYCALL_OK) break;
        if (got == 0) break;                                   /* EOF */
        if (!pc_buf_append_n(&in, chunk, got)) { rc = POLYCALL_E_TOO_LARGE; break; }
        /* stop early once a Content-Length response is complete */
        if (clen < 0 && (end = find_crlfcrlf(in.data, in.len)) != NULL) {
            const char *p = in.data;
            while (p < end) {
                const char *eol = strstr(p, "\r\n");
                if (!eol) break;
                if ((size_t)(eol - p) > 15 && ci_eq_n(p, "content-length:", 15)) {
                    const char *v = p + 15;
                    while (*v == ' ' || *v == '\t') ++v;
                    clen = strtol(v, NULL, 10);
                }
                p = eol + 2;
            }
            if (clen < 0) clen = -2;   /* no length: read to EOF */
        }
        if (clen >= 0) {
            end = find_crlfcrlf(in.data, in.len);
            if (end && in.len - (size_t)(end + 4 - in.data) >= (size_t)clen) break;
        }
    }
    pc_sock_close(s);
    if (rc != POLYCALL_OK) {
        pc_buf_free(&in);
        if (err && errcap) {
            snprintf(err, errcap, "%s while reading the reply from %s:%u",
                     rc == POLYCALL_E_TIMEOUT ? "timed out"
                     : rc == POLYCALL_E_TOO_LARGE ? "reply too large" : "connection lost",
                     host, (unsigned)port);
        }
        return rc;
    }

    end = in.data ? find_crlfcrlf(in.data, in.len) : NULL;
    if (!end || in.len < 12 || strncmp(in.data, "HTTP/1.", 7) != 0) {
        pc_buf_free(&in);
        if (err && errcap) snprintf(err, errcap, "%s:%u did not answer with HTTP",
                                    host, (unsigned)port);
        return POLYCALL_E_PROTOCOL;
    }
    *http_status = atoi(in.data + 9);
    {
        size_t off = (size_t)(end + 4 - in.data);
        size_t blen = in.len - off;
        if (clen >= 0 && (size_t)clen < blen) blen = (size_t)clen;
        if (clen >= 0 && (size_t)clen > blen) {
            pc_buf_free(&in);
            if (err && errcap) snprintf(err, errcap, "truncated reply from %s:%u",
                                        host, (unsigned)port);
            return POLYCALL_E_PROTOCOL;
        }
        *resp = malloc(blen + 1);
        if (!*resp) { pc_buf_free(&in); return POLYCALL_E_NO_MEMORY; }
        memcpy(*resp, in.data + off, blen);
        (*resp)[blen] = '\0';
        *resp_len = blen;
    }
    pc_buf_free(&in);
    return POLYCALL_OK;
}
