/**
 * @file accessibility_interface.c
 * @brief Implementation of the accessibility interface for LibPolyCall CLI
 *
 * @copyright OBINexus Computing, 2025
 */

 #include "polycall/core/accessibility/accessibility_interface.h"
 #include "polycall/core/accessibility/accessibility_colors.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 // Maximum buffer size for internal operations
 #define MAX_BUFFER_SIZE 8192
 
 // Maximum table cell width
 #define MAX_CELL_WIDTH 64
 

 // Forward declarations
 static bool detect_screen_reader(void);
 static bool detect_high_contrast(void);
 static void detect_terminal_dimensions(int* width, int* height);
 static bool format_table_row(char* buffer, size_t buffer_size, const char** cells, size_t cell_count, 
                            const int* widths, bool is_header, polycall_accessibility_context_t* ctx);
 /**
 * @brief Play an audio notification with accessibility context
 */
bool polycall_accessibility_play_notification(
    polycall_core_context_t* core_ctx,
    polycall_accessibility_context_t* access_ctx,
    polycall_audio_notification_t notification_type
) {
    if (!core_ctx || !access_ctx) {
        return false;
    }
    
    // Only play notification if audio is enabled in accessibility config
    if (!access_ctx->config.enable_audio_notifications) {
        return true;  // Return success but do nothing
    }
    
    // Configure audio system if needed
    bool audio_enabled = false;
    polycall_audio_is_enabled(core_ctx, &audio_enabled);
    
    if (audio_enabled != access_ctx->config.enable_audio_notifications) {
        polycall_audio_enable(core_ctx, access_ctx->config.enable_audio_notifications);
    }
    
    // Set volume if needed
    polycall_audio_set_volume(core_ctx, access_ctx->config.audio_volume);
    
    // Play the notification
    polycall_core_error_t result = polycall_audio_play_notification(
        core_ctx, notification_type
    );
    
    return (result == POLYCALL_CORE_SUCCESS);
}

/**
 * @brief Configure audio notification settings
 */
bool polycall_accessibility_configure_audio(
    polycall_core_context_t* core_ctx,
    polycall_accessibility_context_t* access_ctx,
    bool enable_audio,
    int volume
) {
    if (!core_ctx || !access_ctx) {
        return false;
    }
    
    // Validate volume
    if (volume < 0 || volume > 100) {
        return false;
    }
    
    // Update accessibility config
    access_ctx->config.enable_audio_notifications = enable_audio;
    access_ctx->config.audio_volume = volume;
    
    // Update audio system
    polycall_audio_enable(core_ctx, enable_audio);
    polycall_audio_set_volume(core_ctx, volume);
    
    return true;
}
 /**
  * @brief Create default accessibility configuration
  */
 polycall_accessibility_config_t polycall_accessibility_default_config(void) {
     polycall_accessibility_config_t config;
     
     // Initialize all fields
     config.color_theme = POLYCALL_THEME_BIAFRAN;
     config.enable_high_contrast = false;
     config.enable_screen_reader_support = true;
     config.enable_keyboard_shortcuts = true;
     config.enable_motion_reduction = false;
     config.auto_detect_preferences = true;
     config.min_font_size = 12;
     config.focus_indicator_width = 3;
     config.custom_stylesheet = NULL;
     
     return config;
 }
 polycall_accessibility_config_t polycall_accessibility_default_config(void) {
    polycall_accessibility_config_t config;
    
    // Initialize all fields
    config.color_theme = POLYCALL_THEME_BIAFRAN;
    config.enable_high_contrast = false;
    config.enable_screen_reader_support = true;
    config.enable_keyboard_shortcuts = true;
    config.enable_motion_reduction = false;
    config.enable_audio_notifications = true;
    config.audio_volume = 80;
    config.auto_detect_preferences = true;
    config.min_font_size = 12;
    config.focus_indicator_width = 3;
    config.custom_stylesheet = NULL;
    
    return config;
}
 /**
  * @brief Initialize accessibility context
  */
 polycall_core_error_t polycall_accessibility_init(
     polycall_core_context_t* core_ctx,
     const polycall_accessibility_config_t* config,
     polycall_accessibility_context_t** access_ctx
 ) {
     if (!core_ctx || !config || !access_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate context
     polycall_accessibility_context_t* ctx = (polycall_accessibility_context_t*)
         malloc(sizeof(polycall_accessibility_context_t));
     if (!ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize fields
     memcpy(&ctx->config, config, sizeof(polycall_accessibility_config_t));
     
     // Allocate internal buffer
     ctx->buffer_size = MAX_BUFFER_SIZE;
     ctx->buffer = (char*)malloc(ctx->buffer_size);
     if (!ctx->buffer) {
         free(ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Auto-detect preferences if enabled
     if (config->auto_detect_preferences) {
         ctx->screen_reader_active = detect_screen_reader();
         ctx->high_contrast_active = detect_high_contrast();
         
         // Override config with detected settings
         if (ctx->high_contrast_active) {
             ctx->config.enable_high_contrast = true;
             ctx->config.color_theme = POLYCALL_THEME_HIGH_CONTRAST;
         }
     } else {
         ctx->screen_reader_active = config->enable_screen_reader_support;
         ctx->high_contrast_active = config->enable_high_contrast;
     }
     
     // Detect terminal dimensions
     detect_terminal_dimensions(&ctx->terminal_width, &ctx->terminal_height);
     
     // Initialize color system
     polycall_colors_init(ctx->config.color_theme);
     
     *access_ctx = ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup accessibility context
  */
 void polycall_accessibility_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 ) {
     if (!access_ctx) {
         return;
     }
     
     // Free custom stylesheet if allocated
     if (access_ctx->config.custom_stylesheet) {
         free(access_ctx->config.custom_stylesheet);
     }
     
     // Free internal buffer
     if (access_ctx->buffer) {
         free(access_ctx->buffer);
     }
     
     // Free context
     free(access_ctx);
 }
 
 /**
  * @brief Format text with accessibility settings
  */
 bool polycall_accessibility_format_text(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* text,
     polycall_text_type_t type,
     polycall_text_style_t style,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !text || !buffer || buffer_size == 0) {
         return false;
     }
     
     // If screen reader is active, add screen reader hints for certain text types
     if (access_ctx->screen_reader_active) {
         const char* sr_prefix = "";
         
         switch (type) {
             case POLYCALL_TEXT_HEADING:
                 sr_prefix = "Heading: ";
                 break;
                 
             case POLYCALL_TEXT_COMMAND:
                 sr_prefix = "Command: ";
                 break;
                 
             case POLYCALL_TEXT_ERROR:
                 sr_prefix = "Error: ";
                 break;
                 
             case POLYCALL_TEXT_WARNING:
                 sr_prefix = "Warning: ";
                 break;
                 
             case POLYCALL_TEXT_SUCCESS:
                 sr_prefix = "Success: ";
                 break;
                 
             default:
                 sr_prefix = "";
                 break;
         }
         
         // Format with screen reader prefix
         if (strlen(sr_prefix) > 0) {
             char temp[MAX_BUFFER_SIZE];
             snprintf(temp, sizeof(temp), "%s%s", sr_prefix, text);
             return polycall_format_colored_text(temp, type, style, buffer, buffer_size);
         }
     }
     
     // Default formatting using color system
     return polycall_format_colored_text(text, type, style, buffer, buffer_size);
 }
 
 /**
  * @brief Format command help with accessibility settings
  */
 bool polycall_accessibility_format_command_help(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* command,
     const char* description,
     const char* usage,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !command || !buffer || buffer_size == 0) {
         return false;
     }
     
     // Format command name with colors
     char command_buf[256];
     polycall_accessibility_format_text(core_ctx, access_ctx, command, 
                                      POLYCALL_TEXT_COMMAND, POLYCALL_STYLE_BOLD,
                                      command_buf, sizeof(command_buf));
     
     // Format description if provided
     char description_buf[512] = "";
     if (description) {
         polycall_accessibility_format_text(core_ctx, access_ctx, description,
                                          POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL,
                                          description_buf, sizeof(description_buf));
     }
     
     // Format usage if provided
     char usage_buf[1024] = "";
     if (usage) {
         char usage_label[128];
         polycall_accessibility_format_text(core_ctx, access_ctx, "Usage:",
                                          POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD,
                                          usage_label, sizeof(usage_label));
         
         char usage_text[896];
         polycall_accessibility_format_text(core_ctx, access_ctx, usage,
                                          POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL,
                                          usage_text, sizeof(usage_text));
         
         snprintf(usage_buf, sizeof(usage_buf), "\n%s %s", usage_label, usage_text);
     }
     
     // Screen reader friendly formatting
     if (access_ctx->screen_reader_active) {
         snprintf(buffer, buffer_size, 
                  "Command: %s\nDescription: %s%s\n",
                  command, 
                  description ? description : "No description available", 
                  usage ? usage_buf : "");
     } else {
         // Visual formatting
         snprintf(buffer, buffer_size, 
                  "%s - %s%s\n",
                  command_buf, 
                  description ? description_buf : "No description available", 
                  usage ? usage_buf : "");
     }
     
     return true;
 }
 
 /**
  * @brief Format error message with accessibility settings
  */
 bool polycall_accessibility_format_error(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     int error_code,
     const char* error_message,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !buffer || buffer_size == 0) {
         return false;
     }
     
     // Format error message
     char message_buf[1024];
     if (error_message) {
         polycall_accessibility_format_text(core_ctx, access_ctx, error_message,
                                          POLYCALL_TEXT_ERROR, POLYCALL_STYLE_NORMAL,
                                          message_buf, sizeof(message_buf));
     } else {
         polycall_accessibility_format_text(core_ctx, access_ctx, "Unknown error",
                                          POLYCALL_TEXT_ERROR, POLYCALL_STYLE_NORMAL,
                                          message_buf, sizeof(message_buf));
     }
     
     // Format error code
     char code_buf[128];
     snprintf(code_buf, sizeof(code_buf), "Error code: %d", error_code);
     
     char code_formatted[256];
     polycall_accessibility_format_text(core_ctx, access_ctx, code_buf,
                                      POLYCALL_TEXT_ERROR, POLYCALL_STYLE_NORMAL,
                                      code_formatted, sizeof(code_formatted));
     
     // Screen reader friendly formatting
     if (access_ctx->screen_reader_active) {
         snprintf(buffer, buffer_size, 
                  "Error: %s. %s",
                  error_message ? error_message : "Unknown error", 
                  code_buf);
     } else {
         // Visual formatting
         snprintf(buffer, buffer_size, 
                  "%s (%s)",
                  message_buf, 
                  code_formatted);
     }
     
     return true;
 }
 
 /**
  * @brief Format success message with accessibility settings
  */
 bool polycall_accessibility_format_success(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* message,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !message || !buffer || buffer_size == 0) {
         return false;
     }
     
     // Format with color system
     return polycall_accessibility_format_text(core_ctx, access_ctx, message,
                                             POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL,
                                             buffer, buffer_size);
 }
 
 /**
  * @brief Format progress display with accessibility settings
  */
 bool polycall_accessibility_format_progress(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     float progress,
     const char* label,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !buffer || buffer_size == 0 || 
         progress < 0.0f || progress > 1.0f) {
         return false;
     }
     
     // Calculate percentages for display
     int percent = (int)(progress * 100.0f);
     
     // For screen readers, provide a simple percentage
     if (access_ctx->screen_reader_active) {
         if (label) {
             snprintf(buffer, buffer_size, "%s: %d%% complete", label, percent);
         } else {
             snprintf(buffer, buffer_size, "%d%% complete", percent);
         }
         return true;
     }
     
     // For visual display, generate a progress bar
     int bar_width = access_ctx->terminal_width > 80 ? 40 : 20;
     int progress_chars = (int)(progress * bar_width);
     
     // Format the bar in the right color
     char progress_bar[128] = "";
     char bar_color[64];
     polycall_get_color_code(POLYCALL_TEXT_SUCCESS, POLYCALL_STYLE_NORMAL);
     
     char reset[16];
     strcpy(reset, polycall_get_reset_code());
     
     // Generate visual progress bar
     snprintf(progress_bar, sizeof(progress_bar), "[%s", bar_color);
     
     for (int i = 0; i < bar_width; i++) {
         if (i < progress_chars) {
             strcat(progress_bar, "=");
         } else {
             strcat(progress_bar, " ");
         }
     }
     
     strcat(progress_bar, reset);
     strcat(progress_bar, "]");
     
     // Combine label, bar, and percentage
     if (label) {
         snprintf(buffer, buffer_size, "%s: %s %d%%", label, progress_bar, percent);
     } else {
         snprintf(buffer, buffer_size, "%s %d%%", progress_bar, percent);
     }
     
     return true;
 }
 
 /**
  * @brief Format table with accessibility settings
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
 ) {
     if (!core_ctx || !access_ctx || !headers || !data || !buffer || buffer_size == 0 ||
         header_count == 0 || row_count == 0 || col_count == 0) {
         return false;
     }
     
     // For screen readers, simple list format is more accessible
     if (access_ctx->screen_reader_active) {
         size_t offset = 0;
         
         // Add header row as description
         offset += snprintf(buffer + offset, buffer_size - offset, "Table with %zu rows and %zu columns.\n", 
                           row_count, col_count);
         
         // Format each row
         for (size_t row = 0; row < row_count; row++) {
             offset += snprintf(buffer + offset, buffer_size - offset, "Row %zu:\n", row + 1);
             
             // Format each cell
             for (size_t col = 0; col < col_count; col++) {
                 offset += snprintf(buffer + offset, buffer_size - offset, "  %s: %s\n", 
                                   headers[col], data[row][col]);
                 
                 if (offset >= buffer_size - 1) {
                     buffer[buffer_size - 1] = '\0';
                     return false; // Buffer overflow
                 }
             }
         }
         
         return true;
     }
     
     // For visual display, traditional table format
     // Calculate column widths
     int widths[32]; // Assume max 32 columns
     if (col_count > 32) {
         return false; // Too many columns
     }
     
     // Initialize with header widths
     for (size_t col = 0; col < col_count; col++) {
         widths[col] = strlen(headers[col]);
     }
     
     // Find maximum width for each column
     for (size_t row = 0; row < row_count; row++) {
         for (size_t col = 0; col < col_count; col++) {
             int cell_width = strlen(data[row][col]);
             if (cell_width > widths[col]) {
                 widths[col] = cell_width;
             }
             
             // Cap at maximum width
             if (widths[col] > MAX_CELL_WIDTH) {
                 widths[col] = MAX_CELL_WIDTH;
             }
         }
     }
     
     // Format table
     size_t offset = 0;
     
     // Format header row
     if (!format_table_row(buffer + offset, buffer_size - offset, 
                         headers, col_count, widths, true, access_ctx)) {
         return false;
     }
     
     offset = strlen(buffer);
     
     // Add separator row
     buffer[offset++] = '\n';
     
     for (size_t col = 0; col < col_count; col++) {
         for (int i = 0; i < widths[col] + 2; i++) {
             if (offset < buffer_size - 1) {
                 buffer[offset++] = '-';
             } else {
                 buffer[buffer_size - 1] = '\0';
                 return false; // Buffer overflow
             }
         }
         
         if (col < col_count - 1 && offset < buffer_size - 1) {
             buffer[offset++] = ' ';
         }
     }
     
     buffer[offset++] = '\n';
     
     // Format data rows
     for (size_t row = 0; row < row_count; row++) {
         if (!format_table_row(buffer + offset, buffer_size - offset, 
                             data[row], col_count, widths, false, access_ctx)) {
             return false;
         }
         
         offset = strlen(buffer);
         
         if (offset < buffer_size - 1) {
             buffer[offset++] = '\n';
             buffer[offset] = '\0';
         } else {
             buffer[buffer_size - 1] = '\0';
             return false; // Buffer overflow
         }
     }
     
     return true;
 }
 
 /**
  * @brief Get screen reader text for GUI element
  */
 bool polycall_accessibility_get_screen_reader_text(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* element_type,
     const char* element_name,
     const char* element_state,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !element_type || !buffer || buffer_size == 0) {
         return false;
     }
     
     // Format appropriate screen reader text based on element type
     if (strcmp(element_type, "button") == 0) {
         snprintf(buffer, buffer_size, "Button: %s%s%s", 
                 element_name ? element_name : "Unnamed",
                 element_state ? ", " : "",
                 element_state ? element_state : "");
     } else if (strcmp(element_type, "input") == 0) {
         snprintf(buffer, buffer_size, "Input field: %s%s%s", 
                 element_name ? element_name : "Unnamed",
                 element_state ? ", " : "",
                 element_state ? element_state : "");
     } else if (strcmp(element_type, "checkbox") == 0) {
         // Assume element_state is "checked" or "unchecked"
         snprintf(buffer, buffer_size, "Checkbox: %s, %s", 
                 element_name ? element_name : "Unnamed",
                 element_state ? element_state : "unchecked");
     } else if (strcmp(element_type, "link") == 0) {
         snprintf(buffer, buffer_size, "Link: %s", 
                 element_name ? element_name : "Unnamed URL");
     } else if (strcmp(element_type, "dropdown") == 0) {
         snprintf(buffer, buffer_size, "Dropdown menu: %s%s%s", 
                 element_name ? element_name : "Unnamed",
                 element_state ? ", selected: " : "",
                 element_state ? element_state : "");
     } else if (strcmp(element_type, "heading") == 0) {
         snprintf(buffer, buffer_size, "Heading: %s", 
                 element_name ? element_name : "Unnamed");
     } else {
         // Generic element
         snprintf(buffer, buffer_size, "%s: %s%s%s", 
                 element_type,
                 element_name ? element_name : "Unnamed",
                 element_state ? ", " : "",
                 element_state ? element_state : "");
     }
     
     return true;
 }
 
 /**
  * @brief Format REPL prompt with accessibility settings
  */
 bool polycall_accessibility_format_prompt(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx,
     const char* prompt,
     char* buffer,
     size_t buffer_size
 ) {
     if (!core_ctx || !access_ctx || !prompt || !buffer || buffer_size == 0) {
         return false;
     }
     
     // For screen readers, add context information
     if (access_ctx->screen_reader_active) {
         snprintf(buffer, buffer_size, "Command prompt: %s", prompt);
     } else {
         // For visual, format with appropriate color
         return polycall_accessibility_format_text(core_ctx, access_ctx, prompt,
                                                POLYCALL_TEXT_COMMAND, POLYCALL_STYLE_BOLD,
                                                buffer, buffer_size);
     }
     
     return true;
 }
 
 /**
  * @brief Check if screen reader is active
  */
 bool polycall_accessibility_is_screen_reader_active(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 ) {
     if (!access_ctx) {
         return false;
     }
     
     return access_ctx->screen_reader_active;
 }
 
 /**
  * @brief Apply accessibility settings based on environment
  */
 polycall_core_error_t polycall_accessibility_apply_environment_settings(
     polycall_core_context_t* core_ctx,
     polycall_accessibility_context_t* access_ctx
 ) {
     if (!core_ctx || !access_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Re-detect screen reader and high contrast settings
     access_ctx->screen_reader_active = detect_screen_reader();
     access_ctx->high_contrast_active = detect_high_contrast();
     
     // Apply color theme based on detected settings
     if (access_ctx->high_contrast_active) {
         access_ctx->config.color_theme = POLYCALL_THEME_HIGH_CONTRAST;
         polycall_set_color_theme(POLYCALL_THEME_HIGH_CONTRAST);
     } else if (access_ctx->config.color_theme != POLYCALL_THEME_DEFAULT) {
         polycall_set_color_theme(access_ctx->config.color_theme);
     }
     
     // Re-detect terminal dimensions
     detect_terminal_dimensions(&access_ctx->terminal_width, &access_ctx->terminal_height);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Detect if a screen reader is likely active
  */
 static bool detect_screen_reader(void) {
     // Check for environment variables set by common screen readers
     if (getenv("NVDA_LAUNCHED") != NULL || 
         getenv("JAWS_LAUNCHED") != NULL || 
         getenv("SCREEN_READER_ACTIVE") != NULL ||
         getenv("ACCESSIBILITY_ENABLED") != NULL) {
         return true;
     }
     
     // On macOS, check for VoiceOver
     #ifdef __APPLE__
     if (getenv("VOICEOVER_RUNNING") != NULL) {
         return true;
     }
     #endif
     
     // On Linux, check for Orca
     #ifdef __linux__
     if (getenv("ORCA_RUNNING") != NULL || getenv("AT_SPI_BUS") != NULL) {
         return true;
     }
     #endif
     
     // Default to using configuration value
     return false;
 }
 
 /**
  * @brief Detect if high contrast mode is likely active
  */
 static bool detect_high_contrast(void) {
     // Check for environment variables suggesting high contrast
     if (getenv("HIGH_CONTRAST") != NULL || 
         getenv("ACCESSIBILITY_VISUAL") != NULL) {
         return true;
     }
     
     // On Windows, could check registry but not practical here
     
     // Default to using configuration value
     return false;
 }
 
 /**
  * @brief Detect terminal dimensions
  */
 static void detect_terminal_dimensions(int* width, int* height) {
     // Default values
     *width = 80;
     *height = 24;
     
     // Try to get actual terminal size
     #ifdef _WIN32
     CONSOLE_SCREEN_BUFFER_INFO csbi;
     if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
         *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
         *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
     }
     #else
     // Try to use ioctl to get terminal size
     #include <sys/ioctl.h>
     #include <unistd.h>
     struct winsize ws;
     if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
         *width = ws.ws_col;
         *height = ws.ws_row;
     }
     #endif
     
     // Check for environment variable overrides
     const char* cols = getenv("COLUMNS");
     const char* rows = getenv("LINES");
     
     if (cols) {
         int value = atoi(cols);
         if (value > 0) {
             *width = value;
         }
     }
     
     if (rows) {
         int value = atoi(rows);
         if (value > 0) {
             *height = value;
         }
     }
 }
 
 /**
  * @brief Format a table row with appropriate colors
  */
 static bool format_table_row(
     char* buffer, 
     size_t buffer_size, 
     const char** cells, 
     size_t cell_count, 
     const int* widths,
     bool is_header, 
     polycall_accessibility_context_t* ctx
 ) {
     size_t offset = 0;
     
     for (size_t col = 0; col < cell_count; col++) {
         // Get cell text
         const char* cell_text = cells[col];
         
         // Format cell with colors
         char cell_buffer[MAX_CELL_WIDTH + 64];
         
         if (is_header) {
             polycall_accessibility_format_text(NULL, ctx, cell_text,
                                              POLYCALL_TEXT_HEADING, POLYCALL_STYLE_BOLD,
                                              cell_buffer, sizeof(cell_buffer));
         } else {
             polycall_accessibility_format_text(NULL, ctx, cell_text,
                                              POLYCALL_TEXT_NORMAL, POLYCALL_STYLE_NORMAL,
                                              cell_buffer, sizeof(cell_buffer));
         }
         
         // Add cell to row with padding
         offset += snprintf(buffer + offset, buffer_size - offset, " %-*s ", 
                           widths[col], cell_buffer);
         
         if (offset >= buffer_size - 1) {
             buffer[buffer_size - 1] = '\0';
             return false; // Buffer overflow
         }
     }
     
     buffer[offset] = '\0';
     return true;
 }

/**
 * @brief Detect terminal dimensions
 */
static void detect_terminal_dimensions(int* width, int* height) {
    // Default values
    *width = 80;
    *height = 24;
    
    #ifdef _WIN32
    // On Windows, use GetConsoleScreenBufferInfo
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    #else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *width = ws.ws_col;
        *height = ws.ws_row;
    }
    #endif
    
    // Check for environment variable overrides
    const char* cols = getenv("COLUMNS");
    const char* rows = getenv("LINES");
    
    if (cols) {
        int value = atoi(cols);
        if (value > 0) {
            *width = value;
        }
    }
    
    if (rows) {
        int value = atoi(rows);
        if (value > 0) {
            *height = value;
        }
    }
}

