/**
 * @file telemetry_analytics.h
 * @brief Analytics functionality for telemetry commands
 */

#ifndef POLYCALL_CLI_TELEMETRY_ANALYTICS_H
#define POLYCALL_CLI_TELEMETRY_ANALYTICS_H

#include "polycall/core/polycall/polycall.h"

/**
 * Run telemetry analytics
 * 
 * @param core_ctx Core context
 * @param timeframe Timeframe for analytics (hour, day, week, month)
 * @param output_format Output format (text, json)
 * @param query Optional query pattern
 * @return 0 on success, non-zero on error
 */
int telemetry_run_analytics(polycall_core_context_t* core_ctx, 
                           const char* timeframe,
                           const char* output_format,
                           const char* query);

#endif /* POLYCALL_CLI_TELEMETRY_ANALYTICS_H */
