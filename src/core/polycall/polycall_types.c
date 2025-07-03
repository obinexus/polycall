/**
 * @file polycall_types.c
 * @brief Type system implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides type conversion utilities, string representations, and
 * cross-module type coordination for the polycall framework.
 */

#include "polycall/core/polycall/polycall_types.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_core.h"
#include <string.h>
#include <stdio.h>

/* Type registry for dynamic type resolution */
typedef struct {
    polycall_context_type_t type;
    const char* name;
    size_t size;
} polycall_type_info_t;

/* Static type registry */
static const polycall_type_info_t type_registry[] = {
    { POLYCALL_CONTEXT_TYPE_CORE,     "core",     sizeof(polycall_core_context_t) },
    { POLYCALL_CONTEXT_TYPE_PROTOCOL, "protocol", sizeof(polycall_protocol_context_t) },
    { POLYCALL_CONTEXT_TYPE_NETWORK,  "network",  sizeof(NetworkEndpoint) },
    { POLYCALL_CONTEXT_TYPE_MICRO,    "micro",    sizeof(void*) }, /* Placeholder size */
    { POLYCALL_CONTEXT_TYPE_EDGE,     "edge",     sizeof(void*) }, /* Placeholder size */
    { POLYCALL_CONTEXT_TYPE_PARSER,   "parser",   sizeof(void*) }, /* Placeholder size */
};

/**
 * @brief Convert context type to string representation
 */
const char* polycall_context_type_to_string(polycall_context_type_t type) {
    const size_t count = sizeof(type_registry) / sizeof(type_registry[0]);
    
    for (size_t i = 0; i < count; i++) {
        if (type_registry[i].type == type) {
            return type_registry[i].name;
        }
    }
    
    return (type >= POLYCALL_CONTEXT_TYPE_USER) ? "user-defined" : "unknown";
}

/**
 * @brief Get size of context type for allocation
 */
size_t polycall_context_type_size(polycall_context_type_t type) {
    const size_t count = sizeof(type_registry) / sizeof(type_registry[0]);
    
    for (size_t i = 0; i < count; i++) {
        if (type_registry[i].type == type) {
            return type_registry[i].size;
        }
    }
    
    return 0; /* Unknown type */
}

/**
 * @brief Convert log level to string
 */
const char* polycall_log_level_to_string(polycall_log_level_t level) {
    switch (level) {
        case POLYCALL_LOG_DEBUG:   return "DEBUG";
        case POLYCALL_LOG_INFO:    return "INFO";
        case POLYCALL_LOG_WARNING: return "WARNING";
        case POLYCALL_LOG_ERROR:   return "ERROR";
        case POLYCALL_LOG_FATAL:   return "FATAL";
        default:                   return "UNKNOWN";
    }
}

/**
 * @brief Parse log level from string
 */
polycall_log_level_t polycall_log_level_from_string(const char* str) {
    if (!str) return POLYCALL_LOG_INFO;
    
    if (strcasecmp(str, "debug") == 0)   return POLYCALL_LOG_DEBUG;
    if (strcasecmp(str, "info") == 0)    return POLYCALL_LOG_INFO;
    if (strcasecmp(str, "warning") == 0) return POLYCALL_LOG_WARNING;
    if (strcasecmp(str, "error") == 0)   return POLYCALL_LOG_ERROR;
    if (strcasecmp(str, "fatal") == 0)   return POLYCALL_LOG_FATAL;
    
    return POLYCALL_LOG_INFO; /* Default */
}

/**
 * @brief Convert configuration section to string
 */
const char* polycall_config_section_to_string(polycall_config_section_t section) {
    switch (section) {
        case POLYCALL_CONFIG_SECTION_CORE:        return "core";
        case POLYCALL_CONFIG_SECTION_SECURITY:    return "security";
        case POLYCALL_CONFIG_SECTION_MEMORY:      return "memory";
        case POLYCALL_CONFIG_SECTION_TYPE:        return "type";
        case POLYCALL_CONFIG_SECTION_PERFORMANCE: return "performance";
        case POLYCALL_CONFIG_SECTION_PROTOCOL:    return "protocol";
        case POLYCALL_CONFIG_SECTION_C:           return "c";
        case POLYCALL_CONFIG_SECTION_JVM:         return "jvm";
        case POLYCALL_CONFIG_SECTION_JS:          return "js";
        case POLYCALL_CONFIG_SECTION_PYTHON:      return "python";
        default:
            return (section >= POLYCALL_CONFIG_SECTION_USER) ? "user" : "unknown";
    }
}

/**
 * @brief Convert configuration value type to string
 */
const char* polycall_config_value_type_to_string(polycall_config_value_type_t type) {
    switch (type) {
        case POLYCALL_CONFIG_VALUE_BOOLEAN: return "boolean";
        case POLYCALL_CONFIG_VALUE_INTEGER: return "integer";
        case POLYCALL_CONFIG_VALUE_FLOAT:   return "float";
        case POLYCALL_CONFIG_VALUE_STRING:  return "string";
        case POLYCALL_CONFIG_VALUE_OBJECT:  return "object";
        default:                            return "unknown";
    }
}

/**
 * @brief Check if context flags indicate initialization
 */
int polycall_context_is_initialized(polycall_context_flags_t flags) {
    return (flags & POLYCALL_CONTEXT_FLAG_INITIALIZED) != 0;
}

/**
 * @brief Check if context flags indicate locked state
 */
int polycall_context_is_locked(polycall_context_flags_t flags) {
    return (flags & POLYCALL_CONTEXT_FLAG_LOCKED) != 0;
}

/**
 * @brief Combine multiple context flags
 */
polycall_context_flags_t polycall_context_flags_combine(
    polycall_context_flags_t flags1,
    polycall_context_flags_t flags2) {
    return (polycall_context_flags_t)(flags1 | flags2);
}

/**
 * @brief Remove specific flags from context
 */
polycall_context_flags_t polycall_context_flags_remove(
    polycall_context_flags_t flags,
    polycall_context_flags_t remove) {
    return (polycall_context_flags_t)(flags & ~remove);
}

/* Configuration value structure implementation */
struct polycall_config_value {
    polycall_config_value_type_t type;
    union {
        int boolean_value;
        int64_t integer_value;
        double float_value;
        char* string_value;
        void* object_value;
    } data;
    size_t size; /* For object values */
};

/**
 * @brief Create a boolean configuration value
 */
struct polycall_config_value* polycall_config_value_create_boolean(int value) {
    struct polycall_config_value* val = calloc(1, sizeof(struct polycall_config_value));
    if (val) {
        val->type = POLYCALL_CONFIG_VALUE_BOOLEAN;
        val->data.boolean_value = value ? 1 : 0;
    }
    return val;
}

/**
 * @brief Create an integer configuration value
 */
struct polycall_config_value* polycall_config_value_create_integer(int64_t value) {
    struct polycall_config_value* val = calloc(1, sizeof(struct polycall_config_value));
    if (val) {
        val->type = POLYCALL_CONFIG_VALUE_INTEGER;
        val->data.integer_value = value;
    }
    return val;
}

/**
 * @brief Create a string configuration value
 */
struct polycall_config_value* polycall_config_value_create_string(const char* value) {
    if (!value) return NULL;
    
    struct polycall_config_value* val = calloc(1, sizeof(struct polycall_config_value));
    if (val) {
        val->type = POLYCALL_CONFIG_VALUE_STRING;
        val->data.string_value = strdup(value);
        if (!val->data.string_value) {
            free(val);
            return NULL;
        }
    }
    return val;
}

/**
 * @brief Free configuration value
 */
void polycall_config_value_free(struct polycall_config_value* value) {
    if (!value) return;
    
    if (value->type == POLYCALL_CONFIG_VALUE_STRING && value->data.string_value) {
        free(value->data.string_value);
    } else if (value->type == POLYCALL_CONFIG_VALUE_OBJECT && value->data.object_value) {
        free(value->data.object_value);
    }
    
    free(value);
}

/**
 * @brief Type system initialization
 */
int polycall_types_init(void) {
    /* Perform any one-time type system initialization */
    /* Currently no global state to initialize */
    return 0;
}

/**
 * @brief Type system cleanup
 */
void polycall_types_cleanup(void) {
    /* Cleanup any type system resources */
    /* Currently no global state to cleanup */
}