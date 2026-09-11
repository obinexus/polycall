/*
 * PolyCall runtime: deterministic operation registry + single foreground
 * polycall_rpc v1 server. The CLI `call` and the language clients all reach
 * the SAME polycall_op_fn here; nothing is echoed or mocked.
 */

#include "polycall_runtime.h"
#include "rpc_wire.h"
#include "../config/json.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <ws2tcpip.h>
#  include <windows.h>
#else
#  include <arpa/inet.h>
#  include <dlfcn.h>
#  include <errno.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <time.h>
#  include <unistd.h>
#endif

#define RT_MAX_OPS 32
#define RT_MAX_PLUGINS 16

struct polycall_runtime {
    polycall_op_desc_t ops[RT_MAX_OPS];
    int op_count;
    void *plugin_handles[RT_MAX_PLUGINS];
    int plugin_count;
};

static volatile sig_atomic_t g_stop = 0;

void polycall_runtime_request_stop(void) { g_stop = 1; }

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
    snprintf(em, em_cap, "no inventory item with id '%s'", item);
    json_free(in);
    return POLYCALL_OP_ERR_NOTFOUND;
}

static polycall_op_status_t op_debug_echo(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    (void)deadline_ms; (void)user; (void)ec; (void)ec_cap; (void)em; (void)em_cap;
    snprintf(out, out_cap, "{\"echo\":%s}",
             (input_json && *input_json) ? input_json : "null");
    return POLYCALL_OP_OK;
}

static void sleep_ms(unsigned ms)
{
#if defined(_WIN32)
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
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
    if (ms < 0) ms = 0;
    if (deadline_ms > 0 && ms > (double)deadline_ms) {
        snprintf(ec, ec_cap, "deadline.exceeded");
        snprintf(em, em_cap,
                 "requested sleep %.0f ms exceeds the %u ms deadline",
                 ms, deadline_ms);
        return POLYCALL_OP_ERR_DEADLINE;
    }
    sleep_ms((unsigned)ms);
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
    return rt;
}

/* Unload every plugin library. Only ever called from here -- normal
 * execution after polycall_runtime_serve() returns -- never a signal
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

void polycall_runtime_destroy(polycall_runtime_t *rt)
{
    if (!rt) return;
    unload_plugins(rt);
    free(rt);
}

int polycall_runtime_register(polycall_runtime_t *rt, const polycall_op_desc_t *d)
{
    int i;
    if (!rt || !d || !d->service || !d->operation || !d->fn) return -1;
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
/* JSON emit helpers for replies                                       */
/* ================================================================== */

static void jesc(const char *s, char *buf, size_t cap)
{
    size_t w = 0;
    if (cap == 0) return;
    for (; s && *s && w + 2 < cap; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { buf[w++] = '\\'; buf[w++] = (char)c; }
        else if (c == '\n') { buf[w++] = '\\'; buf[w++] = 'n'; }
        else if (c == '\r') { buf[w++] = '\\'; buf[w++] = 'r'; }
        else if (c == '\t') { buf[w++] = '\\'; buf[w++] = 't'; }
        else if (c < 0x20) { if (w + 6 < cap) { snprintf(buf + w, cap - w, "\\u%04x", c); w += 6; } }
        else buf[w++] = (char)c;
    }
    buf[w] = '\0';
}

/* ================================================================== */
/* request / control dispatch                                          */
/* ================================================================== */

static char *dispatch_request(const polycall_runtime_t *rt, const char *payload,
                              uint32_t *status_class)
{
    char jerr[64];
    json_value *req = json_parse(payload, strlen(payload), jerr, sizeof jerr);
    const char *svc, *op, *input_raw;
    const json_value *jin;
    const polycall_op_desc_t *desc;
    uint32_t deadline;
    char out[8192], ec[64], em[256];
    char *resp;
    polycall_op_status_t st;

    *status_class = POLYCALL_OP_ERR_INPUT;
    if (!req) {
        resp = malloc(160);
        snprintf(resp, 160,
                 "{\"ok\":false,\"error\":{\"code\":\"request.malformed\","
                 "\"message\":\"request payload is not valid JSON\"}}");
        json_free(req);
        return resp;
    }
    svc = json_str(json_get(req, "service"), NULL, NULL);
    op = json_str(json_get(req, "operation"), NULL, NULL);
    deadline = (uint32_t)json_num(json_get(req, "deadline_ms"), 5000, NULL);
    jin = json_get(req, "input");

    if (!svc || !op) {
        resp = malloc(200);
        snprintf(resp, 200,
                 "{\"ok\":false,\"error\":{\"code\":\"request.malformed\","
                 "\"message\":\"request needs string 'service' and 'operation'\"}}");
        json_free(req);
        return resp;
    }
    desc = find_op(rt, svc, op);
    if (!desc) {
        char e[96];
        snprintf(e, sizeof e, "%s.%s", svc, op);
        resp = malloc(220);
        snprintf(resp, 220,
                 "{\"ok\":false,\"error\":{\"code\":\"operation.unknown\","
                 "\"message\":\"no registered operation '%s'\"}}", e);
        *status_class = POLYCALL_OP_ERR_NOTFOUND;
        json_free(req);
        return resp;
    }

    /* re-serialise the input sub-value as compact JSON text for the handler */
    input_raw = "null";
    {
        /* json.c has no serialiser; for the built-ins the input is a shallow
         * object, so pass the original slice. Cheap + safe: hand the handler
         * the whole request's "input" by re-reading it from payload via a
         * minimal re-emit. */
        static char inbuf[8192];
        if (jin && jin->type == JSON_OBJECT) {
            size_t k;
            size_t w = 0;
            w += (size_t)snprintf(inbuf + w, sizeof inbuf - w, "{");
            for (k = 0; k < jin->count && w < sizeof inbuf - 64; ++k) {
                char kq[128], vq[512];
                const json_value *v = jin->values[k];
                jesc(jin->keys[k], kq, sizeof kq);
                if (v->type == JSON_STRING) {
                    jesc(v->string, vq, sizeof vq);
                    w += (size_t)snprintf(inbuf + w, sizeof inbuf - w,
                                          "%s\"%s\":\"%s\"", k ? "," : "", kq, vq);
                } else if (v->type == JSON_NUMBER) {
                    w += (size_t)snprintf(inbuf + w, sizeof inbuf - w,
                                          "%s\"%s\":%g", k ? "," : "", kq, v->number);
                } else if (v->type == JSON_BOOL) {
                    w += (size_t)snprintf(inbuf + w, sizeof inbuf - w,
                                          "%s\"%s\":%s", k ? "," : "", kq,
                                          v->boolean ? "true" : "false");
                } else {
                    w += (size_t)snprintf(inbuf + w, sizeof inbuf - w,
                                          "%s\"%s\":null", k ? "," : "", kq);
                }
            }
            snprintf(inbuf + w, sizeof inbuf - w, "}");
            input_raw = inbuf;
        }
    }

    out[0] = ec[0] = em[0] = '\0';
    st = desc->fn(input_raw, deadline, out, sizeof out, ec, sizeof ec,
                  em, sizeof em, desc->user);
    *status_class = (uint32_t)st;

    resp = malloc(9000);
    if (!resp) { json_free(req); return NULL; }
    if (st == POLYCALL_OP_OK) {
        snprintf(resp, 9000, "{\"ok\":true,\"output\":%s}",
                 out[0] ? out : "null");
    } else {
        char emq[300];
        jesc(em, emq, sizeof emq);
        snprintf(resp, 9000,
                 "{\"ok\":false,\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
                 ec[0] ? ec : "operation.failed", emq);
    }
    json_free(req);
    return resp;
}

/* generous, fixed reply buffer for ping/shutdown/unknown; describe below
 * allocates its own, sized to the actual (built-in + plugin) op count. */
#define CTRL_REPLY_CAP 512

static char *control_reply_describe(const polycall_runtime_t *rt)
{
    /* input/output are literal JSON snippets (e.g. {"item_id":"string"}),
     * so they -- like summary -- must be JSON-string-escaped before being
     * embedded as a quoted string, not spliced in raw. */
    size_t cap = 128 + (size_t)rt->op_count * 700;
    char *r = malloc(cap);
    size_t w = 0;
    int i;
    if (!r) return NULL;

    w += (size_t)snprintf(r + w, cap - w, "{\"ok\":true,\"data\":{\"operations\":[");
    for (i = 0; i < rt->op_count; ++i) {
        const polycall_op_desc_t *d = &rt->ops[i];
        char qs[80], qo[80], qsum[256], qin[300], qout[300];
        jesc(d->service, qs, sizeof qs);
        jesc(d->operation, qo, sizeof qo);
        jesc(d->summary, qsum, sizeof qsum);
        jesc(d->input_schema, qin, sizeof qin);
        jesc(d->output_schema, qout, sizeof qout);
        w += (size_t)snprintf(r + w, cap - w,
            "%s{\"service\":\"%s\",\"operation\":\"%s\",\"summary\":\"%s\","
            "\"input\":\"%s\",\"output\":\"%s\",\"idempotent\":%s}",
            i ? "," : "", qs, qo, qsum, qin, qout,
            d->idempotent ? "true" : "false");
    }
    snprintf(r + w, cap - w, "]}}");
    return r;
}

static char *control_reply(const polycall_runtime_t *rt, const char *payload,
                           const char *auth_token, int *want_stop)
{
    char jerr[64];
    json_value *c = json_parse(payload, strlen(payload), jerr, sizeof jerr);
    const char *action = c ? json_str(json_get(c, "action"), "", NULL) : "";
    const char *tok = c ? json_str(json_get(c, "auth_token"), NULL, NULL) : NULL;
    char *r;
    *want_stop = 0;

    if (!strcmp(action, "describe")) {
        r = control_reply_describe(rt);
        json_free(c);
        return r;
    }

    r = malloc(CTRL_REPLY_CAP);
    if (!r) { json_free(c); return NULL; }

    if (!strcmp(action, "ping")) {
        snprintf(r, CTRL_REPLY_CAP, "{\"ok\":true,\"data\":{\"pong\":true,\"abi\":%d}}",
                 POLYCALL_RUNTIME_ABI_VERSION);
    } else if (!strcmp(action, "shutdown")) {
        if (auth_token && *auth_token && (!tok || strcmp(tok, auth_token) != 0)) {
            snprintf(r, CTRL_REPLY_CAP,
                     "{\"ok\":false,\"error\":{\"code\":\"auth.denied\","
                     "\"message\":\"shutdown requires the matching --auth-token\"}}");
        } else {
            snprintf(r, CTRL_REPLY_CAP, "{\"ok\":true,\"data\":{\"stopping\":true}}");
            *want_stop = 1;
        }
    } else {
        snprintf(r, CTRL_REPLY_CAP,
                 "{\"ok\":false,\"error\":{\"code\":\"control.unknown\","
                 "\"message\":\"unknown control action\"}}");
    }
    json_free(c);
    return r;
}

/* ================================================================== */
/* serve loop                                                          */
/* ================================================================== */

static void handle_client(polycall_runtime_t *rt, pcr_sock_t c,
                          const char *auth_token)
{
    for (;;) {
        pcr_frame_t f;
        int rc = pcr_recv(c, &f, 0);
        char *reply = NULL;
        uint8_t rtype = PCR_T_RESPONSE;
        uint32_t sclass = 0;

        if (rc != 0) { pcr_frame_free(&f); return; }

        if (f.type == PCR_T_REQUEST) {
            reply = dispatch_request(rt, f.payload ? f.payload : "null", &sclass);
            rtype = PCR_T_RESPONSE;
        } else if (f.type == PCR_T_CONTROL) {
            int want_stop = 0;
            reply = control_reply(rt, f.payload ? f.payload : "{}", auth_token,
                                  &want_stop);
            rtype = PCR_T_REPLY;
            if (want_stop) g_stop = 1;
        } else {
            reply = malloc(96);
            if (reply) snprintf(reply, 96,
                "{\"ok\":false,\"error\":{\"code\":\"frame.type\"}}");
            rtype = PCR_T_REPLY;
        }

        if (reply) {
            pcr_send(c, rtype, f.corr, reply, (uint32_t)strlen(reply));
            free(reply);
        }
        pcr_frame_free(&f);
        if (g_stop) return;
    }
}

int polycall_runtime_serve(polycall_runtime_t *rt, const char *bind_host,
                           uint16_t port, const char *auth_token,
                           polycall_on_bound_fn on_bound, void *on_bound_user)
{
    char bound[96];
    pcr_sock_t ls;
    struct sockaddr_in addr;
    socklen_t alen;
    int yes = 1;
    const char *host = (bind_host && *bind_host) ? bind_host : "127.0.0.1";

    if (!rt) return -1;
    g_stop = 0;
    pcr_net_init();

    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == PCR_BAD_SOCKET) { pcr_net_shutdown(); return -1; }
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    {
        /* inet_addr is deprecated but universally declared; loopback fallback */
        unsigned long a = inet_addr(host);
        if (a == INADDR_NONE) {
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else {
            memcpy(&addr.sin_addr, &a, sizeof(addr.sin_addr));
        }
    }
    if (bind(ls, (struct sockaddr *)&addr, sizeof addr) != 0 ||
        listen(ls, 8) != 0) {
        pcr_close(ls);
        pcr_net_shutdown();
        return -1;
    }
    alen = sizeof addr;
    bound[0] = '\0';
    if (getsockname(ls, (struct sockaddr *)&addr, &alen) == 0) {
        snprintf(bound, sizeof bound, "%s:%u", host,
                 (unsigned)ntohs(addr.sin_port));
    }
    if (on_bound) {
        on_bound(bound, on_bound_user);
    }

    while (!g_stop) {
        fd_set rf;
        struct timeval tv;
        pcr_sock_t c;
        int r;

        FD_ZERO(&rf);
        FD_SET(ls, &rf);
        tv.tv_sec = 0;
        tv.tv_usec = 200000;  /* 200 ms so a signal is noticed promptly */
#if defined(_WIN32)
        r = select(0, &rf, NULL, NULL, &tv);
#else
        r = select((int)ls + 1, &rf, NULL, NULL, &tv);
#endif
        if (r <= 0) continue;

        c = accept(ls, NULL, NULL);
        if (c == PCR_BAD_SOCKET) continue;
        handle_client(rt, c, auth_token);
        pcr_close(c);
    }

    /* cleanup here, in normal execution -- never in the signal handler */
    pcr_close(ls);
    pcr_net_shutdown();
    return 0;
}
