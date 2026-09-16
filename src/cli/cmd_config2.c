/*
 * Native (schema v2) config subcommands.
 *
 *   config validate  --provider <c:LIB | node:MODULE | python:MODULE>
 *   config validate  --envelope <file>
 *   config validate  [path]                  (no selector -> legacy loader)
 *   config show      <same selectors> [--provenance]
 *   config migrate   <src> <Polycallrc.lang>            (legacy form, bridge)
 *   config migrate   --from-legacy [--language L] --output <file>  (v2 form)
 *
 * Selecting a provider executes local code. Only the explicitly named module /
 * library is used; parent directories are never scanned. `doctor` never runs
 * a provider.
 */

#include "cli_internal.h"

#include <stdlib.h>
#include <string.h>

#include "polycall_config2.h"
#include "polycall_provider.h"

/* ---- tiny local-option parser ------------------------------------------ */

typedef struct {
    const char *provider;   /* "c:...", "node:...", "python:..."  */
    const char *envelope;   /* --envelope PATH                    */
    const char *output;     /* --output PATH                      */
    long deadline_ms;       /* --deadline-ms N (-1 unset)         */
    long max_bytes;         /* --max-bytes N (-1 unset)           */
    bool provenance;        /* --provenance                       */
    bool from_legacy;       /* --from-legacy                      */
    const char *pos[8];
    int npos;
} cargs_t;

static int parse_cargs(const polycall_invocation_t *inv, cargs_t *a,
                       const char **bad)
{
    int i;
    memset(a, 0, sizeof *a);
    a->deadline_ms = -1;
    a->max_bytes = -1;
    for (i = 0; i < inv->argc; ++i) {
        const char *t = inv->argv[i];
        const char *eq = strchr(t, '=');
        const char *val = NULL;
        char name[32];
        bool literal = (inv->literal_from >= 0 && i >= inv->literal_from);

        if (literal || t[0] != '-') {
            if (a->npos < (int)(sizeof a->pos / sizeof a->pos[0])) {
                a->pos[a->npos++] = t;
            }
            continue;
        }
        if (eq) {
            size_t n = (size_t)(eq - t);
            if (n >= sizeof name) n = sizeof name - 1;
            memcpy(name, t, n);
            name[n] = '\0';
            val = eq + 1;
        } else {
            snprintf(name, sizeof name, "%s", t);
        }
        #define NEED_VAL(opt) do { \
            if (!val) { if (i + 1 >= inv->argc) { *bad = opt; return -1; } \
                        val = inv->argv[++i]; } } while (0)

        if (!strcmp(name, "--provider"))      { NEED_VAL("--provider");   a->provider = val; }
        else if (!strcmp(name, "--envelope")) { NEED_VAL("--envelope");   a->envelope = val; }
        else if (!strcmp(name, "--output"))   { NEED_VAL("--output");     a->output = val; }
        else if (!strcmp(name, "--deadline-ms")) { NEED_VAL("--deadline-ms"); a->deadline_ms = strtol(val, NULL, 10); }
        else if (!strcmp(name, "--max-bytes"))   { NEED_VAL("--max-bytes");   a->max_bytes = strtol(val, NULL, 10); }
        else if (!strcmp(name, "--provenance"))  { a->provenance = true; }
        else if (!strcmp(name, "--from-legacy")) { a->from_legacy = true; }
        else { *bad = t; return -1; }
        #undef NEED_VAL
    }
    return 0;
}

static int usage(const polycall_invocation_t *inv, const char *msg, const char *bad)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        char m[256];
        snprintf(m, sizeof m, "%s%s%s", msg, bad ? ": " : "", bad ? bad : "");
        polycall_json_result(inv->out, inv->command, false, NULL, "usage", m,
                             "see 'polycall help config'");
    } else {
        fprintf(inv->err, "polycall config: %s%s%s\n", msg,
                bad ? ": " : "", bad ? bad : "");
    }
    return POLYCALL_EXIT_USAGE;
}

static int cfg_err(const polycall_invocation_t *inv,
                   const polycall_config2_error_t *e)
{
    if (inv->g->format == POLYCALL_FMT_JSON) {
        polycall_json_result(inv->out, inv->command, false, NULL,
                             e->code[0] ? e->code : "config",
                             e->message[0] ? e->message : "configuration invalid",
                             e->field[0] ? e->field : NULL);
    } else {
        fprintf(inv->err, "polycall config: %s", e->message[0] ? e->message : "invalid");
        if (e->field[0]) fprintf(inv->err, "  [%s]", e->field);
        if (e->code[0]) fprintf(inv->err, "  (%s)", e->code);
        fputc('\n', inv->err);
    }
    return POLYCALL_EXIT_CONFIG;
}

/* ---- provider resolution -------------------------------------------- */

static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0 || n > (long)POLYCALL_CFG2_ENVELOPE_MAX) { fclose(f); return NULL; }
    buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, (size_t)n, f);
    buf[*len] = '\0';
    fclose(f);
    return buf;
}

/* Load a config through the selected provider. Returns a config (caller frees)
 * or NULL with *e set. */
static polycall_config2_t *load_selected(const polycall_invocation_t *inv,
                                         const cargs_t *a,
                                         polycall_config2_error_t *e)
{
    polycall_provider_opts_t o;
    polycall_provider_opts_init(&o);
    o.project_root = inv->g->project_root;
    o.language = inv->g->language;
    if (a->deadline_ms >= 0) o.deadline_ms = (uint32_t)a->deadline_ms;
    if (a->max_bytes >= 0) o.max_output_bytes = (uint32_t)a->max_bytes;

    if (a->envelope) {
        size_t len = 0;
        char *txt = read_file(a->envelope, &len);
        polycall_config2_t *cfg;
        if (!txt) {
            polycall_config2_error_init(e);
            snprintf(e->code, sizeof e->code, "envelope.read");
            snprintf(e->message, sizeof e->message,
                     "cannot read envelope file: %s", a->envelope);
            return NULL;
        }
        cfg = polycall_config2_from_envelope(txt, len, e);
        free(txt);
        return cfg;
    }

    if (a->provider) {
        const char *colon = strchr(a->provider, ':');
        char kind[16];
        const char *arg;
        size_t kn;
        if (!colon) {
            polycall_config2_error_init(e);
            snprintf(e->code, sizeof e->code, "provider.spec");
            snprintf(e->message, sizeof e->message,
                     "--provider must be c:PATH, node:MODULE or python:MODULE");
            return NULL;
        }
        kn = (size_t)(colon - a->provider);
        if (kn >= sizeof kind) kn = sizeof kind - 1;
        memcpy(kind, a->provider, kn);
        kind[kn] = '\0';
        arg = colon + 1;

        if (!strcmp(kind, "c")) {
            return polycall_provider_load_c(arg, &o, e);
        }
        if (!strcmp(kind, "node") || !strcmp(kind, "python")) {
            const char *env_name = !strcmp(kind, "node")
                                   ? "POLYCALL_NODE_PROVIDER"
                                   : "POLYCALL_PYTHON_PROVIDER";
            const char *runner = getenv(env_name);
            const char *prog;
            const char *argv[4];
            if (!runner || !*runner) {
                polycall_config2_error_init(e);
                snprintf(e->code, sizeof e->code, "provider.runner_unset");
                snprintf(e->message, sizeof e->message,
                         "no %s runner script configured", kind);
                snprintf(e->field, sizeof e->field, "%s", env_name);
                return NULL;
            }
            if (!strcmp(kind, "node")) {
                prog = "node";
            } else {
                /* $POLYCALL_PYTHON wins; else python3 on POSIX, python on Windows */
                const char *pe = getenv("POLYCALL_PYTHON");
#if defined(_WIN32)
                prog = (pe && *pe) ? pe : "python";
#else
                prog = (pe && *pe) ? pe : "python3";
#endif
            }
            argv[0] = prog;
            argv[1] = runner;
            argv[2] = arg;
            argv[3] = NULL;
            return polycall_provider_run_subprocess(argv, &o, e);
        }
        polycall_config2_error_init(e);
        snprintf(e->code, sizeof e->code, "provider.kind");
        snprintf(e->message, sizeof e->message,
                 "unknown provider kind '%s' (use c / node / python)", kind);
        return NULL;
    }

    polycall_config2_error_init(e);
    snprintf(e->code, sizeof e->code, "provider.none");
    snprintf(e->message, sizeof e->message, "no provider or envelope selected");
    return NULL;
}

static bool has_selector(const cargs_t *a)
{
    return a->provider != NULL || a->envelope != NULL || a->from_legacy;
}

/* Serialise a config to a heap buffer (never a ~1 MiB stack array -- that
 * overflows MSVC's 1 MB default stack). Caller frees. NULL on OOM. */
static char *envelope_dup(const polycall_config2_t *cfg)
{
    int need = polycall_config2_to_envelope(cfg, NULL, 0);
    char *buf;
    if (need <= 0) {
        return NULL;
    }
    buf = malloc((size_t)need + 1);
    if (!buf) {
        return NULL;
    }
    polycall_config2_to_envelope(cfg, buf, (size_t)need + 1);
    return buf;
}

/* ---- config validate --------------------------------------------------- */

int polycall_cmd_config_validate(const polycall_invocation_t *inv)
{
    cargs_t a;
    const char *bad = NULL;
    polycall_config2_error_t e;
    polycall_config2_t *cfg;

    if (parse_cargs(inv, &a, &bad) != 0) {
        return usage(inv, "bad option", bad);
    }
    if (!has_selector(&a)) {
        /* legacy path: bare `config validate [path]` -- unchanged behaviour */
        return polycall_config_run_legacy(inv, "validate");
    }

    polycall_config2_error_init(&e);
    cfg = load_selected(inv, &a, &e);
    if (!cfg) {
        return cfg_err(inv, &e);
    }

    if (inv->g->format == POLYCALL_FMT_JSON) {
        char *buf = envelope_dup(cfg);
        polycall_json_result(inv->out, "config", true, buf ? buf : "null",
                             NULL, NULL, NULL);
        free(buf);
    } else {
        int i, sc = polycall_config2_service_count(cfg);
        const char *ns = polycall_config2_extension_namespace(cfg);
        fprintf(inv->out,
                "configuration is valid: project '%s'%s%s%s, %d service(s)\n",
                polycall_config2_project_name(cfg),
                (ns && ns[0]) ? ", namespace '" : "",
                (ns && ns[0]) ? ns : "",
                (ns && ns[0]) ? "'" : "",
                sc);
        for (i = 0; i < sc; ++i) {
            polycall_service_view_t v;
            v.struct_size = (uint32_t)sizeof v;
            if (polycall_config2_service_at(cfg, i, &v) == 0) {
                fprintf(inv->out, "  - %-12s %-6s %s:%u -> %u  ws=%s tls=%s\n",
                        v.id, v.language, v.bind_address, v.host_port,
                        v.target_port, v.workspace,
                        v.tls_mode == POLYCALL_TLS_OFF ? "off" :
                        v.tls_mode == POLYCALL_TLS_SERVER ? "server" : "mutual");
            }
        }
    }
    polycall_config2_free(cfg);
    return POLYCALL_EXIT_OK;
}

/* ---- config show ----------------------------------------------------- */

static const char *src_label(polycall_cfg_source_t s)
{
    switch (s) {
    case POLYCALL_SRC_NATIVE_GLOBAL:  return "native_global";
    case POLYCALL_SRC_NATIVE_PROJECT: return "native_project";
    case POLYCALL_SRC_LANG_OVERRIDE:  return "lang_override";
    case POLYCALL_SRC_ENV:            return "env";
    case POLYCALL_SRC_CLI:            return "cli";
    case POLYCALL_SRC_LEGACY:         return "legacy";
    default:                          return "default";
    }
}

int polycall_cmd_config_show(const polycall_invocation_t *inv)
{
    cargs_t a;
    const char *bad = NULL;
    polycall_config2_error_t e;
    polycall_config2_t *cfg;
    char *buf;

    if (parse_cargs(inv, &a, &bad) != 0) {
        return usage(inv, "bad option", bad);
    }
    if (!has_selector(&a)) {
        return polycall_config_run_legacy(inv, "show");
    }
    polycall_config2_error_init(&e);
    cfg = load_selected(inv, &a, &e);
    if (!cfg) {
        return cfg_err(inv, &e);
    }
    buf = envelope_dup(cfg);

    if (inv->g->format == POLYCALL_FMT_JSON) {
        polycall_json_result(inv->out, "config", true, buf ? buf : "null",
                             NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "%s\n", buf ? buf : "");
        if (a.provenance) {
            int i, sc = polycall_config2_service_count(cfg);
            fprintf(inv->out, "\nprovenance:\n");
            fprintf(inv->out, "  project.name             %s\n",
                    src_label(polycall_config2_provenance(cfg, "project.name")));
            for (i = 0; i < sc; ++i) {
                char f[64];
                polycall_service_view_t v;
                v.struct_size = (uint32_t)sizeof v;
                polycall_config2_service_at(cfg, i, &v);
                snprintf(f, sizeof f, "services[%d].workspace", i);
                fprintf(inv->out, "  %-24s %s\n", f,
                        src_label(polycall_config2_provenance(cfg, f)));
                snprintf(f, sizeof f, "services[%d].host_port", i);
                fprintf(inv->out, "  %-24s %s\n", f,
                        src_label(polycall_config2_provenance(cfg, f)));
            }
            fprintf(inv->out, "  (secret values are never printed; only env:/file: references)\n");
        }
    }
    free(buf);
    polycall_config2_free(cfg);
    return POLYCALL_EXIT_OK;
}

/* ---- config migrate ------------------------------------------------- */

int polycall_cmd_config_migrate(const polycall_invocation_t *inv)
{
    cargs_t a;
    const char *bad = NULL;

    if (parse_cargs(inv, &a, &bad) != 0) {
        return usage(inv, "bad option", bad);
    }

    if (!a.from_legacy) {
        /* legacy copy-migrate form: `config migrate <src> <Polycallrc.lang>` */
        return polycall_config_run_legacy(inv, "migrate");
    }

    /* v2 form: compute the legacy effective model, emit a native envelope. */
    return polycall_config_migrate_to_v2(inv, inv->g->language, a.output);
}

int polycall_config_migrate_to_v2(const polycall_invocation_t *inv,
                                  const char *language, const char *output)
{
    polycall_config2_error_t e;
    polycall_config2_t *cfg;
    char unresolved[4096];
    char *buf;
    int n;
    FILE *out;

    if (!output || !*output) {
        return usage(inv, "--from-legacy requires --output <file>", NULL);
    }
    {
        FILE *probe = fopen(output, "rb");
        if (probe) {
            fclose(probe);
            if (inv->g->format == POLYCALL_FMT_JSON) {
                polycall_json_result(inv->out, "config", false, NULL,
                                     "output.exists",
                                     "refusing to overwrite the output file", output);
            } else {
                fprintf(inv->err, "polycall config: refusing to overwrite %s\n",
                        output);
            }
            return POLYCALL_EXIT_CONFIG;
        }
    }

    polycall_config2_error_init(&e);
    cfg = polycall_config2_import_legacy(
        inv->g->project_root, language, unresolved, sizeof unresolved, &e);
    if (!cfg) {
        return cfg_err(inv, &e);
    }

    buf = envelope_dup(cfg);
    n = buf ? (int)strlen(buf) : -1;
    polycall_config2_free(cfg);
    if (n <= 0) {
        return usage(inv, "internal: envelope serialisation failed", NULL);
    }

    out = fopen(output, "wb");
    if (!out) {
        free(buf);
        if (inv->g->format == POLYCALL_FMT_JSON) {
            polycall_json_result(inv->out, "config", false, NULL, "output.write",
                                 "cannot create the output file", output);
        } else {
            fprintf(inv->err, "polycall config: cannot create %s\n", output);
        }
        return POLYCALL_EXIT_CONFIG;
    }
    fwrite(buf, 1, (size_t)n, out);
    fputc('\n', out);
    fclose(out);
    free(buf);

    if (inv->g->format == POLYCALL_FMT_JSON) {
        char qo[512], qu[4096], body[8192];
        char *w = qu; const char *r = unresolved;
        *w++ = '[';
        /* turn the "file:line key\n" report into a JSON array of strings */
        while (*r) {
            const char *nl = strchr(r, '\n');
            size_t len = nl ? (size_t)(nl - r) : strlen(r);
            if (w != qu + 1) *w++ = ',';
            *w++ = '"';
            for (; len && (size_t)(w - qu) < sizeof qu - 8; --len, ++r) {
                if (*r == '"' || *r == '\\') *w++ = '\\';
                *w++ = *r;
            }
            *w++ = '"';
            if (nl) r = nl + 1; else break;
        }
        *w++ = ']'; *w = '\0';
        polycall_json_quote(qo, sizeof qo, output);
        snprintf(body, sizeof body,
                 "{\"output\":%s,\"unresolved\":%s}", qo, qu);
        polycall_json_result(inv->out, "config", true, body, NULL, NULL, NULL);
    } else {
        fprintf(inv->out, "wrote native envelope: %s\n", output);
        if (unresolved[0]) {
            fprintf(inv->out,
                    "\nlegacy keys not represented in the v2 model "
                    "(values were NOT dropped from the source file):\n%s",
                    unresolved);
        } else {
            fprintf(inv->out, "all legacy keys mapped into the v2 model.\n");
        }
    }
    return POLYCALL_EXIT_OK;
}
