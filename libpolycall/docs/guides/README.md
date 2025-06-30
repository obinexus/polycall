# LibPolyCall Testing Framework

## Overview
Comprehensive testing infrastructure for the LibPolyCall cross-language communication framework, following the Arrange-Act-Assert (AAA) pattern and Test-Driven Development (TDD) methodology.

## Test Structure

### Test Categories

#### 1. Unit Tests (`/unit/`)
- **Purpose**: Test individual module functionality
- **Scope**: Single module components
- **Pattern**: Arrange-Act-Assert
- **Focus**: Correctness of individual functions and components

#### 2. Unit QA Tests (`/unit_qa/`)
- **Purpose**: Test module resilience and error handling
- **Scope**: Single module components under stress
- **Pattern**: Arrange-Act-Assert
- **Focus**: Error paths, resource management, memory cleanup

#### 3. Integration Tests (`/integration/`)
- **Purpose**: Test cross-module communication
- **Scope**: Multiple modules working together
- **Pattern**: Arrange-Act-Assert
- **Focus**: Module interaction and data flow

#### 4. Integration QA Tests (`/integration_qa/`)
- **Purpose**: Test system-wide resilience
- **Scope**: Multiple modules under stress conditions
- **Pattern**: Arrange-Act-Assert
- **Focus**: Error propagation, performance, resource coordination

## Module Coverage

### Core Modules
- accessibility: Audio/visual accessibility features
- auth: Authentication and authorization
- config: Configuration management
- edge: Edge computing capabilities
- ffi: Foreign Function Interface
- micro: Microservice components
- network: Network communication
- polycall: Core runtime engine
- polycallfile: Configuration file parsing
- polycallrc: Runtime configuration
- protocol: Communication protocols
- repl: Read-Eval-Print Loop
- schema: Configuration schema validation
- security: Security framework
- socket: WebSocket implementation
- telemetry: Monitoring and analytics

### CLI Modules
- All core modules with command-line interfaces

## Running Tests

### Build and Run All Tests
```bash
cd tests
./scripts/run_tests.sh
```

### Build Tests Only
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make
```

### Run Specific Test Category
```bash
# Unit tests only
find build/tests/unit -name "test_*" -exec {} \;

# QA tests only
find build/tests/unit_qa -name "test_*" -exec {} \;
find build/tests/integration_qa -name "test_*" -exec {} \;
```

### Memory Leak Detection
```bash
./scripts/check_memory_leaks.sh
```

### Performance Profiling
```bash
./scripts/profile_performance.sh
```

## Test Development Guidelines

### AAA Pattern Implementation
Each test follows the Arrange-Act-Assert pattern:

```c
void test_example_function(void) {
    // Arrange: Set up test conditions
    polycall_core_context_t* ctx = setup_test_context();
    const char* input = "test_data";
    char output[256];
    
    // Act: Execute the function under test
    polycall_core_error_t result = example_function(ctx, input, output, sizeof(output));
    
    // Assert: Verify expected outcomes
    assert(result == POLYCALL_CORE_SUCCESS);
    assert(strcmp(output, "expected_result") == 0);
    
    // Cleanup
    cleanup_test_context(ctx);
}
```

### QA Test Focus Areas
- **Error Handling**: Invalid parameters, resource exhaustion
- **Memory Management**: Leak detection, double-free protection
- **Resource Limits**: CPU, memory, file descriptor limits
- **Performance**: Latency, throughput under load
- **Telemetry**: Accurate data collection and reporting

### Integration Test Scenarios
- **Protocol + Network**: Message passing over network
- **FFI + Protocol**: Cross-language function calls
- **REPL + Telemetry**: Interactive monitoring
- **Auth + Security**: Authentication flow validation
- **Edge + Micro**: Distributed microservice coordination

## Test Reports
Test execution generates reports in `/reports/`:
- `*.log`: Individual test output
- `*.valgrind`: Memory leak reports
- `*.perf`: Performance profiling data

## Continuous Integration
Tests are designed for automated CI/CD pipelines:
- Exit codes indicate pass/fail status
- JSON reports for integration with CI tools
- Configurable test timeouts
- Parallel test execution support

## Contributing
When adding new features:
1. Write QA tests first (TDD approach)
2. Implement feature to pass QA tests
3. Add unit tests for edge cases
4. Update integration tests if cross-module impact
5. Verify all test categories pass

## Test Environment Requirements
- C11-compatible compiler
- CMake 3.13+
- pthreads library
- valgrind (for memory leak detection)
- perf (for performance profiling)
- LibPolyCall development headers and libraries
