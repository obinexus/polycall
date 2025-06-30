/**
 * @file mock_protocol_context.h
 * @brief Mock implementation of protocol context for testing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides mock implementations of the protocol context functionality
 * for use in unit tests of protocol enhancements.
 */

 #ifndef MOCK_PROTOCOL_CONTEXT_H
 #define MOCK_PROTOCOL_CONTEXT_H
 
 #include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/protocol/protocol_state_machine.h"
 #include "mock_network_endpoint.h"
 
 /**
  * @brief Create a mock protocol context for testing
  * 
  * @param core_ctx The core context
  * @param endpoint The mock network endpoint
  * @return A mock protocol context instance
  */
 polycall_protocol_context_t* mock_protocol_context_create(
     polycall_core_context_t* core_ctx,
     NetworkEndpoint* endpoint
 );
 
 /**
  * @brief Destroy a mock protocol context
  * 
  * @param proto_ctx The mock protocol context to destroy
  */
 void mock_protocol_context_destroy(polycall_protocol_context_t* proto_ctx);
 
 /**
  * @brief Mock implementation of sending a protocol message
  * 
  * @param proto_ctx The protocol context
  * @param type Message type
  * @param payload Message payload
  * @param payload_length Payload length
  * @param flags Message flags
  * @return true if successful, false otherwise
  */
 bool mock_protocol_send(
     polycall_protocol_context_t* proto_ctx,
     polycall_protocol_msg_type_t type,
     const void* payload,
     size_t payload_length,
     polycall_protocol_flags_t flags
 );
 
 /**
  * @brief Mock implementation of processing a received message
  * 
  * @param proto_ctx The protocol context
  * @param data Message data
  * @param length Message length
  * @return true if successful, false otherwise
  */
 bool mock_protocol_process(
     polycall_protocol_context_t* proto_ctx,
     const void* data,
     size_t length
 );
 
 /**
  * @brief Get the state machine from a mock protocol context
  * 
  * @param proto_ctx The protocol context
  * @return The state machine, or NULL if not available
  */
 polycall_state_machine_t* mock_protocol_get_state_machine(
     polycall_protocol_context_t* proto_ctx
 );
 
 /**
  * @brief Add a message handler to the mock protocol context
  * 
  * @param core_ctx The core context
  * @param proto_ctx The protocol context
  * @param command_name Command name
  * @param handler Handler function
  * @param user_data User data for handler
  * @return true if successful, false otherwise
  */
 bool mock_protocol_register_message_handler(
     polycall_core_context_t* core_ctx,
     polycall_protocol_context_t* proto_ctx,
     const char* command_name,
     void* handler,
     void* user_data
 );
 
 #endif /* MOCK_PROTOCOL_CONTEXT_H */