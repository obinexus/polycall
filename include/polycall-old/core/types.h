#ifndef POLYCALL_CORE_TYPES_H
#define POLYCALL_CORE_TYPES_H

/* Standard includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>

/* Core error enumeration */
typedef enum {
    POLYCALL_SUCCESS = 0,
    POLYCALL_ERROR_INVALID_PARAM = -1,
    POLYCALL_ERROR_NO_MEMORY = -2,
    POLYCALL_ERROR_NOT_FOUND = -3,
    POLYCALL_ERROR_PERMISSION_DENIED = -4,
    POLYCALL_ERROR_LIMIT_EXCEEDED = -5,
    POLYCALL_ERROR_INTERNAL = -6,
} polycall_core_error_t;

/* Resource types */
typedef enum {
    POLYCALL_RESOURCE_MEMORY,
    POLYCALL_RESOURCE_CPU,
    POLYCALL_RESOURCE_IO,
} polycall_resource_type_t;

/* Component states */
typedef enum {
    POLYCALL_COMPONENT_INIT,
    POLYCALL_COMPONENT_READY,
    POLYCALL_COMPONENT_RUNNING,
    POLYCALL_COMPONENT_STOPPED,
    POLYCALL_COMPONENT_ERROR,
} polycall_component_state_t;

/* Isolation levels */
typedef enum {
    POLYCALL_ISOLATION_NONE,
    POLYCALL_ISOLATION_THREAD,
    POLYCALL_ISOLATION_PROCESS,
    POLYCALL_ISOLATION_CONTAINER,
} polycall_isolation_level_t;

/* Type definitions */
typedef uint32_t polycall_command_flags_t;

/* Forward declarations of core structures */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_micro_context polycall_micro_context_t;
typedef struct polycall_micro_component polycall_micro_component_t;
typedef struct polycall_micro_command polycall_micro_command_t;
typedef struct polycall_micro_config polycall_micro_config_t;

/* Function pointer types */
typedef polycall_core_error_t (*polycall_command_handler_t)(
    polycall_core_context_t* ctx,
    const void* params,
    void* result
);

typedef void (*resource_threshold_callback_t)(
    polycall_core_context_t* ctx,
    polycall_resource_type_t type,
    size_t current,
    size_t limit
);

typedef void (*component_event_callback_t)(
    polycall_core_context_t* ctx,
    polycall_micro_component_t* component,
    polycall_component_state_t old_state,
    polycall_component_state_t new_state
);

/* Common structures */
typedef struct polycall_resource_usage {
    size_t memory_usage;
    size_t peak_memory_usage;
    uint32_t cpu_usage;
    uint32_t peak_cpu_usage;
    uint32_t io_usage;
    uint32_t peak_io_usage;
} polycall_resource_usage_t;

#endif /* POLYCALL_CORE_TYPES_H */
