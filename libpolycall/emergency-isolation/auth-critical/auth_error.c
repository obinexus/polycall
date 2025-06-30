/**
 * @file auth_error.c
 * @brief Error handling for auth module
 */

#include "polycall/core/auth/auth_error.h"

/**
 * Get error message for auth error code
 */
const char* auth_get_error_message(auth_error_t error) {
    switch (error) {
        case AUTH_ERROR_SUCCESS:
            return "Success";
        case AUTH_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case AUTH_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case AUTH_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
