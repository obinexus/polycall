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
#include "polycall/core/config/factory/config_factory_mergers.h"

 * @file config_factory_mergers.c
 * @brief Component-specific configuration merging implementation
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file implements the component-specific configuration merging functions
 * for the configuration factory.
 */

#include "polycall/core/config/factory/config_factory_mergers.h"

/*-------------------------------------------------------------------------*/
/* Micro component configuration merging                                   */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to micro component
 */
void apply_global_micro_config(polycall_core_context_t *ctx,
                               polycall_global_config_context_t *global_ctx,
                               micro_component_config_t *micro_config) {
  if (!ctx || !global_ctx || !micro_config) {
    return;
  }

  // Get the micro section from global configuration
  polycall_config_node_t *micro_section =
      polycall_global_config_get_section(ctx, global_ctx, "micro");

  if (!micro_section) {
    return; // No micro section in global config
  }

  // Apply global defaults for all components
  polycall_config_node_t *defaults_section =
      polycall_config_find_node(micro_section, "defaults");

  if (defaults_section) {
    // Isolation level
    char isolation_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, defaults_section, "isolation_level", isolation_str,
            sizeof(isolation_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(isolation_str, "none") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_NONE;
      } else if (strcmp(isolation_str, "memory") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_MEMORY;
      } else if (strcmp(isolation_str, "resources") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_RESOURCES;
      } else if (strcmp(isolation_str, "security") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_SECURITY;
      } else if (strcmp(isolation_str, "strict") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_STRICT;
      }
    }

    // Resource quotas
    int64_t memory_quota;
    if (polycall_global_config_get_int_from_node(
            ctx, global_ctx, defaults_section, "memory_quota", &memory_quota) ==
        POLYCALL_CORE_SUCCESS) {
      micro_config->memory_quota = (size_t)memory_quota;
    }

    int64_t cpu_quota;
    if (polycall_global_config_get_int_from_node(
            ctx, global_ctx, defaults_section, "cpu_quota", &cpu_quota) ==
        POLYCALL_CORE_SUCCESS) {
      micro_config->cpu_quota = (uint32_t)cpu_quota;
    }

    int64_t io_quota;
    if (polycall_global_config_get_int_from_node(
            ctx, global_ctx, defaults_section, "io_quota", &io_quota) ==
        POLYCALL_CORE_SUCCESS) {
      micro_config->io_quota = (uint32_t)io_quota;
    }

    // Security settings
    bool require_auth;
    if (polycall_global_config_get_bool_from_node(
            ctx, global_ctx, defaults_section, "require_authentication",
            &require_auth) == POLYCALL_CORE_SUCCESS) {
      micro_config->require_authentication = require_auth;
    }

    bool audit_access;
    if (polycall_global_config_get_bool_from_node(
            ctx, global_ctx, defaults_section, "audit_access", &audit_access) ==
        POLYCALL_CORE_SUCCESS) {
      micro_config->audit_access = audit_access;
    }

    // Other settings
    bool enforce_quotas;
    if (polycall_global_config_get_bool_from_node(
            ctx, global_ctx, defaults_section, "enforce_quotas",
            &enforce_quotas) == POLYCALL_CORE_SUCCESS) {
      micro_config->enforce_quotas = enforce_quotas;
    }
  }

  // Apply component-specific configuration if available
  char component_path[256];
  snprintf(component_path, sizeof(component_path), "components.%s",
           micro_config->name);

  polycall_config_node_t *component_section =
      polycall_config_find_node(micro_section, component_path);

  if (component_section) {
    // Override with component-specific settings (same properties as above)

    // Isolation level
    char isolation_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, component_section, "isolation_level",
            isolation_str, sizeof(isolation_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(isolation_str, "none") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_NONE;
      } else if (strcmp(isolation_str, "memory") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_MEMORY;
      } else if (strcmp(isolation_str, "resources") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_RESOURCES;
      } else if (strcmp(isolation_str, "security") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_SECURITY;
      } else if (strcmp(isolation_str, "strict") == 0) {
        micro_config->isolation_level = POLYCALL_ISOLATION_STRICT;
      }
    }

    // Resource quotas (same pattern as above)
    int64_t memory_quota;
    if (polycall_global_config_get_int_from_node(
            ctx, global_ctx, component_section, "memory_quota",
            &memory_quota) == POLYCALL_CORE_SUCCESS) {
      micro_config->memory_quota = (size_t)memory_quota;
    }

    // Additional component-specific settings...
  }
}

/**
 * @brief Apply binding configuration to micro component
 */
void apply_binding_micro_config(polycall_core_context_t *ctx,
                                polycall_binding_config_context_t *binding_ctx,
                                const char *component_name,
                                micro_component_config_t *micro_config) {
  if (!ctx || !binding_ctx || !component_name || !micro_config) {
    return;
  }

  // Check if component name matches
  if (strcmp(component_name, micro_config->name) != 0) {
    return;
  }

  // Override with binding-specific settings

  // Isolation level
  char isolation_str[32];
  if (polycall_binding_config_get_string(
          ctx, binding_ctx, "micro", "isolation_level", isolation_str,
          sizeof(isolation_str)) == POLYCALL_CORE_SUCCESS) {

    if (strcmp(isolation_str, "none") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_NONE;
    } else if (strcmp(isolation_str, "memory") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_MEMORY;
    } else if (strcmp(isolation_str, "resources") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_RESOURCES;
    } else if (strcmp(isolation_str, "security") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_SECURITY;
    } else if (strcmp(isolation_str, "strict") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_STRICT;
    }
  }

  // Resource quotas
  int64_t memory_quota;
  if (polycall_binding_config_get_int(ctx, binding_ctx, "micro", "memory_quota",
                                      &memory_quota) == POLYCALL_CORE_SUCCESS) {
    micro_config->memory_quota = (size_t)memory_quota;
  }

  int64_t cpu_quota;
  if (polycall_binding_config_get_int(ctx, binding_ctx, "micro", "cpu_quota",
                                      &cpu_quota) == POLYCALL_CORE_SUCCESS) {
    micro_config->cpu_quota = (uint32_t)cpu_quota;
  }

  int64_t io_quota;
  if (polycall_binding_config_get_int(ctx, binding_ctx, "micro", "io_quota",
                                      &io_quota) == POLYCALL_CORE_SUCCESS) {
    micro_config->io_quota = (uint32_t)io_quota;
  }

  // Security settings
  bool require_auth;
  if (polycall_binding_config_get_bool(
          ctx, binding_ctx, "micro", "require_authentication", &require_auth) ==
      POLYCALL_CORE_SUCCESS) {
    micro_config->require_authentication = require_auth;
  }

  bool audit_access;
  if (polycall_binding_config_get_bool(ctx, binding_ctx, "micro",
                                       "audit_access", &audit_access) ==
      POLYCALL_CORE_SUCCESS) {
    micro_config->audit_access = audit_access;
  }

  // Component-specific binding section (highest precedence)
  char section_name[256];
  snprintf(section_name, sizeof(section_name), "micro.%s", component_name);

  // Override with component-specific binding settings

  // Isolation level
  if (polycall_binding_config_get_string(
          ctx, binding_ctx, section_name, "isolation_level", isolation_str,
          sizeof(isolation_str)) == POLYCALL_CORE_SUCCESS) {

    if (strcmp(isolation_str, "none") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_NONE;
    } else if (strcmp(isolation_str, "memory") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_MEMORY;
    } else if (strcmp(isolation_str, "resources") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_RESOURCES;
    } else if (strcmp(isolation_str, "security") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_SECURITY;
    } else if (strcmp(isolation_str, "strict") == 0) {
      micro_config->isolation_level = POLYCALL_ISOLATION_STRICT;
    }
  }

  // Additional component-specific binding settings...
}

/*-------------------------------------------------------------------------*/
/* Edge component configuration merging                                    */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to edge component
 */
void apply_global_edge_config(polycall_core_context_t *ctx,
                              polycall_global_config_context_t *global_ctx,
                              polycall_edge_component_config_t *edge_config) {
  if (!ctx || !global_ctx || !edge_config) {
    return;
  }

  // Get the edge section from global configuration
  polycall_config_node_t *edge_section =
      polycall_global_config_get_section(ctx, global_ctx, "edge");

  if (!edge_section) {
    return; // No edge section in global config
  }

  // Apply global defaults for all edge components
  polycall_config_node_t *component_section =
      polycall_config_find_node(edge_section, "component");

  if (component_section) {
    // Component type
    char type_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, component_section, "type", type_str,
            sizeof(type_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(type_str, "compute") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_COMPUTE;
      } else if (strcmp(type_str, "storage") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_STORAGE;
      } else if (strcmp(type_str, "gateway") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_GATEWAY;
      } else if (strcmp(type_str, "sensor") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_SENSOR;
      } else if (strcmp(type_str, "actuator") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_ACTUATOR;
      } else if (strcmp(type_str, "coordinator") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_COORDINATOR;
      } else if (strcmp(type_str, "custom") == 0) {
        edge_config->type = EDGE_COMPONENT_TYPE_CUSTOM;
      }
    }

    // Task policy
    char policy_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, component_section, "task_policy", policy_str,
            sizeof(policy_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(policy_str, "queue") == 0) {
        edge_config->task_policy = EDGE_TASK_POLICY_QUEUE;
      } else if (strcmp(policy_str, "immediate") == 0) {
        edge_config->task_policy = EDGE_TASK_POLICY_IMMEDIATE;
      } else if (strcmp(policy_str, "priority") == 0) {
        edge_config->task_policy = EDGE_TASK_POLICY_PRIORITY;
      } else if (strcmp(policy_str, "deadline") == 0) {
        edge_config->task_policy = EDGE_TASK_POLICY_DEADLINE;
      } else if (strcmp(policy_str, "fair_share") == 0) {
        edge_config->task_policy = EDGE_TASK_POLICY_FAIR_SHARE;
      }
    }

    // Additional edge component settings...
  }

  // Apply component-specific configuration if available
  char component_path[256];
  snprintf(component_path, sizeof(component_path), "components.%s",
           edge_config->name);

  polycall_config_node_t *specific_section =
      polycall_config_find_node(edge_section, component_path);

  if (specific_section) {
    // Override with component-specific settings
    // (Same pattern as above)
  }
}

/**
 * @brief Apply binding configuration to edge component
 */
void apply_binding_edge_config(polycall_core_context_t *ctx,
                               polycall_binding_config_context_t *binding_ctx,
                               const char *component_name,
                               polycall_edge_component_config_t *edge_config) {
  if (!ctx || !binding_ctx || !component_name || !edge_config) {
    return;
  }

  // Check if component name matches
  if (strcmp(component_name, edge_config->name) != 0) {
    return;
  }

  // Apply binding configuration for edge components
  // (Similar pattern to micro component binding config)

  // Component-specific binding section (highest precedence)
  char section_name[256];
  snprintf(section_name, sizeof(section_name), "edge.%s", component_name);

  // Override with component-specific binding settings
  // (Similar pattern to micro component binding config)
}

/*-------------------------------------------------------------------------*/
/* Network configuration merging                                           */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to network component
 */
void apply_global_network_config(polycall_core_context_t *ctx,
                                 polycall_global_config_context_t *global_ctx,
                                 polycall_network_config_t *network_config) {
  if (!ctx || !global_ctx || !network_config) {
    return;
  }

  // Get the network section from global configuration
  polycall_config_node_t *network_section =
      polycall_global_config_get_section(ctx, global_ctx, "network");

  if (!network_section) {
    return; // No network section in global config
  }

  // Apply general network settings

  // Buffer size
  int64_t buffer_size;
  if (polycall_global_config_get_int_from_node(ctx, global_ctx, network_section,
                                               "buffer_size", &buffer_size) ==
      POLYCALL_CORE_SUCCESS) {
    polycall_network_config_set_int(ctx, network_config, SECTION_GENERAL,
                                    "buffer_size", (int)buffer_size);
  }

  // Connection timeout
  int64_t connection_timeout;
  if (polycall_global_config_get_int_from_node(
          ctx, global_ctx, network_section, "connection_timeout",
          &connection_timeout) == POLYCALL_CORE_SUCCESS) {
    polycall_network_config_set_uint(ctx, network_config, SECTION_GENERAL,
                                     "connection_timeout",
                                     (unsigned int)connection_timeout);
  }

  // Additional network settings...

  // Apply security settings if available
  polycall_config_node_t *security_section =
      polycall_config_find_node(network_section, "security");

  if (security_section) {
    // TLS settings
    bool enable_tls;
    if (polycall_global_config_get_bool_from_node(
            ctx, global_ctx, security_section, "enable_tls", &enable_tls) ==
        POLYCALL_CORE_SUCCESS) {
      polycall_network_config_set_bool(ctx, network_config, SECTION_SECURITY,
                                       "enable_tls", enable_tls);
    }

    // Additional security settings...
  }
}

/**
 * @brief Apply binding configuration to network component
 */
void apply_binding_network_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    polycall_network_config_t *network_config) {
  if (!ctx || !binding_ctx || !component_name || !network_config) {
    return;
  }

  // Apply binding configuration for network
  // Similar pattern to other binding configurations
}

/*-------------------------------------------------------------------------*/
/* Protocol configuration merging                                          */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to protocol component
 */
void apply_global_protocol_config(polycall_core_context_t *ctx,
                                  polycall_global_config_context_t *global_ctx,
                                  protocol_config_t *protocol_config) {
  if (!ctx || !global_ctx || !protocol_config) {
    return;
  }

  // Get the protocol section from global configuration
  polycall_config_node_t *protocol_section =
      polycall_global_config_get_section(ctx, global_ctx, "protocol");

  if (!protocol_section) {
    return; // No protocol section in global config
  }

  // Apply core settings
  polycall_config_node_t *core_section =
      polycall_config_find_node(protocol_section, "core");

  if (core_section) {
    // Transport type
    char transport_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, core_section, "transport_type", transport_str,
            sizeof(transport_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(transport_str, "tcp") == 0) {
        protocol_config->core.transport_type = PROTOCOL_TRANSPORT_TCP;
      } else if (strcmp(transport_str, "udp") == 0) {
        protocol_config->core.transport_type = PROTOCOL_TRANSPORT_UDP;
      } else if (strcmp(transport_str, "websocket") == 0) {
        protocol_config->core.transport_type = PROTOCOL_TRANSPORT_WEBSOCKET;
      } else if (strcmp(transport_str, "unix") == 0) {
        protocol_config->core.transport_type = PROTOCOL_TRANSPORT_UNIX;
      }
    }

    // Encoding format
    char encoding_str[32];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, core_section, "encoding_format", encoding_str,
            sizeof(encoding_str)) == POLYCALL_CORE_SUCCESS) {

      if (strcmp(encoding_str, "json") == 0) {
        protocol_config->core.encoding_format = PROTOCOL_ENCODING_JSON;
      } else if (strcmp(encoding_str, "msgpack") == 0) {
        protocol_config->core.encoding_format = PROTOCOL_ENCODING_MSGPACK;
      } else if (strcmp(encoding_str, "protobuf") == 0) {
        protocol_config->core.encoding_format = PROTOCOL_ENCODING_PROTOBUF;
      } else if (strcmp(encoding_str, "binary") == 0) {
        protocol_config->core.encoding_format = PROTOCOL_ENCODING_BINARY;
      }
    }

    // Additional core settings...
  }

  // Apply TLS settings if available
  polycall_config_node_t *tls_section =
      polycall_config_find_node(protocol_section, "tls");

  if (tls_section) {
    // TLS certificate file
    char cert_file[256];
    if (polycall_global_config_get_string_from_node(
            ctx, global_ctx, tls_section, "cert_file", cert_file,
            sizeof(cert_file)) == POLYCALL_CORE_SUCCESS) {

      if (protocol_config->tls.cert_file) {
        polycall_core_free(ctx, (void *)protocol_config->tls.cert_file);
      }
      protocol_config->tls.cert_file = duplicate_string(ctx, cert_file);
    }

    // Additional TLS settings...
  }

  // Apply other protocol configuration sections as needed
}

/**
 * @brief Apply binding configuration to protocol component
 */
void apply_binding_protocol_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    protocol_config_t *protocol_config) {
  if (!ctx || !binding_ctx || !component_name || !protocol_config) {
    return;
  }

  // Apply binding configuration for protocol
  // Similar pattern to other binding configurations
}

/*-------------------------------------------------------------------------*/
/* FFI configuration merging                                              */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to FFI component
 */
void apply_global_ffi_config(polycall_core_context_t *ctx,
                             polycall_global_config_context_t *global_ctx,
                             polycall_ffi_config_options_t *ffi_config) {
  if (!ctx || !global_ctx || !ffi_config) {
    return;
  }

  // Get the FFI section from global configuration
  polycall_config_node_t *ffi_section =
      polycall_global_config_get_section(ctx, global_ctx, "ffi");

  if (!ffi_section) {
    return; // No FFI section in global config
  }

  // Apply FFI settings

  // Config file path
  char config_file_path[256];
  if (polycall_global_config_get_string_from_node(
          ctx, global_ctx, ffi_section, "config_file_path", config_file_path,
          sizeof(config_file_path)) == POLYCALL_CORE_SUCCESS) {

    if (ffi_config->config_file_path) {
      free((void *)ffi_config->config_file_path);
    }
    ffi_config->config_file_path = strdup(config_file_path);
  }

  // Provider name
  char provider_name[64];
  if (polycall_global_config_get_string_from_node(
          ctx, global_ctx, ffi_section, "provider_name", provider_name,
          sizeof(provider_name)) == POLYCALL_CORE_SUCCESS) {

    if (ffi_config->provider_name) {
      free((void *)ffi_config->provider_name);
    }
    ffi_config->provider_name = strdup(provider_name);
  }

  // Boolean settings
  bool enable_persistence;
  if (polycall_global_config_get_bool_from_node(
          ctx, global_ctx, ffi_section, "enable_persistence",
          &enable_persistence) == POLYCALL_CORE_SUCCESS) {
    ffi_config->enable_persistence = enable_persistence;
  }

  bool enable_change_notification;
  if (polycall_global_config_get_bool_from_node(
          ctx, global_ctx, ffi_section, "enable_change_notification",
          &enable_change_notification) == POLYCALL_CORE_SUCCESS) {
    ffi_config->enable_change_notification = enable_change_notification;
  }

  bool validate_configuration;
  if (polycall_global_config_get_bool_from_node(
          ctx, global_ctx, ffi_section, "validate_configuration",
          &validate_configuration) == POLYCALL_CORE_SUCCESS) {
    ffi_config->validate_configuration = validate_configuration;
  }
}

/**
 * @brief Apply binding configuration to FFI component
 */
void apply_binding_ffi_config(polycall_core_context_t *ctx,
                              polycall_binding_config_context_t *binding_ctx,
                              const char *component_name,
                              polycall_ffi_config_options_t *ffi_config) {
  if (!ctx || !binding_ctx || !component_name || !ffi_config) {
    return;
  }

  // Apply binding configuration for FFI
  // Similar pattern to other binding configurations
}

/*-------------------------------------------------------------------------*/
/* Telemetry configuration merging                                         */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to telemetry component
 */
void apply_global_telemetry_config(
    polycall_core_context_t *ctx, polycall_global_config_context_t *global_ctx,
    polycall_telemetry_config_t *telemetry_config) {
  if (!ctx || !global_ctx || !telemetry_config) {
    return;
  }

  // Get the telemetry section from global configuration
  polycall_config_node_t *telemetry_section =
      polycall_global_config_get_section(ctx, global_ctx, "telemetry");

  if (!telemetry_section) {
    return; // No telemetry section in global config
  }

  // Apply telemetry settings

  // Enable telemetry
  bool enable_telemetry;
  if (polycall_global_config_get_bool_from_node(
          ctx, global_ctx, telemetry_section, "enable_telemetry",
          &enable_telemetry) == POLYCALL_CORE_SUCCESS) {
    telemetry_config->enable_telemetry = enable_telemetry;
  }

  // Minimum severity
  int64_t min_severity;
  if (polycall_global_config_get_int_from_node(
          ctx, global_ctx, telemetry_section, "min_severity", &min_severity) ==
      POLYCALL_CORE_SUCCESS) {
    telemetry_config->min_severity =
        (polycall_telemetry_severity_t)min_severity;
  }

  // Maximum event queue size
  int64_t max_event_queue_size;
  if (polycall_global_config_get_int_from_node(
          ctx, global_ctx, telemetry_section, "max_event_queue_size",
          &max_event_queue_size) == POLYCALL_CORE_SUCCESS) {
    telemetry_config->max_event_queue_size = (uint32_t)max_event_queue_size;
  }

  // Format
  int64_t format;
  if (polycall_global_config_get_int_from_node(
          ctx, global_ctx, telemetry_section, "format", &format) ==
      POLYCALL_CORE_SUCCESS) {
    telemetry_config->format = (polycall_telemetry_format_t)format;
  }

  // Output path
  char output_path[256];
  if (polycall_global_config_get_string_from_node(
          ctx, global_ctx, telemetry_section, "output_path", output_path,
          sizeof(output_path)) == POLYCALL_CORE_SUCCESS) {
    strncpy(telemetry_config->output_path, output_path,
            sizeof(telemetry_config->output_path) - 1);
    telemetry_config->output_path[sizeof(telemetry_config->output_path) - 1] =
        '\0';
  }

  // Additional telemetry settings...
}

/**
 * @brief Apply binding configuration to telemetry component
 */
void apply_binding_telemetry_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    polycall_telemetry_config_t *telemetry_config) {
  if (!ctx || !binding_ctx || !component_name || !telemetry_config) {
    return;
  }

  // Apply binding configuration for telemetry
  // Similar pattern to other binding configurations
}

// Helper function to duplicate a string (duplicated from binding_config.c)
static char *duplicate_string(polycall_core_context_t *ctx, const char *src) {
  if (!src) {
    return NULL;
  }

  size_t len = strlen(src) + 1;
  char *dest = polycall_core_malloc(ctx, len);
  if (!dest) {
    return NULL;
  }

  memcpy(dest, src, len);
  return dest;
}
