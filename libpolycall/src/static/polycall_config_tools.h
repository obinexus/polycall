/**
 * @file polycall_config_tools.h
 * @brief Unified configuration tools for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header provides unified access to both the REPL and DOCTOR components
 * of LibPolyCall, enabling seamless integration between interactive and
 * non-interactive configuration management.
 */

#ifndef POLYCALL_CONFIG_TOOLS_H
#define POLYCALL_CONFIG_TOOLS_H

#include "polycall/core/polycall/polycall_repl.h"
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/accessibility/accessibility_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration tools context (opaque)
 */
typedef struct polycall_config_tools_context polycall_config_tools_context_t;

/**
 * @brief Configuration tools configuration
 */
typedef struct {
    polycall_repl_config_t repl_config;         /**< REPL configuration */
    polycall_doctor_config_t doctor_config;     /**< DOCTOR configuration */
    bool enable_accessibility;                  /**< Enable accessibility features */
    polycall_accessibility_theme_t theme;       /**< Accessibility theme */
    bool auto_doctor_on_exit;                   /**< Run DOCTOR automatically on exit */
    bool confirm_dangerous_changes;             /**< Confirm changes that could break config */
    const char* default_config_path;            /**< Default configuration path */
    uint32_t flags;                            /**< Additional flags */
} polycall_config_tools_config_t;

/**
 * @brief Initialize configuration tools
 *
 * @param core_ctx Core context
 * @param tools_ctx Pointer to receive tools context
 * @param config Configuration
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_init(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t** tools_ctx,
    const polycall_config_tools_config_t* config
);

/**
 * @brief Clean up configuration tools
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 */
void polycall_config_tools_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
);

/**
 * @brief Run REPL
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_run_repl(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
);

/**
 * @brief Run DOCTOR
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param fix_issues Whether to automatically fix issues
 * @param report_path Path to save report (NULL for no report)
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_run_doctor(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    bool fix_issues,
    const char* report_path
);

/**
 * @brief Get REPL context
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @return REPL context, or NULL on error
 */
polycall_repl_context_t* polycall_config_tools_get_repl(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
);

/**
 * @brief Get DOCTOR context
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @return DOCTOR context, or NULL on error
 */
polycall_doctor_context_t* polycall_config_tools_get_doctor(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
);

/**
 * @brief Get config context
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @return Configuration context, or NULL on error
 */
polycall_config_context_t* polycall_config_tools_get_config(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
);

/**
 * @brief Set config context
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param config_ctx Configuration context
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_set_config(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    polycall_config_context_t* config_ctx
);

/**
 * @brief Create default configuration tools configuration
 *
 * @return Default configuration
 */
polycall_config_tools_config_t polycall_config_tools_default_config(void);

/**
 * @brief Load configuration from file and validate
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param file_path Configuration file path
 * @param validate Whether to validate after loading
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_load_and_validate(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    const char* file_path,
    bool validate
);

/**
 * @brief Save configuration to file
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param file_path Configuration file path
 * @param validate Whether to validate before saving
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_save(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    const char* file_path,
    bool validate
);

/**
 * @brief Import configuration from another format
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param file_path File to import
 * @param format Format (json, yaml, xml, ini)
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_import(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    const char* file_path,
    const char* format
);

/**
 * @brief Export configuration to another format
 *
 * @param core_ctx Core context
 * @param tools_ctx Tools context
 * @param file_path File to export to
 * @param format Format (json, yaml, xml, ini)
 * @return Error code
 */
polycall_core_error_t polycall_config_tools_export(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx,
    const char* file_path,
    const char* format
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_TOOLS_H */
