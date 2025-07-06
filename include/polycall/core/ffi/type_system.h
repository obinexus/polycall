/**
#include <string.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

 * @file type_system.h
 * @brief Type system module for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the type system for the LibPolyCall FFI module, enabling 
 * safe and efficient type conversion between different language runtimes.
 * It provides canonical type representations and bidirectional mappings.
 */

#ifndef POLYCALL_FFI_TYPE_SYSTEM_H_H
#define POLYCALL_FFI_TYPE_SYSTEM_H_H

#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
 extern "C" {
#endif

   /**
    * @brief Type mapping context (opaque)
    */
   typedef struct type_mapping_context type_mapping_context_t;

   // Type mapping context structure
   typedef struct {
     ffi_type_info_t *types;           /**< Array of type information */
     size_t type_count;                /**< Number of registered types */
     size_t type_capacity;             /**< Capacity of the type array */
     mapping_rule_t *rules;            /**< Array of conversion rules */
     size_t rule_count;                /**< Number of registered rules */
     size_t rule_capacity;             /**< Capacity of the rule array */
     polycall_type_conv_flags_t flags; /**< Type system flags */
   } type_mapping_context_t;

   struct type_mapping_context {
     ffi_type_info_t *types;
     size_t type_count;
     size_t type_capacity;
     mapping_rule_t *rules;
     size_t rule_count;
     size_t rule_capacity;
     polycall_type_conv_flags_t flags;
   };

   /**
    * @brief Type conversion rule (opaque)
    */
   typedef struct mapping_rule mapping_rule_t;

   /**
    * @brief Type conversion registry (opaque)
    */
   typedef struct conversion_registry conversion_registry_t;

   /**
    * @brief Type conversion flags
    */
   typedef enum {
     POLYCALL_TYPE_CONV_FLAG_NONE = 0,
     POLYCALL_TYPE_CONV_FLAG_STRICT = (1 << 0),    /**< Strict type checking */
     POLYCALL_TYPE_CONV_FLAG_COPY = (1 << 1),      /**< Copy data */
     POLYCALL_TYPE_CONV_FLAG_NULLABLE = (1 << 2),  /**< Allow null values */
     POLYCALL_TYPE_CONV_FLAG_RECURSIVE = (1 << 3), /**< Recursive conversion */
     POLYCALL_TYPE_CONV_FLAG_REFERENCE = (1 << 4), /**< Reference semantics */
     POLYCALL_TYPE_CONV_FLAG_USER =
         (1 << 16) /**< Start of user-defined flags */
   } polycall_type_conv_flags_t;

   /**
    * @brief Type conversion rule entry
    */
   typedef struct {
     const char *source_language;
     polycall_ffi_type_t source_type;
     const char *target_language;
     polycall_ffi_type_t target_type;
     polycall_type_conv_flags_t flags;

     // Conversion functions
     polycall_core_error_t (*convert)(polycall_core_context_t *ctx,
                                      const void *src, void *dst,
                                      size_t *dst_size, void *user_data);

     polycall_core_error_t (*validate)(polycall_core_context_t *ctx,
                                       const void *data, size_t size,
                                       void *user_data);

     void *user_data;
   } type_conversion_rule_t;

   /**
    * @brief Type system configuration
    */
   typedef struct {
     size_t type_capacity; /**< Maximum number of type definitions */
     size_t rule_capacity; /**< Maximum number of conversion rules */
     polycall_type_conv_flags_t flags; /**< Type system flags */
     bool auto_register_primitives; /**< Automatically register primitive types
                                     */
     void *user_data;               /**< User data */
   } type_system_config_t;

   /**
    * @brief Initialize type system
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Pointer to receive type mapping context
    * @param config Type system configuration
    * @return Error code
    */
   polycall_core_error_t polycall_type_init(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * *type_ctx, const type_system_config_t *config);

   /**
    * @brief Clean up type system
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context to clean up
    */
   void polycall_type_cleanup(polycall_core_context_t * ctx,
                              polycall_ffi_context_t * ffi_ctx,
                              type_mapping_context_t * type_ctx);

   /**
    * @brief Register a type with the type system
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param type_info Type information
    * @param language Language for the type
    * @return Error code
    */
   polycall_core_error_t polycall_type_register(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, ffi_type_info_t * type_info,
       const char *language);

   /**
    * @brief Find a registered type by name
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param type_name Type name
    * @param language Language for the type
    * @param type_info Pointer to receive type information
    * @return Error code
    */
   polycall_core_error_t polycall_type_find_by_name(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const char *type_name,
       const char *language, ffi_type_info_t **type_info);

   /**
    * @brief Register a type conversion rule
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param rule Conversion rule
    * @return Error code
    */
   polycall_core_error_t polycall_type_register_conversion(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const type_conversion_rule_t *rule);

   /**
    * @brief Convert a value from one type to another
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param src Source value
    * @param src_language Source language
    * @param dst Destination value
    * @param dst_language Destination language
    * @param flags Conversion flags
    * @return Error code
    */
   polycall_core_error_t polycall_type_convert(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const ffi_value_t *src,
       const char *src_language, ffi_value_t *dst, const char *dst_language,
       polycall_type_conv_flags_t flags);

   /**
    * @brief Validate a value against a type
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param value Value to validate
    * @param language Language for validation
    * @return Error code
    */
   polycall_core_error_t polycall_type_validate(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const ffi_value_t *value,
       const char *language);

   /**
    * @brief Serialize a value to a buffer
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param value Value to serialize
    * @param buffer Buffer to receive serialized data
    * @param buffer_size Pointer to buffer size (in/out)
    * @return Error code
    */
   polycall_core_error_t polycall_type_serialize(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const ffi_value_t *value,
       void *buffer, size_t *buffer_size);

   /**
    * @brief Deserialize a value from a buffer
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param buffer Buffer containing serialized data
    * @param buffer_size Buffer size
    * @param value Pointer to receive deserialized value
    * @param type Expected type
    * @param language Language for deserialization
    * @return Error code
    */
   polycall_core_error_t polycall_type_deserialize(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const void *buffer,
       size_t buffer_size, ffi_value_t *value, polycall_ffi_type_t type,
       const char *language);

   /**
    * @brief Create a type descriptor for a struct
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param struct_name Struct name
    * @param fields Field type information array
    * @param field_names Field name array
    * @param field_count Field count
    * @param type_info Pointer to receive type information
    * @return Error code
    */
   polycall_core_error_t polycall_type_create_struct(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const char *struct_name,
       polycall_ffi_type_t *fields, const char **field_names,
       size_t field_count, ffi_type_info_t **type_info);

   /**
    * @brief Create a type descriptor for an array
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param element_type Element type
    * @param element_count Element count (0 for variable-length)
    * @param type_info Pointer to receive type information
    * @return Error code
    */
   polycall_core_error_t polycall_type_create_array(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, polycall_ffi_type_t element_type,
       size_t element_count, ffi_type_info_t * *type_info);

   /**
    * @brief Get canonical type for a language-specific type
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param type_name Language-specific type name
    * @param language Language
    * @param canonical_type Pointer to receive canonical type
    * @return Error code
    */
   polycall_core_error_t polycall_type_get_canonical(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, const char *type_name,
       const char *language, polycall_ffi_type_t *canonical_type);

   /**
    * @brief Get language-specific type for a canonical type
    *
    * @param ctx Core context
    * @param ffi_ctx FFI context
    * @param type_ctx Type mapping context
    * @param canonical_type Canonical type
    * @param language Language
    * @param type_name Pointer to receive language-specific type name
    * @return Error code
    */
   polycall_core_error_t polycall_type_get_language_specific(
       polycall_core_context_t * ctx, polycall_ffi_context_t * ffi_ctx,
       type_mapping_context_t * type_ctx, polycall_ffi_type_t canonical_type,
       const char *language, const char **type_name);

   /**
    * @brief Create a default type system configuration
    *
    * @return Default configuration
    */
   type_system_config_t polycall_type_create_default_config(void);

#ifdef __cplusplus
 }
#endif

#endif /* POLYCALL_FFI_TYPE_SYSTEM_H_H */