# OBINexus Polycall Consolidated Build System
# Integrates Make, CMake, and Meson build systems

# Project configuration
PROJECT_NAME = polycall
VERSION = 0.7.0
BUILD_MODE ?= release
PARALLEL_JOBS ?= $(shell nproc)

# Directories
ROOT_DIR = $(shell pwd)
BUILD_DIR = $(ROOT_DIR)/build
SRC_DIR = $(ROOT_DIR)/src
INCLUDE_DIR = $(ROOT_DIR)/include

# Include library build rules
-include lib/Makefile.lib

# Build system selection
BUILD_SYSTEM ?= make
CMAKE_BUILD_DIR = $(BUILD_DIR)/cmake
MESON_BUILD_DIR = $(BUILD_DIR)/meson

# Default target
.PHONY: all build clean test docs help

all: build

# Make-based build (default)
build:
	@echo "[MAKE] Building polycall project..."
	@mkdir -p $(BUILD_DIR)
	@$(MAKE) -C src BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE)
	@$(MAKE) lib

# CMake-based build
cmake-build:
	@echo "[CMAKE] Building polycall project..."
	@mkdir -p $(CMAKE_BUILD_DIR)
	@cd $(CMAKE_BUILD_DIR) && cmake ../.. \
		-DCMAKE_BUILD_TYPE=$(shell echo $(BUILD_MODE) | sed 's/.*/\u&/') \
		-DBUILD_TESTING=ON
	@cd $(CMAKE_BUILD_DIR) && cmake --build . --parallel $(PARALLEL_JOBS)

# Meson-based build
meson-build: meson-setup
	@echo "[MESON] Building polycall project..."
	@cd $(MESON_BUILD_DIR) && meson compile

meson-setup:
	@echo "[MESON] Setting up build directory..."
	@mkdir -p $(MESON_BUILD_DIR)
	@cd $(MESON_BUILD_DIR) && meson setup .. \
		--buildtype=$(BUILD_MODE) \
		-Denable_testing=true

# Core component builds
build-core:
	@echo "[MAKE] Building core components..."
	@$(MAKE) -C src/core BUILD_MODE=$(BUILD_MODE)

build-cli:
	@echo "[MAKE] Building CLI components..."
	@$(MAKE) -C src/cli BUILD_MODE=$(BUILD_MODE)

# Testing targets
test: build
	@echo "[TEST] Running test suite..."
	@$(MAKE) -C tests run-tests

cmake-test: cmake-build
	@cd $(CMAKE_BUILD_DIR) && ctest --output-on-failure

meson-test: meson-build
	@cd $(MESON_BUILD_DIR) && meson test

# Documentation
docs:
	@echo "[DOCS] Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Warning: doxygen not found, skipping docs"

# Clean targets
clean:
	@echo "[CLEAN] Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@$(MAKE) lib-clean
	@find . -name "*.o" -delete
	@find . -name "*.so" -delete
	@find . -name "*.a" -delete

cmake-clean:
	@echo "[CMAKE-CLEAN] Cleaning CMake build..."
	@rm -rf $(CMAKE_BUILD_DIR)

meson-clean:
	@echo "[MESON-CLEAN] Cleaning Meson build..."
	@rm -rf $(MESON_BUILD_DIR)

clean-all: clean cmake-clean meson-clean

# Installation
install: build
	@echo "[INSTALL] Installing polycall..."
	@$(MAKE) lib-install
	@install -D -m 755 $(BUILD_DIR)/polycall $(DESTDIR)/usr/bin/polycall

# Development targets
format:
	@echo "[FORMAT] Formatting source code..."
	@find src include -name "*.c" -o -name "*.h" | xargs clang-format -i

lint:
	@echo "[LINT] Running static analysis..."
	@cppcheck --enable=all --inconclusive src/ 2>/dev/null || echo "Warning: cppcheck not found"

# Help target
help:
	@echo "OBINexus Polycall Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  build         - Build project using Make (default)"
	@echo "  cmake-build   - Build project using CMake"
	@echo "  meson-build   - Build project using Meson"
	@echo "  test          - Run test suite"
	@echo "  docs          - Generate documentation"
	@echo "  clean         - Clean build artifacts"
	@echo "  install       - Install binaries and libraries"
	@echo "  format        - Format source code"
	@echo "  lint          - Run static analysis"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Build modes: debug, release (default: release)"
	@echo "Example: make build BUILD_MODE=debug"
