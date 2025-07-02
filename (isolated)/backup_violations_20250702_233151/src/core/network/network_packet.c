/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/network/network_packet.h"

 * @file packet.c
 * @brief Network packet implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the packet interface for LibPolyCall networking,
 * providing data containers for network communication.
 */

 #include "polycall/core/network/network_packet.h"

 
 
 // Helper function to calculate CRC32 checksum
 static uint32_t calculate_crc32(const void* data, size_t size) {
     const uint8_t* buf = (const uint8_t*)data;
     uint32_t crc = 0xFFFFFFFF;
     
     for (size_t i = 0; i < size; i++) {
         crc ^= buf[i];
         for (int j = 0; j < 8; j++) {
             crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
         }
     }
     
     return ~crc;
 }
 
 polycall_core_error_t polycall_network_packet_create(
     polycall_core_context_t* ctx,
     polycall_network_packet_t** packet,
     size_t initial_capacity
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Use default capacity if not specified
     if (initial_capacity == 0) {
         initial_capacity = DEFAULT_PACKET_CAPACITY;
     }
     
     // Allocate packet structure
     polycall_network_packet_t* new_packet = polycall_core_malloc(ctx, sizeof(polycall_network_packet_t));
     if (!new_packet) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize packet structure
     memset(new_packet, 0, sizeof(polycall_network_packet_t));
     
     // Allocate data buffer
     new_packet->data = polycall_core_malloc(ctx, initial_capacity);
     if (!new_packet->data) {
         polycall_core_free(ctx, new_packet);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize fields
     new_packet->buffer_capacity = initial_capacity;
     new_packet->data_size = 0;
     new_packet->owns_data = true;
     new_packet->timestamp = (uint64_t)time(NULL);
     
     *packet = new_packet;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_create_from_data(
     polycall_core_context_t* ctx,
     polycall_network_packet_t** packet,
     void* data,
     size_t size,
     bool take_ownership
 ) {
     if (!ctx || !packet || !data || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate packet structure
     polycall_network_packet_t* new_packet = polycall_core_malloc(ctx, sizeof(polycall_network_packet_t));
     if (!new_packet) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize packet structure
     memset(new_packet, 0, sizeof(polycall_network_packet_t));
     
     if (take_ownership) {
         // Just take the provided buffer
         new_packet->data = data;
         new_packet->owns_data = true;
     } else {
         // Allocate and copy data
         new_packet->data = polycall_core_malloc(ctx, size);
         if (!new_packet->data) {
             polycall_core_free(ctx, new_packet);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_packet->data, data, size);
         new_packet->owns_data = true;
     }
     
     new_packet->buffer_capacity = size;
     new_packet->data_size = size;
     new_packet->timestamp = (uint64_t)time(NULL);
     
     // Calculate initial checksum
     new_packet->checksum = calculate_crc32(new_packet->data, new_packet->data_size);
     
     *packet = new_packet;
     return POLYCALL_CORE_SUCCESS;
 }
 
 void polycall_network_packet_destroy(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet
 ) {
     if (!ctx || !packet) {
         return;
     }
     
     // Free data buffer if we own it
     if (packet->owns_data && packet->data) {
         polycall_core_free(ctx, packet->data);
         packet->data = NULL;
     }
     
     // Free metadata values
     for (size_t i = 0; i < packet->metadata_count; i++) {
         if (packet->metadata[i].value) {
             polycall_core_free(ctx, packet->metadata[i].value);
         }
     }
     
     // Free packet structure
     polycall_core_free(ctx, packet);
 }
 
 polycall_core_error_t polycall_network_packet_get_data(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     void** data,
     size_t* size
 ) {
     if (!ctx || !packet || !data || !size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *data = packet->data;
     *size = packet->data_size;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_data(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const void* data,
     size_t size
 ) {
     if (!ctx || !packet || !data || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // If our current buffer is too small, reallocate
     if (size > packet->buffer_capacity) {
         void* new_buffer = polycall_core_malloc(ctx, size);
         if (!new_buffer) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Free old buffer if we own it
         if (packet->owns_data && packet->data) {
             polycall_core_free(ctx, packet->data);
         }
         
         packet->data = new_buffer;
         packet->buffer_capacity = size;
         packet->owns_data = true;
     }
     
     // Copy new data
     memcpy(packet->data, data, size);
     packet->data_size = size;
     
     // Update checksum
     packet->checksum = calculate_crc32(packet->data, packet->data_size);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_append_data(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const void* data,
     size_t size
 ) {
     if (!ctx || !packet || !data || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Calculate new size
     size_t new_size = packet->data_size + size;
     
     // If our current buffer is too small, reallocate
     if (new_size > packet->buffer_capacity) {
         // Grow buffer by at least 50% to avoid frequent reallocations
         size_t new_capacity = packet->buffer_capacity * 3 / 2;
         if (new_capacity < new_size) {
             new_capacity = new_size;
         }
         
         void* new_buffer = polycall_core_malloc(ctx, new_capacity);
         if (!new_buffer) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing data
         if (packet->data && packet->data_size > 0) {
             memcpy(new_buffer, packet->data, packet->data_size);
         }
         
         // Free old buffer if we own it
         if (packet->owns_data && packet->data) {
             polycall_core_free(ctx, packet->data);
         }
         
         packet->data = new_buffer;
         packet->buffer_capacity = new_capacity;
         packet->owns_data = true;
     }
     
     // Append new data
     memcpy((uint8_t*)packet->data + packet->data_size, data, size);
     packet->data_size = new_size;
     
     // Update checksum
     packet->checksum = calculate_crc32(packet->data, packet->data_size);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_clear(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Reset data size but keep the buffer
     packet->data_size = 0;
     
     // Reset checksum
     packet->checksum = 0;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_flags(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     polycall_packet_flags_t* flags
 ) {
     if (!ctx || !packet || !flags) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *flags = packet->flags;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_flags(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     polycall_packet_flags_t flags
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->flags = flags;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_id(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint32_t* id
 ) {
     if (!ctx || !packet || !id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *id = packet->id;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_id(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint32_t id
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->id = id;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_sequence(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint32_t* sequence
 ) {
     if (!ctx || !packet || !sequence) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *sequence = packet->sequence;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_sequence(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint32_t sequence
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->sequence = sequence;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_timestamp(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint64_t* timestamp
 ) {
     if (!ctx || !packet || !timestamp) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *timestamp = packet->timestamp;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_timestamp(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint64_t timestamp
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->timestamp = timestamp;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_type(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint16_t* type
 ) {
     if (!ctx || !packet || !type) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *type = packet->type;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_type(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint16_t type
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->type = type;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_clone(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     polycall_network_packet_t** clone
 ) {
     if (!ctx || !packet || !clone) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create a new packet with the same capacity
     polycall_network_packet_t* new_packet = polycall_core_malloc(ctx, sizeof(polycall_network_packet_t));
     if (!new_packet) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize the new packet structure
     memset(new_packet, 0, sizeof(polycall_network_packet_t));
     
     // Copy data if there is any
     if (packet->data && packet->data_size > 0) {
         new_packet->data = polycall_core_malloc(ctx, packet->buffer_capacity);
         if (!new_packet->data) {
             polycall_core_free(ctx, new_packet);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_packet->data, packet->data, packet->data_size);
         new_packet->buffer_capacity = packet->buffer_capacity;
         new_packet->data_size = packet->data_size;
         new_packet->owns_data = true;
     }
     
     // Copy other fields
     new_packet->type = packet->type;
     new_packet->id = packet->id;
     new_packet->sequence = packet->sequence;
     new_packet->timestamp = packet->timestamp;
     new_packet->flags = packet->flags;
     new_packet->checksum = packet->checksum;
     new_packet->priority = packet->priority;
     
     // Clone metadata
     for (size_t i = 0; i < packet->metadata_count && i < MAX_METADATA_ENTRIES; i++) {
         strcpy(new_packet->metadata[i].key, packet->metadata[i].key);
         
         if (packet->metadata[i].value && packet->metadata[i].value_size > 0) {
             new_packet->metadata[i].value = polycall_core_malloc(ctx, packet->metadata[i].value_size);
             if (!new_packet->metadata[i].value) {
                 // Clean up on failure
                 for (size_t j = 0; j < i; j++) {
                     if (new_packet->metadata[j].value) {
                         polycall_core_free(ctx, new_packet->metadata[j].value);
                     }
                 }
                 
                 if (new_packet->data) {
                     polycall_core_free(ctx, new_packet->data);
                 }
                 
                 polycall_core_free(ctx, new_packet);
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memcpy(new_packet->metadata[i].value, packet->metadata[i].value, packet->metadata[i].value_size);
             new_packet->metadata[i].value_size = packet->metadata[i].value_size;
         }
     }
     
     new_packet->metadata_count = packet->metadata_count;
     
     *clone = new_packet;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_compress(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if already compressed
     if (packet->flags & POLYCALL_PACKET_FLAG_COMPRESSED) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // In a real implementation, we would compress the data here using a library like zlib
     // For this implementation, we'll just set the flag
     
     packet->flags |= POLYCALL_PACKET_FLAG_COMPRESSED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_decompress(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if not compressed
     if (!(packet->flags & POLYCALL_PACKET_FLAG_COMPRESSED)) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // In a real implementation, we would decompress the data here using a library like zlib
     // For this implementation, we'll just clear the flag
     
     packet->flags &= ~POLYCALL_PACKET_FLAG_COMPRESSED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_encrypt(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const void* key,
     size_t key_size
 ) {
     if (!ctx || !packet || !key || key_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if already encrypted
     if (packet->flags & POLYCALL_PACKET_FLAG_ENCRYPTED) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // In a real implementation, we would encrypt the data here using a cryptographic library
     // For this implementation, we'll just set the flag
     
     packet->flags |= POLYCALL_PACKET_FLAG_ENCRYPTED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_decrypt(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const void* key,
     size_t key_size
 ) {
     if (!ctx || !packet || !key || key_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if not encrypted
     if (!(packet->flags & POLYCALL_PACKET_FLAG_ENCRYPTED)) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // In a real implementation, we would decrypt the data here using a cryptographic library
     // For this implementation, we'll just clear the flag
     
     packet->flags &= ~POLYCALL_PACKET_FLAG_ENCRYPTED;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_create_fragment(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint16_t fragment_index,
     size_t fragment_size,
     polycall_network_packet_t** fragment
 ) {
     if (!ctx || !packet || !fragment || fragment_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we have data to fragment
     if (!packet->data || packet->data_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Calculate fragment bounds
     size_t start_offset = fragment_index * fragment_size;
     if (start_offset >= packet->data_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     size_t actual_fragment_size = packet->data_size - start_offset;
     if (actual_fragment_size > fragment_size) {
         actual_fragment_size = fragment_size;
     }
     
     // Create new packet for the fragment
     polycall_network_packet_t* new_fragment = polycall_core_malloc(ctx, sizeof(polycall_network_packet_t));
     if (!new_fragment) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize fragment structure
     memset(new_fragment, 0, sizeof(polycall_network_packet_t));
     
     // Allocate fragment data buffer
     new_fragment->data = polycall_core_malloc(ctx, actual_fragment_size);
     if (!new_fragment->data) {
         polycall_core_free(ctx, new_fragment);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy fragment data
     memcpy(new_fragment->data, (uint8_t*)packet->data + start_offset, actual_fragment_size);
     new_fragment->buffer_capacity = actual_fragment_size;
     new_fragment->data_size = actual_fragment_size;
     new_fragment->owns_data = true;
     
     // Copy relevant packet properties
     new_fragment->type = packet->type;
     new_fragment->id = packet->id;
     new_fragment->sequence = packet->sequence;
     new_fragment->timestamp = packet->timestamp;
     new_fragment->flags = packet->flags | POLYCALL_PACKET_FLAG_FRAGMENTED;
     new_fragment->priority = packet->priority;
     
     // Set fragment-specific flags
     if (fragment_index == 0) {
         new_fragment->flags |= POLYCALL_PACKET_FLAG_FIRST_FRAGMENT;
     }
     
     if (start_offset + actual_fragment_size >= packet->data_size) {
         new_fragment->flags |= POLYCALL_PACKET_FLAG_LAST_FRAGMENT;
     }
     
     // Calculate fragment checksum
     new_fragment->checksum = calculate_crc32(new_fragment->data, new_fragment->data_size);
     
     // Add fragment metadata
     char key[32];
     snprintf(key, sizeof(key), "fragment_index");
     polycall_network_packet_set_metadata(ctx, new_fragment, key, &fragment_index, sizeof(fragment_index));
     
     uint16_t total_fragments = (uint16_t)((packet->data_size + fragment_size - 1) / fragment_size);
     snprintf(key, sizeof(key), "total_fragments");
     polycall_network_packet_set_metadata(ctx, new_fragment, key, &total_fragments, sizeof(total_fragments));
     
     *fragment = new_fragment;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_reassemble_fragments(
     polycall_core_context_t* ctx,
     polycall_network_packet_t** fragments,
     size_t fragment_count,
     polycall_network_packet_t** complete
 ) {
     if (!ctx || !fragments || fragment_count == 0 || !complete) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Verify all fragments are valid and belong to the same packet
     uint32_t packet_id = fragments[0]->id;
     uint16_t total_fragments = 0;
     
     // Find total fragment count from metadata
     for (size_t i = 0; i < fragment_count; i++) {
         if (!fragments[i]) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         if (fragments[i]->id != packet_id) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         if (fragments[i]->flags & POLYCALL_PACKET_FLAG_FIRST_FRAGMENT) {
             // Get total fragment count from first fragment
             uint16_t* total_ptr = NULL;
             size_t size = sizeof(uint16_t);
             
             polycall_core_error_t result = polycall_network_packet_get_metadata(
                 ctx, fragments[i], "total_fragments", &total_ptr, &size);
                 
             if (result != POLYCALL_CORE_SUCCESS || !total_ptr) {
                 return POLYCALL_CORE_ERROR_INVALID_STATE;
             }
             
             total_fragments = *total_ptr;
             break;
         }
     }
     
     if (total_fragments == 0) {
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     if (fragment_count != total_fragments) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Sort fragments by index
     // In a real implementation, we would have a proper sort algorithm
     // For simplicity, we'll assume fragments are already sorted
     
     // Calculate total data size
     size_t total_size = 0;
     for (size_t i = 0; i < fragment_count; i++) {
         total_size += fragments[i]->data_size;
     }
     
     // Create a new packet for the reassembled data
     polycall_network_packet_t* new_packet = polycall_core_malloc(ctx, sizeof(polycall_network_packet_t));
     if (!new_packet) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize packet structure
     memset(new_packet, 0, sizeof(polycall_network_packet_t));
     
     // Allocate data buffer
     new_packet->data = polycall_core_malloc(ctx, total_size);
     if (!new_packet->data) {
         polycall_core_free(ctx, new_packet);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy data from fragments
     size_t offset = 0;
     for (size_t i = 0; i < fragment_count; i++) {
         memcpy((uint8_t*)new_packet->data + offset, fragments[i]->data, fragments[i]->data_size);
         offset += fragments[i]->data_size;
     }
     
     new_packet->buffer_capacity = total_size;
     new_packet->data_size = total_size;
     new_packet->owns_data = true;
     
     // Copy properties from first fragment
     new_packet->type = fragments[0]->type;
     new_packet->id = fragments[0]->id;
     new_packet->sequence = fragments[0]->sequence;
     new_packet->timestamp = fragments[0]->timestamp;
     new_packet->priority = fragments[0]->priority;
     
     // Set flags but remove fragmentation flags
     new_packet->flags = fragments[0]->flags & ~(
         POLYCALL_PACKET_FLAG_FRAGMENTED | 
         POLYCALL_PACKET_FLAG_FIRST_FRAGMENT | 
         POLYCALL_PACKET_FLAG_LAST_FRAGMENT
     );
     
     // Calculate checksum
     new_packet->checksum = calculate_crc32(new_packet->data, new_packet->data_size);
     
     *complete = new_packet;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_metadata(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const char* key,
     const void* value,
     size_t value_size
 ) {
     if (!ctx || !packet || !key || !value || value_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if key already exists
     for (size_t i = 0; i < packet->metadata_count; i++) {
         if (strcmp(packet->metadata[i].key, key) == 0) {
             // Update existing entry
             void* new_value = polycall_core_malloc(ctx, value_size);
             if (!new_value) {
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memcpy(new_value, value, value_size);
             
             // Free old value
             if (packet->metadata[i].value) {
                 polycall_core_free(ctx, packet->metadata[i].value);
             }
             
             packet->metadata[i].value = new_value;
             packet->metadata[i].value_size = value_size;
             
             // Set metadata flag
             packet->flags |= POLYCALL_PACKET_FLAG_METADATA;
             
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Add new entry if we have space
     if (packet->metadata_count < MAX_METADATA_ENTRIES) {
         // Check key length
         if (strlen(key) >= sizeof(packet->metadata[0].key)) {
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
         }
         
         strcpy(packet->metadata[packet->metadata_count].key, key);
         
         void* new_value = polycall_core_malloc(ctx, value_size);
         if (!new_value) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         memcpy(new_value, value, value_size);
         
         packet->metadata[packet->metadata_count].value = new_value;
         packet->metadata[packet->metadata_count].value_size = value_size;
         
         packet->metadata_count++;
         
         // Set metadata flag
         packet->flags |= POLYCALL_PACKET_FLAG_METADATA;
         
         return POLYCALL_CORE_SUCCESS;
     }
     
     return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
 }
 
 polycall_core_error_t polycall_network_packet_get_metadata(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     const char* key,
     void* value,
     size_t* value_size
 ) {
     if (!ctx || !packet || !key || !value_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Look for key
     for (size_t i = 0; i < packet->metadata_count; i++) {
         if (strcmp(packet->metadata[i].key, key) == 0) {
             // Found the key
             if (value) {
                 // Check if buffer is large enough
                 if (*value_size < packet->metadata[i].value_size) {
                     *value_size = packet->metadata[i].value_size;
                     return POLYCALL_CORE_ERROR_BUFFER_TOO_SMALL;
                 }
                 
                 // Copy value
                 memcpy(value, packet->metadata[i].value, packet->metadata[i].value_size);
             }
             
             // Return actual size
             *value_size = packet->metadata[i].value_size;
             
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     return POLYCALL_CORE_ERROR_NOT_FOUND;
 }
 
 polycall_core_error_t polycall_network_packet_calculate_checksum(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint32_t* checksum
 ) {
     if (!ctx || !packet || !checksum) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!packet->data || packet->data_size == 0) {
         *checksum = 0;
         return POLYCALL_CORE_SUCCESS;
     }
     
     *checksum = calculate_crc32(packet->data, packet->data_size);
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_verify_checksum(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     bool* valid
 ) {
     if (!ctx || !packet || !valid) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!packet->data || packet->data_size == 0) {
         *valid = (packet->checksum == 0);
         return POLYCALL_CORE_SUCCESS;
     }
     
     uint32_t calculated_checksum = calculate_crc32(packet->data, packet->data_size);
     *valid = (calculated_checksum == packet->checksum);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_get_priority(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint8_t* priority
 ) {
     if (!ctx || !packet || !priority) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *priority = packet->priority;
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_network_packet_set_priority(
     polycall_core_context_t* ctx,
     polycall_network_packet_t* packet,
     uint8_t priority
 ) {
     if (!ctx || !packet) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     packet->priority = priority;
     
     // Update priority flags for quick priority checks
     packet->flags &= ~(POLYCALL_PACKET_FLAG_PRIORITY_HIGH | POLYCALL_PACKET_FLAG_PRIORITY_LOW);
     
     if (priority > 128) {
         packet->flags |= POLYCALL_PACKET_FLAG_PRIORITY_HIGH;
     } else if (priority < 64) {
         packet->flags |= POLYCALL_PACKET_FLAG_PRIORITY_LOW;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
