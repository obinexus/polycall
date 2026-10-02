/*
 * polycall peer serve|send|recv|health|peers|register [options] [Polycallfile]
 *
 * The CLI face of the library's peer node (polycall_peer_* in polycall.h,
 * protocol polycall-peer/1, docs/PEER_PROTOCOL.md). `serve` runs one node
 * in the foreground; the other subcommands are one-shot clients. Settings
 * come from the Polycallfile (peer_node_id, peer_endpoint, `peer <id>
 * <host:port>` lines, auth_token_env); flags override them. The shared
 * token is read from the environment (default POLYCALL_DEV_TOKEN), never
 * from argv, and never printed.
 */

#if defined(_WIN32) && !defined(_WIN32_WINNT)
#  define _WIN32_WINNT 0x0601
#endif

#include "cli_internal.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "polycall.h"
#include "polycall_telemetry.h"
#include "../config/cfgfile.h"
#include "../config/json.h"
#include "../core/pc_buf.h"
#include "../core/pc_sys.h"
#include "../peer/http.h"

#if defined(_WIN32)
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#endif

#define P_MAX_PEERS 64

int polycall_cli_exit_for_status(int status)
{
    switch (status) {
    case POLYCALL_OK:                 return POLYCALL_EXIT_OK;
    case POLYCALL_E_INVALID_ARGUMENT: return POLYCALL_EXIT_USAGE;
    case POLYCALL_E_TOO_LARGE:        return POLYCALL_EXIT_USAGE;
    case POLYCALL_E_CONFIG:           return POLYCALL_EXIT_CONFIG;
    case POLYCALL_E_NOT_FOUND:        return POLYCALL_EXIT_UNSUPPORTED;
    case POLYCALL_E_UNSUPPORTED:      return POLYCALL_EXIT_UNSUPPORTED;
    case POLYCALL_E_TRANSPORT:
    case POLYCALL_E_PROTOCOL:
    case POLYCALL_E_ADDRESS_IN_USE:   return POLYCALL_EXIT_TRANSPORT;
    case POLYCALL_E_TIMEOUT:          return POLYCALL_EXIT_DEADLINE;
    case POLYCALL_E_AUTH:             return POLYCALL_EXIT_AUTH;
    default:                          return POLYCALL_EXIT_RUNTIME;
    }
}

typedef struct {
    const char *node_id;
    const char *endpoint;        /* serve: bind; others: unused        */
    const char *to;              /* target id or host:port             */
    const char *from;
    const char *msg_id;
    const char *payload;
    const char *payload_file;
    const char *endpoint_file;
    const char *auth_token_env;
    const char *peer_id;         /* register --id                      */
    const char *peer_endpoint;   /* register --peer-endpoint           */
    const char *peers[P_MAX_PEERS];  /* --peer ID=HOST:PORT            */
    int npeers;
    long timeout_ms;
    int print_messages;
    int raw;
    const char *polycallfile;
} pargs_t;

typedef struct {
    char node_id[POLYCALL_PEER_ID_MAX];
    char endpoint[POLYCALL_ENDPOINT_MAX];
    char token_env[128];
    const char *token;
    char peer_ids[P_MAX_PEERS][POLYCALL_PEER_ID_MAX];
    char peer_eps[P_MAX_PEERS][POLYCALL_ENDPOINT_MAX];
    int npeers;
} psettings_t;

static int pusage(const polycall_invocation_t *inv, const char *m, const char *b)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char mm[300];
        snprintf(mm, sizeof mm, "%s%s%s", m, b ? ": " : "", b ? b : "");
        polycall_json_result(inv->out, "peer", false, NULL, "usage", mm,
                             "run 'polycall help peer'");
    } else {
        fprintf(inv->err, "polycall peer %s: %s%s%s\n",
                inv->subcommand ? inv->subcommand : "", m, b ? ": " : "", b ? b : "");
    }
    return POLYCALL_EXIT_USAGE;
}

static int pfail(const polycall_invocation_t *inv, int status, const char *detail)
{
    const char *name = polycall_strerror(status);
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char code[64];
        const char *colon = strchr(name, ':');
        size_t n = colon ? (size_t)(colon - name) : strlen(name);
        if (n >= sizeof code) n = sizeof code - 1;
        memcpy(code, name, n);
        code[n] = '\0';
        polycall_json_result(inv->out, "peer", false, NULL, code,
                             detail && *detail ? detail : name, NULL);
    } else {
        fprintf(inv->err, "polycall peer %s: %s\n", inv->subcommand ? inv->subcommand : "",
                detail && *detail ? detail : name);
    }
    return polycall_cli_exit_for_status(status);
}

static int scan_pargs(const polycall_invocation_t *inv, pargs_t *a, const char **bad)
{
    int i;
    memset(a, 0, sizeof *a);
    a->timeout_ms = -1;
    for (i = 0; i < inv->argc; ++i) {
        const char *t = inv->argv[i];
        const char *eq = strchr(t, '=');
        const char *val = NULL;
        char name[32];
        int literal = (inv->literal_from >= 0 && i >= inv->literal_from);
        if (literal || t[0] != '-' || !strcmp(t, "-")) {
            if (a->polycallfile) { *bad = t; return -1; }
            a->polycallfile = t;
            continue;
        }
        if (eq && t[1] == '-') {
            size_t n = (size_t)(eq - t);
            if (n >= sizeof name) n = sizeof name - 1;
            memcpy(name, t, n);
            name[n] = '\0';
            val = eq + 1;
        } else {
            snprintf(name, sizeof name, "%s", t);
        }
#define NEEDV(o) do { if (!val) { if (i + 1 >= inv->argc) { *bad = o; return -1; } val = inv->argv[++i]; } } while (0)
        if (!strcmp(name, "--node-id") || !strcmp(name, "-n")) { NEEDV("--node-id"); a->node_id = val; }
        else if (!strcmp(name, "--endpoint") || !strcmp(name, "-e")) { NEEDV("--endpoint"); a->endpoint = val; }
        else if (!strcmp(name, "--to")) { NEEDV("--to"); a->to = val; }
        else if (!strcmp(name, "--from")) { NEEDV("--from"); a->from = val; }
        else if (!strcmp(name, "--id")) { NEEDV("--id"); a->msg_id = val; a->peer_id = val; }
        else if (!strcmp(name, "--payload") || !strcmp(name, "-p")) { NEEDV("--payload"); a->payload = val; }
        else if (!strcmp(name, "--payload-file")) { NEEDV("--payload-file"); a->payload_file = val; }
        else if (!strcmp(name, "--endpoint-file")) { NEEDV("--endpoint-file"); a->endpoint_file = val; }
        else if (!strcmp(name, "--peer-endpoint")) { NEEDV("--peer-endpoint"); a->peer_endpoint = val; }
        else if (!strcmp(name, "--auth-token-env")) {
            NEEDV("--auth-token-env");
            if (!polycall_cfg_valid_env_name(val)) { *bad = "--auth-token-env"; return -1; }
            a->auth_token_env = val;
        } else if (!strcmp(name, "--peer")) {
            NEEDV("--peer");
            if (a->npeers >= P_MAX_PEERS) { *bad = "--peer (max 64)"; return -1; }
            a->peers[a->npeers++] = val;
        } else if (!strcmp(name, "--timeout-ms") || !strcmp(name, "-t")) {
            char *end = NULL;
            long v;
            NEEDV("--timeout-ms");
            v = strtol(val, &end, 10);
            if (end == val || *end || v < 0 || v > 600000) { *bad = "--timeout-ms (0..600000)"; return -1; }
            a->timeout_ms = v;
        } else if (!strcmp(name, "--print-messages")) { a->print_messages = 1; }
        else if (!strcmp(name, "--raw")) { a->raw = 1; }
        else { *bad = t; return -1; }
#undef NEEDV
    }
    return 0;
}

/* Polycallfile is optional for peer commands: positional, else --config,
 * else <project-root>/Polycallfile when it exists. */
static int resolve_psettings(const polycall_invocation_t *inv, const pargs_t *a,
                             psettings_t *s, char *err, size_t errcap)
{
    char path[1024];
    const char *v;
    int i, explicit_cfg = 0;
    polycall_cfgfile_t *cf = NULL;

    memset(s, 0, sizeof *s);
    path[0] = '\0';
    if (a->polycallfile) { snprintf(path, sizeof path, "%s", a->polycallfile); explicit_cfg = 1; }
    else if (inv->g->config_path) { snprintf(path, sizeof path, "%s", inv->g->config_path); explicit_cfg = 1; }
    else if (inv->g->project_root) snprintf(path, sizeof path, "%s/Polycallfile", inv->g->project_root);

    if (path[0]) {
        int st = 0;
        FILE *f = fopen(path, "rb");
        if (!f) {
            if (explicit_cfg) {
                snprintf(err, errcap, "no Polycallfile at '%s'", path);
                return POLYCALL_E_CONFIG;
            }
        } else {
            fclose(f);
            cf = polycall_cfgfile_load(path, 0u, &st, err, errcap);
            if (!cf) return POLYCALL_E_CONFIG;
        }
    }

    v = a->node_id ? a->node_id : (cf ? polycall_cfgfile_get(cf, "peer_node_id") : NULL);
    pc_copy_out(s->node_id, sizeof s->node_id, v ? v : "");
    v = a->endpoint ? a->endpoint : (cf ? polycall_cfgfile_get(cf, "peer_endpoint") : NULL);
    pc_copy_out(s->endpoint, sizeof s->endpoint, v ? v : "127.0.0.1:0");
    v = a->auth_token_env ? a->auth_token_env : (cf ? polycall_cfgfile_get(cf, "auth_token_env") : NULL);
    pc_copy_out(s->token_env, sizeof s->token_env, v ? v : "POLYCALL_DEV_TOKEN");
    s->token = polycall_cli_auth_token(NULL, s->token_env);

    if (cf) {
        size_t k;
        for (k = 0; k < polycall_cfgfile_peer_count(cf) && s->npeers < P_MAX_PEERS; ++k) {
            const char *id, *ep;
            polycall_cfgfile_peer_at(cf, k, &id, &ep);
            pc_copy_out(s->peer_ids[s->npeers], POLYCALL_PEER_ID_MAX, id);
            pc_copy_out(s->peer_eps[s->npeers], POLYCALL_ENDPOINT_MAX, ep);
            s->npeers++;
        }
        polycall_cfgfile_free(cf);
    }
    for (i = 0; i < a->npeers; ++i) {
        const char *eq = strchr(a->peers[i], '=');
        size_t n;
        if (!eq || eq == a->peers[i] || s->npeers >= P_MAX_PEERS) {
            snprintf(err, errcap, "--peer expects ID=HOST:PORT, got '%s'", a->peers[i]);
            return POLYCALL_E_INVALID_ARGUMENT;
        }
        n = (size_t)(eq - a->peers[i]);
        if (n >= POLYCALL_PEER_ID_MAX) n = POLYCALL_PEER_ID_MAX - 1;
        memcpy(s->peer_ids[s->npeers], a->peers[i], n);
        s->peer_ids[s->npeers][n] = '\0';
        pc_copy_out(s->peer_eps[s->npeers], POLYCALL_ENDPOINT_MAX, eq + 1);
        s->npeers++;
    }
    return POLYCALL_OK;
}

static int open_node(const psettings_t *s, const char *node_id, const char *bind,
                     polycall_peer_t *h, char *err, size_t errcap)
{
    int rc = polycall_peer_open(node_id, bind, s->token, h);
    int i;
    if (rc != POLYCALL_OK) {
        polycall_last_error(err, errcap);
        return rc;
    }
    for (i = 0; i < s->npeers; ++i) {
        rc = polycall_peer_register(*h, s->peer_ids[i], s->peer_eps[i]);
        if (rc != POLYCALL_OK) {
            polycall_last_error(err, errcap);
            polycall_peer_close(*h);
            *h = 0;
            return rc;
        }
    }
    return POLYCALL_OK;
}

/* resolve a --to target that names a registered peer into host:port */
static const char *target_endpoint(const psettings_t *s, const char *to)
{
    int i;
    for (i = 0; i < s->npeers; ++i) {
        if (!strcmp(s->peer_ids[i], to)) return s->peer_eps[i];
    }
    return to;
}

/* ================================================================== */
/* serve                                                               */
/* ================================================================== */

static volatile sig_atomic_t g_peer_stop = 0;

static void on_peer_signal(int sig)
{
    (void)sig;
    g_peer_stop = 1;
}

#if defined(_WIN32)
static BOOL WINAPI on_peer_ctrl(DWORD t)
{
    (void)t;
    g_peer_stop = 1;
    return TRUE;
}
#endif

static void print_event(FILE *out, const char *event, const char *node, const char *ep)
{
    pc_buf_t b;
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"event\":");
    pc_buf_json_string(&b, event);
    pc_buf_append(&b, ",\"node_id\":");
    pc_buf_json_string(&b, node);
    pc_buf_append(&b, ",\"endpoint\":");
    pc_buf_json_string(&b, ep);
    pc_buf_append(&b, "}\n");
    fwrite(b.data, 1, b.len, out);
    fflush(out);
    pc_buf_free(&b);
}

int polycall_cmd_peer_serve(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL;
    char err[512], ep[POLYCALL_ENDPOINT_MAX];
    polycall_peer_t h = 0;
    int rc;
    unsigned char *buf;

    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    if (!s.node_id[0]) return pusage(inv, "serve needs --node-id or peer_node_id", NULL);

    rc = open_node(&s, s.node_id, s.endpoint, &h, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    polycall_peer_endpoint(h, ep, sizeof ep);
    if (a.endpoint_file) {
        FILE *f = fopen(a.endpoint_file, "w");
        if (!f) {
            polycall_peer_close(h);
            snprintf(err, sizeof err, "cannot write --endpoint-file '%s'", a.endpoint_file);
            return pfail(inv, POLYCALL_E_PERMISSION, err);
        }
        fprintf(f, "%s\n", ep);
        fclose(f);
    }

    g_peer_stop = 0;
    signal(SIGINT, on_peer_signal);
#if defined(SIGTERM)
    signal(SIGTERM, on_peer_signal);
#endif
#if defined(_WIN32)
    SetConsoleCtrlHandler(on_peer_ctrl, TRUE);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    print_event(inv->out, "listening", s.node_id, ep);

    buf = malloc(POLYCALL_PEER_MAX_PAYLOAD);
    if (!buf) {
        polycall_peer_close(h);
        return pfail(inv, POLYCALL_E_NO_MEMORY, "out of memory");
    }
    while (!g_peer_stop) {
        if (a.print_messages) {
            char from[POLYCALL_PEER_ID_MAX], mid[POLYCALL_MESSAGE_ID_MAX];
            size_t len = 0;
            rc = polycall_peer_recv(h, 200, from, sizeof from, mid, sizeof mid,
                                    buf, POLYCALL_PEER_MAX_PAYLOAD, &len);
            if (rc == POLYCALL_OK) {
                pc_buf_t b;
                pc_buf_init(&b, 0);
                pc_buf_append(&b, "{\"event\":\"message\",\"from\":");
                pc_buf_json_string(&b, from);
                pc_buf_append(&b, ",\"id\":");
                pc_buf_json_string(&b, mid);
                pc_buf_appendf(&b, ",\"bytes\":%lu,\"payload_b64\":\"", (unsigned long)len);
                pc_buf_base64(&b, buf, len);
                pc_buf_append(&b, "\"}\n");
                fwrite(b.data, 1, b.len, inv->out);
                fflush(inv->out);
                pc_buf_free(&b);
            }
        } else {
            pc_sleep_ms(200);
        }
    }
    free(buf);
    polycall_peer_close(h);
    print_event(inv->out, "stopped", s.node_id, ep);
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* send                                                                */
/* ================================================================== */

static unsigned char *slurp_bytes(const char *path, size_t *len, size_t limit)
{
    FILE *f;
    pc_buf_t b;
    char chunk[8192];
    size_t n;
    if (strcmp(path, "-") == 0) {
        f = stdin;
#if defined(_WIN32)
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    } else {
        f = fopen(path, "rb");
        if (!f) return NULL;
    }
    pc_buf_init(&b, limit + 1);
    while ((n = fread(chunk, 1, sizeof chunk, f)) > 0) {
        if (!pc_buf_append_n(&b, chunk, n)) break;
    }
    if (f != stdin) fclose(f);
    if (pc_buf_failed(&b)) {
        pc_buf_free(&b);
        *len = limit + 1;
        return NULL;
    }
    return (unsigned char *)pc_buf_take(&b, len);
}

int polycall_cmd_peer_send(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL, *from;
    char err[512], mid[POLYCALL_MESSAGE_ID_MAX];
    polycall_peer_t h = 0;
    unsigned char *data = NULL;
    size_t len = 0;
    int rc;
    uint32_t timeout;

    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    if (!a.to) return pusage(inv, "send needs --to PEER_ID|HOST:PORT", NULL);
    if (!!a.payload == !!a.payload_file) return pusage(inv, "give exactly one of --payload TEXT / --payload-file FILE|-", NULL);
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);

    if (a.payload) {
        data = (unsigned char *)a.payload;
        len = strlen(a.payload);
    } else {
        data = slurp_bytes(a.payload_file, &len, POLYCALL_PEER_MAX_PAYLOAD);
        if (!data) {
            if (len > POLYCALL_PEER_MAX_PAYLOAD) {
                return pfail(inv, POLYCALL_E_TOO_LARGE, "payload exceeds the 1 MiB limit");
            }
            snprintf(err, sizeof err, "cannot read --payload-file '%s'", a.payload_file);
            return pfail(inv, POLYCALL_E_NOT_FOUND, err);
        }
    }
    if (a.msg_id) {
        pc_copy_out(mid, sizeof mid, a.msg_id);
    } else {
        char guid[POLYCALL_TELEMETRY_GUID_LEN];
        polycall_telemetry_new_guid(guid);
        pc_copy_out(mid, sizeof mid, guid);
    }
    from = a.from ? a.from : (s.node_id[0] ? s.node_id : "polycall-cli");
    timeout = a.timeout_ms >= 0 ? (uint32_t)a.timeout_ms
            : inv->g->timeout_ms >= 0 ? (uint32_t)inv->g->timeout_ms : 5000u;

    rc = open_node(&s, from, NULL, &h, err, sizeof err);
    if (rc == POLYCALL_OK) {
        rc = polycall_peer_send(h, a.to, data, len, mid, timeout);
        if (rc != POLYCALL_OK) polycall_last_error(err, sizeof err);
        polycall_peer_close(h);
    }
    if (a.payload_file) free(data);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);

    if (inv->g->format == POLYCALL_FMT_JSON) {
        pc_buf_t b;
        pc_buf_init(&b, 0);
        pc_buf_append(&b, "{\"delivered\":true,\"to\":");
        pc_buf_json_string(&b, a.to);
        pc_buf_append(&b, ",\"from\":");
        pc_buf_json_string(&b, from);
        pc_buf_append(&b, ",\"id\":");
        pc_buf_json_string(&b, mid);
        pc_buf_appendf(&b, ",\"bytes\":%lu}", (unsigned long)len);
        polycall_json_result(inv->out, "peer", true, b.data, NULL, NULL, NULL);
        pc_buf_free(&b);
    } else {
        fprintf(inv->out, "polycall peer send: delivered %lu bytes to %s as %s (id %s)\n",
                (unsigned long)len, a.to, from, mid);
    }
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* one-shot HTTP clients: recv / health / peers / register             */
/* ================================================================== */

static int http_to(const polycall_invocation_t *inv, const psettings_t *s, const char *to,
                   const char *method, const char *path, const char *body,
                   uint32_t timeout, int *http, char **resp, size_t *rlen, int *exit_code)
{
    char host[POLYCALL_ENDPOINT_MAX], err[256];
    uint16_t port;
    const char *ep = target_endpoint(s, to);
    int rc;
    err[0] = '\0';
    if (pc_split_endpoint(ep, host, sizeof host, &port, 0) != POLYCALL_OK) {
        snprintf(err, sizeof err, "'%s' is not a registered peer id or host:port", to);
        *exit_code = pfail(inv, POLYCALL_E_INVALID_ARGUMENT, err);
        return -1;
    }
    rc = pc_http_exchange(host, port, method, path, s->token, "application/json",
                          body, body ? strlen(body) : 0, timeout, 4u << 20,
                          http, resp, rlen, err, sizeof err);
    if (rc != POLYCALL_OK) {
        *exit_code = pfail(inv, rc, err[0] ? err : polycall_strerror(rc));
        return -1;
    }
    return 0;
}

static int http_status_exit(const polycall_invocation_t *inv, int http, const char *body)
{
    int st = http == 401 ? POLYCALL_E_AUTH : http == 503 ? POLYCALL_E_BUSY
           : http == 404 || http == 405 ? POLYCALL_E_PROTOCOL : POLYCALL_E_REMOTE;
    char m[600];
    snprintf(m, sizeof m, "HTTP %d: %.500s", http, body ? body : "");
    return pfail(inv, st, m);
}

static int finish_json_reply(const polycall_invocation_t *inv, int http, char *resp, size_t rlen)
{
    int ex;
    (void)rlen;
    if (http != 200) {
        ex = http_status_exit(inv, http, resp);
    } else {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "peer", true, resp, NULL, NULL, NULL);
        } else {
            fprintf(inv->out, "%s\n", resp);
        }
        ex = POLYCALL_EXIT_OK;
    }
    free(resp);
    return ex;
}

int polycall_cmd_peer_health(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL;
    char err[512];
    int rc, http = 0;
    char *resp = NULL;
    size_t rlen = 0;
    int ex = 0;
    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    if (!a.to) return pusage(inv, "health needs --to PEER_ID|HOST:PORT", NULL);
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    rc = http_to(inv, &s, a.to, "GET", "/health", NULL,
                 a.timeout_ms >= 0 ? (uint32_t)a.timeout_ms : 3000u, &http, &resp, &rlen, &ex);
    if (rc != 0) return ex;
    return finish_json_reply(inv, http, resp, rlen);
}

int polycall_cmd_peer_peers(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL;
    char err[512];
    int rc, http = 0;
    char *resp = NULL;
    size_t rlen = 0;
    int ex = 0;
    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    if (!a.to) return pusage(inv, "peers needs --to HOST:PORT", NULL);
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    rc = http_to(inv, &s, a.to, "GET", "/peers", NULL,
                 a.timeout_ms >= 0 ? (uint32_t)a.timeout_ms : 3000u, &http, &resp, &rlen, &ex);
    if (rc != 0) return ex;
    return finish_json_reply(inv, http, resp, rlen);
}

int polycall_cmd_peer_register(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL;
    char err[512];
    int rc, http = 0;
    char *resp = NULL;
    size_t rlen = 0;
    int ex = 0;
    pc_buf_t b;
    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    if (!a.to || !a.peer_id || !a.peer_endpoint) {
        return pusage(inv, "register needs --to HOST:PORT --id PEER_ID --peer-endpoint HOST:PORT", NULL);
    }
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    pc_buf_init(&b, 0);
    pc_buf_append(&b, "{\"node_id\":");
    pc_buf_json_string(&b, a.peer_id);
    pc_buf_append(&b, ",\"endpoint\":");
    pc_buf_json_string(&b, a.peer_endpoint);
    pc_buf_append(&b, "}");
    rc = http_to(inv, &s, a.to, "POST", "/register", b.data,
                 a.timeout_ms >= 0 ? (uint32_t)a.timeout_ms : 3000u, &http, &resp, &rlen, &ex);
    pc_buf_free(&b);
    if (rc != 0) return ex;
    return finish_json_reply(inv, http, resp, rlen);
}

int polycall_cmd_peer_recv(const polycall_invocation_t *inv)
{
    pargs_t a;
    psettings_t s;
    const char *bad = NULL;
    char err[512], body[64];
    int rc, http = 0;
    char *resp = NULL;
    size_t rlen = 0;
    int ex = 0;
    long wait_ms;
    if (scan_pargs(inv, &a, &bad) != 0) return pusage(inv, "bad option", bad);
    if (!a.to) return pusage(inv, "recv needs --to HOST:PORT (the node to read from)", NULL);
    err[0] = '\0';
    rc = resolve_psettings(inv, &a, &s, err, sizeof err);
    if (rc != POLYCALL_OK) return pfail(inv, rc, err);
    wait_ms = a.timeout_ms >= 0 ? a.timeout_ms : 5000;
    if (wait_ms > 30000) return pusage(inv, "recv --timeout-ms is at most 30000", NULL);
    snprintf(body, sizeof body, "{\"timeout_ms\":%ld}", wait_ms);
    rc = http_to(inv, &s, a.to, "POST", "/inbox/next", body, (uint32_t)wait_ms + 5000u,
                 &http, &resp, &rlen, &ex);
    if (rc != 0) return ex;
    if (http != 200) {
        int ex = http_status_exit(inv, http, resp);
        free(resp);
        return ex;
    }
    {
        char jerr[64];
        json_value *j = json_parse(resp, rlen, jerr, sizeof jerr);
        const json_value *m = json_get(j, "message");
        int ex;
        if (!m || m->type != JSON_OBJECT) {
            json_free(j);
            free(resp);
            return pfail(inv, POLYCALL_E_TIMEOUT, "no message arrived within the timeout");
        }
        if (a.raw) {
            const char *b64 = json_str(json_get(m, "payload_b64"), "", NULL);
            unsigned char *bytes = NULL;
            size_t n = 0;
            if (pc_base64_decode(b64, strlen(b64), &bytes, &n) != 0) {
                json_free(j);
                free(resp);
                return pfail(inv, POLYCALL_E_PROTOCOL, "node returned invalid base64");
            }
#if defined(_WIN32)
            _setmode(_fileno(inv->out), _O_BINARY);
#endif
            fwrite(bytes, 1, n, inv->out);
            fflush(inv->out);
            free(bytes);
            ex = POLYCALL_EXIT_OK;
        } else {
            pc_buf_t b;
            pc_buf_init(&b, 0);
            pc_buf_json_value(&b, m);
            if (inv->g->format == POLYCALL_FMT_JSON) {
                polycall_json_result(inv->out, "peer", true, b.data, NULL, NULL, NULL);
            } else {
                fprintf(inv->out, "%s\n", b.data);
            }
            pc_buf_free(&b);
            ex = POLYCALL_EXIT_OK;
        }
        json_free(j);
        free(resp);
        return ex;
    }
}
