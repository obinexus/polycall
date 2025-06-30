/**
 * @file mock_network_endpoint.c
 * @brief Mock implementation of network endpoint for testing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides mock implementations of the network endpoint functionality
 * for use in unit tests of protocol enhancements.
 */

 #include "mock_network_endpoint.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 #define MOCK_ENDPOINT_MAGIC 0x4D4E4550 /* "MNEP" */
 #define MOCK_ENDPOINT_MAX_QUEUE 32
 
 /**
  * @brief Mock network endpoint structure
  */
 struct NetworkEndpoint {
     uint32_t magic;                      /* Magic number for validation */
     
     /* Send queue */
     struct {
         NetworkPacket packets[MOCK_ENDPOINT_MAX_QUEUE];
         uint32_t count;
     } send_queue;
     
     /* Receive queue */
     struct {
         NetworkPacket packets[MOCK_ENDPOINT_MAX_QUEUE];
         uint32_t count;
     } recv_queue;
     
     /* Statistics */
     struct {
         uint32_t packets_sent;
         uint32_t packets_received;
         uint32_t bytes_sent;
         uint32_t bytes_received;
         uint32_t errors;
     } stats;
 };
 
 /**
  * @brief Create a mock network endpoint for testing
  */
 NetworkEndpoint* mock_network_endpoint_create(void) {
     /* Allocate endpoint */
     NetworkEndpoint* endpoint = malloc(sizeof(NetworkEndpoint));
     if (!endpoint) {
         return NULL;
     }
     
     /* Initialize endpoint */
     memset(endpoint, 0, sizeof(NetworkEndpoint));
     endpoint->magic = MOCK_ENDPOINT_MAGIC;
     
     return endpoint;
 }
 
 /**
  * @brief Destroy a mock network endpoint
  */
 void mock_network_endpoint_destroy(NetworkEndpoint* endpoint) {
     if (!endpoint || endpoint->magic != MOCK_ENDPOINT_MAGIC) {
         return;
     }
     
     /* Free any remaining packets in queues */
     for (uint32_t i = 0; i < endpoint->send_queue.count; i++) {
         free(endpoint->send_queue.packets[i].data);
     }
     
     for (uint32_t i = 0; i < endpoint->recv_queue.count; i++) {
         free(endpoint->recv_queue.packets[i].data);
     }
     
     /* Clear magic and free */
     endpoint->magic = 0;
     free(endpoint);
 }
 
 /**
  * @brief Mock implementation of sending a packet
  */
 bool mock_network_endpoint_send(NetworkEndpoint* endpoint, const NetworkPacket* packet, uint32_t flags) {
     if (!endpoint || endpoint->magic != MOCK_ENDPOINT_MAGIC || !packet || !packet->data) {
         return false;
     }
     
     /* Check if send queue is full */
     if (endpoint->send_queue.count >= MOCK_ENDPOINT_MAX_QUEUE) {
         endpoint->stats.errors++;
         return false;
     }
     
     /* Allocate memory for packet data */
     void* data_copy = malloc(packet->size);
     if (!data_copy) {
         endpoint->stats.errors++;
         return false;
     }
     
     /* Copy packet data */
     memcpy(data_copy, packet->data, packet->size);
     
     /* Add to send queue */
     NetworkPacket* new_packet = &endpoint->send_queue.packets[endpoint->send_queue.count++];
     new_packet->data = data_copy;
     new_packet->size = packet->size;
     new_packet->flags = packet->flags | flags;
     
     /* Update statistics */
     endpoint->stats.packets_sent++;
     endpoint->stats.bytes_sent += packet->size;
     
     return true;
 }
 
 /**
  * @brief Mock implementation of receiving a packet
  */
 bool mock_network_endpoint_receive(NetworkEndpoint* endpoint, NetworkPacket* packet, uint32_t flags) {
     if (!endpoint || endpoint->magic != MOCK_ENDPOINT_MAGIC || !packet) {
         return false;
     }
     
     /* Check if receive queue is empty */
     if (endpoint->recv_queue.count == 0) {
         return false;
     }
     
     /* Get packet from front of queue */
     NetworkPacket* src_packet = &endpoint->recv_queue.packets[0];
     
     /* Copy packet data */
     packet->data = src_packet->data;
     packet->size = src_packet->size;
     packet->flags = src_packet->flags;
     
     /* Remove packet from queue by shifting remaining packets */
     for (uint32_t i = 0; i < endpoint->recv_queue.count - 1; i++) {
         endpoint->recv_queue.packets[i] = endpoint->recv_queue.packets[i + 1];
     }
     
     /* Clear last packet and decrement count */
     memset(&endpoint->recv_queue.packets[endpoint->recv_queue.count - 1], 0, sizeof(NetworkPacket));
     endpoint->recv_queue.count--;
     
     /* Update statistics */
     endpoint->stats.packets_received++;
     endpoint->stats.bytes_received += packet->size;
     
     return true;
 }
 
 /**
  * @brief Get the number of packets in the mock endpoint queue
  */
 uint32_t mock_network_endpoint_get_queue_size(NetworkEndpoint* endpoint) {
     if (!endpoint || endpoint->magic != MOCK_ENDPOINT_MAGIC) {
         return 0;
     }
     
     return endpoint->recv_queue.count;
 }
 
 /**
  * @brief Add a test packet to the mock endpoint queue
  */
 bool mock_network_endpoint_add_test_packet(NetworkEndpoint* endpoint, const void* data, size_t size, uint32_t flags) {
     if (!endpoint || endpoint->magic != MOCK_ENDPOINT_MAGIC || !data || size == 0) {
         return false;
     }
     
     /* Check if receive queue is full */
     if (endpoint->recv_queue.count >= MOCK_ENDPOINT_MAX_QUEUE) {
         return false;
     }
     
     /* Allocate memory for packet data */
     void* data_copy = malloc(size);
     if (!data_copy) {
         return false;
     }
     
     /* Copy packet data */
     memcpy(data_copy, data, size);
     
     /* Add to receive queue */
     NetworkPacket* new_packet = &endpoint->recv_queue.packets[endpoint->recv_queue.count++];
     new_packet->data = data_copy;
     new_packet->size = size;
     new_packet->flags = flags;
     
     return true;
 }