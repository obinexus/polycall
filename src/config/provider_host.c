/*
 * Provider hosts: sub-process (argv array, deadline, bounded output) and
 * in-process C (dlopen / LoadLibraryExW of an explicit path).
 */

#include "polycall_provider.h"
#include "polycall_config2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <signal.h>
#  include <spawn.h>
#  include <sys/wait.h>
#  include <time.h>
#  include <unistd.h>
extern char **environ;
#endif

void polycall_provider_opts_init(polycall_provider_opts_t *o)
{
    if (o) {
        memset(o, 0, sizeof *o);
        o->struct_size = (uint32_t)sizeof *o;
    }
}

static uint32_t clamp_deadline(uint32_t d)
{
    if (d == 0) {
        return POLYCALL_CFG2_PROVIDER_DEADLINE_MS_DEFAULT;
    }
    if (d > POLYCALL_CFG2_PROVIDER_DEADLINE_MS_MAX) {
        return POLYCALL_CFG2_PROVIDER_DEADLINE_MS_MAX;
    }
    return d;
}

static uint32_t out_budget(const polycall_provider_opts_t *o)
{
    uint32_t b = o ? o->max_output_bytes : 0;
    if (b == 0 || b > POLYCALL_CFG2_ENVELOPE_MAX) {
        return POLYCALL_CFG2_ENVELOPE_MAX;
    }
    return b;
}

static int fail(polycall_config2_error_t *err, const char *code, const char *msg)
{
    if (err) {
        if (err->struct_size == 0) err->struct_size = (uint32_t)sizeof *err;
        snprintf(err->code, sizeof err->code, "%s", code);
        snprintf(err->message, sizeof err->message, "%s", msg);
        err->field[0] = '\0';
    }
    return -1;
}

/* ================================================================== */
/* sub-process host                                                    */
/* ================================================================== */

#if defined(_WIN32)

/* Quote one argv element per the MS C runtime rules. */
static void win_quote(const char *arg, char **w)
{
    const char *p;
    int needs = (*arg == '\0');
    for (p = arg; *p; ++p) {
        if (*p == ' ' || *p == '\t' || *p == '"') { needs = 1; break; }
    }
    if (!needs) {
        for (p = arg; *p; ++p) *(*w)++ = *p;
        return;
    }
    *(*w)++ = '"';
    for (p = arg;; ++p) {
        unsigned slashes = 0;
        while (*p == '\\') { slashes++; p++; }
        if (*p == '\0') {
            for (; slashes; --slashes) { *(*w)++ = '\\'; *(*w)++ = '\\'; }
            break;
        } else if (*p == '"') {
            for (; slashes; --slashes) { *(*w)++ = '\\'; *(*w)++ = '\\'; }
            *(*w)++ = '\\'; *(*w)++ = '"';
        } else {
            for (; slashes; --slashes) *(*w)++ = '\\';
            *(*w)++ = *p;
        }
    }
    *(*w)++ = '"';
}

static char *win_build_cmdline(const char *const *argv)
{
    size_t total = 1;
    int i;
    char *cmd, *w;
    for (i = 0; argv[i]; ++i) {
        total += strlen(argv[i]) * 2 + 3;
    }
    cmd = malloc(total);
    if (!cmd) return NULL;
    w = cmd;
    for (i = 0; argv[i]; ++i) {
        if (i) *w++ = ' ';
        win_quote(argv[i], &w);
    }
    *w = '\0';
    return cmd;
}

polycall_config2_t *polycall_provider_run_subprocess(
    const char *const *argv, const polycall_provider_opts_t *opts,
    polycall_config2_error_t *err)
{
    HANDLE rd = NULL, wr = NULL;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char *cmdline;
    char *buf;
    uint32_t cap, deadline;
    DWORD start, waited;
    size_t len = 0;
    polycall_config2_t *cfg = NULL;
    BOOL running = TRUE;

    polycall_config2_error_init(err);
    if (!argv || !argv[0]) {
        fail(err, "provider.argv", "empty provider argv");
        return NULL;
    }
    cap = out_budget(opts);
    deadline = clamp_deadline(opts ? opts->deadline_ms : 0);

    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        fail(err, "provider.pipe", "CreatePipe failed");
        return NULL;
    }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    cmdline = win_build_cmdline(argv);
    if (!cmdline) {
        CloseHandle(rd); CloseHandle(wr);
        fail(err, "oom", "out of memory");
        return NULL;
    }

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    memset(&pi, 0, sizeof pi);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL,
                        opts ? opts->cwd : NULL, &si, &pi)) {
        DWORD e = GetLastError();
        char m[160];
        snprintf(m, sizeof m, "cannot start provider '%s' (error %lu)",
                 argv[0], (unsigned long)e);
        CloseHandle(rd); CloseHandle(wr); free(cmdline);
        fail(err, "provider.spawn", m);
        return NULL;
    }
    CloseHandle(wr); /* parent keeps only the read end */
    free(cmdline);

    buf = malloc(cap + 1);
    if (!buf) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
        fail(err, "oom", "out of memory");
        return NULL;
    }

    start = GetTickCount();
    for (;;) {
        DWORD avail = 0, got = 0;
        waited = GetTickCount() - start;
        if (waited >= deadline) {
            TerminateProcess(pi.hProcess, 1);
            free(buf);
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
            fail(err, "provider.timeout", "provider exceeded its deadline");
            return NULL;
        }
        if (PeekNamedPipe(rd, NULL, 0, NULL, &avail, NULL) && avail > 0) {
            DWORD want = avail;
            if (want > cap - (DWORD)len) want = cap - (DWORD)len;
            if (want == 0) {
                TerminateProcess(pi.hProcess, 1);
                free(buf);
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
                fail(err, "provider.output_too_large",
                     "provider output exceeded the envelope budget");
                return NULL;
            }
            if (ReadFile(rd, buf + len, want, &got, NULL) && got > 0) {
                len += got;
                continue;
            }
        }
        if (running) {
            DWORD w = WaitForSingleObject(pi.hProcess, 15);
            if (w == WAIT_OBJECT_0) {
                running = FALSE; /* drain remaining, then exit loop next pass */
            }
        } else {
            DWORD avail2 = 0;
            if (!PeekNamedPipe(rd, NULL, 0, NULL, &avail2, NULL) || avail2 == 0) {
                break;
            }
        }
    }
    buf[len] = '\0';

    {
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread); CloseHandle(rd);
        if (code != 0) {
            char m[128];
            snprintf(m, sizeof m, "provider exited with status %lu",
                     (unsigned long)code);
            free(buf);
            fail(err, "provider.exit", m);
            return NULL;
        }
    }

    cfg = polycall_config2_from_envelope(buf, len, err);
    free(buf);
    return cfg;
}

#else /* POSIX */

static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

polycall_config2_t *polycall_provider_run_subprocess(
    const char *const *argv, const polycall_provider_opts_t *opts,
    polycall_config2_error_t *err)
{
    int pipefd[2];
    posix_spawn_file_actions_t fa;
    pid_t pid;
    char *buf;
    uint32_t cap, deadline;
    size_t len = 0;
    long start;
    int spawn_rc;
    polycall_config2_t *cfg = NULL;
    int status = 0;
    bool timed_out = false;

    polycall_config2_error_init(err);
    if (!argv || !argv[0]) {
        fail(err, "provider.argv", "empty provider argv");
        return NULL;
    }
    cap = out_budget(opts);
    deadline = clamp_deadline(opts ? opts->deadline_ms : 0);

    if (pipe(pipefd) != 0) {
        fail(err, "provider.pipe", "pipe() failed");
        return NULL;
    }

    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&fa, pipefd[0]);
    posix_spawn_file_actions_addclose(&fa, pipefd[1]);

    /* chdir via a wrapper is avoided; use a child that respects cwd through
     * a helper only when asked. Most callers pass an absolute argv[0]. */
    if (opts && opts->cwd) {
        /* posix_spawn has no chdir file action portably; emulate with fork. */
        posix_spawn_file_actions_destroy(&fa);
        pid = fork();
        if (pid == 0) {
            if (chdir(opts->cwd) != 0) _exit(127);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(argv[0], (char *const *)argv);
            _exit(127);
        }
        if (pid < 0) {
            close(pipefd[0]); close(pipefd[1]);
            fail(err, "provider.spawn", "fork() failed");
            return NULL;
        }
    } else {
        spawn_rc = posix_spawnp(&pid, argv[0], &fa, NULL,
                                (char *const *)argv, environ);
        posix_spawn_file_actions_destroy(&fa);
        if (spawn_rc != 0) {
            char m[160];
            snprintf(m, sizeof m, "cannot start provider '%s' (%s)",
                     argv[0], strerror(spawn_rc));
            close(pipefd[0]); close(pipefd[1]);
            fail(err, "provider.spawn", m);
            return NULL;
        }
    }
    close(pipefd[1]);

    buf = malloc(cap + 1);
    if (!buf) {
        close(pipefd[0]);
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        fail(err, "oom", "out of memory");
        return NULL;
    }

    start = now_ms();
    for (;;) {
        struct pollfd pfd;
        long elapsed = now_ms() - start;
        int to = (int)((long)deadline - elapsed);
        int pr;
        if (to <= 0) { timed_out = true; break; }

        pfd.fd = pipefd[0];
        pfd.events = POLLIN;
        pr = poll(&pfd, 1, to);
        if (pr == 0) { timed_out = true; break; }
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pfd.revents & (POLLIN | POLLHUP)) {
            ssize_t r;
            if (len >= cap) {
                timed_out = false;
                fail(err, "provider.output_too_large",
                     "provider output exceeded the envelope budget");
                free(buf);
                close(pipefd[0]);
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                return NULL;
            }
            r = read(pipefd[0], buf + len, cap - len);
            if (r > 0) { len += (size_t)r; continue; }
            if (r == 0) break;       /* EOF */
            if (errno == EINTR) continue;
            break;
        }
    }
    close(pipefd[0]);
    buf[len] = '\0';

    if (timed_out) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        free(buf);
        fail(err, "provider.timeout", "provider exceeded its deadline");
        return NULL;
    }

    if (waitpid(pid, &status, 0) < 0) {
        free(buf);
        fail(err, "provider.wait", "waitpid failed");
        return NULL;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        char m[128];
        snprintf(m, sizeof m, "provider exited abnormally (status %d)", status);
        free(buf);
        fail(err, "provider.exit", m);
        return NULL;
    }

    cfg = polycall_config2_from_envelope(buf, len, err);
    free(buf);
    return cfg;
}

#endif

/* ================================================================== */
/* in-process C provider host                                          */
/* ================================================================== */

static void dirname_of(const char *path, char *out, size_t cap)
{
    const char *slash = NULL, *p;
    for (p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') slash = p;
    }
    if (slash && (size_t)(slash - path) < cap) {
        size_t n = (size_t)(slash - path);
        memcpy(out, path, n);
        out[n] = '\0';
    } else {
        snprintf(out, cap, ".");
    }
}

polycall_config2_t *polycall_provider_load_c(const char *library_path,
                                             const polycall_provider_opts_t *opts,
                                             polycall_config2_error_t *err)
{
    polycall_provider_ctx_t ctx;
    polycall_config2_builder_t *b;
    polycall_config2_t *cfg;
    polycall_config_provider_v1_fn entry;
    char base[POLYCALL_CFG2_PATH_MAX];
    int prc;

    polycall_config2_error_init(err);
    if (!library_path || !*library_path) {
        fail(err, "provider.path", "empty C provider path");
        return NULL;
    }
    dirname_of(library_path, base, sizeof base);

    memset(&ctx, 0, sizeof ctx);
    ctx.struct_size = (uint32_t)sizeof ctx;
    ctx.abi_version = POLYCALL_PROVIDER_ABI_VERSION;
    ctx.base_dir = base;
    ctx.project_root = opts ? opts->project_root : NULL;
    ctx.language = opts ? opts->language : NULL;

#if defined(_WIN32)
    {
        wchar_t wpath[POLYCALL_CFG2_PATH_MAX];
        HMODULE h;
        int wn = MultiByteToWideChar(CP_UTF8, 0, library_path, -1, wpath,
                                     (int)(sizeof wpath / sizeof wpath[0]));
        if (wn <= 0) {
            fail(err, "provider.path", "cannot widen provider path");
            return NULL;
        }
        h = LoadLibraryExW(wpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!h) {
            char m[160];
            snprintf(m, sizeof m, "LoadLibraryEx('%s') failed (error %lu)",
                     library_path, (unsigned long)GetLastError());
            fail(err, "provider.dlopen", m);
            return NULL;
        }
        entry = (polycall_config_provider_v1_fn)(void *)
                GetProcAddress(h, "polycall_config_provider_v1");
        if (!entry) {
            fail(err, "provider.symbol",
                 "provider has no polycall_config_provider_v1");
            FreeLibrary(h);
            return NULL;
        }
        b = polycall_config2_builder_create();
        if (!b) { FreeLibrary(h); fail(err, "oom", "out of memory"); return NULL; }
        polycall_config2_builder_set_base_dir(b, base);
        prc = entry(b, &ctx);
        if (prc != 0) {
            char m[96];
            snprintf(m, sizeof m, "C provider returned %d", prc);
            polycall_config2_builder_free(b);
            FreeLibrary(h);
            fail(err, "provider.failed", m);
            return NULL;
        }
        cfg = polycall_config2_builder_build(b, err);
        polycall_config2_builder_free(b);
        /* keep the module resident: cfg is a plain copy, but be tidy */
        FreeLibrary(h);
        return cfg;
    }
#else
    {
        void *h = dlopen(library_path, RTLD_NOW | RTLD_LOCAL);
        if (!h) {
            char m[200];
            snprintf(m, sizeof m, "dlopen('%s') failed: %s", library_path,
                     dlerror());
            fail(err, "provider.dlopen", m);
            return NULL;
        }
        *(void **)(&entry) = dlsym(h, "polycall_config_provider_v1");
        if (!entry) {
            fail(err, "provider.symbol",
                 "provider has no polycall_config_provider_v1");
            dlclose(h);
            return NULL;
        }
        b = polycall_config2_builder_create();
        if (!b) { dlclose(h); fail(err, "oom", "out of memory"); return NULL; }
        polycall_config2_builder_set_base_dir(b, base);
        prc = entry(b, &ctx);
        if (prc != 0) {
            char m[96];
            snprintf(m, sizeof m, "C provider returned %d", prc);
            polycall_config2_builder_free(b);
            dlclose(h);
            fail(err, "provider.failed", m);
            return NULL;
        }
        cfg = polycall_config2_builder_build(b, err);
        polycall_config2_builder_free(b);
        dlclose(h);
        return cfg;
    }
#endif
}
