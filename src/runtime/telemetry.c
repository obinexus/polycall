/*
 * Telemetry sink: GUID + timestamp correlated JSONL events (see
 * include/polycall_telemetry.h). Self-contained -- does not reach into the
 * CLI's JSON helpers (src/cli/cli_json.c), since this file also links into
 * the core library, which the CLI layer does not.
 */

#if defined(_WIN32)
#define _CRT_RAND_S /* declares rand_s() from <stdlib.h> */
#endif

#include "polycall_telemetry.h"
#include "polycall_pthread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/* ---- string escaping (mirrors src/cli/cli_json.c's polycall_json_quote,
 * duplicated rather than shared: this translation unit also builds into the
 * core library, which does not link the CLI layer) ---------------------- */

static char *tel_json_quote(char *buf, size_t buf_len, const char *s)
{
    size_t w = 0;
    if (buf_len == 0) {
        return buf;
    }
    if (s == NULL) {
        s = "";
    }
    buf[w++] = '"';
    for (; *s && w + 7 < buf_len; ++s) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  buf[w++] = '\\'; buf[w++] = '"';  break;
        case '\\': buf[w++] = '\\'; buf[w++] = '\\'; break;
        case '\b': buf[w++] = '\\'; buf[w++] = 'b';  break;
        case '\f': buf[w++] = '\\'; buf[w++] = 'f';  break;
        case '\n': buf[w++] = '\\'; buf[w++] = 'n';  break;
        case '\r': buf[w++] = '\\'; buf[w++] = 'r';  break;
        case '\t': buf[w++] = '\\'; buf[w++] = 't';  break;
        default:
            if (c < 0x20) {
                static const char hex[] = "0123456789abcdef";
                buf[w++] = '\\'; buf[w++] = 'u'; buf[w++] = '0'; buf[w++] = '0';
                buf[w++] = hex[(c >> 4) & 0xF];
                buf[w++] = hex[c & 0xF];
            } else {
                buf[w++] = (char)c; /* pass UTF-8 bytes through untouched */
            }
        }
    }
    if (w + 1 < buf_len) {
        buf[w++] = '"';
    }
    buf[w < buf_len ? w : buf_len - 1] = '\0';
    return buf;
}

/* ---- clocks ------------------------------------------------------------ */

uint64_t POLYCALL_CALL polycall_telemetry_mono_ns(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static bool have_freq = false;
    LARGE_INTEGER now;
    if (!have_freq) {
        QueryPerformanceFrequency(&freq);
        have_freq = true;
    }
    QueryPerformanceCounter(&now);
    return (uint64_t)((double)now.QuadPart * (1000000000.0 / (double)freq.QuadPart));
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

void POLYCALL_CALL polycall_telemetry_timestamp(char out[POLYCALL_TELEMETRY_TS_LEN])
{
    /* C11 timespec_get()/TIME_UTC would be the portable choice, but not
     * every C11-claiming toolchain's runtime headers actually declare it
     * (seen with an older MinGW runtime under -std=gnu11) -- use the same
     * explicit per-platform clock split as polycall_telemetry_mono_ns(). */
#if defined(_WIN32)
    SYSTEMTIME st;
    GetSystemTime(&st); /* already UTC */
    snprintf(out, POLYCALL_TELEMETRY_TS_LEN,
             "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    struct timespec ts;
    struct tm tm_utc;
    long ms;
    clock_gettime(CLOCK_REALTIME, &ts);
    ms = ts.tv_nsec / 1000000;
    gmtime_r(&ts.tv_sec, &tm_utc);
    snprintf(out, POLYCALL_TELEMETRY_TS_LEN,
             "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
             tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
             tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec, ms);
#endif
}

/* ---- GUID ---------------------------------------------------------------
 * RFC 4122 version 4: 122 random bits, version nibble fixed to 4, variant
 * bits fixed to 10xx. Source is a CSPRNG (rand_s on Windows, /dev/urandom on
 * POSIX); the non-cryptographic fallback below only ever runs if that source
 * is unavailable, which does not happen in normal operation. */

void POLYCALL_CALL polycall_telemetry_new_guid(char out[POLYCALL_TELEMETRY_GUID_LEN])
{
    unsigned char b[16];
    size_t i;

#if defined(_WIN32)
    for (i = 0; i < 16; i += 4) {
        unsigned int r = 0;
        rand_s(&r);
        memcpy(b + i, &r, 4);
    }
#else
    {
        FILE *f = fopen("/dev/urandom", "rb");
        size_t got = f ? fread(b, 1, sizeof b, f) : 0;
        if (f) {
            fclose(f);
        }
        if (got != sizeof b) {
            uint64_t seed = (uint64_t)time(NULL)
                           ^ (uint64_t)(uintptr_t)&b
                           ^ polycall_telemetry_mono_ns();
            for (i = 0; i < 16; ++i) {
                seed = seed * 6364136223846793005ull + 1442695040888963407ull;
                b[i] = (unsigned char)(seed >> 33);
            }
        }
    }
#endif

    b[6] = (unsigned char)((b[6] & 0x0Fu) | 0x40u); /* version 4 */
    b[8] = (unsigned char)((b[8] & 0x3Fu) | 0x80u); /* variant 10xx */

    snprintf(out, POLYCALL_TELEMETRY_GUID_LEN,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
             b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
}

/* ---- enable / sink path -------------------------------------------------- */

static bool tel_ieq(const char *a, const char *b)
{
    for (; *a && *b; ++a, ++b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
    }
    return *a == *b;
}

bool POLYCALL_CALL polycall_telemetry_enabled(void)
{
    const char *v = getenv("POLYCALL_TELEMETRY");
    if (!v || !*v) {
        return true;
    }
    return !(tel_ieq(v, "off") || tel_ieq(v, "0") || tel_ieq(v, "false") ||
             tel_ieq(v, "no"));
}

void POLYCALL_CALL polycall_telemetry_sink_path(const char *project_root,
                                                char *buf, size_t buf_cap)
{
    const char *env = getenv("POLYCALL_TELEMETRY_LOG");
    if (env && *env) {
        snprintf(buf, buf_cap, "%s", env);
        return;
    }
    snprintf(buf, buf_cap, "%s/.polycall/telemetry.jsonl",
             (project_root && *project_root) ? project_root : ".");
}

/* Create the sink's immediate parent directory if missing. Best-effort: a
 * failure here just means the fopen() below also fails, which emit()
 * already treats as a silent no-op. */
static void tel_ensure_parent_dir(const char *path)
{
    char dir[POLYCALL_TELEMETRY_PATH_MAX];
    char *slash;

    snprintf(dir, sizeof dir, "%s", path);
    slash = strrchr(dir, '/');
#if defined(_WIN32)
    {
        char *bslash = strrchr(dir, '\\');
        if (!slash || (bslash && bslash > slash)) {
            slash = bslash;
        }
    }
#endif
    if (!slash || slash == dir) {
        return;
    }
    *slash = '\0';
#if defined(_WIN32)
    _mkdir(dir);
#else
    mkdir(dir, 0755);
#endif
}

/* ---- emit ---------------------------------------------------------------- */

static pthread_mutex_t g_tel_mutex;
static pthread_once_t g_tel_once = PTHREAD_ONCE_INIT;

static void tel_mutex_init(void)
{
    pthread_mutex_init(&g_tel_mutex, NULL);
}

void POLYCALL_CALL polycall_telemetry_emit(const char *project_root,
                                           const polycall_telemetry_event_t *ev)
{
    char path[POLYCALL_TELEMETRY_PATH_MAX];
    char guid_buf[POLYCALL_TELEMETRY_GUID_LEN];
    char ts[POLYCALL_TELEMETRY_TS_LEN];
    const char *corr;
    char qevent[160], qcorr[48], qservice[80], qoperation[80], qstatus[40],
         qdetail[320];
    char line[1024];
    int n;
    FILE *f;

    if (!ev || !ev->event || !ev->event[0] || !polycall_telemetry_enabled()) {
        return;
    }

    if (ev->correlation_id && ev->correlation_id[0]) {
        corr = ev->correlation_id;
    } else {
        polycall_telemetry_new_guid(guid_buf);
        corr = guid_buf;
    }
    polycall_telemetry_timestamp(ts);
    polycall_telemetry_sink_path(project_root, path, sizeof path);

    n = snprintf(line, sizeof line,
                 "{\"schema_version\":1,\"ts\":\"%s\",\"correlation_id\":%s,"
                 "\"event\":%s",
                 ts, tel_json_quote(qcorr, sizeof qcorr, corr),
                 tel_json_quote(qevent, sizeof qevent, ev->event));
    if (n < 0 || (size_t)n >= sizeof line) {
        return;
    }

    if (ev->service && ev->service[0]) {
        n += snprintf(line + n, sizeof line - (size_t)n, ",\"service\":%s",
                      tel_json_quote(qservice, sizeof qservice, ev->service));
        if (n < 0 || (size_t)n >= sizeof line) return;
    }
    if (ev->operation && ev->operation[0]) {
        n += snprintf(line + n, sizeof line - (size_t)n, ",\"operation\":%s",
                      tel_json_quote(qoperation, sizeof qoperation, ev->operation));
        if (n < 0 || (size_t)n >= sizeof line) return;
    }
    if (ev->status && ev->status[0]) {
        n += snprintf(line + n, sizeof line - (size_t)n, ",\"status\":%s",
                      tel_json_quote(qstatus, sizeof qstatus, ev->status));
        if (n < 0 || (size_t)n >= sizeof line) return;
    }
    if (ev->duration_ns >= 0) {
        n += snprintf(line + n, sizeof line - (size_t)n, ",\"duration_ns\":%lld",
                      (long long)ev->duration_ns);
        if (n < 0 || (size_t)n >= sizeof line) return;
    }
    if (ev->detail && ev->detail[0]) {
        n += snprintf(line + n, sizeof line - (size_t)n, ",\"detail\":%s",
                      tel_json_quote(qdetail, sizeof qdetail, ev->detail));
        if (n < 0 || (size_t)n >= sizeof line) return;
    }
    if ((size_t)n + 2 >= sizeof line) {
        return;
    }
    line[n++] = '}';
    line[n++] = '\n';

    tel_ensure_parent_dir(path);

    pthread_once(&g_tel_once, tel_mutex_init);
    pthread_mutex_lock(&g_tel_mutex);
    f = fopen(path, "ab");
    if (f) {
        fwrite(line, 1, (size_t)n, f);
        fclose(f);
    }
    pthread_mutex_unlock(&g_tel_mutex);
}
