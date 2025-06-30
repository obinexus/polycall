// accessibility_service_provider.c
#include "polycall/cli/providers/accessibility_service_provider.h"
#include "polycall/cli/providers/cli_container.h"
#include "polycall/core/accessibility/accessibility_interface.h"

// Initialize accessibility services
polycall_core_error_t polycall_accessibility_register_services(
    polycall_cli_container_t* container
) {
    if (!container) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get core context from container
    polycall_core_context_t* core_ctx = container->resolve_service(
        container, "core_context");
    if (!core_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create default accessibility configuration
    polycall_accessibility_config_t access_config = polycall_accessibility_default_config();
    
    // Initialize accessibility context
    polycall_accessibility_context_t* access_ctx = NULL;
    polycall_core_error_t result = polycall_accessibility_init(
        core_ctx, &access_config, &access_ctx);
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Register accessibility context in container
    container->register_service(container, "accessibility_context", access_ctx);
    
    return POLYCALL_CORE_SUCCESS;
}

// Cleanup accessibility services
void polycall_accessibility_cleanup_services(
    polycall_cli_container_t* container
) {
    if (!container) {
        return;
    }
    
    // Get core context from container
    polycall_core_context_t* core_ctx = container->resolve_service(
        container, "core_context");
    if (!core_ctx) {
        return;
    }
    
    // Get accessibility context from container
    polycall_accessibility_context_t* access_ctx = container->resolve_service(
        container, "accessibility_context");
    if (!access_ctx) {
        return;
    }
    
    // Cleanup accessibility context
    polycall_accessibility_cleanup(core_ctx, access_ctx);
    
    // Unregister from container
    container->register_service(container, "accessibility_context", NULL);
}