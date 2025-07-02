/**
 * @file network_commands.h
 * @brief Network command handlers for LibPolyCall CLI
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file defines the network-related command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_NETWORK_COMMANDS_H
 #define POLYCALL_CLI_NETWORK_COMMANDS_H
 
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
  * @brief Start network command handler
  */
 command_result_t network_cmd_start(int argc, char** argv, void* context);
 
 /**
  * @brief Stop network command handler
  */
 command_result_t network_cmd_stop(int argc, char** argv, void* context);
 
 /**
  * @brief List endpoints command handler
  */
 command_result_t network_cmd_list_endpoints(int argc, char** argv, void* context);
 
 /**
  * @brief List clients command handler
  */
 command_result_t network_cmd_list_clients(int argc, char** argv, void* context);
 
 /**
  * @brief Bind endpoint command handler
  */
 command_result_t network_cmd_bind(int argc, char** argv, void* context);
 
 /**
  * @brief Unbind endpoint command handler
  */
 command_result_t network_cmd_unbind(int argc, char** argv, void* context);
 
 /**
  * @brief Connect to remote endpoint command handler
  */
 command_result_t network_cmd_connect(int argc, char** argv, void* context);
 
 /**
  * @brief Disconnect from remote endpoint command handler
  */
 command_result_t network_cmd_disconnect(int argc, char** argv, void* context);
 
 /**
  * @brief Register network commands
  */
 int network_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_NETWORK_COMMANDS_H */