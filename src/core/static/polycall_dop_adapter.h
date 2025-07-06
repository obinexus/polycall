// ==================================================================
// LibPolyCall DOP Adapter Interface Specification
// OBINexus Aegis Project - Component Standard API
//
// Integrates with LibPolyCall's hierarchical state management,
// Zero Trust security model, and micro command architecture
// ==================================================================
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations for LibPolyCall integration
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_core_error polycall_core_error_t;
typedef struct polycall_protocol_context polycall_protocol_context_t;
typedef struct polycall_micro_context polycall_micro_context_t;

// DOP Adapter specific types
typedef struct polycall_dop_adapter_context polycall_dop_adapter_context_t;
typedef struct polycall_dop_adapter_config polycall_dop_adapter_config_t;
typedef struct polycall_dop_adapter_component polycall_dop_adapter_component_t;

// Component type enumeration for micro isolation
typedef enum {
  POLYCALL_DOP_COMPONENT_REACT = 1,
  POLYCALL_DOP_COMPONENT_VUE = 2,
  POLYCALL_DOP_COMPONENT_NODE = 3,
  POLYCALL_DOP_COMPONENT_PYTHON = 4,
  POLYCALL_DOP_COMPONENT_WASM = 5,
  POLYCALL_DOP_COMPONENT_CUSTOM = 99
} polycall_dop_component_type_t;

// Security isolation levels for Zero Trust integration
typedef enum {
  POLYCALL_DOP_ISOLATION_NONE = 0,
  POLYCALL_DOP_ISOLATION_SANDBOX = 1,
  POLYCALL_DOP_ISOLATION_CONTAINER = 2,
  POLYCALL_DOP_ISOLATION_VM = 3
} polycall_dop_isolation_level_t;

// Configuration structure for DOP Adapter initialization
struct polycall_dop_adapter_config {
  polycall_dop_component_type_t component_type;
  polycall_dop_isolation_level_t isolation_level;

  // Security policy configuration
  const char *allowed_connections; // JSON array of allowed endpoints
  const char *permission_policy;   // JSON permission specification

  // Resource limits for micro isolation
  uint64_t memory_limit_bytes;
  uint32_t cpu_time_limit_ms;
  uint32_t io_operations_limit;

  // State validation configuration
  bool enable_state_validation;
  bool enable_schema_enforcement;
  const char *state_schema; // JSON schema for state validation

  // Integration settings
  void *user_data;
  uint32_t flags;
};

// DOP Adapter context initialization with LibPolyCall integration
polycall_core_error_t polycall_dop_adapter_init(
    polycall_core_context_t *ctx, // Core LibPolyCall context
    polycall_protocol_context_t
        *proto_ctx, // Protocol context for state integration
    polycall_micro_context_t *micro_ctx,          // Micro command context
    polycall_dop_adapter_context_t **adapter_ctx, // Output DOP adapter context
    const polycall_dop_adapter_config_t *config   // Configuration
);

// Destroy DOP Adapter context with proper resource cleanup
polycall_core_error_t
polycall_dop_adapter_destroy(polycall_core_context_t *ctx,
                             polycall_dop_adapter_context_t *adapter_ctx);

// Load component definition with security validation
polycall_core_error_t polycall_dop_adapter_load_definition(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    const char *definition, size_t definition_length);

// Convert to functional representation with state validation
polycall_core_error_t polycall_dop_adapter_to_functional(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    char **functional_json, // Output JSON (caller must free)
    size_t *json_length);

// Convert to OOP representation with state validation
polycall_core_error_t
polycall_dop_adapter_to_oop(polycall_core_context_t *ctx,
                            polycall_dop_adapter_context_t *adapter_ctx,
                            char **oop_json, // Output JSON (caller must free)
                            size_t *json_length);

// Render component with security boundary enforcement
polycall_core_error_t polycall_dop_adapter_render(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    const char *render_context, // Rendering context (DOM, server-side, etc.)
    char **rendered_output,     // Output (caller must free)
    size_t *output_length);

// Validate component state against schema and security policy
polycall_core_error_t polycall_dop_adapter_validate_state(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    const char *state_json, bool *is_valid,
    char **validation_errors // Output errors if any (caller must free)
);

// Execute component method with micro isolation enforcement
polycall_core_error_t polycall_dop_adapter_invoke_method(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    const char *method_name,
    const char *method_params, // JSON parameters
    char **method_result,      // Output result (caller must free)
    size_t *result_length);

// Reset adapter state with security audit trail
polycall_core_error_t
polycall_dop_adapter_reset(polycall_core_context_t *ctx,
                           polycall_dop_adapter_context_t *adapter_ctx,
                           bool preserve_security_context);

// Get component telemetry data for GUID tracking integration
polycall_core_error_t polycall_dop_adapter_get_telemetry(
    polycall_core_context_t *ctx, polycall_dop_adapter_context_t *adapter_ctx,
    char **telemetry_json, // Output telemetry (caller must free)
    size_t *json_length);

#ifdef __cplusplus
}
#endif
