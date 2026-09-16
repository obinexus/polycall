/*
 * Command registry, help rendering, and the built-in handlers that do not
 * need the runtime: help, version, doctor. Runtime-touching commands
 * (run/status/stop/call) and the binding inventory are registered here too
 * so the parser has a closed command set; until their stage lands they
 * return a definite unsupported-capability error (exit 4), never a success
 * stub.
 */

#include "cli_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "polycall.h"

/* ---- build-time facts (overridable by -D from the build system) --------- */

#ifndef POLYCALL_BUILD_SYSTEM
#define POLYCALL_BUILD_SYSTEM "unknown"
#endif
#ifndef POLYCALL_BUILD_TYPE
#define POLYCALL_BUILD_TYPE "unspecified"
#endif
#ifndef POLYCALL_BUILD_CC
#define POLYCALL_BUILD_CC "unknown"
#endif
#ifndef POLYCALL_BUILD_TARGET
#define POLYCALL_BUILD_TARGET "unknown"
#endif
#ifndef POLYCALL_BUILD_CFLAGS
#define POLYCALL_BUILD_CFLAGS ""
#endif

#if defined(__VERSION__)
#define POLYCALL_CC_VERSION __VERSION__
#else
#define POLYCALL_CC_VERSION "unknown"
#endif

static const char *platform_name(void)
{
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

static const char *linkage_name(void)
{
#if defined(POLYCALL_USE_SHARED)
    return "shared";
#else
    return "static";
#endif
}

static long stdc_version(void)
{
#if defined(__STDC_VERSION__)
    return (long)__STDC_VERSION__;
#else
    return 0;
#endif
}

/* ====================================================================== */
/* help / version                                                          */
/* ====================================================================== */

int polycall_cmd_help(const polycall_invocation_t *inv);   /* fwd */

static int cmd_version(const polycall_invocation_t *inv)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char body[256], qv[64], qa[64];
        snprintf(body, sizeof body,
                 "{\"version\":%s,\"abi\":%s,\"cli_schema\":%d}",
                 polycall_json_quote(qv, sizeof qv, polycall_get_version()),
                 polycall_json_quote(qa, sizeof qa, POLYCALL_ABI_VERSION_STRING),
                 POLYCALL_CLI_SCHEMA_VERSION);
        polycall_json_result(inv->out, "version", true, body, NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "polycall %s\n", polycall_get_version());
    }
    return POLYCALL_EXIT_OK;
}

/* ====================================================================== */
/* doctor                                                                  */
/* ====================================================================== */

static int cmd_doctor(const polycall_invocation_t *inv)
{
    int failures = 0;
    polycall_context_t ctx = NULL;
    polycall_config_t cfg;
    bool ctx_ok;

    /* A bare context is malloc/free only: no service, no config evaluation. */
    memset(&cfg, 0, sizeof cfg);
    cfg.memory_pool_size = 64 * 1024;
    ctx_ok = (polycall_init_with_config(&ctx, &cfg) == POLYCALL_SUCCESS);
    if (ctx_ok) {
        polycall_cleanup(ctx);
    } else {
        failures++;
    }

    if (inv->g->format == POLYCALL_FMT_JSON) {
        char qplat[64], qlink[32], qver[64], qabi[64];
        char qsys[64], qcc[128], qccver[512], qtarget[128], qflags[512];
        char body[2560];
        snprintf(body, sizeof body,
            "{"
            "\"platform\":%s,"
            "\"pointer_bits\":%d,"
            "\"stdc_version\":%ld,"
            "\"linkage\":%s,"
            "\"library_version\":%s,"
            "\"abi_version\":%s,"
            "\"cli_schema\":%d,"
            "\"build\":{\"system\":%s,\"type\":\"%s\",\"cc\":%s,\"cc_version\":%s,"
                       "\"target\":%s,\"cflags\":%s},"
            "\"checks\":{\"core_context_construct\":%s},"
            "\"adapters\":[]"
            "}",
            polycall_json_quote(qplat, sizeof qplat, platform_name()),
            (int)(sizeof(void *) * 8),
            stdc_version(),
            polycall_json_quote(qlink, sizeof qlink, linkage_name()),
            polycall_json_quote(qver, sizeof qver, polycall_get_version()),
            polycall_json_quote(qabi, sizeof qabi, POLYCALL_ABI_VERSION_STRING),
            POLYCALL_CLI_SCHEMA_VERSION,
            polycall_json_quote(qsys, sizeof qsys, POLYCALL_BUILD_SYSTEM),
            POLYCALL_BUILD_TYPE,
            polycall_json_quote(qcc, sizeof qcc, POLYCALL_BUILD_CC),
            polycall_json_quote(qccver, sizeof qccver, POLYCALL_CC_VERSION),
            polycall_json_quote(qtarget, sizeof qtarget, POLYCALL_BUILD_TARGET),
            polycall_json_quote(qflags, sizeof qflags, POLYCALL_BUILD_CFLAGS),
            ctx_ok ? "true" : "false");
        polycall_json_result(inv->out, "doctor", failures == 0, body,
                             failures ? "doctor_failed" : NULL,
                             failures ? "one or more core checks failed" : NULL,
                             failures ? "see the failing check and rebuild the core library" : NULL);
        return failures ? POLYCALL_EXIT_RUNTIME : POLYCALL_EXIT_OK;
    }

    fprintf(inv->out, "polycall doctor\n");
    fprintf(inv->out, "  platform          : %s\n", platform_name());
    fprintf(inv->out, "  pointer width     : %d-bit\n", (int)(sizeof(void *) * 8));
    fprintf(inv->out, "  C standard        : %ld\n", stdc_version());
    fprintf(inv->out, "  linkage           : %s\n", linkage_name());
    fprintf(inv->out, "  library version   : %s\n", polycall_get_version());
    fprintf(inv->out, "  ABI version       : %s\n", POLYCALL_ABI_VERSION_STRING);
    fprintf(inv->out, "  CLI schema        : %d\n", POLYCALL_CLI_SCHEMA_VERSION);
    fprintf(inv->out, "  build system      : %s\n", POLYCALL_BUILD_SYSTEM);
    fprintf(inv->out, "  build type        : %s\n", POLYCALL_BUILD_TYPE);
    fprintf(inv->out, "  compiler          : %s (%s)\n", POLYCALL_BUILD_CC, POLYCALL_CC_VERSION);
    fprintf(inv->out, "  compiler target   : %s\n", POLYCALL_BUILD_TARGET);
    if (POLYCALL_BUILD_CFLAGS[0]) {
        fprintf(inv->out, "  compile flags     : %s\n", POLYCALL_BUILD_CFLAGS);
    }
    fprintf(inv->out, "  core context      : %s\n",
            ctx_ok ? "constructs OK" : "FAILED to construct");
    fprintf(inv->out, "  adapters          : none reported by this build\n");

    if (failures) {
        fprintf(inv->err,
                "doctor: %d core check(s) failed; the library is not usable as built\n",
                failures);
        return POLYCALL_EXIT_RUNTIME;
    }
    fprintf(inv->out, "\nNo problems detected.\n");
    return POLYCALL_EXIT_OK;
}

/* ====================================================================== */
/* legacy config bridge: delegate to src/polycall_config.c unchanged       */
/* (shared with the native path in src/cli/cmd_config2.c)                   */
/* ====================================================================== */

int polycall_config_run_legacy(const polycall_invocation_t *inv, const char *sub)
{
    /* Rebuild an argv the legacy entry understands:
     *   polycall config <sub> [positionals...]
     * The legacy function re-derives everything from argv[1] == "config". */
    const char *av[4 + POLYCALL_CLI_MAX_POSITIONALS];
    int n = 0, k;
    int rc;

    av[n++] = "polycall";
    av[n++] = "config";
    av[n++] = sub;
    for (k = 0; k < inv->argc && n < (int)(sizeof av / sizeof av[0]); ++k) {
        av[n++] = inv->argv[k];
    }

    rc = polycall_config_cli(n, (char **)av);
    if (rc < 0) {
        rc = POLYCALL_EXIT_USAGE;
    }
    /* legacy returns 0 ok / 1 problem; map 1 -> config error (3) */
    if (rc == 1) {
        rc = POLYCALL_EXIT_CONFIG;
    }
    if (inv->g->format == POLYCALL_FMT_JSON) {
        polycall_json_result(inv->out, "config", rc == 0, "null",
                             rc == 0 ? NULL : "config",
                             rc == 0 ? NULL : "configuration command failed; see stderr",
                             rc == 0 ? NULL : "run 'polycall config validate' for detail");
    }
    return rc;
}

static int cfg_load(const polycall_invocation_t *i)
{
    return polycall_config_run_legacy(i, "load");
}

/* ====================================================================== */
/* generic "recognised but unavailable in this build" handler (exit 4).    */
/* Kept for staged commands that land later; wired where needed.           */
/* ====================================================================== */

int polycall_cmd_unsupported(const polycall_invocation_t *inv)
{
    const char *what = inv->command;
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "'%s' is recognised but not available in this build", what);
        polycall_json_result(inv->out, what, false, NULL, "unsupported", msg,
                             "this capability lands in a later delivery stage");
    } else {
        fprintf(inv->err,
                "polycall: '%s' is recognised but not available in this build\n",
                what);
    }
    return POLYCALL_EXIT_UNSUPPORTED;
}

/* ====================================================================== */
/* registry                                                               */
/* ====================================================================== */

static const polycall_subcommand_t CONFIG_SUBS[] = {
    { "validate", "validate a native provider / envelope, or a legacy file",
      "config validate [--provider c:LIB|node:MOD|python:MOD | --envelope F | path]",
      polycall_cmd_config_validate },
    { "show", "print the effective configuration (native selector or legacy)",
      "config show [--provider ... | --envelope F | language] [--provenance]",
      polycall_cmd_config_show },
    { "load", "load the legacy configuration hierarchy and report the order",
      "config load [language]", cfg_load },
    { "migrate", "legacy file -> language-native, or --from-legacy -> v2 envelope",
      "config migrate <src> <Polycallrc.lang> | --from-legacy [--language L] --output F",
      polycall_cmd_config_migrate },
};

static const polycall_subcommand_t TELEMETRY_SUBS[] = {
    { "emit", "append one GUID + timestamp correlated event to the sink",
      "telemetry emit --event NAME [--service S] [--operation OP]\n"
      "                [--status ST] [--detail TEXT] [--correlation-id GUID]",
      polycall_cmd_telemetry_emit },
    { "show", "print the most recent recorded events (newest last)",
      "telemetry show [--limit N]", polycall_cmd_telemetry_show },
    { "status", "report whether telemetry is enabled and where it writes",
      "telemetry status", polycall_cmd_telemetry_status },
};

static const polycall_command_t COMMANDS[] = {
    /* name, summary, usage, help_body, run, subs, sub_count,
       starts_runtime, takes_local_opts */
    { "help", "show help for polycall or a specific command",
      "help [command [subcommand]]", NULL, polycall_cmd_help, NULL, 0,
      false, true },

    { "version", "print the version and exit",
      "version", NULL, cmd_version, NULL, 0, false, false },

    { "doctor", "report build, platform, ABI and core-capability status",
      "doctor [--format json]",
      "doctor performs read-only checks. It never starts a service, opens a\n"
      "socket, or evaluates a configuration provider.\n",
      cmd_doctor, NULL, 0, false, false },

    { "config", "inspect, validate and migrate configuration",
      "config <subcommand> [args]",
      "validate/show accept a native selector (--provider c:LIB | node:MOD |\n"
      "python:MOD, or --envelope FILE) or, with no selector, fall back to the\n"
      "legacy Polycallfile / Polycallrc loader. Selecting a provider executes\n"
      "the explicitly named module only. None of these start the runtime.\n",
      NULL, CONFIG_SUBS, (int)(sizeof CONFIG_SUBS / sizeof CONFIG_SUBS[0]),
      false, true },

    { "telemetry", "record and inspect GUID + timestamp correlated events",
      "telemetry <subcommand> [args]",
      "Every event carries a v4 GUID correlation_id and a millisecond UTC\n"
      "timestamp; 'run' and 'call' emit their own lifecycle events under the\n"
      "same sink automatically. Disable with POLYCALL_TELEMETRY=off; override\n"
      "the sink file with POLYCALL_TELEMETRY_LOG (default:\n"
      "<project-root>/.polycall/telemetry.jsonl). None of these start the\n"
      "runtime.\n",
      NULL, TELEMETRY_SUBS, (int)(sizeof TELEMETRY_SUBS / sizeof TELEMETRY_SUBS[0]),
      false, true },

    { "repl", "start an explicit interactive session (same command set)",
      "repl", NULL, polycall_repl_run, NULL, 0, false, false },

    { "start", "start the runtime in the foreground (polycall_rpc v1)",
      "start [--endpoint host:port] [--auth-token TOK] [--endpoint-file PATH]\n"
      "      [--load PATH ...] [--daemon]",
      "start serves the built-in deterministic operations until Ctrl-C or an\n"
      "authenticated 'stop'. It binds loopback by default; port 0 = ephemeral.\n"
      "Cleanup runs in normal execution, not in the signal handler.\n"
      "\n"
      "--load PATH loads one operation plugin (a shared library exporting\n"
      "polycall_ops_register); repeat --load for more than one. Plugins are\n"
      "loaded, and any ABI mismatch or missing symbol fails start before it\n"
      "binds anything (exit 4). See docs/PLUGINS.md.\n"
      "\n"
      "--daemon detaches into the background and returns immediately, printing\n"
      "the detached process's PID; use --endpoint-file to learn the bound\n"
      "address asynchronously, the same way a foreground caller would poll it.\n",
      polycall_cmd_start, NULL, 0, true, true },

    { "status", "query a running instance over its control channel",
      "status --endpoint host:port", NULL, polycall_cmd_status, NULL, 0,
      true, true },

    { "stop", "ask a running instance to shut down (authenticated)",
      "stop --endpoint host:port [--auth-token TOK]", NULL, polycall_cmd_stop,
      NULL, 0, true, true },

    { "call", "invoke a registered typed operation through the runtime",
      "call SERVICE OPERATION --endpoint host:port [--input FILE|- | --input-value JSON]",
      "call performs exactly one round trip and never retries. Distinct exit\n"
      "codes: 5 transport, 6 deadline, 7 auth, 4 unknown operation/item.\n",
      polycall_cmd_call, NULL, 0, true, true },
};

const polycall_command_t *polycall_cli_registry(int *count)
{
    if (count) {
        *count = (int)(sizeof COMMANDS / sizeof COMMANDS[0]);
    }
    return COMMANDS;
}

const polycall_command_t *polycall_cli_find(const char *name)
{
    size_t i;
    if (!name) {
        return NULL;
    }
    for (i = 0; i < sizeof COMMANDS / sizeof COMMANDS[0]; ++i) {
        if (strcmp(COMMANDS[i].name, name) == 0) {
            return &COMMANDS[i];
        }
    }
    return NULL;
}

const polycall_subcommand_t *polycall_cli_find_sub(const polycall_command_t *c,
                                                   const char *name)
{
    int i;
    if (!c || !name) {
        return NULL;
    }
    for (i = 0; i < c->sub_count; ++i) {
        if (strcmp(c->subs[i].name, name) == 0) {
            return &c->subs[i];
        }
    }
    return NULL;
}

/* ====================================================================== */
/* help rendering                                                          */
/* ====================================================================== */

void polycall_cli_print_root_help(FILE *out)
{
    int n = 0, i;
    const polycall_command_t *reg = polycall_cli_registry(&n);

    fprintf(out,
        "polycall %s - program-first cross-language runtime\n\n"
        "usage: polycall [global-options] <command> [subcommand] [arguments]\n\n"
        "commands:\n",
        polycall_get_version());

    for (i = 0; i < n; ++i) {
        fprintf(out, "  %-10s %s\n", reg[i].name, reg[i].summary);
    }

    fprintf(out,
        "\nglobal options:\n"
        "  --project-root PATH   project root to resolve relative paths against\n"
        "  --config PATH         explicit configuration file / provider\n"
        "  --language LANG       language layer / adapter to select\n"
        "  --format text|json    output format for finite commands (default text)\n"
        "  --no-color            disable ANSI colour\n"
        "  --quiet               suppress non-essential text on stderr\n"
        "  --timeout-ms N        deadline for operations that support one\n"
        "  -h, --help            show help and exit\n"
        "  -V, --version         show version and exit\n"
        "  --                    end option parsing\n\n"
        "Run 'polycall help <command>' for details on a command.\n");
}

void polycall_cli_print_command_help(FILE *out, const polycall_command_t *c)
{
    fprintf(out, "polycall %s - %s\n\n", c->name, c->summary);
    fprintf(out, "usage: polycall %s\n", c->usage ? c->usage : c->name);

    if (c->sub_count > 0) {
        int i;
        fprintf(out, "\nsubcommands:\n");
        for (i = 0; i < c->sub_count; ++i) {
            fprintf(out, "  %-10s %s\n", c->subs[i].name, c->subs[i].summary);
            if (c->subs[i].usage) {
                fprintf(out, "  %-10s usage: polycall %s\n", "", c->subs[i].usage);
            }
        }
    }
    if (c->help_body && c->help_body[0]) {
        fprintf(out, "\n%s", c->help_body);
    }
    if (c->starts_runtime) {
        fprintf(out, "\nNote: this command starts or contacts the runtime.\n");
    }
}

int polycall_cmd_help(const polycall_invocation_t *inv)
{
    const polycall_command_t *c;

    if (inv->argc == 0) {
        polycall_cli_print_root_help(inv->out);
        return POLYCALL_EXIT_OK;
    }

    c = polycall_cli_find(inv->argv[0]);
    if (c == NULL) {
        fprintf(inv->err, "polycall: no such command: %s\n", inv->argv[0]);
        fprintf(inv->err, "Try 'polycall help' for the command list.\n");
        return POLYCALL_EXIT_USAGE;
    }

    if (inv->argc >= 2 && c->sub_count > 0) {
        const polycall_subcommand_t *s = polycall_cli_find_sub(c, inv->argv[1]);
        if (s == NULL) {
            fprintf(inv->err, "polycall: %s has no subcommand '%s'\n",
                    c->name, inv->argv[1]);
            return POLYCALL_EXIT_USAGE;
        }
        fprintf(inv->out, "polycall %s %s - %s\n\n", c->name, s->name, s->summary);
        fprintf(inv->out, "usage: polycall %s\n", s->usage ? s->usage : s->name);
        return POLYCALL_EXIT_OK;
    }

    polycall_cli_print_command_help(inv->out, c);
    return POLYCALL_EXIT_OK;
}
