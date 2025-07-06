#ifndef POLYCALL_SHARED_CORE_H
#define POLYCALL_SHARED_CORE_H

/* Shared core definitions to break circular dependencies */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Core error type */
typedef int32_t polycall_core_error_t;

/* Error codes */
#define POLYCALL_SUCCESS               0
#define POLYCALL_ERROR_INVALID_PARAM  -1
#define POLYCALL_ERROR_OUT_OF_MEMORY  -2
#define POLYCALL_ERROR_NOT_INITIALIZED -3

/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;

#endif /* POLYCALL_SHARED_CORE_H */
