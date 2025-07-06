/**
 * @file ffi_config.h
 * @brief Configuration module for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the configuration system for LibPolyCall FFI, providing
 * a comprehensive, extensible configuration interface for all FFI components.
 * It enables centralized configuration management, persistent settings, and
 * runtime reconfiguration of FFI behavior.
 */

#ifndef POLYCALL_FFI_FFI_CONFIG_H_H
#define POLYCALL_FFI_FFI_CONFIG_H_H

#include "polycall/core/ffi/c_bridge.h"
#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/ffi/js_bridge.h"
#include "polycall/core/ffi/jvm_bridge.h"
#include "polycall/core/ffi/memory_bridge.h"
#include "polycall/core/ffi/performance.h"
#include "polycall/core/ffi/protocol_bridge.h"
#include "polycall/core/ffi/python_bridge.h"
#include "polycall/core/ffi/security.h"
#include "polycall/core/ffi/type_system.h"
#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FFI configuration context (opaque)
 */
typedef struct polycall_ffi_config_context polycall_ffi_config_context_t;

*@brief FFI configuration options * / typedef struct {
  bool enable_persistence;         /**< Enable configuration persistence */
  bool enable_change_notification; /**< Enable change notifications */
  bool validate_configuration;     /**< Validate configuration values */
  const char *config_file_path;    /**< Path to configuration file */
  const char *provider_name;       /**< Configuration provider name */
  void *provider_data;             /**< Provider-specific data */
} polycall_ffi_config_options_t;

/**
 * @brief Initialize configuration module
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Pointer to receive configuration context
 * @param options Configuration options
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_config_init(polycall_core_context_t *ctx,
                         polycall_ffi_context_t *ffi_ctx,
                         polycall_ffi_config_context_t **config_ctx,
                         const polycall_ffi_config_options_t *options);

/**
 * @brief Clean up configuration module
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context to clean up
 */
void polycall_ffi_config_cleanup(polycall_core_context_t *ctx,
                                 polycall_ffi_context_t *ffi_ctx,
                                 polycall_ffi_config_context_t *config_ctx);

/**
 * @brief Register configuration provider
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param provider Configuration provider
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_register_provider(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    const polycall_config_provider_t *provider);

/**
 * @brief Get boolean configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return Boolean value
 */
bool polycall_ffi_config_get_bool(polycall_core_context_t *ctx,
                                  polycall_ffi_context_t *ffi_ctx,
                                  polycall_ffi_config_context_t *config_ctx,
                                  polycall_config_section_t section_id,
                                  const char *key, bool default_value);

/**
 * @brief Get integer configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return Integer value
 */
int64_t polycall_ffi_config_get_int(polycall_core_context_t *ctx,
                                    polycall_ffi_context_t *ffi_ctx,
                                    polycall_ffi_config_context_t *config_ctx,
                                    polycall_config_section_t section_id,
                                    const char *key, int64_t default_value);

/**
 * @brief Get floating-point configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return Floating-point value
 */
double polycall_ffi_config_get_float(polycall_core_context_t *ctx,
                                     polycall_ffi_context_t *ffi_ctx,
                                     polycall_ffi_config_context_t *config_ctx,
                                     polycall_config_section_t section_id,
                                     const char *key, double default_value);

/**
 * @brief Get string configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param default_value Default value if not found
 * @return String value (caller must free)
 */
char *polycall_ffi_config_get_string(polycall_core_context_t *ctx,
                                     polycall_ffi_context_t *ffi_ctx,
                                     polycall_ffi_config_context_t *config_ctx,
                                     polycall_config_section_t section_id,
                                     const char *key,
                                     const char *default_value);

/**
 * @brief Get object configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @return Object value or NULL if not found
 */
void *polycall_ffi_config_get_object(polycall_core_context_t *ctx,
                                     polycall_ffi_context_t *ffi_ctx,
                                     polycall_ffi_config_context_t *config_ctx,
                                     polycall_config_section_t section_id,
                                     const char *key);

/**
 * @brief Set boolean configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param value Boolean value
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_set_bool(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key, bool value);

/**
 * @brief Set integer configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param value Integer value
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_set_int(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key, int64_t value);

/**
 * @brief Set floating-point configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param value Floating-point value
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_set_float(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key, double value);

/**
 * @brief Set string configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param value String value
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_set_string(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key, const char *value);

/**
 * @brief Set object configuration value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key
 * @param value Object value
 * @param object_free Function to free the object
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_set_object(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key, void *value,
    void (*object_free)(void *object));

/**
 * @brief Register configuration change handler
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key (NULL for all keys)
 * @param handler Change handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_register_change_handler(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key,
    polycall_config_change_handler_t handler, void *user_data);

/**
 * @brief Unregister configuration change handler
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier
 * @param key Configuration key (NULL for all keys)
 * @param handler Change handler function
 * @param user_data User data for handler
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_unregister_change_handler(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx,
    polycall_config_section_t section_id, const char *key,
    polycall_config_change_handler_t handler, void *user_data);

/**
 * @brief Load configuration from file
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param file_path Path to configuration file
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_load_file(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx, const char *file_path);

/**
 * @brief Save configuration to file
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param file_path Path to configuration file
 * @return Error code
 */
polycall_core_error_t polycall_ffi_config_save_file(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_ffi_config_context_t *config_ctx, const char *file_path);

/**
 * @brief Reset configuration to defaults
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @param section_id Section identifier (or -1 for all sections)
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_config_reset_defaults(polycall_core_context_t *ctx,
                                   polycall_ffi_context_t *ffi_ctx,
                                   polycall_ffi_config_context_t *config_ctx,
                                   polycall_config_section_t section_id);

/**
 * @brief Apply configuration to FFI system
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param config_ctx Configuration context
 * @return Error code
 */
polycall_core_error_t
polycall_ffi_config_apply(polycall_core_context_t *ctx,
                          polycall_ffi_context_t *ffi_ctx,
                          polycall_ffi_config_context_t *config_ctx);

/**
 * @brief Create a default configuration options
 *
 * @return Default options
 */
polycall_ffi_config_options_t polycall_ffi_config_create_default_options(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_FFI_CONFIG_H_H */