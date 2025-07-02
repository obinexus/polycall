#!/bin/bash
# integrate-build-system.sh - Connect root Makefile with codebase build
# OBINexus Waterfall Development - Build Integration Phase

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CODEBASE_DIR="$PROJECT_ROOT/libpolycall"

log_info() { echo "[INFO] $1"; }
log_error() { echo "[ERROR] $1" >&2; }
log_success() { echo "[SUCCESS] $1"; }

# Create unified Makefile
create_unified_makefile() {
    log_info "Creating unified Makefile..."
    
    cat > "$PROJECT_ROOT/Makefile" << 'EOF'
# LibPolyCall Unified Build System
# OBINexus Waterfall Development Methodology
# Version: 2.0.0

# Configuration
SHELL := /bin/bash
.DEFAULT_GOAL := help
PROJECT_ROOT := $(shell pwd)
BUILD_DIR := $(PROJECT_ROOT)/build
ADHOC_DIR := $(PROJECT_ROOT)/adhoc
SCRIPTS_DIR := $(PROJECT_ROOT)/scripts
WATERFALL_DIR := $(PROJECT_ROOT)/.waterfall
COMPLIANCE_DIR := $(PROJECT_ROOT)/.compliance

# Build configuration
CMAKE := cmake
CMAKE_BUILD_TYPE ?= Release
MAKE_JOBS := $(shell nproc 2>/dev/null || echo 4)

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
endif
ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
endif
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
endif

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m

# Help target
help:
	@echo "$(GREEN)LibPolyCall Build System - Sinphasé Governance Enabled$(NC)"
	@echo ""
	@echo "$(YELLOW)Primary Targets:$(NC)"
	@echo "  setup      - Initialize environment with ad-hoc compliance"
	@echo "  build      - Build with policy enforcement"
	@echo "  install    - Install package with governance"
	@echo "  test       - Run tests with compliance tracking"
	@echo "  clean      - Clean artifacts (preserve traces)"
	@echo ""
	@echo "$(YELLOW)Compliance Targets:$(NC)"
	@echo "  check      - Run Sinphasé governance check"
	@echo "  report     - Generate compliance report"
	@echo "  refactor   - Run enhanced refactoring"
	@echo "  validate-all - Run all validations"
	@echo ""
	@echo "$(YELLOW)Module Targets:$(NC)"
	@echo "  core-auth  - Build auth module"
	@echo "  core-edge  - Build edge module"
	@echo "  core-ffi   - Build FFI module"
	@echo "  core-micro - Build micro module"
	@echo "  core-network - Build network module"
	@echo "  core-protocol - Build protocol module"
	@echo ""
	@echo "$(YELLOW)Ad-hoc Compliance:$(NC)"
	@echo "  adhoc-init - Initialize ad-hoc framework"
	@echo "  adhoc-validate - Validate ad-hoc scripts"
	@echo "  adhoc-trace - Initialize trace system"
	@echo "  policy-inject - Inject Sinphasé policies"
	@echo ""
	@echo "$(YELLOW)Development:$(NC)"
	@echo "  dev-setup  - Full development environment"
	@echo "  dir-map    - Generate directory mappings"
	@echo "  link-all   - Link all components"
	@echo ""
	@echo "$(YELLOW)Waterfall:$(NC)"
	@echo "  waterfall-status - Show development phase status"
	@echo "  waterfall-advance - Advance to next phase"
	@echo "  tdd-cycle  - Run TDD red-green-refactor cycle"

# Setup target
setup: dev-setup adhoc-init compliance-init waterfall-init
	@echo "$(GREEN)✓ LibPolyCall environment ready$(NC)"

# Development setup
dev-setup:
	@echo "$(BLUE)Setting up development environment...$(NC)"
	@bash $(SCRIPTS_DIR)/setup.sh
	@bash $(SCRIPTS_DIR)/fix-cmake-integration.sh
	@bash $(SCRIPTS_DIR)/setup-module-layers.sh
	@echo "$(GREEN)✓ Development environment configured$(NC)"

# Build targets
build: cmake-configure
	@echo "$(BLUE)Building LibPolyCall...$(NC)"
	@$(CMAKE) --build $(BUILD_DIR) --config $(CMAKE_BUILD_TYPE) -j $(MAKE_JOBS)
	@echo "$(GREEN)✓ Build complete$(NC)"

cmake-configure:
	@echo "$(BLUE)Configuring with CMake...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DBUILD_TESTS=ON \
		-DENABLE_BENCHMARKS=OFF \
		-DENABLE_PYTHON_BINDING=ON \
		-DENABLE_SINPHASE_COMPLIANCE=ON

# Module-specific targets
core-%: cmake-configure
	@echo "$(BLUE)Building module: $*$(NC)"
	@$(CMAKE) --build $(BUILD_DIR) --target polycall_$* -j $(MAKE_JOBS)
	@echo "$(GREEN)✓ Module $* built$(NC)"

# Test targets
test: build
	@echo "$(BLUE)Running tests...$(NC)"
	@cd $(BUILD_DIR) && ctest --output-on-failure
	@$(MAKE) adhoc-test
	@echo "$(GREEN)✓ All tests passed$(NC)"

test-module-%: core-%
	@echo "$(BLUE)Testing module: $*$(NC)"
	@cd $(BUILD_DIR) && ctest -R $* --output-on-failure

# Installation
install: build
	@echo "$(BLUE)Installing LibPolyCall...$(NC)"
	@$(CMAKE) --install $(BUILD_DIR)
	@echo "$(GREEN)✓ Installation complete$(NC)"

# Cleaning
clean:
	@echo "$(BLUE)Cleaning build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@find . -name "*.o" -delete
	@find . -name "*.pyc" -delete
	@find . -name "__pycache__" -type d -exec rm -rf {} +
	@echo "$(GREEN)✓ Clean complete$(NC)"

clean-all: clean
	@rm -rf $(WATERFALL_DIR) $(COMPLIANCE_DIR)
	@echo "$(YELLOW)⚠ All project metadata removed$(NC)"

# Adhoc module targets
adhoc-init:
	@echo "$(BLUE)Initializing adhoc modules...$(NC)"
	@bash $(SCRIPTS_DIR)/setup-module-layers.sh

adhoc-build:
	@$(ADHOC_DIR)/main.sh build

adhoc-test:
	@$(ADHOC_DIR)/main.sh test

adhoc-qa:
	@$(ADHOC_DIR)/main.sh qa

adhoc-cycle:
	@$(ADHOC_DIR)/main.sh cycle

adhoc-validate: compliance-check

adhoc-trace: trace-init

# Compliance targets
check: compliance-check waterfall-check

report: compliance-report waterfall-report

refactor:
	@echo "$(BLUE)Running enhanced refactoring...$(NC)"
	@bash $(SCRIPTS_DIR)/refactor-enhanced.sh

validate-all: adhoc-validate test check
	@echo "$(GREEN)✓ All validations passed$(NC)"

# Compliance implementation
compliance-init:
	@bash $(SCRIPTS_DIR)/fix-adhoc-compliance.sh

compliance-check:
	@echo "$(BLUE)Running compliance check...$(NC)"
	@python3 $(COMPLIANCE_DIR)/generate_report.py

compliance-report: compliance-check
	@echo "$(BLUE)Compliance report generated$(NC)"

compliance-fix:
	@find adhoc -type f \( -name "*.sh" -o -name "*.py" \) -exec $(COMPLIANCE_DIR)/enforce_compliance.sh {} \;

trace-init:
	@$(COMPLIANCE_DIR)/init_trace.sh

policy-inject:
	@echo "$(BLUE)Injecting Sinphasé policies...$(NC)"
	@cp $(COMPLIANCE_DIR)/adhoc-policy.json $(ADHOC_DIR)/
	@echo "$(GREEN)✓ Policies injected$(NC)"

# Waterfall targets
waterfall-init:
	@bash $(SCRIPTS_DIR)/implement-waterfall-cycle.sh

waterfall-status:
	@./waterfall-status.sh

waterfall-check:
	@bash $(WATERFALL_DIR)/check_gate.sh $$(cat $(WATERFALL_DIR)/current_phase) all

waterfall-advance:
	@bash $(WATERFALL_DIR)/transition_phase.sh

waterfall-report:
	@phase=$$(cat $(WATERFALL_DIR)/current_phase); \
	 echo "Generating report for phase: $$phase"

# TDD targets
tdd-cycle:
	@if [ -z "$(MODULE)" ] || [ -z "$(TEST)" ]; then \
		echo "Usage: make tdd-cycle MODULE=<module> TEST=<test_file>"; \
		echo "Example: make tdd-cycle MODULE=auth TEST=tests/test_auth.c"; \
		exit 1; \
	fi
	@./tdd/tdd-cycle.sh $(MODULE) $(TEST)

# Directory mapping
dir-map:
	@bash $(SCRIPTS_DIR)/generate-dir-mapping.sh

link-all:
	@echo "$(BLUE)Linking all components...$(NC)"
	@bash $(SCRIPTS_DIR)/link-all.sh

# Development shortcuts
dev: dev-setup build test
	@echo "$(GREEN)✓ Development build complete$(NC)"

qa: test adhoc-qa compliance-check
	@echo "$(GREEN)✓ QA complete$(NC)"

full: clean dev qa validate-all
	@echo "$(GREEN)✓ Full build cycle complete$(NC)"

# Platform-specific targets
ifeq ($(PLATFORM),windows)
build-windows:
	@echo "$(BLUE)Building for Windows...$(NC)"
	@powershell -ExecutionPolicy Bypass -File $(ADHOC_DIR)/powershell/build/Setup-BuildEnvironment.ps1
	@$(MAKE) build
endif

ifeq ($(PLATFORM),linux)
build-linux:
	@echo "$(BLUE)Building for Linux...$(NC)"
	@$(MAKE) build CMAKE_BUILD_TYPE=Release
endif

ifeq ($(PLATFORM),macos)
build-macos:
	@echo "$(BLUE)Building for macOS...$(NC)"
	@$(MAKE) build CMAKE_BUILD_TYPE=Release
endif

# Phony targets
.PHONY: help setup dev-setup build cmake-configure test install clean clean-all \
        adhoc-init adhoc-build adhoc-test adhoc-qa adhoc-cycle adhoc-validate adhoc-trace \
        check report refactor validate-all compliance-init compliance-check compliance-report \
        compliance-fix trace-init policy-inject waterfall-init waterfall-status waterfall-check \
        waterfall-advance waterfall-report tdd-cycle dir-map link-all dev qa full \
        build-windows build-linux build-macos core-%

# Special variables
.EXPORT_ALL_VARIABLES:
LIBPOLYCALL_VERSION := 2.0.0
LIBPOLYCALL_ROOT := $(PROJECT_ROOT)
EOF

    log_success "Unified Makefile created"
}

# Create build orchestrator
create_build_orchestrator() {
    log_info "Creating build orchestrator..."
    
    mkdir -p "$PROJECT_ROOT/build-tools"
    
    cat > "$PROJECT_ROOT/build-tools/orchestrate.py" << 'EOF'
#!/usr/bin/env python3
"""
LibPolyCall Build Orchestrator
Coordinates build across multiple systems
"""

import os
import sys
import json
import subprocess
import argparse
from pathlib import Path
from datetime import datetime

class BuildOrchestrator:
    def __init__(self, project_root):
        self.project_root = Path(project_root)
        self.build_dir = self.project_root / "build"
        self.log_file = self.build_dir / f"build_{datetime.now():%Y%m%d_%H%M%S}.log"
        
    def run_command(self, cmd, cwd=None):
        """Run command with logging"""
        if cwd is None:
            cwd = self.project_root
            
        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True
        )
        
        # Log output
        with open(self.log_file, 'a') as f:
            f.write(f"\n=== Command: {' '.join(cmd)} ===\n")
            f.write(f"Exit Code: {result.returncode}\n")
            f.write(f"STDOUT:\n{result.stdout}\n")
            f.write(f"STDERR:\n{result.stderr}\n")
            
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            return False
            
        return True
        
    def check_prerequisites(self):
        """Check build prerequisites"""
        print("Checking prerequisites...")
        
        required_tools = ["cmake", "make", "gcc", "python3"]
        missing = []
        
        for tool in required_tools:
            if subprocess.run(["which", tool], capture_output=True).returncode != 0:
                missing.append(tool)
                
        if missing:
            print(f"Missing tools: {', '.join(missing)}")
            return False
            
        return True
        
    def configure(self, build_type="Release"):
        """Configure build with CMake"""
        print(f"Configuring build (type: {build_type})...")
        
        self.build_dir.mkdir(exist_ok=True)
        
        cmake_args = [
            "cmake",
            "..",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DBUILD_TESTS=ON",
            "-DENABLE_PYTHON_BINDING=ON",
            "-DENABLE_SINPHASE_COMPLIANCE=ON"
        ]
        
        return self.run_command(cmake_args, cwd=self.build_dir)
        
    def build(self, target=None, jobs=None):
        """Build project"""
        if jobs is None:
            jobs = os.cpu_count() or 4
            
        print(f"Building{'target: ' + target if target else ''} with {jobs} jobs...")
        
        build_args = ["cmake", "--build", ".", f"-j{jobs}"]
        if target:
            build_args.extend(["--target", target])
            
        return self.run_command(build_args, cwd=self.build_dir)
        
    def test(self):
        """Run tests"""
        print("Running tests...")
        
        # Run CTest
        if not self.run_command(["ctest", "--output-on-failure"], cwd=self.build_dir):
            return False
            
        # Run adhoc tests
        adhoc_script = self.project_root / "adhoc" / "main.sh"
        if adhoc_script.exists():
            return self.run_command([str(adhoc_script), "test"])
            
        return True
        
    def package(self):
        """Create distribution package"""
        print("Creating package...")
        
        # Create package directory
        package_dir = self.project_root / "dist"
        package_dir.mkdir(exist_ok=True)
        
        # Package metadata
        metadata = {
            "name": "libpolycall",
            "version": "2.0.0",
            "build_time": datetime.now().isoformat(),
            "platform": sys.platform,
            "modules": ["core", "auth", "network", "protocol", "edge", "micro"]
        }
        
        with open(package_dir / "metadata.json", 'w') as f:
            json.dump(metadata, f, indent=2)
            
        print(f"Package created in: {package_dir}")
        return True
        
    def full_build(self, build_type="Release"):
        """Run full build cycle"""
        print("=== LibPolyCall Full Build Cycle ===")
        
        steps = [
            ("Prerequisites", self.check_prerequisites),
            ("Configure", lambda: self.configure(build_type)),
            ("Build", self.build),
            ("Test", self.test),
            ("Package", self.package)
        ]
        
        for step_name, step_func in steps:
            print(f"\n--- {step_name} ---")
            if not step_func():
                print(f"✗ {step_name} failed")
                return False
            print(f"✓ {step_name} complete")
            
        print("\n✓ Full build cycle complete!")
        return True

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Build Orchestrator")
    parser.add_argument("command", choices=["configure", "build", "test", "package", "full"],
                       help="Build command to execute")
    parser.add_argument("--build-type", default="Release", 
                       choices=["Debug", "Release", "RelWithDebInfo"],
                       help="CMake build type")
    parser.add_argument("--target", help="Specific target to build")
    parser.add_argument("--jobs", type=int, help="Number of parallel jobs")
    
    args = parser.parse_args()
    
    project_root = Path(__file__).parent.parent
    orchestrator = BuildOrchestrator(project_root)
    
    if args.command == "configure":
        success = orchestrator.configure(args.build_type)
    elif args.command == "build":
        success = orchestrator.build(args.target, args.jobs)
    elif args.command == "test":
        success = orchestrator.test()
    elif args.command == "package":
        success = orchestrator.package()
    elif args.command == "full":
        success = orchestrator.full_build(args.build_type)
        
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
EOF
    chmod +x "$PROJECT_ROOT/build-tools/orchestrate.py"
}

# Create module linker
create_module_linker() {
    log_info "Creating module linker script..."

    SCRIPTS_DIR="$PROJECT_ROOT/scripts"
    mkdir -p "$SCRIPTS_DIR"
    
    cat > "$SCRIPTS_DIR/link-all.sh" << 'EOF'
#!/bin/bash
# Module Linker
# Links all libpolycall modules

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

link_modules() {
    echo "Linking LibPolyCall modules..."
    
    # Core modules to link
    MODULES=(
        "polycall_core"
        "polycall_auth" 
        "polycall_network"
        "polycall_protocol"
        "polycall_edge"
        "polycall_micro"
        "polycall_ffi"
    )
    
    # Create consolidated library
    if [ -d "$BUILD_DIR" ]; then
        cd "$BUILD_DIR"
        
        # Collect all object files
        OBJECTS=""
        for module in "${MODULES[@]}"; do
            if [ -f "lib${module}.a" ]; then
                echo "  Found: lib${module}.a"
                OBJECTS="$OBJECTS lib${module}.a"
            fi
        done
        
        if [ -n "$OBJECTS" ]; then
            echo "Creating consolidated library..."
            ar -x $OBJECTS
            ar -rcs libpolycall_full.a *.o
            rm -f *.o
            echo "✓ Created: libpolycall_full.a"
        fi
    fi
}

link_modules
EOF
    chmod +x "$SCRIPTS_DIR/link-all.sh"
}

# Create integration test
create_integration_test() {
    log_info "Creating build integration test..."
    
    mkdir -p "$PROJECT_ROOT/tests/integration"
    
    cat > "$PROJECT_ROOT/tests/integration/test_build_integration.sh" << 'EOF'
#!/bin/bash
# Build System Integration Test

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

echo "=== Build System Integration Test ==="

# Test 1: Makefile targets exist
echo -n "Test 1: Checking Makefile targets... "
targets=("setup" "build" "test" "clean" "adhoc-cycle" "waterfall-status")
for target in "${targets[@]}"; do
    if ! make -n "$target" >/dev/null 2>&1; then
        echo "FAILED (missing target: $target)"
        exit 1
    fi
done
echo "PASSED"

# Test 2: CMake configuration
echo -n "Test 2: Testing CMake configuration... "
if make cmake-configure >/dev/null 2>&1; then
    echo "PASSED"
else
    echo "FAILED"
    exit 1
fi

# Test 3: Module build targets
echo -n "Test 3: Testing module targets... "
if make -n core-auth >/dev/null 2>&1; then
    echo "PASSED"
else
    echo "FAILED"
    exit 1
fi

# Test 4: Adhoc integration
echo -n "Test 4: Testing adhoc integration... "
if [ -x "$PROJECT_ROOT/adhoc/main.sh" ]; then
    echo "PASSED"
else
    echo "FAILED"
    exit 1
fi

# Test 5: Build orchestrator
echo -n "Test 5: Testing build orchestrator... "
if [ -x "$PROJECT_ROOT/build-tools/orchestrate.py" ]; then
    echo "PASSED"
else
    echo "FAILED"
    exit 1
fi

echo ""
echo "✓ All integration tests passed!"
EOF
    chmod +x "$PROJECT_ROOT/tests/integration/test_build_integration.sh"
}

# Main execution
main() {
    log_info "Integrating build system for libpolycall..."
    
    # Create unified Makefile
    create_unified_makefile
    
    # Create build orchestrator
    create_build_orchestrator
    
    # Create module linker
    create_module_linker
    
    # Create integration test
    create_integration_test
    
    # Run integration test
    if "$PROJECT_ROOT/tests/integration/test_build_integration.sh"; then
        log_success "Build system integration complete!"
    else
        log_error "Integration test failed"
        exit 1
    fi
    
    echo ""
    log_info "Quick start commands:"
    echo "  make setup     # Initialize complete environment"
    echo "  make build     # Build project"
    echo "  make test      # Run all tests"
    echo "  make dev       # Development build + test"
    echo "  make full      # Complete build cycle"
    echo ""
    log_info "Or use the orchestrator:"
    echo "  ./build-tools/orchestrate.py full"
}

main "$@"
