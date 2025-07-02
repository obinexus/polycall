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
 * @file network_error.c
 * @brief Error handling for network module
 */

#include "polycall/core/network/network_error.h"

/**
 * Get error message for network error code
 */
const char* network_get_error_message(network_error_t error) {
    switch (error) {
        case NETWORK_ERROR_SUCCESS:
            return "Success";
        case NETWORK_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case NETWORK_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case NETWORK_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
