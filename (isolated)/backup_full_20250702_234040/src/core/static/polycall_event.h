/**
 * @file polycall_event.h
 * @brief Event handling definitions and declarations for the Polycall system
 */

 #ifndef POLYCALL_POLYCALL_POLYCALL_EVENT_H_H
 #define POLYCALL_POLYCALL_POLYCALL_EVENT_H_H
 
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
    #include <errno.h>
 #include "polycall/core/polycall/polycall_core.h"
    #include "polycall/core/polycall/polycall_error.h" 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /** Task routing event types */
 typedef enum polycall_routing_event {
     ROUTING_EVENT_TASK_INITIATED = 0,
     ROUTING_EVENT_NODE_SELECTED = 1,
     ROUTING_EVENT_TASK_DISPATCHED = 2,
     ROUTING_EVENT_TASK_COMPLETED = 3,
     ROUTING_EVENT_ROUTING_FAILED = 4,
     ROUTING_EVENT_NODE_FAILURE = 5
 } polycall_routing_event_t;
  /**
  * @brief Memory subsystem error codes
  */
 typedef enum {
    POLYCALL_MEMORY_SUCCESS = 0,
    POLYCALL_MEMORY_ERROR_ALLOCATION_FAILED,
    POLYCALL_MEMORY_ERROR_INVALID_ADDRESS,
    POLYCALL_MEMORY_ERROR_OUT_OF_BOUNDS,
    POLYCALL_MEMORY_ERROR_ALIGNMENT,
    POLYCALL_MEMORY_ERROR_DOUBLE_FREE,
    POLYCALL_MEMORY_ERROR_LEAK_DETECTED,
    POLYCALL_MEMORY_ERROR_POOL_EXHAUSTED,
    POLYCALL_MEMORY_ERROR_INVALID_SIZE,
} polycall_memory_error_t;

 /** Event data structure */
 typedef struct polycall_event {
     polycall_routing_event_t type;
     unsigned int task_id;
     unsigned int node_id;
     unsigned int timestamp;
     void* payload;
     size_t payload_size;
 } polycall_event_t;
 
 /** Event handler function pointer type */
 typedef void (*polycall_event_handler_t)(polycall_event_t* event, void* user_data);
 
 /**
  * Initialize the event subsystem
  * @return 0 on success, error code otherwise
  */
 int polycall_event_init(void);
 
 /**
  * Clean up the event subsystem resources
  */
 void polycall_event_cleanup(void);
 
 /**
  * Trigger a routing event
  * @param event_type The type of routing event
  * @param task_id ID of the task
  * @param node_id ID of the node
  * @param payload Additional event data
  * @param payload_size Size of the payload
  * @return 0 on success, error code otherwise
  */
 int polycall_trigger_event(polycall_routing_event_t event_type, 
                           unsigned int task_id, 
                           unsigned int node_id, 
                           void* payload, 
                           size_t payload_size);
 
 /**
  * Register an event handler for a specific event type
  * @param event_type The type of routing event
  * @param handler Function pointer to the event handler
  * @param user_data User data to pass to the handler when called
  * @return Handler ID on success, negative value on error
  */
 int polycall_register_event_handler(polycall_routing_event_t event_type,
                                    polycall_event_handler_t handler,
                                    void* user_data);
 
 /**
  * Unregister an event handler
  * @param handler_id The ID of the handler to unregister
  * @return 0 on success, error code otherwise
  */
 int polycall_unregister_event_handler(int handler_id);
 
 /**
  * Process pending events (can be called in a loop or periodically)
  * @param timeout_ms Maximum time to wait for events (0 for non-blocking)
  * @return Number of events processed, or negative value on error
  */
 int polycall_process_events(unsigned int timeout_ms);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_POLYCALL_POLYCALL_EVENT_H_H */