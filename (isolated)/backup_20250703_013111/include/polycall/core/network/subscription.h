/**
 * @file subscription.h
 * @brief Subscription system enhancement for the PolyCall protocol
 *
 * This module provides publish/subscribe messaging capabilities for the PolyCall protocol.
 */

#ifndef POLYCALL_NETWORK_SUBSCRIPTION_H_H
#define POLYCALL_NETWORK_SUBSCRIPTION_H_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of subscribers per topic
 */
#define POLYCALL_NETWORK_SUBSCRIPTION_H_H

/**
 * @brief Maximum topic length
 */
#define POLYCALL_NETWORK_SUBSCRIPTION_H_H

/**
 * @brief Maximum number of delivery attempts
 */
#define POLYCALL_NETWORK_SUBSCRIPTION_H_H

/**
 * @brief Default subscription configuration
 */
static const protocol_enhancement_subscription_config_t DEFAULT_SUBSCRIPTION_CONFIG = {
	.max_subscriptions = 1000,
	.enable_wildcards = true,
	.max_subscribers_per_topic = 100,
	.delivery_attempt_count = 3
};

/**
 * @brief Subscriber entry structure
 */
typedef struct subscriber_entry {
	uint32_t subscription_id;
	char* topic;
	void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data);
	void* user_data;
	struct subscriber_entry* next;
} subscriber_entry_t;

/**
 * @brief Topic entry structure
 */
typedef struct topic_entry {
	char* topic;
	subscriber_entry_t* subscribers;
	uint32_t subscriber_count;
	struct topic_entry* next;
} topic_entry_t;

/**
 * @brief Subscription context structure
 */
struct polycall_subscription_context {
	protocol_enhancement_subscription_config_t config;
	topic_entry_t* topics;
	uint32_t topic_count;
	uint32_t next_subscription_id;
	pthread_mutex_t mutex;
};

/**
 * @brief Create default subscription configuration
 */
protocol_enhancement_subscription_config_t polycall_subscription_create_default_config(void) {
	return DEFAULT_SUBSCRIPTION_CONFIG;
}
/**
 * @brief Subscription configuration structure
 */
typedef struct {
	uint32_t max_subscriptions;          /**< Maximum number of subscriptions allowed */
	bool enable_wildcards;               /**< Whether wildcard subscriptions are allowed */
	uint32_t max_subscribers_per_topic;  /**< Maximum subscribers per topic */
	uint32_t delivery_attempt_count;     /**< Number of delivery attempts for messages */
} protocol_enhancement_subscription_config_t;

/**
 * @brief Subscription context structure
 */
typedef struct polycall_subscription_context polycall_subscription_context_t;

/**
 * @brief Initialize the subscription system
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param subscription_ctx Pointer to subscription context pointer
 * @param config Subscription configuration
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_init(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	polycall_subscription_context_t** subscription_ctx,
	const protocol_enhancement_subscription_config_t* config
);

/**
 * @brief Clean up the subscription system
 *
 * @param ctx Core context
 * @param subscription_ctx Subscription context
 */
void polycall_subscription_cleanup(
	polycall_core_context_t* ctx,
	polycall_subscription_context_t* subscription_ctx
);

/**
 * @brief Handle subscribe message
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param msg Message
 * @param user_data User data (subscription context)
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_handle_subscribe(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	const polycall_message_t* msg,
	void* user_data
);

/**
 * @brief Handle unsubscribe message
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param msg Message
 * @param user_data User data (subscription context)
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_handle_unsubscribe(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	const polycall_message_t* msg,
	void* user_data
);

/**
 * @brief Handle publish message
 *
 * @param ctx Core context
 * @param proto_ctx Protocol context
 * @param msg Message
 * @param user_data User data (subscription context)
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_handle_publish(
	polycall_core_context_t* ctx,
	polycall_protocol_context_t* proto_ctx,
	const polycall_message_t* msg,
	void* user_data
);

/**
 * @brief Publish a message to a topic
 *
 * @param ctx Core context
 * @param subscription_ctx Subscription context
 * @param topic Topic to publish to
 * @param data Message data
 * @param data_size Message data size
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_publish(
	polycall_core_context_t* ctx,
	polycall_subscription_context_t* subscription_ctx,
	const char* topic,
	const void* data,
	size_t data_size
);

/**
 * @brief Subscribe to a topic
 *
 * @param ctx Core context
 * @param subscription_ctx Subscription context
 * @param topic Topic to subscribe to
 * @param callback Callback function to call when a message is published
 * @param user_data User data to pass to callback
 * @param subscription_id Pointer to store subscription ID
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_subscribe(
	polycall_core_context_t* ctx,
	polycall_subscription_context_t* subscription_ctx,
	const char* topic,
	void (*callback)(const char* topic, const void* data, size_t data_size, void* user_data),
	void* user_data,
	uint32_t* subscription_id
);

/**
 * @brief Unsubscribe from a topic
 *
 * @param ctx Core context
 * @param subscription_ctx Subscription context
 * @param subscription_id Subscription ID
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_subscription_unsubscribe(
	polycall_core_context_t* ctx,
	polycall_subscription_context_t* subscription_ctx,
	uint32_t subscription_id
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_SUBSCRIPTION_H_H */
