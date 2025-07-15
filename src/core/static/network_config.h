/**
#include <ctype.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <errno.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stddef.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdio.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdlib.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <string.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

 * @file network_config.h
 * @brief Network configuration interface for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the configuration management interface for LibPolyCall's
 * network module, providing consistent configuration handling across components.
 */
#include "polycall/core/polycall/core.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
 typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/polycall/polycall.h"
 /* Forward declarations */
 typedef struct polycall_core_context polycall_core_context_t;
 typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/network/network_client.h"
 /* Forward declarations */
 typedef struct polycall_core_context polycall_core_context_t;
 typedef struct polycall_config_context polycall_config_context_t;

#ifndef POLYCALL_NETWORK_NETWORK_CONFIG_H_H
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H
#ifdef __cplusplus
 extern "C" {
#endif

 // Configuration file version
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H

 // Configuration section identifiers
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H
#define POLYCALL_NETWORK_NETWORK_CONFIG_H_H

 // Add typedef for validation callback
 typedef polycall_core_error_t (*polycall_network_config_validate_fn)(
     polycall_core_context_t *ctx, polycall_network_config_t *config);
 // Forward declaration of the network configuration structure
 typedef struct polycall_network_config polycall_network_config_t;
 struct polycall_network_config {
   polycall_core_context_t *core_ctx; // Core context
   char config_file[256];             // Configuration file path
   bool initialized;                  // Initialization status
   bool modified;                     // Modification status
   void *entries;                     // Configuration entries
   polycall_network_config_validate_fn
       validate_callback;    // Validation callback function
   void *validate_user_data; // User data for validation callback
 };

 /**
  * @brief Configuration validation callback
  *
  * @param ctx Core context
  * @param config Network configuration
  * @param user_data User data
  * @return true if configuration is valid, false otherwise
  */
 typedef bool (*polycall_network_config_validate_fn)(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     void *user_data);

 /**
  * @brief Create a network configuration context
  *
  * @param ctx Core context
  * @param config Pointer to receive configuration context
  * @param config_file Configuration file path (can be NULL)
  * @return Error code
  */
 polycall_core_error_t
 polycall_network_config_create(polycall_core_context_t *ctx,
                                polycall_network_config_t **config,
                                const char *config_file);

 /**
  * @brief Destroy a network configuration context
  *
  * @param ctx Core context
  * @param config Configuration context to destroy
  */
 void polycall_network_config_destroy(polycall_core_context_t *ctx,
                                      polycall_network_config_t *config);

 /**
  * @brief Set configuration validation callback
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param validator Validation callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_validator(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     polycall_network_config_validate_fn validator, void *user_data);

 /**
  * @brief Set integer configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Integer value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_int(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, int value);

 /**
  * @brief Get integer configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Pointer to receive integer value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_get_int(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, int *value);

 /**
  * @brief Set unsigned integer configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Unsigned integer value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_uint(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, unsigned int value);

 /**
  * @brief Get unsigned integer configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Pointer to receive unsigned integer value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_get_uint(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, unsigned int *value);

 /**
  * @brief Set boolean configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Boolean value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_bool(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, bool value);

 /**
  * @brief Get boolean configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Pointer to receive boolean value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_get_bool(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, bool *value);

 /**
  * @brief Set string configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value String value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_string(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, const char *value);

 /**
  * @brief Get string configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Buffer to receive string value
  * @param max_length Maximum buffer length
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_get_string(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, char *value, size_t max_length);

 /**
  * @brief Set float configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Float value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_set_float(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, float value);

 /**
  * @brief Get float configuration value
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section
  * @param key Configuration key
  * @param value Pointer to receive float value
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_get_float(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section, const char *key, float *value);

 /**
  * @brief Load configuration from file
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param filename Configuration file path
  * @return Error code
  */
 polycall_core_error_t
 polycall_network_config_load(polycall_core_context_t *ctx,
                              polycall_network_config_t *config,
                              const char *filename);

 /**
  * @brief Save configuration to file
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param filename Configuration file path (can be NULL to use existing path)
  * @return Error code
  */
 polycall_core_error_t
 polycall_network_config_save(polycall_core_context_t *ctx,
                              polycall_network_config_t *config,
                              const char *filename);

 /**
  * @brief Apply default configuration settings
  *
  * @param ctx Core context
  * @param config Configuration context
  * @return Error code
  */
 POLYCALL_API polycall_core_error_t apply_defaults(
     polycall_core_context_t *ctx, polycall_network_config_t *config);

 /**
  * @brief Load configuration from a file
  *
  * @param ctx Core context
  * @param config Configuration context
  * @return Error code
  */
 POLYCALL_API polycall_core_error_t load_config_from_file(
     polycall_core_context_t *ctx, polycall_network_config_t *config);

 /**
  * @brief Reset configuration to defaults
  *
  * @param ctx Core context
  * @param config Configuration context
  * @return Error code
  */
 polycall_core_error_t
 polycall_network_config_reset(polycall_core_context_t *ctx,
                               polycall_network_config_t *config);

 /**
  * @brief Enumerate configuration keys
  *
  * @param ctx Core context
  * @param config Configuration context
  * @param section Configuration section (empty string for all sections)
  * @param callback Enumeration callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_network_config_enumerate(
     polycall_core_context_t *ctx, polycall_network_config_t *config,
     const char *section,
     bool (*callback)(const char *section, const char *key, void *user_data),
     void *user_data);

#ifdef __cplusplus
 }
#endif

#endif /* POLYCALL_NETWORK_NETWORK_CONFIG_H_H */