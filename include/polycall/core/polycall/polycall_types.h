#ifndef POLYCALL_TYPES_H
#define POLYCALL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Core error type definition */
typedef enum {
    POLYCALL_SUCCESS = 0,
    POLYCALL_ERROR_INVALID_PARAM = -1,
    POLYCALL_ERROR_OUT_OF_MEMORY = -2,
    POLYCALL_ERROR_NOT_INITIALIZED = -3,
    POLYCALL_ERROR_ALREADY_EXISTS = -4,
    POLYCALL_ERROR_NOT_FOUND = -5,
    POLYCALL_ERROR_PERMISSION_DENIED = -6,
    POLYCALL_ERROR_IO_FAILURE = -7,
    POLYCALL_ERROR_TIMEOUT = -8,
    POLYCALL_ERROR_UNKNOWN = -999
} polycall_core_error_t;

/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;

#endif /* POLYCALL_TYPES_H */
