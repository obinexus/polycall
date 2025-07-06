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
 * @file mock_core_context.h
 * @brief Mock implementation of core context for testing
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file provides mock implementations of the core context functionality
 * for use in unit tests of protocol enhancements.
 */

#ifndef MOCK_CORE_CONTEXT_H
#define MOCK_CORE_CONTEXT_H

#include "polycall/core/polycall/polycall_core.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a mock core context for testing
 *
 * @return A mock core context instance
 */
polycall_core_context_t *mock_core_context_create(void);

/**
 * @brief Destroy a mock core context
 *
 * @param ctx The mock core context to destroy
 */
void mock_core_context_destroy(polycall_core_context_t *ctx);

/**
 * @brief Mock implementation of polycall_core_malloc
 */
void *mock_core_malloc(polycall_core_context_t *ctx, size_t size);

/**
 * @brief Mock implementation of polycall_core_free
 */
void mock_core_free(polycall_core_context_t *ctx, void *ptr);

/**
 * @brief Mock implementation of polycall_core_realloc
 */
void *mock_core_realloc(polycall_core_context_t *ctx, void *ptr, size_t size);

#endif /* MOCK_CORE_CONTEXT_H */
