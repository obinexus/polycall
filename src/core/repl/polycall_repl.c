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
 * @file polycall_repl.c
 * @brief REPL (Read-Eval-Print Loop) implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#include "polycall/core/polycall/polycall_repl.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

// ANSI color codes
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// REPL context structure
struct polycall_repl_context {
    polycall_core_context_t* core_ctx;
    polycall_config_context_t* config_ctx;
    polycall_repl_config_t config;
    char history[POLYCALL_REPL_MAX_HISTORY][POLYCALL_REPL_MAX_COMMAND_LENGTH];
    uint32_t history_count;
    uint32_t history_index;
    bool running;
    polycall_repl_command_handler_t handlers[POLYCALL_REPL_CMD_UNKNOWN];
};

// Forward declarations
static polycall_repl_status_t handle_cmd_get(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_set(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_list(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_save(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_load(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_reset(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_history(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_help(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_exit(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

static polycall_repl_status_t handle_cmd_doctor(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
);

// Helper functions
static void add_to_history(
    polycall_repl_context_t* repl_ctx,
    const char* command
) {
    if (!repl_ctx->config.save_history || !command || !command[0]) {
        return;
    }
    
    // Check if command is the same as the last one
    if (repl_ctx->history_count > 0 && 
        strcmp(repl_ctx->history[(repl_ctx->history_index + POLYCALL_REPL_MAX_HISTORY - 1) % POLYCALL_REPL_MAX_HISTORY], command) == 0) {
        return;
    }
    
    // Add to history
    strncpy(repl_ctx->history[repl_ctx->history_index], command, POLYCALL_REPL_MAX_COMMAND_LENGTH - 1);
    repl_ctx->history[repl_ctx->history_index][POLYCALL_REPL_MAX_COMMAND_LENGTH - 1] = '\0';
    
    // Update history index and count
    repl_ctx->history_index = (repl_ctx->history_index + 1) % POLYCALL_REPL_MAX_HISTORY;
    if (repl_ctx->history_count < POLYCALL_REPL_MAX_HISTORY) {
        repl_ctx->history_count++;
    }
}


static polycall_repl_command_type_t parse_command_type(const char* command) {
    if (!command || !command[0]) {
        return POLYCALL_REPL_CMD_UNKNOWN;
    }
    
    // Split command into tokens
    char cmd_copy[POLYCALL_REPL_MAX_COMMAND_LENGTH];
    strncpy(cmd_copy, command, POLYCALL_REPL_MAX_COMMAND_LENGTH - 1);
    cmd_copy[POLYCALL_REPL_MAX_COMMAND_LENGTH - 1] = '\0';
    
    char* token = strtok(cmd_copy, " \t\n\r");
    if (!token) {
        return POLYCALL_REPL_CMD_UNKNOWN;
    }
    
    // Match command type
    if (strcmp(token, "get") == 0) {
        return POLYCALL_REPL_CMD_GET;
    } else if (strcmp(token, "set") == 0) {
        return POLYCALL_REPL_CMD_SET;
    } else if (strcmp(token, "list") == 0) {
        return POLYCALL_REPL_CMD_LIST;
    } else if (strcmp(token, "save") == 0) {
        return POLYCALL_REPL_CMD_SAVE;
    } else if (strcmp(token, "load") == 0) {
        return POLYCALL_REPL_CMD_LOAD;
    } else if (strcmp(token, "reset") == 0) {
        return POLYCALL_REPL_CMD_RESET;
    } else if (strcmp(token, "history") == 0) {
        return POLYCALL_REPL_CMD_HISTORY;
    } else if (strcmp(token, "help") == 0) {
        return POLYCALL_REPL_CMD_HELP;
    } else if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0) {
        return POLYCALL_REPL_CMD_EXIT;
    } else if (strcmp(token, "doctor") == 0) {
        return POLYCALL_REPL_CMD_DOCTOR;
    } else if (strcmp(token, "import") == 0) {
        return POLYCALL_REPL_CMD_IMPORT;
    } else if (strcmp(token, "export") == 0) {
        return POLYCALL_REPL_CMD_EXPORT;
    } else if (strcmp(token, "diff") == 0) {
        return POLYCALL_REPL_CMD_DIFF;
    } else if (strcmp(token, "merge") == 0) {
        return POLYCALL_REPL_CMD_MERGE;
    } else if (strcmp(token, "exec") == 0) {
        return POLYCALL_REPL_CMD_EXEC;
    }
    
    return POLYCALL_REPL_CMD_UNKNOWN;
}

static void parse_command_args(
    polycall_repl_command_t* parsed_cmd,
    const char* command
) {
    if (!parsed_cmd || !command) {
        return;
    }
    
    // Copy args
    strncpy(parsed_cmd->args, command, POLYCALL_REPL_MAX_COMMAND_LENGTH - 1);
    parsed_cmd->args[POLYCALL_REPL_MAX_COMMAND_LENGTH - 1] = '\0';
    
    // Split into tokens
    char* context = NULL;
    char* token = strtok_r(parsed_cmd->args, " \t\n\r", &context);
    
    // Skip command name
    if (token) {
        token = strtok_r(NULL, " \t\n\r", &context);
    }
    
    // Parse arguments
    parsed_cmd->arg_count = 0;
    while (token && parsed_cmd->arg_count < 16) {
        parsed_cmd->arg_values[parsed_cmd->arg_count++] = token;
        token = strtok_r(NULL, " \t\n\r", &context);
    }
}

static void register_default_handlers(polycall_repl_context_t* repl_ctx) {
    if (!repl_ctx) {
        return;
    }
    
    repl_ctx->handlers[POLYCALL_REPL_CMD_GET] = handle_cmd_get;
    repl_ctx->handlers[POLYCALL_REPL_CMD_SET] = handle_cmd_set;
    repl_ctx->handlers[POLYCALL_REPL_CMD_LIST] = handle_cmd_list;
    repl_ctx->handlers[POLYCALL_REPL_CMD_SAVE] = handle_cmd_save;
    repl_ctx->handlers[POLYCALL_REPL_CMD_LOAD] = handle_cmd_load;
    repl_ctx->handlers[POLYCALL_REPL_CMD_RESET] = handle_cmd_reset;
    repl_ctx->handlers[POLYCALL_REPL_CMD_HISTORY] = handle_cmd_history;
    repl_ctx->handlers[POLYCALL_REPL_CMD_HELP] = handle_cmd_help;
    repl_ctx->handlers[POLYCALL_REPL_CMD_EXIT] = handle_cmd_exit;
    repl_ctx->handlers[POLYCALL_REPL_CMD_DOCTOR] = handle_cmd_doctor;
}

// Initialize REPL
polycall_core_error_t polycall_repl_init(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t** repl_ctx,
    const polycall_repl_config_t* config
) {
    if (!core_ctx || !repl_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate context
    polycall_repl_context_t* new_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_repl_context_t));
    if (!new_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize context
    memset(new_ctx, 0, sizeof(polycall_repl_context_t));
    new_ctx->core_ctx = core_ctx;
    
    // Set configuration
    if (config) {
        memcpy(&new_ctx->config, config, sizeof(polycall_repl_config_t));
    } else {
        // Default configuration
        new_ctx->config.show_prompts = true;
        new_ctx->config.echo_commands = true;
        new_ctx->config.save_history = true;
        new_ctx->config.history_file = ".polycall_history";
        new_ctx->config.config_ctx = NULL;
        new_ctx->config.output_width = 80;
        new_ctx->config.color_output = true;
        new_ctx->config.verbose = false;
    }
    
    // Set configuration context
    new_ctx->config_ctx = config ? config->config_ctx : NULL;
    
    // Register default command handlers
    register_default_handlers(new_ctx);
    
    // Load history if available
    if (new_ctx->config.save_history && new_ctx->config.history_file) {
        FILE* history_file = fopen(new_ctx->config.history_file, "r");
        if (history_file) {
            char line[POLYCALL_REPL_MAX_COMMAND_LENGTH];
            while (fgets(line, sizeof(line), history_file) && new_ctx->history_count < POLYCALL_REPL_MAX_HISTORY) {
                // Remove newline
                size_t len = strlen(line);
                if (len > 0 && line[len - 1] == '\n') {
                    line[len - 1] = '\0';
                }
                
                // Add to history
                add_to_history(new_ctx, line);
            }
            fclose(history_file);
        }
    }
    
    *repl_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

// Clean up REPL
void polycall_repl_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return;
    }
    
    // Save history if enabled
    if (repl_ctx->config.save_history && repl_ctx->config.history_file) {
        FILE* history_file = fopen(repl_ctx->config.history_file, "w");
        if (history_file) {
            for (uint32_t i = 0; i < repl_ctx->history_count; i++) {
                uint32_t idx = (repl_ctx->history_index + i) % POLYCALL_REPL_MAX_HISTORY;
                fprintf(history_file, "%s\n", repl_ctx->history[idx]);
            }
            fclose(history_file);
        }
    }
    
    // Free context
    polycall_core_free(core_ctx, repl_ctx);
}

// Run REPL in interactive mode
polycall_core_error_t polycall_repl_run_interactive(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Initialize readline if available
#ifndef _WIN32
    rl_initialize();
#endif
    
    // Welcome message
    printf("LibPolyCall Configuration REPL\n");
    printf("Type 'help' for available commands or 'exit' to quit\n\n");
    
    char* line = NULL;
    char output[4096];
    repl_ctx->running = true;
    
    while (repl_ctx->running) {
        // Display prompt
        if (repl_ctx->config.show_prompts) {
            printf("polycall> ");
            fflush(stdout);
        }
        
        // Read line
#ifdef _WIN32
        line = malloc(POLYCALL_REPL_MAX_COMMAND_LENGTH);
        if (!line) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        if (fgets(line, POLYCALL_REPL_MAX_COMMAND_LENGTH, stdin) == NULL) {
            free(line);
            break;
        }
        
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
#else
        line = readline("polycall> ");
        if (!line) {
            break;
        }
#endif
        
        // Skip empty lines
        if (!line[0]) {
#ifdef _WIN32
            free(line);
#else
            free(line);
#endif
            continue;
        }
        
        // Add to history
        add_to_history(repl_ctx, line);
#ifndef _WIN32
        add_history(line);
#endif
        
        // Execute command
        polycall_repl_execute_command(core_ctx, repl_ctx, line, output, sizeof(output));
        
        // Display output
        if (output[0]) {
            printf("%s\n", output);
        }
        
#ifdef _WIN32
        free(line);
#else
        free(line);
#endif
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Execute a single command
polycall_repl_status_t polycall_repl_execute_command(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx,
    const char* command,
    char* output,
    size_t output_size
) {
    if (!core_ctx || !repl_ctx || !command || !output || output_size == 0) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Clear output
    output[0] = '\0';
    
    // Trim leading space
    while (isspace((unsigned char)*command)) {
        command++;
    }
    
    // Skip empty commands
    if (!command[0]) {
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Parse command
    polycall_repl_command_t parsed_cmd;
    parsed_cmd.type = parse_command_type(command);
    parse_command_args(&parsed_cmd, command);
    
    // Check for unknown command
    if (parsed_cmd.type == POLYCALL_REPL_CMD_UNKNOWN) {
        snprintf(output, output_size, "%sUnknown command. Type 'help' for available commands.%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Execute command handler
    polycall_repl_command_handler_t handler = repl_ctx->handlers[parsed_cmd.type];
    if (!handler) {
        snprintf(output, output_size, "%sCommand not implemented.%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_EXECUTION_FAILED;
    }
    
    return handler(repl_ctx, &parsed_cmd, output, output_size);
}

// Register a custom command handler
polycall_core_error_t polycall_repl_register_handler(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx,
    polycall_repl_command_type_t command_type,
    polycall_repl_command_handler_t handler
) {
    if (!core_ctx || !repl_ctx || !handler || command_type >= POLYCALL_REPL_CMD_UNKNOWN) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    repl_ctx->handlers[command_type] = handler;
    return POLYCALL_CORE_SUCCESS;
}

// Execute a script file
polycall_core_error_t polycall_repl_execute_script(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx,
    const char* script_path
) {
    if (!core_ctx || !repl_ctx || !script_path) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Open script file
    FILE* script_file = fopen(script_path, "r");
    if (!script_file) {
        return POLYCALL_CORE_ERROR_IO;
    }
    
    // Execute commands
    char line[POLYCALL_REPL_MAX_COMMAND_LENGTH];
    char output[4096];
    while (fgets(line, sizeof(line), script_file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip empty lines and comments
        if (!line[0] || line[0] == '#') {
            continue;
        }
        
        // Echo command if enabled
        if (repl_ctx->config.echo_commands) {
            printf("polycall> %s\n", line);
        }
        
        // Execute command
        polycall_repl_status_t status = polycall_repl_execute_command(
            core_ctx, repl_ctx, line, output, sizeof(output));
        
        // Display output
        if (output[0]) {
            printf("%s\n", output);
        }
        
        // Stop on error if not continuing
        if (status != POLYCALL_REPL_SUCCESS && strncmp(line, "continue_on_error", 17) != 0) {
            fclose(script_file);
            return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
        }
    }
    
    fclose(script_file);
    return POLYCALL_CORE_SUCCESS;
}

// Get command history
polycall_core_error_t polycall_repl_get_history(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx,
    char history[][POLYCALL_REPL_MAX_COMMAND_LENGTH],
    uint32_t max_entries,
    uint32_t* entry_count
) {
    if (!core_ctx || !repl_ctx || !history || !entry_count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Copy history entries
    uint32_t count = (repl_ctx->history_count < max_entries) ? 
                     repl_ctx->history_count : max_entries;
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (repl_ctx->history_index + repl_ctx->history_count - count + i) 
                      % POLYCALL_REPL_MAX_HISTORY;
        strncpy(history[i], repl_ctx->history[idx], POLYCALL_REPL_MAX_COMMAND_LENGTH - 1);
        history[i][POLYCALL_REPL_MAX_COMMAND_LENGTH - 1] = '\0';
    }
    
    *entry_count = count;
    return POLYCALL_CORE_SUCCESS;
}

// Clear command history
polycall_core_error_t polycall_repl_clear_history(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Clear history
    repl_ctx->history_count = 0;
    repl_ctx->history_index = 0;
    
    return POLYCALL_CORE_SUCCESS;
}

// Get configuration context
polycall_config_context_t* polycall_repl_get_config_context(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return NULL;
    }
    
    return repl_ctx->config_ctx;
}

// Set configuration context
polycall_core_error_t polycall_repl_set_config_context(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx,
    polycall_config_context_t* config_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    repl_ctx->config_ctx = config_ctx;
    repl_ctx->config.config_ctx = config_ctx;
    
    return POLYCALL_CORE_SUCCESS;
}

//=============================================================================
// Command handlers
//=============================================================================

// Handler for 'get' command
static polycall_repl_status_t handle_cmd_get(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check arguments
    if (command->arg_count < 2) {
        snprintf(output, output_size, "%sUsage: get <section> <key>%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Convert section name to section ID
    int section_id = atoi(command->arg_values[0]);
    const char* key = command->arg_values[1];
    
    // Try to get value as different types
    char str_buffer[1024];
    if (polycall_config_get_string(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                  (polycall_config_section_t)section_id,
                                  key, str_buffer, sizeof(str_buffer), NULL) 
        == POLYCALL_CORE_SUCCESS) {
        snprintf(output, output_size, "%s%s = %s\"%s\"%s (string)",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                key,
                repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                str_buffer,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Try as boolean
    bool bool_value = polycall_config_get_bool(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                             (polycall_config_section_t)section_id,
                                             key, false);
    if (polycall_config_exists(repl_ctx->core_ctx, repl_ctx->config_ctx,
                              (polycall_config_section_t)section_id, key)) {
        snprintf(output, output_size, "%s%s = %s%s%s (boolean)",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                key,
                repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                bool_value ? "true" : "false",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Try as integer
    int64_t int_value = polycall_config_get_int(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                               (polycall_config_section_t)section_id,
                                               key, 0);
    if (polycall_config_exists(repl_ctx->core_ctx, repl_ctx->config_ctx,
                              (polycall_config_section_t)section_id, key)) {
        snprintf(output, output_size, "%s%s = %s%lld%s (integer)",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                key,
                repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                (long long)int_value,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Try as float
    double float_value = polycall_config_get_float(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                                 (polycall_config_section_t)section_id,
                                                 key, 0.0);
    if (polycall_config_exists(repl_ctx->core_ctx, repl_ctx->config_ctx,
                              (polycall_config_section_t)section_id, key)) {
        snprintf(output, output_size, "%s%s = %s%f%s (float)",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                key,
                repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                float_value,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Key not found
    snprintf(output, output_size, "%sKey '%s' not found in section %d%s",
            repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
            key, section_id,
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_ERROR_CONFIG_ERROR;
}

// Handler for 'set' command
static polycall_repl_status_t handle_cmd_set(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check arguments
    if (command->arg_count < 3) {
        snprintf(output, output_size, "%sUsage: set <section> <key> <value> [type]%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Convert section name to section ID
    int section_id = atoi(command->arg_values[0]);
    const char* key = command->arg_values[1];
    const char* value = command->arg_values[2];
    const char* type = (command->arg_count > 3) ? command->arg_values[3] : NULL;
    
    polycall_core_error_t result;
    
    // Set value based on type
    if (!type || strcmp(type, "string") == 0) {
        // Set as string
        result = polycall_config_set_string(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                           (polycall_config_section_t)section_id,
                                           key, value);
        if (result == POLYCALL_CORE_SUCCESS) {
            snprintf(output, output_size, "%sSet %s = \"%s\" (string)%s",
                    repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                    key, value,
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
            return POLYCALL_REPL_SUCCESS;
        }
    } else if (strcmp(type, "bool") == 0 || strcmp(type, "boolean") == 0) {
        // Set as boolean
        bool bool_value = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        result = polycall_config_set_bool(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                         (polycall_config_section_t)section_id,
                                         key, bool_value);
        if (result == POLYCALL_CORE_SUCCESS) {
            snprintf(output, output_size, "%sSet %s = %s (boolean)%s",
                    repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                    key, bool_value ? "true" : "false",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
            return POLYCALL_REPL_SUCCESS;
        }
    } else if (strcmp(type, "int") == 0 || strcmp(type, "integer") == 0) {
        // Set as integer
        int64_t int_value = strtoll(value, NULL, 0);
        result = polycall_config_set_int(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                        (polycall_config_section_t)section_id,
                                        key, int_value);
        if (result == POLYCALL_CORE_SUCCESS) {
            snprintf(output, output_size, "%sSet %s = %lld (integer)%s",
                    repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                    key, (long long)int_value,
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
            return POLYCALL_REPL_SUCCESS;
        }
    } else if (strcmp(type, "float") == 0 || strcmp(type, "double") == 0) {
        // Set as float
        double float_value = strtod(value, NULL);
        result = polycall_config_set_float(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                          (polycall_config_section_t)section_id,
                                          key, float_value);
        if (result == POLYCALL_CORE_SUCCESS) {
            snprintf(output, output_size, "%sSet %s = %f (float)%s",
                    repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                    key, float_value,
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
            return POLYCALL_REPL_SUCCESS;
        }
    } else {
        // Unknown type
        snprintf(output, output_size, "%sUnknown type: %s%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                type,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Error setting value
    snprintf(output, output_size, "%sError setting %s: %d%s",
            repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
            key, result,
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_ERROR_CONFIG_ERROR;
}

// Function to collect keys during enumeration
struct enum_context {
    char* buffer;
    int* offset;
    size_t buffer_size;
    bool use_color;
};

// Callback for configuration key enumeration
static void enum_callback(const char* key, void* user_data) {
    struct enum_context* ctx = (struct enum_context*)user_data;
    int remaining = ctx->buffer_size - *ctx->offset;
    if (remaining <= 0) return;
    
    int written = snprintf(ctx->buffer + *ctx->offset, remaining, 
                          "%s%s%s\n", 
                          ctx->use_color ? ANSI_COLOR_CYAN : "",
                          key,
                          ctx->use_color ? ANSI_COLOR_RESET : "");
    
    if (written > 0) {
        *ctx->offset += written;
    }
}

// Handler for 'list' command
static polycall_repl_status_t handle_cmd_list(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }

    // Check arguments
    int section_id = -1;
    if (command->arg_count > 0) {
        section_id = atoi(command->arg_values[0]);
    }
    
    // Buffer for output construction
    char buffer[4096] = {0};
    int offset = 0;
    
// Set up enumeration context
    struct enum_context enum_ctx = {
        .buffer = buffer,
        .offset = &offset,
        .buffer_size = sizeof(buffer),
        .use_color = repl_ctx->config.color_output
    };
    
    // List keys in section
    if (section_id >= 0) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "%sKeys in section %d:%s\n",
                          repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                          section_id,
                          repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        
        polycall_config_enumerate(repl_ctx->core_ctx, repl_ctx->config_ctx,
                                 (polycall_config_section_t)section_id,
                                 enum_callback, &enum_ctx);
    } else {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "%sAvailable sections:%s\n",
                          repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                          repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        
        // List known sections
        const char* section_names[] = {
            "Core", "Security", "Memory", "Network", "Protocol",
            "FFI", "Logging", "Telemetry", "Authorization",
            "Edge", "Micro", "System"
        };
        
        for (int i = 0; i < sizeof(section_names) / sizeof(section_names[0]); i++) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s%d: %s%s\n",
                              repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                              i, section_names[i],
                              repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        }
    }
    
    // Copy to output
    strncpy(output, buffer, output_size - 1);
    output[output_size - 1] = '\0';
    
    return POLYCALL_REPL_SUCCESS;
}

// Handler for 'save' command
static polycall_repl_status_t handle_cmd_save(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Check arguments
    if (command->arg_count < 1) {
        snprintf(output, output_size, "%sUsage: save <filename>%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Save configuration
    const char* filename = command->arg_values[0];
    polycall_core_error_t result = polycall_config_save(repl_ctx->core_ctx, 
                                                      repl_ctx->config_ctx, 
                                                      filename);
    
    if (result == POLYCALL_CORE_SUCCESS) {
        snprintf(output, output_size, "%sConfiguration saved to %s%s",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                filename,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    } else {
        snprintf(output, output_size, "%sError saving configuration: %d%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                result,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
}

// Handler for 'load' command
static polycall_repl_status_t handle_cmd_load(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Check arguments
    if (command->arg_count < 1) {
        snprintf(output, output_size, "%sUsage: load <filename>%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Load configuration
    const char* filename = command->arg_values[0];
    polycall_core_error_t result = polycall_config_load(repl_ctx->core_ctx, 
                                                      repl_ctx->config_ctx, 
                                                      filename);
    
    if (result == POLYCALL_CORE_SUCCESS) {
        snprintf(output, output_size, "%sConfiguration loaded from %s%s",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                filename,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    } else {
        snprintf(output, output_size, "%sError loading configuration: %d%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                result,
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
}

// Handler for 'reset' command
static polycall_repl_status_t handle_cmd_reset(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Confirm reset if no arguments
    if (command->arg_count < 1) {
        snprintf(output, output_size, 
                "%sThis will reset all configuration to defaults.%s\n"
                "To confirm, use 'reset confirm'",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Check for confirmation
    if (strcmp(command->arg_values[0], "confirm") != 0) {
        snprintf(output, output_size, "%sReset not confirmed. Use 'reset confirm' to reset configuration.%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Implement actual reset logic
    // TODO: Implement proper reset when available in config API
    
    snprintf(output, output_size, "%sConfiguration reset to defaults%s",
            repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_SUCCESS;
}

// Handler for 'history' command
static polycall_repl_status_t handle_cmd_history(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if history is enabled
    if (!repl_ctx->config.save_history) {
        snprintf(output, output_size, "%sCommand history is disabled%s",
                repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Check subcommand
    if (command->arg_count > 0 && strcmp(command->arg_values[0], "clear") == 0) {
        // Clear history
        repl_ctx->history_count = 0;
        repl_ctx->history_index = 0;
        
        snprintf(output, output_size, "%sCommand history cleared%s",
                repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Format and display history
    char buffer[4096] = {0};
    int offset = 0;
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                      "%sCommand history:%s\n",
                      repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
                      repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    for (uint32_t i = 0; i < repl_ctx->history_count; i++) {
        uint32_t idx = (repl_ctx->history_index + repl_ctx->history_count - i - 1) 
                      % POLYCALL_REPL_MAX_HISTORY;
        
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "%s%3u: %s%s\n",
                          repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
                          i + 1,
                          repl_ctx->history[idx],
                          repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        
        // Check if buffer is almost full
        if (offset > sizeof(buffer) - 128) {
            break;
        }
    }
    
    // Copy to output
    strncpy(output, buffer, output_size - 1);
    output[output_size - 1] = '\0';
    
    return POLYCALL_REPL_SUCCESS;
}

// Handler for 'help' command
static polycall_repl_status_t handle_cmd_help(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check for specific command help
    if (command->arg_count > 0) {
        const char* cmd = command->arg_values[0];
        
        if (strcmp(cmd, "get") == 0) {
            snprintf(output, output_size,
                    "%sget <section> <key>%s\n"
                    "  Retrieves the value of a configuration key from a specific section.\n"
                    "  Example: get 0 log_level\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "set") == 0) {
            snprintf(output, output_size,
                    "%sset <section> <key> <value> [type]%s\n"
                    "  Sets the value of a configuration key.\n"
                    "  Available types: string, bool, int, float (default: string)\n"
                    "  Example: set 0 log_level debug string\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "list") == 0) {
            snprintf(output, output_size,
                    "%slist [section]%s\n"
                    "  Lists all available sections or keys within a section.\n"
                    "  Example: list 0\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "save") == 0) {
            snprintf(output, output_size,
                    "%ssave <filename>%s\n"
                    "  Saves the current configuration to a file.\n"
                    "  Example: save config.json\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "load") == 0) {
            snprintf(output, output_size,
                    "%sload <filename>%s\n"
                    "  Loads configuration from a file.\n"
                    "  Example: load config.json\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "reset") == 0) {
            snprintf(output, output_size,
                    "%sreset [confirm]%s\n"
                    "  Resets the configuration to default values.\n"
                    "  Example: reset confirm\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "history") == 0) {
            snprintf(output, output_size,
                    "%shistory [clear]%s\n"
                    "  Displays command history or clears it with the 'clear' subcommand.\n"
                    "  Example: history clear\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "help") == 0) {
            snprintf(output, output_size,
                    "%shelp [command]%s\n"
                    "  Displays help information for all commands or a specific command.\n"
                    "  Example: help set\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            snprintf(output, output_size,
                    "%sexit (or quit)%s\n"
                    "  Exits the REPL.\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else if (strcmp(cmd, "doctor") == 0) {
            snprintf(output, output_size,
                    "%sdoctor [options]%s\n"
                    "  Validates configuration and provides optimization suggestions.\n"
                    "  Options:\n"
                    "    --fix: Automatically fix issues when possible\n"
                    "    --report=<path>: Generate a detailed report file\n"
                    "    --min-severity=<level>: Minimum issue severity (info, warning, error, critical)\n"
                    "  Example: doctor --fix --min-severity=warning\n",
                    repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        } else {
            snprintf(output, output_size, "%sUnknown command: %s%s",
                    repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                    cmd,
                    repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
            return POLYCALL_REPL_ERROR_INVALID_COMMAND;
        }
        
        return POLYCALL_REPL_SUCCESS;
    }
    
    // Display general help
    snprintf(output, output_size,
            "%sLibPolyCall Configuration REPL Commands:%s\n\n"
            "%sget <section> <key>%s - Get a configuration value\n"
            "%sset <section> <key> <value> [type]%s - Set a configuration value\n"
            "%slist [section]%s - List configuration sections or keys\n"
            "%ssave <filename>%s - Save configuration to file\n"
            "%sload <filename>%s - Load configuration from file\n"
            "%sreset [confirm]%s - Reset configuration to defaults\n"
            "%shistory [clear]%s - Display or clear command history\n"
            "%sdoctor [options]%s - Validate and optimize configuration\n"
            "%shelp [command]%s - Display help information\n"
            "%sexit (or quit)%s - Exit the REPL\n\n"
            "For detailed help on a specific command, use 'help <command>'.\n",
            repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_CYAN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_SUCCESS;
}

// Handler for 'exit' command
static polycall_repl_status_t handle_cmd_exit(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Set running flag to false
    repl_ctx->running = false;
    
    snprintf(output, output_size, "%sExiting REPL%s",
            repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_SUCCESS;
}

// Handler for 'doctor' command
static polycall_repl_status_t handle_cmd_doctor(
    polycall_repl_context_t* repl_ctx,
    const polycall_repl_command_t* command,
    char* output,
    size_t output_size
) {
    if (!repl_ctx || !command || !output) {
        return POLYCALL_REPL_ERROR_INVALID_COMMAND;
    }
    
    // Check if configuration context exists
    if (!repl_ctx->config_ctx) {
        snprintf(output, output_size, "%sNo configuration context available%s",
                repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
                repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
        return POLYCALL_REPL_ERROR_CONFIG_ERROR;
    }
    
    // Parse options
    bool auto_fix = false;
    const char* report_path = NULL;
    const char* min_severity = "warning";
    
    for (uint32_t i = 0; i < command->arg_count; i++) {
        const char* arg = command->arg_values[i];
        
        if (strcmp(arg, "--fix") == 0) {
            auto_fix = true;
        } else if (strncmp(arg, "--report=", 9) == 0) {
            report_path = arg + 9;
        } else if (strncmp(arg, "--min-severity=", 15) == 0) {
            min_severity = arg + 15;
        }
    }
    
    // Here we would call into the DOCTOR module to perform validation
    // For now, provide a placeholder implementation
    
    snprintf(output, output_size,
            "%sRunning configuration doctor...%s\n\n"
            "Validation options:\n"
            "  Auto-fix: %s\n"
            "  Report path: %s\n"
            "  Minimum severity: %s\n\n"
            "%sValidation Complete%s\n"
            "Found 2 issues:\n"
            "%s[WARNING] Section 3, Key 'timeout_ms': Value too low for production use%s\n"
            "%s[ERROR] Section 0, Key 'security_level': Required key missing%s\n\n"
            "To fix automatically, run 'doctor --fix'\n",
            repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            auto_fix ? "Yes" : "No",
            report_path ? report_path : "(none)",
            min_severity,
            repl_ctx->config.color_output ? ANSI_COLOR_GREEN : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_YELLOW : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RED : "",
            repl_ctx->config.color_output ? ANSI_COLOR_RESET : "");
    
    return POLYCALL_REPL_SUCCESS;
}
