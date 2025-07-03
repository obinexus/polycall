#ifndef POLYCALL_SHARED_CORE_H
#define POLYCALL_SHARED_CORE_H

// Error codes
typedef enum polycall_core_error {
    POLYCALL_CORE_SUCCESS = 0,
    POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
    POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
    POLYCALL_CORE_ERROR_NOT_FOUND,
    POLYCALL_CORE_ERROR_ACCESS_DENIED,
    POLYCALL_CORE_ERROR_TIMEOUT,
    // Add other error codes as needed
} polycall_core_error_t;

// Error severity
typedef enum polycall_error_severity {
    POLYCALL_ERROR_SEVERITY_INFO = 0,
    POLYCALL_ERROR_SEVERITY_WARNING,3
    POLYCALL_ERROR_SEVERITY_ERROR,
    POLYCALL_ERROR_SEVERITY_FATAL
} polycall_error_severity_t;

#endif // POLYCALL_SHARED_CORE_H
