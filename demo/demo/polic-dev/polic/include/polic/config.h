/**
 * @file config.h
 * @brief Configuration options for PoliC
 *
 * This file contains configuration options and settings for the PoliC framework.
 *
 * @copyright OBINexus Computing
 * @author Nnamdi Michael Okpala
 */

#ifndef POLIC_CONFIG_H
#define POLIC_CONFIG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable or disable VM hooks
 * @param enable Whether to enable VM hooks
 * @return 0 on success, non-zero error code otherwise
 */
int polic_config_vm_hooks(bool enable);

/**
 * @brief Enable or disable stack protection
 * @param enable Whether to enable stack protection
 * @return 0 on success, non-zero error code otherwise
 */
int polic_config_stack_protection(bool enable);

/* More configuration functions will be added here */

#ifdef __cplusplus
}
#endif

#endif /* POLIC_CONFIG_H */
