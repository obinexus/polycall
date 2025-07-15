/**
 * @file ffi_types.h
 * @brief Core FFI type definitions for LibPolyCall
 * @author OBINexus Engineering - Aegis Project Phase 2
 *
 * This header provides the fundamental type definitions for the FFI subsystem,
 * ensuring consistent type representation across all FFI modules.
 */

#ifndef POLYCALL_FFI_TYPES_H
#define POLYCALL_FFI_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FFI value types enumeration
 */
typedef enum {
  POLYCALL_FFI_TYPE_VOID = 0,
  POLYCALL_FFI_TYPE_BOOL,
  POLYCALL_FFI_TYPE_CHAR,
  POLYCALL_FFI_TYPE_UINT8,
  POLYCALL_FFI_TYPE_INT8,
  POLYCALL_FFI_TYPE_UINT16,
  POLYCALL_FFI_TYPE_INT16,
  POLYCALL_FFI_TYPE_UINT32,
  POLYCALL_FFI_TYPE_INT32,
  POLYCALL_FFI_TYPE_UINT64,
  POLYCALL_FFI_TYPE_INT64,
  POLYCALL_FFI_TYPE_FLOAT,
  POLYCALL_FFI_TYPE_DOUBLE,
  POLYCALL_FFI_TYPE_STRING,
  POLYCALL_FFI_TYPE_POINTER,
  POLYCALL_FFI_TYPE_STRUCT,
  POLYCALL_FFI_TYPE_ARRAY,
  POLYCALL_FFI_TYPE_FUNCTION,
  POLYCALL_FFI_TYPE_CALLBACK,
  POLYCALL_FFI_TYPE_OBJECT,
  POLYCALL_FFI_TYPE_OPAQUE,
  POLYCALL_FFI_TYPE_CUSTOM = 0x1000,
  POLYCALL_FFI_TYPE_USER = 0x2000
} polycall_ffi_type_t;

/**
 * @brief FFI error codes
 */
typedef enum {
  POLYCALL_FFI_SUCCESS = 0,
  POLYCALL_FFI_ERROR_INVALID_TYPE,
  POLYCALL_FFI_ERROR_CONVERSION_FAILED,
  POLYCALL_FFI_ERROR_MEMORY_ALLOCATION,
  POLYCALL_FFI_ERROR_INVALID_ARGUMENT,
  POLYCALL_FFI_ERROR_NOT_SUPPORTED,
  POLYCALL_FFI_ERROR_OVERFLOW,
  POLYCALL_FFI_ERROR_UNDERFLOW,
  POLYCALL_FFI_ERROR_SECURITY_VIOLATION,
  POLYCALL_FFI_ERROR_LANGUAGE_BRIDGE
} polycall_ffi_error_t;

/**
 * @brief FFI flags for various operations
 */
typedef enum {
  POLYCALL_FFI_FLAG_NONE = 0,
  POLYCALL_FFI_FLAG_ASYNC = (1 << 0),
  POLYCALL_FFI_FLAG_CACHED = (1 << 1),
  POLYCALL_FFI_FLAG_SECURE = (1 << 2),
  POLYCALL_FFI_FLAG_VALIDATED = (1 << 3),
  POLYCALL_FFI_FLAG_TRACE = (1 << 4),
  POLYCALL_FFI_FLAG_PERF_OPT = (1 << 5)
} polycall_ffi_flags_t;

/* Forward declarations */
typedef struct polycall_ffi_value polycall_ffi_value_t;
typedef struct polycall_ffi_type_desc polycall_ffi_type_desc_t;
typedef struct polycall_ffi_signature polycall_ffi_signature_t;

/**
 * @brief FFI value container structure
 */
struct polycall_ffi_value {
  polycall_ffi_type_t type;
  size_t size;
  uint32_t flags;
  union {
    bool bool_val;
    char char_val;
    uint8_t uint8_val;
    int8_t int8_val;
    uint16_t uint16_val;
    int16_t int16_val;
    uint32_t uint32_val;
    int32_t int32_val;
    uint64_t uint64_val;
    int64_t int64_val;
    float float_val;
    double double_val;
    char *string_val;
    void *pointer_val;
    void *struct_val;
    void *array_val;
    void *function_val;
    void *object_val;
    void *opaque_val;
    void *custom_val;
  } value;
  polycall_ffi_type_desc_t *type_desc;
};

/**
 * @brief Type descriptor structure
 */
struct polycall_ffi_type_desc {
  polycall_ffi_type_t type;
  size_t size;
  size_t alignment;
  char *name;
  uint32_t flags;

  union {
    /* Array type information */
    struct {
      polycall_ffi_type_t element_type;
      size_t element_count;
      size_t element_size;
    } array_info;

    /* Struct type information */
    struct {
      char **field_names;
      polycall_ffi_type_t *field_types;
      size_t *field_offsets;
      size_t field_count;
    } struct_info;

    /* Function type information */
    struct {
      polycall_ffi_signature_t *signature;
    } function_info;

    /* Custom type information */
    struct {
      uint32_t type_id;
      void *custom_data;
      void (*destructor)(void *);
    } custom_info;
  };
};

/**
 * @brief Function signature structure
 */
struct polycall_ffi_signature {
  char *name;
  polycall_ffi_type_t return_type;
  polycall_ffi_type_t *param_types;
  char **param_names;
  size_t param_count;
  bool variadic;
  uint32_t flags;
};

/**
 * @brief Type conversion context
 */
typedef struct {
  polycall_ffi_type_t source_type;
  polycall_ffi_type_t target_type;
  void *conversion_data;
  uint32_t flags;
} polycall_ffi_conversion_t;

/* Type utility functions */

/**
 * @brief Get human-readable name for FFI type
 *
 * @param type FFI type
 * @return Type name string
 */
const char *polycall_ffi_type_name(polycall_ffi_type_t type);

/**
 * @brief Get size of FFI type in bytes
 *
 * @param type FFI type
 * @return Size in bytes, 0 for variable-size types
 */
size_t polycall_ffi_type_size(polycall_ffi_type_t type);

/**
 * @brief Check if two types are compatible
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
 * @param type FFI type
 * @return Type descriptor or NULL on failure
 */
polycall_ffi_type_desc_t *
polycall_ffi_type_desc_create(polycall_ffi_type_t type);

/**
 * @brief Free type descriptor
 *
 * @param desc Type descriptor to free
 */
void polycall_ffi_type_desc_free(polycall_ffi_type_desc_t *desc);

/**
 * @brief Initialize FFI value with type
 *
 * @param value Value to initialize
 * @param type FFI type
 */
void polycall_ffi_value_init(polycall_ffi_value_t *value,
                             polycall_ffi_type_t type);

/**
 * @brief Copy FFI value (deep copy for complex types)
 *
 * @param dest Destination value
 * @param src Source value
 * @return Error code
 */
polycall_ffi_error_t polycall_ffi_value_copy(polycall_ffi_value_t *dest,
                                             const polycall_ffi_value_t *src);

/**
 * @brief Free FFI value resources
 *
 * @param value Value to free
 */
void polycall_ffi_value_free(polycall_ffi_value_t *value);

/**
 * @brief Convert value between types
 *
 * @param src Source value
 * @param target_type Target type
 * @param dest Destination value
 * @return Error code
 */
polycall_ffi_error_t polycall_ffi_value_convert(const polycall_ffi_value_t *src,
                                                polycall_ffi_type_t target_type,
                                                polycall_ffi_value_t *dest);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_TYPES_H */