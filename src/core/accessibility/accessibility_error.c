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
 * @file accessibility_error.c
 * @brief Error handling for accessibility module
 */

#include "polycall/core/accessibility/accessibility_error.h"

/**
 * Get error message for accessibility error code
 */
const char* accessibility_get_error_message(accessibility_error_t error) {
    switch (error) {
        case ACCESSIBILITY_ERROR_SUCCESS:
            return "Success";
        case ACCESSIBILITY_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case ACCESSIBILITY_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case ACCESSIBILITY_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
