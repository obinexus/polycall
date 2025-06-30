/**
 * @file polic.c
 * @brief Implementation of the core PoliC API
 */

#include "polic/polic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Configuration structure */
typedef struct {
    bool is_sandboxed;
    bool enable_vm_hooks;
    bool stack_protection;
    PolicyAction default_action;
    void (*logger)(const char* message);
} PolICConfig;

/* Global configuration instance */
static PolICConfig g_config = {
    .is_sandboxed = false,
    .enable_vm_hooks = false,
    .stack_protection = false,
    .default_action = POLIC_BLOCK,
    .logger = NULL
};

/* Default logger implementation */
static void default_logger(const char* message) {
    printf("[POLIC] %s\n", message);
}

/* Initialize the framework */
int polic_init(bool sandbox_mode, PolicyAction action) {
    g_config.is_sandboxed = sandbox_mode;
    g_config.default_action = action;
    g_config.logger = default_logger;
    
    /* Log initialization */
    if (g_config.logger) {
        g_config.logger("PoliC security framework initialized");
    }
    
    return 0;
}

/* Set custom logger */
void polic_set_logger(void (*logger_func)(const char* message)) {
    g_config.logger = logger_func ? logger_func : default_logger;
}
