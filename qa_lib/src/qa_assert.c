/**
 * qa_assert.c - QA Assertion Implementation
 * OBINexus Polycall v2 Testing Framework
 */

#include "qa_assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Thread-local storage for test context
static __thread struct {
    qa_category_t last_category;
    char last_message[256];
    int assertion_count;
    int failure_count;
} qa_context = {0};

// ANSI color codes
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

void qa_assert_fail(const char* file, int line, const char* expr) {
    fprintf(stderr, COLOR_RED "[ASSERT FAILED] %s:%d: %s" COLOR_RESET "\n", 
            file, line, expr);
    qa_context.failure_count++;
}

void qa_assert_category(qa_category_t cat, bool condition, 
                       const char* file, int line, const char* msg) {
    qa_context.assertion_count++;
    qa_context.last_category = cat;
    strncpy(qa_context.last_message, msg, sizeof(qa_context.last_message) - 1);
    
    const char* cat_str = "";
    const char* color = COLOR_RESET;
    
    switch (cat) {
        case QA_TRUE_POSITIVE:
            cat_str = "TP";
            color = COLOR_GREEN;
            break;
        case QA_TRUE_NEGATIVE:
            cat_str = "TN";
            color = COLOR_BLUE;
            break;
        case QA_FALSE_POSITIVE:
            cat_str = "FP";
            color = COLOR_YELLOW;
            break;
        case QA_FALSE_NEGATIVE:
            cat_str = "FN";
            color = COLOR_RED;
            break;
    }
    
    if (!condition) {
        fprintf(stderr, "%s[%s ASSERTION FAILED]%s %s:%d: %s\n", 
                color, cat_str, COLOR_RESET, file, line, msg);
        qa_context.failure_count++;
        
        // Log to QA report
        FILE* report = fopen("/tmp/polycall_qa_report.json", "a");
        if (report) {
            fprintf(report, "{\"category\":\"%s\",\"file\":\"%s\",\"line\":%d,"
                          "\"message\":\"%s\",\"timestamp\":%ld},\n",
                    cat_str, file, line, msg, time(NULL));
            fclose(report);
        }
    } else if (getenv("QA_VERBOSE")) {
        fprintf(stdout, "%s[%s PASS]%s %s\n", color, cat_str, COLOR_RESET, msg);
    }
}

void qa_assert_fp_check(int actual, int expected,
                       const char* file, int line, const char* msg) {
    if (actual == expected) {
        // This is a false positive - we expected failure but got success
        qa_assert_category(QA_FALSE_POSITIVE, false, file, line, msg);
    } else {
        // Correct behavior
        qa_assert_category(QA_TRUE_NEGATIVE, true, file, line, msg);
    }
}

void qa_assert_fn_check(bool condition,
                       const char* file, int line, const char* msg) {
    if (!condition) {
        // This is a false negative - we expected success but got failure
        qa_assert_category(QA_FALSE_NEGATIVE, false, file, line, msg);
    } else {
        // Correct behavior
        qa_assert_category(QA_TRUE_POSITIVE, true, file, line, msg);
    }
}

// Report generation
void qa_generate_report(const char* test_name) {
    printf("\n%s=== QA Report: %s ===%s\n", COLOR_BLUE, test_name, COLOR_RESET);
    printf("Total Assertions: %d\n", qa_context.assertion_count);
    printf("Failures: %d\n", qa_context.failure_count);
    
    if (qa_context.failure_count == 0) {
        printf("%s[PASSED]%s All assertions succeeded\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%s[FAILED]%s %d assertions failed\n", 
               COLOR_RED, qa_context.failure_count, COLOR_RESET);
    }
}
