/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/micro/component_isolation.h"

 * @file component_isolation.c
 * @brief Component isolation implementation for LibPolyCall micro command
system
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the component isolation mechanisms defined in
 * polycall_micro_component.h, providing memory and resource isolation
 * between micro components.
 */

#include "polycall/core/micro/polycall_micro_component.h"
#include "polycall/core/micro/polycall_micro_context.h"
#include "polycall/core/micro/polycall_micro_resource.h"
#include "polycall/core/micro/polycall_micro_security.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of components
#define MAX_COMPONENTS 64

// Maximum number of callbacks per component
#define MAX_CALLBACKS 16

// Component structure
struct polycall_micro_component {
  char *name;                                 // Component name
  polycall_isolation_level_t isolation;       // Isolation level
  polycall_component_state_t state;           // Current state
  resource_limiter_t *resource_limiter;       // Resource limiter
  component_security_context_t *security_ctx; // Security context
  polycall_micro_command_t **commands;        // Array of commands
  size_t command_count;                       // Number of commands
  size_t command_capacity;                    // Command array capacity
  struct {
    component_event_callback_t callback;
    void *user_data;
  } callbacks[MAX_CALLBACKS]; // State change callbacks
  size_t callback_count;      // Number of callbacks
  pthread_mutex_t lock;       // Thread safety mutex
  void *user_data;            // User data
  void *memory_region;        // Isolated memory region
  size_t memory_region_size;  // Memory region size
};

// Component registry structure
struct component_registry {
  polycall_micro_component_t **components; // Array of components
  size_t component_count;                  // Number of components
  size_t capacity;                         // Registry capacity
  pthread_mutex_t lock;                    // Thread safety mutex
};

/**
 * @brief Initialize component registry
 */
polycall_core_error_t
component_registry_init(polycall_core_context_t *ctx,
                        component_registry_t **registry,
                        const component_registry_config_t *config) {
  if (!ctx || !registry || !config) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate registry
  component_registry_t *new_registry =
      polycall_core_malloc(ctx, sizeof(component_registry_t));
  if (!new_registry) {
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component registry");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize registry fields
  memset(new_registry, 0, sizeof(component_registry_t));

  // Set initial capacity
  size_t capacity =
      config->initial_capacity > 0 ? config->initial_capacity : MAX_COMPONENTS;

  // Allocate components array
  new_registry->components = polycall_core_malloc(
      ctx, capacity * sizeof(polycall_micro_component_t *));
  if (!new_registry->components) {
    polycall_core_free(ctx, new_registry);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate components array");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  new_registry->capacity = capacity;
  new_registry->component_count = 0;

  // Initialize mutex if thread-safe
  if (config->thread_safe) {
    if (pthread_mutex_init(&new_registry->lock, NULL) != 0) {
      polycall_core_free(ctx, new_registry->components);
      polycall_core_free(ctx, new_registry);
      POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                         POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                         POLYCALL_ERROR_SEVERITY_ERROR,
                         "Failed to initialize registry mutex");
      return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
  }

  *registry = new_registry;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up component registry
 */
void component_registry_cleanup(polycall_core_context_t *ctx,
                                component_registry_t *registry) {
  if (!ctx || !registry) {
    return;
  }

  // Destroy all components (but don't free them, that's the caller's
  // responsibility)
  if (registry->components) {
    polycall_core_free(ctx, registry->components);
  }

  // Destroy mutex
  pthread_mutex_destroy(&registry->lock);

  // Free registry
  polycall_core_free(ctx, registry);
}

/**
 * @brief Register a component
 */
polycall_core_error_t
component_registry_register(polycall_core_context_t *ctx,
                            component_registry_t *registry,
                            polycall_micro_component_t *component) {
  if (!ctx || !registry || !component) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&registry->lock);

  // Check if registry is full
  if (registry->component_count >= registry->capacity) {
    // Resize registry
    size_t new_capacity = registry->capacity * 2;
    polycall_micro_component_t **new_components = polycall_core_realloc(
        ctx, registry->components,
        new_capacity * sizeof(polycall_micro_component_t *));

    if (!new_components) {
      pthread_mutex_unlock(&registry->lock);
      POLYCALL_ERROR_SET(
          ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
          POLYCALL_ERROR_SEVERITY_ERROR, "Failed to resize component registry");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    registry->components = new_components;
    registry->capacity = new_capacity;
  }

  // Check if component already exists
  for (size_t i = 0; i < registry->component_count; i++) {
    if (strcmp(registry->components[i]->name, component->name) == 0) {
      pthread_mutex_unlock(&registry->lock);
      POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                         POLYCALL_CORE_ERROR_ALREADY_REGISTERED,
                         POLYCALL_ERROR_SEVERITY_ERROR,
                         "Component '%s' already registered", component->name);
      return POLYCALL_CORE_ERROR_ALREADY_REGISTERED;
    }
  }

  // Add component to registry
  registry->components[registry->component_count++] = component;

  pthread_mutex_unlock(&registry->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Unregister a component
 */
polycall_core_error_t
component_registry_unregister(polycall_core_context_t *ctx,
                              component_registry_t *registry,
                              polycall_micro_component_t *component) {
  if (!ctx || !registry || !component) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&registry->lock);

  // Find component
  size_t index = SIZE_MAX;
  for (size_t i = 0; i < registry->component_count; i++) {
    if (registry->components[i] == component) {
      index = i;
      break;
    }
  }

  if (index == SIZE_MAX) {
    pthread_mutex_unlock(&registry->lock);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_NOT_FOUND,
        POLYCALL_ERROR_SEVERITY_ERROR, "Component not found in registry");
    return POLYCALL_CORE_ERROR_NOT_FOUND;
  }

  // Remove component by shifting remaining items
  for (size_t i = index; i < registry->component_count - 1; i++) {
    registry->components[i] = registry->components[i + 1];
  }

  registry->component_count--;

  pthread_mutex_unlock(&registry->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Find a component by name
 */
polycall_core_error_t
component_registry_find(polycall_core_context_t *ctx,
                        component_registry_t *registry, const char *name,
                        polycall_micro_component_t **component) {
  if (!ctx || !registry || !name || !component) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&registry->lock);

  // Search for component by name
  for (size_t i = 0; i < registry->component_count; i++) {
    if (strcmp(registry->components[i]->name, name) == 0) {
      *component = registry->components[i];
      pthread_mutex_unlock(&registry->lock);
      return POLYCALL_CORE_SUCCESS;
    }
  }

  // Not found
  *component = NULL;
  pthread_mutex_unlock(&registry->lock);

  POLYCALL_ERROR_SET(
      ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_NOT_FOUND,
      POLYCALL_ERROR_SEVERITY_ERROR, "Component '%s' not found", name);
  return POLYCALL_CORE_ERROR_NOT_FOUND;
}

/**
 * @brief Get all components
 */
polycall_core_error_t component_registry_get_all(
    polycall_core_context_t *ctx, component_registry_t *registry,
    polycall_micro_component_t **components, size_t *count) {
  if (!ctx || !registry || !count) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&registry->lock);

  // Check if output buffer is large enough
  if (components && *count < registry->component_count) {
    *count = registry->component_count;
    pthread_mutex_unlock(&registry->lock);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL,
        POLYCALL_ERROR_SEVERITY_ERROR, "Buffer too small for components");
    return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
  }

  // Copy components to output buffer if provided
  if (components) {
    memcpy(components, registry->components,
           registry->component_count * sizeof(polycall_micro_component_t *));
  }

  // Set actual count
  *count = registry->component_count;

  pthread_mutex_unlock(&registry->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Notify component state change
 */
static void
notify_component_state_change(polycall_core_context_t *ctx,
                              polycall_micro_component_t *component,
                              polycall_component_state_t old_state,
                              polycall_component_state_t new_state) {
  if (!ctx || !component) {
    return;
  }

  pthread_mutex_lock(&component->lock);

  // Call all registered callbacks
  for (size_t i = 0; i < component->callback_count; i++) {
    if (component->callbacks[i].callback) {
      // Unlock during callback to prevent deadlocks
      pthread_mutex_unlock(&component->lock);
      component->callbacks[i].callback(ctx, NULL, component, old_state,
                                       new_state,
                                       component->callbacks[i].user_data);
      pthread_mutex_lock(&component->lock);
    }
  }

  pthread_mutex_unlock(&component->lock);
}

/**
 * @brief Create a component
 */
polycall_core_error_t polycall_micro_component_create(
    polycall_core_context_t *ctx, polycall_micro_component_t **component,
    const char *name, polycall_isolation_level_t isolation_level) {
  if (!ctx || !component || !name) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate component structure
  polycall_micro_component_t *new_component =
      polycall_core_malloc(ctx, sizeof(polycall_micro_component_t));
  if (!new_component) {
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize component
  memset(new_component, 0, sizeof(polycall_micro_component_t));

  // Copy name
  size_t name_len = strlen(name) + 1;
  new_component->name = polycall_core_malloc(ctx, name_len);
  if (!new_component->name) {
    polycall_core_free(ctx, new_component);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component name");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  memcpy(new_component->name, name, name_len);

  // Set isolation level
  new_component->isolation = isolation_level;

  // Initialize state
  new_component->state = POLYCALL_COMPONENT_STATE_UNINITIALIZED;

  // Initialize mutex
  if (pthread_mutex_init(&new_component->lock, NULL) != 0) {
    polycall_core_free(ctx, new_component->name);
    polycall_core_free(ctx, new_component);
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                       POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to initialize component mutex");
    return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
  }

  // Allocate command array
  new_component->command_capacity = 8; // Initial capacity
  new_component->commands =
      polycall_core_malloc(ctx, new_component->command_capacity *
                                    sizeof(polycall_micro_command_t *));
  if (!new_component->commands) {
    pthread_mutex_destroy(&new_component->lock);
    polycall_core_free(ctx, new_component->name);
    polycall_core_free(ctx, new_component);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate command array");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Set up memory isolation if required
  if (isolation_level >= POLYCALL_ISOLATION_MEMORY) {
    // In a real implementation, this would set up proper memory isolation
    // For now, we'll just allocate a dedicated memory region
    size_t region_size = 1024 * 1024; // 1MB default
    new_component->memory_region = polycall_core_malloc(ctx, region_size);
    if (!new_component->memory_region) {
      polycall_core_free(ctx, new_component->commands);
      pthread_mutex_destroy(&new_component->lock);
      polycall_core_free(ctx, new_component->name);
      polycall_core_free(ctx, new_component);
      POLYCALL_ERROR_SET(
          ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
          POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate memory region");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    new_component->memory_region_size = region_size;
  }

  *component = new_component;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Destroy a component
 */
void polycall_micro_component_destroy(polycall_core_context_t *ctx,
                                      polycall_micro_component_t *component) {
  if (!ctx || !component) {
    return;
  }

  // Free memory region if allocated
  if (component->memory_region) {
    polycall_core_free(ctx, component->memory_region);
  }

  // Free resource limiter if allocated
  if (component->resource_limiter) {
    resource_limiter_cleanup(ctx, component->resource_limiter);
  }

  // Free commands array
  if (component->commands) {
    polycall_core_free(ctx, component->commands);
  }

  // Destroy mutex
  pthread_mutex_destroy(&component->lock);

  // Free name
  if (component->name) {
    polycall_core_free(ctx, component->name);
  }

  // Free component structure
  polycall_core_free(ctx, component);
}

/**
 * @brief Get component info
 */
polycall_core_error_t
polycall_micro_component_get_info(polycall_core_context_t *ctx,
                                  polycall_micro_component_t *component,
                                  polycall_component_info_t *info) {
  if (!ctx || !component || !info) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&component->lock);

  // Fill info structure
  info->name = component->name;
  info->isolation = component->isolation;
  info->state = component->state;
  info->command_count = component->command_count;

  // Get resource usage if available
  if (component->resource_limiter) {
    polycall_resource_usage_t usage;
    if (resource_limiter_get_usage(ctx, component->resource_limiter, &usage) ==
        POLYCALL_CORE_SUCCESS) {
      info->memory_usage = usage.memory_usage;
      info->cpu_usage = usage.cpu_usage;
      info->io_usage = usage.io_usage;
    } else {
      // Default values if resource usage not available
      info->memory_usage = 0;
      info->cpu_usage = 0;
      info->io_usage = 0;
    }
  } else {
    // Default values if resource limiter not available
    info->memory_usage = 0;
    info->cpu_usage = 0;
    info->io_usage = 0;
  }

  pthread_mutex_unlock(&component->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register component state change callback
 */
polycall_core_error_t polycall_micro_component_register_callback(
    polycall_core_context_t *ctx, polycall_micro_component_t *component,
    component_event_callback_t callback, void *user_data) {
  if (!ctx || !component || !callback) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&component->lock);

  // Check if we've reached the maximum number of callbacks
  if (component->callback_count >= MAX_CALLBACKS) {
    pthread_mutex_unlock(&component->lock);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
        POLYCALL_ERROR_SEVERITY_ERROR, "Maximum number of callbacks reached");
    return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
  }

  // Add callback
  component->callbacks[component->callback_count].callback = callback;
  component->callbacks[component->callback_count].user_data = user_data;
  component->callback_count++;

  pthread_mutex_unlock(&component->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Unregister component state change callback
 */
polycall_core_error_t polycall_micro_component_unregister_callback(
    polycall_core_context_t *ctx, polycall_micro_component_t *component,
    component_event_callback_t callback) {
  if (!ctx || !component || !callback) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&component->lock);

  // Find callback
  size_t index = SIZE_MAX;
  for (size_t i = 0; i < component->callback_count; i++) {
    if (component->callbacks[i].callback == callback) {
      index = i;
      break;
    }
  }

  if (index == SIZE_MAX) {
    pthread_mutex_unlock(&component->lock);
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                       POLYCALL_CORE_ERROR_NOT_FOUND,
                       POLYCALL_ERROR_SEVERITY_ERROR, "Callback not found");
    return POLYCALL_CORE_ERROR_NOT_FOUND;
  }

  // Remove callback by shifting remaining items
  for (size_t i = index; i < component->callback_count - 1; i++) {
    component->callbacks[i] = component->callbacks[i + 1];
  }

  component->callback_count--;

  pthread_mutex_unlock(&component->lock);
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Initialize component security context
 */
polycall_core_error_t polycall_micro_component_init_security(
    polycall_core_context_t *ctx, polycall_micro_component_t *component,
    component_security_context_t **security_ctx) {
  // This would integrate with the security system
  // For now, we'll just return success
  if (!ctx || !component || !security_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // In a real implementation, we would initialize the security context here
  *security_ctx = NULL;

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get component security context
 */
polycall_core_error_t polycall_micro_component_get_security(
    polycall_core_context_t *ctx, polycall_micro_component_t *component,
    component_security_context_t **security_ctx) {
  if (!ctx || !component || !security_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  *security_ctx = component->security_ctx;

  if (!*security_ctx) {
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_NOT_INITIALIZED,
        POLYCALL_ERROR_SEVERITY_ERROR, "Security context not initialized");
    return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
  }

  return POLYCALL_CORE_SUCCESS;
}
