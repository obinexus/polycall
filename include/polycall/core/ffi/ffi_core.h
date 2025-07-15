/**
 * @file ffi_core.h
 * @brief Core Foreign Function Interface module for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the core FFI functionality for LibPolyCall, enabling
 * cross-language interoperability with the Program-First design philosophy.
 * It provides the foundation for language bridges, type conversion, and
 * function dispatch across language boundaries.
 */

#ifndef POLYCALL_FFI_CORE_H
#define POLYCALL_FFI_CORE_H

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Core includes - order matters */
#include "polycall/core/polycall.h"
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_types.h"

/* FFI type definitions must come before other FFI includes */
#include "polycall/core/ffi/ffi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FFI module version information
 */
#define POLYCALL_FFI_VERSION_MAJOR 2
#define POLYCALL_FFI_VERSION_MINOR 0
#define POLYCALL_FFI_VERSION_PATCH 0
#define POLYCALL_FFI_VERSION_STRING "2.0.0"

/**
 * @brief FFI context type identifier
 */
#define POLYCALL_FFI_CONTEXT_TYPE_ID 0xFF100001

/* Forward declarations */
typedef struct polycall_ffi_context polycall_ffi_context_t;
typedef struct ffi_registry ffi_registry_t;
typedef struct type_mapping_context type_mapping_context_t;
typedef struct memory_manager memory_manager_t;
typedef struct security_context security_context_t;
typedef struct language_bridge language_bridge_t;
typedef struct ffi_value ffi_value_t;
typedef struct ffi_type_info ffi_type_info_t;
typedef struct ffi_signature ffi_signature_t;
typedef struct call_cache call_cache_t;
typedef struct type_cache type_cache_t;

/**
 * @brief FFI type information structure
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
 * @brief FFI value container
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
  ffi_type_info_t *type_info;
};

/**
 * @brief Function signature
 */
struct ffi_signature {
  polycall_ffi_type_t return_type;
  ffi_type_info_t *return_type_info;
  size_t param_count;
  polycall_ffi_type_t *param_types;
  ffi_type_info_t **param_type_infos;
  const char **param_names;
  bool *param_optional;
  bool variadic;
};

/**
 * @brief Language bridge interface
 */
struct language_bridge {
  const char *language_name;
  const char *version;

  /* Type conversion functions */
  polycall_core_error_t (*convert_to_native)(polycall_core_context_t *ctx,
                                             const ffi_value_t *src, void *dest,
                                             ffi_type_info_t *dest_type);

  polycall_core_error_t (*convert_from_native)(polycall_core_context_t *ctx,
                                               const void *src,
                                               ffi_type_info_t *src_type,
                                               ffi_value_t *dest);

  /* Function handling */
  polycall_core_error_t (*register_function)(polycall_core_context_t *ctx,
                                             const char *function_name,
                                             void *function_ptr,
                                             ffi_signature_t *signature,
                                             uint32_t flags);

  polycall_core_error_t (*call_function)(polycall_core_context_t *ctx,
                                         const char *function_name,
                                         ffi_value_t *args, size_t arg_count,
                                         ffi_value_t *result);

  /* Memory management */
  polycall_core_error_t (*acquire_memory)(polycall_core_context_t *ctx,
                                          void *ptr, size_t size);

  polycall_core_error_t (*release_memory)(polycall_core_context_t *ctx,
                                          void *ptr);

  /* Exception handling */
  polycall_core_error_t (*handle_exception)(polycall_core_context_t *ctx,
                                            void *exception, char *message,
                                            size_t message_size);

  /* Lifecycle */
  polycall_core_error_t (*initialize)(polycall_core_context_t *ctx);

  void (*cleanup)(polycall_core_context_t *ctx);

  void *user_data;
};

/**
 * @brief FFI context structure
 */
struct polycall_ffi_context {
  polycall_context_ref_t context_ref; /* Reference for context system */
  polycall_core_context_t *core_ctx;  /* Core context reference */
  ffi_registry_t *registry;           /* Function registry */
  type_mapping_context_t *type_ctx;   /* Type mapping context */
  memory_manager_t *memory_mgr;       /* Memory manager */
  security_context_t *security_ctx;   /* Security context */
  polycall_ffi_flags_t flags;         /* FFI flags */
  void *user_data;                    /* User data */
};

/* Core FFI API functions */

/**
 * @brief Create FFI context
 *
 * @param ctx Core context
 * @param ffi_ctx Pointer to receive FFI context
 * @param flags FFI flags
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_create_context(polycall_core_context_t *ctx,
                            polycall_ffi_context_t **ffi_ctx,
                            polycall_ffi_flags_t flags);

/**
 * @brief Destroy FFI context
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context to destroy
 */
void polycall_ffi_destroy_context(polycall_core_context_t *ctx,
                                  polycall_ffi_context_t *ffi_ctx);

/**
 * @brief Register language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Language bridge interface
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_register_bridge(polycall_core_context_t *ctx,
                             polycall_ffi_context_t *ffi_ctx,
                             const language_bridge_t *bridge);

/**
 * @brief Register function
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param language Language name
 * @param function_name Function name
 * @param function_ptr Function pointer
 * @param signature Function signature
 * @param flags Function flags
 * @return Error code
 */
polycall_core_error_t polycall_ffi_register_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    const char *language, const char *function_name, void *function_ptr,
    ffi_signature_t *signature, uint32_t flags);

/**
 * @brief Call function
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param language Target language
 * @param function_name Function name
 * @param args Arguments array
 * @param arg_count Argument count
 * @param result Pointer to receive result
 * @return Error code
 */
polycall_core_error_t polycall_ffi_call_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    const char *language, const char *function_name, ffi_value_t *args,
    size_t arg_count, ffi_value_t *result);

/**
 * @brief Create a FFI value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param type Value type
 * @param value Pointer to receive created value
 * @return Error code
 */
polycall_core_error_t polycall_ffi_create_value(polycall_core_context_t *ctx,
                                                polycall_ffi_context_t *ffi_ctx,
                                                polycall_ffi_type_t type,
                                                ffi_value_t **value);

/**
 * @brief Destroy a FFI value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param value Value to destroy
 */
void polycall_ffi_destroy_value(polycall_core_context_t *ctx,
                                polycall_ffi_context_t *ffi_ctx,
                                ffi_value_t *value);

/**
 * @brief Set FFI value data
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param value FFI value to update
 * @param data Data pointer
 * @param size Data size
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_set_value_data(polycall_core_context_t *ctx,
                            polycall_ffi_context_t *ffi_ctx, ffi_value_t *value,
                            const void *data, size_t size);

/**
 * @brief Get FFI value data
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param value FFI value to query
 * @param data Pointer to receive data pointer
 * @param size Pointer to receive data size
 * @return Error code
 */
polycall_core_error_t polycall_ffi_get_value_data(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    const ffi_value_t *value, void **data, size_t *size);

/**
 * @brief Get FFI context information
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param language_count Pointer to receive language count
 * @param function_count Pointer to receive function count
 * @param type_count Pointer to receive type count
 * @return Error code
 */
polycall_core_error_t polycall_ffi_get_info(polycall_core_context_t *ctx,
                                            polycall_ffi_context_t *ffi_ctx,
                                            size_t *language_count,
                                            size_t *function_count,
                                            size_t *type_count);

/**
 * @brief Get FFI version string
 *
 * @return Version string
 */
const char *polycall_ffi_get_version(void);

/* Internal function declarations for performance module */
/* (Removed unused static inline function declarations to avoid warnings and
 * errors) */

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_CORE_H */
