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
 * @file repl.c
 * @brief Enhancements to the REPL implementation to support Biafran UI/UX design system
 *
 * This file shows the key modifications needed to integrate the accessibility
 * module into the REPL component of LibPolyCall CLI.
 *
 * @copyright OBINexus Computing, 2025
 */

/**
 * Below are the key sections to modify in the existing repl.c file to
 * incorporate the Biafran UI/UX design system through the accessibility module.
 */

// Add new include for accessibility interface
#include "polycall/core/accessibility/accessibility_interface.h"

// Add accessibility context to REPL context
struct polycall_repl_context {
    polycall_core_context_t* core_ctx;
    command_history_t* history;
    char* history_file;
    char* prompt;
    bool enable_history;
    bool enable_completion;
    bool enable_syntax_highlighting;
    bool enable_log_inspection;
    bool enable_zero_trust_inspection;
    bool running;
    void* user_data;
    // New field for accessibility support
    polycall_accessibility_context_t* access_ctx;  // Accessibility context
};

/**
 * @brief Enhanced REPL configuration to include accessibility
 */
polycall_repl_config_t polycall_repl_default_config(void) {
    polycall_repl_config_t config;
    
    config.enable_history = true;
    config.enable_completion = true;
    config.enable_syntax_highlighting = true;
    config.enable_log_inspection = false;
    config.enable_zero_trust_inspection = false;
    config.history_file = NULL;
    config.prompt = NULL;
    config.max_history_entries = DEFAULT_MAX_HISTORY;
    
    // New field for Biafran UI/UX theme
    config.enable_accessibility = true;  // Enable accessibility by default
    config.accessibility_theme = POLYCALL_THEME_BIAFRAN;  // Use Biafran theme
    
    return config;
}

/**
 * @brief Initialize REPL context with accessibility support
 */
polycall_core_error_t polycall_repl_init(
    polycall_core_context_t* core_ctx,
    const polycall_repl_config_t* config,
    polycall_repl_context_t** repl_ctx
) {
    // Existing initialization code...
    
    // Initialize accessibility if enabled
    if (config->enable_accessibility) {
        polycall_accessibility_config_t access_config = polycall_accessibility_default_config();
        
        // Set theme from REPL config
        access_config.color_theme = config->accessibility_theme;
        
        // Initialize accessibility context
        polycall_core_error_t result = polycall_accessibility_init(
            core_ctx, 
            &access_config, 
            &(ctx->access_ctx)
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            // Handle error, free resources allocated so far
            if (ctx->history) {
                destroy_command_history(ctx->history);
            }
            if (ctx->history_file) {
                free(ctx->history_file);
            }
            if (ctx->prompt) {
                free(ctx->prompt);
            }
            free(ctx);
            return result;
        }
    } else {
        ctx->access_ctx = NULL;
    }
    
    // Rest of initialization...
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Enhanced print_prompt function
 */
static void print_prompt(polycall_repl_context_t* repl_ctx) {
    if (!repl_ctx || !repl_ctx->prompt) {
        return;
    }
    
    // Use accessibility formatting if available
    if (repl_ctx->access_ctx) {
        char formatted_prompt[256];
        if (polycall_accessibility_format_prompt(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                repl_ctx->prompt,
                formatted_prompt,
                sizeof(formatted_prompt))) {
            printf("%s", formatted_prompt);
            fflush(stdout);
            return;
        }
        // Fall back to default if formatting fails
    }
    
    // Default prompt display as fallback
    printf("%s%s%s", COLOR_BOLD, repl_ctx->prompt, COLOR_RESET);
    fflush(stdout);
}

/**
 * @brief Enhanced print_error function
 */
static void print_error(const char* message, polycall_repl_context_t* repl_ctx) {
    if (!message) {
        return;
    }
    
    // Use accessibility formatting if available
    if (repl_ctx && repl_ctx->access_ctx) {
        char formatted_error[512];
        if (polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                message,
                POLYCALL_TEXT_ERROR,
                POLYCALL_STYLE_NORMAL,
                formatted_error,
                sizeof(formatted_error))) {
            fprintf(stderr, "%s\n", formatted_error);
            return;
        }
        // Fall back to default if formatting fails
    }
    
    // Default error display as fallback
    fprintf(stderr, "%s%s%s\n", COLOR_RED, message, COLOR_RESET);
}

/**
 * @brief Enhanced print_success function
 */
static void print_success(const char* message, polycall_repl_context_t* repl_ctx) {
    if (!message) {
        return;
    }
    
    // Use accessibility formatting if available
    if (repl_ctx && repl_ctx->access_ctx) {
        char formatted_success[512];
        if (polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                message,
                POLYCALL_TEXT_SUCCESS,
                POLYCALL_STYLE_NORMAL,
                formatted_success,
                sizeof(formatted_success))) {
            printf("%s\n", formatted_success);
            return;
        }
        // Fall back to default if formatting fails
    }
    
    // Default success display as fallback
    printf("%s%s%s\n", COLOR_GREEN, message, COLOR_RESET);
}

/**
 * @brief Enhanced print_info function
 */
static void print_info(const char* message, polycall_repl_context_t* repl_ctx) {
    if (!message) {
        return;
    }
    
    // Use accessibility formatting if available
    if (repl_ctx && repl_ctx->access_ctx) {
        char formatted_info[512];
        if (polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                message,
                POLYCALL_TEXT_NORMAL,
                POLYCALL_STYLE_NORMAL,
                formatted_info,
                sizeof(formatted_info))) {
            printf("%s\n", formatted_info);
            return;
        }
        // Fall back to default if formatting fails
    }
    
    // Default info display as fallback
    printf("%s%s%s\n", COLOR_BLUE, message, COLOR_RESET);
}

/**
 * @brief Enhanced print_help function
 */
static void print_help(polycall_repl_context_t* repl_ctx) {
    int width = get_terminal_width();
    if (width <= 0) {
        width = 80;  // Default width
    }
    
    // Use accessibility formatting if available
    if (repl_ctx && repl_ctx->access_ctx) {
        char heading[256];
        polycall_accessibility_format_text(
            repl_ctx->core_ctx,
            repl_ctx->access_ctx,
            "LibPolyCall REPL Commands",
            POLYCALL_TEXT_HEADING,
            POLYCALL_STYLE_BOLD,
            heading,
            sizeof(heading)
        );
        
        printf("\n%s\n\n", heading);
        
        // Format and display built-in commands section
        char section_heading[256];
        polycall_accessibility_format_text(
            repl_ctx->core_ctx,
            repl_ctx->access_ctx,
            "Built-in Commands:",
            POLYCALL_TEXT_SUBCOMMAND,
            POLYCALL_STYLE_BOLD,
            section_heading,
            sizeof(section_heading)
        );
        
        printf("%s\n", section_heading);
        
        // Define command pairs (name + description)
        struct {
            const char* name;
            const char* desc;
        } built_in_commands[] = {
            {"help", "Display this help information"},
            {"exit, quit", "Exit the REPL"},
            {"inspect log [filter]", "Inspect logs with optional filter"},
            {"inspect security [target]", "Inspect security with optional target"}
        };
        
        // Format and display each built-in command
        for (size_t i = 0; i < sizeof(built_in_commands) / sizeof(built_in_commands[0]); i++) {
            char cmd_name[128];
            polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                built_in_commands[i].name,
                POLYCALL_TEXT_COMMAND,
                POLYCALL_STYLE_NORMAL,
                cmd_name,
                sizeof(cmd_name)
            );
            
            char cmd_desc[256];
            polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                built_in_commands[i].desc,
                POLYCALL_TEXT_NORMAL,
                POLYCALL_STYLE_NORMAL,
                cmd_desc,
                sizeof(cmd_desc)
            );
            
            printf("  %-20s %s\n", cmd_name, cmd_desc);
        }
        
        // Get and format registered commands
        command_t commands[64];  // Assume maximum 64 commands
        int count = cli_list_commands(commands, 64);
        
        if (count > 0) {
            char reg_heading[256];
            polycall_accessibility_format_text(
                repl_ctx->core_ctx,
                repl_ctx->access_ctx,
                "Registered Commands:",
                POLYCALL_TEXT_SUBCOMMAND,
                POLYCALL_STYLE_BOLD,
                reg_heading,
                sizeof(reg_heading)
            );
            
            printf("\n%s\n", reg_heading);
            
            for (int i = 0; i < count; i++) {
                char cmd_name[128];
                polycall_accessibility_format_text(
                    repl_ctx->core_ctx,
                    repl_ctx->access_ctx,
                    commands[i].name,
                    POLYCALL_TEXT_COMMAND,
                    POLYCALL_STYLE_NORMAL,
                    cmd_name,
                    sizeof(cmd_name)
                );
                
                char cmd_desc[256];
                polycall_accessibility_format_text(
                    repl_ctx->core_ctx,
                    repl_ctx->access_ctx,
                    commands[i].description,
                    POLYCALL_TEXT_NORMAL,
                    POLYCALL_STYLE_NORMAL,
                    cmd_desc,
                    sizeof(cmd_desc)
                );
                
                printf("  %-20s %s\n", cmd_name, cmd_desc);
            }
        }
        
        printf("\n");
        return;
    }
    
    // Default help display as fallback (original implementation)
    printf("\n%s", COLOR_BOLD);
    printf("LibPolyCall REPL Commands");
    printf("%s\n\n", COLOR_RESET);
    
    printf("%sBuilt-in Commands:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %-20s %s\n", "help", "Display this help information");
    printf("  %-20s %s\n", "exit, quit", "Exit the REPL");
    
    // Rest of the original implementation...
}

/**
 * @brief Run REPL with accessibility support
 */
polycall_core_error_t polycall_repl_run(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!core_ctx || !repl_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    char* line = NULL;
    repl_ctx->running = true;
    
    // Print welcome message with Biafran UI/UX theme if accessibility is enabled
    if (repl_ctx->access_ctx) {
        char welcome[512];
        polycall_accessibility_format_text(
            core_ctx,
            repl_ctx->access_ctx,
            "LibPolyCall Interactive REPL",
            POLYCALL_TEXT_HEADING,
            POLYCALL_STYLE_BOLD,
            welcome,
            sizeof(welcome)
        );
        
        char instructions[512];
        polycall_accessibility_format_text(
            core_ctx,
            repl_ctx->access_ctx,
            "Type 'help' for available commands, 'exit' to quit",
            POLYCALL_TEXT_NORMAL,
            POLYCALL_STYLE_NORMAL,
            instructions,
            sizeof(instructions)
        );
        
        printf("\n%s\n%s\n\n", welcome, instructions);
    } else {
        // Original welcome message as fallback
        printf("\n");
        printf("%s", COLOR_BOLD);
        printf("LibPolyCall Interactive REPL\n");
        printf("%s", COLOR_RESET);
        printf("Type 'help' for available commands, 'exit' to quit\n\n");
    }
    
    // Main REPL loop - mostly unchanged but using enhanced print_* functions
    while (repl_ctx->running) {
        // Read command line (call print_prompt with repl_ctx to use enhanced version)
        print_prompt(repl_ctx);
        line = read_line(repl_ctx);
        
        if (!line) {
            continue;  // Handle readline error or empty line
        }
        
        // Skip empty lines
        if (line[0] == '\0') {
            free(line);
            continue;
        }
        
        // Add to history if not empty
        if (repl_ctx->enable_history && line[0] != '\0') {
            add_to_history(repl_ctx->history, line);
        }
        
        // Process the command (pass repl_ctx for accessibility support)
        process_command(repl_ctx, line);
        
        free(line);
    }
    
    // Save history on exit if enabled
    if (repl_ctx->enable_history && repl_ctx->history_file) {
        save_history_to_file(repl_ctx->history, repl_ctx->history_file);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup REPL context with accessibility support
 */
void polycall_repl_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_repl_context_t* repl_ctx
) {
    if (!repl_ctx) {
        return;
    }
    
    // Original cleanup code...
    
    // Cleanup accessibility context if initialized
    if (repl_ctx->access_ctx) {
        polycall_accessibility_cleanup(core_ctx, repl_ctx->access_ctx);
        repl_ctx->access_ctx = NULL;
    }
    
    // Rest of the cleanup...
    
    // Free resources
    if (repl_ctx->history) {
        destroy_command_history(repl_ctx->history);
    }
    
    if (repl_ctx->history_file) {
        free(repl_ctx->history_file);
    }
    
    if (repl_ctx->prompt) {
        free(repl_ctx->prompt);
    }
    
    // Clear global reference
    if (g_repl_ctx == repl_ctx) {
        g_repl_ctx = NULL;
    }
    
    free(repl_ctx);
}
