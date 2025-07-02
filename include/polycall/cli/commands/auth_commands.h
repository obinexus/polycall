/**
 * @file auth_commands.h
 * @brief Authentication command handlers for LibPolyCall CLI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the interface for authentication and authorization command
 * handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_AUTH_COMMANDS_H
 #define POLYCALL_CLI_AUTH_COMMANDS_H
 
 #include "polycall/cli/command.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Login command handler
  */
 command_result_t auth_cmd_login(int argc, char** argv, void* context);
 
 /**
  * @brief Logout command handler
  */
 command_result_t auth_cmd_logout(int argc, char** argv, void* context);
 
 /**
  * @brief Check authentication status command handler
  */
 command_result_t auth_cmd_status(int argc, char** argv, void* context);
 
 /**
  * @brief Change password command handler
  */
 command_result_t auth_cmd_change_password(int argc, char** argv, void* context);
 
 /**
  * @brief List users command handler
  */
 command_result_t auth_cmd_list_users(int argc, char** argv, void* context);
 
 /**
  * @brief Add user command handler
  */
 command_result_t auth_cmd_add_user(int argc, char** argv, void* context);
 
 /**
  * @brief Remove user command handler
  */
 command_result_t auth_cmd_remove_user(int argc, char** argv, void* context);
 
 /**
  * @brief List roles command handler
  */
 command_result_t auth_cmd_list_roles(int argc, char** argv, void* context);
 
 /**
  * @brief Add role command handler
  */
 command_result_t auth_cmd_add_role(int argc, char** argv, void* context);
 
 /**
  * @brief Remove role command handler
  */
 command_result_t auth_cmd_remove_role(int argc, char** argv, void* context);
 
 /**
  * @brief Grant permission command handler
  */
 command_result_t auth_cmd_grant_permission(int argc, char** argv, void* context);
 
 /**
  * @brief Revoke permission command handler
  */
 command_result_t auth_cmd_revoke_permission(int argc, char** argv, void* context);
 
 /**
  * @brief List permissions command handler
  */
 command_result_t auth_cmd_list_permissions(int argc, char** argv, void* context);
 
 /**
  * @brief Register authentication commands
  */
 int auth_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_AUTH_COMMANDS_H */