/**
 * @file accessibility_interface.h
 * @brief Main accessibility interface for LibPolyCall CLI
 *
 * This file defines the main accessibility interface that integrates
 * the Biafran UI/UX design system and provides accessibility utilities
 * for the command-line interface.
 *
 * @copyright OBINexus Computing, 2025
 */

 #ifndef POLYCALL_ACCESSIBILITY_ACCESSIBILITY_INTERFACE_H_H
 #define POLYCALL_ACCESSIBILITY_ACCESSIBILITY_INTERFACE_H_H
 
 #include <stdbool.h>
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/accessibility/accessibility_colors.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Accessibility context structure
  */
 typedef struct polycall_accessibility_context polycall_accessibility_context_t;
 
 /**
  * @brief Accessibility configuration structure
  */
 typedef struct {
     polycall_color_theme_t color_theme;      /**< Color theme to use */
     bool enable_high_contrast;               /**< Enable high contrast mode */
     bool enable_screen_reader_support;       /**< Enable screen reader support */
     bool enable_keyboard_shortcuts;          /**< Enable keyboard shortcuts */
     bool enable_motion_reduction;            /**< Reduce motion for animations */
     bool auto_detect_preferences;            /**< Auto-detect accessibility preferences */
     int min_font_size;                       /**< Minimum font size (if applicable) */
     int focus_indicator_width;               /**< Width of focus indicators in pixels */
     char* custom_stylesheet;                 /**< Path to custom stylesheet (if applicable) */
 } polycall_accessibility_config_t;
 
 /**
  * @brief Create default accessibility configuration
  *
  * @return polycall_accessibility_config_t Default configuration
  */
 polycall_accessibility_config_t polycall_accessibility_default_config(void);
 
 /**
  * @brief Initialize accessibility context
  *
  * @param core_ctx Core context
  * @param config Accessibility configuration
  * @param access_ctx Pointer to receive accessibility context
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_accessibility_init(
     polycall_core_context_t* core_ctx,
     const polycall_accessibility_config_t* config,
     polycall_accessibility_context_t** access_ctx
 );
 
 /**
  * @brief Cleanup accessibility context
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context to cleanup
  */
 void polycall_accessibility_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 );
 
 /**
  * @brief Format text for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param text Input text
  * @param type Text type
  * @param style Text style
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_text(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* text,
     polycall_text_type_t type,
     polycall_text_style_t style,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format command help for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param command Command name
  * @param description Command description
  * @param usage Command usage
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_command_help(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* command,
     const char* description,
     const char* usage,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format error message for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param error_code Error code
  * @param error_message Error message
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_error(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     int error_code,
     const char* error_message,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format success message for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param message Success message
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_success(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* message,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format progress display for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param progress Progress value (0.0 to 1.0)
  * @param label Progress label (optional)
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_progress(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     float progress,
     const char* label,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format table for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param headers Table headers
  * @param header_count Number of headers
  * @param data Table data
  * @param row_count Number of rows
  * @param col_count Number of columns
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_table(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char** headers,
     size_t header_count,
     const char*** data,
     size_t row_count,
     size_t col_count,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Get screen reader text for GUI element
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param element_type Element type
  * @param element_name Element name
  * @param element_state Element state
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_get_screen_reader_text(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* element_type,
     const char* element_name,
     const char* element_state,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Format REPL prompt for accessibility
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @param prompt Prompt text
  * @param buffer Output buffer
  * @param buffer_size Buffer size
  * @return bool True if successful, false otherwise
  */
 bool polycall_accessibility_format_prompt(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* prompt,
     char* buffer,
     size_t buffer_size
 );
 
 /**
  * @brief Check if screen reader is active
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @return bool True if screen reader is active, false otherwise
  */
 bool polycall_accessibility_is_screen_reader_active(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 );
 
 /**
  * @brief Apply accessibility settings based on environment
  *
  * @param core_ctx Core context
  * @param access_ctx Accessibility context
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_accessibility_apply_environment_settings(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_ACCESSIBILITY_ACCESSIBILITY_INTERFACE_H_H */