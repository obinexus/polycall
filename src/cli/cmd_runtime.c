/*
 * start / status / stop / call -- the CLI face of the runtime.
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
#include "polycall_telemetry.h"
#include "../runtime/rpc_wire.h"
#include "../config/json.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

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

#define RARGS_MAX_LOAD 16

typedef struct {
    const char *endpoint;
    const char *endpoint_file;   /* start: write the resolved endpoint here */
    const char *auth_token;
    const char *input_file;
    const char *input_value;
    long deadline_ms;
    const char *load_paths[RARGS_MAX_LOAD];  /* start --load PATH, repeatable */
    int load_count;
    bool daemon;                  /* start --daemon */
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
        else if (!strcmp(name, "--load")) {
            NEEDV("--load");
            if (a->load_count >= RARGS_MAX_LOAD) { *bad = "--load"; return -1; }
            a->load_paths[a->load_count++] = val;
        }
        else if (!strcmp(name, "--daemon")) { a->daemon = true; }
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
/* start                                                              */
/* ================================================================== */

static void on_signal(int sig) { (void)sig; polycall_runtime_request_stop(); }

#if defined(_WIN32)

/* No fork() on Windows: re-exec this same binary, detached from any
 * console, with the already-validated/already-resolved options carried
 * over explicitly (never the raw --daemon flag, so the child runs
 * in the foreground of its own detached session instead of re-daemonizing).
 * Returns 0 and the new process's PID on success. */
static int win_daemonize(const char *project_root, const char *host,
                         uint16_t port, const char *auth_token,
                         const char *endpoint_file,
                         const char *const *load_paths, int load_count,
                         unsigned long *out_pid)
{
    char self[MAX_PATH];
    char cmdline[4096];
    int n, i;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    if (!GetModuleFileNameA(NULL, self, sizeof self)) {
        return -1;
    }
    n = snprintf(cmdline, sizeof cmdline, "\"%s\"", self);
    if (project_root && *project_root) {
        n += snprintf(cmdline + n, sizeof cmdline - (size_t)n,
                       " --project-root \"%s\"", project_root);
    }
    n += snprintf(cmdline + n, sizeof cmdline - (size_t)n,
                  " start --endpoint %s:%u", host, (unsigned)port);
    if (auth_token && *auth_token) {
        n += snprintf(cmdline + n, sizeof cmdline - (size_t)n,
                      " --auth-token \"%s\"", auth_token);
    }
    if (endpoint_file && *endpoint_file) {
        n += snprintf(cmdline + n, sizeof cmdline - (size_t)n,
                      " --endpoint-file \"%s\"", endpoint_file);
    }
    for (i = 0; i < load_count; ++i) {
        n += snprintf(cmdline + n, sizeof cmdline - (size_t)n,
                      " --load \"%s\"", load_paths[i]);
    }
    if (n < 0 || (size_t)n >= sizeof cmdline) {
        return -1;
    }

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    memset(&pi, 0, sizeof pi);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
                        NULL, NULL, &si, &pi)) {
        return -1;
    }
    *out_pid = pi.dwProcessId;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

#else

/* Standard double-fork daemonize (setsid + a second fork so the daemon can
 * never reacquire a controlling terminal), with a pipe carrying the final
 * daemon's pid back to the original process. Deliberately does not
 * chdir("/") -- --project-root-relative paths (Polycallfile, the telemetry
 * sink, --endpoint-file, --load) must keep resolving exactly as they did
 * in the foreground invocation.
 *
 * Returns 1 in the original calling process (caller should print *out_pid
 * and return without serving), 0 in the detached daemon itself (caller
 * should fall through and serve normally, with stdout/stderr now
 * discarded), or -1 on failure. */
static int posix_daemonize(pid_t *out_pid)
{
    int pfd[2];
    pid_t p1;

    if (pipe(pfd) != 0) {
        return -1;
    }
    p1 = fork();
    if (p1 < 0) {
        close(pfd[0]); close(pfd[1]);
        return -1;
    }

    if (p1 > 0) {
        pid_t daemon_pid = -1;
        ssize_t n;
        close(pfd[1]);
        n = read(pfd[0], &daemon_pid, sizeof daemon_pid);
        close(pfd[0]);
        waitpid(p1, NULL, 0);
        if (n != (ssize_t)sizeof daemon_pid || daemon_pid <= 0) {
            return -1;
        }
        *out_pid = daemon_pid;
        return 1;
    }

    /* first child: detach from the controlling terminal's session, then
     * fork once more so the actual daemon is never a session leader */
    close(pfd[0]);
    if (setsid() < 0) {
        _exit(1);
    }
    {
        pid_t p2 = fork();
        if (p2 < 0) {
            _exit(1);
        }
        if (p2 > 0) {
            ssize_t written = write(pfd[1], &p2, sizeof p2);
            (void)written;
            close(pfd[1]);
            _exit(0);
        }
    }

    /* second child: this is the daemon */
    close(pfd[1]);
    {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, 0);
            dup2(devnull, 1);
            dup2(devnull, 2);
            if (devnull > 2) close(devnull);
        }
    }
    return 0;
}

#endif

typedef struct {
    const polycall_invocation_t *inv;
    const char *endpoint_file;
    char endpoint[96];
} bound_ctx_t;

static void POLYCALL_CALL on_bound(const char *endpoint, void *user)
{
    bound_ctx_t *bc = user;
    polycall_telemetry_event_t tev;
    snprintf(bc->endpoint, sizeof bc->endpoint, "%s", endpoint);
    if (bc->endpoint_file) {
        FILE *f = fopen(bc->endpoint_file, "w");
        if (f) { fprintf(f, "%s\n", endpoint); fclose(f); }
    }
    memset(&tev, 0, sizeof tev);
    tev.event = "start.bound";
    tev.detail = endpoint;
    tev.duration_ns = -1;
    polycall_telemetry_emit(bc->inv->g->project_root, &tev);
    if (bc->inv->g->format == POLYCALL_FMT_JSON) {
        fprintf(bc->inv->out,
                "{\"event\":\"listening\",\"endpoint\":\"%s\"}\n", endpoint);
    } else {
        fprintf(bc->inv->out, "polycall start: listening on %s (Ctrl-C to stop)\n",
                endpoint);
    }
    fflush(bc->inv->out);
}

int polycall_cmd_start(const polycall_invocation_t *inv)
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

    if (a.daemon) {
#if defined(_WIN32)
        unsigned long dpid = 0;
        if (win_daemonize(inv->g->project_root, host, port, a.auth_token,
                          a.endpoint_file, a.load_paths, a.load_count,
                          &dpid) != 0) {
            if (inv->g->format == POLYCALL_FMT_JSON) {
                polycall_json_result(inv->out, "start", false, NULL,
                                     "daemon.spawn_failed",
                                     "could not spawn the detached process", NULL);
            } else {
                fprintf(inv->err, "polycall start: could not daemonize\n");
            }
            return POLYCALL_EXIT_RUNTIME;
        }
        if (inv->g->format == POLYCALL_FMT_JSON) {
            char body[64];
            snprintf(body, sizeof body, "{\"pid\":%lu}", dpid);
            polycall_json_result(inv->out, "start", true, body, NULL, NULL, NULL);
        } else {
            fprintf(inv->out, "polycall start: daemonized (pid %lu)\n", dpid);
        }
        return POLYCALL_EXIT_OK;
#else
        pid_t dpid = 0;
        int dr = posix_daemonize(&dpid);
        if (dr < 0) {
            if (inv->g->format == POLYCALL_FMT_JSON) {
                polycall_json_result(inv->out, "start", false, NULL,
                                     "daemon.spawn_failed",
                                     "could not fork/detach", NULL);
            } else {
                fprintf(inv->err, "polycall start: could not daemonize\n");
            }
            return POLYCALL_EXIT_RUNTIME;
        }
        if (dr == 1) {
            /* original process: the daemon continues independently */
            if (inv->g->format == POLYCALL_FMT_JSON) {
                char body[64];
                snprintf(body, sizeof body, "{\"pid\":%d}", (int)dpid);
                polycall_json_result(inv->out, "start", true, body, NULL, NULL, NULL);
            } else {
                fprintf(inv->out, "polycall start: daemonized (pid %d)\n", (int)dpid);
            }
            return POLYCALL_EXIT_OK;
        }
        /* dr == 0: this process IS the daemon now (stdout/stderr already
         * redirected to the null device) -- fall through and serve. Plugin
         * load and bind failures from here on are reported only via
         * telemetry, since there is no terminal left to print to. */
#endif
    }

    rt = polycall_runtime_create();
    if (!rt) {
        fprintf(inv->err, "polycall start: out of memory\n");
        return POLYCALL_EXIT_RUNTIME;
    }

    /* Load plugins before binding anything: an unloadable library, a
     * missing polycall_ops_register, or an ABI mismatch fails start fast,
     * with no socket ever opened. */
    {
        int i;
        for (i = 0; i < a.load_count; ++i) {
            char err[256];
            int prc = polycall_runtime_load_plugin(rt, a.load_paths[i],
                                                    err, sizeof err);
            if (prc != POLYCALL_PLUGIN_OK) {
                polycall_telemetry_event_t tev;
                polycall_runtime_destroy(rt);
                memset(&tev, 0, sizeof tev);
                tev.event = "start.plugin_failed";
                tev.detail = err;
                tev.duration_ns = -1;
                polycall_telemetry_emit(inv->g->project_root, &tev);
                if (inv->g->format == POLYCALL_FMT_JSON) {
                    polycall_json_result(inv->out, "start", false, NULL,
                        prc == POLYCALL_PLUGIN_ABI_MISMATCH
                            ? "plugin.abi_mismatch" : "plugin.load_failed",
                        err, "check the plugin was built against this "
                             "runtime's ABI major version");
                } else {
                    fprintf(inv->err, "polycall start: %s\n", err);
                }
                return POLYCALL_EXIT_UNSUPPORTED;
            }
            if (inv->g->format != POLYCALL_FMT_JSON) {
                fprintf(inv->out, "polycall start: loaded plugin %s\n",
                        a.load_paths[i]);
            }
        }
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
        polycall_telemetry_event_t tev;
        memset(&tev, 0, sizeof tev);
        tev.event = "start.bind_failed";
        tev.detail = host;
        tev.duration_ns = -1;
        polycall_telemetry_emit(inv->g->project_root, &tev);
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "start", false, NULL, "bind",
                                 "could not bind the runtime endpoint", host);
        } else {
            fprintf(inv->err, "polycall start: could not bind %s\n", host);
        }
        return POLYCALL_EXIT_TRANSPORT;
    }
    {
        polycall_telemetry_event_t tev;
        memset(&tev, 0, sizeof tev);
        tev.event = "start.stopped";
        tev.detail = bc.endpoint;
        tev.duration_ns = -1;
        polycall_telemetry_emit(inv->g->project_root, &tev);
    }
    if (inv->g->format == POLYCALL_FMT_JSON) {
        fprintf(inv->out, "{\"event\":\"stopped\",\"endpoint\":\"%s\"}\n",
                bc.endpoint);
    } else {
        fprintf(inv->out, "polycall start: %s stopped cleanly\n", bc.endpoint);
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

    {
        /* Fixed-length literal instead of a hardcoded byte count: a
         * hand-counted length here previously truncated the payload
         * ({"action":"describe"} is 21 bytes, not 20), producing an
         * unterminated-JSON control request that always fell through to
         * "unknown control action". */
        static const char req[] = "{\"action\":\"describe\"}";
        rc = pcr_roundtrip(host, port, PCR_T_CONTROL, req,
                           (uint32_t)(sizeof req - 1), &rep, 3000);
    }
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
    char corr[POLYCALL_TELEMETRY_GUID_LEN];
    uint64_t t0;
    polycall_telemetry_event_t tev;

    if (scan_rargs(inv, &a, &bad) != 0) return rusage(inv, "bad option", bad);
    if (a.npos < 2) {
        return rusage(inv, "usage: call SERVICE OPERATION --input FILE|- [--endpoint host:port]", NULL);
    }
    service = a.pos[0];
    operation = a.pos[1];

    if (split_endpoint(a.endpoint, host, sizeof host, &port) != 0 || port == 0) {
        return rusage(inv, "call requires --endpoint host:port", a.endpoint);
    }

    polycall_telemetry_new_guid(corr);
    t0 = polycall_telemetry_mono_ns();
    memset(&tev, 0, sizeof tev);
    tev.event = "call.start";
    tev.correlation_id = corr;
    tev.service = service;
    tev.operation = operation;
    tev.duration_ns = -1;
    polycall_telemetry_emit(inv->g->project_root, &tev);

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
        const char *reason = rc == -3 ? "no reply before the deadline"
                                       : "could not reach the runtime";
        memset(&tev, 0, sizeof tev);
        tev.event = "call.end";
        tev.correlation_id = corr;
        tev.service = service;
        tev.operation = operation;
        tev.status = "error";
        tev.detail = reason;
        tev.duration_ns = (int64_t)(polycall_telemetry_mono_ns() - t0);
        polycall_telemetry_emit(inv->g->project_root, &tev);
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "call", false, NULL,
                                 rc == -3 ? "deadline" : "transport",
                                 reason, a.endpoint);
        } else {
            fprintf(inv->err, "polycall call: %s\n", reason);
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
        memset(&tev, 0, sizeof tev);
        tev.event = "call.end";
        tev.correlation_id = corr;
        tev.service = service;
        tev.operation = operation;
        tev.status = ok ? "ok" : "error";
        tev.detail = ok ? NULL
                        : json_str(json_get(json_get(v, "error"), "code"), "", NULL);
        tev.duration_ns = (int64_t)(polycall_telemetry_mono_ns() - t0);
        polycall_telemetry_emit(inv->g->project_root, &tev);
        json_free(v);
    }
    pcr_frame_free(&rep);
    return exit_code;
}
