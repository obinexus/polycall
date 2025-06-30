// polycall/core/common/polycall_identifier.h
#ifndef POLYCALL_IDENTIFIER_H
#define POLYCALL_IDENTIFIER_H

#include "polycall/core/polycall/polycall_types.h"
#include "polycall/core/polycall/polycall_error.h"

#define POLYCALL_GUID_LEN 36  // Standard GUID string length including hyphens
#define POLYCALL_UUID_LEN 36  // Standard UUID string length including hyphens
#define POLYCALL_MAX_ID_LEN 40 // Allow for null terminator and format variations

typedef enum {
    POLYCALL_ID_FORMAT_GUID = 0,  // Microsoft GUID format (hyphenated, uppercase)
    POLYCALL_ID_FORMAT_UUID = 1,  // RFC 4122 UUID format (hyphenated, lowercase)
    POLYCALL_ID_FORMAT_COMPACT = 2, // Non-hyphenated compact representation
    POLYCALL_ID_FORMAT_CRYPTONOMIC = 3 // Secure cryptographic identifier
} polycall_identifier_format_t;

typedef struct {
    unsigned char bytes[16];  // Internal binary representation - 128 bits
    char string[POLYCALL_MAX_ID_LEN];  // String representation
    polycall_identifier_format_t format;  // Current format
} polycall_identifier_t;

// Core API Functions
polycall_core_error_t polycall_identifier_create(
    polycall_core_context_t* core_ctx,
    polycall_identifier_t* identifier,
    polycall_identifier_format_t format);

polycall_core_error_t polycall_identifier_from_string(
    polycall_core_context_t* core_ctx,
    polycall_identifier_t* identifier,
    const char* id_string);

polycall_core_error_t polycall_identifier_to_string(
    polycall_core_context_t* core_ctx,
    const polycall_identifier_t* identifier,
    char* buffer,
    size_t buffer_size,
    polycall_identifier_format_t output_format);

polycall_core_error_t polycall_identifier_validate(
    polycall_core_context_t* core_ctx,
    const polycall_identifier_t* identifier);

// Cryptonomic functions (secure identifiers with state tracking)
polycall_core_error_t polycall_identifier_generate_cryptonomic(
    polycall_core_context_t* core_ctx,
    polycall_identifier_t* identifier,
    const char* namespace_id,
    uint32_t state_id,
    const char* entity_id);

polycall_core_error_t polycall_identifier_update_state(
    polycall_core_context_t* core_ctx,
    polycall_identifier_t* identifier,
    const polycall_identifier_t* parent_id,
    uint32_t state_id,
    uint32_t event_id);

#endif // POLYCALL_IDENTIFIER_H