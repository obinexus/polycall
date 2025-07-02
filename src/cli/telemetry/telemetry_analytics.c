/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file telemetry_analytics.c
 * @brief Analytics functionality for telemetry commands
 */

#include "polycall/cli/telemetry/telemetry_commands.h"
#include "polycall/core/telemetry/polycall_telemetry_reporting.h"
#include "polycall/core/telemetry/polycall_telemetry.h"
#include <string.h>
#include <time.h>

/**
 * Run telemetry analytics
 */
int telemetry_run_analytics(polycall_core_context_t* core_ctx, 
                           const char* timeframe,
                           const char* output_format,
                           const char* query) {
    // Get telemetry container
    telemetry_container_t* container = polycall_get_service(core_ctx, "telemetry_container");
    if (!container) {
        fprintf(stderr, "Error: telemetry module not initialized\n");
        return -1;
    }
    
    // Get reporting context
    polycall_telemetry_reporting_context_t* reporting_ctx = container->reporting_ctx;
    if (!reporting_ctx) {
        fprintf(stderr, "Error: telemetry reporting not initialized\n");
        return -1;
    }
    
    // Parse timeframe
    uint64_t start_time = 0;
    uint64_t end_time = time(NULL) * 1000000000ULL; // Current time in nanoseconds
    
    if (timeframe) {
        if (strcmp(timeframe, "hour") == 0) {
            start_time = end_time - 3600LL * 1000000000ULL;
        } else if (strcmp(timeframe, "day") == 0) {
            start_time = end_time - 86400LL * 1000000000ULL;
        } else if (strcmp(timeframe, "week") == 0) {
            start_time = end_time - 7LL * 86400LL * 1000000000ULL;
        } else if (strcmp(timeframe, "month") == 0) {
            start_time = end_time - 30LL * 86400LL * 1000000000ULL;
        } else {
            fprintf(stderr, "Error: Invalid timeframe, must be hour, day, week, or month\n");
            return -1;
        }
    } else {
        // Default to 24 hours
        start_time = end_time - 86400LL * 1000000000ULL;
    }
    
    // Run analytics
    polycall_telemetry_analytics_config_t analytics_config = {
        .start_timestamp = start_time,
        .end_timestamp = end_time,
        .query_pattern = query ? query : "*",
        .output_format = output_format && strcmp(output_format, "json") == 0 ? 
                          POLYCALL_TELEMETRY_FORMAT_JSON : POLYCALL_TELEMETRY_FORMAT_TEXT
    };
    
    // Generate report
    size_t report_size = 0;
    polycall_telemetry_reporting_run_analytics(reporting_ctx, &analytics_config);
    
    // Output is handled by the reporting system

    return 0;
}
