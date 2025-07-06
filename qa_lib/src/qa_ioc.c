/**
 * qa_ioc.c - IoC Container for Test Isolation
 */

#include "qa_ioc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Service entry
typedef struct qa_service {
    char name[64];
    void* instance;
    qa_service_destructor_fn destructor;
    struct qa_service* next;
} qa_service_t;

// IoC container structure
struct qa_ioc_container {
    qa_service_t* services;
    pthread_mutex_t lock;
    bool thread_safe;
};

// Create IoC container
qa_ioc_container_t* qa_ioc_create(void) {
    qa_ioc_container_t* container = calloc(1, sizeof(qa_ioc_container_t));
    if (!container) return NULL;
    
    pthread_mutex_init(&container->lock, NULL);
    container->thread_safe = true;
    
    return container;
}

// Register service
int qa_ioc_register(qa_ioc_container_t* container, 
                    const char* name, 
                    void* instance) {
    return qa_ioc_register_with_destructor(container, name, instance, free);
}

// Register with custom destructor
int qa_ioc_register_with_destructor(qa_ioc_container_t* container,
                                   const char* name,
                                   void* instance,
                                   qa_service_destructor_fn destructor) {
    if (!container || !name || !instance) return -1;
    
    if (container->thread_safe) {
        pthread_mutex_lock(&container->lock);
    }
    
    // Check for duplicate
    qa_service_t* current = container->services;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (container->thread_safe) {
                pthread_mutex_unlock(&container->lock);
            }
            return -1;  // Already registered
        }
        current = current->next;
    }
    
    // Create new service
    qa_service_t* service = calloc(1, sizeof(qa_service_t));
    strncpy(service->name, name, sizeof(service->name) - 1);
    service->instance = instance;
    service->destructor = destructor;
    
    // Add to list
    service->next = container->services;
    container->services = service;
    
    if (container->thread_safe) {
        pthread_mutex_unlock(&container->lock);
    }
    
    return 0;
}

// Resolve service
void* qa_ioc_resolve(qa_ioc_container_t* container, const char* name) {
    if (!container || !name) return NULL;
    
    if (container->thread_safe) {
        pthread_mutex_lock(&container->lock);
    }
    
    qa_service_t* current = container->services;
    void* instance = NULL;
    
    while (current) {
        if (strcmp(current->name, name) == 0) {
            instance = current->instance;
            break;
        }
        current = current->next;
    }
    
    if (container->thread_safe) {
        pthread_mutex_unlock(&container->lock);
    }
    
    return instance;
}

// Create scoped container
qa_ioc_container_t* qa_ioc_create_scope(qa_ioc_container_t* parent) {
    qa_ioc_container_t* scope = qa_ioc_create();
    if (!scope) return NULL;
    
    // Copy parent services (shallow copy)
    if (parent && parent->thread_safe) {
        pthread_mutex_lock(&parent->lock);
    }
    
    qa_service_t* current = parent ? parent->services : NULL;
    while (current) {
        // In scope, we don't own the instances
        qa_ioc_register_with_destructor(scope, current->name, 
                                       current->instance, NULL);
        current = current->next;
    }
    
    if (parent && parent->thread_safe) {
        pthread_mutex_unlock(&parent->lock);
    }
    
    return scope;
}

// Destroy container
void qa_ioc_destroy(qa_ioc_container_t* container) {
    if (!container) return;
    
    // Clean up services
    qa_service_t* current = container->services;
    while (current) {
        qa_service_t* next = current->next;
        
        // Call destructor if provided
        if (current->destructor && current->instance) {
            current->destructor(current->instance);
        }
        
        free(current);
        current = next;
    }
    
    pthread_mutex_destroy(&container->lock);
    free(container);
}
