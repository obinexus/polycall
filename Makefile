# OBINexus PolyCall Root Makefile
# Build Orchestration with Security and Edge Micro Features
# Copyright (c) 2025 OBINexus Computing

# Version and Build Configuration
VERSION := 0.1.0-dev
BUILD_DATE := $(shell date -u +"%Y-%m-%dT%H:%M:%SZ")
BUILD_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "no-git")

# Detect OS and Architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Build Mode Control
BUILD_MODE ?= release
EDGE_MICRO ?= disabled
SECURITY_LEVEL ?= standard

# Compiler Configuration
CC ?= gcc
CXX ?= g++
AR ?= ar
RANLIB ?= ranlib

# Base Directories
ROOT_DIR := $(shell pwd)
BUILD_DIR := build
SRC_DIR := src
INCLUDE_DIR := include
TOOLS_DIR := tools
CMAKE_DIR := cmake

# Feature Flags
FEATURES := \
	-DPOLYCALL_VERSION=\"$(VERSION)\" \
	-DPOLYCALL_BUILD_DATE=\"$(BUILD_DATE)\" \
	-DPOLYCALL_BUILD_HASH=\"$(BUILD_HASH)\"

# Security Flags
ifeq ($(SECURITY_LEVEL),paranoid)
	SECURITY_FLAGS := -fstack-protector-all -D_FORTIFY_SOURCE=2 -fPIE
else ifeq ($(SECURITY_LEVEL),standard)
	SECURITY_FLAGS := -fstack-protector-strong -D_FORTIFY_SOURCE=1
else
	SECURITY_FLAGS :=
endif

# Edge Micro Configuration
ifeq ($(EDGE_MICRO),enabled)
	FEATURES += -DPOLYCALL_EDGE_MICRO_ENABLED
	EDGE_FLAGS := -Os -ffunction-sections -fdata-sections
	EDGE_LDFLAGS := -Wl,--gc-sections
else
	EDGE_FLAGS :=
	EDGE_LDFLAGS :=
endif

# Common Flags
COMMON_FLAGS := \
	-Wall -Wextra -Werror \
	-I$(INCLUDE_DIR) \
	$(FEATURES) \
	$(SECURITY_FLAGS) \
	$(EDGE_FLAGS)

# Mode-specific Flags
ifeq ($(BUILD_MODE),debug)
	CFLAGS := $(COMMON_FLAGS) -g -O0 -DDEBUG
	CXXFLAGS := $(COMMON_FLAGS) -g -O0 -DDEBUG
else ifeq ($(BUILD_MODE),release)
	CFLAGS := $(COMMON_FLAGS) -O2 -DNDEBUG
	CXXFLAGS := $(COMMON_FLAGS) -O2 -DNDEBUG
else ifeq ($(BUILD_MODE),profile)
	CFLAGS := $(COMMON_FLAGS) -O2 -pg -DPROFILE
	CXXFLAGS := $(COMMON_FLAGS) -O2 -pg -DPROFILE
endif

# Subcommand Makefiles
MAKEFILE_PURITY := Makefile.purity
MAKEFILE_SPEC := Makefile.spec
MAKEFILE_BUILD := Makefile.build
MAKEFILE_VENDOR := Makefile.vendor

# Primary Targets
.PHONY: all clean test install uninstall help

all: check-mutex build

# Mutex Check for Command Exclusivity
check-mutex:
	@$(MAKE) -f $(MAKEFILE_PURITY) check-commands

# Build Targets
build: check-mutex
	@echo "Building PolyCall [$(BUILD_MODE)] [Security: $(SECURITY_LEVEL)] [Edge: $(EDGE_MICRO)]"
	@$(MAKE) -f $(MAKEFILE_BUILD) build-all \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		BUILD_DIR="$(BUILD_DIR)" \
		EDGE_LDFLAGS="$(EDGE_LDFLAGS)"

# QA and Testing
test: check-mutex
	@$(MAKE) -f $(MAKEFILE_SPEC) run-tests \
		BUILD_DIR="$(BUILD_DIR)"

qa: check-mutex
	@$(MAKE) -f $(MAKEFILE_SPEC) qa-full \
		SRC_DIR="$(SRC_DIR)" \
		INCLUDE_DIR="$(INCLUDE_DIR)"

# Vendor/Browser Testing
vendor-test: check-mutex
	@$(MAKE) -f $(MAKEFILE_VENDOR) test-all-browsers

# Edge Micro Features
edge-deploy: check-mutex
	@if [ "$(EDGE_MICRO)" != "enabled" ]; then \
		echo "Error: Edge micro features not enabled. Set EDGE_MICRO=enabled"; \
		exit 1; \
	fi
	@$(MAKE) -f $(MAKEFILE_BUILD) edge-deploy

# Security Commands
security-audit: check-mutex
	@echo "Running security audit..."
	@$(MAKE) -f $(MAKEFILE_PURITY) security-scan \
		SRC_DIR="$(SRC_DIR)" \
		SECURITY_LEVEL="$(SECURITY_LEVEL)"

# Clean Targets
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@find . -name "*.o" -delete
	@find . -name "*.a" -delete
	@find . -name "*.so" -delete
	@find . -name "*.dylib" -delete

distclean: clean
	@echo "Removing all generated files..."
	@rm -rf .cache
	@rm -rf compile_commands.json
	@rm -rf .clangd

# Installation
install: build
	@echo "Installing PolyCall..."
	@$(MAKE) -f $(MAKEFILE_BUILD) install \
		PREFIX="$(PREFIX)"

uninstall:
	@echo "Uninstalling PolyCall..."
	@$(MAKE) -f $(MAKEFILE_BUILD) uninstall \
		PREFIX="$(PREFIX)"

# CMake Integration
cmake-gen:
	@echo "Generating CMake configuration..."
	@mkdir -p $(BUILD_DIR)/cmake
	@cd $(BUILD_DIR)/cmake && cmake ../.. \
		-DCMAKE_BUILD_TYPE=$(BUILD_MODE) \
		-DPOLYCALL_EDGE_MICRO=$(EDGE_MICRO) \
		-DPOLYCALL_SECURITY_LEVEL=$(SECURITY_LEVEL)

# Development Commands
format:
	@echo "Formatting source code..."
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.c" -o -name "*.h" | \
		xargs clang-format -i

lint:
	@echo "Running linters..."
	@$(MAKE) -f $(MAKEFILE_SPEC) lint-all

# Documentation
docs:
	@echo "Generating documentation..."
	@doxygen docs/Doxyfile

# Help
help:
	@echo "PolyCall Build System"
	@echo "===================="
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build everything (default)"
	@echo "  build        - Build the project"
	@echo "  test         - Run tests"
	@echo "  qa           - Run full QA suite"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install the library"
	@echo "  docs         - Generate documentation"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_MODE={debug|release|profile}    (default: release)"
	@echo "  EDGE_MICRO={enabled|disabled}         (default: disabled)"
	@echo "  SECURITY_LEVEL={none|standard|paranoid} (default: standard)"
	@echo "  PREFIX=/path/to/install               (default: /usr/local)"
	@echo ""
	@echo "Edge Micro Commands:"
	@echo "  edge-deploy  - Deploy edge micro features"
	@echo ""
	@echo "Security Commands:"
	@echo "  security-audit - Run security audit"
	@echo ""
	@echo "Development:"
	@echo "  format       - Format source code"
	@echo "  lint         - Run linters"
	@echo "  vendor-test  - Test browser compatibility"

# Include dependency tracking
-include $(BUILD_DIR)/*.d