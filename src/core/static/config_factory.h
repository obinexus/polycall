/**
 * @file config_factory.h
 * @brief Configuration factory interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the configuration factory interface, which provides a
 * unified approach to creating component-specific configurations.
 */

#ifndef POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_H_H
#define POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_H_H

#include "polycall/core/edge/edge_config.h"
#include "polycall/core/ffi/ffi_config.h"
#include "polycall/core/network/network_config.h"
#include "polycall/core/polycall/config/binding_config.h"
#include "polycall/core/polycall/config/global_config.h"
#include "polycall/core/polycall/micro/micro_config.h"
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/telemetry/polycall_telemetry_config.h"
#include "polycall/core/protocol/protocol_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Component types
 */
typedef enum {
  POLYCALL_COMPONENT_TYPE_MICRO,
  POLYCALL_COMPONENT_TYPE_EDGE,
  POLYCALL_COMPONENT_TYPE_NETWORK,
  POLYCALL_COMPONENT_TYPE_PROTOCOL,
  POLYCALL_COMPONENT_TYPE_FFI,
  POLYCALL_COMPONENT_TYPE_TELEMETRY
} polycall_component_type_t;

/**
 * @brief Configuration factory context (opaque)
 */
typedef struct polycall_config_factory_context
    polycall_config_factory_context_t;

/**
 * @brief Initialize configuration factory
 *
 * @param ctx Core context
 * @param factory_ctx Pointer to receive the created factory context
 * @param global_ctx Global configuration context, or NULL to create a new one
 * @param binding_ctx Binding configuration context, or NULL to create a new one
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t
polycall_config_factory_init(polycall_core_context_t *ctx,
                             polycall_config_factory_context_t **factory_ctx,
                             polycall_global_config_context_t *global_ctx,
                             polycall_binding_config_context_t *binding_ctx);

/**
 * @brief Create component configuration based on component type
 *
 * @param ctx Core context
 * @param factory_ctx Factory context
 * @param component_type Component type
 * @param component_name Component name
 * @param component_config Pointer to receive the created configuration
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t polycall_config_factory_create_component_config(
    polycall_core_context_t *ctx,
    polycall_config_factory_context_t *factory_ctx,
    polycall_component_type_t component_type, const char *component_name,
    void **component_config);

/**
 * @brief Reload configurations from disk
 *
 * @param ctx Core context
 * @param factory_ctx Factory context
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t
polycall_config_factory_reload(polycall_core_context_t *ctx,
                               polycall_config_factory_context_t *factory_ctx);

/**
 * @brief Save configurations to disk
 *
 * @param ctx Core context
 * @param factory_ctx Factory context
 * @return polycall_core_error_t Success or error code
 */
polycall_core_error_t
polycall_config_factory_save(polycall_core_context_t *ctx,
                             polycall_config_factory_context_t *factory_ctx);

/**
 * @brief Cleanup configuration factory
 *
 * @param ctx Core context
 * @param factory_ctx Factory context
 */
void polycall_config_factory_cleanup(
    polycall_core_context_t *ctx,
    polycall_config_factory_context_t *factory_ctx);

/*-------------------------------------------------------------------------*/
/* Component-specific configuration merging functions                      */
/*-------------------------------------------------------------------------*/

/**
 * @brief Apply global configuration to micro component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param micro_config Micro component configuration
 */
void apply_global_micro_config(polycall_core_context_t *ctx,
                               polycall_global_config_context_t *global_ctx,
                               micro_component_config_t *micro_config);

/**
 * @brief Apply binding configuration to micro component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param micro_config Micro component configuration
 */
void apply_binding_micro_config(polycall_core_context_t *ctx,
                                polycall_binding_config_context_t *binding_ctx,
                                const char *component_name,
                                micro_component_config_t *micro_config);

/**
 * @brief Apply global configuration to edge component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param edge_config Edge component configuration
 */
void apply_global_edge_config(polycall_core_context_t *ctx,
                              polycall_global_config_context_t *global_ctx,
                              polycall_edge_component_config_t *edge_config);

/**
 * @brief Apply binding configuration to edge component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param edge_config Edge component configuration
 */
void apply_binding_edge_config(polycall_core_context_t *ctx,
                               polycall_binding_config_context_t *binding_ctx,
                               const char *component_name,
                               polycall_edge_component_config_t *edge_config);

/**
 * @brief Apply global configuration to network component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param network_config Network configuration
 */
void apply_global_network_config(polycall_core_context_t *ctx,
                                 polycall_global_config_context_t *global_ctx,
                                 polycall_network_config_t *network_config);

/**
 * @brief Apply binding configuration to network component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param network_config Network configuration
 */
void apply_binding_network_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    polycall_network_config_t *network_config);

/**
 * @brief Apply global configuration to protocol component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param protocol_config Protocol configuration
 */
void apply_global_protocol_config(polycall_core_context_t *ctx,
                                  polycall_global_config_context_t *global_ctx,
                                  protocol_config_t *protocol_config);

/**
 * @brief Apply binding configuration to protocol component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param protocol_config Protocol configuration
 */
void apply_binding_protocol_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    protocol_config_t *protocol_config);

/**
 * @brief Apply global configuration to FFI component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param ffi_config FFI configuration options
 */
void apply_global_ffi_config(polycall_core_context_t *ctx,
                             polycall_global_config_context_t *global_ctx,
                             polycall_ffi_config_options_t *ffi_config);

/**
 * @brief Apply binding configuration to FFI component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param ffi_config FFI configuration options
 */
void apply_binding_ffi_config(polycall_core_context_t *ctx,
                              polycall_binding_config_context_t *binding_ctx,
                              const char *component_name,
                              polycall_ffi_config_options_t *ffi_config);

/**
 * @brief Apply global configuration to telemetry component
 *
 * @param ctx Core context
 * @param global_ctx Global configuration context
 * @param telemetry_config Telemetry configuration
 */
void apply_global_telemetry_config(
    polycall_core_context_t *ctx, polycall_global_config_context_t *global_ctx,
    polycall_telemetry_config_t *telemetry_config);

/**
 * @brief Apply binding configuration to telemetry component
 *
 * @param ctx Core context
 * @param binding_ctx Binding configuration context
 * @param component_name Component name
 * @param telemetry_config Telemetry configuration
 */
void apply_binding_telemetry_config(
    polycall_core_context_t *ctx,
    polycall_binding_config_context_t *binding_ctx, const char *component_name,
    polycall_telemetry_config_t *telemetry_config);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_FACTORY_CONFIG_FACTORY_H_H */