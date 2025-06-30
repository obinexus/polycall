```mermaid 
classDiagram
    class polycall_micro_context {
        +polycall_core_context_t* core_ctx
        +component_registry_t* components
        +command_registry_t* commands
        +security_policy_t* security_policy
        +resource_limits_t* resource_limits
    }
    
    class component_registry {
        +polycall_micro_component_t** components
        +size_t component_count
        +mutex lock
    }
    
    class command_registry {
        +polycall_micro_command_t** commands
        +size_t command_count
        +mutex lock
    }
    
    class polycall_micro_component {
        +const char* name
        +isolation_level_t isolation_level
        +component_state_t state
        +void* private_data
        +security_context_t* security_ctx
    }
    
    class polycall_micro_command {
        +const char* name
        +command_handler_t handler
        +command_flags_t flags
        +security_attributes_t* security_attrs
    }
    
    class resource_limiter {
        +memory_quota_t memory_quota
        +cpu_quota_t cpu_quota
        +io_quota_t io_quota
        +enforce_limits()
    }

    polycall_micro_context "1" -- "1" component_registry: contains
    polycall_micro_context "1" -- "1" command_registry: contains
    polycall_micro_context "1" -- "1" resource_limiter: manages
    component_registry "1" -- "*" polycall_micro_component: registers
    command_registry "1" -- "*" polycall_micro_command: registers
    polycall_micro_component "1" -- "*" polycall_micro_command: executes
    resource_limiter -- polycall_micro_component: constrains
    ```