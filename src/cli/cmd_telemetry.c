/*
 * telemetry emit / show / status -- GUID + timestamp correlated structured
 * events (see include/polycall_telemetry.h). `emit` is also how an external
 * script or a loaded operation plugin can log into the same sink the
 * runtime uses for its own run/call lifecycle events (cmd_runtime.c).
 */

#include "cli_internal.h"

#include <stdlib.h>
#include <string.h>

#include "polycall_telemetry.h"
#include "../config/json.h"

typedef struct {
    const char *event;
    const char *service;
    const char *operation;
    const char *status;
    const char *detail;
    const char *correlation_id;
    long limit;
    const char *pos[4];
    int npos;
} targs_t;

static int parse_targs(const polycall_invocation_t *inv, targs_t *a, const char **bad)
{
    int i;
    memset(a, 0, sizeof *a);
    a->limit = -1;
    for (i = 0; i < inv->argc; ++i) {
        const char *t = inv->argv[i];
        const char *eq = strchr(t, '=');
        const char *val = NULL;
        char name[32];
        bool literal = (inv->literal_from >= 0 && i >= inv->literal_from);

        if (literal || t[0] != '-') {
            if (a->npos < (int)(sizeof a->pos / sizeof a->pos[0])) {
                a->pos[a->npos++] = t;
            }
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
        #define NEED_VAL(opt) do { \
            if (!val) { if (i + 1 >= inv->argc) { *bad = opt; return -1; } \
                        val = inv->argv[++i]; } } while (0)

        if (!strcmp(name, "--event"))          { NEED_VAL("--event");          a->event = val; }
        else if (!strcmp(name, "--service"))   { NEED_VAL("--service");        a->service = val; }
        else if (!strcmp(name, "--operation")) { NEED_VAL("--operation");      a->operation = val; }
        else if (!strcmp(name, "--status"))    { NEED_VAL("--status");         a->status = val; }
        else if (!strcmp(name, "--detail"))    { NEED_VAL("--detail");         a->detail = val; }
        else if (!strcmp(name, "--correlation-id")) { NEED_VAL("--correlation-id"); a->correlation_id = val; }
        else if (!strcmp(name, "--limit"))     { NEED_VAL("--limit"); a->limit = strtol(val, NULL, 10); }
        else { *bad = t; return -1; }
        #undef NEED_VAL
    }
    return 0;
}

static int tusage(const polycall_invocation_t *inv, const char *msg, const char *bad)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char m[256];
        snprintf(m, sizeof m, "%s%s%s", msg, bad ? ": " : "", bad ? bad : "");
        polycall_json_result(inv->out, inv->command, false, NULL, "usage", m, NULL);
    } else {
        fprintf(inv->err, "polycall telemetry: %s%s%s\n", msg,
                bad ? ": " : "", bad ? bad : "");
    }
    return POLYCALL_EXIT_USAGE;
}

/* ---- telemetry emit ---------------------------------------------------- */

int polycall_cmd_telemetry_emit(const polycall_invocation_t *inv)
{
    targs_t a;
    const char *bad = NULL;
    polycall_telemetry_event_t ev;
    char guid[POLYCALL_TELEMETRY_GUID_LEN];
    const char *corr;

    if (parse_targs(inv, &a, &bad) != 0) {
        return tusage(inv, "bad option", bad);
    }
    if (!a.event || !a.event[0]) {
        return tusage(inv, "emit requires --event NAME", NULL);
    }

    if (a.correlation_id && a.correlation_id[0]) {
        corr = a.correlation_id;
    } else {
        polycall_telemetry_new_guid(guid);
        corr = guid;
    }

    memset(&ev, 0, sizeof ev);
    ev.event = a.event;
    ev.correlation_id = corr;
    ev.service = a.service;
    ev.operation = a.operation;
    ev.status = a.status;
    ev.detail = a.detail;
    ev.duration_ns = -1;

    polycall_telemetry_emit(inv->g->project_root, &ev);

    if (inv->g->format == POLYCALL_FMT_JSON) {
        char body[256], qc[48];
        snprintf(body, sizeof body, "{\"correlation_id\":%s}",
                 polycall_json_quote(qc, sizeof qc, corr));
        polycall_json_result(inv->out, "telemetry", true, body, NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "telemetry: emitted '%s' (correlation_id %s)\n",
                a.event, corr);
    }
    return POLYCALL_EXIT_OK;
}

/* ---- telemetry status --------------------------------------------------- */

int polycall_cmd_telemetry_status(const polycall_invocation_t *inv)
{
    char path[POLYCALL_TELEMETRY_PATH_MAX];
    bool enabled = polycall_telemetry_enabled();
    bool exists = false;
    long size = 0;
    FILE *f;

    polycall_telemetry_sink_path(inv->g->project_root, path, sizeof path);
    f = fopen(path, "rb");
    if (f) {
        exists = true;
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        if (size < 0) size = 0;
        fclose(f);
    }

    if (inv->g->format == POLYCALL_FMT_JSON) {
        char body[POLYCALL_TELEMETRY_PATH_MAX + 128], qp[POLYCALL_TELEMETRY_PATH_MAX];
        snprintf(body, sizeof body,
                 "{\"enabled\":%s,\"sink\":%s,\"sink_exists\":%s,\"sink_bytes\":%ld}",
                 enabled ? "true" : "false",
                 polycall_json_quote(qp, sizeof qp, path),
                 exists ? "true" : "false", size);
        polycall_json_result(inv->out, "telemetry", true, body, NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "telemetry status\n");
        fprintf(inv->out, "  enabled : %s%s\n", enabled ? "yes" : "no",
                enabled ? "" : "  (POLYCALL_TELEMETRY=off)");
        fprintf(inv->out, "  sink    : %s%s\n", path,
                exists ? "" : "  (not yet created)");
        if (exists) {
            fprintf(inv->out, "  size    : %ld bytes\n", size);
        }
    }
    return POLYCALL_EXIT_OK;
}

/* ---- telemetry show ------------------------------------------------------ */

/* Read up to the last TEL_SHOW_WINDOW bytes of the sink (dropping a leading
 * partial line), rather than the whole file -- a tail, not a full parse, of
 * what may be a long-lived append-only log. */
#define TEL_SHOW_WINDOW (1u << 20)

static char *read_tail(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    long fsize, start;
    char *buf;
    size_t n;

    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    if (fsize < 0) { fclose(f); return NULL; }
    start = (fsize > (long)TEL_SHOW_WINDOW) ? fsize - (long)TEL_SHOW_WINDOW : 0;
    fseek(f, start, SEEK_SET);
    buf = malloc((size_t)(fsize - start) + 1);
    if (!buf) { fclose(f); return NULL; }
    n = fread(buf, 1, (size_t)(fsize - start), f);
    fclose(f);
    buf[n] = '\0';
    if (start > 0) {
        /* the window may start mid-line; drop that partial line */
        char *nl = memchr(buf, '\n', n);
        if (nl) {
            size_t drop = (size_t)(nl - buf) + 1;
            memmove(buf, nl + 1, n - drop);
            n -= drop;
            buf[n] = '\0';
        }
    }
    *len = n;
    return buf;
}

#define TEL_SHOW_MAX_LINES 4096

int polycall_cmd_telemetry_show(const polycall_invocation_t *inv)
{
    targs_t a;
    const char *bad = NULL;
    char path[POLYCALL_TELEMETRY_PATH_MAX];
    char *buf;
    size_t len;
    const char *lines[TEL_SHOW_MAX_LINES];   /* 4096 * 16B = 64 KiB, fine on-stack */
    size_t line_lens[TEL_SHOW_MAX_LINES];
    size_t nlines = 0;
    long limit;
    size_t first, i;

    if (parse_targs(inv, &a, &bad) != 0) {
        return tusage(inv, "bad option", bad);
    }
    limit = (a.limit >= 0) ? a.limit : 20;
    if (limit < 0) limit = 0;

    polycall_telemetry_sink_path(inv->g->project_root, path, sizeof path);
    buf = read_tail(path, &len);
    if (!buf) {
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "telemetry", true, "[]", NULL, NULL, NULL);
        } else {
            fprintf(inv->out, "telemetry: no events recorded yet (%s)\n", path);
        }
        return POLYCALL_EXIT_OK;
    }

    {
        char *p = buf;
        char *end = buf + len;
        while (p < end && nlines < TEL_SHOW_MAX_LINES) {
            char *nl = memchr(p, '\n', (size_t)(end - p));
            size_t linelen = nl ? (size_t)(nl - p) : (size_t)(end - p);
            if (linelen > 0) {
                lines[nlines] = p;
                line_lens[nlines] = linelen;
                ++nlines;
            }
            if (!nl) break;
            p = nl + 1;
        }
    }

    first = (nlines > (size_t)limit) ? nlines - (size_t)limit : 0;

    if (inv->g->format == POLYCALL_FMT_JSON) {
        /* each stored line is already a complete JSON object (written by
         * polycall_telemetry_emit); join the tail window into one array */
        size_t cap = len + 8, w = 0;
        char *arr = malloc(cap);
        if (!arr) { free(buf); return POLYCALL_EXIT_RUNTIME; }
        arr[w++] = '[';
        for (i = first; i < nlines; ++i) {
            if (i > first) arr[w++] = ',';
            memcpy(arr + w, lines[i], line_lens[i]);
            w += line_lens[i];
        }
        arr[w++] = ']';
        arr[w] = '\0';
        polycall_json_result(inv->out, "telemetry", true, arr, NULL, NULL, NULL);
        free(arr);
    } else {
        if (first == nlines) {
            fprintf(inv->out, "telemetry: no events recorded yet (%s)\n", path);
        }
        for (i = first; i < nlines; ++i) {
            char jerr[64];
            json_value *v = json_parse(lines[i], line_lens[i], jerr, sizeof jerr);
            const char *ts = json_str(json_get(v, "ts"), "?", NULL);
            const char *ev = json_str(json_get(v, "event"), "?", NULL);
            const char *corr = json_str(json_get(v, "correlation_id"), "?", NULL);
            const char *svc = json_str(json_get(v, "service"), "", NULL);
            const char *op = json_str(json_get(v, "operation"), "", NULL);
            const char *st = json_str(json_get(v, "status"), "", NULL);
            bool has_dur = false;
            double dur = json_num(json_get(v, "duration_ns"), 0, &has_dur);

            fprintf(inv->out, "%s  %-36s  %-16s", ts, corr, ev);
            if (svc[0] || op[0]) fprintf(inv->out, "  %s.%s", svc, op);
            if (st[0]) fprintf(inv->out, "  status=%s", st);
            if (has_dur) fprintf(inv->out, "  %.3fms", dur / 1000000.0);
            fputc('\n', inv->out);
            json_free(v);
        }
    }
    free(buf);
    return POLYCALL_EXIT_OK;
}
