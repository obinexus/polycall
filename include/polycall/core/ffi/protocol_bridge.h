/**
 * @file protocol_bridge.h
 * @brief Protocol bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the protocol bridge for LibPolyCall FFI, connecting
 * the FFI layer to the PolyCall Protocol system to enable network-transparent
 * function calls between different language runtimes.
 */

#ifndef POLYCALL_FFI_PROTOCOL_BRIDGE_H_H
#define POLYCALL_FFI_PROTOCOL_BRIDGE_H_H

#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/polycall/polycall_config.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_permission.h"
#include "polycall/core/polycall/polycall_types.h"
#include "polycall/core/protocol/message.h"
#include "polycall/core/protocol/polycall_protocol_context.h"

#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Protocol bridge (opaque)
 */
typedef struct protocol_bridge protocol_bridge_t;

/**
 * @brief Message converter (opaque)
 */
typedef struct message_converter message_converter_t;

/**
 * @brief Routing table (opaque)
 */
typedef struct routing_table routing_table_t;

/**
 * @brief Protocol bridge configuration
 */
typedef struct {
  bool enable_message_compression; /**< Enable message compression */
  bool enable_streaming;           /**< Enable streaming support */
  bool enable_fragmentation;       /**< Enable message fragmentation */
  size_t max_message_size;         /**< Maximum message size */
  uint32_t timeout_ms;             /**< Operation timeout in milliseconds */
  void *user_data;                 /**< User data */
} protocol_bridge_config_t;

/**
 * @brief Message conversion result
 */
typedef struct {
  bool success;            /**< Whether conversion succeeded */
  char error_message[256]; /**< Error message if not successful */
  void *result;            /**< Conversion result */
  size_t result_size;      /**< Result size */
} message_conversion_result_t;

/**
 * @brief Initialize protocol bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param proto_ctx Protocol context
 * @param bridge Pointer to receive protocol bridge
 * @param config Bridge configuration
 * @return Error code
 */
polycall_core_error_t polycall_protocol_bridge_init(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_protocol_context_t *proto_ctx, protocol_bridge_t **bridge,
    const protocol_bridge_config_t *config);

/**
 * @brief Clean up protocol bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge to clean up
 */
void polycall_protocol_bridge_cleanup(polycall_core_context_t *ctx,
                                      polycall_ffi_context_t *ffi_ctx,
                                      protocol_bridge_t *bridge);

/**
 * @brief Route protocol message to FFI function
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param message Protocol message
 * @param target_language Target language
 * @param function_name Function name
 * @return Error code
 */
polycall_core_error_t polycall_protocol_route_to_ffi(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, polycall_message_t *message,
    const char *target_language, const char *function_name);

/**
 * @brief Convert FFI result to protocol message
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param result FFI function result
 * @param message Pointer to receive protocol message
 * @return Error code
 */
polycall_core_error_t polycall_protocol_ffi_result_to_message(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, ffi_value_t *result,
    polycall_message_t **message);

/**
 * @brief Register FFI function for remote calls
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param function_name Function name
 * @param language Source language
 * @param signature Function signature
 * @return Error code
 */
polycall_core_error_t polycall_protocol_register_remote_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, const char *function_name, const char *language,
    ffi_signature_t *signature);

/**
 * @brief Call a remote FFI function
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param function_name Function name
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Pointer to receive function result
 * @param target_endpoint Target endpoint
 * @return Error code
 */
polycall_core_error_t polycall_protocol_call_remote_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, const char *function_name, ffi_value_t *args,
    size_t arg_count, ffi_value_t *result, const char *target_endpoint);

/**
 * @brief Register message converter
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param source_type Source message type
 * @param target_type Target message type
 * @param converter Converter function
 * @param user_data User data for converter
 * @return Error code
 */
polycall_core_error_t polycall_protocol_register_converter(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, uint32_t source_type, uint32_t target_type,
    message_conversion_result_t (*converter)(polycall_core_context_t *ctx,
                                             const void *source,
                                             size_t source_size,
                                             void *user_data),
    void *user_data);

/**
 * @brief Convert message between formats
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param source_type Source message type
 * @param source Source message data
 * @param source_size Source message size
 * @param target_type Target message type
 * @param result Pointer to receive conversion result
 * @return Error code
 */
polycall_core_error_t polycall_protocol_convert_message(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, uint32_t source_type, const void *source,
    size_t source_size, uint32_t target_type,
    message_conversion_result_t *result);

/**
 * @brief Add routing rule
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param source_pattern Source pattern
 * @param target_endpoint Target endpoint
 * @param priority Routing priority
 * @return Error code
 */
polycall_core_error_t polycall_protocol_add_routing_rule(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, const char *source_pattern,
    const char *target_endpoint, uint32_t priority);

/**
 * @brief Remove routing rule
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param source_pattern Source pattern
 * @param target_endpoint Target endpoint
 * @return Error code
 */
polycall_core_error_t polycall_protocol_remove_routing_rule(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, const char *source_pattern,
    const char *target_endpoint);

/**
 * @brief Synchronize state between protocol and FFI
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param proto_ctx Protocol context
 * @return Error code
 */
polycall_core_error_t polycall_protocol_sync_state(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, polycall_protocol_context_t *proto_ctx);

/**
 * @brief Handle protocol message
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param bridge Protocol bridge
 * @param message Protocol message
 * @param response Pointer to receive response message
 * @return Error code
 */
polycall_core_error_t polycall_protocol_handle_message(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    protocol_bridge_t *bridge, polycall_message_t *message,
    polycall_message_t **response);

// Message type definitions
#define POLYCALL_FFI_PROTOCOL_BRIDGE_H_H
#define POLYCALL_FFI_PROTOCOL_BRIDGE_H_H
#define POLYCALL_FFI_PROTOCOL_BRIDGE_H_H

// Maximum path length for routing
#define POLYCALL_FFI_PROTOCOL_BRIDGE_H_H

// Message converter structure
struct message_converter {
  uint32_t source_type;
  uint32_t target_type;
  message_conversion_result_t (*converter)(polycall_core_context_t *ctx,
                                           const void *source,
                                           size_t source_size, void *user_data);
  void *user_data;
  struct message_converter *next;
};

// Routing rule structure
typedef struct routing_rule {
  char source_pattern[MAX_PATH_LENGTH];
  char target_endpoint[MAX_PATH_LENGTH];
  uint32_t priority;
  struct routing_rule *next;
} routing_rule_t;

// Routing table structure
struct routing_table {
  routing_rule_t *rules;
  pthread_mutex_t mutex;
};

// Remote function registration
typedef struct remote_function {
  char name[MAX_PATH_LENGTH];
  char language[64];
  ffi_signature_t *signature;
  struct remote_function *next;
} remote_function_t;

// Protocol bridge structure
struct protocol_bridge {
  polycall_core_context_t *core_ctx;
  polycall_ffi_context_t *ffi_ctx;
  polycall_protocol_context_t *proto_ctx;
  message_converter_t *converters;
  routing_table_t *routing_table;
  remote_function_t *remote_functions;
  protocol_bridge_config_t config;
  pthread_mutex_t mutex;
};

// Forward declarations of internal functions
static polycall_core_error_t init_routing_table(polycall_core_context_t *ctx,
                                                routing_table_t **table);

static void cleanup_routing_table(polycall_core_context_t *ctx,
                                  routing_table_t *table);

static polycall_core_error_t
add_routing_rule_internal(polycall_core_context_t *ctx, routing_table_t *table,
                          const char *source_pattern,
                          const char *target_endpoint, uint32_t priority);

static polycall_core_error_t
remove_routing_rule_internal(polycall_core_context_t *ctx,
                             routing_table_t *table, const char *source_pattern,
                             const char *target_endpoint);

static message_converter_t *find_converter(protocol_bridge_t *bridge,
                                           uint32_t source_type,
                                           uint32_t target_type);

static polycall_core_error_t register_converter_internal(
    polycall_core_context_t *ctx, protocol_bridge_t *bridge,
    uint32_t source_type, uint32_t target_type,
    message_conversion_result_t (*converter)(polycall_core_context_t *ctx,
                                             const void *source,
                                             size_t source_size,
                                             void *user_data),
    void *user_data);

static polycall_core_error_t register_remote_function_internal(
    polycall_core_context_t *ctx, protocol_bridge_t *bridge,
    const char *function_name, const char *language,
    ffi_signature_t *signature);

static remote_function_t *find_remote_function(protocol_bridge_t *bridge,
                                               const char *function_name);

static polycall_core_error_t serialize_ffi_value(polycall_core_context_t *ctx,
                                                 ffi_value_t *value,
                                                 void **data,
                                                 size_t *data_size);

static polycall_core_error_t deserialize_ffi_value(polycall_core_context_t *ctx,
                                                   const void *data,
                                                   size_t data_size,
                                                   ffi_value_t *value);

static polycall_core_error_t route_message(polycall_core_context_t *ctx,
                                           protocol_bridge_t *bridge,
                                           polycall_message_t *message,
                                           const char **target_endpoint);

/**
 * @brief Create a default protocol bridge configuration
 *
 * @return Default configuration
 */
protocol_bridge_config_t polycall_protocol_bridge_create_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_PROTOCOL_BRIDGE_H_H */