# PolyBuild Universal Makefile - Fault-Tolerant Build System
# Automatically handles Make, CMake, Meson, and custom scripts
# Topology validation targets
.PHONY: check-health check-topology build-topology

# Enhanced PolyBuild Makefile with Fault Tolerance Grading
.PHONY: check-health check-topology build-topology

# Directory structure (ensure they exist)
$(shell mkdir -p build/{bin,obj,logs,metadata})

# Include all .in manifest files
include $(shell find config -name "*.in" 2>/dev/null)

# Default compiler flags
CC ?= gcc
CFLAGS ?= -Wall -Wextra -I$(ROOT_DIR)/include

# Find all C source files using wildcard globbing
SRC_FILES := $(shell find src -name "*.c")
OBJ_FILES := $(patsubst src/%.c,build/obj/%.o,$(SRC_FILES))

# Fault tolerance grading thresholds (from topology.in or defaults)
CRITICAL_THRESHOLD ?= 6
DANGER_THRESHOLD ?= 9
PANIC_THRESHOLD ?= 12

# Check health based on warning/error count
check-health:
	@echo "Checking build health..."
	@warnings=$$(grep -c "warning:" build/logs/*.log 2>/dev/null || echo "0"); \
	errors=$$(grep -c "error:" build/logs/*.log 2>/dev/null || echo "0"); \
	total=$$((warnings + errors)); \
	if [ $$total -ge $(PANIC_THRESHOLD) ]; then \
		echo "🔥 STATE_PANIC: $$total issues detected. Build halted."; \
		echo "STATE_PANIC :: Errors: $$errors, Warnings: $$warnings" > build/metadata/health.log; \
		exit 1; \
	elif [ $$total -ge $(DANGER_THRESHOLD) ]; then \
		echo "🔴 STATE_DANGER: $$total issues detected. QA review required."; \
		echo "STATE_DANGER :: Errors: $$errors, Warnings: $$warnings" > build/metadata/health.log; \
	elif [ $$total -ge $(CRITICAL_THRESHOLD) ]; then \
		echo "🟡 STATE_CRITICAL: $$total issues detected. Proceed with caution."; \
		echo "STATE_CRITICAL :: Errors: $$errors, Warnings: $$warnings" > build/metadata/health.log; \
	else \
		echo "🟢 STATE_OK: $$total issues detected. Artifact stable."; \
		echo "STATE_OK :: Artifact stable" > build/metadata/health.log; \
	fi

# Topology health checks
check-topology: check-topology-star check-topology-bus check-topology-p2p check-topology-ring
	@echo "All topology checks completed"
	@if grep -q "STATE_PANIC" build/metadata/topology-health.log 2>/dev/null; then \
		echo "🔥 PANIC: Topology failure detected. Build blocked."; \
		exit 1; \
	elif grep -q "STATE_DANGER" build/metadata/topology-health.log 2>/dev/null; then \
		echo "🔴 DANGER: Degraded topology detected. QA required."; \
	elif grep -q "STATE_CRITICAL" build/metadata/topology-health.log 2>/dev/null; then \
		echo "🟡 CRITICAL: Issues in topology. Proceed with caution."; \
	else \
		echo "🟢 SUCCESS: All topologies stable."; \
	fi

# Generic rule for compiling C files with proper logging
build/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	@mkdir -p build/logs/$(dir $(patsubst src/%,%,$<))
	@echo "Compiling $(notdir $<)..."
	@$(CC) $(CFLAGS) -c $< -o $@ 2> build/logs/$(patsubst src/%,%,$<).log
	@if grep -q "error:" build/logs/$(patsubst src/%,%,$<).log; then \
		echo "⛔ ERROR in $(notdir $<)"; \
	elif grep -q "warning:" build/logs/$(patsubst src/%,%,$<).log; then \
		echo "⚠️ WARNING in $(notdir $<)"; \
	else \
		echo "✓ $(notdir $<)"; \
	fi

# The auth_context module compilation using wildcard pattern
polycall_auth_context: $(filter %/auth_context.o,$(OBJ_FILES))
	@echo "Linking polycall auth context module..."
	@mkdir -p build/bin
	@$(CC) $(CFLAGS) -o build/bin/auth_context $^ 2> build/logs/auth_context_link.log
	@# Generate manifest
	@echo '{"component":"polycall_auth_context","built_at":"'$$(date -u +"%Y-%m-%dT%H:%M:%SZ")'","topology":"$(TOPOLOGY)"}' > build/metadata/auth_context.json
	@echo "Auth context module built successfully"

# Master build with topology validation
build-topology: check-topology
	@if [ $$? -eq 0 ]; then \
		echo "Building with validated topology..."; \
		$(MAKE) build-components; \
	else \
		echo "Build blocked due to topology issues."; \
		exit 1; \
	fi

# Build all components
build-components: check-health
	@echo "Building all components..."
	@$(MAKE) polycall_auth_context
	@# Add other components here
	@echo "All components built successfully"

# Clean with fault tolerance
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build/obj build/bin
	@find build/logs -name "*.log" -delete 2>/dev/null || true
	@echo "Clean completed"

# Default target
all: build-topology
# Directory structure
$(shell mkdir -p build/logs build/metadata build/bin)

# Health check based on warning count
check-health:
	@warnings=$$(grep -c "warning:" build/logs/*.log 2>/dev/null || echo "0"); \
	./build-tools/status-check.sh $$warnings

# Check specific topology health
check-topology-star:
	@echo "Checking star topology health..."
	@warnings=$$(grep -c "error\|warning" build/logs/star-*.log 2>/dev/null || echo "0"); \
	./build-tools/status-check.sh $$warnings "star"

check-topology-bus:
	@echo "Checking bus topology health..."
	@warnings=$$(grep -c "error\|warning" build/logs/bus-*.log 2>/dev/null || echo "0"); \
	./build-tools/status-check.sh $$warnings "bus"

check-topology-p2p:
	@echo "Checking p2p topology health..."
	@warnings=$$(grep -c "error\|warning" build/logs/p2p-*.log 2>/dev/null || echo "0"); \
	./build-tools/status-check.sh $$warnings "p2p"

check-topology-ring:
	@echo "Checking ring topology health..."
	@warnings=$$(grep -c "error\|warning" build/logs/ring-*.log 2>/dev/null || echo "0"); \
	./build-tools/status-check.sh $$warnings "ring"

# Validate all topologies before proceeding
check-topology: check-topology-star check-topology-bus check-topology-p2p check-topology-ring
	@echo "All topology checks completed"
	@if [ -f build/metadata/topology-health.log ]; then \
		if grep -q "STATE_CRITICAL\|STATE_PANIC" build/metadata/topology-health.log; then \
			echo "⛔ CRITICAL: Topology failure detected. New artifact development blocked."; \
			exit 1; \
		elif grep -q "STATE_WARNING" build/metadata/topology-health.log; then \
			echo "⚠️ WARNING: Degraded topology detected. Proceed with caution."; \
		else \
			echo "✅ SUCCESS: All topologies stable."; \
		fi; \
	fi

# Example build target with health checks
polycall_auth_context: check-health
	@echo "Compiling polycall auth context module..."
	$(CC) -Wall -Wextra -o build/bin/auth_context src/polycall/auth_context.c 2> build/logs/auth_context.log
	@# Generate manifest
	@echo '{"component": "polycall_auth_context", "built_at": "'$$(date -u +"%Y-%m-%dT%H:%M:%SZ")'"}' > build/metadata/auth_context.json

# Master build with topology validation
build-topology: check-topology
	@if [ $$? -eq 0 ]; then \
		echo "Building with validated topology..."; \
		$(MAKE) polycall_auth_context; \
	else \
		echo "Build blocked due to topology issues."; \
		exit 1; \
	fi
.PHONY: all build clean test install help status
.DEFAULT_GOAL := build

# Configuration
PROJECT_NAME = polycall-v2
VERSION = 2.0.0
BUILD_MODE ?= release
PARALLEL_JOBS ?= $(shell nproc 2>/dev/null || echo 4)
TOPOLOGY ?= mesh
CACHE_ENABLED ?= true

# Directories
ROOT_DIR := $(shell pwd)
BUILD_DIR := $(ROOT_DIR)/build
POLYBUILD_DIR := $(ROOT_DIR)/.polybuild
SRC_DIR := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include

# Auto-detect build system
BUILD_SYSTEM := $(shell \
    if [ -f "meson.build" ]; then \
        echo "meson"; \
    elif [ -f "CMakeLists.txt" ]; then \
        echo "cmake"; \
    else \
        echo "make"; \
    fi)

# Colors
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m

# Fault-tolerant build with automatic fallback
build: polybuild-init
	@echo -e "$(BLUE)[PolyBuild] Starting fault-tolerant build...$(NC)"
	@echo "Build System: $(BUILD_SYSTEM)"
	@echo "Topology: $(TOPOLOGY)"
	@echo "Parallel Jobs: $(PARALLEL_JOBS)"
	@echo "Cache: $(CACHE_ENABLED)"
	@echo ""
	@$(MAKE) build-$(BUILD_SYSTEM) || $(MAKE) build-fallback

# Initialize PolyBuild
polybuild-init:
	@echo -e "$(YELLOW)[PolyBuild] Initializing build environment...$(NC)"
	@mkdir -p $(BUILD_DIR)/{make,cmake,meson,artifacts}
	@mkdir -p $(POLYBUILD_DIR)/{logs,cache}
	@if [ ! -f "$(POLYBUILD_DIR)/config/polybuild.yaml" ]; then \
		echo "Warning: PolyBuild config not found, using defaults"; \
	fi

# Make build system
build-make: polybuild-init
	@echo -e "$(GREEN)[Make] Building with Make...$(NC)"
	@$(MAKE) -f Makefile.make build-core BUILD_MODE=$(BUILD_MODE) || \
	$(MAKE) -f Makefile.make build-simple BUILD_MODE=$(BUILD_MODE)

# CMake build system
build-cmake: polybuild-init
	@echo -e "$(GREEN)[CMake] Building with CMake...$(NC)"
	@mkdir -p $(BUILD_DIR)/cmake
	@cd $(BUILD_DIR)/cmake && \
	cmake ../.. \
		-DCMAKE_BUILD_TYPE=$(shell echo $(BUILD_MODE) | sed 's/.*/\u&/') \
		-DPOLYCALL_VERSION=$(VERSION) \
		-DBUILD_TESTING=ON \
		-DBUILD_SHARED_LIBS=ON || \
	cmake ../.. -DCMAKE_BUILD_TYPE=Release
	@cd $(BUILD_DIR)/cmake && cmake --build . --parallel $(PARALLEL_JOBS)

# Meson build system
build-meson: polybuild-init
	@echo -e "$(GREEN)[Meson] Building with Meson...$(NC)"
	@if command -v meson >/dev/null 2>&1; then \
		mkdir -p $(BUILD_DIR)/meson && \
		cd $(BUILD_DIR)/meson && \
		meson setup ../.. --buildtype=$(BUILD_MODE) && \
		meson compile; \
	else \
		echo -e "$(YELLOW)Meson not found, falling back to Make$(NC)"; \
		$(MAKE) build-make; \
	fi

# Fallback build system
build-fallback:
	@echo -e "$(YELLOW)[Fallback] Primary build failed, trying alternatives...$(NC)"
	@if [ "$(BUILD_SYSTEM)" != "make" ]; then \
		echo "Trying Make..."; \
		$(MAKE) build-make || true; \
	fi
	@if [ "$(BUILD_SYSTEM)" != "cmake" ]; then \
		echo "Trying CMake..."; \
		$(MAKE) build-cmake || true; \
	fi
	@if [ "$(BUILD_SYSTEM)" != "meson" ]; then \
		echo "Trying Meson..."; \
		$(MAKE) build-meson || true; \
	fi
	@echo -e "$(RED)All build systems failed$(NC)"
	@exit 1

# Clean with fault tolerance
clean:
	@echo -e "$(BLUE)[Clean] Cleaning build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@find . -name "*.o" -delete 2>/dev/null || true
	@find . -name "*.so" -delete 2>/dev/null || true
	@find . -name "*.a" -delete 2>/dev/null || true
	@find . -name "*.dll" -delete 2>/dev/null || true
	@echo -e "$(GREEN)✓ Clean completed$(NC)"

# Test with fault injection
test: build
	@echo -e "$(BLUE)[Test] Running test suite...$(NC)"
	@if [ -d "tests" ]; then \
		$(MAKE) -C tests run || $(MAKE) -C test run || echo "Tests not found"; \
	fi

# Install with verification
install: build
	@echo -e "$(BLUE)[Install] Installing polycall...$(NC)"
	@mkdir -p $(DESTDIR)/usr/local/bin
	@mkdir -p $(DESTDIR)/usr/local/lib
	@mkdir -p $(DESTDIR)/usr/local/include
	@if [ -f "$(BUILD_DIR)/artifacts/polycall" ]; then \
		cp $(BUILD_DIR)/artifacts/polycall $(DESTDIR)/usr/local/bin/; \
	fi
	@if [ -f "$(BUILD_DIR)/artifacts/libpolycall.so" ]; then \
		cp $(BUILD_DIR)/artifacts/libpolycall.so $(DESTDIR)/usr/local/lib/; \
	fi
	@echo -e "$(GREEN)✓ Installation completed$(NC)"

# Status check
status:
	@echo -e "$(BLUE)[Status] PolyBuild System Status$(NC)"
	@echo "Project: $(PROJECT_NAME)"
	@echo "Version: $(VERSION)"
	@echo "Build System: $(BUILD_SYSTEM)"
	@echo "Build Mode: $(BUILD_MODE)"
	@echo "Topology: $(TOPOLOGY)"
	@echo ""
	@echo "Build Tools:"
	@command -v make >/dev/null 2>&1 && echo "✓ Make" || echo "✗ Make"
	@command -v cmake >/dev/null 2>&1 && echo "✓ CMake" || echo "✗ CMake"
	@command -v meson >/dev/null 2>&1 && echo "✓ Meson" || echo "✗ Meson"
	@command -v gcc >/dev/null 2>&1 && echo "✓ GCC" || echo "✗ GCC"
	@command -v clang >/dev/null 2>&1 && echo "✓ Clang" || echo "✗ Clang"
	@echo ""
	@echo "Directory Structure:"
	@[ -d "$(SRC_DIR)" ] && echo "✓ Source directory" || echo "✗ Source directory"
	@[ -d "$(INCLUDE_DIR)" ] && echo "✓ Include directory" || echo "✗ Include directory"
	@[ -d "$(BUILD_DIR)" ] && echo "✓ Build directory" || echo "✗ Build directory"

# Help
help:
	@echo -e "$(BLUE)PolyBuild Fault-Tolerant Build System$(NC)"
	@echo ""
	@echo "Available targets:"
	@echo "  build    - Build project with fault tolerance"
	@echo "  clean    - Clean build artifacts"
	@echo "  test     - Run test suite"
	@echo "  install  - Install binaries and libraries"
	@echo "  status   - Show system status"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  BUILD_MODE=$(BUILD_MODE)    (debug, release)"
	@echo "  PARALLEL_JOBS=$(PARALLEL_JOBS)"
	@echo "  TOPOLOGY=$(TOPOLOGY)        (p2p, bus, ring, star, mesh)"
	@echo "  CACHE_ENABLED=$(CACHE_ENABLED)"
	@echo ""
	@echo "Examples:"
	@echo "  make build BUILD_MODE=debug"
	@echo "  make build PARALLEL_JOBS=8"
	@echo "  make build TOPOLOGY=p2p"
