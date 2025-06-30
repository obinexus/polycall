#!/bin/bash
# ================================================================
# LibPolyCall Test Infrastructure Configuration Generator
# OBINexus Framework - IoC Integration Support
# ================================================================

# Test configuration template for libpolycall
cat > tests/fixtures/config.Polycallfile.test << 'EOF'
# LibPolyCall Test Configuration File
# Zero-Trust Architecture Testing Configuration

# ----- Test Server Definitions -----
server node 18080:18084       # Test Node.js endpoint binding
server python 13001:18084     # Test Python endpoint binding
server java 13002:18082       # Test Java endpoint binding
server go 13003:18083         # Test Go endpoint binding

# ----- Test Network Configuration -----
network start
network_timeout=2000
max_connections=100
retry_interval=500
reconnect_attempts=2
packet_validation=strict

# ----- Test Global Settings -----
log_directory=/tmp/polycall_test_logs
workspace_root=/tmp/polycall_test
base_protocol_version=1.0
message_encoding=utf-8
state_persistence=false

# ----- Test Security Configuration -----
tls_enabled=false
auth_required=false
default_policy=allow

# ----- Test Resource Limits -----
max_memory_per_service=256M
max_cpu_per_service=1
max_message_size=4M
rate_limit_requests=50/s
rate_limit_window=30

# ----- Test Monitoring & Alerts -----
enable_metrics=true
metrics_port=19090
health_check_interval=10
dead_service_timeout=30

# ----- Test Logging & Auditing -----
log_level=debug
audit_enabled=true
audit_log=/tmp/polycall_test_logs/audit.log
log_rotation=none
log_retention=1
structured_logging=true
log_format=json

# ----- Test Error Handling -----
graceful_degradation=true
circuit_breaker_enabled=true
failure_threshold=3
circuit_reset_timeout=30
EOF

# Valgrind suppressions for known issues
cat > tests/fixtures/valgrind.supp << 'EOF'
# Valgrind suppressions for LibPolyCall testing
{
   pthread_create_glibc
   Memcheck:Leak
   match-leak-kinds: possible
   fun:calloc
   fun:allocate_dtv
   fun:_dl_allocate_tls
   fun:allocate_stack
   fun:pthread_create@@GLIBC_*
}

{
   dlopen_leak
   Memcheck:Leak
   match-leak-kinds: reachable
   fun:*alloc
   ...
   fun:dlopen*
}
EOF

# Common test utilities implementation
cat > tests/fixtures/polycall_test_utils.c << 'EOF'
/**
 * @file polycall_test_utils.c
 * @brief Enhanced test utilities for IoC-driven libpolycall testing
 * @author OBINexus LibPolyCall Testing Framework
 */

#include "polycall_test_utils.h"
#include <polycall/core/polycall/polycall.h>
#include <polycall/core/config/config.h>
#include <polycall/core/telemetry/polycall_telemetry.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Global test configuration context
static polycall_test_context_t* g_test_context = NULL;

/**
 * @brief Initialize IoC-aware test environment
 */
polycall_core_error_t polycall_test_init_context(polycall_test_context_t** ctx) {
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate test context
    *ctx = calloc(1, sizeof(polycall_test_context_t));
    if (!*ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    polycall_test_context_t* test_ctx = *ctx;
    
    // Create temporary test directory
    snprintf(test_ctx->temp_dir, sizeof(test_ctx->temp_dir), "/tmp/polycall_test_%d", getpid());
    if (mkdir(test_ctx->temp_dir, 0755) != 0) {
        free(test_ctx);
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }
    
    // Initialize core context from test configuration
    polycall_core_error_t result = polycall_core_context_create(&test_ctx->core_ctx);
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_test_cleanup_context(test_ctx);
        return result;
    }
    
    // Load test configuration
    const char* test_config = getenv("POLYCALL_TEST_CONFIG_FILE");
    if (!test_config) {
        test_config = "tests/fixtures/config.Polycallfile.test";
    }
    
    polycall_config_context_t* config_ctx = NULL;
    result = polycall_config_init(test_ctx->core_ctx, &config_ctx, NULL);
    if (result == POLYCALL_CORE_SUCCESS) {
        result = polycall_config_load_file(test_ctx->core_ctx, config_ctx, test_config);
        test_ctx->config_ctx = config_ctx;
    }
    
    // Initialize telemetry for test monitoring
    result = polycall_telemetry_init(test_ctx->core_ctx, &test_ctx->telemetry_ctx, NULL);
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_test_cleanup_context(test_ctx);
        return result;
    }
    
    // Record initial memory state
    test_ctx->initial_memory = polycall_core_get_allocated_memory(test_ctx->core_ctx);
    test_ctx->start_time = clock();
    
    g_test_context = test_ctx;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup IoC test environment
 */
polycall_core_error_t polycall_test_cleanup_context(polycall_test_context_t* ctx) {
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check for memory leaks
    if (ctx->core_ctx) {
        size_t final_memory = polycall_core_get_allocated_memory(ctx->core_ctx);
        if (final_memory > ctx->initial_memory) {
            fprintf(stderr, "MEMORY LEAK DETECTED: %zu bytes leaked\n", 
                    final_memory - ctx->initial_memory);
        }
    }
    
    // Cleanup telemetry
    if (ctx->telemetry_ctx && ctx->core_ctx) {
        polycall_telemetry_cleanup(ctx->core_ctx, ctx->telemetry_ctx);
    }
    
    // Cleanup configuration
    if (ctx->config_ctx && ctx->core_ctx) {
        polycall_config_cleanup(ctx->core_ctx, ctx->config_ctx);
    }
    
    // Cleanup core context
    if (ctx->core_ctx) {
        polycall_core_context_destroy(ctx->core_ctx);
    }
    
    // Remove temporary directory
    if (strlen(ctx->temp_dir) > 0) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", ctx->temp_dir);
        system(cmd);
    }
    
    free(ctx);
    g_test_context = NULL;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Setup module-specific test fixture with IoC integration
 */
polycall_core_error_t polycall_test_setup_module_fixture(
    const char* module_name,
    polycall_test_module_fixture_t** fixture
) {
    if (!module_name || !fixture || !g_test_context) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    *fixture = calloc(1, sizeof(polycall_test_module_fixture_t));
    if (!*fixture) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    polycall_test_module_fixture_t* mod_fixture = *fixture;
    
    // Copy module name
    strncpy(mod_fixture->module_name, module_name, sizeof(mod_fixture->module_name) - 1);
    
    // Reference global test context
    mod_fixture->test_ctx = g_test_context;
    
    // Initialize module-specific context based on module name
    polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
    
    if (strcmp(module_name, "protocol") == 0) {
        result = polycall_protocol_init(g_test_context->core_ctx, 
                                       &mod_fixture->module_ctx.protocol_ctx, NULL);
    } else if (strcmp(module_name, "network") == 0) {
        result = polycall_network_init(g_test_context->core_ctx, 
                                      &mod_fixture->module_ctx.network_ctx, NULL);
    } else if (strcmp(module_name, "telemetry") == 0) {
        mod_fixture->module_ctx.telemetry_ctx = g_test_context->telemetry_ctx;
    } else if (strcmp(module_name, "config") == 0) {
        mod_fixture->module_ctx.config_ctx = g_test_context->config_ctx;
    } else {
        // Generic module initialization
        result = POLYCALL_CORE_SUCCESS;
    }
    
    if (result != POLYCALL_CORE_SUCCESS) {
        free(mod_fixture);
        *fixture = NULL;
    }
    
    return result;
}

/**
 * @brief Cleanup module-specific test fixture
 */
polycall_core_error_t polycall_test_cleanup_module_fixture(
    polycall_test_module_fixture_t* fixture
) {
    if (!fixture) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Cleanup module-specific context
    if (strcmp(fixture->module_name, "protocol") == 0 && fixture->module_ctx.protocol_ctx) {
        polycall_protocol_cleanup(fixture->test_ctx->core_ctx, fixture->module_ctx.protocol_ctx);
    } else if (strcmp(fixture->module_name, "network") == 0 && fixture->module_ctx.network_ctx) {
        polycall_network_cleanup(fixture->test_ctx->core_ctx, fixture->module_ctx.network_ctx);
    }
    
    free(fixture);
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Validate telemetry data integrity
 */
polycall_core_error_t polycall_test_validate_telemetry(
    polycall_test_context_t* ctx,
    const char* operation_name,
    size_t expected_count
) {
    if (!ctx || !operation_name || !ctx->telemetry_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    polycall_telemetry_stats_t stats;
    polycall_core_error_t result = polycall_telemetry_get_stats(
        ctx->core_ctx, ctx->telemetry_ctx, &stats
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Validate operation count
    if (stats.operation_count < expected_count) {
        fprintf(stderr, "Telemetry validation failed: expected %zu operations, found %zu\n",
                expected_count, stats.operation_count);
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Generate test data with specified characteristics
 */
void polycall_test_generate_data(void* buffer, size_t size, polycall_test_data_type_t type) {
    if (!buffer || size == 0) {
        return;
    }
    
    unsigned char* bytes = (unsigned char*)buffer;
    
    switch (type) {
        case POLYCALL_TEST_DATA_RANDOM:
            for (size_t i = 0; i < size; i++) {
                bytes[i] = (unsigned char)(rand() % 256);
            }
            break;
            
        case POLYCALL_TEST_DATA_SEQUENTIAL:
            for (size_t i = 0; i < size; i++) {
                bytes[i] = (unsigned char)(i % 256);
            }
            break;
            
        case POLYCALL_TEST_DATA_ZEROS:
            memset(buffer, 0, size);
            break;
            
        case POLYCALL_TEST_DATA_ONES:
            memset(buffer, 0xFF, size);
            break;
            
        default:
            memset(buffer, 0xAA, size); // Alternating pattern
            break;
    }
}

/**
 * @brief Performance measurement utilities
 */
double polycall_test_measure_performance(polycall_test_context_t* ctx) {
    if (!ctx) {
        return 0.0;
    }
    
    clock_t end_time = clock();
    return ((double)(end_time - ctx->start_time)) / CLOCKS_PER_SEC;
}

/**
 * @brief Resource usage validation
 */
polycall_core_error_t polycall_test_validate_resources(
    polycall_test_context_t* ctx,
    size_t max_memory_bytes,
    double max_cpu_seconds
) {
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check memory usage
    size_t current_memory = polycall_core_get_allocated_memory(ctx->core_ctx);
    if (current_memory > max_memory_bytes) {
        fprintf(stderr, "Memory limit exceeded: %zu bytes > %zu bytes\n",
                current_memory, max_memory_bytes);
        return POLYCALL_CORE_ERROR_RESOURCE_EXHAUSTED;
    }
    
    // Check CPU time
    double cpu_time = polycall_test_measure_performance(ctx);
    if (cpu_time > max_cpu_seconds) {
        fprintf(stderr, "CPU time limit exceeded: %.2f seconds > %.2f seconds\n",
                cpu_time, max_cpu_seconds);
        return POLYCALL_CORE_ERROR_RESOURCE_EXHAUSTED;
    }
    
    return POLYCALL_CORE_SUCCESS;
}
EOF

# Enhanced test utilities header
cat > tests/fixtures/polycall_test_utils.h << 'EOF'
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
EOF

# Telemetry validation test
cat > tests/fixtures/telemetry_validation.c << 'EOF'
/**
 * @file telemetry_validation.c
 * @brief System-wide telemetry validation test
 * @author OBINexus LibPolyCall Testing Framework
 */

#include "polycall_test_utils.h"
#include <polycall/core/telemetry/polycall_telemetry.h>

int main(void) {
    printf("Starting telemetry system validation...\n");
    
    polycall_test_context_t* test_ctx = NULL;
    polycall_core_error_t result = polycall_test_init_context(&test_ctx);
    POLYCALL_TEST_ASSERT_SUCCESS(result, "test context initialization");
    
    // Validate telemetry is properly initialized
    POLYCALL_TEST_ASSERT(test_ctx->telemetry_ctx != NULL, "telemetry context initialized");
    
    // Generate some telemetry events
    polycall_telemetry_record_operation(test_ctx->core_ctx, test_ctx->telemetry_ctx, 
                                       "test_operation", 100);
    polycall_telemetry_record_operation(test_ctx->core_ctx, test_ctx->telemetry_ctx, 
                                       "test_operation", 150);
    
    // Validate telemetry data
    result = polycall_test_validate_telemetry(test_ctx, "test_operation", 2);
    POLYCALL_TEST_ASSERT_SUCCESS(result, "telemetry validation");
    
    // Cleanup
    polycall_test_cleanup_context(test_ctx);
    
    printf("✅ Telemetry system validation passed\n");
    return 0;
}
EOF

echo "Test configuration and fixtures created successfully!"
echo ""
echo "Created files:"
echo "  📁 tests/fixtures/config.Polycallfile.test"
echo "  📁 tests/fixtures/valgrind.supp"
echo "  📁 tests/fixtures/polycall_test_utils.h"
echo "  📁 tests/fixtures/polycall_test_utils.c"
echo "  📁 tests/fixtures/telemetry_validation.c"
echo ""
echo "Key features:"
echo "  ✅ IoC-aware test context management"
echo "  ✅ Config.Polycallfile integration"
echo "  ✅ Module-specific fixture support"
echo "  ✅ Telemetry validation framework"
echo "  ✅ Resource usage monitoring"
echo "  ✅ Memory leak detection"
