/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file config_error.c
 * @brief Error handling for config module
 */

#include "polycall/core/config/config_error.h"

/**
 * Get error message for config error code
 */
const char* config_get_error_message(config_error_t error) {
    switch (error) {
        case CONFIG_ERROR_SUCCESS:
            return "Success";
        case CONFIG_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case CONFIG_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case CONFIG_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
