/**
 * @file protocol_error.c
 * @brief Error handling for protocol module
 */

#include "polycall/core/protocol/protocol_error.h"

/**
 * Get error message for protocol error code
 */
const char* protocol_get_error_message(protocol_error_t error) {
    switch (error) {
        case PROTOCOL_ERROR_SUCCESS:
            return "Success";
        case PROTOCOL_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case PROTOCOL_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case PROTOCOL_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
