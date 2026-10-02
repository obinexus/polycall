/*
 * polycall daemon run|start|stop|status|restart [options] [Polycallfile]
 *
 * One long-running runtime (polycall_rpc v1) per Polycallfile, managed
 * through the unified CLI. Its settings come from the same Polycallfile
 * grammar as everything else (daemon_* keys, auth_token_env); flags
 * override them. See docs/DAEMON.md.
 *
 *   run      foreground (systemd, containers, `start` itself)
 *   start    background; returns only once the daemon answers health
 *   stop     authenticated graceful shutdown, bounded wait, optional --force
 *   status   pid / endpoint / health; exit 0 healthy, 5 not running
 *   restart  stop (if running) + start
 *
 * Single instance: an OS file lock (fcntl / exclusive CreateFile) held for
 * the daemon's lifetime. The OS drops it when the process dies however it
 * dies, so a daemon killed with SIGKILL / TerminateProcess never blocks the
 * next start: the leftover state file is detected as stale and replaced.
 *
 * Windows: `start` creates a detached background process (no console, own
 * process group). It is NOT a Windows Service; Service Control Manager
 * integration is not implemented (docs/DAEMON.md#windows).
 */

#if defined(_WIN32) && !defined(_WIN32_WINNT)
#  define _WIN32_WINNT 0x0601
#endif

#include "cli_internal.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "polycall_runtime.h"
#include "polycall_telemetry.h"
#include "../config/cfgfile.h"
#include "../config/json.h"
#include "../core/pc_buf.h"
#include "../core/pc_sys.h"
#include "../runtime/rpc_wire.h"

#if defined(_WIN32)
#  include <windows.h>
#  include <direct.h>
#else
#  include <fcntl.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/un.h>
#  include <unistd.h>
#endif

#define D_PATH 1024
#define D_MAX_LOAD 16
#define D_DEFAULT_ENDPOINT "127.0.0.1:8084"
#define D_DEFAULT_TIMEOUT_MS 15000

/* ================================================================== */
/* options                                                             */
/* ================================================================== */

typedef struct {
    const char *endpoint;
    const char *state_dir;
    const char *log_file;
    const char *log_format;       /* text | json */
    const char *auth_token_env;
    const char *polycallfile;
    const char *load_paths[D_MAX_LOAD];
    int load_count;
    long timeout_ms;
    long max_conns, drain_ms, io_ms;
    int force;
} dargs_t;

typedef struct {
    char polycallfile[D_PATH];
    char base_dir[D_PATH];
    char host[128];
    uint16_t port;
    char state_dir[D_PATH];
    char log_file[D_PATH];
    char lock_path[D_PATH];
    char state_path[D_PATH];
    char pid_path[D_PATH];
    uint32_t max_conns, drain_ms, io_ms;
    char token_env[128];
    const char *token;
} dsettings_t;

static int dusage(const polycall_invocation_t *inv, const char *m, const char *b)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char mm[300];
        snprintf(mm, sizeof mm, "%s%s%s", m, b ? ": " : "", b ? b : "");
        polycall_json_result(inv->out, "daemon", false, NULL, "usage", mm,
                             "run 'polycall help daemon'");
    } else {
        fprintf(inv->err, "polycall daemon %s: %s%s%s\n",
                inv->subcommand ? inv->subcommand : "", m, b ? ": " : "", b ? b : "");
    }
    return POLYCALL_EXIT_USAGE;
}

static int dfail(const polycall_invocation_t *inv, int exit_code, const char *code,
                 const char *msg, const char *hint)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        polycall_json_result(inv->out, "daemon", false, NULL, code, msg, hint);
    } else {
        fprintf(inv->err, "polycall daemon %s: %s\n",
                inv->subcommand ? inv->subcommand : "", msg);
        if (hint) fprintf(inv->err, "  hint: %s\n", hint);
    }
    return exit_code;
}

static int parse_long(const char *v, long lo, long hi, long *out)
{
    char *end = NULL;
    long x;
    if (!v || !*v) return -1;
    errno = 0;
    x = strtol(v, &end, 10);
    if (errno || !end || *end || x < lo || x > hi) return -1;
    *out = x;
    return 0;
}

static int scan_dargs(const polycall_invocation_t *inv, dargs_t *a, const char **bad)
{
    int i;
    memset(a, 0, sizeof *a);
    a->timeout_ms = -1;
    a->max_conns = a->drain_ms = a->io_ms = -1;
    for (i = 0; i < inv->argc; ++i) {
        const char *t = inv->argv[i];
        const char *eq = strchr(t, '=');
        const char *val = NULL;
        char name[32];
        int literal = (inv->literal_from >= 0 && i >= inv->literal_from);
        if (literal || t[0] != '-') {
            if (a->polycallfile) { *bad = t; return -1; }   /* one config file */
            a->polycallfile = t;
            continue;
        }
        if (eq) {
            size_t n = (size_t)(eq - t);
            if (n >= sizeof name) n = sizeof name - 1;
            memcpy(name, t, n);
            name[n] = '\0';
            val = eq + 1;
        } else {
            snprintf(name, sizeof name, "%s", t);
        }
#define NEEDV(o) do { if (!val) { if (i + 1 >= inv->argc) { *bad = o; return -1; } val = inv->argv[++i]; } } while (0)
        if (!strcmp(name, "--endpoint") || !strcmp(name, "-e")) {
            NEEDV("--endpoint");
            if (!polycall_cfg_valid_endpoint(val, true)) { *bad = "--endpoint (host:port)"; return -1; }
            a->endpoint = val;
        } else if (!strcmp(name, "--state-dir")) { NEEDV("--state-dir"); a->state_dir = val; }
        else if (!strcmp(name, "--log-file")) { NEEDV("--log-file"); a->log_file = val; }
        else if (!strcmp(name, "--log-format")) {
            NEEDV("--log-format");
            if (strcmp(val, "json") && strcmp(val, "text")) { *bad = "--log-format (json|text)"; return -1; }
            a->log_format = val;
        } else if (!strcmp(name, "--auth-token-env")) {
            NEEDV("--auth-token-env");
            if (!polycall_cfg_valid_env_name(val)) { *bad = "--auth-token-env"; return -1; }
            a->auth_token_env = val;
        } else if (!strcmp(name, "--load")) {
            NEEDV("--load");
            if (a->load_count >= D_MAX_LOAD) { *bad = "--load (max 16)"; return -1; }
            a->load_paths[a->load_count++] = val;
        } else if (!strcmp(name, "--timeout-ms") || !strcmp(name, "-t")) {
            NEEDV("--timeout-ms");
            if (parse_long(val, 1, 600000, &a->timeout_ms)) { *bad = "--timeout-ms (1..600000)"; return -1; }
        } else if (!strcmp(name, "--max-connections")) {
            NEEDV("--max-connections");
            if (parse_long(val, 1, 1024, &a->max_conns)) { *bad = "--max-connections (1..1024)"; return -1; }
        } else if (!strcmp(name, "--drain-timeout-ms")) {
            NEEDV("--drain-timeout-ms");
            if (parse_long(val, 1, 600000, &a->drain_ms)) { *bad = "--drain-timeout-ms (1..600000)"; return -1; }
        } else if (!strcmp(name, "--io-timeout-ms")) {
            NEEDV("--io-timeout-ms");
            if (parse_long(val, 1, 600000, &a->io_ms)) { *bad = "--io-timeout-ms (1..600000)"; return -1; }
        } else if (!strcmp(name, "--force") || !strcmp(name, "-f")) {
            if (val) { *bad = "--force takes no value"; return -1; }
            a->force = 1;
        } else {
            *bad = t;
            return -1;
        }
#undef NEEDV
    }
    return 0;
}

/* ================================================================== */
/* paths                                                               */
/* ================================================================== */

static int is_abs(const char *p)
{
#if defined(_WIN32)
    return (p[0] && p[1] == ':' && (p[2] == '\\' || p[2] == '/')) ||
           (p[0] == '\\' && p[1] == '\\') || p[0] == '/' || p[0] == '\\';
#else
    return p[0] == '/';
#endif
}

static int file_exists(const char *p)
{
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

/* out = rel when absolute (or no base), else base/rel; -1 if it won't fit */
static int join_path(char *out, size_t cap, const char *base, const char *rel)
{
    size_t lb = 0, lr = strlen(rel), need;
    int sep = 0;
    if (!is_abs(rel) && base && *base) {
        lb = strlen(base);
        sep = 1;
    }
    need = lb + (size_t)sep + lr;
    if (need >= cap) return -1;
    if (lb) memmove(out, base, lb);
#if defined(_WIN32)
    if (sep) out[lb] = '\\';
#else
    if (sep) out[lb] = '/';
#endif
    memmove(out + lb + (size_t)sep, rel, lr);
    out[need] = '\0';
    return 0;
}

static int absolutize(char *out, size_t cap, const char *p)
{
#if defined(_WIN32)
    wchar_t w[D_PATH], full[D_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, p, -1, w, D_PATH) <= 0) return -1;
    if (!GetFullPathNameW(w, D_PATH, full, NULL)) return -1;
    return WideCharToMultiByte(CP_UTF8, 0, full, -1, out, (int)cap, NULL, NULL) > 0 ? 0 : -1;
#else
    char cwd[D_PATH];
    if (is_abs(p)) return snprintf(out, cap, "%s", p) < (int)cap ? 0 : -1;
    if (!getcwd(cwd, sizeof cwd)) return -1;
    return join_path(out, cap, cwd, p);
#endif
}

static void dirname_of(char *out, size_t cap, const char *path)
{
    const char *s1 = strrchr(path, '/');
    const char *s2 = strrchr(path, '\\');
    const char *s = s1 > s2 ? s1 : s2;
    if (!s) {
        snprintf(out, cap, ".");
    } else if (s == path) {
        snprintf(out, cap, "/");
    } else {
        size_t n = (size_t)(s - path);
        if (n >= cap) n = cap - 1;
        memcpy(out, path, n);
        out[n] = '\0';
    }
}

/* mkdir -p; 0 ok, -1 error (errno-like detail in err) */
static int make_dirs(const char *path, char *err, size_t errcap)
{
    char tmp[D_PATH];
    size_t i, n = strlen(path);
    if (n == 0 || n >= sizeof tmp) {
        snprintf(err, errcap, "invalid directory path");
        return -1;
    }
    memcpy(tmp, path, n + 1);
    for (i = 1; i <= n; ++i) {
        if (tmp[i] == '/' || tmp[i] == '\\' || tmp[i] == '\0') {
            char c = tmp[i];
            tmp[i] = '\0';
#if defined(_WIN32)
            if (!(i == 2 && tmp[1] == ':')) {
                if (_mkdir(tmp) != 0 && errno != EEXIST) {
                    snprintf(err, errcap, "cannot create '%s': %s", tmp, strerror(errno));
                    return -1;
                }
            }
#else
            if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
                snprintf(err, errcap, "cannot create '%s': %s", tmp, strerror(errno));
                return -1;
            }
#endif
            tmp[i] = c;
        }
    }
    return 0;
}

/* ================================================================== */
/* settings: flags > Polycallfile > defaults                           */
/* ================================================================== */

static int resolve_settings(const polycall_invocation_t *inv, const dargs_t *a,
                            dsettings_t *s, int *exit_code)
{
    char err[512], rel[D_PATH];
    int st = 0;
    polycall_cfgfile_t *cf;
    const char *v, *ep, *tls;
    long n;

    memset(s, 0, sizeof *s);

    /* the one configuration file */
    if (a->polycallfile) {
        snprintf(rel, sizeof rel, "%s", a->polycallfile);
    } else if (inv->g->config_path) {
        snprintf(rel, sizeof rel, "%s", inv->g->config_path);
    } else if (inv->g->project_root) {
        snprintf(rel, sizeof rel, "%s/Polycallfile", inv->g->project_root);
    } else {
        snprintf(rel, sizeof rel, "Polycallfile");
    }
    if (!file_exists(rel)) {
        snprintf(err, sizeof err, "no Polycallfile at '%.400s'", rel);
        *exit_code = dfail(inv, POLYCALL_EXIT_CONFIG, "config.missing", err,
                           "pass the Polycallfile path, or --project-root DIR");
        return -1;
    }
    if (absolutize(s->polycallfile, sizeof s->polycallfile, rel) != 0) {
        *exit_code = dfail(inv, POLYCALL_EXIT_CONFIG, "config.path", "cannot resolve the Polycallfile path", NULL);
        return -1;
    }
    dirname_of(s->base_dir, sizeof s->base_dir, s->polycallfile);

    cf = polycall_cfgfile_load(s->polycallfile, 0u, &st, err, sizeof err);
    if (!cf) {
        *exit_code = dfail(inv, POLYCALL_EXIT_CONFIG, "config.invalid", err,
                           "fix the Polycallfile; 'polycall config validate' shows the same error");
        return -1;
    }
    if (!polycall_cfgfile_is_project(cf)) {
        polycall_cfgfile_free(cf);
        *exit_code = dfail(inv, POLYCALL_EXIT_CONFIG, "config.not_polycallfile",
                           "the daemon reads a project file named Polycallfile", NULL);
        return -1;
    }
    tls = polycall_cfgfile_get(cf, "tls_enabled");
    if (tls && strcmp(tls, "true") == 0) {
        polycall_cfgfile_free(cf);
        *exit_code = dfail(inv, POLYCALL_EXIT_UNSUPPORTED, "tls.unsupported",
                           "tls_enabled=true, but this build has no TLS transport; "
                           "refusing to serve plaintext under a TLS configuration",
                           "set tls_enabled=false (bind loopback, or front it with a TLS proxy)");
        return -1;
    }

    /* endpoint */
    ep = a->endpoint ? a->endpoint
       : (v = polycall_cfgfile_get(cf, "daemon_endpoint")) ? v : D_DEFAULT_ENDPOINT;
    if (pc_split_endpoint(ep, s->host, sizeof s->host, &s->port, 1) != POLYCALL_OK) {
        polycall_cfgfile_free(cf);
        *exit_code = dusage(inv, "invalid endpoint", ep);
        return -1;
    }

    /* state directory and files */
    v = a->state_dir ? a->state_dir : polycall_cfgfile_get(cf, "daemon_state_dir");
    if (join_path(s->state_dir, sizeof s->state_dir, s->base_dir, v ? v : ".polycall") != 0 ||
        join_path(s->lock_path, sizeof s->lock_path, s->state_dir, "daemon.lock") != 0 ||
        join_path(s->state_path, sizeof s->state_path, s->state_dir, "daemon.json") != 0 ||
        join_path(s->pid_path, sizeof s->pid_path, s->state_dir, "daemon.pid") != 0) {
        polycall_cfgfile_free(cf);
        *exit_code = dusage(inv, "state directory path too long", NULL);
        return -1;
    }
    v = a->log_file ? a->log_file : polycall_cfgfile_get(cf, "daemon_log_file");
    if (join_path(s->log_file, sizeof s->log_file, a->log_file ? NULL : s->state_dir,
                  v ? v : "daemon.log") != 0) {
        polycall_cfgfile_free(cf);
        *exit_code = dusage(inv, "log file path too long", NULL);
        return -1;
    }
    if (!a->log_file && v && !is_abs(v)) {
        /* a relative daemon_log_file is relative to the Polycallfile */
        join_path(s->log_file, sizeof s->log_file, s->base_dir, v);
    }

    /* limits */
    s->max_conns = 64; s->drain_ms = 5000; s->io_ms = 10000;
    if ((v = polycall_cfgfile_get(cf, "daemon_max_connections")) && !parse_long(v, 1, 1024, &n)) s->max_conns = (uint32_t)n;
    if ((v = polycall_cfgfile_get(cf, "daemon_drain_timeout_ms")) && !parse_long(v, 1, 600000, &n)) s->drain_ms = (uint32_t)n;
    if ((v = polycall_cfgfile_get(cf, "daemon_io_timeout_ms")) && !parse_long(v, 1, 600000, &n)) s->io_ms = (uint32_t)n;
    if (a->max_conns > 0) s->max_conns = (uint32_t)a->max_conns;
    if (a->drain_ms > 0) s->drain_ms = (uint32_t)a->drain_ms;
    if (a->io_ms > 0) s->io_ms = (uint32_t)a->io_ms;

    /* shared token: NAME from flag / Polycallfile, value from the env */
    v = a->auth_token_env ? a->auth_token_env : polycall_cfgfile_get(cf, "auth_token_env");
    snprintf(s->token_env, sizeof s->token_env, "%s", v ? v : "POLYCALL_DEV_TOKEN");
    s->token = getenv(s->token_env);
    if (s->token && !*s->token) s->token = NULL;
    polycall_cfgfile_free(cf);

    if (!s->token && strcmp(s->host, "127.0.0.1") != 0 && strcmp(s->host, "localhost") != 0 &&
        strncmp(s->host, "127.", 4) != 0) {
        char m[300];
        snprintf(m, sizeof m, "refusing to serve non-loopback %.100s without a shared token "
                 "(set %.100s)", s->host, s->token_env);
        *exit_code = dfail(inv, POLYCALL_EXIT_CONFIG, "auth.required", m, NULL);
        return -1;
    }
    return 0;
}

/* ================================================================== */
/* structured log                                                      */
/* ================================================================== */

static FILE *g_dlog;
static int g_dlog_json = 1;

/* dlog(level, event, message, key, value, key, value, ..., NULL)
 * values are strings; a key prefixed with '#' is emitted as a raw number */
static void dlog(const char *level, const char *event, const char *message, ...)
{
    va_list ap;
    char ts[POLYCALL_TELEMETRY_TS_LEN];
    pc_buf_t b;
    const char *k, *val;
    FILE *f = g_dlog ? g_dlog : stderr;

    polycall_telemetry_timestamp(ts);
    pc_buf_init(&b, 0);
    if (g_dlog_json) {
        pc_buf_appendf(&b, "{\"ts\":\"%s\",\"level\":\"%s\",\"event\":", ts, level);
        pc_buf_json_string(&b, event);
        pc_buf_appendf(&b, ",\"pid\":%ld", polycall_cli_getpid());
        if (message) {
            pc_buf_append(&b, ",\"message\":");
            pc_buf_json_string(&b, message);
        }
    } else {
        pc_buf_appendf(&b, "%s %-5s %s", ts, level, event);
        if (message) pc_buf_appendf(&b, " - %s", message);
    }
    va_start(ap, message);
    while ((k = va_arg(ap, const char *)) != NULL) {
        val = va_arg(ap, const char *);
        if (g_dlog_json) {
            pc_buf_append(&b, ",");
            pc_buf_json_string(&b, k[0] == '#' ? k + 1 : k);
            pc_buf_append(&b, ":");
            if (k[0] == '#') pc_buf_append(&b, val ? val : "null");
            else pc_buf_json_string(&b, val ? val : "");
        } else {
            pc_buf_appendf(&b, " %s=%s", k[0] == '#' ? k + 1 : k, val ? val : "");
        }
    }
    va_end(ap);
    pc_buf_append(&b, g_dlog_json ? "}\n" : "\n");
    if (!pc_buf_failed(&b)) {
        fwrite(b.data, 1, b.len, f);
        fflush(f);
    }
    pc_buf_free(&b);
}

/* ================================================================== */
/* single-instance lock + state file                                   */
/* ================================================================== */

typedef struct {
#if defined(_WIN32)
    HANDLE h;
#else
    int fd;
#endif
    int held;
} dlock_t;

/* 0 acquired, 1 held by another process (*holder set when known), -1 error */
static int lock_acquire(const char *path, dlock_t *l, long *holder, char *err, size_t errcap)
{
    *holder = 0;
    l->held = 0;
#if defined(_WIN32)
    {
        wchar_t w[D_PATH];
        if (MultiByteToWideChar(CP_UTF8, 0, path, -1, w, D_PATH) <= 0) return -1;
        l->h = CreateFileW(w, GENERIC_READ | GENERIC_WRITE, 0 /* exclusive */, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (l->h == INVALID_HANDLE_VALUE) {
            DWORD e = GetLastError();
            if (e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION) return 1;
            snprintf(err, errcap, "cannot open lock file '%.300s' (error %lu%s)", path,
                     (unsigned long)e, e == ERROR_ACCESS_DENIED ? ": permission denied" : "");
            return -1;
        }
        SetHandleInformation(l->h, HANDLE_FLAG_INHERIT, 0);
    }
#else
    {
        struct flock fl;
        l->fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        if (l->fd < 0) {
            snprintf(err, errcap, "cannot open lock file '%s': %s", path, strerror(errno));
            return -1;
        }
        memset(&fl, 0, sizeof fl);
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        if (fcntl(l->fd, F_SETLK, &fl) != 0) {
            if (errno == EACCES || errno == EAGAIN) {
                memset(&fl, 0, sizeof fl);
                fl.l_type = F_WRLCK;
                fl.l_whence = SEEK_SET;
                if (fcntl(l->fd, F_GETLK, &fl) == 0 && fl.l_type != F_UNLCK) {
                    *holder = (long)fl.l_pid;
                }
                close(l->fd);
                l->fd = -1;
                return 1;
            }
            snprintf(err, errcap, "cannot lock '%s': %s", path, strerror(errno));
            close(l->fd);
            l->fd = -1;
            return -1;
        }
    }
#endif
    l->held = 1;
    return 0;
}

static void lock_release(dlock_t *l)
{
    if (!l->held) return;
#if defined(_WIN32)
    CloseHandle(l->h);
#else
    close(l->fd);
#endif
    l->held = 0;
}

static int write_atomic(const char *path, const char *text)
{
    char tmp[D_PATH + 16];
    FILE *f;
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    f = fopen(tmp, "wb");
    if (!f) return -1;
    if (fputs(text, f) == EOF || fclose(f) != 0) {
        remove(tmp);
        return -1;
    }
#if defined(_WIN32)
    {
        wchar_t wf[D_PATH + 16], wt[D_PATH + 16];
        if (MultiByteToWideChar(CP_UTF8, 0, tmp, -1, wf, D_PATH + 16) <= 0 ||
            MultiByteToWideChar(CP_UTF8, 0, path, -1, wt, D_PATH + 16) <= 0 ||
            !MoveFileExW(wf, wt, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            remove(tmp);
            return -1;
        }
    }
#else
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return -1;
    }
#endif
    return 0;
}

static char *read_small_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    size_t n;
    if (!f) return NULL;
    buf = malloc(8193);
    if (!buf) { fclose(f); return NULL; }
    n = fread(buf, 1, 8192, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

typedef struct {
    long pid;
    char endpoint[160];
    int present;
} dstate_t;

static void read_state(const dsettings_t *s, dstate_t *st)
{
    char *text = read_small_file(s->state_path);
    char jerr[64];
    json_value *j;
    memset(st, 0, sizeof *st);
    if (!text) return;
    j = json_parse(text, strlen(text), jerr, sizeof jerr);
    free(text);
    if (!j) return;
    st->pid = (long)json_num(json_get(j, "pid"), 0, NULL);
    snprintf(st->endpoint, sizeof st->endpoint, "%s",
             json_str(json_get(j, "endpoint"), "", NULL));
    st->present = st->pid > 0 && st->endpoint[0];
    json_free(j);
}

static void clear_state(const dsettings_t *s)
{
    remove(s->state_path);
    remove(s->pid_path);
}

/* health over the control channel: 0 healthy (data JSON copied), else rc */
static int health_probe(const char *endpoint, uint32_t timeout_ms, char *data, size_t cap)
{
    char host[128];
    uint16_t port;
    pcr_frame_t rep;
    static const char req[] = "{\"action\":\"health\"}";
    int rc;
    if (data && cap) data[0] = '\0';
    if (pc_split_endpoint(endpoint, host, sizeof host, &port, 0) != POLYCALL_OK) return -1;
    rc = pcr_roundtrip(host, port, PCR_T_CONTROL, req, (uint32_t)(sizeof req - 1), &rep, timeout_ms);
    if (rc != 0) return rc;
    {
        char jerr[64];
        json_value *j = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
        int ok = json_bool(json_get(j, "ok"), false, NULL);
        if (ok && data && cap) {
            pc_buf_t b;
            pc_buf_init(&b, cap - 1);
            pc_buf_json_value(&b, json_get(j, "data"));
            if (!pc_buf_failed(&b)) memcpy(data, b.data, b.len + 1);
            pc_buf_free(&b);
        }
        json_free(j);
        pcr_frame_free(&rep);
        return ok ? 0 : -2;
    }
}

/* ================================================================== */
/* systemd readiness (Type=notify), no libsystemd dependency           */
/* ================================================================== */

static void sd_notify_msg(const char *msg)
{
#if defined(__linux__)
    const char *sock = getenv("NOTIFY_SOCKET");
    struct sockaddr_un sa;
    int fd;
    size_t n;
    if (!sock || (sock[0] != '/' && sock[0] != '@')) return;
    n = strlen(sock);
    if (n >= sizeof sa.sun_path) return;
    fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return;
    memset(&sa, 0, sizeof sa);
    sa.sun_family = AF_UNIX;
    memcpy(sa.sun_path, sock, n);
    if (sa.sun_path[0] == '@') sa.sun_path[0] = '\0';
    (void)sendto(fd, msg, strlen(msg), MSG_NOSIGNAL, (struct sockaddr *)&sa,
                 (socklen_t)(offsetof(struct sockaddr_un, sun_path) + n));
    close(fd);
#else
    (void)msg;
#endif
}

/* ================================================================== */
/* run (foreground)                                                    */
/* ================================================================== */

static void on_daemon_signal(int sig)
{
    (void)sig;
    polycall_runtime_request_stop();
}

#if defined(_WIN32)
static BOOL WINAPI on_console_ctrl(DWORD type)
{
    (void)type;
    polycall_runtime_request_stop();
    return TRUE;
}
#endif

typedef struct {
    const dsettings_t *s;
    char endpoint[160];
    int state_written;
} run_ctx_t;

static void POLYCALL_CALL daemon_on_bound(const char *endpoint, void *user)
{
    run_ctx_t *rc = (run_ctx_t *)user;
    char ts[POLYCALL_TELEMETRY_TS_LEN], pidbuf[32], ver[32];
    pc_buf_t b;
    snprintf(rc->endpoint, sizeof rc->endpoint, "%s", endpoint);
    polycall_telemetry_timestamp(ts);
    polycall_ffi_version(ver, (int)sizeof ver);
    pc_buf_init(&b, 0);
    pc_buf_appendf(&b, "{\"pid\":%ld,\"endpoint\":", polycall_cli_getpid());
    pc_buf_json_string(&b, endpoint);
    pc_buf_appendf(&b, ",\"started_at\":\"%s\",\"version\":\"%s\",\"polycallfile\":", ts, ver);
    pc_buf_json_string(&b, rc->s->polycallfile);
    pc_buf_append(&b, ",\"auth\":");
    pc_buf_append(&b, rc->s->token ? "true" : "false");
    pc_buf_append(&b, "}\n");
    snprintf(pidbuf, sizeof pidbuf, "%ld\n", polycall_cli_getpid());
    if (!pc_buf_failed(&b) && write_atomic(rc->s->state_path, b.data) == 0 &&
        write_atomic(rc->s->pid_path, pidbuf) == 0) {
        rc->state_written = 1;
        snprintf(pidbuf, sizeof pidbuf, "%ld", polycall_cli_getpid());
        dlog("info", "daemon.ready", "accepting connections",
             "endpoint", endpoint, "state_file", rc->s->state_path, NULL);
        sd_notify_msg("READY=1\nSTATUS=accepting connections");
    } else {
        dlog("error", "daemon.state_write_failed",
             "could not write the state file; stopping", "state_file",
             rc->s->state_path, NULL);
        polycall_runtime_request_stop();
    }
    pc_buf_free(&b);
}

int polycall_cmd_daemon_run(const polycall_invocation_t *inv)
{
    dargs_t a;
    dsettings_t s;
    const char *bad = NULL;
    int ex = 0, rc, i;
    char err[512];
    dlock_t lk;
    long holder = 0;
    polycall_runtime_t *rt;
    run_ctx_t ctx;
    FILE *logf = NULL;

    if (scan_dargs(inv, &a, &bad) != 0) return dusage(inv, "bad option", bad);
    if (resolve_settings(inv, &a, &s, &ex) != 0) return ex;

    g_dlog_json = !(a.log_format && strcmp(a.log_format, "text") == 0);
    if (a.log_file) {
        logf = fopen(s.log_file, "ab");
        if (!logf) {
            snprintf(err, sizeof err, "cannot open log file '%.400s': %.60s", s.log_file, strerror(errno));
            return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", err, NULL);
        }
        g_dlog = logf;
    } else {
        g_dlog = stderr;
    }

    if (make_dirs(s.state_dir, err, sizeof err) != 0) {
        dlog("error", "daemon.state_dir", err, NULL);
        if (logf) fclose(logf);
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", err,
                     "choose a writable --state-dir / daemon_state_dir");
    }
    rc = lock_acquire(s.lock_path, &lk, &holder, err, sizeof err);
    if (rc != 0) {
        if (rc == 1) {
            dstate_t st;
            read_state(&s, &st);
            snprintf(err, sizeof err, "a daemon for this Polycallfile is already running (pid %ld, %s)",
                     holder ? holder : st.pid, st.present ? st.endpoint : "starting");
            dlog("error", "daemon.already_running", err, NULL);
            if (logf) fclose(logf);
            return dfail(inv, POLYCALL_EXIT_RUNTIME, "daemon.already_running", err,
                         "use 'polycall daemon status' or 'stop'");
        }
        dlog("error", "daemon.lock", err, NULL);
        if (logf) fclose(logf);
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", err, NULL);
    }

    /* we own the instance; anything left by a dead predecessor is stale */
    {
        dstate_t st;
        read_state(&s, &st);
        if (st.present) {
            char p[32];
            snprintf(p, sizeof p, "%ld", st.pid);
            dlog("warn", "daemon.recovered_stale_state",
                 "previous daemon did not shut down cleanly; replacing its state",
                 "#previous_pid", p, NULL);
        }
        clear_state(&s);
    }

    {
        char port[16], mc[16];
        snprintf(port, sizeof port, "%u", (unsigned)s.port);
        snprintf(mc, sizeof mc, "%u", (unsigned)s.max_conns);
        dlog("info", "daemon.starting", NULL, "polycallfile", s.polycallfile,
             "host", s.host, "#port", port, "#max_connections", mc,
             "auth", s.token ? "token" : "none", "auth_token_env", s.token_env, NULL);
    }

    rt = polycall_runtime_create();
    if (!rt) {
        dlog("error", "daemon.oom", "out of memory", NULL);
        lock_release(&lk);
        if (logf) fclose(logf);
        return POLYCALL_EXIT_RUNTIME;
    }
    polycall_runtime_set_limits(rt, s.max_conns, s.io_ms, s.drain_ms);
    for (i = 0; i < a.load_count; ++i) {
        int prc = polycall_runtime_load_plugin(rt, a.load_paths[i], err, sizeof err);
        if (prc != POLYCALL_PLUGIN_OK) {
            dlog("error", prc == POLYCALL_PLUGIN_ABI_MISMATCH ? "daemon.plugin_abi_mismatch"
                                                             : "daemon.plugin_failed",
                 err, "plugin", a.load_paths[i], NULL);
            polycall_runtime_destroy(rt);
            lock_release(&lk);
            if (logf) fclose(logf);
            return dfail(inv, POLYCALL_EXIT_UNSUPPORTED,
                         prc == POLYCALL_PLUGIN_ABI_MISMATCH ? "plugin.abi_mismatch" : "plugin.load_failed",
                         err, NULL);
        }
        dlog("info", "daemon.plugin_loaded", NULL, "plugin", a.load_paths[i], NULL);
    }

    signal(SIGINT, on_daemon_signal);
#if defined(SIGTERM)
    signal(SIGTERM, on_daemon_signal);
#endif
#if !defined(_WIN32)
    signal(SIGHUP, SIG_IGN);     /* no configuration reload; never die on HUP */
    signal(SIGPIPE, SIG_IGN);
#else
    SetConsoleCtrlHandler(on_console_ctrl, TRUE);
#endif

    memset(&ctx, 0, sizeof ctx);
    ctx.s = &s;
    rc = polycall_runtime_serve(rt, s.host, s.port, s.token, daemon_on_bound, &ctx);

    if (rc != POLYCALL_RUNTIME_SERVE_OK) {
        const char *code = "daemon.bind_failed", *why = "could not bind the endpoint";
        int exc = POLYCALL_EXIT_TRANSPORT;
        char ep[160];
        if (rc == POLYCALL_RUNTIME_SERVE_ADDRESS_IN_USE) {
            code = "bind.address_in_use"; why = "address already in use";
        } else if (rc == POLYCALL_RUNTIME_SERVE_PERMISSION) {
            code = "bind.permission"; why = "permission denied binding the endpoint";
            exc = POLYCALL_EXIT_RUNTIME;
        } else if (rc == POLYCALL_RUNTIME_SERVE_BAD_ADDRESS) {
            code = "bind.address"; why = "the endpoint host is not a local, resolvable address";
            exc = POLYCALL_EXIT_CONFIG;
        }
        snprintf(ep, sizeof ep, "%s:%u", s.host, (unsigned)s.port);
        dlog("error", code, why, "endpoint", ep, NULL);
        polycall_runtime_destroy(rt);
        clear_state(&s);
        lock_release(&lk);
        if (logf) fclose(logf);
        snprintf(err, sizeof err, "%s: %s", ep, why);
        return dfail(inv, exc, code, err, NULL);
    }

    sd_notify_msg("STOPPING=1");
    dlog("info", "daemon.stopping", "draining connections", "endpoint", ctx.endpoint, NULL);
    polycall_runtime_destroy(rt);
    clear_state(&s);
    dlog("info", "daemon.stopped", "clean shutdown", "endpoint", ctx.endpoint, NULL);
    lock_release(&lk);
    if (logf) fclose(logf);
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* status                                                              */
/* ================================================================== */

/* 0 healthy (st filled), 1 not running, 2 running but unhealthy,
 * 3 cannot even inspect the lock (permission/IO: detail in g_probe_err) */
static char g_probe_err[512];

static int probe_daemon(const dsettings_t *s, dstate_t *st, long *holder, char *health,
                        size_t hcap)
{
    dlock_t lk;
    int rc = lock_acquire(s->lock_path, &lk, holder, g_probe_err, sizeof g_probe_err);
    read_state(s, st);
    if (rc == 0) {
        lock_release(&lk);
        return 1;                    /* nobody holds it: not running */
    }
    if (rc < 0) return 3;            /* not "held": we could not open it */
    if (!st->present) return 2;      /* held (starting?) but no state yet */
    return health_probe(st->endpoint, 3000, health, hcap) == 0 ? 0 : 2;
}

int polycall_cmd_daemon_status(const polycall_invocation_t *inv)
{
    dargs_t a;
    dsettings_t s;
    dstate_t st;
    const char *bad = NULL;
    int ex = 0, r;
    long holder = 0;
    char health[2048];

    if (scan_dargs(inv, &a, &bad) != 0) return dusage(inv, "bad option", bad);
    if (resolve_settings(inv, &a, &s, &ex) != 0) return ex;
    r = probe_daemon(&s, &st, &holder, health, sizeof health);

    if (r == 3) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", g_probe_err, NULL);
    }
    if (r == 1) {
        int stale = st.present;
        if (inv->g->format == POLYCALL_FMT_JSON) {
            char body[160];
            snprintf(body, sizeof body, "{\"running\":false,\"stale_state\":%s}",
                     stale ? "true" : "false");
            polycall_json_result(inv->out, "daemon", false, body, "daemon.not_running",
                                 "no daemon is running for this Polycallfile", NULL);
        } else {
            fprintf(inv->out, "polycall daemon: not running%s\n",
                    stale ? " (stale state from an unclean exit; 'start' replaces it)" : "");
        }
        return POLYCALL_EXIT_TRANSPORT;
    }
    if (r == 2) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            char body[256];
            snprintf(body, sizeof body, "{\"running\":true,\"healthy\":false,\"pid\":%ld}",
                     holder ? holder : st.pid);
            polycall_json_result(inv->out, "daemon", false, body, "daemon.unhealthy",
                                 "the daemon holds its lock but does not answer health", NULL);
        } else {
            fprintf(inv->out, "polycall daemon: running (pid %ld) but not answering health%s\n",
                    holder ? holder : st.pid, st.present ? "" : " (still starting?)");
        }
        return POLYCALL_EXIT_RUNTIME;
    }
    if (inv->g->format == POLYCALL_FMT_JSON) {
        pc_buf_t b;
        pc_buf_init(&b, 0);
        pc_buf_appendf(&b, "{\"running\":true,\"healthy\":true,\"pid\":%ld,\"endpoint\":", st.pid);
        pc_buf_json_string(&b, st.endpoint);
        pc_buf_append(&b, ",\"polycallfile\":");
        pc_buf_json_string(&b, s.polycallfile);
        pc_buf_append(&b, ",\"health\":");
        pc_buf_append(&b, health[0] ? health : "null");
        pc_buf_append(&b, "}");
        polycall_json_result(inv->out, "daemon", true, b.data, NULL, NULL, NULL);
        pc_buf_free(&b);
    } else {
        fprintf(inv->out, "polycall daemon: running (pid %ld) at %s\n", st.pid, st.endpoint);
        fprintf(inv->out, "  polycallfile : %s\n", s.polycallfile);
        fprintf(inv->out, "  health       : %s\n", health);
    }
    return POLYCALL_EXIT_OK;
}

/* ================================================================== */
/* start (background)                                                  */
/* ================================================================== */

static void tail_log(const polycall_invocation_t *inv, const char *path)
{
    char *t = read_small_file(path);
    if (!t) return;
    {
        size_t n = strlen(t);
        const char *p = n > 1500 ? t + n - 1500 : t;
        fprintf(inv->err, "--- last lines of %s ---\n%s", path, p);
        if (n && t[n - 1] != '\n') fputc('\n', inv->err);
    }
    free(t);
}

static int do_start(const polycall_invocation_t *inv, const dargs_t *a, const dsettings_t *s)
{
    dstate_t st;
    long holder = 0, pid = 0;
    char health[2048], err[512], self[D_PATH], ep[160];
    char mc[16], dm[16], im[16];
    const char *argv[48];
    int n = 0, i, r;
    uint64_t deadline;
    uint32_t timeout = a->timeout_ms > 0 ? (uint32_t)a->timeout_ms : D_DEFAULT_TIMEOUT_MS;

    if (make_dirs(s->state_dir, err, sizeof err) != 0) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", err,
                     "choose a writable --state-dir / daemon_state_dir");
    }
    r = probe_daemon(s, &st, &holder, health, sizeof health);
    if (r == 0) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            pc_buf_t b;
            pc_buf_init(&b, 0);
            pc_buf_appendf(&b, "{\"pid\":%ld,\"endpoint\":", st.pid);
            pc_buf_json_string(&b, st.endpoint);
            pc_buf_append(&b, ",\"already_running\":true}");
            polycall_json_result(inv->out, "daemon", true, b.data, NULL, NULL, NULL);
            pc_buf_free(&b);
        } else {
            fprintf(inv->out, "polycall daemon: already running (pid %ld) at %s\n",
                    st.pid, st.endpoint);
        }
        return POLYCALL_EXIT_OK;
    }
    if (r == 3) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", g_probe_err,
                     "make the state directory and its daemon.lock writable");
    }
    if (r == 2) {
        snprintf(err, sizeof err, "a daemon (pid %ld) holds the lock but is not healthy",
                 holder ? holder : st.pid);
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "daemon.unhealthy", err,
                     "'polycall daemon stop --force' then start again");
    }
    /* stale state from an unclean exit is left for the new daemon itself to
     * detect, log (daemon.recovered_stale_state) and replace */

    if (polycall_cli_self_path(self, sizeof self) != 0) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "daemon.self_path",
                     "cannot determine the polycall executable path", NULL);
    }
    snprintf(ep, sizeof ep, "%s:%u", s->host, (unsigned)s->port);
    snprintf(mc, sizeof mc, "%u", (unsigned)s->max_conns);
    snprintf(dm, sizeof dm, "%u", (unsigned)s->drain_ms);
    snprintf(im, sizeof im, "%u", (unsigned)s->io_ms);
    argv[n++] = self;
    argv[n++] = "daemon";
    argv[n++] = "run";
    argv[n++] = "--endpoint";        argv[n++] = ep;
    argv[n++] = "--state-dir";       argv[n++] = s->state_dir;
    argv[n++] = "--log-file";        argv[n++] = s->log_file;
    argv[n++] = "--log-format";      argv[n++] = "json";
    argv[n++] = "--auth-token-env";  argv[n++] = s->token_env;
    argv[n++] = "--max-connections"; argv[n++] = mc;
    argv[n++] = "--drain-timeout-ms"; argv[n++] = dm;
    argv[n++] = "--io-timeout-ms";   argv[n++] = im;
    for (i = 0; i < a->load_count; ++i) {
        argv[n++] = "--load";
        argv[n++] = a->load_paths[i];
    }
    argv[n++] = "--";
    argv[n++] = s->polycallfile;
    argv[n] = NULL;

    if (polycall_cli_spawn_detached(argv, s->log_file, &pid, err, sizeof err) != 0) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "daemon.spawn_failed", err, NULL);
    }

    /* readiness: the child's own state file + a healthy answer, bounded */
    deadline = pc_deadline_after(timeout);
    for (;;) {
        int code = 0;
        read_state(s, &st);
        if (st.present && st.pid == pid && health_probe(st.endpoint, 1000, health, sizeof health) == 0) {
            break;
        }
        if (polycall_cli_child_exited(pid, &code)) {
            if (inv->g->format != POLYCALL_FMT_JSON) tail_log(inv, s->log_file);
            snprintf(err, sizeof err, "the daemon exited during startup (exit %d); see %.400s",
                     code, s->log_file);
            return dfail(inv, (code > 0 && code <= 7) ? code : POLYCALL_EXIT_RUNTIME,
                         "daemon.start_failed", err, NULL);
        }
        if (pc_remaining_ms(deadline) == 0) {
            polycall_cli_kill(pid);
            clear_state(s);
            if (inv->g->format != POLYCALL_FMT_JSON) tail_log(inv, s->log_file);
            snprintf(err, sizeof err, "the daemon did not become ready within %u ms "
                     "(stopped it); see %.400s", (unsigned)timeout, s->log_file);
            return dfail(inv, POLYCALL_EXIT_DEADLINE, "daemon.not_ready", err, NULL);
        }
        pc_sleep_ms(100);
    }

    if (inv->g->format == POLYCALL_FMT_JSON) {
        pc_buf_t b;
        pc_buf_init(&b, 0);
        pc_buf_appendf(&b, "{\"pid\":%ld,\"endpoint\":", st.pid);
        pc_buf_json_string(&b, st.endpoint);
        pc_buf_append(&b, ",\"log_file\":");
        pc_buf_json_string(&b, s->log_file);
        pc_buf_append(&b, ",\"already_running\":false}");
        polycall_json_result(inv->out, "daemon", true, b.data, NULL, NULL, NULL);
        pc_buf_free(&b);
    } else {
        fprintf(inv->out, "polycall daemon: started (pid %ld) at %s\n", st.pid, st.endpoint);
        fprintf(inv->out, "  log: %s\n", s->log_file);
    }
    return POLYCALL_EXIT_OK;
}

int polycall_cmd_daemon_start(const polycall_invocation_t *inv)
{
    dargs_t a;
    dsettings_t s;
    const char *bad = NULL;
    int ex = 0;
    if (scan_dargs(inv, &a, &bad) != 0) return dusage(inv, "bad option", bad);
    if (resolve_settings(inv, &a, &s, &ex) != 0) return ex;
    return do_start(inv, &a, &s);
}

/* ================================================================== */
/* stop                                                                */
/* ================================================================== */

static int do_stop(const polycall_invocation_t *inv, const dargs_t *a, const dsettings_t *s,
                   int quiet_when_not_running)
{
    dstate_t st;
    long holder = 0, pid;
    char health[64], err[512], host[128];
    uint16_t port;
    int r, refused = 0;
    pc_buf_t body;
    pcr_frame_t rep;
    uint64_t deadline;
    uint32_t timeout = a->timeout_ms > 0 ? (uint32_t)a->timeout_ms
                                         : (s->drain_ms + s->io_ms + 5000u);

    r = probe_daemon(s, &st, &holder, health, sizeof health);
    if (r == 3) {
        return dfail(inv, POLYCALL_EXIT_RUNTIME, "permission.denied", g_probe_err, NULL);
    }
    if (r == 1) {
        if (st.present) clear_state(s);
        if (!quiet_when_not_running) {
            if (inv->g->format == POLYCALL_FMT_JSON) {
                polycall_json_result(inv->out, "daemon", true,
                                     "{\"stopped\":false,\"was_running\":false}", NULL, NULL, NULL);
            } else {
                fprintf(inv->out, "polycall daemon: not running%s\n",
                        st.present ? " (removed stale state)" : "");
            }
        }
        return POLYCALL_EXIT_OK;
    }
    pid = holder ? holder : st.pid;

    if (st.present && pc_split_endpoint(st.endpoint, host, sizeof host, &port, 0) == POLYCALL_OK) {
        pc_buf_init(&body, 0);
        pc_buf_append(&body, "{\"action\":\"shutdown\"");
        if (s->token) {
            pc_buf_append(&body, ",\"auth_token\":");
            pc_buf_json_string(&body, s->token);
        }
        pc_buf_append(&body, "}");
        if (!pc_buf_failed(&body) &&
            pcr_roundtrip(host, port, PCR_T_CONTROL, body.data, (uint32_t)body.len, &rep, 3000) == 0) {
            char jerr[64];
            json_value *j = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
            const char *code = json_str(json_get(json_get(j, "error"), "code"), "", NULL);
            if (!json_bool(json_get(j, "ok"), false, NULL)) {
                refused = !strcmp(code, "auth.denied") ? 2 : 1;
            }
            json_free(j);
            pcr_frame_free(&rep);
        }
        pc_buf_free(&body);
    }
    if (refused == 2 && !a->force) {
        snprintf(err, sizeof err, "the daemon refused shutdown: wrong or missing token (%s)",
                 s->token_env);
        return dfail(inv, POLYCALL_EXIT_AUTH, "auth.denied", err,
                     "export the daemon's token in that variable, or use --force");
    }

    /* wait for the process to go (its lock disappears with it) */
    deadline = pc_deadline_after(timeout);
    for (;;) {
        dlock_t lk;
        long h2 = 0;
        char e2[512];
        int lr = lock_acquire(s->lock_path, &lk, &h2, e2, sizeof e2);
        if (lr == 0) { lock_release(&lk); break; }
        if (pc_remaining_ms(deadline) == 0) {
            if (a->force && pid > 0) {
                polycall_cli_kill(pid);
                clear_state(s);
                if (inv->g->format == POLYCALL_FMT_JSON) {
                    char b2[96];
                    snprintf(b2, sizeof b2, "{\"stopped\":true,\"forced\":true,\"pid\":%ld}", pid);
                    polycall_json_result(inv->out, "daemon", true, b2, NULL, NULL, NULL);
                } else {
                    fprintf(inv->out, "polycall daemon: forced termination of pid %ld "
                            "after %u ms\n", pid, (unsigned)timeout);
                }
                return POLYCALL_EXIT_OK;
            }
            snprintf(err, sizeof err, "pid %ld did not stop within %u ms", pid, (unsigned)timeout);
            return dfail(inv, POLYCALL_EXIT_DEADLINE, "daemon.stop_timeout", err,
                         "retry with --force to terminate it");
        }
        pc_sleep_ms(100);
    }
    clear_state(s);
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char b2[96];
        snprintf(b2, sizeof b2, "{\"stopped\":true,\"forced\":false,\"pid\":%ld}", pid);
        polycall_json_result(inv->out, "daemon", true, b2, NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "polycall daemon: stopped (pid %ld)\n", pid);
    }
    return POLYCALL_EXIT_OK;
}

int polycall_cmd_daemon_stop(const polycall_invocation_t *inv)
{
    dargs_t a;
    dsettings_t s;
    const char *bad = NULL;
    int ex = 0;
    if (scan_dargs(inv, &a, &bad) != 0) return dusage(inv, "bad option", bad);
    if (resolve_settings(inv, &a, &s, &ex) != 0) return ex;
    return do_stop(inv, &a, &s, 0);
}

int polycall_cmd_daemon_restart(const polycall_invocation_t *inv)
{
    dargs_t a;
    dsettings_t s;
    const char *bad = NULL;
    int ex = 0, rc;
    polycall_invocation_t quiet = *inv;
    if (scan_dargs(inv, &a, &bad) != 0) return dusage(inv, "bad option", bad);
    if (resolve_settings(inv, &a, &s, &ex) != 0) return ex;
    /* stop's success line is suppressed in JSON mode: one object per run */
    if (inv->g->format == POLYCALL_FMT_JSON) {
        static FILE *devnull;
        if (!devnull) devnull = fopen(
#if defined(_WIN32)
            "NUL",
#else
            "/dev/null",
#endif
            "w");
        if (devnull) quiet.out = devnull;
    }
    rc = do_stop(&quiet, &a, &s, 1);
    if (rc != POLYCALL_EXIT_OK) return rc;
    return do_start(inv, &a, &s);
}
