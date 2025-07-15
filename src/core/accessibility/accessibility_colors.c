/**
 * @file accessibility_colors.c
 * @brief Implementation of color management and accessibility utilities for
 * LibPolyCall CLI
 *
 * @copyright OBINexus Computing, 2025
 */

#include "polycall/core/accessibility/accessibility_colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ANSI escape code prefix
#define ANSI_ESCAPE "\033["

// Basic ANSI color codes
#define ANSI_RESET "0m"
#define ANSI_BOLD "1m"
#define ANSI_ITALIC "3m"
#define ANSI_UNDERLINE "4m"
#define ANSI_STRIKE "9m"
#define ANSI_INVERSE "7m"

#define ANSI_FG_BLACK "30m"
#define ANSI_FG_RED "31m"
#define ANSI_FG_GREEN "32m"
#define ANSI_FG_YELLOW "33m"
#define ANSI_FG_BLUE "34m"
#define ANSI_FG_MAGENTA "35m"
#define ANSI_FG_CYAN "36m"
#define ANSI_FG_WHITE "37m"

#define ANSI_BG_BLACK "40m"
#define ANSI_BG_RED "41m"
#define ANSI_BG_GREEN "42m"
#define ANSI_BG_YELLOW "43m"
#define ANSI_BG_BLUE "44m"
#define ANSI_BG_MAGENTA "45m"
#define ANSI_BG_CYAN "46m"
#define ANSI_BG_WHITE "47m"

// Bright color variants
#define ANSI_FG_BRIGHT_BLACK "90m"
#define ANSI_FG_BRIGHT_RED "91m"
#define ANSI_FG_BRIGHT_GREEN "92m"
#define ANSI_FG_BRIGHT_YELLOW "93m"
#define ANSI_FG_BRIGHT_BLUE "94m"
#define ANSI_FG_BRIGHT_MAGENTA "95m"
#define ANSI_FG_BRIGHT_CYAN "96m"
#define ANSI_FG_BRIGHT_WHITE "97m"

// Biafran theme color codes (following the style guide)
// These are escape sequences for the closest ANSI approximations
// Actual theme colors from style guide:
// - Liberation Red: #E22C28
// - Palm Black: #000100
// - Forest Green: #008753
// - Golden Sun: #FFD700
// - Red Tint: #FF6666
// - Green Shade: #006B45
// - Sun Yellow: #CC9900

// Biafran theme-specific color sequences
#define BIAFRAN_RED "\033[38;5;196m"             // Liberation Red approximation
#define BIAFRAN_ACCESSIBLE_RED "\033[38;5;203m"  // Red Tint approximation
#define BIAFRAN_BLACK "\033[38;5;16m"            // Palm Black approximation
#define BIAFRAN_GREEN "\033[38;5;29m"            // Forest Green approximation
#define BIAFRAN_ACCESSIBLE_GREEN "\033[38;5;22m" // Green Shade approximation
#define BIAFRAN_GOLD "\033[38;5;220m"            // Golden Sun approximation
#define BIAFRAN_ACCESSIBLE_GOLD "\033[38;5;136m" // Sun Yellow approximation
#define BIAFRAN_IVORY "\033[38;5;255m"           // Ivory White approximation
#define BIAFRAN_CLAY "\033[38;5;242m"            // Clay Gray approximation
#define BIAFRAN_MIDNIGHT "\033[38;5;234m" // Midnight (dark mode) approximation

// Static variables
static polycall_color_theme_t current_theme = POLYCALL_THEME_DEFAULT;
static bool colors_enabled = true;

// Forward declarations
static const char *get_theme_color_code(polycall_text_type_t type,
                                        polycall_text_style_t style);
static const char *get_default_color_code(polycall_text_type_t type,
                                          polycall_text_style_t style);
static const char *get_high_contrast_color_code(polycall_text_type_t type,
                                                polycall_text_style_t style);
static bool detect_terminal_color_support(void);

/**
 * @brief Initialize color system with specified theme
 */
polycall_core_error_t polycall_colors_init(polycall_color_theme_t theme) {
  // Check if colors are supported by the terminal
  colors_enabled = detect_terminal_color_support();

  // Set the theme
  current_theme = theme;

  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get color code for specified text type and style
 */
const char *polycall_get_color_code(polycall_text_type_t type,
                                    polycall_text_style_t style) {
  // If colors are disabled, return empty string
  if (!colors_enabled) {
    return "";
  }

  // Choose color code based on current theme
  switch (current_theme) {
  case POLYCALL_THEME_BIAFRAN:
    return get_theme_color_code(type, style);

  case POLYCALL_THEME_HIGH_CONTRAST:
    return get_high_contrast_color_code(type, style);

  case POLYCALL_THEME_DEFAULT:
  default:
    return get_default_color_code(type, style);
  }
}

/**
 * @brief Get reset code to return to default terminal colors
 */
const char *polycall_get_reset_code(void) {
  static char reset_code[8];

  if (!colors_enabled) {
    return "";
  }

  snprintf(reset_code, sizeof(reset_code), "%s%s", ANSI_ESCAPE, ANSI_RESET);
  return reset_code;
}

/**
 * @brief Format text with specified color and style
 */
bool polycall_format_colored_text(const char *text, polycall_text_type_t type,
                                  polycall_text_style_t style, char *buffer,
                                  size_t buffer_size) {
  if (!text || !buffer || buffer_size == 0) {
    return false;
  }

  // If colors are disabled, just copy the text
  if (!colors_enabled) {
    strncpy(buffer, text, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return true;
  }

  // Get color code
  const char *color_code = polycall_get_color_code(type, style);
  const char *reset_code = polycall_get_reset_code();

  // Format text with color
  int written =
      snprintf(buffer, buffer_size, "%s%s%s", color_code, text, reset_code);

  // Check if the buffer was large enough
  return (written >= 0 && (size_t)written < buffer_size);
}

/**
 * @brief Check if terminal supports colors
 */
bool polycall_colors_supported(void) { return colors_enabled; }

/**
 * @brief Enable or disable colors
 */
void polycall_colors_enable(bool enable) { colors_enabled = enable; }

/**
 * @brief Set current color theme
 */
polycall_core_error_t polycall_set_color_theme(polycall_color_theme_t theme) {
  current_theme = theme;
  return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get current color theme
 */
polycall_color_theme_t polycall_get_color_theme(void) { return current_theme; }

/**
 * @brief Get Biafran theme color code for text type and style
 */
static const char *get_theme_color_code(polycall_text_type_t type,
                                        polycall_text_style_t style) {
  static char color_buffer[32];
  const char *color_code = "";
  const char *style_code = "";

  // Determine base color by text type
  switch (type) {
  case POLYCALL_TEXT_NORMAL:
    color_code = BIAFRAN_IVORY;
    break;

  case POLYCALL_TEXT_HEADING:
    color_code = BIAFRAN_BLACK;
    break;

  case POLYCALL_TEXT_COMMAND:
    color_code = BIAFRAN_RED;
    break;

  case POLYCALL_TEXT_SUBCOMMAND:
    color_code = BIAFRAN_ACCESSIBLE_RED;
    break;

  case POLYCALL_TEXT_PARAMETER:
    color_code = BIAFRAN_ACCESSIBLE_GREEN;
    break;

  case POLYCALL_TEXT_VALUE:
    color_code = BIAFRAN_ACCESSIBLE_GOLD;
    break;

  case POLYCALL_TEXT_SUCCESS:
    color_code = BIAFRAN_GREEN;
    break;

  case POLYCALL_TEXT_WARNING:
    color_code = BIAFRAN_GOLD;
    break;

  case POLYCALL_TEXT_ERROR:
    color_code = BIAFRAN_RED;
    break;

  case POLYCALL_TEXT_CODE:
    color_code = BIAFRAN_CLAY;
    break;

  case POLYCALL_TEXT_HIGHLIGHT:
    color_code = BIAFRAN_GOLD;
    break;
  }

  // Apply style if requested (simplified for demonstration)
  if (style & POLYCALL_STYLE_BOLD) {
    style_code = ANSI_ESCAPE ANSI_BOLD;
  } else if (style & POLYCALL_STYLE_ITALIC) {
    style_code = ANSI_ESCAPE ANSI_ITALIC;
  } else if (style & POLYCALL_STYLE_UNDERLINE) {
    style_code = ANSI_ESCAPE ANSI_UNDERLINE;
  } else if (style & POLYCALL_STYLE_STRIKETHROUGH) {
    style_code = ANSI_ESCAPE ANSI_STRIKE;
  } else if (style & POLYCALL_STYLE_INVERSE) {
    style_code = ANSI_ESCAPE ANSI_INVERSE;
  }

  // For combined styles, we would need a more complex approach

  // Return color+style combination
  snprintf(color_buffer, sizeof(color_buffer), "%s%s", color_code, style_code);
  return color_buffer;
}

/**
 * @brief Get default color code for text type and style
 */
static const char *get_default_color_code(polycall_text_type_t type,
                                          polycall_text_style_t style) {
  static char color_buffer[16];
  const char *color_code = "";

  // Basic ANSI colors for default theme
  switch (type) {
  case POLYCALL_TEXT_NORMAL:
    color_code = ""; // Default terminal color
    break;

  case POLYCALL_TEXT_HEADING:
    color_code = ANSI_ESCAPE ANSI_BOLD;
    break;

  case POLYCALL_TEXT_COMMAND:
    color_code = ANSI_ESCAPE ANSI_FG_BLUE;
    break;

  case POLYCALL_TEXT_SUBCOMMAND:
    color_code = ANSI_ESCAPE ANSI_FG_CYAN;
    break;

  case POLYCALL_TEXT_PARAMETER:
    color_code = ANSI_ESCAPE ANSI_FG_MAGENTA;
    break;

  case POLYCALL_TEXT_VALUE:
    color_code = ANSI_ESCAPE ANSI_FG_YELLOW;
    break;

  case POLYCALL_TEXT_SUCCESS:
    color_code = ANSI_ESCAPE ANSI_FG_GREEN;
    break;

  case POLYCALL_TEXT_WARNING:
    color_code = ANSI_ESCAPE ANSI_FG_YELLOW;
    break;

  case POLYCALL_TEXT_ERROR:
    color_code = ANSI_ESCAPE ANSI_FG_RED;
    break;

  case POLYCALL_TEXT_CODE:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_BLACK;
    break;

  case POLYCALL_TEXT_HIGHLIGHT:
    color_code = ANSI_ESCAPE ANSI_FG_YELLOW;
    break;
  }

  // Apply style if requested
  if (style & POLYCALL_STYLE_BOLD) {
    snprintf(color_buffer, sizeof(color_buffer), "%s%s%s", color_code,
             ANSI_ESCAPE, ANSI_BOLD);
    color_code = color_buffer;
  }

  return color_code;
}

/**
 * @brief Get high contrast color code for text type and style
 */
static const char *get_high_contrast_color_code(polycall_text_type_t type,
                                                polycall_text_style_t style) {
  static char color_buffer[16];
  const char *color_code = "";

  // High contrast theme for accessibility
  switch (type) {
  case POLYCALL_TEXT_NORMAL:
    color_code = ANSI_ESCAPE ANSI_FG_WHITE;
    break;

  case POLYCALL_TEXT_HEADING:
    color_code = ANSI_ESCAPE ANSI_FG_WHITE ANSI_ESCAPE ANSI_BOLD;
    break;

  case POLYCALL_TEXT_COMMAND:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_YELLOW;
    break;

  case POLYCALL_TEXT_SUBCOMMAND:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_CYAN;
    break;

  case POLYCALL_TEXT_PARAMETER:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_MAGENTA;
    break;

  case POLYCALL_TEXT_VALUE:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_GREEN;
    break;

  case POLYCALL_TEXT_SUCCESS:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_GREEN;
    break;

  case POLYCALL_TEXT_WARNING:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_YELLOW;
    break;

  case POLYCALL_TEXT_ERROR:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_RED;
    break;

  case POLYCALL_TEXT_CODE:
    color_code = ANSI_ESCAPE ANSI_FG_BRIGHT_WHITE;
    break;

  case POLYCALL_TEXT_HIGHLIGHT:
    color_code = ANSI_ESCAPE ANSI_FG_BLACK ANSI_ESCAPE ANSI_BG_WHITE;
    break;
  }

  // Apply style if requested
  if (style & POLYCALL_STYLE_BOLD) {
    snprintf(color_buffer, sizeof(color_buffer), "%s%s%s", color_code,
             ANSI_ESCAPE, ANSI_BOLD);
    color_code = color_buffer;
  }

  return color_code;
}

/**
 * @brief Detect terminal color support
 */
static bool detect_terminal_color_support(void) {
  // Check for NO_COLOR environment variable (https://no-color.org/)
  if (getenv("NO_COLOR") != NULL) {
    return false;
  }

  // Check for TERM environment variable
  const char *term = getenv("TERM");
  if (term != NULL) {
    if (strcmp(term, "dumb") == 0) {
      return false;
    }

    // Common color-supporting terminal types
    if (strstr(term, "color") != NULL || strstr(term, "ansi") != NULL ||
        strstr(term, "xterm") != NULL || strstr(term, "linux") != NULL ||
        strstr(term, "vt100") != NULL || strstr(term, "screen") != NULL) {
      return true;
    }
  }

  // Check for COLORTERM
  const char *colorterm = getenv("COLORTERM");
  if (colorterm != NULL) {
    return true;
  }

  // Check for CI/CD environments
  if (getenv("CI") != NULL || getenv("GITHUB_ACTIONS") != NULL) {
    return true;
  }

// On Windows, check for ANSICON or ConEmu
#ifdef _WIN32
  if (getenv("ANSICON") != NULL || getenv("ConEmuANSI") != NULL) {
    return true;
  }
#endif

  // Default to enabled for interactive terminals
  return isatty(1) == 1;
}