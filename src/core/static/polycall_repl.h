/**
 * @file polycall_repl.h
 * @brief REPL (Read-Eval-Print Loop) for LibPolyCall configuration
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This module provides a dynamic, interactive configuration interface that allows
 * users to fine-tune, modify, and inspect system settings in real-time.
 */

 #ifndef POLYCALL_REPL_POLYCALL_REPL_H_H
 #define POLYCALL_REPL_POLYCALL_REPL_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_config.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdbool.h>
 #include <stddef.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Maximum command length
  */
 #define POLYCALL_REPL_POLYCALL_REPL_H_H
 
 /**
  * @brief Maximum history entries
  */
 #define POLYCALL_REPL_POLYCALL_REPL_H_H
 
 /**
  * @brief Command execution status
  */
 typedef enum {
     POLYCALL_REPL_SUCCESS = 0,
     POLYCALL_REPL_ERROR_INVALID_COMMAND,
     POLYCALL_REPL_ERROR_EXECUTION_FAILED,
     POLYCALL_REPL_ERROR_SYNTAX_ERROR,
     POLYCALL_REPL_ERROR_CONFIG_ERROR,
     POLYCALL_REPL_ERROR_PERMISSION_DENIED,
     POLYCALL_REPL_ERROR_UNKNOWN
 } polycall_repl_status_t;
 
 /**
  * @brief Command types
  */
 typedef enum {
     POLYCALL_REPL_CMD_GET = 0,      /**< Get a configuration value */
     POLYCALL_REPL_CMD_SET,          /**< Set a configuration value */
     POLYCALL_REPL_CMD_LIST,         /**< List configuration sections/keys */
     POLYCALL_REPL_CMD_SAVE,         /**< Save configuration to file */
     POLYCALL_REPL_CMD_LOAD,         /**< Load configuration from file */
     POLYCALL_REPL_CMD_RESET,        /**< Reset configuration to defaults */
     POLYCALL_REPL_CMD_HISTORY,      /**< Display command history */
     POLYCALL_REPL_CMD_HELP,         /**< Display help information */
     POLYCALL_REPL_CMD_EXIT,         /**< Exit REPL */
     POLYCALL_REPL_CMD_DOCTOR,       /**< Run doctor for validation */
     POLYCALL_REPL_CMD_IMPORT,       /**< Import configuration */
     POLYCALL_REPL_CMD_EXPORT,       /**< Export configuration */
     POLYCALL_REPL_CMD_DIFF,         /**< Show differences between configurations */
     POLYCALL_REPL_CMD_MERGE,        /**< Merge configurations */
     POLYCALL_REPL_CMD_EXEC,         /**< Execute script */
     POLYCALL_REPL_CMD_UNKNOWN       /**< Unknown command */
 } polycall_repl_command_type_t;
 
 /**
  * @brief Command structure
  */
 typedef struct {
     polycall_repl_command_type_t type;
     char args[POLYCALL_REPL_MAX_COMMAND_LENGTH];
     uint32_t arg_count;
     char* arg_values[16];  /* Maximum 16 arguments */
 } polycall_repl_command_t;
 
 /**
  * @brief REPL context opaque structure
  */
 typedef struct polycall_repl_context polycall_repl_context_t;
 
 /**
  * @brief Command handler function type
  */
 typedef polycall_repl_status_t (*polycall_repl_command_handler_t)(
     polycall_repl_context_t* repl_ctx,
     const polycall_repl_command_t* command,
     char* output,
     size_t output_size
 );
 
 /**
  * @brief REPL configuration
  */
 typedef struct {
     bool show_prompts;              /**< Show prompts in interactive mode */
     bool echo_commands;             /**< Echo commands in non-interactive mode */
     bool save_history;              /**< Save command history */
     const char* history_file;       /**< History file path */
     polycall_config_context_t* config_ctx; /**< Configuration context */
     uint32_t output_width;          /**< Output width in characters */
     bool color_output;              /**< Enable colored output */
     bool verbose;                   /**< Verbose output */
 } polycall_repl_config_t;
 
 /**
  * @brief Initialize REPL
  *
  * @param core_ctx Core context
  * @param repl_ctx Pointer to receive REPL context
  * @param config REPL configuration
  * @return Error code
  */
 polycall_core_error_t polycall_repl_init(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t** repl_ctx,
     const polycall_repl_config_t* config
 );
 
 /**
  * @brief Clean up REPL
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  */
 void polycall_repl_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 );
 
 /**
  * @brief Run REPL in interactive mode
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return Error code
  */
 polycall_core_error_t polycall_repl_run_interactive(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 );
 
 /**
  * @brief Execute a single command
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @param command Command string
  * @param output Output buffer
  * @param output_size Output buffer size
  * @return Command execution status
  */
 polycall_repl_status_t polycall_repl_execute_command(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx,
     const char* command,
     char* output,
     size_t output_size
 );
 
 /**
  * @brief Register a custom command handler
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @param command_type Command type
  * @param handler Handler function
  * @return Error code
  */
 polycall_core_error_t polycall_repl_register_handler(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx,
     polycall_repl_command_type_t command_type,
     polycall_repl_command_handler_t handler
 );
 
 /**
  * @brief Execute a script file
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @param script_path Script file path
  * @return Error code
  */
 polycall_core_error_t polycall_repl_execute_script(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx,
     const char* script_path
 );
 
 /**
  * @brief Get command history
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @param history Array to receive history entries
  * @param max_entries Maximum number of entries to retrieve
  * @param entry_count Pointer to receive entry count
  * @return Error code
  */
 polycall_core_error_t polycall_repl_get_history(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx,
     char history[][POLYCALL_REPL_MAX_COMMAND_LENGTH],
     uint32_t max_entries,
     uint32_t* entry_count
 );
 
 /**
  * @brief Clear command history
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return Error code
  */
 polycall_core_error_t polycall_repl_clear_history(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 );
 
 /**
  * @brief Get configuration context
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return Configuration context, or NULL on error
  */
 polycall_config_context_t* polycall_repl_get_config_context(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 );
 
 /**
  * @brief Set configuration context
  *
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @param config_ctx Configuration context
  * @return Error code
  */
 polycall_core_error_t polycall_repl_set_config_context(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx,
     polycall_config_context_t* config_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_REPL_POLYCALL_REPL_H_H */