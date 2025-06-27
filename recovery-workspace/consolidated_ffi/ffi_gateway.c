/*
 * Sinphasé-Compliant FFI Gateway
 * Single entry point for all FFI operations
 * Target: < 0.4 cost (< 200 lines, minimal includes)
 */

#include "ffi_gateway.h"
#include "bridge_registry.h"
#include <stddef.h>
#include <string.h>

// Global bridge registry (single instance)
static bridge_registry_t* g_registry = NULL;

// Initialize FFI gateway (single-pass)
int ffi_gateway_init(void) {
    if (g_registry != NULL) {
        return -1; // Already initialized
    }
    
    g_registry = bridge_registry_create();
    if (!g_registry) {
        return -1;
    }
    
    // Register core bridges only
    bridge_registry_add(g_registry, "c", c_bridge_create);
    bridge_registry_add(g_registry, "python", python_bridge_create);
    bridge_registry_add(g_registry, "js", js_bridge_create);
    bridge_registry_add(g_registry, "jvm", jvm_bridge_create);
    
    return 0;
}

// Single call interface
int ffi_gateway_call(const char* language, const char* function, 
                     void* args, void* result) {
    if (!g_registry || !language || !function) {
        return -1;
    }
    
    bridge_t* bridge = bridge_registry_get(g_registry, language);
    if (!bridge) {
        return -1;
    }
    
    return bridge->call(function, args, result);
}

// Cleanup (single-pass)
void ffi_gateway_cleanup(void) {
    if (g_registry) {
        bridge_registry_destroy(g_registry);
        g_registry = NULL;
    }
}
