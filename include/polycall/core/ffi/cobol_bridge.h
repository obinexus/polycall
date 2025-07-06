/**
 * @file cobol_bridge.h
 * @brief COBOL language bridge for LibPolyCall FFI
 *
 * This header defines the COBOL language bridge for LibPolyCall FFI, providing
 * an interface for interfacing with COBOL programs through the FFI system.
 */

#ifndef POLYCALL_FFI_COBOL_BRIDGE_H_H
#define POLYCALL_FFI_COBOL_BRIDGE_H_H

#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
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
  const char *runtime_path;         /**< Path to COBOL runtime */
  const char *program_path;         /**< Path to COBOL programs */
  bool enable_direct_calls;         /**< Enable direct COBOL program calls */
  bool enable_copybook_integration; /**< Enable COBOL copybook parsing for type
                                       mapping */
  size_t max_record_size; /**< Maximum record size for data transfer */
  void *user_data;        /**< User data */
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
polycall_core_error_t
polycall_cobol_bridge_init(polycall_core_context_t *ctx,
                           polycall_ffi_context_t *ffi_ctx,
                           polycall_cobol_bridge_t **cobol_bridge,
                           const polycall_cobol_bridge_config_t *config);

/**
 * @brief Clean up COBOL language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param cobol_bridge COBOL bridge handle to clean up
 */
void polycall_cobol_bridge_cleanup(polycall_core_context_t *ctx,
                                   polycall_ffi_context_t *ffi_ctx,
                                   polycall_cobol_bridge_t *cobol_bridge);

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
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_cobol_bridge_t *cobol_bridge, const char *function_name,
    const char *program_name, const char *linkage_section_desc, uint32_t flags);

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
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_cobol_bridge_t *cobol_bridge, const char *function_name,
    ffi_value_t *args, size_t arg_count, ffi_value_t *result);

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
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_cobol_bridge_t *cobol_bridge, const char *copybook_path,
    const char *record_name, ffi_type_info_t **type_info);

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
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_cobol_bridge_t *cobol_bridge, language_bridge_t *bridge);

/**
 * @brief Create a default COBOL bridge configuration
 *
 * @return Default configuration
 */
polycall_cobol_bridge_config_t
polycall_cobol_bridge_create_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_COBOL_BRIDGE_H_H */