/**
 * @file micro_commands.h
 * @brief Micro command handlers for LibPolyCall CLI
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file defines the micro command system command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_MICRO_COMMANDS_H
 #define POLYCALL_CLI_MICRO_COMMANDS_H
 
 #include "polycall/cli/command.h"
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief List components command handler
  */
 command_result_t micro_cmd_list_components(int argc, char** argv, void* context);
 
 /**
  * @brief Create component command handler
  */
 command_result_t micro_cmd_create_component(int argc, char** argv, void* context);
 
 /**
  * @brief Destroy component command handler
  */
 command_result_t micro_cmd_destroy_component(int argc, char** argv, void* context);
 
 /**
  * @brief Start component command handler
  */
 command_result_t micro_cmd_start_component(int argc, char** argv, void* context);
 
 /**
  * @brief Stop component command handler
  */
 command_result_t micro_cmd_stop_component(int argc, char** argv, void* context);
 
 /**
  * @brief List commands command handler
  */
 command_result_t micro_cmd_list_commands(int argc, char** argv, void* context);
 
 /**
  * @brief Register command handler
  */
 command_result_t micro_cmd_register_command(int argc, char** argv, void* context);
 
 /**
  * @brief Unregister command handler
  */
 command_result_t micro_cmd_unregister_command(int argc, char** argv, void* context);
 
 /**
  * @brief Execute command handler
  */
 command_result_t micro_cmd_execute_command(int argc, char** argv, void* context);
 
 /**
  * @brief Register micro commands
  */
 int micro_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_MICRO_COMMANDS_H */