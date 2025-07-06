/**
 * qa_framework.c - QA Test Framework Implementation
 */

#include "qa_framework.h"
#include "qa_assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

// Test suite structure
struct qa_suite {
    char name[128];
    qa_test_entry_t* tests;
    size_t test_count;
    size_t test_capacity;
    qa_suite_options_t options;
};

// Create test suite
qa_suite_t* qa_suite_create(const char* name) {
    qa_suite_t* suite = calloc(1, sizeof(qa_suite_t));
    if (!suite) return NULL;
    
    strncpy(suite->name, name, sizeof(suite->name) - 1);
    suite->test_capacity = 16;
    suite->tests = calloc(suite->test_capacity, sizeof(qa_test_entry_t));
    
    // Default options
    suite->options.timeout_ms = 5000;
    suite->options.parallel = false;
    suite->options.coverage = true;
    suite->options.profiling = false;
    
    return suite;
}

// Add test to suite
void qa_suite_add_test(qa_suite_t* suite, const char* name, qa_test_fn fn) {
    if (!suite || !fn) return;
    
    // Resize if needed
    if (suite->test_count >= suite->test_capacity) {
        suite->test_capacity *= 2;
        suite->tests = realloc(suite->tests, 
                              suite->test_capacity * sizeof(qa_test_entry_t));
    }
    
    qa_test_entry_t* entry = &suite->tests[suite->test_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->fn = fn;
    entry->category = QA_CAT_UNIT;  // Default
}

// Run single test with timeout
static qa_result_t run_test_with_timeout(qa_test_fn fn, unsigned int timeout_ms) {
    // For simplicity, run without actual timeout mechanism
    // In production, use fork() or pthread with timeout
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    qa_result_t result = fn();
    
    gettimeofday(&end, NULL);
    long elapsed = (end.tv_sec - start.tv_sec) * 1000 + 
                   (end.tv_usec - start.tv_usec) / 1000;
    
    if (elapsed > timeout_ms) {
        fprintf(stderr, "[TIMEOUT] Test exceeded %ums (took %ldms)\n", 
                timeout_ms, elapsed);
        return QA_FAILURE;
    }
    
    return result;
}

// Run test suite
qa_suite_result_t qa_suite_run(qa_suite_t* suite) {
    qa_suite_result_t result = {0};
    
    printf("\n====== Running Test Suite: %s ======\n", suite->name);
    
    // Initialize result tracking
    result.total = suite->test_count;
    result.start_time = time(NULL);
    
    // Open JSON report
    char report_path[256];
    snprintf(report_path, sizeof(report_path), 
             "/tmp/qa_suite_%s_%ld.json", suite->name, result.start_time);
    FILE* report = fopen(report_path, "w");
    if (report) {
        fprintf(report, "{\n\"suite\":\"%s\",\n\"tests\":[\n", suite->name);
    }
    
    // Run each test
    for (size_t i = 0; i < suite->test_count; i++) {
        qa_test_entry_t* test = &suite->tests[i];
        
        printf("\n[%zu/%zu] Running: %s\n", i + 1, suite->test_count, test->name);
        
        // Run test
        qa_result_t test_result = run_test_with_timeout(test->fn, 
                                                        suite->options.timeout_ms);
        
        // Update statistics
        switch (test_result) {
            case QA_SUCCESS:
                result.passed++;
                printf("[PASS] %s\n", test->name);
                break;
            case QA_FAILURE:
                result.failed++;
                printf("[FAIL] %s\n", test->name);
                break;
            case QA_SKIP:
                result.skipped++;
                printf("[SKIP] %s\n", test->name);
                break;
        }
        
        // Log to report
        if (report) {
            fprintf(report, "  {\"name\":\"%s\",\"result\":%d,\"category\":\"%s\"}%s\n",
                    test->name, test_result, 
                    test->category == QA_CAT_UNIT ? "unit" : "integration",
                    i < suite->test_count - 1 ? "," : "");
        }
    }
    
    result.end_time = time(NULL);
    result.duration_ms = (result.end_time - result.start_time) * 1000;
    
    // Close report
    if (report) {
        fprintf(report, "],\n\"summary\":{\n");
        fprintf(report, "  \"total\":%zu,\n", result.total);
        fprintf(report, "  \"passed\":%zu,\n", result.passed);
        fprintf(report, "  \"failed\":%zu,\n", result.failed);
        fprintf(report, "  \"skipped\":%zu,\n", result.skipped);
        fprintf(report, "  \"duration_ms\":%zu\n", result.duration_ms);
        fprintf(report, "}\n}\n");
        fclose(report);
        
        printf("\nReport saved to: %s\n", report_path);
    }
    
    // Summary
    printf("\n====== Test Suite Summary ======\n");
    printf("Total:   %zu\n", result.total);
    printf("Passed:  %zu (%.1f%%)\n", result.passed, 
           result.total > 0 ? (100.0 * result.passed / result.total) : 0);
    printf("Failed:  %zu\n", result.failed);
    printf("Skipped: %zu\n", result.skipped);
    printf("Duration: %zu ms\n", result.duration_ms);
    
    return result;
}

// Destroy test suite
void qa_suite_destroy(qa_suite_t* suite) {
    if (suite) {
        free(suite->tests);
        free(suite);
    }
}
