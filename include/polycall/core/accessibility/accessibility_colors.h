/**
 * @file accessibility_colors.h
 * @brief Color definitions and accessibility utilities for LibPolyCall CLI
 *
 * This file defines the color constants and utility functions for
 * rendering text with standardized colors, implementing the Biafran
 * theme with proper accessibility considerations.
 *
 * @copyright OBINexus Computing, 2025
 */

 #ifndef POLYCALL_ACCESSIBILITY_ACCESSIBILITY_COLORS_H_H
 #define POLYCALL_ACCESSIBILITY_ACCESSIBILITY_COLORS_H_H
 
 #include <stdbool.h>
 #include "polycall/core/polycall/polycall_error.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Color theme enumeration
  */
 typedef enum {
     POLYCALL_THEME_DEFAULT,     /**< Default system theme */
     POLYCALL_THEME_BIAFRAN,     /**< Biafran-inspired theme */
     POLYCALL_THEME_HIGH_CONTRAST /**< High-contrast accessibility theme */
 } polycall_color_theme_t;
 
 /**
  * @brief Text element type for which color is applied
  */
 typedef enum {
     POLYCALL_TEXT_NORMAL,       /**< Normal text */
     POLYCALL_TEXT_HEADING,      /**< Headings and titles */
     POLYCALL_TEXT_COMMAND,      /**< Command names */
     POLYCALL_TEXT_SUBCOMMAND,   /**< Subcommand names */
     POLYCALL_TEXT_PARAMETER,    /**< Parameter names */
     POLYCALL_TEXT_VALUE,        /**< Parameter values */
     POLYCALL_TEXT_SUCCESS,      /**< Success messages */
     POLYCALL_TEXT_WARNING,      /**< Warning messages */
     POLYCALL_TEXT_ERROR,        /**< Error messages */
     POLYCALL_TEXT_CODE,         /**< Code snippets */
     POLYCALL_TEXT_HIGHLIGHT     /**< Highlighted text */
 } polycall_text_type_t;
 
 /**
  * @brief Text style attributes
  */
 typedef enum {
     POLYCALL_STYLE_NORMAL       = 0x00, /**< Normal text */
     POLYCALL_STYLE_BOLD         = 0x01, /**< Bold text */
     POLYCALL_STYLE_ITALIC       = 0x02, /**< Italic text */
     POLYCALL_STYLE_UNDERLINE    = 0x04, /**< Underlined text */
     POLYCALL_STYLE_STRIKETHROUGH= 0x08, /**< Strikethrough text */
     POLYCALL_STYLE_INVERSE      = 0x10  /**< Inverse (reverse video) */
 } polycall_text_style_t;
 
 /**
  * @brief Initialize the color system with the specified theme
  *
  * @param theme Color theme to use
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_colors_init(polycall_color_theme_t theme);
 
 /**
  * @brief Get the ANSI color code for the specified text type
  *
  * @param type Text type
  * @param style Text style
  * @return const char* ANSI color code sequence
  */
 const char* polycall_get_color_code(polycall_text_type_t type, polycall_text_style_t style);
 
 /**
  * @brief Get the ANSI reset code
  *
  * @return const char* ANSI reset code sequence
  */
 const char* polycall_get_reset_code(void);
 
 /**
  * @brief Format text with the specified text type and style
  *
  * @param text Text to format
  * @param type Text type
  * @param style Text style
  * @param buffer Buffer to store formatted text
  * @param buffer_size Size of the buffer
  * @return bool True if formatting successful, false otherwise
  */
 bool polycall_format_colored_text(const char* text, polycall_text_type_t type, polycall_text_style_t style,
                               char* buffer, size_t buffer_size);
 
 /**
  * @brief Check if colors are supported in the current terminal
  *
  * @return bool True if colors are supported, false otherwise
  */
 bool polycall_colors_supported(void);
 
 /**
  * @brief Enable or disable colors
  *
  * @param enable True to enable colors, false to disable
  */
 void polycall_colors_enable(bool enable);
 
 /**
  * @brief Set the current color theme
  *
  * @param theme Color theme to use
  * @return polycall_core_error_t Error code
  */
 polycall_core_error_t polycall_set_color_theme(polycall_color_theme_t theme);
 
 /**
  * @brief Get the current color theme
  *
  * @return polycall_color_theme_t Current color theme
  */
 polycall_color_theme_t polycall_get_color_theme(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_ACCESSIBILITY_ACCESSIBILITY_COLORS_H_H */
/* Biafran color scheme support */
typedef enum {
    POLYCALL_BIAFRAN_RED = 0,
    POLYCALL_BIAFRAN_BLACK,
    POLYCALL_BIAFRAN_GREEN,
    POLYCALL_BIAFRAN_YELLOW,
    POLYCALL_BIAFRAN_ORANGE,
    POLYCALL_BIAFRAN_WHITE,
    POLYCALL_BIAFRAN_COLOR_COUNT
} polycall_biafran_color_t;

/* Biafran theme */
#define POLYCALL_THEME_BIAFRAN 0x42494146  /* 'BIAF' in hex */

/* Biafran color functions */
polycall_core_error_t polycall_biafran_colors_init(void);
const char* polycall_get_biafran_color(polycall_biafran_color_t color);
void polycall_print_biafran_banner(const char* text);
