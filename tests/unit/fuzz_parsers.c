/*
 * Bounded, deterministic mutation fuzzing of every parser that consumes
 * untrusted bytes (fixed seed -> reproducible; iteration count = argv[1]).
 * Each target checks a property, not just "did not crash":
 *
 *   json      parse -> serialise -> parse -> serialise is a fixed point
 *   base64    decode(x) succeeds only for canonical x; encode(decode(x)) == x
 *   cfgfile   mutated Polycallfiles load or fail cleanly (status, message)
 *   http      mutated requests on a real loopback socket: the parser returns
 *             a definite verdict within its deadline
 *   rpc       mutated polycall_rpc frames against a live runtime: it keeps
 *             answering health afterwards
 *
 * Build with POLYCALL_SANITIZERS=address;undefined (Linux CI) to turn any
 * memory error into a failure.
 */

#include "polycall.h"
#include "polycall_runtime.h"

#include "config/cfgfile.h"
#include "config/json.h"
#include "core/pc_buf.h"
#include "core/pc_sys.h"
#include "peer/http.h"
#include "runtime/rpc_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t g_rng = 0x9E3779B97F4A7C15ull;
static uint64_t rnd(void)
{
    g_rng ^= g_rng >> 12;
    g_rng ^= g_rng << 25;
    g_rng ^= g_rng >> 27;
    return g_rng * 0x2545F4914F6CDD1Dull;
}
static size_t rndn(size_t n) { return n ? (size_t)(rnd() % n) : 0; }

static int g_fail;
#define BAD(...) do { g_fail++; printf("FAIL  "); printf(__VA_ARGS__); printf("\n"); } while (0)

/* mutate `in` (len n) into out (cap), returns new length */
static size_t mutate(const char *in, size_t n, char *out, size_t cap)
{
    size_t len = n < cap ? n : cap, k, ops = 1 + rndn(4);
    static const char *const tokens[] = {
        "\"", "\\", "{", "}", "[", "]", ",", ":", "null", "true", "1e309", "-0",
        "\\u0000", "\\ud800", "\xff", "\xc3", "\r\n", "\r\n\r\n", "\0", "=",
        "Content-Length: 99999999999", "Transfer-Encoding: x", "%", "PCR1",
        "peer ", "server ", "#", "9999999999999999999999"
    };
    memcpy(out, in, len);
    for (k = 0; k < ops; ++k) {
        switch (rndn(5)) {
        case 0: if (len) out[rndn(len)] = (char)rnd(); break;                /* flip */
        case 1: if (len) len = rndn(len + 1); break;                          /* truncate */
        case 2: {                                                             /* insert token */
            const char *t = tokens[rndn(sizeof tokens / sizeof tokens[0])];
            size_t tl = strlen(t) ? strlen(t) : 1, at = rndn(len + 1);
            if (len + tl < cap) {
                memmove(out + at + tl, out + at, len - at);
                memcpy(out + at, t, tl);
                len += tl;
            }
            break;
        }
        case 3: if (len > 1) {                                                /* delete span */
            size_t at = rndn(len), l = 1 + rndn(len - at);
            memmove(out + at, out + at + l, len - at - l);
            len -= l;
        } break;
        default: if (len && len * 2 < cap) {                                  /* duplicate */
            memcpy(out + len, out, len);
            len *= 2;
        } break;
        }
    }
    return len;
}

/* ---------------------------------------------------------------- json */
static void fuzz_json(long iters)
{
    static const char *const seeds[] = {
        "{\"service\":\"inventory\",\"operation\":\"get\",\"deadline_ms\":500,\"input\":{\"item_id\":\"w\"}}",
        "{\"v\":1,\"id\":\"m-1\",\"from\":\"alpha\",\"payload_b64\":\"aGVsbG8=\"}",
        "[1,2.5,-3e10,{\"a\":[[],{}]},\"\\u00e9\\ud83d\\ude00\",true,false,null]",
        "{\"schema_version\":2,\"services\":[{\"id\":\"x\",\"host_port\":8080}]}",
    };
    char in[8192];
    long i;
    long parsed = 0;
    for (i = 0; i < iters; ++i) {
        const char *s = seeds[rndn(sizeof seeds / sizeof seeds[0])];
        size_t n = mutate(s, strlen(s), in, sizeof in);
        char err[64];
        json_value *v = json_parse(in, n, err, sizeof err);
        if (v) {
            pc_buf_t a, b;
            json_value *v2;
            parsed++;
            pc_buf_init(&a, 0);
            pc_buf_json_value(&a, v);
            v2 = a.data ? json_parse(a.data, a.len, err, sizeof err) : NULL;
            if (!v2) {
                BAD("json: re-serialised output does not parse: %.200s", a.data ? a.data : "(null)");
            } else {
                pc_buf_init(&b, 0);
                pc_buf_json_value(&b, v2);
                if (a.len != b.len || memcmp(a.data, b.data, a.len) != 0) {
                    BAD("json: serialise is not a fixed point: %.120s vs %.120s", a.data, b.data);
                }
                pc_buf_free(&b);
                json_free(v2);
            }
            pc_buf_free(&a);
            json_free(v);
        }
    }
    printf("PASS  json: %ld mutated inputs, %ld parsed, round-trip fixed point held\n", iters, parsed);
}

/* -------------------------------------------------------------- base64 */
static void fuzz_base64(long iters)
{
    long i, ok = 0;
    unsigned char raw[300];
    for (i = 0; i < iters; ++i) {
        size_t n = rndn(sizeof raw), k;
        pc_buf_t e, e2;
        unsigned char *d = NULL;
        size_t dl = 0;
        for (k = 0; k < n; ++k) raw[k] = (unsigned char)rnd();
        pc_buf_init(&e, 0);
        pc_buf_base64(&e, raw, n);
        if (pc_base64_decode(e.data ? e.data : "", e.len, &d, &dl) != 0 || dl != n ||
            (n && memcmp(d, raw, n) != 0)) {
            BAD("base64: encode/decode round trip failed for %lu bytes", (unsigned long)n);
        }
        free(d);
        d = NULL;
        /* mutated encodings: decode only canonical text */
        {
            char m[600];
            size_t ml = mutate(e.data ? e.data : "", e.len, m, sizeof m);
            if (pc_base64_decode(m, ml, &d, &dl) == 0) {
                ok++;
                pc_buf_init(&e2, 0);
                pc_buf_base64(&e2, d, dl);
                if (e2.len != ml || memcmp(e2.data ? e2.data : "", m, ml) != 0) {
                    BAD("base64: accepted non-canonical input (len %lu)", (unsigned long)ml);
                }
                pc_buf_free(&e2);
            }
            free(d);
        }
        pc_buf_free(&e);
    }
    printf("PASS  base64: %ld round trips, %ld mutated encodings accepted (all canonical)\n", iters, ok);
}

/* ------------------------------------------------------------- cfgfile */
static void fuzz_cfgfile(long iters, const char *dir)
{
    static const char *const seed =
        "# fuzz\nserver node 8080:8084\nnetwork start\nworkspace_root=/opt/x\n"
        "log_directory=/var/log/x\nmax_connections=1000\ntls_enabled=false\n"
        "daemon_endpoint=127.0.0.1:0\nauth_token_env=POLYCALL_DEV_TOKEN\n"
        "peer beta 127.0.0.1:9002\npeer_node_id=alpha\n";
    char path[600], buf[4096], err[256];
    long i, valid = 0;
    snprintf(path, sizeof path, "%s/Polycallfile", dir);
    for (i = 0; i < iters; ++i) {
        size_t n = mutate(seed, strlen(seed), buf, sizeof buf);
        FILE *f = fopen(path, "wb");
        int st = 0;
        polycall_cfgfile_t *cf;
        if (!f) { BAD("cfgfile: cannot write %s", path); return; }
        fwrite(buf, 1, n, f);
        fclose(f);
        err[0] = '\0';
        cf = polycall_cfgfile_load(path, (unsigned)(i & 3), &st, err, sizeof err);
        if (cf) {
            pc_buf_t d;
            valid++;
            pc_buf_init(&d, 0);
            if (polycall_cfgfile_describe(cf, &d) != 0 || !d.data) {
                BAD("cfgfile: describe failed for a loaded file");
            } else {
                char jerr[64];
                json_value *j = json_parse(d.data, d.len, jerr, sizeof jerr);
                if (!j) BAD("cfgfile: describe produced invalid JSON: %.160s", d.data);
                json_free(j);
            }
            pc_buf_free(&d);
            polycall_cfgfile_free(cf);
        } else if (st != POLYCALL_CFGFILE_INVALID || err[0] == '\0') {
            BAD("cfgfile: rejection without status/message (st=%d)", st);
        }
    }
    remove(path);
    printf("PASS  cfgfile: %ld mutated Polycallfiles, %ld accepted, every rejection explained\n",
           iters, valid);
}

/* ---------------------------------------------------------------- http */
static void fuzz_http(long iters)
{
    static const char *const seeds[] = {
        "POST /receive HTTP/1.1\r\nHost: x\r\nAuthorization: Bearer t\r\n"
        "Content-Type: application/json\r\nContent-Length: 10\r\n\r\n{\"v\":1,\"x\"",
        "GET /health HTTP/1.1\r\nHost: 127.0.0.1:9\r\n\r\n",
        "POST /inbox/next HTTP/1.0\r\nContent-Length: 2\r\n\r\n{}",
    };
    pc_sock_t ls, c, s;
    uint16_t port = 0;
    char err[128], req[4096];
    long i, verdicts[6] = { 0 };
    if (pc_listen("127.0.0.1", 0, 16, &ls, &port, err, sizeof err) != POLYCALL_OK) {
        BAD("http: cannot listen: %s", err);
        return;
    }
    for (i = 0; i < iters; ++i) {
        const char *sd = seeds[rndn(sizeof seeds / sizeof seeds[0])];
        size_t n = mutate(sd, strlen(sd), req, sizeof req);
        pc_http_req_t r;
        int v;
        if (pc_connect("127.0.0.1", port, 2000, &c, err, sizeof err) != POLYCALL_OK) {
            BAD("http: connect failed: %s", err);
            break;
        }
        if (pc_accept(ls, 2000, &s) != POLYCALL_OK) {
            pc_sock_close(c);
            BAD("http: accept failed");
            break;
        }
        pc_send_all(c, req, n, pc_deadline_after(1000));
        if (rndn(2)) pc_sock_shutdown(c);      /* half the time: EOF after the bytes */
        v = pc_http_read_request(s, 150, 150, 1u << 16, &r);
        if (v == 0) {
            if (r.content_length > 0 && (size_t)r.content_length != r.body_len) {
                BAD("http: body length %lu != Content-Length %ld",
                    (unsigned long)r.body_len, r.content_length);
            }
            pc_http_req_free(&r);
            verdicts[0]++;
        } else if (v == -1) {
            verdicts[1]++;
        } else if (v >= 400 && v <= 505) {
            verdicts[v == 408 ? 3 : (v == 413 ? 4 : 2)]++;
        } else {
            BAD("http: unexpected verdict %d", v);
        }
        pc_sock_close(s);
        pc_sock_close(c);
    }
    pc_sock_close(ls);
    printf("PASS  http: %ld mutated requests -> %ld parsed, %ld eof, %ld 4xx/5xx, %ld timeout, %ld too-large\n",
           iters, verdicts[0], verdicts[1], verdicts[2], verdicts[3], verdicts[4]);
}

/* ----------------------------------------------------------------- rpc */
typedef struct {
    polycall_runtime_t *rt;
    char ep[96];
    volatile int bound;
} rt_arg_t;

static void POLYCALL_CALL rt_bound(const char *ep, void *u)
{
    rt_arg_t *a = (rt_arg_t *)u;
    snprintf(a->ep, sizeof a->ep, "%s", ep);
    pc_atomic_store(&a->bound, 1);   /* publishes ep to the fuzzing thread */
}

static void rt_main(void *p)
{
    rt_arg_t *a = (rt_arg_t *)p;
    polycall_runtime_serve(a->rt, "127.0.0.1", 0, NULL, rt_bound, a);
}

static uint64_t g_max_connect_ms;

static void fuzz_rpc(long iters)
{
    rt_arg_t a;
    pc_thread_t t;
    long i;
    char host[64];
    uint16_t port;
    static const unsigned char seed[] =
        "PCR1\x01\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x45"
        "{\"service\":\"debug\",\"operation\":\"echo\",\"input\":{\"a\":[1,2]}}";
    memset(&a, 0, sizeof a);
    a.rt = polycall_runtime_create();
    polycall_runtime_set_limits(a.rt, 16, 200, 500);
    pc_thread_start(&t, rt_main, &a);
    for (i = 0; i < 200 && !pc_atomic_load(&a.bound); ++i) pc_sleep_ms(10);
    if (!pc_atomic_load(&a.bound) || pc_split_endpoint(a.ep, host, sizeof host, &port, 0) != POLYCALL_OK) {
        BAD("rpc: runtime did not start");
        return;
    }
    for (i = 0; i < iters; ++i) {
        char frame[2048];
        size_t n = mutate((const char *)seed, sizeof seed - 1, frame, sizeof frame);
        pc_sock_t c;
        char sink[512], cerr[200];
        size_t got;
        int crc;
        uint64_t t0 = pc_mono_ms(), dt;
        cerr[0] = '\0';
        /* 5 s: rapid connect/close churn on loopback can cost one TCP SYN
         * retransmission (1 s initial RTO) when an ephemeral port in
         * TIME_WAIT is reused -- a kernel effect, measured below */
        crc = pc_connect(host, port, 5000, &c, cerr, sizeof cerr);
        dt = pc_mono_ms() - t0;
        if (dt > g_max_connect_ms) g_max_connect_ms = dt;
        if (crc != POLYCALL_OK) {
            BAD("rpc: connect failed at iteration %ld: %s (%s)", i,
                polycall_strerror(crc), cerr);
            break;
        }
        pc_send_all(c, frame, n, pc_deadline_after(500));
        pc_sock_shutdown(c);
        while (pc_recv_some(c, sink, sizeof sink, pc_deadline_after(400), &got) == POLYCALL_OK && got) {
        }
        pc_sock_close(c);
    }
    {
        pcr_frame_t rep;
        static const char ping[] = "{\"action\":\"health\"}";
        int rc = pcr_roundtrip(host, port, PCR_T_CONTROL, ping, (uint32_t)(sizeof ping - 1), &rep, 2000);
        if (rc != 0 || !rep.payload || !strstr(rep.payload, "\"status\":\"ok\"")) {
            BAD("rpc: runtime unhealthy after fuzzing (rc=%d)", rc);
        } else {
            printf("PASS  rpc: %ld mutated frames, runtime still healthy "
                   "(slowest connect %lu ms): %.100s\n",
                   iters, (unsigned long)g_max_connect_ms, rep.payload);
        }
        pcr_frame_free(&rep);
    }
    polycall_runtime_stop(a.rt);
    pc_thread_join(t);
    polycall_runtime_destroy(a.rt);
}

int main(int argc, char **argv)
{
    long iters = argc > 1 ? atol(argv[1]) : 20000;
    const char *dir = argc > 2 ? argv[2] : NULL;
    char tmp[512];
    if (!dir) {
        const char *t = getenv("TMPDIR");
        if (!t) t = getenv("TEMP");
        snprintf(tmp, sizeof tmp, "%s", t ? t : ".");
        dir = tmp;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    pc_net_init();
    printf("fuzz: seed 0x9E3779B97F4A7C15, %ld iterations per pure target\n", iters);
    fuzz_json(iters);
    fuzz_base64(iters);
    fuzz_cfgfile(iters / 10, dir);
    fuzz_http(iters / 20);
    fuzz_rpc(iters / 40);
    printf("--- fuzz: %s (%d failure(s)) ---\n", g_fail ? "FAILED" : "all properties held", g_fail);
    return g_fail ? 1 : 0;
}
