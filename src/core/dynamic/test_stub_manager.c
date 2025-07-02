/**
 * @file test_stub_manager.c
 * @brief Test stub manager implementation for LibPolyCall
 * @author LibPolyCall Implementation Team
 */

#include "test_stub_manager.h"
#include <string.h>
#include <stdio.h>

// Include all component test stubs
#include "polycall_test_stub.h"
#include "auth_test_stub.h"
#include "config_test_stub.h"
#include "edge_test_stub.h"
#include "ffi_test_stub.h"
#include "micro_test_stub.h"
#include "network_test_stub.h"
#include "protocol_test_stub.h"
#include "telemetry_test_stub.h"
#include "accessibility_test_stub.h"

#define MAX_COMPONENTS 32

typedef struct {
    char name[64];
    bool initialized;
} component_status_t;

// Static data
static component_status_t s_components[MAX_COMPONENTS];
static int s_component_count = 0;

/**
 * @brief Initialize test stubs for specified components
 */
bool test_stub_manager_init(const char** components, int count) {
    if (!components || count <= 0 || count > MAX_COMPONENTS) {
        return false;
    }
    
    // Reset status
    memset(s_components, 0, sizeof(s_components));
    s_component_count = 0;
    
    // Initialize each component
    for (int i = 0; i < count; i++) {
        const char* component = components[i];
        bool initialized = false;
        
        // Call appropriate init function based on component name
        if (strcmp(component, "polycall") == 0) {
    if (polycall_polycall_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "auth") == 0) {
    if (polycall_auth_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "config") == 0) {
    if (polycall_config_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "edge") == 0) {
    if (polycall_edge_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "ffi") == 0) {
    if (polycall_ffi_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "micro") == 0) {
    if (polycall_micro_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "network") == 0) {
    if (polycall_network_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "protocol") == 0) {
    if (polycall_protocol_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "telemetry") == 0) {
    if (polycall_telemetry_init_test_stubs() == 0) {
        initialized = true;
    }
}
if (strcmp(component, "accessibility") == 0) {
    if (polycall_accessibility_init_test_stubs() == 0) {
        initialized = true;
    }
}
        
        // Store component status
        if (initialized) {
            strncpy(s_components[s_component_count].name, component, sizeof(s_components[0].name) - 1);
            s_components[s_component_count].initialized = true;
            s_component_count++;
        } else {
            fprintf(stderr, "Failed to initialize test stubs for component: %s\n", component);
            test_stub_manager_cleanup();
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Clean up all initialized test stubs
 */
void test_stub_manager_cleanup(void) {
    // Clean up each initialized component
    for (int i = 0; i < s_component_count; i++) {
        if (s_components[i].initialized) {
            const char* component = s_components[i].name;
            
            // Call appropriate cleanup function based on component name
            if (strcmp(component, "polycall") == 0) {
    polycall_polycall_cleanup_test_stubs();
}
if (strcmp(component, "auth") == 0) {
    polycall_auth_cleanup_test_stubs();
}
if (strcmp(component, "config") == 0) {
    polycall_config_cleanup_test_stubs();
}
if (strcmp(component, "edge") == 0) {
    polycall_edge_cleanup_test_stubs();
}
if (strcmp(component, "ffi") == 0) {
    polycall_ffi_cleanup_test_stubs();
}
if (strcmp(component, "micro") == 0) {
    polycall_micro_cleanup_test_stubs();
}
if (strcmp(component, "network") == 0) {
    polycall_network_cleanup_test_stubs();
}
if (strcmp(component, "protocol") == 0) {
    polycall_protocol_cleanup_test_stubs();
}
if (strcmp(component, "telemetry") == 0) {
    polycall_telemetry_cleanup_test_stubs();
}
if (strcmp(component, "accessibility") == 0) {
    polycall_accessibility_cleanup_test_stubs();
}
            
            s_components[i].initialized = false;
        }
    }
    
    s_component_count = 0;
}

/**
 * @brief Check if component has been initialized
 */
bool test_stub_manager_is_initialized(const char* component_name) {
    if (!component_name) {
        return false;
    }
    
    for (int i = 0; i < s_component_count; i++) {
        if (strcmp(s_components[i].name, component_name) == 0) {
            return s_components[i].initialized;
        }
    }
    
    return false;
}
