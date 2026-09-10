/*
 * Explicit interactive session.
 *
 * The REPL is never entered implicitly (see docs/CLI.md). It is reached only
 * by `polycall repl`. Each line is parsed and dispatched through the very same
 * registry and argv parser as a one-shot invocation, so behaviour cannot
 * drift between the two. readline is intentionally not required.
 */

#include "cli_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define REPL_MAX_LINE 4096
#define REPL_MAX_ARGS 64

/*
 * Split one line into argv, honouring "double quotes" and \-escapes so paths
 * with spaces survive. Tokens are carved in place inside `line`. Returns argc,
 * or -1 on an unterminated quote.
 */
static int split_line(char *line, char **argv, int max)
{
    int argc = 0;
    char *w = line;      /* write cursor (compacted token store) */
    const char *r = line;

    while (*r) {
        while (*r && isspace((unsigned char)*r)) {
            r++;
        }
        if (!*r) {
            break;
        }
        if (argc >= max) {
            return argc; /* silently stop; caller will still dispatch */
        }
        argv[argc++] = w;
        while (*r && !isspace((unsigned char)*r)) {
            if (*r == '"') {
                r++;
                while (*r && *r != '"') {
                    if (*r == '\\' && (r[1] == '"' || r[1] == '\\')) {
                        r++;
                    }
                    *w++ = *r++;
                }
                if (*r != '"') {
                    return -1; /* unterminated quote */
                }
                r++;
            } else if (*r == '\\' && (r[1] == ' ' || r[1] == '"' || r[1] == '\\')) {
                r++;
                *w++ = *r++;
            } else {
                *w++ = *r++;
            }
        }
        *w++ = '\0';
    }
    return argc;
}

int polycall_repl_run(const polycall_invocation_t *inv)
{
    char line[REPL_MAX_LINE];

    if (inv->g->format == POLYCALL_FMT_JSON) {
        fprintf(inv->err, "polycall: repl is an interactive session; "
                          "--format json is not meaningful here\n");
        return POLYCALL_EXIT_USAGE;
    }

    fprintf(inv->out,
            "polycall %s interactive session. Type 'help', or 'quit' to exit.\n",
            polycall_get_version());

    for (;;) {
        char *args[REPL_MAX_ARGS];
        char *av[REPL_MAX_ARGS + 1];
        int ac, i, n;

        fputs("polycall> ", inv->out);
        fflush(inv->out);

        if (!fgets(line, sizeof line, stdin)) {
            fputc('\n', inv->out);
            break; /* EOF: leave the loop, never spin */
        }
        line[strcspn(line, "\r\n")] = '\0';

        ac = split_line(line, args, REPL_MAX_ARGS);
        if (ac == 0) {
            continue;
        }
        if (ac < 0) {
            fprintf(inv->err, "polycall: unterminated quote\n");
            continue;
        }
        if (strcmp(args[0], "quit") == 0 || strcmp(args[0], "exit") == 0) {
            break;
        }

        /* Reuse the exact one-shot path: argv[0] = program name. */
        n = 0;
        av[n++] = "polycall";
        for (i = 0; i < ac && n <= REPL_MAX_ARGS; ++i) {
            av[n++] = args[i];
        }
        (void)polycall_cli_main(n, av);
    }

    return POLYCALL_EXIT_OK;
}
