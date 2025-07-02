/**

 * @file connection_pool.c
 * @brief Connection Pool Implementation for LibPolyCall Protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Implements efficient connection management with dynamic scaling,
 * load balancing, and resource optimization for high-volume scenarios.
 */


 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <pthread.h>
 #include "polycall/core/polycall/core/polycall.h"
    #include "polycall/core/protocol/polycall_protocol.h"
    #include "polycall/core/polycall/polycall_memory.h"
    #include "polycall/core/polycall/polycall_error.h"
    #include "polycall/core/network/connection_pool.h"

 
 #define POLYCALL_CONNECTION_POOL_MAGIC 0xC0AAEC71
 
 /**
  * @brief Connection entry in the pool
  */
 typedef struct {
     polycall_protocol_context_t* proto_ctx;     // Protocol context
     polycall_connection_state_t state;          // Connection state
     uint64_t creation_time;                     // Creation timestamp
     uint64_t last_used_time;                    // Last used timestamp
     uint64_t last_validated_time;               // Last validation timestamp
     uint32_t request_count;                     // Requests processed
     bool is_valid;                              // Validity flag
 } polycall_connection_entry_t;
 
 /**
  * @brief Connection pool context structure
  */
 struct polycall_connection_pool_context {
     uint32_t magic;                             // Magic number for validation
     polycall_connection_pool_config_t config;   // Pool configuration
     polycall_connection_entry_t* connections;   // Connection array
     uint32_t pool_size;                         // Current pool size
     uint32_t active_count;                      // Active connection count
     uint32_t next_index;                        // Next connection index for round-robin
     
     // Statistics
     polycall_connection_pool_stats_t stats;     // Pool statistics
     
     // Synchronization
     pthread_mutex_t pool_mutex;                 // Pool mutex
     pthread_cond_t available_cond;              // Condition for available connections
     
     // Reference to core context
     polycall_core_context_t* core_ctx;          // Core context
 };
 
 /**
  * @brief Get current timestamp in milliseconds
  */
 static uint64_t get_timestamp_ms() {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
 }
 
 /**
  * @brief Validate a connection in the pool
  */
 static bool validate_connection(
     polycall_core_context_t* core_ctx,
     polycall_connection_entry_t* entry
 ) {
     if (!entry || !entry->proto_ctx) {
         return false;
     }
     
     // Check if connection is in a valid state
     polycall_protocol_state_t state = polycall_protocol_get_state(entry->proto_ctx);
     if (state == POLYCALL_PROTOCOL_STATE_ERROR || state == POLYCALL_PROTOCOL_STATE_CLOSED) {
         return false;
     }
     
     // Perform a lightweight ping/heartbeat check
     // This is a placeholder - in a real implementation, you would send a heartbeat
     // message and verify the response
     bool is_responsive = true; // Placeholder
     
     // Update validation timestamp
     if (is_responsive) {
         entry->last_validated_time = get_timestamp_ms();
     }
     
     return is_responsive;
 }
 
 /**
  * @brief Initialize connection pool
  */
 polycall_core_error_t polycall_connection_pool_init(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t** pool_ctx,
     const polycall_connection_pool_config_t* config
 ) {
     if (!core_ctx || !pool_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (config->initial_pool_size > POLYCALL_MAX_POOL_CONNECTIONS ||
         config->max_pool_size > POLYCALL_MAX_POOL_CONNECTIONS) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate pool context
     polycall_connection_pool_context_t* new_pool = 
         polycall_core_malloc(core_ctx, sizeof(polycall_connection_pool_context_t));
     
     if (!new_pool) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize pool context
     memset(new_pool, 0, sizeof(polycall_connection_pool_context_t));
     new_pool->magic = POLYCALL_CONNECTION_POOL_MAGIC;
     memcpy(&new_pool->config, config, sizeof(polycall_connection_pool_config_t));
     new_pool->core_ctx = core_ctx;
     
     // Allocate connection array
     new_pool->connections = polycall_core_malloc(
         core_ctx, 
         config->max_pool_size * sizeof(polycall_connection_entry_t)
     );
     
     if (!new_pool->connections) {
         polycall_core_free(core_ctx, new_pool);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize connection array
     memset(new_pool->connections, 0, 
            config->max_pool_size * sizeof(polycall_connection_entry_t));
     
     // Initialize synchronization primitives
     if (pthread_mutex_init(&new_pool->pool_mutex, NULL) != 0) {
         polycall_core_free(core_ctx, new_pool->connections);
         polycall_core_free(core_ctx, new_pool);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     if (pthread_cond_init(&new_pool->available_cond, NULL) != 0) {
         pthread_mutex_destroy(&new_pool->pool_mutex);
         polycall_core_free(core_ctx, new_pool->connections);
         polycall_core_free(core_ctx, new_pool);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Set initial pool size
     new_pool->pool_size = config->initial_pool_size;
     
     *pool_ctx = new_pool;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up connection pool
  */
 void polycall_connection_pool_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC) {
         return;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Close all connections
     for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
         polycall_connection_entry_t* entry = &pool_ctx->connections[i];
         if (entry->proto_ctx) {
             // Clean up protocol context
             polycall_protocol_cleanup(entry->proto_ctx);
             
             // Free protocol context
             polycall_core_free(core_ctx, entry->proto_ctx);
             entry->proto_ctx = NULL;
         }
     }
     
     // Free connection array
     polycall_core_free(core_ctx, pool_ctx->connections);
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     
     // Destroy synchronization primitives
     pthread_mutex_destroy(&pool_ctx->pool_mutex);
     pthread_cond_destroy(&pool_ctx->available_cond);
     
     // Clear magic number
     pool_ctx->magic = 0;
     
     // Free pool context
     polycall_core_free(core_ctx, pool_ctx);
 }
 
 /**
  * @brief Find an available connection based on the allocation strategy
  */
 static int find_available_connection(
     polycall_connection_pool_context_t* pool_ctx
 ) {
     uint64_t current_time = get_timestamp_ms();
     int candidate_idx = -1;
     uint64_t candidate_metric = 0;
     bool found = false;
     
     // Apply different allocation strategies
     switch (pool_ctx->config.strategy) {
         case POLYCALL_POOL_STRATEGY_FIFO:
             // First idle connection we find
             for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
                 if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE) {
                     candidate_idx = i;
                     found = true;
                     break;
                 }
             }
             break;
             
         case POLYCALL_POOL_STRATEGY_LIFO:
             // Last idle connection we find
             for (int i = pool_ctx->pool_size - 1; i >= 0; i--) {
                 if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE) {
                     candidate_idx = i;
                     found = true;
                     break;
                 }
             }
             break;
             
         case POLYCALL_POOL_STRATEGY_LRU:
             // Least recently used connection
             for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
                 if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE) {
                     uint64_t idle_time = current_time - pool_ctx->connections[i].last_used_time;
                     
                     if (!found || idle_time > candidate_metric) {
                         candidate_idx = i;
                         candidate_metric = idle_time;
                         found = true;
                     }
                 }
             }
             break;
             
         case POLYCALL_POOL_STRATEGY_ROUND_ROBIN:
             // Start from next_index and wrap around
             for (uint32_t count = 0; count < pool_ctx->pool_size; count++) {
                 uint32_t i = (pool_ctx->next_index + count) % pool_ctx->pool_size;
                 if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE) {
                     candidate_idx = i;
                     pool_ctx->next_index = (i + 1) % pool_ctx->pool_size;
                     found = true;
                     break;
                 }
             }
             break;
     }
     
     // Check cooling connections if no idle ones found
     if (!found) {
         for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
             if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_COOLING) {
                 uint64_t cooling_time = current_time - pool_ctx->connections[i].last_used_time;
                 
                 // Check if cooldown period has elapsed
                 if (cooling_time >= pool_ctx->config.connection_cooldown_ms) {
                     candidate_idx = i;
                     found = true;
                     break;
                 }
             }
         }
     }
     
     return candidate_idx;
 }
 
 /**
  * @brief Create a new connection in the pool
  */
 static polycall_core_error_t create_new_connection(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     uint32_t index
 ) {
     // Allocate protocol context
     polycall_protocol_context_t* proto_ctx = 
         polycall_core_malloc(core_ctx, sizeof(polycall_protocol_context_t));
     
     if (!proto_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize protocol context (placeholder)
     // In a real implementation, you would use network endpoints and proper initialization
     bool init_success = true; // Placeholder
     
     if (!init_success) {
         polycall_core_free(core_ctx, proto_ctx);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Update connection entry
     polycall_connection_entry_t* entry = &pool_ctx->connections[index];
     entry->proto_ctx = proto_ctx;
     entry->state = POLYCALL_CONN_STATE_IDLE;
     entry->creation_time = get_timestamp_ms();
     entry->last_used_time = entry->creation_time;
     entry->last_validated_time = entry->creation_time;
     entry->request_count = 0;
     entry->is_valid = true;
     
     // Update statistics
     pool_ctx->stats.total_connections++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Acquire a connection from the pool
  */
 polycall_core_error_t polycall_connection_pool_acquire(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     uint32_t timeout_ms,
     polycall_protocol_context_t** proto_ctx
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC || !proto_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Start time for timeout tracking
     uint64_t start_time = get_timestamp_ms();
     uint64_t end_time = start_time + timeout_ms;
     uint64_t current_time = start_time;
     bool timed_out = false;
     
     // Try to find an available connection
     int connection_idx = find_available_connection(pool_ctx);
     
     // If no connection available, wait until one becomes available or timeout
     while (connection_idx < 0 && !timed_out) {
         // Check if we can create a new connection
         if (pool_ctx->pool_size < pool_ctx->config.max_pool_size) {
             connection_idx = pool_ctx->pool_size++;
             
             // Update statistics
             if (pool_ctx->pool_size > pool_ctx->stats.peak_connections) {
                 pool_ctx->stats.peak_connections = pool_ctx->pool_size;
             }
             
             if (pool_ctx->config.enable_auto_scaling) {
                 pool_ctx->stats.scaling_events++;
             }
             
             // Break out of the loop to create connection
             break;
         }
         
         // Calculate remaining timeout
         if (timeout_ms > 0) {
             current_time = get_timestamp_ms();
             if (current_time >= end_time) {
                 timed_out = true;
                 break;
             }
             
             // Wait with timeout
             struct timespec ts;
             clock_gettime(CLOCK_REALTIME, &ts);
             ts.tv_sec += (end_time - current_time) / 1000;
             ts.tv_nsec += ((end_time - current_time) % 1000) * 1000000;
             
             // Normalize nanoseconds
             if (ts.tv_nsec >= 1000000000) {
                 ts.tv_sec += 1;
                 ts.tv_nsec -= 1000000000;
             }
             
             pthread_cond_timedwait(&pool_ctx->available_cond, &pool_ctx->pool_mutex, &ts);
         } else if (timeout_ms == 0) {
             // Non-blocking mode
             timed_out = true;
             break;
         } else {
             // Infinite wait
             pthread_cond_wait(&pool_ctx->available_cond, &pool_ctx->pool_mutex);
         }
         
         // Try again to find an available connection
         connection_idx = find_available_connection(pool_ctx);
     }
     
     // Handle timeout
     if (timed_out) {
         pthread_mutex_unlock(&pool_ctx->pool_mutex);
         return POLYCALL_CORE_ERROR_TIMEOUT;
     }
     
     // Get connection entry
     polycall_connection_entry_t* entry = &pool_ctx->connections[connection_idx];
     
     // Create new connection if necessary
     if (!entry->proto_ctx) {
         polycall_core_error_t result = create_new_connection(core_ctx, pool_ctx, connection_idx);
         if (result != POLYCALL_CORE_SUCCESS) {
             pthread_mutex_unlock(&pool_ctx->pool_mutex);
             return result;
         }
     }
     
     // Check if cooling connection needs validation
     if (entry->state == POLYCALL_CONN_STATE_COOLING && 
         pool_ctx->config.validate_on_return) {
         
         bool is_valid = validate_connection(core_ctx, entry);
         if (!is_valid) {
             // Close and recreate connection
             polycall_protocol_cleanup(entry->proto_ctx);
             polycall_core_free(core_ctx, entry->proto_ctx);
             
             polycall_core_error_t result = create_new_connection(core_ctx, pool_ctx, connection_idx);
             if (result != POLYCALL_CORE_SUCCESS) {
                 pthread_mutex_unlock(&pool_ctx->pool_mutex);
                 return result;
             }
         }
     }
     
     // Mark connection as active
     entry->state = POLYCALL_CONN_STATE_ACTIVE;
     entry->last_used_time = get_timestamp_ms();
     entry->request_count++;
     
     // Update statistics
     pool_ctx->active_count++;
     pool_ctx->stats.total_requests++;
     pool_ctx->stats.active_connections = pool_ctx->active_count;
     pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
     pool_ctx->stats.utilization_rate = (float)pool_ctx->active_count / pool_ctx->pool_size;
     
     if (current_time > start_time) {
         pool_ctx->stats.total_wait_time += current_time - start_time;
     }
     
     // Return the protocol context
     *proto_ctx = entry->proto_ctx;
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Release a connection back to the pool
  */
 polycall_core_error_t polycall_connection_pool_release(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     polycall_protocol_context_t* proto_ctx,
     bool force_close
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC || !proto_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Find the connection entry
     int index = -1;
     for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
         if (pool_ctx->connections[i].proto_ctx == proto_ctx) {
             index = i;
             break;
         }
     }
     
     // Connection not found in pool
     if (index < 0) {
         pthread_mutex_unlock(&pool_ctx->pool_mutex);
         return POLYCALL_CORE_ERROR_NOT_FOUND;
     }
     
     polycall_connection_entry_t* entry = &pool_ctx->connections[index];
     
     // Check if connection was active
     if (entry->state != POLYCALL_CONN_STATE_ACTIVE) {
         pthread_mutex_unlock(&pool_ctx->pool_mutex);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Calculate connection time
     uint64_t current_time = get_timestamp_ms();
     uint64_t conn_time = current_time - entry->last_used_time;
     pool_ctx->stats.total_connection_time += conn_time;
     
     // Check max requests per connection threshold
     bool should_close = force_close;
     if (!should_close && pool_ctx->config.max_requests_per_connection > 0) {
         should_close = (entry->request_count >= pool_ctx->config.max_requests_per_connection);
     }
     
     // Check idle timeout
     if (!should_close && pool_ctx->config.idle_timeout_ms > 0) {
         should_close = (conn_time >= pool_ctx->config.idle_timeout_ms);
     }
     
     if (should_close) {
         // Close the connection
         polycall_protocol_cleanup(entry->proto_ctx);
         polycall_core_free(core_ctx, entry->proto_ctx);
         
         // Create a new connection
         create_new_connection(core_ctx, pool_ctx, index);
     } else {
         // Return connection to the pool
         entry->state = pool_ctx->config.connection_cooldown_ms > 0 ? 
                       POLYCALL_CONN_STATE_COOLING : POLYCALL_CONN_STATE_IDLE;
     }
     
     // Update statistics
     pool_ctx->active_count--;
     pool_ctx->stats.active_connections = pool_ctx->active_count;
     pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
     pool_ctx->stats.utilization_rate = (float)pool_ctx->active_count / pool_ctx->pool_size;
     
     // Auto-scale down if utilization is too low
     if (pool_ctx->config.enable_auto_scaling && 
         pool_ctx->stats.utilization_rate < (pool_ctx->config.scaling_threshold / 2) &&
         pool_ctx->pool_size > pool_ctx->config.min_pool_size) {
         
         // Find an idle connection to remove
         for (int i = pool_ctx->pool_size - 1; i >= 0; i--) {
             if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE ||
                 pool_ctx->connections[i].state == POLYCALL_CONN_STATE_COOLING) {
                 
                 // Close the connection
                 polycall_protocol_cleanup(pool_ctx->connections[i].proto_ctx);
                 polycall_core_free(core_ctx, pool_ctx->connections[i].proto_ctx);
                 
                 // Move the last connection to this slot
                 if (i < (int)(pool_ctx->pool_size - 1)) {
                     memcpy(&pool_ctx->connections[i], 
                            &pool_ctx->connections[pool_ctx->pool_size - 1],
                            sizeof(polycall_connection_entry_t));
                 }
                 
                 // Decrease pool size
                 pool_ctx->pool_size--;
                 pool_ctx->stats.scaling_events++;
                 
                 // Update statistics
                 pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
                 break;
             }
         }
     }
     
     // Signal waiting threads
     pthread_cond_signal(&pool_ctx->available_cond);
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get connection pool statistics
  */
 polycall_core_error_t polycall_connection_pool_get_stats(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     polycall_connection_pool_stats_t* stats
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC || !stats) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Copy statistics
     memcpy(stats, &pool_ctx->stats, sizeof(polycall_connection_pool_stats_t));
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Adjust connection pool size
  */
 polycall_core_error_t polycall_connection_pool_resize(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     uint32_t new_size
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (new_size > pool_ctx->config.max_pool_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Handle pool size reduction
     if (new_size < pool_ctx->pool_size) {
         // Attempt to close idle connections
         uint32_t connections_to_close = pool_ctx->pool_size - new_size;
         uint32_t closed_count = 0;
         
         for (int i = pool_ctx->pool_size - 1; i >= 0 && closed_count < connections_to_close; i--) {
             if (pool_ctx->connections[i].state == POLYCALL_CONN_STATE_IDLE ||
                 pool_ctx->connections[i].state == POLYCALL_CONN_STATE_COOLING) {
                 
                 // Close the connection
                 polycall_protocol_cleanup(pool_ctx->connections[i].proto_ctx);
                 polycall_core_free(core_ctx, pool_ctx->connections[i].proto_ctx);
                 
                 // Move the last connection to this slot
                 if (i < (int)(pool_ctx->pool_size - 1)) {
                     memcpy(&pool_ctx->connections[i], 
                            &pool_ctx->connections[pool_ctx->pool_size - 1],
                            sizeof(polycall_connection_entry_t));
                 }
                 
                 // Decrease pool size
                 pool_ctx->pool_size--;
                 closed_count++;
             }
         }
         
         // Update statistics
         pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
         pool_ctx->stats.utilization_rate = (float)pool_ctx->active_count / pool_ctx->pool_size;
     }
     // Handle pool size increase
     else if (new_size > pool_ctx->pool_size) {
         // Add connections up to new_size
         uint32_t old_size = pool_ctx->pool_size;
         pool_ctx->pool_size = new_size;
         
         // Initialize new connections
         for (uint32_t i = old_size; i < new_size; i++) {
             create_new_connection(core_ctx, pool_ctx, i);
         }
         
         // Update statistics
         pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
         pool_ctx->stats.utilization_rate = (float)pool_ctx->active_count / pool_ctx->pool_size;
         
         // Update peak connections if needed
         if (pool_ctx->pool_size > pool_ctx->stats.peak_connections) {
             pool_ctx->stats.peak_connections = pool_ctx->pool_size;
         }
         
         // Signal waiting threads
         pthread_cond_broadcast(&pool_ctx->available_cond);
     }
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Validate connections in the pool
  */
 polycall_core_error_t polycall_connection_pool_validate(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     bool close_invalid
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
         polycall_connection_entry_t* entry = &pool_ctx->connections[i];
         
         // Skip active connections
         if (entry->state == POLYCALL_CONN_STATE_ACTIVE) {
             continue;
         }
         
         // Validate connection
         bool is_valid = validate_connection(core_ctx, entry);
         entry->is_valid = is_valid;
         
         // Handle invalid connections
         if (!is_valid && close_invalid) {
             // Close and recreate connection
             polycall_protocol_cleanup(entry->proto_ctx);
             polycall_core_free(core_ctx, entry->proto_ctx);
             
             create_new_connection(core_ctx, pool_ctx, i);
             
             // Update statistics
             pool_ctx->stats.connection_failures++;
         }
     }
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default connection pool configuration
  */
 polycall_connection_pool_config_t polycall_connection_pool_default_config(void) {
     polycall_connection_pool_config_t config;
     
     config.initial_pool_size = 4;
     config.max_pool_size = 16;
     config.min_pool_size = 2;
     config.connection_timeout_ms = 30000;     // 30 seconds
     config.idle_timeout_ms = 300000;          // 5 minutes
     config.max_requests_per_connection = 1000;
     config.strategy = POLYCALL_POOL_STRATEGY_LRU;
     config.enable_auto_scaling = true;
     config.scaling_threshold = 0.75f;
     config.connection_cooldown_ms = 1000;     // 1 second
     config.validate_on_return = true;
     
     return config;
 }
 
 /**
  * @brief Set pool allocation strategy
  */
 polycall_core_error_t polycall_connection_pool_set_strategy(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     polycall_pool_strategy_t strategy
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Update strategy
     pool_ctx->config.strategy = strategy;
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Prefetch and warm up connections
  */
 polycall_core_error_t polycall_connection_pool_warm_up(
     polycall_core_context_t* core_ctx,
     polycall_connection_pool_context_t* pool_ctx,
     uint32_t count
 ) {
     if (!core_ctx || !pool_ctx || pool_ctx->magic != POLYCALL_CONNECTION_POOL_MAGIC) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (count > pool_ctx->config.max_pool_size) {
         count = pool_ctx->config.max_pool_size;
     }
     
     pthread_mutex_lock(&pool_ctx->pool_mutex);
     
     // Ensure pool size is at least count
     if (pool_ctx->pool_size < count) {
         uint32_t old_size = pool_ctx->pool_size;
         pool_ctx->pool_size = count;
         
         // Initialize new connections
         for (uint32_t i = old_size; i < count; i++) {
             create_new_connection(core_ctx, pool_ctx, i);
         }
         
         // Update statistics
         pool_ctx->stats.idle_connections = pool_ctx->pool_size - pool_ctx->active_count;
         pool_ctx->stats.utilization_rate = (float)pool_ctx->active_count / pool_ctx->pool_size;
         
         // Update peak connections if needed
         if (pool_ctx->pool_size > pool_ctx->stats.peak_connections) {
             pool_ctx->stats.peak_connections = pool_ctx->pool_size;
         }
     }
     
     // Warm up existing connections by performing a validation
     for (uint32_t i = 0; i < pool_ctx->pool_size; i++) {
         validate_connection(core_ctx, &pool_ctx->connections[i]);
     }
     
     pthread_mutex_unlock(&pool_ctx->pool_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
