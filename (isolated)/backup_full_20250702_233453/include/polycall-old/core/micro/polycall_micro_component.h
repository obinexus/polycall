#ifndef POLYCALL_MICRO_COMPONENT_H
#define POLYCALL_MICRO_COMPONENT_H

#include "polycall/core/types.h"

/* Resource limiter structure */
typedef struct resource_limiter {
    size_t memory_quota;
    size_t memory_usage;
    size_t peak_memory_usage;
    uint32_t cpu_quota;
    uint32_t cpu_usage;
    uint32_t peak_cpu_usage;
    uint32_t io_quota;
    uint32_t io_usage;
    uint32_t peak_io_usage;
    bool enforce_limits;
    bool track_usage;
    uint32_t limit_violations;
    uint32_t memory_allocations;
    uint32_t memory_frees;
    void* threshold_callbacks;
    size_t threshold_callback_count;
    pthread_mutex_t lock;
} resource_limiter_t;

/* Security structures */
typedef struct command_security_attributes {
    uint32_t flags;
    uid_t owner;
    gid_t group;
    mode_t permissions;
} command_security_attributes_t;

typedef struct component_security_context {
    uid_t uid;
    gid_t gid;
    char* security_label;
} component_security_context_t;

/* Registry structures */
typedef struct component_registry {
    void* components;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} component_registry_t;

typedef struct security_policy {
    uint32_t flags;
    bool enforce_isolation;
} security_policy_t;

/* Micro config structure */
struct polycall_micro_config {
    size_t max_components;
    size_t max_commands;
    bool enable_async;
    bool enable_isolation;
};

/* Function declarations */
polycall_core_error_t polycall_micro_create_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t** component,
    const char* name,
    polycall_isolation_level_t isolation_level
);

polycall_core_error_t polycall_micro_destroy_component(
    polycall_core_context_t* core_ctx,
    polycall_micro_context_t* micro_ctx,
    polycall_micro_component_t* component
);

#endif /* POLYCALL_MICRO_COMPONENT_H */
