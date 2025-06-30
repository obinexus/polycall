/**
 * @file telemetry_debug.h
 * @brief Debug functionality for telemetry commands
 */

#ifndef POLYCALL_CLI_TELEMETRY_DEBUG_H
#define POLYCALL_CLI_TELEMETRY_DEBUG_H

#include "polycall/core/polycall/polycall.h"

/**
 * Debug telemetry event by GUID
 * 
 * @param core_ctx Core context
 * @param event_guid Event GUID to debug
 * @return 0 on success, non-zero on error
 */
int telemetry_debug_event(polycall_core_context_t* core_ctx, const char* event_guid);

/**
 * List recent telemetry events
 * 
 * @param core_ctx Core context
 * @param limit Maximum number of events to list (0 for default)
 * @return 0 on success, non-zero on error
 */
int telemetry_list_events(polycall_core_context_t* core_ctx, int limit);

#endif /* POLYCALL_CLI_TELEMETRY_DEBUG_H */
