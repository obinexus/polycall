# OBINexus LibPolyCall - Hybrid Build System Makefile
# Combines Python packaging with CMake C/C++ builds
# Maintains Sinphasé governance enforcement

.PHONY: all install build clean test check report refactor help

# Default target
all: help

# Help message
help:
	@echo "LibPolyCall Build System Commands:"
	@echo "  install    - Install Python package in editable mode"
	@echo "  build      - Build C/C++ components via CMake"
	@echo "  clean      - Remove build artifacts and caches"
	@echo "  test       - Run Python test suite"
	@echo "  check      - Run Sinphasé governance check"
	@echo "  report     - Generate governance report"
	@echo "  refactor   - Run refactoring (dry-run)"
	@echo "  full       - Run full build pipeline (clean, build, install, test)"

# Python package installation
install:
	@echo "Installing LibPolyCall Python package..."
	@pip install -e .
	@echo "Installation complete."

# CMake build for C/C++ components
build:
	@echo "Building C/C++ components..."
	@cmake -B build/ -S . -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/ --parallel
	@echo "Build complete."

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build/ dist/ *.egg-info
	@rm -rf __pycache__ .pytest_cache
	@rm -rf sinphase_governance.egg-info
	@find . -type d -name "__pycache__" -exec rm -rf {} +
	@find . -type f -name "*.pyc" -delete
	@find . -type f -name "*.pyo" -delete
	@find . -type f -name "*.so" -delete
	@find . -type f -name "*.dylib" -delete
	@echo "Clean complete."

# Run test suite
test:
	@echo "Running test suite..."
	@pytest tests/ -v

# Sinphasé governance check
check:
	@echo "Running Sinphasé governance check..."
	@python -m sinphase_governance check

# Generate governance report
report:
	@echo "Generating governance report..."
	@python -m sinphase_governance report

# Run refactoring
refactor:
	@echo "Running refactoring (dry-run)..."
	@bash refactor.sh --dry-run

# Full build pipeline
full: clean build install test
	@echo "Full build pipeline complete."

# Development setup
dev-setup:
	@echo "Setting up development environment..."
	@pip install -e ".[dev]"
	@pre-commit install
	@echo "Development setup complete."

# Build Python wheel
wheel:
	@echo "Building Python wheel..."
	@pip install --upgrade build
	@python -m build
	@echo "Wheel build complete."

# Build documentation
docs:
	@echo "Building documentation..."
	@cd docs && make html
	@echo "Documentation build complete."

# CMake specific targets
cmake-debug:
	@cmake -B build-debug/ -S . -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build-debug/ --parallel

cmake-test:
	@cd build && ctest --output-on-failure

# Platform-specific builds
build-linux:
	@cmake -B build-linux/ -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux
	@cmake --build build-linux/ --parallel

build-windows:
	@cmake -B build-windows/ -S . -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
	@cmake --build build-windows/ --parallel

# Isolated FFI/Protocol builds (Sinphasé enforcement)
build-ffi:
	@echo "Building isolated FFI components..."
	@cmake --build build/ --target root-dynamic-c
	@echo "FFI build complete."