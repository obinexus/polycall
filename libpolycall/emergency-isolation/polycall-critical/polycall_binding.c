// Implementation for polycall_binding.c

#include "polycall/core/polycall/polycall.h"
#include "polycall/core/polycall/polycall_binding.h"
#include <string.h>

// Registry of registered bindings
typedef struct {
    char name[32];
    binding_init_fn init;
    binding_cleanup_fn cleanup;
    binding_process_fn process;
    void* binding_data;
} binding_registry_entry_t;

#define MAX_BINDINGS 16
static binding_registry_entry_t bindings[MAX_BINDINGS];
static int binding_count = 0;

polycall_error_t polycall_register_binding(
    const char* name,
    binding_init_fn init,
    binding_cleanup_fn cleanup,
    binding_process_fn process
) {
    if (!name || !init || !cleanup || !process) {
        return POLYCALL_ERROR_INVALID_PARAMETERS;
    }
    
    if (binding_count >= MAX_BINDINGS) {
        return POLYCALL_ERROR_CAPACITY_EXCEEDED;
    }
    
    // Check if binding already exists
    for (int i = 0; i < binding_count; i++) {
        if (strcmp(bindings[i].name, name) == 0) {
            return POLYCALL_ERROR_ALREADY_INITIALIZED;
        }
    }
    
    // Register binding
    strncpy(bindings[binding_count].name, name, 31);
    bindings[binding_count].name[31] = '\0';
    bindings[binding_count].init = init;
    bindings[binding_count].cleanup = cleanup;
    bindings[binding_count].process = process;
    bindings[binding_count].binding_data = NULL;
    binding_count++;
    
    return POLYCALL_OK;
}

polycall_error_t polycall_binding_init(
    polycall_context_t* ctx,
    const char* binding_name,
    const void* binding_config
) {
    if (!ctx || !binding_name) {
        return POLYCALL_ERROR_INVALID_PARAMETERS;
    }
    
    // Find binding
    for (int i = 0; i < binding_count; i++) {
        if (strcmp(bindings[i].name, binding_name) == 0) {
            // Initialize binding
            polycall_error_t result = bindings[i].init(ctx, binding_config, &bindings[i].binding_data);
            return result;
        }
    }
    
    return POLYCALL_ERROR_NOT_FOUND;
}

polycall_error_t polycall_binding_cleanup(
    polycall_context_t* ctx,
    const char* binding_name
) {
    if (!ctx || !binding_name) {
        return POLYCALL_ERROR_INVALID_PARAMETERS;
    }
    
    // Find binding
    for (int i = 0; i < binding_count; i++) {
        if (strcmp(bindings[i].name, binding_name) == 0) {
            // Clean up binding
            polycall_error_t result = bindings[i].cleanup(ctx, bindings[i].binding_data);
            bindings[i].binding_data = NULL;
            return result;
        }
    }
    
    return POLYCALL_ERROR_NOT_FOUND;
}

polycall_error_t polycall_binding_process_message(
    polycall_context_t* ctx,
    const char* binding_name,
    polycall_message_t* message,
    polycall_message_t** response
) {
    if (!ctx || !binding_name || !message) {
        return POLYCALL_ERROR_INVALID_PARAMETERS;
    }
    
    // Find binding
    for (int i = 0; i < binding_count; i++) {
        if (strcmp(bindings[i].name, binding_name) == 0) {
            // Process message
            return bindings[i].process(ctx, message, response, bindings[i].binding_data);
        }
    }
    
    return POLYCALL_ERROR_NOT_FOUND;
}