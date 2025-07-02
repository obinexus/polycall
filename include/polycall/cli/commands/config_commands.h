/**
 * @file config_commands.h
 * @brief Configuration command handlers for LibPolyCall CLI
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file defines the configuration-related command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_CONFIG_COMMANDS_H
 #define POLYCALL_CLI_CONFIG_COMMANDS_H
 
 #include "polycall/cli/command.h"
 #include <stdbool.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Load configuration command handler
  */
 command_result_t config_cmd_load(int argc, char** argv, void* context);
 
 /**
  * @brief Save configuration command handler
  */
 command_result_t config_cmd_save(int argc, char** argv, void* context);
 
 /**
  * @brief Show configuration command handler
  */
 command_result_t config_cmd_show(int argc, char** argv, void* context);
 
 /**
  * @brief Set configuration command handler
  */
 command_result_t config_cmd_set(int argc, char** argv, void* context);
 
 /**
  * @brief Get configuration command handler
  */
 command_result_t config_cmd_get(int argc, char** argv, void* context);
 
 /**
  * @brief Reset configuration command handler
  */
 command_result_t config_cmd_reset(int argc, char** argv, void* context);
 
 /**
  * @brief Validate configuration command handler
  */
 command_result_t config_cmd_validate(int argc, char** argv, void* context);
 
 /**
  * @brief Export configuration command handler
  */
 command_result_t config_cmd_export(int argc, char** argv, void* context);
 
 /**
  * @brief Import configuration command handler
  */
 command_result_t config_cmd_import(int argc, char** argv, void* context);
 
 /**
  * @brief Register configuration commands
  */
 int config_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_CONFIG_COMMANDS_H */