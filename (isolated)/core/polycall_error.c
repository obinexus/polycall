/**
 * @file polycall_error.c
 * @brief Error handling implementation for polycall module
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#include "polycall/core/polycall/polycall_hierarchical_error.h"
#include "polycall/core/polycall/polycall_error.h"
#include "polycall/core/polycall/polycall_logger.h"
#include "polycall/core/polycall/polycall_polycall_error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Error handler context */
typedef struct {
    polycall_hierarchical_error_context_t* hier_error_ctx;
    polycall_error_record_t last_error;
    bool has_error;
} polycall_error_context_t;

/* Module-specific error context (thread-local) */
static __thread polycall_error_context_t* error_ctx = NULL;

/* Default error handler */
static void polycall_default_error_handler(
    polycall_core_context_t* ctx,
    const char* component_name,
    polycall_error_source_t source,
    int32_t code,
    polycall_error_severity_t severity,
    const char* message,
    void* user_data
) {
    /* Log the error */
    polycall_logger_log(ctx, severity, __FILE__, __LINE__, "[%s] %s", component_name, message);
    
    /* Store as last error */
    polycall_error_context_t* err_ctx = (polycall_error_context_t*)user_data;
    if (err_ctx) {
        err_ctx->last_error.source = source;
        err_ctx->last_error.code = code;
        err_ctx->last_error.severity = severity;
        strncpy(err_ctx->last_error.message, message, POLYCALL_ERROR_MAX_MESSAGE_LENGTH - 1);
        err_ctx->last_error.message[POLYCALL_ERROR_MAX_MESSAGE_LENGTH - 1] = '\0';
        err_ctx->has_error = true;
        
        /* If severity is FATAL, also output to stderr */
        if (severity == POLYCALL_ERROR_SEVERITY_FATAL) {
            fprintf(stderr, "[FATAL][%s] %s\n", component_name, message);
        }
    }
}

/* Initialize error subsystem */
polycall_core_error_t polycall_polycall_error_init(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
) {
    if (!core_ctx || !hier_error_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    /* Allocate error context if not already done */
    if (!error_ctx) {
        error_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_error_context_t));
        if (!error_ctx) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        memset(error_ctx, 0, sizeof(polycall_error_context_t));
    }
    
    /* Store hierarchical error context */
    error_ctx->hier_error_ctx = hier_error_ctx;
    
    /* Register component error handler */
    polycall_hierarchical_error_handler_config_t config = {
        .component_name = "polycall",
        .source = POLYCALL_ERROR_SOURCE_POLYCALL,
        .handler = polycall_default_error_handler,
        .user_data = error_ctx,
        .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
        .parent_component = "core"  /* Parent component */
    };
    
    return polycall_hierarchical_error_register_handler(
        core_ctx,
        hier_error_ctx,
        &config
    );
}

/* Clean up error subsystem */
void polycall_polycall_error_cleanup(
    polycall_core_context_t* core_ctx
) {
    if (!core_ctx || !error_ctx) {
        return;
    }
    
    /* Free error context */
    polycall_core_free(core_ctx, error_ctx);
    error_ctx = NULL;
}

/* Get last error */
bool polycall_polycall_error_get_last(
    polycall_core_context_t* core_ctx,
    polycall_error_record_t* error_record
) {
    if (!core_ctx || !error_ctx || !error_record) {
        return false;
    }
    
    if (!error_ctx->has_error) {
        return false;
    }
    
    /* Copy error record */
    memcpy(error_record, &error_ctx->last_error, sizeof(polycall_error_record_t));
    
    return true;
}

/* Set error */
void polycall_polycall_error_set(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_polycall_error_t code,
    polycall_error_severity_t severity,
    const char* file,
    int line,
    const char* message,
    ...
) {
    if (!core_ctx || !hier_error_ctx || !file || !message) {
        return;
    }
    
    /* Format message */
    char formatted_message[POLYCALL_ERROR_MAX_MESSAGE_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, POLYCALL_ERROR_MAX_MESSAGE_LENGTH - 1, message, args);
    va_end(args);
    
    /* Set error in hierarchical system */
    polycall_hierarchical_error_set(
        core_ctx,
        hier_error_ctx,
        "polycall",
        POLYCALL_ERROR_SOURCE_POLYCALL,
        code,
        severity,
        file,
        line,
        formatted_message
    );
    
    /* If severity is ERROR or FATAL, also output to stderr */
    if (severity >= POLYCALL_ERROR_SEVERITY_ERROR) {
        fprintf(stderr, "[%s][%s] %s\n", 
                severity == POLYCALL_ERROR_SEVERITY_FATAL ? "FATAL" : "ERROR",
                "polycall", 
                formatted_message);
    }
}

/* Clear errors */
polycall_core_error_t polycall_polycall_error_clear(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx
) {
    if (!core_ctx || !error_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    /* Clear error flag */
    error_ctx->has_error = false;
    
    /* Clear error record */
    memset(&error_ctx->last_error, 0, sizeof(polycall_error_record_t));
    
    /* Clear in hierarchical system */
    return polycall_hierarchical_error_clear(
        core_ctx,
        hier_error_ctx,
        "polycall"
    );
}

/* Register error handler */
polycall_core_error_t polycall_polycall_error_register_handler(
    polycall_core_context_t* core_ctx,
    polycall_hierarchical_error_context_t* hier_error_ctx,
    polycall_hierarchical_error_handler_fn handler,
    void* user_data
) {
    if (!core_ctx || !hier_error_ctx || !handler) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    /* Register custom handler */
    polycall_hierarchical_error_handler_config_t config = {
        .component_name = "polycall",
        .source = POLYCALL_ERROR_SOURCE_POLYCALL,
        .handler = handler,
        .user_data = user_data,
        .propagation_mode = POLYCALL_ERROR_PROPAGATE_BIDIRECTIONAL,
        .parent_component = "core"  /* Parent component */
    };
    
    return polycall_hierarchical_error_register_handler(
        core_ctx,
        hier_error_ctx,
        &config
    );
}

/* Convert error code to string */
const char* polycall_polycall_error_to_string(
    polycall_polycall_error_t error
) {
    switch (error) {
        case POLYCALL_POLYCALL_SUCCESS:
            return "Success";
        case POLYCALL_POLYCALL_ERROR_INITIALIZATION_FAILED:
            return "Initialization failed";
        case POLYCALL_POLYCALL_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case POLYCALL_POLYCALL_ERROR_INVALID_STATE:
            return "Invalid state";
        case POLYCALL_POLYCALL_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case POLYCALL_POLYCALL_ERROR_ALREADY_INITIALIZED:
            return "Already initialized";
        case POLYCALL_POLYCALL_ERROR_UNSUPPORTED_OPERATION:
            return "Unsupported operation";
        case POLYCALL_POLYCALL_ERROR_RESOURCE_ALLOCATION:
            return "Resource allocation failure";
        case POLYCALL_POLYCALL_ERROR_TIMEOUT:
            return "Operation timed out";
        case POLYCALL_POLYCALL_ERROR_PERMISSION_DENIED:
            return "Permission denied";
        default:
            if (error >= POLYCALL_POLYCALL_ERROR_CUSTOM_START) {
                return "Custom error";
            }
            return "Unknown error";
    }
}
