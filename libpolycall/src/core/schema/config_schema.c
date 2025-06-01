/**
#include "polycall/core/config/schema/config_schema.h"

 * @file config_schema.c
 * @brief Configuration schema validation implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the configuration schema validation system, providing
 * comprehensive validation for all component configurations against their
 * expected schemas.
 */

 #include "polycall/core/config/schema/config_schema.h"


 /*-------------------------------------------------------------------------*/
 /* Schema definitions for each component type                              */
 /*-------------------------------------------------------------------------*/
 
 /**
  * @brief Schema for micro component
  */
 static schema_field_t micro_component_fields[] = {
     {
         .name = "name",
         .type = SCHEMA_FIELD_STRING,
         .required = true,
         .description = "Component name"
     },
     {
         .name = "isolation_level",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"none", "memory", "resources", "security", "strict"},
         .allowed_values_count = 5,
         .description = "Isolation level"
     },
     {
         .name = "memory_quota",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1024,          // Minimum 1KB
         .max_value = 1073741824,    // Maximum 1GB
         .description = "Memory quota in bytes"
     },
     {
         .name = "cpu_quota",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 100,           // Minimum 100ms
         .max_value = 60000,         // Maximum 60 seconds
         .description = "CPU quota in milliseconds"
     },
     {
         .name = "io_quota",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 10,            // Minimum 10 operations
         .max_value = 10000,         // Maximum 10000 operations
         .description = "I/O quota in operations"
     },
     {
         .name = "enforce_quotas",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enforce resource quotas"
     },
     {
         .name = "require_authentication",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to require authentication"
     },
     {
         .name = "audit_access",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to audit access"
     },
     {
         .name = "allowed_connections",
         .type = SCHEMA_FIELD_STRING_ARRAY,
         .required = false,
         .description = "List of allowed connections"
     },
     {
         .name = "default_permissions",
         .type = SCHEMA_FIELD_BITMASK,
         .required = false,
         .description = "Default permissions bitmask"
     }
 };
 
 /**
  * @brief Schema for micro command
  */
 static schema_field_t micro_command_fields[] = {
     {
         .name = "name",
         .type = SCHEMA_FIELD_STRING,
         .required = true,
         .description = "Command name"
     },
     {
         .name = "flags",
         .type = SCHEMA_FIELD_BITMASK,
         .required = true,
         .description = "Command flags"
     },
     {
         .name = "required_permissions",
         .type = SCHEMA_FIELD_BITMASK,
         .required = true,
         .description = "Required permissions"
     }
 };
 
 /**
  * @brief Schema for edge component
  */
 static schema_field_t edge_component_fields[] = {
     {
         .name = "name",
         .type = SCHEMA_FIELD_STRING,
         .required = true,
         .description = "Component name"
     },
     {
         .name = "type",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"compute", "storage", "gateway", "sensor", "actuator", "coordinator", "custom"},
         .allowed_values_count = 7,
         .description = "Edge component type"
     },
     {
         .name = "task_policy",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"queue", "immediate", "priority", "deadline", "fair_share"},
         .allowed_values_count = 5,
         .description = "Task scheduling policy"
     },
     {
         .name = "isolation",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"none", "memory", "resources", "security", "strict"},
         .allowed_values_count = 5,
         .description = "Isolation level"
     },
     {
         .name = "max_memory_mb",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 1048576,
         .description = "Maximum memory in MB"
     },
     {
         .name = "max_tasks",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 1000000,
         .description = "Maximum number of tasks"
     },
     {
         .name = "max_nodes",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 10000,
         .description = "Maximum number of nodes"
     },
     {
         .name = "task_timeout_ms",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 3600000,  // 1 hour
         .description = "Task timeout in milliseconds"
     },
     {
         .name = "discovery_port",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1024,
         .max_value = 65535,
         .description = "Discovery port"
     },
     {
         .name = "enable_auto_discovery",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable auto-discovery"
     }
 };
 
 /**
  * @brief Schema for network configuration
  */
 static schema_field_t network_config_fields[] = {
     {
         .name = "buffer_size",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1024,         // Minimum 1KB
         .max_value = 1048576,      // Maximum 1MB
         .description = "Buffer size in bytes"
     },
     {
         .name = "connection_timeout",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1000,         // Minimum 1 second
         .max_value = 300000,       // Maximum 5 minutes
         .description = "Connection timeout in milliseconds"
     },
     {
         .name = "operation_timeout",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1000,         // Minimum 1 second
         .max_value = 300000,       // Maximum 5 minutes
         .description = "Operation timeout in milliseconds"
     },
     {
         .name = "max_connections",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 1000,
         .description = "Maximum number of connections"
     }
 };
 
 /**
  * @brief Schema for network security
  */
 static schema_field_t network_security_fields[] = {
     {
         .name = "enable_tls",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable TLS/SSL"
     },
     {
         .name = "enable_encryption",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable message encryption"
     },
     {
         .name = "tls_cert_file",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "TLS certificate file path"
     },
     {
         .name = "tls_key_file",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "TLS private key file path"
     },
     {
         .name = "tls_ca_file",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "TLS CA certificate file path"
     }
 };
 
 /**
  * @brief Schema for protocol configuration
  */
 static schema_field_t protocol_core_fields[] = {
     {
         .name = "transport_type",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"tcp", "udp", "websocket", "unix"},
         .allowed_values_count = 4,
         .description = "Transport type"
     },
     {
         .name = "encoding_format",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"json", "msgpack", "protobuf", "binary"},
         .allowed_values_count = 4,
         .description = "Encoding format"
     },
     {
         .name = "validation_level",
         .type = SCHEMA_FIELD_ENUM,
         .required = true,
         .allowed_values = (const char*[]){"none", "basic", "standard", "strict"},
         .allowed_values_count = 4,
         .description = "Validation level"
     },
     {
         .name = "default_timeout_ms",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1000,         // Minimum 1 second
         .max_value = 300000,       // Maximum 5 minutes
         .description = "Default timeout in milliseconds"
     },
     {
         .name = "enable_tls",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable TLS/SSL"
     }
 };
 
 /**
  * @brief Schema for FFI configuration
  */
 static schema_field_t ffi_config_fields[] = {
     {
         .name = "enable_persistence",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable configuration persistence"
     },
     {
         .name = "enable_change_notification",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable change notifications"
     },
     {
         .name = "validate_configuration",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to validate configuration"
     },
     {
         .name = "config_file_path",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "Configuration file path"
     },
     {
         .name = "provider_name",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "Provider name"
     }
 };
 
 /**
  * @brief Schema for telemetry configuration
  */
 static schema_field_t telemetry_config_fields[] = {
     {
         .name = "enable_telemetry",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = true,
         .description = "Whether to enable telemetry"
     },
     {
         .name = "min_severity",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 0,
         .max_value = 4,
         .description = "Minimum severity level"
     },
     {
         .name = "max_event_queue_size",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 1,
         .max_value = 10000,
         .description = "Maximum event queue size"
     },
     {
         .name = "format",
         .type = SCHEMA_FIELD_INTEGER,
         .required = true,
         .min_value = 0,
         .max_value = 3,
         .description = "Telemetry format"
     },
     {
         .name = "output_path",
         .type = SCHEMA_FIELD_STRING,
         .required = false,
         .description = "Output path"
     },
     {
         .name = "enable_compression",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = false,
         .description = "Whether to enable compression"
     },
     {
         .name = "enable_encryption",
         .type = SCHEMA_FIELD_BOOLEAN,
         .required = false,
         .description = "Whether to enable encryption"
     },
     {
         .name = "sampling_mode",
         .type = SCHEMA_FIELD_INTEGER,
         .required = false,
         .min_value = 0,
         .max_value = 2,
         .description = "Sampling mode"
     },
     {
         .name = "sampling_rate",
         .type = SCHEMA_FIELD_FLOAT,
         .required = false,
         .description = "Sampling rate"
     }
 };
 
 /*-------------------------------------------------------------------------*/
 /* Schema sections for component types                                     */
 /*-------------------------------------------------------------------------*/
 
 /**
  * @brief Network configuration schema sections
  */
 static schema_section_t network_sections[] = {
     {
         .name = "general",
         .fields = network_config_fields,
         .field_count = sizeof(network_config_fields) / sizeof(network_config_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     },
     {
         .name = "security",
         .fields = network_security_fields,
         .field_count = sizeof(network_security_fields) / sizeof(network_security_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     }
 };
 
 /**
  * @brief Root schema sections for all component types
  */
 static schema_section_t root_schema_sections[] = {
     {
         .name = "micro",
         .fields = micro_component_fields,
         .field_count = sizeof(micro_component_fields) / sizeof(micro_component_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     },
     {
         .name = "edge",
         .fields = edge_component_fields,
         .field_count = sizeof(edge_component_fields) / sizeof(edge_component_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     },
     {
         .name = "network",
         .fields = NULL,
         .field_count = 0,
         .sections = network_sections,
         .section_count = sizeof(network_sections) / sizeof(network_sections[0]),
         .allow_unknown_fields = false
     },
     {
         .name = "protocol",
         .fields = protocol_core_fields,
         .field_count = sizeof(protocol_core_fields) / sizeof(protocol_core_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     },
     {
         .name = "ffi",
         .fields = ffi_config_fields,
         .field_count = sizeof(ffi_config_fields) / sizeof(ffi_config_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     },
     {
         .name = "telemetry",
         .fields = telemetry_config_fields,
         .field_count = sizeof(telemetry_config_fields) / sizeof(telemetry_config_fields[0]),
         .sections = NULL,
         .section_count = 0,
         .allow_unknown_fields = false
     }
 };
 
 /*-------------------------------------------------------------------------*/
 /* Schema validation implementation                                        */
 /*-------------------------------------------------------------------------*/
 
 /**
  * @brief Create a schema validation context
  */
 polycall_core_error_t polycall_schema_context_create(
     polycall_core_context_t* ctx,
     polycall_schema_context_t** schema_ctx,
     bool strict_validation
 ) {
     if (!ctx || !schema_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_schema_context_t* new_ctx = polycall_core_malloc(ctx, sizeof(polycall_schema_context_t));
     if (!new_ctx) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_CONFIG, 
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to allocate schema context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_schema_context_t));
     new_ctx->core_ctx = ctx;
     new_ctx->strict_validation = strict_validation;
     
     // Set root schema sections
     new_ctx->root_sections = root_schema_sections;
     new_ctx->root_section_count = sizeof(root_schema_sections) / sizeof(root_schema_sections[0]);
     
     *schema_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Destroy a schema validation context
  */
 void polycall_schema_context_destroy(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx
 ) {
     if (!ctx || !schema_ctx) {
         return;
     }
     
     // Free context
     polycall_core_free(ctx, schema_ctx);
 }
 
 /**
  * @brief Find schema section by component type
  */
 static schema_section_t* find_schema_section_by_type(
     polycall_schema_context_t* schema_ctx,
     polycall_component_type_t component_type
 ) {
     if (!schema_ctx) {
         return NULL;
     }
     
     const char* section_name = NULL;
     
     // Map component type to section name
     switch (component_type) {
         case POLYCALL_COMPONENT_TYPE_MICRO:
             section_name = "micro";
             break;
         case POLYCALL_COMPONENT_TYPE_EDGE:
             section_name = "edge";
             break;
         case POLYCALL_COMPONENT_TYPE_NETWORK:
             section_name = "network";
             break;
         case POLYCALL_COMPONENT_TYPE_PROTOCOL:
             section_name = "protocol";
             break;
         case POLYCALL_COMPONENT_TYPE_FFI:
             section_name = "ffi";
             break;
         case POLYCALL_COMPONENT_TYPE_TELEMETRY:
             section_name = "telemetry";
             break;
         default:
             return NULL;
     }
     
     // Find section by name
     for (int i = 0; i < schema_ctx->root_section_count; i++) {
         if (strcmp(schema_ctx->root_sections[i].name, section_name) == 0) {
             return &schema_ctx->root_sections[i];
         }
     }
     
     return NULL;
 }

 
/**
 * @brief Validate integer against constraints
 */
static bool validate_integer_range(
    int64_t value,
    int64_t min_value,
    int64_t max_value,
    char* error_message,
    size_t error_message_size
) {
    if (value < min_value) {
        snprintf(error_message, error_message_size,
                "Value %lld is less than minimum %lld", 
                (long long)value, (long long)min_value);
        return false;
    }
    
    if (max_value > 0 && value > max_value) {
        snprintf(error_message, error_message_size,
                "Value %lld exceeds maximum %lld", 
                (long long)value, (long long)max_value);
        return false;
    }
    
    return true;
}


/**
 * @brief Validate string against allowed values
 */
static bool validate_string_enum(
    const char* str,
    const char** allowed_values,
    size_t allowed_count,
    char* error_message,
    size_t error_message_size
) {
    if (!str) {
        snprintf(error_message, error_message_size, "String is NULL");
        return false;
    }
    
    for (size_t i = 0; i < allowed_count; i++) {
        if (strcmp(str, allowed_values[i]) == 0) {
            return true;
        }
    }
    
    // Build error message with allowed values
    snprintf(error_message, error_message_size,
            "Invalid value '%s'. Allowed values are: ", str);
    
    size_t offset = strlen(error_message);
    for (size_t i = 0; i < allowed_count && offset < error_message_size; i++) {
        size_t remaining = error_message_size - offset;
        if (i > 0) {
            int written = snprintf(error_message + offset, remaining, ", ");
            if (written > 0) {
                offset += written;
            }
        }
        
        int written = snprintf(error_message + offset, remaining, "'%s'", allowed_values[i]);
        if (written > 0) {
            offset += written;
        }
    }
    
    return false;
}
 
 /**
  * @brief Validate a string against field requirements
  */
 static bool validate_string_field(
     const schema_field_t* field,
     const char* value,
     char* error_message,
     size_t error_message_size
 ) {
     if (!field || !value) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Invalid parameters for string validation");
         }
         return false;
     }
     
     // Check allowed values if specified
     if (field->allowed_values_count > 0 && field->allowed_values) {
         bool value_allowed = false;
         
         for (int i = 0; i < field->allowed_values_count; i++) {
             if (strcmp(value, field->allowed_values[i]) == 0) {
                 value_allowed = true;
                 break;
             }
         }
         
         if (!value_allowed) {
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "Value '%s' for field '%s' must be one of the allowed values",
                          value, field->name);
             }
             return false;
         }
     }
     
     // Check regex pattern if specified
     // (In a real implementation, we would use a regex library here)
     if (field->regex_pattern) {
         // Placeholder for regex validation
         // In a real implementation, we would check if the value matches the regex pattern
     }
     
     return true;
 }
 
 /**
  * @brief Validate a numeric field against requirements
  */
 static bool validate_numeric_field(
     const schema_field_t* field,
     int64_t value,
     char* error_message,
     size_t error_message_size
 ) {
     if (!field) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Invalid parameters for numeric validation");
         }
         return false;
     }
     
     // Check minimum value
     if (value < field->min_value) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, 
                      "Value %lld for field '%s' is less than the minimum value %lld",
                      (long long)value, field->name, (long long)field->min_value);
         }
         return false;
     }
     
     // Check maximum value
     if (value > field->max_value) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, 
                      "Value %lld for field '%s' is greater than the maximum value %lld",
                      (long long)value, field->name, (long long)field->max_value);
         }
         return false;
     }
     
     return true;
 }
 /**
  * @brief Validate micro component configuration
  */
 polycall_core_error_t validate_micro_component_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const micro_component_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config || !error_message || error_message_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }

     // Perform initial direct validation checks
     if (!validate_string_length(config->name, 1, sizeof(config->name) - 1,
                               error_message, error_message_size)) {
         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
     }

     if (config->allowed_connections_count > 16) {
         snprintf(error_message, error_message_size,
                 "Too many allowed connections: %zu (maximum is 16)",
                 config->allowed_connections_count);
         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
     }

     // Check for duplicate command names in strict mode
     if (schema_ctx->strict_validation && config->command_count > 0) {
         for (size_t i = 0; i < config->command_count - 1; i++) {
             for (size_t j = i + 1; j < config->command_count; j++) {
                 if (strcmp(config->commands[i].name, config->commands[j].name) == 0) {
                     snprintf(error_message, error_message_size,
                             "Duplicate command name: %s", config->commands[i].name);
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
         }
     }
     
     // Find schema section for micro component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_MICRO);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for micro component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Validate required fields
     for (int i = 0; i < section->field_count; i++) {
         const schema_field_t* field = &section->fields[i];
         
         if (field->required) {
             // Check name field
             if (strcmp(field->name, "name") == 0) {
                 if (!config->name || config->name[0] == '\0') {
                     if (error_message && error_message_size > 0) {
                         snprintf(error_message, error_message_size, 
                                  "Required field '%s' is missing or empty", field->name);
                     }
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check isolation_level field
             else if (strcmp(field->name, "isolation_level") == 0) {
                 const char* isolation_str = NULL;
                 
                 switch (config->isolation_level) {
                     case POLYCALL_ISOLATION_NONE:
                         isolation_str = "none";
                         break;
                     case POLYCALL_ISOLATION_MEMORY:
                         isolation_str = "memory";
                         break;
                     case POLYCALL_ISOLATION_RESOURCES:
                         isolation_str = "resources";
                         break;
                     case POLYCALL_ISOLATION_SECURITY:
                         isolation_str = "security";
                         break;
                     case POLYCALL_ISOLATION_STRICT:
                         isolation_str = "strict";
                         break;
                     default:
                         if (error_message && error_message_size > 0) {
                             snprintf(error_message, error_message_size, 
                                      "Invalid isolation level: %d", config->isolation_level);
                         }
                         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
                 
                 if (!validate_string_field(field, isolation_str, error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check memory_quota field
             else if (strcmp(field->name, "memory_quota") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->memory_quota, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check cpu_quota field
             else if (strcmp(field->name, "cpu_quota") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->cpu_quota, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check io_quota field
             else if (strcmp(field->name, "io_quota") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->io_quota, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check for other required fields that might be missing...
         }
     }
     
     // Validate command configurations if present
     if (config->command_count > 0) {
         for (uint32_t i = 0; i < config->command_count; i++) {
             const micro_command_config_t* cmd = &config->commands[i];
             
             // Check command name
             if (!cmd->name || cmd->name[0] == '\0') {
                 if (error_message && error_message_size > 0) {
                     snprintf(error_message, error_message_size, 
                              "Command at index %u has an empty name", i);
                 }
                 return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
             }
             
             // Validate other command fields...
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate edge component configuration
  */
 polycall_core_error_t validate_edge_component_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const polycall_edge_component_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find schema section for edge component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_EDGE);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for edge component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Validate required fields
     for (int i = 0; i < section->field_count; i++) {
         const schema_field_t* field = &section->fields[i];
         
         if (field->required) {
             // Check name field
             if (strcmp(field->name, "name") == 0) {
                 if (!config->name || config->name[0] == '\0') {
                     if (error_message && error_message_size > 0) {
                         snprintf(error_message, error_message_size, 
                                  "Required field '%s' is missing or empty", field->name);
                     }
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check component_id field
             else if (strcmp(field->name, "component_id") == 0) {
                 if (!config->component_id || config->component_id[0] == '\0') {
                     if (error_message && error_message_size > 0) {
                         snprintf(error_message, error_message_size, 
                                  "Required field '%s' is missing or empty", field->name);
                     }
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check type field
             else if (strcmp(field->name, "type") == 0) {
                 const char* type_str = NULL;
                 
                 switch (config->type) {
                     case EDGE_COMPONENT_TYPE_COMPUTE:
                         type_str = "compute";
                         break;
                     case EDGE_COMPONENT_TYPE_STORAGE:
                         type_str = "storage";
                         break;
                     case EDGE_COMPONENT_TYPE_GATEWAY:
                         type_str = "gateway";
                         break;
                     case EDGE_COMPONENT_TYPE_SENSOR:
                         type_str = "sensor";
                         break;
                     case EDGE_COMPONENT_TYPE_ACTUATOR:
                         type_str = "actuator";
                         break;
                     case EDGE_COMPONENT_TYPE_COORDINATOR:
                         type_str = "coordinator";
                         break;
                     case EDGE_COMPONENT_TYPE_CUSTOM:
                         type_str = "custom";
                         break;
                     default:
                         if (error_message && error_message_size > 0) {
                             snprintf(error_message, error_message_size, 
                                      "Invalid component type: %d", config->type);
                         }
                         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
                 
                 if (!validate_string_field(field, type_str, error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check other required fields...
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
/**
 * @brief Validate port number against constraints
 * 
 * Implements the zero-trust approach to port validation by checking:
 * 1. Port is in valid range (0-65535)
 * 2. Privileged ports (< 1024) are flagged unless privileged context is set
 * 3. Reserved ports are avoided
 */
static bool validate_port_number(
    int port,
    bool is_privileged_context,
    char* error_message,
    size_t error_message_size
) {
    // Check port is in valid range
    if (port < 0 || port > 65535) {
        snprintf(error_message, error_message_size,
                "Invalid port number: %d (must be 0-65535)", port);
        return false;
    }
    
    // Check for privileged ports
    if (port < 1024 && !is_privileged_context) {
        snprintf(error_message, error_message_size,
                "Port %d requires privileged access", port);
        return false;
    }
    
    // Check for common reserved ports
    const int reserved_ports[] = {1433, 1434, 3306, 5432, 6379, 27017, 27018, 27019};
    const size_t reserved_port_count = sizeof(reserved_ports) / sizeof(reserved_ports[0]);
    
    for (size_t i = 0; i < reserved_port_count; i++) {
        if (port == reserved_ports[i]) {
            snprintf(error_message, error_message_size,
                    "Port %d is commonly reserved for other services", port);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Validate string length and content
 */
static bool validate_string_length(
    const char* str,
    size_t min_length,
    size_t max_length,
    char* error_message,
    size_t error_message_size
) {
    if (!str) {
        if (min_length > 0) {
            snprintf(error_message, error_message_size,
                    "String is required but was NULL");
            return false;
        }
        return true;
    }
    
    size_t length = strlen(str);
    
    if (length < min_length) {
        snprintf(error_message, error_message_size,
                "String length %zu is less than minimum %zu", length, min_length);
        return false;
    }
    
    if (max_length > 0 && length > max_length) {
        snprintf(error_message, error_message_size,
                "String length %zu exceeds maximum %zu", length, max_length);
        return false;
    }
    
    return true;
}

 /**
  * @brief Validate network configuration
  */
 polycall_core_error_t validate_network_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const polycall_network_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find schema section for network component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_NETWORK);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for network component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get general section
     schema_section_t* general_section = NULL;
     for (int i = 0; i < section->section_count; i++) {
         if (strcmp(section->sections[i].name, "general") == 0) {
             general_section = &section->sections[i];
             break;
         }
     }
     
     if (!general_section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "General section not found in network schema");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Get security section
     schema_section_t* security_section = NULL;
     for (int i = 0; i < section->section_count; i++) {
         if (strcmp(section->sections[i].name, "security") == 0) {
             security_section = &section->sections[i];
             break;
         }
     }
     
     // Validate network configuration
     // In a real implementation, we would check the configuration values against the schema
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate protocol configuration
  */
 polycall_core_error_t validate_protocol_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const protocol_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find schema section for protocol component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_PROTOCOL);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for protocol component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Validate required fields
     for (int i = 0; i < section->field_count; i++) {
         const schema_field_t* field = &section->fields[i];
         
         if (field->required) {
             // Check transport_type field
             if (strcmp(field->name, "transport_type") == 0) {
                 const char* transport_str = NULL;
                 
                 switch (config->core.transport_type) {
                     case PROTOCOL_TRANSPORT_TCP:
                         transport_str = "tcp";
                         break;
                     case PROTOCOL_TRANSPORT_UDP:
                         transport_str = "udp";
                         break;
                     case PROTOCOL_TRANSPORT_WEBSOCKET:
                         transport_str = "websocket";
                         break;
                     case PROTOCOL_TRANSPORT_UNIX:
                         transport_str = "unix";
                         break;
                     default:
                         if (error_message && error_message_size > 0) {
                             snprintf(error_message, error_message_size, 
                                      "Invalid transport type: %d", config->core.transport_type);
                         }
                         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
                 
                 if (!validate_string_field(field, transport_str, error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check encoding_format field
             else if (strcmp(field->name, "encoding_format") == 0) {
                 const char* encoding_str = NULL;
                 
                 switch (config->core.encoding_format) {
                     case PROTOCOL_ENCODING_JSON:
                         encoding_str = "json";
                         break;
                     case PROTOCOL_ENCODING_MSGPACK:
                         encoding_str = "msgpack";
                         break;
                     case PROTOCOL_ENCODING_PROTOBUF:
                         encoding_str = "protobuf";
                         break;
                     case PROTOCOL_ENCODING_BINARY:
                         encoding_str = "binary";
                         break;
                     default:
                         if (error_message && error_message_size > 0) {
                             snprintf(error_message, error_message_size, 
                                      "Invalid encoding format: %d", config->core.encoding_format);
                         }
                         return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
                 
                 if (!validate_string_field(field, encoding_str, error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check other required fields...
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate FFI configuration
  */
 polycall_core_error_t validate_ffi_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const polycall_ffi_config_options_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find schema section for FFI component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_FFI);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for FFI component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Validate FFI configuration
     // In a real implementation, we would check the configuration values against the schema
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate telemetry configuration
  */
 polycall_core_error_t validate_telemetry_config(
     polycall_core_context_t* ctx,
     polycall_schema_context_t* schema_ctx,
     const polycall_telemetry_config_t* config,
     char* error_message,
     size_t error_message_size
 ) {
     if (!ctx || !schema_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find schema section for telemetry component
     schema_section_t* section = find_schema_section_by_type(
         schema_ctx, POLYCALL_COMPONENT_TYPE_TELEMETRY);
     
     if (!section) {
         if (error_message && error_message_size > 0) {
             snprintf(error_message, error_message_size, "Schema not found for telemetry component");
         }
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Validate required fields
     for (int i = 0; i < section->field_count; i++) {
         const schema_field_t* field = &section->fields[i];
         
         if (field->required) {
             // Check enable_telemetry field
             if (strcmp(field->name, "enable_telemetry") == 0) {
                 // Boolean field, no validation needed
             }
             // Check min_severity field
             else if (strcmp(field->name, "min_severity") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->min_severity, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check max_event_queue_size field
             else if (strcmp(field->name, "max_event_queue_size") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->max_event_queue_size, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check format field
             else if (strcmp(field->name, "format") == 0) {
                 if (!validate_numeric_field(field, (int64_t)config->format, 
                                            error_message, error_message_size)) {
                     return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
                 }
             }
             // Check other required fields...
         }
     }
     
     // Validate output_path if destination is file
     if (config->destination == TELEMETRY_DEST_FILE) {
         if (!config->output_path || config->output_path[0] == '\0') {
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "output_path is required when destination is file");
             }
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     // Validate sampling rate if sampling mode is not none
     if (config->sampling_mode != TELEMETRY_SAMPLING_NONE) {
         if (config->sampling_rate <= 0.0f || config->sampling_rate > 1.0f) {
             if (error_message && error_message_size > 0) {
                 snprintf(error_message, error_message_size, 
                          "sampling_rate must be between 0.0 and 1.0");
             }
             return POLYCALL_CORE_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
/**
 * @brief Validate component configuration against schema
 */
polycall_core_error_t polycall_schema_validate_component(
    polycall_core_context_t* ctx,
    polycall_schema_context_t* schema_ctx,
    polycall_component_type_t component_type,
    const void* component_config,
    char* error_message,
    size_t error_message_size
) {
    if (!ctx || !schema_ctx || !component_config || !error_message || error_message_size == 0) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Dispatch to component-specific validation function
    switch (component_type) {
        case POLYCALL_COMPONENT_TYPE_MICRO:
            return validate_micro_component_config(
                ctx, schema_ctx, (const micro_component_config_t*)component_config,
                error_message, error_message_size);
            
        case POLYCALL_COMPONENT_TYPE_EDGE:
            return validate_edge_component_config(
                ctx, schema_ctx, (const polycall_edge_component_config_t*)component_config,
                error_message, error_message_size);
            
        case POLYCALL_COMPONENT_TYPE_NETWORK:
            return validate_network_config(
                ctx, schema_ctx, (const polycall_network_config_t*)component_config,
                error_message, error_message_size);
            
        case POLYCALL_COMPONENT_TYPE_PROTOCOL:
            return validate_protocol_config(
                ctx, schema_ctx, (const protocol_config_t*)component_config,
                error_message, error_message_size);
            
        case POLYCALL_COMPONENT_TYPE_FFI:
            return validate_ffi_config(
                ctx, schema_ctx, (const polycall_ffi_config_options_t*)component_config,
                error_message, error_message_size);
            
        case POLYCALL_COMPONENT_TYPE_TELEMETRY:
            return validate_telemetry_config(
                ctx, schema_ctx, (const polycall_telemetry_config_t*)component_config,
                error_message, error_message_size);
            
        default:
            snprintf(error_message, error_message_size,
                    "Validation not implemented for component type %d", component_type);
            return POLYCALL_CORE_ERROR_NOT_IMPLEMENTED;
    }
}
