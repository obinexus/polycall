/**
 * @file polycall_config_tools.c
 * @brief Unified configuration tools implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#include "polycall/core/config/polycall_config_tools.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration tools context
struct polycall_config_tools_context {
    polycall_core_context_t* core_ctx;
    polycall_config_context_t* config_ctx;
    polycall_repl_context_t* repl_ctx;
    polycall_doctor_context_t* doctor_ctx;
    polycall_accessibility_context_t* access_ctx;
    polycall_config_tools_config_t config;
};

// Initialize configuration tools
polycall_core_error_t polycall_config_tools_init(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t** tools_ctx,
    const polycall_config_tools_config_t* config
) {
    if (!core_ctx || !tools_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate context
    polycall_config_tools_context_t* new_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_config_tools_context_t));
    if (!new_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize context
    memset(new_ctx, 0, sizeof(polycall_config_tools_context_t));
    new_ctx->core_ctx = core_ctx;
    
    // Set configuration
    if (config) {
        memcpy(&new_ctx->config, config, sizeof(polycall_config_tools_config_t));
    } else {
        new_ctx->config = polycall_config_tools_default_config();
    }
    
    // Initialize accessibility if enabled
    if (new_ctx->config.enable_accessibility) {
        polycall_accessibility_config_t access_config = {
            .color_theme = new_ctx->config.theme,
            .high_contrast = false,
            .large_text = false,
            .screen_reader_support = false,
            .text_to_speech = false
        };
        
        polycall_core_error_t result = polycall_accessibility_init(
            core_ctx,
            &access_config,
            &new_ctx->access_ctx
        );
        
        if (result != POLYCALL_CORE_SUCCESS) {
            polycall_core_free(core_ctx, new_ctx);
            return result;
        }
    }
    
    // Initialize REPL
    polycall_core_error_t result = polycall_repl_init(
        core_ctx,
        &new_ctx->repl_ctx,
        &new_ctx->config.repl_config
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        if (new_ctx->access_ctx) {
            polycall_accessibility_cleanup(core_ctx, new_ctx->access_ctx);
        }
        polycall_core_free(core_ctx, new_ctx);
        return result;
    }
    
    // Initialize DOCTOR
    result = polycall_doctor_init(
        core_ctx,
        &new_ctx->doctor_ctx,
        &new_ctx->config.doctor_config
    );
    
    if (result != POLYCALL_CORE_SUCCESS) {
        polycall_repl_cleanup(core_ctx, new_ctx->repl_ctx);
        if (new_ctx->access_ctx) {
            polycall_accessibility_cleanup(core_ctx, new_ctx->access_ctx);
        }
        polycall_core_free(core_ctx, new_ctx);
        return result;
    }
    
    // Get config context from REPL
    new_ctx->config_ctx = polycall_repl_get_config_context(core_ctx, new_ctx->repl_ctx);
    
    *tools_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

// Clean up configuration tools
void polycall_config_tools_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_config_tools_context_t* tools_ctx
) {
    if (!core_ctx || !tools_ctx) {
        return;
    }
    
    // Auto-run DOCTOR on exit if enabled
    if (tools_ctx->config.auto_doctor_on_exit && tools_ctx->config_ctx) {
        polycall_doctor_validate(core_ctx, tools_ctx->doctor_ctx, tools_ctx->config_ctx);
        
        // Generate report if requested
        const char* report_path = tools_ctx->config.default_config_path;
        if (report_path) {
            char report_file[256];
            snprintf(report_file, sizeof(report_file), "%s.doctor-report.txt", report_path);
            polycall_doctor_generate_report(core_ctx, tools_ctx->doctor_ctx, 
                                          tools_ctx->config_ctx, report_file, "text");
        }
    }
    
  // Clean up DOCTOR
  if (tools_ctx->doctor_ctx) {
    polycall_doctor_cleanup(core_ctx, tools_ctx->doctor_ctx);
}

// Clean up REPL
if (tools_ctx->repl_ctx) {
    polycall_repl_cleanup(core_ctx, tools_ctx->repl_ctx);
}

// Clean up accessibility
if (tools_ctx->access_ctx) {
    polycall_accessibility_cleanup(core_ctx, tools_ctx->access_ctx);
}

// Free context
polycall_core_free(core_ctx, tools_ctx);
}

// Run REPL
polycall_core_error_t polycall_config_tools_run_repl(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx
) {
if (!core_ctx || !tools_ctx || !tools_ctx->repl_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Register DOCTOR command in REPL if not already registered
polycall_repl_command_handler_t doctor_handler = NULL;

// Run interactive REPL
return polycall_repl_run_interactive(core_ctx, tools_ctx->repl_ctx);
}

// Run DOCTOR
polycall_core_error_t polycall_config_tools_run_doctor(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
bool fix_issues,
const char* report_path
) {
if (!core_ctx || !tools_ctx || !tools_ctx->doctor_ctx || !tools_ctx->config_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Set auto-fix flag
tools_ctx->doctor_ctx->config.auto_fix = fix_issues;

// Run validation
polycall_core_error_t result = polycall_doctor_validate(
    core_ctx,
    tools_ctx->doctor_ctx,
    tools_ctx->config_ctx
);

if (result != POLYCALL_CORE_SUCCESS) {
    return result;
}

// Generate report if requested
if (report_path) {
    result = polycall_doctor_generate_report(
        core_ctx,
        tools_ctx->doctor_ctx,
        tools_ctx->config_ctx,
        report_path,
        "text"  // Default to text format
    );
}

return result;
}

// Get REPL context
polycall_repl_context_t* polycall_config_tools_get_repl(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx
) {
if (!core_ctx || !tools_ctx) {
    return NULL;
}

return tools_ctx->repl_ctx;
}

// Get DOCTOR context
polycall_doctor_context_t* polycall_config_tools_get_doctor(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx
) {
if (!core_ctx || !tools_ctx) {
    return NULL;
}

return tools_ctx->doctor_ctx;
}

// Get config context
polycall_config_context_t* polycall_config_tools_get_config(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx
) {
if (!core_ctx || !tools_ctx) {
    return NULL;
}

return tools_ctx->config_ctx;
}

// Set config context
polycall_core_error_t polycall_config_tools_set_config(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
polycall_config_context_t* config_ctx
) {
if (!core_ctx || !tools_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Set config context
tools_ctx->config_ctx = config_ctx;

// Update REPL's config context
if (tools_ctx->repl_ctx) {
    polycall_repl_set_config_context(core_ctx, tools_ctx->repl_ctx, config_ctx);
}

return POLYCALL_CORE_SUCCESS;
}

// Create default configuration tools configuration
polycall_config_tools_config_t polycall_config_tools_default_config(void) {
polycall_config_tools_config_t config;

// Initialize with default values
memset(&config, 0, sizeof(config));

// Set REPL defaults
config.repl_config.show_prompts = true;
config.repl_config.echo_commands = true;
config.repl_config.save_history = true;
config.repl_config.history_file = ".polycall_history";
config.repl_config.config_ctx = NULL;
config.repl_config.output_width = 80;
config.repl_config.color_output = true;
config.repl_config.verbose = false;

// Set DOCTOR defaults
config.doctor_config.auto_fix = false;
config.doctor_config.min_severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
config.doctor_config.rules_path = NULL;
config.doctor_config.validate_schema = true;
config.doctor_config.validate_security = true;
config.doctor_config.validate_performance = true;
config.doctor_config.validate_consistency = true;
config.doctor_config.validate_dependencies = true;
config.doctor_config.timeout_ms = 5000;

// Set tools defaults
config.enable_accessibility = true;
config.theme = 0;  // Default theme
config.auto_doctor_on_exit = false;
config.confirm_dangerous_changes = true;
config.default_config_path = "polycall.conf";
config.flags = 0;

return config;
}

// Load configuration from file and validate
polycall_core_error_t polycall_config_tools_load_and_validate(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
const char* file_path,
bool validate
) {
if (!core_ctx || !tools_ctx || !file_path) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Load configuration
polycall_core_error_t result = polycall_config_load(
    core_ctx,
    tools_ctx->config_ctx,
    file_path
);

if (result != POLYCALL_CORE_SUCCESS) {
    return result;
}

// Validate if requested
if (validate && tools_ctx->doctor_ctx) {
    result = polycall_doctor_validate(
        core_ctx,
        tools_ctx->doctor_ctx,
        tools_ctx->config_ctx
    );
    
    // We don't return validation errors, just log them
    if (result != POLYCALL_CORE_SUCCESS) {
        POLYCALL_LOG_WARNING(core_ctx, "Configuration validation failed: %d", result);
    }
}

return POLYCALL_CORE_SUCCESS;
}

// Save configuration to file
polycall_core_error_t polycall_config_tools_save(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
const char* file_path,
bool validate
) {
if (!core_ctx || !tools_ctx || !file_path || !tools_ctx->config_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Validate before saving if requested
if (validate && tools_ctx->doctor_ctx) {
    polycall_core_error_t validate_result = polycall_doctor_validate(
        core_ctx,
        tools_ctx->doctor_ctx,
        tools_ctx->config_ctx
    );
    
    // Check for critical issues
    if (validate_result != POLYCALL_CORE_SUCCESS) {
        // Get issues
        polycall_doctor_issue_t issues[16];
        uint32_t issue_count = 0;
        
        polycall_doctor_get_issues(
            core_ctx,
            tools_ctx->doctor_ctx,
            issues,
            16,
            &issue_count
        );
        
        // Check for critical issues
        bool has_critical_issues = false;
        for (uint32_t i = 0; i < issue_count; i++) {
            if (issues[i].severity == POLYCALL_DOCTOR_SEVERITY_CRITICAL) {
                has_critical_issues = true;
                break;
            }
        }
        
        // Abort if critical issues found and confirmation required
        if (has_critical_issues && tools_ctx->config.confirm_dangerous_changes) {
            POLYCALL_LOG_ERROR(core_ctx, "Critical configuration issues found, aborting save");
            return POLYCALL_CORE_ERROR_INVALID_STATE;
        }
    }
}

// Save configuration
return polycall_config_save(
    core_ctx,
    tools_ctx->config_ctx,
    file_path
);
}

// Import configuration from another format
polycall_core_error_t polycall_config_tools_import(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
const char* file_path,
const char* format
) {
if (!core_ctx || !tools_ctx || !file_path || !format || !tools_ctx->config_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Import logic depends on the available parsers in the system
// This is a placeholder implementation

// TODO: Implement different format importers
POLYCALL_LOG_WARNING(core_ctx, "Configuration import not implemented for format: %s", format);

return POLYCALL_CORE_ERROR_NOT_IMPLEMENTED;
}

// Export configuration to another format
polycall_core_error_t polycall_config_tools_export(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
const char* file_path,
const char* format
) {
if (!core_ctx || !tools_ctx || !file_path || !format || !tools_ctx->config_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}

// Export logic depends on the available serializers in the system
// This is a placeholder implementation

// TODO: Implement different format exporters
POLYCALL_LOG_WARNING(core_ctx, "Configuration export not implemented for format: %s", format);

return POLYCALL_CORE_ERROR_NOT_IMPLEMENTED;
}

// Get configuration tools context
polycall_config_tools_context_t* polycall_config_tools_get_context(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx
) {
if (!core_ctx || !tools_ctx) {
    return NULL;
}

return tools_ctx;
}

// Set configuration tools context
polycall_core_error_t polycall_config_tools_set_context(
polycall_core_context_t* core_ctx,
polycall_config_tools_context_t* tools_ctx,
polycall_config_tools_context_t* new_tools_ctx
) {
if (!core_ctx || !tools_ctx || !new_tools_ctx) {
    return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
}
// Set new context
tools_ctx->core_ctx = new_tools_ctx->core_ctx;
tools_ctx->config_ctx = new_tools_ctx->config_ctx;
tools_ctx->repl_ctx = new_tools_ctx->repl_ctx;
tools_ctx->doctor_ctx = new_tools_ctx->doctor_ctx;
tools_ctx->access_ctx = new_tools_ctx->access_ctx;
tools_ctx->config = new_tools_ctx->config;
return POLYCALL_CORE_SUCCESS;
}
