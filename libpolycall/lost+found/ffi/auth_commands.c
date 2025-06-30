/**
 * @file auth_commands.c
 * @brief Authentication command implementations for LibPolyCall CLI
 *
 * Provides commands for authentication, token management, and security
 * policy configuration with accessibility support.
 *
 * @copyright OBINexus Computing, 2025
 */

 #include "polycall/cli/commands/auth_commands.h"
 #include "polycall/cli/command.h"
 #include "polycall/core/accessibility/accessibility_interface.h"
 #include "polycall/core/auth/polycall_auth_config.h"
 #include "polycall/core/auth/polycall_auth_token.h"
 #include "polycall/core/auth/polycall_auth_identity.h"
 #include "polycall/core/auth/polycall_auth_policy.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 /* Forward declarations of command handlers */
 static command_result_t handle_auth(int argc, char** argv, void* context);
 static command_result_t handle_login(int argc, char** argv, void* context);
 static command_result_t handle_logout(int argc, char** argv, void* context);
 static command_result_t handle_token_create(int argc, char** argv, void* context);
 static command_result_t handle_token_verify(int argc, char** argv, void* context);
 static command_result_t handle_token_revoke(int argc, char** argv, void* context);
 static command_result_t handle_policy_list(int argc, char** argv, void* context);
 static command_result_t handle_policy_set(int argc, char** argv, void* context);
 static command_result_t handle_policy_reset(int argc, char** argv, void* context);
 
 /* Token subcommands */
 static subcommand_t token_subcommands[] = {
     {
         .name = "create",
         .description = "Create a new authentication token",
         .usage = "polycall auth token create --type <token_type> --identity <identity> [--expiry <seconds>]",
         .handler = handle_token_create,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Creates a new authentication token with specified parameters"
     },
     {
         .name = "verify",
         .description = "Verify an authentication token",
         .usage = "polycall auth token verify <token>",
         .handler = handle_token_verify,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Verifies the validity of an authentication token"
     },
     {
         .name = "revoke",
         .description = "Revoke an authentication token",
         .usage = "polycall auth token revoke <token>",
         .handler = handle_token_revoke,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Revokes an active authentication token"
     }
 };
 
 /* Policy subcommands */
 static subcommand_t policy_subcommands[] = {
     {
         .name = "list",
         .description = "List security policies",
         .usage = "polycall auth policy list [--type <policy_type>]",
         .handler = handle_policy_list,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Lists configured security policies"
     },
     {
         .name = "set",
         .description = "Set a security policy",
         .usage = "polycall auth policy set <policy_name> <policy_value>",
         .handler = handle_policy_set,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Updates a specific security policy setting"
     },
     {
         .name = "reset",
         .description = "Reset security policies to default",
         .usage = "polycall auth policy reset [--confirm]",
         .handler = handle_policy_reset,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Resets all security policies to default values"
     }
 };
 
 /* Auth subcommands */
 static subcommand_t auth_subcommands[] = {
     {
         .name = "login",
         .description = "Authenticate with credentials",
         .usage = "polycall auth login <username> [--password]",
         .handler = handle_login,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Authenticate with the system using credentials"
     },
     {
         .name = "logout",
         .description = "End the current session",
         .usage = "polycall auth logout",
         .handler = handle_logout,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "End the current authenticated session"
     },
     {
         .name = "token",
         .description = "Token management",
         .usage = "polycall auth token <subcommand>",
         .handler = NULL, // No direct handler, uses subcommands
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Manage authentication tokens"
     },
     {
         .name = "policy",
         .description = "Security policy management",
         .usage = "polycall auth policy <subcommand>",
         .handler = NULL, // No direct handler, uses subcommands
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Manage security policies"
     }
 };
 
 /* Main auth command */
 static command_t auth_command = {
     .name = "auth",
     .description = "Authentication and security commands",
     .usage = "polycall auth <subcommand>",
     .handler = handle_auth,
     .subcommands = auth_subcommands,
     .subcommand_count = sizeof(auth_subcommands) / sizeof(auth_subcommands[0]),
     .requires_context = true,
     .text_type = POLYCALL_TEXT_COMMAND,
     .screen_reader_desc = "Commands for authentication, token management, and security policies"
 };
 
 /* Global state for maintaining auth context */
 static polycall_auth_context_t* g_auth_ctx = NULL;
 static bool g_logged_in = false;
 static char g_current_user[64] = {0};
 
 /**
  * @brief Initialize the auth subsystem
  */
 static command_result_t init_auth_subsystem(polycall_core_context_t* core_ctx) {
     if (g_auth_ctx) {
         return COMMAND_SUCCESS; // Already initialized
     }
     
     // Initialize auth context
     polycall_auth_config_t config;
     memset(&config, 0, sizeof(config));
     
     // Set sensible defaults
     config.token_lifetime = 3600; // 1 hour
     config.require_secure_channel = true;
     config.max_failed_attempts = 5;
     config.lockout_duration = 300; // 5 minutes
     
     polycall_core_error_t result = polycall_auth_init(
         core_ctx,
         &config,
         &g_auth_ctx
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to initialize auth subsystem: %d\n", result);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Format output text with appropriate accessibility styling
  */
 static void format_output(polycall_core_context_t* core_ctx, const char* text, polycall_text_type_t type, polycall_text_style_t style) {
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(core_ctx);
     
     if (access_ctx) {
         char formatted_text[1024];
         polycall_accessibility_format_text(
             core_ctx,
             access_ctx,
             text,
             type,
             style,
             formatted_text,
             sizeof(formatted_text)
         );
         printf("%s", formatted_text);
     } else {
         printf("%s", text);
     }
 }
 
 /**
  * @brief Format error message with appropriate accessibility styling
  */
 static void format_error(polycall_core_context_t* core_ctx, const char* text) {
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(core_ctx);
     
     if (access_ctx) {
         char formatted_text[1024];
         polycall_accessibility_format_text(
             core_ctx,
             access_ctx,
             text,
             POLYCALL_TEXT_ERROR,
             POLYCALL_STYLE_NORMAL,
             formatted_text,
             sizeof(formatted_text)
         );
         fprintf(stderr, "%s\n", formatted_text);
     } else {
         fprintf(stderr, "%s\n", text);
     }
 }
 
 /**
  * @brief Main auth command handler
  */
 static command_result_t handle_auth(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(core_ctx);
     
     // If no subcommand is specified, show help
     if (argc < 2) {
         if (access_ctx) {
             char title[256];
             polycall_accessibility_format_text(
                 core_ctx,
                 access_ctx,
                 "Authentication Commands",
                 POLYCALL_TEXT_HEADING,
                 POLYCALL_STYLE_BOLD,
                 title,
                 sizeof(title)
             );
             printf("\n%s\n\n", title);
         } else {
             printf("\nAuthentication Commands\n\n");
         }
         
         // Show subcommands
         for (int i = 0; i < auth_command.subcommand_count; i++) {
             const subcommand_t* subcmd = &auth_command.subcommands[i];
             
             if (access_ctx) {
                 char subcmd_name[128];
                 polycall_accessibility_format_text(
                     core_ctx,
                     access_ctx,
                     subcmd->name,
                     POLYCALL_TEXT_SUBCOMMAND,
                     POLYCALL_STYLE_NORMAL,
                     subcmd_name,
                     sizeof(subcmd_name)
                 );
                 
                 char subcmd_desc[256];
                 polycall_accessibility_format_text(
                     core_ctx,
                     access_ctx,
                     subcmd->description,
                     POLYCALL_TEXT_NORMAL,
                     POLYCALL_STYLE_NORMAL,
                     subcmd_desc,
                     sizeof(subcmd_desc)
                 );
                 
                 printf("  %-15s  %s\n", subcmd_name, subcmd_desc);
             } else {
                 printf("  %-15s  %s\n", subcmd->name, subcmd->description);
             }
         }
         
         printf("\nUse 'polycall help auth <subcommand>' for more information about a specific subcommand.\n");
         return COMMAND_SUCCESS;
     }
     
     // Get the subcommand
     const char* subcommand_name = argv[1];
     
     // Check for token subcommands
     if (strcmp(subcommand_name, "token") == 0) {
         if (argc < 3) {
             fprintf(stderr, "Missing token subcommand. Available subcommands: create, verify, revoke\n");
             return COMMAND_ERROR_INVALID_ARGUMENTS;
         }
         
         const char* token_cmd = argv[2];
         for (int i = 0; i < sizeof(token_subcommands) / sizeof(token_subcommands[0]); i++) {
             if (strcmp(token_subcommands[i].name, token_cmd) == 0) {
                 return token_subcommands[i].handler(argc - 2, &argv[2], context);
             }
         }
         
         fprintf(stderr, "Unknown token subcommand: %s\n", token_cmd);
         return COMMAND_ERROR_NOT_FOUND;
     }
     
     // Check for policy subcommands
     if (strcmp(subcommand_name, "policy") == 0) {
         if (argc < 3) {
             fprintf(stderr, "Missing policy subcommand. Available subcommands: list, set, reset\n");
             return COMMAND_ERROR_INVALID_ARGUMENTS;
         }
         
         const char* policy_cmd = argv[2];
         for (int i = 0; i < sizeof(policy_subcommands) / sizeof(policy_subcommands[0]); i++) {
             if (strcmp(policy_subcommands[i].name, policy_cmd) == 0) {
                 return policy_subcommands[i].handler(argc - 2, &argv[2], context);
             }
         }
         
         fprintf(stderr, "Unknown policy subcommand: %s\n", policy_cmd);
         return COMMAND_ERROR_NOT_FOUND;
     }
     
     // Find appropriate subcommand handler
     for (int i = 0; i < auth_command.subcommand_count; i++) {
         if (strcmp(auth_command.subcommands[i].name, subcommand_name) == 0) {
             command_handler_t handler = auth_command.subcommands[i].handler;
             if (handler) {
                 return handler(argc - 1, &argv[1], context);
             }
         }
     }
     
     // Subcommand not found
     if (access_ctx) {
         char error_msg[256];
         char unknown_cmd[128];
         snprintf(unknown_cmd, sizeof(unknown_cmd), "Unknown auth subcommand: %s", subcommand_name);
         
         polycall_accessibility_format_text(
             core_ctx, 
             access_ctx,
             unknown_cmd,
             POLYCALL_TEXT_ERROR,
             POLYCALL_STYLE_NORMAL,
             error_msg,
             sizeof(error_msg)
         );
         fprintf(stderr, "%s\n", error_msg);
     } else {
         fprintf(stderr, "Unknown auth subcommand: %s\n", subcommand_name);
     }
     
     return COMMAND_ERROR_NOT_FOUND;
 }
 
 /**
  * @brief Login command handler
  */
 static command_result_t handle_login(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Initialize auth subsystem
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Already logged in?
     if (g_logged_in) {
         char msg[128];
         snprintf(msg, sizeof(msg), "Already logged in as %s", g_current_user);
         format_output(core_ctx, msg, POLYCALL_TEXT_WARNING, POLYCALL_STYLE_NORMAL);
         printf("\n");
         return COMMAND_SUCCESS;
     }
     
     // Check username
     if (argc < 2) {
         format_error(core_ctx, "Username required. Usage: polycall auth login <username> [--password]");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     const char* username = argv[1];
     char password[128] = {0};
     bool password_provided = false;
     
     // Parse flags for password
     for (int i = 2; i < argc; i++) {
         if (strcmp(argv[i], "--password") == 0) {
             // Get password interactively
             printf("Password: ");
             // In a real implementation, you would use a secure method to read the password
             if (fgets(password, sizeof(password), stdin) != NULL) {
                 // Remove trailing newline
                 size_t len = strlen(password);
                 if (len > 0 && password[len-1] == '\n') {
                     password[len-1] = '\0';
                 }
                 password_provided = true;
             }
         }
     }
     
     // If password not provided through flag, check if provided as an argument
     if (!password_provided && argc > 2 && argv[2][0] != '-') {
         strncpy(password, argv[2], sizeof(password) - 1);
         password_provided = true;
     }
     
     // If still no password, prompt for it
     if (!password_provided) {
         printf("Password: ");
         // In a real implementation, you would use a secure method to read the password
         if (fgets(password, sizeof(password), stdin) != NULL) {
             // Remove trailing newline
             size_t len = strlen(password);
             if (len > 0 && password[len-1] == '\n') {
                 password[len-1] = '\0';
             }
         }
     }
     
     // Perform authentication
     polycall_auth_token_t token;
     polycall_core_error_t result = polycall_auth_authenticate(
         core_ctx,
         g_auth_ctx,
         username,
         password,
         &token
     );
     
     // Clear password from memory for security
     memset(password, 0, sizeof(password));
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Authentication failed: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Store authentication state
     g_logged_in = true;
     strncpy(g_current_user, username, sizeof(g_current_user) - 1);
     
     // Success message
     char success_msg[256];
     snprintf(success_msg, sizeof(success_msg), "Successfully logged in as %s", username);
     format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_BOLD);
     printf("\n");
     
     // Token info
     char token_info[512];
     snprintf(token_info, sizeof(token_info), "Token: %.16s... (expires in %u seconds)", 
              token.token_string, token.expiry);
     format_output(core_ctx, token_info, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
     printf("\n");
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Logout command handler
  */
 static command_result_t handle_logout(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Check if logged in
     if (!g_logged_in) {
         format_error(core_ctx, "Not logged in");
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Perform logout
     polycall_core_error_t result = polycall_auth_logout(
         core_ctx,
         g_auth_ctx
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Logout failed: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Clear authentication state
     g_logged_in = false;
     memset(g_current_user, 0, sizeof(g_current_user));
     
     // Success message
     format_output(core_ctx, "Successfully logged out", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
     printf("\n");
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Token create command handler
  */
 static command_result_t handle_token_create(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Parse arguments
     const char* token_type = NULL;
     const char* identity = NULL;
     uint32_t expiry = 3600; // Default 1 hour
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
             token_type = argv[++i];
         } else if (strcmp(argv[i], "--identity") == 0 && i + 1 < argc) {
             identity = argv[++i];
         } else if (strcmp(argv[i], "--expiry") == 0 && i + 1 < argc) {
             expiry = (uint32_t)atoi(argv[++i]);
         }
     }
     
     // Check required arguments
     if (!token_type || !identity) {
         format_error(core_ctx, "Missing required arguments. Usage: polycall auth token create --type <token_type> --identity <identity> [--expiry <seconds>]");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Create token
     polycall_auth_token_t token;
     polycall_core_error_t result = polycall_auth_create_token(
         core_ctx,
         g_auth_ctx,
         token_type,
         identity,
         expiry,
         &token
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Failed to create token: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Display token information
     format_output(core_ctx, "Token created successfully", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_BOLD);
     printf("\n\n");
     
     char token_info[2048];
     snprintf(token_info, sizeof(token_info),
              "Token: %s\n"
              "Type: %s\n"
              "Identity: %s\n"
              "Expiry: %u seconds\n"
              "Created: %s",
              token.token_string,
              token.type,
              token.identity,
              token.expiry,
              ctime(&token.created_at));
     
     format_output(core_ctx, token_info, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Token verify command handler
  */
 static command_result_t handle_token_verify(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Check token argument
     if (argc < 2) {
         format_error(core_ctx, "Token required. Usage: polycall auth token verify <token>");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     const char* token_string = argv[1];
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Verify token
     polycall_auth_token_info_t token_info;
     polycall_core_error_t result = polycall_auth_verify_token(
         core_ctx,
         g_auth_ctx,
         token_string,
         &token_info
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Token verification failed: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Display token information
     format_output(core_ctx, "Token is valid", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_BOLD);
     printf("\n\n");
     
     char info_str[1024];
     snprintf(info_str, sizeof(info_str),
              "Type: %s\n"
              "Identity: %s\n"
              "Issued at: %s"
              "Expires at: %s"
              "Remaining validity: %ld seconds\n",
              token_info.type,
              token_info.identity,
              ctime(&token_info.issued_at),
              ctime(&token_info.expires_at),
              (long)(token_info.expires_at - time(NULL)));
     
     format_output(core_ctx, info_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Token revoke command handler
  */
 static command_result_t handle_token_revoke(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Check token argument
     if (argc < 2) {
         format_error(core_ctx, "Token required. Usage: polycall auth token revoke <token>");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     const char* token_string = argv[1];
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Revoke token
     polycall_core_error_t result = polycall_auth_revoke_token(
         core_ctx,
         g_auth_ctx,
         token_string
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Failed to revoke token: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Success message
     format_output(core_ctx, "Token revoked successfully", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
     printf("\n");
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Policy list command handler
  */
 static command_result_t handle_policy_list(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Parse policy type argument
     const char* policy_type = NULL;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
             policy_type = argv[++i];
         }
     }
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // List security policies
     polycall_auth_policy_list_t policy_list;
     polycall_core_error_t result = polycall_auth_list_policies(
         core_ctx,
         g_auth_ctx,
         policy_type,
         &policy_list
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Failed to list policies: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Display policies
     char title[256];
     if (policy_type) {
         snprintf(title, sizeof(title), "Security Policies (%s)", policy_type);
     } else {
         snprintf(title, sizeof(title), "All Security Policies");
     }
     
     format_output(core_ctx, title, POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD);
     printf("\n\n");
     
     // In a real implementation, this would iterate through policy_list
     // For demonstration, show some sample policies
     const char* policies[] = {
         "password.min_length = 12",
         "password.require_uppercase = true",
         "password.require_lowercase = true",
         "password.require_digit = true",
         "password.require_special = true",
         "password.expiry_days = 90",
         "session.max_duration = 3600",
         "session.idle_timeout = 600",
         "token.default_lifetime = 3600",
         "access.max_failed_attempts = 5",
         "access.lockout_duration = 300"
     };
     
     for (int i = 0; i < sizeof(policies) / sizeof(policies[0]); i++) {
         format_output(core_ctx, policies[i], POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Policy set command handler
  */
 static command_result_t handle_policy_set(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Check required arguments
     if (argc < 3) {
         format_error(core_ctx, "Missing required arguments. Usage: polycall auth policy set <policy_name> <policy_value>");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     const char* policy_name = argv[1];
     const char* policy_value = argv[2];
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Set policy
     polycall_core_error_t result = polycall_auth_set_policy(
         core_ctx,
         g_auth_ctx,
         policy_name,
         policy_value
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Failed to set policy: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Success message
     char success_msg[256];
     snprintf(success_msg, sizeof(success_msg), "Policy '%s' set to '%s'", policy_name, policy_value);
     format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
     printf("\n");
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Policy reset command handler
  */
 static command_result_t handle_policy_reset(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Check for confirmation flag
     bool confirmed = false;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--confirm") == 0) {
             confirmed = true;
             break;
         }
     }
     
     if (!confirmed) {
         format_error(core_ctx, "This will reset ALL security policies to their defaults. Use --confirm to proceed.");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     // Initialize auth subsystem if needed
     command_result_t cmd_result = init_auth_subsystem(core_ctx);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Reset policies
     polycall_core_error_t result = polycall_auth_reset_policies(
         core_ctx,
         g_auth_ctx
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         char error_msg[128];
         snprintf(error_msg, sizeof(error_msg), "Failed to reset policies: %s", 
                  polycall_core_get_error_string(result));
         format_error(core_ctx, error_msg);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     // Success message
     format_output(core_ctx, "All security policies have been reset to their default values", 
                  POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_BOLD);
     printf("\n");
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Register authentication commands
  */
 bool register_auth_commands(void) {
     return cli_register_command(&auth_command);
 }