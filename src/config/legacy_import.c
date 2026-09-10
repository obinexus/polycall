/*
 * Legacy (v1) -> schema-v2 configuration import.
 *
 * Reads Polycallfile / Polycallrc / Polycallrc.<language> with the documented
 * v1 semantics (comments after '#', trimmed, later scalar settings override
 * earlier ones, server topology owned by Polycallfile) and projects the
 * *effective* values onto the typed v2 model. The original files are opened
 * read-only and never modified.
 *
 * This is a self-contained reader; it deliberately does not touch
 * src/polycall_config.c, which still backs the legacy `config validate/show`
 * paths unchanged.
 */

#include "polycall_config2.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KV_MAX     64
#define SRV_MAX    POLYCALL_CFG2_MAX_SERVICES
#define LINE_MAX   1024

typedef struct { char key[64], value[256]; } kv_t;
typedef struct { char lang[64]; unsigned hp, tp; } srv_t;

typedef struct {
    kv_t  kv[KV_MAX];
    int   nkv;
    srv_t srv[SRV_MAX];
    int   nsrv;
} legacy_t;

/* v2 knows these legacy keys; anything else is reported as unresolved. */
static const char *const KNOWN_KEYS[] = {
    "network_timeout", "max_connections", "workspace_root", "workspace",
    "log_directory", "log_level", "tls_enabled", "cert_file", "key_file",
    "timeout", "server_type", "port", "bind_address", "allow_remote",
    "require_auth", "auto_discover", "discovery_interval", "enable_metrics",
    "metrics_port", "max_memory", "max_memory_per_service", "max_cpu_per_service",
    "supports_diagnostics", "supports_completion", "supports_formatting"
};

/* keys that actually influence the v2 projection */
static const char *const MAPPED_KEYS[] = {
    "network_timeout", "max_connections", "workspace_root", "workspace",
    "tls_enabled", "cert_file", "key_file", "timeout", "bind_address"
};

static char *trim(char *s)
{
    char *e;
    while (*s && isspace((unsigned char)*s)) s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

static bool in_list(const char *k, const char *const *list, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) if (!strcmp(k, list[i])) return true;
    return false;
}

static void kv_set(legacy_t *L, const char *k, const char *v)
{
    int i;
    for (i = 0; i < L->nkv; ++i) {
        if (!strcmp(L->kv[i].key, k)) {           /* later scalar wins */
            snprintf(L->kv[i].value, sizeof L->kv[i].value, "%s", v);
            return;
        }
    }
    if (L->nkv < KV_MAX) {
        snprintf(L->kv[L->nkv].key, sizeof L->kv[L->nkv].key, "%s", k);
        snprintf(L->kv[L->nkv].value, sizeof L->kv[L->nkv].value, "%s", v);
        L->nkv++;
    }
}

static const char *kv_get(const legacy_t *L, const char *k)
{
    int i;
    for (i = 0; i < L->nkv; ++i) if (!strcmp(L->kv[i].key, k)) return L->kv[i].value;
    return NULL;
}

static int read_layer(legacy_t *L, const char *path, bool allow_servers,
                      char *unresolved, size_t ucap, size_t *ulen)
{
    FILE *f = fopen(path, "r");
    char raw[LINE_MAX];
    unsigned ln = 0;
    if (!f) {
        return -1;   /* absent layer: caller decides if that is fatal */
    }
    while (fgets(raw, sizeof raw, f)) {
        char *line, *hash, *eq;
        ln++;
        hash = strchr(raw, '#');
        if (hash) *hash = '\0';
        line = trim(raw);
        if (!*line) continue;

        if (allow_servers && !strncmp(line, "server", 6) &&
            isspace((unsigned char)line[6])) {
            char lang[64]; unsigned hp, tp;
            if (sscanf(line, "server %63s %u:%u", lang, &hp, &tp) == 3 &&
                L->nsrv < SRV_MAX) {
                snprintf(L->srv[L->nsrv].lang, sizeof L->srv[L->nsrv].lang, "%s", lang);
                L->srv[L->nsrv].hp = hp;
                L->srv[L->nsrv].tp = tp;
                L->nsrv++;
            }
            continue;
        }
        if (!strncmp(line, "network", 7) && isspace((unsigned char)line[7])) {
            continue;  /* 'network start/stop' is not an evaluation-time action */
        }
        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        {
            char *k = trim(line);
            char *v = trim(eq + 1);
            kv_set(L, k, v);
            if (!in_list(k, KNOWN_KEYS, sizeof KNOWN_KEYS / sizeof KNOWN_KEYS[0]) &&
                unresolved && ucap) {
                int add = snprintf(unresolved + *ulen, ucap - *ulen,
                                   "%s:%u %s\n", path, ln, k);
                if (add > 0 && (size_t)add < ucap - *ulen) *ulen += (size_t)add;
            } else if (in_list(k, KNOWN_KEYS, sizeof KNOWN_KEYS / sizeof KNOWN_KEYS[0]) &&
                       !in_list(k, MAPPED_KEYS, sizeof MAPPED_KEYS / sizeof MAPPED_KEYS[0]) &&
                       unresolved && ucap) {
                int add = snprintf(unresolved + *ulen, ucap - *ulen,
                                   "%s:%u %s (known, not mapped into v2)\n",
                                   path, ln, k);
                if (add > 0 && (size_t)add < ucap - *ulen) *ulen += (size_t)add;
            }
        }
    }
    fclose(f);
    return 0;
}

static unsigned to_uint(const char *s, unsigned dflt)
{
    char *end = NULL;
    unsigned long v;
    if (!s || !*s) return dflt;
    v = strtoul(s, &end, 10);
    if (end == s || *end != '\0' || v == 0 || v > 0xFFFFFFFFUL) return dflt;
    return (unsigned)v;
}

polycall_config2_t *polycall_config2_import_legacy(const char *project_root,
                                                  const char *language,
                                                  char *unresolved,
                                                  size_t unresolved_cap,
                                                  polycall_config2_error_t *err)
{
    legacy_t L;
    polycall_config2_builder_t *b;
    polycall_config2_t *cfg;
    char path[POLYCALL_CFG2_PATH_MAX];
    const char *root = (project_root && *project_root) ? project_root : ".";
    const char *ws_root, *cert, *key, *tls;
    size_t ulen = 0;
    int i;
    unsigned def_timeout;
    bool tls_on;

    memset(&L, 0, sizeof L);
    polycall_config2_error_init(err);
    if (unresolved && unresolved_cap) unresolved[0] = '\0';

    snprintf(path, sizeof path, "%s/Polycallfile", root);
    if (read_layer(&L, path, true, unresolved, unresolved_cap, &ulen) != 0) {
        if (err) {
            snprintf(err->code, sizeof err->code, "legacy.missing");
            snprintf(err->message, sizeof err->message,
                     "no Polycallfile under %s", root);
        }
        return NULL;
    }
    snprintf(path, sizeof path, "%s/Polycallrc", root);
    read_layer(&L, path, false, unresolved, unresolved_cap, &ulen);
    if (language && *language) {
        snprintf(path, sizeof path, "%s/Polycallrc.%s", root, language);
        read_layer(&L, path, false, unresolved, unresolved_cap, &ulen);
    }

    if (L.nsrv == 0) {
        if (err) {
            snprintf(err->code, sizeof err->code, "legacy.no_servers");
            snprintf(err->message, sizeof err->message,
                     "Polycallfile declares no 'server <lang> <h>:<t>' lines");
        }
        return NULL;
    }

    ws_root = kv_get(&L, "workspace_root");
    cert = kv_get(&L, "cert_file");
    key = kv_get(&L, "key_file");
    tls = kv_get(&L, "tls_enabled");
    tls_on = tls && !strcmp(tls, "true");
    def_timeout = to_uint(kv_get(&L, "network_timeout"),
                          to_uint(kv_get(&L, "timeout"), 30000));

    b = polycall_config2_builder_create();
    if (!b) {
        if (err) { snprintf(err->code, sizeof err->code, "oom");
                   snprintf(err->message, sizeof err->message, "out of memory"); }
        return NULL;
    }
    polycall_config2_builder_set_project(b, "legacy-import", NULL);

    for (i = 0; i < L.nsrv; ++i) {
        int idx = polycall_config2_builder_add_service(b, L.srv[i].lang,
                                                       L.srv[i].lang, err);
        char ws[POLYCALL_CFG2_PATH_MAX];
        unsigned mc;
        if (idx < 0) { polycall_config2_builder_free(b); return NULL; }
        if (polycall_config2_builder_service_set_ports(b, idx, L.srv[i].hp,
                                                       L.srv[i].tp, err) != 0) {
            polycall_config2_builder_free(b);
            return NULL;
        }
        if (ws_root && *ws_root)
            snprintf(ws, sizeof ws, "%s/%s", ws_root, L.srv[i].lang);
        else
            snprintf(ws, sizeof ws, "/%s", L.srv[i].lang);
        polycall_config2_builder_service_set_workspace(b, idx, ws, err);

        mc = to_uint(kv_get(&L, "max_connections"), 64);
        polycall_config2_builder_service_set_limits(b, idx, def_timeout, mc, err);

        if (kv_get(&L, "bind_address"))
            polycall_config2_builder_service_set_bind(b, idx,
                                                      kv_get(&L, "bind_address"), err);

        if (tls_on && cert && key) {
            char cref[POLYCALL_CFG2_PATH_MAX], kref[POLYCALL_CFG2_PATH_MAX];
            snprintf(cref, sizeof cref, "file:%s", cert);
            snprintf(kref, sizeof kref, "file:%s", key);
            polycall_config2_builder_service_set_tls(b, idx, POLYCALL_TLS_SERVER,
                                                     cref, kref, err);
        }
    }

    cfg = polycall_config2_builder_build(b, err);
    polycall_config2_builder_free(b);
    (void)ulen;
    return cfg;
}
