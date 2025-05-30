/**
 * @file telemetry_error.c
 * @brief Error handling for telemetry module
 */

#include "polycall/core/telemetry/telemetry_error.h"

/**
 * Get error message for telemetry error code
 */
const char* telemetry_get_error_message(telemetry_error_t error) {
    switch (error) {
        case TELEMETRY_ERROR_SUCCESS:
            return "Success";
        case TELEMETRY_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case TELEMETRY_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case TELEMETRY_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
