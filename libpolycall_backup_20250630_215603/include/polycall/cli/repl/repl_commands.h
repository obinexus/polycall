/**
 * @file repl_commands.h
 * @brief REPL command handlers for LibPolyCall CLI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the REPL-related command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_REPL_COMMANDS_H
 #define POLYCALL_CLI_REPL_COMMANDS_H
 
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
  * @brief Start REPL command handler
  */
 command_result_t repl_cmd_start(int argc, char** argv, void* context);
 
 /**
  * @brief Enable log inspection command handler
  */
 command_result_t repl_cmd_enable_log_inspection(int argc, char** argv, void* context);
 
 /**
  * @brief Enable zero-trust inspection command handler
  */
 command_result_t repl_cmd_enable_zero_trust_inspection(int argc, char** argv, void* context);
 
 /**
  * @brief REPL help command handler
  */
 command_result_t repl_cmd_help(int argc, char** argv, void* context);
 
 /**
  * @brief Register REPL commands
  */
 int repl_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_REPL_COMMANDS_H */