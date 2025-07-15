/**
 * @file binding_config.h
 * @brief Polycall binding configuration management with zero-trust validation
 *
 * This header has been refactored to resolve function signature conflicts
 * and implement zero-trust security for bind-to-bind operations.
 */

#ifndef POLYCALL_BINDING_CONFIG_H
#define POLYCALL_BINDING_CONFIG_H

#include "polycall/core/polycall_core.h"
#include "polycall/core/polycall_error.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct polycall_binding_config_context
    polycall_binding_config_context_t;
typedef struct polycall_binding_config_value polycall_binding_config_value_t;

/**
 * @brief Binding configuration value types
 */
typedef enum {
  POLYCALL_BINDING_CONFIG_TYPE_STRING,
  POLYCALL_BINDING_CONFIG_TYPE_INT,
  POLYCALL_BINDING_CONFIG_TYPE_BOOL,
  POLYCALL_BINDING_CONFIG_TYPE_FLOAT,
  POLYCALL_BINDING_CONFIG_TYPE_OBJECT,
  POLYCALL_BINDING_CONFIG_TYPE_ARRAY
} polycall_binding_config_type_t;

/**
 * @brief Binding configuration value structure
 */
struct polycall_binding_config_value {
  polycall_binding_config_type_t type;
  union {
    char *string_value;
    int64_t int_value;
    bool bool_value;
    double float_value;
    void *object_value;
    void *array_value;
  } data;
};

/**
 * @brief Zero-trust validation context for binding operations
 */
typedef struct {
  uint8_t challenge[32]; /**< Cryptographic challenge */
  uint8_t response[64];  /**< Challenge response */
  uint64_t timestamp;    /**< Operation timestamp */
  uint32_t policy_flags; /**< Policy enforcement flags */
} polycall_binding_zerotrust_t;

/**
 * @brief Binding configuration context
 */
struct polycall_binding_config_context {
  void *internal_data;                     /**< Internal implementation data */
  polycall_binding_zerotrust_t *zerotrust; /**< Zero-trust context */
  char *config_path;                       /**< Configuration file path */
  bool is_modified;                        /**< Modification flag */
  bool is_readonly;                        /**< Read-only flag */
};

/**
 * @brief Initialize binding configuration context
 *
 * @param core_ctx Core context
 * @param cfg_ctx Pointer to receive configuration context
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_init(polycall_core_context_t *core_ctx,
                             polycall_binding_config_context_t **cfg_ctx);

/**
 * @brief Initialize with zero-trust validation
 *
 * @param core_ctx Core context
 * @param cfg_ctx Pointer to receive configuration context
 * @param zerotrust Zero-trust validation parameters
 * @return Error code
 */
polycall_core_error_t polycall_binding_config_init_zerotrust(
    polycall_core_context_t *core_ctx,
    polycall_binding_config_context_t **cfg_ctx,
    const polycall_binding_zerotrust_t *zerotrust);

/**
 * @brief Clean up binding configuration context
 *
 * @param cfg_ctx Configuration context
 */
void polycall_binding_config_cleanup(
    polycall_binding_config_context_t *cfg_ctx);

/**
 * @brief Load configuration from file
 *
 * @param cfg_ctx Configuration context
 * @param filename Configuration file path
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_load(polycall_binding_config_context_t *cfg_ctx,
                             const char *filename);

/**
 * @brief Save configuration to file (FIXED: Single consistent signature)
 *
 * @param cfg_ctx Configuration context
 * @param filename Configuration file path
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_save(polycall_binding_config_context_t *cfg_ctx,
                             const char *filename);

/**
 * @brief Get string value from configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Buffer to receive value
 * @param value_size Size of value buffer
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_get_string(polycall_binding_config_context_t *cfg_ctx,
                                   const char *key, char *value,
                                   size_t value_size);

/**
 * @brief Get integer value from configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Pointer to receive value
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_get_int(polycall_binding_config_context_t *cfg_ctx,
                                const char *key, int64_t *value);

/**
 * @brief Get boolean value from configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Pointer to receive value
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_get_bool(polycall_binding_config_context_t *cfg_ctx,
                                 const char *key, bool *value);

/**
 * @brief Set string value in configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Value to set
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_set_string(polycall_binding_config_context_t *cfg_ctx,
                                   const char *key, const char *value);

/**
 * @brief Set integer value in configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Value to set
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_set_int(polycall_binding_config_context_t *cfg_ctx,
                                const char *key, int64_t value);

/**
 * @brief Set boolean value in configuration
 *
 * @param cfg_ctx Configuration context
 * @param key Configuration key
 * @param value Value to set
 * @return Error code
 */
polycall_core_error_t
polycall_binding_config_set_bool(polycall_binding_config_context_t *cfg_ctx,
                                 const char *key, bool value);

/**
 * @brief Validate zero-trust credentials for bind-to-bind operation
 *
 * @param src_ctx Source binding context
 * @param dst_ctx Destination binding context
 * @param operation Operation identifier
 * @return Error code
 */
polycall_core_error_t
polycall_binding_validate_zerotrust(polycall_binding_config_context_t *src_ctx,
                                    polycall_binding_config_context_t *dst_ctx,
                                    const char *operation);

/**
 * @brief Generate cryptographic proof for binding operation
 *
 * @param cfg_ctx Configuration context
 * @param operation Operation identifier
 * @param proof Buffer to receive proof (min 64 bytes)
 * @return Error code
 */
polycall_core_error_t
polycall_binding_generate_proof(polycall_binding_config_context_t *cfg_ctx,
                                const char *operation, uint8_t *proof);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_BINDING_CONFIG_H */