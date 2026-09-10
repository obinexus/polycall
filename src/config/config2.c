/*
 * PolyCall typed configuration model, schema v2 (see include/polycall_config2.h).
 *
 * Independent of the legacy polycall_config_t / polycall_config.c loader.
 * The core validates every value regardless of what a provider checked.
 */

#include "polycall_config2.h"
#include "json.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* internal structures                                                */
/* ------------------------------------------------------------------ */

typedef struct {
    char id[POLYCALL_CFG2_NAME_MAX];
    char language[POLYCALL_CFG2_NAME_MAX];
    char bind_address[POLYCALL_CFG2_ADDR_MAX];
    char workspace[POLYCALL_CFG2_PATH_MAX];
    uint32_t host_port;
    uint32_t target_port;
    uint32_t timeout_ms;
    uint32_t max_connections;
    polycall_tls_mode_t tls_mode;
    char tls_cert_ref[POLYCALL_CFG2_PATH_MAX];
    char tls_key_ref[POLYCALL_CFG2_PATH_MAX];
    bool has_ports;
} svc_t;

typedef struct {
    char field[128];
    polycall_cfg_source_t source;
} prov_t;

struct polycall_config2_builder {
    char base_dir[POLYCALL_CFG2_PATH_MAX];
    char project_name[POLYCALL_CFG2_NAME_MAX];
    char ext_ns[POLYCALL_CFG2_NS_MAX];
    svc_t services[POLYCALL_CFG2_MAX_SERVICES];
    int service_count;
    prov_t prov[64];
    int prov_count;
    bool failed;
};

struct polycall_config2 {
    char project_name[POLYCALL_CFG2_NAME_MAX];
    char ext_ns[POLYCALL_CFG2_NS_MAX];
    svc_t services[POLYCALL_CFG2_MAX_SERVICES];  /* sorted by id */
    int service_count;
    prov_t prov[64];
    int prov_count;
};

/* ------------------------------------------------------------------ */
/* error helpers                                                      */
/* ------------------------------------------------------------------ */

void polycall_config2_error_init(polycall_config2_error_t *err)
{
    if (err) {
        memset(err, 0, sizeof *err);
        err->struct_size = (uint32_t)sizeof *err;
    }
}

static int set_err(polycall_config2_error_t *err, const char *code,
                   const char *field, const char *fmt, ...)
{
    if (err) {
        va_list ap;
        if (err->struct_size == 0) {
            err->struct_size = (uint32_t)sizeof *err;
        }
        snprintf(err->code, sizeof err->code, "%s", code ? code : "error");
        snprintf(err->field, sizeof err->field, "%s", field ? field : "");
        va_start(ap, fmt);
        vsnprintf(err->message, sizeof err->message, fmt, ap);
        va_end(ap);
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* validation primitives                                              */
/* ------------------------------------------------------------------ */

static bool ident_ok(const char *s, size_t cap)
{
    size_t n = 0;
    if (!s || !*s) {
        return false;
    }
    for (; *s; ++s, ++n) {
        unsigned char c = (unsigned char)*s;
        if (!(isalnum(c) || c == '_' || c == '-' || c == '.')) {
            return false;
        }
    }
    return n < cap;
}

static bool port_ok(uint32_t p) { return p >= 1 && p <= 65535; }

static bool ref_ok(const char *s)
{
    /* a reference, never an inline secret */
    return s && (strncmp(s, "env:", 4) == 0 || strncmp(s, "file:", 5) == 0);
}

static bool is_abs_path(const char *p)
{
    if (!p || !*p) return false;
    if (p[0] == '/' || p[0] == '\\') return true;
    if (isalpha((unsigned char)p[0]) && p[1] == ':' &&
        (p[2] == '/' || p[2] == '\\')) return true;   /* C:\ */
    return false;
}

/* ------------------------------------------------------------------ */
/* provenance                                                         */
/* ------------------------------------------------------------------ */

static void prov_set(prov_t *arr, int *n, int cap, const char *field,
                     polycall_cfg_source_t src)
{
    int i;
    for (i = 0; i < *n; ++i) {
        if (strcmp(arr[i].field, field) == 0) {
            arr[i].source = src;
            return;
        }
    }
    if (*n < cap) {
        snprintf(arr[*n].field, sizeof arr[*n].field, "%s", field);
        arr[*n].source = src;
        (*n)++;
    }
}

/* ------------------------------------------------------------------ */
/* builder                                                            */
/* ------------------------------------------------------------------ */

polycall_config2_builder_t *polycall_config2_builder_create(void)
{
    return calloc(1, sizeof(polycall_config2_builder_t));
}

void polycall_config2_builder_free(polycall_config2_builder_t *b)
{
    free(b);
}

void polycall_config2_builder_set_base_dir(polycall_config2_builder_t *b,
                                           const char *dir)
{
    if (b && dir) {
        snprintf(b->base_dir, sizeof b->base_dir, "%s", dir);
    }
}

int polycall_config2_builder_set_project(polycall_config2_builder_t *b,
                                         const char *project_name,
                                         const char *extension_namespace)
{
    if (!b) {
        return -1;
    }
    if (project_name) {
        snprintf(b->project_name, sizeof b->project_name, "%s", project_name);
    }
    if (extension_namespace) {
        snprintf(b->ext_ns, sizeof b->ext_ns, "%s", extension_namespace);
    }
    return 0;
}

static svc_t *svc_at(polycall_config2_builder_t *b, int i)
{
    if (!b || i < 0 || i >= b->service_count) {
        return NULL;
    }
    return &b->services[i];
}

int polycall_config2_builder_add_service(polycall_config2_builder_t *b,
                                         const char *id, const char *language,
                                         polycall_config2_error_t *err)
{
    svc_t *s;
    int i;

    if (!b) {
        return set_err(err, "builder.null", "", "builder is NULL");
    }
    if (b->service_count >= POLYCALL_CFG2_MAX_SERVICES) {
        return set_err(err, "services.too_many", "services",
                       "at most %d services", POLYCALL_CFG2_MAX_SERVICES);
    }
    if (!ident_ok(id, POLYCALL_CFG2_NAME_MAX)) {
        return set_err(err, "service.id.invalid", "services[].id",
                       "service id '%s' is empty or has invalid characters",
                       id ? id : "");
    }
    if (!ident_ok(language, POLYCALL_CFG2_NAME_MAX)) {
        return set_err(err, "service.language.invalid", "services[].language",
                       "service '%s' has an invalid language '%s'",
                       id, language ? language : "");
    }
    for (i = 0; i < b->service_count; ++i) {
        if (strcmp(b->services[i].id, id) == 0) {
            return set_err(err, "service.id.duplicate", "services[].id",
                           "duplicate service id '%s'", id);
        }
    }
    s = &b->services[b->service_count];
    memset(s, 0, sizeof *s);
    snprintf(s->id, sizeof s->id, "%s", id);
    snprintf(s->language, sizeof s->language, "%s", language);
    snprintf(s->bind_address, sizeof s->bind_address, "127.0.0.1");
    s->timeout_ms = 30000;
    s->max_connections = 64;
    s->tls_mode = POLYCALL_TLS_OFF;
    return b->service_count++;
}

int polycall_config2_builder_service_set_ports(polycall_config2_builder_t *b,
                                               int idx, uint32_t hp, uint32_t tp,
                                               polycall_config2_error_t *err)
{
    svc_t *s = svc_at(b, idx);
    char f[64];
    if (!s) {
        return set_err(err, "service.index", "", "no service at index %d", idx);
    }
    if (!port_ok(hp)) {
        snprintf(f, sizeof f, "services[%d].host_port", idx);
        return set_err(err, "port.range", f, "host_port %lu out of range 1-65535",
                       (unsigned long)hp);
    }
    if (!port_ok(tp)) {
        snprintf(f, sizeof f, "services[%d].target_port", idx);
        return set_err(err, "port.range", f, "target_port %lu out of range 1-65535",
                       (unsigned long)tp);
    }
    s->host_port = hp;
    s->target_port = tp;
    s->has_ports = true;
    return 0;
}

int polycall_config2_builder_service_set_bind(polycall_config2_builder_t *b,
                                              int idx, const char *addr,
                                              polycall_config2_error_t *err)
{
    svc_t *s = svc_at(b, idx);
    if (!s) {
        return set_err(err, "service.index", "", "no service at index %d", idx);
    }
    if (!addr || !*addr || strlen(addr) >= sizeof s->bind_address) {
        char f[64];
        snprintf(f, sizeof f, "services[%d].bind_address", idx);
        return set_err(err, "bind_address.invalid", f,
                       "bind_address is empty or too long");
    }
    snprintf(s->bind_address, sizeof s->bind_address, "%s", addr);
    return 0;
}

int polycall_config2_builder_service_set_workspace(polycall_config2_builder_t *b,
                                                   int idx, const char *path,
                                                   polycall_config2_error_t *err)
{
    svc_t *s = svc_at(b, idx);
    char f[64];
    if (!s) {
        return set_err(err, "service.index", "", "no service at index %d", idx);
    }
    snprintf(f, sizeof f, "services[%d].workspace", idx);
    if (!path || !*path || strlen(path) >= sizeof s->workspace) {
        return set_err(err, "workspace.invalid", f,
                       "workspace is empty or too long");
    }
    if (!is_abs_path(path) && b->base_dir[0]) {
        /* join into a local buffer first: s->workspace and b->base_dir are
         * both members of *b, which -Wrestrict flags as a possible overlap. */
        char joined[POLYCALL_CFG2_PATH_MAX];
        int n = snprintf(joined, sizeof joined, "%s/%s", b->base_dir, path);
        if (n < 0 || (size_t)n >= sizeof joined) {
            return set_err(err, "workspace.too_long", f,
                           "resolved workspace path too long");
        }
        snprintf(s->workspace, sizeof s->workspace, "%s", joined);
        prov_set(b->prov, &b->prov_count, (int)(sizeof b->prov / sizeof b->prov[0]),
                 f, POLYCALL_SRC_NATIVE_PROJECT);
    } else {
        snprintf(s->workspace, sizeof s->workspace, "%s", path);
    }
    return 0;
}

int polycall_config2_builder_service_set_limits(polycall_config2_builder_t *b,
                                                int idx, uint32_t timeout_ms,
                                                uint32_t max_connections,
                                                polycall_config2_error_t *err)
{
    svc_t *s = svc_at(b, idx);
    char f[64];
    if (!s) {
        return set_err(err, "service.index", "", "no service at index %d", idx);
    }
    if (timeout_ms == 0) {
        snprintf(f, sizeof f, "services[%d].timeout_ms", idx);
        return set_err(err, "timeout.range", f, "timeout_ms must be > 0");
    }
    if (max_connections == 0) {
        snprintf(f, sizeof f, "services[%d].max_connections", idx);
        return set_err(err, "max_connections.range", f,
                       "max_connections must be > 0");
    }
    s->timeout_ms = timeout_ms;
    s->max_connections = max_connections;
    return 0;
}

int polycall_config2_builder_service_set_tls(polycall_config2_builder_t *b,
                                             int idx, polycall_tls_mode_t mode,
                                             const char *cert_ref,
                                             const char *key_ref,
                                             polycall_config2_error_t *err)
{
    svc_t *s = svc_at(b, idx);
    char f[64];
    if (!s) {
        return set_err(err, "service.index", "", "no service at index %d", idx);
    }
    snprintf(f, sizeof f, "services[%d].tls", idx);
    if (mode != POLYCALL_TLS_OFF && mode != POLYCALL_TLS_SERVER &&
        mode != POLYCALL_TLS_MUTUAL) {
        return set_err(err, "tls.mode.invalid", f, "tls mode %d is not valid",
                       (int)mode);
    }
    if (mode == POLYCALL_TLS_OFF) {
        if ((cert_ref && *cert_ref) || (key_ref && *key_ref)) {
            return set_err(err, "tls.refs.unexpected", f,
                           "tls mode 'off' must not carry cert/key references");
        }
        s->tls_mode = mode;
        s->tls_cert_ref[0] = s->tls_key_ref[0] = '\0';
        return 0;
    }
    if (!ref_ok(cert_ref) || !ref_ok(key_ref)) {
        return set_err(err, "tls.refs.invalid", f,
                       "tls cert_ref/key_ref must be an 'env:' or 'file:' "
                       "reference, not an inline secret");
    }
    if (strlen(cert_ref) >= sizeof s->tls_cert_ref ||
        strlen(key_ref) >= sizeof s->tls_key_ref) {
        return set_err(err, "tls.refs.too_long", f, "tls reference too long");
    }
    s->tls_mode = mode;
    snprintf(s->tls_cert_ref, sizeof s->tls_cert_ref, "%s", cert_ref);
    snprintf(s->tls_key_ref, sizeof s->tls_key_ref, "%s", key_ref);
    return 0;
}

/* ------------------------------------------------------------------ */
/* build (final validation + freeze + canonical service order)        */
/* ------------------------------------------------------------------ */

static int svc_cmp(const void *a, const void *b)
{
    return strcmp(((const svc_t *)a)->id, ((const svc_t *)b)->id);
}

polycall_config2_t *polycall_config2_builder_build(polycall_config2_builder_t *b,
                                                   polycall_config2_error_t *err)
{
    polycall_config2_t *cfg;
    int i;

    if (!b) {
        set_err(err, "builder.null", "", "builder is NULL");
        return NULL;
    }
    if (!ident_ok(b->project_name, POLYCALL_CFG2_NAME_MAX)) {
        set_err(err, "project.name.invalid", "project.name",
                "project name is empty or has invalid characters");
        return NULL;
    }
    if (b->ext_ns[0] && !ident_ok(b->ext_ns, POLYCALL_CFG2_NS_MAX)) {
        set_err(err, "project.extension_namespace.invalid",
                "project.extension_namespace",
                "extension_namespace has invalid characters");
        return NULL;
    }
    if (b->service_count < 1) {
        set_err(err, "services.empty", "services",
                "at least one service is required");
        return NULL;
    }
    for (i = 0; i < b->service_count; ++i) {
        svc_t *s = &b->services[i];
        char f[64];
        if (!s->has_ports) {
            snprintf(f, sizeof f, "services[%d]", i);
            set_err(err, "port.missing", f,
                    "service '%s' has no host:target ports", s->id);
            return NULL;
        }
        if (!s->workspace[0]) {
            snprintf(f, sizeof f, "services[%d].workspace", i);
            set_err(err, "workspace.missing", f,
                    "service '%s' has no workspace", s->id);
            return NULL;
        }
        if (s->tls_mode != POLYCALL_TLS_OFF &&
            (!s->tls_cert_ref[0] || !s->tls_key_ref[0])) {
            snprintf(f, sizeof f, "services[%d].tls", i);
            set_err(err, "tls.refs.missing", f,
                    "service '%s' enables TLS but is missing cert/key refs",
                    s->id);
            return NULL;
        }
    }

    cfg = calloc(1, sizeof *cfg);
    if (!cfg) {
        set_err(err, "oom", "", "out of memory");
        return NULL;
    }
    snprintf(cfg->project_name, sizeof cfg->project_name, "%s", b->project_name);
    snprintf(cfg->ext_ns, sizeof cfg->ext_ns, "%s", b->ext_ns);
    cfg->service_count = b->service_count;
    memcpy(cfg->services, b->services, sizeof b->services);
    qsort(cfg->services, (size_t)cfg->service_count, sizeof cfg->services[0],
          svc_cmp);
    cfg->prov_count = b->prov_count;
    memcpy(cfg->prov, b->prov, sizeof b->prov);
    return cfg;
}

void polycall_config2_free(polycall_config2_t *cfg) { free(cfg); }

const char *polycall_config2_project_name(const polycall_config2_t *cfg)
{
    return cfg ? cfg->project_name : NULL;
}
const char *polycall_config2_extension_namespace(const polycall_config2_t *cfg)
{
    return cfg ? cfg->ext_ns : NULL;
}
int polycall_config2_service_count(const polycall_config2_t *cfg)
{
    return cfg ? cfg->service_count : 0;
}

int polycall_config2_service_at(const polycall_config2_t *cfg, int index,
                                polycall_service_view_t *view)
{
    const svc_t *s;
    if (!cfg || !view || view->struct_size < sizeof *view) {
        return -1;
    }
    if (index < 0 || index >= cfg->service_count) {
        return -1;
    }
    s = &cfg->services[index];
    view->id = s->id;
    view->language = s->language;
    view->bind_address = s->bind_address;
    view->host_port = (uint16_t)s->host_port;
    view->target_port = (uint16_t)s->target_port;
    view->workspace = s->workspace;
    view->timeout_ms = s->timeout_ms;
    view->max_connections = s->max_connections;
    view->tls_mode = s->tls_mode;
    view->tls_cert_ref = s->tls_cert_ref[0] ? s->tls_cert_ref : NULL;
    view->tls_key_ref = s->tls_key_ref[0] ? s->tls_key_ref : NULL;
    return 0;
}

/* ------------------------------------------------------------------ */
/* canonical envelope writer                                          */
/* ------------------------------------------------------------------ */

typedef struct { char *p; size_t len, cap; bool oom; } sb_t;

static void sb_putc(sb_t *sb, char c)
{
    if (sb->oom) return;
    if (sb->len + 1 >= sb->cap) {
        size_t nc = sb->cap ? sb->cap * 2 : 256;
        char *np = realloc(sb->p, nc);
        if (!np) { sb->oom = true; return; }
        sb->p = np; sb->cap = nc;
    }
    sb->p[sb->len++] = c;
    sb->p[sb->len] = '\0';
}

static void sb_puts(sb_t *sb, const char *s) { for (; s && *s; ++s) sb_putc(sb, *s); }

static void sb_json_str(sb_t *sb, const char *s)
{
    sb_putc(sb, '"');
    for (; s && *s; ++s) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  sb_puts(sb, "\\\""); break;
        case '\\': sb_puts(sb, "\\\\"); break;
        case '\b': sb_puts(sb, "\\b"); break;
        case '\f': sb_puts(sb, "\\f"); break;
        case '\n': sb_puts(sb, "\\n"); break;
        case '\r': sb_puts(sb, "\\r"); break;
        case '\t': sb_puts(sb, "\\t"); break;
        default:
            if (c < 0x20) {
                char b[8];
                snprintf(b, sizeof b, "\\u%04x", c);
                sb_puts(sb, b);
            } else {
                sb_putc(sb, (char)c);
            }
        }
    }
    sb_putc(sb, '"');
}

static void sb_u32(sb_t *sb, uint32_t v)
{
    char b[16];
    snprintf(b, sizeof b, "%lu", (unsigned long)v);
    sb_puts(sb, b);
}

static const char *tls_name(polycall_tls_mode_t m)
{
    switch (m) {
    case POLYCALL_TLS_SERVER: return "server";
    case POLYCALL_TLS_MUTUAL: return "mutual";
    default: return "off";
    }
}

int polycall_config2_to_envelope(const polycall_config2_t *cfg,
                                 char *buf, size_t cap)
{
    sb_t sb = {0};
    int i, total;

    if (!cfg) {
        if (buf && cap) buf[0] = '\0';
        return -1;
    }

    sb_puts(&sb, "{\"schema_version\":2,\"project\":{\"name\":");
    sb_json_str(&sb, cfg->project_name);
    sb_puts(&sb, ",\"extension_namespace\":");
    if (cfg->ext_ns[0]) sb_json_str(&sb, cfg->ext_ns); else sb_puts(&sb, "null");
    sb_puts(&sb, "},\"services\":[");
    for (i = 0; i < cfg->service_count; ++i) {
        const svc_t *s = &cfg->services[i];
        if (i) sb_putc(&sb, ',');
        sb_puts(&sb, "{\"id\":");            sb_json_str(&sb, s->id);
        sb_puts(&sb, ",\"language\":");      sb_json_str(&sb, s->language);
        sb_puts(&sb, ",\"bind_address\":");  sb_json_str(&sb, s->bind_address);
        sb_puts(&sb, ",\"host_port\":");     sb_u32(&sb, s->host_port);
        sb_puts(&sb, ",\"target_port\":");   sb_u32(&sb, s->target_port);
        sb_puts(&sb, ",\"workspace\":");     sb_json_str(&sb, s->workspace);
        sb_puts(&sb, ",\"timeout_ms\":");    sb_u32(&sb, s->timeout_ms);
        sb_puts(&sb, ",\"max_connections\":"); sb_u32(&sb, s->max_connections);
        sb_puts(&sb, ",\"tls\":{\"mode\":");
        sb_json_str(&sb, tls_name(s->tls_mode));
        sb_puts(&sb, ",\"cert_ref\":");
        if (s->tls_cert_ref[0]) sb_json_str(&sb, s->tls_cert_ref); else sb_puts(&sb, "null");
        sb_puts(&sb, ",\"key_ref\":");
        if (s->tls_key_ref[0]) sb_json_str(&sb, s->tls_key_ref); else sb_puts(&sb, "null");
        sb_puts(&sb, "}}");
    }
    sb_puts(&sb, "]}");

    if (sb.oom) {
        free(sb.p);
        if (buf && cap) buf[0] = '\0';
        return -1;
    }
    total = (int)sb.len;
    if (buf && cap) {
        size_t n = sb.len < cap - 1 ? sb.len : cap - 1;
        memcpy(buf, sb.p, n);
        buf[n] = '\0';
    }
    free(sb.p);
    return total;
}

/* ------------------------------------------------------------------ */
/* envelope reader                                                    */
/* ------------------------------------------------------------------ */

static int known_only(const json_value *obj, const char *const *keys, size_t nk,
                      const char *where, polycall_config2_error_t *err)
{
    size_t i, j;
    if (!obj || obj->type != JSON_OBJECT) {
        return set_err(err, "envelope.type", where, "%s must be an object", where);
    }
    for (i = 0; i < obj->count; ++i) {
        bool found = false;
        for (j = 0; j < nk; ++j) {
            if (strcmp(obj->keys[i], keys[j]) == 0) { found = true; break; }
        }
        if (!found) {
            return set_err(err, "field.unknown", where,
                           "unknown field '%s' in %s", obj->keys[i], where);
        }
    }
    return 0;
}

static polycall_cfg_source_t src_from_name(const char *n)
{
    if (!n) return POLYCALL_SRC_DEFAULT;
    if (!strcmp(n, "native_global")) return POLYCALL_SRC_NATIVE_GLOBAL;
    if (!strcmp(n, "native_project")) return POLYCALL_SRC_NATIVE_PROJECT;
    if (!strcmp(n, "lang_override")) return POLYCALL_SRC_LANG_OVERRIDE;
    if (!strcmp(n, "env")) return POLYCALL_SRC_ENV;
    if (!strcmp(n, "cli")) return POLYCALL_SRC_CLI;
    if (!strcmp(n, "legacy")) return POLYCALL_SRC_LEGACY;
    return POLYCALL_SRC_DEFAULT;
}

polycall_config2_t *polycall_config2_from_envelope(const char *text, size_t len,
                                                   polycall_config2_error_t *err)
{
    static const char *TOP[]  = { "schema_version", "project", "services",
                                  "provenance" };
    static const char *PROJ[] = { "name", "extension_namespace" };
    static const char *SVC[]  = { "id", "language", "bind_address", "host_port",
                                  "target_port", "workspace", "timeout_ms",
                                  "max_connections", "tls" };
    static const char *TLS[]  = { "mode", "cert_ref", "key_ref" };
    char jerr[128];
    json_value *root;
    const json_value *jp, *js, *jv;
    polycall_config2_builder_t *b;
    polycall_config2_t *cfg;
    bool ok;
    double sv;
    size_t i;

    polycall_config2_error_init(err);

    if (len > POLYCALL_CFG2_ENVELOPE_MAX) {
        set_err(err, "envelope.too_large", "",
                "envelope exceeds %u bytes", POLYCALL_CFG2_ENVELOPE_MAX);
        return NULL;
    }
    root = json_parse(text, len, jerr, sizeof jerr);
    if (!root) {
        set_err(err, "envelope.parse", "", "%s", jerr);
        return NULL;
    }
    if (known_only(root, TOP, sizeof TOP / sizeof TOP[0], "envelope", err) != 0) {
        json_free(root);
        return NULL;
    }
    sv = json_num(json_get(root, "schema_version"), -1, &ok);
    if (!ok || sv != 2) {
        set_err(err, "schema_version", "schema_version",
                "schema_version must be 2");
        json_free(root);
        return NULL;
    }

    jp = json_get(root, "project");
    if (known_only(jp, PROJ, sizeof PROJ / sizeof PROJ[0], "project", err) != 0) {
        json_free(root);
        return NULL;
    }
    js = json_get(root, "services");
    if (!js || js->type != JSON_ARRAY || js->count == 0) {
        set_err(err, "services.empty", "services",
                "services must be a non-empty array");
        json_free(root);
        return NULL;
    }

    b = polycall_config2_builder_create();
    if (!b) {
        set_err(err, "oom", "", "out of memory");
        json_free(root);
        return NULL;
    }
    polycall_config2_builder_set_project(
        b, json_str(json_get(jp, "name"), "", NULL),
        json_str(json_get(jp, "extension_namespace"), NULL, NULL));

    for (i = 0; i < js->count; ++i) {
        const json_value *sv_obj = js->items[i];
        const json_value *tls;
        int idx;
        double hp, tp, to, mc;
        polycall_tls_mode_t mode = POLYCALL_TLS_OFF;
        const char *mname;

        if (known_only(sv_obj, SVC, sizeof SVC / sizeof SVC[0],
                       "services[]", err) != 0) {
            goto fail;
        }
        idx = polycall_config2_builder_add_service(
            b, json_str(json_get(sv_obj, "id"), "", NULL),
            json_str(json_get(sv_obj, "language"), "", NULL), err);
        if (idx < 0) {
            goto fail;
        }

        hp = json_num(json_get(sv_obj, "host_port"), -1, &ok);
        if (!ok) { set_err(err, "port.type", "host_port", "host_port must be a number"); goto fail; }
        tp = json_num(json_get(sv_obj, "target_port"), -1, &ok);
        if (!ok) { set_err(err, "port.type", "target_port", "target_port must be a number"); goto fail; }
        if (hp != (double)(uint32_t)hp || tp != (double)(uint32_t)tp) {
            set_err(err, "port.type", "host_port", "ports must be integers");
            goto fail;
        }
        if (polycall_config2_builder_service_set_ports(
                b, idx, (uint32_t)hp, (uint32_t)tp, err) != 0) {
            goto fail;
        }

        jv = json_get(sv_obj, "bind_address");
        if (jv && polycall_config2_builder_service_set_bind(
                      b, idx, json_str(jv, "", NULL), err) != 0) {
            goto fail;
        }
        jv = json_get(sv_obj, "workspace");
        if (!jv || jv->type != JSON_STRING) {
            set_err(err, "workspace.missing", "workspace",
                    "workspace is required and must be a string");
            goto fail;
        }
        if (polycall_config2_builder_service_set_workspace(
                b, idx, jv->string, err) != 0) {
            goto fail;
        }

        to = json_num(json_get(sv_obj, "timeout_ms"), 30000, &ok);
        if (!ok && json_get(sv_obj, "timeout_ms")) {
            set_err(err, "timeout.type", "timeout_ms", "timeout_ms must be a number");
            goto fail;
        }
        mc = json_num(json_get(sv_obj, "max_connections"), 64, &ok);
        if (!ok && json_get(sv_obj, "max_connections")) {
            set_err(err, "max_connections.type", "max_connections",
                    "max_connections must be a number");
            goto fail;
        }
        if (to < 1 || to != (double)(uint32_t)to ||
            mc < 1 || mc != (double)(uint32_t)mc) {
            set_err(err, "limits.range", "timeout_ms/max_connections",
                    "timeout_ms and max_connections must be positive integers");
            goto fail;
        }
        if (polycall_config2_builder_service_set_limits(
                b, idx, (uint32_t)to, (uint32_t)mc, err) != 0) {
            goto fail;
        }

        tls = json_get(sv_obj, "tls");
        if (tls) {
            if (known_only(tls, TLS, sizeof TLS / sizeof TLS[0],
                           "services[].tls", err) != 0) {
                goto fail;
            }
            mname = json_str(json_get(tls, "mode"), "off", NULL);
            if (!strcmp(mname, "off")) mode = POLYCALL_TLS_OFF;
            else if (!strcmp(mname, "server")) mode = POLYCALL_TLS_SERVER;
            else if (!strcmp(mname, "mutual")) mode = POLYCALL_TLS_MUTUAL;
            else { set_err(err, "tls.mode.invalid", "tls.mode",
                           "tls mode '%s' is not off/server/mutual", mname); goto fail; }
            if (polycall_config2_builder_service_set_tls(
                    b, idx, mode,
                    json_str(json_get(tls, "cert_ref"), NULL, NULL),
                    json_str(json_get(tls, "key_ref"), NULL, NULL), err) != 0) {
                goto fail;
            }
        }
    }

    cfg = polycall_config2_builder_build(b, err);
    if (!cfg) {
        goto fail;
    }

    /* optional provenance sidecar */
    jv = json_get(root, "provenance");
    if (jv && jv->type == JSON_OBJECT) {
        size_t k;
        for (k = 0; k < jv->count && cfg->prov_count <
             (int)(sizeof cfg->prov / sizeof cfg->prov[0]); ++k) {
            prov_set(cfg->prov, &cfg->prov_count,
                     (int)(sizeof cfg->prov / sizeof cfg->prov[0]),
                     jv->keys[k],
                     src_from_name(json_str(jv->values[k], NULL, NULL)));
        }
    }

    polycall_config2_builder_free(b);
    json_free(root);
    return cfg;

fail:
    polycall_config2_builder_free(b);
    json_free(root);
    return NULL;
}

polycall_cfg_source_t polycall_config2_provenance(const polycall_config2_t *cfg,
                                                  const char *field)
{
    int i;
    if (!cfg || !field) {
        return POLYCALL_SRC_DEFAULT;
    }
    for (i = 0; i < cfg->prov_count; ++i) {
        if (strcmp(cfg->prov[i].field, field) == 0) {
            return cfg->prov[i].source;
        }
    }
    return POLYCALL_SRC_DEFAULT;
}
