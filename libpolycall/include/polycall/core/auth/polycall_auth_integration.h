/**
 * @file polycall_auth_integration.h
 * @brief Integration with other LibPolyCall components
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the interfaces for integrating authentication with
 * other LibPolyCall components.
 */

 #ifndef POLYCALL_AUTH_POLYCALL_AUTH_INTEGRATION_H_H
 #define POLYCALL_AUTH_POLYCALL_AUTH_INTEGRATION_H_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/auth/polycall_auth_context.h"
 #include <stdbool.h>
 #include <stdint.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Forward declarations for other LibPolyCall components */
 typedef struct polycall_protocol_context polycall_protocol_context_t;
 typedef struct polycall_micro_context polycall_micro_context_t;
 typedef struct polycall_edge_context polycall_edge_context_t;
 typedef struct polycall_telemetry_context polycall_telemetry_context_t;
 typedef struct polycall_message polycall_message_t;
 
 /**
  * @brief Register with the protocol system
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param protocol_ctx Protocol context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_register_with_protocol(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     polycall_protocol_context_t* protocol_ctx
 );
 
 /**
  * @brief Register with the micro command system
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param micro_ctx Micro command context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_register_with_micro(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     polycall_micro_context_t* micro_ctx
 );
 
 /**
  * @brief Register with the edge command system
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param edge_ctx Edge command context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_register_with_edge(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     polycall_edge_context_t* edge_ctx
 );
 
 /**
  * @brief Register with the telemetry system
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param telemetry_ctx Telemetry context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_register_with_telemetry(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     polycall_telemetry_context_t* telemetry_ctx
 );
 
 /**
  * @brief Validate a protocol message
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param message Protocol message
  * @param identity_id Pointer to receive the identity ID associated with the message
  * @param allowed Pointer to receive whether the message is allowed
  * @return Error code
  */
 polycall_core_error_t polycall_auth_validate_message(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     const polycall_message_t* message,
     char** identity_id,
     bool* allowed
 );
 
 /**
  * @brief Set up authentication message handlers
  *
  * @param core_ctx Core context
  * @param auth_ctx Auth context
  * @param protocol_ctx Protocol context
  * @return Error code
  */
 polycall_core_error_t polycall_auth_setup_message_handlers(
     polycall_core_context_t* core_ctx,
     polycall_auth_context_t* auth_ctx,
     polycall_protocol_context_t* protocol_ctx
 );
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_AUTH_POLYCALL_AUTH_INTEGRATION_H_H */