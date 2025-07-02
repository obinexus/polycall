# COBOL Bridge Implementation Plan for LibPolyCall FFI

## Overview

The COBOL bridge would enable LibPolyCall to interface with COBOL programs, particularly legacy financial systems. This bridge must follow the established patterns of the existing language bridges while addressing COBOL-specific considerations.

## Architecture Components

### 1. COBOL Bridge Header (`cobol_bridge.h`)

This header will define the public interface for the COBOL bridge:

```c
/**
 * @file cobol_bridge.h
 * @brief COBOL language bridge for LibPolyCall FFI
 *
 * This header defines the COBOL language bridge for LibPolyCall FFI, providing
 * an interface for interfacing with COBOL programs through the FFI system.
 */

#ifndef POLYCALL_COBOL_BRIDGE_H
#define POLYCALL_COBOL_BRIDGE_H

#include "ffi_core.h"
#include "../polycall/polycall_core.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief COBOL bridge handle (opaque)
 */
typedef struct polycall_cobol_bridge polycall_cobol_bridge_t;

/**
 * @brief COBOL bridge configuration
 */
typedef struct {
    const char* runtime_path;           /**< Path to COBOL runtime */
    const char* program_path;           /**< Path to COBOL programs */
    bool enable_direct_calls;           /**< Enable direct COBOL program calls */
    bool enable_copybook_integration;   /**< Enable COBOL copybook parsing for type mapping */
    size_t max_record_size;             /**< Maximum record size for data transfer */
    void* user_data;                    /**< User data */
} polycall_cobol_bridge_config_t;

/**
 * @brief Initialize the COBOL language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge Pointer to receive COBOL bridge handle
 * @param config Bridge configuration
 * @return Error code
 */
polycall_core_error_t polycall_cobol_bridge_init(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t** cobol_bridge,
    const polycall_cobol_bridge_config_t* config
);

/**
 * @brief Clean up COBOL language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle to clean up
 */
void polycall_cobol_bridge_cleanup(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t* cobol_bridge
);

/**
 * @brief Register a COBOL program with the FFI system
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle
 * @param function_name Function name for FFI
 * @param program_name COBOL program name
 * @param linkage_section_desc Linkage section descriptor
 * @param flags Function flags
 * @return Error code
 */
polycall_core_error_t polycall_cobol_bridge_register_program(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t* cobol_bridge,
    const char* function_name,
    const char* program_name,
    const char* linkage_section_desc,
    uint32_t flags
);

/**
 * @brief Call a COBOL program through the FFI system
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle
 * @param function_name Function name
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Pointer to receive function result
 * @return Error code
 */
polycall_core_error_t polycall_cobol_bridge_call_program(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t* cobol_bridge,
    const char* function_name,
    ffi_value_t* args,
    size_t arg_count,
    ffi_value_t* result
);

/**
 * @brief Parse COBOL copybook for type mapping
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle
 * @param copybook_path Path to COBOL copybook file
 * @param record_name Record name to extract
 * @param type_info Pointer to receive type information
 * @return Error code
 */
polycall_core_error_t polycall_cobol_bridge_parse_copybook(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t* cobol_bridge,
    const char* copybook_path,
    const char* record_name,
    ffi_type_info_t** type_info
);

/**
 * @brief Get language bridge interface for COBOL
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle
 * @param bridge Pointer to receive language bridge interface
 * @return Error code
 */
polycall_core_error_t polycall_cobol_bridge_get_interface(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    polycall_cobol_bridge_t* cobol_bridge,
    language_bridge_t* bridge
);

/**
 * @brief Create a default COBOL bridge configuration
 *
 * @return Default configuration
 */
polycall_cobol_bridge_config_t polycall_cobol_bridge_create_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_COBOL_BRIDGE_H */
```

### 2. COBOL Bridge Implementation (`cobol_bridge.c`)

The implementation will need to handle:

1. **COBOL Runtime Integration**: Interface with a COBOL runtime (e.g., GnuCOBOL, Micro Focus, IBM Enterprise COBOL)
2. **Type Mapping**: Convert between COBOL data types and FFI types
3. **Memory Management**: Handle COBOL's record-based memory model
4. **Program Invocation**: Call COBOL programs and retrieve results
5. **Error Handling**: Map COBOL-specific errors to FFI errors

### 3. COBOL-Specific Challenges

- **Record Layouts**: COBOL uses fixed record layouts that must be mapped to C structures
- **Numeric Types**: COBOL has unique numeric formats (COMP, COMP-3, etc.)
- **String Handling**: COBOL strings are fixed-length and space-padded
- **Subprogram Calling**: Different calling conventions than C
- **File I/O**: COBOL programs often use direct file I/O rather than returns

### 4. Integration with Type System

The type system must be extended to support COBOL data types:

```c
// COBOL-specific type conversions
static polycall_core_error_t register_cobol_types(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    type_mapping_context_t* type_ctx
) {
    // Register DISPLAY numeric type
    ffi_type_info_t display_numeric;
    memset(&display_numeric, 0, sizeof(display_numeric));
    display_numeric.type = POLYCALL_FFI_TYPE_CUSTOM;
    display_numeric.name = "DISPLAY-NUMERIC";
    // Set up custom conversion handlers
    
    // Register COMP-3 (packed decimal)
    ff