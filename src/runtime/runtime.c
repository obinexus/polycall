/*
 * PolyCall runtime: deterministic operation registry + single foreground
 * polycall_rpc v1 server. The CLI `call` and the language clients all reach
 * the SAME polycall_op_fn here; nothing is echoed or mocked.
 *
 * Concurrency (docs/CONCURRENCY.md): one thread per connection, bounded by
 * max_connections; excess connections get an explicit "server.busy" reply.
 * The op registry and plugin table are write-once before serving and
 * read-only while serving; the connection table and counters are guarded
 * by the runtime's own mutex -- there is no global connection state, so
 * two runtimes in one process never interfere.
 */

#include "polycall_runtime.h"
#include "polycall.h"
#include "rpc_wire.h"
#include "../config/json.h"
#include "../core/pc_buf.h"
#include "../core/pc_sys.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include <unistd.h>
#endif

#define RT_MAX_OPS 32
#define RT_MAX_PLUGINS 16
#define RT_MAX_CONNS_LIMIT 1024
#define RT_DEFAULT_CONNS 64
#define RT_DEFAULT_IO_MS 10000
#define RT_DEFAULT_DRAIN_MS 5000
#define RT_OUT_CAP (64u * 1024u)          /* handler output buffer        */
#define RT_MAX_DEADLINE_MS 600000u

typedef struct {
    pc_sock_t sock;
    int in_use;
} conn_slot;

struct polycall_runtime {
    polycall_op_desc_t ops[RT_MAX_OPS];
    int op_count;
    void *plugin_handles[RT_MAX_PLUGINS];
    int plugin_count;

    /* limits */
    uint32_t max_conns, io_timeout_ms, drain_timeout_ms;

    /* serving state (guarded by mu) */
    pc_mutex_t mu;
    pc_cond_t cv;
    int mu_ready;
    volatile int stop;          /* stop accepting; idle connections close  */
    volatile int force_close;   /* drain expired: abort in-flight socket IO */
    int draining;
    int active;
    int destroy_pending;
    conn_slot *conns;
    uint64_t started_ms;
    uint64_t requests, rejected_busy, protocol_errors;
};

static volatile sig_atomic_t g_stop = 0;

void polycall_runtime_request_stop(void) { g_stop = 1; }

static int should_stop(polycall_runtime_t *rt)
{
    return g_stop || pc_atomic_load(&rt->stop);
}

/* ================================================================== */
/* built-in operations                                                */
/* ================================================================== */

static const struct { const char *id; long qty; } STOCK[] = {
    { "widget-a", 42 }, { "widget-b", 7 }, { "gadget-c", 0 }
};

static polycall_op_status_t op_inventory_get(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    char jerr[64];
    json_value *in;
    const char *item;
    size_t i;
    (void)deadline_ms; (void)user;

    in = json_parse(input_json, strlen(input_json), jerr, sizeof jerr);
    item = in ? json_str(json_get(in, "item_id"), NULL, NULL) : NULL;
    if (!item || !*item) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "inventory.get requires a non-empty string 'item_id'");
        json_free(in);
        return POLYCALL_OP_ERR_INPUT;
    }
    for (i = 0; i < sizeof STOCK / sizeof STOCK[0]; ++i) {
        if (strcmp(STOCK[i].id, item) == 0) {
            snprintf(out, out_cap,
                     "{\"item_id\":\"%s\",\"quantity\":%ld,\"in_stock\":%s}",
                     STOCK[i].id, STOCK[i].qty,
                     STOCK[i].qty > 0 ? "true" : "false");
            json_free(in);
            return POLYCALL_OP_OK;
        }
    }
    snprintf(ec, ec_cap, "item.unknown");
    snprintf(em, em_cap, "no inventory item with id '%.200s'", item);
    json_free(in);
    return POLYCALL_OP_ERR_NOTFOUND;
}

static polycall_op_status_t op_debug_echo(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    size_t n = input_json ? strlen(input_json) : 0;
    (void)deadline_ms; (void)user;
    if (n + 10 >= out_cap) {
        snprintf(ec, ec_cap, "output.too_large");
        snprintf(em, em_cap, "echo of %lu bytes exceeds the %lu-byte output limit",
                 (unsigned long)n, (unsigned long)out_cap);
        return POLYCALL_OP_ERR_INPUT;
    }
    snprintf(out, out_cap, "{\"echo\":%s}", n ? input_json : "null");
    return POLYCALL_OP_OK;
}

static polycall_op_status_t op_debug_sleep(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    char jerr[64];
    json_value *in = json_parse(input_json, strlen(input_json), jerr, sizeof jerr);
    double ms = in ? json_num(json_get(in, "ms"), 0, NULL) : 0;
    json_free(in);
    (void)user;
    if (!(ms >= 0)) ms = 0;                     /* also rejects NaN */
    if (ms > RT_MAX_DEADLINE_MS) ms = RT_MAX_DEADLINE_MS;
    if (deadline_ms > 0 && ms > (double)deadline_ms) {
        snprintf(ec, ec_cap, "deadline.exceeded");
        snprintf(em, em_cap,
                 "requested sleep %.0f ms exceeds the %u ms deadline",
                 ms, deadline_ms);
        return POLYCALL_OP_ERR_DEADLINE;
    }
    pc_sleep_ms((uint32_t)ms);
    snprintf(out, out_cap, "{\"slept_ms\":%.0f}", ms);
    return POLYCALL_OP_OK;
}

/* ================================================================== */
/* registry                                                           */
/* ================================================================== */

polycall_runtime_t *polycall_runtime_create(void)
{
    static const polycall_op_desc_t builtins[] = {
        { "inventory", "get", "query stock level by item id",
          "{\"item_id\":\"string\"}",
          "{\"item_id\":\"string\",\"quantity\":\"integer\",\"in_stock\":\"boolean\"}",
          true, op_inventory_get, NULL },
        { "debug", "echo", "return the input unchanged",
          "any", "{\"echo\":\"any\"}", true, op_debug_echo, NULL },
        { "debug", "sleep", "sleep for input.ms (fails if it exceeds the deadline)",
          "{\"ms\":\"integer\"}", "{\"slept_ms\":\"integer\"}",
          false, op_debug_sleep, NULL },
    };
    polycall_runtime_t *rt = calloc(1, sizeof *rt);
    size_t i;
    if (!rt) return NULL;
    for (i = 0; i < sizeof builtins / sizeof builtins[0]; ++i) {
        rt->ops[rt->op_count++] = builtins[i];
    }
    rt->max_conns = RT_DEFAULT_CONNS;
    rt->io_timeout_ms = RT_DEFAULT_IO_MS;
    rt->drain_timeout_ms = RT_DEFAULT_DRAIN_MS;
    pc_mutex_init(&rt->mu);
    pc_cond_init(&rt->cv);
    rt->mu_ready = 1;
    return rt;
}

int polycall_runtime_set_limits(polycall_runtime_t *rt, uint32_t max_connections,
                                uint32_t io_timeout_ms, uint32_t drain_timeout_ms)
{
    if (!rt) return -1;
    if (max_connections > RT_MAX_CONNS_LIMIT ||
        io_timeout_ms > RT_MAX_DEADLINE_MS || drain_timeout_ms > RT_MAX_DEADLINE_MS) {
        return -1;
    }
    if (max_connections) rt->max_conns = max_connections;
    if (io_timeout_ms) rt->io_timeout_ms = io_timeout_ms;
    if (drain_timeout_ms) rt->drain_timeout_ms = drain_timeout_ms;
    return 0;
}

void polycall_runtime_stop(polycall_runtime_t *rt)
{
    if (!rt) return;
    pc_mutex_lock(&rt->mu);
    pc_atomic_store(&rt->stop, 1);
    pc_cond_broadcast(&rt->cv);
    pc_mutex_unlock(&rt->mu);
}

/* Unload every plugin library. Only ever called in normal execution once
 * no connection thread can be running plugin code -- never from a signal
 * handler; see polycall_runtime_request_stop(). */
static void unload_plugins(polycall_runtime_t *rt)
{
    int i;
    for (i = 0; i < rt->plugin_count; ++i) {
        if (!rt->plugin_handles[i]) continue;
#if defined(_WIN32)
        FreeLibrary((HMODULE)rt->plugin_handles[i]);
#else
        dlclose(rt->plugin_handles[i]);
#endif
        rt->plugin_handles[i] = NULL;
    }
    rt->plugin_count = 0;
}

static void free_runtime(polycall_runtime_t *rt)
{
    unload_plugins(rt);
    free(rt->conns);
    pc_cond_destroy(&rt->cv);
    pc_mutex_destroy(&rt->mu);
    free(rt);
}

void polycall_runtime_destroy(polycall_runtime_t *rt)
{
    int defer;
    if (!rt) return;
    pc_mutex_lock(&rt->mu);
    defer = rt->active > 0;     /* a handler still running: the last one frees */
    if (defer) rt->destroy_pending = 1;
    pc_mutex_unlock(&rt->mu);
    if (!defer) free_runtime(rt);
}

int polycall_runtime_register(polycall_runtime_t *rt, const polycall_op_desc_t *d)
{
    int i;
    if (!rt || !d || !d->service || !d->operation || !d->fn) return -1;
    if (!*d->service || !*d->operation) return -1;
    if (rt->op_count >= RT_MAX_OPS) return -1;
    for (i = 0; i < rt->op_count; ++i) {
        if (!strcmp(rt->ops[i].service, d->service) &&
            !strcmp(rt->ops[i].operation, d->operation)) {
            return -1;   /* duplicate: always refused, never "last wins" */
        }
    }
    rt->ops[rt->op_count++] = *d;
    return 0;
}

int polycall_runtime_load_plugin(polycall_runtime_t *rt, const char *path,
                                 char *err, size_t err_cap)
{
    polycall_ops_register_fn entry = NULL;
    void *handle;
    int rc;

    if (err && err_cap) err[0] = '\0';
    if (!rt || !path || !*path) {
        if (err) snprintf(err, err_cap, "no plugin path given");
        return POLYCALL_PLUGIN_ERROR;
    }
    if (rt->plugin_count >= RT_MAX_PLUGINS) {
        if (err) snprintf(err, err_cap, "too many plugins loaded (max %d)",
                          RT_MAX_PLUGINS);
        return POLYCALL_PLUGIN_ERROR;
    }

#if defined(_WIN32)
    {
        wchar_t wpath[1024];
        int wn = MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath,
                                     (int)(sizeof wpath / sizeof wpath[0]));
        if (wn <= 0) {
            if (err) snprintf(err, err_cap, "cannot widen plugin path");
            return POLYCALL_PLUGIN_ERROR;
        }
        handle = LoadLibraryExW(wpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!handle) {
            if (err) snprintf(err, err_cap,
                              "cannot load plugin '%s' (error %lu)", path,
                              (unsigned long)GetLastError());
            return POLYCALL_PLUGIN_ERROR;
        }
        entry = (polycall_ops_register_fn)(void *)
                GetProcAddress((HMODULE)handle, "polycall_ops_register");
    }
#else
    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        if (err) snprintf(err, err_cap, "cannot load plugin '%s': %s", path,
                          dlerror());
        return POLYCALL_PLUGIN_ERROR;
    }
    *(void **)(&entry) = dlsym(handle, "polycall_ops_register");
#endif

    if (!entry) {
        if (err) snprintf(err, err_cap,
                          "plugin '%s' has no polycall_ops_register", path);
#if defined(_WIN32)
        FreeLibrary((HMODULE)handle);
#else
        dlclose(handle);
#endif
        return POLYCALL_PLUGIN_ERROR;
    }

    /* Keep the library open even on a registration failure: the plugin may
     * have partially touched process state, and unloading a library whose
     * code might still be referenced (e.g. by a handler it half-registered)
     * is its own hazard. Unload happens only in polycall_runtime_destroy(). */
    rt->plugin_handles[rt->plugin_count++] = handle;

    rc = entry(rt, (uint32_t)POLYCALL_RUNTIME_ABI_VERSION);
    if (rc == POLYCALL_PLUGIN_ABI_MISMATCH) {
        if (err) snprintf(err, err_cap,
                          "plugin '%s' rejected ABI major %d", path,
                          POLYCALL_RUNTIME_ABI_VERSION);
        return POLYCALL_PLUGIN_ABI_MISMATCH;
    }
    if (rc != POLYCALL_PLUGIN_OK) {
        if (err) snprintf(err, err_cap,
                          "plugin '%s' failed to register its operations "
                          "(status %d)", path, rc);
        return POLYCALL_PLUGIN_ERROR;
    }
    return POLYCALL_PLUGIN_OK;
}

static const polycall_op_desc_t *find_op(const polycall_runtime_t *rt,
                                         const char *svc, const char *op)
{
    int i;
    for (i = 0; i < rt->op_count; ++i) {
        if (!strcmp(rt->ops[i].service, svc) && !strcmp(rt->ops[i].operation, op)) {
            return &rt->ops[i];
        }
    }
    return NULL;
}

/* ================================================================== */
/* replies                                                             */
/* ================================================================== */

static char *error_reply(const char *code, const char *message)
{
    pc_buf_t b;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"ok\":false,\"error\":{\"code\":");
    pc_buf_json_string(&b, code);
    pc_buf_append(&b, ",\"message\":");
    pc_buf_json_string(&b, message);
    pc_buf_append(&b, "}}");
    if (pc_buf_failed(&b)) { pc_buf_free(&b); return NULL; }
    return pc_buf_take(&b, NULL);
}

static char *dispatch_request(const polycall_runtime_t *rt, const char *payload,
                              size_t payload_len, uint32_t *status_class)
{
    char jerr[64];
    json_value *req = json_parse(payload, payload_len, jerr, sizeof jerr);
    const char *svc, *op;
    const json_value *jin, *jdl;
    const polycall_op_desc_t *desc;
    uint32_t deadline = 5000;
    char *out = NULL, ec[64], em[256];
    char *input = NULL;
    char *resp;
    pc_buf_t b;
    polycall_op_status_t st;
    size_t outlen;

    *status_class = POLYCALL_OP_ERR_INPUT;
    if (!req || req->type != JSON_OBJECT) {
        json_free(req);
        return error_reply("request.malformed", "request payload is not a JSON object");
    }
    svc = json_str(json_get(req, "service"), NULL, NULL);
    op = json_str(json_get(req, "operation"), NULL, NULL);
    jin = json_get(req, "input");
    jdl = json_get(req, "deadline_ms");

    if (!svc || !op || !*svc || !*op) {
        json_free(req);
        return error_reply("request.malformed",
                           "request needs string 'service' and 'operation'");
    }
    if (jdl) {
        /* an integer 0..600000; anything else is rejected, never cast */
        if (jdl->type != JSON_NUMBER || !jdl->is_integer ||
            !(jdl->number >= 0) || jdl->number > RT_MAX_DEADLINE_MS) {
            json_free(req);
            return error_reply("request.malformed",
                               "deadline_ms must be an integer in 0..600000");
        }
        deadline = (uint32_t)jdl->number;
    }
    desc = find_op(rt, svc, op);
    if (!desc) {
        char e[160];
        snprintf(e, sizeof e, "no registered operation '%.60s.%.60s'", svc, op);
        *status_class = POLYCALL_OP_ERR_NOTFOUND;
        json_free(req);
        return error_reply("operation.unknown", e);
    }

    /* exact compact re-serialisation of "input" (any JSON type, any depth) */
    pc_buf_init(&b, PCR_MAX_PAYLOAD);
    pc_buf_json_value(&b, jin);       /* NULL -> "null" */
    if (pc_buf_failed(&b)) {
        pc_buf_free(&b);
        json_free(req);
        return error_reply("request.too_large", "input exceeds the payload limit");
    }
    input = pc_buf_take(&b, NULL);

    out = malloc(RT_OUT_CAP);
    if (!out || !input) {
        free(out);
        free(input);
        json_free(req);
        return error_reply("internal", "out of memory");
    }
    out[0] = ec[0] = em[0] = '\0';
    st = desc->fn(input, deadline, out, RT_OUT_CAP, ec, sizeof ec,
                  em, sizeof em, desc->user);
    out[RT_OUT_CAP - 1] = '\0';
    ec[sizeof ec - 1] = '\0';
    em[sizeof em - 1] = '\0';
    free(input);
    *status_class = (uint32_t)st;

    if (st == POLYCALL_OP_OK) {
        outlen = strlen(out);
        if (outlen >= RT_OUT_CAP - 1) {
            *status_class = POLYCALL_OP_ERR_INTERNAL;
            resp = error_reply("output.too_large",
                               "operation output filled its buffer and was truncated");
        } else {
            /* never forward malformed output from a handler or plugin */
            char jerr2[64];
            json_value *chk = outlen ? json_parse(out, outlen, jerr2, sizeof jerr2) : NULL;
            if (outlen && !chk) {
                *status_class = POLYCALL_OP_ERR_INTERNAL;
                resp = error_reply("operation.bad_output",
                                   "operation produced output that is not valid JSON");
            } else {
                pc_buf_init(&b, 0);
                pc_buf_append(&b, "{\"ok\":true,\"output\":");
                pc_buf_append(&b, outlen ? out : "null");
                pc_buf_append(&b, "}");
                resp = pc_buf_failed(&b) ? NULL : pc_buf_take(&b, NULL);
                pc_buf_free(&b);
            }
            json_free(chk);
        }
    } else {
        resp = error_reply(ec[0] ? ec : "operation.failed", em);
    }
    free(out);
    json_free(req);
    return resp;
}

static char *control_reply_describe(const polycall_runtime_t *rt)
{
    pc_buf_t b;
    int i;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"ok\":true,\"data\":{\"operations\":[");
    for (i = 0; i < rt->op_count; ++i) {
        const polycall_op_desc_t *d = &rt->ops[i];
        if (i) pc_buf_append(&b, ",");
        pc_buf_append(&b, "{\"service\":");
        pc_buf_json_string(&b, d->service);
        pc_buf_append(&b, ",\"operation\":");
        pc_buf_json_string(&b, d->operation);
        pc_buf_append(&b, ",\"summary\":");
        pc_buf_json_string(&b, d->summary ? d->summary : "");
        pc_buf_append(&b, ",\"input\":");
        pc_buf_json_string(&b, d->input_schema ? d->input_schema : "");
        pc_buf_append(&b, ",\"output\":");
        pc_buf_json_string(&b, d->output_schema ? d->output_schema : "");
        pc_buf_appendf(&b, ",\"idempotent\":%s}", d->idempotent ? "true" : "false");
    }
    pc_buf_append(&b, "]}}");
    if (pc_buf_failed(&b)) { pc_buf_free(&b); return NULL; }
    return pc_buf_take(&b, NULL);
}

static char *control_reply_health(polycall_runtime_t *rt)
{
    pc_buf_t b;
    char ver[32];
    polycall_ffi_version(ver, (int)sizeof ver);
    pc_buf_init(&b, 0);
    pc_mutex_lock(&rt->mu);
    pc_buf_appendf(&b,
        "{\"ok\":true,\"data\":{\"status\":\"%s\",\"version\":\"%s\",\"abi\":%d,"
        "\"pid\":%ld,\"uptime_ms\":%llu,\"active_connections\":%d,"
        "\"max_connections\":%u,\"requests\":%llu,\"rejected_busy\":%llu,"
        "\"protocol_errors\":%llu,\"operations\":%d}}",
        rt->draining ? "draining" : "ok", ver, POLYCALL_RUNTIME_ABI_VERSION,
#if defined(_WIN32)
        (long)GetCurrentProcessId(),
#else
        (long)getpid(),
#endif
        (unsigned long long)(pc_mono_ms() - rt->started_ms), rt->active,
        rt->max_conns, (unsigned long long)rt->requests,
        (unsigned long long)rt->rejected_busy,
        (unsigned long long)rt->protocol_errors, rt->op_count);
    pc_mutex_unlock(&rt->mu);
    if (pc_buf_failed(&b)) { pc_buf_free(&b); return NULL; }
    return pc_buf_take(&b, NULL);
}

static int token_matches(const char *given, const char *want)
{
    size_t la = given ? strlen(given) : 0, lb = strlen(want), i, n;
    unsigned char diff = (unsigned char)(la != lb);
    n = la > lb ? la : lb;
    for (i = 0; i < n; ++i) {
        unsigned char a = i < la ? (unsigned char)given[i] : 0;
        unsigned char c = i < lb ? (unsigned char)want[i] : 0;
        diff |= (unsigned char)(a ^ c);
    }
    return diff == 0;
}

static char *control_reply(polycall_runtime_t *rt, const char *payload,
                           size_t payload_len, const char *auth_token,
                           int *want_stop)
{
    char jerr[64];
    json_value *c = json_parse(payload, payload_len, jerr, sizeof jerr);
    const char *action = c ? json_str(json_get(c, "action"), "", NULL) : "";
    const char *tok = c ? json_str(json_get(c, "auth_token"), NULL, NULL) : NULL;
    char *r;
    *want_stop = 0;

    if (!strcmp(action, "describe")) {
        r = control_reply_describe(rt);
    } else if (!strcmp(action, "health")) {
        r = control_reply_health(rt);
    } else if (!strcmp(action, "ping")) {
        pc_buf_t b;
        pc_buf_init(&b, 0);
        pc_buf_appendf(&b, "{\"ok\":true,\"data\":{\"pong\":true,\"abi\":%d}}",
                       POLYCALL_RUNTIME_ABI_VERSION);
        r = pc_buf_failed(&b) ? NULL : pc_buf_take(&b, NULL);
        pc_buf_free(&b);
    } else if (!strcmp(action, "shutdown")) {
        if (auth_token && *auth_token && !token_matches(tok, auth_token)) {
            r = error_reply("auth.denied", "shutdown requires the matching auth token");
        } else {
            pc_buf_t b;
            pc_buf_init(&b, 0);
            pc_buf_append(&b, "{\"ok\":true,\"data\":{\"stopping\":true}}");
            r = pc_buf_failed(&b) ? NULL : pc_buf_take(&b, NULL);
            pc_buf_free(&b);
            *want_stop = 1;
        }
    } else {
        r = error_reply("control.unknown", "unknown control action");
    }
    json_free(c);
    return r;
}

/* ================================================================== */
/* connections                                                         */
/* ================================================================== */

typedef struct {
    polycall_runtime_t *rt;
    int slot;
    const char *auth_token;
} conn_ctx_t;

static void handle_client(polycall_runtime_t *rt, pc_sock_t c, const char *auth_token)
{
    for (;;) {
        pcr_frame_t f;
        char *reply = NULL;
        uint8_t rtype = PCR_T_RESPONSE;
        uint32_t sclass = 0;
        int rc;

        /* idle between frames: poll for stop every 200 ms */
        for (;;) {
            int w;
            if (should_stop(rt)) return;
            w = pc_wait_readable(c, 200);
            if (w > 0) break;
            if (w < 0) return;
        }
        /* a frame has started: read it whole within io_timeout_ms */
        rc = pcr_recv(c, &f, rt->io_timeout_ms);
        if (rc != 0) {
            if (rc == -2) {
                char *e = error_reply("frame.invalid",
                                      "bad magic, type, reserved bits or length");
                pc_mutex_lock(&rt->mu);
                rt->protocol_errors++;
                pc_mutex_unlock(&rt->mu);
                if (e) {
                    pcr_send(c, PCR_T_REPLY, 0, e, (uint32_t)strlen(e), rt->io_timeout_ms);
                    free(e);
                }
            }
            pcr_frame_free(&f);
            return;
        }
        pc_mutex_lock(&rt->mu);
        rt->requests++;
        pc_mutex_unlock(&rt->mu);

        if (f.type == PCR_T_REQUEST) {
            reply = dispatch_request(rt, f.payload ? f.payload : "null",
                                     f.payload ? f.length : 4, &sclass);
            rtype = PCR_T_RESPONSE;
        } else if (f.type == PCR_T_CONTROL) {
            int want_stop = 0;
            reply = control_reply(rt, f.payload ? f.payload : "{}",
                                  f.payload ? f.length : 2, auth_token, &want_stop);
            rtype = PCR_T_REPLY;
            if (want_stop) polycall_runtime_stop(rt);
        } else {
            reply = error_reply("frame.type", "expected a REQUEST or CONTROL frame");
            rtype = PCR_T_REPLY;
        }

        if (!reply) reply = error_reply("internal", "out of memory");
        if (reply) {
            size_t rl = strlen(reply);
            if (rl > PCR_MAX_PAYLOAD) {
                free(reply);
                reply = error_reply("output.too_large", "reply exceeds the 1 MiB frame limit");
                rl = reply ? strlen(reply) : 0;
            }
            if (reply) pcr_send(c, rtype, f.corr, reply, (uint32_t)rl, rt->io_timeout_ms);
            free(reply);
        }
        pcr_frame_free(&f);
        if (should_stop(rt)) return;
    }
}

static void conn_main(void *arg)
{
    conn_ctx_t *ctx = (conn_ctx_t *)arg;
    polycall_runtime_t *rt = ctx->rt;
    pc_sock_t sock;
    int last;

    pc_mutex_lock(&rt->mu);
    sock = rt->conns[ctx->slot].sock;
    pc_mutex_unlock(&rt->mu);

    pc_set_thread_abort(&rt->force_close);
    handle_client(rt, sock, ctx->auth_token);
    pc_sock_finish(sock, 200);    /* a final error reply survives the close */
    pc_set_thread_abort(NULL);

    pc_mutex_lock(&rt->mu);
    pc_sock_close(rt->conns[ctx->slot].sock);   /* under the lock: see drain */
    rt->conns[ctx->slot].sock = PC_BAD_SOCK;
    rt->conns[ctx->slot].in_use = 0;
    rt->active--;
    last = (rt->active == 0 && rt->destroy_pending);
    pc_cond_broadcast(&rt->cv);
    pc_mutex_unlock(&rt->mu);
    free(ctx);
    if (last) free_runtime(rt);
}

static void spawn_connection(polycall_runtime_t *rt, pc_sock_t c, const char *auth_token)
{
    conn_ctx_t *ctx;
    pc_thread_t t;
    int slot = -1;
    uint32_t i;

    pc_mutex_lock(&rt->mu);
    if ((uint32_t)rt->active < rt->max_conns) {
        for (i = 0; i < rt->max_conns; ++i) {
            if (!rt->conns[i].in_use) { slot = (int)i; break; }
        }
    }
    if (slot < 0) {
        rt->rejected_busy++;
        pc_mutex_unlock(&rt->mu);
        {
            /* explicit backpressure: the client learns why, then we close */
            char *e = error_reply("server.busy",
                                  "connection limit reached; retry later");
            if (e) {
                pcr_send(c, PCR_T_REPLY, 0, e, (uint32_t)strlen(e), 1000);
                free(e);
            }
        }
        pc_sock_finish(c, 200);   /* let the client read the reply: no RST */
        pc_sock_close(c);
        return;
    }
    rt->conns[slot].in_use = 1;
    rt->conns[slot].sock = c;
    rt->active++;
    pc_mutex_unlock(&rt->mu);

    ctx = malloc(sizeof *ctx);
    if (ctx) {
        ctx->rt = rt;
        ctx->slot = slot;
        ctx->auth_token = auth_token;
    }
    if (!ctx || pc_thread_start(&t, conn_main, ctx) != 0) {
        free(ctx);
        pc_mutex_lock(&rt->mu);
        pc_sock_close(rt->conns[slot].sock);
        rt->conns[slot].sock = PC_BAD_SOCK;
        rt->conns[slot].in_use = 0;
        rt->active--;
        pc_cond_broadcast(&rt->cv);
        pc_mutex_unlock(&rt->mu);
        return;
    }
#if defined(_WIN32)
    CloseHandle(t);               /* detached: conn_main reports completion */
#else
    pthread_detach(t);
#endif
}

/* ================================================================== */
/* serve loop                                                          */
/* ================================================================== */

int polycall_runtime_serve(polycall_runtime_t *rt, const char *bind_host,
                           uint16_t port, const char *auth_token,
                           polycall_on_bound_fn on_bound, void *on_bound_user)
{
    char bound[160];
    pc_sock_t ls = PC_BAD_SOCK;
    uint16_t bport = 0;
    const char *host = (bind_host && *bind_host) ? bind_host : "127.0.0.1";
    char err[200];
    int rc;
    uint64_t drain_deadline;
    uint32_t i;

    if (!rt) return POLYCALL_RUNTIME_SERVE_ERROR;
    g_stop = 0;
    pc_mutex_lock(&rt->mu);
    pc_atomic_store(&rt->stop, 0);
    pc_atomic_store(&rt->force_close, 0);
    rt->draining = 0;
    rt->active = 0;
    rt->requests = rt->rejected_busy = rt->protocol_errors = 0;
    rt->started_ms = pc_mono_ms();
    free(rt->conns);
    rt->conns = calloc(rt->max_conns, sizeof *rt->conns);
    pc_mutex_unlock(&rt->mu);
    if (!rt->conns) return POLYCALL_RUNTIME_SERVE_ERROR;
    for (i = 0; i < rt->max_conns; ++i) rt->conns[i].sock = PC_BAD_SOCK;

    rc = pc_listen(host, port, 128, &ls, &bport, err, sizeof err);
    if (rc != POLYCALL_OK) {
        if (rc == POLYCALL_E_ADDRESS_IN_USE) return POLYCALL_RUNTIME_SERVE_ADDRESS_IN_USE;
        if (rc == POLYCALL_E_PERMISSION) return POLYCALL_RUNTIME_SERVE_PERMISSION;
        if (rc == POLYCALL_E_CONFIG) return POLYCALL_RUNTIME_SERVE_BAD_ADDRESS;
        return POLYCALL_RUNTIME_SERVE_ERROR;
    }
    snprintf(bound, sizeof bound, "%s:%u", host, (unsigned)bport);
    if (on_bound) {
        on_bound(bound, on_bound_user);
    }

    while (!should_stop(rt)) {
        pc_sock_t c;
        rc = pc_accept(ls, 200, &c);   /* 200 ms so a signal is noticed promptly */
        if (rc == POLYCALL_E_TIMEOUT) continue;
        if (rc != POLYCALL_OK) { pc_sleep_ms(20); continue; }   /* no busy spin */
        spawn_connection(rt, c, auth_token);
    }

    /* Stop accepting first, then drain: in-flight requests get
     * drain_timeout_ms; after that every connection still open is shut
     * down (its blocked I/O fails at once) and we wait for the threads to
     * leave. A handler still executing keeps the runtime alive past this
     * point -- polycall_runtime_destroy() defers the free to it. */
    pc_sock_close(ls);
    pc_mutex_lock(&rt->mu);
    rt->draining = 1;
    drain_deadline = pc_deadline_after(rt->drain_timeout_ms);
    while (rt->active > 0 && pc_remaining_ms(drain_deadline) > 0) {
        pc_cond_timedwait(&rt->cv, &rt->mu, pc_remaining_ms(drain_deadline));
    }
    if (rt->active > 0) {
        pc_atomic_store(&rt->force_close, 1);   /* their waits give up now */
        for (i = 0; i < rt->max_conns; ++i) {
            if (rt->conns[i].in_use && rt->conns[i].sock != PC_BAD_SOCK) {
                pc_sock_shutdown(rt->conns[i].sock);
            }
        }
        drain_deadline = pc_deadline_after(rt->io_timeout_ms);
        while (rt->active > 0 && pc_remaining_ms(drain_deadline) > 0) {
            pc_cond_timedwait(&rt->cv, &rt->mu, pc_remaining_ms(drain_deadline));
        }
    }
    pc_mutex_unlock(&rt->mu);
    return POLYCALL_RUNTIME_SERVE_OK;
}
