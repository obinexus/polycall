/**
 * @file repl.h
 * @brief Interactive REPL for LibPolyCall with manual inspection capabilities
 * @author OBINexusComputing
 */

 #ifndef POLYCALL_REPL_H
 #define POLYCALL_REPL_H
 
 #include <stdbool.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    
 #include "polycall/core/polycall/context.h"
 #include "polycall/core/polycall/error.h"
 
 /**
  * @brief REPL context structure
  */
 typedef struct polycall_repl_context polycall_repl_context_t;
 
 /**
  * @brief REPL configuration structure
  */
 typedef struct {
     bool enable_history;                 /**< Enable command history */
     bool enable_completion;              /**< Enable tab completion */
     bool enable_syntax_highlighting;     /**< Enable syntax highlighting */
     bool enable_log_inspection;          /**< Enable log inspection mode */
     bool enable_zero_trust_inspection;   /**< Enable zero-trust inspection */
     char* history_file;                  /**< Path to history file */
     char* prompt;                        /**< REPL prompt string */
     int max_history_entries;             /**< Maximum history entries */
 } polycall_repl_config_t;
 
 /**
  * @brief Create default REPL configuration
  * 
  * @return Default REPL configuration
  */
 polycall_repl_config_t polycall_repl_default_config(void);
 
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
 );
 
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
 );
 
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
 );
 
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
 );
 
 /**
  * @brief Cleanup REPL context
  * 
  * @param core_ctx Core context
  * @param repl_ctx REPL context to cleanup
  */
 void polycall_repl_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_repl_context_t* repl_ctx
 );
 
 #endif /* POLYCALL_REPL_H */