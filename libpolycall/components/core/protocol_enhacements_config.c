/**
#include "polycall/core/protocol/enhancements/protocol_enhacements_config.h"

 * @file protocol_enhancements_config.c
 * @brief Configuration Implementation for LibPolyCall Protocol Enhancements
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements the comprehensive configuration module for protocol enhancement
 * components, providing a unified initialization and management interface.
 */

 #include "polycall/core/config/protocol_enhacements_config.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 
 // Forward declarations for internal configuration handlers
 static polycall_core_error_t config_advanced_security(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_security_config_t* security_config
 );
 
 static polycall_core_error_t config_connection_pool(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_pool_config_t* pool_config
 );
 
 static polycall_core_error_t config_hierarchical_state(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     polycall_state_machine_t* state_machine
 );
 
 static polycall_core_error_t config_message_optimization(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_optimization_config_t* optimization_config
 );
 
 static polycall_core_error_t config_subscription(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_subscription_config_t* subscription_config
 );
 
 /**
  * @brief Initialize protocol enhancements with configuration
  */
 polycall_core_error_t polycall_protocol_enhancements_config_init(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t** enhancements_ctx,
     const polycall_protocol_enhancements_config_t* config
 ) {
     if (!core_ctx || !proto_ctx || !enhancements_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate enhancements context
     polycall_protocol_enhancements_context_t* new_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_protocol_enhancements_context_t));
     
     if (!new_ctx) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate protocol enhancements context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_protocol_enhancements_context_t));
     
     // Store the protocol context reference
     new_ctx->proto_ctx = proto_ctx;
     
     // Store configuration settings
     memcpy(&new_ctx->config, config, sizeof(polycall_protocol_enhancements_config_t));
     
     // Initialize each module as requested with appropriate configuration
     polycall_core_error_t result = POLYCALL_CORE_SUCCESS;
     
     // 1. Advanced Security
     if (config->enable_advanced_security) {
         result = config_advanced_security(
             core_ctx,
             new_ctx,
             &config->security_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure advanced security");
             polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
             return result;
         }
     }
     
     // 2. Connection Pool
     if (config->enable_connection_pool) {
         result = config_connection_pool(
             core_ctx,
             new_ctx,
             &config->pool_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure connection pool");
             polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
             return result;
         }
     }
     
     // 3. Hierarchical State Machine
     if (config->enable_hierarchical_state) {
         // For hierarchical state, we need the underlying state machine from the protocol context
         if (proto_ctx && proto_ctx->state_machine) {
             result = config_hierarchical_state(
                 core_ctx,
                 new_ctx,
                 proto_ctx->state_machine
             );
             
             if (result != POLYCALL_CORE_SUCCESS) {
                 POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                                   result,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to configure hierarchical state machine");
                 polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
                 return result;
             }
         } else {
             // Cannot initialize hierarchical state without a state machine
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               POLYCALL_CORE_ERROR_INVALID_STATE,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Cannot configure hierarchical state without state machine");
             polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
             return POLYCALL_CORE_ERROR_INVALID_STATE;
         }
     }
     
     // 4. Message Optimization
     if (config->enable_message_optimization) {
         result = config_message_optimization(
             core_ctx,
             proto_ctx,
             new_ctx,
             &config->optimization_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure message optimization");
             polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
             return result;
         }
     }
     
     // 5. Subscription System
     if (config->enable_subscription) {
         result = config_subscription(
             core_ctx,
             proto_ctx,
             new_ctx,
             &config->subscription_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure subscription system");
             polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
             return result;
         }
     }
     
     // Apply enhancements to protocol context by registering callbacks
     result = polycall_protocol_enhancements_register_callbacks(core_ctx, proto_ctx, new_ctx);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to register protocol enhancement callbacks");
         polycall_protocol_enhancements_config_cleanup(core_ctx, new_ctx);
         return result;
     }
     
     *enhancements_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up protocol enhancements configuration
  */
 void polycall_protocol_enhancements_config_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_protocol_enhancements_context_t* enhancements_ctx
 ) {
     if (!core_ctx || !enhancements_ctx) {
         return;
     }
     
     // Clean up each initialized module
     
     // 1. Unregister callbacks from protocol context if needed
     if (enhancements_ctx->proto_ctx) {
         polycall_protocol_enhancements_unregister_callbacks(
             core_ctx, 
             enhancements_ctx->proto_ctx, 
             enhancements_ctx
         );
     }
     
     // 2. Subscription System (clean up in reverse order of initialization)
     if (enhancements_ctx->subscription_ctx) {
         polycall_subscription_cleanup(core_ctx, enhancements_ctx->subscription_ctx);
         enhancements_ctx->subscription_ctx = NULL;
     }
     
     // 3. Message Optimization
     if (enhancements_ctx->optimization_ctx) {
         polycall_message_optimization_cleanup(core_ctx, enhancements_ctx->optimization_ctx);
         enhancements_ctx->optimization_ctx = NULL;
     }
     
     // 4. Hierarchical State Machine
     if (enhancements_ctx->hierarchical_ctx) {
         polycall_hierarchical_state_cleanup(core_ctx, enhancements_ctx->hierarchical_ctx);
         enhancements_ctx->hierarchical_ctx = NULL;
     }
     
     // 5. Connection Pool
     if (enhancements_ctx->pool_ctx) {
         polycall_connection_pool_cleanup(core_ctx, enhancements_ctx->pool_ctx);
         enhancements_ctx->pool_ctx = NULL;
     }
     
     // 6. Advanced Security
     if (enhancements_ctx->security_ctx) {
         polycall_advanced_security_cleanup(core_ctx, enhancements_ctx->security_ctx);
         enhancements_ctx->security_ctx = NULL;
     }
     
     // Free context
     polycall_core_free(core_ctx, enhancements_ctx);
 }
 
 /**
  * @brief Create default enhancements configuration
  */
 polycall_protocol_enhancements_config_t polycall_protocol_enhancements_default_config(void) {
     polycall_protocol_enhancements_config_t config;
     
     // Initialize defaults
     memset(&config, 0, sizeof(polycall_protocol_enhancements_config_t));
     
     // Enable all enhancements by default with moderate settings
     config.enable_advanced_security = true;
     config.enable_connection_pool = true;
     config.enable_hierarchical_state = true;
     config.enable_message_optimization = true;
     config.enable_subscription = true;
     
     // Setup default security configuration
     config.security_config.security_level = PROTOCOL_SECURITY_LEVEL_MEDIUM;
     config.security_config.enable_zero_trust = true;
     config.security_config.enable_encryption = true;
     config.security_config.audit_level = PROTOCOL_AUDIT_LEVEL_STANDARD;
     config.security_config.max_auth_attempts = 3;
     
     // Setup default connection pool configuration
     config.pool_config.max_connections = 16;
     config.pool_config.idle_timeout_ms = 60000;  // 1 minute
     config.pool_config.connection_timeout_ms = 5000;  // 5 seconds
     config.pool_config.enable_keep_alive = true;
     config.pool_config.keep_alive_interval_ms = 30000;  // 30 seconds
     
     // Setup default message optimization configuration
     config.optimization_config.enable_compression = true;
     config.optimization_config.compression_level = PROTOCOL_COMPRESSION_BALANCED;
     config.optimization_config.enable_batching = true;
     config.optimization_config.max_batch_size = 64;
     config.optimization_config.batch_timeout_ms = 100;  // 100 milliseconds
     
     // Setup default subscription configuration
     config.subscription_config.max_subscriptions = 1000;
     config.subscription_config.enable_wildcards = true;
     config.subscription_config.max_subscribers_per_topic = 100;
     config.subscription_config.delivery_attempt_count = 3;
     
     return config;
 }
 
 /**
  * @brief Parse configuration from file
  */
 polycall_core_error_t polycall_protocol_enhancements_load_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     polycall_protocol_enhancements_config_t* config
 ) {
     if (!core_ctx || !config_file || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Start with default configuration
     *config = polycall_protocol_enhancements_default_config();
     
     // TODO: Implement configuration file parsing
     // This would use the ConfigParser system to load and parse the configuration file
     // For now, we'll just use the defaults and return success
     
     POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                        POLYCALL_CORE_SUCCESS,
                        POLYCALL_ERROR_SEVERITY_INFO, 
                        "Using default protocol enhancements configuration");
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Register protocol enhancement callbacks
  */
 polycall_core_error_t polycall_protocol_enhancements_register_callbacks(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx
 ) {
     if (!core_ctx || !proto_ctx || !enh_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Register callbacks for each enabled enhancement
     
     // 1. Advanced Security callbacks if enabled
     if (enh_ctx->security_ctx) {
         // Register security validation callback
         polycall_protocol_register_validation_callback(
             core_ctx,
             proto_ctx,
             polycall_advanced_security_validate_message,
             enh_ctx->security_ctx
         );
     }
     
     // 2. Connection Pool callbacks if enabled
     if (enh_ctx->pool_ctx) {
         // Register connection management callbacks
         polycall_protocol_register_connection_callback(
             core_ctx,
             proto_ctx,
             PROTOCOL_CONNECTION_EVENT_CREATED,
             polycall_connection_pool_on_connection_created,
             enh_ctx->pool_ctx
         );
         
         polycall_protocol_register_connection_callback(
             core_ctx,
             proto_ctx,
             PROTOCOL_CONNECTION_EVENT_CLOSED,
             polycall_connection_pool_on_connection_closed,
             enh_ctx->pool_ctx
         );
     }
     
     // 3. Message Optimization callbacks if enabled
     if (enh_ctx->optimization_ctx) {
         // Register message interceptor for optimization
         polycall_protocol_register_message_interceptor(
             core_ctx,
             proto_ctx,
             PROTOCOL_MESSAGE_DIRECTION_OUTBOUND,
             polycall_message_optimization_process_outbound,
             enh_ctx->optimization_ctx
         );
         
         polycall_protocol_register_message_interceptor(
             core_ctx,
             proto_ctx,
             PROTOCOL_MESSAGE_DIRECTION_INBOUND,
             polycall_message_optimization_process_inbound,
             enh_ctx->optimization_ctx
         );
     }
     
     // 4. Subscription callbacks if enabled
     if (enh_ctx->subscription_ctx) {
         // Register subscription handlers
         polycall_protocol_register_message_handler(
             core_ctx,
             proto_ctx,
             "subscribe",
             polycall_subscription_handle_subscribe,
             enh_ctx->subscription_ctx
         );
         
         polycall_protocol_register_message_handler(
             core_ctx,
             proto_ctx,
             "unsubscribe",
             polycall_subscription_handle_unsubscribe,
             enh_ctx->subscription_ctx
         );
         
         polycall_protocol_register_message_handler(
             core_ctx,
             proto_ctx,
             "publish",
             polycall_subscription_handle_publish,
             enh_ctx->subscription_ctx
         );
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Unregister protocol enhancement callbacks
  */
 polycall_core_error_t polycall_protocol_enhancements_unregister_callbacks(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx
 ) {
     if (!core_ctx || !proto_ctx || !enh_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Unregister callbacks for each enabled enhancement
     
     // 1. Advanced Security callbacks if enabled
     if (enh_ctx->security_ctx) {
         // Unregister security validation callback
         polycall_protocol_unregister_validation_callback(
             core_ctx,
             proto_ctx,
             polycall_advanced_security_validate_message
         );
     }
     
     // 2. Connection Pool callbacks if enabled
     if (enh_ctx->pool_ctx) {
         // Unregister connection management callbacks
         polycall_protocol_unregister_connection_callback(
             core_ctx,
             proto_ctx,
             PROTOCOL_CONNECTION_EVENT_CREATED,
             polycall_connection_pool_on_connection_created
         );
         
         polycall_protocol_unregister_connection_callback(
             core_ctx,
             proto_ctx,
             PROTOCOL_CONNECTION_EVENT_CLOSED,
             polycall_connection_pool_on_connection_closed
         );
     }
     
     // 3. Message Optimization callbacks if enabled
     if (enh_ctx->optimization_ctx) {
         // Unregister message interceptors
         polycall_protocol_unregister_message_interceptor(
             core_ctx,
             proto_ctx,
             PROTOCOL_MESSAGE_DIRECTION_OUTBOUND,
             polycall_message_optimization_process_outbound
         );
         
         polycall_protocol_unregister_message_interceptor(
             core_ctx,
             proto_ctx,
             PROTOCOL_MESSAGE_DIRECTION_INBOUND,
             polycall_message_optimization_process_inbound
         );
     }
     
     // 4. Subscription callbacks if enabled
     if (enh_ctx->subscription_ctx) {
         // Unregister subscription handlers
         polycall_protocol_unregister_message_handler(
             core_ctx,
             proto_ctx,
             "subscribe",
             polycall_subscription_handle_subscribe
         );
         
         polycall_protocol_unregister_message_handler(
             core_ctx,
             proto_ctx,
             "unsubscribe",
             polycall_subscription_handle_unsubscribe
         );
         
         polycall_protocol_unregister_message_handler(
             core_ctx,
             proto_ctx,
             "publish",
             polycall_subscription_handle_publish
         );
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /*---------------------------------------------------------------------------*/
 /* Internal configuration implementation functions */
 /*---------------------------------------------------------------------------*/
 
 /**
  * @brief Configure advanced security module
  */
 static polycall_core_error_t config_advanced_security(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_security_config_t* security_config
 ) {
     // Initialize advanced security with the provided configuration
     return polycall_advanced_security_init(
         ctx,
         &enh_ctx->security_ctx,
         security_config
     );
 }
 
 /**
  * @brief Configure connection pool module
  */
 static polycall_core_error_t config_connection_pool(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_pool_config_t* pool_config
 ) {
     // Initialize connection pool with the provided configuration
     return polycall_connection_pool_init(
         ctx,
         &enh_ctx->pool_ctx,
         pool_config
     );
 }
 
 /**
  * @brief Configure hierarchical state machine module
  */
 static polycall_core_error_t config_hierarchical_state(
     polycall_core_context_t* ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     polycall_state_machine_t* state_machine
 ) {
     // Initialize hierarchical state machine with the provided state machine
     return polycall_hierarchical_state_init(
         ctx,
         &enh_ctx->hierarchical_ctx,
         state_machine
     );
 }
 
 /**
  * @brief Configure message optimization module
  */
 static polycall_core_error_t config_message_optimization(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_optimization_config_t* optimization_config
 ) {
     // Initialize message optimization with the provided configuration
     return polycall_message_optimization_init(
         ctx,
         proto_ctx,
         &enh_ctx->optimization_ctx,
         optimization_config
     );
 }
 
 /**
  * @brief Configure subscription system module
  */
 static polycall_core_error_t config_subscription(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_enhancements_context_t* enh_ctx,
     const protocol_enhancement_subscription_config_t* subscription_config
 ) {
     // Initialize subscription system with the provided configuration
     return polycall_subscription_init(
         ctx,
         proto_ctx,
         &enh_ctx->subscription_ctx,
         subscription_config
     );
 }