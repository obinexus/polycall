/**
 * @file polycall_telemetry.h
 * @brief Core Telemetry Infrastructure for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Defines the foundational telemetry collection and reporting mechanisms
 * for comprehensive system observability and diagnostics.
 */

 #ifndef POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_H_H
 #define POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_H_H
 
 #include "../core/polycall_core.h"
 #include "../ffi/polycall_ffi.h"
 #include "../edge/polycall_edge.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Telemetry event severity levels
  */
 typedef enum {
     POLYCALL_TELEMETRY_DEBUG = 0,      // Lowest verbosity, detailed debugging
     POLYCALL_TELEMETRY_INFO = 1,       // Informational events
     POLYCALL_TELEMETRY_WARNING = 2,    // Potential issues or performance concerns
     POLYCALL_TELEMETRY_ERROR = 3,      // Significant operational errors
     POLYCALL_TELEMETRY_CRITICAL = 4    // Severe system threats or failures
 } polycall_telemetry_severity_t;
 
 /**
  * @brief Telemetry event categories
  */
 typedef enum {
     TELEMETRY_CATEGORY_SYSTEM = 0,     // Core system operations
     TELEMETRY_CATEGORY_PERFORMANCE = 1,// Performance metrics
     TELEMETRY_CATEGORY_SECURITY = 2,   // Security-related events
     TELEMETRY_CATEGORY_NETWORK = 3,    // Network communication events
     TELEMETRY_CATEGORY_FFI = 4,        // Foreign Function Interface events
     TELEMETRY_CATEGORY_EDGE = 5,       // Edge computing events
     TELEMETRY_CATEGORY_PROTOCOL = 6    // Protocol-level events
 } polycall_telemetry_category_t;
 
 /**
  * @brief Telemetry event structure
  */
// Add to polycall_telemetry.h in the polycall_telemetry_event_t structure:

typedef struct {
    uint64_t timestamp;                    // Event timestamp (nanoseconds since epoch)
    polycall_telemetry_severity_t severity; // Event severity level
    polycall_telemetry_category_t category; // Event category
    const char* source_module;             // Module generating the event
    const char* event_id;                  // Unique event identifier
    const char* description;               // Human-readable event description
    void* additional_data;                 // Optional additional context
    size_t additional_data_size;           // Size of additional data
    polycall_guid_t event_guid;            //d State tracking GUID
    polycall_guid_t parent_guid;           // Parent event GUID if applicable
} polycall_telemetry_event_t;
 /**
  * @brief Telemetry configuration structure
  */
 typedef struct {
     bool enable_telemetry;                 // Global telemetry enable/disable
     polycall_telemetry_severity_t min_severity; // Minimum severity to log
     uint32_t max_event_queue_size;         // Maximum number of events to queue
     bool enable_encryption;                // Encrypt sensitive telemetry data
     bool enable_compression;               // Compress telemetry logs
     const char* log_file_path;             // Path for persistent log storage
     uint32_t log_rotation_size_mb;         // Log file rotation size
 } polycall_telemetry_config_t;
 
 /**
  * @brief Telemetry context opaque structure
  */
 typedef struct polycall_telemetry_context polycall_telemetry_context_t;
 
 /**
  * @brief Telemetry event callback function type
  */
 typedef void (*polycall_telemetry_callback_t)(
     const polycall_telemetry_event_t* event,
     void* user_data
 );
 
 /**
  * @brief Initialize telemetry system
  * 
  * @param core_ctx Core LibPolyCall context
  * @param telemetry_ctx Pointer to receive telemetry context
  * @param config Telemetry configuration
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_init(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_context_t** telemetry_ctx,
     const polycall_telemetry_config_t* config
 );
 
 /**
  * @brief Record a telemetry event
  * 
  * @param telemetry_ctx Telemetry context
  * @param event Telemetry event to record
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_record_event(
     polycall_telemetry_context_t* telemetry_ctx,
     const polycall_telemetry_event_t* event
 );
 
 /**
  * @brief Register a telemetry event callback
  * 
  * @param telemetry_ctx Telemetry context
  * @param callback Callback function
  * @param user_data User-provided context
  * @return Error code
  */
 polycall_core_error_t polycall_telemetry_register_callback(
     polycall_telemetry_context_t* telemetry_ctx,
     polycall_telemetry_callback_t callback,
     void* user_data
 );
 
 /**
  * @brief Create default telemetry configuration
  * 
  * @return Default telemetry configuration
  */
 polycall_telemetry_config_t polycall_telemetry_create_default_config(void);
 
 /**
  * @brief Cleanup telemetry system
  * 
  * @param core_ctx Core LibPolyCall context
  * @param telemetry_ctx Telemetry context to clean up
  */
 void polycall_telemetry_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_telemetry_context_t* telemetry_ctx
 );

 
/**
 * @brief Telemetry context structure
 */
struct polycall_telemetry_context {
    bool initialized;                      // Initialization flag
    polycall_telemetry_config_t config;   // Configuration settings
    polycall_identifier_format_t id_format; // Format for event identifiers
    polycall_telemetry_callback_t callback; // Event callback
    void* callback_data;                   // Callback user data
    // Other implementation-specific fields...
};

/**
 * @brief Initialize telemetry system
 * 
 * @param core_ctx Core LibPolyCall context
 * @param telemetry_ctx Pointer to receive telemetry context 
 * @param config Telemetry configuration
 * @return Error code
 *
 * Note: Inherits identifier format from core context
 */
polycall_core_error_t polycall_telemetry_init(
    polycall_core_context_t* core_ctx,
    polycall_telemetry_context_t** telemetry_ctx, 
    const polycall_telemetry_config_t* config
);
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_TELEMETRY_POLYCALL_TELEMETRY_H_H */