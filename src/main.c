/*
 * PolyCall process entry point.
 *
 * Minimal by intent: platform bootstrap, hand off to the one CLI dispatch
 * path, map its result to the process exit code. No global runtime, no
 * command table, no REPL here -- see src/cli/.
 */

#include "polycall_cli.h"

#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <shellapi.h>

/* The CRT hands main() an argv decoded in the ANSI code page, which turns
 * every character outside it into '?' -- a payload or path given on the
 * command line would be silently corrupted. Rebuild argv as UTF-8 from the
 * wide command line instead (argv boundaries exactly as Windows splits
 * them). Returns NULL on failure (caller falls back to the CRT argv). */
static char **utf8_argv(int *argc_out)
{
    int argc = 0, i;
    LPWSTR *w = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **out;
    if (!w) return NULL;
    out = calloc((size_t)argc + 1, sizeof *out);
    if (!out) { LocalFree(w); return NULL; }
    for (i = 0; i < argc; ++i) {
        int n = WideCharToMultiByte(CP_UTF8, 0, w[i], -1, NULL, 0, NULL, NULL);
        out[i] = n > 0 ? malloc((size_t)n) : NULL;
        if (!out[i] || WideCharToMultiByte(CP_UTF8, 0, w[i], -1, out[i], n, NULL, NULL) <= 0) {
            LocalFree(w);
            return NULL;      /* (leak on this rare path is irrelevant: we exit) */
        }
    }
    LocalFree(w);
    *argc_out = argc;
    return out;
}
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    int wargc = 0;
    char **wargv = utf8_argv(&wargc);
    if (wargv) {
        argc = wargc;
        argv = wargv;
    }
    /* Machine output must be byte-exact: stop the CRT rewriting \n to \r\n
     * on stdout when a command emits a single JSON object. Text help is
     * unaffected in practice (terminals accept bare \n). */
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    return polycall_cli_main(argc, argv);
}
