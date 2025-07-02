/**
 * @file command_registry.h
 * @brief Command registry interface for LibPolyCall CLI
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file defines the command registration interface for all command modules.
 */

 #ifndef POLYCALL_CLI_COMMAND_REGISTRY_H
 #define POLYCALL_CLI_COMMAND_REGISTRY_H
 
 #include "polycall/cli/command.h"
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Register all commands
  * 
  * This function registers all command modules.
  * 
  * @return POLYCALL_CORE_SUCCESS on success, error code otherwise
  */
 int cli_register_all_commands(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_COMMAND_REGISTRY_H */