/*
 * polycall-peer/1: an NSIGII-style peer node -- independent processes,
 * each with its OWN registry and inbox, exchanging payloads directly over
 * HTTP/1.1 with no central broker. See docs/PEER_PROTOCOL.md.
 *
 * Adapted from the NSIGII two-process reference (Go Alpha :9001 / Python
 * Beta :9002); every defect found in that reference has an explicit
 * counterpart here (docs/NSIGII_ADAPTATION.md maps them one by one):
 *   - the message body is actually sent (reference: nil request body)
 *   - every client socket/response is closed on every path
 *   - no library-allocated memory is handed to callers (no CString leak,
 *     no lost pointer in ctypes): all outputs go to caller buffers
 *   - the registry is only touched under the node lock (incl. /health)
 *   - malformed JSON / base64 / ids are rejected with 400, never ignored
 *   - methods, paths, sizes and read deadlines are validated
 *   - the sender identity is this node's own id, never a hard-coded one
 *   - delivery is acknowledged by the receiver under its id and the
 *     message id, so "200" alone never counts as delivered
 *   - receiving never auto-registers the sender (no registry poisoning)
 */

#include "polycall.h"
#include "polycall_telemetry.h"

#include "http.h"
#include "../core/pc_buf.h"
#include "../core/pc_sys.h"
#include "../core/status.h"
#include "../config/json.h"
#include "../config/cfgfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PEER_MAX_NODES       255
#define PEER_MAX_REGISTRY    64
#define PEER_MAX_WORKERS     32
#define PEER_INBOX_DEFAULT   256
#define PEER_INBOX_BYTES     (64u * 1024u * 1024u)
#define PEER_DEDUP           1024
#define PEER_HEADER_TIMEOUT  5000
#define PEER_BODY_TIMEOUT    10000
#define PEER_WRITE_TIMEOUT   5000
#define PEER_WAIT_MAX_MS     30000
#define PEER_RESP_MAX        (64u * 1024u)
#define PEER_BODY_MAX        (POLYCALL_PEER_MAX_PAYLOAD / 3 * 4 + 8192)

typedef struct peer_msg {
    char from[POLYCALL_PEER_ID_MAX];
    char id[POLYCALL_MESSAGE_ID_MAX];
    unsigned char *data;
    size_t len;
    struct peer_msg *next;
} peer_msg;

typedef struct {
    char id[POLYCALL_PEER_ID_MAX];
    char endpoint[POLYCALL_ENDPOINT_MAX];
} peer_entry;

typedef struct peer_node peer_node;

typedef struct {
    peer_node *node;
    pc_sock_t sock;
    pc_thread_t thread;
    int in_use;     /* slot owns a thread that must be joined */
    int done;       /* thread finished; safe to join */
} worker_slot;

struct peer_node {
    pc_mutex_t mu;
    pc_cond_t inbox_cv;
    pc_cond_t worker_cv;

    char node_id[POLYCALL_PEER_ID_MAX];
    char endpoint[POLYCALL_ENDPOINT_MAX];
    char token[256];

    int has_listener;
    pc_sock_t ls;
    pc_thread_t accept_thread;
    volatile int stopping;      /* also read lock-free by worker IO waits */

    peer_entry registry[PEER_MAX_REGISTRY];
    int nregistry;

    peer_msg *head, *tail;
    size_t inbox_count, inbox_cap, inbox_bytes;

    char dedup[PEER_DEDUP][POLYCALL_PEER_ID_MAX + POLYCALL_MESSAGE_ID_MAX];
    size_t dedup_next, dedup_fill;

    unsigned cancel_gen;
    worker_slot workers[PEER_MAX_WORKERS];

    uint64_t started_ms;
    uint64_t received, duplicates, rejected_busy, rejected_bad, rejected_auth;
    uint64_t sent_ok, sent_failed;
};

/* ===================================================================== */
/* handle table                                                          */
/* ===================================================================== */

typedef struct {
    peer_node *node;
    uint32_t gen;
    int refs;
    int closing;
} handle_slot;

static pc_once_t g_tab_once = PC_ONCE_INIT;
static pc_mutex_t g_tab_mu;
static pc_cond_t g_tab_cv;
static handle_slot g_tab[PEER_MAX_NODES];

static void tab_init(void)
{
    pc_mutex_init(&g_tab_mu);
    pc_cond_init(&g_tab_cv);
}

static int32_t make_handle(int slot, uint32_t gen)
{
    return (int32_t)(((gen & 0x7FFFFFu) << 8) | (uint32_t)(slot + 1));
}

/* resolve + pin a handle; NULL with pc_err set when invalid */
static peer_node *acquire(polycall_peer_t h, int *slot_out)
{
    int slot;
    uint32_t gen;
    peer_node *n = NULL;
    pc_once(&g_tab_once, tab_init);
    if (h <= 0) {
        pc_err(POLYCALL_E_INVALID_HANDLE, "peer handle %ld is not valid", (long)h);
        return NULL;
    }
    slot = (int)(h & 0xFF) - 1;
    gen = ((uint32_t)h >> 8) & 0x7FFFFFu;
    if (slot < 0 || slot >= PEER_MAX_NODES) {
        pc_err(POLYCALL_E_INVALID_HANDLE, "peer handle %ld is not valid", (long)h);
        return NULL;
    }
    pc_mutex_lock(&g_tab_mu);
    if (g_tab[slot].node && g_tab[slot].gen == gen && !g_tab[slot].closing) {
        n = g_tab[slot].node;
        g_tab[slot].refs++;
    }
    pc_mutex_unlock(&g_tab_mu);
    if (!n) {
        pc_err(POLYCALL_E_INVALID_HANDLE,
               "peer handle %ld is unknown, closed or stale", (long)h);
        return NULL;
    }
    if (slot_out) *slot_out = slot;
    return n;
}

static void release(int slot)
{
    pc_mutex_lock(&g_tab_mu);
    g_tab[slot].refs--;
    pc_cond_broadcast(&g_tab_cv);
    pc_mutex_unlock(&g_tab_mu);
}

/* ===================================================================== */
/* node internals (caller holds n->mu unless noted)                      */
/* ===================================================================== */

static int registry_find(const peer_node *n, const char *id)
{
    int i;
    for (i = 0; i < n->nregistry; ++i) {
        if (strcmp(n->registry[i].id, id) == 0) return i;
    }
    return -1;
}

static int registry_put(peer_node *n, const char *id, const char *endpoint)
{
    int i = registry_find(n, id);
    if (i < 0) {
        if (n->nregistry >= PEER_MAX_REGISTRY) return POLYCALL_E_TOO_LARGE;
        i = n->nregistry++;
        pc_copy_out(n->registry[i].id, sizeof n->registry[i].id, id);
    }
    pc_copy_out(n->registry[i].endpoint, sizeof n->registry[i].endpoint, endpoint);
    return POLYCALL_OK;
}

static void dedup_key(char *out, size_t cap, const char *from, const char *id)
{
    snprintf(out, cap, "%s\x1f%s", from, id);
}

static int dedup_seen(const peer_node *n, const char *key)
{
    size_t i;
    for (i = 0; i < n->dedup_fill; ++i) {
        if (strcmp(n->dedup[i], key) == 0) return 1;
    }
    return 0;
}

static void dedup_add(peer_node *n, const char *key)
{
    pc_copy_out(n->dedup[n->dedup_next], sizeof n->dedup[0], key);
    n->dedup_next = (n->dedup_next + 1) % PEER_DEDUP;
    if (n->dedup_fill < PEER_DEDUP) n->dedup_fill++;
}

static void health_json(peer_node *n, pc_buf_t *b, int with_registry)
{
    char ver[32];
    int i;
    polycall_ffi_version(ver, (int)sizeof ver);
    pc_buf_append(b, "{\"ok\":true,\"protocol\":\"polycall-peer/1\",\"node_id\":");
    pc_buf_json_string(b, n->node_id);
    pc_buf_append(b, ",\"endpoint\":");
    pc_buf_json_string(b, n->endpoint);
    pc_buf_appendf(b,
        ",\"status\":\"%s\",\"peers\":%d,\"inbox\":%lu,\"inbox_capacity\":%lu,"
        "\"received\":%llu,\"duplicates\":%llu,\"rejected_busy\":%llu,"
        "\"rejected_malformed\":%llu,\"rejected_auth\":%llu,"
        "\"sent_ok\":%llu,\"sent_failed\":%llu,\"uptime_ms\":%llu,"
        "\"auth\":%s,\"implementation\":\"libpolycall %s\"",
        n->stopping ? "stopping" : "ok", n->nregistry,
        (unsigned long)n->inbox_count, (unsigned long)n->inbox_cap,
        (unsigned long long)n->received, (unsigned long long)n->duplicates,
        (unsigned long long)n->rejected_busy, (unsigned long long)n->rejected_bad,
        (unsigned long long)n->rejected_auth,
        (unsigned long long)n->sent_ok, (unsigned long long)n->sent_failed,
        (unsigned long long)(pc_mono_ms() - n->started_ms),
        n->token[0] ? "true" : "false", ver);
    if (with_registry) {
        pc_buf_append(b, ",\"registry\":{");
        for (i = 0; i < n->nregistry; ++i) {
            if (i) pc_buf_append(b, ",");
            pc_buf_json_string(b, n->registry[i].id);
            pc_buf_append(b, ":");
            pc_buf_json_string(b, n->registry[i].endpoint);
        }
        pc_buf_append(b, "}");
    }
    pc_buf_append(b, "}");
}

static void registry_json(peer_node *n, pc_buf_t *b)
{
    int i;
    pc_buf_append(b, "{");
    for (i = 0; i < n->nregistry; ++i) {
        if (i) pc_buf_append(b, ",");
        pc_buf_json_string(b, n->registry[i].id);
        pc_buf_append(b, ":");
        pc_buf_json_string(b, n->registry[i].endpoint);
    }
    pc_buf_append(b, "}");
}

/* ===================================================================== */
/* HTTP server side                                                      */
/* ===================================================================== */

static void respond_json(pc_sock_t s, int status, const char *extra, pc_buf_t *b)
{
    pc_http_respond(s, status, "application/json", extra,
                    b->data ? b->data : "", b->len, PEER_WRITE_TIMEOUT);
}

static void respond_error(pc_sock_t s, int status, const char *code,
                          const char *message, const char *extra)
{
    pc_buf_t b;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"ok\":false,\"error\":{\"code\":");
    pc_buf_json_string(&b, code);
    pc_buf_append(&b, ",\"message\":");
    pc_buf_json_string(&b, message);
    pc_buf_append(&b, "}}");
    respond_json(s, status, extra, &b);
    pc_buf_free(&b);
}

static int authorized(peer_node *n, const pc_http_req_t *req)
{
    if (!n->token[0]) return 1;
    if (strncmp(req->authorization, "Bearer ", 7) != 0) return 0;
    return pc_secret_equal(req->authorization + 7, n->token);
}

static int valid_id(const char *s)
{
    return polycall_cfg_valid_peer_id(s);
}

static void handle_receive(peer_node *n, pc_sock_t s, const pc_http_req_t *req)
{
    char jerr[64];
    json_value *j;
    const char *from, *id, *b64;
    double v;
    bool ok_v = false;
    unsigned char *data = NULL;
    size_t dlen = 0;
    char key[POLYCALL_PEER_ID_MAX + POLYCALL_MESSAGE_ID_MAX];
    peer_msg *m;
    int dup = 0;
    pc_buf_t b;

    j = json_parse(req->body ? req->body : "", req->body_len, jerr, sizeof jerr);
    if (!j || j->type != JSON_OBJECT) {
        json_free(j);
        pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
        respond_error(s, 400, "request.malformed", "body is not a JSON object", NULL);
        return;
    }
    v = json_num(json_get(j, "v"), -1, &ok_v);
    from = json_str(json_get(j, "from"), NULL, NULL);
    id = json_str(json_get(j, "id"), NULL, NULL);
    b64 = json_str(json_get(j, "payload_b64"), NULL, NULL);
    if (!ok_v || v != 1) {
        json_free(j);
        pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
        respond_error(s, 400, "protocol.version", "expected \"v\":1", NULL);
        return;
    }
    if (!from || !valid_id(from) || !id || !valid_id(id) || !b64) {
        json_free(j);
        pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
        respond_error(s, 400, "request.malformed",
                      "need string from, id ([A-Za-z0-9._-]{1,63}) and payload_b64", NULL);
        return;
    }
    {
        int dr = pc_base64_decode(b64, strlen(b64), &data, &dlen);
        if (dr == -2) {
            json_free(j);
            respond_error(s, 500, "internal", "out of memory", NULL);
            return;
        }
        if (dr != 0) {
            json_free(j);
            pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
            respond_error(s, 400, "request.malformed", "payload_b64 is not valid base64", NULL);
            return;
        }
    }
    if (dlen > POLYCALL_PEER_MAX_PAYLOAD) {
        free(data);
        json_free(j);
        pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
        respond_error(s, 413, "payload.too_large", "payload exceeds 1 MiB", NULL);
        return;
    }

    dedup_key(key, sizeof key, from, id);
    pc_mutex_lock(&n->mu);
    if (dedup_seen(n, key)) {
        dup = 1;
        n->duplicates++;
    } else if (n->inbox_count >= n->inbox_cap ||
               n->inbox_bytes + dlen > PEER_INBOX_BYTES) {
        n->rejected_busy++;
        pc_mutex_unlock(&n->mu);
        free(data);
        json_free(j);
        respond_error(s, 503, "inbox.full",
                      "receiver inbox is full; retry later with the same id",
                      "Retry-After: 1\r\n");
        return;
    } else {
        m = calloc(1, sizeof *m);
        if (!m) {
            pc_mutex_unlock(&n->mu);
            free(data);
            json_free(j);
            respond_error(s, 500, "internal", "out of memory", NULL);
            return;
        }
        pc_copy_out(m->from, sizeof m->from, from);
        pc_copy_out(m->id, sizeof m->id, id);
        m->data = data;
        m->len = dlen;
        data = NULL;
        if (n->tail) n->tail->next = m; else n->head = m;
        n->tail = m;
        n->inbox_count++;
        n->inbox_bytes += dlen;
        n->received++;
        dedup_add(n, key);
        pc_cond_broadcast(&n->inbox_cv);
    }
    pc_mutex_unlock(&n->mu);
    free(data);

    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"ok\":true,\"status\":\"received\",\"node_id\":");
    pc_buf_json_string(&b, n->node_id);
    pc_buf_append(&b, ",\"id\":");
    pc_buf_json_string(&b, id);
    pc_buf_appendf(&b, ",\"duplicate\":%s}", dup ? "true" : "false");
    json_free(j);
    respond_json(s, 200, NULL, &b);
    pc_buf_free(&b);
}

static void handle_register(peer_node *n, pc_sock_t s, const pc_http_req_t *req)
{
    char jerr[64];
    json_value *j = json_parse(req->body ? req->body : "", req->body_len, jerr, sizeof jerr);
    const char *id = j ? json_str(json_get(j, "node_id"), NULL, NULL) : NULL;
    const char *ep = j ? json_str(json_get(j, "endpoint"), NULL, NULL) : NULL;
    int rc;
    pc_buf_t b;
    if (!id || !valid_id(id) || !ep || !polycall_cfg_valid_endpoint(ep, false)) {
        json_free(j);
        respond_error(s, 400, "request.malformed",
                      "need node_id ([A-Za-z0-9._-]{1,63}) and endpoint host:port", NULL);
        return;
    }
    pc_mutex_lock(&n->mu);
    rc = registry_put(n, id, ep);
    pc_mutex_unlock(&n->mu);
    if (rc != POLYCALL_OK) {
        json_free(j);
        respond_error(s, 409, "registry.full", "peer registry is full", NULL);
        return;
    }
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"ok\":true,\"registered\":");
    pc_buf_json_string(&b, id);
    pc_buf_append(&b, "}");
    json_free(j);
    respond_json(s, 200, NULL, &b);
    pc_buf_free(&b);
}

static int take_message(peer_node *n, uint32_t timeout_ms, unsigned gen0,
                        peer_msg **out);

static void handle_inbox_next(peer_node *n, pc_sock_t s, const pc_http_req_t *req)
{
    char jerr[64];
    json_value *j = json_parse(req->body ? req->body : "{}", req->body ? req->body_len : 2,
                               jerr, sizeof jerr);
    double t = j ? json_num(json_get(j, "timeout_ms"), 0, NULL) : -1;
    peer_msg *m = NULL;
    unsigned gen0;
    int rc;
    pc_buf_t b;
    json_free(j);
    if (t < 0 || t > PEER_WAIT_MAX_MS) {
        respond_error(s, 400, "request.malformed",
                      "timeout_ms must be 0..30000", NULL);
        return;
    }
    pc_mutex_lock(&n->mu);
    gen0 = n->cancel_gen;
    rc = take_message(n, (uint32_t)t, gen0, &m);
    pc_mutex_unlock(&n->mu);
    pc_buf_init(&b, 0);
    if (rc == POLYCALL_OK && m) {
        pc_buf_append(&b, "{\"ok\":true,\"message\":{\"from\":");
        pc_buf_json_string(&b, m->from);
        pc_buf_append(&b, ",\"id\":");
        pc_buf_json_string(&b, m->id);
        pc_buf_append(&b, ",\"payload_b64\":\"");
        pc_buf_base64(&b, m->data, m->len);
        pc_buf_append(&b, "\"}}");
        free(m->data);
        free(m);
    } else {
        pc_buf_append(&b, "{\"ok\":true,\"message\":null}");
    }
    respond_json(s, 200, NULL, &b);
    pc_buf_free(&b);
}

static void serve_one(peer_node *n, pc_sock_t s)
{
    pc_http_req_t req;
    int rs = pc_http_read_request(s, PEER_HEADER_TIMEOUT, PEER_BODY_TIMEOUT,
                                  PEER_BODY_MAX, &req);
    const char *p;
    if (rs != 0) {
        if (rs > 0) {
            if (rs == 413) {
                pc_mutex_lock(&n->mu); n->rejected_bad++; pc_mutex_unlock(&n->mu);
            }
            respond_error(s, rs, "http.rejected", pc_http_reason(rs), NULL);
        }
        return;
    }
    p = req.path;

    if (strcmp(p, "/health") == 0) {
        if (strcmp(req.method, "GET") != 0) {
            respond_error(s, 405, "http.method", "use GET", "Allow: GET\r\n");
        } else {
            pc_buf_t b;
            pc_buf_init(&b, 0);
            pc_mutex_lock(&n->mu);
            health_json(n, &b, 0);
            pc_mutex_unlock(&n->mu);
            respond_json(s, 200, NULL, &b);
            pc_buf_free(&b);
        }
        pc_http_req_free(&req);
        return;
    }

    if (strcmp(p, "/peers") != 0 && strcmp(p, "/receive") != 0 &&
        strcmp(p, "/register") != 0 && strcmp(p, "/inbox/next") != 0) {
        respond_error(s, 404, "http.not_found", "unknown path", NULL);
        pc_http_req_free(&req);
        return;
    }
    if (!authorized(n, &req)) {
        pc_mutex_lock(&n->mu); n->rejected_auth++; pc_mutex_unlock(&n->mu);
        respond_error(s, 401, req.authorization[0] ? "auth.denied" : "auth.required",
                      "this node requires its shared token (Authorization: Bearer)",
                      "WWW-Authenticate: Bearer\r\n");
        pc_http_req_free(&req);
        return;
    }
    if (strcmp(p, "/peers") == 0) {
        if (strcmp(req.method, "GET") != 0) {
            respond_error(s, 405, "http.method", "use GET", "Allow: GET\r\n");
        } else {
            pc_buf_t b;
            pc_buf_init(&b, 0);
            pc_buf_append(&b, "{\"ok\":true,\"node_id\":");
            pc_buf_json_string(&b, n->node_id);
            pc_buf_append(&b, ",\"peers\":");
            pc_mutex_lock(&n->mu);
            registry_json(n, &b);
            pc_mutex_unlock(&n->mu);
            pc_buf_append(&b, "}");
            respond_json(s, 200, NULL, &b);
            pc_buf_free(&b);
        }
    } else if (strcmp(req.method, "POST") != 0) {
        respond_error(s, 405, "http.method", "use POST", "Allow: POST\r\n");
    } else if (strcmp(p, "/receive") == 0) {
        handle_receive(n, s, &req);
    } else if (strcmp(p, "/register") == 0) {
        handle_register(n, s, &req);
    } else {
        handle_inbox_next(n, s, &req);
    }
    pc_http_req_free(&req);
}

static void worker_main(void *arg)
{
    worker_slot *w = (worker_slot *)arg;
    peer_node *n = w->node;
    /* every socket wait in this worker gives up promptly on close */
    pc_set_thread_abort(&n->stopping);
    serve_one(n, w->sock);
    pc_sock_finish(w->sock, 200);   /* e.g. a 413 sent before the body was read */
    pc_set_thread_abort(NULL);
    /* close and retire the socket under the lock: polycall_peer_close()
     * shuts down still-open worker sockets, and must never touch a
     * descriptor number this thread already released for reuse */
    pc_mutex_lock(&n->mu);
    pc_sock_close(w->sock);
    w->sock = PC_BAD_SOCK;
    w->done = 1;
    pc_cond_broadcast(&n->worker_cv);
    pc_mutex_unlock(&n->mu);
}

/* join finished workers; caller holds n->mu. Returns a free slot or -1. */
static int reap_workers(peer_node *n)
{
    int i, free_slot = -1;
    for (i = 0; i < PEER_MAX_WORKERS; ++i) {
        worker_slot *w = &n->workers[i];
        if (w->in_use && w->done) {
            pc_thread_join(w->thread);   /* already finished: returns at once */
            w->in_use = 0;
            w->done = 0;
        }
        if (!w->in_use && free_slot < 0) free_slot = i;
    }
    return free_slot;
}

static void accept_main(void *arg)
{
    peer_node *n = (peer_node *)arg;
    for (;;) {
        pc_sock_t c;
        int rc, slot;
        pc_mutex_lock(&n->mu);
        if (n->stopping) { pc_mutex_unlock(&n->mu); break; }
        pc_mutex_unlock(&n->mu);

        rc = pc_accept(n->ls, 200, &c);
        if (rc == POLYCALL_E_TIMEOUT) continue;
        if (rc != POLYCALL_OK) { pc_sleep_ms(50); continue; }

        pc_mutex_lock(&n->mu);
        slot = n->stopping ? -1 : reap_workers(n);
        if (slot < 0) {
            int stopping = n->stopping;
            n->rejected_busy++;
            pc_mutex_unlock(&n->mu);
            /* bounded backpressure: answer at once instead of queueing */
            respond_error(c, 503, stopping ? "node.stopping" : "server.busy",
                          stopping ? "node is shutting down"
                                   : "too many concurrent connections; retry later",
                          "Retry-After: 1\r\n");
            pc_sock_finish(c, 200);   /* the client reads the 503: no RST */
            pc_sock_close(c);
            continue;
        }
        n->workers[slot].node = n;
        n->workers[slot].sock = c;
        n->workers[slot].done = 0;
        n->workers[slot].in_use = 1;
        if (pc_thread_start(&n->workers[slot].thread, worker_main, &n->workers[slot]) != 0) {
            n->workers[slot].in_use = 0;
            pc_mutex_unlock(&n->mu);
            pc_sock_close(c);
            continue;
        }
        pc_mutex_unlock(&n->mu);
    }
}

/* caller holds n->mu */
static int take_message(peer_node *n, uint32_t timeout_ms, unsigned gen0,
                        peer_msg **out)
{
    uint64_t deadline = pc_deadline_after(timeout_ms);
    *out = NULL;
    for (;;) {
        if (n->stopping) return POLYCALL_E_CLOSED;
        if (n->cancel_gen != gen0) return POLYCALL_E_CANCELLED;
        if (n->head) {
            peer_msg *m = n->head;
            n->head = m->next;
            if (!n->head) n->tail = NULL;
            n->inbox_count--;
            n->inbox_bytes -= m->len;
            m->next = NULL;
            *out = m;
            return POLYCALL_OK;
        }
        if (timeout_ms == 0 || pc_remaining_ms(deadline) == 0) return POLYCALL_E_TIMEOUT;
        if (timeout_ms == UINT32_MAX) {
            pc_cond_wait(&n->inbox_cv, &n->mu);
        } else {
            pc_cond_timedwait(&n->inbox_cv, &n->mu, pc_remaining_ms(deadline));
        }
    }
}

/* ===================================================================== */
/* public API                                                            */
/* ===================================================================== */

static int is_loopback_host(const char *h)
{
    return strcmp(h, "127.0.0.1") == 0 || strcmp(h, "localhost") == 0 ||
           strncmp(h, "127.", 4) == 0;
}

int polycall_peer_open(const char *node_id, const char *bind_endpoint,
                       const char *auth_token, polycall_peer_t *out_handle)
{
    peer_node *n;
    int slot = -1, i, rc;
    char host[96];          /* "host:port" of the bound socket must fit */
    uint16_t port = 0, bound = 0;
    char err[200];

    pc_err_clear();
    pc_once(&g_tab_once, tab_init);
    if (out_handle) *out_handle = 0;
    if (!out_handle) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "out_handle is NULL");
    if (!node_id || !valid_id(node_id)) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT,
                      "node_id must be 1-63 characters of [A-Za-z0-9._-]");
    }
    if (auth_token && strlen(auth_token) >= 256) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "auth_token longer than 255 bytes");
    }
    if (bind_endpoint) {
        if (pc_split_endpoint(bind_endpoint, host, sizeof host, &port, 1) != POLYCALL_OK) {
            return pc_err(POLYCALL_E_INVALID_ARGUMENT,
                          "bind_endpoint '%s' is not host:port", bind_endpoint);
        }
        if (!is_loopback_host(host) && !(auth_token && *auth_token)) {
            return pc_err(POLYCALL_E_CONFIG,
                          "refusing to listen on non-loopback '%s' without an auth token",
                          host);
        }
    }

    n = calloc(1, sizeof *n);
    if (!n) return pc_err(POLYCALL_E_NO_MEMORY, "out of memory");
    pc_mutex_init(&n->mu);
    pc_cond_init(&n->inbox_cv);
    pc_cond_init(&n->worker_cv);
    pc_copy_out(n->node_id, sizeof n->node_id, node_id);
    if (auth_token) pc_copy_out(n->token, sizeof n->token, auth_token);
    n->inbox_cap = PEER_INBOX_DEFAULT;
    n->started_ms = pc_mono_ms();
    n->ls = PC_BAD_SOCK;

    if (bind_endpoint) {
        rc = pc_listen(host, port, 64, &n->ls, &bound, err, sizeof err);
        if (rc != POLYCALL_OK) {
            pc_cond_destroy(&n->worker_cv);
            pc_cond_destroy(&n->inbox_cv);
            pc_mutex_destroy(&n->mu);
            free(n);
            return pc_err(rc, "cannot listen on %s: %s", bind_endpoint, err);
        }
        snprintf(n->endpoint, sizeof n->endpoint, "%s:%u", host, (unsigned)bound);
        n->has_listener = 1;
    }

    pc_mutex_lock(&g_tab_mu);
    for (i = 0; i < PEER_MAX_NODES; ++i) {
        if (!g_tab[i].node) { slot = i; break; }
    }
    if (slot < 0) {
        pc_mutex_unlock(&g_tab_mu);
        pc_sock_close(n->ls);
        pc_cond_destroy(&n->worker_cv);
        pc_cond_destroy(&n->inbox_cv);
        pc_mutex_destroy(&n->mu);
        free(n);
        return pc_err(POLYCALL_E_BUSY, "too many open peer nodes (max %d)", PEER_MAX_NODES);
    }
    g_tab[slot].node = n;
    g_tab[slot].gen = (g_tab[slot].gen + 1) & 0x7FFFFFu;
    if (g_tab[slot].gen == 0) g_tab[slot].gen = 1;
    g_tab[slot].refs = 0;
    g_tab[slot].closing = 0;
    *out_handle = make_handle(slot, g_tab[slot].gen);
    pc_mutex_unlock(&g_tab_mu);

    if (n->has_listener &&
        pc_thread_start(&n->accept_thread, accept_main, n) != 0) {
        polycall_peer_close(*out_handle);
        *out_handle = 0;
        return pc_err(POLYCALL_E_INTERNAL, "cannot start the listener thread");
    }
    return POLYCALL_OK;
}

int polycall_peer_close(polycall_peer_t h)
{
    int slot, i;
    peer_node *n;
    pc_err_clear();
    n = acquire(h, &slot);
    if (!n) return POLYCALL_E_INVALID_HANDLE;

    pc_mutex_lock(&g_tab_mu);
    if (g_tab[slot].closing) {          /* lost a race with another close */
        g_tab[slot].refs--;
        pc_cond_broadcast(&g_tab_cv);
        pc_mutex_unlock(&g_tab_mu);
        return pc_err(POLYCALL_E_INVALID_HANDLE, "peer handle is already closing");
    }
    g_tab[slot].closing = 1;            /* no new acquire() succeeds */
    pc_mutex_unlock(&g_tab_mu);

    /* wake everything blocked on this node */
    pc_mutex_lock(&n->mu);
    pc_atomic_store(&n->stopping, 1);
    pc_cond_broadcast(&n->inbox_cv);
    pc_mutex_unlock(&n->mu);

    /* wait until we hold the only reference (blocked recv calls return) */
    pc_mutex_lock(&g_tab_mu);
    while (g_tab[slot].refs > 1) {
        pc_cond_timedwait(&g_tab_cv, &g_tab_mu, 100);
    }
    pc_mutex_unlock(&g_tab_mu);

    if (n->has_listener) {
        pc_thread_join(n->accept_thread);   /* exits within one accept poll */
        pc_sock_close(n->ls);
    }
    /* every worker is bounded by the request deadlines; join them all */
    pc_mutex_lock(&n->mu);
    for (i = 0; i < PEER_MAX_WORKERS; ++i) {
        worker_slot *w = &n->workers[i];
        if (!w->in_use) continue;
        if (!w->done && w->sock != PC_BAD_SOCK) {
            pc_sock_shutdown(w->sock);             /* unblock its IO now */
        }
        while (!w->done) pc_cond_wait(&n->worker_cv, &n->mu);
        pc_mutex_unlock(&n->mu);
        pc_thread_join(w->thread);
        pc_mutex_lock(&n->mu);
        w->in_use = 0;
    }
    while (n->head) {
        peer_msg *m = n->head;
        n->head = m->next;
        free(m->data);
        free(m);
    }
    pc_mutex_unlock(&n->mu);

    pc_cond_destroy(&n->worker_cv);
    pc_cond_destroy(&n->inbox_cv);
    pc_mutex_destroy(&n->mu);
    free(n);

    pc_mutex_lock(&g_tab_mu);
    g_tab[slot].node = NULL;
    g_tab[slot].refs = 0;
    g_tab[slot].closing = 0;
    pc_mutex_unlock(&g_tab_mu);
    return POLYCALL_OK;
}

static int finish_text(pc_buf_t *b, char *buf, size_t cap, size_t *out_len)
{
    if (pc_buf_failed(b)) {
        pc_buf_free(b);
        return pc_err(POLYCALL_E_NO_MEMORY, "out of memory");
    }
    if (out_len) *out_len = b->len;
    if (!buf || cap <= b->len) {
        size_t need = b->len;
        if (buf && cap) buf[0] = '\0';
        pc_buf_free(b);
        return pc_err(POLYCALL_E_TOO_LARGE, "buffer too small: need %lu bytes plus NUL",
                      (unsigned long)need);
    }
    memcpy(buf, b->data ? b->data : "", b->len + 1);
    pc_buf_free(b);
    return POLYCALL_OK;
}

int polycall_peer_endpoint(polycall_peer_t h, char *buf, size_t cap)
{
    int slot, rc;
    peer_node *n;
    pc_buf_t b;
    pc_err_clear();
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, n->endpoint);
    rc = finish_text(&b, buf, cap, NULL);
    release(slot);
    return rc;
}

int polycall_peer_node_id(polycall_peer_t h, char *buf, size_t cap)
{
    int slot, rc;
    peer_node *n;
    pc_buf_t b;
    pc_err_clear();
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, n->node_id);
    rc = finish_text(&b, buf, cap, NULL);
    release(slot);
    return rc;
}

int polycall_peer_register(polycall_peer_t h, const char *peer_id,
                           const char *endpoint)
{
    int slot, rc;
    peer_node *n;
    pc_err_clear();
    if (!peer_id || !valid_id(peer_id)) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT,
                      "peer_id must be 1-63 characters of [A-Za-z0-9._-]");
    }
    if (!endpoint || !polycall_cfg_valid_endpoint(endpoint, false)) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "endpoint must be host:port (port 1-65535)");
    }
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_mutex_lock(&n->mu);
    rc = registry_put(n, peer_id, endpoint);
    pc_mutex_unlock(&n->mu);
    release(slot);
    if (rc != POLYCALL_OK) {
        return pc_err(rc, "peer registry is full (max %d)", PEER_MAX_REGISTRY);
    }
    return POLYCALL_OK;
}

int polycall_peer_unregister(polycall_peer_t h, const char *peer_id)
{
    int slot, i, rc = POLYCALL_OK;
    peer_node *n;
    pc_err_clear();
    if (!peer_id || !*peer_id) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "peer_id is empty");
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_mutex_lock(&n->mu);
    i = registry_find(n, peer_id);
    if (i < 0) {
        rc = POLYCALL_E_NOT_FOUND;
    } else {
        n->registry[i] = n->registry[n->nregistry - 1];
        n->nregistry--;
    }
    pc_mutex_unlock(&n->mu);
    release(slot);
    if (rc != POLYCALL_OK) return pc_err(rc, "peer '%s' is not registered", peer_id);
    return POLYCALL_OK;
}

int polycall_peer_list(polycall_peer_t h, char *buf, size_t cap, size_t *out_len)
{
    int slot, rc;
    peer_node *n;
    pc_buf_t b;
    pc_err_clear();
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_buf_init(&b, 0);
    pc_mutex_lock(&n->mu);
    registry_json(n, &b);
    pc_mutex_unlock(&n->mu);
    rc = finish_text(&b, buf, cap, out_len);
    release(slot);
    return rc;
}

int polycall_peer_health(polycall_peer_t h, char *buf, size_t cap, size_t *out_len)
{
    int slot, rc;
    peer_node *n;
    pc_buf_t b;
    pc_err_clear();
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_buf_init(&b, 0);
    pc_mutex_lock(&n->mu);
    health_json(n, &b, 1);
    pc_mutex_unlock(&n->mu);
    rc = finish_text(&b, buf, cap, out_len);
    release(slot);
    return rc;
}

/* resolve `peer` (registered id or host:port) for node n; expected_id is
 * set when the target was named by id. */
static int resolve_target(peer_node *n, const char *peer, char *host, size_t hostcap,
                          uint16_t *port, char *expected_id, size_t idcap)
{
    char ep[POLYCALL_ENDPOINT_MAX];
    int i;
    expected_id[0] = '\0';
    pc_mutex_lock(&n->mu);
    i = registry_find(n, peer);
    if (i >= 0) {
        pc_copy_out(ep, sizeof ep, n->registry[i].endpoint);
        pc_copy_out(expected_id, idcap, n->registry[i].id);
    }
    pc_mutex_unlock(&n->mu);
    if (i < 0) {
        if (!strchr(peer, ':')) {
            return pc_err(POLYCALL_E_NOT_FOUND,
                          "peer '%s' is not registered on node '%s'", peer, n->node_id);
        }
        pc_copy_out(ep, sizeof ep, peer);
    }
    if (pc_split_endpoint(ep, host, hostcap, port, 0) != POLYCALL_OK) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "'%s' is not host:port", ep);
    }
    return POLYCALL_OK;
}

static int status_from_http(int http, const char *body, char *code, size_t codecap)
{
    char jerr[64];
    json_value *j = json_parse(body ? body : "", body ? strlen(body) : 0, jerr, sizeof jerr);
    pc_copy_out(code, codecap,
                json_str(json_get(json_get(j, "error"), "code"), "", NULL));
    json_free(j);
    switch (http) {
    case 401: return POLYCALL_E_AUTH;
    case 413: return POLYCALL_E_TOO_LARGE;
    case 503: return POLYCALL_E_BUSY;
    case 400: return POLYCALL_E_REMOTE;
    case 404: case 405: case 501: case 505: return POLYCALL_E_PROTOCOL;
    default:  return POLYCALL_E_REMOTE;
    }
}

int polycall_peer_ping(polycall_peer_t h, const char *peer, uint32_t timeout_ms)
{
    int slot, rc, http = 0;
    peer_node *n;
    char host[POLYCALL_ENDPOINT_MAX], expected[POLYCALL_PEER_ID_MAX], err[200];
    uint16_t port = 0;
    char *resp = NULL;
    size_t rlen = 0;
    pc_err_clear();
    if (!peer || !*peer) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "peer is empty");
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    rc = resolve_target(n, peer, host, sizeof host, &port, expected, sizeof expected);
    release(slot);
    if (rc != POLYCALL_OK) return rc;
    err[0] = '\0';
    rc = pc_http_exchange(host, port, "GET", "/health", NULL, NULL, NULL, 0,
                          timeout_ms, PEER_RESP_MAX, &http, &resp, &rlen, err, sizeof err);
    if (rc != POLYCALL_OK) return pc_err(rc, "%s", err[0] ? err : polycall_strerror(rc));
    {
        char jerr[64];
        json_value *j = json_parse(resp, rlen, jerr, sizeof jerr);
        const char *nid = json_str(json_get(j, "node_id"), NULL, NULL);
        const char *st = json_str(json_get(j, "status"), NULL, NULL);
        if (http != 200 || !j || !nid || !st) {
            rc = pc_err(POLYCALL_E_PROTOCOL, "%s:%u is not a healthy polycall peer (HTTP %d)",
                        host, (unsigned)port, http);
        } else if (expected[0] && strcmp(nid, expected) != 0) {
            rc = pc_err(POLYCALL_E_PROTOCOL, "%s:%u answers as '%s', expected '%s'",
                        host, (unsigned)port, nid, expected);
        } else if (strcmp(st, "ok") != 0) {
            rc = pc_err(POLYCALL_E_BUSY, "peer '%s' reports status '%s'", nid, st);
        } else {
            rc = POLYCALL_OK;
        }
        json_free(j);
    }
    free(resp);
    return rc;
}

int polycall_peer_send(polycall_peer_t h, const char *peer, const void *payload,
                       size_t len, const char *message_id, uint32_t timeout_ms)
{
    int slot, rc, http = 0;
    peer_node *n;
    char host[POLYCALL_ENDPOINT_MAX], expected[POLYCALL_PEER_ID_MAX], err[200];
    char mid[POLYCALL_MESSAGE_ID_MAX], self[POLYCALL_PEER_ID_MAX], token[256];
    uint16_t port = 0;
    pc_buf_t body;
    char *resp = NULL;
    size_t rlen = 0;

    pc_err_clear();
    if (!peer || !*peer) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "peer is empty");
    if (len && !payload) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "payload is NULL but len > 0");
    if (len > POLYCALL_PEER_MAX_PAYLOAD) {
        return pc_err(POLYCALL_E_TOO_LARGE, "payload of %lu bytes exceeds the 1 MiB limit",
                      (unsigned long)len);
    }
    if (message_id && *message_id) {
        if (!valid_id(message_id)) {
            return pc_err(POLYCALL_E_INVALID_ARGUMENT,
                          "message_id must be 1-63 characters of [A-Za-z0-9._-]");
        }
        pc_copy_out(mid, sizeof mid, message_id);
    } else {
        char guid[POLYCALL_TELEMETRY_GUID_LEN];
        polycall_telemetry_new_guid(guid);
        pc_copy_out(mid, sizeof mid, guid);
    }
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    rc = resolve_target(n, peer, host, sizeof host, &port, expected, sizeof expected);
    pc_copy_out(self, sizeof self, n->node_id);
    pc_copy_out(token, sizeof token, n->token);
    if (rc != POLYCALL_OK) {
        pc_mutex_lock(&n->mu); n->sent_failed++; pc_mutex_unlock(&n->mu);
        release(slot);
        return rc;
    }

    pc_buf_init(&body, 0);
    pc_buf_append(&body, "{\"v\":1,\"id\":");
    pc_buf_json_string(&body, mid);
    pc_buf_append(&body, ",\"from\":");
    pc_buf_json_string(&body, self);
    pc_buf_append(&body, ",\"payload_b64\":\"");
    pc_buf_base64(&body, payload ? payload : "", len);
    pc_buf_append(&body, "\"}");
    if (pc_buf_failed(&body)) {
        pc_buf_free(&body);
        release(slot);
        return pc_err(POLYCALL_E_NO_MEMORY, "out of memory");
    }

    err[0] = '\0';
    rc = pc_http_exchange(host, port, "POST", "/receive", token, "application/json",
                          body.data, body.len, timeout_ms, PEER_RESP_MAX,
                          &http, &resp, &rlen, err, sizeof err);
    pc_buf_free(&body);
    if (rc != POLYCALL_OK) {
        rc = pc_err(rc, "delivery of '%s' to %s:%u failed: %s", mid, host,
                    (unsigned)port, err[0] ? err : polycall_strerror(rc));
    } else if (http != 200) {
        char code[64];
        rc = status_from_http(http, resp, code, sizeof code);
        rc = pc_err(rc, "%s:%u refused '%s' (HTTP %d %s)", host, (unsigned)port, mid,
                    http, code[0] ? code : pc_http_reason(http));
    } else {
        char jerr[64];
        json_value *j = json_parse(resp, rlen, jerr, sizeof jerr);
        bool ok = json_bool(json_get(j, "ok"), false, NULL);
        const char *ack_id = json_str(json_get(j, "id"), NULL, NULL);
        const char *ack_node = json_str(json_get(j, "node_id"), NULL, NULL);
        if (!j || !ok || !ack_id || strcmp(ack_id, mid) != 0 || !ack_node) {
            rc = pc_err(POLYCALL_E_PROTOCOL,
                        "%s:%u answered 200 without acknowledging message '%s'",
                        host, (unsigned)port, mid);
        } else if (expected[0] && strcmp(ack_node, expected) != 0) {
            rc = pc_err(POLYCALL_E_PROTOCOL,
                        "message '%s' was acknowledged by '%s', expected '%s'",
                        mid, ack_node, expected);
        } else {
            rc = POLYCALL_OK;
        }
        json_free(j);
    }
    free(resp);
    pc_mutex_lock(&n->mu);
    if (rc == POLYCALL_OK) n->sent_ok++; else n->sent_failed++;
    pc_mutex_unlock(&n->mu);
    release(slot);
    return rc;
}

int polycall_peer_recv(polycall_peer_t h, uint32_t timeout_ms,
                       char *sender, size_t sender_cap,
                       char *message_id, size_t message_id_cap,
                       void *payload, size_t payload_cap, size_t *payload_len)
{
    int slot, rc;
    peer_node *n;
    peer_msg *m = NULL;
    unsigned gen0;

    pc_err_clear();
    if (payload_len) *payload_len = 0;
    if (payload_cap && !payload) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "payload is NULL but payload_cap > 0");
    }
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_mutex_lock(&n->mu);
    gen0 = n->cancel_gen;
    /* peek first: a too-small buffer must leave the message queued */
    for (;;) {
        rc = take_message(n, timeout_ms, gen0, &m);
        if (rc != POLYCALL_OK) break;
        if (m->len > payload_cap) {
            /* put it back at the head */
            m->next = n->head;
            n->head = m;
            if (!n->tail) n->tail = m;
            n->inbox_count++;
            n->inbox_bytes += m->len;
            if (payload_len) *payload_len = m->len;
            m = NULL;
            rc = POLYCALL_E_TOO_LARGE;
        }
        break;
    }
    pc_mutex_unlock(&n->mu);
    release(slot);

    if (rc == POLYCALL_E_TOO_LARGE) {
        return pc_err(rc, "payload buffer too small: message needs %lu bytes",
                      (unsigned long)(payload_len ? *payload_len : 0));
    }
    if (rc == POLYCALL_E_TIMEOUT) return pc_err(rc, "no message within %u ms", (unsigned)timeout_ms);
    if (rc == POLYCALL_E_CANCELLED) return pc_err(rc, "receive was cancelled");
    if (rc == POLYCALL_E_CLOSED) return pc_err(rc, "peer node closed while waiting");
    if (rc != POLYCALL_OK) return rc;

    pc_copy_out(sender, sender_cap, m->from);
    pc_copy_out(message_id, message_id_cap, m->id);
    if (m->len) memcpy(payload, m->data, m->len);
    if (payload_len) *payload_len = m->len;
    free(m->data);
    free(m);
    return POLYCALL_OK;
}

int polycall_peer_cancel(polycall_peer_t h)
{
    int slot;
    peer_node *n;
    pc_err_clear();
    if (!(n = acquire(h, &slot))) return POLYCALL_E_INVALID_HANDLE;
    pc_mutex_lock(&n->mu);
    n->cancel_gen++;
    pc_cond_broadcast(&n->inbox_cv);
    pc_mutex_unlock(&n->mu);
    release(slot);
    return POLYCALL_OK;
}
