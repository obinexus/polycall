# Hierarchical Error Handling Implementation Guide

## Overview

This document outlines the implementation of hierarchical error handling across the LibPolyCall codebase. The hierarchical error system enhances the existing error handling capabilities by adding component relationships, error propagation, and component-specific error handling.

## Design Goals

The hierarchical error handling system is designed to:

1. **Preserve component isolation** while enabling error flow between related components
2. **Maintain the Program-First approach** established by Nnamdi Okpala
3. **Provide meaningful error context** that includes component relationship information
4. **Enable flexible error propagation** with configurable propagation modes
5. **Integrate with the existing error system** without breaking compatibility

## Implementation Strategy

### Phase 1: Core Implementation

1. Implement the `polycall_hierarchical_error.h` header and `hierarchical_error.c` implementation
2. Add unit tests to verify functionality
3. Integrate with CMake build system
4. Update documentation

### Phase 2: Component Integration

For each LibPolyCall component:

1. Add component-specific error handling
2. Define parent-child relationships
3. Configure propagation modes
4. Add component-specific error codes and messages

## Component Hierarchy

The component hierarchy should follow this structure:

```
core
├── memory
├── context
├── config
├── security
├── network
│   ├── client
│   └── server
├── protocol
│   ├── handshake
│   └── message
├── ffi
│   ├── js_bridge
│   ├── python_bridge
│   └── jvm_bridge
├── micro
└── edge
```

## Integration Guide for Components

### Step 1: Initialize Hierarchical Error System

In the component initialization:

```c
// Initialize hierarchical error context if not already done
if (!component->hierarchical_error_ctx) {
    polycall_hierarchical_error_init(
        core_ctx,
        &component->hierarchical_error_ctx
    );
}
```

### Step 2: Register Component Handler

Register each component with its parent:

```c
polycall_hierarchical_error_handler_config_t config = {
    .component_name = "component_name",
    .source = POLYCALL_ERROR_SOURCE_COMPONENT,
    .handler = component_error_handler,
    .user_data = component_context,
    .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
    .parent_component = "parent_component_name"
};

polycall_hierarchical_error_register_handler(
    core_ctx,
    component->hierarchical_error_ctx,
    &config
);
```

### Step 3: Implement Component Error Handler

Create a component-specific error handler:

```c
static void component_error_handler(
    polycall_core_context_t* ctx,
    const char* component_name,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* message,
    void* user_data
) {
    // Component-specific error handling logic
    component_context_t* component = (component_context_t*)user_data;
    
    // Log the error
    polycall_logger_log(ctx->logger, severity, "[%s] %s", component_name, message);
    
    // Component-specific recovery/handling
    if (severity == POLYCALL_ERROR_SEVERITY_FATAL) {
        // Handle fatal errors
    }
}
```

### Step 4: Use Hierarchical Error Setting

Replace standard error setting with hierarchical version:

```c
// Before
polycall_error_set(ctx, POLYCALL_ERROR_SOURCE_COMPONENT, 
    POLYCALL_CORE_ERROR_INVALID_PARAMETERS, "Invalid parameter: %s", param_name);

// After
POLYCALL_HIERARCHICAL_ERROR_SET(ctx, component->hierarchical_error_ctx,
    "component_name", POLYCALL_ERROR_SOURCE_COMPONENT,
    POLYCALL_CORE_ERROR_INVALID_PARAMETERS, POLYCALL_ERROR_SEVERITY_ERROR,
    "Invalid parameter: %s", param_name);
```

## Error Propagation

The hierarchical error system supports four propagation modes:

1. **POLYCALL_ERROR_PROPAGATE_NONE**: No propagation (errors stay in the component)
2. **POLYCALL_ERROR_PROPAGATE_UPWARD**: Propagate to parent components
3. **POLYCALL_ERROR_PROPAGATE_DOWNWARD**: Propagate to child components
4. **POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL**: Propagate both ways

Choose the appropriate mode based on the component's role:

- **Core components**: Use `POLYCALL_ERROR_PROPAGATE_DOWNWARD` to notify dependent components
- **Leaf components**: Use `POLYCALL_ERROR_PROPAGATE_UPWARD` to notify parent components
- **Middle-tier components**: Use `POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL` to notify in both directions

## Error Severity Guidelines

Use the following guidelines for error severity:

- **POLYCALL_ERROR_SEVERITY_INFO**: Informational messages that don't represent errors
- **POLYCALL_ERROR_SEVERITY_WARNING**: Non-critical issues that don't prevent operation
- **POLYCALL_ERROR_SEVERITY_ERROR**: Errors that prevent an operation but not the whole component
- **POLYCALL_ERROR_SEVERITY_FATAL**: Critical errors that render the component inoperable

## Testing

Each component should include tests that verify:

1. Proper error registration
2. Error propagation according to configured mode
3. Error handling through the component-specific handler
4. Error clearing and management

## Migration Strategy

1. Start with core components (memory, context, error)
2. Move to infrastructure components (network, protocol)
3. Finally migrate application-level components (micro, edge, ffi)

This phased approach allows for incremental integration while maintaining backward compatibility.

## Example Implementation for Network Component

```c
// In network initialization
polycall_hierarchical_error_handler_config_t config = {
    .component_name = "network",
    .source = POLYCALL_ERROR_SOURCE_NETWORK,
    .handler = network_error_handler,
    .user_data = network_ctx,
    .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
    .parent_component = "core"
};

polycall_hierarchical_error_register_handler(
    core_ctx,
    network_ctx->hierarchical_error_ctx,
    &config
);

// In network error handling
POLYCALL_HIERARCHICAL_ERROR_SET(ctx, network_ctx->hierarchical_error_ctx,
    "network", POLYCALL_ERROR_SOURCE_NETWORK,
    POLYCALL_CORE_ERROR_NETWORK, POLYCALL_ERROR_SEVERITY_ERROR,
    "Connection to %s failed: %s", address, strerror(errno));
```

## Conclusion

The hierarchical error handling system enhances LibPolyCall's error management while preserving the Program-First design principles. By following this guide, each component can be integrated into the hierarchy, enabling more sophisticated error propagation and handling across the system.