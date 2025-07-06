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
#include "polycall/core/micro/security_policy.h"

#include "polycall/core/micro/polycall_micro_security.h"
* @file security_policy.c
* @brief Security policy implementation for LibPolyCall micro command system
* @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
*
* This file implements the security policy enforcement interfaces defined in
* polycall_micro_security.h for the LibPolyCall micro command system, providing
* robust permission management, command verification, and audit capabilities.
*/

/**
 * @brief Create a timestamp for audit entries
 *
 * @return Current timestamp
 */
static uint64_t create_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Find a component's permissions in the permission map
 *
 * @param policy Security policy
 * @param component_name Component name
 * @return Pointer to permissions, or NULL if not found
 */
static polycall_permission_t *
find_component_permissions(security_policy_t *policy,
                           const char *component_name) {
  if (!policy || !component_name) {
    return NULL;
  }

  for (size_t i = 0; i < policy->permission_map_size; i++) {
    if (strcmp(policy->permission_map[i].component_name, component_name) == 0) {
      return &policy->permission_map[i].permissions;
    }
  }

  return NULL;
}

/**
 * @brief Add a new audit log entry
 *
 * @param ctx Core context
 * @param policy Security policy
 * @param entry Audit entry to add
 * @return Error code
 */
static polycall_core_error_t
add_audit_log_entry(polycall_core_context_t *ctx, security_policy_t *policy,
                    const polycall_security_audit_entry_t *entry) {
  if (!ctx || !policy || !entry) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Create new entry
  audit_log_entry_t *log_entry =
      polycall_core_malloc(ctx, sizeof(audit_log_entry_t));
  if (!log_entry) {
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Copy entry data
  memcpy(&log_entry->entry, entry, sizeof(polycall_security_audit_entry_t));
  log_entry->next = NULL;

  // Add to log
  pthread_mutex_lock(&policy->mutex);

  if (policy->audit_log_tail) {
    policy->audit_log_tail->next = log_entry;
  } else {
    policy->audit_log_head = log_entry;
  }

  policy->audit_log_tail = log_entry;
  policy->audit_log_size++;

  // Trim log if it exceeds maximum size
  if (policy->audit_log_size > MAX_AUDIT_LOG_SIZE) {
    audit_log_entry_t *old_head = policy->audit_log_head;
    policy->audit_log_head = old_head->next;
    polycall_core_free(ctx, old_head);
    policy->audit_log_size--;
  }

  pthread_mutex_unlock(&policy->mutex);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Notify audit callbacks about an event
 *
 * @param ctx Core context
 * @param micro_ctx Micro context
 * @param policy Security policy
 * @param entry Audit entry to notify about
 */
static void notify_audit_callbacks(
    polycall_core_context_t *ctx, polycall_micro_context_t *micro_ctx,
    security_policy_t *policy, const polycall_security_audit_entry_t *entry) {
  if (!ctx || !policy || !entry) {
    return;
  }

  pthread_mutex_lock(&policy->mutex);

  for (size_t i = 0; i < policy->audit_callback_count; i++) {
    security_audit_callback_t callback = policy->audit_callbacks[i].callback;
    void *user_data = policy->audit_callbacks[i].user_data;

    if (callback) {
      callback(ctx, micro_ctx, entry, user_data);
    }
  }

  pthread_mutex_unlock(&policy->mutex);
}

/**
 * @brief Initialize security policy
 */
polycall_core_error_t
security_policy_init(polycall_core_context_t *ctx, security_policy_t **policy,
                     const security_policy_config_t *config) {
  if (!ctx || !policy || !config) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate policy structure
  security_policy_t *new_policy =
      polycall_core_malloc(ctx, sizeof(security_policy_t));
  if (!new_policy) {
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate security policy");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize policy
  memset(new_policy, 0, sizeof(security_policy_t));
  new_policy->enforce_policy = config->enforce_policy;
  new_policy->default_permissions = config->default_permissions;
  new_policy->allow_privilege_escalation = config->allow_privilege_escalation;
  new_policy->verify_commands = config->verify_commands;
  new_policy->audit_events = config->audit_events;

  // Copy policy file path if provided
  if (config->policy_file) {
    size_t path_len = strlen(config->policy_file) + 1;
    char *policy_file = polycall_core_malloc(ctx, path_len);
    if (!policy_file) {
      polycall_core_free(ctx, new_policy);
      POLYCALL_ERROR_SET(
          ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
          POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate policy file path");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(policy_file, config->policy_file, path_len);
    new_policy->policy_file = policy_file;
  }

  // Copy audit log file path if provided
  if (config->audit_log_file) {
    size_t path_len = strlen(config->audit_log_file) + 1;
    char *audit_log_file = polycall_core_malloc(ctx, path_len);
    if (!audit_log_file) {
      if (new_policy->policy_file) {
        polycall_core_free(ctx, (void *)new_policy->policy_file);
      }
      polycall_core_free(ctx, new_policy);
      POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                         POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                         POLYCALL_ERROR_SEVERITY_ERROR,
                         "Failed to allocate audit log file path");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(audit_log_file, config->audit_log_file, path_len);
    new_policy->audit_log_file = audit_log_file;
  }

  // Allocate permission map
  new_policy->permission_map_capacity = MAX_COMPONENT_PERMISSIONS;
  new_policy->permission_map =
      polycall_core_malloc(ctx, new_policy->permission_map_capacity *
                                    sizeof(permission_map_entry_t));
  if (!new_policy->permission_map) {
    if (new_policy->policy_file) {
      polycall_core_free(ctx, (void *)new_policy->policy_file);
    }
    if (new_policy->audit_log_file) {
      polycall_core_free(ctx, (void *)new_policy->audit_log_file);
    }
    polycall_core_free(ctx, new_policy);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate permission map");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize mutex
  pthread_mutex_init(&new_policy->mutex, NULL);

  // Load policy file if provided
  if (new_policy->policy_file) {
    polycall_core_error_t load_result =
        security_policy_load(ctx, new_policy, new_policy->policy_file);
    if (load_result != POLYCALL_CORE_SUCCESS) {
      security_policy_cleanup(ctx, new_policy);
      POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, load_result,
                         POLYCALL_ERROR_SEVERITY_ERROR,
                         "Failed to load security policy from file");
      return load_result;
    }
  }

  // Create audit entry for policy initialization
  if (new_policy->audit_events) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_LOADED;
    entry.component_name = NULL;
    entry.command_name = NULL;
    entry.permissions = new_policy->default_permissions;
    entry.details = "Security policy initialized";

    add_audit_log_entry(ctx, new_policy, &entry);
  }

  *policy = new_policy;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Clean up security policy
 */
void security_policy_cleanup(polycall_core_context_t *ctx,
                             security_policy_t *policy) {
  if (!ctx || !policy) {
    return;
  }

  // Free permission map
  if (policy->permission_map) {
    // Free component names
    for (size_t i = 0; i < policy->permission_map_size; i++) {
      if (policy->permission_map[i].component_name) {
        polycall_core_free(ctx,
                           (void *)policy->permission_map[i].component_name);
      }
    }
    polycall_core_free(ctx, policy->permission_map);
  }

  // Free audit log
  audit_log_entry_t *entry = policy->audit_log_head;
  while (entry) {
    audit_log_entry_t *next = entry->next;
    polycall_core_free(ctx, entry);
    entry = next;
  }

  // Free file paths
  if (policy->policy_file) {
    polycall_core_free(ctx, (void *)policy->policy_file);
  }
  if (policy->audit_log_file) {
    polycall_core_free(ctx, (void *)policy->audit_log_file);
  }

  // Destroy mutex
  pthread_mutex_destroy(&policy->mutex);

  // Free policy structure
  polycall_core_free(ctx, policy);
}

/**
 * @brief Load security policy from file
 */
polycall_core_error_t security_policy_load(polycall_core_context_t *ctx,
                                           security_policy_t *policy,
                                           const char *file_path) {
  if (!ctx || !policy || !file_path) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // In a full implementation, this would parse a policy file
  // For this implementation, we'll just set a placeholder

  // Log the event
  if (policy->audit_events) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_LOADED;
    entry.component_name = NULL;
    entry.command_name = NULL;
    entry.permissions = policy->default_permissions;
    entry.details = "Security policy loaded from file";

    add_audit_log_entry(ctx, policy, &entry);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Save security policy to file
 */
polycall_core_error_t security_policy_save(polycall_core_context_t *ctx,
                                           security_policy_t *policy,
                                           const char *file_path) {
  if (!ctx || !policy || !file_path) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // In a full implementation, this would serialize the policy to a file
  // For this implementation, we'll just log the event

  // Log the event
  if (policy->audit_events) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
    entry.component_name = NULL;
    entry.command_name = NULL;
    entry.permissions = policy->default_permissions;
    entry.details = "Security policy saved to file";

    add_audit_log_entry(ctx, policy, &entry);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Create security context for a component
 */
polycall_core_error_t security_policy_create_context(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_component_t *component, security_context_t **security_ctx) {
  if (!ctx || !policy || !component || !security_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get component info
  polycall_component_info_t component_info;
  polycall_core_error_t info_result =
      polycall_micro_component_get_info(ctx, component, &component_info);

  if (info_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, info_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get component info");
    return info_result;
  }

  // Allocate security context
  security_context_t *new_ctx =
      polycall_core_malloc(ctx, sizeof(security_context_t));
  if (!new_ctx) {
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate security context");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize context
  memset(new_ctx, 0, sizeof(security_context_t));

  // Copy component name
  size_t name_len = strlen(component_info.name) + 1;
  char *component_name = polycall_core_malloc(ctx, name_len);
  if (!component_name) {
    polycall_core_free(ctx, new_ctx);
    POLYCALL_ERROR_SET(
        ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
        POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component name");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  memcpy(component_name, component_info.name, name_len);
  new_ctx->component_name = component_name;

  // Get component permissions or use defaults
  pthread_mutex_lock(&policy->mutex);

  polycall_permission_t *permissions =
      find_component_permissions(policy, component_info.name);
  if (permissions) {
    new_ctx->permissions = *permissions;
  } else {
    // Component not found in permission map, add it with default permissions
    if (policy->permission_map_size >= policy->permission_map_capacity) {
      // Expand permission map
      size_t new_capacity = policy->permission_map_capacity * 2;
      permission_map_entry_t *new_map =
          polycall_core_realloc(ctx, policy->permission_map,
                                new_capacity * sizeof(permission_map_entry_t));

      if (!new_map) {
        pthread_mutex_unlock(&policy->mutex);
        polycall_core_free(ctx, (void *)new_ctx->component_name);
        polycall_core_free(ctx, new_ctx);
        POLYCALL_ERROR_SET(
            ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
            POLYCALL_ERROR_SEVERITY_ERROR, "Failed to expand permission map");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
      }

      policy->permission_map = new_map;
      policy->permission_map_capacity = new_capacity;
    }

    // Add component to permission map
    permission_map_entry_t *entry =
        &policy->permission_map[policy->permission_map_size++];
    entry->component_name = component_name;
    entry->permissions = policy->default_permissions;

    new_ctx->permissions = policy->default_permissions;
  }

  pthread_mutex_unlock(&policy->mutex);

  // Set trust and escalation flags
  new_ctx->is_trusted = (component_info.isolation <= POLYCALL_ISOLATION_MEMORY);
  new_ctx->can_escalate = policy->allow_privilege_escalation;

  // Log component security context creation
  if (policy->audit_events) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_COMPONENT_CREATED;
    entry.component_name = new_ctx->component_name;
    entry.command_name = NULL;
    entry.permissions = new_ctx->permissions;
    entry.details = "Component security context created";

    add_audit_log_entry(ctx, policy, &entry);
  }

  *security_ctx = new_ctx;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Check if a component has a permission
 */
polycall_core_error_t security_policy_check_permission(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_component_t *component, polycall_permission_t permission,
    bool *has_permission) {
  if (!ctx || !policy || !component || !has_permission) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // If policy enforcement is disabled, always grant permission
  if (!policy->enforce_policy) {
    *has_permission = true;
    return POLYCALL_CORE_SUCCESS;
  }

  // Get component info
  polycall_component_info_t component_info;
  polycall_core_error_t info_result =
      polycall_micro_component_get_info(ctx, component, &component_info);

  if (info_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, info_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get component info");
    return info_result;
  }

  // Get component permissions
  pthread_mutex_lock(&policy->mutex);

  polycall_permission_t *permissions =
      find_component_permissions(policy, component_info.name);
  polycall_permission_t component_permissions =
      permissions ? *permissions : policy->default_permissions;

  pthread_mutex_unlock(&policy->mutex);

  // Check if component has the required permission
  *has_permission = (component_permissions & permission) == permission;

  // Log permission check if it failed
  if (policy->audit_events && !*has_permission) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_PERMISSION_DENIED;
    entry.component_name = component_info.name;
    entry.command_name = NULL;
    entry.permissions = permission;
    entry.details = "Permission check failed";

    add_audit_log_entry(ctx, policy, &entry);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Grant permission to a component
 */
polycall_core_error_t security_policy_grant_permission(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_component_t *component, polycall_permission_t permission) {
  if (!ctx || !policy || !component) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get component info
  polycall_component_info_t component_info;
  polycall_core_error_t info_result =
      polycall_micro_component_get_info(ctx, component, &component_info);

  if (info_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, info_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get component info");
    return info_result;
  }

  // Update component permissions
  pthread_mutex_lock(&policy->mutex);

  polycall_permission_t *permissions =
      find_component_permissions(policy, component_info.name);

  if (permissions) {
    // Component found, update permissions
    polycall_permission_t old_permissions = *permissions;
    *permissions |= permission;

    // Log permission grant
    if (policy->audit_events && old_permissions != *permissions) {
      polycall_security_audit_entry_t entry;
      entry.timestamp = create_timestamp();
      entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
      entry.component_name = component_info.name;
      entry.command_name = NULL;
      entry.permissions = permission;
      entry.details = "Permission granted";

      add_audit_log_entry(ctx, policy, &entry);
    }
  } else {
    // Component not found, add it
    if (policy->permission_map_size >= policy->permission_map_capacity) {
      // Expand permission map
      size_t new_capacity = policy->permission_map_capacity * 2;
      permission_map_entry_t *new_map =
          polycall_core_realloc(ctx, policy->permission_map,
                                new_capacity * sizeof(permission_map_entry_t));

      if (!new_map) {
        pthread_mutex_unlock(&policy->mutex);
        POLYCALL_ERROR_SET(
            ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
            POLYCALL_ERROR_SEVERITY_ERROR, "Failed to expand permission map");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
      }

      policy->permission_map = new_map;
      policy->permission_map_capacity = new_capacity;
    }

    // Copy component name
    size_t name_len = strlen(component_info.name) + 1;
    char *component_name = polycall_core_malloc(ctx, name_len);
    if (!component_name) {
      pthread_mutex_unlock(&policy->mutex);
      POLYCALL_ERROR_SET(
          ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
          POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component name");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    memcpy(component_name, component_info.name, name_len);

    // Add component to permission map
    permission_map_entry_t *entry =
        &policy->permission_map[policy->permission_map_size++];
    entry->component_name = component_name;
    entry->permissions = policy->default_permissions | permission;

    // Log permission grant
    if (policy->audit_events) {
      polycall_security_audit_entry_t audit_entry;
      audit_entry.timestamp = create_timestamp();
      audit_entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
      audit_entry.component_name = component_info.name;
      audit_entry.command_name = NULL;
      audit_entry.permissions = permission;
      audit_entry.details = "Permission granted (new component)";

      add_audit_log_entry(ctx, policy, &audit_entry);
    }
  }

  pthread_mutex_unlock(&policy->mutex);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Verify command execution
 */
polycall_core_error_t security_policy_verify_command(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_component_t *component, polycall_micro_command_t *command,
    bool *is_allowed) {
  if (!ctx || !policy || !component || !command || !is_allowed) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // If policy enforcement is disabled, always allow command execution
  if (!policy->enforce_policy || !policy->verify_commands) {
    *is_allowed = true;
    return POLYCALL_CORE_SUCCESS;
  }

  // Get component info
  polycall_component_info_t component_info;
  polycall_core_error_t info_result =
      polycall_micro_component_get_info(ctx, component, &component_info);

  if (info_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, info_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get component info");
    return info_result;
  }

  // Get command security attributes
  command_security_attributes_t *attrs = NULL;
  polycall_core_error_t attr_result =
      security_policy_get_command_attributes(ctx, policy, command, &attrs);

  if (attr_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, attr_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get command security attributes");
    return attr_result;
  }

  // Get component permissions
  pthread_mutex_lock(&policy->mutex);

  polycall_permission_t *permissions =
      find_component_permissions(policy, component_info.name);
  polycall_permission_t component_permissions =
      permissions ? *permissions : policy->default_permissions;

  pthread_mutex_unlock(&policy->mutex);

  // Check if component has the required permissions
  *is_allowed = (component_permissions & attrs->required_permissions) ==
                attrs->required_permissions;

  // Check component restrictions
  if (*is_allowed && attrs->restricted_to_component) {
    *is_allowed =
        (strcmp(component_info.name, attrs->restricted_to_component) == 0);
  }

  // Check untrusted execution
  if (*is_allowed && !attrs->allow_untrusted) {
    *is_allowed = (component_info.isolation <= POLYCALL_ISOLATION_MEMORY);
  }

  // Log command verification if it failed
  if (policy->audit_events && !*is_allowed) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_PERMISSION_DENIED;
    entry.component_name = component_info.name;
    entry.command_name = NULL; // Would use command->name if accessible
    entry.permissions = attrs->required_permissions;
    entry.details = "Command verification failed";

    add_audit_log_entry(ctx, policy, &entry);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Audit security event
 */
polycall_core_error_t security_policy_audit_event(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_security_event_t event_type, polycall_micro_component_t *component,
    polycall_micro_command_t *command, const char *details) {
  if (!ctx || !policy) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // If auditing is disabled, do nothing
  if (!policy->audit_events) {
    return POLYCALL_CORE_SUCCESS;
  }

  // Create audit entry
  polycall_security_audit_entry_t entry;
  entry.timestamp = create_timestamp();
  entry.event_type = event_type;

  // Get component name if provided
  if (component) {
    polycall_component_info_t component_info;
    polycall_core_error_t info_result =
        polycall_micro_component_get_info(ctx, component, &component_info);

    if (info_result == POLYCALL_CORE_SUCCESS) {
      entry.component_name = component_info.name;
    } else {
      entry.component_name = "unknown";
    }
  } else {
    entry.component_name = NULL;
  }

  // Get command name if provided (assuming command has a name field)
  entry.command_name = NULL; // Would use command->name if accessible

  // Set permissions (default to none)
  entry.permissions = POLYCALL_PERMISSION_NONE;

  // Set details
  entry.details = details ? details : "No details provided";

  // Add entry to log
  polycall_core_error_t log_result = add_audit_log_entry(ctx, policy, &entry);
  if (log_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, log_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to add audit log entry");
    return log_result;
  }

  // Notify callbacks
  notify_audit_callbacks(ctx, NULL, policy, &entry);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Register security audit callback
 */
polycall_core_error_t security_policy_register_audit_callback(
    polycall_core_context_t *ctx, security_policy_t *policy,
    security_audit_callback_t callback, void *user_data) {
  if (!ctx || !policy || !callback) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  pthread_mutex_lock(&policy->mutex);

  // Check if we have space for another callback
  if (policy->audit_callback_count >= MAX_AUDIT_CALLBACKS) {
    pthread_mutex_unlock(&policy->mutex);
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                       POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Maximum number of audit callbacks reached");
    return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
  }

  // Add callback
  policy->audit_callbacks[policy->audit_callback_count].callback = callback;
  policy->audit_callbacks[policy->audit_callback_count].user_data = user_data;
  policy->audit_callback_count++;

  pthread_mutex_unlock(&policy->mutex);

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get security attributes for a command
 */
polycall_core_error_t security_policy_get_command_attributes(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_command_t *command,
    command_security_attributes_t **attributes) {
  if (!ctx || !policy || !command || !attributes) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // In a real implementation, this would retrieve the attributes from the
  // command For this implementation, we'll create default attributes

  return security_create_command_attributes(ctx, attributes,
                                            POLYCALL_PERMISSION_EXECUTE);
}

/**
 * @brief Create default security policy configuration
 */
security_policy_config_t security_policy_create_default_config(void) {
  security_policy_config_t config;

  config.enforce_policy = true;
  config.default_permissions =
      POLYCALL_PERMISSION_EXECUTE | POLYCALL_PERMISSION_READ;
  config.policy_file = NULL;
  config.allow_privilege_escalation = false;
  config.verify_commands = true;
  config.audit_events = true;
  config.audit_log_file = NULL;

  return config;
}

/**
 * @brief Create command security attributes
 */
polycall_core_error_t
security_create_command_attributes(polycall_core_context_t *ctx,
                                   command_security_attributes_t **attributes,
                                   polycall_permission_t required_permissions) {
  if (!ctx || !attributes) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Allocate attributes structure
  command_security_attributes_t *attrs =
      polycall_core_malloc(ctx, sizeof(command_security_attributes_t));
  if (!attrs) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO,
                       POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to allocate command security attributes");
    return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
  }

  // Initialize attributes
  memset(attrs, 0, sizeof(command_security_attributes_t));
  attrs->required_permissions = required_permissions;
  attrs->allow_untrusted = false;
  attrs->require_verification = true;
  attrs->restricted_to_component = NULL;
  attrs->audit_execution = true;

  *attributes = attrs;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Integrate with FFI security subsystem
 */
polycall_core_error_t
security_policy_integrate_ffi(polycall_core_context_t *ctx,
                              security_policy_t *policy,
                              ffi_security_context_t *ffi_security) {
  if (!ctx || !policy || !ffi_security) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // In a real implementation, this would integrate with the FFI security
  // subsystem For this implementation, we'll just log the event

  if (policy->audit_events) {
    polycall_security_audit_entry_t entry;
    entry.timestamp = create_timestamp();
    entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
    entry.component_name = NULL;
    entry.command_name = NULL;
    entry.permissions = POLYCALL_PERMISSION_NONE;
    entry.details = "Integrated with FFI security subsystem";

    add_audit_log_entry(ctx, policy, &entry);
  }

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Revoke permission from a component
 */
polycall_core_error_t security_policy_revoke_permission(
    polycall_core_context_t *ctx, security_policy_t *policy,
    polycall_micro_component_t *component, polycall_permission_t permission) {
  if (!ctx || !policy || !component) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Get component info
  polycall_component_info_t component_info;
  polycall_core_error_t info_result =
      polycall_micro_component_get_info(ctx, component, &component_info);

  if (info_result != POLYCALL_CORE_SUCCESS) {
    POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_MICRO, info_result,
                       POLYCALL_ERROR_SEVERITY_ERROR,
                       "Failed to get component info");
    return info_result;
  }

  // Update component permissions
  pthread_mutex_lock(&policy->mutex);

  polycall_permission_t *permissions =
      find_component_permissions(policy, component_info.name);

  if (permissions) {
    // Component found, update permissions
    polycall_permission_t old_permissions = *permissions;
    *permissions &= ~permission;

    // Log permission revocation
    if (policy->audit_events && old_permissions != *permissions) {
      polycall_security_audit_entry_t entry;
      entry.timestamp = create_timestamp();
      entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
      entry.component_name = component_info.name;
      entry.command_name = NULL;
      entry.permissions = permission;
      entry.details = "Permission revoked";

      add_audit_log_entry(ctx, policy, &entry);
    }
  } else {
    // Component not found, add it with default permissions minus the revoked
    // permission
    if (policy->permission_map_size >= policy->permission_map_capacity) {
      // Expand permission map
      size_t new_capacity = policy->permission_map_capacity * 2;
      permission_map_entry_t *new_map =
          polycall_core_realloc(ctx, policy->permission_map,
                                new_capacity * sizeof(permission_map_entry_t));

      if (!new_map) {
        pthread_mutex_unlock(&policy->mutex);
        POLYCALL_ERROR_SET(
            ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
            POLYCALL_ERROR_SEVERITY_ERROR, "Failed to expand permission map");
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
      }

      policy->permission_map = new_map;
      policy->permission_map_capacity = new_capacity;
    }

    // Copy component name
    size_t name_len = strlen(component_info.name) + 1;
    char *component_name = polycall_core_malloc(ctx, name_len);
    if (!component_name) {
      pthread_mutex_unlock(&policy->mutex);
      POLYCALL_ERROR_SET(
          ctx, POLYCALL_ERROR_SOURCE_MICRO, POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
          POLYCALL_ERROR_SEVERITY_ERROR, "Failed to allocate component name");
      return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }

    memcpy(component_name, component_info.name, name_len);

    // Add component to permission map with default permissions minus the
    // revoked permission
    permission_map_entry_t *entry =
        &policy->permission_map[policy->permission_map_size++];
    entry->component_name = component_name;
    entry->permissions = policy->default_permissions & ~permission;

    // Log permission revocation
    if (policy->audit_events) {
      polycall_security_audit_entry_t audit_entry;
      audit_entry.timestamp = create_timestamp();
      audit_entry.event_type = POLYCALL_SECURITY_EVENT_POLICY_UPDATED;
      audit_entry.component_name = component_info.name;
      audit_entry.command_name = NULL;
      audit_entry.permissions = permission;
      audit_entry.details = "Permission revoked (new component)";

      add_audit_log_entry(ctx, policy, &audit_entry);
    }
  }

  pthread_mutex_unlock(&policy->mutex);

  return POLYCALL_CORE_SUCCESS;
}
