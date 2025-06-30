# LibPolyCall Unit Testing Guide

This document provides comprehensive guidance on the unit testing framework used in LibPolyCall, how to write tests, and how to run them.

## Testing Framework Overview

LibPolyCall uses a dual-mode testing framework:

1. **Check Framework**: When available, the [Check](https://libcheck.github.io/check/) unit testing framework is used for advanced testing capabilities.
2. **Minimal Framework**: A lightweight custom framework (`unit_test_framework.h`) serves as a fallback when the Check library is not available.

## Directory Structure

```
tests/
├── core/                  # Core module tests
│   ├── polycall/          # Polycall core tests
│   │   ├── test_context.c # Context management tests
│   │   ├── test_error.c   # Error handling tests
│   │   ├── test_memory.c  # Memory management tests
│   │   ├── test_config.c  # Configuration system tests
│   │   └── test_polycall_core.c   # Main test runner
├── common/
│   └── unit_test_framework.h      # Minimal testing framework
└── CMakeLists.txt         # Main test configuration
```

## Test Organization

Tests are organized by module, with each core component having its own dedicated test file. The source files are compiled into an object library for build efficiency.

## Writing Tests

Each test file should include:
- Test cases for specific functionality
- Edge cases and error conditions
- Setup and teardown procedures

## Building and Running Tests

### Configuration
```bash
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
```

### Building Tests
```bash
make all_tests
```

### Running Tests
```bash
# Run all tests
make run_tests

# Using CTest directly
ctest --output-on-failure

# Run a specific test
./tests/core/polycall/test_polycall_core
```

## Test Modules

### Context Tests
Tests initialization, data access, flags, locking, sharing, and listeners

### Error Tests
Tests error setting, retrieval, callbacks, and macros

### Memory Tests
Tests pool creation, allocation, reallocation, regions, permissions, and statistics

### Config Tests
Tests value types, notifications, providers, and enumeration
