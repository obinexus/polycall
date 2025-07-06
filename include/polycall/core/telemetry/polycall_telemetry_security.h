/**
 * @file polycall_telemetry_security.h
 * @brief Security-Focused Telemetry Mechanisms for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides specialized telemetry tracking for security-critical events
 * across LibPolyCall's distributed computing ecosystem.
 */

#ifndef POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_SECURITY_H_H
#define POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_SECURITY_H_H

#include "../core/polycall_core.h"
#include "../edge/polycall_edge.h"
#include "polycall/core/polycall/polycall_telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Security event types for granular tracking
 */
typedef enum {
  // Authentication Events
  SECURITY_EVENT_AUTH_ATTEMPT = 0, // Authentication attempt
  SECURITY_EVENT_AUTH_SUCCESS = 1, // Successful authentication
  SECURITY_EVENT_AUTH_FAILURE = 2, // Authentication failure
  SECURITY_EVENT_AUTH_LOCKOUT = 3, // Account lockout

  // Access Control Events
  SECURITY_EVENT_ACCESS_GRANTED = 10,       // Access permission granted
  SECURITY_EVENT_ACCESS_DENIED = 11,        // Access permission denied
  SECURITY_EVENT_PRIVILEGE_ESCALATION = 12, // Potential privilege escalation

  // Cryptographic Events
  SECURITY_EVENT_CRYPTO_INIT = 20,    // Cryptographic operation started
  SECURITY_EVENT_CRYPTO_SUCCESS = 21, // Cryptographic operation completed
  SECURITY_EVENT_CRYPTO_FAILURE = 22, // Cryptographic operation failed

  // Intrusion Detection
  SECURITY_EVENT_POTENTIAL_BREACH = 30, // Potential security breach detected
  SECURITY_EVENT_ANOMALY_DETECTED = 31, // Unusual system behavior
  SECURITY_EVENT_BREACH_CONFIRMED = 32, // Confirmed security breach

  // Network Security
  SECURITY_EVENT_CONNECTION_ATTEMPT = 40,  // New connection attempt
  SECURITY_EVENT_CONNECTION_REJECTED = 41, // Connection rejected
  SECURITY_EVENT_NETWORK_SCAN = 42,        // Potential network scanning

  // System Integrity
  SECURITY_EVENT_SYSTEM_MODIFICATION = 50,  // Critical system modification
  SECURITY_EVENT_CONFIGURATION_CHANGE = 51, // Security configuration altered
  SECURITY_EVENT_INTEGRITY_BREACH = 52      // System integrity compromised
} polycall_security_event_type_t;

/**
 * @brief Security context tracking structure
 */
typedef struct {
  const char *node_id;                       // Specific node identifier
  polycall_edge_threat_level_t threat_level; // Current threat assessment
  uint64_t last_event_timestamp;             // Timestamp of last security event
  uint32_t consecutive_failures; // Consecutive authentication/access failures
} polycall_security_context_tracking_t;

/**
 * @brief Security telemetry configuration
 */
typedef struct {
  bool enable_security_tracking;     // Global security tracking enable
  uint32_t max_consecutive_failures; // Threshold for consecutive failures
  bool auto_block_on_threshold;      // Automatically block after threshold
  bool log_all_security_events;      // Log all security-related events
  polycall_telemetry_severity_t min_log_severity; // Minimum severity to log
} polycall_security_telemetry_config_t;

/**
 * @brief Record a security-specific telemetry event
 *
 * @param telemetry_ctx Base telemetry context
 * @param security_ctx Security context tracking
 * @param event_type Specific security event type
 * @param description Optional detailed description
 * @return Error code indicating event recording status
 */
polycall_core_error_t polycall_security_telemetry_record_event(
    polycall_telemetry_context_t *telemetry_ctx,
    polycall_security_context_tracking_t *security_ctx,
    polycall_security_event_type_t event_type, const char *description);

/**
 * @brief Initialize security telemetry tracking
 *
 * @param core_ctx Core LibPolyCall context
 * @param telemetry_ctx Base telemetry context
 * @param config Security telemetry configuration
 * @param security_ctx Pointer to receive security context
 * @return Error code indicating initialization status
 */
polycall_core_error_t polycall_security_telemetry_init(
    polycall_core_context_t *core_ctx,
    polycall_telemetry_context_t *telemetry_ctx,
    const polycall_security_telemetry_config_t *config,
    polycall_security_context_tracking_t **security_ctx);

/**
 * @brief Generate security incident report
 *
 * Produces a comprehensive report of security-related events
 *
 * @param security_ctx Security context tracking
 * @param start_time Optional start time for report
 * @param end_time Optional end time for report
 * @param report_buffer Buffer to receive report
 * @param buffer_size Size of report buffer
 * @param required_size Pointer to receive required buffer size
 * @return Error code indicating report generation status
 */
polycall_core_error_t polycall_security_telemetry_generate_report(
    polycall_security_context_tracking_t *security_ctx, uint64_t start_time,
    uint64_t end_time, void *report_buffer, size_t buffer_size,
    size_t *required_size);

/**
 * @brief Reset security context tracking
 *
 * Resets consecutive failure counts and clears temporary tracking data
 *
 * @param security_ctx Security context tracking
 * @return Error code indicating reset status
 */
polycall_core_error_t polycall_security_telemetry_reset(
    polycall_security_context_tracking_t *security_ctx);

/**
 * @brief Cleanup security telemetry resources
 *
 * @param core_ctx Core LibPolyCall context
 * @param security_ctx Security context tracking to clean up
 */
void polycall_security_telemetry_cleanup(
    polycall_core_context_t *core_ctx,
    polycall_security_context_tracking_t *security_ctx);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_SECURITY_H_H */