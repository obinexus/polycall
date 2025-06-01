/**
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

 * @file polycall.h
 * @brief Main public API header for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This header defines the main public API for the LibPolyCall library,
 * as specified in polycall_public.h. It serves as the primary entry point
 * for applications using the library, implementing the Program-First design.
 */

#ifndef POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H

#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

// Forward declarations of opaque types
typedef struct polycall_context polycall_context_t;
typedef struct polycall_session polycall_session_t;
typedef struct polycall_message polycall_message_t;
typedef struct polycall_core_context polycall_core_context_t;

// Error type definition
typedef enum polycall_error {
	POLYCALL_OK = 0,
	POLYCALL_ERROR_INVALID_PARAMETERS,
	POLYCALL_ERROR_OUT_OF_MEMORY,
	POLYCALL_ERROR_INITIALIZATION_FAILED,
	POLYCALL_ERROR_CONNECTION_FAILED,
	POLYCALL_ERROR_NOT_CONNECTED,
	POLYCALL_ERROR_SEND_FAILED,
	POLYCALL_ERROR_RECEIVE_FAILED
} polycall_error_t;

// Message type definition
typedef enum polycall_message_type {
	POLYCALL_MESSAGE_REQUEST,
	POLYCALL_MESSAGE_RESPONSE,
	POLYCALL_MESSAGE_NOTIFICATION
} polycall_message_type_t;
#include "polycall/core/polycall/polycall_config.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_logger.h"
#include "polycall/core/polycall/polycall_version.h"



#ifdef __cplusplus
extern "C" {
#endif

/* PolyCall Library Configuration Flags */
#define POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H
#define POLYCALL_POLYCALL_POLYCALL_H_H

 
 // Internal context structure that extends the opaque public type
 struct polycall_context {
	polycall_core_context_t* core_ctx;
	polycall_error_t last_error;
	char error_message[256];
	void* user_data;
	
	// Extension for FFI integration
	struct {
		bool ffi_initialized;
		void* ffi_context;
	} ffi;
	
	// Extension for protocol integration
	struct {
		bool protocol_initialized;
		void* protocol_context;
	} protocol;
};

// Internal session structure
struct polycall_session {
	polycall_context_t* ctx;
	void* connection;
	char address[INET_ADDRSTRLEN];
	uint16_t port;
	bool connected;
	bool authenticated;
	uint32_t timeout_ms;
	uint32_t sequence_number;
	void* secure_context;
};

// Internal message structure
struct polycall_message {
	polycall_message_type_t type;
	char path[256];
	void* data;
	size_t data_size;
	void* json_data;
	uint32_t flags;
	uint32_t sequence;
	void* secure_envelope;
};
// Internal configuration structure

/**
 * @brief Initialize the LibPolyCall library
 * 
 * @param ctx Pointer to receive context
 * @param config Configuration (NULL for defaults)
 * @return Error code
 */
polycall_error_t polycall_init(
	polycall_context_t** ctx,
	const polycall_config_t* config
);

/**
 * @brief Clean up and release LibPolyCall resources
 *
 * @param ctx Context to clean up
 */
void polycall_cleanup(polycall_context_t* ctx);

/**
 * @brief Get the LibPolyCall version
 *
 * @return Version information
 */
polycall_version_t polycall_get_version(void);

/**
 * @brief Get the last error message
 *
 * @param ctx Context
 * @return Error message string
 */
const char* polycall_get_error_message(polycall_context_t* ctx);

/**
 * @brief Get the last error code
 *
 * @param ctx Context
 * @return Error code
 */
polycall_error_t polycall_get_error_code(polycall_context_t* ctx);

/**
 * @brief Connect to a server
 *
 * @param ctx Context
 * @param session Pointer to receive session handle
 * @param info Connection information
 * @return Error code
 */
polycall_error_t polycall_connect(
	polycall_context_t* ctx,
	polycall_session_t** session,
	const polycall_connection_info_t* info
);

/**
 * @brief Disconnect from a server
 *
 * @param ctx Context
 * @param session Session to disconnect
 * @return Error code
 */
polycall_error_t polycall_disconnect(
	polycall_context_t* ctx,
	polycall_session_t* session
);

/**
 * @brief Create a message
 *
 * @param ctx Context
 * @param message Pointer to receive message handle
 * @param type Message type
 * @return Error code
 */
polycall_error_t polycall_create_message(
	polycall_context_t* ctx,
	polycall_message_t** message,
	polycall_message_type_t type
);

/**
 * @brief Destroy a message
 *
 * @param ctx Context
 * @param message Message to destroy
 * @return Error code
 */
polycall_error_t polycall_destroy_message(
	polycall_context_t* ctx,
	polycall_message_t* message
);

/**
 * @brief Set message path
 *
 * @param ctx Context
 * @param message Message to modify
 * @param path Path to set
 * @return Error code
 */
polycall_error_t polycall_message_set_path(
	polycall_context_t* ctx,
	polycall_message_t* message,
	const char* path
);

/**
 * @brief Set message data
 *
 * @param ctx Context
 * @param message Message to modify
 * @param data Data to set
 * @param size Size of data
 * @return Error code
 */
polycall_error_t polycall_message_set_data(
	polycall_context_t* ctx,
	polycall_message_t* message,
	const void* data,
	size_t size
);

/**
 * @brief Set message string
 *
 * @param ctx Context
 * @param message Message to modify
 * @param string String to set
 * @return Error code
 */
polycall_error_t polycall_message_set_string(
	polycall_context_t* ctx,
	polycall_message_t* message,
	const char* string
);

/**
 * @brief Set message JSON
 *
 * @param ctx Context
 * @param message Message to modify
 * @param json JSON to set
 * @return Error code
 */
polycall_error_t polycall_message_set_json(
	polycall_context_t* ctx,
	polycall_message_t* message,
	const char* json
);

/**
 * @brief Send a message
 *
 * @param ctx Context
 * @param session Session to use
 * @param message Message to send
 * @param response Pointer to receive response (NULL to ignore)
 * @return Error code
 */
polycall_error_t polycall_send_message(
	polycall_context_t* ctx,
	polycall_session_t* session,
	polycall_message_t* message,
	polycall_message_t** response
);

/**
 * @brief Get message path
 *
 * @param ctx Context
 * @param message Message
 * @return Path
 */
const char* polycall_message_get_path(
	polycall_context_t* ctx,
	polycall_message_t* message
);

/**
 * @brief Get message data
 *
 * @param ctx Context
 * @param message Message
 * @param size Pointer to receive size
 * @return Data
 */
const void* polycall_message_get_data(
	polycall_context_t* ctx,
	polycall_message_t* message,
	size_t* size
);

/**
 * @brief Get message string
 *
 * @param ctx Context
 * @param message Message
 * @return String
 */
const char* polycall_message_get_string(
	polycall_context_t* ctx,
	polycall_message_t* message
);

/**
 * @brief Get message JSON
 *
 * @param ctx Context
 * @param message Message
 * @return JSON
 */
const char* polycall_message_get_json(
	polycall_context_t* ctx,
	polycall_message_t* message
);

/**
 * @brief Create default configuration
 *
 * @return Default configuration
 */
polycall_config_t polycall_create_default_config(void);



/**
 * @brief Load configuration
 *
 * @param filename File to load
 * @return Configuration
 */
polycall_config_t polycall_load_config(const char* filename);

/**
 * @brief Initialize FFI subsystem
 *
 * @param ctx Context
 * @param ffi_config FFI configuration
 * @return Error code
 */
polycall_error_t polycall_init_ffi(
	polycall_context_t* ctx,
	const void* ffi_config
);

/**
 * @brief Initialize protocol subsystem
 *
 * @param ctx Context
 * @param protocol_config Protocol configuration
 * @return Error code
 */
polycall_error_t polycall_init_protocol(
	polycall_context_t* ctx,
	const void* protocol_config
);

/**
 * @brief Set user data
 *
 * @param ctx Context
 * @param user_data User data
 * @return Error code
 */
polycall_error_t polycall_set_user_data(
	polycall_context_t* ctx,
	void* user_data
);

/**
 * @brief Get user data
 *
 * @param ctx Context
 * @return User data
 */
void* polycall_get_user_data(polycall_context_t* ctx);

/**
 * @brief Register callback
 *
 * @param ctx Context
 * @param callback Callback function
 * @param user_data User data
 * @return Error code
 */
polycall_error_t polycall_register_callback(
	polycall_context_t* ctx,
	uint32_t event_type,
	void (*callback)(polycall_context_t*, void*),
	void* user_data
);

/**
 * @brief Unregister callback
 *
 * @param ctx Context
 * @param event_type Event type to unregister
 * @param callback Callback function
 * @return Error code
 */
polycall_error_t polycall_unregister_callback(
	polycall_context_t* ctx,
	uint32_t event_type,
	void (*callback)(polycall_context_t*, void*)
);

/**
 * @brief Set log callback
 *
 * @param ctx Context
 * @param callback Callback function
 * @param user_data User data
 * @return Error code
 */
polycall_error_t polycall_set_log_callback(
	polycall_context_t* ctx,
	void (*callback)(int, const char*, void*),
	void* user_data
);

/**
 * @brief Process messages
 *
 * @param ctx Context
 * @param session Session
 * @param timeout_ms Timeout in milliseconds
 * @return Error code
 */
polycall_error_t polycall_process_messages(
	polycall_context_t* ctx,
	polycall_session_t* session,
	uint32_t timeout_ms
);

/**
 * @brief Initialize all subsystems
 *
 * @param ctx Pointer to receive context
 * @param config Configuration
 * @return Error code
 */
polycall_error_t polycall_init_all(
	polycall_context_t** ctx,
	const polycall_config_t* config
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_H_H */
