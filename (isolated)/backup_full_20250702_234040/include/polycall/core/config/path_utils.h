/**
 * @file path_utils.h
 * @brief Path utilities for LibPolyCall
 * @author Based on Nnamdi Okpala's architecture design
 *
 * This header defines utilities for path manipulation, resolution, and validation
 * used throughout the LibPolyCall system.
 */

 #ifndef POLYCALL_CONFIG_PATH_UTILS_H_H
 #define POLYCALL_CONFIG_PATH_UTILS_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Maximum path length supported by the system
  */
 #define POLYCALL_CONFIG_PATH_UTILS_H_H
 
 /**
  * @brief Resolve a path to its canonical form
  *
  * Resolves a path, which may be relative, to its absolute canonical form.
  * If the path is relative, it will be resolved relative to the current
  * working directory.
  *
  * @param core_ctx Core context
  * @param path Path to resolve
  * @param resolved_path Buffer to store the resolved path
  * @param resolved_path_size Size of the resolved_path buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_resolve(
     polycall_core_context_t* core_ctx,
     const char* path,
     char* resolved_path,
     size_t resolved_path_size
 );
 
 /**
  * @brief Get the user's home directory
  *
  * @param home_path Buffer to store the home directory path
  * @param home_path_size Size of the home_path buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_get_home_directory(
     char* home_path,
     size_t home_path_size
 );
 
 /**
  * @brief Check if a file exists
  *
  * @param path Path to the file
  * @return true if the file exists, false otherwise
  */
 bool polycall_path_file_exists(
     const char* path
 );
 
 /**
  * @brief Check if a path is a directory
  *
  * @param path Path to check
  * @return true if the path is a directory, false otherwise
  */
 bool polycall_path_is_directory(
     const char* path
 );
 
 /**
  * @brief Get the base name of a path
  *
  * Extracts the file name component of a path.
  *
  * @param path Full path
  * @param basename Buffer to store the base name
  * @param basename_size Size of the basename buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_get_basename(
     const char* path,
     char* basename,
     size_t basename_size
 );
 
 /**
  * @brief Get the directory name of a path
  *
  * Extracts the directory component of a path.
  *
  * @param path Full path
  * @param dirname Buffer to store the directory name
  * @param dirname_size Size of the dirname buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_get_dirname(
     const char* path,
     char* dirname,
     size_t dirname_size
 );
 
 /**
  * @brief Join two path components
  *
  * @param base Base path
  * @param component Path component to append
  * @param result Buffer to store the joined path
  * @param result_size Size of the result buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_join(
     const char* base,
     const char* component,
     char* result,
     size_t result_size
 );
 
 /**
  * @brief Normalize a path
  *
  * Removes redundant path separators, resolves "." and ".." components,
  * but doesn't convert to absolute path.
  *
  * @param path Path to normalize
  * @param normalized_path Buffer to store the normalized path
  * @param normalized_path_size Size of the normalized_path buffer
  * @return Error code
  */
 polycall_core_error_t polycall_path_normalize(
     const char* path,
     char* normalized_path,
     size_t normalized_path_size
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CONFIG_PATH_UTILS_H_H */