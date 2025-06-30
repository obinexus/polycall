
#include "polycall/core/ffi/type_system.h"

/**
 * @file type_system.c
 * @brief Type system module implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

 #include "polycall/core/ffi/type_system.h"


// Mapping rule structure
struct mapping_rule {
    const char* source_language;
    polycall_ffi_type_t source_type;
    const char* target_language;
    polycall_ffi_type_t target_type;
    polycall_type_conv_flags_t flags;
    polycall_core_error_t (*convert)(
    // Register primitive types if configured
    if (config->auto_register_primitives) {
        register_primitive_types(ctx, ffi_ctx, new_ctx);
    }
    
    *type_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register primitive types in the type system
 * 
 * Registers all basic primitive types supported by LibPolyCall
 */
static polycall_core_error_t register_primitive_types(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    type_mapping_context_t* type_ctx
) {
    // Primitive types to register
    static const struct {
        const char* name;
        size_t size;
        polycall_ffi_type_t type_id;
    } primitives[] = {
        {"void", 0, POLYCALL_FFI_TYPE_VOID},
        {"int8", sizeof(int8_t), POLYCALL_FFI_TYPE_INT8},
        {"uint8", sizeof(uint8_t), POLYCALL_FFI_TYPE_UINT8},
        {"int16", sizeof(int16_t), POLYCALL_FFI_TYPE_INT16},
        {"uint16", sizeof(uint16_t), POLYCALL_FFI_TYPE_UINT16},
        {"int32", sizeof(int32_t), POLYCALL_FFI_TYPE_INT32},
        {"uint32", sizeof(uint32_t), POLYCALL_FFI_TYPE_UINT32},
        {"int64", sizeof(int64_t), POLYCALL_FFI_TYPE_INT64},
        {"uint64", sizeof(uint64_t), POLYCALL_FFI_TYPE_UINT64},
        {"float", sizeof(float), POLYCALL_FFI_TYPE_FLOAT},
        {"double", sizeof(double), POLYCALL_FFI_TYPE_DOUBLE},
        {"bool", sizeof(bool), POLYCALL_FFI_TYPE_BOOL},
        {"char", sizeof(char), POLYCALL_FFI_TYPE_CHAR},
        {"string", sizeof(char*), POLYCALL_FFI_TYPE_STRING},
        {"pointer", sizeof(void*), POLYCALL_FFI_TYPE_POINTER}
    };
    
    // Register each primitive type
    for (size_t i = 0; i < sizeof(primitives) / sizeof(primitives[0]); i++) {
        ffi_type_info_t type_info = {
            .name = primitives[i].name,
            .type_id = primitives[i].type_id,
            .size = primitives[i].size,
            .flags = POLYCALL_TYPE_CONV_FLAG_NONE,
            .is_primitive = true
        };
        
        polycall_core_error_t error = register_type(ctx, type_ctx, &type_info);
        if (error != POLYCALL_CORE_SUCCESS) {
            return error;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register a type in the type registry
 * 
 * Adds a type to the type system's registry
 */
static polycall_core_error_t register_type(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    const ffi_type_info_t* type_info
) {
    // Check if we have capacity
    if (type_ctx->type_count >= type_ctx->type_capacity) {
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Check if type already exists
    for (size_t i = 0; i < type_ctx->type_count; i++) {
        if (type_ctx->types[i].type_id == type_info->type_id) {
            return POLYCALL_CORE_ERROR_ALREADY_REGISTERED;
        }
    }
    
    // Register the type
    type_ctx->types[type_ctx->type_count] = *type_info;
    type_ctx->type_count++;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Look up a type by ID in the type registry
 */
polycall_core_error_t polycall_type_lookup(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    polycall_ffi_type_t type_id,
    ffi_type_info_t** type_info
) {
    // Validate parameters
    if (!ctx || !type_ctx || !type_info) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Search for the type
    for (size_t i = 0; i < type_ctx->type_count; i++) {
        if (type_ctx->types[i].type_id == type_id) {
            *type_info = &type_ctx->types[i];
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    *type_info = NULL;
    return POLYCALL_CORE_ERROR_TYPE_NOT_FOUND;
}

/**
 * @brief Register a type conversion rule
 */
polycall_core_error_t polycall_type_register_rule(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    const type_conversion_rule_t* rule
) {
    // Validate parameters
    if (!ctx || !type_ctx || !rule) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if we have capacity
    if (type_ctx->rule_count >= type_ctx->rule_capacity) {
        return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Check if rule already exists
    for (size_t i = 0; i < type_ctx->rule_count; i++) {
        mapping_rule_t* existing = &type_ctx->rules[i];
        if (strcmp(existing->source_language, rule->source_language) == 0 &&
            existing->source_type == rule->source_type &&
            strcmp(existing->target_language, rule->target_language) == 0 &&
            existing->target_type == rule->target_type) {
            return POLYCALL_CORE_ERROR_ALREADY_REGISTERED;
        }
    }
    
    // Register the rule
    mapping_rule_t new_rule = {
        .source_language = rule->source_language,
        .source_type = rule->source_type,
        .target_language = rule->target_language,
        .target_type = rule->target_type,
        .flags = rule->flags,
        .convert = rule->convert,
        .validate = rule->validate,
        .user_data = rule->user_data
    };
    
    type_ctx->rules[type_ctx->rule_count] = new_rule;
    type_ctx->rule_count++;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Find a conversion rule for the given source and target types
 */
polycall_core_error_t polycall_type_find_conversion(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    const char* source_language,
    polycall_ffi_type_t source_type,
    const char* target_language,
    polycall_ffi_type_t target_type,
    mapping_rule_t** rule
) {
    // Validate parameters
    if (!ctx || !type_ctx || !source_language || !target_language || !rule) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Search for matching rule
    for (size_t i = 0; i < type_ctx->rule_count; i++) {
        mapping_rule_t* current = &type_ctx->rules[i];
        if (strcmp(current->source_language, source_language) == 0 &&
            current->source_type == source_type &&
            strcmp(current->target_language, target_language) == 0 &&
            current->target_type == target_type) {
            *rule = current;
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    *rule = NULL;
    return POLYCALL_CORE_ERROR_CONVERSION_NOT_FOUND;
}

/**
 * @brief Convert a value from source type to target type
 */
polycall_core_error_t polycall_type_convert(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    const char* source_language,
    polycall_ffi_type_t source_type,
    const void* source_value,
    const char* target_language,
    polycall_ffi_type_t target_type,
    void* target_value,
    size_t* target_size
) {
    // Validate parameters
    if (!ctx || !type_ctx || !source_language || !source_value || 
        !target_language || !target_value || !target_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find conversion rule
    mapping_rule_t* rule = NULL;
    polycall_core_error_t error = polycall_type_find_conversion(
        ctx, type_ctx, source_language, source_type, 
        target_language, target_type, &rule
    );
    
    if (error != POLYCALL_CORE_SUCCESS) {
        return error;
    }
    
    // Apply the conversion
    return rule->convert(ctx, source_value, target_value, target_size, rule->user_data);
}

/**
 * @brief Validate a value against type constraints
 */
polycall_core_error_t polycall_type_validate(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    polycall_ffi_type_t type_id,
    const void* value,
    size_t size
) {
    // Validate parameters
    if (!ctx || !type_ctx || !value) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get type info
    ffi_type_info_t* type_info = NULL;
    polycall_core_error_t error = polycall_type_lookup(ctx, type_ctx, type_id, &type_info);
    
    if (error != POLYCALL_CORE_SUCCESS) {
        return error;
    }
    
    // Basic size validation for primitive types
    if (type_info->is_primitive && type_info->size > 0 && size != type_info->size) {
        return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup type system resources
 */
polycall_core_error_t polycall_type_cleanup(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx
) {
    // Validate parameters
    if (!ctx || !type_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Free type registry
    if (type_ctx->types) {
        polycall_core_free(ctx, type_ctx->types);
    }
    
    // Free rules
    if (type_ctx->rules) {
        polycall_core_free(ctx, type_ctx->rules);
    }
    
    // Free context itself
    polycall_core_free(ctx, type_ctx);
    
    return POLYCALL_CORE_SUCCESS;
}
        polycall_core_context_t* ctx,
        const void* data,
        size_t size,
        void* user_data
    );
    
    void* user_data;
};

// Conversion registry structure
struct conversion_registry {
    mapping_rule_t* rules;
    size_t count;
    size_t capacity;
};

// Forward declarations of internal functions
static polycall_core_error_t register_primitive_types(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    type_mapping_context_t* type_ctx
);

static polycall_core_error_t register_type(
    polycall_core_context_t* ctx,
    type_mapping_context_t* type_ctx,
    const ffi_type_info_t* type_info
);

// Core type registration and mapping functionality
polycall_core_error_t polycall_type_init(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    type_mapping_context_t** type_ctx,
    const type_system_config_t* config
) {
    // Allocate type context
    type_mapping_context_t* new_ctx = polycall_core_malloc(ctx, sizeof(type_mapping_context_t));
    if (!new_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize type registry
    new_ctx->types = polycall_core_malloc(ctx, config->type_capacity * sizeof(ffi_type_info_t));
    if (!new_ctx->types) {
        polycall_core_free(ctx, new_ctx);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize conversion rules registry
    new_ctx->rules = polycall_core_malloc(ctx, config->rule_capacity * sizeof(mapping_rule_t));
    if (!new_ctx->rules) {
        polycall_core_free(ctx, new_ctx->types);
        polycall_core_free(ctx, new_ctx);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Set up type registry
    new_ctx->type_count = 0;
    new_ctx->rule_count = 0;
    
    // Register primitive types if configured
    if (config->auto_register_primitives) {
        register_primitive_types(ctx, ffi_ctx, new_ctx);
    }
    
    *type_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

