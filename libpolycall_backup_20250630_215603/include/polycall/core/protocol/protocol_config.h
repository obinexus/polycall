/**
 * @file protocol_config.h
 * @brief Comprehensive Protocol Configuration for LibPolyCall
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the comprehensive configuration interface for the protocol
 * module and all its enhancements, providing a unified initialization and
 * management API.
 */

 #ifndef POLYCALL_PROTOCOL_PROTOCOL_CONFIG_H_H
 #define POLYCALL_PROTOCOL_PROTOCOL_CONFIG_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/protocol/enhancements/protocol_enhacements_config.h"
 #include <stddef.h>
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Protocol transport types
  */
 typedef enum {
     PROTOCOL_TRANSPORT_NONE = 0,          /**< No transport */
     PROTOCOL_TRANSPORT_TCP,               /**< TCP transport */
     PROTOCOL_TRANSPORT_WEBSOCKET,         /**< WebSocket transport */
     PROTOCOL_TRANSPORT_UNIX_SOCKET,       /**< Unix domain socket transport */
     PROTOCOL_TRANSPORT_NAMED_PIPE,        /**< Named pipe transport */
     PROTOCOL_TRANSPORT_SHARED_MEMORY,     /**< Shared memory transport */
     PROTOCOL_TRANSPORT_CUSTOM             /**< Custom transport */
 } protocol_transport_type_t;
 
 /**
  * @brief Protocol encoding formats
  */
 typedef enum {
     PROTOCOL_ENCODING_NONE = 0,           /**< No encoding */
     PROTOCOL_ENCODING_JSON,               /**< JSON encoding */
     PROTOCOL_ENCODING_MSGPACK,            /**< MessagePack encoding */
     PROTOCOL_ENCODING_PROTOBUF,           /**< Protocol Buffers encoding */
     PROTOCOL_ENCODING_BINARY,             /**< Custom binary encoding */
     PROTOCOL_ENCODING_CUSTOM              /**< Custom encoding */
 } protocol_encoding_format_t;
 
 /**
  * @brief Protocol message validation level
  */
 typedef enum {
     PROTOCOL_VALIDATION_NONE = 0,         /**< No validation */
     PROTOCOL_VALIDATION_BASIC,            /**< Basic validation */
     PROTOCOL_VALIDATION_STANDARD,         /**< Standard validation */
     PROTOCOL_VALIDATION_STRICT            /**< Strict validation */
 } protocol_validation_level_t;
 
 /**
  * @brief Protocol message retry policy
  */
 typedef enum {
     PROTOCOL_RETRY_NONE = 0,              /**< No retry */
     PROTOCOL_RETRY_FIXED,                 /**< Fixed interval retry */
     PROTOCOL_RETRY_LINEAR,                /**< Linear backoff retry */
     PROTOCOL_RETRY_EXPONENTIAL            /**< Exponential backoff retry */
 } protocol_retry_policy_t;
 
 /**
  * @brief Core protocol configuration
  */
 typedef struct {
     protocol_transport_type_t transport_type;    /**< Transport type */
     protocol_encoding_format_t encoding_format;  /**< Encoding format */
     protocol_validation_level_t validation_level; /**< Validation level */
     uint32_t default_timeout_ms;                 /**< Default operation timeout */
     uint32_t handshake_timeout_ms;               /**< Handshake timeout */
     uint32_t keep_alive_interval_ms;             /**< Keep alive interval */
     uint16_t default_port;                       /**< Default port number */
     bool enable_tls;                             /**< Enable TLS encryption */
     bool enable_compression;                     /**< Enable compression */
     bool enable_auto_reconnect;                  /**< Enable auto reconnection */
     protocol_retry_policy_t retry_policy;        /**< Retry policy */
     uint32_t max_retry_count;                    /**< Maximum retry count */
     uint32_t max_message_size;                   /**< Maximum message size */
     bool strict_mode;                            /**< Enable strict mode */
 } protocol_core_config_t;
 
 /**
  * @brief TLS security configuration
  */
 typedef struct {
     const char* cert_file;                      /**< Certificate file path */
     const char* key_file;                       /**< Private key file path */
     const char* ca_file;                        /**< CA certificate file path */
     bool verify_peer;                           /**< Verify peer certificate */
     bool allow_self_signed;                     /**< Allow self-signed certificates */
     const char* cipher_list;                    /**< Cipher list */
     uint32_t min_tls_version;                   /**< Minimum TLS version */
 } protocol_tls_config_t;
 
 /**
  * @brief Message serialization configuration
  */
 typedef struct {
     bool enable_schema_validation;              /**< Enable schema validation */
     bool enable_field_caching;                  /**< Enable field name caching */
     bool enable_serialization_optimization;     /**< Enable serialization optimization */
     bool enable_null_suppression;               /**< Enable null field suppression */
     bool enable_binary_format;                  /**< Enable binary format for efficiency */
     uint32_t max_depth;                         /**< Maximum nesting depth */
 } protocol_serialization_config_t;
 
 /**
  * @brief State machine configuration
  */
 typedef struct {
     bool enable_state_logging;                  /**< Enable state transition logging */
     bool enable_state_metrics;                  /**< Enable state metrics collection */
     bool strict_transitions;                    /**< Enforce strict state transitions */
     uint32_t state_timeout_ms;                  /**< State timeout for prevention of stuck states */
     bool enable_recovery_transitions;           /**< Enable recovery transitions */
 } protocol_state_machine_config_t;
 
 /**
  * @brief Command handling configuration
  */
 typedef struct {
     bool enable_command_queuing;                /**< Enable command queuing */
     uint32_t command_queue_size;                /**< Command queue size */
     uint32_t command_timeout_ms;                /**< Command execution timeout */
     bool enable_command_prioritization;         /**< Enable command prioritization */
     bool enable_command_retry;                  /**< Enable command retry */
     uint32_t max_concurrent_commands;           /**< Maximum concurrent commands */
 } protocol_command_config_t;
 
 /**
  * @brief Handshake configuration
  */
 typedef struct {
     bool enable_version_negotiation;            /**< Enable protocol version negotiation */
     bool enable_capability_negotiation;         /**< Enable capability negotiation */
     bool enable_authentication;                 /**< Enable authentication during handshake */
     bool enable_identity_verification;          /**< Enable identity verification */
     uint32_t handshake_retry_count;             /**< Handshake retry count */
     uint32_t handshake_retry_interval_ms;       /**< Handshake retry interval */
 } protocol_handshake_config_t;
 
 /**
  * @brief Crypto configuration
  */
 typedef struct {
     bool enable_encryption;                     /**< Enable message encryption */
     bool enable_signing;                        /**< Enable message signing */
     const char* encryption_algorithm;           /**< Encryption algorithm */
     const char* signing_algorithm;              /**< Signing algorithm */
     const char* key_exchange_method;            /**< Key exchange method */
     uint32_t key_rotation_interval_ms;          /**< Key rotation interval */
     bool enable_perfect_forward_secrecy;        /**< Enable perfect forward secrecy */
 } protocol_crypto_config_t;
 
 /**
  * @brief Comprehensive protocol configuration
  */
 typedef struct {
     protocol_core_config_t core;                 /**< Core configuration */
     protocol_tls_config_t tls;                   /**< TLS configuration */
     protocol_serialization_config_t serialization; /**< Serialization configuration */
     protocol_state_machine_config_t state_machine; /**< State machine configuration */
     protocol_command_config_t command;            /**< Command handling configuration */
     protocol_handshake_config_t handshake;        /**< Handshake configuration */
     protocol_crypto_config_t crypto;              /**< Crypto configuration */
     polycall_protocol_enhancements_config_t enhancements; /**< Enhancements configuration */
 } protocol_config_t;
 
 /**
  * @brief Initialize protocol with configuration
  *
  * @param core_ctx Core context
  * @param proto_ctx Protocol context to initialize
  * @param config Protocol configuration
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_config_init(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_config_t* config
 );
 
 /**
  * @brief Apply configuration to protocol context
  *
  * This function applies the configuration to an already initialized protocol context.
  *
  * @param core_ctx Core context
  * @param proto_ctx Protocol context
  * @param config Protocol configuration
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_apply_config(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     const protocol_config_t* config
 );
 
 /**
  * @brief Create default protocol configuration
  *
  * @return Default configuration
  */
 protocol_config_t polycall_protocol_default_config(void);
 
 /**
  * @brief Load protocol configuration from file
  *
  * @param core_ctx Core context
  * @param config_file Configuration file path
  * @param config Pointer to receive configuration
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_load_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     protocol_config_t* config
 );
 
 /**
  * @brief Save protocol configuration to file
  *
  * @param core_ctx Core context
  * @param config_file Configuration file path
  * @param config Configuration to save
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_save_config(
     polycall_core_context_t* core_ctx,
     const char* config_file,
     const protocol_config_t* config
 );
 
 /**
  * @brief Validate protocol configuration
  *
  * @param core_ctx Core context
  * @param config Configuration to validate
  * @param error_message Buffer to receive error message
  * @param message_size Size of error message buffer
  * @return true if valid, false otherwise
  */
 bool polycall_protocol_validate_config(
     polycall_core_context_t* core_ctx,
     const protocol_config_t* config,
     char* error_message,
     size_t message_size
 );
 
 /**
  * @brief Merge protocol configurations
  *
  * This function merges a source configuration into a destination configuration,
  * overriding only non-default values.
  *
  * @param core_ctx Core context
  * @param dest Destination configuration
  * @param src Source configuration
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_merge_config(
     polycall_core_context_t* core_ctx,
     protocol_config_t* dest,
     const protocol_config_t* src
 );
 
 /**
  * @brief Copy protocol configuration
  *
  * @param core_ctx Core context
  * @param dest Destination configuration
  * @param src Source configuration
  * @return Error code
  */
 polycall_core_error_t polycall_protocol_copy_config(
     polycall_core_context_t* core_ctx,
     protocol_config_t* dest,
     const protocol_config_t* src
 );
 
 /**
  * @brief Get protocol configuration schema
  *
  * This function returns a schema definition for the protocol configuration
  * which can be used for validation and code generation.
  *
  * @param core_ctx Core context
  * @return Configuration schema (opaque pointer)
  */
 void* polycall_protocol_get_config_schema(
     polycall_core_context_t* core_ctx
 );
 
 /**
  * @brief Print protocol configuration
  *
  * This function prints the protocol configuration to a string buffer.
  *
  * @param core_ctx Core context
  * @param config Configuration to print
  * @param buffer Buffer to receive configuration string
  * @param size Size of buffer
  * @return Number of bytes written (excluding null terminator)
  */
 size_t polycall_protocol_print_config(
     polycall_core_context_t* core_ctx,
     const protocol_config_t* config,
     char* buffer,
     size_t size
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_PROTOCOL_PROTOCOL_CONFIG_H_H */