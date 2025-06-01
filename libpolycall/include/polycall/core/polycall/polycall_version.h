/**
 * @file polycall_version.h
 * @brief Version information for LibPolyCall
 * 
 * This header defines version macros and functions for LibPolyCall.
 * It is generated from polycall_version.h.in during the build process.
 */

#ifndef POLYCALL_VERSION_H
#define POLYCALL_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LibPolyCall version information
 */
#define POLYCALL_VERSION          "@PROJECT_VERSION@"
#define POLYCALL_VERSION_MAJOR    @PROJECT_VERSION_MAJOR@
#define POLYCALL_VERSION_MINOR    @PROJECT_VERSION_MINOR@
#define POLYCALL_VERSION_PATCH    @PROJECT_VERSION_PATCH@

/**
 * Version number as a single integer (major * 10000 + minor * 100 + patch)
 * This can be used for numeric version comparisons
 */
#define POLYCALL_VERSION_NUMBER \
    (@PROJECT_VERSION_MAJOR@ * 10000 + @PROJECT_VERSION_MINOR@ * 100 + @PROJECT_VERSION_PATCH@)

/**
 * Build information
 */
#define POLYCALL_BUILD_DATE       "@POLYCALL_BUILD_DATE@"
#define POLYCALL_BUILD_TIME       "@POLYCALL_BUILD_TIME@"
#define POLYCALL_BUILD_SYSTEM     "@CMAKE_SYSTEM_NAME@"
#define POLYCALL_BUILD_PROCESSOR  "@CMAKE_SYSTEM_PROCESSOR@"

/**
 * Get the LibPolyCall version string
 * 
 * @return A string containing the version information
 */
const char* polycall_get_version(void);

/**
 * Check if the current version is at least the specified version
 * 
 * @param major Major version component
 * @param minor Minor version component
 * @param patch Patch version component
 * @return 1 if current version is at least the specified version, 0 otherwise
 */
int polycall_check_version(int major, int minor, int patch);

/**
 * Get the build date string
 * 
 * @return A string containing the build date
 */
const char* polycall_get_build_date(void);

/**
 * Get the build time string
 * 
 * @return A string containing the build time
 */
const char* polycall_get_build_time(void);

/**
 * Get the build system string
 * 
 * @return A string containing the build system information
 */
const char* polycall_get_build_system(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_VERSION_H */