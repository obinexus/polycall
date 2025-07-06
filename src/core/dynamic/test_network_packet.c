/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file test_network_packet.c
 * @brief Unit tests for the network packet module of LibPolyCall
 * @author Unit tests for LibPolyCall
 *
 * This file contains unit tests for the packet handling interface in
 * LibPolyCall.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "polycall/core/network/network_packet.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_memory.h"

// Helper function to create a core context for testing
static polycall_core_context_t *create_test_core_context(void) {
  polycall_core_context_t *ctx = malloc(sizeof(polycall_core_context_t));
  assert(ctx != NULL);
  memset(ctx, 0, sizeof(polycall_core_context_t));

  // Initialize memory functions
  ctx->memory_allocate = malloc;
  ctx->memory_free = free;
  ctx->memory_realloc = realloc;

  return ctx;
}

// Test data for packet tests
static const char *test_data = "Hello, this is test data for packet tests!";
static const size_t test_data_size =
    43; // Length of test_data including null terminator

void test_packet_create_destroy(void) {
  printf("Testing packet_create and packet_destroy functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Test packet_create with NULL parameters
  polycall_network_packet_t *packet = NULL;
  polycall_core_error_t result =
      polycall_network_packet_create(NULL, &packet, 1024);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_create(core_ctx, NULL, 1024);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_packet_create(core_ctx, &packet, 1024);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(packet != NULL);

  // Verify packet properties
  assert(packet->buffer_capacity == 1024);
  assert(packet->data_size == 0);
  assert(packet->data != NULL);
  assert(packet->owns_data == true);

  // Test with default capacity
  polycall_network_packet_t *packet2 = NULL;
  result = polycall_network_packet_create(core_ctx, &packet2, 0);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(packet2 != NULL);
  assert(packet2->buffer_capacity > 0); // Should have used default capacity

  // Test packet_destroy with NULL parameters
  polycall_network_packet_destroy(NULL, packet);   // Should not crash
  polycall_network_packet_destroy(core_ctx, NULL); // Should not crash

  // Test with valid parameters
  polycall_network_packet_destroy(core_ctx, packet);
  polycall_network_packet_destroy(core_ctx, packet2);

  // Clean up
  free(core_ctx);

  printf("packet_create and packet_destroy tests passed!\n");
}

void test_packet_create_from_data(void) {
  printf("Testing packet_create_from_data function...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Test with NULL parameters
  polycall_network_packet_t *packet = NULL;
  polycall_core_error_t result = polycall_network_packet_create_from_data(
      NULL, &packet, (void *)test_data, test_data_size, false);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_create_from_data(
      core_ctx, NULL, (void *)test_data, test_data_size, false);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_create_from_data(core_ctx, &packet, NULL,
                                                    test_data_size, false);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_create_from_data(
      core_ctx, &packet, (void *)test_data, 0, false);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters - copy data
  result = polycall_network_packet_create_from_data(
      core_ctx, &packet, (void *)test_data, test_data_size, false);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(packet != NULL);

  // Verify packet properties
  assert(packet->buffer_capacity >= test_data_size);
  assert(packet->data_size == test_data_size);
  assert(packet->data != NULL);
  assert(packet->data != test_data); // Should be a copy
  assert(packet->owns_data == true);
  assert(memcmp(packet->data, test_data, test_data_size) == 0);

  // Clean up packet
  polycall_network_packet_destroy(core_ctx, packet);

  // Test with take ownership = true (need to make a copy of test_data for this)
  void *data_copy = malloc(test_data_size);
  assert(data_copy != NULL);
  memcpy(data_copy, test_data, test_data_size);

  result = polycall_network_packet_create_from_data(
      core_ctx, &packet, data_copy, test_data_size, true);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(packet != NULL);

  // Verify packet properties
  assert(packet->buffer_capacity >= test_data_size);
  assert(packet->data_size == test_data_size);
  assert(packet->data == data_copy); // Should be the same pointer
  assert(packet->owns_data == true);
  assert(memcmp(packet->data, test_data, test_data_size) == 0);

  // Clean up packet (which should also free data_copy)
  polycall_network_packet_destroy(core_ctx, packet);

  // Clean up
  free(core_ctx);

  printf("packet_create_from_data tests passed!\n");
}

void test_packet_get_set_data(void) {
  printf("Testing packet_get_data and packet_set_data functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_data with NULL parameters
  void *data = NULL;
  size_t size = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_data(NULL, packet, &data, &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_data(core_ctx, NULL, &data, &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_data(core_ctx, packet, NULL, &size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_data(core_ctx, packet, &data, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_data with valid parameters (empty packet)
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(data == packet->data);
  assert(size == 0);

  // Test set_data with NULL parameters
  result = polycall_network_packet_set_data(NULL, packet, (void *)test_data,
                                            test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_data(core_ctx, NULL, (void *)test_data,
                                            test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_packet_set_data(core_ctx, packet, NULL, test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_packet_set_data(core_ctx, packet, (void *)test_data, 0);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_data with valid parameters
  result = polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                            test_data_size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify data was set correctly
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(data == packet->data);
  assert(size == test_data_size);
  assert(memcmp(data, test_data, test_data_size) == 0);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_get_data and packet_set_data tests passed!\n");
}

void test_packet_append_data(void) {
  printf("Testing packet_append_data function...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet,
                                 32); // Smaller than test data to test resizing

  // Test with NULL parameters
  polycall_core_error_t result = polycall_network_packet_append_data(
      NULL, packet, (void *)test_data, test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_append_data(
      core_ctx, NULL, (void *)test_data, test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_append_data(core_ctx, packet, NULL,
                                               test_data_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_append_data(core_ctx, packet,
                                               (void *)test_data, 0);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_packet_append_data(
      core_ctx, packet, (void *)test_data, test_data_size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify data was appended correctly
  void *data = NULL;
  size_t size = 0;
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(size == test_data_size);
  assert(memcmp(data, test_data, test_data_size) == 0);

  // Append more data
  const char *more_data = "More test data!";
  size_t more_data_size = strlen(more_data) + 1;

  result = polycall_network_packet_append_data(
      core_ctx, packet, (void *)more_data, more_data_size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify both data pieces were appended correctly
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(size == test_data_size + more_data_size);
  assert(memcmp(data, test_data, test_data_size) == 0);
  assert(memcmp((char *)data + test_data_size, more_data, more_data_size) == 0);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_append_data tests passed!\n");
}

void test_packet_clear(void) {
  printf("Testing packet_clear function...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet and set data
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);
  polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                   test_data_size);

  // Test with NULL parameters
  polycall_core_error_t result = polycall_network_packet_clear(NULL, packet);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_clear(core_ctx, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Verify data exists before clearing
  void *data = NULL;
  size_t size = 0;
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(size == test_data_size);

  // Test with valid parameters
  result = polycall_network_packet_clear(core_ctx, packet);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify data was cleared
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(size == 0);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_clear tests passed!\n");
}

void test_packet_flags(void) {
  printf("Testing packet flags functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_flags with NULL parameters
  polycall_packet_flags_t flags = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_flags(NULL, packet, &flags);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_flags(core_ctx, NULL, &flags);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_flags(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_flags with valid parameters
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags == 0); // Default should be 0

  // Test set_flags with NULL parameters
  flags = POLYCALL_PACKET_FLAG_ENCRYPTED | POLYCALL_PACKET_FLAG_COMPRESSED;
  result = polycall_network_packet_set_flags(NULL, packet, flags);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_flags(core_ctx, NULL, flags);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_flags with valid parameters
  result = polycall_network_packet_set_flags(core_ctx, packet, flags);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify flags were set
  polycall_packet_flags_t retrieved_flags = 0;
  result =
      polycall_network_packet_get_flags(core_ctx, packet, &retrieved_flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(retrieved_flags == flags);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet flags tests passed!\n");
}

void test_packet_id(void) {
  printf("Testing packet_get_id and packet_set_id functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_id with NULL parameters
  uint32_t id = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_id(NULL, packet, &id);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_id(core_ctx, NULL, &id);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_id(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_id with valid parameters
  result = polycall_network_packet_get_id(core_ctx, packet, &id);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Test set_id with NULL parameters
  uint32_t new_id = 12345;
  result = polycall_network_packet_set_id(NULL, packet, new_id);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_id(core_ctx, NULL, new_id);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_id with valid parameters
  result = polycall_network_packet_set_id(core_ctx, packet, new_id);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify ID was set
  id = 0;
  result = polycall_network_packet_get_id(core_ctx, packet, &id);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(id == new_id);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_get_id and packet_set_id tests passed!\n");
}

void test_packet_sequence(void) {
  printf("Testing packet_get_sequence and packet_set_sequence functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_sequence with NULL parameters
  uint32_t sequence = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_sequence(NULL, packet, &sequence);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_sequence(core_ctx, NULL, &sequence);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_sequence(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_sequence with valid parameters
  result = polycall_network_packet_get_sequence(core_ctx, packet, &sequence);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Test set_sequence with NULL parameters
  uint32_t new_sequence = 54321;
  result = polycall_network_packet_set_sequence(NULL, packet, new_sequence);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_sequence(core_ctx, NULL, new_sequence);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_sequence with valid parameters
  result = polycall_network_packet_set_sequence(core_ctx, packet, new_sequence);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify sequence was set
  sequence = 0;
  result = polycall_network_packet_get_sequence(core_ctx, packet, &sequence);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(sequence == new_sequence);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_get_sequence and packet_set_sequence tests passed!\n");
}

void test_packet_type(void) {
  printf("Testing packet_get_type and packet_set_type functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_type with NULL parameters
  uint16_t type = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_type(NULL, packet, &type);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_type(core_ctx, NULL, &type);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_type(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_type with valid parameters
  result = polycall_network_packet_get_type(core_ctx, packet, &type);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Test set_type with NULL parameters
  uint16_t new_type = 42;
  result = polycall_network_packet_set_type(NULL, packet, new_type);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_type(core_ctx, NULL, new_type);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_type with valid parameters
  result = polycall_network_packet_set_type(core_ctx, packet, new_type);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify type was set
  type = 0;
  result = polycall_network_packet_get_type(core_ctx, packet, &type);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(type == new_type);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_get_type and packet_set_type tests passed!\n");
}

void test_packet_clone(void) {
  printf("Testing packet_clone function...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet and set up data and properties
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);
  polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                   test_data_size);
  polycall_network_packet_set_type(core_ctx, packet, 42);
  polycall_network_packet_set_id(core_ctx, packet, 12345);
  polycall_network_packet_set_sequence(core_ctx, packet, 54321);
  polycall_network_packet_set_flags(core_ctx, packet,
                                    POLYCALL_PACKET_FLAG_ENCRYPTED |
                                        POLYCALL_PACKET_FLAG_COMPRESSED);

  // Test with NULL parameters
  polycall_network_packet_t *clone = NULL;
  polycall_core_error_t result =
      polycall_network_packet_clone(NULL, packet, &clone);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_clone(core_ctx, NULL, &clone);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_clone(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test with valid parameters
  result = polycall_network_packet_clone(core_ctx, packet, &clone);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone != NULL);

  // Verify clone properties
  void *clone_data = NULL;
  size_t clone_size = 0;
  result = polycall_network_packet_get_data(core_ctx, clone, &clone_data,
                                            &clone_size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone_size == test_data_size);
  assert(memcmp(clone_data, test_data, test_data_size) == 0);

  uint16_t clone_type = 0;
  result = polycall_network_packet_get_type(core_ctx, clone, &clone_type);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone_type == 42);

  uint32_t clone_id = 0;
  result = polycall_network_packet_get_id(core_ctx, clone, &clone_id);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone_id == 12345);

  uint32_t clone_sequence = 0;
  result =
      polycall_network_packet_get_sequence(core_ctx, clone, &clone_sequence);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone_sequence == 54321);

  polycall_packet_flags_t clone_flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, clone, &clone_flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(clone_flags ==
         (POLYCALL_PACKET_FLAG_ENCRYPTED | POLYCALL_PACKET_FLAG_COMPRESSED));

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  polycall_network_packet_destroy(core_ctx, clone);
  free(core_ctx);

  printf("packet_clone tests passed!\n");
}

void test_packet_compression(void) {
  printf("Testing packet compression functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet and set up data
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);
  polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                   test_data_size);

  // Test compress with NULL parameters
  polycall_core_error_t result = polycall_network_packet_compress(NULL, packet);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_compress(core_ctx, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test compress with valid parameters
  result = polycall_network_packet_compress(core_ctx, packet);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify compression flag is set
  polycall_packet_flags_t flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags & POLYCALL_PACKET_FLAG_COMPRESSED);

  // Test decompress with NULL parameters
  result = polycall_network_packet_decompress(NULL, packet);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_decompress(core_ctx, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test decompress with valid parameters
  result = polycall_network_packet_decompress(core_ctx, packet);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify compression flag is cleared
  flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(!(flags & POLYCALL_PACKET_FLAG_COMPRESSED));

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet compression tests passed!\n");
}

void test_packet_encryption(void) {
  printf("Testing packet encryption functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet and set up data
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);
  polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                   test_data_size);

  // Test encrypt with NULL parameters
  const char *key = "test encryption key";
  size_t key_size = strlen(key);

  polycall_core_error_t result =
      polycall_network_packet_encrypt(NULL, packet, key, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_encrypt(core_ctx, NULL, key, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_encrypt(core_ctx, packet, NULL, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_encrypt(core_ctx, packet, key, 0);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test encrypt with valid parameters
  result = polycall_network_packet_encrypt(core_ctx, packet, key, key_size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify encryption flag is set
  polycall_packet_flags_t flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags & POLYCALL_PACKET_FLAG_ENCRYPTED);

  // Test decrypt with NULL parameters
  result = polycall_network_packet_decrypt(NULL, packet, key, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_decrypt(core_ctx, NULL, key, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_decrypt(core_ctx, packet, NULL, key_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_decrypt(core_ctx, packet, key, 0);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test decrypt with valid parameters
  result = polycall_network_packet_decrypt(core_ctx, packet, key, key_size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify encryption flag is cleared
  flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(!(flags & POLYCALL_PACKET_FLAG_ENCRYPTED));

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet encryption tests passed!\n");
}

void test_packet_metadata(void) {
  printf("Testing packet metadata functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test set_metadata with NULL parameters
  const char *key = "test_key";
  int value = 12345;
  polycall_core_error_t result = polycall_network_packet_set_metadata(
      NULL, packet, key, &value, sizeof(value));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_metadata(core_ctx, NULL, key, &value,
                                                sizeof(value));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_metadata(core_ctx, packet, NULL, &value,
                                                sizeof(value));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_metadata(core_ctx, packet, key, NULL,
                                                sizeof(value));
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_packet_set_metadata(core_ctx, packet, key, &value, 0);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_metadata with valid parameters
  result = polycall_network_packet_set_metadata(core_ctx, packet, key, &value,
                                                sizeof(value));
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify metadata flag is set
  polycall_packet_flags_t flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags & POLYCALL_PACKET_FLAG_METADATA);

  // Test get_metadata with NULL parameters
  void *retrieved_value = NULL;
  size_t retrieved_size = 0;
  result = polycall_network_packet_get_metadata(
      NULL, packet, key, &retrieved_value, &retrieved_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_metadata(
      core_ctx, NULL, key, &retrieved_value, &retrieved_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_metadata(
      core_ctx, packet, NULL, &retrieved_value, &retrieved_size);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_metadata(core_ctx, packet, key, NULL,
                                                &retrieved_size);
  assert(result == POLYCALL_CORE_SUCCESS); // Valid to get size only

  result = polycall_network_packet_get_metadata(core_ctx, packet, key,
                                                &retrieved_value, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_metadata with valid parameters
  int retrieved_int = 0;
  retrieved_size = sizeof(retrieved_int);
  result = polycall_network_packet_get_metadata(
      core_ctx, packet, key, &retrieved_int, &retrieved_size);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(retrieved_size == sizeof(int));
  assert(retrieved_int == value);

  // Test get_metadata with non-existent key
  result = polycall_network_packet_get_metadata(
      core_ctx, packet, "non_existent_key", &retrieved_value, &retrieved_size);
  assert(result == POLYCALL_CORE_ERROR_NOT_FOUND);

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet metadata tests passed!\n");
}

void test_packet_checksum(void) {
  printf("Testing packet checksum functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet and set data
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);
  polycall_network_packet_set_data(core_ctx, packet, (void *)test_data,
                                   test_data_size);

  // Test calculate_checksum with NULL parameters
  uint32_t checksum = 0;
  polycall_core_error_t result =
      polycall_network_packet_calculate_checksum(NULL, packet, &checksum);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result =
      polycall_network_packet_calculate_checksum(core_ctx, NULL, &checksum);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_calculate_checksum(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test calculate_checksum with valid parameters
  result =
      polycall_network_packet_calculate_checksum(core_ctx, packet, &checksum);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(checksum != 0); // Should have some non-zero value

  // Test verify_checksum with NULL parameters
  bool valid = false;
  result = polycall_network_packet_verify_checksum(NULL, packet, &valid);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_verify_checksum(core_ctx, NULL, &valid);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_verify_checksum(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test verify_checksum with valid parameters
  result = polycall_network_packet_verify_checksum(core_ctx, packet, &valid);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(valid == true); // Checksum should be valid

  // Modify data and verify checksum is invalid
  // We need to do this in a way that maintains data size but changes the
  // content
  void *data = NULL;
  size_t size = 0;
  result = polycall_network_packet_get_data(core_ctx, packet, &data, &size);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Make a modified copy of the data
  char *modified_data = malloc(size);
  assert(modified_data != NULL);
  memcpy(modified_data, data, size);
  modified_data[0] = ~modified_data[0]; // Flip bits in first byte

  // Set the modified data
  polycall_network_packet_set_data(core_ctx, packet, modified_data, size);
  free(modified_data);

  // Verify checksum is now invalid (without recalculating it)
  valid = true;
  result = polycall_network_packet_verify_checksum(core_ctx, packet, &valid);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(valid == false); // Checksum should be invalid

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet checksum tests passed!\n");
}

void test_packet_priority(void) {
  printf("Testing packet_get_priority and packet_set_priority functions...\n");

  // Initialize core context
  polycall_core_context_t *core_ctx = create_test_core_context();

  // Create packet
  polycall_network_packet_t *packet = NULL;
  polycall_network_packet_create(core_ctx, &packet, 1024);

  // Test get_priority with NULL parameters
  uint8_t priority = 0;
  polycall_core_error_t result =
      polycall_network_packet_get_priority(NULL, packet, &priority);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_priority(core_ctx, NULL, &priority);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_get_priority(core_ctx, packet, NULL);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test get_priority with valid parameters
  result = polycall_network_packet_get_priority(core_ctx, packet, &priority);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Test set_priority with NULL parameters
  uint8_t new_priority = 200; // High priority
  result = polycall_network_packet_set_priority(NULL, packet, new_priority);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  result = polycall_network_packet_set_priority(core_ctx, NULL, new_priority);
  assert(result == POLYCALL_CORE_ERROR_INVALID_PARAMETERS);

  // Test set_priority with valid parameters - high priority
  result = polycall_network_packet_set_priority(core_ctx, packet, new_priority);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify priority was set
  priority = 0;
  result = polycall_network_packet_get_priority(core_ctx, packet, &priority);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(priority == new_priority);

  // Verify priority high flag was set
  polycall_packet_flags_t flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags & POLYCALL_PACKET_FLAG_PRIORITY_HIGH);

  // Test set_priority with low priority
  new_priority = 50; // Low priority
  result = polycall_network_packet_set_priority(core_ctx, packet, new_priority);
  assert(result == POLYCALL_CORE_SUCCESS);

  // Verify priority was set
  priority = 0;
  result = polycall_network_packet_get_priority(core_ctx, packet, &priority);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(priority == new_priority);

  // Verify priority low flag was set and high flag was cleared
  flags = 0;
  result = polycall_network_packet_get_flags(core_ctx, packet, &flags);
  assert(result == POLYCALL_CORE_SUCCESS);
  assert(flags & POLYCALL_PACKET_FLAG_PRIORITY_LOW);
  assert(!(flags & POLYCALL_PACKET_FLAG_PRIORITY_HIGH));

  // Clean up
  polycall_network_packet_destroy(core_ctx, packet);
  free(core_ctx);

  printf("packet_get_priority and packet_set_priority tests passed!\n");
}

int main(void) {
  printf("Running network packet module unit tests...\n");

  // Run all test functions
  test_packet_create_destroy();
  test_packet_create_from_data();
  test_packet_get_set_data();
  test_packet_append_data();
  test_packet_clear();
  test_packet_flags();
  test_packet_id();
  test_packet_sequence();
  test_packet_timestamp();
  test_packet_type();
  test_packet_clone();
  test_packet_compression();
  test_packet_encryption();
  test_packet_metadata();
  test_packet_checksum();
  test_packet_priority();

  printf("All network packet module tests passed!\n");
  return 0;
}
