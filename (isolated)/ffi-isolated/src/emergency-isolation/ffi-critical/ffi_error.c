/**
 * @file ffi_error.c
 * @brief Error handling for ffi module
 */

#include "polycall/core/ffi/ffi_error.h"

/**
 * Get error message for ffi error code
 */
const char* ffi_get_error_message(ffi_error_t error) {
    switch (error) {
        case FFI_ERROR_SUCCESS:
            return "Success";
        case FFI_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case FFI_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case FFI_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        // Add component-specific error messages here
        default:
            return "Unknown error";
    }
}
