/**
 * @file micro_error.c
 * @brief Error handling for micro module
 */

#include "polycall/core/micro/micro_error.h"

/**
 * Get error message for micro error code
 */
const char* micro_get_error_message(micro_error_t error) {
    switch (error) {
        case MICRO_ERROR_SUCCESS:
            return "Success";
        case MICRO_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case MICRO_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case MICRO_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
