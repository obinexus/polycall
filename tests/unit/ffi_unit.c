/*
 * Binding ABI v1 unit tests. Uses ONLY the installed public header and the
 * shared library, exactly as a language binding would (no internal header).
 * Two peer nodes here live in one process and talk over loopback TCP; the
 * two-PROCESS exchange is tests/peer/p2p.sh.
 *
 * Prints PASS/FAIL lines; exit 0 only when every check passed.
 */

#include <polycall.h>
#include <polycall_runtime.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#  include <process.h>
typedef HANDLE thr_t;
typedef SOCKET sock_t;
#  define BAD_SOCK INVALID_SOCKET
#  define CLOSESOCK closesocket
static void msleep(unsigned ms) { Sleep(ms); }
static uint64_t now_ms(void) { return GetTickCount64(); }
static int thr_start(thr_t *t, unsigned (__stdcall *fn)(void *), void *a)
{
    uintptr_t h = _beginthreadex(NULL, 0, fn, a, 0, NULL);
    if (!h) return -1;
    *t = (HANDLE)h;
    return 0;
}
static void thr_join(thr_t t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
#  define THR_RET unsigned __stdcall
#  define THR_RETURN return 0
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <pthread.h>
#  include <sys/socket.h>
#  include <time.h>
#  include <unistd.h>
typedef pthread_t thr_t;
typedef int sock_t;
#  define BAD_SOCK (-1)
#  define CLOSESOCK close
static void msleep(unsigned ms)
{
    struct timespec ts = { (time_t)(ms / 1000), (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}
static int thr_start(thr_t *t, void *(*fn)(void *), void *a)
{
    return pthread_create(t, NULL, fn, a) == 0 ? 0 : -1;
}
static void thr_join(thr_t t) { pthread_join(t, NULL); }
#  define THR_RET void *
#  define THR_RETURN return NULL
#endif

static int g_pass, g_fail;

/* release/acquire flag for cross-thread handshakes in these tests */
static void flag_set(volatile int *p)
{
#if defined(_WIN32)
    InterlockedExchange((volatile LONG *)p, 1);
#else
    __atomic_store_n(p, 1, __ATOMIC_RELEASE);
#endif
}
static int flag_get(volatile int *p)
{
#if defined(_WIN32)
    return (int)InterlockedCompareExchange((volatile LONG *)p, 0, 0);
#else
    return __atomic_load_n(p, __ATOMIC_ACQUIRE);
#endif
}

#define CHECK(cond, ...) do { \
    if (cond) { g_pass++; printf("PASS  "); printf(__VA_ARGS__); printf("\n"); } \
    else { g_fail++; printf("FAIL  "); printf(__VA_ARGS__); \
           printf("  [%s:%d]\n", __FILE__, __LINE__); } \
    fflush(stdout); } while (0)

#define EXPECT_RC(expr, want, what) do { int _rc = (expr); \
    CHECK(_rc == (want), "%s -> %s (got %d %s)", what, #want, _rc, polycall_strerror(_rc)); \
    } while (0)

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    if (f) { fputs(text, f); fclose(f); }
}

static char g_tmp[512];
static void tmp_path(char *out, size_t cap, const char *name)
{
    snprintf(out, cap, "%s/%s", g_tmp, name);
}

/* ===================================================================== */

static void test_library(void)
{
    char buf[64], err[256];
    int n, i;
    printf("\n### library\n");
    CHECK(polycall_ffi_abi_version() == POLYCALL_FFI_ABI_VERSION,
          "ABI version is %d", POLYCALL_FFI_ABI_VERSION);
    n = polycall_ffi_version(buf, (int)sizeof buf);
    CHECK(n == (int)strlen(POLYCALL_ABI_VERSION_STRING) &&
          strcmp(buf, POLYCALL_ABI_VERSION_STRING) == 0,
          "polycall_ffi_version == header version %s (got '%s')", POLYCALL_ABI_VERSION_STRING, buf);
    CHECK(strcmp(polycall_get_version(), buf) == 0, "polycall_get_version agrees");
    n = polycall_ffi_version(NULL, 0);
    CHECK(n == (int)strlen(POLYCALL_ABI_VERSION_STRING), "version size query with NULL,0 -> %d", n);
    n = polycall_ffi_version(buf, 2);
    CHECK(n == (int)strlen(POLYCALL_ABI_VERSION_STRING) && buf[1] == '\0' && strlen(buf) == 1,
          "version into 2-byte buffer truncates with NUL, returns full length");
    EXPECT_RC(polycall_ffi_version(buf, -1), POLYCALL_E_INVALID_ARGUMENT, "version len < 0");
    EXPECT_RC(polycall_ffi_version(NULL, 5), POLYCALL_E_INVALID_ARGUMENT, "version NULL buf with len > 0");
    for (i = 0; i >= -18; --i) {
        const char *s = polycall_strerror(i);
        if (!s || strncmp(s, "POLYCALL_", 9) != 0) break;
    }
    CHECK(i < -18, "polycall_strerror names every code 0..-18");
    CHECK(strstr(polycall_strerror(-999), "UNKNOWN") != NULL, "unknown status -> UNKNOWN");
    polycall_peer_close(0);
    n = polycall_last_error(err, sizeof err);
    CHECK(n > 0 && strstr(err, "handle"), "last_error carries the failure detail ('%s')", err);
    polycall_ffi_abi_version();
    polycall_peer_list(0, NULL, 0, NULL);
    EXPECT_RC(polycall_ffi_run_config(NULL, 0), POLYCALL_E_INVALID_ARGUMENT, "run_config(NULL)");
}

static void test_config(void)
{
    char p[600], q[600], buf[4096], err[512];
    int n;
    printf("\n### configuration (polycall_ffi_run_config / describe)\n");
    tmp_path(p, sizeof p, "cpp-polycallrc");
    write_file(p, "# binding override\nlog_level=info\nmax_connections=1000\n"
                  "network_timeout=5000\ntls_enabled=false\n");
    EXPECT_RC(polycall_ffi_run_config(p, 1), POLYCALL_OK, "binding rc file, run=1");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_OK, "binding rc file, run=0");
    EXPECT_RC(polycall_ffi_run_config("", 1), POLYCALL_E_INVALID_ARGUMENT, "run_config(\"\")");
    tmp_path(q, sizeof q, "does-not-exist-polycallrc");
    EXPECT_RC(polycall_ffi_run_config(q, 1), POLYCALL_E_NOT_FOUND, "missing file");

    tmp_path(p, sizeof p, "bad-polycallrc");
    write_file(p, "max_connections=lots\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_E_CONFIG, "non-integer max_connections");
    polycall_last_error(err, sizeof err);
    CHECK(strstr(err, "max_connections") != NULL, "error names the offending key ('%s')", err);

    tmp_path(p, sizeof p, "unknown-polycallrc");
    write_file(p, "log_level=info\nmystery_key=1\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_OK, "unknown key, run=0 (warning)");
    EXPECT_RC(polycall_ffi_run_config(p, 1), POLYCALL_E_CONFIG, "unknown key, run=1 (strict)");

    tmp_path(p, sizeof p, "tls-polycallrc");
    write_file(p, "tls_enabled=true\ncert_file=/x/c.pem\nkey_file=/x/k.pem\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_OK, "tls_enabled=true, run=0");
    EXPECT_RC(polycall_ffi_run_config(p, 1), POLYCALL_E_UNSUPPORTED,
              "tls_enabled=true, run=1 (no TLS transport: refused, not faked)");

    tmp_path(p, sizeof p, "Polycallfile");
    write_file(p, "server node 8080:8084\nnetwork start\nworkspace_root=/opt/x\n"
                  "log_directory=/var/log/x\ndaemon_endpoint=127.0.0.1:0\n"
                  "auth_token_env=POLYCALL_DEV_TOKEN\npeer_node_id=alpha\n"
                  "peer beta 127.0.0.1:9002\npeer gamma localhost:9003\n");
    EXPECT_RC(polycall_ffi_run_config(p, 1), POLYCALL_OK, "Polycallfile with daemon/peer keys");
    n = polycall_ffi_describe(p, buf, (int)sizeof buf);
    CHECK(n > 0 && strstr(buf, "\"beta\":\"127.0.0.1:9002\"") &&
          strstr(buf, "\"layer\":\"project\"") && strstr(buf, "\"auth_token_env\":\"POLYCALL_DEV_TOKEN\""),
          "describe reports layer, peers and keys (%d bytes)", n);
    CHECK(polycall_ffi_describe(p, NULL, 0) == n, "describe size query returns the same length");

    write_file(p, "peer bad!id 127.0.0.1:1\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_E_CONFIG, "invalid peer id in Polycallfile");
    write_file(p, "peer ok 127.0.0.1:0\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_E_CONFIG, "peer endpoint with port 0");
    write_file(p, "daemon_max_connections=5000\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_E_CONFIG, "daemon_max_connections out of range");
    write_file(p, "auth_token_env=hunter2 secret\n");
    EXPECT_RC(polycall_ffi_run_config(p, 0), POLYCALL_E_CONFIG, "auth_token_env must be a NAME");
}

/* ---- peers -------------------------------------------------------------- */

static int recv_expect(polycall_peer_t h, const char *want_from, const char *want_id,
                       const void *want, size_t want_len, uint32_t timeout)
{
    char from[POLYCALL_PEER_ID_MAX], mid[POLYCALL_MESSAGE_ID_MAX];
    unsigned char *buf = malloc(POLYCALL_PEER_MAX_PAYLOAD + 1);
    size_t len = 0;
    int rc, ok;
    if (!buf) return 0;
    rc = polycall_peer_recv(h, timeout, from, sizeof from, mid, sizeof mid,
                            buf, POLYCALL_PEER_MAX_PAYLOAD, &len);
    ok = rc == POLYCALL_OK && strcmp(from, want_from) == 0 &&
         (!want_id || strcmp(mid, want_id) == 0) && len == want_len &&
         (want_len == 0 || memcmp(buf, want, want_len) == 0);
    if (!ok) {
        char e[256];
        polycall_last_error(e, sizeof e);
        printf("      recv rc=%d from='%s' id='%s' len=%lu (want %lu) err='%s'\n", rc,
               rc == 0 ? from : "", rc == 0 ? mid : "", (unsigned long)len,
               (unsigned long)want_len, e);
    }
    free(buf);
    return ok;
}

typedef struct {
    polycall_peer_t h;
    int rc;
    uint32_t timeout;
} recv_arg_t;

static THR_RET blocked_recv(void *p)
{
    recv_arg_t *a = (recv_arg_t *)p;
    char from[64], mid[64], buf[16];
    size_t len;
    a->rc = polycall_peer_recv(a->h, a->timeout, from, sizeof from, mid, sizeof mid,
                               buf, sizeof buf, &len);
    THR_RETURN;
}

typedef struct {
    polycall_peer_t h;
    int thread_no;
    int ok;
} send_arg_t;

static THR_RET concurrent_sender(void *p)
{
    send_arg_t *a = (send_arg_t *)p;
    int i;
    char id[64], payload[64];
    for (i = 0; i < 50; ++i) {
        snprintf(id, sizeof id, "t%d-m%d", a->thread_no, i);
        snprintf(payload, sizeof payload, "payload %d/%d", a->thread_no, i);
        if (polycall_peer_send(a->h, "beta", payload, strlen(payload), id, 5000) == POLYCALL_OK) {
            a->ok++;
        }
    }
    THR_RETURN;
}

static sock_t raw_connect(const char *endpoint)
{
    char host[64];
    const char *colon = strrchr(endpoint, ':');
    struct sockaddr_in sa;
    sock_t s;
    size_t hl = (size_t)(colon - endpoint);
    memcpy(host, endpoint, hl);
    host[hl] = '\0';
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)atoi(colon + 1));
    inet_pton(AF_INET, strcmp(host, "localhost") == 0 ? "127.0.0.1" : host, &sa.sin_addr);
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == BAD_SOCK) return BAD_SOCK;
    if (connect(s, (struct sockaddr *)&sa, sizeof sa) != 0) {
        CLOSESOCK(s);
        return BAD_SOCK;
    }
    return s;
}

static void test_peers(void)
{
    polycall_peer_t A = 0, B = 0, C = 0, old;
    char epA[POLYCALL_ENDPOINT_MAX], epB[POLYCALL_ENDPOINT_MAX], buf[8192], buf4[4];
    size_t len = 0;
    unsigned char *big;
    unsigned char bin[256];
    int i, rc;
    uint64_t t0;

    printf("\n### peer nodes: arguments and handles\n");
    EXPECT_RC(polycall_peer_open("alpha", "127.0.0.1:0", NULL, NULL), POLYCALL_E_INVALID_ARGUMENT, "open with NULL out");
    EXPECT_RC(polycall_peer_open(NULL, "127.0.0.1:0", NULL, &A), POLYCALL_E_INVALID_ARGUMENT, "open with NULL id");
    EXPECT_RC(polycall_peer_open("", "127.0.0.1:0", NULL, &A), POLYCALL_E_INVALID_ARGUMENT, "open with empty id");
    EXPECT_RC(polycall_peer_open("bad id", "127.0.0.1:0", NULL, &A), POLYCALL_E_INVALID_ARGUMENT, "open with space in id");
    EXPECT_RC(polycall_peer_open("a123456789012345678901234567890123456789012345678901234567890123",
                                 NULL, NULL, &A), POLYCALL_E_INVALID_ARGUMENT, "open with 64-char id");
    EXPECT_RC(polycall_peer_open("alpha", "nohostport", NULL, &A), POLYCALL_E_INVALID_ARGUMENT, "open with bad endpoint");
    EXPECT_RC(polycall_peer_open("alpha", "0.0.0.0:0", NULL, &A), POLYCALL_E_CONFIG,
              "non-loopback bind without a token is refused");
    EXPECT_RC(polycall_peer_open("alpha", "127.0.0.1:0", "tok-A", &A), POLYCALL_OK, "open alpha (token)");
    EXPECT_RC(polycall_peer_open("beta", "127.0.0.1:0", "tok-A", &B), POLYCALL_OK, "open beta (same shared token)");
    CHECK(A > 0 && B > 0 && A != B, "handles are positive and distinct (%d, %d)", (int)A, (int)B);
    EXPECT_RC(polycall_peer_endpoint(A, epA, sizeof epA), POLYCALL_OK, "alpha endpoint");
    EXPECT_RC(polycall_peer_endpoint(B, epB, sizeof epB), POLYCALL_OK, "beta endpoint");
    CHECK(strncmp(epA, "127.0.0.1:", 10) == 0 && atoi(epA + 10) > 0, "alpha bound to %s", epA);
    EXPECT_RC(polycall_peer_endpoint(A, buf4, sizeof buf4), POLYCALL_E_TOO_LARGE, "endpoint into 4-byte buffer");
    EXPECT_RC(polycall_peer_node_id(B, buf, sizeof buf), POLYCALL_OK, "node_id");
    CHECK(strcmp(buf, "beta") == 0, "node_id is 'beta'");
    {
        polycall_peer_t D;
        char ep2[POLYCALL_ENDPOINT_MAX + 8];
        snprintf(ep2, sizeof ep2, "%s", epA);
        EXPECT_RC(polycall_peer_open("dup", ep2, NULL, &D), POLYCALL_E_ADDRESS_IN_USE,
                  "second node on alpha's port -> ADDRESS_IN_USE");
    }

    printf("\n### peer nodes: registry ownership\n");
    EXPECT_RC(polycall_peer_register(A, "beta", epB), POLYCALL_OK, "alpha registers beta");
    EXPECT_RC(polycall_peer_register(A, "bad id", epB), POLYCALL_E_INVALID_ARGUMENT, "register invalid id");
    EXPECT_RC(polycall_peer_register(A, "x", "127.0.0.1:0"), POLYCALL_E_INVALID_ARGUMENT, "register port 0");
    EXPECT_RC(polycall_peer_register(A, "x", NULL), POLYCALL_E_INVALID_ARGUMENT, "register NULL endpoint");
    EXPECT_RC(polycall_peer_list(A, buf, sizeof buf, &len), POLYCALL_OK, "alpha list");
    CHECK(strstr(buf, "\"beta\"") != NULL && len == strlen(buf), "alpha's registry has beta: %s", buf);
    EXPECT_RC(polycall_peer_list(B, buf, sizeof buf, &len), POLYCALL_OK, "beta list");
    CHECK(strcmp(buf, "{}") == 0, "beta's registry is still empty (no implicit sync): %s", buf);
    EXPECT_RC(polycall_peer_list(A, NULL, 0, &len), POLYCALL_E_TOO_LARGE, "list size query");
    CHECK(len > 2, "size query reports %lu bytes", (unsigned long)len);
    EXPECT_RC(polycall_peer_unregister(A, "nobody"), POLYCALL_E_NOT_FOUND, "unregister unknown");
    EXPECT_RC(polycall_peer_register(B, "alpha", epA), POLYCALL_OK, "beta registers alpha");

    printf("\n### peer nodes: health / ping / identity\n");
    EXPECT_RC(polycall_peer_ping(A, "beta", 2000), POLYCALL_OK, "alpha pings beta by id");
    EXPECT_RC(polycall_peer_ping(A, epB, 2000), POLYCALL_OK, "alpha pings beta by endpoint");
    EXPECT_RC(polycall_peer_register(A, "gamma", epB), POLYCALL_OK, "register 'gamma' -> beta's endpoint");
    EXPECT_RC(polycall_peer_ping(A, "gamma", 2000), POLYCALL_E_PROTOCOL,
              "ping 'gamma' answered by 'beta' -> identity mismatch detected");
    EXPECT_RC(polycall_peer_send(A, "gamma", "x", 1, "misroute-1", 2000), POLYCALL_E_PROTOCOL,
              "send to 'gamma' acknowledged by 'beta' -> not counted as delivered");
    EXPECT_RC(polycall_peer_unregister(A, "gamma"), POLYCALL_OK, "unregister gamma");
    CHECK(recv_expect(B, "alpha", "misroute-1", "x", 1, 1000),
          "(the misrouted copy did reach beta, which is why it is reported as a protocol error)");
    EXPECT_RC(polycall_peer_ping(A, "nobody", 500), POLYCALL_E_NOT_FOUND, "ping unknown id");
    EXPECT_RC(polycall_peer_ping(A, "127.0.0.1:1", 1000), POLYCALL_E_TRANSPORT, "ping a closed port");
    EXPECT_RC(polycall_peer_health(B, buf, sizeof buf, &len), POLYCALL_OK, "beta health");
    CHECK(strstr(buf, "\"node_id\":\"beta\"") && strstr(buf, "\"status\":\"ok\""), "health JSON: %.120s", buf);

    printf("\n### peer nodes: payload exchange (both directions)\n");
    EXPECT_RC(polycall_peer_send(A, "beta", "", 0, "empty-1", 2000), POLYCALL_OK, "send empty payload");
    CHECK(recv_expect(B, "alpha", "empty-1", "", 0, 2000), "beta received the empty payload from alpha");
    EXPECT_RC(polycall_peer_send(A, "beta", NULL, 0, "nullzero", 2000), POLYCALL_OK, "send NULL payload with len 0");
    CHECK(recv_expect(B, "alpha", "nullzero", "", 0, 2000), "NULL,0 received as empty");
    EXPECT_RC(polycall_peer_send(A, "beta", NULL, 5, "nullfive", 2000), POLYCALL_E_INVALID_ARGUMENT, "NULL payload with len 5");
    {
        const char *u = "h\xc3\xa9llo \xe2\x80\x94 \xe4\xb8\x96\xe7\x95\x8c \xf0\x9f\x8c\x8d";
        EXPECT_RC(polycall_peer_send(A, "beta", u, strlen(u), "utf8-1", 2000), POLYCALL_OK, "send UTF-8");
        CHECK(recv_expect(B, "alpha", "utf8-1", u, strlen(u), 2000), "UTF-8 bytes identical at beta");
    }
    for (i = 0; i < 256; ++i) bin[i] = (unsigned char)i;
    EXPECT_RC(polycall_peer_send(A, "beta", bin, sizeof bin, "binary-1", 2000), POLYCALL_OK,
              "send 256 bytes incl. embedded NULs");
    CHECK(recv_expect(B, "alpha", "binary-1", bin, sizeof bin, 2000), "binary payload identical at beta");
    EXPECT_RC(polycall_peer_send(B, "alpha", "pong", 4, "b2a-1", 2000), POLYCALL_OK, "beta -> alpha");
    CHECK(recv_expect(A, "beta", "b2a-1", "pong", 4, 2000), "alpha received beta's payload, sender 'beta'");

    big = malloc(POLYCALL_PEER_MAX_PAYLOAD + 1);
    if (big) {
        for (i = 0; i < (int)POLYCALL_PEER_MAX_PAYLOAD + 1; ++i) big[i] = (unsigned char)(i * 31 + 7);
        EXPECT_RC(polycall_peer_send(A, "beta", big, POLYCALL_PEER_MAX_PAYLOAD, "max-1", 10000), POLYCALL_OK,
                  "send exactly 1 MiB (the maximum)");
        CHECK(recv_expect(B, "alpha", "max-1", big, POLYCALL_PEER_MAX_PAYLOAD, 5000), "1 MiB payload identical");
        EXPECT_RC(polycall_peer_send(A, "beta", big, POLYCALL_PEER_MAX_PAYLOAD + 1, "over-1", 2000),
                  POLYCALL_E_TOO_LARGE, "1 MiB + 1 byte refused before sending");
        free(big);
    }

    printf("\n### peer nodes: receive buffers, timeouts, duplicates\n");
    EXPECT_RC(polycall_peer_send(A, "beta", "0123456789", 10, "size-1", 2000), POLYCALL_OK, "send 10 bytes");
    {
        char f[64], m[64], tiny[4];
        size_t need = 0;
        EXPECT_RC(polycall_peer_recv(B, 2000, f, sizeof f, m, sizeof m, tiny, sizeof tiny, &need),
                  POLYCALL_E_TOO_LARGE, "recv into 4-byte buffer");
        CHECK(need == 10, "needed size reported (%lu)", (unsigned long)need);
        EXPECT_RC(polycall_peer_recv(B, 2000, f, sizeof f, m, sizeof m, NULL, 5, &need),
                  POLYCALL_E_INVALID_ARGUMENT, "recv NULL payload with cap > 0");
    }
    CHECK(recv_expect(B, "alpha", "size-1", "0123456789", 10, 2000), "message stayed queued after TOO_LARGE");
    t0 = now_ms();
    {
        char f[64], m[64], b2[8];
        size_t l2;
        rc = polycall_peer_recv(B, 150, f, sizeof f, m, sizeof m, b2, sizeof b2, &l2);
        CHECK(rc == POLYCALL_E_TIMEOUT && now_ms() - t0 >= 100 && now_ms() - t0 < 3000,
              "empty inbox: recv(150 ms) -> TIMEOUT after %lu ms", (unsigned long)(now_ms() - t0));
        rc = polycall_peer_recv(B, 0, f, sizeof f, m, sizeof m, b2, sizeof b2, &l2);
        CHECK(rc == POLYCALL_E_TIMEOUT, "recv(0) polls without waiting");
    }
    EXPECT_RC(polycall_peer_send(A, "beta", "once", 4, "dup-1", 2000), POLYCALL_OK, "send dup-1");
    EXPECT_RC(polycall_peer_send(A, "beta", "once", 4, "dup-1", 2000), POLYCALL_OK,
              "re-send dup-1 (retry after an ambiguous outcome) is acknowledged");
    CHECK(recv_expect(B, "alpha", "dup-1", "once", 4, 2000), "dup-1 delivered once");
    {
        char f[64], m[64], b2[8];
        size_t l2;
        rc = polycall_peer_recv(B, 300, f, sizeof f, m, sizeof m, b2, sizeof b2, &l2);
        CHECK(rc == POLYCALL_E_TIMEOUT, "...and only once (no second copy queued)");
    }
    EXPECT_RC(polycall_peer_send(A, "beta", "x", 1, "bad id!", 2000), POLYCALL_E_INVALID_ARGUMENT, "invalid message id");

    printf("\n### peer nodes: failures are reported, never faked\n");
    EXPECT_RC(polycall_peer_send(A, "127.0.0.1:1", "x", 1, NULL, 1000), POLYCALL_E_TRANSPORT, "send to a closed port");
    EXPECT_RC(polycall_peer_send(A, "unknown-peer", "x", 1, NULL, 1000), POLYCALL_E_NOT_FOUND, "send to an unregistered id");
    EXPECT_RC(polycall_peer_open("carol", "127.0.0.1:0", NULL, &C), POLYCALL_OK, "open carol (no token)");
    EXPECT_RC(polycall_peer_send(C, epB, "x", 1, NULL, 2000), POLYCALL_E_AUTH, "carol without beta's token -> AUTH");
    {
        polycall_peer_t W;
        EXPECT_RC(polycall_peer_open("wrongtok", NULL, "not-the-token", &W), POLYCALL_OK, "send-only node, wrong token");
        EXPECT_RC(polycall_peer_send(W, epB, "x", 1, NULL, 2000), POLYCALL_E_AUTH, "wrong token -> AUTH");
        polycall_peer_close(W);
    }
    {
        /* a listener that accepts and never answers: bounded timeout */
        polycall_peer_t S;
        char ep[64];
        sock_t raw;
        EXPECT_RC(polycall_peer_open("silent", "127.0.0.1:0", "tok-A", &S), POLYCALL_OK, "open a node to stall");
        polycall_peer_endpoint(S, ep, sizeof ep);
        raw = raw_connect(ep);            /* half-open request: worker waits */
        if (raw != BAD_SOCK) send(raw, "POST /receive HTTP/1.1\r\n", 24, 0);
        t0 = now_ms();
        rc = polycall_peer_ping(A, "127.0.0.1:9", 300);   /* discard port */
        CHECK(rc == POLYCALL_E_TRANSPORT || rc == POLYCALL_E_TIMEOUT,
              "ping an unreachable service -> %s in %lu ms", polycall_strerror(rc),
              (unsigned long)(now_ms() - t0));
        t0 = now_ms();
        EXPECT_RC(polycall_peer_close(S), POLYCALL_OK, "close a node with a stalled client connection");
        CHECK(now_ms() - t0 < 3000, "close did not wait for the stalled client (%lu ms)",
              (unsigned long)(now_ms() - t0));
        if (raw != BAD_SOCK) CLOSESOCK(raw);
    }
    {
        /* malformed HTTP straight onto beta's socket: beta survives */
        const char *junk[] = {
            "GARBAGE\r\n\r\n",
            "POST /receive HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\nhello",
            "POST /receive HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n",
            "POST /receive HTTP/1.1\r\nAuthorization: Bearer tok-A\r\nContent-Length: 9\r\n\r\n{\"v\":1,}x",
            "GET /../../etc/passwd HTTP/1.1\r\n\r\n",
            "DELETE /peers HTTP/1.1\r\nAuthorization: Bearer tok-A\r\n\r\n",
            "POST /nowhere HTTP/1.1\r\nContent-Length: 0\r\n\r\n",
            "POST /receive HTTP/1.1\r\n\r\n",
        };
        size_t k;
        for (k = 0; k < sizeof junk / sizeof junk[0]; ++k) {
            sock_t s = raw_connect(epB);
            char r[512];
            int got;
            if (s == BAD_SOCK) continue;
            send(s, junk[k], (int)strlen(junk[k]), 0);
            got = recv(s, r, sizeof r - 1, 0);
            if (got > 0) r[got] = '\0'; else r[0] = '\0';
            CHECK(got > 12 && (strstr(r, "HTTP/1.1 4") || strstr(r, "HTTP/1.1 5")),
                  "malformed request %lu answered with %.12s", (unsigned long)k, got > 0 ? r : "(nothing)");
            CLOSESOCK(s);
        }
        EXPECT_RC(polycall_peer_ping(A, "beta", 2000), POLYCALL_OK, "beta still healthy after malformed input");
    }

    printf("\n### peer nodes: backpressure (bounded inbox)\n");
    {
        int sent = 0, busy = 0;
        for (i = 0; i < 300; ++i) {
            char id[32];
            snprintf(id, sizeof id, "flood-%d", i);
            rc = polycall_peer_send(A, "beta", "f", 1, id, 2000);
            if (rc == POLYCALL_OK) sent++;
            else if (rc == POLYCALL_E_BUSY) { busy++; break; }
            else break;
        }
        CHECK(sent == 256 && busy == 1, "inbox accepts 256 then answers BUSY (sent %d, busy %d)", sent, busy);
        for (i = 0; i < sent; ++i) {
            char f[64], m[64], b2[8];
            size_t l2;
            if (polycall_peer_recv(B, 1000, f, sizeof f, m, sizeof m, b2, sizeof b2, &l2) != POLYCALL_OK) break;
        }
        CHECK(i == sent, "drained %d queued messages", i);
        EXPECT_RC(polycall_peer_send(A, "beta", "f", 1, "after-drain", 2000), POLYCALL_OK, "accepts again after draining");
        CHECK(recv_expect(B, "alpha", "after-drain", "f", 1, 2000), "post-drain message received");
    }

    printf("\n### peer nodes: concurrency\n");
    {
        thr_t th[8];
        send_arg_t sa[8];
        int total = 0, got = 0, started = 0;
        for (i = 0; i < 8; ++i) {
            sa[i].h = A; sa[i].thread_no = i; sa[i].ok = 0;
            if (thr_start(&th[i], concurrent_sender, &sa[i]) == 0) started++;
        }
        /* drain concurrently so the 256-message inbox never fills */
        while (got < 400) {
            char f[64], m[64], b2[64];
            size_t l2;
            if (polycall_peer_recv(B, 5000, f, sizeof f, m, sizeof m, b2, sizeof b2, &l2) != POLYCALL_OK) break;
            got++;
        }
        for (i = 0; i < started; ++i) { thr_join(th[i]); total += sa[i].ok; }
        CHECK(started == 8 && total == 400 && got == 400,
              "8 threads x 50 sends: %d acknowledged, %d received", total, got);
    }

    printf("\n### peer nodes: cancellation, close while blocked, stale handles\n");
    {
        thr_t t;
        recv_arg_t ra;
        ra.h = A; ra.rc = 1; ra.timeout = UINT32_MAX;
        if (thr_start(&t, blocked_recv, &ra) == 0) {
            msleep(200);
            EXPECT_RC(polycall_peer_cancel(A), POLYCALL_OK, "cancel");
            thr_join(t);
            CHECK(ra.rc == POLYCALL_E_CANCELLED, "blocked recv woke with CANCELLED (%s)", polycall_strerror(ra.rc));
        }
        ra.h = B; ra.rc = 1; ra.timeout = UINT32_MAX;
        if (thr_start(&t, blocked_recv, &ra) == 0) {
            msleep(200);
            EXPECT_RC(polycall_peer_close(B), POLYCALL_OK, "close beta while a recv is blocked");
            thr_join(t);
            CHECK(ra.rc == POLYCALL_E_CLOSED, "blocked recv woke with CLOSED (%s)", polycall_strerror(ra.rc));
        }
    }
    EXPECT_RC(polycall_peer_close(B), POLYCALL_E_INVALID_HANDLE, "double close");
    EXPECT_RC(polycall_peer_send(B, "alpha", "x", 1, NULL, 100), POLYCALL_E_INVALID_HANDLE, "send after close");
    EXPECT_RC(polycall_peer_list(B, buf, sizeof buf, &len), POLYCALL_E_INVALID_HANDLE, "list after close");
    EXPECT_RC(polycall_peer_send(A, "beta", "x", 1, NULL, 1000), POLYCALL_E_TRANSPORT,
              "alpha -> closed beta reports failed delivery");
    EXPECT_RC(polycall_peer_ping(A, epA, 1000), POLYCALL_OK, "alpha stays healthy after beta died");
    old = B;
    EXPECT_RC(polycall_peer_open("beta2", "127.0.0.1:0", NULL, &B), POLYCALL_OK, "open a new node (slot reuse)");
    CHECK(B != old, "new handle %d differs from the stale one %d", (int)B, (int)old);
    EXPECT_RC(polycall_peer_list(old, buf, sizeof buf, &len), POLYCALL_E_INVALID_HANDLE, "stale handle stays invalid");
    EXPECT_RC(polycall_peer_list(0, buf, sizeof buf, &len), POLYCALL_E_INVALID_HANDLE, "handle 0");
    EXPECT_RC(polycall_peer_list(-1, buf, sizeof buf, &len), POLYCALL_E_INVALID_HANDLE, "handle -1");
    EXPECT_RC(polycall_peer_list(0x7FFFFFFF, buf, sizeof buf, &len), POLYCALL_E_INVALID_HANDLE, "handle INT32_MAX");
    EXPECT_RC(polycall_peer_cancel(12345), POLYCALL_E_INVALID_HANDLE, "cancel on a never-issued handle");
    polycall_peer_close(B);
    polycall_peer_close(C);
    EXPECT_RC(polycall_peer_close(A), POLYCALL_OK, "close alpha");
}

/* ---- RPC: polycall_call against an in-process runtime ------------------ */

typedef struct {
    polycall_runtime_t *rt;
    char endpoint[96];
    volatile int bound;
    int rc;
} serve_arg_t;

static void POLYCALL_CALL on_bound(const char *ep, void *user)
{
    serve_arg_t *a = (serve_arg_t *)user;
    snprintf(a->endpoint, sizeof a->endpoint, "%s", ep);
    flag_set(&a->bound);     /* publishes endpoint to the main thread */
}

static THR_RET serve_thread(void *p)
{
    serve_arg_t *a = (serve_arg_t *)p;
    a->rc = polycall_runtime_serve(a->rt, "127.0.0.1", 0, "rt-token", on_bound, a);
    THR_RETURN;
}

static void test_call(void)
{
    serve_arg_t sa;
    thr_t t;
    char out[8192];
    size_t len = 0;
    int i;
    char *bigin;

    printf("\n### RPC: polycall_call\n");
    memset(&sa, 0, sizeof sa);
    sa.rt = polycall_runtime_create();
    CHECK(sa.rt != NULL, "runtime created");
    if (!sa.rt) return;
    CHECK(polycall_runtime_set_limits(sa.rt, 5000, 0, 0) == -1, "set_limits rejects 5000 connections");
    CHECK(polycall_runtime_set_limits(sa.rt, 8, 2000, 1000) == 0, "set_limits(8, 2000, 1000)");
    if (thr_start(&t, serve_thread, &sa) != 0) return;
    for (i = 0; i < 100 && !flag_get(&sa.bound); ++i) msleep(20);
    CHECK(flag_get(&sa.bound), "runtime serving at %s", sa.endpoint);

    EXPECT_RC(polycall_call(sa.endpoint, "inventory", "get", "{\"item_id\":\"widget-a\"}", 2000,
                            out, sizeof out, &len), POLYCALL_OK, "inventory.get widget-a");
    CHECK(strcmp(out, "{\"item_id\":\"widget-a\",\"quantity\":42,\"in_stock\":true}") == 0,
          "exact output: %s", out);
    {
        const char *nested = "{\"a\":[1,2.5,{\"b\":\"x\\\"y\",\"n\":null}],\"t\":true,"
                             "\"u\":\"h\\u00e9\",\"neg\":-7}";
        EXPECT_RC(polycall_call(sa.endpoint, "debug", "echo", nested, 2000, out, sizeof out, &len),
                  POLYCALL_OK, "debug.echo with nested input");
        CHECK(strcmp(out, "{\"echo\":{\"a\":[1,2.5,{\"b\":\"x\\\"y\",\"n\":null}],\"t\":true,"
                          "\"u\":\"h\xc3\xa9\",\"neg\":-7}}") == 0,
              "nested input echoed exactly (was flattened to null before): %s", out);
    }
    bigin = malloc(40000);
    if (bigin) {
        /* this request shape overflowed a stack buffer in the old runtime */
        size_t w = 0;
        w += (size_t)sprintf(bigin + w, "{");
        for (i = 0; i < 30; ++i) {
            w += (size_t)sprintf(bigin + w, "%s\"k%03d_%0100d\":\"", i ? "," : "", i, 0);
            memset(bigin + w, 'v', 600);
            w += 600;
            w += (size_t)sprintf(bigin + w, "\"");
        }
        sprintf(bigin + w, "}");
        EXPECT_RC(polycall_call(sa.endpoint, "inventory", "get", bigin, 2000, out, sizeof out, &len),
                  POLYCALL_E_REMOTE, "30 x (104-char key, 600-char value) input -> clean input.invalid");
        CHECK(strstr(out, "input.invalid") != NULL, "error object: %.80s", out);
        EXPECT_RC(polycall_call(sa.endpoint, "debug", "echo", bigin, 2000, out, sizeof out, &len),
                  POLYCALL_E_TOO_LARGE, "echo of 20 KB into an 8 KB buffer -> TOO_LARGE");
        CHECK(len > 20000, "needed size reported (%lu)", (unsigned long)len);
        free(bigin);
    }
    EXPECT_RC(polycall_call(sa.endpoint, "inventory", "get", "{\"item_id\":\"nope\"}", 2000,
                            out, sizeof out, &len), POLYCALL_E_REMOTE, "unknown item");
    CHECK(strstr(out, "item.unknown") != NULL, "error code item.unknown: %s", out);
    EXPECT_RC(polycall_call(sa.endpoint, "inventory", "teleport", "{}", 2000, out, sizeof out, &len),
              POLYCALL_E_NOT_FOUND, "unknown operation");
    EXPECT_RC(polycall_call(sa.endpoint, "debug", "sleep", "{\"ms\":2000}", 100, out, sizeof out, &len),
              POLYCALL_E_TIMEOUT, "operation over its deadline");
    EXPECT_RC(polycall_call(sa.endpoint, "debug", "echo", "{not json", 1000, out, sizeof out, &len),
              POLYCALL_E_INVALID_ARGUMENT, "invalid input JSON rejected locally");
    EXPECT_RC(polycall_call(sa.endpoint, "debug", "echo", NULL, 1000, out, sizeof out, &len),
              POLYCALL_OK, "NULL input sent as null");
    CHECK(strcmp(out, "{\"echo\":null}") == 0, "echo of null: %s", out);
    EXPECT_RC(polycall_call(sa.endpoint, "debug", "echo", "{}", 0, out, sizeof out, &len),
              POLYCALL_E_INVALID_ARGUMENT, "timeout 0");
    EXPECT_RC(polycall_call("nonsense", "a", "b", NULL, 100, out, sizeof out, &len),
              POLYCALL_E_INVALID_ARGUMENT, "endpoint without a port");
    EXPECT_RC(polycall_call(sa.endpoint, "", "b", NULL, 100, out, sizeof out, &len),
              POLYCALL_E_INVALID_ARGUMENT, "empty service");
    EXPECT_RC(polycall_call("127.0.0.1:1", "a", "b", NULL, 500, out, sizeof out, &len),
              POLYCALL_E_TRANSPORT, "no runtime at the endpoint");
    {
        /* a framing violation straight on the socket: runtime survives */
        sock_t s = raw_connect(sa.endpoint);
        if (s != BAD_SOCK) {
            char r[512];
            int got, total = 0;
            send(s, "XXXX\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02{}", 18, 0);
            /* header and payload may arrive in separate segments: read to EOF */
            while (total < (int)sizeof r - 1 &&
                   (got = recv(s, r + total, (int)sizeof r - 1 - total, 0)) > 0) {
                total += got;
            }
            r[total] = '\0';
            CHECK(total > 16 && strstr(r + 16, "frame.invalid") != NULL,
                  "bad magic answered with a frame.invalid reply (%d bytes)", total);
            CLOSESOCK(s);
        }
        EXPECT_RC(polycall_call(sa.endpoint, "inventory", "get", "{\"item_id\":\"widget-b\"}", 2000,
                                out, sizeof out, &len), POLYCALL_OK, "runtime still serves after it");
    }

    polycall_runtime_stop(sa.rt);
    thr_join(t);
    CHECK(sa.rc == 0, "serve returned cleanly after polycall_runtime_stop (%d)", sa.rc);
    polycall_runtime_destroy(sa.rt);
}

int main(int argc, char **argv)
{
#if defined(_WIN32)
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
    snprintf(g_tmp, sizeof g_tmp, "%s", getenv("TEMP") ? getenv("TEMP") : ".");
#else
    snprintf(g_tmp, sizeof g_tmp, "%s", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
#endif
    if (argc > 1) snprintf(g_tmp, sizeof g_tmp, "%s", argv[1]);
    setvbuf(stdout, NULL, _IONBF, 0);

    test_library();
    test_config();
    test_peers();
    test_call();

    printf("\n--- ffi_unit: %d passed, %d failed ---\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
