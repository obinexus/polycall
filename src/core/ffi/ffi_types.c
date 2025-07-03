/**
 * @file ffi_types.c
 * @brief FFI type system implementation for LibPolyCall
 * @author OBINexus Engineering - Aegis Project Phase 2
 * 
 * Implements type validation, conversion, and management functions
 * for the FFI subsystem with zero-trust security integration.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "polycall/core/ffi/ffi_types.h"
#include "polycall/core/polycall_log.h"

/**
 * @brief Type name lookup table
 */
static const struct {
    polycall_ffi_type_t type;
    const char* name;
    size_t size;
    size_t alignment;
} type_info_table[] = {
    { POLYCALL_FFI_TYPE_VOID,    "void",     0,                0 },
    { POLYCALL_FFI_TYPE_INT8,    "int8",     sizeof(int8_t),   alignof(int8_t) },
    { POLYCALL_FFI_TYPE_UINT8,   "uint8",    sizeof(uint8_t),  alignof(uint8_t) },
    { POLYCALL_FFI_TYPE_INT16,   "int16",    sizeof(int16_t),  alignof(int16_t) },
    { POLYCALL_FFI_TYPE_UINT16,  "uint16",   sizeof(uint16_t), alignof(uint16_t) },
    { POLYCALL_FFI_TYPE_INT32,   "int32",    sizeof(int32_t),  alignof(int32_t) },
    { POLYCALL_FFI_TYPE_UINT32,  "uint32",   sizeof(uint32_t), alignof(uint32_t) },
    { POLYCALL_FFI_TYPE_INT64,   "int64",    sizeof(int64_t),  alignof(int64_t) },
    { POLYCALL_FFI_TYPE_UINT64,  "uint64",   sizeof(uint64_t), alignof(uint64_t) },
    { POLYCALL_FFI_TYPE_FLOAT,   "float",    sizeof(float),    alignof(float) },
    { POLYCALL_FFI_TYPE_DOUBLE,  "double",   sizeof(double),   alignof(double) },
    { POLYCALL_FFI_TYPE_BOOL,    "bool",     sizeof(bool),     alignof(bool) },
    { POLYCALL_FFI_TYPE_CHAR,    "char",     sizeof(char),     alignof(char) },
    { POLYCALL_FFI_TYPE_STRING,  "string",   sizeof(char*),    alignof(char*) },
    { POLYCALL_FFI_TYPE_POINTER, "pointer",  sizeof(void*),    alignof(void*) },
    { POLYCALL_FFI_TYPE_ARRAY,   "array",    0,                alignof(void*) },
    { POLYCALL_FFI_TYPE_STRUCT,  "struct",   0,                alignof(void*) },
    { POLYCALL_FFI_TYPE_FUNCTION,"function", sizeof(void*),    alignof(void*) },
    { POLYCALL_FFI_TYPE_OPAQUE,  "opaque",   0,                0 },
    { POLYCALL_FFI_TYPE_CUSTOM,  "custom",   0,                0 }
};

static const size_t type_info_count = sizeof(type_info_table) / sizeof(type_info_table[0]);

/**
 * @brief Get human-readable type name
 */
const char* polycall_ffi_type_name(polycall_ffi_type_t type) {
    for (size_t i = 0; i < type_info_count; i++) {
        if (type_info_table[i].type == type) {
            return type_info_table[i].name;
        }
    }
    
    /* Check for custom types */
    if (type >= POLYCALL_FFI_TYPE_CUSTOM) {
        return "custom";
    }
    
    return "unknown";
}

/**
 * @brief Get type size in bytes
 */
size_t polycall_ffi_type_size(polycall_ffi_type_t type) {
    for (size_t i = 0; i < type_info_count; i++) {
        if (type_info_table[i].type == type) {
            return type_info_table[i].size;
        }
    }
    
    /* Custom types have variable size */
    if (type >= POLYCALL_FFI_TYPE_CUSTOM) {
        return 0;
    }
    
    return 0;
}

/**
 * @brief Check if types are compatible
 */
bool polycall_ffi_types_compatible(polycall_ffi_type_t type1, 
                                  polycall_ffi_type_t type2) {
    /* Exact match */
    if (type1 == type2) {
        return true;
    }
    
    /* Void is compatible with nothing else */
    if (type1 == POLYCALL_FFI_TYPE_VOID || type2 == POLYCALL_FFI_TYPE_VOID) {
        return false;
    }
    
    /* Numeric type compatibility rules */
    bool type1_numeric = (type1 >= POLYCALL_FFI_TYPE_INT8 && 
                         type1 <= POLYCALL_FFI_TYPE_DOUBLE);
    bool type2_numeric = (type2 >= POLYCALL_FFI_TYPE_INT8 && 
                         type2 <= POLYCALL_FFI_TYPE_DOUBLE);
    
    if (type1_numeric && type2_numeric) {
        /* Allow numeric conversions with potential data loss */
        return true;
    }
    
    /* Pointer compatibility */
    if (type1 == POLYCALL_FFI_TYPE_POINTER || type2 == POLYCALL_FFI_TYPE_POINTER) {
        /* Pointers can convert to/from certain types */
        if (type1 == POLYCALL_FFI_TYPE_STRING || type2 == POLYCALL_FFI_TYPE_STRING ||
            type1 == POLYCALL_FFI_TYPE_ARRAY || type2 == POLYCALL_FFI_TYPE_ARRAY ||
            type1 == POLYCALL_FFI_TYPE_FUNCTION || type2 == POLYCALL_FFI_TYPE_FUNCTION) {
            return true;
        }
    }
    
    /* String and char array compatibility */
    if ((type1 == POLYCALL_FFI_TYPE_STRING && type2 == POLYCALL_FFI_TYPE_ARRAY) ||
        (type2 == POLYCALL_FFI_TYPE_STRING && type1 == POLYCALL_FFI_TYPE_ARRAY)) {
        return true;
    }
    
    /* No other compatibility */
    return false;
}

/**
 * @brief Create type descriptor
 */
polycall_ffi_type_desc_t* polycall_ffi_type_desc_create(polycall_ffi_type_t type) {
    polycall_ffi_type_desc_t* desc = calloc(1, sizeof(polycall_ffi_type_desc_t));
    if (!desc) {
        return NULL;
    }
    
    desc->type = type;
    
    /* Find type info */
    for (size_t i = 0; i < type_info_count; i++) {
        if (type_info_table[i].type == type) {
            desc->size = type_info_table[i].size;
            desc->alignment = type_info_table[i].alignment;
            break;
        }
    }
    
    /* Initialize type-specific fields */
    switch (type) {
        case POLYCALL_FFI_TYPE_ARRAY:
            /* Array info will be filled by caller */
            desc->array_info.element_type = POLYCALL_FFI_TYPE_VOID;
            desc->array_info.element_count = 0;
            desc->array_info.element_size = 0;
            break;
            
        case POLYCALL_FFI_TYPE_STRUCT:
            /* Struct info will be filled by caller */
            desc->struct_info.field_names = NULL;
            desc->struct_info.field_types = NULL;
            desc->struct_info.field_offsets = NULL;
            desc->struct_info.field_count = 0;
            break;
            
        case POLYCALL_FFI_TYPE_FUNCTION:
            /* Function info will be filled by caller */
            desc->function_info.signature = NULL;
            break;
            
        default:
            /* Simple types need no additional info */
            break;
    }
    
    return desc;
}

/**
 * @brief Free type descriptor
 */
void polycall_ffi_type_desc_free(polycall_ffi_type_desc_t* desc) {
    if (!desc) {
        return;
    }
    
    /* Free type-specific resources */
    switch (desc->type) {
        case POLYCALL_FFI_TYPE_STRUCT:
            if (desc->struct_info.field_names) {
                for (size_t i = 0; i < desc->struct_info.field_count; i++) {
                    free(desc->struct_info.field_names[i]);
                }
                free(desc->struct_info.field_names);
            }
            free(desc->struct_info.field_types);
            free(desc->struct_info.field_offsets);
            break;
            
        case POLYCALL_FFI_TYPE_FUNCTION:
            if (desc->function_info.signature) {
                free(desc->function_info.signature->name);
                free(desc->function_info.signature->param_types);
                free(desc->function_info.signature);
            }
            break;
            
        default:
            /* No additional cleanup needed */
            break;
    }
    
    free(desc);
}

/**
 * @brief Initialize FFI value with type
 */
void polycall_ffi_value_init(polycall_ffi_value_t* value, polycall_ffi_type_t type) {
    if (!value) {
        return;
    }
    
    memset(value, 0, sizeof(polycall_ffi_value_t));
    value->type = type;
    value->size = polycall_ffi_type_size(type);
}

/**
 * @brief Copy FFI value (deep copy for complex types)
 */
polycall_ffi_error_t polycall_ffi_value_copy(polycall_ffi_value_t* dest,
                                             const polycall_ffi_value_t* src) {
    if (!dest || !src) {
        return POLYCALL_FFI_ERROR_INVALID_TYPE;
    }
    
    /* Copy basic structure */
    memcpy(dest, src, sizeof(polycall_ffi_value_t));
    
    /* Deep copy for complex types */
    switch (src->type) {
        case POLYCALL_FFI_TYPE_STRING:
            if (src->value.string_val) {
                size_t len = strlen(src->value.string_val) + 1;
                dest->value.string_val = malloc(len);
                if (!dest->value.string_val) {
                    return POLYCALL_FFI_ERROR_MEMORY_ALLOCATION;
                }
                memcpy(dest->value.string_val, src->value.string_val, len);
            }
            break;
            
        case POLYCALL_FFI_TYPE_ARRAY:
            if (src->value.array_val.data && src->value.array_val.size > 0) {
                dest->value.array_val.data = malloc(src->value.array_val.size);
                if (!dest->value.array_val.data) {
                    return POLYCALL_FFI_ERROR_MEMORY_ALLOCATION;
                }
                memcpy(dest->value.array_val.data, 
                       src->value.array_val.data, 
                       src->value.array_val.size);
            }
            break;
            
        case POLYCALL_FFI_TYPE_STRUCT:
            /* Struct copying requires type descriptor for proper field handling */
            if (src->value.struct_val.fields && src->value.struct_val.count > 0) {
                /* This is a simplified copy - real implementation would need
                 * the type descriptor to properly copy each field */
                size_t struct_size = src->size;
                if (struct_size > 0) {
                    dest->value.struct_val.fields = malloc(struct_size);
                    if (!dest->value.struct_val.fields) {
                        return POLYCALL_FFI_ERROR_MEMORY_ALLOCATION;
                    }
                    memcpy(dest->value.struct_val.fields,
                           src->value.struct_val.fields,
                           struct_size);
                }
            }
            break;
            
        default:
            /* Simple types are already copied */
            break;
    }
    
    return POLYCALL_FFI_SUCCESS;
}

/**
 * @brief Free FFI value resources
 */
void polycall_ffi_value_free(polycall_ffi_value_t* value) {
    if (!value) {
        return;
    }
    
    /* Free allocated resources based on type */
    switch (value->type) {
        case POLYCALL_FFI_TYPE_STRING:
            free(value->value.string_val);
            value->value.string_val = NULL;
            break;
            
        case POLYCALL_FFI_TYPE_ARRAY:
            free(value->value.array_val.data);
            value->value.array_val.data = NULL;
            value->value.array_val.size = 0;
            break;
            
        case POLYCALL_FFI_TYPE_STRUCT:
            free(value->value.struct_val.fields);
            value->value.struct_val.fields = NULL;
            value->value.struct_val.count = 0;
            break;
            
        default:
            /* No cleanup needed for simple types */
            break;
    }
    
    /* Clear the value */
    memset(value, 0, sizeof(polycall_ffi_value_t));
}

/**
 * @brief Validate FFI signature
 */
bool polycall_ffi_signature_valid(const polycall_ffi_signature_t* sig) {
    if (!sig || !sig->name) {
        return false;
    }
    
    /* Validate return type */
    if (sig->return_type >= POLYCALL_FFI_TYPE_CUSTOM) {
        /* Custom types need additional validation */
        /* For now, accept them */
    }
    
    /* Validate parameter types */
    if (sig->param_count > 0 && !sig->param_types) {
        return false;
    }
    
    for (size_t i = 0; i < sig->param_count; i++) {
        if (sig->param_types[i] == POLYCALL_FFI_TYPE_VOID) {
            /* Void is not valid as parameter type */
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Compare FFI signatures for compatibility
 */
bool polycall_ffi_signatures_compatible(const polycall_ffi_signature_t* sig1,
                                       const polycall_ffi_signature_t* sig2) {
    if (!sig1 || !sig2) {
        return false;
    }
    
    /* Check return type compatibility */
    if (!polycall_ffi_types_compatible(sig1->return_type, sig2->return_type)) {
        return false;
    }
    
    /* Check parameter count (variadic functions are special case) */
    if (!sig1->is_variadic && !sig2->is_variadic) {
        if (sig1->param_count != sig2->param_count) {
            return false;
        }
    } else {
        /* At least one is variadic - check minimum params */
        size_t min_params = sig1->param_count < sig2->param_count ? 
                           sig1->param_count : sig2->param_count;
        
        /* For variadic, we only check the fixed parameters */
        for (size_t i = 0; i < min_params; i++) {
            if (!polycall_ffi_types_compatible(sig1->param_types[i], 
                                             sig2->param_types[i])) {
                return false;
            }
        }
        return true;
    }
    
    /* Check each parameter type */
    for (size_t i = 0; i < sig1->param_count; i++) {
        if (!polycall_ffi_types_compatible(sig1->param_types[i], 
                                         sig2->param_types[i])) {
            return false;
        }
    }
    
    return true;
}