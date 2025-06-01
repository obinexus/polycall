/**
 * @file polycall.h 
 * @brief Unified API header for LibPolyCall CLI system
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This header provides the unified public API for the LibPolyCall CLI system,
 * consolidating all component APIs and providing central initialization.
 */

#ifndef POLYCALL_CORE_POLYCALL_H
#define POLYCALL_CORE_POLYCALL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Core System */
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"

/* FFI System */
#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/ffi/type_system.h"

/* Protocol System */
#include "polycall/core/protocol/polycall_protocol_context.h"
#include "polycall/core/protocol/command.h"

/* CLI System */
#include "polycall/cli/command.h"
#include "polycall/cli/repl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Version Information */
#define POLYCALL_VERSION_MAJOR 1
#define POLYCALL_VERSION_MINOR 1
#define POLYCALL_VERSION_PATCH 0
#define POLYCALL_VERSION_STRING "1.1.0"

/**
 * @brief Global initialization flags
 */
typedef enum {
    POLYCALL_INIT_FLAG_NONE = 0,
    POLYCALL_INIT_FLAG_CLI = (1 << 0),
    POLYCALL_INIT_FLAG_FFI = (1 << 1),
    POLYCALL_INIT_FLAG_PROTOCOL = (1 << 2),
    POLYCALL_INIT_FLAG_ALL = 0xFFFFFFFF
} polycall_init_flags_t;

/**
 * @brief Global configuration structure
 */
typedef struct {
    polycall_init_flags_t flags;         /* Initialization flags */
    const char* config_file;             /* Path to config file */
    void* user_data;                     /* User data pointer */
    polycall_core_config_t* core_config; /* Core config */
    polycall_ffi_config_t* ffi_config;   /* FFI config */
    void* protocol_config;               /* Protocol config */
} polycall_config_t;

/**
 * @brief Initialize the LibPolyCall system
 * @param config Configuration parameters (NULL for defaults)
 * @return Error code
 */
polycall_error_t polycall_init(const polycall_config_t* config);

/**
 * @brief Clean up and shutdown LibPolyCall
 */
void polycall_cleanup(void);

/**
 * @brief Get LibPolyCall version information
 * @param major Major version number
 * @param minor Minor version number
 * @param patch Patch version number
 */
void polycall_get_version(int* major, int* minor, int* patch);

/**
 * @brief Register event handler
 * @param event_type Event type to handle
 * @param handler Handler function
 * @param user_data User data passed to handler
 * @return Error code
 */
polycall_error_t polycall_register_event_handler(
    int event_type,
    void (*handler)(void* event, void* user_data),
    void* user_data
);

/**
 * @brief Unregister event handler
 * @param event_type Event type
 * @param handler Handler function to unregister
 * @return Error code
 */
polycall_error_t polycall_unregister_event_handler(
    int event_type,
    void (*handler)(void* event, void* user_data)
);

/**
 * @brief Get global core context
 * @return Pointer to core context
 */
polycall_core_context_t* polycall_get_core_context(void);

/**
 * @brief Get last error information
 * @param error Pointer to receive error code
 * @param message Buffer to receive error message
 * @param size Size of message buffer
 */
void polycall_get_last_error(
    polycall_error_t* error,
    char* message,
    size_t size
);

/**
 * @brief Execute CLI command
 * @param argc Argument count
 * @param argv Argument vector
 * @return Command result code
 */
command_result_t polycall_execute_command(int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CORE_POLYCALL_H */