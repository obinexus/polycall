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
 * @file protocol_test_stub.c
 * @brief Test stub implementation for protocol component
 * @author LibPolyCall Implementation Team
 */

#include "polycall_test_framework.h"
#include "protocol_test_stub.h"

/**
 * @brief Initialize protocol test stubs
 *
 * @return int 0 on success, non-zero on failure
 */
int polycall_protocol_init_test_stubs(void) {
  // Initialize any required mock objects or stubs
  return 0;
}

/**
 * @brief Clean up protocol test stubs
 */
void polycall_protocol_cleanup_test_stubs(void) {
  // Clean up any allocated resources
}
