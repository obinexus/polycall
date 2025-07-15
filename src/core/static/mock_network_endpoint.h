/**
 * @file mock_network_endpoint.h
 * @brief Mock implementation of network endpoint for testing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides mock implementations of the network endpoint functionality
 * for use in unit tests of protocol enhancements.
 */

#ifndef MOCK_NETWORK_ENDPOINT_H
#define MOCK_NETWORK_ENDPOINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Mock network packet structure
 */
typedef struct {
  void *data;     /* Packet data */
  size_t size;    /* Packet size */
  uint32_t flags; /* Packet flags */
} NetworkPacket;

/**
 * @brief Mock network endpoint structure (opaque)
 */
typedef struct NetworkEndpoint NetworkEndpoint;

/**
 * @brief Create a mock network endpoint for testing
 *
 * @return A mock network endpoint instance
 */
NetworkEndpoint *mock_network_endpoint_create(void);

/**
 * @brief Destroy a mock network endpoint
 *
 * @param endpoint The mock endpoint to destroy
 */
void mock_network_endpoint_destroy(NetworkEndpoint *endpoint);

/**
 * @brief Mock implementation of sending a packet
 *
 * @param endpoint The mock endpoint
 * @param packet The packet to send
 * @param flags Send flags
 * @return true if successful, false otherwise
 */
bool mock_network_endpoint_send(NetworkEndpoint *endpoint,
                                const NetworkPacket *packet, uint32_t flags);

/**
 * @brief Mock implementation of receiving a packet
 *
 * @param endpoint The mock endpoint
 * @param packet Pointer to store received packet
 * @param flags Receive flags
 * @return true if successful, false otherwise
 */
bool mock_network_endpoint_receive(NetworkEndpoint *endpoint,
                                   NetworkPacket *packet, uint32_t flags);

/**
 * @brief Get the number of packets in the mock endpoint queue
 *
 * @param endpoint The mock endpoint
 * @return Number of packets
 */
uint32_t mock_network_endpoint_get_queue_size(NetworkEndpoint *endpoint);

/**
 * @brief Add a test packet to the mock endpoint queue
 *
 * @param endpoint The mock endpoint
 * @param data Packet data
 * @param size Packet size
 * @param flags Packet flags
 * @return true if successful, false otherwise
 */
bool mock_network_endpoint_add_test_packet(NetworkEndpoint *endpoint,
                                           const void *data, size_t size,
                                           uint32_t flags);

#endif /* MOCK_NETWORK_ENDPOINT_H */