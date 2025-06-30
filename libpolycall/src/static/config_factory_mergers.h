#ifndef POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_MERGERS_H_H
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_MERGERS_H_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Configuration merge status codes
 */
typedef enum {
    CONFIG_MERGE_SUCCESS = 0,
    CONFIG_MERGE_FAILURE = -1,
    CONFIG_MERGE_INVALID_ARGS = -2,
    CONFIG_MERGE_MEMORY_ERROR = -3
} config_merge_status_t;

/**
 * @brief Configuration merge options
 */
typedef struct {
    bool override_existing;    // Whether to override existing values
    bool preserve_nulls;       // Whether to preserve null values during merge
    bool deep_copy;           // Whether to perform deep copy during merge
} config_merge_options_t;

/**
 * @brief Merges two configuration objects
 * 
 * @param dest Destination configuration object
 * @param source Source configuration object
 * @param options Merge options
 * @return config_merge_status_t Status code indicating success or failure
 */
config_merge_status_t config_merge(void* dest, const void* source, const config_merge_options_t* options);

/**
 * @brief Creates default merge options
 * 
 * @return config_merge_options_t Default merge options
 */
config_merge_options_t config_merge_default_options(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_MERGERS_H_H */