/**
 * @file connection_pool.h
 * @brief Connection Pool Interface for LibPolyCall Protocol
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Defines interfaces for efficient connection management with dynamic scaling,
 * load balancing, and resource optimization for high-volume scenarios.
 */

#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/network/network.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/polycall/polycall_core.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/protocol/polycall_protocol_context.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

#ifndef POLYCALL_NETWORK_CONNECTION_POOL_H_H
#define POLYCALL_NETWORK_CONNECTION_POOL_H_H

#ifdef __cplusplus
extern "C" {
#endif

#define POLYCALL_NETWORK_CONNECTION_POOL_H_H

/**
 * @brief Connection state enumeration
 */
typedef enum {
  POLYCALL_CONN_STATE_IDLE,    // Connection is idle
  POLYCALL_CONN_STATE_ACTIVE,  // Connection is active
  POLYCALL_CONN_STATE_COOLING, // Connection is cooling down
  POLYCALL_CONN_STATE_ERROR    // Connection is in error state
} polycall_connection_state_t;

/**
 * @brief Pool allocation strategy enumeration
 */
typedef enum {
  POLYCALL_POOL_STRATEGY_FIFO,       // First in, first out
  POLYCALL_POOL_STRATEGY_LIFO,       // Last in, first out
  POLYCALL_POOL_STRATEGY_LRU,        // Least recently used
  POLYCALL_POOL_STRATEGY_ROUND_ROBIN // Round robin
} polycall_pool_strategy_t;

/**
 * @brief Configuration for connection pool
 */
typedef struct {
  uint32_t initial_pool_size;           // Initial number of connections
  uint32_t max_pool_size;               // Maximum number of connections
  uint32_t min_pool_size;               // Minimum number of connections
  uint32_t connection_timeout_ms;       // Connection timeout in milliseconds
  uint32_t idle_timeout_ms;             // Idle timeout in milliseconds
  uint32_t max_requests_per_connection; // Max requests per connection
  polycall_pool_strategy_t strategy;    // Allocation strategy
  bool enable_auto_scaling;             // Enable auto-scaling
  float scaling_threshold;              // Scaling threshold
  uint32_t connection_cooldown_ms;      // Cooldown period in milliseconds
  bool validate_on_return;              // Validate connections on return
} polycall_connection_pool_config_t;

/**
 * @brief Statistics for connection pool
 */
typedef struct {
  uint32_t total_connections;     // Total connections created
  uint32_t active_connections;    // Current active connections
  uint32_t idle_connections;      // Current idle connections
  uint32_t peak_connections;      // Peak number of connections
  uint32_t connection_failures;   // Number of connection failures
  uint32_t total_requests;        // Total requests processed
  uint64_t total_wait_time;       // Total wait time in milliseconds
  uint64_t total_connection_time; // Total connection time in milliseconds
  float utilization_rate;         // Current utilization rate
  uint32_t scaling_events;        // Number of scaling events
} polycall_connection_pool_stats_t;

/**
 * @brief Connection pool context (opaque)
 */
typedef struct polycall_connection_pool_context
    polycall_connection_pool_context_t;

/**
 * @brief Initialize connection pool
 *
 * @param core_ctx Core context
 * @param pool_ctx Pointer to store the created pool context
 * @param config Configuration for the pool
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_init(polycall_core_context_t *core_ctx,
                              polycall_connection_pool_context_t **pool_ctx,
                              const polycall_connection_pool_config_t *config);

/**
 * @brief Clean up connection pool
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context to clean up
 */
void polycall_connection_pool_cleanup(
    polycall_core_context_t *core_ctx,
    polycall_connection_pool_context_t *pool_ctx);

/**
 * @brief Acquire a connection from the pool
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param timeout_ms Timeout in milliseconds (0 for non-blocking, UINT32_MAX for
 * infinite)
 * @param proto_ctx Pointer to store the acquired protocol context
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_acquire(polycall_core_context_t *core_ctx,
                                 polycall_connection_pool_context_t *pool_ctx,
                                 uint32_t timeout_ms,
                                 polycall_protocol_context_t **proto_ctx);

/**
 * @brief Release a connection back to the pool
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param proto_ctx Protocol context to release
 * @param force_close Force connection close
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_release(polycall_core_context_t *core_ctx,
                                 polycall_connection_pool_context_t *pool_ctx,
                                 polycall_protocol_context_t *proto_ctx,
                                 bool force_close);

/**
 * @brief Get connection pool statistics
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param stats Pointer to store statistics
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_get_stats(polycall_core_context_t *core_ctx,
                                   polycall_connection_pool_context_t *pool_ctx,
                                   polycall_connection_pool_stats_t *stats);

/**
 * @brief Adjust connection pool size
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param new_size New pool size
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_resize(polycall_core_context_t *core_ctx,
                                polycall_connection_pool_context_t *pool_ctx,
                                uint32_t new_size);

/**
 * @brief Validate connections in the pool
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param close_invalid Close invalid connections
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_validate(polycall_core_context_t *core_ctx,
                                  polycall_connection_pool_context_t *pool_ctx,
                                  bool close_invalid);

/**
 * @brief Create default connection pool configuration
 *
 * @return polycall_connection_pool_config_t Default configuration
 */
polycall_connection_pool_config_t polycall_connection_pool_default_config(void);

/**
 * @brief Set pool allocation strategy
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param strategy New allocation strategy
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_connection_pool_set_strategy(
    polycall_core_context_t *core_ctx,
    polycall_connection_pool_context_t *pool_ctx,
    polycall_pool_strategy_t strategy);

/**
 * @brief Prefetch and warm up connections
 *
 * @param core_ctx Core context
 * @param pool_ctx Pool context
 * @param count Number of connections to warm up
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t
polycall_connection_pool_warm_up(polycall_core_context_t *core_ctx,
                                 polycall_connection_pool_context_t *pool_ctx,
                                 uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_NETWORK_CONNECTION_POOL_H_H */
