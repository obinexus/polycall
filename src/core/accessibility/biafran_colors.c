/**
 * @file biafran_colors.c
 * @brief Biafran color scheme implementation for accessibility
 *
 * Implements the Biafran flag color palette for terminal and UI accessibility
 */

#include "polycall/core/accessibility/accessibility_colors.h"
#include <stdio.h>
#include <string.h>

/* Biafran flag colors in RGB */
static const polycall_color_rgb_t biafran_palette[] = {
    {255, 0, 0},     /* Red - Top stripe */
    {0, 0, 0},       /* Black - Middle stripe */
    {0, 128, 0},     /* Green - Bottom stripe */
    {255, 255, 0},   /* Yellow - Rising sun */
    {255, 165, 0},   /* Orange - Sun rays */
    {255, 255, 255}, /* White - Contrast */
};

/* ANSI escape sequences for Biafran colors */
static const char *biafran_ansi_codes[] = {
    "\033[38;2;255;0;0m",     /* Red */
    "\033[38;2;0;0;0m",       /* Black */
    "\033[38;2;0;128;0m",     /* Green */
    "\033[38;2;255;255;0m",   /* Yellow */
    "\033[38;2;255;165;0m",   /* Orange */
    "\033[38;2;255;255;255m", /* White */
};

polycall_core_error_t polycall_biafran_colors_init(void) {
  /* Initialize Biafran color theme */
  return polycall_set_color_theme(POLYCALL_THEME_BIAFRAN);
}

const char *polycall_get_biafran_color(polycall_biafran_color_t color) {
  if (color >= 0 && color < POLYCALL_BIAFRAN_COLOR_COUNT) {
    return biafran_ansi_codes[color];
  }
  return "\033[0m"; /* Reset */
}

void polycall_print_biafran_banner(const char *text) {
  printf("%s", polycall_get_biafran_color(POLYCALL_BIAFRAN_RED));
  printf("════════════════════════════════════════════════════════════\n");

  printf("%s║%s ☀☀☀ %s%s\n", polycall_get_biafran_color(POLYCALL_BIAFRAN_BLACK),
         polycall_get_biafran_color(POLYCALL_BIAFRAN_YELLOW), text, "\033[0m");

  printf("%s", polycall_get_biafran_color(POLYCALL_BIAFRAN_GREEN));
  printf("════════════════════════════════════════════════════════════\n");
  printf("\033[0m");
}
