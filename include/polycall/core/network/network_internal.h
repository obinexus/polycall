/**
 * @file network_internal.h
 * @brief Internal network configuration functions for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines internal functions used by the network configuration
 * subsystem that are not exposed in the public API.
 */

#ifndef POLYCALL_NETWORK_NETWORK_INTERNAL_H_H
#define POLYCALL_NETWORK_NETWORK_INTERNAL_H_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration entry types
 */
typedef enum {
  CONFIG_TYPE_INT,
  CONFIG_TYPE_UINT,
  CONFIG_TYPE_BOOL,
  CONFIG_TYPE_FLOAT,
  CONFIG_TYPE_STRING
} config_entry_type_t;

/**
 * @brief Configuration entry value union
 */
typedef union {
  int int_value;
  unsigned int uint_value;
  bool bool_value;
  float float_value;
  char *string_value;
} config_value_t;

/**
 * @brief Configuration entry structure
 */
typedef struct config_entry {
  char section[64];
  char key[64];
  config_entry_type_t type;
  config_value_t value;
  char *description;
  struct config_entry *next;
} config_entry_t;

/**
 * @brief Apply default configuration values
 *
 * @param ctx Core context
 * @param config Network configuration context
 * @return Error code
 */
polycall_core_error_t apply_defaults(polycall_core_context_t *ctx,
                                     polycall_network_config_t *config);

/**
 * @brief Load configuration from file
 *
 * @param ctx Core context
 * @param config Network configuration context
 * @return Error code
 */
polycall_core_error_t load_config_from_file(polycall_core_context_t *ctx,
                                            polycall_network_config_t *config);

/**
 * @brief Save configuration to file
 *
 * @param ctx Core context
 * @param config Network configuration context
 * @return Error code
 */
polycall_core_error_t save_config_to_file(polycall_core_context_t *ctx,
                                          polycall_network_config_t *config);

/**
 * @brief Add configuration entry
 *
 * @param ctx Core context
 * @param config Network configuration context
 * @param section Configuration section
 * @param key Configuration key
 * @param type Entry type
 * @param value Entry value
 * @param description Entry description
 * @return Error code
 */
polycall_core_error_t add_config_entry(polycall_core_context_t *ctx,
                                       polycall_network_config_t *config,
                                       const char *section, const char *key,
                                       config_entry_type_t type,
                                       const void *value,
                                       const char *description);

/**
 * @brief Find configuration entry
 *
 * @param config Network configuration context
 * @param section Configuration section
 * @param key Configuration key
 * @return Found entry or NULL
 */
config_entry_t *find_config_entry(polycall_network_config_t *config,
                                  const char *section, const char *key);

/**
 * @brief Free configuration entries
 *
 * @param ctx Core context
 * @param entries Entry list to free
 */
void free_config_entries(polycall_core_context_t *ctx, config_entry_t *entries);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_NETWORK_INTERNAL_H_H */
