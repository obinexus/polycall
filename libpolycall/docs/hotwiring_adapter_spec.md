# Hotwiring Adapter Integration Specification

This document outlines the core structures for the hotwiring adapter layer. The adapter layer provides a unified interface for language-specific bindings to interact with the core topology manager.

## Structures

The primary interface is defined in `adapter_base.h`:

```c
typedef struct adapter_vtable {
    int (*init)(void* adapter, struct topology_manager* manager);
    int (*enter_layer)(void* adapter, uint64_t thread_id, uint32_t layer_id);
    int (*exit_layer)(void* adapter, uint64_t thread_id);
    int (*validate_transition)(void* adapter, uint32_t from, uint32_t to);
    int (*emit_trace)(void* adapter, struct trace_event* event);
    int (*cleanup)(void* adapter);
} adapter_vtable_t;
```

Language specific adapters extend `adapter_base_t` and provide implementations for these callbacks.

See `src/core/adapters` for the initial skeleton implementation.
