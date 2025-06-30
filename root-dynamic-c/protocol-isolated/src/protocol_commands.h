/**
 * @file protocol_commands.c - Hotwiring Protocol Map Interface Contract
 * @brief LibPolyCall v2 Protocol Commands Integration for Hotwiring Architecture
 * @version 2.0.0
 * @author OBINexus Computing - OpenACE Division
 * 
 * Constitutional interface contract exposing hotwire_protocol_map() through
 * the core protocol layer with backward compatibility guarantees
 */

#include "core/protocol/protocol_commands.h"
#include "core/hotwire/hotwire_router.h"
#include "core/hotwire/config_binding.h"
#include "telemetry/telemetry_commands.h"
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------------------------*/
/* Protocol Map Interface Constants */
/*---------------------------------------------------------------------------*/

#define HOTWIRE_PROTOCOL_MAP_VERSION "2.0.0"
#define HOTWIRE_PROTOCOL_MAP_SIGNATURE 0x48504D50  // "HPMP"
#define MAX_PROTOCOL_MAP_ENTRIES 256
#define MAX_PROTOCOL_MAP_DEPTH 8

/*---------------------------------------------------------------------------*/
/* Protocol Map Entry Structure */
/*---------------------------------------------------------------------------*/

typedef struct hotwire_protocol_map_entry {
    char source_protocol[64];
    char target_protocol[64];
    char binding_interface[128];
    uint32_t flags;
    polycall_protocol_handler_t handler;
    polycall_protocol_fallback_t v1_fallback;
    void* private_data;
    uint64_t registration_timestamp;
} hotwire_protocol_map_entry_t;

/*---------------------------------------------------------------------------*/
/* Protocol Map Context */
/*---------------------------------------------------------------------------*/

typedef struct hotwire_protocol_map_context {
    uint32_t signature;
    size_t entry_count;
    hotwire_protocol_map_entry_t entries[MAX_PROTOCOL_MAP_ENTRIES];
    polycall_protocol_context_t* protocol_ctx;
    bool v1_compatibility_enabled;
    bool constitutional_mode_enabled;
    void* telemetry_ctx;
} hotwire_protocol_map_context_t;

/*---------------------------------------------------------------------------*/
/* Global Protocol Map Context */
/*---------------------------------------------------------------------------*/

static hotwire_protocol_map_context_t* g_protocol_map_ctx = NULL;

/*---------------------------------------------------------------------------*/
/* Constitutional Interface Definition */
/*---------------------------------------------------------------------------*/

/**
 * @brief Protocol map interface structure for constitutional compliance
 */
static const polycall_protocol_enhancement_interface_t hotwire_protocol_map_interface = {
    .name = "hotwire_protocol_map",
    .version = HOTWIRE_PROTOCOL_MAP_VERSION,
    .init = hotwire_protocol_map_init,
    .cleanup = hotwire_protocol_map_cleanup,
    .execute = hotwire_protocol_map_execute,
    .fallback = hotwire_protocol_v1_fallback,
    .validate = hotwire_protocol_map_validate,
    .get_stats = hotwire_protocol_map_get_stats
};

/*---------------------------------------------------------------------------*/
/* Core Protocol Map Implementation */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize hotwiring protocol map subsystem
 * @param protocol_ctx Protocol layer context
 * @param config Hotwiring configuration (optional)
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_protocol_map_init(
    polycall_protocol_context_t* protocol_ctx,
    const hotwire_config_t* config
) {
    if (!protocol_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    // Allocate protocol map context
    g_protocol_map_ctx = polycall_core_alloc(
        protocol_ctx->core_ctx, 
        sizeof(hotwire_protocol_map_context_t)
    );
    if (!g_protocol_map_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    // Initialize context
    memset(g_protocol_map_ctx, 0, sizeof(hotwire_protocol_map_context_t));
    g_protocol_map_ctx->signature = HOTWIRE_PROTOCOL_MAP_SIGNATURE;
    g_protocol_map_ctx->protocol_ctx = protocol_ctx;
    g_protocol_map_ctx->v1_compatibility_enabled = true;
    
    // Enable constitutional mode if requested
    if (config && config->enable_constitutional_mode) {
        g_protocol_map_ctx->constitutional_mode_enabled = true;
        
        // Initialize telemetry for constitutional audit
        polycall_core_error_t telemetry_result = telemetry_commands_init(
            protocol_ctx->core_ctx,
            &g_protocol_map_ctx->telemetry_ctx
        );
        if (telemetry_result != POLYCALL_CORE_SUCCESS) {
            printf("[PROTOCOL_MAP] Warning: Constitutional telemetry initialization failed\n");
        }
    }

    // Register core v1 compatibility handlers
    polycall_core_error_t compat_result = hotwire_register_v1_compatibility_handlers();
    if (compat_result != POLYCALL_CORE_SUCCESS) {
        hotwire_protocol_map_cleanup();
        return compat_result;
    }

    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup hotwiring protocol map subsystem
 */
void hotwire_protocol_map_cleanup(void) {
    if (!g_protocol_map_ctx) {
        return;
    }

    // Validate context integrity
    if (g_protocol_map_ctx->signature != HOTWIRE_PROTOCOL_MAP_SIGNATURE) {
        printf("[PROTOCOL_MAP] Critical: Context corruption detected\n");
        return;
    }

    // Cleanup telemetry if enabled
    if (g_protocol_map_ctx->telemetry_ctx) {
        telemetry_commands_cleanup(
            g_protocol_map_ctx->protocol_ctx->core_ctx,
            g_protocol_map_ctx->telemetry_ctx
        );
    }

    // Free private data for all entries
    for (size_t i = 0; i < g_protocol_map_ctx->entry_count; i++) {
        if (g_protocol_map_ctx->entries[i].private_data) {
            polycall_core_free(
                g_protocol_map_ctx->protocol_ctx->core_ctx,
                g_protocol_map_ctx->entries[i].private_data
            );
        }
    }

    // Free context
    polycall_core_free(
        g_protocol_map_ctx->protocol_ctx->core_ctx,
        g_protocol_map_ctx
    );
    g_protocol_map_ctx = NULL;
}

/**
 * @brief Execute protocol mapping with constitutional compliance
 * @param protocol_ctx Protocol layer context
 * @param source_protocol Source protocol identifier
 * @param target_protocol Target protocol identifier
 * @param request Input request data
 * @param response Output response buffer
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_protocol_map_execute(
    polycall_protocol_context_t* protocol_ctx,
    const char* source_protocol,
    const char* target_protocol,
    const polycall_request_t* request,
    polycall_response_t* response
) {
    if (!g_protocol_map_ctx || !source_protocol || !target_protocol || 
        !request || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    // Constitutional audit if enabled
    if (g_protocol_map_ctx->constitutional_mode_enabled) {
        char audit_entry[256];
        snprintf(audit_entry, sizeof(audit_entry),
            "PROTOCOL_MAP_EXECUTE: %s -> %s", source_protocol, target_protocol);
        telemetry_commands_log_audit(g_protocol_map_ctx->telemetry_ctx, audit_entry);
    }

    // Find protocol mapping entry
    hotwire_protocol_map_entry_t* entry = hotwire_find_protocol_mapping(
        source_protocol, target_protocol
    );
    
    if (!entry) {
        // Check if this is a v1 protocol that needs compatibility handling
        if (g_protocol_map_ctx->v1_compatibility_enabled) {
            return hotwire_protocol_v1_fallback(
                protocol_ctx, source_protocol, request, response
            );
        }
        return POLYCALL_CORE_ERROR_NOT_FOUND;
    }

    // Execute mapped protocol handler
    polycall_core_error_t exec_result = entry->handler(
        protocol_ctx, request, response, entry->private_data
    );

    // If execution failed and v1 fallback is available, try fallback
    if (exec_result != POLYCALL_CORE_SUCCESS && entry->v1_fallback) {
        exec_result = entry->v1_fallback(
            protocol_ctx, source_protocol, request, response
        );
    }

    // Update execution metrics
    entry->registration_timestamp = polycall_core_get_timestamp();

    return exec_result;
}

/**
 * @brief Register new protocol mapping
 * @param source_protocol Source protocol identifier
 * @param target_protocol Target protocol identifier
 * @param handler Protocol handler function
 * @param v1_fallback V1 compatibility fallback handler (optional)
 * @param private_data Private handler data (optional)
 * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
 */
polycall_core_error_t hotwire_protocol_map_register(
    const char* source_protocol,
    const char* target_protocol,
    polycall_protocol_handler_t handler,
    polycall_protocol_fallback_t v1_fallback,
    void* private_data
) {
    if (!g_protocol_map_ctx || !source_protocol || !target_protocol || !handler) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    // Check capacity
    if (g_protocol_map_ctx->entry_count >= MAX_PROTOCOL_MAP_ENTRIES) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    // Validate protocols don't conflict with existing entries
    for (size_t i = 0; i < g_protocol_map_ctx->entry_count; i++) {
        hotwire_protocol_map_entry_t* existing = &g_protocol_map_ctx->entries[i];
        if (strcmp(existing->source_protocol, source_protocol) == 0 &&
            strcmp(existing->target_protocol, target_protocol) == 0) {
            return POLYCALL_CORE_ERROR_ALREADY_EXISTS;
        }
    }

    // Create new entry
    hotwire_protocol_map_entry_t* entry = 
        &g_protocol_map_ctx->entries[g_protocol_map_ctx->entry_count];
    
    strncpy(entry->source_protocol, source_protocol, sizeof(entry->source_protocol) - 1);
    strncpy(entry->target_protocol, target_protocol, sizeof(entry->target_protocol) - 1);
    entry->handler = handler;
    entry->v1_fallback = v1_fallback;
    entry->private_data = private_data;
    entry->registration_timestamp = polycall_core_get_timestamp();
    entry->flags = 0;

    // Generate binding interface description
    snprintf(entry->binding_interface, sizeof(entry->binding_interface),
        "hotwire://%s/%s", source_protocol, target_protocol);

    g_protocol_map_ctx->entry_count++;

    // Constitutional audit if enabled
    if (g_protocol_map_ctx->constitutional_mode_enabled) {
        char audit_entry[256];
        snprintf(audit_entry, sizeof(audit_entry),
            "PROTOCOL_MAP_REGISTER: %s -> %s [%s]", 
            source_protocol, target_protocol, entry->binding_interface);
        telemetry_commands_log_audit(g_protocol_map_ctx->telemetry_ctx, audit_entry);
    }

    return POLYCALL_CORE_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/* V1 Compatibility Implementation */
/*---------------------------------------------------------------------------*/

/**
 * @brief V1 protocol fallback handler for constitutional compliance
 */
polycall_core_error_t hotwire_protocol_v1_fallback(
    polycall_protocol_context_t* protocol_ctx,
    const char* protocol_name,
    const polycall_request_t* request,
    polycall_response_t* response
) {
    if (!protocol_ctx || !protocol_name || !request || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    // Constitutional audit for v1 fallback usage
    if (g_protocol_map_ctx && g_protocol_map_ctx->constitutional_mode_enabled) {
        char audit_entry[256];
        snprintf(audit_entry, sizeof(audit_entry),
            "V1_FALLBACK_INVOKED: %s", protocol_name);
        telemetry_commands_log_audit(g_protocol_map_ctx->telemetry_ctx, audit_entry);
    }

    // Check for known v1 protocol patterns
    if (strncmp(protocol_name, "polycall.v1.", 12) == 0) {
        // Delegate to legacy v1 handler
        return polycall_v1_protocol_execute(protocol_ctx, protocol_name, request, response);
    }

    // If no v1 handler found, return graceful error
    response->error_code = POLYCALL_CORE_ERROR_PROTOCOL_NOT_SUPPORTED;
    snprintf(response->error_message, sizeof(response->error_message),
        "Protocol '%s' not supported in v1 compatibility mode", protocol_name);

    return POLYCALL_CORE_ERROR_PROTOCOL_NOT_SUPPORTED;
}

/**
 * @brief Register core v1 compatibility handlers
 */
static polycall_core_error_t hotwire_register_v1_compatibility_handlers(void) {
    polycall_core_error_t result;

    // Register core v1 protocol mappings
    result = hotwire_protocol_map_register(
        "polycall.v1.core",
        "polycall.v2.core",
        hotwire_v1_core_handler,
        hotwire_v1_core_fallback,
        NULL
    );
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }

    // Register command v1 protocol mappings
    result = hotwire_protocol_map_register(
        "polycall.v1.command",
        "polycall.v2.command",
        hotwire_v1_command_handler,
        hotwire_v1_command_fallback,
        NULL
    );
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }

    // Register binding v1 protocol mappings
    result = hotwire_protocol_map_register(
        "polycall.v1.binding",
        "polycall.v2.binding",
        hotwire_v1_binding_handler,
        hotwire_v1_binding_fallback,
        NULL
    );

    return result;
}

/*---------------------------------------------------------------------------*/
/* Protocol Map Utilities */
/*---------------------------------------------------------------------------*/

/**
 * @brief Find protocol mapping entry by source and target
 */
static hotwire_protocol_map_entry_t* hotwire_find_protocol_mapping(
    const char* source_protocol,
    const char* target_protocol
) {
    if (!g_protocol_map_ctx) {
        return NULL;
    }

    for (size_t i = 0; i < g_protocol_map_ctx->entry_count; i++) {
        hotwire_protocol_map_entry_t* entry = &g_protocol_map_ctx->entries[i];
        if (strcmp(entry->source_protocol, source_protocol) == 0 &&
            strcmp(entry->target_protocol, target_protocol) == 0) {
            return entry;
        }
    }

    return NULL;
}

/**
 * @brief Validate protocol map configuration
 */
polycall_core_error_t hotwire_protocol_map_validate(
    const hotwire_config_t* config
) {
    if (!config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    // Validate constitutional compliance
    if (config->enable_constitutional_mode) {
        // Ensure audit is enabled for constitutional mode
        if (!config->enable_audit) {
            return POLYCALL_CORE_ERROR_INVALID_CONFIGURATION;
        }

        // Validate security configuration
        if (!config->security.enable_zero_trust) {
            return POLYCALL_CORE_ERROR_SECURITY_VIOLATION;
        }
    }

    // Validate route configurations
    for (size_t i = 0; i < config->route_count; i++) {
        const hotwire_route_config_t* route = &config->routes[i];
        
        // Check for empty protocol names
        if (strlen(route->source_protocol) == 0 || 
            strlen(route->target_protocol) == 0) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
        }

        // Validate compatibility mode settings
        if (route->compatibility_mode == HOTWIRE_COMPAT_V1_STRICT &&
            !config->enable_v1_compatibility) {
            return POLYCALL_CORE_ERROR_CONFIGURATION_MISMATCH;
        }
    }

    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get protocol map statistics
 */
polycall_core_error_t hotwire_protocol_map_get_stats(
    hotwire_protocol_map_stats_t* stats
) {
    if (!stats || !g_protocol_map_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    memset(stats, 0, sizeof(hotwire_protocol_map_stats_t));
    stats->total_mappings = g_protocol_map_ctx->entry_count;
    stats->v1_compatibility_enabled = g_protocol_map_ctx->v1_compatibility_enabled;
    stats->constitutional_mode_enabled = g_protocol_map_ctx->constitutional_mode_enabled;

    return POLYCALL_CORE_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/* Protocol Map Interface Export */
/*---------------------------------------------------------------------------*/

/**
 * @brief Get hotwiring protocol map interface for registration
 */
const polycall_protocol_enhancement_interface_t* hotwire_get_protocol_map_interface(void) {
    return &hotwire_protocol_map_interface;
}