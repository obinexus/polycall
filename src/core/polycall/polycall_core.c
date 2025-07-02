/**
 * @file polycall_core.c
 * @brief Core module implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 */

#include "polycall/core/polycall/polycall_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define POLYCALL_VERSION "1.0.0"
#define MAX_ERROR_MESSAGE_LEN 512

/**
 * @brief Internal core context structure
 */
struct polycall_core_context {
    /* Configuration */
    polycall_core_config_t config;
    
    /* Memory management */
    polycall_core_malloc_fn custom_malloc;
    polycall_core_free_fn custom_free;
    void* memory_user_data;
    
    /* Error handling */
    polycall_core_error_t last_error;
    char error_message[MAX_ERROR_MESSAGE_LEN];
    
    /* State tracking */
    bool initialized;
    
    /* Memory pool (if configured) */
    void* memory_pool;
    size_t memory_pool_size;
    size_t memory_pool_used;
};

/**
 * @brief Default memory allocation function
 */
static void* default_malloc(size_t size, void* user_data) {
    (void)user_data;
    return malloc(size);
}

/**
 * @brief Default memory free function
 */
static void default_free(void* ptr, void* user_data) {
    (void)user_data;
    free(ptr);
}

/**
 * @brief Internal memory allocation wrapper
 */
static void* internal_malloc(polycall_core_context_t* ctx, size_t size) {
    if (!ctx) {
        return NULL;
    }
    
    return ctx->custom_malloc(size, ctx->memory_user_data);
}

/**
 * @brief Internal memory free wrapper
 */
static void internal_free(polycall_core_context_t* ctx, void* ptr) {
    if (!ctx || !ptr) {
        return;
    }
    
    ctx->custom_free(ptr, ctx->memory_user_data);
}

polycall_core_error_t polycall_core_init(
    polycall_core_context_t** ctx,
    const polycall_core_config_t* config) {
    
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    *ctx = NULL;
    
    /* Allocate context using default allocator */
    polycall_core_context_t* new_ctx = 
        (polycall_core_context_t*)malloc(sizeof(polycall_core_context_t));
    
    if (!new_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize context */
    memset(new_ctx, 0, sizeof(polycall_core_context_t));
    
    /* Copy configuration if provided */
    if (config) {
        memcpy(&new_ctx->config, config, sizeof(polycall_core_config_t));
    }
    
    /* Set default memory functions */
    new_ctx->custom_malloc = default_malloc;
    new_ctx->custom_free = default_free;
    new_ctx->memory_user_data = NULL;
    
    /* Allocate memory pool if requested */
    if (new_ctx->config.memory_pool_size > 0) {
        new_ctx->memory_pool = malloc(new_ctx->config.memory_pool_size);
        if (!new_ctx->memory_pool) {
            free(new_ctx);
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        new_ctx->memory_pool_size = new_ctx->config.memory_pool_size;
        new_ctx->memory_pool_used = 0;
    }
    
    new_ctx->initialized = true;
    new_ctx->last_error = POLYCALL_CORE_SUCCESS;
    
    *ctx = new_ctx;
    
    /* Log initialization if debug mode is enabled */
    if (new_ctx->config.flags & POLYCALL_CORE_FLAG_DEBUG_MODE) {
        polycall_core_log(new_ctx, POLYCALL_LOG_LEVEL_DEBUG, 
            "PolyCall Core initialized (version %s)", POLYCALL_VERSION);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

void polycall_core_cleanup(polycall_core_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->config.flags & POLYCALL_CORE_FLAG_DEBUG_MODE) {
        polycall_core_log(ctx, POLYCALL_LOG_LEVEL_DEBUG, 
            "Cleaning up PolyCall Core context");
    }
    
    /* Free memory pool if allocated */
    if (ctx->memory_pool) {
        free(ctx->memory_pool);
        ctx->memory_pool = NULL;
    }
    
    /* Mark as uninitialized */
    ctx->initialized = false;
    
    /* Free the context itself */
    free(ctx);
}

polycall_core_error_t polycall_core_set_memory_functions(
    polycall_core_context_t* ctx,
    polycall_core_malloc_fn malloc_fn,
    polycall_core_free_fn free_fn,
    void* user_data) {
    
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (!ctx->initialized) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    if (!malloc_fn || !free_fn) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ctx->custom_malloc = malloc_fn;
    ctx->custom_free = free_fn;
    ctx->memory_user_data = user_data;
    
    return POLYCALL_CORE_SUCCESS;
}

void* polycall_core_malloc(polycall_core_context_t* ctx, size_t size) {
    if (!ctx || !ctx->initialized || size == 0) {
        return NULL;
    }
    
    /* Use memory pool if available and has space */
    if (ctx->memory_pool && 
        (ctx->memory_pool_used + size) <= ctx->memory_pool_size) {
        
        void* ptr = (char*)ctx->memory_pool + ctx->memory_pool_used;
        ctx->memory_pool_used += size;
        
        /* Align to 8-byte boundary */
        size_t alignment = 8;
        size_t remainder = ctx->memory_pool_used % alignment;
        if (remainder != 0) {
            ctx->memory_pool_used += (alignment - remainder);
        }
        
        return ptr;
    }
    
    /* Fall back to custom allocator */
    return internal_malloc(ctx, size);
}

void polycall_core_free(polycall_core_context_t* ctx, void* ptr) {
    if (!ctx || !ctx->initialized || !ptr) {
        return;
    }
    
    /* Check if pointer is within memory pool */
    if (ctx->memory_pool &&
        ptr >= ctx->memory_pool &&
        ptr < ((char*)ctx->memory_pool + ctx->memory_pool_size)) {
        /* Memory pool allocations are not individually freed */
        return;
    }
    
    /* Use custom free function */
    internal_free(ctx, ptr);
}

polycall_core_error_t polycall_core_set_error(
    polycall_core_context_t* ctx,
    polycall_core_error_t error,
    const char* message) {
    
    if (!ctx) {
        return error;
    }
    
    ctx->last_error = error;
    
    if (message) {
        strncpy(ctx->error_message, message, MAX_ERROR_MESSAGE_LEN - 1);
        ctx->error_message[MAX_ERROR_MESSAGE_LEN - 1] = '\0';
    } else {
        ctx->error_message[0] = '\0';
    }
    
    /* Call error callback if configured */
    if (ctx->config.error_callback) {
        ctx->config.error_callback(error, message, ctx->config.user_data);
    }
    
    return error;
}

polycall_core_error_t polycall_core_get_last_error(
    polycall_core_context_t* ctx,
    const char** message) {
    
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (message) {
        *message = ctx->error_message[0] ? ctx->error_message : NULL;
    }
    
    return ctx->last_error;
}

const char* polycall_core_get_version(void) {
    return POLYCALL_VERSION;
}

void* polycall_core_get_user_data(polycall_core_context_t* ctx) {
    if (!ctx) {
        return NULL;
    }
    
    return ctx->config.user_data;
}

polycall_core_error_t polycall_core_set_user_data(
    polycall_core_context_t* ctx,
    void* user_data) {
    
    if (!ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (!ctx->initialized) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    ctx->config.user_data = user_data;
    return POLYCALL_CORE_SUCCESS;
}

void polycall_core_vlog(
    polycall_core_context_t* ctx,
    polycall_log_level_t level,
    const char* format,
    va_list args) {
    
    if (!ctx || !format) {
        return;
    }
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    /* Call log callback if configured */
    if (ctx->config.log_callback) {
        ctx->config.log_callback(level, buffer, ctx->config.user_data);
    } else {
        /* Default logging to stderr */
        const char* level_str = "UNKNOWN";
        switch (level) {
            case POLYCALL_LOG_LEVEL_TRACE: level_str = "TRACE"; break;
            case POLYCALL_LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
            case POLYCALL_LOG_LEVEL_INFO:  level_str = "INFO";  break;
            case POLYCALL_LOG_LEVEL_WARN:  level_str = "WARN";  break;
            case POLYCALL_LOG_LEVEL_ERROR: level_str = "ERROR"; break;
            case POLYCALL_LOG_LEVEL_FATAL: level_str = "FATAL"; break;
        }
        
        fprintf(stderr, "[POLYCALL][%s] %s\n", level_str, buffer);
    }
}

void polycall_core_log(
    polycall_core_context_t* ctx,
    polycall_log_level_t level,
    const char* format,
    ...) {
    
    va_list args;
    va_start(args, format);
    polycall_core_vlog(ctx, level, format, args);
    va_end(args);
}