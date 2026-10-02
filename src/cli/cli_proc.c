/*
 * Process helpers for daemon management: self path, detached spawn,
 * liveness, forced termination. POSIX and Windows.
 */

#if defined(_WIN32) && !defined(_WIN32_WINNT)
#  define _WIN32_WINNT 0x0601   /* PROC_THREAD_ATTRIBUTE_HANDLE_LIST */
#endif

#include "cli_internal.h"

#include "../core/pc_buf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <signal.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <time.h>
#  include <unistd.h>
#  if defined(__APPLE__)
#    include <mach-o/dyld.h>
#  endif
#endif

long polycall_cli_getpid(void)
{
#if defined(_WIN32)
    return (long)GetCurrentProcessId();
#else
    return (long)getpid();
#endif
}

#if defined(_WIN32)

/* The process we spawned last: keep its handle so its exit code can still
 * be read after it exits (a pid alone cannot be reopened once it is gone). */
static HANDLE g_child;
static DWORD g_child_pid;

static wchar_t *utf8_to_wide(const char *s)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w;
    if (n <= 0) return NULL;
    w = malloc((size_t)n * sizeof *w);
    if (w && MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n) <= 0) {
        free(w);
        return NULL;
    }
    return w;
}

int polycall_cli_self_path(char *buf, size_t cap)
{
    wchar_t w[1024];
    DWORD n = GetModuleFileNameW(NULL, w, (DWORD)(sizeof w / sizeof w[0]));
    if (n == 0 || n >= sizeof w / sizeof w[0]) return -1;
    if (WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, (int)cap, NULL, NULL) <= 0) return -1;
    return 0;
}

/* One argument quoted so CommandLineToArgvW / the MSVC CRT give back
 * exactly `arg`: backslashes are literal unless they precede a quote,
 * where they (and the quote) must be escaped. */
static void win_quote_arg(pc_buf_t *b, const char *arg)
{
    const char *p;
    if (*arg && !strpbrk(arg, " \t\n\v\"")) {
        pc_buf_append(b, arg);
        return;
    }
    pc_buf_append(b, "\"");
    for (p = arg;; ++p) {
        size_t backslashes = 0;
        while (*p == '\\') { ++p; ++backslashes; }
        if (*p == '\0') {
            size_t i;
            for (i = 0; i < backslashes * 2; ++i) pc_buf_append(b, "\\");
            break;
        } else if (*p == '"') {
            size_t i;
            for (i = 0; i < backslashes * 2 + 1; ++i) pc_buf_append(b, "\\");
            pc_buf_append(b, "\"");
        } else {
            size_t i;
            for (i = 0; i < backslashes; ++i) pc_buf_append(b, "\\");
            pc_buf_append_n(b, p, 1);
        }
    }
    pc_buf_append(b, "\"");
}

int polycall_cli_spawn_detached(const char *const *argv, const char *log_path,
                                long *pid_out, char *err, size_t errcap)
{
    pc_buf_t cmd;
    wchar_t *wcmd = NULL, *wapp = NULL, *wlog = NULL;
    HANDLE hlog = INVALID_HANDLE_VALUE, hnull = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOEXW si;
    PROCESS_INFORMATION pi;
    SIZE_T attr_size = 0;
    HANDLE inherit[2];
    int i, rc = -1;
    BOOL ok;

    pc_buf_init(&cmd, 32767);
    for (i = 0; argv[i]; ++i) {
        if (i) pc_buf_append(&cmd, " ");
        win_quote_arg(&cmd, argv[i]);
    }
    if (pc_buf_failed(&cmd)) {
        if (err) snprintf(err, errcap, "command line too long");
        pc_buf_free(&cmd);
        return -1;
    }
    wcmd = utf8_to_wide(cmd.data);
    wapp = utf8_to_wide(argv[0]);
    pc_buf_free(&cmd);
    if (!wcmd || !wapp) goto done;

    memset(&sa, 0, sizeof sa);
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    hnull = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    if (log_path && *log_path) {
        wlog = utf8_to_wide(log_path);
        if (wlog) {
            hlog = CreateFileW(wlog, FILE_APPEND_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        if (hlog == INVALID_HANDLE_VALUE) {
            if (err) snprintf(err, errcap, "cannot open log file '%s' (error %lu)",
                              log_path, (unsigned long)GetLastError());
            goto done;
        }
    }
    if (hnull == INVALID_HANDLE_VALUE) {
        if (err) snprintf(err, errcap, "cannot open NUL");
        goto done;
    }

    memset(&si, 0, sizeof si);
    si.StartupInfo.cb = sizeof si;
    si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    si.StartupInfo.hStdInput = hnull;
    si.StartupInfo.hStdOutput = hlog != INVALID_HANDLE_VALUE ? hlog : hnull;
    si.StartupInfo.hStdError = si.StartupInfo.hStdOutput;

    /* inherit ONLY the standard handles, nothing else this process holds */
    inherit[0] = hnull;
    inherit[1] = hlog;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)malloc(attr_size);
    if (!si.lpAttributeList ||
        !InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attr_size) ||
        !UpdateProcThreadAttribute(si.lpAttributeList, 0,
                                   PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherit,
                                   (hlog != INVALID_HANDLE_VALUE ? 2 : 1) * sizeof(HANDLE),
                                   NULL, NULL)) {
        if (err) snprintf(err, errcap, "cannot prepare the child's handle list");
        free(si.lpAttributeList);
        si.lpAttributeList = NULL;
        goto done;
    }

    memset(&pi, 0, sizeof pi);
    ok = CreateProcessW(wapp, wcmd, NULL, NULL, TRUE,
                        DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP |
                        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
                        NULL, NULL, &si.StartupInfo, &pi);
    DeleteProcThreadAttributeList(si.lpAttributeList);
    free(si.lpAttributeList);
    if (!ok) {
        if (err) snprintf(err, errcap, "CreateProcess failed (error %lu)",
                          (unsigned long)GetLastError());
        goto done;
    }
    *pid_out = (long)pi.dwProcessId;
    CloseHandle(pi.hThread);
    if (g_child) CloseHandle(g_child);
    g_child = pi.hProcess;
    g_child_pid = pi.dwProcessId;
    rc = 0;

done:
    if (hlog != INVALID_HANDLE_VALUE) CloseHandle(hlog);
    if (hnull != INVALID_HANDLE_VALUE) CloseHandle(hnull);
    free(wcmd);
    free(wapp);
    free(wlog);
    return rc;
}

int polycall_cli_pid_alive(long pid)
{
    HANDLE h;
    DWORD r;
    if (pid <= 0) return 0;
    h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (!h) return GetLastError() == ERROR_ACCESS_DENIED ? 1 : 0;
    r = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return r == WAIT_TIMEOUT;
}

int polycall_cli_kill(long pid)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, (DWORD)pid);
    BOOL ok;
    if (!h) return -1;
    ok = TerminateProcess(h, 137);
    if (ok) WaitForSingleObject(h, 5000);
    CloseHandle(h);
    return ok ? 0 : -1;
}

int polycall_cli_child_exited(long pid, int *code)
{
    HANDLE h;
    DWORD ec = 0;
    int exited = 0, own = (g_child && g_child_pid == (DWORD)pid);
    h = own ? g_child
            : OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
    if (!h) {
        if (code) *code = -1;
        return 1;       /* gone entirely, exit code unknown */
    }
    if (WaitForSingleObject(h, 0) == WAIT_OBJECT_0) {
        exited = 1;
        if (GetExitCodeProcess(h, &ec) && code) *code = (int)ec;
    }
    if (!own) CloseHandle(h);
    return exited;
}

#else /* POSIX */

int polycall_cli_self_path(char *buf, size_t cap)
{
#if defined(__linux__)
    ssize_t n = readlink("/proc/self/exe", buf, cap - 1);
    if (n <= 0 || (size_t)n >= cap - 1) return -1;
    buf[n] = '\0';
    return 0;
#elif defined(__APPLE__)
    uint32_t sz = (uint32_t)cap;
    char tmp[4096];
    if (_NSGetExecutablePath(buf, &sz) != 0) return -1;
    if (realpath(buf, tmp)) {
        if (strlen(tmp) >= cap) return -1;
        strcpy(buf, tmp);
    }
    return 0;
#else
    (void)buf; (void)cap;
    return -1;
#endif
}

int polycall_cli_spawn_detached(const char *const *argv, const char *log_path,
                                long *pid_out, char *err, size_t errcap)
{
    int logfd = -1, devnull;
    pid_t pid;

    if (log_path && *log_path) {
        logfd = open(log_path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
        if (logfd < 0) {
            if (err) snprintf(err, errcap, "cannot open log file '%s': %s",
                              log_path, strerror(errno));
            return -1;
        }
    }
    pid = fork();
    if (pid < 0) {
        if (err) snprintf(err, errcap, "fork failed: %s", strerror(errno));
        if (logfd >= 0) close(logfd);
        return -1;
    }
    if (pid == 0) {
        long fd, maxfd;
        setsid();
        devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) dup2(devnull, 0);
        if (logfd >= 0) {
            dup2(logfd, 1);
            dup2(logfd, 2);
        } else if (devnull >= 0) {
            dup2(devnull, 1);
            dup2(devnull, 2);
        }
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd < 0 || maxfd > 65536) maxfd = 65536;
        for (fd = 3; fd < maxfd; ++fd) close((int)fd);
        signal(SIGPIPE, SIG_DFL);
        execv(argv[0], (char *const *)argv);
        _exit(127);
    }
    if (logfd >= 0) close(logfd);
    *pid_out = (long)pid;
    return 0;
}

int polycall_cli_pid_alive(long pid)
{
    int st;
    if (pid <= 0) return 0;
    /* reap it first if it is our own exited child (a zombie still "exists") */
    if (waitpid((pid_t)pid, &st, WNOHANG) == (pid_t)pid) return 0;
    if (kill((pid_t)pid, 0) == 0) return 1;
    return errno == EPERM;
}

int polycall_cli_kill(long pid)
{
    int i;
    if (pid <= 0) return -1;
    if (kill((pid_t)pid, SIGKILL) != 0 && errno != ESRCH) return -1;
    for (i = 0; i < 100 && polycall_cli_pid_alive(pid); ++i) {
        struct timespec ts = { 0, 50 * 1000000L };
        nanosleep(&ts, NULL);
    }
    return 0;
}

int polycall_cli_child_exited(long pid, int *code)
{
    int st = 0;
    pid_t r = waitpid((pid_t)pid, &st, WNOHANG);
    if (r == (pid_t)pid) {
        if (code) {
            if (WIFEXITED(st)) *code = WEXITSTATUS(st);
            else if (WIFSIGNALED(st)) *code = 128 + WTERMSIG(st);
            else *code = -1;
        }
        return 1;
    }
    if (r < 0 && errno == ECHILD) {
        /* not our child (or already reaped): exited if it no longer runs */
        if (kill((pid_t)pid, 0) != 0 && errno == ESRCH) {
            if (code) *code = -1;
            return 1;
        }
    }
    return 0;
}

#endif
