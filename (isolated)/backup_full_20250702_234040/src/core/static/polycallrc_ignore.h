/**
 * @file polycallrc_ignore.h
 * @brief Header for the PolycallRC ignore pattern system
 * @author Based on Nnamdi Okpala's architecture design
 *
 * This file defines the interface for the PolycallRC-specific ignore pattern system,
 * which allows users to specify files and directories that should be ignored
 * by the binding configuration system during processing.
 */

 #ifndef POLYCALL_CONFIG_POLYCALLRC_IGNORE_POLYCALLRC_IGNORE_H_H
 #define POLYCALL_CONFIG_POLYCALLRC_IGNORE_POLYCALLRC_IGNORE_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Opaque type for PolycallRC ignore context
  */
 typedef struct polycallrc_ignore_context polycallrc_ignore_context_t;
 
 /**
  * @brief Initialize a PolycallRC ignore context
  * 
  * @param core_ctx Core context
  * @param ignore_ctx Pointer to receive the ignore context
  * @param case_sensitive Whether pattern matching is case sensitive
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycallrc_ignore_init(
     polycall_core_context_t* core_ctx,
     polycallrc_ignore_context_t** ignore_ctx,
     bool case_sensitive
 );
 
 /**
  * @brief Clean up a PolycallRC ignore context
  * 
  * @param core_ctx Core context
  * @param ignore_ctx Ignore context to clean up
  */
 void polycallrc_ignore_cleanup(
     polycall_core_context_t* core_ctx,
     polycallrc_ignore_context_t* ignore_ctx
 );
 
 /**
  * @brief Add an ignore pattern
  * 
  * @param ignore_ctx Ignore context
  * @param pattern Pattern to add
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycallrc_ignore_add_pattern(
     polycallrc_ignore_context_t* ignore_ctx,
     const char* pattern
 );
 
 /**
  * @brief Load ignore patterns from a file
  * 
  * @param ignore_ctx Ignore context
  * @param file_path Path to the ignore file (NULL for default)
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycallrc_ignore_load_file(
     polycallrc_ignore_context_t* ignore_ctx,
     const char* file_path
 );
 
 /**
  * @brief Check if a path should be ignored
  * 
  * @param ignore_ctx Ignore context
  * @param path Path to check
  * @return bool True if the path should be ignored, false otherwise
  */
 bool polycallrc_ignore_should_ignore(
     polycallrc_ignore_context_t* ignore_ctx,
     const char* path
 );
 
 /**
  * @brief Get the number of loaded patterns
  * 
  * @param ignore_ctx Ignore context
  * @return size_t Number of patterns
  */
 size_t polycallrc_ignore_get_pattern_count(
     polycallrc_ignore_context_t* ignore_ctx
 );
 
 /**
  * @brief Get a specific pattern
  * 
  * @param ignore_ctx Ignore context
  * @param index Pattern index
  * @return const char* Pattern string, or NULL if index is out of range
  */
 const char* polycallrc_ignore_get_pattern(
     polycallrc_ignore_context_t* ignore_ctx,
     size_t index
 );
 
 /**
  * @brief Add standard ignore patterns for binding configurations
  * 
  * This adds common patterns that should be ignored in binding configurations,
  * such as security-sensitive files, development artifacts, etc.
  * 
  * @param ignore_ctx Ignore context
  * @return polycall_core_error_t Success or error code
  */
 polycall_core_error_t polycallrc_ignore_add_defaults(
     polycallrc_ignore_context_t* ignore_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_POLYCALLRC_IGNORE_POLYCALLRC_IGNORE_H_H */