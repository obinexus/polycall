/**
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

 * @file polycall.h
 * @brief Comprehensive public header for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This is the main include file for applications using LibPolyCall.
 * It provides access to all public APIs across all LibPolyCall components.
 */

#ifndef POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
typedef struct polycall_version {
  int major;          /**< Major version number */
  int minor;          /**< Minor version number */
  int patch;          /**< Patch level */
  const char *string; /**< Version as a string */
} polycall_version_t;

#define POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H

/* Core system */
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_program.h"
#include "polycall/core/polycall/polycall_public.h"

/* Foreign Function Interface (FFI) */
#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/ffi/memory_bridge.h"
#include "polycall/core/ffi/performance.h"
#include "polycall/core/ffi/protocol_bridge.h"
#include "polycall/core/ffi/security.h"
#include "polycall/core/ffi/type_system.h"

/* Language-specific bridges */
#include "polycall/core/ffi/c_bridge.h"
#include "polycall/core/ffi/js_bridge.h"
#include "polycall/core/ffi/jvm_bridge.h"
#include "polycall/core/ffi/python_bridge.h"

/* Protocol system */
#include "polycall/core/protocol/command.h"
#include "polycall/core/protocol/communication.h"
#include "polycall/core/protocol/polycall_protocol_context.h"

/* Protocol enhancements */
#include "polycall/core/protocol/enhancements/advanced_security.h"
#include "polycall/core/protocol/enhancements/connection_pool.h"
#include "polycall/core/protocol/enhancements/hierarchical_state.h"
#include "polycall/core/protocol/enhancements/message_optimization.h"
#include "polycall/core/protocol/enhancements/polycall_state_machine.h"
#include "polycall/core/protocol/enhancements/protocol_enhacements_config.h"
#include "polycall/core/protocol/enhancements/subscription.h"

/* Configuration and parsing */

/* Micro command system */
#include "polycall/core/micro/polycall_micro_component.h"
#include "polycall/core/micro/polycall_micro_context.h"
#include "polycall/core/micro/polycall_micro_resource.h"
#include "polycall/core/micro/polycall_micro_security.h"

/* Edge computing */
#include "polycall/core/edge/compute_router.h"
#include "polycall/core/edge/fallback.h"
#include "polycall/core/edge/polycall_edge.h"
#include "polycall/core/edge/security.h"

/* Network layer */
#include "polycall/core/network/network.h"
#include "polycall/core/network/network_client.h"
#include "polycall/core/network/network_endpoint.h"
#include "polycall/core/network/network_packet.h"
#include "polycall/core/network/network_server.h"

/* Telemetry system */
#include "polycall/core/telemetry/polycall_telemetry.h"
#include "polycall/core/telemetry/polycall_telemetry_reporting.h"
#include "polycall/core/telemetry/polycall_telemetry_security.h"

/**
 * @brief Initialize the LibPolyCall system
 *
 * This function initializes all components of the LibPolyCall system.
 * It must be called before any other LibPolyCall functions are used.
 *
 * @param config Configuration parameters
 * @return POLYCALL_SUCCESS on success, error code otherwise
 */
int polycall_initialize(void *config);

/**
 * @brief Shut down the LibPolyCall system
 *
 * This function releases all resources used by the LibPolyCall system.
 * It should be called when the application is done using LibPolyCall.
 *
 * @return POLYCALL_SUCCESS on success, error code otherwise
 */
int polycall_shutdown(void);

/**
 * @brief Get the version of LibPolyCall
 *
 * @param major Pointer to receive major version number
 * @param minor Pointer to receive minor version number
 * @param patch Pointer to receive patch version number
 */
 polycall_version_t polycall_get_version(void)* patch);

 /**
  * @brief Register an event handler for LibPolyCall events
  *
  * @param event_type Type of event to register for
#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H */

 /**
  * @brief Unregister an event handler
  *
  * @param event_type Type of event to unregister for
  * @param handler Event handler function
  * @return POLYCALL_SUCCESS on success, error code otherwise
  */
 int polycall_unregister_event_handler(int event_type,
                                       void (*handler)(void *event,
                                                       void *user_data));

#ifdef __cplusplus
 }
#endif

#endif /* POLYCALL_CONFIG_POLYCALLFILE_POLYCALL_H_H */