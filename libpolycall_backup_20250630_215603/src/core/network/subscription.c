/**
 * @file subscription.c
 * @brief Network subscription management implementation for LibPolyCall
 *
 * Originally part of protocol enhancements, integrated into core network module.
 */

#include "polycall/core/network/subscription.h"
#include "polycall/core/network/network_internal.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"

#include <stdlib.h>
#include <string.h>
 
 #define POLYCALL_SUBSCRIPTION_MAGIC 0xF3B5C091
/**
 * @brief Subscription structure
 */
struct polycall_subscription {
    polycall_network_endpoint_t* endpoint;
    polycall_subscription_handler_t handler;
    void* user_data;
    bool active;
    polycall_mutex_t lock;
    void* internal_context; // For endpoint-specific data
};
 /**
  * @brief Subscriber entry structure
  */
 typedef struct {
     uint32_t subscription_id;
     void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data);
     void* user_data;
     bool is_active;
 } polycall_subscriber_t;
 
 /**
  * @brief Topic entry structure
  */
 typedef struct {
     char name[POLYCALL_MAX_TOPIC_LENGTH];
     polycall_subscriber_t* subscribers;
     uint32_t subscriber_count;
     uint32_t subscriber_capacity;
     uint64_t last_message_timestamp;
     uint32_t total_messages;
 } polycall_topic_t;
 
 /**
  * @brief Subscription context structure
  */
 struct polycall_subscription_context {
     uint32_t magic;                       // Magic number for validation
     protocol_enhancement_subscription_config_t config;  // Configuration
     
     // Topic storage
     polycall_topic_t* topics;             // Array of topics
     uint32_t topic_count;                 // Number of topics
     uint32_t topic_capacity;              // Capacity of topics array
     
     // Subscription tracking
     uint32_t next_subscription_id;        // Next subscription ID to assign
     uint32_t total_subscribers;           // Total subscribers across all topics
     
     // Statistics
     struct {
         uint32_t messages_published;      // Total messages published
         uint32_t delivery_attempts;       // Total delivery attempts
         uint32_t delivery_failures;       // Failed deliveries
         uint32_t wildcard_matches;        // Wildcard subscription matches
     } stats;
     
     // Core context reference
     polycall_core_context_t* core_ctx;    // Core context
     polycall_protocol_context_t* proto_ctx; // Protocol context
 };
 
 // Forward declarations for internal functions
 static bool validate_subscription_context(polycall_subscription_context_t* sub_ctx);
 static bool match_topic(const char* pattern, const char* topic, bool enable_wildcards);
 static polycall_topic_t* find_topic(polycall_subscription_context_t* sub_ctx, const char* topic);
 static polycall_topic_t* create_topic(polycall_subscription_context_t* sub_ctx, const char* topic);
 static polycall_core_error_t add_subscriber_to_topic(
     polycall_core_context_t* ctx,
     polycall_topic_t* topic,
     void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data),
     void* user_data,
     uint32_t subscription_id
 );
 static bool remove_subscriber_from_topic(
     polycall_topic_t* topic,
     uint32_t subscription_id
 );
 static uint32_t find_topic_by_subscription_id(
     polycall_subscription_context_t* sub_ctx,
     uint32_t subscription_id,
     uint32_t* subscriber_index
 );
 static uint64_t get_current_timestamp(void);
 
 /**
  * @brief Initialize the subscription system
  */
 polycall_core_error_t polycall_subscription_init(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     polycall_subscription_context_t** subscription_ctx,
     const protocol_enhancement_subscription_config_t* config
 ) {
     if (!ctx || !proto_ctx || !subscription_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate subscription context
     polycall_subscription_context_t* new_ctx = 
         polycall_core_malloc(ctx, sizeof(polycall_subscription_context_t));
     
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     memset(new_ctx, 0, sizeof(polycall_subscription_context_t));
     new_ctx->magic = POLYCALL_SUBSCRIPTION_MAGIC;
     new_ctx->core_ctx = ctx;
     new_ctx->proto_ctx = proto_ctx;
     
     // Copy configuration
     memcpy(&new_ctx->config, config, sizeof(protocol_enhancement_subscription_config_t));
     
     // Initialize topic storage
     uint32_t initial_capacity = POLYCALL_DEFAULT_TOPIC_CAPACITY;
     new_ctx->topics = polycall_core_malloc(ctx, initial_capacity * sizeof(polycall_topic_t));
     
     if (!new_ctx->topics) {
         polycall_core_free(ctx, new_ctx);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     new_ctx->topic_capacity = initial_capacity;
     new_ctx->topic_count = 0;
     
     // Initialize subscription ID counter (start at 1, 0 is invalid)
     new_ctx->next_subscription_id = 1;
     
     *subscription_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up the subscription system
  */
 void polycall_subscription_cleanup(
     polycall_core_context_t* ctx,
     polycall_subscription_context_t* subscription_ctx
 ) {
     if (!ctx || !validate_subscription_context(subscription_ctx)) {
         return;
     }
     
     // Clean up all topics and subscribers
     for (uint32_t i = 0; i < subscription_ctx->topic_count; i++) {
         if (subscription_ctx->topics[i].subscribers) {
             polycall_core_free(ctx, subscription_ctx->topics[i].subscribers);
         }
     }
     
     // Free topics array
     if (subscription_ctx->topics) {
         polycall_core_free(ctx, subscription_ctx->topics);
     }
     
     // Clear magic number
     subscription_ctx->magic = 0;
     
     // Free context
     polycall_core_free(ctx, subscription_ctx);
 }
 
 /**
  * @brief Validate subscription context
  */
 static bool validate_subscription_context(polycall_subscription_context_t* sub_ctx) {
     return sub_ctx && sub_ctx->magic == POLYCALL_SUBSCRIPTION_MAGIC;
 }
 
 /**
  * @brief Match a topic against a pattern (with optional wildcards)
  */
 static bool match_topic(const char* pattern, const char* topic, bool enable_wildcards) {
     if (!pattern || !topic) {
         return false;
     }
     
     // Exact match
     if (strcmp(pattern, topic) == 0) {
         return true;
     }
     
     // Wildcard matching (if enabled)
     if (enable_wildcards) {
         // Simple wildcard implementation
         // '*' matches any sequence of characters
         // '#' matches a single segment in a hierarchical topic (e.g., a/b/c)
         
         // Handle full wildcard
         if (strcmp(pattern, "*") == 0) {
             return true;
         }
         
         // TODO: Implement more sophisticated wildcard matching
         // This would handle patterns like "sensors/#" or "weather/*/temperature"
         
         // For now, just handle trailing wildcards
         size_t pattern_len = strlen(pattern);
         if (pattern_len > 0 && pattern[pattern_len - 1] == '*') {
             // Check if topic starts with the pattern prefix
             if (strncmp(topic, pattern, pattern_len - 1) == 0) {
                 return true;
             }
         }
     }
     
     return false;
 }
 
 /**
  * @brief Find a topic by name
  */
 static polycall_topic_t* find_topic(polycall_subscription_context_t* sub_ctx, const char* topic) {
     if (!sub_ctx || !topic) {
         return NULL;
     }
     
     for (uint32_t i = 0; i < sub_ctx->topic_count; i++) {
         if (strcmp(sub_ctx->topics[i].name, topic) == 0) {
             return &sub_ctx->topics[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Create a new topic
  */
 static polycall_topic_t* create_topic(polycall_subscription_context_t* sub_ctx, const char* topic) {
     if (!sub_ctx || !topic) {
         return NULL;
     }
     
     // Check if we need to grow the topics array
     if (sub_ctx->topic_count >= sub_ctx->topic_capacity) {
         uint32_t new_capacity = sub_ctx->topic_capacity * 2;
         polycall_topic_t* new_topics = polycall_core_malloc(
             sub_ctx->core_ctx, 
             new_capacity * sizeof(polycall_topic_t)
         );
         
         if (!new_topics) {
             return NULL;
         }
         
         // Copy existing topics
         memcpy(new_topics, sub_ctx->topics, sub_ctx->topic_count * sizeof(polycall_topic_t));
         
         // Free old array
         polycall_core_free(sub_ctx->core_ctx, sub_ctx->topics);
         
         // Update topics array
         sub_ctx->topics = new_topics;
         sub_ctx->topic_capacity = new_capacity;
     }
     
     // Initialize new topic
     polycall_topic_t* new_topic = &sub_ctx->topics[sub_ctx->topic_count++];
     memset(new_topic, 0, sizeof(polycall_topic_t));
     
     // Copy topic name (with truncation if necessary)
     strncpy(new_topic->name, topic, POLYCALL_MAX_TOPIC_LENGTH - 1);
     new_topic->name[POLYCALL_MAX_TOPIC_LENGTH - 1] = '\0';
     
     // Initialize subscribers array
     uint32_t initial_capacity = POLYCALL_DEFAULT_SUBSCRIBER_CAPACITY;
     new_topic->subscribers = polycall_core_malloc(
         sub_ctx->core_ctx, 
         initial_capacity * sizeof(polycall_subscriber_t)
     );
     
     if (!new_topic->subscribers) {
         // Failed to allocate subscribers array
         sub_ctx->topic_count--;
         return NULL;
     }
     
     new_topic->subscriber_capacity = initial_capacity;
     new_topic->subscriber_count = 0;
     new_topic->last_message_timestamp = get_current_timestamp();
     
     return new_topic;
 }
 
 /**
  * @brief Add a subscriber to a topic
  */
 static polycall_core_error_t add_subscriber_to_topic(
     polycall_core_context_t* ctx,
     polycall_topic_t* topic,
     void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data),
     void* user_data,
     uint32_t subscription_id
 ) {
     if (!ctx || !topic || !callback) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we need to grow the subscribers array
     if (topic->subscriber_count >= topic->subscriber_capacity) {
         uint32_t new_capacity = topic->subscriber_capacity * 2;
         polycall_subscriber_t* new_subscribers = polycall_core_malloc(
             ctx, 
             new_capacity * sizeof(polycall_subscriber_t)
         );
         
         if (!new_subscribers) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing subscribers
         memcpy(new_subscribers, topic->subscribers, 
                topic->subscriber_count * sizeof(polycall_subscriber_t));
         
         // Free old array
         polycall_core_free(ctx, topic->subscribers);
         
         // Update subscribers array
         topic->subscribers = new_subscribers;
         topic->subscriber_capacity = new_capacity;
     }
     
     // Add new subscriber
     polycall_subscriber_t* new_subscriber = &topic->subscribers[topic->subscriber_count++];
     new_subscriber->subscription_id = subscription_id;
     new_subscriber->callback = callback;
     new_subscriber->user_data = user_data;
     new_subscriber->is_active = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Remove a subscriber from a topic
  */
 static bool remove_subscriber_from_topic(
     polycall_topic_t* topic,
     uint32_t subscription_id
 ) {
     if (!topic) {
         return false;
     }
     
     // Find the subscriber
     for (uint32_t i = 0; i < topic->subscriber_count; i++) {
         if (topic->subscribers[i].subscription_id == subscription_id) {
             // Remove by shifting remaining subscribers
             if (i < topic->subscriber_count - 1) {
                 memmove(&topic->subscribers[i], &topic->subscribers[i + 1],
                        (topic->subscriber_count - i - 1) * sizeof(polycall_subscriber_t));
             }
             
             // Decrement count
             topic->subscriber_count--;
             return true;
         }
     }
     
     return false;
 }
 
 /**
  * @brief Find a topic containing a specific subscription ID
  */
 static uint32_t find_topic_by_subscription_id(
     polycall_subscription_context_t* sub_ctx,
     uint32_t subscription_id,
     uint32_t* subscriber_index
 ) {
     if (!sub_ctx || !subscriber_index) {
         return UINT32_MAX;
     }
     
     for (uint32_t i = 0; i < sub_ctx->topic_count; i++) {
         for (uint32_t j = 0; j < sub_ctx->topics[i].subscriber_count; j++) {
             if (sub_ctx->topics[i].subscribers[j].subscription_id == subscription_id) {
                 *subscriber_index = j;
                 return i;
             }
         }
     }
     
     return UINT32_MAX;
 }
 
 /**
  * @brief Get current timestamp
  */
 static uint64_t get_current_timestamp(void) {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
 }
 
 /**
  * @brief Handle subscribe message
  */
 polycall_core_error_t polycall_subscription_handle_subscribe(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const polycall_message_t* msg,
     void* user_data
 ) {
     if (!ctx || !proto_ctx || !msg || !user_data) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_subscription_context_t* sub_ctx = (polycall_subscription_context_t*)user_data;
     if (!validate_subscription_context(sub_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract subscription request from message
     // For a real implementation, this would deserialize the message payload
     // For simplicity, we'll assume the payload is a null-terminated topic string
     size_t payload_size = 0;
     const char* payload = (const char*)polycall_message_get_payload(msg, &payload_size);
     
     if (!payload || payload_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create a null-terminated copy of the topic
     char topic[POLYCALL_MAX_TOPIC_LENGTH];
     size_t copy_size = payload_size < POLYCALL_MAX_TOPIC_LENGTH - 1 ?
                       payload_size : POLYCALL_MAX_TOPIC_LENGTH - 1;
     memcpy(topic, payload, copy_size);
     topic[copy_size] = '\0';
     
     // Generate a subscription ID
     uint32_t subscription_id = sub_ctx->next_subscription_id++;
     
     // Create a callback that will forward messages to the subscriber
     void (*callback)(const char*, const void*, size_t, void*) = 
         polycall_subscription_message_forwarder;
     
     // Subscribe to the topic
     return polycall_subscription_subscribe(
         ctx,
         sub_ctx,
         topic,
         callback,
         proto_ctx,  // Store protocol context as user data for message forwarding
         &subscription_id
     );
 }
 
 /**
  * @brief Handle unsubscribe message
  */
 polycall_core_error_t polycall_subscription_handle_unsubscribe(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const polycall_message_t* msg,
     void* user_data
 ) {
     if (!ctx || !proto_ctx || !msg || !user_data) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_subscription_context_t* sub_ctx = (polycall_subscription_context_t*)user_data;
     if (!validate_subscription_context(sub_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract subscription ID from message
     // For a real implementation, this would deserialize the message payload
     // For simplicity, we'll assume the payload is the subscription ID as a uint32_t
     size_t payload_size = 0;
     const void* payload = polycall_message_get_payload(msg, &payload_size);
     
     if (!payload || payload_size < sizeof(uint32_t)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract subscription ID
     uint32_t subscription_id = *(const uint32_t*)payload;
     
     // Unsubscribe
     return polycall_subscription_unsubscribe(
         ctx,
         sub_ctx,
         subscription_id
     );
 }
 
 /**
  * @brief Handle publish message
  */
 polycall_core_error_t polycall_subscription_handle_publish(
     polycall_core_context_t* ctx,
     polycall_protocol_context_t* proto_ctx,
     const polycall_message_t* msg,
     void* user_data
 ) {
     if (!ctx || !proto_ctx || !msg || !user_data) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_subscription_context_t* sub_ctx = (polycall_subscription_context_t*)user_data;
     if (!validate_subscription_context(sub_ctx)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract publish information from message
     // For a real implementation, this would deserialize the message payload
     // For simplicity, we'll assume a special format:
     // - First POLYCALL_MAX_TOPIC_LENGTH bytes: topic name (null-terminated)
     // - Remaining bytes: message data
     size_t payload_size = 0;
     const void* payload = polycall_message_get_payload(msg, &payload_size);
     
     if (!payload || payload_size <= POLYCALL_MAX_TOPIC_LENGTH) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract topic name (null-terminated)
     const char* topic = (const char*)payload;
     
     // Extract message data (starts after topic)
     size_t topic_len = strlen(topic) + 1;  // Include null terminator
     const void* data = (const uint8_t*)payload + topic_len;
     size_t data_size = payload_size - topic_len;
     
     // Publish message
     return polycall_subscription_publish(
         ctx,
         sub_ctx,
         topic,
         data,
         data_size
     );
 }
 
 /**
  * @brief Publish a message to a topic
  */
 polycall_core_error_t polycall_subscription_publish(
     polycall_core_context_t* ctx,
     polycall_subscription_context_t* sub_ctx,
     const char* topic,
     const void* data,
     size_t data_size
 ) {
     if (!ctx || !validate_subscription_context(sub_ctx) || !topic || 
         (!data && data_size > 0)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Update statistics
     sub_ctx->stats.messages_published++;
     
     // Find the topic first
     polycall_topic_t* topic_entry = find_topic(sub_ctx, topic);
     
     // No subscribers for this topic yet
     if (!topic_entry) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Update topic statistics
     topic_entry->total_messages++;
     topic_entry->last_message_timestamp = get_current_timestamp();
     
     // Notify each subscriber
     for (uint32_t i = 0; i < topic_entry->subscriber_count; i++) {
         if (topic_entry->subscribers[i].is_active && topic_entry->subscribers[i].callback) {
             sub_ctx->stats.delivery_attempts++;
             
             // Try to deliver the message
             // In a real implementation, this might track delivery status
             bool delivered = true;
             
             // Invoke subscriber callback
             topic_entry->subscribers[i].callback(
                 topic,
                 data,
                 data_size,
                 topic_entry->subscribers[i].user_data
             );
             
             if (!delivered) {
                 sub_ctx->stats.delivery_failures++;
             }
         }
     }
     
     // Check for wildcard subscribers
     if (sub_ctx->config.enable_wildcards) {
         // Check all topics for wildcard patterns that match this topic
         for (uint32_t i = 0; i < sub_ctx->topic_count; i++) {
             // Skip the exact match topic (already processed)
             if (strcmp(sub_ctx->topics[i].name, topic) == 0) {
                 continue;
             }
             
             // Check if this topic pattern matches
             if (match_topic(sub_ctx->topics[i].name, topic, true)) {
                 // Found a wildcard match
                 sub_ctx->stats.wildcard_matches++;
                 
                 // Notify each subscriber to the wildcard topic
                 for (uint32_t j = 0; j < sub_ctx->topics[i].subscriber_count; j++) {
                     if (sub_ctx->topics[i].subscribers[j].is_active && 
                         sub_ctx->topics[i].subscribers[j].callback) {
                         
                         sub_ctx->stats.delivery_attempts++;
                         
                         // Try to deliver the message
                         bool delivered = true;
                         
                         // Invoke subscriber callback
                         sub_ctx->topics[i].subscribers[j].callback(
                             topic,  // Note: We pass the actual topic, not the pattern
                             data,
                             data_size,
                             sub_ctx->topics[i].subscribers[j].user_data
                         );
                         
                         if (!delivered) {
                             sub_ctx->stats.delivery_failures++;
                         }
                     }
                 }
             }
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Subscribe to a topic
  */
 polycall_core_error_t polycall_subscription_subscribe(
     polycall_core_context_t* ctx,
     polycall_subscription_context_t* sub_ctx,
     const char* topic,
     void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data),
     void* user_data,
     uint32_t* subscription_id
 ) {
     if (!ctx || !validate_subscription_context(sub_ctx) || !topic || 
         !callback || !subscription_id) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if we've reached the maximum number of subscriptions
     if (sub_ctx->total_subscribers >= sub_ctx->config.max_subscriptions) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Find or create the topic
     polycall_topic_t* topic_entry = find_topic(sub_ctx, topic);
     if (!topic_entry) {
         topic_entry = create_topic(sub_ctx, topic);
         if (!topic_entry) {
             return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
         }
     }
     
     // Check if we've reached the maximum subscribers per topic
     if (topic_entry->subscriber_count >= sub_ctx->config.max_subscribers_per_topic) {
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Generate subscription ID if needed
     uint32_t new_subscription_id = *subscription_id;
     if (new_subscription_id == 0) {
         new_subscription_id = sub_ctx->next_subscription_id++;
         *subscription_id = new_subscription_id;
     }
     
     // Add subscriber to topic
     polycall_core_error_t result = add_subscriber_to_topic(
         ctx,
         topic_entry,
         callback,
         user_data,
         new_subscription_id
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         sub_ctx->total_subscribers++;
     }
     
     return result;
 }
 
 /**
  * @brief Unsubscribe from a topic
  */
 polycall_core_error_t polycall_subscription_unsubscribe(
     polycall_core_context_t* ctx,
     polycall_subscription_context_t* sub_ctx,
     uint32_t subscription_id
 ) {
     if (!ctx || !validate_subscription_context(sub_ctx) || subscription_id == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find the topic containing this subscription
     uint32_t subscriber_index = 0;
     uint32_t topic_index = find_topic_by_subscription_id(sub_ctx, subscription_id, &subscriber_index);
     
     if (topic_index == UINT32_MAX) {
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     // Remove the subscriber
     bool removed = remove_subscriber_from_topic(&sub_ctx->topics[topic_index], subscription_id);
     
     if (removed) {
         sub_ctx->total_subscribers--;
         
         // If topic has no subscribers left, consider removing it
         if (sub_ctx->topics[topic_index].subscriber_count == 0) {
             // Free subscribers array
             polycall_core_free(ctx, sub_ctx->topics[topic_index].subscribers);
             
             // Remove topic by shifting remaining topics
             if (topic_index < sub_ctx->topic_count - 1) {
                 memmove(&sub_ctx->topics[topic_index], &sub_ctx->topics[topic_index + 1],
                        (sub_ctx->topic_count - topic_index - 1) * sizeof(polycall_topic_t));
             }
             
             // Decrement count
             sub_ctx->topic_count--;
         }
         
         return POLYCALL_CORE_SUCCESS;
     }
     
     return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
 }
 
 /**
  * @brief Message forwarding callback for network subscribers
  */
 void polycall_subscription_message_forwarder(
     const char* topic,
     const void* data,
     size_t data_size,
     void* user_data
 ) {
     // The user_data is the protocol context
     polycall_protocol_context_t* proto_ctx = (polycall_protocol_context_t*)user_data;
     if (!proto_ctx || !topic || !data) {
         return;
     }
     
     // Create a special message format:
     // - First POLYCALL_MAX_TOPIC_LENGTH bytes: topic name (null-terminated)
     // - Remaining bytes: message data
     size_t topic_len = strlen(topic) + 1;  // Include null terminator
     size_t payload_size = topic_len + data_size;
     
     uint8_t* payload = malloc(payload_size);
     if (!payload) {
         return;
     }
     
     // Copy topic name
     strcpy((char*)payload, topic);
     
     // Copy message data
     memcpy(payload + topic_len, data, data_size);
     
     // Send message through protocol
     polycall_protocol_send(
         proto_ctx,
         POLYCALL_PROTOCOL_MSG_PUBLISHED,  // A new message type for published messages
         payload,
         payload_size,
         POLYCALL_PROTOCOL_FLAG_NONE
     );
     
     // Free temporary payload
     free(payload);
 }