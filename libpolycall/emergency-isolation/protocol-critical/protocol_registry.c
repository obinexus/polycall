/**
 * @file protocol_registry.c
 * @brief Registry implementation for protocol module
 */

#include <string.h>
#include <stdlib.h>
#include "polycall/core/protocol/protocol_registry.h"

/* Default max services */
#define MAX_SERVICES 64

/* Registry implementation */
protocol_registry_t* protocol_registry_create() {
    protocol_registry_t* registry = malloc(sizeof(protocol_registry_t));
    if (!registry) {
        return NULL;
    }
    
    registry->services = calloc(MAX_SERVICES, sizeof(protocol_service_t));
    if (!registry->services) {
        free(registry);
        return NULL;
    }
    
    registry->count = 0;
    registry->capacity = MAX_SERVICES;
    
    return registry;
}

void protocol_registry_destroy(protocol_registry_t* registry) {
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

int protocol_registry_register(protocol_registry_t* registry, const char* name, void* service) {
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

void* protocol_registry_get(protocol_registry_t* registry, const char* name) {
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

int protocol_registry_register_defaults(protocol_registry_t* registry) {
    if (!registry) {
        return -1;
    }
    
    // Module-specific default service registration would go here
    
    return 0;
}
