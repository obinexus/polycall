/**
 * @file command.h
 * @brief Command system for LibPolyCall CLI
 *
 * Defines the architecture for a hierarchical command system
 * with support for subcommands, arguments, and flags.
 *
 * @copyright OBINexus Computing, 2025
 */

 #ifndef LIBPOLYCALL_COMMAND_H
 #define LIBPOLYCALL_COMMAND_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/accessibility/accessibility_colors.h"
 #include <stdbool.h>
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Command result codes
  */
 typedef enum {
     COMMAND_SUCCESS = 0,                  /**< Command completed successfully */
     COMMAND_ERROR_NOT_FOUND = 1,          /**< Command not found */
     COMMAND_ERROR_INVALID_ARGUMENTS = 2,  /**< Invalid arguments */
     COMMAND_ERROR_EXECUTION_FAILED = 3,   /**< Command execution failed */
     COMMAND_ERROR_PERMISSION_DENIED = 4,  /**< Permission denied */
     COMMAND_ERROR_CONTEXT_REQUIRED = 5,   /**< Command requires context */
     COMMAND_ERROR_INTERNAL = 6            /**< Internal error */
 } command_result_t;
 
 /**
  * @brief Command handler function type
  * 
  * @param argc Argument count
  * @param argv Argument vector
  * @param context Core context
  * @return command_result_t Command result
  */
 typedef command_result_t (*command_handler_t)(int argc, char** argv, void* context);
 
 /**
  * @brief Forward declaration for command structure
  */
 typedef struct command_s command_t;
 
 /**
  * @brief Subcommand structure
  */
 typedef struct {
     const char* name;                   /**< Subcommand name */
     const char* description;            /**< Subcommand description */
     const char* usage;                  /**< Subcommand usage */
     command_handler_t handler;          /**< Subcommand handler */
     bool requires_context;              /**< Whether requires context */
     polycall_text_type_t text_type;     /**< Text type for accessibility */
     const char* screen_reader_desc;     /**< Description for screen readers */
 } subcommand_t;
 
 /**
  * @brief Command structure
  */
 struct command_s {
     const char* name;                   /**< Command name */
     const char* description;            /**< Command description */
     const char* usage;                  /**< Command usage */
     command_handler_t handler;          /**< Command handler function */
     subcommand_t* subcommands;          /**< Array of subcommands */
     int subcommand_count;               /**< Number of subcommands */
     bool requires_context;              /**< Whether command requires context */
     polycall_text_type_t text_type;     /**< Text type for accessibility */
     const char* screen_reader_desc;     /**< Description for screen readers */
 };
 
 /**
  * @brief Flag structure for command arguments
  */
 typedef struct {
     const char* name;                   /**< Flag name */
     const char* short_name;             /**< Short flag name (single character) */
     const char* description;            /**< Flag description */
     bool requires_value;                /**< Whether flag requires a value */
     bool is_present;                    /**< Whether flag is present in arguments */
     char* value;                        /**< Flag value if provided */
 } command_flag_t;
 
 /**
  * @brief Process command-line arguments and dispatch to appropriate handler
  * 
  * @param argc Argument count
  * @param argv Argument vector
  * @param context Core context
  * @return int Exit code
  */
 int process_command_line(int argc, char** argv, polycall_core_context_t* context);
 
 /**
  * @brief Register a command
  * 
  * @param command Command to register
  * @return bool True if registered successfully, false otherwise
  */
 bool cli_register_command(const command_t* command);
 
 /**
  * @brief Execute a command
  * 
  * @param argc Argument count
  * @param argv Argument vector
  * @param context Core context
  * @return command_result_t Command result
  */
 command_result_t cli_execute_command(int argc, char** argv, void* context);
 
 /**
  * @brief Find a command by name
  * 
  * @param name Command name
  * @return const command_t* Command or NULL if not found
  */
 const command_t* find_command(const char* name);
 
 /**
  * @brief Find a subcommand by name
  * 
  * @param command Parent command
  * @param name Subcommand name
  * @return const subcommand_t* Subcommand or NULL if not found
  */
 const subcommand_t* find_subcommand(const command_t* command, const char* name);
 
 /**
  * @brief Get command help
  * 
  * @param name Command name
  * @return const command_t* Command or NULL if not found
  */
 const command_t* cli_get_command_help(const char* name);
 
 /**
  * @brief Show help for a command or general help
  * 
  * @param command_name Command name or NULL for general help
  * @param context Core context
  * @return int Exit code
  */
 int show_help(const char* command_name, polycall_core_context_t* context);
 
 /**
  * @brief Parse flags from arguments
  * 
  * @param argc Argument count
  * @param argv Argument vector
  * @param flags Flag array
  * @param flag_count Number of flags in array
  * @param remaining_args Array to store remaining arguments
  * @param remaining_count Number of remaining arguments
  * @return bool True if parsing successful, false otherwise
  */
 bool parse_flags(int argc, char** argv, command_flag_t* flags, int flag_count, 
                  char** remaining_args, int* remaining_count);
 
 /**
  * @brief Initialize the command system
  * 
  * @return bool True if initialization successful, false otherwise
  */
 bool cli_init_commands(void);
 
 /**
  * @brief Cleanup the command system
  */
 void cli_cleanup_commands(void);
 
 /**
  * @brief Register all built-in commands
  * 
  * @return bool True if registration successful, false otherwise
  */
 bool register_all_commands(void);
 
 /**
  * @brief Get the accessibility context from the core context
  * 
  * @param core_ctx Core context
  * @return polycall_accessibility_context_t* Accessibility context or NULL
  */
 polycall_accessibility_context_t* get_accessibility_context(polycall_core_context_t* core_ctx);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* LIBPOLYCALL_COMMAND_H */