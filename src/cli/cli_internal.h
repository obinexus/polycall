#ifndef POLYCALL_CLI_INTERNAL_H
#define POLYCALL_CLI_INTERNAL_H

/*
 * Internal contract shared by the CLI translation units. Not installed.
 */

#include <stdbool.h>
#include <stdio.h>

#include "polycall.h"      /* preserved public ABI: version / init / cleanup */
#include "polycall_cli.h"

#define POLYCALL_CLI_MAX_COMMANDS 32
#define POLYCALL_CLI_MAX_SUBCOMMANDS 16
#define POLYCALL_CLI_MAX_POSITIONALS 32

/* ---- parsed global options ------------------------------------------------ */

typedef enum {
    POLYCALL_FMT_TEXT = 0,
    POLYCALL_FMT_JSON = 1
} polycall_format_t;

typedef struct {
    const char *project_root;   /* --project-root PATH            */
    const char *config_path;    /* --config PATH                 */
    const char *language;       /* --language LANG               */
    polycall_format_t format;   /* --format text|json            */
    bool no_color;              /* --no-color                    */
    bool quiet;                 /* --quiet                       */
    long timeout_ms;            /* --timeout-ms N (-1 = unset)   */
} polycall_globals_t;

/* ---- invocation passed to a command handler ------------------------------ */

typedef struct {
    const polycall_globals_t *g;
    const char *command;                     /* resolved top-level command   */
    const char *subcommand;                  /* resolved subcommand or NULL  */
    int argc;                                /* positional count             */
    char *const *argv;                       /* positionals after cmd/subcmd */
    int literal_from;                        /* argv index >= which tokens
                                                followed "--" (-1 = none)    */
    FILE *out;                               /* stdout sink                   */
    FILE *err;                               /* stderr sink                   */
} polycall_invocation_t;

typedef int (*polycall_cmd_fn)(const polycall_invocation_t *inv);

typedef struct {
    const char *name;
    const char *summary;
    const char *usage;      /* one line, no leading "polycall " */
    polycall_cmd_fn run;
} polycall_subcommand_t;

typedef struct {
    const char *name;
    const char *summary;
    const char *usage;
    const char *help_body;  /* multi-line long help, may be NULL */
    polycall_cmd_fn run;    /* used when there are no subcommands */
    const polycall_subcommand_t *subs;
    int sub_count;
    bool starts_runtime;    /* documentation flag: help/doctor never call these */
    bool takes_local_opts;  /* if false, a stray -x positional is a usage error */
} polycall_command_t;

/* ---- registry ---------------------------------------------------------- */

const polycall_command_t *polycall_cli_registry(int *count);
const polycall_command_t *polycall_cli_find(const char *name);
const polycall_subcommand_t *polycall_cli_find_sub(const polycall_command_t *c,
                                                   const char *name);

/* ---- machine (JSON) output helpers ----------------------------------- */

/* Emit exactly one JSON object: {schema_version, ok, command, data, error}.
 * `data_json` is a raw JSON fragment (object/array/scalar) or NULL -> null.
 * `err_code`/`err_msg`/`err_hint` are ignored when ok is true. */
void polycall_json_result(FILE *out, const char *command, bool ok,
                          const char *data_json,
                          const char *err_code, const char *err_msg,
                          const char *err_hint);

/* Escape a C string into a JSON string literal (with surrounding quotes).
 * Writes to buf; returns buf. Truncates safely if too small. */
char *polycall_json_quote(char *buf, size_t buf_len, const char *s);

/* ---- shared helpers -------------------------------------------------- */

int polycall_cmd_help(const polycall_invocation_t *inv);   /* commands_help.c */
void polycall_cli_print_root_help(FILE *out);
void polycall_cli_print_command_help(FILE *out, const polycall_command_t *c);

/* the carved-out interactive loop (explicit `repl` command) */
int polycall_repl_run(const polycall_invocation_t *inv);

/* bridge to the legacy config CLI in src/polycall_config.c */
int polycall_config_cli(int argc, char *argv[]);

/* native (schema v2) config subcommands + inventory (src/cli/cmd_config2.c) */
int polycall_cmd_config_validate(const polycall_invocation_t *inv);
int polycall_cmd_config_show(const polycall_invocation_t *inv);
int polycall_cmd_config_migrate(const polycall_invocation_t *inv);

/* runtime dispatch: start/status/stop/call (src/cli/cmd_runtime.c) */
int polycall_cmd_start(const polycall_invocation_t *inv);
int polycall_cmd_status(const polycall_invocation_t *inv);
int polycall_cmd_stop(const polycall_invocation_t *inv);
int polycall_cmd_call(const polycall_invocation_t *inv);

/* telemetry emit/show/status (src/cli/cmd_telemetry.c) */
int polycall_cmd_telemetry_emit(const polycall_invocation_t *inv);
int polycall_cmd_telemetry_show(const polycall_invocation_t *inv);
int polycall_cmd_telemetry_status(const polycall_invocation_t *inv);

/* generic exit-4 handler for staged-but-unavailable commands */
int polycall_cmd_unsupported(const polycall_invocation_t *inv);

/* legacy config bridge, shared with the native path (src/cli/cli_commands.c) */
int polycall_config_run_legacy(const polycall_invocation_t *inv, const char *sub);
/* v2 migration writer (src/cli/cmd_config2.c) */
int polycall_config_migrate_to_v2(const polycall_invocation_t *inv,
                                  const char *language, const char *output);

#endif /* POLYCALL_CLI_INTERNAL_H */
