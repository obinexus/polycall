/**
 * @file polycall_test_utils.h
 * @brief Enhanced test utilities for IoC-driven libpolycall testing
 * @author OBINexus LibPolyCall Testing Framework
 */

#ifndef POLYCALL_TEST_UTILS_H
#define POLYCALL_TEST_UTILS_H

#include <polycall/core/polycall/polycall.h>
#include <polycall/core/polycall/polycall_error.h>
#include <polycall/core/config/config.h>
#include <polycall/core/telemetry/polycall_telemetry.h>
#include <polycall/core/protocol/protocol.h>
#include <polycall/core/network/network.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// Enhanced test assertion macros with telemetry
#define POLYCALL_TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "TEST ASSERTION FAILED: %s at %s:%d\n", \
                    message, __FILE__, __LINE__); \
            if (g_test_context && g_test_context->telemetry_ctx) { \
                polycall_telemetry_record_error(g_test_context->core_ctx, \
                    g_test_context->telemetry_ctx, "test_assertion_failure", message); \
            } \
            abort(); \
        } \
    } while(0)

#define POLYCALL_TEST_ASSERT_SUCCESS(result, operation) \
    POLYCALL_TEST_ASSERT((result) == POLYCALL_CORE_SUCCESS, \
        "Operation " operation " failed with error code: " #result)

#define POLYCALL_TEST_ASSERT_MODULE_INIT(module_ctx, module_name) \
    POLYCALL_TEST_ASSERT((module_ctx) != NULL, \
        "Module " module_name " failed to initialize")

// Test data generation types
typedef enum {
    POLYCALL_TEST_DATA_RANDOM,
    POLYCALL_TEST_DATA_SEQUENTIAL,
    POLYCALL_TEST_DATA_ZEROS,
    POLYCALL_TEST_DATA_ONES,
    POLYCALL_TEST_DATA_PATTERN
} polycall_test_data_type_t;

// IoC-aware test context structure
typedef struct {
    polycall_core_context_t* core_ctx;
    polycall_config_context_t* config_ctx;
    polycall_telemetry_context_t* telemetry_ctx;
    
    // Resource tracking
    size_t initial_memory;
    clock_t start_time;
    
    // Test environment
    char temp_dir[256];
    
    // Test metadata
    char test_name[128];
    bool is_qa_test;
} polycall_test_context_t;

// Module-specific test fixture
typedef struct {
    polycall_test_context_t* test_ctx;
    char module_name[64];
    
    union {
        polycall_protocol_context_t* protocol_ctx;
        polycall_network_context_t* network_ctx;
        polycall_telemetry_context_t* telemetry_ctx;
        polycall_config_context_t* config_ctx;
        void* generic_ctx;
    } module_ctx;
} polycall_test_module_fixture_t;

// IoC-aware test environment management
polycall_core_error_t polycall_test_init_context(polycall_test_context_t** ctx);
polycall_core_error_t polycall_test_cleanup_context(polycall_test_context_t* ctx);

// Module-specific fixture management
polycall_core_error_t polycall_test_setup_module_fixture(
    const char* module_name,
    polycall_test_module_fixture_t** fixture
);
polycall_core_error_t polycall_test_cleanup_module_fixture(
    polycall_test_module_fixture_t* fixture
);

// Telemetry validation utilities
polycall_core_error_t polycall_test_validate_telemetry(
    polycall_test_context_t* ctx,
    const char* operation_name,
    size_t expected_count
);

// Test data generation
void polycall_test_generate_data(void* buffer, size_t size, polycall_test_data_type_t type);

// Performance and resource validation
double polycall_test_measure_performance(polycall_test_context_t* ctx);
polycall_core_error_t polycall_test_validate_resources(
    polycall_test_context_t* ctx,
    size_t max_memory_bytes,
    double max_cpu_seconds
);

// Global test context (for assertion macros)
extern polycall_test_context_t* g_test_context;

#ifdef __cplusplus
}
#endif

#endif // POLYCALL_TEST_UTILS_H
