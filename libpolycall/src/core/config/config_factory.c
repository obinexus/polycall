/**
#include "polycall/core/config/factory/config_factory.h"
#include "polycall/core/config/factory/config_factory.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polycall/core/config/factory/config_factory_mergers.h"
#include "polycall/core/config/polycallfile/parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"

 * @file config_factory.c
 * @brief Unified configuration factory implementation for LibPolyCall
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements a unified configuration factory interface that provides
 * a consistent approach for component configuration creation, loading, and validation
 * across all LibPolyCall components.
 */




// Component-specific includes


/**
 * @brief Configuration factory context structure
 */
struct polycall_config_factory {
    polycall_core_context_t* core_ctx;             // Core context
    polycall_config_parser_t* parser;              // Configuration parser
    polycall_schema_context_t* schema_ctx;         // Schema context
    
    void* global_config;                           // Global configuration
    void* binding_config;                          // Binding configuration
    
    polycall_config_factory_options_t options;     // Factory options
    
    char error_message[512];                       // Last error message
    bool has_error;                                // Error flag
};

/**
 * @brief Initialize configuration factory
 */
polycall_core_error_t polycall_config_factory_init(
    polycall_core_context_t* ctx,
    polycall_config_factory_t** factory,
    const polycall_config_factory_options_t* options
) {
    if (!ctx || !factory) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Use default options if none provided
    polycall_config_factory_options_t default_options;
    if (!options) {
        memset(&default_options, 0, sizeof(default_options));
        default_options.validate_configs = true;
        default_options.apply_environment_vars = true;
        default_options.fallback_to_defaults = true;
        options = &default_options;
    }
    
    // Allocate factory context
    polycall_config_factory_t* new_factory = polycall_core_malloc(ctx, sizeof(polycall_config_factory_t));
    if (!new_factory) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate configuration factory context");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize factory context
    memset(new_factory, 0, sizeof(polycall_config_factory_t));
    new_factory->core_ctx = ctx;
    memcpy(&new_factory->options, options, sizeof(polycall_config_factory_options_t));
    
    // Initialize parser
    polycall_config_parser_options_t parser_options;
    memset(&parser_options, 0, sizeof(parser_options));
    parser_options.apply_environment_vars = options->apply_environment_vars;
    parser_options.trace_changes = true;
    parser_options.case_sensitive = false;
    
    polycall_core_error_t result = polycall_config_parser_init(
        ctx, &new_factory->parser, &parser_options);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_core_free(ctx, new_factory);
        return result;
    }
    
    // Initialize schema context if validation is enabled
    if (options->validate_configs) {
        result = polycall_schema_context_create(
            ctx, &new_factory->schema_ctx, options->strict_validation);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_config_parser_cleanup(ctx, new_factory->parser);
            polycall_core_free(ctx, new_factory);
            return result;
        }
    }
    
    *factory = new_factory;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup configuration factory
 */
void polycall_config_factory_cleanup(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory
) {
    if (!ctx || !factory) {
        return;
    }
    
    // Cleanup schema context
    if (factory->schema_ctx) {
        polycall_schema_context_destroy(ctx, factory->schema_ctx);
    }
    
    // Cleanup parser
    if (factory->parser) {
        polycall_config_parser_cleanup(ctx, factory->parser);
    }
    
    // Cleanup global and binding config
    // Note: These are cleanup by their respective components
    
    // Free factory context
    polycall_core_free(ctx, factory);
}

/**
 * @brief Create component configuration
 */
polycall_core_error_t polycall_config_factory_create_component(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory,
    polycall_component_type_t component_type,
    const char* component_name,
    void** config_out
) {
    if (!ctx || !factory || !component_name || !config_out) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create appropriate configuration based on component type
    switch (component_type) {
        case POLYCALL_COMPONENT_TYPE_MICRO: {
            micro_component_config_t* micro_config = NULL;
            polycall_core_error_t result = micro_config_create_default_component(
                ctx, component_name, &micro_config);
            
            if (result != POLYCALL_CORE_SUCCESS) {
                return result;
            }
            
            *config_out = micro_config;
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_EDGE: {
            polycall_edge_component_config_t* edge_config = polycall_core_malloc(
                ctx, sizeof(polycall_edge_component_config_t));
            
            if (!edge_config) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            
            *edge_config = polycall_edge_component_default_config();
            strncpy(edge_config->name, component_name, sizeof(edge_config->name) - 1);
            
            *config_out = edge_config;
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_NETWORK: {
            polycall_network_config_t* network_config = NULL;
            polycall_core_error_t result = polycall_network_config_create(
                ctx, &network_config, NULL);
            
            if (result != POLYCALL_CORE_SUCCESS) {
                return result;
            }
            
            *config_out = network_config;
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_PROTOCOL: {
            protocol_config_t* protocol_config = polycall_core_malloc(
                ctx, sizeof(protocol_config_t));
            
            if (!protocol_config) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            
            memset(protocol_config, 0, sizeof(protocol_config_t));
            protocol_config->enable_protocol_dispatch = true;
            protocol_config->max_message_size = 1048576; // 1MB
            protocol_config->enable_compression = true;
            protocol_config->enable_batching = true;
            
            *config_out = protocol_config;
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_FFI: {
            polycall_ffi_config_options_t* ffi_config = polycall_core_malloc(
                ctx, sizeof(polycall_ffi_config_options_t));
            
            if (!ffi_config) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            
            *ffi_config = polycall_ffi_config_create_default_options();
            
            *config_out = ffi_config;
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_TELEMETRY: {
            polycall_telemetry_config_t* telemetry_config = polycall_core_malloc(
                ctx, sizeof(polycall_telemetry_config_t));
            
            if (!telemetry_config) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            
            *telemetry_config = polycall_telemetry_config_create_default();
            
            *config_out = telemetry_config;
            break;
        }
        
        default:
            snprintf(factory->error_message, sizeof(factory->error_message),
                    "Unsupported component type: %d", component_type);
            factory->has_error = true;
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Validate configuration if schema context is available
    if (factory->schema_ctx && factory->options.validate_configs) {
        polycall_core_error_t result = polycall_schema_validate_component(
            ctx, factory->schema_ctx, component_type, *config_out,
            factory->error_message, sizeof(factory->error_message));
        
        if (result != POLYCALL_CORE_SUCCESS) {
            factory->has_error = true;
            return result;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Load configuration from Polycallfile
 */
polycall_core_error_t polycall_config_factory_load_from_file(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory,
    const char* file_path,
    polycall_config_load_flags_t flags
) {
    if (!ctx || !factory || !file_path) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Resolve path
    char resolved_path[POLYCALL_MAX_PATH];
    polycall_core_error_t result = polycall_path_resolve(
        ctx, file_path, resolved_path, sizeof(resolved_path));
    
    if (result != POLYCALL_CORE_SUCCESS) {
        snprintf(factory->error_message, sizeof(factory->error_message),
                "Failed to resolve path: %s", file_path);
        factory->has_error = true;
        return result;
    }
    
    // Load configuration
    polycall_config_node_t* config_root = NULL;
    result = polycall_config_parser_parse_file(
        factory->parser, resolved_path, &config_root);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        if (result == POLYCALL_CORE_ERROR_FILE_NOT_FOUND) {
            if (!(flags & CONFIG_LOAD_REQUIRED)) {
                // File is not required, return success
                return POLYCALL_CORE_SUCCESS;
            }
        }
        
        snprintf(factory->error_message, sizeof(factory->error_message),
                "Failed to parse configuration file: %s", file_path);
        factory->has_error = true;
        return result;
    }
    
    // Validate configuration if requested
    if ((flags & CONFIG_LOAD_VALIDATE) && factory->schema_ctx) {
        // TODO: Implement global configuration validation
    }
    
    // Store configuration
    factory->global_config = config_root;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Extract component configuration from global config
 */
polycall_core_error_t polycall_config_factory_extract_component(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory,
    polycall_component_type_t component_type,
    const char* component_name,
    void** config_out
) {
    if (!ctx || !factory || !component_name || !config_out) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if global configuration is loaded
    if (!factory->global_config) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    // Create default component configuration
    polycall_core_error_t result = polycall_config_factory_create_component(
        ctx, factory, component_type, component_name, config_out);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Find component section in global configuration
    const char* section_path = NULL;
    switch (component_type) {
        case POLYCALL_COMPONENT_TYPE_MICRO:
            section_path = "micro.components";
            break;
            
        case POLYCALL_COMPONENT_TYPE_EDGE:
            section_path = "edge.components";
            break;
            
        case POLYCALL_COMPONENT_TYPE_NETWORK:
            section_path = "network";
            break;
            
        case POLYCALL_COMPONENT_TYPE_PROTOCOL:
            section_path = "protocol";
            break;
            
        case POLYCALL_COMPONENT_TYPE_FFI:
            section_path = "ffi";
            break;
            
        case POLYCALL_COMPONENT_TYPE_TELEMETRY:
            section_path = "telemetry";
            break;
            
        default:
            snprintf(factory->error_message, sizeof(factory->error_message),
                    "Unsupported component type: %d", component_type);
            factory->has_error = true;
            
            // Free allocated config
            polycall_config_factory_free_component(ctx, factory, component_type, *config_out);
            *config_out = NULL;
            
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find component section
    polycall_config_node_t* components_section = polycall_config_find_node(
        factory->global_config, section_path);
    
    if (!components_section) {
        // Section not found, use default configuration
        return POLYCALL_CORE_SUCCESS;
    }
    
    // If component name is specified, find component-specific section
    if (component_name[0] != '\0' && 
        (component_type == POLYCALL_COMPONENT_TYPE_MICRO || 
         component_type == POLYCALL_COMPONENT_TYPE_EDGE)) {
        
        char component_path[256];
        snprintf(component_path, sizeof(component_path), 
                "%s.%s", section_path, component_name);
        
        polycall_config_node_t* component_section = polycall_config_find_node(
            factory->global_config, component_path);
        
        if (!component_section) {
            // Component-specific section not found, use default configuration
            return POLYCALL_CORE_SUCCESS;
        }
        
        // Merge component-specific configuration
        result = polycall_config_factory_merge_component(
            ctx, factory, component_type, *config_out, component_section);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            // Free allocated config
            polycall_config_factory_free_component(ctx, factory, component_type, *config_out);
            *config_out = NULL;
            
            return result;
        }
    } else {
        // Merge component type configuration
        result = polycall_config_factory_merge_component(
            ctx, factory, component_type, *config_out, components_section);
        
        if (result != POLYCALL_CORE_SUCCESS) {
            // Free allocated config
            polycall_config_factory_free_component(ctx, factory, component_type, *config_out);
            *config_out = NULL;
            
            return result;
        }
    }
    
    // Validate merged configuration if schema context is available
    if (factory->schema_ctx && factory->options.validate_configs) {
        result = polycall_schema_validate_component(
            ctx, factory->schema_ctx, component_type, *config_out,
            factory->error_message, sizeof(factory->error_message));
        
        if (result != POLYCALL_CORE_SUCCESS) {
            factory->has_error = true;
            
            // Free allocated config
            polycall_config_factory_free_component(ctx, factory, component_type, *config_out);
            *config_out = NULL;
            
            return result;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Free component configuration
 */
void polycall_config_factory_free_component(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory,
    polycall_component_type_t component_type,
    void* component_config
) {
    if (!ctx || !factory || !component_config) {
        return;
    }
    
    switch (component_type) {
        case POLYCALL_COMPONENT_TYPE_MICRO: {
            // Free micro component config
            micro_component_config_t* micro_config = (micro_component_config_t*)component_config;
            
            // Free allowed connections
            for (size_t i = 0; i < micro_config->allowed_connections_count; i++) {
                if (micro_config->allowed_connections[i]) {
                    polycall_core_free(ctx, micro_config->allowed_connections[i]);
                }
            }
            
            // Free config structure
            polycall_core_free(ctx, micro_config);
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_EDGE: {
            // Free edge component config
            polycall_edge_component_config_t* edge_config = 
                (polycall_edge_component_config_t*)component_config;
            
            // Free component name if dynamically allocated
            if (edge_config->component_name) {
                polycall_core_free(ctx, edge_config->component_name);
            }
            
            // Free component ID if dynamically allocated
            if (edge_config->component_id) {
                polycall_core_free(ctx, edge_config->component_id);
            }
            
            // Free log path if dynamically allocated
            if (edge_config->log_path) {
                polycall_core_free(ctx, edge_config->log_path);
            }
            
            // Free config structure
            polycall_core_free(ctx, edge_config);
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_NETWORK: {
            // Free network config
            polycall_network_config_t* network_config = 
                (polycall_network_config_t*)component_config;
            
            // Use provided cleanup function
            polycall_network_config_destroy(ctx, network_config);
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_PROTOCOL: {
            // Free protocol config
            protocol_config_t* protocol_config = 
                (protocol_config_t*)component_config;
            
            // Free config structure
            polycall_core_free(ctx, protocol_config);
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_FFI: {
            // Free FFI config
            polycall_ffi_config_options_t* ffi_config = 
                (polycall_ffi_config_options_t*)component_config;
            
            // Free dynamically allocated strings
            if (ffi_config->config_file_path) {
                free((void*)ffi_config->config_file_path); // Note: Allocated with strdup
            }
            
            if (ffi_config->provider_name) {
                free((void*)ffi_config->provider_name); // Note: Allocated with strdup
            }
            
            // Free config structure
            polycall_core_free(ctx, ffi_config);
            break;
        }
        
        case POLYCALL_COMPONENT_TYPE_TELEMETRY: {
            // Free telemetry config
            polycall_core_free(ctx, component_config);
            break;
        }
        
        default:
            // Unknown component type, just free the memory
            polycall_core_free(ctx, component_config);
            break;
    }
}

/**
 * @brief Get last error message
 */
const char* polycall_config_factory_get_error(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory
) {
    if (!ctx || !factory || !factory->has_error) {
        return NULL;
    }
    
    return factory->error_message;
}

/**
 * @brief Clear error state
 */
void polycall_config_factory_clear_error(
    polycall_core_context_t* ctx,
    polycall_config_factory_t* factory
) {
    if (!ctx || !factory) {
        return;
    }
    
    factory->has_error = false;
    factory->error_message[0] = '\0';
}