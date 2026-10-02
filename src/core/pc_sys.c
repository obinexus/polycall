#include "pc_sys.h"

#include "polycall.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <process.h>
#  include <mstcpip.h>
#else
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <poll.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <time.h>
#  include <unistd.h>
#endif

#if defined(__APPLE__)
#  include <mach/mach_time.h>
#endif

/* ===================================================================== */
/* time                                                                  */
/* ===================================================================== */

uint64_t pc_mono_ms(void)
{
#if defined(_WIN32)
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#endif
}

void pc_sleep_ms(uint32_t ms)
{
#if defined(_WIN32)
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
    }
#endif
}

uint64_t pc_deadline_after(uint32_t timeout_ms)
{
    if (timeout_ms == UINT32_MAX) return PC_NO_DEADLINE;
    return pc_mono_ms() + timeout_ms;
}

uint32_t pc_remaining_ms(uint64_t deadline)
{
    uint64_t now;
    if (deadline == PC_NO_DEADLINE) return UINT32_MAX;
    now = pc_mono_ms();
    if (now >= deadline) return 0;
    if (deadline - now > (uint64_t)(UINT32_MAX - 1)) return UINT32_MAX - 1;
    return (uint32_t)(deadline - now);
}

/* ===================================================================== */
/* threads / sync                                                        */
/* ===================================================================== */

typedef struct {
    pc_thread_fn fn;
    void *arg;
} thread_start_t;

#if defined(_WIN32)
static unsigned __stdcall thread_tramp(void *p)
{
    thread_start_t st = *(thread_start_t *)p;
    free(p);
    st.fn(st.arg);
    return 0;
}
#else
static void *thread_tramp(void *p)
{
    thread_start_t st = *(thread_start_t *)p;
    free(p);
    st.fn(st.arg);
    return NULL;
}
#endif

int pc_thread_start(pc_thread_t *t, pc_thread_fn fn, void *arg)
{
    thread_start_t *st = malloc(sizeof *st);
    if (!st) return -1;
    st->fn = fn;
    st->arg = arg;
#if defined(_WIN32)
    {
        uintptr_t h = _beginthreadex(NULL, 0, thread_tramp, st, 0, NULL);
        if (h == 0) { free(st); return -1; }
        *t = (HANDLE)h;
    }
#else
    if (pthread_create(t, NULL, thread_tramp, st) != 0) { free(st); return -1; }
#endif
    return 0;
}

void pc_thread_join(pc_thread_t t)
{
#if defined(_WIN32)
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
#else
    pthread_join(t, NULL);
#endif
}

#if defined(_WIN32)
void pc_mutex_init(pc_mutex_t *m) { InitializeCriticalSection(m); }
void pc_mutex_destroy(pc_mutex_t *m) { DeleteCriticalSection(m); }
void pc_mutex_lock(pc_mutex_t *m) { EnterCriticalSection(m); }
void pc_mutex_unlock(pc_mutex_t *m) { LeaveCriticalSection(m); }
void pc_cond_init(pc_cond_t *c) { InitializeConditionVariable(c); }
void pc_cond_destroy(pc_cond_t *c) { (void)c; }
void pc_cond_wait(pc_cond_t *c, pc_mutex_t *m) { SleepConditionVariableCS(c, m, INFINITE); }
int pc_cond_timedwait(pc_cond_t *c, pc_mutex_t *m, uint32_t ms)
{
    if (SleepConditionVariableCS(c, m, ms)) return 0;
    return GetLastError() == ERROR_TIMEOUT ? 1 : 0;
}
void pc_cond_signal(pc_cond_t *c) { WakeConditionVariable(c); }
void pc_cond_broadcast(pc_cond_t *c) { WakeAllConditionVariable(c); }

static BOOL CALLBACK once_tramp(PINIT_ONCE o, PVOID p, PVOID *ctx)
{
    (void)o; (void)ctx;
    ((void (*)(void))p)();
    return TRUE;
}
void pc_once(pc_once_t *o, void (*fn)(void))
{
    /* function pointer -> data pointer: fine on every Windows ABI */
    InitOnceExecuteOnce(o, once_tramp, (PVOID)(uintptr_t)fn, NULL);
}
#else
void pc_mutex_init(pc_mutex_t *m) { pthread_mutex_init(m, NULL); }
void pc_mutex_destroy(pc_mutex_t *m) { pthread_mutex_destroy(m); }
void pc_mutex_lock(pc_mutex_t *m) { pthread_mutex_lock(m); }
void pc_mutex_unlock(pc_mutex_t *m) { pthread_mutex_unlock(m); }

void pc_cond_init(pc_cond_t *c)
{
#if defined(__APPLE__)
    pthread_cond_init(c, NULL);
#else
    pthread_condattr_t a;
    pthread_condattr_init(&a);
    pthread_condattr_setclock(&a, CLOCK_MONOTONIC);
    pthread_cond_init(c, &a);
    pthread_condattr_destroy(&a);
#endif
}
void pc_cond_destroy(pc_cond_t *c) { pthread_cond_destroy(c); }
void pc_cond_wait(pc_cond_t *c, pc_mutex_t *m) { pthread_cond_wait(c, m); }
int pc_cond_timedwait(pc_cond_t *c, pc_mutex_t *m, uint32_t ms)
{
    struct timespec ts;
    int rc;
#if defined(__APPLE__)
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    rc = pthread_cond_timedwait_relative_np(c, m, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
    rc = pthread_cond_timedwait(c, m, &ts);
#endif
    return rc == ETIMEDOUT ? 1 : 0;
}
void pc_cond_signal(pc_cond_t *c) { pthread_cond_signal(c); }
void pc_cond_broadcast(pc_cond_t *c) { pthread_cond_broadcast(c); }
void pc_once(pc_once_t *o, void (*fn)(void)) { pthread_once(o, fn); }
#endif

/* ===================================================================== */
/* atomics + per-thread abort flag                                       */
/* ===================================================================== */

#if defined(_MSC_VER)
#  define PC_TLS __declspec(thread)
#else
#  define PC_TLS _Thread_local
#endif

int pc_atomic_load(const volatile int *p)
{
#if defined(_WIN32)
    return (int)InterlockedCompareExchange((volatile LONG *)p, 0, 0);
#else
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
#endif
}

void pc_atomic_store(volatile int *p, int v)
{
#if defined(_WIN32)
    InterlockedExchange((volatile LONG *)p, (LONG)v);
#else
    __atomic_store_n(p, v, __ATOMIC_SEQ_CST);
#endif
}

static PC_TLS const volatile int *g_thread_abort;

void pc_set_thread_abort(const volatile int *flag)
{
    g_thread_abort = flag;
}

static int aborted(void)
{
    return g_thread_abort && pc_atomic_load(g_thread_abort) != 0;
}

/* ===================================================================== */
/* sockets                                                               */
/* ===================================================================== */

#if defined(_WIN32)
#  define PC_SOCKERR() WSAGetLastError()
#  define PC_EWOULDBLOCK(e) ((e) == WSAEWOULDBLOCK || (e) == WSAEINPROGRESS)
#  define PC_EINTR(e) ((e) == WSAEINTR)
#else
#  define PC_SOCKERR() errno
#  define PC_EWOULDBLOCK(e) ((e) == EAGAIN || (e) == EWOULDBLOCK || (e) == EINPROGRESS)
#  define PC_EINTR(e) ((e) == EINTR)
#endif

#if defined(_WIN32)
/* SIO_TCP_INITIAL_RTO / TCP_INITIAL_RTO_PARAMETERS (mstcpip.h, Windows 8+);
 * spelled out so older MinGW headers build too. Ignored where unsupported. */
typedef struct { USHORT Rtt; UCHAR MaxSynRetransmissions; } pc_rto_params_t;
#  define PC_SIO_TCP_INITIAL_RTO _WSAIOW(IOC_VENDOR, 17)
#endif

#if defined(MSG_NOSIGNAL)
#  define PC_SEND_FLAGS MSG_NOSIGNAL
#else
#  define PC_SEND_FLAGS 0
#endif

static pc_once_t g_net_once = PC_ONCE_INIT;
static int g_net_status = POLYCALL_OK;

static void net_init_once(void)
{
#if defined(_WIN32)
    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) {
        g_net_status = POLYCALL_E_TRANSPORT;
    }
#endif
}

int pc_net_init(void)
{
    pc_once(&g_net_once, net_init_once);
    return g_net_status;
}

void pc_sock_close(pc_sock_t s)
{
    if (s == PC_BAD_SOCK) return;
#if defined(_WIN32)
    closesocket(s);
#else
    close(s);
#endif
}

void pc_sock_shutdown(pc_sock_t s)
{
    if (s == PC_BAD_SOCK) return;
#if defined(_WIN32)
    shutdown(s, SD_BOTH);
#else
    shutdown(s, SHUT_RDWR);
#endif
}

void pc_sock_finish(pc_sock_t s, uint32_t linger_ms)
{
    char sink[2048];
    size_t got = 0;
    uint64_t deadline = pc_deadline_after(linger_ms);
    if (s == PC_BAD_SOCK) return;
#if defined(_WIN32)
    shutdown(s, SD_SEND);
#else
    shutdown(s, SHUT_WR);
#endif
    while (pc_recv_some(s, sink, sizeof sink, deadline, &got) == POLYCALL_OK && got) {
    }
}

static int set_nonblocking(pc_sock_t s)
{
#if defined(_WIN32)
    u_long on = 1;
    return ioctlsocket(s, FIONBIO, &on) == 0 ? 0 : -1;
#else
    int fl = fcntl(s, F_GETFL, 0);
    if (fl < 0) return -1;
    return fcntl(s, F_SETFL, fl | O_NONBLOCK) == 0 ? 0 : -1;
#endif
}

/* non-blocking, non-inheritable, no SIGPIPE */
static int prepare_socket(pc_sock_t s)
{
#if defined(_WIN32)
    SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, 0);
#else
    int fd = fcntl(s, F_GETFD, 0);
    if (fd >= 0) fcntl(s, F_SETFD, fd | FD_CLOEXEC);
#  if defined(SO_NOSIGPIPE)
    {
        int one = 1;
        setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
    }
#  endif
#endif
    return set_nonblocking(s);
}

static pc_sock_t new_tcp_socket(void)
{
    pc_sock_t s;
#if defined(_WIN32)
    s = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
                   WSA_FLAG_NO_HANDLE_INHERIT);
    if (s == INVALID_SOCKET) {
        /* WSA_FLAG_NO_HANDLE_INHERIT needs Windows 7 SP1; fall back */
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
#elif defined(SOCK_CLOEXEC)
    s = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    s = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (s == PC_BAD_SOCK) return PC_BAD_SOCK;
    if (prepare_socket(s) != 0) {
        pc_sock_close(s);
        return PC_BAD_SOCK;
    }
    return s;
}

int pc_split_endpoint(const char *ep, char *host, size_t hostcap,
                      uint16_t *port, int allow_zero)
{
    const char *colon;
    char *end = NULL;
    unsigned long p;
    size_t hl;
    if (!ep || !*ep || !host || hostcap == 0 || !port) {
        return POLYCALL_E_INVALID_ARGUMENT;
    }
    colon = strrchr(ep, ':');
    if (!colon || colon == ep) return POLYCALL_E_INVALID_ARGUMENT;
    hl = (size_t)(colon - ep);
    if (hl >= hostcap) return POLYCALL_E_INVALID_ARGUMENT;
    if (colon[1] < '0' || colon[1] > '9') return POLYCALL_E_INVALID_ARGUMENT;
    errno = 0;
    p = strtoul(colon + 1, &end, 10);
    if (errno || !end || *end != '\0' || p > 65535 || (p == 0 && !allow_zero)) {
        return POLYCALL_E_INVALID_ARGUMENT;
    }
    memcpy(host, ep, hl);
    host[hl] = '\0';
    *port = (uint16_t)p;
    return POLYCALL_OK;
}

static int resolve_v4(const char *host, uint16_t port, int passive,
                      struct sockaddr_in *out)
{
    struct addrinfo hints, *res = NULL;
    char portstr[8];
    int rc;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    snprintf(portstr, sizeof portstr, "%u", (unsigned)port);
    rc = getaddrinfo((host && *host) ? host : NULL, portstr, &hints, &res);
    if (rc != 0 || !res) return -1;
    memcpy(out, res->ai_addr, sizeof *out);
    freeaddrinfo(res);
    return 0;
}

static void fmt_sys_err(char *err, size_t errcap, const char *what, int e)
{
    if (!err || !errcap) return;
#if defined(_WIN32)
    snprintf(err, errcap, "%s (WSA error %d)", what, e);
#else
    snprintf(err, errcap, "%s: %s", what, strerror(e));
#endif
}

int pc_listen(const char *host, uint16_t port, int backlog, pc_sock_t *out,
              uint16_t *bound_port, char *err, size_t errcap)
{
    struct sockaddr_in addr;
    socklen_t alen = sizeof addr;
    pc_sock_t s;
    int e, one = 1;

    *out = PC_BAD_SOCK;
    if (pc_net_init() != POLYCALL_OK) return POLYCALL_E_TRANSPORT;
    if (resolve_v4(host, port, 1, &addr) != 0) {
        if (err && errcap) snprintf(err, errcap, "cannot resolve listen address '%s'",
                                    host ? host : "");
        return POLYCALL_E_CONFIG;
    }
    s = new_tcp_socket();
    if (s == PC_BAD_SOCK) {
        fmt_sys_err(err, errcap, "socket() failed", PC_SOCKERR());
        return POLYCALL_E_TRANSPORT;
    }
#if defined(_WIN32)
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&one, sizeof one);
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
#endif
    if (bind(s, (struct sockaddr *)&addr, sizeof addr) != 0) {
        e = PC_SOCKERR();
        pc_sock_close(s);
#if defined(_WIN32)
        if (e == WSAEADDRINUSE) {
            fmt_sys_err(err, errcap, "address already in use", e);
            return POLYCALL_E_ADDRESS_IN_USE;
        }
        if (e == WSAEACCES) {
            fmt_sys_err(err, errcap, "permission denied binding the address", e);
            return POLYCALL_E_PERMISSION;
        }
        if (e == WSAEADDRNOTAVAIL) {
            fmt_sys_err(err, errcap, "address is not local to this host", e);
            return POLYCALL_E_CONFIG;
        }
#else
        if (e == EADDRINUSE) {
            fmt_sys_err(err, errcap, "address already in use", e);
            return POLYCALL_E_ADDRESS_IN_USE;
        }
        if (e == EACCES || e == EPERM) {
            fmt_sys_err(err, errcap, "permission denied binding the address", e);
            return POLYCALL_E_PERMISSION;
        }
        if (e == EADDRNOTAVAIL) {
            fmt_sys_err(err, errcap, "address is not local to this host", e);
            return POLYCALL_E_CONFIG;
        }
#endif
        fmt_sys_err(err, errcap, "bind() failed", e);
        return POLYCALL_E_TRANSPORT;
    }
    if (listen(s, backlog > 0 ? backlog : 64) != 0) {
        e = PC_SOCKERR();
        pc_sock_close(s);
        fmt_sys_err(err, errcap, "listen() failed", e);
        return POLYCALL_E_TRANSPORT;
    }
    if (bound_port) {
        if (getsockname(s, (struct sockaddr *)&addr, &alen) == 0) {
            *bound_port = ntohs(addr.sin_port);
        } else {
            *bound_port = port;
        }
    }
    *out = s;
    return POLYCALL_OK;
}

/* wait for readability (want_write == 0) or writability; 1 ready, 0
 * timeout, -1 error. Windows uses select() (WSAPoll misreports refused
 * connects on older Windows 10 builds); POSIX uses poll(). */
static int wait_sock_raw(pc_sock_t s, int want_write, uint32_t timeout_ms)
{
#if defined(_WIN32)
    fd_set rs, ws, es;
    struct timeval tv, *tvp = NULL;
    int r;
    FD_ZERO(&rs); FD_ZERO(&ws); FD_ZERO(&es);
    if (want_write) FD_SET(s, &ws); else FD_SET(s, &rs);
    FD_SET(s, &es);
    if (timeout_ms != UINT32_MAX) {
        tv.tv_sec = (long)(timeout_ms / 1000);
        tv.tv_usec = (long)(timeout_ms % 1000) * 1000;
        tvp = &tv;
    }
    r = select(0, &rs, &ws, &es, tvp);
    if (r == SOCKET_ERROR) return -1;
    return r > 0 ? 1 : 0;
#else
    struct pollfd p;
    int r;
    p.fd = s;
    p.events = want_write ? POLLOUT : POLLIN;
    p.revents = 0;
    for (;;) {
        r = poll(&p, 1, timeout_ms == UINT32_MAX ? -1
                        : (timeout_ms > (uint32_t)INT32_MAX ? INT32_MAX : (int)timeout_ms));
        if (r < 0 && errno == EINTR) continue;
        break;
    }
    if (r < 0) return -1;
    return r > 0 ? 1 : 0;
#endif
}

/* 1 ready, 0 timeout, -1 error, -2 aborted (pc_set_thread_abort) */
static int wait_sock(pc_sock_t s, int want_write, uint32_t timeout_ms)
{
    uint64_t deadline;
    if (!g_thread_abort) return wait_sock_raw(s, want_write, timeout_ms);
    deadline = pc_deadline_after(timeout_ms);
    for (;;) {
        uint32_t left = pc_remaining_ms(deadline);
        int r;
        if (aborted()) return -2;
        r = wait_sock_raw(s, want_write, left < 100 ? left : 100);
        if (r != 0) return r;
        if (left <= 100) return aborted() ? -2 : 0;
    }
}

int pc_wait_readable(pc_sock_t s, uint32_t timeout_ms)
{
    int r = wait_sock(s, 0, timeout_ms);
    return r == -2 ? -1 : r;
}

int pc_accept(pc_sock_t ls, uint32_t timeout_ms, pc_sock_t *out)
{
    int w = wait_sock(ls, 0, timeout_ms);
    pc_sock_t c;
    *out = PC_BAD_SOCK;
    if (w == 0) return POLYCALL_E_TIMEOUT;
    if (w == -2) return POLYCALL_E_CANCELLED;
    if (w < 0) return POLYCALL_E_TRANSPORT;
    c = accept(ls, NULL, NULL);
    if (c == PC_BAD_SOCK) {
        int e = PC_SOCKERR();
        /* a client that reset before we accepted, or a spurious wakeup */
        if (PC_EWOULDBLOCK(e) || PC_EINTR(e)) return POLYCALL_E_TIMEOUT;
#if !defined(_WIN32)
        if (e == ECONNABORTED || e == EPROTO) return POLYCALL_E_TIMEOUT;
#else
        if (e == WSAECONNRESET) return POLYCALL_E_TIMEOUT;
#endif
        return POLYCALL_E_TRANSPORT;
    }
    if (prepare_socket(c) != 0) {
        pc_sock_close(c);
        return POLYCALL_E_TRANSPORT;
    }
    {
        int one = 1;
        setsockopt(c, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    }
    *out = c;
    return POLYCALL_OK;
}

int pc_connect(const char *host, uint16_t port, uint32_t timeout_ms,
               pc_sock_t *out, char *err, size_t errcap)
{
    struct sockaddr_in addr;
    pc_sock_t s;
    int e, w;

    *out = PC_BAD_SOCK;
    if (pc_net_init() != POLYCALL_OK) return POLYCALL_E_TRANSPORT;
    if (!host || !*host || port == 0) return POLYCALL_E_INVALID_ARGUMENT;
    if (resolve_v4(host, port, 0, &addr) != 0) {
        if (err && errcap) snprintf(err, errcap, "cannot resolve '%s'", host);
        return POLYCALL_E_CONFIG;
    }
    s = new_tcp_socket();
    if (s == PC_BAD_SOCK) {
        fmt_sys_err(err, errcap, "socket() failed", PC_SOCKERR());
        return POLYCALL_E_TRANSPORT;
    }
#if defined(_WIN32)
    /* Windows answers a RST to a SYN by retransmitting the SYN for ~2 s,
     * so a dead local peer looks like a timeout. Loopback cannot lose
     * packets: disable SYN retransmission there (and only there) so a
     * refused local port is reported at once as TRANSPORT. */
    if ((ntohl(addr.sin_addr.s_addr) >> 24) == 127) {
        pc_rto_params_t rto;
        DWORD br = 0;
        rto.Rtt = (USHORT)0xFFFF;                 /* unspecified RTT */
        rto.MaxSynRetransmissions = (UCHAR)0xFE;  /* no SYN retransmission */
        WSAIoctl(s, PC_SIO_TCP_INITIAL_RTO, &rto, sizeof rto, NULL, 0, &br, NULL, NULL);
    }
#endif
    if (connect(s, (struct sockaddr *)&addr, sizeof addr) != 0) {
        e = PC_SOCKERR();
        if (!PC_EWOULDBLOCK(e)) {
            pc_sock_close(s);
            fmt_sys_err(err, errcap, "connect() failed", e);
            return POLYCALL_E_TRANSPORT;
        }
        w = wait_sock(s, 1, timeout_ms);
        if (w == -2) {
            pc_sock_close(s);
            return POLYCALL_E_CANCELLED;
        }
        if (w == 0) {
            pc_sock_close(s);
            if (err && errcap) snprintf(err, errcap, "connect to %s:%u timed out after %u ms",
                                        host, (unsigned)port, (unsigned)timeout_ms);
            return POLYCALL_E_TIMEOUT;
        }
        {
            int soerr = 0;
            socklen_t sl = sizeof soerr;
            if (w < 0 || getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&soerr, &sl) != 0 ||
                soerr != 0) {
                pc_sock_close(s);
                fmt_sys_err(err, errcap, "connection refused or unreachable",
                            soerr ? soerr : PC_SOCKERR());
                return POLYCALL_E_TRANSPORT;
            }
        }
    }
    {
        int one = 1;
        setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
    }
    *out = s;
    return POLYCALL_OK;
}

int pc_send_all(pc_sock_t s, const void *buf, size_t len, uint64_t deadline)
{
    const char *p = (const char *)buf;
    size_t off = 0;
    while (off < len) {
        size_t chunk = len - off;
#if defined(_WIN32)
        int n;
        if (chunk > (size_t)INT32_MAX) chunk = (size_t)INT32_MAX;
        n = send(s, p + off, (int)chunk, 0);
        if (n == SOCKET_ERROR) {
#else
        ssize_t n = send(s, p + off, chunk, PC_SEND_FLAGS);
        if (n < 0) {
#endif
            int e = PC_SOCKERR();
            int w;
            if (PC_EINTR(e)) continue;
            if (!PC_EWOULDBLOCK(e)) return POLYCALL_E_TRANSPORT;
            if (pc_remaining_ms(deadline) == 0) return POLYCALL_E_TIMEOUT;
            w = wait_sock(s, 1, pc_remaining_ms(deadline));
            if (w == 0) return POLYCALL_E_TIMEOUT;
            if (w == -2) return POLYCALL_E_CANCELLED;
            if (w < 0) return POLYCALL_E_TRANSPORT;
            continue;
        }
        off += (size_t)n;
    }
    return POLYCALL_OK;
}

int pc_recv_some(pc_sock_t s, void *buf, size_t cap, uint64_t deadline,
                 size_t *got)
{
    *got = 0;
    if (cap == 0) return POLYCALL_OK;
    for (;;) {
#if defined(_WIN32)
        int n = recv(s, (char *)buf, cap > (size_t)INT32_MAX ? INT32_MAX : (int)cap, 0);
        if (n == SOCKET_ERROR) {
#else
        ssize_t n = recv(s, buf, cap, 0);
        if (n < 0) {
#endif
            int e = PC_SOCKERR();
            int w;
            if (PC_EINTR(e)) continue;
            if (!PC_EWOULDBLOCK(e)) return POLYCALL_E_TRANSPORT;
            if (pc_remaining_ms(deadline) == 0) return POLYCALL_E_TIMEOUT;
            w = wait_sock(s, 0, pc_remaining_ms(deadline));
            if (w == 0) return POLYCALL_E_TIMEOUT;
            if (w == -2) return POLYCALL_E_CANCELLED;
            if (w < 0) return POLYCALL_E_TRANSPORT;
            continue;
        }
        *got = (size_t)n;
        return POLYCALL_OK;
    }
}

int pc_recv_exact(pc_sock_t s, void *buf, size_t len, uint64_t deadline)
{
    size_t off = 0;
    while (off < len) {
        size_t got = 0;
        int rc = pc_recv_some(s, (char *)buf + off, len - off, deadline, &got);
        if (rc != POLYCALL_OK) return rc;
        if (got == 0) return POLYCALL_E_TRANSPORT;   /* EOF mid-frame */
        off += got;
    }
    return POLYCALL_OK;
}
