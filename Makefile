# OBINexus LibPolyCall - Enhanced Build System with Ad-hoc Compliance
# Sinphasé Governance Enforcement with Policy Injection
# Aegis Project Phase 2 - Zero-Trust Architecture

.PHONY: all install build clean test check report refactor help setup
.PHONY: adhoc-init adhoc-validate adhoc-trace policy-inject
.PHONY: dir-map compile-core compile-cli link-all

# Build Configuration
BUILD_DIR ?= build
INSTALL_PREFIX ?= /usr/local
CMAKE_BUILD_TYPE ?= Release
ADHOC_DIR = scripts/adhoc
POLICY_DIR = scripts/policies
TRACE_DIR = logs/trace

# Compiler settings
CC ?= gcc
CXX ?= g++
AR ?= ar
CFLAGS = -Wall -Wextra -O2 -fPIC -I./include
LDFLAGS = -L./lib

# Core library settings
LIBPOLYCALL_STATIC = lib/libpolycall.a
LIBPOLYCALL_SHARED = lib/libpolycall.so
POLYCALL_BIN = bin/polycall

# Default target
all: help

# Setup target - Initialize environment with ad-hoc compliance
setup: adhoc-init policy-inject dir-map
	@echo "=== LibPolyCall Setup with Ad-hoc Compliance ==="
	@echo "Initializing Sinphasé policy enforcement..."
	@mkdir -p $(BUILD_DIR) $(ADHOC_DIR) $(POLICY_DIR) $(TRACE_DIR)
	@mkdir -p lib bin logs
	
	@echo "Running setup.sh with policy injection..."
	@bash $(POLICY_DIR)/policy-wrapper.sh ./setup.sh
	
	@echo "Executing ad-hoc compliant scripts..."
	@find scripts -name "fix_*.py" -o -name "fix_*.sh" | while read script; do \
		bash $(ADHOC_DIR)/adhoc-execute.sh "$$script"; \
	done
	
	@echo "Building directory mappings..."
	@bash scripts/generate-dir-mappings.sh
	
	@echo "Setup complete with Sinphasé governance."

# Ad-hoc compliance initialization
adhoc-init:
	@echo "Initializing ad-hoc compliance framework..."
	@bash scripts/adhoc/adhoc-init.sh
	@touch $(ADHOC_DIR)/.initialized

# Ad-hoc validation
adhoc-validate:
	@echo "Validating ad-hoc scripts..."
	@bash scripts/adhoc/adhoc-validator.sh

# Ad-hoc trace system
adhoc-trace:
	@echo "Initializing trace system..."
	@bash scripts/adhoc/tracer-root.sh init

# Policy injection system
policy-inject:
	@echo "Injecting Sinphasé policies..."
	@bash scripts/policies/policy-injector.sh

# Directory mapping for include->src
dir-map:
	@echo "Generating directory mappings..."
	@bash scripts/generate-dir-mappings.sh
	@echo "Validating mappings..."
	@bash scripts/validate-mappings.sh

# Core library compilation
compile-core: dir-map
	@echo "Compiling core library..."
	@mkdir -p obj/core
	
	# Compile all core modules
	@for module in polycall auth accessibility config edge ffi micro network protocol telemetry; do \
		echo "Building module: $$module"; \
		find src/core/$$module -name "*.c" -exec $(CC) $(CFLAGS) -c {} -o obj/core/$$(basename {} .c).o \; ; \
	done
	
	# Create static library
	@$(AR) rcs $(LIBPOLYCALL_STATIC) obj/core/*.o
	@echo "Static library created: $(LIBPOLYCALL_STATIC)"
	
	# Create shared library
	@$(CC) -shared -o $(LIBPOLYCALL_SHARED) obj/core/*.o $(LDFLAGS)
	@echo "Shared library created: $(LIBPOLYCALL_SHARED)"

# CLI compilation
compile-cli: compile-core
	@echo "Compiling CLI..."
	@mkdir -p obj/cli
	
	# Compile CLI components
	@find src/cli -name "*.c" -exec $(CC) $(CFLAGS) -c {} -o obj/cli/$$(basename {} .c).o \; ;
	
	# Link CLI executable
	@$(CC) -o $(POLYCALL_BIN) obj/cli/*.o -L./lib -lpolycall $(LDFLAGS)
	@echo "CLI executable created: $(POLYCALL_BIN)"

# Full linking process
link-all: compile-cli
	@echo "Linking all components..."
	@bash scripts/link-validator.sh
	@echo "Linking complete."

# Enhanced build with ad-hoc compliance
build: adhoc-validate compile-core compile-cli link-all
	@echo "Building with ad-hoc compliance..."
	@$(ADHOC_DIR)/adhoc-execute.sh "cmake -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)"
	@$(ADHOC_DIR)/adhoc-execute.sh "cmake --build $(BUILD_DIR) --parallel"
	@echo "Build complete with policy enforcement."

# Enhanced clean with trace preservation
clean:
	@echo "Cleaning build artifacts (preserving traces)..."
	@rm -rf $(BUILD_DIR) obj lib/libpolycall.* bin/polycall
	@rm -rf dist/ *.egg-info __pycache__ .pytest_cache
	@find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@find . -type f -name "*.pyc" -delete 2>/dev/null || true
	@find . -type f -name "*.pyo" -delete 2>/dev/null || true
	@find . -type f -name "*.so" -delete 2>/dev/null || true
	@find . -type f -name "*.dylib" -delete 2>/dev/null || true
	@echo "Clean complete (traces preserved in $(TRACE_DIR))."

# Python package installation with policy
install: build
	@echo "Installing with Sinphasé governance..."
	@$(ADHOC_DIR)/adhoc-execute.sh "pip install -e ."
	@echo "Installation complete."

# Test execution with compliance
test: build
	@echo "Running tests with compliance tracking..."
	@$(ADHOC_DIR)/adhoc-execute.sh "pytest tests/ -v"
	@bash scripts/test-compliance-report.sh

# Sinphasé governance check
check:
	@echo "Running Sinphasé governance check..."
	@python -m sinphase_governance check
	@bash scripts/adhoc/compliance-check.sh

# Enhanced governance report
report:
	@echo "Generating comprehensive governance report..."
	@python -m sinphase_governance report
	@bash scripts/generate-compliance-report.sh > reports/compliance-$(shell date +%Y%m%d).md

# Enhanced refactoring with policy
refactor:
	@echo "Running refactoring with Sinphasé policies..."
	@bash scripts/refactor-enhanced.sh --policy-mode

# Development setup with full compliance
dev-setup: setup
	@echo "Setting up development environment..."
	@pip install -e ".[dev]"
	@pre-commit install
	@bash scripts/dev-hooks-install.sh
	@echo "Development setup complete."

# Core module targets
core-auth:
	@$(MAKE) compile-module MODULE=auth

core-edge:
	@$(MAKE) compile-module MODULE=edge

core-ffi:
	@$(MAKE) compile-module MODULE=ffi

core-micro:
	@$(MAKE) compile-module MODULE=micro

core-network:
	@$(MAKE) compile-module MODULE=network

core-protocol:
	@$(MAKE) compile-module MODULE=protocol

# Generic module compilation
compile-module:
	@echo "Compiling module: $(MODULE)"
	@mkdir -p obj/core/$(MODULE)
	@find src/core/$(MODULE) -name "*.c" -exec $(CC) $(CFLAGS) -c {} -o obj/core/$(MODULE)/$$(basename {} .c).o \;

# Platform-specific builds with compliance
build-linux: adhoc-validate
	@CMAKE_ARGS="-DCMAKE_SYSTEM_NAME=Linux" $(MAKE) build

build-windows: adhoc-validate
	@CMAKE_ARGS="-G 'MinGW Makefiles'" $(MAKE) build

build-macos: adhoc-validate
	@CMAKE_ARGS="-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15" $(MAKE) build

# Compliance validation targets
validate-structure:
	@echo "Validating project structure..."
	@bash scripts/validate-structure.sh

validate-includes:
	@echo "Validating include paths..."
	@python scripts/validate_include_paths.py

validate-policies:
	@echo "Validating Sinphasé policies..."
	@bash scripts/policies/validate-policies.sh

# Full validation suite
validate-all: validate-structure validate-includes validate-policies adhoc-validate
	@echo "All validations passed."

# Help with enhanced information
help:
	@echo "LibPolyCall Build System - Sinphasé Governance Enabled"
	@echo ""
	@echo "Primary Targets:"
	@echo "  setup      - Initialize environment with ad-hoc compliance"
	@echo "  build      - Build with policy enforcement"
	@echo "  install    - Install package with governance"
	@echo "  test       - Run tests with compliance tracking"
	@echo "  clean      - Clean artifacts (preserve traces)"
	@echo ""
	@echo "Compliance Targets:"
	@echo "  check      - Run Sinphasé governance check"
	@echo "  report     - Generate compliance report"
	@echo "  refactor   - Run enhanced refactoring"
	@echo "  validate-all - Run all validations"
	@echo ""
	@echo "Module Targets:"
	@echo "  core-auth  - Build auth module"
	@echo "  core-edge  - Build edge module"
	@echo "  core-ffi   - Build FFI module"
	@echo "  core-micro - Build micro module"
	@echo "  core-network - Build network module"
	@echo "  core-protocol - Build protocol module"
	@echo ""
	@echo "Ad-hoc Compliance:"
	@echo "  adhoc-init - Initialize ad-hoc framework"
	@echo "  adhoc-validate - Validate ad-hoc scripts"
	@echo "  adhoc-trace - Initialize trace system"
	@echo "  policy-inject - Inject Sinphasé policies"
	@echo ""
	@echo "Development:"
	@echo "  dev-setup  - Full development environment"
	@echo "  dir-map    - Generate directory mappings"
	@echo "  link-all   - Link all components"

# Continuous compliance monitoring
monitor:
	@echo "Starting compliance monitoring..."
	@bash scripts/compliance-monitor.sh &

# Stop monitoring
monitor-stop:
	@echo "Stopping compliance monitoring..."
	@pkill -f compliance-monitor.sh || true

.EXPORT_ALL_VARIABLES: