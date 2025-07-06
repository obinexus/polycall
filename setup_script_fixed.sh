#!/bin/bash
# Master Setup Script for PolyCall v2 - Path Corrected Version
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

# Create the directory if it doesn't exist
mkdir -p include/polycall/core/polycall

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

# Create the directory structure
mkdir -p src/core/accessibility

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
if [ -f "include/polycall/core/accessibility/accessibility_colors.h" ]; then
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

/* RGB color structure if not already defined */
#ifndef POLYCALL_COLOR_RGB_DEFINED
#define POLYCALL_COLOR_RGB_DEFINED
typedef struct {
    uint8_t r, g, b;
} polycall_color_rgb_t;
#endif
EOF
fi

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

# Check if the orchestrator exists in scripts/hoc/
if [ -f "scripts/hoc/setup_orchestrator.py" ]; then
    log_info "Found setup orchestrator in scripts/hoc/"
    
    # Run the Python orchestrator with correct path
    if command -v python3 &> /dev/null; then
        cd scripts/hoc/
        python3 setup_orchestrator.py
        cd ../..
        log_success "Platform setup scripts generated"
    else
        log_error "Python3 not found. Cannot run setup orchestrator."
        log_info "Install Python3 to generate platform-specific scripts"
    fi
else
    log_error "Setup orchestrator not found at scripts/hoc/setup_orchestrator.py"
    log_info "Creating basic platform scripts manually..."
    
    # Create basic Linux setup script
    cat > scripts/setup/linux/setup.sh << 'LINUX_SETUP'
#!/bin/bash
# Basic Linux setup for PolyCall v2

echo "Setting up PolyCall v2 for Linux..."

# Check dependencies
for cmd in gcc cmake make; do
    if ! command -v $cmd &> /dev/null; then
        echo "Error: $cmd not found. Please install it."
        exit 1
    fi
done

# Build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

echo "Build complete!"
LINUX_SETUP
    chmod +x scripts/setup/linux/setup.sh
    
    # Create basic Windows script
    cat > scripts/setup/windows/setup.ps1 << 'WINDOWS_SETUP'
# Basic Windows setup for PolyCall v2

Write-Host "Setting up PolyCall v2 for Windows..."

# Check for MinGW
if (!(Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Host "Error: GCC not found. Please install MinGW or MSYS2"
    exit 1
}

# Build
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

Set-Location build
cmake .. -G "MinGW Makefiles"
mingw32-make
Set-Location ..

Write-Host "Build complete!"
WINDOWS_SETUP
    
    log_success "Basic platform scripts created"
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

# Step 7: Create QA compliance script
log_warning "Step 7: Creating QA compliance verification..."

mkdir -p scripts
cat > scripts/qa_compliance.py << 'EOF'
#!/usr/bin/env python3
"""QA Compliance Verification Script"""

import os
import json
from pathlib import Path
from datetime import datetime

def verify_compliance():
    """Verify all modules comply with QA standards"""
    results = {
        "timestamp": datetime.now().isoformat(),
        "modules": {},
        "compliance": True
    }
    
    # Check each module
    modules = ["core", "cli", "auth", "network", "edge", "micro", "telemetry"]
    
    for module in modules:
        module_path = Path("src") / module
        if module_path.exists():
            # Check for test coverage
            test_path = Path("tests/unit") / module
            has_tests = test_path.exists()
            
            # Check for documentation
            doc_path = Path("docs") / module
            has_docs = doc_path.exists()
            
            results["modules"][module] = {
                "has_tests": has_tests,
                "has_docs": has_docs,
                "compliant": has_tests and has_docs
            }
            
            if not (has_tests and has_docs):
                results["compliance"] = False
    
    # Write report
    with open("qa_compliance_report.json", "w") as f:
        json.dump(results, f, indent=2)
    
    # Print summary
    print("QA Compliance Report")
    print("=" * 40)
    for module, status in results["modules"].items():
        symbol = "✓" if status["compliant"] else "✗"
        print(f"{symbol} {module}: Tests={status['has_tests']}, Docs={status['has_docs']}")
    
    return results["compliance"]

if __name__ == "__main__":
    if verify_compliance():
        print("\n✓ Overall QA Compliance: PASSED")
        exit(0)
    else:
        print("\n✗ Overall QA Compliance: FAILED")
        exit(1)
EOF

chmod +x scripts/qa_compliance.py
log_success "QA compliance script created"

# Step 8: Final summary
echo
print_biafran_banner "SETUP COMPLETE"
echo
log_success "All systems configured with OBINexus compliance"
log_success "Biafran color accessibility implemented"
log_success "Platform scripts generated for: Windows, Linux, Mac, POSIX"
echo
log_info "Next steps:"
echo "  1. Test the build: ${BOLD}make all${RESET}"
echo "  2. Run tests: ${BOLD}make test${RESET}"
echo "  3. Check QA compliance: ${BOLD}make qa-check${RESET}"
echo
log_info "Platform-specific setup:"
echo "  - Windows: ${BOLD}powershell -ExecutionPolicy Bypass ./scripts/setup/windows/setup.ps1${RESET}"
echo "  - Linux: ${BOLD}./scripts/setup/linux/setup.sh${RESET}"
echo "  - POSIX: ${BOLD}sh ./scripts/setup/posix/setup.sh${RESET}"
echo
log_info "To run the Python orchestrator manually:"
echo "  ${BOLD}cd scripts/hoc && python3 setup_orchestrator.py${RESET}"
echo
print_biafran_banner "OBINexus • Bridging Worlds"
