/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file polycall_registry.c
 * @brief Registry implementation for polycall module
 */

#include <string.h>
#include "polycall/core/polycall/polycall_registry.h"

/* Default max services */
#define MAX_SERVICES 64

/* Registry implementation */
polycall_registry_t* polycall_registry_create() {
    polycall_registry_t* registry = malloc(sizeof(polycall_registry_t));
    if (!registry) {
        return NULL;
    }
    
    registry->services = calloc(MAX_SERVICES, sizeof(polycall_service_t));
    if (!registry->services) {
        free(registry);
        return NULL;
    }
    
    registry->count = 0;
    registry->capacity = MAX_SERVICES;
    
    return registry;
}

void polycall_registry_destroy(polycall_registry_t* registry) {
    if (registry) {
        if (registry->services) {
            // Free service names
            for (int i = 0; i < registry->count; i++) {
                free(registry->services[i].name);
            }
            free(registry->services);
        }
        free(registry);
    }
}

int polycall_registry_register(polycall_registry_t* registry, const char* name, void* service) {
    if (!registry || !name || !service) {
        return -1;
    }
    
    // Check if service already exists
    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->services[i].name, name) == 0) {
            // Update existing service
            registry->services[i].service = service;
            return 0;
        }
    }
    
    // Check capacity
    if (registry->count >= registry->capacity) {
        return -2; // Registry full
    }
    
    // Add new service
    char* service_name = strdup(name);
    if (!service_name) {
        return -3; // Memory allocation error
    }
    
    registry->services[registry->count].name = service_name;
    registry->services[registry->count].service = service;
    registry->count++;
    
    return 0;
}

void* polycall_registry_get(polycall_registry_t* registry, const char* name) {
    if (!registry || !name) {
        return NULL;
    }
    
    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->services[i].name, name) == 0) {
            return registry->services[i].service;
        }
    }
    
    return NULL;
}

int polycall_registry_register_defaults(polycall_registry_t* registry) {
    if (!registry) {
        return -1;
    }
    
    // Module-specific default service registration would go here
    
    return 0;
}
