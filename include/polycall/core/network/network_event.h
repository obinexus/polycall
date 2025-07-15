#ifndef POLYCALL_NETWORK_NETWORK_EVENT_H_H
#include <stddef.h>

#define POLYCALL_NETWORK_NETWORK_EVENT_H_H

#ifdef __cplusplus
extern "C" {
#endif

// Network event types
typedef enum polycall_network_event_type {
  POLYCALL_NET_EVENT_NONE = 0,
  POLYCALL_NET_EVENT_CONNECT,
  POLYCALL_NET_EVENT_DISCONNECT,
  POLYCALL_NET_EVENT_DATA_RECEIVED,
  POLYCALL_NET_EVENT_ERROR,
  POLYCALL_NET_EVENT_TIMEOUT
} polycall_network_event_type_t;

// Network event data structure
typedef struct polycall_network_event {
  polycall_network_event_type_t type;
  void *data;
  size_t data_size;
  int connection_id;
} polycall_network_event_t;

// Event handling functions
void polycall_network_event_init(polycall_network_event_t *event);
void polycall_network_event_cleanup(polycall_network_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_NETWORK_EVENT_H_H */