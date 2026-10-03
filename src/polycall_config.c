#include "polycall_config.h"
#include "config/cfgfile.h"
#include "core/pc_file.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Diagnostics. The CLI path prints them to stderr exactly as before; the
 * library path (polycall_cfgfile_load, used by polycall_ffi_run_config and
 * the daemon) captures the first one into a caller buffer instead, so a
 * binding gets the reason as data and nothing is written to its stderr.
 * The capture pointer is thread-local: concurrent loads never share it.
 */
#if defined(_MSC_VER)
#  define CFG_TLS __declspec(thread)
#else
#  define CFG_TLS _Thread_local
#endif
static CFG_TLS char *g_cfg_capture;
static CFG_TLS size_t g_cfg_capture_cap;

static void cfg_errf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (g_cfg_capture) {
        if (g_cfg_capture_cap && g_cfg_capture[0] == '\0') {
            size_t n;
            vsnprintf(g_cfg_capture, g_cfg_capture_cap, fmt, ap);
            n = strlen(g_cfg_capture);
            while (n && (g_cfg_capture[n - 1] == '\n' || g_cfg_capture[n - 1] == '\r')) {
                g_cfg_capture[--n] = '\0';
            }
        }
    } else {
        vfprintf(stderr, fmt, ap);
    }
    va_end(ap);
}

#define POLYCALL_CONFIG_MAX_SERVERS 32
#define POLYCALL_CONFIG_MAX_PEERS 64
#define POLYCALL_CONFIG_MAX_VALUES 128
#define POLYCALL_CONFIG_KEY_SIZE 64
#define POLYCALL_CONFIG_VALUE_SIZE 256
#define POLYCALL_CONFIG_PATH_SIZE 512
#define POLYCALL_CONFIG_LINE_SIZE 1024

typedef enum {
    CONFIG_LAYER_PROJECT,
    CONFIG_LAYER_RUNTIME,
    CONFIG_LAYER_LANGUAGE
} ConfigLayer;

typedef struct {
    char language[32];
    unsigned int host_port;
    unsigned int target_port;
} ConfigServer;

typedef struct {
    char key[POLYCALL_CONFIG_KEY_SIZE];
    char value[POLYCALL_CONFIG_VALUE_SIZE];
} ConfigValue;

typedef struct {
    char id[64];
    char endpoint[128];
} ConfigPeer;

typedef struct {
    ConfigServer servers[POLYCALL_CONFIG_MAX_SERVERS];
    size_t server_count;
    ConfigValue values[POLYCALL_CONFIG_MAX_VALUES];
    size_t value_count;
    ConfigPeer peers[POLYCALL_CONFIG_MAX_PEERS];
    size_t peer_count;
    bool network_enabled;
    bool network_seen;
    unsigned int warnings;
} PolycallConfig;

typedef struct {
    char paths[3][POLYCALL_CONFIG_PATH_SIZE];
    size_t count;
    bool used_legacy;
} LoadedSources;

static const char* const PROJECT_KEYS[] = {
    "network_timeout",
    "max_connections",
    "log_directory",
    "workspace_root",
    "auto_discover",
    "discovery_interval",
    "tls_enabled",
    "cert_file",
    "key_file",
    "max_memory_per_service",
    "max_cpu_per_service",
    "enable_metrics",
    "metrics_port",
    /* daemon (`polycall daemon ...`) -- Polycallfile only */
    "daemon_endpoint",
    "daemon_state_dir",
    "daemon_log_file",
    "daemon_max_connections",
    "daemon_drain_timeout_ms",
    "daemon_io_timeout_ms",
    /* shared-token authentication: NAME of the environment variable that
     * holds the secret (default POLYCALL_DEV_TOKEN); never the secret */
    "auth_token_env",
    /* peer node (`polycall peer serve`); peers are `peer <id> <host:port>` */
    "peer_node_id",
    "peer_endpoint",
    "peer_inbox_capacity"
};

static const char* const RUNTIME_KEYS[] = {
    "port",
    "server_type",
    "workspace",
    "log_level",
    "max_connections",
    "supports_diagnostics",
    "supports_completion",
    "supports_formatting",
    "max_memory",
    "timeout",
    "allow_remote",
    "require_auth",
    "network_timeout",
    "log_directory",
    "workspace_root",
    "auto_discover",
    "discovery_interval",
    "tls_enabled",
    "cert_file",
    "key_file",
    "max_memory_per_service",
    "max_cpu_per_service",
    "enable_metrics",
    "metrics_port"
};

static const char* const BOOLEAN_KEYS[] = {
    "auto_discover",
    "tls_enabled",
    "enable_metrics",
    "supports_diagnostics",
    "supports_completion",
    "supports_formatting",
    "allow_remote",
    "require_auth"
};

static const char* const POSITIVE_INTEGER_KEYS[] = {
    "network_timeout",
    "max_connections",
    "discovery_interval",
    "max_cpu_per_service",
    "metrics_port",
    "timeout"
};

static char* trim(char* text) {
    char* end;

    while (*text && isspace((unsigned char)*text)) {
        text++;
    }

    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';
    return text;
}

static bool file_exists(const char* path) {
    FILE* file = pc_fopen(path, "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

static bool string_in_list(
    const char* value,
    const char* const* values,
    size_t value_count
) {
    size_t i;
    for (i = 0; i < value_count; i++) {
        if (strcmp(value, values[i]) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_known_key(const char* key, ConfigLayer layer) {
    if (layer == CONFIG_LAYER_PROJECT) {
        return string_in_list(
            key,
            PROJECT_KEYS,
            sizeof(PROJECT_KEYS) / sizeof(PROJECT_KEYS[0])
        );
    }
    return string_in_list(
        key,
        RUNTIME_KEYS,
        sizeof(RUNTIME_KEYS) / sizeof(RUNTIME_KEYS[0])
    );
}

static bool parse_positive_integer(const char* value, unsigned long* result) {
    char* end = NULL;
    unsigned long parsed;

    if (!value || !*value || *value == '-') {
        return false;
    }

    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || !end || *end != '\0' || parsed == 0) {
        return false;
    }

    if (result) {
        *result = parsed;
    }
    return true;
}

static bool parse_port_mapping(
    const char* value,
    unsigned int* host_port,
    unsigned int* target_port
) {
    char buffer[64];
    char* separator;
    unsigned long host;
    unsigned long target;

    if (!value || strlen(value) >= sizeof(buffer)) {
        return false;
    }

    strcpy(buffer, value);
    separator = strchr(buffer, ':');
    if (!separator || strchr(separator + 1, ':')) {
        return false;
    }

    *separator = '\0';
    if (!parse_positive_integer(buffer, &host) ||
        !parse_positive_integer(separator + 1, &target) ||
        host > 65535 || target > 65535) {
        return false;
    }

    if (host_port) {
        *host_port = (unsigned int)host;
    }
    if (target_port) {
        *target_port = (unsigned int)target;
    }
    return true;
}

static const char* config_get(const PolycallConfig* config, const char* key) {
    size_t i;
    for (i = 0; i < config->value_count; i++) {
        if (strcmp(config->values[i].key, key) == 0) {
            return config->values[i].value;
        }
    }
    return NULL;
}

static bool config_set(
    PolycallConfig* config,
    const char* key,
    const char* value
) {
    size_t i;

    for (i = 0; i < config->value_count; i++) {
        if (strcmp(config->values[i].key, key) == 0) {
            snprintf(
                config->values[i].value,
                sizeof(config->values[i].value),
                "%s",
                value
            );
            return true;
        }
    }

    if (config->value_count >= POLYCALL_CONFIG_MAX_VALUES) {
        return false;
    }

    snprintf(
        config->values[config->value_count].key,
        sizeof(config->values[config->value_count].key),
        "%s",
        key
    );
    snprintf(
        config->values[config->value_count].value,
        sizeof(config->values[config->value_count].value),
        "%s",
        value
    );
    config->value_count++;
    return true;
}

static bool validate_value(
    const char* path,
    unsigned int line_number,
    const char* key,
    const char* value
) {
    unsigned long number;

    if (strcmp(key, "port") == 0) {
        if (!parse_port_mapping(value, NULL, NULL)) {
            cfg_errf("%s:%u: invalid port mapping '%s' (expected 1-65535:1-65535)\n",
                path,
                line_number,
                value
            );
            return false;
        }
    }

    if (string_in_list(
            key,
            BOOLEAN_KEYS,
            sizeof(BOOLEAN_KEYS) / sizeof(BOOLEAN_KEYS[0])
        ) &&
        strcmp(value, "true") != 0 &&
        strcmp(value, "false") != 0) {
        cfg_errf("%s:%u: '%s' must be true or false\n",
            path,
            line_number,
            key
        );
        return false;
    }

    if (string_in_list(
            key,
            POSITIVE_INTEGER_KEYS,
            sizeof(POSITIVE_INTEGER_KEYS) /
                sizeof(POSITIVE_INTEGER_KEYS[0])
        ) &&
        !parse_positive_integer(value, &number)) {
        cfg_errf("%s:%u: '%s' must be a positive integer\n",
            path,
            line_number,
            key
        );
        return false;
    }

    if (strcmp(key, "metrics_port") == 0 &&
        (!parse_positive_integer(value, &number) || number > 65535)) {
        cfg_errf("%s:%u: metrics_port must be between 1 and 65535\n",
            path,
            line_number
        );
        return false;
    }

    if ((strcmp(key, "daemon_endpoint") == 0 || strcmp(key, "peer_endpoint") == 0) &&
        !polycall_cfg_valid_endpoint(value, true)) {
        cfg_errf("%s:%u: '%s' must be host:port (port 0-65535)\n",
                 path, line_number, key);
        return false;
    }
    {
        static const struct { const char *key; unsigned long lo, hi; } ranges[] = {
            { "daemon_max_connections", 1, 1024 },
            { "daemon_drain_timeout_ms", 1, 600000 },
            { "daemon_io_timeout_ms", 1, 600000 },
            { "peer_inbox_capacity", 1, 65536 },
        };
        size_t r;
        for (r = 0; r < sizeof ranges / sizeof ranges[0]; ++r) {
            if (strcmp(key, ranges[r].key) == 0 &&
                (!parse_positive_integer(value, &number) ||
                 number < ranges[r].lo || number > ranges[r].hi)) {
                cfg_errf("%s:%u: '%s' must be an integer in %lu..%lu\n",
                         path, line_number, key, ranges[r].lo, ranges[r].hi);
                return false;
            }
        }
    }
    if (strcmp(key, "auth_token_env") == 0 && !polycall_cfg_valid_env_name(value)) {
        cfg_errf("%s:%u: auth_token_env must name an environment variable "
                 "([A-Za-z_][A-Za-z0-9_]*), not hold a secret\n",
                 path, line_number);
        return false;
    }
    if (strcmp(key, "peer_node_id") == 0 && !polycall_cfg_valid_peer_id(value)) {
        cfg_errf("%s:%u: peer_node_id must be 1-63 characters of [A-Za-z0-9._-]\n",
                 path, line_number);
        return false;
    }

    return true;
}

/* ---- shared validators (also used by the daemon / peer CLI) ---------- */

bool polycall_cfg_valid_peer_id(const char *id)
{
    size_t n = 0;
    if (!id || !*id) {
        return false;
    }
    for (; id[n]; ++n) {
        unsigned char c = (unsigned char)id[n];
        if (n >= 63 || !(isalnum(c) || c == '.' || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

bool polycall_cfg_valid_env_name(const char *name)
{
    size_t n;
    if (!name || !*name || isdigit((unsigned char)name[0])) {
        return false;
    }
    for (n = 0; name[n]; ++n) {
        unsigned char c = (unsigned char)name[n];
        if (n >= 127 || !(isalnum(c) || c == '_')) {
            return false;
        }
    }
    return true;
}

bool polycall_cfg_valid_endpoint(const char *ep, bool allow_port_zero)
{
    const char *colon;
    size_t hostlen, i;
    char *end = NULL;
    unsigned long port;

    if (!ep || !*ep || strlen(ep) >= 128) {
        return false;
    }
    colon = strrchr(ep, ':');
    if (!colon || colon == ep || colon[1] == '\0') {
        return false;
    }
    hostlen = (size_t)(colon - ep);
    for (i = 0; i < hostlen; ++i) {
        unsigned char c = (unsigned char)ep[i];
        if (!(isalnum(c) || c == '.' || c == '-' || c == '_')) {
            return false;   /* IPv4 dotted quad or a DNS name; no IPv6 yet */
        }
    }
    if (!isdigit((unsigned char)colon[1])) {
        return false;
    }
    errno = 0;
    port = strtoul(colon + 1, &end, 10);
    if (errno || !end || *end != '\0' || port > 65535 ||
        (port == 0 && !allow_port_zero)) {
        return false;
    }
    return true;
}

static bool add_peer(
    PolycallConfig* config,
    const char* path,
    unsigned int line_number,
    const char* id,
    const char* endpoint
) {
    size_t i;
    if (!polycall_cfg_valid_peer_id(id)) {
        cfg_errf("%s:%u: invalid peer id '%s' (1-63 of [A-Za-z0-9._-])\n",
                 path, line_number, id);
        return false;
    }
    if (!polycall_cfg_valid_endpoint(endpoint, false)) {
        cfg_errf("%s:%u: invalid peer endpoint '%s' (expected host:port)\n",
                 path, line_number, endpoint);
        return false;
    }
    for (i = 0; i < config->peer_count; i++) {
        if (strcmp(config->peers[i].id, id) == 0) {
            cfg_errf("%s:%u: duplicate peer id '%s'\n", path, line_number, id);
            return false;
        }
    }
    if (config->peer_count >= POLYCALL_CONFIG_MAX_PEERS) {
        cfg_errf("%s:%u: too many peer definitions\n", path, line_number);
        return false;
    }
    snprintf(config->peers[config->peer_count].id,
             sizeof config->peers[0].id, "%s", id);
    snprintf(config->peers[config->peer_count].endpoint,
             sizeof config->peers[0].endpoint, "%s", endpoint);
    config->peer_count++;
    return true;
}

static bool add_server(
    PolycallConfig* config,
    const char* path,
    unsigned int line_number,
    const char* language,
    const char* mapping
) {
    size_t i;
    unsigned int host_port;
    unsigned int target_port;

    if (!*language || strlen(language) >= sizeof(config->servers[0].language)) {
        cfg_errf("%s:%u: invalid server language\n", path, line_number);
        return false;
    }

    if (!parse_port_mapping(mapping, &host_port, &target_port)) {
        cfg_errf("%s:%u: invalid server port mapping '%s'\n",
            path,
            line_number,
            mapping
        );
        return false;
    }

    for (i = 0; i < config->server_count; i++) {
        if (strcmp(config->servers[i].language, language) == 0) {
            cfg_errf("%s:%u: duplicate server language '%s'\n",
                path,
                line_number,
                language
            );
            return false;
        }
    }

    if (config->server_count >= POLYCALL_CONFIG_MAX_SERVERS) {
        cfg_errf("%s:%u: too many server definitions\n", path, line_number);
        return false;
    }

    snprintf(
        config->servers[config->server_count].language,
        sizeof(config->servers[config->server_count].language),
        "%s",
        language
    );
    config->servers[config->server_count].host_port = host_port;
    config->servers[config->server_count].target_port = target_port;
    config->server_count++;
    return true;
}

static bool parse_config_file(
    PolycallConfig* config,
    const char* path,
    ConfigLayer layer
) {
    FILE* file;
    char raw_line[POLYCALL_CONFIG_LINE_SIZE];
    unsigned int line_number = 0;

    file = pc_fopen(path, "r");
    if (!file) {
        cfg_errf("Unable to open configuration file: %s\n", path);
        return false;
    }

    while (fgets(raw_line, sizeof(raw_line), file)) {
        char* line;
        char* comment;
        char* equals;

        line_number++;
        if (!strchr(raw_line, '\n') && !feof(file)) {
            cfg_errf("%s:%u: line is too long\n", path, line_number);
            fclose(file);
            return false;
        }

        comment = strchr(raw_line, '#');
        if (comment) {
            *comment = '\0';
        }
        line = trim(raw_line);
        if (!*line) {
            continue;
        }

        if (strncmp(line, "server", 6) == 0 &&
            isspace((unsigned char)line[6])) {
            char language[32];
            char mapping[64];
            char extra[2];

            if (layer != CONFIG_LAYER_PROJECT ||
                sscanf(line, "server %31s %63s %1s", language, mapping, extra) != 2 ||
                !add_server(config, path, line_number, language, mapping)) {
                if (layer != CONFIG_LAYER_PROJECT) {
                    cfg_errf("%s:%u: server definitions belong in Polycallfile\n",
                        path,
                        line_number
                    );
                } else if (sscanf(
                               line,
                               "server %31s %63s %1s",
                               language,
                               mapping,
                               extra
                           ) != 2) {
                    cfg_errf("%s:%u: expected 'server <language> <host>:<target>'\n",
                        path,
                        line_number
                    );
                }
                fclose(file);
                return false;
            }
            continue;
        }

        if (strncmp(line, "network", 7) == 0 &&
            isspace((unsigned char)line[7])) {
            char action[16];
            char extra[2];

            if (layer != CONFIG_LAYER_PROJECT ||
                sscanf(line, "network %15s %1s", action, extra) != 1 ||
                (strcmp(action, "start") != 0 &&
                 strcmp(action, "stop") != 0)) {
                cfg_errf("%s:%u: expected 'network start' or 'network stop' in Polycallfile\n",
                    path,
                    line_number
                );
                fclose(file);
                return false;
            }
            config->network_enabled = strcmp(action, "start") == 0;
            config->network_seen = true;
            continue;
        }

        if (strncmp(line, "peer", 4) == 0 &&
            isspace((unsigned char)line[4])) {
            char id[64];
            char endpoint[128];
            char extra[2];

            if (layer != CONFIG_LAYER_PROJECT) {
                cfg_errf("%s:%u: peer definitions belong in Polycallfile\n",
                         path, line_number);
                fclose(file);
                return false;
            }
            if (sscanf(line, "peer %63s %127s %1s", id, endpoint, extra) != 2) {
                cfg_errf("%s:%u: expected 'peer <id> <host>:<port>'\n",
                         path, line_number);
                fclose(file);
                return false;
            }
            if (!add_peer(config, path, line_number, id, endpoint)) {
                fclose(file);
                return false;
            }
            continue;
        }

        equals = strchr(line, '=');
        if (equals) {
            char* key;
            char* value;

            if (strchr(equals + 1, '=')) {
                cfg_errf("%s:%u: malformed key=value entry\n", path, line_number);
                fclose(file);
                return false;
            }
            *equals = '\0';
            key = trim(line);
            value = trim(equals + 1);
            if (!*key || !*value || strlen(key) >= POLYCALL_CONFIG_KEY_SIZE ||
                strlen(value) >= POLYCALL_CONFIG_VALUE_SIZE) {
                cfg_errf("%s:%u: malformed key=value entry\n", path, line_number);
                fclose(file);
                return false;
            }
            if (!is_known_key(key, layer)) {
                cfg_errf("%s:%u: warning: unknown key '%s'\n",
                    path,
                    line_number,
                    key
                );
                config->warnings++;
            }
            if (!validate_value(path, line_number, key, value) ||
                !config_set(config, key, value)) {
                if (config->value_count >= POLYCALL_CONFIG_MAX_VALUES) {
                    cfg_errf("%s:%u: too many configuration values\n", path, line_number);
                }
                fclose(file);
                return false;
            }
            continue;
        }

        cfg_errf("%s:%u: malformed configuration syntax\n", path, line_number);
        fclose(file);
        return false;
    }

    if (ferror(file)) {
        cfg_errf("Failed while reading configuration file: %s\n", path);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

static bool validate_required(
    const PolycallConfig* config,
    bool require_project_fields,
    const char* language
) {
    const char* tls_enabled;
    const char* server_type;
    bool valid = true;

    if (require_project_fields) {
        if (!config_get(config, "workspace_root")) {
            cfg_errf("Missing required field: workspace_root\n");
            valid = false;
        }
        if (!config_get(config, "log_directory")) {
            cfg_errf("Missing required field: log_directory\n");
            valid = false;
        }
    }

    tls_enabled = config_get(config, "tls_enabled");
    if (tls_enabled && strcmp(tls_enabled, "true") == 0) {
        if (!config_get(config, "cert_file")) {
            cfg_errf("Missing required field: cert_file (tls_enabled=true)\n");
            valid = false;
        }
        if (!config_get(config, "key_file")) {
            cfg_errf("Missing required field: key_file (tls_enabled=true)\n");
            valid = false;
        }
    }

    if (language) {
        if (!config_get(config, "workspace")) {
            cfg_errf("Missing required field: workspace\n");
            valid = false;
        }
        server_type = config_get(config, "server_type");
        if (!server_type) {
            cfg_errf("Missing required field: server_type\n");
            valid = false;
        } else if (strcmp(server_type, language) != 0) {
            cfg_errf("server_type '%s' does not match Polycallrc.%s\n",
                server_type,
                language
            );
            valid = false;
        }
    }

    return valid;
}

static bool valid_language(const char* language) {
    const unsigned char* cursor = (const unsigned char*)language;
    if (!language || !*language) {
        return false;
    }
    while (*cursor) {
        if (!isalnum(*cursor) && *cursor != '-' && *cursor != '_') {
            return false;
        }
        cursor++;
    }
    return true;
}

static bool record_source(LoadedSources* sources, const char* path) {
    if (sources->count >= sizeof(sources->paths) / sizeof(sources->paths[0])) {
        return false;
    }
    snprintf(
        sources->paths[sources->count],
        sizeof(sources->paths[sources->count]),
        "%s",
        path
    );
    sources->count++;
    return true;
}

static bool load_hierarchy(
    PolycallConfig* config,
    const char* language,
    LoadedSources* sources
) {
    char language_path[POLYCALL_CONFIG_PATH_SIZE];

    if (!file_exists("Polycallfile")) {
        cfg_errf("Missing project configuration: Polycallfile\n");
        return false;
    }
    if (!parse_config_file(config, "Polycallfile", CONFIG_LAYER_PROJECT) ||
        !record_source(sources, "Polycallfile")) {
        return false;
    }

    if (file_exists("Polycallrc")) {
        if (!parse_config_file(config, "Polycallrc", CONFIG_LAYER_RUNTIME) ||
            !record_source(sources, "Polycallrc")) {
            return false;
        }
    }

    if (language) {
        if (!valid_language(language)) {
            cfg_errf("Invalid language name: %s\n", language);
            return false;
        }
        snprintf(language_path, sizeof(language_path), "Polycallrc.%s", language);
        if (file_exists(language_path)) {
            if (!parse_config_file(config, language_path, CONFIG_LAYER_LANGUAGE) ||
                !record_source(sources, language_path)) {
                return false;
            }
        } else if (file_exists(".polycallrc")) {
            cfg_errf("warning: Legacy .polycallrc detected. "
                "Use Polycallrc.<language> for v1.0.0.\n"
            );
            if (!parse_config_file(config, ".polycallrc", CONFIG_LAYER_LANGUAGE) ||
                !record_source(sources, ".polycallrc")) {
                return false;
            }
            sources->used_legacy = true;
        } else {
            cfg_errf("Missing runtime override: %s\n", language_path);
            return false;
        }
    }

    return validate_required(config, true, language);
}

static void show_config(const PolycallConfig* config) {
    size_t i;

    for (i = 0; i < config->server_count; i++) {
        printf(
            "server %s %u:%u\n",
            config->servers[i].language,
            config->servers[i].host_port,
            config->servers[i].target_port
        );
    }
    if (config->network_seen) {
        printf("network %s\n", config->network_enabled ? "start" : "stop");
    }
    for (i = 0; i < config->peer_count; i++) {
        printf("peer %s %s\n", config->peers[i].id, config->peers[i].endpoint);
    }
    for (i = 0; i < config->value_count; i++) {
        printf("%s=%s\n", config->values[i].key, config->values[i].value);
    }
}

static ConfigLayer infer_layer(const char* path, const char** language) {
    const char* name = strrchr(path, '/');
    const char* windows_name = strrchr(path, '\\');
    const char* suffix;

    if (!name || (windows_name && windows_name > name)) {
        name = windows_name;
    }
    name = name ? name + 1 : path;

    if (strcmp(name, "Polycallfile") == 0) {
        return CONFIG_LAYER_PROJECT;
    }
    suffix = strstr(name, "Polycallrc.");
    if (suffix == name && suffix[11] != '\0') {
        *language = suffix + 11;
        return CONFIG_LAYER_LANGUAGE;
    }
    if (strcmp(name, ".polycallrc") == 0) {
        return CONFIG_LAYER_LANGUAGE;
    }
    return CONFIG_LAYER_RUNTIME;
}

static int validate_one(const char* path) {
    PolycallConfig config = {0};
    const char* language = NULL;
    ConfigLayer layer = infer_layer(path, &language);

    if (!parse_config_file(&config, path, layer) ||
        !validate_required(
            &config,
            layer == CONFIG_LAYER_PROJECT,
            language
        )) {
        return 1;
    }

    printf("%s is valid", path);
    if (config.warnings) {
        printf(" with %u warning(s)", config.warnings);
    }
    printf("\n");
    return 0;
}

static int load_and_report(const char* language, bool show) {
    PolycallConfig config = {0};
    LoadedSources sources = {0};
    size_t i;

    if (!load_hierarchy(&config, language, &sources)) {
        return 1;
    }

    if (show) {
        show_config(&config);
    } else {
        printf("Loaded configuration in order:\n");
        for (i = 0; i < sources.count; i++) {
            printf("%u. %s\n", (unsigned int)(i + 1), sources.paths[i]);
        }
        printf("Configuration is valid");
        if (config.warnings) {
            printf(" with %u warning(s)", config.warnings);
        }
        printf("\n");
    }
    return 0;
}

static int show_rc(const char* language, bool validate_only) {
    PolycallConfig config = {0};
    char path[POLYCALL_CONFIG_PATH_SIZE];
    const char* selected_path;

    if (!valid_language(language)) {
        cfg_errf("Invalid language name: %s\n", language ? language : "");
        return 1;
    }

    snprintf(path, sizeof(path), "Polycallrc.%s", language);
    selected_path = path;
    if (!file_exists(selected_path)) {
        if (!file_exists(".polycallrc")) {
            cfg_errf("Missing runtime override: %s\n", path);
            return 1;
        }
        selected_path = ".polycallrc";
        cfg_errf("warning: Legacy .polycallrc detected. "
            "Use Polycallrc.<language> for v1.0.0.\n"
        );
    }

    if (!parse_config_file(&config, selected_path, CONFIG_LAYER_LANGUAGE) ||
        !validate_required(&config, false, language)) {
        return 1;
    }

    if (validate_only) {
        printf("%s is valid for %s\n", selected_path, language);
    } else {
        show_config(&config);
    }
    return 0;
}

static int migrate_config(const char* source, const char* destination) {
    PolycallConfig config = {0};
    const char* language = NULL;
    ConfigLayer destination_layer;
    FILE* input;
    FILE* output;
    char buffer[4096];
    size_t read_count;

    if (file_exists(destination)) {
        cfg_errf("Refusing to overwrite existing file: %s\n", destination);
        return 1;
    }

    destination_layer = infer_layer(destination, &language);
    if (destination_layer != CONFIG_LAYER_LANGUAGE || !language ||
        !valid_language(language)) {
        cfg_errf("Migration destination must be Polycallrc.<language>\n"
        );
        return 1;
    }

    if (!parse_config_file(&config, source, CONFIG_LAYER_LANGUAGE) ||
        !validate_required(&config, false, language)) {
        return 1;
    }

    input = pc_fopen(source, "rb");
    if (!input) {
        cfg_errf("Unable to open migration source: %s\n", source);
        return 1;
    }
    output = pc_fopen(destination, "wb");
    if (!output) {
        cfg_errf("Unable to create migration destination: %s\n", destination);
        fclose(input);
        return 1;
    }

    while ((read_count = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        if (fwrite(buffer, 1, read_count, output) != read_count) {
            cfg_errf("Failed to write migration destination: %s\n", destination);
            fclose(input);
            fclose(output);
            remove(destination);
            return 1;
        }
    }

    if (ferror(input) || fclose(output) != 0) {
        cfg_errf("Failed to migrate configuration to: %s\n", destination);
        fclose(input);
        remove(destination);
        return 1;
    }
    fclose(input);

    printf("Migrated %s to %s\n", source, destination);
    return 0;
}

static void print_config_usage(void) {
    printf("PolyCall configuration commands:\n");
    printf("  polycall config load [language]\n");
    printf("  polycall config validate [path]\n");
    printf("  polycall config show [language]\n");
    printf("  polycall config rc show <language>\n");
    printf("  polycall config rc validate <language>\n");
    printf("  polycall config migrate <source> <Polycallrc.language>\n");
}

int polycall_config_cli(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "config") != 0) {
        return -1;
    }

    if (argc < 3) {
        print_config_usage();
        return 1;
    }

    if (strcmp(argv[2], "load") == 0) {
        if (argc > 4) {
            print_config_usage();
            return 1;
        }
        return load_and_report(argc == 4 ? argv[3] : NULL, false);
    }

    if (strcmp(argv[2], "validate") == 0) {
        if (argc == 3) {
            return load_and_report(NULL, false);
        }
        if (argc == 4) {
            return validate_one(argv[3]);
        }
        print_config_usage();
        return 1;
    }

    if (strcmp(argv[2], "show") == 0) {
        if (argc > 4) {
            print_config_usage();
            return 1;
        }
        return load_and_report(argc == 4 ? argv[3] : NULL, true);
    }

    if (strcmp(argv[2], "migrate") == 0 && argc == 5) {
        return migrate_config(argv[3], argv[4]);
    }

    if (strcmp(argv[2], "rc") == 0) {
        if (argc == 6 && strcmp(argv[3], "migrate") == 0) {
            return migrate_config(argv[4], argv[5]);
        }
        if (argc != 5) {
            print_config_usage();
            return 1;
        }
        if (strcmp(argv[3], "show") == 0) {
            return show_rc(argv[4], false);
        }
        if (strcmp(argv[3], "validate") == 0) {
            return show_rc(argv[4], true);
        }
    }

    print_config_usage();
    return 1;
}

/* ====================================================================== */
/* library entry: load one file with the same grammar the CLI uses         */
/* ====================================================================== */

struct polycall_cfgfile {
    PolycallConfig config;
    int layer;          /* ConfigLayer */
    char language[64];
};

polycall_cfgfile_t *polycall_cfgfile_load(const char *path, unsigned flags,
                                          int *status, char *err, size_t errcap)
{
    polycall_cfgfile_t *cf;
    const char *language = NULL;
    ConfigLayer layer;
    char local_err[1] = { 0 };
    char *prev_cap = g_cfg_capture;
    size_t prev_cap_len = g_cfg_capture_cap;
    bool ok;
    FILE *probe;

    if (status) *status = POLYCALL_CFGFILE_OK;
    if (err && errcap) err[0] = '\0';
    if (!path || !*path) {
        if (status) *status = POLYCALL_CFGFILE_INVALID_ARGUMENT;
        if (err && errcap) snprintf(err, errcap, "no configuration path given");
        return NULL;
    }
    probe = pc_fopen(path, "rb");
    if (!probe) {
        if (status) *status = POLYCALL_CFGFILE_NOT_FOUND;
        if (err && errcap) {
            snprintf(err, errcap, "cannot open configuration file '%s': %s",
                     path, strerror(errno));
        }
        return NULL;
    }
    fclose(probe);

    cf = calloc(1, sizeof *cf);   /* ~50 KiB: keep it off small thread stacks */
    if (!cf) {
        if (status) *status = POLYCALL_CFGFILE_NO_MEMORY;
        if (err && errcap) snprintf(err, errcap, "out of memory");
        return NULL;
    }

    layer = infer_layer(path, &language);
    cf->layer = (int)layer;
    if (language) {
        snprintf(cf->language, sizeof cf->language, "%s", language);
    }

    g_cfg_capture = (err && errcap) ? err : local_err;
    g_cfg_capture_cap = (err && errcap) ? errcap : sizeof local_err;
    ok = parse_config_file(&cf->config, path, layer);
    if (ok && (flags & POLYCALL_CFGFILE_LEGACY_REQUIRED)) {
        ok = validate_required(&cf->config, layer == CONFIG_LAYER_PROJECT,
                               language);
    } else if (ok) {
        ok = validate_required(&cf->config, false, NULL);
    }
    g_cfg_capture = prev_cap;
    g_cfg_capture_cap = prev_cap_len;

    if (ok && (flags & POLYCALL_CFGFILE_STRICT) && cf->config.warnings) {
        ok = false;
        if (err && errcap && err[0] == '\0') {
            snprintf(err, errcap,
                     "%s: %u unknown key(s); strict validation treats them as errors",
                     path, cf->config.warnings);
        }
    }
    if (!ok) {
        if (status) *status = POLYCALL_CFGFILE_INVALID;
        free(cf);
        return NULL;
    }
    return cf;
}

void polycall_cfgfile_free(polycall_cfgfile_t *cf)
{
    free(cf);
}

const char *polycall_cfgfile_get(const polycall_cfgfile_t *cf, const char *key)
{
    return (cf && key) ? config_get(&cf->config, key) : NULL;
}

unsigned polycall_cfgfile_warnings(const polycall_cfgfile_t *cf)
{
    return cf ? cf->config.warnings : 0;
}

int polycall_cfgfile_is_project(const polycall_cfgfile_t *cf)
{
    return cf && cf->layer == CONFIG_LAYER_PROJECT;
}

size_t polycall_cfgfile_peer_count(const polycall_cfgfile_t *cf)
{
    return cf ? cf->config.peer_count : 0;
}

int polycall_cfgfile_peer_at(const polycall_cfgfile_t *cf, size_t i,
                             const char **id, const char **endpoint)
{
    if (!cf || i >= cf->config.peer_count) return -1;
    if (id) *id = cf->config.peers[i].id;
    if (endpoint) *endpoint = cf->config.peers[i].endpoint;
    return 0;
}

int polycall_cfgfile_network(const polycall_cfgfile_t *cf)
{
    if (!cf || !cf->config.network_seen) return -1;
    return cf->config.network_enabled ? 1 : 0;
}

int polycall_cfgfile_describe(const polycall_cfgfile_t *cf, pc_buf_t *out)
{
    size_t i;
    static const char *const layer_names[] = { "project", "runtime", "language" };
    if (!cf || !out) return -1;
    pc_buf_append(out, "{\"layer\":");
    pc_buf_json_string(out, layer_names[(cf->layer >= 0 && cf->layer <= 2) ? cf->layer : 1]);
    if (cf->language[0]) {
        pc_buf_append(out, ",\"language\":");
        pc_buf_json_string(out, cf->language);
    }
    pc_buf_append(out, ",\"network\":");
    pc_buf_append(out, !cf->config.network_seen ? "null"
                       : cf->config.network_enabled ? "\"start\"" : "\"stop\"");
    pc_buf_append(out, ",\"servers\":[");
    for (i = 0; i < cf->config.server_count; ++i) {
        pc_buf_append(out, i ? ",{\"language\":" : "{\"language\":");
        pc_buf_json_string(out, cf->config.servers[i].language);
        pc_buf_appendf(out, ",\"host_port\":%u,\"target_port\":%u}",
                       cf->config.servers[i].host_port,
                       cf->config.servers[i].target_port);
    }
    pc_buf_append(out, "],\"peers\":{");
    for (i = 0; i < cf->config.peer_count; ++i) {
        if (i) pc_buf_append(out, ",");
        pc_buf_json_string(out, cf->config.peers[i].id);
        pc_buf_append(out, ":");
        pc_buf_json_string(out, cf->config.peers[i].endpoint);
    }
    pc_buf_append(out, "},\"values\":{");
    for (i = 0; i < cf->config.value_count; ++i) {
        if (i) pc_buf_append(out, ",");
        pc_buf_json_string(out, cf->config.values[i].key);
        pc_buf_append(out, ":");
        pc_buf_json_string(out, cf->config.values[i].value);
    }
    pc_buf_appendf(out, "},\"warnings\":%u}", cf->config.warnings);
    return pc_buf_failed(out) ? -1 : 0;
}
