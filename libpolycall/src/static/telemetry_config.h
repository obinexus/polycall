/**
 * @file polycall_telemetry_config.h
 * @brief Telemetry Configuration System for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Defines the configuration system for telemetry components, supporting
 * centralized management of telemetry settings across all components.
 */

 #ifndef POLYCALL_TELEMETRY_TELEMETRY_CONFIG_H_H
 #define POLYCALL_TELEMETRY_TELEMETRY_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_telemetry.h"
 #include "polycall/core/polycall/polycall_telemetry_reporting.h"
 #include "polycall/core/polycall/polycall_telemetry_security.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Telemetry output formats
  */
 typedef enum {
     TELEMETRY_FORMAT_JSON = 0,      // JSON output format
     TELEMETRY_FORMAT_XML = 1,       // XML output format
     TELEMETRY_FORMAT_CSV = 2,       // CSV output format
     TELEMETRY_FORMAT_BINARY = 3,    // Binary output format
     TELEMETRY_FORMAT_CUSTOM = 4     // Custom format (requires formatter)
 } polycall_telemetry_format_t;
 
 /**
  * @brief Telemetry export destinations
  */
 typedef enum {
     TELEMETRY_DEST_FILE = 0,        // Export to file
     TELEMETRY_DEST_NETWORK = 1,     // Export over network
     TELEMETRY_DEST_CONSOLE = 2,     // Export to console
     TELEMETRY_DEST_SYSLOG = 3,      // Export to syslog
     TELEMETRY_DEST_CALLBACK = 4     // Export via callback
 } polycall_telemetry_destination_t;
 
 /**
  * @brief Telemetry sampling mode
  */
 typedef enum {
     TELEMETRY_SAMPLING_NONE = 0,      // No sampling (record all)
     TELEMETRY_SAMPLING_FIXED = 1,     // Fixed interval sampling
     TELEMETRY_SAMPLING_ADAPTIVE = 2,  // Adaptive sampling based on system load
     TELEMETRY_SAMPLING_RANDOM = 3     // Random sampling
 } polycall_telemetry_sampling_t;
 
 /**
  * @brief Telemetry rotation policy
  */
 typedef enum {
     TELEMETRY_ROTATION_SIZE = 0,     // Rotate based on size
     TELEMETRY_ROTATION_TIME = 1,     // Rotate based on time
     TELEMETRY_ROTATION_HYBRID = 2    // Rotate based on size or time
 } polycall_telemetry_rotation_policy_t;
 
 /**
  * @brief Comprehensive telemetry configuration structure
  */
 typedef struct {
     /* General telemetry settings */
     bool enable_telemetry;                      // Master enable for telemetry
     polycall_telemetry_severity_t min_severity; // Minimum severity to record
     uint32_t max_event_queue_size;              // Maximum events in queue
     
     /* Output configuration */
     polycall_telemetry_format_t format;         // Output format
     polycall_telemetry_destination_t destination;// Output destination
     char output_path[256];                      // Output path/URL
     bool enable_compression;                    // Enable data compression
     bool enable_encryption;                     // Enable data encryption
     
     /* Sampling configuration */
     polycall_telemetry_sampling_t sampling_mode;// Sampling mode
     uint32_t sampling_interval;                 // Sampling interval (ms)
     float sampling_rate;                        // Sampling rate (0.0-1.0)
     
     /* Performance optimization */
     bool use_buffering;                         // Use buffered output
     uint32_t buffer_flush_interval_ms;          // Buffer flush interval
     uint32_t buffer_size;                       // Buffer size in bytes
     
     /* Log rotation */
     polycall_telemetry_rotation_policy_t rotation_policy; // Rotation policy
     uint32_t max_log_size_mb;                   // Maximum log size
     uint32_t max_log_age_hours;                 // Maximum log age
     uint32_t max_log_files;                     // Maximum log files
     
     /* Security telemetry */
     bool enable_security_tracking;              // Enable security tracking
     uint32_t security_event_retention_days;     // Security event retention
     bool enable_integrity_verification;         // Verify log integrity
     
     /* Reporting */
     bool enable_advanced_analytics;             // Enable advanced analytics
     bool enable_pattern_matching;               // Enable pattern matching
     uint32_t analytics_window_ms;               // Analytics time window
     
     /* Integration */
     bool forward_to_core_logging;               // Forward to core logging
     bool integrate_with_edge;                   // Integrate with edge telemetry
     bool forward_to_external_systems;           // Forward to external systems
 } polycall_telemetry_config_t;
 
 /**
  * @brief Telemetry configuration context
  */
 typedef struct polycall_telemetry_config_context polycall_telemetry_config_context_t;
 
 /**
  * @brief Initialize telemetry configuration
  * 
  * @param core_ctx Core context
  * @param config_ctx Pointer to receive configuration context
  * @param config Initial configuration
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_init(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_config_context_t** config_ctx,
     const polycall_telemetry_config_t* config
 );
 
 /**
  * @brief Load telemetry configuration from file
  * 
  * @param config_ctx Configuration context
  * @param file_path Path to configuration file
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_load(
     polycall_telemetry_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Save telemetry configuration to file
  * 
  * @param config_ctx Configuration context
  * @param file_path Path to save configuration
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_save(
     polycall_telemetry_config_context_t* config_ctx,
     const char* file_path
 );
 
 /**
  * @brief Apply configuration to telemetry system
  * 
  * @param config_ctx Configuration context
  * @param telemetry_ctx Telemetry context
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_apply(
     polycall_telemetry_config_context_t* config_ctx,
     polycall_telemetry_context_t* telemetry_ctx
 );
 
 /**
  * @brief Update specific configuration parameter
  * 
  * @param config_ctx Configuration context
  * @param param_name Parameter name
  * @param param_value Parameter value
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_update_param(
     polycall_telemetry_config_context_t* config_ctx,
     const char* param_name,
     const void* param_value
 );
 
 /**
  * @brief Get current telemetry configuration
  * 
  * @param config_ctx Configuration context
  * @param config Pointer to receive configuration
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_get(
     polycall_telemetry_config_context_t* config_ctx,
     polycall_telemetry_config_t* config
 );
 
 /**
  * @brief Register configuration change callback
  * 
  * @param config_ctx Configuration context
  * @param callback Callback function
  * @param user_data User data for callback
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_config_register_callback(
     polycall_telemetry_config_context_t* config_ctx,
     void (*callback)(const polycall_telemetry_config_t* new_config, void* user_data),
     void* user_data
 );
 
 /**
  * @brief Create default telemetry configuration
  * 
  * @return Default configuration
  */
 polycall_telemetry_config_t polycall_telemetry_config_create_default(void);
 
 /**
  * @brief Validate telemetry configuration
  * 
  * @param config Configuration to validate
  * @param error_message Buffer to receive error message
  * @param buffer_size Size of error message buffer
  * @return Whether configuration is valid
  */
 bool polycall_telemetry_config_validate(
     const polycall_telemetry_config_t* config,
     char* error_message,
     size_t buffer_size
 );
 
 /**
  * @brief Cleanup telemetry configuration
  * 
  * @param core_ctx Core context
  * @param config_ctx Configuration context to clean up
  */
 void polycall_telemetry_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_config_context_t* config_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_TELEMETRY_TELEMETRY_CONFIG_H_H */