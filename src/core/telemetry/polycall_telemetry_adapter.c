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

// polycall/core/telemetry/polycall_telemetry_adapter.c
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/telemetry/polycall_telemetry_adapter.h"
#include <string.h>

// Legacy function implementation using new identifier system
bool polycall_guid_validate(polycall_core_context_t *core_ctx,
                            const char *guid_str) {

  if (!core_ctx || !guid_str) {
    return false;
  }

  polycall_identifier_t identifier;
  polycall_core_error_t result =
      polycall_identifier_from_string(core_ctx, &identifier, guid_str);

  return (result == POLYCALL_CORE_SUCCESS);
}

char *polycall_update_guid_state(polycall_core_context_t *core_ctx,
                                 const char *parent_guid, uint32_t state_id,
                                 uint32_t event_id) {

  if (!core_ctx || !parent_guid) {
    return NULL;
  }

  // Parse parent GUID
  polycall_identifier_t parent_id;
  if (polycall_identifier_from_string(core_ctx, &parent_id, parent_guid) !=
      POLYCALL_CORE_SUCCESS) {
    return NULL;
  }

  // Create new identifier with updated state
  polycall_identifier_t new_id;
  if (polycall_identifier_update_state(core_ctx, &new_id, &parent_id, state_id,
                                       event_id) != POLYCALL_CORE_SUCCESS) {
    return NULL;
  }

  // Allocate and return string representation
  char *result = polycall_core_malloc(core_ctx, POLYCALL_MAX_ID_LEN);
  if (!result) {
    return NULL;
  }

  strncpy(result, new_id.string, POLYCALL_MAX_ID_LEN - 1);
  result[POLYCALL_MAX_ID_LEN - 1] = '\0';

  return result;
}

char *polycall_generate_cryptonomic_guid(polycall_core_context_t *core_ctx,
                                         const char *namespace_id,
                                         uint32_t state_id,
                                         const char *entity_id) {

  if (!core_ctx) {
    return NULL;
  }

  // Generate cryptonomic identifier
  polycall_identifier_t identifier;
  if (polycall_identifier_generate_cryptonomic(
          core_ctx, &identifier, namespace_id, state_id, entity_id) !=
      POLYCALL_CORE_SUCCESS) {
    return NULL;
  }

  // Allocate and return string representation
  char *result = polycall_core_malloc(core_ctx, POLYCALL_MAX_ID_LEN);
  if (!result) {
    return NULL;
  }

  strncpy(result, identifier.string, POLYCALL_MAX_ID_LEN - 1);
  result[POLYCALL_MAX_ID_LEN - 1] = '\0';

  return result;
}

// New method to set identifier format preference
polycall_core_error_t polycall_telemetry_set_identifier_format(
    polycall_core_context_t *core_ctx,
    polycall_telemetry_context_t *telemetry_ctx,
    polycall_identifier_format_t format) {

  if (!core_ctx || !telemetry_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
  }

  // Update telemetry context with preferred format
  telemetry_ctx->id_format = format;

  return POLYCALL_CORE_SUCCESS;
}
