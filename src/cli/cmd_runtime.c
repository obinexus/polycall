/*
 * run / status / stop / call -- the CLI face of the runtime.
 *
 * All four talk to the SAME polycall_op_fn set through polycall_rpc v1. `call`
 * performs exactly one round trip and never retries. Errors are mapped to the
 * central exit-code contract; a missing runtime is a failure, not a stub.
 */

#include "cli_internal.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "polycall_runtime.h"
#include "../runtime/rpc_wire.h"
#include "../config/json.h"

/* ---- shared: split "host:port" -------------------------------------- */

static int split_endpoint(const char *ep, char *host, size_t hostcap,
                          uint16_t *port)
{
    const char *colon;
    long p;
    if (!ep || !*ep) {
        snprintf(host, hostcap, "127.0.0.1");
        *port = 0;
        return 0;
    }
    colon = strrchr(ep, ':');
    if (!colon || colon == ep) {
        return -1;
    }
    if ((size_t)(colon - ep) >= hostcap) {
        return -1;
    }
    memcpy(host, ep, (size_t)(colon - ep));
    host[colon - ep] = '\0';
    p = strtol(colon + 1, NULL, 10);
    if (p < 0 || p > 65535) {
        return -1;
    }
    *port = (uint16_t)p;
    return 0;
}

/* ---- local option scan -------------------------------------------- */

typedef struct {
    const char *endpoint;
    const char *endpoint_file;   /* run: write the resolved endpoint here */
    const char *auth_token;
    const char *input_file;
    const char *input_value;
    long deadline_ms;
    const char *pos[8];
    int npos;
} rargs_t;

static int scan_rargs(const polycall_invocation_t *inv, rargs_t *a, const char **bad)
{
    int i;
    memset(a, 0, sizeof *a);
    a->deadline_ms = -1;
    for (i = 0; i < inv->argc; ++i) {
        const char *t = inv->argv[i];
        const char *eq = strchr(t, '=');
        const char *val = NULL;
        char name[24];
        bool literal = (inv->literal_from >= 0 && i >= inv->literal_from);

        if (literal || t[0] != '-') {
            if (a->npos < 8) a->pos[a->npos++] = t;
            continue;
        }
        if (eq) {
            size_t n = (size_t)(eq - t);
            if (n >= sizeof name) n = sizeof name - 1;
            memcpy(name, t, n); name[n] = '\0';
            val = eq + 1;
        } else {
            snprintf(name, sizeof name, "%s", t);
        }
        #define NEEDV(o) do { if (!val) { if (i + 1 >= inv->argc) { *bad = o; return -1; } val = inv->argv[++i]; } } while (0)
        if (!strcmp(name, "--endpoint"))       { NEEDV("--endpoint");    a->endpoint = val; }
        else if (!strcmp(name, "--endpoint-file")) { NEEDV("--endpoint-file"); a->endpoint_file = val; }
        else if (!strcmp(name, "--auth-token")) { NEEDV("--auth-token"); a->auth_token = val; }
        else if (!strcmp(name, "--input"))      { NEEDV("--input");      a->input_file = val; }
        else if (!strcmp(name, "--input-value")) { NEEDV("--input-value"); a->input_value = val; }
        else if (!strcmp(name, "--deadline-ms")) { NEEDV("--deadline-ms"); a->deadline_ms = strtol(val, NULL, 10); }
        else { *bad = t; return -1; }
        #undef NEEDV
    }
    return 0;
}

static int rusage(const polycall_invocation_t *inv, const char *m, const char *b)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char mm[256];
        snprintf(mm, sizeof mm, "%s%s%s", m, b ? ": " : "", b ? b : "");
        polycall_json_result(inv->out, inv->command, false, NULL, "usage", mm, NULL);
    } else {
        fprintf(inv->err, "polycall %s: %s%s%s\n", inv->command, m,
                b ? ": " : "", b ? b : "");
    }
    return POLYCALL_EXIT_USAGE;
}

/* ================================================================== */
/* run                                                                */
/* ================================================================== */

static void on_signal(int sig) { (void)sig; polycall_runtime_request_stop(); }

typedef struct {
    const polycall_invocation_t *inv;
    const char *endpoint_file;
    char endpoint[96];
} bound_ctx_t;

static void POLYCALL_CALL on_bound(const char *endpoint, void *user)
{
    bound_ctx_t *bc = user;
    snprintf(bc->endpoint, sizeof bc->endpoint, "%s", endpoint);
    if (bc->endpoint_file) {
        FILE *f = fopen(bc->endpoint_file, "w");
        if (f) { fprintf(f, "%s\n", endpoint); fclose(f); }
    }
    if (bc->inv->g->format == POLYCALL_FMT_JSON) {
        fprintf(bc->inv->out,
                "{\"event\":\"listening\",\"endpoint\":\"%s\"}\n", endpoint);
    } else {
        fprintf(bc->inv->out, "polycall run: listening on %s (Ctrl-C to stop)\n",
                endpoint);
    }
    fflush(bc->inv->out);
}

int polycall_cmd_run(const polycall_invocation_t *inv)
{
    rargs_t a;
    const char *bad = NULL;
    char host[128];
    uint16_t port = 0;
    polycall_runtime_t *rt;
    bound_ctx_t bc;
    int rc;

    if (scan_rargs(inv, &a, &bad) != 0) return rusage(inv, "bad option", bad);
    if (split_endpoint(a.endpoint ? a.endpoint : "127.0.0.1:0",
                       host, sizeof host, &port) != 0) {
        return rusage(inv, "invalid --endpoint (expected host:port)", a.endpoint);
    }

    rt = polycall_runtime_create();
    if (!rt) {
        fprintf(inv->err, "polycall run: out of memory\n");
        return POLYCALL_EXIT_RUNTIME;
    }

    signal(SIGINT, on_signal);
#ifdef SIGTERM
    signal(SIGTERM, on_signal);
#endif

    memset(&bc, 0, sizeof bc);
    bc.inv = inv;
    bc.endpoint_file = a.endpoint_file;

    rc = polycall_runtime_serve(rt, host, port, a.auth_token, on_bound, &bc);
    polycall_runtime_destroy(rt);

    if (rc != 0) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "run", false, NULL, "bind",
                                 "could not bind the runtime endpoint", host);
        } else {
            fprintf(inv->err, "polycall run: could not bind %s\n", host);
        }
        return POLYCALL_EXIT_TRANSPORT;
    }
    if (inv->g->format == POLYCALL_FMT_JSON) {
        fprintf(inv->out, "{\"event\":\"stopped\",\"endpoint\":\"%s\"}\n",
                bc.endpoint);
    } else {
        fprintf(inv->out, "polycall run: %s stopped cleanly\n", bc.endpoint);
    }
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* transport error -> exit code                                        */
/* ================================================================== */

static int map_transport(int rc)   /* rc from pcr_roundtrip */
{
    if (rc == -3) return POLYCALL_EXIT_DEADLINE;
    return POLYCALL_EXIT_TRANSPORT;   /* -1 connect/io, -2 protocol */
}

/* ================================================================== */
/* status                                                             */
/* ================================================================== */

int polycall_cmd_status(const polycall_invocation_t *inv)
{
    rargs_t a;
    const char *bad = NULL;
    char host[128];
    uint16_t port;
    pcr_frame_t rep;
    int rc;

    if (scan_rargs(inv, &a, &bad) != 0) return rusage(inv, "bad option", bad);
    if (!a.endpoint) return rusage(inv, "status requires --endpoint host:port", NULL);
    if (split_endpoint(a.endpoint, host, sizeof host, &port) != 0 || port == 0) {
        return rusage(inv, "invalid --endpoint", a.endpoint);
    }

    rc = pcr_roundtrip(host, port, PCR_T_CONTROL,
                       "{\"action\":\"describe\"}", 20, &rep, 3000);
    if (rc != 0) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "status", false, NULL, "transport",
                                 "no runtime answered at the endpoint", a.endpoint);
        } else {
            fprintf(inv->err, "polycall status: no runtime at %s\n", a.endpoint);
        }
        return map_transport(rc);
    }

    if (inv->g->format == POLYCALL_FMT_JSON) {
        polycall_json_result(inv->out, "status", true,
                             rep.payload ? rep.payload : "null", NULL, NULL, NULL);
    } else {
        char jerr[64];
        json_value *v = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
        const json_value *ops = v ? json_get(json_get(v, "data"), "operations") : NULL;
        fprintf(inv->out, "runtime at %s: responding\n", a.endpoint);
        if (ops && ops->type == JSON_ARRAY) {
            size_t i;
            fprintf(inv->out, "registered operations:\n");
            for (i = 0; i < ops->count; ++i) {
                const json_value *o = ops->items[i];
                fprintf(inv->out, "  %s.%s  - %s\n",
                        json_str(json_get(o, "service"), "?", NULL),
                        json_str(json_get(o, "operation"), "?", NULL),
                        json_str(json_get(o, "summary"), "", NULL));
                fprintf(inv->out, "      in  %s\n",
                        json_str(json_get(o, "input"), "", NULL));
                fprintf(inv->out, "      out %s   idempotent=%s\n",
                        json_str(json_get(o, "output"), "", NULL),
                        json_bool(json_get(o, "idempotent"), false, NULL) ? "yes" : "no");
            }
        }
        json_free(v);
    }
    pcr_frame_free(&rep);
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* stop                                                               */
/* ================================================================== */

int polycall_cmd_stop(const polycall_invocation_t *inv)
{
    rargs_t a;
    const char *bad = NULL;
    char host[128];
    uint16_t port;
    char body[256], q[160];
    pcr_frame_t rep;
    int rc, exit_code = POLYCALL_EXIT_OK;

    if (scan_rargs(inv, &a, &bad) != 0) return rusage(inv, "bad option", bad);
    if (!a.endpoint) return rusage(inv, "stop requires --endpoint host:port", NULL);
    if (split_endpoint(a.endpoint, host, sizeof host, &port) != 0 || port == 0) {
        return rusage(inv, "invalid --endpoint", a.endpoint);
    }

    if (a.auth_token) {
        snprintf(body, sizeof body, "{\"action\":\"shutdown\",\"auth_token\":%s}",
                 polycall_json_quote(q, sizeof q, a.auth_token));
    } else {
        snprintf(body, sizeof body, "{\"action\":\"shutdown\"}");
    }

    rc = pcr_roundtrip(host, port, PCR_T_CONTROL, body, (uint32_t)strlen(body),
                       &rep, 3000);
    if (rc != 0) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "stop", false, NULL, "transport",
                                 "no runtime answered at the endpoint", a.endpoint);
        } else {
            fprintf(inv->err, "polycall stop: no runtime at %s\n", a.endpoint);
        }
        return map_transport(rc);
    }

    {
        char jerr[64];
        json_value *v = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
        bool ok = json_bool(json_get(v, "ok"), false, NULL);
        const char *code = json_str(json_get(json_get(v, "error"), "code"), "", NULL);
        if (!ok && !strcmp(code, "auth.denied")) {
            exit_code = POLYCALL_EXIT_AUTH;
        } else if (!ok) {
            exit_code = POLYCALL_EXIT_RUNTIME;
        }
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "stop", ok,
                                 rep.payload ? rep.payload : "null",
                                 ok ? NULL : (code[0] ? code : "stop.failed"),
                                 ok ? NULL : "shutdown was refused", NULL);
        } else if (ok) {
            fprintf(inv->out, "polycall stop: %s acknowledged shutdown\n", a.endpoint);
        } else {
            fprintf(inv->err, "polycall stop: refused (%s)\n", code[0] ? code : "?");
        }
        json_free(v);
    }
    pcr_frame_free(&rep);
    return exit_code;
}

/* ================================================================== */
/* call                                                               */
/* ================================================================== */

static char *slurp(const char *path, size_t *len)
{
    FILE *f;
    char *buf;
    size_t cap = 0, n = 0;
    int c;
    if (path && strcmp(path, "-") != 0) {
        f = fopen(path, "rb");
        if (!f) return NULL;
    } else {
        f = stdin;
    }
    buf = malloc(cap = 4096);
    if (!buf) { if (f != stdin) fclose(f); return NULL; }
    while ((c = fgetc(f)) != EOF) {
        if (n + 1 >= cap) {
            char *nb = realloc(buf, cap *= 2);
            if (!nb) { free(buf); if (f != stdin) fclose(f); return NULL; }
            buf = nb;
        }
        buf[n++] = (char)c;
    }
    buf[n] = '\0';
    *len = n;
    if (f != stdin) fclose(f);
    return buf;
}

static int err_code_to_exit(const char *code)
{
    if (!code || !*code) return POLYCALL_EXIT_RUNTIME;
    if (!strcmp(code, "request.malformed") || !strcmp(code, "input.invalid"))
        return POLYCALL_EXIT_CONFIG;         /* bad request payload */
    if (!strcmp(code, "operation.unknown")) return POLYCALL_EXIT_UNSUPPORTED;
    if (!strcmp(code, "item.unknown"))      return POLYCALL_EXIT_UNSUPPORTED;
    if (!strcmp(code, "deadline.exceeded")) return POLYCALL_EXIT_DEADLINE;
    if (!strcmp(code, "auth.denied"))       return POLYCALL_EXIT_AUTH;
    return POLYCALL_EXIT_RUNTIME;
}

int polycall_cmd_call(const polycall_invocation_t *inv)
{
    rargs_t a;
    const char *bad = NULL;
    const char *service, *operation;
    char host[128];
    uint16_t port;
    char *input = NULL;
    size_t input_len = 0;
    char *req;
    size_t reqcap;
    uint32_t deadline;
    pcr_frame_t rep;
    int rc, exit_code = POLYCALL_EXIT_OK;

    if (scan_rargs(inv, &a, &bad) != 0) return rusage(inv, "bad option", bad);
    if (a.npos < 2) {
        return rusage(inv, "usage: call SERVICE OPERATION --input FILE|- [--endpoint host:port]", NULL);
    }
    service = a.pos[0];
    operation = a.pos[1];

    if (split_endpoint(a.endpoint, host, sizeof host, &port) != 0 || port == 0) {
        return rusage(inv, "call requires --endpoint host:port", a.endpoint);
    }

    if (a.input_value) {
        input = strdup(a.input_value);
        input_len = strlen(input);
    } else if (a.input_file) {
        input = slurp(a.input_file, &input_len);
        if (!input) return rusage(inv, "cannot read --input", a.input_file);
    } else {
        input = strdup("null");
        input_len = 4;
    }

    /* deadline: --deadline-ms local wins, else global --timeout-ms, else 5000 */
    deadline = a.deadline_ms >= 0 ? (uint32_t)a.deadline_ms
             : inv->g->timeout_ms >= 0 ? (uint32_t)inv->g->timeout_ms
             : 5000;

    reqcap = input_len + 256;
    req = malloc(reqcap);
    if (!req) { free(input); return POLYCALL_EXIT_RUNTIME; }
    snprintf(req, reqcap,
             "{\"service\":\"%s\",\"operation\":\"%s\",\"deadline_ms\":%u,\"input\":%s}",
             service, operation, deadline,
             (input_len && input[0]) ? input : "null");
    free(input);

    rc = pcr_roundtrip(host, port, PCR_T_REQUEST, req, (uint32_t)strlen(req),
                       &rep, deadline + 1000);
    free(req);
    if (rc != 0) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "call", false, NULL,
                                 rc == -3 ? "deadline" : "transport",
                                 rc == -3 ? "no reply before the deadline"
                                          : "could not reach the runtime",
                                 a.endpoint);
        } else {
            fprintf(inv->err, "polycall call: %s\n",
                    rc == -3 ? "no reply before the deadline"
                             : "could not reach the runtime");
        }
        return map_transport(rc);
    }

    {
        char jerr[64];
        json_value *v = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
        bool ok = json_bool(json_get(v, "ok"), false, NULL);
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "call", ok,
                                 rep.payload ? rep.payload : "null",
                                 ok ? NULL
                                    : json_str(json_get(json_get(v, "error"), "code"),
                                               "operation.failed", NULL),
                                 ok ? NULL
                                    : json_str(json_get(json_get(v, "error"), "message"),
                                               "operation failed", NULL),
                                 NULL);
        } else if (ok) {
            /* print the output value as-is (already compact JSON) */
            const json_value *out = json_get(v, "output");
            (void)out;
            fprintf(inv->out, "%s\n", rep.payload);
        } else {
            const char *code = json_str(json_get(json_get(v, "error"), "code"), "", NULL);
            const char *msg = json_str(json_get(json_get(v, "error"), "message"), "", NULL);
            fprintf(inv->err, "polycall call: %s (%s)\n", msg, code);
        }
        if (!ok) {
            exit_code = err_code_to_exit(
                json_str(json_get(json_get(v, "error"), "code"), "", NULL));
        }
        json_free(v);
    }
    pcr_frame_free(&rep);
    return exit_code;
}
