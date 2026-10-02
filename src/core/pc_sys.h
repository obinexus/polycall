#ifndef POLYCALL_INTERNAL_PC_SYS_H
#define POLYCALL_INTERNAL_PC_SYS_H

/*
 * Portable system layer for the transports: monotonic time, threads,
 * mutex/condvar, one-time init, and TCP sockets with ABSOLUTE deadlines.
 * Internal; not installed.
 *
 * Every socket made here is non-blocking and non-inheritable. Blocking
 * semantics are rebuilt on top of poll()/select() with a deadline, so no
 * send, receive, connect or accept can wait longer than its caller allows:
 * a stalled or trickling peer costs at most one deadline, never a thread.
 *
 * All int-returning socket helpers return a POLYCALL_* status from
 * polycall.h (0 = POLYCALL_OK, negative = error) unless noted.
 */

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
typedef SOCKET pc_sock_t;
#  define PC_BAD_SOCK INVALID_SOCKET
typedef HANDLE pc_thread_t;
typedef CRITICAL_SECTION pc_mutex_t;
typedef CONDITION_VARIABLE pc_cond_t;
typedef INIT_ONCE pc_once_t;
#  define PC_ONCE_INIT INIT_ONCE_STATIC_INIT
#else
#  include <pthread.h>
typedef int pc_sock_t;
#  define PC_BAD_SOCK (-1)
typedef pthread_t pc_thread_t;
typedef pthread_mutex_t pc_mutex_t;
typedef pthread_cond_t pc_cond_t;
typedef pthread_once_t pc_once_t;
#  define PC_ONCE_INIT PTHREAD_ONCE_INIT
#endif

/* ---- time ------------------------------------------------------------ */
uint64_t pc_mono_ms(void);            /* monotonic, arbitrary epoch      */
void     pc_sleep_ms(uint32_t ms);
/* deadline helpers: 0 timeout -> "now" (poll once); UINT32_MAX -> never */
uint64_t pc_deadline_after(uint32_t timeout_ms);
uint32_t pc_remaining_ms(uint64_t deadline);   /* 0 when passed            */
#define PC_NO_DEADLINE UINT64_MAX

/* ---- threads / sync ------------------------------------------------- */
typedef void (*pc_thread_fn)(void *arg);
int  pc_thread_start(pc_thread_t *t, pc_thread_fn fn, void *arg); /* 0 ok */
void pc_thread_join(pc_thread_t t);

void pc_mutex_init(pc_mutex_t *m);
void pc_mutex_destroy(pc_mutex_t *m);
void pc_mutex_lock(pc_mutex_t *m);
void pc_mutex_unlock(pc_mutex_t *m);

void pc_cond_init(pc_cond_t *c);
void pc_cond_destroy(pc_cond_t *c);
void pc_cond_wait(pc_cond_t *c, pc_mutex_t *m);
/* returns 0 when signalled (or spuriously woken), 1 on timeout */
int  pc_cond_timedwait(pc_cond_t *c, pc_mutex_t *m, uint32_t ms);
void pc_cond_signal(pc_cond_t *c);
void pc_cond_broadcast(pc_cond_t *c);

void pc_once(pc_once_t *o, void (*fn)(void));

/* lock-free flags shared between threads (sequentially consistent) */
int  pc_atomic_load(const volatile int *p);
void pc_atomic_store(volatile int *p, int v);

/* Per-thread abort flag. While set, every socket wait made by THIS thread
 * is sliced into <= 100 ms waits and gives up with POLYCALL_E_CANCELLED as
 * soon as *flag becomes non-zero -- portable prompt cancellation (on
 * Windows, shutdown() from another thread does not wake select()).
 * Pass NULL to clear. */
void pc_set_thread_abort(const volatile int *flag);

/* ---- sockets -------------------------------------------------------- */
/* Process-wide socket startup (WSAStartup on Windows); idempotent and
 * thread-safe. Never torn down: Winsock is released at process exit. */
int  pc_net_init(void);

void pc_sock_close(pc_sock_t s);
/* shut down both directions: wakes any thread blocked on `s` */
void pc_sock_shutdown(pc_sock_t s);
/* Finish sending, then discard what the peer still sends (up to linger_ms)
 * before closing. Closing a socket with unread input makes Windows (and
 * Linux) send a RST, which can destroy a reply the peer has not read yet
 * -- e.g. a "server.busy" or 413 answer sent before reading the request.
 * Does NOT close the socket: the caller does, so it can do that under its
 * own lock. */
void pc_sock_finish(pc_sock_t s, uint32_t linger_ms);

/* "host:port" -> host + port. allow_zero permits port 0 (ephemeral bind).
 * Returns POLYCALL_OK or POLYCALL_E_INVALID_ARGUMENT. */
int  pc_split_endpoint(const char *ep, char *host, size_t hostcap,
                       uint16_t *port, int allow_zero);

/* Listen on host:port (IPv4; "localhost" resolves, "0.0.0.0" = any).
 * Windows uses SO_EXCLUSIVEADDRUSE so no other process can bind the same
 * port over ours; POSIX uses SO_REUSEADDR (TIME_WAIT only, never steal).
 * Distinct statuses: POLYCALL_E_ADDRESS_IN_USE, POLYCALL_E_PERMISSION,
 * POLYCALL_E_CONFIG (not a local address / unresolvable). */
int  pc_listen(const char *host, uint16_t port, int backlog, pc_sock_t *out,
               uint16_t *bound_port, char *err, size_t errcap);

/* Wait up to timeout_ms for a connection. POLYCALL_OK with *out set,
 * POLYCALL_E_TIMEOUT when none arrived, POLYCALL_E_TRANSPORT on error. */
int  pc_accept(pc_sock_t ls, uint32_t timeout_ms, pc_sock_t *out);

/* Connect with a bounded timeout. POLYCALL_E_TRANSPORT (refused /
 * unreachable), POLYCALL_E_TIMEOUT, POLYCALL_E_CONFIG (cannot resolve). */
int  pc_connect(const char *host, uint16_t port, uint32_t timeout_ms,
                pc_sock_t *out, char *err, size_t errcap);

/* Send all bytes before `deadline` (pc_mono_ms clock). */
int  pc_send_all(pc_sock_t s, const void *buf, size_t len, uint64_t deadline);
/* Receive some bytes before `deadline`. *got == 0 means orderly EOF. */
int  pc_recv_some(pc_sock_t s, void *buf, size_t cap, uint64_t deadline,
                  size_t *got);
/* Receive exactly len bytes; EOF before that is POLYCALL_E_TRANSPORT. */
int  pc_recv_exact(pc_sock_t s, void *buf, size_t len, uint64_t deadline);

/* 1 = readable (or EOF/error pending), 0 = timeout, -1 = error */
int  pc_wait_readable(pc_sock_t s, uint32_t timeout_ms);

#endif /* POLYCALL_INTERNAL_PC_SYS_H */
