/**
#include "polycall/core/protocol/command.h"
#include "polycall/core/protocol/command.h"

#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
 * @file command.c
 * @brief Protocol command handling for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the command processing functionality for the LibPolyCall protocol,
 * enabling secure, validated command execution between endpoints within the
 * Program-First architecture.
 */



// Initialize command registry
polycall_core_error_t polycall_command_init(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    polycall_command_registry_t** registry,
    const polycall_command_config_t* config
) {
    if (!ctx || !proto_ctx || !registry || !config) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }

    // Allocate registry 
    command_registry_t* new_registry = polycall_core_malloc(ctx, sizeof(command_registry_t));
    if (!new_registry) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize registry fields
    memset(new_registry, 0, sizeof(command_registry_t));
    new_registry->flags = config->flags;
    new_registry->user_data = config->user_data;
    
    // Create memory pool for command registry
    polycall_core_error_t result = polycall_memory_create_pool(
        ctx,
        &new_registry->memory_pool,
        config->memory_pool_size > 0 ? config->memory_pool_size : (1024 * 1024) // Default 1MB
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_core_free(ctx, new_registry);
        return result;
    }
    
    // Allocate initial command array
    uint32_t initial_capacity = config->initial_command_capacity > 0 ? 
                              config->initial_command_capacity : 
                              POLYCALL_INITIAL_COMMAND_CAPACITY;
                              
    new_registry->commands = polycall_memory_alloc(
        ctx,
        new_registry->memory_pool,
        initial_capacity * sizeof(polycall_command_entry_t),
        POLYCALL_MEMORY_FLAG_ZERO_INIT
    );
    
    if (!new_registry->commands) {
        polycall_memory_destroy_pool(ctx, new_registry->memory_pool);
        polycall_core_free(ctx, new_registry);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_registry->capacity = initial_capacity;
    new_registry->command_count = 0;
    
    // Register with protocol context if provided
    if (proto_ctx) {
        // In a full implementation, we would register this registry with the
        // protocol context here. For now, we'll just cast and return.
    }
    
    *registry = (polycall_command_registry_t*)new_registry;
    return POLYCALL_CORE_SUCCESS;
}

// Clean up command registry
void polycall_command_cleanup(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry
) {
    if (!ctx || !registry) {
        return;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    
    // Destroy the memory pool (which will free all commands)
    if (internal_registry->memory_pool) {
        polycall_memory_destroy_pool(ctx, internal_registry->memory_pool);
    }
    
    // Free registry structure
    polycall_core_free(ctx, internal_registry);
}

// Register a command
polycall_core_error_t polycall_command_register(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry,
    const polycall_command_info_t* command_info,
    uint32_t* command_id
) {
    if (!ctx || !registry || !command_info || !command_info->handler) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    
    // Check if command with same name already exists
    if (find_command_by_name(internal_registry, command_info->name)) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                         POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                         POLYCALL_ERROR_SEVERITY_ERROR, 
                         "Command with name '%s' already registered", command_info->name);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if we need to grow the command array
    if (internal_registry->command_count >= internal_registry->capacity) {
        uint32_t new_capacity = internal_registry->capacity * 2;
        polycall_command_entry_t* new_commands = polycall_memory_alloc(
            ctx,
            internal_registry->memory_pool,
            new_capacity * sizeof(polycall_command_entry_t),
            POLYCALL_MEMORY_FLAG_ZERO_INIT
        );
        
        if (!new_commands) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy existing commands
        memcpy(new_commands, internal_registry->commands, 
               internal_registry->command_count * sizeof(polycall_command_entry_t));
        
        // Replace command array
        polycall_memory_free(ctx, internal_registry->memory_pool, internal_registry->commands);
        internal_registry->commands = new_commands;
        internal_registry->capacity = new_capacity;
    }
    
    // Generate command ID if not provided
    uint32_t new_command_id = command_info->command_id;
    if (new_command_id == 0) {
        // Simple algorithm: use command count + 1000 as ID
        new_command_id = internal_registry->command_count + 1000;
        
        // Ensure ID is unique
        while (find_command_by_id(internal_registry, new_command_id)) {
            new_command_id++;
        }
    } else {
        // Check if ID is already in use
        if (find_command_by_id(internal_registry, new_command_id)) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                             POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                             POLYCALL_ERROR_SEVERITY_ERROR, 
                             "Command ID %u already in use", new_command_id);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
    }
    
    // Add command to registry
    polycall_command_entry_t* entry = &internal_registry->commands[internal_registry->command_count];
    entry->command_id = new_command_id;
    strncpy(entry->name, command_info->name, POLYCALL_MAX_COMMAND_NAME - 1);
    entry->name[POLYCALL_MAX_COMMAND_NAME - 1] = '\0';
    entry->handler = command_info->handler;
    entry->validator = command_info->validator;
    entry->permissions = command_info->permissions;
    entry->flags = command_info->flags;
    entry->user_data = command_info->user_data;
    
    internal_registry->command_count++;
    
    // Return command ID if requested
    if (command_id) {
        *command_id = new_command_id;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Unregister a command
polycall_core_error_t polycall_command_unregister(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry,
    uint32_t command_id
) {
    if (!ctx || !registry || command_id == 0) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    
    // Find command index
    int32_t found_index = -1;
    for (uint32_t i = 0; i < internal_registry->command_count; i++) {
        if (internal_registry->commands[i].command_id == command_id) {
            found_index = i;
            break;
        }
    }
    
    // Return if command not found
    if (found_index < 0) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Remove command by shifting remaining entries
    if ((uint32_t)found_index < internal_registry->command_count - 1) {
        memmove(&internal_registry->commands[found_index],
                &internal_registry->commands[found_index + 1],
                (internal_registry->command_count - found_index - 1) * sizeof(polycall_command_entry_t));
    }
    
    // Clear the last entry and decrement count
    memset(&internal_registry->commands[internal_registry->command_count - 1], 0, sizeof(polycall_command_entry_t));
    internal_registry->command_count--;
    
    return POLYCALL_CORE_SUCCESS;
}

// Find command by ID
polycall_core_error_t polycall_command_find_by_id(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry,
    uint32_t command_id,
    polycall_command_info_t* command_info
) {
    if (!ctx || !registry || command_id == 0 || !command_info) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    polycall_command_entry_t* entry = find_command_by_id(internal_registry, command_id);
    
    if (!entry) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Copy command info
    command_info->command_id = entry->command_id;
    strncpy(command_info->name, entry->name, POLYCALL_MAX_COMMAND_NAME - 1);
    command_info->name[POLYCALL_MAX_COMMAND_NAME - 1] = '\0';
    command_info->handler = entry->handler;
    command_info->validator = entry->validator;
    command_info->permissions = entry->permissions;
    command_info->flags = entry->flags;
    command_info->user_data = entry->user_data;
    
    return POLYCALL_CORE_SUCCESS;
}

// Find command by name
polycall_core_error_t polycall_command_find_by_name(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry,
    const char* name,
    polycall_command_info_t* command_info
) {
    if (!ctx || !registry || !name || !command_info) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    polycall_command_entry_t* entry = find_command_by_name(internal_registry, name);
    
    if (!entry) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Copy command info
    command_info->command_id = entry->command_id;
    strncpy(command_info->name, entry->name, POLYCALL_MAX_COMMAND_NAME - 1);
    command_info->name[POLYCALL_MAX_COMMAND_NAME - 1] = '\0';
    command_info->handler = entry->handler;
    command_info->validator = entry->validator;
    command_info->permissions = entry->permissions;
    command_info->flags = entry->flags;
    command_info->user_data = entry->user_data;
    
    return POLYCALL_CORE_SUCCESS;
}

// Create a command message
polycall_core_error_t polycall_command_create_message(
    polycall_core_context_t* ctx,
    polycall_command_message_t** message,
    uint32_t command_id
) {
    if (!ctx || !message || command_id == 0) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate command message
    polycall_command_message_t* new_message = polycall_core_malloc(ctx, sizeof(polycall_command_message_t));
    if (!new_message) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize message
    memset(new_message, 0, sizeof(polycall_command_message_t));
    new_message->header.version = POLYCALL_COMMAND_VERSION;
    new_message->header.command_id = command_id;
    new_message->header.param_count = 0;
    new_message->header.flags = 0;
    
    // Allocate initial parameter array
    new_message->parameters = polycall_core_malloc(ctx, POLYCALL_INITIAL_PARAM_CAPACITY * sizeof(polycall_command_parameter_t));
    if (!new_message->parameters) {
        polycall_core_free(ctx, new_message);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    new_message->capacity = POLYCALL_INITIAL_PARAM_CAPACITY;
    
    *message = new_message;
    return POLYCALL_CORE_SUCCESS;
}

// Destroy a command message
void polycall_command_destroy_message(
    polycall_core_context_t* ctx,
    polycall_command_message_t* message
) {
    if (!ctx || !message) {
        return;
    }
    
    // Free parameter data
    if (message->parameters) {
        for (uint32_t i = 0; i < message->header.param_count; i++) {
            free_parameter_data(ctx, &message->parameters[i]);
        }
        polycall_core_free(ctx, message->parameters);
    }
    
    // Free message structure
    polycall_core_free(ctx, message);
}

// Get parameter from command message
polycall_core_error_t polycall_command_get_parameter(
    polycall_core_context_t* ctx,
    const polycall_command_message_t* message,
    uint16_t param_id,
    polycall_parameter_type_t expected_type,
    void* data,
    uint32_t* data_size
) {
    if (!ctx || !message || !data || !data_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Find parameter by ID
    const polycall_command_parameter_t* param = NULL;
    for (uint32_t i = 0; i < message->header.param_count; i++) {
        if (message->parameters[i].param_id == param_id) {
            param = &message->parameters[i];
            break;
        }
    }
    
    // Return error if parameter not found
    if (!param) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check type if expected type is specified
    if (expected_type != POLYCALL_PARAM_TYPE_ANY && param->type != expected_type) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                         POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                         POLYCALL_ERROR_SEVERITY_ERROR, 
                         "Parameter type mismatch: expected %d, got %d", 
                         expected_type, param->type);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check buffer size
    if (*data_size < param->data_size) {
        *data_size = param->data_size;
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Copy parameter data
    memcpy(data, param->data, param->data_size);
    *data_size = param->data_size;
    
    return POLYCALL_CORE_SUCCESS;
}

// Serialize command message to buffer
polycall_core_error_t polycall_command_serialize(
    polycall_core_context_t* ctx,
    const polycall_command_message_t* message,
    void** buffer,
    size_t* buffer_size
) {
    if (!ctx || !message || !buffer || !buffer_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Calculate total size needed for serialization
    size_t total_size = sizeof(message->header);
    
    // Add size for each parameter
    for (uint32_t i = 0; i < message->header.param_count; i++) {
        const polycall_command_parameter_t* param = &message->parameters[i];
        total_size += sizeof(uint16_t); // param_id
        total_size += sizeof(uint8_t);  // param_type
        total_size += sizeof(uint16_t); // param_flags
        total_size += sizeof(uint32_t); // data_size
        total_size += param->data_size; // data
    }
    
    // Allocate buffer
    void* serialized = polycall_core_malloc(ctx, total_size);
    if (!serialized) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Copy header
    uint8_t* ptr = (uint8_t*)serialized;
    memcpy(ptr, &message->header, sizeof(message->header));
    ptr += sizeof(message->header);
    
    // Copy each parameter
    for (uint32_t i = 0; i < message->header.param_count; i++) {
        const polycall_command_parameter_t* param = &message->parameters[i];
        
        // Copy param_id
        memcpy(ptr, &param->param_id, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        
        // Copy param_type
        memcpy(ptr, &param->type, sizeof(uint8_t));
        ptr += sizeof(uint8_t);
        
        // Copy param_flags
        memcpy(ptr, &param->flags, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        
        // Copy data_size
        memcpy(ptr, &param->data_size, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Copy data
        memcpy(ptr, param->data, param->data_size);
        ptr += param->data_size;
    }
    
    *buffer = serialized;
    *buffer_size = total_size;
    
    return POLYCALL_CORE_SUCCESS;
}

// Deserialize command message from buffer
polycall_core_error_t polycall_command_deserialize(
    polycall_core_context_t* ctx,
    const void* buffer,
    size_t buffer_size,
    polycall_command_message_t** message
) {
    if (!ctx || !buffer || buffer_size < sizeof(struct {
            uint8_t version;
            uint32_t command_id;
            uint32_t flags;
            uint32_t param_count;
        }) || !message) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    const uint8_t* ptr = (const uint8_t*)buffer;
    
    // Read header
    struct {
        uint8_t version;
        uint32_t command_id;
        uint32_t flags;
        uint32_t param_count;
    } header;
    
    memcpy(&header, ptr, sizeof(header));
    ptr += sizeof(header);
    
    // Validate header
    if (header.version != POLYCALL_COMMAND_VERSION) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                         POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                         POLYCALL_ERROR_SEVERITY_ERROR, 
                         "Unsupported command version: %d", header.version);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create command message
    polycall_command_message_t* new_message;
    polycall_core_error_t result = polycall_command_create_message(
        ctx,
        &new_message,
        header.command_id
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    new_message->header.flags = header.flags;
    
    // Read parameters
    for (uint32_t i = 0; i < header.param_count; i++) {
        // Check if we have enough buffer left
        if ((size_t)(ptr - (const uint8_t*)buffer) >= buffer_size) {
            polycall_command_destroy_message(ctx, new_message);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Read parameter header
        uint16_t param_id;
        uint8_t param_type;
        uint16_t param_flags;
        uint32_t data_size;
        
        memcpy(&param_id, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        
        memcpy(&param_type, ptr, sizeof(uint8_t));
        ptr += sizeof(uint8_t);
        
        memcpy(&param_flags, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);
        
        memcpy(&data_size, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Check if we have enough buffer left for data
        if ((size_t)(ptr - (const uint8_t*)buffer) + data_size > buffer_size) {
            polycall_command_destroy_message(ctx, new_message);
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        // Add parameter to message
        result = polycall_command_add_parameter(
            ctx,
            new_message,
            param_id,
            (polycall_parameter_type_t)param_type,
            ptr,
            data_size,
            param_flags
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_command_destroy_message(ctx, new_message);
            return result;
        }
        
        ptr += data_size;
    }
    
    *message = new_message;
    return POLYCALL_CORE_SUCCESS;
}

// Execute a command
polycall_core_error_t polycall_command_execute(
    polycall_core_context_t* ctx,
    polycall_command_registry_t* registry,
    polycall_protocol_context_t* proto_ctx,
    const polycall_command_message_t* message,
    polycall_command_response_t** response
) {
    if (!ctx || !registry || !proto_ctx || !message || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    command_registry_t* internal_registry = (command_registry_t*)registry;
    
    // Find command
    polycall_command_entry_t* command = find_command_by_id(
        internal_registry,
        message->header.command_id
    );
    
    if (!command) {
        // Create error response
        polycall_core_error_t result = create_command_response(
            ctx,
            POLYCALL_COMMAND_STATUS_ERROR,
            NULL,
            0,
            POLYCALL_COMMAND_ERROR_INVALID_COMMAND,
            "Command not found",
            response
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Validate protocol state
    polycall_core_error_t result = validate_command_state(ctx, proto_ctx, command);
    if (result != POLYCALL_CORE_SUCCESS) {
        // Create error response
        result = create_command_response(
            ctx,
            POLYCALL_COMMAND_STATUS_ERROR,
            NULL,
            0,
            POLYCALL_COMMAND_ERROR_INVALID_STATE,
            "Command cannot be executed in current protocol state",
            response
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Validate permissions
    result = validate_command_permissions(ctx, proto_ctx, command);
    if (result != POLYCALL_CORE_SUCCESS) {
        // Create error response
        result = create_command_response(
            ctx,
            POLYCALL_COMMAND_STATUS_ERROR,
            NULL,
            0,
            POLYCALL_COMMAND_ERROR_PERMISSION_DENIED,
            "Permission denied for command execution",
            response
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Validate command parameters if validator is provided
    if (command->validator) {
        polycall_command_validation_t validation = command->validator(
            ctx,
            proto_ctx,
            message,
            command->user_data
        );
        
        if (validation.status != POLYCALL_COMMAND_STATUS_SUCCESS) {
            // Create error response using validation result
            result = create_command_response(
                ctx,
                POLYCALL_COMMAND_STATUS_ERROR,
                NULL,
                0,
                validation.error_code,
                validation.error_message,
                response
            );
            
            if (result != POLYCALL_CORE_SUCCESS) {
                return result;
            }
            
            return POLYCALL_CORE_SUCCESS;
        }
    }
    
    // Execute command
    polycall_command_response_t* cmd_response = command->handler(
        ctx,
        proto_ctx,
        message,
        command->user_data
    );
    
    if (!cmd_response) {
        // Create error response for handler failure
        result = create_command_response(
            ctx,
            POLYCALL_COMMAND_STATUS_ERROR,
            NULL,
            0,
            POLYCALL_COMMAND_ERROR_EXECUTION_FAILED,
            "Command execution failed",
            response
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            return result;
        }
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Pass response back to caller
    *response = cmd_response;
    
    return POLYCALL_CORE_SUCCESS;
}

// Create a command response
polycall_core_error_t polycall_command_create_response(
    polycall_core_context_t* ctx,
    polycall_command_status_t status,
    const void* data,
    uint32_t data_size,
    polycall_command_response_t** response
) {
    return create_command_response(
        ctx,
        status,
        data,
        data_size,
        0,
        NULL,
        response
    );
}

// Create an error response
polycall_core_error_t polycall_command_create_error_response(
    polycall_core_context_t* ctx,
    uint32_t error_code,
    const char* error_message,
    polycall_command_response_t** response
) {
    return create_command_response(
        ctx,
        POLYCALL_COMMAND_STATUS_ERROR,
        NULL,
        0,
        error_code,
        error_message,
        response
    );
}

// Destroy a command response
void polycall_command_destroy_response(
    polycall_core_context_t* ctx,
    polycall_command_response_t* response
) {
    if (!ctx || !response) {
        return;
    }
    
    // Free response data if any
    if (response->response_data) {
        polycall_core_free(ctx, response->response_data);
    }
    
    // Free response structure
    polycall_core_free(ctx, response);
}

// Serialize command response to buffer
polycall_core_error_t polycall_command_serialize_response(
    polycall_core_context_t* ctx,
    const polycall_command_response_t* response,
    void** buffer,
    size_t* buffer_size
) {
    if (!ctx || !response || !buffer || !buffer_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Calculate total size
    size_t total_size = sizeof(uint32_t); // status
    total_size += sizeof(uint32_t);      // error_code
    total_size += sizeof(uint32_t);      // data_size
    
    // Add error message size if present
    size_t error_message_length = 0;
    if (response->status == POLYCALL_COMMAND_STATUS_ERROR && response->error_message[0] != '\0') {
        error_message_length = strlen(response->error_message) + 1;
        total_size += error_message_length;
    }
    
    // Add response data size if present
    if (response->response_data && response->data_size > 0) {
        total_size += response->data_size;
    }
    
    // Allocate buffer
    void* serialized = polycall_core_malloc(ctx, total_size);
    if (!serialized) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Write to buffer
    uint8_t* ptr = (uint8_t*)serialized;
    
    // Write status
    memcpy(ptr, &response->status, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // Write error code
    memcpy(ptr, &response->error_code, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // Write data size
    memcpy(ptr, &response->data_size, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // Write error message if present
    if (error_message_length > 0) {
        memcpy(ptr, response->error_message, error_message_length);
        ptr += error_message_length;
    }
    
    // Write response data if present
    if (response->response_data && response->data_size > 0) {
        memcpy(ptr, response->response_data, response->data_size);
    }
    
    *buffer = serialized;
    *buffer_size = total_size;
    
    return POLYCALL_CORE_SUCCESS;
}

// Deserialize command response from buffer
polycall_core_error_t polycall_command_deserialize_response(
    polycall_core_context_t* ctx,
    const void* buffer,
    size_t buffer_size,
    polycall_command_response_t** response
) {
    if (!ctx || !buffer || buffer_size < 3 * sizeof(uint32_t) || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    const uint8_t* ptr = (const uint8_t*)buffer;
    
    // Read header fields
    uint32_t status, error_code, data_size;
    
    memcpy(&status, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    memcpy(&error_code, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    memcpy(&data_size, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    // Validate status
    if (status != POLYCALL_COMMAND_STATUS_SUCCESS && status != POLYCALL_COMMAND_STATUS_ERROR) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if buffer has enough data
    size_t remaining = buffer_size - 3 * sizeof(uint32_t);
    if (remaining < data_size) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create response object
    polycall_command_response_t* new_response = polycall_core_malloc(ctx, sizeof(polycall_command_response_t));
    if (!new_response) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize response
    memset(new_response, 0, sizeof(polycall_command_response_t));
    new_response->status = (polycall_command_status_t)status;
    new_response->error_code = error_code;
    new_response->data_size = data_size;
    
    // Read error message if present
    if (status == POLYCALL_COMMAND_STATUS_ERROR && remaining > data_size) {
        size_t max_error_length = POLYCALL_MAX_ERROR_LENGTH - 1;
        size_t error_message_length = remaining - data_size;
        if (error_message_length > max_error_length) {
            error_message_length = max_error_length;
        }
        
        strncpy(new_response->error_message, (const char*)ptr, error_message_length);
        new_response->error_message[error_message_length] = '\0';
        ptr += error_message_length;
    }
    
    // Read response data if present
    if (data_size > 0) {
        new_response->response_data = polycall_core_malloc(ctx, data_size);
        if (!new_response->response_data) {
            polycall_core_free(ctx, new_response);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(new_response->response_data, ptr, data_size);
    }
    
    *response = new_response;
    return POLYCALL_CORE_SUCCESS;
}

// Add parameter to command message
polycall_core_error_t polycall_command_add_parameter(
    polycall_core_context_t* ctx,
    polycall_command_message_t* message,
    uint16_t param_id,
    polycall_parameter_type_t type,
    const void* data,
    uint32_t data_size,
    uint16_t flags
) {
    if (!ctx || !message || !data) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check if we need to grow the parameter array
    if (message->header.param_count >= message->capacity) {
        uint32_t new_capacity = message->capacity * 2;
        polycall_command_parameter_t* new_params = polycall_core_malloc(
            ctx,
            new_capacity * sizeof(polycall_command_parameter_t)
        );
        
        if (!new_params) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy existing parameters
        memcpy(new_params, message->parameters, 
               message->header.param_count * sizeof(polycall_command_parameter_t));
        
        // Replace parameter array
        polycall_core_free(ctx, message->parameters);
        message->parameters = new_params;
        message->capacity = new_capacity;
    }
    
    // Initialize parameter
    polycall_command_parameter_t* param = &message->parameters[message->header.param_count];
    param->param_id = param_id;
    param->type = type;
    param->flags = flags;
    
    // Copy parameter data based on type
    switch (type) {
        case POLYCALL_PARAM_TYPE_INT32:
            param->data_size = sizeof(int32_t);
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_INT64:
            param->data_size = sizeof(int64_t);
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_FLOAT:
            param->data_size = sizeof(float);
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_DOUBLE:
            param->data_size = sizeof(double);
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_BOOL:
            param->data_size = sizeof(bool);
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_STRING:
            param->data_size = data_size;
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        case POLYCALL_PARAM_TYPE_BINARY:
            param->data_size = data_size;
            param->data = polycall_core_malloc(ctx, param->data_size);
            if (!param->data) {
                return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(param->data, data, param->data_size);
            break;
            
        default:
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    message->header.param_count++;
    return POLYCALL_CORE_SUCCESS;
}

// Implementation of internal helpers

// Find command by ID
static polycall_command_entry_t* find_command_by_id(
    command_registry_t* registry,
    uint32_t command_id
) {
    if (!registry || command_id == 0) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < registry->command_count; i++) {
        if (registry->commands[i].command_id == command_id) {
            return &registry->commands[i];
        }
    }
    
    return NULL;
}

// Find command by name
static polycall_command_entry_t* find_command_by_name(
    command_registry_t* registry,
    const char* name
) {
    if (!registry || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < registry->command_count; i++) {
        if (strcmp(registry->commands[i].name, name) == 0) {
            return &registry->commands[i];
        }
    }
    
    return NULL;
}

// Validate command state
static polycall_core_error_t validate_command_state(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const polycall_command_entry_t* command
) {
    if (!ctx || !proto_ctx || !command) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get current protocol state
    polycall_protocol_state_t current_state = polycall_protocol_get_state(proto_ctx);
    
    // Only allow commands in READY state unless specifically flagged
    if (current_state != POLYCALL_PROTOCOL_STATE_READY) {
        // Check if command is allowed in non-READY states
        if (!(command->flags & POLYCALL_COMMAND_FLAG_ALLOW_ANY_STATE)) {
            // Special case for authentication commands
            if (current_state == POLYCALL_PROTOCOL_STATE_AUTH && 
                (command->flags & POLYCALL_COMMAND_FLAG_AUTH_COMMAND)) {
                return POLYCALL_CORE_SUCCESS;
            }
            
            // Special case for handshake commands
            if (current_state == POLYCALL_PROTOCOL_STATE_HANDSHAKE && 
                (command->flags & POLYCALL_COMMAND_FLAG_HANDSHAKE_COMMAND)) {
                return POLYCALL_CORE_SUCCESS;
            }
            
            // Not allowed in this state
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                             POLYCALL_CORE_ERROR_INVALID_STATE,
                             POLYCALL_ERROR_SEVERITY_ERROR, 
                             "Command not allowed in current protocol state");
            return POLYCALL_CORE_ERROR_INVALID_STATE;
        }
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Validate command permissions
static polycall_core_error_t validate_command_permissions(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const polycall_command_entry_t* command
) {
    if (!ctx || !proto_ctx || !command) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // For now, just check if secure commands are only allowed in secure state
    if ((command->flags & POLYCALL_COMMAND_FLAG_SECURE) && 
        !(proto_ctx->header.flags & POLYCALL_PROTOCOL_FLAG_SECURE)) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Secure command not allowed in non-secure protocol state");
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // In a real implementation, we would check current authentication level,
    // role-based permissions, etc. For now, just assume success.
    
    return POLYCALL_CORE_SUCCESS;
}

// Free parameter data
static void free_parameter_data(
    polycall_core_context_t* ctx,
    polycall_command_parameter_t* param
) {
    if (!ctx || !param || !param->data) {
        return;
    }
    
    // Free data pointer
    polycall_core_free(ctx, param->data);
    param->data = NULL;
    param->data_size = 0;
}

// Create a command response (internal)
static polycall_core_error_t create_command_response(
    polycall_core_context_t* ctx,
    polycall_command_status_t status,
    const void* data,
    uint32_t data_size,
    uint32_t error_code,
    const char* error_message,
    polycall_command_response_t** response
) {
    if (!ctx || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Validate status
    if (status != POLYCALL_COMMAND_STATUS_SUCCESS && status != POLYCALL_COMMAND_STATUS_ERROR) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Create response object
    polycall_command_response_t* new_response = polycall_core_malloc(ctx, sizeof(polycall_command_response_t));
    if (!new_response) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize response
    memset(new_response, 0, sizeof(polycall_command_response_t));
    new_response->status = status;
    new_response->error_code = error_code;
    
    // Copy error message if provided
    if (status == POLYCALL_COMMAND_STATUS_ERROR && error_message) {
        strncpy(new_response->error_message, error_message, POLYCALL_MAX_ERROR_LENGTH - 1);
        new_response->error_message[POLYCALL_MAX_ERROR_LENGTH - 1] = '\0';
    }
    
    // Copy response data if provided
    if (status == POLYCALL_COMMAND_STATUS_SUCCESS && data && data_size > 0) {
        new_response->response_data = polycall_core_malloc(ctx, data_size);
        if (!new_response->response_data) {
            polycall_core_free(ctx, new_response);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(new_response->response_data, data, data_size);
        new_response->data_size = data_size;
    }
    
    *response = new_response;
    return POLYCALL_CORE_SUCCESS;
}

// Helper function to send a command via protocol
polycall_core_error_t polycall_command_send(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const polycall_command_message_t* message
) {
    if (!ctx || !proto_ctx || !message) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Serialize command message
    void* buffer;
    size_t buffer_size;
    polycall_core_error_t result = polycall_command_serialize(
        ctx,
        message,
        &buffer,
        &buffer_size
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Send via protocol
    bool sent = polycall_protocol_send(
        proto_ctx,
        POLYCALL_PROTOCOL_MSG_COMMAND,
        buffer,
        buffer_size,
        POLYCALL_PROTOCOL_FLAG_RELIABLE
    );
    
    // Free serialized buffer
    polycall_core_free(ctx, buffer);
    
    if (!sent) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to send command message");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Helper function to send a command response
polycall_core_error_t polycall_command_send_response(
    polycall_core_context_t* ctx,
    polycall_protocol_context_t* proto_ctx,
    const polycall_command_response_t* response
) {
    if (!ctx || !proto_ctx || !response) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Serialize response
    void* buffer;
    size_t buffer_size;
    polycall_core_error_t result = polycall_command_serialize_response(
        ctx,
        response,
        &buffer,
        &buffer_size
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        return result;
    }
    
    // Send via protocol
    bool sent = polycall_protocol_send(
        proto_ctx,
        POLYCALL_PROTOCOL_MSG_RESPONSE,
        buffer,
        buffer_size,
        POLYCALL_PROTOCOL_FLAG_RELIABLE
    );
    
    // Free serialized buffer
    polycall_core_free(ctx, buffer);
    
    if (!sent) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to send command response");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    return POLYCALL_CORE_SUCCESS;
}