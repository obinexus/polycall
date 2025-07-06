/**
 * @file language_bridge.h
 * @brief Generic language bridge interface for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the generic language bridge interface that all language
 * bridges must implement, establishing a foundation for cross-language
 * interoperability within the FFI system.
 */

#ifndef POLYCALL_FFI_LANGUAGE_BRIDGE_H_H
#define POLYCALL_FFI_LANGUAGE_BRIDGE_H_H

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Forward declaration of FFI value type
 */
typedef struct ffi_value ffi_value_t;

/**
 * @brief Forward declaration of FFI type info
 */
typedef struct ffi_type_info ffi_type_info_t;

/**
 * @brief Forward declaration of FFI signature
 */
typedef struct ffi_signature ffi_signature_t;

/**
 * @brief Language bridge interface
 */
typedef struct {
  const char *language_name;
  const char *version;

  // Type conversion functions
  polycall_core_error_t (*convert_to_native)(polycall_core_context_t *ctx,
                                             const ffi_value_t *src, void *dest,
                                             ffi_type_info_t *dest_type);

  polycall_core_error_t (*convert_from_native)(polycall_core_context_t *ctx,
                                               const void *src,
                                               ffi_type_info_t *src_type,
                                               ffi_value_t *dest);

  // Function handling
  polycall_core_error_t (*register_function)(polycall_core_context_t *ctx,
                                             const char *function_name,
                                             void *function_ptr,
                                             ffi_signature_t *signature,
                                             uint32_t flags);

  polycall_core_error_t (*call_function)(polycall_core_context_t *ctx,
                                         const char *function_name,
                                         ffi_value_t *args, size_t arg_count,
                                         ffi_value_t *result);

  // Memory management
  polycall_core_error_t (*acquire_memory)(polycall_core_context_t *ctx,
                                          void *ptr, size_t size);

  polycall_core_error_t (*release_memory)(polycall_core_context_t *ctx,
                                          void *ptr);

  // Exception handling
  polycall_core_error_t (*handle_exception)(polycall_core_context_t *ctx,
                                            void *exception, char *message,
                                            size_t message_size);

  // Lifecycle
  polycall_core_error_t (*initialize)(polycall_core_context_t *ctx);

  void (*cleanup)(polycall_core_context_t *ctx);

  void *user_data;
} language_bridge_t;

/**
 * @brief Structure definitions for FFI-related types
 */

/**
 * @brief FFI value container definition
 */
struct ffi_value {
  polycall_ffi_type_t type;
  union {
    bool bool_value;
    char char_value;
    uint8_t uint8_value;
    int8_t int8_value;
    uint16_t uint16_value;
    int16_t int16_value;
    uint32_t uint32_value;
    int32_t int32_value;
    uint64_t uint64_value;
    int64_t int64_value;
    float float_value;
    double double_value;
    const char *string_value;
    void *pointer_value;
    void *struct_value;
    void *array_value;
    void *callback_value;
    void *object_value;
    void *user_value;
  } value;
  struct ffi_type_info *type_info;
};

/**
 * @brief FFI type information
 */
struct ffi_type_info {
  polycall_ffi_type_t type;
  union {
    struct {
      const char *name;
      size_t size;
      size_t alignment;
      void *type_info;
      size_t field_count;
      polycall_ffi_type_t *types;
      const char **names;
      size_t *offsets;
    } struct_info;

    struct {
      polycall_ffi_type_t element_type;
      size_t element_count;
      void *type_info;
    } array_info;

    struct {
      polycall_ffi_type_t return_type;
      size_t param_count;
      polycall_ffi_type_t *param_types;
    } callback_info;

    struct {
      const char *type_name;
      void *type_info;
    } object_info;

    struct {
      uint32_t type_id;
      void *type_info;
    } user_info;
  } details;
};

/**
 * @brief Function signature
 */
struct ffi_signature {
  polycall_ffi_type_t return_type;
  struct ffi_type_info *return_type_info;
  size_t param_count;
  polycall_ffi_type_t *param_types;
  struct ffi_type_info **param_type_infos;
  const char **param_names;
  bool *param_optional;
  bool variadic;
};

/**
 * @brief Type conversion rule
 */
typedef struct mapping_rule {
  const char *source_language;
  polycall_ffi_type_t source_type;
  const char *target_language;
  polycall_ffi_type_t target_type;
  void *converter_function;
  void *user_data;
  struct mapping_rule *next;
} mapping_rule_t;

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_LANGUAGE_BRIDGE_H_H */