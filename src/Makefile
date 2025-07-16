# OBINexus Polycall Source Component Coordination Makefile
# Coordinates builds across core, cli, and ffi components

# Build configuration
BUILD_MODE ?= release
BUILD_DIR ?= ../build
PARALLEL_JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# Component directories
CORE_COMPONENTS = $(wildcard core/*/.)
CLI_COMPONENTS = $(wildcard cli/*/.)
FFI_COMPONENTS = $(wildcard ffi/*/.)

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -std=c11 -I../include
LDFLAGS = -L$(BUILD_DIR)

ifeq ($(BUILD_MODE),debug)
    CFLAGS += -g -O0 -DDEBUG -DPOLYCALL_DEBUG=1
    BUILD_SUFFIX = _debug
else
    CFLAGS += -O3 -DNDEBUG
    BUILD_SUFFIX = 
endif

# Export build configuration to child makes
export CC CFLAGS LDFLAGS BUILD_MODE BUILD_DIR

# Main targets
.PHONY: all core cli ffi clean help

all: core cli

# Build core components
core:
	@echo "[SRC] Building core components..."
	@if [ -d core ]; then \
		for comp in core/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[CORE] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No core directory found"; \
	fi

# Build CLI components  
cli: core
	@echo "[SRC] Building CLI components..."
	@if [ -d cli ]; then \
		for comp in cli/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[CLI] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No cli directory found"; \
	fi

# Build FFI components
ffi:
	@echo "[SRC] Building FFI components..."
	@if [ -d ffi ]; then \
		for comp in ffi/*; do \
			if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
				echo "[FFI] Building $$comp..."; \
				$(MAKE) -C "$$comp" BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE) || exit 1; \
			fi; \
		done; \
	else \
		echo "[WARN] No ffi directory found"; \
	fi

# Legacy component support
legacy-build:
	@echo "[SRC] Building legacy components with CMake fallback..."
	@mkdir -p $(BUILD_DIR)
	@find . -name "*.c" -exec basename {} .c \; | sort -u > $(BUILD_DIR)/source_inventory.txt
	@echo "[INFO] Found $(shell wc -l < $(BUILD_DIR)/source_inventory.txt) unique source files"

# Clean all components
clean:
	@echo "[SRC] Cleaning all components..."
	@for comp in core/* cli/* ffi/*; do \
		if [ -d "$$comp" ] && [ -f "$$comp/Makefile" ]; then \
			$(MAKE) -C "$$comp" clean 2>/dev/null || true; \
		fi; \
	done
	@find . -name "*.o" -delete 2>/dev/null || true
	@find . -name "*.so" -delete 2>/dev/null || true

help:
	@echo "OBINexus Source Component Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build core and CLI components (default)"
	@echo "  core       - Build only core components"
	@echo "  cli        - Build only CLI components"
	@echo "  ffi        - Build only FFI components"
	@echo "  clean      - Clean all build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build modes: debug, release (default: release)"
	@echo "Example: make core BUILD_MODE=debug"
