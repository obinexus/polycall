// polycall/core/telemetry/polycall_telemetry_adapter.h
#ifndef POLYCALL_TELEMETRY_ADAPTER_H
#define POLYCALL_TELEMETRY_ADAPTER_H

#include "polycall/core/common/polycall_identifier.h"
#include "polycall/core/telemetry/polycall_telemetry.h"

// Legacy adapter functions
bool polycall_guid_validate(polycall_core_context_t *core_ctx,
                            const char *guid_str);

char *polycall_update_guid_state(polycall_core_context_t *core_ctx,
                                 const char *parent_guid, uint32_t state_id,
                                 uint32_t event_id);

char *polycall_generate_cryptonomic_guid(polycall_core_context_t *core_ctx,
                                         const char *namespace_id,
                                         uint32_t state_id,
                                         const char *entity_id);

// New method to update telemetry context with identifier preference
polycall_core_error_t polycall_telemetry_set_identifier_format(
    polycall_core_context_t *core_ctx,
    polycall_telemetry_context_t *telemetry_ctx,
    polycall_identifier_format_t format);

#endif // POLYCALL_TELEMETRY_ADAPTER_H