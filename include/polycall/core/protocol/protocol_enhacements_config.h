/**
 * @file protocol_enhancements_config.h
 * @brief Unified Configuration Header for LibPolyCall Protocol Enhancements
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Defines the comprehensive configuration structures and interfaces for
 * protocol enhancement modules, providing a unified initialization and
 * management interface.
 */

#ifndef POLYCALL_CONFIG_PROTOCOL_ENHACEMENTS_CONFIG_H_H
#define POLYCALL_CONFIG_PROTOCOL_ENHACEMENTS_CONFIG_H_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration options for protocol enhancements
 */
typedef struct {
  bool enable_advanced_security;
  bool enable_connection_pool;
  bool enable_hierarchical_state;
  bool enable_message_optimization;
  bool enable_subscription;

  /* Advanced security configuration */
  struct {
    const char *cert_path;
    const char *key_path;
    bool enable_tls;
  } security_config;

  /* Connection pool configuration */
  struct {
    uint32_t max_connections;
    uint32_t idle_timeout_ms;
  } connection_pool_config;

  /* Additional configuration parameters for other modules */
  void *user_data;
} polycall_protocol_enhancements_config_t;

/**
 * @brief Context for protocol enhancements
 */
typedef struct {
  void *security_ctx;
  void *connection_pool_ctx;
  void *hierarchical_ctx;
  void *optimization_ctx;
  void *subscription_ctx;

  polycall_core_context_t *core_ctx;
  polycall_protocol_context_t *proto_ctx;

  void *user_data;
} polycall_protocol_enhancements_context_t;

/**
 * @brief Initialize protocol enhancements
 *
 * @param core_ctx The core context
 * @param proto_ctx The protocol context
 * @param enhancements_ctx Pointer to store the created enhancements context
 * @param config Configuration for the enhancements
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_protocol_enhancements_init(
    polycall_core_context_t *core_ctx, polycall_protocol_context_t *proto_ctx,
    polycall_protocol_enhancements_context_t **enhancements_ctx,
    const polycall_protocol_enhancements_config_t *config);

/**
 * @brief Clean up protocol enhancements
 *
 * @param core_ctx The core context
 * @param enhancements_ctx The enhancements context to clean up
 */
void polycall_protocol_enhancements_cleanup(
    polycall_core_context_t *core_ctx,
    polycall_protocol_enhancements_context_t *enhancements_ctx);

/**
 * @brief Get default configuration for protocol enhancements
 *
 * @return polycall_protocol_enhancements_config_t Default configuration
 */
polycall_protocol_enhancements_config_t
polycall_protocol_enhancements_default_config(void);

/**
 * @brief Apply protocol enhancements to a protocol context
 *
 * @param core_ctx The core context
 * @param proto_ctx The protocol context to enhance
 * @param enhancements_ctx The enhancements context
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_protocol_apply_enhancements(
    polycall_core_context_t *core_ctx, polycall_protocol_context_t *proto_ctx,
    polycall_protocol_enhancements_context_t *enhancements_ctx);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_PROTOCOL_ENHACEMENTS_CONFIG_H_H */
