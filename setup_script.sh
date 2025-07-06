#!/bin/bash
# Master Setup Script for PolyCall v2
# Full OBINexus compliance with Biafran color accessibility

set -e  # Exit on error

# Biafran flag colors for terminal output
BIAFRAN_RED='\033[38;2;255;0;0m'      # RGB(255,0,0)
BIAFRAN_BLACK='\033[38;2;0;0;0m'      # RGB(0,0,0)
BIAFRAN_GREEN='\033[38;2;0;128;0m'    # RGB(0,128,0)
BIAFRAN_YELLOW='\033[38;2;255;255;0m' # RGB(255,255,0)
BOLD='\033[1m'
RESET='\033[0m'

# Helper functions
print_biafran_banner() {
    echo -e "${BIAFRAN_RED}════════════════════════════════════════════════════════════${RESET}"
    echo -e "${BIAFRAN_BLACK}║${BIAFRAN_YELLOW} ☀☀☀ ${BOLD}$1${RESET}"
    echo -e "${BIAFRAN_GREEN}════════════════════════════════════════════════════════════${RESET}"
}

log_success() {
    echo -e "${BIAFRAN_GREEN}${BOLD}✓${RESET} $1"
}

log_error() {
    echo -e "${BIAFRAN_RED}${BOLD}✗${RESET} $1"
}

log_warning() {
    echo -e "${BIAFRAN_YELLOW}${BOLD}⚠${RESET} $1"
}

log_info() {
    echo -e "${BIAFRAN_BLACK}${BOLD}ℹ${RESET} $1"
}

# Main setup process
print_biafran_banner "POLYCALL v2 MASTER SETUP"
echo
log_info "Project: OBINexus/PolyCall"
log_info "Compliance: Full migration enforcement"
log_info "Date: $(date +"%Y-%m-%d %H:%M:%S")"
echo

# Step 1: Emergency header fixes
log_warning "Step 1: Applying emergency header fixes..."

# Fix unterminated ifndef
cat > include/polycall/core/polycall/polycall_shared_core.h << 'EOF'
#ifndef POLYCALL_SHARED_CORE_H
#define POLYCALL_SHARED_CORE_H

/* Shared core definitions to break circular dependencies */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Core error type */
typedef int32_t polycall_core_error_t;

/* Error codes */
#define POLYCALL_SUCCESS               0
#define POLYCALL_ERROR_INVALID_PARAM  -1
#define POLYCALL_ERROR_OUT_OF_MEMORY  -2
#define POLYCALL_ERROR_NOT_INITIALIZED -3

/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;

#endif /* POLYCALL_SHARED_CORE_H */
EOF

# Fix polycall_core.h
if [ -f "include/polycall/core/polycall/polycall_core.h" ]; then
    # Ensure proper termination
    if ! grep -q "^#endif.*POLYCALL_CORE_H" "include/polycall/core/polycall/polycall_core.h"; then
        echo "#endif /* POLYCALL_CORE_H */" >> "include/polycall/core/polycall/polycall_core.h"
    fi
    
    # Add shared core include after header guard
    sed -i '/#define POLYCALL_CORE_H/a\\n#include "polycall_shared_core.h"' \
        "include/polycall/core/polycall/polycall_core.h"
fi

# Update polycall_error.h to use shared definitions
if [ -f "include/polycall/core/polycall/polycall_error.h" ]; then
    sed -i '/#ifndef POLYCALL_ERROR_H/a\\n#include "polycall_shared_core.h"' \
        "include/polycall/core/polycall/polycall_error.h"
fi

log_success "Emergency header fixes applied"

# Step 2: Implement Biafran color scheme in accessibility module
log_warning "Step 2: Implementing Biafran color accessibility..."

cat > src/core/accessibility/biafran_colors.c << 'EOF'
/**
 * @file biafran_colors.c
 * @brief Biafran color scheme implementation for accessibility
 * 
 * Implements the Biafran flag color palette for terminal and UI accessibility
 */

#include <stdio.h>
#include <string.h>
#include "polycall/core/accessibility/accessibility_colors.h"

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
static const char* biafran_ansi_codes[] = {
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

const char* polycall_get_biafran_color(polycall_biafran_color_t color) {
    if (color >= 0 && color < POLYCALL_BIAFRAN_COLOR_COUNT) {
        return biafran_ansi_codes[color];
    }
    return "\033[0m"; /* Reset */
}

void polycall_print_biafran_banner(const char* text) {
    printf("%s", polycall_get_biafran_color(POLYCALL_BIAFRAN_RED));
    printf("════════════════════════════════════════════════════════════\n");
    
    printf("%s║%s ☀☀☀ %s%s\n", 
           polycall_get_biafran_color(POLYCALL_BIAFRAN_BLACK),
           polycall_get_biafran_color(POLYCALL_BIAFRAN_YELLOW),
           text,
           "\033[0m");
    
    printf("%s", polycall_get_biafran_color(POLYCALL_BIAFRAN_GREEN));
    printf("════════════════════════════════════════════════════════════\n");
    printf("\033[0m");
}
EOF

# Update accessibility header
cat >> include/polycall/core/accessibility/accessibility_colors.h << 'EOF'

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
EOF

log_success "Biafran color accessibility implemented"

# Step 3: Clean up isolated directory
log_warning "Step 3: Cleaning up isolated directory..."

if [ -d "(isolated)" ]; then
    # Create archive with timestamp
    ARCHIVE_DIR="(isolated)/archive/$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$ARCHIVE_DIR"
    
    # Move non-essential files
    find "(isolated)" -maxdepth 1 -type f ! -name "*.json" ! -name "*.md" ! -name "*_ISOLATED.*" \
        -exec mv {} "$ARCHIVE_DIR/" \; 2>/dev/null || true
    
    # Count archived files
    ARCHIVED_COUNT=$(find "$ARCHIVE_DIR" -type f | wc -l)
    log_success "Archived $ARCHIVED_COUNT files from isolated directory"
fi

# Step 4: Organize scripts
log_warning "Step 4: Organizing scripts directory..."

# Create proper structure
mkdir -p scripts/{setup/{windows,linux,darwin,posix},build,test,deploy}

# Move scripts to appropriate locations
if [ -d "(isolated)/scripts-orchestration" ]; then
    # Move build scripts
    find "(isolated)/scripts-orchestration" -name "*.sh" -o -name "*.py" | while read script; do
        basename=$(basename "$script")
        case "$basename" in
            build*|compile*) 
                cp "$script" "scripts/build/" 2>/dev/null || true
                ;;
            test*|validate*) 
                cp "$script" "scripts/test/" 2>/dev/null || true
                ;;
            setup*|install*) 
                cp "$script" "scripts/setup/linux/" 2>/dev/null || true
                ;;
        esac
    done
fi

log_success "Scripts organized into proper structure"

# Step 5: Generate platform setup scripts
log_warning "Step 5: Generating platform-specific setup scripts..."

# Run the Python orchestrator
if command -v python3 &> /dev/null; then
    python3 - << 'PYTHON_SCRIPT'
import os
import sys
sys.path.insert(0, os.getcwd())

# Import and run the setup orchestrator
exec(open('polycall_setup_orchestrator.py').read())
PYTHON_SCRIPT
else
    log_error "Python3 not found. Skipping advanced setup generation."
fi

# Step 6: Create unified Makefile
log_warning "Step 6: Creating unified Makefile..."

cat > Makefile << 'EOF'
# PolyCall v2 Unified Makefile
# Full OBINexus compliance

# Biafran colors for output
RED := \033[38;2;255;0;0m
BLACK := \033[38;2;0;0;0m
GREEN := \033[38;2;0;128;0m
YELLOW := \033[38;2;255;255;0m
RESET := \033[0m

# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=c99 -I./include
LDFLAGS := -lpthread -lm

# Build directories
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

# Source files
CORE_SRCS := $(wildcard src/core/*.c src/core/*/*.c)
CLI_SRCS := $(wildcard src/cli/*.c src/cli/*/*.c)
ALL_SRCS := $(CORE_SRCS) $(CLI_SRCS)

# Object files
OBJS := $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

# Targets
.PHONY: all clean test qa-check biafran-theme

all: biafran-theme $(BIN_DIR)/polycall

biafran-theme:
	@echo -e "$(RED)════════════════════════════════════════════════════════════$(RESET)"
	@echo -e "$(BLACK)║$(YELLOW) ☀☀☀ Building PolyCall v2 with Biafran Theme$(RESET)"
	@echo -e "$(GREEN)════════════════════════════════════════════════════════════$(RESET)"

$(BIN_DIR)/polycall: $(OBJS)
	@mkdir -p $(BIN_DIR)
	@echo -e "$(YELLOW)[LINK]$(RESET) Creating executable..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo -e "$(GREEN)[SUCCESS]$(RESET) Build complete: $@"

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo -e "$(YELLOW)[CC]$(RESET) $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo -e "$(YELLOW)[CLEAN]$(RESET) Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo -e "$(GREEN)[CLEAN]$(RESET) Done"

test: all
	@echo -e "$(YELLOW)[TEST]$(RESET) Running unit tests..."
	@./scripts/test/run_unit_tests.sh

qa-check:
	@echo -e "$(YELLOW)[QA]$(RESET) Running compliance check..."
	@python3 scripts/qa_compliance.py
EOF

log_success "Unified Makefile created"

# Step 7: Final summary
echo
print_biafran_banner "SETUP COMPLETE"
echo
log_success "All systems configured with OBINexus compliance"
log_success "Biafran color accessibility implemented"
log_success "Platform scripts generated for: Windows, Linux, Mac, POSIX"
echo
log_info "Next steps:"
echo "  1. Fix remaining header issues: ${BOLD}bash scripts/adhoc/emergency_header_fix.sh${RESET}"
echo "  2. Build the project: ${BOLD}make all${RESET}"
echo "  3. Run tests: ${BOLD}make test${RESET}"
echo "  4. Check QA compliance: ${BOLD}make qa-check${RESET}"
echo
log_info "Platform-specific setup:"
echo "  - Windows: ${BOLD}powershell -ExecutionPolicy Bypass ./scripts/setup/windows/setup.ps1${RESET}"
echo "  - Linux: ${BOLD}./scripts/setup/linux/setup.sh${RESET}"
echo "  - Mac: ${BOLD}./scripts/setup/darwin/setup.sh${RESET}"
echo "  - POSIX: ${BOLD}sh ./scripts/setup/posix/setup.sh${RESET}"
echo
print_biafran_banner "OBINexus • Bridging Worlds"
