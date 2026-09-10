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
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Machine output must be byte-exact: stop the CRT rewriting \n to \r\n
     * on stdout when a command emits a single JSON object. Text help is
     * unaffected in practice (terminals accept bare \n). */
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    return polycall_cli_main(argc, argv);
}
