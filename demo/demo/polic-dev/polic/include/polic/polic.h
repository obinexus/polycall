/**
 * @file polic.h
 * @brief Main PoliC API header file
 *
 * PoliC is a security framework for C that protects virtualized and 
 * sandboxed environments by enforcing function-level security.
 *
 * @copyright OBINexus Computing
 * @author Nnamdi Michael Okpala
 */

#ifndef POLIC_H
#define POLIC_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum PolicyAction
 * @brief Defines the possible policy actions
 */
typedef enum {
    POLIC_ALLOW,    /**< Allow the function to execute */
    POLIC_BLOCK,    /**< Block the function execution */
    POLIC_LOG_ONLY  /**< Log the call but allow execution */
} PolicyAction;

/**
 * @brief Initialize the PoliC security framework
 * @param sandbox_mode Whether to enable sandbox mode
 * @param action Default policy action
 * @return 0 on success, non-zero error code otherwise
 */
int polic_init(bool sandbox_mode, PolicyAction action);

/**
 * @brief Set a custom logger function
 * @param logger_func Function pointer to logger
 */
void polic_set_logger(void (*logger_func)(const char* message));

/* More API functions will be added here */

#ifdef __cplusplus
}
#endif

#endif /* POLIC_H */
