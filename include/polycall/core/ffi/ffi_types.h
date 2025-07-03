/**
 * @file ffi_types.h
 * @brief Core type definitions for LibPolyCall FFI subsystem
 * @author OBINexus Engineering - Aegis Project Phase 2
 * 
 * This header establishes the foundational type system for cross-language
 * FFI operations with zero-trust security integration.
 */

#ifndef POLYCALL_FFI_TYPES_H
#define POLYCALL_FFI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations to prevent circular dependencies */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_ffi_context polycall_ffi_context_t;

/**
 * @brief FFI type enumeration for cross-language type mapping
 */
typedef enum {
    POLYCALL_FFI_TYPE_VOID = 0,
    POLYCALL_FFI_TYPE_INT8,
    POLYCALL_FFI_TYPE_UINT8,
    POLYCALL_FFI_TYPE_INT16,
    POLYCALL_FFI_TYPE_UINT16,
    POLYCALL_FFI_TYPE_INT32,
    POLYCALL_FFI_TYPE_UINT32,
    POLYCALL_FFI_TYPE_INT64,
    POLYCALL_FFI_TYPE_UINT64,
    POLYCALL_FFI_TYPE_FLOAT,
    POLYCALL_FFI_TYPE_DOUBLE,
    POLYCALL_FFI_TYPE_BOOL,
    POLYCALL_FFI_TYPE_CHAR,
    POLYCALL_FFI_TYPE_STRING,
    POLYCALL_FFI_TYPE_POINTER,
    POLYCALL_FFI_TYPE_ARRAY,
    POLYCALL_FFI_TYPE_STRUCT,
    POLYCALL_FFI_TYPE_FUNCTION,
    POLYCALL_FFI_TYPE_OPAQUE,
    POLYCALL_FFI_TYPE_CUSTOM = 0x1000  /**< Start of user-defined types */
} polycall_ffi_type_t;

/**
 * @brief FFI value union for type-safe value storage
 */
typedef union {
    int8_t int8_val;
    uint8_t uint8_val;
    int16_t int16_val;
    uint16_t uint16_val;
    int32_t int32_val;
    uint32_t uint32_val;
    int64_t int64_val;
    uint64_t uint64_val;
    float float_val;
    double double_val;
    bool bool_val;
    char char_val;
    char* string_val;
    void* ptr_val;
    struct {
        void* data;
        size_t size;
    } array_val;
    struct {
        void* fields;
        size_t count;
    } struct_val;
} polycall_ffi_value_union_t;

/**
 * @brief FFI value structure with type information
 */
typedef struct {
    polycall_ffi_type_t type;           /**< Value type */
    polycall_ffi_value_union_t value;   /**< Value data */
    size_t size;                        /**< Size in bytes */
    void* metadata;                     /**< Type-specific metadata */
} polycall_ffi_value_t;

/**
 * @brief FFI function signature
 */
typedef struct {
    char* name;                         /**< Function name */
    polycall_ffi_type_t return_type;    /**< Return type */
    polycall_ffi_type_t* param_types;   /**< Parameter types */
    size_t param_count;                 /**< Number of parameters */
    bool is_variadic;                   /**< Variadic function flag */
    void* metadata;                     /**< Language-specific metadata */
} polycall_ffi_signature_t;

/**
 * @brief FFI registry entry
 */
typedef struct {
    char* name;                         /**< Function name */
    polycall_ffi_signature_t* signature; /**< Function signature */
    void* function_ptr;                 /**< Function pointer */
    void* context;                      /**< Function context */
    uint32_t flags;                     /**< Function flags */
} polycall_ffi_registry_entry_t;

/**
 * @brief FFI registry structure
 */
typedef struct {
    polycall_ffi_registry_entry_t* entries; /**< Registry entries */
    size_t count;                           /**< Number of entries */
    size_t capacity;                        /**< Registry capacity */
    void* mutex;                            /**< Thread safety mutex */
} polycall_ffi_registry_t;

/**
 * @brief Type mapping entry for cross-language conversion
 */
typedef struct {
    polycall_ffi_type_t source_type;    /**< Source type */
    polycall_ffi_type_t target_type;    /**< Target type */
    const char* source_language;        /**< Source language */
    const char* target_language;        /**< Target language */
    void* converter_func;               /**< Conversion function */
    void* converter_data;               /**< Converter context */
} polycall_type_mapping_entry_t;

/**
 * @brief Type mapping context
 */
typedef struct {
    polycall_type_mapping_entry_t* mappings; /**< Type mappings */
    size_t count;                            /**< Number of mappings */
    size_t capacity;                         /**< Mapping capacity */
    void* cache;                             /**< Conversion cache */
} polycall_type_mapping_context_t;

/**
 * @brief Memory allocation function type
 */
typedef void* (*polycall_ffi_alloc_func_t)(size_t size, void* context);

/**
 * @brief Memory deallocation function type
 */
typedef void (*polycall_ffi_free_func_t)(void* ptr, void* context);

/**
 * @brief Memory manager for FFI operations
 */
typedef struct {
    polycall_ffi_alloc_func_t alloc;    /**< Allocation function */
    polycall_ffi_free_func_t free;      /**< Deallocation function */
    void* context;                      /**< Memory context */
    size_t allocated_bytes;             /**< Total allocated */
    size_t freed_bytes;                 /**< Total freed */
    uint32_t allocation_count;          /**< Number of allocations */
} polycall_memory_manager_t;

/**
 * @brief Security context for zero-trust FFI operations
 */
typedef struct {
    uint8_t challenge[32];              /**< Security challenge */
    uint8_t response[64];               /**< Challenge response */
    uint64_t timestamp;                 /**< Operation timestamp */
    uint32_t permissions;               /**< Permission flags */
    void* crypto_context;               /**< Cryptographic context */
} polycall_security_context_t;

/**
 * @brief FFI operation flags
 */
typedef enum {
    POLYCALL_FFI_FLAG_NONE = 0,
    POLYCALL_FFI_FLAG_ASYNC = (1 << 0),        /**< Asynchronous call */
    POLYCALL_FFI_FLAG_NO_COPY = (1 << 1),      /**< Zero-copy operation */
    POLYCALL_FFI_FLAG_CACHED = (1 << 2),       /**< Enable caching */
    POLYCALL_FFI_FLAG_TRACED = (1 << 3),       /**< Enable tracing */
    POLYCALL_FFI_FLAG_SECURE = (1 << 4),       /**< Require security */
    POLYCALL_FFI_FLAG_BATCHED = (1 << 5),      /**< Batch operation */
    POLYCALL_FFI_FLAG_PRIORITY = (1 << 6),     /**< High priority */
    POLYCALL_FFI_FLAG_VALIDATED = (1 << 7)     /**< Pre-validated */
} polycall_ffi_flags_t;

/**
 * @brief Language bridge interface
 */
typedef struct {
    const char* language_name;          /**< Language identifier */
    const char* version;                /**< Language version */
    
    /* Bridge functions */
    void* (*initialize)(polycall_core_context_t* ctx);
    void (*cleanup)(void* bridge_ctx);
    int (*call_function)(void* bridge_ctx, const char* name, 
                        polycall_ffi_value_t* args, size_t arg_count,
                        polycall_ffi_value_t* result);
    int (*register_function)(void* bridge_ctx, const char* name,
                           polycall_ffi_signature_t* sig, void* func);
    int (*get_type_info)(void* bridge_ctx, polycall_ffi_type_t type,
                        void** type_info);
    
    void* context;                      /**< Bridge-specific context */
} polycall_language_bridge_t;

/**
 * @brief FFI error codes
 */
typedef enum {
    POLYCALL_FFI_SUCCESS = 0,
    POLYCALL_FFI_ERROR_INVALID_TYPE,
    POLYCALL_FFI_ERROR_TYPE_MISMATCH,
    POLYCALL_FFI_ERROR_CONVERSION_FAILED,
    POLYCALL_FFI_ERROR_FUNCTION_NOT_FOUND,
    POLYCALL_FFI_ERROR_SIGNATURE_MISMATCH,
    POLYCALL_FFI_ERROR_MEMORY_ALLOCATION,
    POLYCALL_FFI_ERROR_SECURITY_VIOLATION,
    POLYCALL_FFI_ERROR_NOT_INITIALIZED,
    POLYCALL_FFI_ERROR_ALREADY_EXISTS,
    POLYCALL_FFI_ERROR_LANGUAGE_NOT_SUPPORTED,
    POLYCALL_FFI_ERROR_BRIDGE_FAILURE,
    POLYCALL_FFI_ERROR_TIMEOUT,
    POLYCALL_FFI_ERROR_CANCELLED,
    POLYCALL_FFI_ERROR_UNKNOWN = -1
} polycall_ffi_error_t;

/**
 * @brief FFI performance metrics
 */
typedef struct {
    uint64_t call_count;                /**< Total calls */
    uint64_t error_count;               /**< Total errors */
    uint64_t total_time_ns;             /**< Total execution time */
    uint64_t conversion_time_ns;        /**< Type conversion time */
    uint64_t security_check_time_ns;    /**< Security validation time */
    size_t memory_allocated;            /**< Memory allocated */
    size_t memory_freed;                /**< Memory freed */
} polycall_ffi_metrics_t;

/**
 * @brief Type descriptor for complex types
 */
typedef struct polycall_ffi_type_desc {
    polycall_ffi_type_t type;           /**< Base type */
    size_t size;                        /**< Type size in bytes */
    size_t alignment;                   /**< Type alignment */
    
    /* For arrays */
    struct {
        polycall_ffi_type_t element_type;
        size_t element_count;
        size_t element_size;
    } array_info;
    
    /* For structs */
    struct {
        char** field_names;
        polycall_ffi_type_t* field_types;
        size_t* field_offsets;
        size_t field_count;
    } struct_info;
    
    /* For functions */
    struct {
        polycall_ffi_signature_t* signature;
    } function_info;
    
} polycall_ffi_type_desc_t;

/**
 * @brief Get human-readable type name
 * 
 * @param type FFI type
 * @return Type name string
 */
const char* polycall_ffi_type_name(polycall_ffi_type_t type);

/**
 * @brief Get type size in bytes
 * 
 * @param type FFI type
 * @return Size in bytes, 0 for variable-size types
 */
size_t polycall_ffi_type_size(polycall_ffi_type_t type);

/**
 * @brief Check if types are compatible
 * 
 * @param type1 First type
 * @param type2 Second type
 * @return true if compatible, false otherwise
 */
bool polycall_ffi_types_compatible(polycall_ffi_type_t type1, 
                                  polycall_ffi_type_t type2);

/**
 * @brief Create type descriptor
 * 
 * @param type Base type
 * @return Type descriptor or NULL on error
 */
polycall_ffi_type_desc_t* polycall_ffi_type_desc_create(polycall_ffi_type_t type);

/**
 * @brief Free type descriptor
 * 
 * @param desc Type descriptor
 */
void polycall_ffi_type_desc_free(polycall_ffi_type_desc_t* desc);

/* Compatibility macros for legacy code */
#define FFI_TYPE_VOID    POLYCALL_FFI_TYPE_VOID
#define FFI_TYPE_INT     POLYCALL_FFI_TYPE_INT32
#define FFI_TYPE_FLOAT   POLYCALL_FFI_TYPE_FLOAT
#define FFI_TYPE_DOUBLE  POLYCALL_FFI_TYPE_DOUBLE
#define FFI_TYPE_BOOL    POLYCALL_FFI_TYPE_BOOL
#define FFI_TYPE_STRING  POLYCALL_FFI_TYPE_STRING
#define FFI_TYPE_POINTER POLYCALL_FFI_TYPE_POINTER

/* Ensure unique type definitions for this module */
#ifndef POLYCALL_FFI_TYPES_DEFINED
#define POLYCALL_FFI_TYPES_DEFINED

/* Type aliases for internal use */
typedef polycall_ffi_value_t ffi_value_t;
typedef polycall_ffi_signature_t ffi_signature_t;
typedef polycall_ffi_registry_t ffi_registry_t;
typedef polycall_type_mapping_context_t type_mapping_context_t;
typedef polycall_memory_manager_t memory_manager_t;
typedef polycall_security_context_t security_context_t;
typedef polycall_language_bridge_t language_bridge_t;

#endif /* POLYCALL_FFI_TYPES_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_TYPES_H */