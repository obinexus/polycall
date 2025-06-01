/**
 * @file polycall_ignore.c
 * @brief Implementation for Polycall's ignore pattern system
 * @author Based on Nnamdi Okpala's architecture design
 *
 * This file implements the ignore pattern system for Polycall, which allows
 * users to specify files and directories that should be ignored by the
 * configuration system during processing.
 */

#include "polycall/core/config/ignore/polycall_ignore.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/config/path_utils.h"

// Forward declarations
struct polycall_ignore_context;
struct polycallfile_ignore_context;
typedef struct polycall_ignore_context polycall_ignore_context_t;
typedef struct polycallfile_ignore_context polycallfile_ignore_context_t;
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <ctype.h>
 
#define POLYCALL_IGNORE_MAGIC 0xA6E115C3
#define POLYCALL_FILE_IGNORE_MAGIC 0xF11E1607
#define MAX_PATTERN_LENGTH 512
#define INITIAL_PATTERNS_CAPACITY 16
#define DEFAULT_IGNORE_FILENAME "config.Polycallfile.ignore"

/**
 * @brief Ignore context structure
 */
struct polycall_ignore_context {
    uint32_t magic;                    // Magic number for validation
    polycall_core_context_t* core_ctx; // Core context
    
    char** patterns;                   // Array of ignore patterns
    size_t pattern_count;              // Number of patterns
    size_t patterns_capacity;          // Capacity of patterns array
    
    char* ignore_file_path;            // Path to the loaded ignore file
    bool case_sensitive;               // Whether pattern matching is case sensitive
};

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
  * @brief Validate an ignore context
  */
 static bool validate_ignore_context(polycall_ignore_context_t* ctx) {
     return ctx && ctx->magic == POLYCALL_IGNORE_MAGIC;
 }
 
 /**
  * @brief Ensure capacity for adding a new pattern
  */
 static bool ensure_pattern_capacity(polycall_ignore_context_t* ctx) {
     if (ctx->pattern_count >= ctx->patterns_capacity) {
         size_t new_capacity = ctx->patterns_capacity == 0 ? 
                              INITIAL_PATTERNS_CAPACITY : ctx->patterns_capacity * 2;
         
         char** new_patterns = polycall_core_realloc(
             ctx->core_ctx, 
             ctx->patterns, 
             new_capacity * sizeof(char*)
         );
         
         if (!new_patterns) {
             return false;
         }
         
         ctx->patterns = new_patterns;
         ctx->patterns_capacity = new_capacity;
     }
     
     return true;
 }
 
 /**
  * @brief Initialize an ignore context
  */
 polycall_core_error_t polycall_ignore_context_init(
     polycall_core_context_t* core_ctx,
     polycall_ignore_context_t** ignore_ctx,
     bool case_sensitive
 ) {
     if (!core_ctx || !ignore_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_ignore_context_t* ctx = polycall_core_malloc(
         core_ctx, 
         sizeof(polycall_ignore_context_t)
     );
     
     if (!ctx) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate ignore context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(ctx, 0, sizeof(polycall_ignore_context_t));
     ctx->magic = POLYCALL_IGNORE_MAGIC;
     ctx->core_ctx = core_ctx;
     ctx->case_sensitive = case_sensitive;
     
     // Allocate initial patterns array
     ctx->patterns = polycall_core_malloc(
         core_ctx, 
         INITIAL_PATTERNS_CAPACITY * sizeof(char*)
     );
     
     if (!ctx->patterns) {
         polycall_core_free(core_ctx, ctx);
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate ignore patterns array");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     ctx->pattern_count = 0;
     ctx->patterns_capacity = INITIAL_PATTERNS_CAPACITY;
     
     *ignore_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up an ignore context
  */
 void polycall_ignore_context_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_ignore_context_t* ignore_ctx
 ) {
     if (!core_ctx || !validate_ignore_context(ignore_ctx)) {
         return;
     }
     
     // Free patterns
     if (ignore_ctx->patterns) {
         for (size_t i = 0; i < ignore_ctx->pattern_count; i++) {
             polycall_core_free(core_ctx, ignore_ctx->patterns[i]);
         }
         polycall_core_free(core_ctx, ignore_ctx->patterns);
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
 polycall_core_error_t polycall_ignore_add_pattern(
     polycall_ignore_context_t* ignore_ctx,
     const char* pattern
 ) {
     if (!validate_ignore_context(ignore_ctx) || !pattern) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check pattern length
     size_t pattern_len = strlen(pattern);
     if (pattern_len == 0 || pattern_len >= MAX_PATTERN_LENGTH) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Skip comment lines and empty lines
     if (pattern[0] == '#' || pattern[0] == '\0') {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Trim leading and trailing whitespace
     const char* start = pattern;
     while (isspace((unsigned char)*start)) {
         start++;
     }
     
     if (*start == '\0') {
         return POLYCALL_CORE_SUCCESS;  // Empty line after trimming
     }
     
     const char* end = pattern + pattern_len - 1;
     while (end > start && isspace((unsigned char)*end)) {
         end--;
     }
     
     size_t trimmed_len = end - start + 1;
     
     // Ensure capacity
     if (!ensure_pattern_capacity(ignore_ctx)) {
         POLYCALL_ERROR_SET(ignore_ctx->core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to expand ignore patterns array");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Allocate and copy pattern
     char* pattern_copy = polycall_core_malloc(ignore_ctx->core_ctx, trimmed_len + 1);
     if (!pattern_copy) {
         POLYCALL_ERROR_SET(ignore_ctx->core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate pattern string");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strncpy(pattern_copy, start, trimmed_len);
     pattern_copy[trimmed_len] = '\0';
     
     // Add pattern to array
     ignore_ctx->patterns[ignore_ctx->pattern_count++] = pattern_copy;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load ignore patterns from a file
  */
 polycall_core_error_t polycall_ignore_load_file(
     polycall_ignore_context_t* ignore_ctx,
     const char* file_path
 ) {
     if (!validate_ignore_context(ignore_ctx) || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Store the ignore file path
     if (ignore_ctx->ignore_file_path) {
         polycall_core_free(ignore_ctx->core_ctx, ignore_ctx->ignore_file_path);
     }
     
     ignore_ctx->ignore_file_path = polycall_core_strdup(ignore_ctx->core_ctx, file_path);
     if (!ignore_ctx->ignore_file_path) {
         POLYCALL_ERROR_SET(ignore_ctx->core_ctx, POLYCALL_ERROR_SOURCE_CONFIG,
                          POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "Failed to allocate ignore file path");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Open the file
     FILE* file = fopen(file_path, "r");
     if (!file) {
         // Not treating as an error - empty ignore list is valid
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Read patterns line by line
     char line[MAX_PATTERN_LENGTH];
     while (fgets(line, sizeof(line), file)) {
         // Remove newline character
         size_t len = strlen(line);
         if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
             line[len - 1] = '\0';
             
             // Also handle \r\n
             if (len > 1 && line[len - 2] == '\r') {
                 line[len - 2] = '\0';
             }
         }
         
         // Add pattern (function handles empty lines and comments)
         polycall_core_error_t result = polycall_ignore_add_pattern(ignore_ctx, line);
         if (result != POLYCALL_CORE_SUCCESS) {
             fclose(file);
             return result;
         }
     }
     
     fclose(file);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Pattern matching function for wildcards
  * 
  * Supports:
  * - '*' for any number of characters (including zero)
  * - '?' for exactly one character
  * - '**/' for recursive directory matching
  */
 static bool match_pattern(const char* pattern, const char* path, bool case_sensitive) {
     // Handle recursive directory pattern "**/"
     if (strncmp(pattern, "**/", 3) == 0) {
         // Try to match the remaining pattern with the current path
         if (match_pattern(pattern + 3, path, case_sensitive)) {
             return true;
         }
         
         // Try to match at each directory level
         const char* slash = strchr(path, '/');
         while (slash) {
             if (match_pattern(pattern, slash + 1, case_sensitive)) {
                 return true;
             }
             slash = strchr(slash + 1, '/');
         }
         
         return false;
     }
     
     // Simple wildcard matching algorithm
     const char* p = pattern;
     const char* s = path;
     
     const char* star = NULL;
     const char* ss = NULL;
     
     while (*s) {
         if (*p == '*') {
             // New star encountered, save position and advance pattern
             star = p++;
             ss = s;
         } else if (*p == '?' || (case_sensitive ? (*p == *s) : (tolower((unsigned char)*p) == tolower((unsigned char)*s)))) {
             // Character match, advance both
             p++;
             s++;
         } else if (star) {
             // No match, but have a previous star, backtrack to star position
             p = star + 1;
             s = ++ss;
         } else {
             // No match and no star to backtrack to
             return false;
         }
     }
     
     // Skip trailing stars
     while (*p == '*') {
         p++;
     }
     
     // Match successful if we've consumed the entire pattern
     return *p == '\0';
 }
 
 /**
  * @brief Check if a path should be ignored
  */
 bool polycall_ignore_should_ignore(
     polycall_ignore_context_t* ignore_ctx,
     const char* path
 ) {
     if (!validate_ignore_context(ignore_ctx) || !path) {
         return false;
     }
     
     // Normalize path separators to forward slashes
     char normalized_path[POLYCALL_MAX_PATH];
     size_t path_len = strlen(path);
     
     if (path_len >= POLYCALL_MAX_PATH) {
         return false;  // Path too long, don't ignore
     }
     
     // Normalize path (replace backslashes with forward slashes)
     for (size_t i = 0; i <= path_len; i++) {
         normalized_path[i] = path[i] == '\\' ? '/' : path[i];
     }
     
     // Check each pattern
     for (size_t i = 0; i < ignore_ctx->pattern_count; i++) {
         const char* pattern = ignore_ctx->patterns[i];
         
         // Check if the pattern is negated (starts with !)
         bool is_negated = pattern[0] == '!';
         if (is_negated) {
             pattern++; // Skip the negation character
             
             // If pattern matches, we explicitly don't ignore this path
             if (match_pattern(pattern, normalized_path, ignore_ctx->case_sensitive)) {
                 return false;
             }
         } else {
             // If pattern matches, we ignore this path
             if (match_pattern(pattern, normalized_path, ignore_ctx->case_sensitive)) {
                 return true;
             }
         }
     }
     
     // No matching pattern found, don't ignore
     return false;
 }
 
 /**
  * @brief Get the number of loaded patterns
  */
 size_t polycall_ignore_get_pattern_count(
     polycall_ignore_context_t* ignore_ctx
 ) {
     if (!validate_ignore_context(ignore_ctx)) {
         return 0;
     }
     
     return ignore_ctx->pattern_count;
 }
 
 /**
  * @brief Get a specific pattern
  */
 const char* polycall_ignore_get_pattern(
     polycall_ignore_context_t* ignore_ctx,
     size_t index
 ) {
     if (!validate_ignore_context(ignore_ctx) || index >= ignore_ctx->pattern_count) {
         return NULL;
     }
     
     return ignore_ctx->patterns[index];
 }

