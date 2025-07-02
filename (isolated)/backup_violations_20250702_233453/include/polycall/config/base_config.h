/*
 * base_config.h
 * Base configuration infrastructure for PolyCall
 * This is safe for all modules to include
 */

#ifndef POLYCALL_BASE_CONFIG_H
#define POLYCALL_BASE_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

typedef struct polycall_config polycall_config_t;

// Configuration API that all modules can use
polycall_config_t* polycall_config_create(void);
void polycall_config_destroy(polycall_config_t* config);
int polycall_config_set(polycall_config_t* config, const char* key, const char* value);
const char* polycall_config_get(polycall_config_t* config, const char* key);
bool polycall_config_get_bool(polycall_config_t* config, const char* key, bool default_value);
int polycall_config_get_int(polycall_config_t* config, const char* key, int default_value);

#endif /* POLYCALL_BASE_CONFIG_H */
