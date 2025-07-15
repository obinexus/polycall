/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

// polycall_repl_accessibility.c
#include "polycall/core/accessibility/accessibility_interface.h"
#include "polycall/core/polycall/polycall_repl.h"
#include <string.h>

// Add accessibility context to REPL context
polycall_core_error_t polycall_repl_set_accessibility_context(
    polycall_core_context_t *core_ctx, polycall_repl_context_t *repl_ctx,
    polycall_accessibility_context_t *access_ctx) {
  if (!core_ctx || !repl_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Store accessibility context in REPL context
  // Note: This would require extending the REPL context structure
  repl_ctx->access_ctx = access_ctx;

  return POLYCALL_CORE_SUCCESS;
}

// Run enhanced REPL with accessibility features
polycall_core_error_t
polycall_repl_run_enhanced(polycall_core_context_t *core_ctx,
                           polycall_repl_context_t *repl_ctx) {
  if (!core_ctx || !repl_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Check for accessibility context
  if (!repl_ctx->access_ctx) {
    // Run standard REPL if accessibility not available
    return polycall_repl_run_interactive(core_ctx, repl_ctx);
  }

  // Display welcome message with accessibility formatting
  char welcome_msg[256];
  polycall_accessibility_format_text(core_ctx, repl_ctx->access_ctx,
                                     "LibPolyCall Configuration REPL",
                                     POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD,
                                     welcome_msg, sizeof(welcome_msg));

  char help_msg[256];
  polycall_accessibility_format_text(
      core_ctx, repl_ctx->access_ctx,
      "Type 'help' for available commands or 'exit' to quit",
      POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL, help_msg, sizeof(help_msg));

  printf("%s\n%s\n\n", welcome_msg, help_msg);

  // Run interactive REPL with enhanced display
  // This would use accessibility formatting for prompts and output
  // Here we just call the standard REPL for demonstration
  return polycall_repl_run_interactive(core_ctx, repl_ctx);
}

// Format output with accessibility support
static void format_repl_output(polycall_core_context_t *core_ctx,
                               polycall_repl_context_t *repl_ctx,
                               const char *command_output,
                               polycall_repl_status_t status,
                               char *formatted_output, size_t output_size) {
  if (!core_ctx || !repl_ctx || !command_output || !formatted_output ||
      !repl_ctx->access_ctx) {
    // Pass through unformatted if any parameters invalid
    if (command_output && formatted_output && output_size > 0) {
      strncpy(formatted_output, command_output, output_size - 1);
      formatted_output[output_size - 1] = '\0';
    }
    return;
  }

  // Determine text style based on command status
  polycall_text_type_t text_type;
  switch (status) {
  case POLYCALL_REPL_SUCCESS:
    text_type = POLYCALL_TEXT_SUCCESS;
    break;
  case POLYCALL_REPL_ERROR_INVALID_COMMAND:
  case POLYCALL_REPL_ERROR_SYNTAX_ERROR:
    text_type = POLYCALL_TEXT_ERROR;
    break;
  case POLYCALL_REPL_ERROR_CONFIG_ERROR:
  case POLYCALL_REPL_ERROR_PERMISSION_DENIED:
  case POLYCALL_REPL_ERROR_EXECUTION_FAILED:
    text_type = POLYCALL_TEXT_WARNING;
    break;
  default:
    text_type = POLYCALL_TEXT_NORMAL;
    break;
  }

  // Format output with accessibility support
  polycall_accessibility_format_text(
      core_ctx, repl_ctx->access_ctx, command_output, text_type,
      POLYCALL_STYLE_NORMAL, formatted_output, output_size);
}

// Override for execute command to use accessibility formatting
polycall_repl_status_t polycall_repl_execute_command_enhanced(
    polycall_core_context_t *core_ctx, polycall_repl_context_t *repl_ctx,
    const char *command, char *output, size_t output_size) {
  if (!core_ctx || !repl_ctx || !command || !output || output_size == 0) {
    return POLYCALL_REPL_ERROR_INVALID_COMMAND;
  }

  // Execute command using standard function
  char raw_output[4096] = {0};
  polycall_repl_status_t status = polycall_repl_execute_command(
      core_ctx, repl_ctx, command, raw_output, sizeof(raw_output));

  // Format output with accessibility support if available
  if (repl_ctx->access_ctx) {
    format_repl_output(core_ctx, repl_ctx, raw_output, status, output,
                       output_size);
  } else {
    // Use raw output if accessibility not available
    strncpy(output, raw_output, output_size - 1);
    output[output_size - 1] = '\0';
  }

  return status;
}
