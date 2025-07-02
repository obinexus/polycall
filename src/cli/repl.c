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
#include "polycall/cli/repl.h"

 * @file repl.c
 * @brief Interactive REPL implementation for LibPolyCall CLI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

 #include "polycall/cli/repl.h"
 #include "polycall/cli/command.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/cli/providers/cli_container.h"
#include "polycall/cli/common/command_registry.h"
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <signal.h>
 #include <ctype.h>
 
 #ifdef _WIN32
 #include <windows.h>
 #else
 #include <unistd.h>
 #include <termios.h>
 #include <sys/ioctl.h>
 #endif
 
 // Maximum input line length
 #define MAX_LINE_LENGTH 4096
 
 // Maximum number of history entries
 #define DEFAULT_MAX_HISTORY 100
 // Default prompt
 #define MAX_ARGS 64
#define PROMPT "polycall> "

 // REPL color codes (ANSI escape sequences)
 #define COLOR_RESET       "\033[0m"
 #define COLOR_BLACK       "\033[30m"
 #define COLOR_RED         "\033[31m"
 #define COLOR_GREEN       "\033[32m"
 #define COLOR_YELLOW      "\033[33m"
 #define COLOR_BLUE        "\033[34m"
 #define COLOR_MAGENTA     "\033[35m"
 #define COLOR_CYAN        "\033[36m"
 #define COLOR_WHITE       "\033[37m"
 #define COLOR_BOLD        "\033[1m"
 #define COLOR_UNDERLINE   "\033[4m"
 
 // History entry structure
 typedef struct history_entry {
     char* command;
     struct history_entry* prev;
     struct history_entry* next;
 } history_entry_t;
 
 // History structure
 typedef struct {
     history_entry_t* head;
     history_entry_t* tail;
     history_entry_t* current;
     int count;
     int max_entries;
 } command_history_t;
 
 // REPL context structure
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
 };
 
 // Forward declarations
 static command_history_t* create_command_history(int max_entries);
 static void destroy_command_history(command_history_t* history);
 static void add_to_history(command_history_t* history, const char* command);
 static char* get_previous_history(command_history_t* history);
 static char* get_next_history(command_history_t* history);
 static void load_history_from_file(command_history_t* history, const char* filename);
 static void save_history_to_file(command_history_t* history, const char* filename);
 static char** tokenize_command(const char* command, int* argc);
 static void free_tokens(char** tokens, int count);
 static char* read_line(const char* prompt, command_history_t* history, bool enable_completion);
 static char* get_completion(const char* prefix);
 static void handle_signal(int sig);
 static void print_prompt(const char* prompt);
 static void print_error(const char* message);
 static void print_success(const char* message);
 static void print_info(const char* message);
 static void process_command(polycall_repl_context_t* repl_ctx, const char* command);
 static void inspect_log(polycall_repl_context_t* repl_ctx, const char* filter);
 static void inspect_security(polycall_repl_context_t* repl_ctx, const char* target);
 static void print_help(void);
 static int get_terminal_width(void);
 
 // Global REPL context for signal handling
 static polycall_repl_context_t* g_repl_ctx = NULL;
// Parse command line
static int parse_command_line(char* line, char** argv) {
    if (!line || !argv) {
        return 0;
    }
    
    int argc = 0;
    char* token = strtok(line, " \t\n\r");
    
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    
    return argc;
}

// Run REPL
int run_repl(polycall_cli_container_t* container, void* context) {
    if (!container) {
        return 1;
    }
    
    char* line = NULL;
    char* args[MAX_ARGS];
    int running = 1;
    
    // Initialize readline
    using_history();
    
    printf("LibPolyCall CLI - Interactive Mode\n");
    printf("Type 'help' for available commands or 'exit' to quit\n");
    
    while (running) {
        // Read line
        line = readline(PROMPT);
        if (!line) {
            break;
        }
        
        // Add to history if non-empty
        if (line[0] != '\0') {
            add_history(line);
        }
        
        // Parse line
        int argc = parse_command_line(line, args);
        
        // Skip if empty
        if (argc == 0) {
            free(line);
            continue;
        }
        
        // Process command
        if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
            running = 0;
        } else if (strcmp(args[0], "help") == 0) {
            if (argc == 1) {
                // Show modules
                char** modules = NULL;
                size_t module_count = 0;
                
                if (polycall_command_registry_list_modules(container->command_registry, 
                                                         &modules, &module_count) == 0) {
                    printf("Available modules:\n");
                    for (size_t i = 0; i < module_count; i++) {
                        printf("  %s\n", modules[i]);
                    }
                    free(modules);
                }
                
                printf("\nUse 'help <module>' to see commands for a specific module\n");
            } else {
                // Show commands for module
                polycall_command_t* commands = NULL;
                size_t command_count = 0;
                
                if (polycall_command_registry_list(container->command_registry, args[1], 
                                                 &commands, &command_count) == 0) {
                    printf("Commands for module '%s':\n", args[1]);
                    for (size_t i = 0; i < command_count; i++) {
                        printf("  %-15s - %s\n", commands[i].name, commands[i].description);
                        printf("     Usage: %s\n", commands[i].usage);
                    }
                } else {
                    printf("Module '%s' not found\n", args[1]);
                }
            }
        } else if (argc >= 2) {
            // Execute command
            const char* module = args[0];
            const char* command = args[1];
            
            container->execute_command(container, module, command, argc - 2, &args[2]);
        } else {
            printf("Invalid command. Type 'help' for available commands.\n");
        }
        
        free(line);
    }
    
    return 0;
}


 /**
  * @brief Create default REPL configuration
  * 
  * @return Default REPL configuration
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
     
     return config;
 }
 
 /**
  * @brief Initialize REPL context
  * 
  * @param core_ctx Core context
  * @param config REPL configuration
  * @param repl_ctx Pointer to receive REPL context
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_repl_init(
     polycall_core_context_t* core_ctx,
     const polycall_repl_config_t* config,
     polycall_repl_context_t** repl_ctx
 ) {
     if (!core_ctx || !config || !repl_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_repl_context_t* ctx = (polycall_repl_context_t*)
         malloc(sizeof(polycall_repl_context_t));
     if (!ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize structure
     memset(ctx, 0, sizeof(polycall_repl_context_t));
     
     // Set core context
     ctx->core_ctx = core_ctx;
     
     // Copy configuration values
     ctx->enable_history = config->enable_history;
     ctx->enable_completion = config->enable_completion;
     ctx->enable_syntax_highlighting = config->enable_syntax_highlighting;
     ctx->enable_log_inspection = config->enable_log_inspection;
     ctx->enable_zero_trust_inspection = config->enable_zero_trust_inspection;
     
     // Set up command history
     if (ctx->enable_history) {
         int max_entries = (config->max_history_entries > 0) ? 
             config->max_history_entries : DEFAULT_MAX_HISTORY;
         ctx->history = create_command_history(max_entries);
         if (!ctx->history) {
             free(ctx);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy history file path if provided
         if (config->history_file) {
             ctx->history_file = strdup(config->history_file);
             load_history_from_file(ctx->history, ctx->history_file);
         }
     }
     
     // Set prompt
     if (config->prompt) {
         ctx->prompt = strdup(config->prompt);
     } else {
         ctx->prompt = strdup(PROMPT);
     }
     if (!ctx->prompt) {
         if (ctx->history_file) {
             free(ctx->history_file);
         }
         if (ctx->history) {
             destroy_command_history(ctx->history);
         }
         free(ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Set global reference for signal handling
     g_repl_ctx = ctx;
     
     // Set up signal handlers
     signal(SIGINT, handle_signal);
     
     *repl_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Start REPL loop
  * 
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return polycall_core_error_t Error code
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
     
     // Print welcome message
     printf("\n");
     printf("%s", COLOR_BOLD);
     printf("LibPolyCall Interactive REPL\n");
     printf("%s", COLOR_RESET);
     printf("Type 'help' for available commands, 'exit' to quit\n\n");
     
     while (repl_ctx->running) {
         // Read command line
         line = read_line(repl_ctx->prompt, repl_ctx->history, repl_ctx->enable_completion);
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
         
         // Process the command
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
  * @brief Enable log inspection mode
  * 
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_repl_enable_log_inspection(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 ) {
     if (!core_ctx || !repl_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     repl_ctx->enable_log_inspection = true;
     printf("%sLog inspection mode enabled.%s\n", COLOR_GREEN, COLOR_RESET);
     printf("Use 'inspect log [filter]' command for log inspection.\n");
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Enable zero-trust inspection mode
  * 
  * @param core_ctx Core context
  * @param repl_ctx REPL context
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_repl_enable_zero_trust_inspection(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 ) {
     if (!core_ctx || !repl_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     repl_ctx->enable_zero_trust_inspection = true;
     printf("%sZero-trust inspection mode enabled.%s\n", COLOR_GREEN, COLOR_RESET);
     printf("Use 'inspect security [target]' command for security inspection.\n");
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup REPL context
  * 
  * @param core_ctx Core context
  * @param repl_ctx REPL context to cleanup
  */
 void polycall_repl_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 ) {
     if (!repl_ctx) {
         return;
     }
     
     // Save history to file if enabled
     if (repl_ctx->enable_history && repl_ctx->history_file) {
         save_history_to_file(repl_ctx->history, repl_ctx->history_file);
     }
     
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
 
 /**
  * @brief Create command history structure
  * 
  * @param max_entries Maximum number of history entries
  * @return command_history_t* History structure or NULL on failure
  */
 static command_history_t* create_command_history(int max_entries) {
     command_history_t* history = (command_history_t*)malloc(sizeof(command_history_t));
     if (!history) {
         return NULL;
     }
     
     history->head = NULL;
     history->tail = NULL;
     history->current = NULL;
     history->count = 0;
     history->max_entries = max_entries;
     
     return history;
 }
 
 /**
  * @brief Destroy command history structure
  * 
  * @param history History structure to destroy
  */
 static void destroy_command_history(command_history_t* history) {
     if (!history) {
         return;
     }
     
     // Free all entries
     history_entry_t* entry = history->head;
     history_entry_t* next;
     
     while (entry) {
         next = entry->next;
         if (entry->command) {
             free(entry->command);
         }
         free(entry);
         entry = next;
     }
     
     free(history);
 }
 
 /**
  * @brief Add command to history
  * 
  * @param history History structure
  * @param command Command to add
  */
 static void add_to_history(command_history_t* history, const char* command) {
     if (!history || !command || command[0] == '\0') {
         return;
     }
     
     // Skip if command is identical to the last entry
     if (history->tail && strcmp(history->tail->command, command) == 0) {
         history->current = NULL;  // Reset current position
         return;
     }
     
     // Create new entry
     history_entry_t* entry = (history_entry_t*)malloc(sizeof(history_entry_t));
     if (!entry) {
         return;
     }
     
     entry->command = strdup(command);
     if (!entry->command) {
         free(entry);
         return;
     }
     
     entry->next = NULL;
     
     // Add to list
     if (!history->head) {
         // First entry
         entry->prev = NULL;
         history->head = entry;
         history->tail = entry;
     } else {
         // Append to end
         entry->prev = history->tail;
         history->tail->next = entry;
         history->tail = entry;
     }
     
     history->count++;
     
     // Remove oldest entry if exceeding maximum
     if (history->count > history->max_entries && history->head) {
         history_entry_t* oldest = history->head;
         history->head = oldest->next;
         
         if (history->head) {
             history->head->prev = NULL;
         } else {
             history->tail = NULL;
         }
         
         free(oldest->command);
         free(oldest);
         history->count--;
     }
     
     // Reset current position
     history->current = NULL;
 }
 
 /**
  * @brief Get previous command from history
  * 
  * @param history History structure
  * @return char* Previous command or NULL if at beginning
  */
 static char* get_previous_history(command_history_t* history) {
     if (!history || !history->head) {
         return NULL;
     }
     
     if (!history->current) {
         // Start from end
         history->current = history->tail;
     } else if (history->current->prev) {
         // Move to previous
         history->current = history->current->prev;
     } else {
         // Already at beginning
         return history->current->command;
     }
     
     return history->current ? history->current->command : NULL;
 }
 
 /**
  * @brief Get next command from history
  * 
  * @param history History structure
  * @return char* Next command or NULL if at end
  */
 static char* get_next_history(command_history_t* history) {
     if (!history || !history->current) {
         return NULL;
     }
     
     history->current = history->current->next;
     return history->current ? history->current->command : NULL;
 }
 
 /**
  * @brief Load command history from file
  * 
  * @param history History structure
  * @param filename History file path
  */
 static void load_history_from_file(command_history_t* history, const char* filename) {
     if (!history || !filename) {
         return;
     }
     
     FILE* file = fopen(filename, "r");
     if (!file) {
         return;
     }
     
     char line[MAX_LINE_LENGTH];
     while (fgets(line, sizeof(line), file)) {
         // Remove trailing newline
         size_t len = strlen(line);
         if (len > 0 && line[len - 1] == '\n') {
             line[len - 1] = '\0';
         }
         
         // Add to history
         add_to_history(history, line);
     }
     
     fclose(file);
 }
 
 /**
  * @brief Save command history to file
  * 
  * @param history History structure
  * @param filename History file path
  */
 static void save_history_to_file(command_history_t* history, const char* filename) {
     if (!history || !filename) {
         return;
     }
     
     FILE* file = fopen(filename, "w");
     if (!file) {
         return;
     }
     
     // Save all entries
     history_entry_t* entry = history->head;
     while (entry) {
         fprintf(file, "%s\n", entry->command);
         entry = entry->next;
     }
     
     fclose(file);
 }
 
 /**
  * @brief Tokenize command line
  * 
  * @param command Command line
  * @param argc Pointer to receive argument count
  * @return char** Array of arguments or NULL on error
  */
 static char** tokenize_command(const char* command, int* argc) {
     if (!command || !argc) {
         return NULL;
     }
     
     *argc = 0;
     
     // Count tokens
     int max_tokens = 1; // Start with 1 for the command itself
     const char* ptr = command;
     bool in_quotes = false;
     
     while (*ptr) {
         if (*ptr == '"') {
             in_quotes = !in_quotes;
         } else if (isspace((unsigned char)*ptr) && !in_quotes) {
             max_tokens++;
             // Skip multiple spaces
             while (isspace((unsigned char)*(ptr + 1))) {
                 ptr++;
             }
         }
         ptr++;
     }
     
     // Allocate token array
     char** tokens = (char**)malloc((max_tokens + 1) * sizeof(char*));
     if (!tokens) {
         return NULL;
     }
     
     // Parse tokens
     const char* start = command;
     int token_idx = 0;
     ptr = command;
     in_quotes = false;
     
     while (*ptr) {
         if (*ptr == '"') {
             in_quotes = !in_quotes;
         } else if (isspace((unsigned char)*ptr) && !in_quotes) {
             // End of token
             if (ptr > start) {
                 size_t token_len = ptr - start;
                 tokens[token_idx] = (char*)malloc(token_len + 1);
                 if (!tokens[token_idx]) {
                     // Cleanup on failure
                     for (int i = 0; i < token_idx; i++) {
                         free(tokens[i]);
                     }
                     free(tokens);
                     return NULL;
                 }
                 
                 // Copy token without quotes
                 char* dest = tokens[token_idx];
                 const char* src = start;
                 bool skip_quotes = false;
                 
                 for (size_t i = 0; i < token_len; i++) {
                     if (*src == '"') {
                         skip_quotes = !skip_quotes;
                         src++;
                     } else {
                         *dest++ = *src++;
                     }
                 }
                 *dest = '\0';
                 
                 token_idx++;
             }
             
             // Skip multiple spaces
             while (isspace((unsigned char)*(ptr + 1))) {
                 ptr++;
             }
             
             start = ptr + 1;
         }
         ptr++;
     }
     
     // Handle last token
     if (ptr > start) {
         size_t token_len = ptr - start;
         tokens[token_idx] = (char*)malloc(token_len + 1);
         if (!tokens[token_idx]) {
             // Cleanup on failure
             for (int i = 0; i < token_idx; i++) {
                 free(tokens[i]);
             }
             free(tokens);
             return NULL;
         }
         
         // Copy token without quotes
         char* dest = tokens[token_idx];
         const char* src = start;
         bool skip_quotes = false;
         
         for (size_t i = 0; i < token_len; i++) {
             if (*src == '"') {
                 skip_quotes = !skip_quotes;
                 src++;
             } else {
                 *dest++ = *src++;
             }
         }
         *dest = '\0';
         
         token_idx++;
     }
     
     // Null-terminate array
     tokens[token_idx] = NULL;
     *argc = token_idx;
     
     return tokens;
 }
 
 /**
  * @brief Free tokenized command
  * 
  * @param tokens Token array
  * @param count Token count
  */
 static void free_tokens(char** tokens, int count) {
     if (!tokens) {
         return;
     }
     
     for (int i = 0; i < count; i++) {
         free(tokens[i]);
     }
     
     free(tokens);
 }
 
 /**
  * @brief Basic line editing implementation
  * 
  * @param prompt Prompt string
  * @param history Command history
  * @param enable_completion Whether to enable tab completion
  * @return char* Line read or NULL on error/EOF
  */
 static char* read_line(const char* prompt, command_history_t* history, bool enable_completion) {
     print_prompt(prompt);
     
     char* buffer = (char*)malloc(MAX_LINE_LENGTH);
     if (!buffer) {
         return NULL;
     }
     
     if (fgets(buffer, MAX_LINE_LENGTH, stdin) == NULL) {
         free(buffer);
         return NULL;
     }
     
     // Remove trailing newline
     size_t len = strlen(buffer);
     if (len > 0 && buffer[len - 1] == '\n') {
         buffer[len - 1] = '\0';
     }
     
     return buffer;
 }
 
 /**
  * @brief Get completion for command
  * 
  * @param prefix Command prefix
  * @return char* Completion or NULL if none
  */
 static char* get_completion(const char* prefix) {
     // This is a simplified implementation that would need to be expanded
     // to provide actual tab completion for commands
     
     // TODO: Implement tab completion based on registered commands
     return NULL;
 }
 
 /**
  * @brief Signal handler for SIGINT
  * 
  * @param sig Signal number
  */
 static void handle_signal(int sig) {
     if (sig == SIGINT) {
         printf("\n");
         if (g_repl_ctx) {
             print_prompt(g_repl_ctx->prompt);
             fflush(stdout);
         }
     }
 }
 
 /**
  * @brief Print prompt
  * 
  * @param prompt Prompt string
  */
 static void print_prompt(const char* prompt) {
     printf("%s%s%s", COLOR_BOLD, prompt, COLOR_RESET);
     fflush(stdout);
 }
 
 /**
  * @brief Print error message
  * 
  * @param message Error message
  */
 static void print_error(const char* message) {
     printf("%s%s%s\n", COLOR_RED, message, COLOR_RESET);
 }
 
 /**
  * @brief Print success message
  * 
  * @param message Success message
  */
 static void print_success(const char* message) {
     printf("%s%s%s\n", COLOR_GREEN, message, COLOR_RESET);
 }
 
 /**
  * @brief Print info message
  * 
  * @param message Info message
  */
 static void print_info(const char* message) {
     printf("%s%s%s\n", COLOR_BLUE, message, COLOR_RESET);
 }
 
 /**
  * @brief Process command
  * 
  * @param repl_ctx REPL context
  * @param command Command to process
  */
/**
 * @brief Process command with audio feedback
 */
static void process_command(polycall_repl_context_t* repl_ctx, const char* command) {
    if (!repl_ctx || !command) {
        return;
    }
    
    // Get accessibility context from repl context
    polycall_accessibility_context_t* access_ctx = repl_ctx->access_ctx;
    
    // Play prompt notification if accessibility is enabled
    if (access_ctx && access_ctx->config.enable_audio_notifications) {
        polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_PROMPT);
    }
    
    // Special commands
    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
        // Exit REPL
        repl_ctx->running = false;
        return;
    } else if (strncmp(command, "help", 4) == 0) {
        // Show help
        print_help();
        if (access_ctx && access_ctx->config.enable_audio_notifications) {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_INFO);
        }
        return;
    } else if (strncmp(command, "inspect log", 11) == 0) {
        // Log inspection
        if (!repl_ctx->enable_log_inspection) {
            print_error("Log inspection mode is not enabled.", repl_ctx);
            if (access_ctx && access_ctx->config.enable_audio_notifications) {
                polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_ERROR);
            }
            return;
        }
        
        const char* filter = command + 11;
        while (isspace((unsigned char)*filter)) {
            filter++;
        }
        
        inspect_log(repl_ctx, filter);
        if (access_ctx && access_ctx->config.enable_audio_notifications) {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_SUCCESS);
        }
        return;
    } else if (strncmp(command, "inspect security", 16) == 0) {
        // Security inspection
        if (!repl_ctx->enable_zero_trust_inspection) {
            print_error("Zero-trust inspection mode is not enabled.", repl_ctx);
            if (access_ctx && access_ctx->config.enable_audio_notifications) {
                polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_ERROR);
            }
            return;
        }
        
        const char* target = command + 16;
        while (isspace((unsigned char)*target)) {
            target++;
        }
        
        inspect_security(repl_ctx, target);
        if (access_ctx && access_ctx->config.enable_audio_notifications) {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_SUCCESS);
        }
        return;
    }
    
    // Tokenize command for standard command processing
    int argc = 0;
    char** argv = tokenize_command(command, &argc);
    if (!argv || argc == 0) {
        if (access_ctx && access_ctx->config.enable_audio_notifications) {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_ERROR);
        }
        return;
    }
    
    // Execute command
    command_result_t result = cli_execute_command(argc, argv, repl_ctx->core_ctx);
    
    // Handle result with audio feedback
    if (access_ctx && access_ctx->config.enable_audio_notifications) {
        if (result == COMMAND_SUCCESS) {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_SUCCESS);
        } else {
            polycall_accessibility_play_notification(POLYCALL_AUDIO_NOTIFICATION_ERROR);
        }
    }
    
    // Handle result with visual feedback
    switch (result) {
        case COMMAND_SUCCESS:
            break;
            
        case COMMAND_ERROR_INVALID_ARGUMENTS:
            print_error("Invalid command arguments. Type 'help' for usage information.", repl_ctx);
            break;
            
        case COMMAND_ERROR_EXECUTION_FAILED:
            print_error("Command execution failed.", repl_ctx);
            break;
            
        case COMMAND_ERROR_NOT_FOUND:
            print_error("Command not found. Type 'help' to see available commands.", repl_ctx);
            break;
            
        case COMMAND_ERROR_PERMISSION_DENIED:
            print_error("Permission denied for this command.", repl_ctx);
            break;
            
        default:
            print_error("Unknown error occurred.", repl_ctx);
            break;
    }
    
    // Free tokens
    free_tokens(argv, argc);
}
 
 /**
  * @brief Inspect log
  * 
  * @param repl_ctx REPL context
  * @param filter Log filter
  */
 static void inspect_log(polycall_repl_context_t* repl_ctx, const char* filter) {
     print_info("Log inspection functionality is not fully implemented.");
     printf("Filter: %s\n", filter && filter[0] ? filter : "(none)");
     
     // TODO: Implement log inspection functionality
     print_info("This would display logs filtered by the specified criteria.");
 }
 
 /**
  * @brief Inspect security
  * 
  * @param repl_ctx REPL context
  * @param target Security target
  */
 static void inspect_security(polycall_repl_context_t* repl_ctx, const char* target) {
     print_info("Security inspection functionality is not fully implemented.");
     printf("Target: %s\n", target && target[0] ? target : "(none)");
     
     // TODO: Implement security inspection functionality
     print_info("This would display security information for the specified target.");
 }
 
 /**
  * @brief Print help information
  */
 static void print_help(void) {
     int width = get_terminal_width();
     if (width <= 0) {
         width = 80;  // Default width
     }
     
     // Print header
     printf("\n%s", COLOR_BOLD);
     printf("LibPolyCall REPL Commands");
     printf("%s\n\n", COLOR_RESET);
     
     // Print built-in commands
     printf("%sBuilt-in Commands:%s\n", COLOR_BOLD, COLOR_RESET);
     printf("  %-20s %s\n", "help", "Display this help information");
     printf("  %-20s %s\n", "exit, quit", "Exit the REPL");
     
     if (g_repl_ctx && g_repl_ctx->enable_log_inspection) {
         printf("  %-20s %s\n", "inspect log [filter]", "Inspect logs with optional filter");
     }
     
     if (g_repl_ctx && g_repl_ctx->enable_zero_trust_inspection) {
         printf("  %-20s %s\n", "inspect security [target]", "Inspect security with optional target");
     }
     
     printf("\n");
     
     // Print registered commands
     command_t commands[64];  // Assume maximum 64 commands
     int count = cli_list_commands(commands, 64);
     
     if (count > 0) {
         printf("%sRegistered Commands:%s\n", COLOR_BOLD, COLOR_RESET);
         for (int i = 0; i < count; i++) {
             printf("  %-20s %s\n", commands[i].name, commands[i].description);
         }
     }
     
     printf("\n");
 }
 
 /**
  * @brief Get terminal width
  * 
  * @return int Terminal width in columns or 0 if unknown
  */
 static int get_terminal_width(void) {
 #ifdef _WIN32
     CONSOLE_SCREEN_BUFFER_INFO csbi;
     if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
         return csbi.srWindow.Right - csbi.srWindow.Left + 1;
     }
     return 0;
 #else
     struct winsize ws;
     if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
         return ws.ws_col;
     }
     return 0;
 #endif
 }
