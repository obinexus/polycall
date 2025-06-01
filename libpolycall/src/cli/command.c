/**
 * @file command.c
 * @brief Implementation of the command system for LibPolyCall CLI
 *
 * Provides the core functionality for command registration,
 * discovery, and execution with accessibility integration.
 *
 * @copyright OBINexus Computing, 2025
 */

 #include "polycall/cli/command.h"
 #include "polycall/core/accessibility/accessibility_interface.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 /* Maximum number of registered commands */
 #define MAX_COMMANDS 32
 
 /* Static array of registered commands */
 static command_t commands[MAX_COMMANDS];
 static int command_count = 0;
 
 /**
  * @brief Register a command
  */
 bool cli_register_command(const command_t* command) {
     if (!command || !command->name || !command->handler) {
         return false;
     }
     
     if (command_count >= MAX_COMMANDS) {
         fprintf(stderr, "Cannot register command: maximum number of commands reached\n");
         return false;
     }
     
     /* Check for duplicate command */
     for (int i = 0; i < command_count; i++) {
         if (strcmp(commands[i].name, command->name) == 0) {
             fprintf(stderr, "Cannot register command: command '%s' already exists\n", command->name);
             return false;
         }
     }
     
     /* Copy command */
     memcpy(&commands[command_count], command, sizeof(command_t));
     command_count++;
     
     return true;
 }
 
 /**
  * @brief Find a command by name
  */
 const command_t* find_command(const char* name) {
     if (!name) {
         return NULL;
     }
     
     for (int i = 0; i < command_count; i++) {
         if (strcmp(commands[i].name, name) == 0) {
             return &commands[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Find a subcommand by name
  */
 const subcommand_t* find_subcommand(const command_t* command, const char* name) {
     if (!command || !name) {
         return NULL;
     }
     
     for (int i = 0; i < command->subcommand_count; i++) {
         if (strcmp(command->subcommands[i].name, name) == 0) {
             return &command->subcommands[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Process command-line arguments and dispatch to appropriate handler
  */
 int process_command_line(int argc, char** argv, polycall_core_context_t* context) {
     if (argc < 2) {
         // No command specified, show help
         return show_help(NULL, context);
     }
     
     const char* command_name = argv[1];
     
     // Check for help request
     if (strcmp(command_name, "help") == 0) {
         if (argc > 2) {
             // Help for a specific command
             if (argc > 3) {
                 // Help for a specific subcommand
                 const command_t* command = find_command(argv[2]);
                 if (command) {
                     const subcommand_t* subcommand = find_subcommand(command, argv[3]);
                     if (subcommand) {
                         return show_subcommand_help(command, subcommand, context);
                     } else {
                         fprintf(stderr, "Unknown subcommand: %s\n", argv[3]);
                         return COMMAND_ERROR_NOT_FOUND;
                     }
                 } else {
                     fprintf(stderr, "Unknown command: %s\n", argv[2]);
                     return COMMAND_ERROR_NOT_FOUND;
                 }
             } else {
                 // Help for a command
                 return show_help(argv[2], context);
             }
         } else {
             // General help
             return show_help(NULL, context);
         }
     }
     
     // Find and execute command
     const command_t* command = find_command(command_name);
     if (!command) {
         polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
         char error_buffer[256];
         
         if (access_ctx) {
             char error_msg[128];
             snprintf(error_msg, sizeof(error_msg), "Unknown command: %s", command_name);
             
             polycall_accessibility_format_text(
                 context,
                 access_ctx,
                 error_msg,
                 POLYCALL_TEXT_ERROR,
                 POLYCALL_STYLE_NORMAL,
                 error_buffer,
                 sizeof(error_buffer)
             );
         } else {
             snprintf(error_buffer, sizeof(error_buffer), "Unknown command: %s", command_name);
         }
         
         fprintf(stderr, "%s\n\n", error_buffer);
         show_help(NULL, context);
         return COMMAND_ERROR_NOT_FOUND;
     }
     
     // Check if the command requires a context
     if (command->requires_context && !context) {
         fprintf(stderr, "Command '%s' requires an initialized context\n", command_name);
         return COMMAND_ERROR_CONTEXT_REQUIRED;
     }
     
     // Check for a subcommand
     if (command->subcommand_count > 0 && argc > 2) {
         const char* subcommand_name = argv[2];
         const subcommand_t* subcommand = find_subcommand(command, subcommand_name);
         
         if (subcommand) {
             // Check if the subcommand requires a context
             if (subcommand->requires_context && !context) {
                 fprintf(stderr, "Subcommand '%s' requires an initialized context\n", subcommand_name);
                 return COMMAND_ERROR_CONTEXT_REQUIRED;
             }
             
             // Execute subcommand with remaining arguments
             return subcommand->handler(argc - 2, &argv[2], context);
         } else {
             // No matching subcommand, let the command handle it
             // (Some commands handle their own subcommands)
         }
     }
     
     // Execute command with remaining arguments
     return command->handler(argc - 1, &argv[1], context);
 }
 
 /**
  * @brief Execute a command
  */
 command_result_t cli_execute_command(int argc, char** argv, void* context) {
     if (argc < 1 || !argv || !argv[0]) {
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     const char* command_name = argv[0];
     
     // Get accessibility context if available
     polycall_accessibility_context_t* access_ctx = NULL;
     if (context) {
         polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
         access_ctx = get_accessibility_context(core_ctx);
     }
     
     // Find the command
     const command_t* command = find_command(command_name);
     if (!command) {
         // Format error message with accessibility if available
         if (access_ctx) {
             char error_msg[256];
             char unknown_cmd[128];
             snprintf(unknown_cmd, sizeof(unknown_cmd), "Unknown command: %s", command_name);
             
             polycall_accessibility_format_text(
                 context,
                 access_ctx,
                 unknown_cmd,
                 POLYCALL_TEXT_ERROR,
                 POLYCALL_STYLE_NORMAL,
                 error_msg,
                 sizeof(error_msg)
             );
             fprintf(stderr, "%s\n", error_msg);
         } else {
             fprintf(stderr, "Unknown command: %s\n", command_name);
         }
         return COMMAND_ERROR_NOT_FOUND;
     }
     
     // Check if the command requires a context
     if (command->requires_context && !context) {
         // Format error message with accessibility if available
         if (access_ctx) {
             char error_msg[256];
             polycall_accessibility_format_error(
                 context,
                 access_ctx,
                 POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                 "Command requires an initialized context",
                 error_msg,
                 sizeof(error_msg)
             );
             fprintf(stderr, "%s\n", error_msg);
         } else {
             fprintf(stderr, "Command '%s' requires an initialized context\n", command_name);
         }
         return COMMAND_ERROR_CONTEXT_REQUIRED;
     }
     
     // Check for a subcommand (if argc > 1)
     if (command->subcommand_count > 0 && argc > 1) {
         const char* subcommand_name = argv[1];
         const subcommand_t* subcommand = find_subcommand(command, subcommand_name);
         
         if (subcommand) {
             // Check if the subcommand requires a context
             if (subcommand->requires_context && !context) {
                 // Format error message with accessibility if available
                 if (access_ctx) {
                     char error_msg[256];
                     char error_text[128];
                     snprintf(error_text, sizeof(error_text), 
                              "Subcommand '%s' requires an initialized context", subcommand_name);
                     
                     polycall_accessibility_format_text(
                         context,
                         access_ctx,
                         error_text,
                         POLYCALL_TEXT_ERROR,
                         POLYCALL_STYLE_NORMAL,
                         error_msg,
                         sizeof(error_msg)
                     );
                     fprintf(stderr, "%s\n", error_msg);
                 } else {
                     fprintf(stderr, "Subcommand '%s' requires an initialized context\n", subcommand_name);
                 }
                 return COMMAND_ERROR_CONTEXT_REQUIRED;
             }
             
             // Execute subcommand with remaining arguments
             return subcommand->handler(argc - 1, &argv[1], context);
         }
     }
     
     // Execute command
     return command->handler(argc, argv, context);
 }
 
 /**
  * @brief Get command help
  */
 const command_t* cli_get_command_help(const char* name) {
     return find_command(name);
 }
 
 /**
  * @brief Show help for a subcommand
  */
 int show_subcommand_help(const command_t* command, const subcommand_t* subcommand, 
                          polycall_core_context_t* context) {
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
     
     // Format and display subcommand help
     if (access_ctx) {
         char title[256];
         char cmd_subcmd[128];
         snprintf(cmd_subcmd, sizeof(cmd_subcmd), "%s %s", command->name, subcommand->name);
         
         polycall_accessibility_format_text(
             context,
             access_ctx,
             cmd_subcmd,
             POLYCALL_TEXT_HEADING,
             POLYCALL_STYLE_BOLD,
             title,
             sizeof(title)
         );
         printf("%s\n\n", title);
         
         char description[512];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             subcommand->description,
             POLYCALL_TEXT_NORMAL,
             POLYCALL_STYLE_NORMAL,
             description,
             sizeof(description)
         );
         printf("%s\n\n", description);
         
         char usage_label[64];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             "Usage:",
             POLYCALL_TEXT_COMMAND,
             POLYCALL_STYLE_BOLD,
             usage_label,
             sizeof(usage_label)
         );
         
         char usage_text[512];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             subcommand->usage,
             POLYCALL_TEXT_NORMAL,
             POLYCALL_STYLE_NORMAL,
             usage_text,
             sizeof(usage_text)
         );
         printf("%s %s\n", usage_label, usage_text);
     } else {
         printf("%s %s\n\n", command->name, subcommand->name);
         printf("%s\n\n", subcommand->description);
         printf("Usage: %s\n", subcommand->usage);
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Show help for a command or general help
  */
 int show_help(const char* command_name, polycall_core_context_t* context) {
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
     
     if (!command_name) {
         // Show general help
         char title[256];
         
         if (access_ctx) {
             polycall_accessibility_format_text(
                 context,
                 access_ctx,
                 "LibPolyCall Command-Line Interface",
                 POLYCALL_TEXT_HEADING,
                 POLYCALL_STYLE_BOLD,
                 title,
                 sizeof(title)
             );
             printf("%s\n\n", title);
         } else {
             printf("LibPolyCall Command-Line Interface\n\n");
         }
         
         printf("Usage: polycall [command] [subcommand] [arguments] [--flags]\n\n");
         printf("Available commands:\n");
         
         // List all commands with descriptions
         for (int i = 0; i < command_count; i++) {
             if (access_ctx) {
                 char cmd_name[128];
                 polycall_accessibility_format_text(
                     context,
                     access_ctx,
                     commands[i].name,
                     POLYCALL_TEXT_COMMAND,
                     POLYCALL_STYLE_NORMAL,
                     cmd_name,
                     sizeof(cmd_name)
                 );
                 
                 char cmd_desc[256];
                 polycall_accessibility_format_text(
                     context,
                     access_ctx,
                     commands[i].description,
                     POLYCALL_TEXT_NORMAL,
                     POLYCALL_STYLE_NORMAL,
                     cmd_desc,
                     sizeof(cmd_desc)
                 );
                 
                 printf("  %-15s  %s\n", cmd_name, cmd_desc);
             } else {
                 printf("  %-15s  %s\n", commands[i].name, commands[i].description);
             }
         }
         
         printf("\nFor more information on a specific command, run: polycall help [command]\n");
         return COMMAND_SUCCESS;
     }
     
     // Show help for a specific command
     const command_t* command = find_command(command_name);
     if (!command) {
         fprintf(stderr, "Unknown command: %s\n", command_name);
         return COMMAND_ERROR_NOT_FOUND;
     }
     
     // Format and display command help
     if (access_ctx) {
         char title[256];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             command->name,
             POLYCALL_TEXT_HEADING,
             POLYCALL_STYLE_BOLD,
             title,
             sizeof(title)
         );
         printf("%s\n\n", title);
         
         char description[512];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             command->description,
             POLYCALL_TEXT_NORMAL,
             POLYCALL_STYLE_NORMAL,
             description,
             sizeof(description)
         );
         printf("%s\n\n", description);
         
         char usage_label[64];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             "Usage:",
             POLYCALL_TEXT_COMMAND,
             POLYCALL_STYLE_BOLD,
             usage_label,
             sizeof(usage_label)
         );
         
         char usage_text[512];
         polycall_accessibility_format_text(
             context,
             access_ctx,
             command->usage,
             POLYCALL_TEXT_NORMAL,
             POLYCALL_STYLE_NORMAL,
             usage_text,
             sizeof(usage_text)
         );
         printf("%s %s\n", usage_label, usage_text);
     } else {
         printf("%s\n\n", command->name);
         printf("%s\n\n", command->description);
         printf("Usage: %s\n", command->usage);
     }
     
     // Show subcommands if available
     if (command->subcommand_count > 0) {
         printf("\nAvailable subcommands:\n");
         
         for (int i = 0; i < command->subcommand_count; i++) {
             if (access_ctx) {
                 char subcmd_name[128];
                 polycall_accessibility_format_text(
                     context,
                     access_ctx,
                     command->subcommands[i].name,
                     POLYCALL_TEXT_SUBCOMMAND,
                     POLYCALL_STYLE_NORMAL,
                     subcmd_name,
                     sizeof(subcmd_name)
                 );
                 
                 char subcmd_desc[256];
                 polycall_accessibility_format_text(
                     context,
                     access_ctx,
                     command->subcommands[i].description,
                     POLYCALL_TEXT_NORMAL,
                     POLYCALL_STYLE_NORMAL,
                     subcmd_desc,
                     sizeof(subcmd_desc)
                 );
                 
                 printf("  %-15s  %s\n", subcmd_name, subcmd_desc);
             } else {
                 printf("  %-15s  %s\n", 
                        command->subcommands[i].name, 
                        command->subcommands[i].description);
             }
         }
         
         printf("\nFor more information on a subcommand, run: polycall help %s [subcommand]\n", 
                command->name);
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Parse flags from arguments
  */
 bool parse_flags(int argc, char** argv, command_flag_t* flags, int flag_count, 
                  char** remaining_args, int* remaining_count) {
     if (!argv || !flags || !remaining_args || !remaining_count) {
         return false;
     }
     
     int remaining_idx = 0;
     
     // Reset flag presence
     for (int i = 0; i < flag_count; i++) {
         flags[i].is_present = false;
         if (flags[i].value) {
             free(flags[i].value);
             flags[i].value = NULL;
         }
     }
     
     // Parse arguments
     for (int i = 0; i < argc; i++) {
         if (argv[i][0] == '-') {
             if (argv[i][1] == '-') {
                 // Long flag (--flag)
                 const char* flag_name = &argv[i][2];
                 
                 // Find matching flag
                 bool found = false;
                 for (int j = 0; j < flag_count; j++) {
                     if (strcmp(flags[j].name, flag_name) == 0) {
                         flags[j].is_present = true;
                         found = true;
                         
                         // Check if flag requires a value
                         if (flags[j].requires_value) {
                             if (i + 1 < argc && argv[i + 1][0] != '-') {
                                 // Next argument is the value
                                 flags[j].value = strdup(argv[i + 1]);
                                 i++; // Skip the value in the next iteration
                             } else {
                                 // Missing value
                                 fprintf(stderr, "Flag --%s requires a value\n", flag_name);
                                 return false;
                             }
                         }
                         
                         break;
                     }
                 }
                 
                 if (!found) {
                     fprintf(stderr, "Unknown flag: %s\n", argv[i]);
                     return false;
                 }
             } else {
                 // Short flag (-f)
                 const char* short_flags = &argv[i][1];
                 size_t len = strlen(short_flags);
                 
                 for (size_t c = 0; c < len; c++) {
                     char short_flag = short_flags[c];
                     
                     // Find matching flag
                     bool found = false;
                     for (int j = 0; j < flag_count; j++) {
                         if (flags[j].short_name && flags[j].short_name[0] == short_flag) {
                             flags[j].is_present = true;
                             found = true;
                             
                             // Check if flag requires a value
                             if (flags[j].requires_value) {
                                 if (c == len - 1 && i + 1 < argc && argv[i + 1][0] != '-') {
                                     // Next argument is the value
                                     flags[j].value = strdup(argv[i + 1]);
                                     i++; // Skip the value in the next iteration
                                 } else if (c == len - 1) {
                                     // Missing value
                                     fprintf(stderr, "Flag -%c requires a value\n", short_flag);
                                     return false;
                                 } else {
                                     // Flag requiring value in the middle of a combined flag
                                     fprintf(stderr, "Flag -%c requires a value and cannot be combined\n", short_flag);
                                     return false;
                                 }
                             }
                             
                             break;
                         }
                     }
                     
                     if (!found) {
                         fprintf(stderr, "Unknown flag: -%c\n", short_flag);
                         return false;
                     }
                 }
             }
         } else {
             // Not a flag, add to remaining arguments
             if (remaining_idx < *remaining_count) {
                 remaining_args[remaining_idx++] = argv[i];
             } else {
                 // Too many arguments
                 fprintf(stderr, "Too many arguments\n");
                 return false;
             }
         }
     }
     
     // Update remaining count
     *remaining_count = remaining_idx;
     
     return true;
 }
 
 /**
  * @brief Initialize the command system
  */
 bool cli_init_commands(void) {
     command_count = 0;
     return register_all_commands();
 }
 
 /**
  * @brief Cleanup the command system
  */
 void cli_cleanup_commands(void) {
     command_count = 0;
 }
 
 /**
  * @brief Register all built-in commands
  * 
  * This function is implemented in command_registry.c
  */
 extern bool register_all_commands(void);