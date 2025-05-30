/**
#include "polycall/core/protocol/protocol_config.h"

 * @file protocol_config.c
 * @brief Comprehensive Protocol Configuration Implementation for LibPolyCall
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements the comprehensive configuration module for the protocol
 * and all its enhancements, providing a unified initialization and
 * management interface.
 */

 #include "polycall/core/protocol/protocol_config.h"
 #include "polycall/core/protocol/enhancements/protocol_enhacements_config.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/protocol/protocol_state_machine.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 // Forward declarations for internal configuration handlers
 static polycall_core_error_t config_core_protocol(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_core_config_t* core_config
 );
 
 static polycall_core_error_t config_tls(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_tls_config_t* tls_config
 );
 
 static polycall_core_error_t config_serialization(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_serialization_config_t* serialization_config
 );
 
 static polycall_core_error_t config_state_machine(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_state_machine_config_t* state_machine_config
 );
 
 static polycall_core_error_t config_command(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_command_config_t* command_config
 );
 
 static polycall_core_error_t config_handshake(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_handshake_config_t* handshake_config
 );
 
 static polycall_core_error_t config_crypto(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_crypto_config_t* crypto_config
 );
 
 // Helper function to duplicate strings safely
 static char* duplicate_string(polycall_core_context_t* ctx, const char* src) {
     if (!src) {
         return NULL;
     }
     
     size_t len = strlen(src) + 1;
     char* dest = polycall_core_malloc(ctx, len);
     if (!dest) {
         return NULL;
     }
     
     memcpy(dest, src, len);
     return dest;
 }
 
 /**
  * @brief Initialize protocol with configuration
  */
 polycall_core_error_t polycall_protocol_config_init(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_config_t* config
 ) {
     if (!core_ctx || !proto_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize protocol context (assuming it has not been initialized yet)
     polycall_core_error_t result = polycall_protocol_context_init(core_ctx, proto_ctx);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize protocol context");
         return result;
     }
     
     // Apply configuration
     result = polycall_protocol_apply_config(core_ctx, proto_ctx, config);
     if (result != POLYCALL_CORE_SUCCESS) {
         // Clean up if configuration failed
         polycall_protocol_context_cleanup(core_ctx, proto_ctx);
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply configuration to protocol context
  */
 polycall_core_error_t polycall_protocol_apply_config(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_config_t* config
 ) {
     if (!core_ctx || !proto_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_core_error_t result;
     
     // 1. Configure core protocol
     result = config_core_protocol(core_ctx, proto_ctx, &config->core);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure core protocol");
         return result;
     }
     
     // 2. Configure TLS
     if (config->core.enable_tls) {
         result = config_tls(core_ctx, proto_ctx, &config->tls);
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure TLS");
             return result;
         }
     }
     
     // 3. Configure serialization
     result = config_serialization(core_ctx, proto_ctx, &config->serialization);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure serialization");
         return result;
     }
     
     // 4. Configure state machine
     result = config_state_machine(core_ctx, proto_ctx, &config->state_machine);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure state machine");
         return result;
     }
     
     // 5. Configure command handling
     result = config_command(core_ctx, proto_ctx, &config->command);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure command handling");
         return result;
     }
     
     // 6. Configure handshake
     result = config_handshake(core_ctx, proto_ctx, &config->handshake);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure handshake");
         return result;
     }
     
     // 7. Configure crypto
     result = config_crypto(core_ctx, proto_ctx, &config->crypto);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to configure crypto");
         return result;
     }
     
     // 8. Initialize and configure enhancements
     if (config->enhancements.enable_advanced_security ||
         config->enhancements.enable_connection_pool ||
         config->enhancements.enable_hierarchical_state ||
         config->enhancements.enable_message_optimization ||
         config->enhancements.enable_subscription) {
         
         // Apply enhancements configuration
         result = polycall_protocol_enhancements_apply_config(
             core_ctx, proto_ctx, &config->enhancements);
             
         if (result != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                               result,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to configure protocol enhancements");
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default protocol configuration
  */
 protocol_config_t polycall_protocol_default_config(void) {
     protocol_config_t config;
     
     // Initialize with zeros
     memset(&config, 0, sizeof(protocol_config_t));
     
     // Core configuration defaults
     config.core.transport_type = PROTOCOL_TRANSPORT_TCP;
     config.core.encoding_format = PROTOCOL_ENCODING_JSON;
     config.core.validation_level = PROTOCOL_VALIDATION_STANDARD;
     config.core.default_timeout_ms = 30000;  // 30 seconds
     config.core.handshake_timeout_ms = 5000;  // 5 seconds
     config.core.keep_alive_interval_ms = 60000;  // 1 minute
     config.core.default_port = 8080;
     config.core.enable_tls = true;
     config.core.enable_compression = true;
     config.core.enable_auto_reconnect = true;
     config.core.retry_policy = PROTOCOL_RETRY_EXPONENTIAL;
     config.core.max_retry_count = 5;
     config.core.max_message_size = 1024 * 1024;  // 1 MB
     config.core.strict_mode = false;
     
     // TLS configuration defaults
     config.tls.cert_file = NULL;  // No default path, must be set by user
     config.tls.key_file = NULL;   // No default path, must be set by user
     config.tls.ca_file = NULL;    // No default path, must be set by user
     config.tls.verify_peer = true;
     config.tls.allow_self_signed = false;
     config.tls.cipher_list = "HIGH:!aNULL:!MD5:!RC4";
     config.tls.min_tls_version = 0x0303;  // TLS 1.2
     
     // Serialization configuration defaults
     config.serialization.enable_schema_validation = true;
     config.serialization.enable_field_caching = true;
     config.serialization.enable_serialization_optimization = true;
     config.serialization.enable_null_suppression = true;
     config.serialization.enable_binary_format = false;
     config.serialization.max_depth = 32;
     
     // State machine configuration defaults
     config.state_machine.enable_state_logging = true;
     config.state_machine.enable_state_metrics = true;
     config.state_machine.strict_transitions = true;
     config.state_machine.state_timeout_ms = 60000;  // 1 minute
     config.state_machine.enable_recovery_transitions = true;
     
     // Command configuration defaults
     config.command.enable_command_queuing = true;
     config.command.command_queue_size = 100;
     config.command.command_timeout_ms = 30000;  // 30 seconds
     config.command.enable_command_prioritization = true;
     config.command.enable_command_retry = true;
     config.command.max_concurrent_commands = 10;
     
     // Handshake configuration defaults
     config.handshake.enable_version_negotiation = true;
     config.handshake.enable_capability_negotiation = true;
     config.handshake.enable_authentication = true;
     config.handshake.enable_identity_verification = true;
     config.handshake.handshake_retry_count = 3;
     config.handshake.handshake_retry_interval_ms = 1000;  // 1 second
     
     // Crypto configuration defaults
     config.crypto.enable_encryption = true;
     config.crypto.enable_signing = true;
     config.crypto.encryption_algorithm = "AES-256-GCM";
     config.crypto.signing_algorithm = "HMAC-SHA256";
     config.crypto.key_exchange_method = "ECDHE";
     config.crypto.key_rotation_interval_ms = 3600000;  // 1 hour
     config.crypto.enable_perfect_forward_secrecy = true;
     
     // Enhancements configuration defaults
     config.enhancements = polycall_protocol_enhancements_default_config();
     
     return config;
 }
 
 /**
  * @brief Load protocol configuration from file
  */
 polycall_core_error_t polycall_protocol_load_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     protocol_config_t* config
 ) {
     if (!core_ctx || !config_file || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Start with default configuration
     *config = polycall_protocol_default_config();
     
     // TODO: Implement configuration file parsing
     // This would use the ConfigParser system to load and parse the configuration file
     // For now, we'll just use the defaults and return success
     
     POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                        POLYCALL_CORE_SUCCESS,
                        POLYCALL_ERROR_SEVERITY_INFO, 
                        "Using default protocol configuration");
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Save protocol configuration to file
  */
 polycall_core_error_t polycall_protocol_save_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     const protocol_config_t* config
 ) {
     if (!core_ctx || !config_file || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // TODO: Implement configuration file serialization
     // This would serialize the configuration to a file in the chosen format
     // For now, return a not implemented error
     
     POLYCALL_ERROR_SET(core_ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                        POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                        POLYCALL_ERROR_SEVERITY_ERROR, 
                        "Configuration saving not implemented yet");
     
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Validate protocol configuration
  */
 bool polycall_protocol_validate_config(
     polycall_core_context_t* core_ctx,
     const protocol_config_t* config,
     char* error_message,
     size_t message_size
 ) {
     if (!core_ctx || !config) {
         if (error_message && message_size > 0) {
             snprintf(error_message, message_size, "Invalid parameters");
         }
         return false;
     }
     
     // Check for essential configurations first
     
     // 1. Validate transport configuration
     if (config->core.transport_type == PROTOCOL_TRANSPORT_NONE) {
         if (error_message && message_size > 0) {
             snprintf(error_message, message_size, "Transport type must be specified");
         }
         return false;
     }
     
     // 2. Validate encoding configuration
     if (config->core.encoding_format == PROTOCOL_ENCODING_NONE) {
         if (error_message && message_size > 0) {
             snprintf(error_message, message_size, "Encoding format must be specified");
         }
         return false;
     }
     
     // 3. Validate TLS configuration if enabled
     if (config->core.enable_tls) {
         // If TLS is enabled, cert and key files should be provided
         // (in practice, there might be exceptions like development mode)
         if (!config->tls.cert_file || !config->tls.key_file) {
             if (error_message && message_size > 0) {
                 snprintf(error_message, message_size, 
                         "TLS is enabled but certificate or key file is missing");
             }
             return false;
         }
     }
     
     // 4. Validate timeouts
     if (config->core.default_timeout_ms == 0 || 
         config->core.handshake_timeout_ms == 0) {
         if (error_message && message_size > 0) {
             snprintf(error_message, message_size, "Timeouts cannot be zero");
         }
         return false;
     }
     
     // 5. Validate crypto configuration if enabled
     if (config->crypto.enable_encryption || config->crypto.enable_signing) {
         if (!config->crypto.encryption_algorithm || !config->crypto.signing_algorithm) {
             if (error_message && message_size > 0) {
                 snprintf(error_message, message_size, 
                         "Crypto enabled but algorithms not specified");
             }
             return false;
         }
     }
     
     // 6. Validate command configuration
     if (config->command.enable_command_queuing && config->command.command_queue_size == 0) {
         if (error_message && message_size > 0) {
             snprintf(error_message, message_size, 
                     "Command queuing enabled but queue size is zero");
         }
         return false;
     }
     
     // 7. Validate enhancements configuration
     if (config->enhancements.enable_advanced_security || 
         config->enhancements.enable_connection_pool ||
         config->enhancements.enable_hierarchical_state ||
         config->enhancements.enable_message_optimization ||
         config->enhancements.enable_subscription) {
         
         // TODO: Implement enhancement-specific validation
         // For now, we'll assume the enhancement configurations are valid
     }
     
     // All validation checks passed
     return true;
 }
 
 /**
  * @brief Merge protocol configurations
  */
 polycall_core_error_t polycall_protocol_merge_config(
     polycall_core_context_t* core_ctx,
     protocol_config_t* dest,
     const protocol_config_t* src
 ) {
     if (!core_ctx || !dest || !src) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get default configuration for comparison
     protocol_config_t default_config = polycall_protocol_default_config();
     
     // Core configuration
     if (src->core.transport_type != default_config.core.transport_type) {
         dest->core.transport_type = src->core.transport_type;
     }
     
     if (src->core.encoding_format != default_config.core.encoding_format) {
         dest->core.encoding_format = src->core.encoding_format;
     }
     
     if (src->core.validation_level != default_config.core.validation_level) {
         dest->core.validation_level = src->core.validation_level;
     }
     
     if (src->core.default_timeout_ms != default_config.core.default_timeout_ms) {
         dest->core.default_timeout_ms = src->core.default_timeout_ms;
     }
     
     if (src->core.handshake_timeout_ms != default_config.core.handshake_timeout_ms) {
         dest->core.handshake_timeout_ms = src->core.handshake_timeout_ms;
     }
     
     if (src->core.keep_alive_interval_ms != default_config.core.keep_alive_interval_ms) {
         dest->core.keep_alive_interval_ms = src->core.keep_alive_interval_ms;
     }
     
     if (src->core.default_port != default_config.core.default_port) {
         dest->core.default_port = src->core.default_port;
     }
     
     if (src->core.enable_tls != default_config.core.enable_tls) {
         dest->core.enable_tls = src->core.enable_tls;
     }
     
     if (src->core.enable_compression != default_config.core.enable_compression) {
         dest->core.enable_compression = src->core.enable_compression;
     }
     
     if (src->core.enable_auto_reconnect != default_config.core.enable_auto_reconnect) {
         dest->core.enable_auto_reconnect = src->core.enable_auto_reconnect;
     }
     
     if (src->core.retry_policy != default_config.core.retry_policy) {
         dest->core.retry_policy = src->core.retry_policy;
     }
     
     if (src->core.max_retry_count != default_config.core.max_retry_count) {
         dest->core.max_retry_count = src->core.max_retry_count;
     }
     
     if (src->core.max_message_size != default_config.core.max_message_size) {
         dest->core.max_message_size = src->core.max_message_size;
     }
     
     if (src->core.strict_mode != default_config.core.strict_mode) {
         dest->core.strict_mode = src->core.strict_mode;
     }
     
     // TLS configuration
     if (src->tls.cert_file) {
         // Free any existing string in dest
         if (dest->tls.cert_file) {
             polycall_core_free(core_ctx, (void*)dest->tls.cert_file);
         }
         dest->tls.cert_file = duplicate_string(core_ctx, src->tls.cert_file);
     }
     
     if (src->tls.key_file) {
         if (dest->tls.key_file) {
             polycall_core_free(core_ctx, (void*)dest->tls.key_file);
         }
         dest->tls.key_file = duplicate_string(core_ctx, src->tls.key_file);
     }
     
     if (src->tls.ca_file) {
         if (dest->tls.ca_file) {
             polycall_core_free(core_ctx, (void*)dest->tls.ca_file);
         }
         dest->tls.ca_file = duplicate_string(core_ctx, src->tls.ca_file);
     }
     
     if (src->tls.verify_peer != default_config.tls.verify_peer) {
         dest->tls.verify_peer = src->tls.verify_peer;
     }
     
     if (src->tls.allow_self_signed != default_config.tls.allow_self_signed) {
         dest->tls.allow_self_signed = src->tls.allow_self_signed;
     }
     
     if (src->tls.cipher_list && strcmp(src->tls.cipher_list, default_config.tls.cipher_list) != 0) {
         if (dest->tls.cipher_list) {
             polycall_core_free(core_ctx, (void*)dest->tls.cipher_list);
         }
         dest->tls.cipher_list = duplicate_string(core_ctx, src->tls.cipher_list);
     }
     
     if (src->tls.min_tls_version != default_config.tls.min_tls_version) {
         dest->tls.min_tls_version = src->tls.min_tls_version;
     }
     
     // Serialization configuration
     if (src->serialization.enable_schema_validation != default_config.serialization.enable_schema_validation) {
         dest->serialization.enable_schema_validation = src->serialization.enable_schema_validation;
     }
     
     if (src->serialization.enable_field_caching != default_config.serialization.enable_field_caching) {
         dest->serialization.enable_field_caching = src->serialization.enable_field_caching;
     }
     
     if (src->serialization.enable_serialization_optimization != default_config.serialization.enable_serialization_optimization) {
         dest->serialization.enable_serialization_optimization = src->serialization.enable_serialization_optimization;
     }
     
     if (src->serialization.enable_null_suppression != default_config.serialization.enable_null_suppression) {
         dest->serialization.enable_null_suppression = src->serialization.enable_null_suppression;
     }
     
     if (src->serialization.enable_binary_format != default_config.serialization.enable_binary_format) {
         dest->serialization.enable_binary_format = src->serialization.enable_binary_format;
     }
     
     if (src->serialization.max_depth != default_config.serialization.max_depth) {
         dest->serialization.max_depth = src->serialization.max_depth;
     }
     
     // State machine configuration
     if (src->state_machine.enable_state_logging != default_config.state_machine.enable_state_logging) {
         dest->state_machine.enable_state_logging = src->state_machine.enable_state_logging;
     }
     
     if (src->state_machine.enable_state_metrics != default_config.state_machine.enable_state_metrics) {
         dest->state_machine.enable_state_metrics = src->state_machine.enable_state_metrics;
     }
     
     if (src->state_machine.strict_transitions != default_config.state_machine.strict_transitions) {
         dest->state_machine.strict_transitions = src->state_machine.strict_transitions;
     }
     
     if (src->state_machine.state_timeout_ms != default_config.state_machine.state_timeout_ms) {
         dest->state_machine.state_timeout_ms = src->state_machine.state_timeout_ms;
     }
     
     if (src->state_machine.enable_recovery_transitions != default_config.state_machine.enable_recovery_transitions) {
         dest->state_machine.enable_recovery_transitions = src->state_machine.enable_recovery_transitions;
     }
     
     // Command configuration
     if (src->command.enable_command_queuing != default_config.command.enable_command_queuing) {
         dest->command.enable_command_queuing = src->command.enable_command_queuing;
     }
     
     if (src->command.command_queue_size != default_config.command.command_queue_size) {
         dest->command.command_queue_size = src->command.command_queue_size;
     }
     
     if (src->command.command_timeout_ms != default_config.command.command_timeout_ms) {
         dest->command.command_timeout_ms = src->command.command_timeout_ms;
     }
     
     if (src->command.enable_command_prioritization != default_config.command.enable_command_prioritization) {
         dest->command.enable_command_prioritization = src->command.enable_command_prioritization;
     }
     
     if (src->command.enable_command_retry != default_config.command.enable_command_retry) {
         dest->command.enable_command_retry = src->command.enable_command_retry;
     }
     
     if (src->command.max_concurrent_commands != default_config.command.max_concurrent_commands) {
         dest->command.max_concurrent_commands = src->command.max_concurrent_commands;
     }
     
     // Handshake configuration
     if (src->handshake.enable_version_negotiation != default_config.handshake.enable_version_negotiation) {
         dest->handshake.enable_version_negotiation = src->handshake.enable_version_negotiation;
     }
     
     if (src->handshake.enable_capability_negotiation != default_config.handshake.enable_capability_negotiation) {
         dest->handshake.enable_capability_negotiation = src->handshake.enable_capability_negotiation;
     }
     
     if (src->handshake.enable_authentication != default_config.handshake.enable_authentication) {
         dest->handshake.enable_authentication = src->handshake.enable_authentication;
     }
     
     if (src->handshake.enable_identity_verification != default_config.handshake.enable_identity_verification) {
         dest->handshake.enable_identity_verification = src->handshake.enable_identity_verification;
     }
     
     if (src->handshake.handshake_retry_count != default_config.handshake.handshake_retry_count) {
         dest->handshake.handshake_retry_count = src->handshake.handshake_retry_count;
     }
     
     if (src->handshake.handshake_retry_interval_ms != default_config.handshake.handshake_retry_interval_ms) {
         dest->handshake.handshake_retry_interval_ms = src->handshake.handshake_retry_interval_ms;
     }
     
     // Crypto configuration
     if (src->crypto.enable_encryption != default_config.crypto.enable_encryption) {
         dest->crypto.enable_encryption = src->crypto.enable_encryption;
     }
     
     if (src->crypto.enable_signing != default_config.crypto.enable_signing) {
         dest->crypto.enable_signing = src->crypto.enable_signing;
     }
     
     if (src->crypto.encryption_algorithm && 
         strcmp(src->crypto.encryption_algorithm, default_config.crypto.encryption_algorithm) != 0) {
         if (dest->crypto.encryption_algorithm) {
             polycall_core_free(core_ctx, (void*)dest->crypto.encryption_algorithm);
         }
         dest->crypto.encryption_algorithm = duplicate_string(core_ctx, src->crypto.encryption_algorithm);
     }
     
     if (src->crypto.signing_algorithm && 
         strcmp(src->crypto.signing_algorithm, default_config.crypto.signing_algorithm) != 0) {
         if (dest->crypto.signing_algorithm) {
             polycall_core_free(core_ctx, (void*)dest->crypto.signing_algorithm);
         }
         dest->crypto.signing_algorithm = duplicate_string(core_ctx, src->crypto.signing_algorithm);
     }
     
     if (src->crypto.key_exchange_method && 
         strcmp(src->crypto.key_exchange_method, default_config.crypto.key_exchange_method) != 0) {
         if (dest->crypto.key_exchange_method) {
             polycall_core_free(core_ctx, (void*)dest->crypto.key_exchange_method);
         }
         dest->crypto.key_exchange_method = duplicate_string(core_ctx, src->crypto.key_exchange_method);
     }
     
     if (src->crypto.key_rotation_interval_ms != default_config.crypto.key_rotation_interval_ms) {
         dest->crypto.key_rotation_interval_ms = src->crypto.key_rotation_interval_ms;
     }
     
     if (src->crypto.enable_perfect_forward_secrecy != default_config.crypto.enable_perfect_forward_secrecy) {
         dest->crypto.enable_perfect_forward_secrecy = src->crypto.enable_perfect_forward_secrecy;
     }
     
     // Enhancements configuration merging
     // For simplicity, we'll just copy over the enablement flags
     // In a real implementation, we would merge the individual enhancement configs
     
     if (src->enhancements.enable_advanced_security != default_config.enhancements.enable_advanced_security) {
         dest->enhancements.enable_advanced_security = src->enhancements.enable_advanced_security;
     }
     
     if (src->enhancements.enable_connection_pool != default_config.enhancements.enable_connection_pool) {
         dest->enhancements.enable_connection_pool = src->enhancements.enable_connection_pool;
     }
     
     if (src->enhancements.enable_hierarchical_state != default_config.enhancements.enable_hierarchical_state) {
         dest->enhancements.enable_hierarchical_state = src->enhancements.enable_hierarchical_state;
     }
     
     if (src->enhancements.enable_message_optimization != default_config.enhancements.enable_message_optimization) {
         dest->enhancements.enable_message_optimization = src->enhancements.enable_message_optimization;
     }
     
     if (src->enhancements.enable_subscription != default_config.enhancements.enable_subscription) {
         dest->enhancements.enable_subscription = src->enhancements.enable_subscription;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Copy protocol configuration
  */
 polycall_core_error_t polycall_protocol_copy_config(
     polycall_core_context_t* core_ctx,
     protocol_config_t* dest,
     const protocol_config_t* src
 ) {
     if (!core_ctx || !dest || !src) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Start with a clean slate
     memset(dest, 0, sizeof(protocol_config_t));
     
     // Copy core configuration
     dest->core = src->core;
     
     // Deep copy string fields in TLS configuration
     dest->tls.verify_peer = src->tls.verify_peer;
     dest->tls.allow_self_signed = src->tls.allow_self_signed;
     dest->tls.min_tls_version = src->tls.min_tls_version;
     
     if (src->tls.cert_file) {
         dest->tls.cert_file = duplicate_string(core_ctx, src->tls.cert_file);
         if (!dest->tls.cert_file) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     if (src->tls.key_file) {
         dest->tls.key_file = duplicate_string(core_ctx, src->tls.key_file);
         if (!dest->tls.key_file) {
             if (dest->tls.cert_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.cert_file);
             }
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     if (src->tls.ca_file) {
         dest->tls.ca_file = duplicate_string(core_ctx, src->tls.ca_file);
         if (!dest->tls.ca_file) {
             if (dest->tls.cert_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.cert_file);
             }
             if (dest->tls.key_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.key_file);
             }
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     if (src->tls.cipher_list) {
         dest->tls.cipher_list = duplicate_string(core_ctx, src->tls.cipher_list);
         if (!dest->tls.cipher_list) {
             if (dest->tls.cert_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.cert_file);
             }
             if (dest->tls.key_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.key_file);
             }
             if (dest->tls.ca_file) {
                 polycall_core_free(core_ctx, (void*)dest->tls.ca_file);
             }
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     // Copy serialization configuration
     dest->serialization = src->serialization;
     
     // Copy state machine configuration
     dest->state_machine = src->state_machine;
     
     // Copy command configuration
     dest->command = src->command;
     
     // Copy handshake configuration
     dest->handshake = src->handshake;
     
     // Deep copy string fields in crypto configuration
     dest->crypto.enable_encryption = src->crypto.enable_encryption;
     dest->crypto.enable_signing = src->crypto.enable_signing;
     dest->crypto.key_rotation_interval_ms = src->crypto.key_rotation_interval_ms;
     dest->crypto.enable_perfect_forward_secrecy = src->crypto.enable_perfect_forward_secrecy;
     
     if (src->crypto.encryption_algorithm) {
         dest->crypto.encryption_algorithm = duplicate_string(core_ctx, src->crypto.encryption_algorithm);
         if (!dest->crypto.encryption_algorithm) {
             // Clean up previous allocations
             polycall_protocol_cleanup_config_strings(core_ctx, dest);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     if (src->crypto.signing_algorithm) {
         dest->crypto.signing_algorithm = duplicate_string(core_ctx, src->crypto.signing_algorithm);
         if (!dest->crypto.signing_algorithm) {
             // Clean up previous allocations
             polycall_protocol_cleanup_config_strings(core_ctx, dest);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     if (src->crypto.key_exchange_method) {
         dest->crypto.key_exchange_method = duplicate_string(core_ctx, src->crypto.key_exchange_method);
         if (!dest->crypto.key_exchange_method) {
             // Clean up previous allocations
             polycall_protocol_cleanup_config_strings(core_ctx, dest);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
     }
     
     // Copy enhancements configuration
     dest->enhancements = src->enhancements;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get protocol configuration schema
  */
 void* polycall_protocol_get_config_schema(
     polycall_core_context_t* core_ctx
 ) {
     // This function would return a schema definition that could be used
     // for validating configurations or generating code
     // For now, return NULL (not implemented)
     return NULL;
 }
 
 /**
  * @brief Print protocol configuration
  */
 size_t polycall_protocol_print_config(
     polycall_core_context_t* core_ctx,
     const protocol_config_t* config,
     char* buffer,
     size_t size
 ) {
     if (!core_ctx || !config || !buffer || size == 0) {
         return 0;
     }
     
     size_t offset = 0;
     int written = 0;
     
     // Print core configuration
     written = snprintf(buffer + offset, size - offset,
         "Protocol Configuration:\n"
         "---------------------\n"
         "Core:\n"
         "  Transport Type: %d\n"
         "  Encoding Format: %d\n"
         "  Validation Level: %d\n"
         "  Default Timeout: %u ms\n"
         "  Handshake Timeout: %u ms\n"
         "  Keep Alive Interval: %u ms\n"
         "  Default Port: %u\n"
         "  TLS Enabled: %s\n"
         "  Compression Enabled: %s\n"
         "  Auto Reconnect Enabled: %s\n"
         "  Retry Policy: %d\n"
         "  Max Retry Count: %u\n"
         "  Max Message Size: %u bytes\n"
         "  Strict Mode: %s\n\n",
         config->core.transport_type,
         config->core.encoding_format,
         config->core.validation_level,
         config->core.default_timeout_ms,
         config->core.handshake_timeout_ms,
         config->core.keep_alive_interval_ms,
         config->core.default_port,
         config->core.enable_tls ? "Yes" : "No",
         config->core.enable_compression ? "Yes" : "No",
         config->core.enable_auto_reconnect ? "Yes" : "No",
         config->core.retry_policy,
         config->core.max_retry_count,
         config->core.max_message_size,
         config->core.strict_mode ? "Yes" : "No"
     );
     
     if (written < 0 || (size_t)written >= size - offset) {
         return offset;  // Buffer full or error
     }
     offset += (size_t)written;
     
     // Print TLS configuration if enabled
     if (config->core.enable_tls) {
         written = snprintf(buffer + offset, size - offset,
             "TLS:\n"
             "  Certificate File: %s\n"
             "  Key File: %s\n"
             "  CA File: %s\n"
             "  Verify Peer: %s\n"
             "  Allow Self-Signed: %s\n"
             "  Cipher List: %s\n"
             "  Min TLS Version: 0x%04x\n\n",
             config->tls.cert_file ? config->tls.cert_file : "Not set",
             config->tls.key_file ? config->tls.key_file : "Not set",
             config->tls.ca_file ? config->tls.ca_file : "Not set",
             config->tls.verify_peer ? "Yes" : "No",
             config->tls.allow_self_signed ? "Yes" : "No",
             config->tls.cipher_list ? config->tls.cipher_list : "Default",
             config->tls.min_tls_version
         );
         
         if (written < 0 || (size_t)written >= size - offset) {
             return offset;  // Buffer full or error
         }
         offset += (size_t)written;
     }
     
     // Print serialization configuration
     written = snprintf(buffer + offset, size - offset,
         "Serialization:\n"
         "  Schema Validation: %s\n"
         "  Field Caching: %s\n"
         "  Serialization Optimization: %s\n"
         "  Null Suppression: %s\n"
         "  Binary Format: %s\n"
         "  Max Depth: %u\n\n",
         config->serialization.enable_schema_validation ? "Yes" : "No",
         config->serialization.enable_field_caching ? "Yes" : "No",
         config->serialization.enable_serialization_optimization ? "Yes" : "No",
         config->serialization.enable_null_suppression ? "Yes" : "No",
         config->serialization.enable_binary_format ? "Yes" : "No",
         config->serialization.max_depth
     );
     
     if (written < 0 || (size_t)written >= size - offset) {
         return offset;  // Buffer full or error
     }
     offset += (size_t)written;
     
     // Add more configuration sections as needed...
     
     // Print enhancements configuration summary
     written = snprintf(buffer + offset, size - offset,
         "Enhancements:\n"
         "  Advanced Security: %s\n"
         "  Connection Pool: %s\n"
         "  Hierarchical State Machine: %s\n"
         "  Message Optimization: %s\n"
         "  Subscription System: %s\n",
         config->enhancements.enable_advanced_security ? "Enabled" : "Disabled",
         config->enhancements.enable_connection_pool ? "Enabled" : "Disabled",
         config->enhancements.enable_hierarchical_state ? "Enabled" : "Disabled",
         config->enhancements.enable_message_optimization ? "Enabled" : "Disabled",
         config->enhancements.enable_subscription ? "Enabled" : "Disabled"
     );
     
     if (written < 0 || (size_t)written >= size - offset) {
         return offset;  // Buffer full or error
     }
     offset += (size_t)written;
     
     return offset;
 }
 
 /**
  * @brief Clean up string allocations in configuration
  */
 void polycall_protocol_cleanup_config_strings(
     polycall_core_context_t* core_ctx,
     protocol_config_t* config
 ) {
     if (!core_ctx || !config) {
         return;
     }
     
     // Free TLS string fields
     if (config->tls.cert_file) {
         polycall_core_free(core_ctx, (void*)config->tls.cert_file);
         config->tls.cert_file = NULL;
     }
     
     if (config->tls.key_file) {
         polycall_core_free(core_ctx, (void*)config->tls.key_file);
         config->tls.key_file = NULL;
     }
     
     if (config->tls.ca_file) {
         polycall_core_free(core_ctx, (void*)config->tls.ca_file);
         config->tls.ca_file = NULL;
     }
     
     if (config->tls.cipher_list) {
         polycall_core_free(core_ctx, (void*)config->tls.cipher_list);
         config->tls.cipher_list = NULL;
     }
     
     // Free crypto string fields
     if (config->crypto.encryption_algorithm) {
         polycall_core_free(core_ctx, (void*)config->crypto.encryption_algorithm);
         config->crypto.encryption_algorithm = NULL;
     }
     
     if (config->crypto.signing_algorithm) {
         polycall_core_free(core_ctx, (void*)config->crypto.signing_algorithm);
         config->crypto.signing_algorithm = NULL;
     }
     
     if (config->crypto.key_exchange_method) {
         polycall_core_free(core_ctx, (void*)config->crypto.key_exchange_method);
         config->crypto.key_exchange_method = NULL;
     }
 }
 
 /*---------------------------------------------------------------------------*/
 /* Internal configuration implementation functions */
 /*---------------------------------------------------------------------------*/
 
 /**
  * @brief Configure core protocol
  */
 static polycall_core_error_t config_core_protocol(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_core_config_t* core_config
 ) {
     if (!ctx || !proto_ctx || !core_config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Store configuration in protocol context
     proto_ctx->transport_type = core_config->transport_type;
     proto_ctx->encoding_format = core_config->encoding_format;
     proto_ctx->validation_level = core_config->validation_level;
     proto_ctx->default_timeout_ms = core_config->default_timeout_ms;
     proto_ctx->handshake_timeout_ms = core_config->handshake_timeout_ms;
     proto_ctx->keep_alive_interval_ms = core_config->keep_alive_interval_ms;
     proto_ctx->default_port = core_config->default_port;
     proto_ctx->enable_tls = core_config->enable_tls;
     proto_ctx->enable_compression = core_config->enable_compression;
     proto_ctx->enable_auto_reconnect = core_config->enable_auto_reconnect;
     proto_ctx->retry_policy = core_config->retry_policy;
     proto_ctx->max_retry_count = core_config->max_retry_count;
     proto_ctx->max_message_size = core_config->max_message_size;
     proto_ctx->strict_mode = core_config->strict_mode;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Configure TLS
  */
static polycall_core_error_t config_tls(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_tls_config_t* tls_config
) {
    if (!ctx || !proto_ctx || !tls_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Ensure TLS is enabled
    if (!proto_ctx->enable_tls) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_WARNING, 
                          "TLS configuration provided but TLS is not enabled");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }

    // Apply TLS configuration to the protocol context
    proto_ctx->tls.verify_peer = tls_config->verify_peer;
    proto_ctx->tls.allow_self_signed = tls_config->allow_self_signed;
    proto_ctx->tls.min_tls_version = tls_config->min_tls_version;
    
    // Copy certificate file path
    if (tls_config->cert_file) {
        if (proto_ctx->tls.cert_file) {
            polycall_core_free(ctx, (void*)proto_ctx->tls.cert_file);
        }
        proto_ctx->tls.cert_file = duplicate_string(ctx, tls_config->cert_file);
        if (!proto_ctx->tls.cert_file) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Copy key file path
    if (tls_config->key_file) {
        if (proto_ctx->tls.key_file) {
            polycall_core_free(ctx, (void*)proto_ctx->tls.key_file);
        }
        proto_ctx->tls.key_file = duplicate_string(ctx, tls_config->key_file);
        if (!proto_ctx->tls.key_file) {
            if (proto_ctx->tls.cert_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.cert_file);
                proto_ctx->tls.cert_file = NULL;
            }
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Copy CA file path
    if (tls_config->ca_file) {
        if (proto_ctx->tls.ca_file) {
            polycall_core_free(ctx, (void*)proto_ctx->tls.ca_file);
        }
        proto_ctx->tls.ca_file = duplicate_string(ctx, tls_config->ca_file);
        if (!proto_ctx->tls.ca_file) {
            if (proto_ctx->tls.cert_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.cert_file);
                proto_ctx->tls.cert_file = NULL;
            }
            if (proto_ctx->tls.key_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.key_file);
                proto_ctx->tls.key_file = NULL;
            }
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Copy cipher list
    if (tls_config->cipher_list) {
        if (proto_ctx->tls.cipher_list) {
            polycall_core_free(ctx, (void*)proto_ctx->tls.cipher_list);
        }
        proto_ctx->tls.cipher_list = duplicate_string(ctx, tls_config->cipher_list);
        if (!proto_ctx->tls.cipher_list) {
            if (proto_ctx->tls.cert_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.cert_file);
                proto_ctx->tls.cert_file = NULL;
            }
            if (proto_ctx->tls.key_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.key_file);
                proto_ctx->tls.key_file = NULL;
            }
            if (proto_ctx->tls.ca_file) {
                polycall_core_free(ctx, (void*)proto_ctx->tls.ca_file);
                proto_ctx->tls.ca_file = NULL;
            }
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}


/**
 * @brief Configure serialization
 */
static polycall_core_error_t config_serialization(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_serialization_config_t* serialization_config
) {
    if (!ctx || !proto_ctx || !serialization_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Apply serialization configuration to the protocol context
    proto_ctx->serialization.enable_schema_validation = serialization_config->enable_schema_validation;
    proto_ctx->serialization.enable_field_caching = serialization_config->enable_field_caching;
    proto_ctx->serialization.enable_serialization_optimization = serialization_config->enable_serialization_optimization;
    proto_ctx->serialization.enable_null_suppression = serialization_config->enable_null_suppression;
    proto_ctx->serialization.enable_binary_format = serialization_config->enable_binary_format;
    proto_ctx->serialization.max_depth = serialization_config->max_depth;
    
    // Initialize serialization components based on format
    switch (proto_ctx->encoding_format) {
        case PROTOCOL_ENCODING_JSON:
            // Initialize JSON serializer
            proto_ctx->serialization.use_compact_json = !serialization_config->enable_binary_format;
            break;
            
        case PROTOCOL_ENCODING_MSGPACK:
            // MessagePack is already binary, so binary_format doesn't apply
            break;
            
        case PROTOCOL_ENCODING_PROTOBUF:
            // For Protocol Buffers, we might need to load schemas
            if (serialization_config->enable_schema_validation) {
                // TODO: Load and validate Protocol Buffer schemas
            }
            break;
            
        case PROTOCOL_ENCODING_BINARY:
            // Custom binary format setup
            break;
            
        default:
            // Unknown or custom format - can't configure automatically
            break;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Configure state machine
 */
static polycall_core_error_t config_state_machine(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_state_machine_config_t* state_machine_config
) {
    if (!ctx || !proto_ctx || !state_machine_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if state machine exists
    if (!proto_ctx->state_machine) {
        // Create state machine if it doesn't exist
        polycall_sm_status_t sm_status = polycall_sm_create(ctx, &proto_ctx->state_machine);
        if (sm_status != POLYCALL_SM_SUCCESS) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                             POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                             POLYCALL_ERROR_SEVERITY_ERROR, 
                             "Failed to create state machine");
            return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
        }
    }
    
    // Apply configuration to state machine
    proto_ctx->state_machine_config.enable_state_logging = state_machine_config->enable_state_logging;
    proto_ctx->state_machine_config.enable_state_metrics = state_machine_config->enable_state_metrics;
    proto_ctx->state_machine_config.strict_transitions = state_machine_config->strict_transitions;
    proto_ctx->state_machine_config.state_timeout_ms = state_machine_config->state_timeout_ms;
    proto_ctx->state_machine_config.enable_recovery_transitions = state_machine_config->enable_recovery_transitions;
    
    // Define protocol states
    polycall_sm_add_state(proto_ctx->state_machine, "init", NULL, NULL, false);
    polycall_sm_add_state(proto_ctx->state_machine, "handshake", NULL, NULL, false);
    polycall_sm_add_state(proto_ctx->state_machine, "auth", NULL, NULL, false);
    polycall_sm_add_state(proto_ctx->state_machine, "ready", NULL, NULL, false);
    polycall_sm_add_state(proto_ctx->state_machine, "error", NULL, NULL, false);
    polycall_sm_add_state(proto_ctx->state_machine, "closed", NULL, NULL, false);
    
    // Define transitions
    polycall_sm_add_transition(proto_ctx->state_machine, "to_handshake", "init", "handshake", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "to_auth", "handshake", "auth", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "to_ready", "auth", "ready", NULL, NULL);
    
    // Error transitions from any state
    polycall_sm_add_transition(proto_ctx->state_machine, "init_to_error", "init", "error", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "handshake_to_error", "handshake", "error", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "auth_to_error", "auth", "error", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "ready_to_error", "ready", "error", NULL, NULL);
    
    // Recovery transition if enabled
    if (state_machine_config->enable_recovery_transitions) {
        polycall_sm_add_transition(proto_ctx->state_machine, "error_to_ready", "error", "ready", NULL, NULL);
    }
    
    // Close transitions
    polycall_sm_add_transition(proto_ctx->state_machine, "init_to_closed", "init", "closed", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "handshake_to_closed", "handshake", "closed", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "auth_to_closed", "auth", "closed", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "ready_to_closed", "ready", "closed", NULL, NULL);
    polycall_sm_add_transition(proto_ctx->state_machine, "error_to_closed", "error", "closed", NULL, NULL);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Configure command handling
 */
static polycall_core_error_t config_command(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_command_config_t* command_config
) {
    if (!ctx || !proto_ctx || !command_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Apply command configuration to protocol context
    proto_ctx->command.enable_queuing = command_config->enable_command_queuing;
    proto_ctx->command.queue_size = command_config->command_queue_size;
    proto_ctx->command.timeout_ms = command_config->command_timeout_ms;
    proto_ctx->command.enable_prioritization = command_config->enable_command_prioritization;
    proto_ctx->command.enable_retry = command_config->enable_command_retry;
    proto_ctx->command.max_concurrent = command_config->max_concurrent_commands;
    
    // Initialize command registry if needed
    if (!proto_ctx->command_registry) {
        polycall_command_config_t cmd_config = {
            .flags = proto_ctx->command.enable_prioritization ? POLYCALL_COMMAND_FLAG_PRIORITY : 0,
            .memory_pool_size = 0,  // Use default
            .initial_command_capacity = proto_ctx->command.queue_size,
            .user_data = NULL
        };
        
        polycall_core_error_t result = polycall_command_init(
            ctx,
            proto_ctx,
            &proto_ctx->command_registry,
            &cmd_config
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                              result,
                              POLYCALL_ERROR_SEVERITY_ERROR, 
                              "Failed to initialize command registry");
            return result;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Configure handshake
 */
static polycall_core_error_t config_handshake(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_handshake_config_t* handshake_config
) {
    if (!ctx || !proto_ctx || !handshake_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Apply handshake configuration to protocol context
    proto_ctx->handshake.enable_version_negotiation = handshake_config->enable_version_negotiation;
    proto_ctx->handshake.enable_capability_negotiation = handshake_config->enable_capability_negotiation;
    proto_ctx->handshake.enable_authentication = handshake_config->enable_authentication;
    proto_ctx->handshake.enable_identity_verification = handshake_config->enable_identity_verification;
    proto_ctx->handshake.retry_count = handshake_config->handshake_retry_count;
    proto_ctx->handshake.retry_interval_ms = handshake_config->handshake_retry_interval_ms;
    
    // Register built-in handshake handlers if needed
    if (!proto_ctx->handshake.handlers_registered) {
        // These would register callbacks for handling the handshake process
        // We're just stubbing them out for now
        proto_ctx->handshake.handlers_registered = true;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Configure crypto
 */
static polycall_core_error_t config_crypto(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const protocol_crypto_config_t* crypto_config
) {
    if (!ctx || !proto_ctx || !crypto_config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Apply crypto configuration to protocol context
    proto_ctx->crypto.enable_encryption = crypto_config->enable_encryption;
    proto_ctx->crypto.enable_signing = crypto_config->enable_signing;
    proto_ctx->crypto.key_rotation_interval_ms = crypto_config->key_rotation_interval_ms;
    proto_ctx->crypto.enable_perfect_forward_secrecy = crypto_config->enable_perfect_forward_secrecy;
    
    // Copy encryption algorithm
    if (crypto_config->encryption_algorithm) {
        if (proto_ctx->crypto.encryption_algorithm) {
            polycall_core_free(ctx, (void*)proto_ctx->crypto.encryption_algorithm);
        }
        proto_ctx->crypto.encryption_algorithm = duplicate_string(ctx, crypto_config->encryption_algorithm);
        if (!proto_ctx->crypto.encryption_algorithm) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Copy signing algorithm
    if (crypto_config->signing_algorithm) {
        if (proto_ctx->crypto.signing_algorithm) {
            polycall_core_free(ctx, (void*)proto_ctx->crypto.signing_algorithm);
        }
        proto_ctx->crypto.signing_algorithm = duplicate_string(ctx, crypto_config->signing_algorithm);
        if (!proto_ctx->crypto.signing_algorithm) {
            if (proto_ctx->crypto.encryption_algorithm) {
                polycall_core_free(ctx, (void*)proto_ctx->crypto.encryption_algorithm);
                proto_ctx->crypto.encryption_algorithm = NULL;
            }
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Copy key exchange method
    if (crypto_config->key_exchange_method) {
        if (proto_ctx->crypto.key_exchange_method) {
            polycall_core_free(ctx, (void*)proto_ctx->crypto.key_exchange_method);
        }
        proto_ctx->crypto.key_exchange_method = duplicate_string(ctx, crypto_config->key_exchange_method);
        if (!proto_ctx->crypto.key_exchange_method) {
            if (proto_ctx->crypto.encryption_algorithm) {
                polycall_core_free(ctx, (void*)proto_ctx->crypto.encryption_algorithm);
                proto_ctx->crypto.encryption_algorithm = NULL;
            }
            if (proto_ctx->crypto.signing_algorithm) {
                polycall_core_free(ctx, (void*)proto_ctx->crypto.signing_algorithm);
                proto_ctx->crypto.signing_algorithm = NULL;
            }
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Initialize crypto context if needed
    if ((crypto_config->enable_encryption || crypto_config->enable_signing) && !proto_ctx->crypto_context) {
        // TODO: Initialize crypto context once implemented
        // For now, just mark it as uninitialized
        proto_ctx->crypto_context = NULL;
    }
    
    return POLYCALL_CORE_SUCCESS;
}