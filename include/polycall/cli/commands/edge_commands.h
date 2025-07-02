/**
 * @file edge_commands.h
 * @brief Edge computing command handlers for LibPolyCall CLI
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file defines the edge computing command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_EDGE_COMMANDS_H
 #define POLYCALL_CLI_EDGE_COMMANDS_H
 
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
  * @brief List nodes command handler
  */
 command_result_t edge_cmd_list_nodes(int argc, char** argv, void* context);
 
 /**
  * @brief Register node command handler
  */
 command_result_t edge_cmd_register_node(int argc, char** argv, void* context);
 
 /**
  * @brief Unregister node command handler
  */
 command_result_t edge_cmd_unregister_node(int argc, char** argv, void* context);
 
 /**
  * @brief Get node metrics command handler
  */
 command_result_t edge_cmd_get_node_metrics(int argc, char** argv, void* context);
 
 /**
  * @brief Route task command handler
  */
 command_result_t edge_cmd_route_task(int argc, char** argv, void* context);
 
 /**
  * @brief Execute task command handler
  */
 command_result_t edge_cmd_execute_task(int argc, char** argv, void* context);
 
 /**
  * @brief Handle node failure command handler
  */
 command_result_t edge_cmd_handle_node_failure(int argc, char** argv, void* context);
 
 /**
  * @brief Set selection strategy command handler
  */
 command_result_t edge_cmd_set_selection_strategy(int argc, char** argv, void* context);
 
 /**
  * @brief Register edge commands
  */
 int edge_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_EDGE_COMMANDS_H */