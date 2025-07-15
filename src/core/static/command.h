/**
#include <stdio.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <string.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

 * @file command.h
 * @brief Protocol command handling for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the command processing API for the LibPolyCall protocol,
 * enabling secure, validated command execution between endpoints within the
 * Program-First architecture.
 */

#ifndef POLYCALL_PROTOCOL_COMMAND_H_H
#define POLYCALL_PROTOCOL_COMMAND_H_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/protocol/message.h"
#include "polycall/core/protocol/polycall_protocol_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

   /**
    * @brief Command registry (opaque)
    */
   typedef struct polycall_command_registry polycall_command_registry_t;

   /**
    * @brief Command parameter types
    */
   typedef enum {
     POLYCALL_PARAM_TYPE_ANY = 0,     /**< Any type (used for queries) */
     POLYCALL_PARAM_TYPE_INT32,       /**< 32-bit integer */
     POLYCALL_PARAM_TYPE_INT64,       /**< 64-bit integer */
     POLYCALL_PARAM_TYPE_FLOAT,       /**< Single-precision float */
     POLYCALL_PARAM_TYPE_DOUBLE,      /**< Double-precision float */
     POLYCALL_PARAM_TYPE_BOOL,        /**< Boolean */
     POLYCALL_PARAM_TYPE_STRING,      /**< Null-terminated string */
     POLYCALL_PARAM_TYPE_BINARY,      /**< Binary data */
     POLYCALL_PARAM_TYPE_USER = 0x100 /**< Start of user-defined types */
   } polycall_parameter_type_t;

   /**
    * @brief Command flags
    */
   typedef enum {
     POLYCALL_COMMAND_FLAG_NONE = 0,
     POLYCALL_COMMAND_FLAG_SECURE = (1 << 0), /**< Requires secure connection */
     POLYCALL_COMMAND_FLAG_ADMIN = (1 << 1),  /**< Requires admin privileges */
     POLYCALL_COMMAND_FLAG_ALLOW_ANY_STATE =
         (1 << 2), /**< Allow execution in any protocol state */
     POLYCALL_COMMAND_FLAG_HANDSHAKE_COMMAND =
         (1 << 3), /**< Command for handshake only */
     POLYCALL_COMMAND_FLAG_AUTH_COMMAND =
         (1 << 4), /**< Command for authentication only */
     POLYCALL_COMMAND_FLAG_RESTRICTED =
         (1 << 5), /**< Command with restricted access */
     POLYCALL_COMMAND_FLAG_STREAMING =
         (1 << 6),                          /**< Command with streaming data */
     POLYCALL_COMMAND_FLAG_USER = (1 << 16) /**< Start of user-defined flags */
   } polycall_command_flags_t;

   /**
    * @brief Command status codes
    */
   typedef enum {
     POLYCALL_COMMAND_STATUS_SUCCESS = 0,
     POLYCALL_COMMAND_STATUS_ERROR
   } polycall_command_status_t;

   /**
    * @brief Command error codes
    */
   typedef enum {
     POLYCALL_COMMAND_ERROR_NONE = 0,
     POLYCALL_COMMAND_ERROR_INVALID_COMMAND,    /**< Command not found */
     POLYCALL_COMMAND_ERROR_INVALID_PARAMETERS, /**< Invalid parameters */
     POLYCALL_COMMAND_ERROR_INVALID_STATE,      /**< Invalid protocol state */
     POLYCALL_COMMAND_ERROR_PERMISSION_DENIED,  /**< Permission denied */
     POLYCALL_COMMAND_ERROR_EXECUTION_FAILED,   /**< Command execution failed */
     POLYCALL_COMMAND_ERROR_TIMEOUT,            /**< Command timed out */
     POLYCALL_COMMAND_ERROR_NOT_IMPLEMENTED,    /**< Command not implemented */
     POLYCALL_COMMAND_ERROR_RESOURCE_UNAVAILABLE, /**< Resource unavailable */
     POLYCALL_COMMAND_ERROR_INTERNAL,             /**< Internal error */
     POLYCALL_COMMAND_ERROR_USER = 0x1000 /**< Start of user-defined errors */
   } polycall_command_error_t;

   // Forward declaration
   struct polycall_command_message;
   struct polycall_command_response;

   /**
    * @brief Command validation result
    */
   typedef struct {
     polycall_command_status_t status;
     uint32_t error_code;
     char error_message[256];
   } polycall_command_validation_t;

   /**
    * @brief Command handler function type
    */
   typedef struct polycall_command_response *(*polycall_command_handler_fn)(
       polycall_core_context_t * ctx, polycall_protocol_context_t * proto_ctx,
       const struct polycall_command_message *message, void *user_data);

   /**
    * @brief Command validator function type
    */
   typedef polycall_command_validation_t (*polycall_command_validator_fn)(
       polycall_core_context_t * ctx, polycall_protocol_context_t * proto_ctx,
       const struct polycall_command_message *message, void *user_data);

   /**
    * @brief Command configuration
    */
   typedef struct {
     uint32_t flags;
     uint32_t initial_command_capacity;
     size_t memory_pool_size;
     void *user_data;
   } polycall_command_config_t;

   /**
    * @brief Command information
    */
   typedef struct {
     uint32_t command_id;
     char name[64];
     polycall_command_handler_fn handler;
     polycall_command_validator_fn validator;
     uint32_t permissions;
     uint32_t flags;
     void *user_data;
   } polycall_command_info_t;

   /**
    * @brief Command entry definition
    */
   typedef struct {
     uint32_t command_id;
     char name[64];
     polycall_command_handler_fn handler;
     polycall_command_validator_fn validator;
     uint32_t permissions;
     uint32_t flags;
     void *user_data;
   } polycall_command_entry_t;

   /**
    * @brief Command parameter
    */
   typedef struct {
     uint16_t param_id;
     polycall_parameter_type_t type;
     void *data;
     uint32_t data_size;
     uint16_t flags;
   } polycall_command_parameter_t;

   /**
    * @brief Command message
    */
   typedef struct polycall_command_message {
     struct {
       uint8_t version;
       uint32_t command_id;
       uint32_t flags;
       uint32_t param_count;
     } header;
     polycall_command_parameter_t *parameters;
     uint32_t capacity;
     uint32_t sequence_number;
   } polycall_command_message_t;

   /**
    * @brief Command response
    */
   typedef struct polycall_command_response {
     polycall_command_status_t status;
     void *response_data;
     uint32_t data_size;
     uint32_t error_code;
     char error_message[256];
   } polycall_command_response_t;

   /**
    * @brief Initialize command registry
    *
    * @param ctx Core context
    * @param proto_ctx Protocol context
    * @param registry Pointer to receive registry
    * @param config Command configuration
    * @return Error code
    */
   polycall_core_error_t polycall_command_init(
       polycall_core_context_t * ctx, polycall_protocol_context_t * proto_ctx,
       polycall_command_registry_t * *registry,
       const polycall_command_config_t *config);

   /**
    * @brief Clean up command registry
    *
    * @param ctx Core context
    * @param registry Registry to clean up
    */
   void polycall_command_cleanup(polycall_core_context_t * ctx,
                                 polycall_command_registry_t * registry);

   /**
    * @brief Register a command
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_info Command information
    * @param command_id Pointer to receive command ID (optional)
    * @return Error code
    */
   polycall_core_error_t polycall_command_register(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       const polycall_command_info_t *command_info, uint32_t *command_id);

   /**
    * @brief Unregister a command
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID to unregister
    * @return Error code
    */
   polycall_core_error_t polycall_command_unregister(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id);

   /**
    * @brief Find command by ID
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID to find
    * @param command_info Pointer to receive command information
    * @return Error code
    */
   polycall_core_error_t polycall_command_find_by_id(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id, polycall_command_info_t * command_info);

   /**
    * @brief Find command by name
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param name Command name to find
    * @param command_info Pointer to receive command information
    * @return Error code
    */
   polycall_core_error_t polycall_command_find_by_name(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       const char *name, polycall_command_info_t *command_info);

   /**
    * @brief Create a command message
    *
    * @param ctx Core context
    * @param message Pointer to receive message
    * @param command_id Command ID
    * @return Error code
    */
   polycall_core_error_t polycall_command_create_message(
       polycall_core_context_t * ctx, polycall_command_message_t * *message,
       uint32_t command_id);

   /**
    * @brief Destroy a command message
    *
    * @param ctx Core context
    * @param message Message to destroy
    * @return Error code
    */
   polycall_core_error_t polycall_command_destroy_message(
       polycall_core_context_t * ctx, polycall_command_message_t * message);

   /**
    * @brief Add a parameter to a command message
    *
    * @param ctx Core context
    * @param message Command message
    * @param param_id Parameter ID
    * @param type Parameter type
    * @param data Parameter data
    * @param data_size Data size in bytes
    * @param flags Parameter flags
    * @return Error code
    */
   polycall_core_error_t polycall_command_add_parameter(
       polycall_core_context_t * ctx, polycall_command_message_t * message,
       uint16_t param_id, polycall_parameter_type_t type, const void *data,
       uint32_t data_size, uint16_t flags);

   /**
    * @brief Get a parameter from a command message
    *
    * @param ctx Core context
    * @param message Command message
    * @param param_id Parameter ID
    * @param type Parameter type
    * @param data Pointer to receive parameter data
    * @param data_size Pointer to receive data size in bytes
    * @return Error code
    */
   polycall_core_error_t polycall_command_get_parameter(
       polycall_core_context_t * ctx, const polycall_command_message_t *message,
       uint16_t param_id, polycall_parameter_type_t type, void **data,
       uint32_t *data_size);

   /**
    * @brief Create a command response
    *
    * @param ctx Core context
    * @param status Status code
    * @param response_data Response data
    * @param data_size Data size in bytes
    * @return Command response
    */
   polycall_command_response_t *polycall_command_create_response(
       polycall_core_context_t * ctx, polycall_command_status_t status,
       const void *response_data, uint32_t data_size);

   /**
    * @brief Create an error response
    *
    * @param ctx Core context
    * @param error_code Error code
    * @param error_message Error message
    * @return Command response
    */
   polycall_command_response_t *polycall_command_create_error_response(
       polycall_core_context_t * ctx, uint32_t error_code,
       const char *error_message);

   /**
    * @brief Destroy a command response
    *
    * @param ctx Core context
    * @param response Response to destroy
    */
   void polycall_command_destroy_response(
       polycall_core_context_t * ctx, polycall_command_response_t * response);

   /**
    * @brief Process a command message
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param proto_ctx Protocol context
    * @param message Command message
    * @param response Pointer to receive response
    * @return Error code
    */
   polycall_core_error_t polycall_command_process(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       polycall_protocol_context_t * proto_ctx,
       const polycall_command_message_t *message,
       polycall_command_response_t **response);

   /**
    * @brief Validate a command message
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param proto_ctx Protocol context
    * @param message Command message
    * @param validation Pointer to receive validation result
    * @return Error code
    */
   polycall_core_error_t polycall_command_validate(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       polycall_protocol_context_t * proto_ctx,
       const polycall_command_message_t *message,
       polycall_command_validation_t *validation);

   /**
    * @brief Serialize a command message
    *
    * @param ctx Core context
    * @param message Command message
    * @param buffer Pointer to receive serialized data
    * @param buffer_size Pointer to receive buffer size
    * @return Error code
    */
   polycall_core_error_t polycall_command_serialize_message(
       polycall_core_context_t * ctx, const polycall_command_message_t *message,
       void **buffer, size_t *buffer_size);

   /**
    * @brief Deserialize a command message
    *
    * @param ctx Core context
    * @param buffer Serialized data
    * @param buffer_size Buffer size
    * @param message Pointer to receive deserialized message
    * @return Error code
    */
   polycall_core_error_t polycall_command_deserialize_message(
       polycall_core_context_t * ctx, const void *buffer, size_t buffer_size,
       polycall_command_message_t **message);

   /**
    * @brief Serialize a command response
    *
    * @param ctx Core context
    * @param response Command response
    * @param buffer Pointer to receive serialized data
    * @param buffer_size Pointer to receive buffer size
    * @return Error code
    */
   polycall_core_error_t polycall_command_serialize_response(
       polycall_core_context_t * ctx,
       const polycall_command_response_t *response, void **buffer,
       size_t *buffer_size);

   /**
    * @brief Deserialize a command response
    *
    * @param ctx Core context
    * @param buffer Serialized data
    * @param buffer_size Buffer size
    * @param response Pointer to receive deserialized response
    * @return Error code
    */
   polycall_core_error_t polycall_command_deserialize_response(
       polycall_core_context_t * ctx, const void *buffer, size_t buffer_size,
       polycall_command_response_t **response);

   /**
    * @brief Set permissions for a command
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID
    * @param permissions Permission mask
    * @return Error code
    */
   polycall_core_error_t polycall_command_set_permissions(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id, uint32_t permissions);

   /**
    * @brief Get permissions for a command
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID
    * @param permissions Pointer to receive permission mask
    * @return Error code
    */
   polycall_core_error_t polycall_command_get_permissions(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id, uint32_t * permissions);

   /**
    * @brief Check if a command exists
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID
    * @return true if command exists, false otherwise
    */
   bool polycall_command_exists(polycall_core_context_t * ctx,
                                polycall_command_registry_t * registry,
                                uint32_t command_id);

   /**
    * @brief Get total number of registered commands
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param count Pointer to receive command count
    * @return Error code
    */
   polycall_core_error_t polycall_command_get_count(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t * count);

   /**
    * @brief Generate a default validator
    *
    * @param ctx Core context
    * @param required_permissions Required permissions
    * @param required_states Required protocol states
    * @param state_count State count
    * @param validator Pointer to receive validator function
    * @return Error code
    */
   polycall_core_error_t polycall_command_generate_validator(
       polycall_core_context_t * ctx, uint32_t required_permissions,
       const polycall_protocol_state_t *required_states, size_t state_count,
       polycall_command_validator_fn *validator);

   /**
    * @brief Set command flags
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID
    * @param flags Command flags
    * @return Error code
    */
   polycall_core_error_t polycall_command_set_flags(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id, uint32_t flags);

   /**
    * @brief Get command flags
    *
    * @param ctx Core context
    * @param registry Command registry
    * @param command_id Command ID
    * @param flags Pointer to receive command flags
    * @return Error code
    */
   polycall_core_error_t polycall_command_get_flags(
       polycall_core_context_t * ctx, polycall_command_registry_t * registry,
       uint32_t command_id, uint32_t * flags);

#ifdef __cplusplus
 }
#endif

#endif /* POLYCALL_PROTOCOL_COMMAND_H_H */