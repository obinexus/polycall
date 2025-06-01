/**
 * @file polycallfile_ignore.c
 * @brief Implementation of the ignore pattern system for Polycallfile
 * @author Based on Nnamdi Okpala's architecture design
 *
 * This file implements the ignore pattern system specifically for Polycallfile,
 * providing functionality to exclude specific files and directories from being
 * processed by the configuration system.
 */

 #include "polycall/core/config/polycallfile/ignore/polycallfile_ignore.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/config/path_utils.h"
 #include "polycall/core/config/ignore/polycall_ignore.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 
 #define POLYCALL_FILE_IGNORE_MAGIC 0xF11E1607
 #define DEFAULT_IGNORE_FILENAME "config.Polycallfile.ignore"
 
 /**
  * @brief Polycallfile ignore context structure
  */
 struct polycallfile_ignore_context {
     uint32_t magic;                       // Magic number for validation
     polycall_core_context_t* core_ctx;    // Core context
     polycall_ignore_context_t* ctx;       // Underlying ignore context
     char* ignore_file_path;               // Path to the loaded ignore file
 };
 
 /**
  * @brief Validate a Polycallfile ignore context
  */
 static bool validate_polycallfile_ignore_context(
     polycallfile_ignore_context_t* ctx
 ) {
     return ctx && ctx->magic == POLYCALL_FILE_IGNORE_MAGIC;
 }
 
 /**
  * @brief Initialize a Polycallfile ignore context
  */
 polycall_core_error_t polycallfile_ignore_init(
     polycall_core_context_t* core_ctx,
     polycallfile_ignore_context_t** ignore_ctx,
     bool case_sensitive
 ) {
     if (!core_ctx || !ignore_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycallfile_ignore_context_t* ctx = polycall_core_malloc(
         core_ctx,
         sizeof(polycallfile_ignore_context_t)
     );
     
     if (!ctx) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate Polycallfile ignore context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(ctx, 0, sizeof(polycallfile_ignore_context_t));
     ctx->magic = POLYCALL_FILE_IGNORE_MAGIC;
     ctx->core_ctx = core_ctx;
     
     // Create underlying ignore context
     polycall_core_error_t result = polycall_ignore_context_init(
         core_ctx,
         &ctx->ctx,
         case_sensitive
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(core_ctx, ctx);
         return result;
     }
     
     *ignore_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up a Polycallfile ignore context
  */
 void polycallfile_ignore_cleanup(
     polycall_core_context_t* core_ctx,
     polycallfile_ignore_context_t* ignore_ctx
 ) {
     if (!core_ctx || !validate_polycallfile_ignore_context(ignore_ctx)) {
         return;
     }
     
     // Clean up underlying ignore context
     if (ignore_ctx->ctx) {
         polycall_ignore_context_cleanup(core_ctx, ignore_ctx->ctx);
     }
     
     // Free ignore file path
     if (ignore_ctx->ignore_file_path) {
         polycall_core_free(core_ctx, ignore_ctx->ignore_file_path);
     }
     
     // Invalidate and free context
     ignore_ctx->magic = 0;
     polycall_core_free(core_ctx, ignore_ctx);
 }
 
 /**
  * @brief Add an ignore pattern
  */
 polycall_core_error_t polycallfile_ignore_add_pattern(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* pattern
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx) || !pattern) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return polycall_ignore_add_pattern(ignore_ctx->ctx, pattern);
 }
 
 /**
  * @brief Load ignore patterns from a file
  * 
  * If the specified file is not found, this function will try to locate
  * the default ignore file in the following order:
  * 1. "config.Polycallfile.ignore" in the same directory as the specified path
  * 2. "config.Polycallfile.ignore" in the current directory
  * 3. "config.Polycallfile.ignore" in the user's home directory
  */
 polycall_core_error_t polycallfile_ignore_load_file(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* file_path
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Store the ignore file path
     if (ignore_ctx->ignore_file_path) {
         polycall_core_free(ignore_ctx->core_ctx, ignore_ctx->ignore_file_path);
         ignore_ctx->ignore_file_path = NULL;
     }
     
     const char* path = file_path;
     char resolved_path[POLYCALL_MAX_PATH];
     
     // If no path provided, look for default locations
     if (!path) {
         // Try current directory
         if (polycall_path_resolve(
                 ignore_ctx->core_ctx,
                 DEFAULT_IGNORE_FILENAME,
                 resolved_path,
                 sizeof(resolved_path)) == POLYCALL_CORE_SUCCESS) {
             path = resolved_path;
         } else {
             // Try home directory
             char home_path[POLYCALL_MAX_PATH];
             if (polycall_path_get_home_directory(
                     home_path,
                     sizeof(home_path)) == POLYCALL_CORE_SUCCESS) {
                 
                 snprintf(resolved_path, sizeof(resolved_path),
                         "%s/%s", home_path, DEFAULT_IGNORE_FILENAME);
                 
                 if (polycall_path_file_exists(resolved_path)) {
                     path = resolved_path;
                 } else {
                     // No default ignore file found
                     return POLYCALL_CORE_SUCCESS;
                 }
             } else {
                 // No home directory found
                 return POLYCALL_CORE_SUCCESS;
             }
         }
     }
     
     // Store the path
     ignore_ctx->ignore_file_path = polycall_core_strdup(ignore_ctx->core_ctx, path);
     if (!ignore_ctx->ignore_file_path) {
         POLYCALL_ERROR_SET(ignore_ctx->core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate ignore file path");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Load patterns
     return polycall_ignore_load_file(ignore_ctx->ctx, path);
 }
 
 /**
  * @brief Check if a path should be ignored
  */
 bool polycallfile_ignore_should_ignore(
     polycallfile_ignore_context_t* ignore_ctx,
     const char* path
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx) || !path) {
         return false;
     }
     
     return polycall_ignore_should_ignore(ignore_ctx->ctx, path);
 }
 
 /**
  * @brief Get the number of loaded patterns
  */
 size_t polycallfile_ignore_get_pattern_count(
     polycallfile_ignore_context_t* ignore_ctx
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx)) {
         return 0;
     }
     
     return polycall_ignore_get_pattern_count(ignore_ctx->ctx);
 }
 
 /**
  * @brief Get a specific pattern
  */
 const char* polycallfile_ignore_get_pattern(
     polycallfile_ignore_context_t* ignore_ctx,
     size_t index
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx)) {
         return NULL;
     }
     
     return polycall_ignore_get_pattern(ignore_ctx->ctx, index);
 }
 
 /**
  * @brief Add standard ignore patterns
  * 
  * This adds common patterns that should be ignored in most Polycall projects,
  * such as build directories, temporary files, and security-sensitive files.
  */
 polycall_core_error_t polycallfile_ignore_add_defaults(
     polycallfile_ignore_context_t* ignore_ctx
 ) {
     if (!validate_polycallfile_ignore_context(ignore_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Development artifacts
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.git/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.vscode/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.idea/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/__pycache__/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.pyc");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.pyo");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.pyd");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.pytest_cache/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.coverage");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/node_modules/");
     
     // Build artifacts
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/build/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/dist/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.egg-info/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.so");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.dll");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.dylib");
     
     // Temporary files
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.tmp");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.bak");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.swp");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.log");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/logs/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/temp/");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/.DS_Store");
     
     // Security sensitive files
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.pem");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.key");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/*.crt");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/credentials.json");
     polycall_ignore_add_pattern(ignore_ctx->ctx, "**/secrets.json");
     
     return POLYCALL_CORE_SUCCESS;