/**
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

 * @file protocol_command.h
 * @brief Protocol command handling definitions for LibPolyCall CLI
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the command processing structures and functions for
 * the LibPolyCall CLI interface, based on the core protocol implementation.
 */

#ifndef POLYCALL_CLI_PROTOCOL_COMMAND_H
#define POLYCALL_CLI_PROTOCOL_COMMAND_H
#include "polycall/cli/command.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Protocol command constants
#define POLYCALL_COMMAND_MAGIC 0x434D44   /* "CMD" in ASCII */
#define POLYCALL_COMMAND_VERSION 1
#define POLYCALL_MAX_COMMAND_NAME 64
#define POLYCALL_MAX_ERROR_LENGTH 256
#define POLYCALL_INITIAL_COMMAND_CAPACITY 32
#define POLYCALL_INITIAL_PARAM_CAPACITY 8

// Command registry structure
typedef struct {
	polycall_command_entry_t* commands;
	uint32_t command_count;
	uint32_t capacity;
	uint32_t flags;
	polycall_memory_pool_t* memory_pool;
	void* user_data;
} command_registry_t;

// Command parameter value data structure
typedef struct {
	union {
		int32_t int_value;
		int64_t int64_value;
		float float_value;
		double double_value;
		bool bool_value;
		struct {
			void* data;
			uint32_t size;
		} binary;
		char* string_value;
	} data;
	polycall_parameter_type_t type;
} command_param_value_t;

/**
 * @brief Find command by ID
 *
 * @param registry Command registry
 * @param command_id Command ID to find
 * @return Command entry if found, NULL otherwise
 */
polycall_command_entry_t* polycall_command_find_by_id(
	command_registry_t* registry,
	uint32_t command_id
);

/**
 * @brief Find command by name
 *
 * @param registry Command registry
 * @param name Command name to find
 * @return Command entry if found, NULL otherwise
 */
polycall_command_entry_t* polycall_command_find_by_name(
	command_registry_t* registry,
	const char* name
);

/**
 * @brief Validate command state
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param command Command entry
 * @return Error code
 */
polycall_core_error_t polycall_command_validate_state(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	const polycall_command_entry_t* command
);

/**
 * @brief Validate command permissions
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param command Command entry
 * @return Error code
 */
polycall_core_error_t polycall_command_validate_permissions(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	const polycall_command_entry_t* command
);

/**
 * @brief Serialize command parameter
 *
 * @param ctx Core context
 * @param param Parameter to serialize
 * @param buffer Target buffer
 * @param buffer_size Buffer size
 * @param bytes_written Number of bytes written
 * @return Error code
 */
polycall_core_error_t polycall_command_serialize_parameter(
	polycall_core_context_t* ctx,
	const polycall_command_parameter_t* param,
	void* buffer,
	size_t buffer_size,
	size_t* bytes_written
);

/**
 * @brief Deserialize command parameter
 *
 * @param ctx Core context
 * @param param Parameter to populate
 * @param buffer Source buffer
 * @param buffer_size Buffer size
 * @param bytes_read Number of bytes read
 * @return Error code
 */
polycall_core_error_t polycall_command_deserialize_parameter(
	polycall_core_context_t* ctx,
	polycall_command_parameter_t* param,
	const void* buffer,
	size_t buffer_size,
	size_t* bytes_read
);

/**
 * @brief Free parameter data
 *
 * @param ctx Core context
 * @param param Parameter whose data should be freed
 */
void polycall_command_free_parameter_data(
	polycall_core_context_t* ctx,
	polycall_command_parameter_t* param
);

/**
 * @brief Create command response
 *
 * @param ctx Core context
 * @param status Response status
 * @param data Response data
 * @param data_size Data size
 * @param error_code Error code if status is failure
 * @param error_message Error message if status is failure
 * @param response Output response object
 * @return Error code
 */
polycall_core_error_t polycall_command_create_response(
	polycall_core_context_t* ctx,
	polycall_command_status_t status,
	const void* data,
	uint32_t data_size,
	uint32_t error_code,
	const char* error_message,
	polycall_command_response_t** response
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_PROTOCOL_COMMAND_H */
