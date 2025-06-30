# LibPolyCall Test Makefile
# Orchestrates the discovery, building, and execution of unit tests

# Default target
.PHONY: all
all: test

# Get project root directory (parent of this directory)
PROJECT_ROOT := $(shell realpath ..)
BUILD_DIR := $(PROJECT_ROOT)/build
TEST_BUILD_DIR := $(BUILD_DIR)/tests

# Configure tests
.PHONY: configure
configure:
	@echo "Configuring tests..."
	@python3 $(PROJECT_ROOT)/scripts/tests/polycall_unit_config.py \
		--project-root=$(PROJECT_ROOT) \
		--build-dir=$(BUILD_DIR)

# Build tests
.PHONY: build
build: configure
	@echo "Building tests..."
	@cmake --build $(TEST_BUILD_DIR) --target unit_tests

# Run tests
.PHONY: test
test: build
	@echo "Running tests..."
	@cmake --build $(TEST_BUILD_DIR) --target run_unit_tests

# Run specific test module
.PHONY: test-%
test-%: build
	@echo "Running tests for module $*..."
	@find $(TEST_BUILD_DIR) -name "*$**" -type f -executable -exec {} \;

# Clean tests
.PHONY: clean
clean:
	@echo "Cleaning tests..."
	@rm -rf $(TEST_BUILD_DIR)

# Show help
.PHONY: help
help:
	@echo "LibPolyCall Test Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build and run all tests (default)"
	@echo "  configure  - Generate test build configuration"
	@echo "  build      - Build tests"
	@echo "  test       - Run all tests"
	@echo "  test-<mod> - Run tests for specific module"
	@echo "  clean      - Clean test build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make test              - Run all tests"
	@echo "  make test-memory       - Run memory tests"
	@echo "  make test-core_polycall - Run core polycall tests"