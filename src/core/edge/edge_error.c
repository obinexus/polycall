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
 * @file edge_error.c
 * @brief Error handling for edge module
 */

#include "polycall/core/edge/edge_error.h"

/**
 * Get error message for edge error code
 */
const char *edge_get_error_message(edge_error_t error) {
  switch (error) {
  case EDGE_ERROR_SUCCESS:
    return "Success";
  case EDGE_ERROR_INVALID_PARAMETERS:
    return "Invalid parameters";
  case EDGE_ERROR_OUT_OF_MEMORY:
    return "Out of memory";
  case EDGE_ERROR_NOT_INITIALIZED:
    return "Module not initialized";
  // Add component-specific error messages here
  default:
    return "Unknown error";
  }
}
