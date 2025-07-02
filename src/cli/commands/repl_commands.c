/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file repl_commands.c
 * @brief REPL command implementations for LibPolyCall CLI
 *
 * Provides commands for the REPL (Read-Eval-Print Loop) interactive
 * shell, with integrated Biafran UI/UX design system accessibility support.
 *
 * @copyright OBINexus Computing, 2025
 */

 #include "polycall/cli/commands/repl_commands.h"
 #include "polycall/cli/command.h"
 #include "polycall/cli/repl.h"
 #include "polycall/core/accessibility/accessibility_interface.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 /* Forward declarations of command handlers */
 static command_result_t handle_repl(int argc, char** argv, void* context);
 static command_result_t handle_start(int argc, char** argv, void* context);
 static command_result_t handle_history(int argc, char** argv, void* context);
 static command_result_t handle_log_inspection(int argc, char** argv, void* context);
 static command_result_t handle_settings(int argc, char** argv, void* context);
 
 /* REPL subcommands */
 static subcommand_t repl_subcommands[] = {
     {
         .name = "start",
         .description = "Start interactive REPL",
         .usage = "polycall repl start [options]",
         .handler = handle_start,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Start an interactive REPL session with optional configuration"
     },
     {
         .name = "history",
         .description = "Manage command history",
         .usage = "polycall repl history [--clear] [--load <file>] [--save <file>]",
         .handler = handle_history,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "View or manage command history"
     },
     {
         .name = "log-inspection",
         .description = "Enable or configure log inspection",
         .usage = "polycall repl log-inspection [--enable] [--disable] [--filter <pattern>]",
         .handler = handle_log_inspection,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "Configure log inspection mode for debugging"
     },
     {
         .name = "settings",
         .description = "View or modify REPL settings",
         .usage = "polycall repl settings [--list] [--set <key>=<value>] [--reset]",
         .handler = handle_settings,
         .requires_context = true,
         .text_type = POLYCALL_TEXT_SUBCOMMAND,
         .screen_reader_desc = "View or modify REPL configuration settings"
     }
 };
 
 /* Main REPL command */
 static command_t repl_command = {
     .name = "repl",
     .description = "Interactive shell commands",
     .usage = "polycall repl <subcommand>",
     .handler = handle_repl,
     .subcommands = repl_subcommands,
     .subcommand_count = sizeof(repl_subcommands) / sizeof(repl_subcommands[0]),
     .requires_context = true,
     .text_type = POLYCALL_TEXT_COMMAND,
     .screen_reader_desc = "Manage the interactive shell and its settings"
 };
 
 /* Global REPL context */
 static polycall_repl_context_t* g_repl_ctx = NULL;
 
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
  * @brief Initialize the REPL subsystem
  */
 static command_result_t init_repl_subsystem(polycall_core_context_t* core_ctx, polycall_repl_config_t* config) {
     if (g_repl_ctx) {
         return COMMAND_SUCCESS; // Already initialized
     }
     
     // If no config provided, use defaults
     polycall_repl_config_t default_config;
     if (!config) {
         default_config = polycall_repl_default_config();
         config = &default_config;
     }
     
     // Initialize REPL context
     polycall_core_error_t result = polycall_repl_init(
         core_ctx,
         config,
         &g_repl_ctx
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "Failed to initialize REPL subsystem: %d\n", result);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Main REPL command handler
  */
 static command_result_t handle_repl(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(core_ctx);
     
     // If no subcommand is specified, show help
     if (argc < 2) {
         if (access_ctx) {
             char title[256];
             polycall_accessibility_format_text(
                 core_ctx,
                 access_ctx,
                 "REPL Commands",
                 POLYCALL_TEXT_HEADING,
                 POLYCALL_STYLE_BOLD,
                 title,
                 sizeof(title)
             );
             printf("\n%s\n\n", title);
         } else {
             printf("\nREPL Commands\n\n");
         }
         
         // Show subcommands
         for (int i = 0; i < repl_command.subcommand_count; i++) {
             const subcommand_t* subcmd = &repl_command.subcommands[i];
             
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
         
         printf("\nUse 'polycall help repl <subcommand>' for more information about a specific subcommand.\n");
         printf("Run 'polycall repl start' without arguments to launch an interactive REPL with default settings.\n");
         return COMMAND_SUCCESS;
     }
     
     // Get the subcommand
     const char* subcommand_name = argv[1];
     
     // Find appropriate subcommand handler
     for (int i = 0; i < repl_command.subcommand_count; i++) {
         if (strcmp(repl_command.subcommands[i].name, subcommand_name) == 0) {
             command_handler_t handler = repl_command.subcommands[i].handler;
             if (handler) {
                 return handler(argc - 1, &argv[1], context);
             }
         }
     }
     
     // Subcommand not found
     if (access_ctx) {
         char error_msg[256];
         char unknown_cmd[128];
         snprintf(unknown_cmd, sizeof(unknown_cmd), "Unknown repl subcommand: %s", subcommand_name);
         
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
         fprintf(stderr, "Unknown repl subcommand: %s\n", subcommand_name);
     }
     
     return COMMAND_ERROR_NOT_FOUND;
 }
 
 /**
  * @brief Start REPL command handler
  */
 static command_result_t handle_start(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     polycall_accessibility_context_t* access_ctx = get_accessibility_context(core_ctx);
     
     // Parse options
     polycall_repl_config_t config = polycall_repl_default_config();
     char* history_file = NULL;
     bool history_specified = false;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--no-history") == 0) {
             config.enable_history = false;
         } else if (strcmp(argv[i], "--no-completion") == 0) {
             config.enable_completion = false;
         } else if (strcmp(argv[i], "--no-highlighting") == 0) {
             config.enable_syntax_highlighting = false;
         } else if (strcmp(argv[i], "--log-inspection") == 0) {
             config.enable_log_inspection = true;
         } else if (strcmp(argv[i], "--zero-trust-inspection") == 0) {
             config.enable_zero_trust_inspection = true;
         } else if (strncmp(argv[i], "--history-file=", 15) == 0) {
             const char* file_path = argv[i] + 15;
             history_file = strdup(file_path);
             config.history_file = history_file;
             history_specified = true;
         } else if (strncmp(argv[i], "--prompt=", 9) == 0) {
             config.prompt = strdup(argv[i] + 9);
         } else if (strncmp(argv[i], "--max-history=", 14) == 0) {
             config.max_history_entries = atoi(argv[i] + 14);
             if (config.max_history_entries <= 0) {
                 config.max_history_entries = 100; // Default if invalid
             }
         }
     }
     
     // If no history file specified, use default
     if (!history_specified) {
         const char* home = getenv("HOME");
 #ifdef _WIN32
         if (!home) {
             home = getenv("USERPROFILE");
         }
 #endif
         if (home) {
             history_file = malloc(strlen(home) + 20);
             if (history_file) {
                 sprintf(history_file, "%s/.polycall_history", home);
                 config.history_file = history_file;
             }
         }
     }
     
     // Format welcome message
     if (access_ctx) {
         char welcome[512];
         polycall_accessibility_format_text(
             core_ctx,
             access_ctx,
             "LibPolyCall Interactive Shell",
             POLYCALL_TEXT_HEADING,
             POLYCALL_STYLE_BOLD,
             welcome,
             sizeof(welcome)
         );
         printf("\n%s\n\n", welcome);
     } else {
         printf("\nLibPolyCall Interactive Shell\n\n");
     }
     
     // Initialize REPL
     command_result_t cmd_result = init_repl_subsystem(core_ctx, &config);
     if (cmd_result != COMMAND_SUCCESS) {
         // Free allocated memory
         if (history_file) {
             free(history_file);
         }
         if (config.prompt && strcmp(config.prompt, "polycall> ") != 0) {
             free((void*)config.prompt);
         }
         return cmd_result;
     }
     
     // Run REPL
     polycall_core_error_t result = polycall_repl_run(
         core_ctx,
         g_repl_ctx
     );
     
     // Clean up
     if (history_file) {
         free(history_file);
     }
     if (config.prompt && strcmp(config.prompt, "polycall> ") != 0) {
         free((void*)config.prompt);
     }
     
     if (result != POLYCALL_CORE_SUCCESS) {
         fprintf(stderr, "REPL exited with error: %d\n", result);
         return COMMAND_ERROR_EXECUTION_FAILED;
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief History command handler
  */
 static command_result_t handle_history(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Parse options
     bool clear_history = false;
     const char* load_file = NULL;
     const char* save_file = NULL;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--clear") == 0) {
             clear_history = true;
         } else if (strncmp(argv[i], "--load=", 7) == 0) {
             load_file = argv[i] + 7;
         } else if (strncmp(argv[i], "--save=", 7) == 0) {
             save_file = argv[i] + 7;
         }
     }
     
     // Initialize REPL if needed
     command_result_t cmd_result = init_repl_subsystem(core_ctx, NULL);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Process history operations
     if (clear_history) {
         polycall_core_error_t result = polycall_repl_clear_history(
             core_ctx,
             g_repl_ctx
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to clear history");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         format_output(core_ctx, "Command history cleared", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     if (load_file) {
         polycall_core_error_t result = polycall_repl_load_history(
             core_ctx,
             g_repl_ctx,
             load_file
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             char error_msg[256];
             snprintf(error_msg, sizeof(error_msg), "Failed to load history from %s", load_file);
             format_error(core_ctx, error_msg);
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         char success_msg[256];
         snprintf(success_msg, sizeof(success_msg), "Command history loaded from %s", load_file);
         format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     if (save_file) {
         polycall_core_error_t result = polycall_repl_save_history(
             core_ctx,
             g_repl_ctx,
             save_file
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             char error_msg[256];
             snprintf(error_msg, sizeof(error_msg), "Failed to save history to %s", save_file);
             format_error(core_ctx, error_msg);
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         char success_msg[256];
         snprintf(success_msg, sizeof(success_msg), "Command history saved to %s", save_file);
         format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     // If no options specified, display history
     if (!clear_history && !load_file && !save_file) {
         // Get history entries
         char** history_entries = NULL;
         size_t entry_count = 0;
         
         polycall_core_error_t result = polycall_repl_get_history(
             core_ctx,
             g_repl_ctx,
             &history_entries,
             &entry_count
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to retrieve command history");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         // Display history
         format_output(core_ctx, "Command History", POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD);
         printf("\n\n");
         
         if (entry_count == 0) {
             format_output(core_ctx, "No history entries", POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_ITALIC);
             printf("\n");
         } else {
             for (size_t i = 0; i < entry_count; i++) {
                 char entry_str[1024];
                 snprintf(entry_str, sizeof(entry_str), "%3zu  %s", i + 1, history_entries[i]);
                 format_output(core_ctx, entry_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
                 printf("\n");
             }
         }
         
         // Free history entries
         for (size_t i = 0; i < entry_count; i++) {
             free(history_entries[i]);
         }
         free(history_entries);
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Log inspection command handler
  */
 static command_result_t handle_log_inspection(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Parse options
     bool enable = false;
     bool disable = false;
     const char* filter = NULL;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--enable") == 0) {
             enable = true;
         } else if (strcmp(argv[i], "--disable") == 0) {
             disable = true;
         } else if (strncmp(argv[i], "--filter=", 9) == 0) {
             filter = argv[i] + 9;
         }
     }
     
     // Check for conflicting options
     if (enable && disable) {
         format_error(core_ctx, "Cannot both enable and disable log inspection");
         return COMMAND_ERROR_INVALID_ARGUMENTS;
     }
     
     // Initialize REPL if needed
     command_result_t cmd_result = init_repl_subsystem(core_ctx, NULL);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Process log inspection operations
     if (enable) {
         polycall_core_error_t result = polycall_repl_enable_log_inspection(
             core_ctx,
             g_repl_ctx,
             filter
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to enable log inspection");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         char success_msg[256];
         if (filter) {
             snprintf(success_msg, sizeof(success_msg), "Log inspection enabled with filter: %s", filter);
         } else {
             snprintf(success_msg, sizeof(success_msg), "Log inspection enabled");
         }
         format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     } else if (disable) {
         polycall_core_error_t result = polycall_repl_disable_log_inspection(
             core_ctx,
             g_repl_ctx
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to disable log inspection");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         format_output(core_ctx, "Log inspection disabled", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     } else if (filter) {
         // Just update the filter
         polycall_core_error_t result = polycall_repl_set_log_filter(
             core_ctx,
             g_repl_ctx,
             filter
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to set log filter");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         char success_msg[256];
         snprintf(success_msg, sizeof(success_msg), "Log filter set to: %s", filter);
         format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     } else {
         // Show current status
         bool is_enabled;
         char current_filter[256];
         
         polycall_core_error_t result = polycall_repl_get_log_inspection_status(
             core_ctx,
             g_repl_ctx,
             &is_enabled,
             current_filter,
             sizeof(current_filter)
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to get log inspection status");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         format_output(core_ctx, "Log Inspection Status", POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD);
         printf("\n\n");
         
         char status_str[32];
         snprintf(status_str, sizeof(status_str), "Enabled: %s", is_enabled ? "Yes" : "No");
         format_output(core_ctx, status_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         if (is_enabled && current_filter[0] != '\0') {
             char filter_str[256 + 16];
             snprintf(filter_str, sizeof(filter_str), "Current filter: %s", current_filter);
             format_output(core_ctx, filter_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
             printf("\n");
         }
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Settings command handler
  */
 static command_result_t handle_settings(int argc, char** argv, void* context) {
     polycall_core_context_t* core_ctx = (polycall_core_context_t*)context;
     
     // Parse options
     bool list_settings = true; // Default action
     bool reset_settings = false;
     const char* setting_key = NULL;
     const char* setting_value = NULL;
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "--list") == 0) {
             list_settings = true;
         } else if (strcmp(argv[i], "--reset") == 0) {
             reset_settings = true;
             list_settings = false; // Don't list if we're resetting
         } else if (strncmp(argv[i], "--set=", 6) == 0) {
             const char* key_value = argv[i] + 6;
             char* equals = strchr(key_value, '=');
             
             if (equals) {
                 *equals = '\0';
                 setting_key = key_value;
                 setting_value = equals + 1;
                 list_settings = false; // Don't list if we're setting
             } else {
                 format_error(core_ctx, "Invalid setting format. Use --set=key=value");
                 return COMMAND_ERROR_INVALID_ARGUMENTS;
             }
         }
     }
     
     // Initialize REPL if needed
     command_result_t cmd_result = init_repl_subsystem(core_ctx, NULL);
     if (cmd_result != COMMAND_SUCCESS) {
         return cmd_result;
     }
     
     // Process settings operations
     if (reset_settings) {
         polycall_core_error_t result = polycall_repl_reset_settings(
             core_ctx,
             g_repl_ctx
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to reset REPL settings");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         format_output(core_ctx, "REPL settings reset to defaults", POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     } else if (setting_key && setting_value) {
         polycall_core_error_t result = polycall_repl_set_setting(
             core_ctx,
             g_repl_ctx,
             setting_key,
             setting_value
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             char error_msg[256];
             snprintf(error_msg, sizeof(error_msg), "Failed to set %s=%s", setting_key, setting_value);
             format_error(core_ctx, error_msg);
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         char success_msg[256];
         snprintf(success_msg, sizeof(success_msg), "Setting updated: %s=%s", setting_key, setting_value);
         format_output(core_ctx, success_msg, POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     // List settings if requested
     if (list_settings) {
         polycall_repl_config_t current_config;
         polycall_core_error_t result = polycall_repl_get_config(
             core_ctx,
             g_repl_ctx,
             &current_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             format_error(core_ctx, "Failed to retrieve REPL settings");
             return COMMAND_ERROR_EXECUTION_FAILED;
         }
         
         format_output(core_ctx, "REPL Settings", POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD);
         printf("\n\n");
         
         // Display settings
         char setting_str[256];
         
         snprintf(setting_str, sizeof(setting_str), "History enabled: %s", 
                  current_config.enable_history ? "Yes" : "No");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Tab completion: %s", 
                  current_config.enable_completion ? "Enabled" : "Disabled");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Syntax highlighting: %s", 
                  current_config.enable_syntax_highlighting ? "Enabled" : "Disabled");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Log inspection: %s", 
                  current_config.enable_log_inspection ? "Enabled" : "Disabled");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Zero-trust inspection: %s", 
                  current_config.enable_zero_trust_inspection ? "Enabled" : "Disabled");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "History file: %s", 
                  current_config.history_file ? current_config.history_file : "Not set");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Prompt: %s", 
                  current_config.prompt ? current_config.prompt : "polycall> ");
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
         
         snprintf(setting_str, sizeof(setting_str), "Max history entries: %d", 
                  current_config.max_history_entries);
         format_output(core_ctx, setting_str, POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL);
         printf("\n");
     }
     
     return COMMAND_SUCCESS;
 }
 
 /**
  * @brief Register REPL commands
  */
 bool register_repl_commands(void) {
     return cli_register_command(&repl_command);
 }
