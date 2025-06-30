/**
 * @file polycallfile_ignore.h
 * @brief Ignore pattern system for Polycallfile
 * @author Based on Nnamdi Okpala's architecture design
 *
 * This header defines the interface for the ignore pattern system specifically
 * for Polycallfile, which allows configuration to exclude certain files and
 * directories from being processed.
 */

 #ifndef POLYCALL_CONFIG_POLYCALLFILE_IGNORE_POLYCALLFILE_IGNORE_H_H
 #define POLYCALL_CONFIG_POLYCALLFILE_IGNORE_POLYCALLFILE_IGNORE_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include <stddef.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Polycallfile ignore context structure (opaque)
  */
 typedef struct polycallfile_ignore_context polycallfile_ignore_context_t;
 
 /**
  * @brief Initialize a Polycallfile ignore context
  *
  * @param core_ctx Core context
  * @param ignore_ctx Pointer to receive the ignore context
  * @param case_sensitive Whether pattern matching should be case-sensitive
  * @return Error code
  */
 polycall_core_error_t polycallfile_ignore_init(
     polycall_core_context_t* core_ctx,
     polycallfile_ignore_context_t** ignore_ctx,
     bool case_sensitive
 );
 
 /**
  * @brief Clean up a Polycallfile ignore context
  *
  * @param core_ctx Core context
  * @param ignore_ctx Ignore context to clean up
  */
 void polycallfile_ignore_cleanup(
     polycall_core_context_t* core_ctx,
     polycallfile_ignore_context_t* ignore_ctx
 );
 
 /**
  * @brief Add an ignore pattern
  *
  * @param ignore_ctx Ignore context
  * @param pattern Pattern to add (glob format)
  * @return Error code
  */
 polycall_core_error_t polycallfile_ignore_add_pattern(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* pattern
 );
 
 /**
  * @brief Load ignore patterns from a file
  *
  * @param ignore_ctx Ignore context
  * @param file_path Path to the ignore file, or NULL to use defaults
  * @return Error code
  */
 polycall_core_error_t polycallfile_ignore_load_file(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* file_path
 );
 
 /**
  * @brief Check if a path should be ignored
  *
  * @param ignore_ctx Ignore context
  * @param path Path to check
  * @return true if the path should be ignored, false otherwise
  */
 bool polycallfile_ignore_should_ignore(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* path
 );
 
 /**
  * @brief Get the number of loaded patterns
  *
  * @param ignore_ctx Ignore context
  * @return Number of patterns
  */
 size_t polycallfile_ignore_get_pattern_count(
     polycallfile_ignore_context_t* ignore_ctx
 );
 
 /**
  * @brief Get a specific pattern
  *
  * @param ignore_ctx Ignore context
  * @param index Pattern index
  * @return Pattern string, or NULL if index is out of bounds
  */
 const char* polycallfile_ignore_get_pattern(
     polycallfile_ignore_context_t* ignore_ctx,
     size_t index
 );
 
 /**
  * @brief Add standard ignore patterns
  *
  * Adds common patterns for build artifacts, temporary files, etc.
  *
  * @param ignore_ctx Ignore context
  * @return Error code
  */
 polycall_core_error_t polycallfile_ignore_add_defaults(
     polycallfile_ignore_context_t* ignore_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_POLYCALLFILE_IGNORE_POLYCALLFILE_IGNORE_H_H */