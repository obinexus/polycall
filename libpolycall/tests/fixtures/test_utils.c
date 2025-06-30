/**
 * @file test_utils.c
 * @brief Implementation of common testing utilities
 */

#include "test_utils.h"
#include <polycall/core/polycall/polycall_memory.h>

polycall_core_error_t test_setup_core_context(polycall_core_context_t** ctx) {
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return polycall_core_context_create(ctx);
}

polycall_core_error_t test_cleanup_core_context(polycall_core_context_t* ctx) {
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return polycall_core_context_destroy(ctx);
}

polycall_core_error_t test_setup_telemetry(polycall_core_context_t* core_ctx,
                                           polycall_telemetry_context_t** telemetry_ctx) {
    if (!core_ctx || !telemetry_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return polycall_telemetry_init(core_ctx, telemetry_ctx, NULL);
}

polycall_core_error_t test_cleanup_telemetry(polycall_core_context_t* core_ctx,
                                             polycall_telemetry_context_t* telemetry_ctx) {
    if (!core_ctx || !telemetry_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    return polycall_telemetry_cleanup(core_ctx, telemetry_ctx);
}

size_t test_get_memory_usage(polycall_core_context_t* ctx) {
    if (!ctx) {
        return 0;
    }
    
    return polycall_core_get_allocated_memory(ctx);
}

void test_check_memory_leaks(polycall_core_context_t* ctx, size_t initial_memory) {
    size_t final_memory = test_get_memory_usage(ctx);
    
    if (final_memory > initial_memory) {
        fprintf(stderr, "MEMORY LEAK DETECTED: %zu bytes leaked\n", 
                final_memory - initial_memory);
        abort();
    }
}

void test_start_performance_measurement(test_fixture_t* fixture) {
    if (!fixture) {
        return;
    }
    
    fixture->start_time = clock();
    fixture->initial_memory = test_get_memory_usage(fixture->core_ctx);
}

double test_end_performance_measurement(test_fixture_t* fixture) {
    if (!fixture) {
        return 0.0;
    }
    
    clock_t end_time = clock();
    return ((double)(end_time - fixture->start_time)) / CLOCKS_PER_SEC;
}

char* test_generate_random_string(size_t length) {
    if (length == 0) {
        return NULL;
    }
    
    char* str = malloc(length + 1);
    if (!str) {
        return NULL;
    }
    
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    for (size_t i = 0; i < length; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length] = '\0';
    
    return str;
}

void test_generate_random_data(void* buffer, size_t size) {
    if (!buffer || size == 0) {
        return;
    }
    
    unsigned char* bytes = (unsigned char*)buffer;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (unsigned char)(rand() % 256);
    }
}
