/*
 * PolyCall CLI: one argv parser, one command registry, one dispatch path.
 *
 * Contract (see docs/CLI.md):
 *   polycall [global-options] <command> [subcommand] [arguments]
 *
 *   - no arguments            -> root help, exit 0
 *   - unknown option/command  -> usage error, exit 2
 *   - --version / -V          -> version line, exit 0, no runtime init
 *   - --help / -h             -> help for the resolved command, exit 0
 *   - "--"                    -> ends option parsing; the rest are positionals
 *   - globals may appear before or after command tokens, never after "--"
 *   - argv boundaries from the OS are preserved; nothing is re-tokenised
 */

#include "cli_internal.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* global-option parsing                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    polycall_globals_t g;
    const char *command;
    const char *subcommand;
    /* positionals that follow command [subcommand] */
    const char *pos[POLYCALL_CLI_MAX_POSITIONALS];
    int pos_count;
    int dd_index;        /* pos[] index where post-"--" literals begin, -1 */
    bool want_help;
    bool want_version;
    /* duplicate-scalar guards */
    bool seen_project_root, seen_config, seen_language, seen_format,
         seen_timeout;
} parsed_t;

typedef enum {
    PARSE_OK = 0,
    PARSE_ERR = 2   /* always a usage error */
} parse_status_t;

static void globals_defaults(polycall_globals_t *g)
{
    g->project_root = NULL;
    g->config_path = NULL;
    g->language = NULL;
    g->format = POLYCALL_FMT_TEXT;
    g->no_color = false;
    g->quiet = false;
    g->timeout_ms = -1;
}

/* returns 1 and sets *out on success; 0 on malformed integer */
static int parse_nonneg_long(const char *s, long *out)
{
    char *end = NULL;
    long v;
    if (s == NULL || *s == '\0') {
        return 0;
    }
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 0) {
        return 0;
    }
    *out = v;
    return 1;
}

/*
 * Consume one option token at argv[*i]. May consume a following value token.
 * `emsg` receives a static/there-owned message pointer on error.
 */
static parse_status_t take_option(parsed_t *p, int argc, char *const argv[],
                                  int *i, const char **emsg,
                                  const char **ebadopt)
{
    const char *tok = argv[*i];
    const char *eq = strchr(tok, '=');
    char name[64];
    const char *inlineval = NULL;

    if (eq) {
        size_t n = (size_t)(eq - tok);
        if (n >= sizeof name) {
            n = sizeof name - 1;
        }
        memcpy(name, tok, n);
        name[n] = '\0';
        inlineval = eq + 1;
    } else {
        snprintf(name, sizeof name, "%s", tok);
    }

    /* removed options: fail with a pointer to the replacement -------------- */
    if (strcmp(name, "-f") == 0 || strcmp(name, "--file") == 0) {
        *emsg = "removed option; use 'polycall run --config PATH'";
        *ebadopt = "-f";
        return PARSE_ERR;
    }

    /* flags (no value) --------------------------------------------------- */
    if (strcmp(name, "--help") == 0 || strcmp(name, "-h") == 0) {
        if (inlineval) { *emsg = "option takes no value"; *ebadopt = "--help"; return PARSE_ERR; }
        p->want_help = true;
        (*i)++;
        return PARSE_OK;
    }
    if (strcmp(name, "--version") == 0 || strcmp(name, "-V") == 0) {
        if (inlineval) { *emsg = "option takes no value"; *ebadopt = "--version"; return PARSE_ERR; }
        p->want_version = true;
        (*i)++;
        return PARSE_OK;
    }
    if (strcmp(name, "--no-color") == 0) {
        if (inlineval) { *emsg = "option takes no value"; *ebadopt = "--no-color"; return PARSE_ERR; }
        p->g.no_color = true;
        (*i)++;
        return PARSE_OK;
    }
    if (strcmp(name, "--quiet") == 0 || strcmp(name, "-q") == 0) {
        if (inlineval) { *emsg = "option takes no value"; *ebadopt = "--quiet"; return PARSE_ERR; }
        p->g.quiet = true;
        (*i)++;
        return PARSE_OK;
    }

    /* scalar options (require a value) --------------------------------- */
    {
        struct { const char *opt; const char **slot; bool *seen; } table[] = {
            { "--project-root", &p->g.project_root, &p->seen_project_root },
            { "--config",       &p->g.config_path,  &p->seen_config       },
            { "--language",     &p->g.language,     &p->seen_language     },
        };
        size_t k;
        for (k = 0; k < sizeof table / sizeof table[0]; ++k) {
            if (strcmp(name, table[k].opt) != 0) {
                continue;
            }
            if (*table[k].seen) {
                *emsg = "option given more than once";
                *ebadopt = table[k].opt;
                return PARSE_ERR;
            }
            const char *val = inlineval;
            if (!val) {
                if (*i + 1 >= argc) {
                    *emsg = "option requires a value";
                    *ebadopt = table[k].opt;
                    return PARSE_ERR;
                }
                val = argv[*i + 1];
                (*i)++;
            }
            if (*val == '\0') {
                *emsg = "option value is empty";
                *ebadopt = table[k].opt;
                return PARSE_ERR;
            }
            *table[k].slot = val;
            *table[k].seen = true;
            (*i)++;
            return PARSE_OK;
        }
    }

    if (strcmp(name, "--format") == 0) {
        const char *val = inlineval;
        if (p->seen_format) { *emsg = "option given more than once"; *ebadopt = "--format"; return PARSE_ERR; }
        if (!val) {
            if (*i + 1 >= argc) { *emsg = "option requires a value"; *ebadopt = "--format"; return PARSE_ERR; }
            val = argv[*i + 1];
            (*i)++;
        }
        if (strcmp(val, "text") == 0) {
            p->g.format = POLYCALL_FMT_TEXT;
        } else if (strcmp(val, "json") == 0) {
            p->g.format = POLYCALL_FMT_JSON;
        } else {
            *emsg = "expected 'text' or 'json'";
            *ebadopt = "--format";
            return PARSE_ERR;
        }
        p->seen_format = true;
        (*i)++;
        return PARSE_OK;
    }

    if (strcmp(name, "--timeout-ms") == 0) {
        const char *val = inlineval;
        long v = 0;
        if (p->seen_timeout) { *emsg = "option given more than once"; *ebadopt = "--timeout-ms"; return PARSE_ERR; }
        if (!val) {
            if (*i + 1 >= argc) { *emsg = "option requires a value"; *ebadopt = "--timeout-ms"; return PARSE_ERR; }
            val = argv[*i + 1];
            (*i)++;
        }
        if (!parse_nonneg_long(val, &v)) {
            *emsg = "expected a non-negative integer";
            *ebadopt = "--timeout-ms";
            return PARSE_ERR;
        }
        p->g.timeout_ms = v;
        p->seen_timeout = true;
        (*i)++;
        return PARSE_OK;
    }

    *emsg = "unknown option";
    *ebadopt = tok;
    return PARSE_ERR;
}

static bool is_known_global(const char *tok)
{
    static const char *const names[] = {
        "--help", "-h", "--version", "-V", "--no-color", "--quiet", "-q",
        "--project-root", "--config", "--language", "--format", "--timeout-ms"
    };
    const char *eq = strchr(tok, '=');
    size_t n = eq ? (size_t)(eq - tok) : strlen(tok);
    size_t i;
    for (i = 0; i < sizeof names / sizeof names[0]; ++i) {
        if (strlen(names[i]) == n && strncmp(tok, names[i], n) == 0) {
            return true;
        }
    }
    return false;
}

static parse_status_t parse_all(parsed_t *p, int argc, char *const argv[],
                                const char **emsg, const char **ebadopt)
{
    int i = 1;
    bool end_opts = false;

    globals_defaults(&p->g);
    p->dd_index = -1;

    while (i < argc) {
        const char *tok = argv[i];

        if (!end_opts && strcmp(tok, "--") == 0) {
            end_opts = true;
            p->dd_index = p->pos_count;   /* pos[] from here on are literal */
            i++;
            continue;
        }

        if (!end_opts && tok[0] == '-' && tok[1] != '\0') {
            /*
             * A dash token that is not a recognised global, appearing AFTER
             * the command, is a command-local option: hand it to the command
             * verbatim. The command is responsible for rejecting the ones it
             * does not know (still a usage error, exit 2). Before any command,
             * an unrecognised dash token is a global usage error.
             */
            if (p->command != NULL && !is_known_global(tok)) {
                if (p->pos_count >= POLYCALL_CLI_MAX_POSITIONALS) {
                    *emsg = "too many arguments";
                    *ebadopt = tok;
                    return PARSE_ERR;
                }
                p->pos[p->pos_count++] = tok;
                i++;
                continue;
            }
            {
                parse_status_t st = take_option(p, argc, argv, &i, emsg, ebadopt);
                if (st != PARSE_OK) {
                    return st;
                }
            }
            continue;
        }

        /* positional */
        if (p->command == NULL) {
            p->command = tok;
        } else if (p->subcommand == NULL &&
                   polycall_cli_find(p->command) != NULL &&
                   polycall_cli_find(p->command)->sub_count > 0 &&
                   polycall_cli_find_sub(polycall_cli_find(p->command), tok) != NULL) {
            p->subcommand = tok;
        } else {
            if (p->pos_count >= POLYCALL_CLI_MAX_POSITIONALS) {
                *emsg = "too many arguments";
                *ebadopt = tok;
                return PARSE_ERR;
            }
            p->pos[p->pos_count++] = tok;
        }
        i++;
    }
    return PARSE_OK;
}

/* ------------------------------------------------------------------ */
/* error rendering                                                    */
/* ------------------------------------------------------------------ */

static int usage_error(const polycall_globals_t *g, const char *command,
                       const char *msg, const char *badopt)
{
    if (g->format == POLYCALL_FMT_JSON) {
        char m[512];
        if (badopt && *badopt) {
            snprintf(m, sizeof m, "%s: %s", msg, badopt);
        } else {
            snprintf(m, sizeof m, "%s", msg);
        }
        polycall_json_result(stdout, command ? command : "polycall", false,
                             NULL, "usage", m,
                             "run 'polycall help' for the command reference");
    } else {
        if (badopt && *badopt) {
            fprintf(stderr, "polycall: %s: %s\n", msg, badopt);
        } else {
            fprintf(stderr, "polycall: %s\n", msg);
        }
        fprintf(stderr, "Try 'polycall help' for the command reference.\n");
    }
    return POLYCALL_EXIT_USAGE;
}

/* ------------------------------------------------------------------ */
/* entry point                                                        */
/* ------------------------------------------------------------------ */

int polycall_cli_main(int argc, char **argv)
{
    parsed_t p;
    const char *emsg = NULL;
    const char *ebadopt = NULL;
    const polycall_command_t *cmd = NULL;
    polycall_invocation_t inv;

    memset(&p, 0, sizeof p);

    if (parse_all(&p, argc, argv, &emsg, &ebadopt) != PARSE_OK) {
        return usage_error(&p.g, p.command, emsg, ebadopt);
    }

    /* --version wins over everything, never starts anything */
    if (p.want_version) {
        if (p.g.format == POLYCALL_FMT_JSON) {
            char body[256];
            char qv[64], qa[64];
            snprintf(body, sizeof body,
                     "{\"version\":%s,\"abi\":%s,\"cli_schema\":%d}",
                     polycall_json_quote(qv, sizeof qv, polycall_get_version()),
                     polycall_json_quote(qa, sizeof qa, POLYCALL_ABI_VERSION_STRING),
                     POLYCALL_CLI_SCHEMA_VERSION);
            polycall_json_result(stdout, "version", true, body, NULL, NULL, NULL);
        } else {
            printf("polycall %s\n", polycall_get_version());
        }
        return POLYCALL_EXIT_OK;
    }

    /* no command -> root help, success */
    if (p.command == NULL) {
        if (p.g.format == POLYCALL_FMT_JSON) {
            polycall_json_result(stdout, "help", true, "null", NULL, NULL, NULL);
        }
        polycall_cli_print_root_help(stdout);
        return POLYCALL_EXIT_OK;
    }

    cmd = polycall_cli_find(p.command);
    if (cmd == NULL) {
        return usage_error(&p.g, p.command, "unknown command", p.command);
    }

    /* --help anywhere -> help for this command */
    if (p.want_help || strcmp(cmd->name, "help") == 0) {
        polycall_invocation_t hinv;
        /* hp must outlive the polycall_cmd_help() call: keep it in this
         * scope, not an inner block (that was a use-after-scope bug). */
        const char *hp[POLYCALL_CLI_MAX_POSITIONALS + 1];
        int n = 0;

        memset(&hinv, 0, sizeof hinv);
        hinv.g = &p.g;
        hinv.command = "help";
        hinv.literal_from = -1;
        hinv.out = stdout;
        hinv.err = stderr;

        if (strcmp(cmd->name, "help") != 0) {
            hp[n++] = p.command;
            if (p.subcommand) hp[n++] = p.subcommand;
        } else {
            int k;
            for (k = 0; k < p.pos_count; ++k) hp[n++] = p.pos[k];
        }
        hinv.argc = n;
        hinv.argv = (char *const *)hp;
        return polycall_cmd_help(&hinv);
    }

    /* build the invocation for the resolved (sub)command */
    /*
     * Commands that declare no local options must not silently swallow a
     * stray dash token that the global parser passed through. Tokens after
     * "--" are literal and exempt.
     */
    if (!cmd->takes_local_opts) {
        int k;
        int lit = (p.dd_index >= 0) ? p.dd_index : p.pos_count;
        for (k = 0; k < lit; ++k) {
            if (p.pos[k][0] == '-' && p.pos[k][1] != '\0') {
                return usage_error(&p.g, cmd->name, "unknown option", p.pos[k]);
            }
        }
    }

    memset(&inv, 0, sizeof inv);
    inv.g = &p.g;
    inv.command = cmd->name;
    inv.subcommand = p.subcommand;
    inv.argc = p.pos_count;
    inv.argv = (char *const *)p.pos;
    inv.literal_from = p.dd_index;
    inv.out = stdout;
    inv.err = stderr;

    if (cmd->sub_count > 0) {
        const polycall_subcommand_t *s;
        if (p.subcommand == NULL) {
            if (p.g.format != POLYCALL_FMT_JSON) {
                polycall_cli_print_command_help(stderr, cmd);
            }
            return usage_error(&p.g, cmd->name,
                               "missing subcommand", cmd->name);
        }
        s = polycall_cli_find_sub(cmd, p.subcommand);
        if (s == NULL) {
            return usage_error(&p.g, cmd->name,
                               "unknown subcommand", p.subcommand);
        }
        return s->run(&inv);
    }

    if (cmd->run == NULL) {
        return usage_error(&p.g, cmd->name, "command is not runnable", cmd->name);
    }
    return cmd->run(&inv);
}
