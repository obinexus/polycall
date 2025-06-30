/**
#include "polycall/core/polycall/context.h"

 * @file context.c
 * @brief Context management implementation for LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * This file implements the context management system defined in
 * polycall_context.h, providing unified state tracking and resource
 * management for the Program-First design approach.
 */

 #include "polycall/core/polycall/polycall_context.h"

 
 // Helper to get the context registry
 static context_registry_t* get_registry(polycall_core_context_t* core_ctx) {
     if (!core_ctx) return NULL;
     
     // Attempt to get existing registry
     context_registry_t* registry = polycall_core_get_user_data(core_ctx);
     
     // If not found, create a new registry
     if (!registry) {
         registry = polycall_core_malloc(core_ctx, sizeof(context_registry_t));
         if (!registry) {
             polycall_core_set_error(core_ctx, POLYCALL_CORE_ERROR_OUT_OF_MEMORY, 
                                    "Failed to allocate context registry");
             return NULL;
         }
         
         // Initialize registry
         memset(registry, 0, sizeof(context_registry_t));
         pthread_mutex_init(&registry->registry_lock, NULL);
         
         // Store in core context
         polycall_core_set_user_data(core_ctx, registry);
     }
     
     return registry;
 }
 
 // Helper to find a context by type
 static polycall_context_ref_t* find_context_by_type_internal(
     context_registry_t* registry,
     polycall_context_type_t type
 ) {
     if (!registry) return NULL;
     
     for (size_t i = 0; i < registry->context_count; i++) {
         if (registry->contexts[i] && registry->contexts[i]->type == type) {
             return registry->contexts[i];
         }
     }
     
     return NULL;
 }
 
 // Helper to find a context by name
 static polycall_context_ref_t* find_context_by_name_internal(
     context_registry_t* registry,
     const char* name
 ) {
     if (!registry || !name) return NULL;
     
     for (size_t i = 0; i < registry->context_count; i++) {
         if (registry->contexts[i] && registry->contexts[i]->name &&
             strcmp(registry->contexts[i]->name, name) == 0) {
             return registry->contexts[i];
         }
     }
     
     return NULL;
 }
 
 // Helper to notify context listeners
 static void notify_listeners(polycall_context_ref_t* ctx_ref) {
     if (!ctx_ref) return;
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     for (size_t i = 0; i < ctx_ref->listener_count; i++) {
         if (ctx_ref->listeners[i].listener) {
             ctx_ref->listeners[i].listener(ctx_ref, ctx_ref->listeners[i].user_data);
         }
     }
     
     pthread_mutex_unlock(&ctx_ref->lock);
 }
 
 // Initialize a context
 polycall_core_error_t polycall_context_init(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t** ctx_ref,
     const polycall_context_init_t* init
 ) {
     if (!core_ctx || !ctx_ref || !init || !init->name) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get registry
     context_registry_t* registry = get_registry(core_ctx);
     if (!registry) {
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     pthread_mutex_lock(&registry->registry_lock);
     
     // Check if we have space for another context
     if (registry->context_count >= MAX_CONTEXTS) {
         pthread_mutex_unlock(&registry->registry_lock);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Check if a context with the same type or name already exists
     if (find_context_by_type_internal(registry, init->type) ||
         find_context_by_name_internal(registry, init->name)) {
         pthread_mutex_unlock(&registry->registry_lock);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Allocate context reference
     polycall_context_ref_t* new_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_context_ref_t));
     if (!new_ctx) {
         pthread_mutex_unlock(&registry->registry_lock);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context reference
     memset(new_ctx, 0, sizeof(polycall_context_ref_t));
     new_ctx->type = init->type;
     new_ctx->name = init->name;
     new_ctx->flags = init->flags;
     new_ctx->init_fn = init->init_fn;
     new_ctx->cleanup_fn = init->cleanup_fn;
     new_ctx->data_size = init->data_size;
     pthread_mutex_init(&new_ctx->lock, NULL);
     
     // Allocate context data if needed
     if (init->data_size > 0) {
         new_ctx->data = polycall_core_malloc(core_ctx, init->data_size);
         if (!new_ctx->data) {
             polycall_core_free(core_ctx, new_ctx);
             pthread_mutex_unlock(&registry->registry_lock);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Zero-initialize data
         memset(new_ctx->data, 0, init->data_size);
     }
     
     // Add to registry
     registry->contexts[registry->context_count++] = new_ctx;
     
     pthread_mutex_unlock(&registry->registry_lock);
     
     // Initialize context data if an init function is provided
     if (init->init_fn) {
         polycall_core_error_t result = init->init_fn(core_ctx, new_ctx->data, init->init_data);
         if (result != POLYCALL_CORE_SUCCESS) {
             // Clean up on initialization failure
             polycall_context_cleanup(core_ctx, new_ctx);
             return result;
         }
     }
     
     // Mark as initialized
     new_ctx->flags |= POLYCALL_CONTEXT_FLAG_INITIALIZED;
     
     // Set result
     *ctx_ref = new_ctx;
     
     return POLYCALL_CORE_SUCCESS;
 }

 polycall_error_t polycall_context_unshare(
    polycall_context_t* core_ctx,
    polycall_context_ref_t* ctx_ref
) {
    if (!core_ctx || !ctx_ref) {
        return POLYCALL_ERROR_INVALID_PARAMETER;
    }

    polycall_context_lock(core_ctx, ctx_ref);
    ctx_ref->flags &= ~POLYCALL_CONTEXT_FLAG_SHARED;
    polycall_context_unlock(core_ctx, ctx_ref);

    return POLYCALL_ERROR_SUCCESS;
}
 // Clean up a context
 void polycall_context_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return;
     }
     
     // Get registry
     context_registry_t* registry = get_registry(core_ctx);
     if (!registry) {
         return;
     }
     
     pthread_mutex_lock(&registry->registry_lock);
     
     // Find and remove context from registry
     size_t index = MAX_CONTEXTS;
     for (size_t i = 0; i < registry->context_count; i++) {
         if (registry->contexts[i] == ctx_ref) {
             index = i;
             break;
         }
     }
     
     if (index < MAX_CONTEXTS) {
         // Clean up context data if a cleanup function is provided
         if (ctx_ref->cleanup_fn && ctx_ref->data) {
             ctx_ref->cleanup_fn(core_ctx, ctx_ref->data);
         }
         
         // Free context data
         if (ctx_ref->data) {
             polycall_core_free(core_ctx, ctx_ref->data);
         }
         
         // Destroy mutex
         pthread_mutex_destroy(&ctx_ref->lock);
         
         // Free context reference
         polycall_core_free(core_ctx, ctx_ref);
         
         // Remove from registry (shift all contexts after it)
         for (size_t i = index; i < registry->context_count - 1; i++) {
             registry->contexts[i] = registry->contexts[i + 1];
         }
         
         registry->context_count--;
     }
     
     pthread_mutex_unlock(&registry->registry_lock);
 }
 
 // Get context data
 void* polycall_context_get_data(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return NULL;
     }
     
     return ctx_ref->data;
 }
 
 // Find a context by type
 polycall_context_ref_t* polycall_context_find_by_type(
     polycall_core_context_t* core_ctx,
     polycall_context_type_t type
 ) {
     if (!core_ctx) {
         return NULL;
     }
     
     // Get registry
     context_registry_t* registry = get_registry(core_ctx);
     if (!registry) {
         return NULL;
     }
     
     pthread_mutex_lock(&registry->registry_lock);
     polycall_context_ref_t* result = find_context_by_type_internal(registry, type);
     pthread_mutex_unlock(&registry->registry_lock);
     
     return result;
 }
 
 // Find a context by name
 polycall_context_ref_t* polycall_context_find_by_name(
     polycall_core_context_t* core_ctx,
     const char* name
 ) {
     if (!core_ctx || !name) {
         return NULL;
     }
     
     // Get registry
     context_registry_t* registry = get_registry(core_ctx);
     if (!registry) {
         return NULL;
     }
     
     pthread_mutex_lock(&registry->registry_lock);
     polycall_context_ref_t* result = find_context_by_name_internal(registry, name);
     pthread_mutex_unlock(&registry->registry_lock);
     
     return result;
 }
 
 // Get context type
 polycall_context_type_t polycall_context_get_type(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return (polycall_context_type_t)0;
     }
     
     return ctx_ref->type;
 }
 
 // Get context name
 const char* polycall_context_get_name(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return NULL;
     }
     
     return ctx_ref->name;
 }
 
 // Get context flags
 polycall_context_flags_t polycall_context_get_flags(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return (polycall_context_flags_t)POLYCALL_CONTEXT_FLAG_NONE;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     polycall_context_flags_t flags = ctx_ref->flags;
     pthread_mutex_unlock(&ctx_ref->lock);
     
     return flags;
 }
 
 // Set context flags
 polycall_core_error_t polycall_context_set_flags(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref,
     polycall_context_flags_t flags
 ) {
     if (!core_ctx || !ctx_ref) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if the context is locked
     if (ctx_ref->flags & POLYCALL_CONTEXT_FLAG_LOCKED) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Update flags while preserving INITIALIZED flag
     bool was_initialized = ctx_ref->flags & POLYCALL_CONTEXT_FLAG_INITIALIZED;
     ctx_ref->flags = flags;
     
     if (was_initialized) {
         ctx_ref->flags |= POLYCALL_CONTEXT_FLAG_INITIALIZED;
     }
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Check if a context is initialized
 bool polycall_context_is_initialized(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return false;
     }
     
     return (ctx_ref->flags & POLYCALL_CONTEXT_FLAG_INITIALIZED) != 0;
 }
 
 // Lock a context
 polycall_core_error_t polycall_context_lock(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if already locked
     if (ctx_ref->flags & POLYCALL_CONTEXT_FLAG_LOCKED) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Lock context
     ctx_ref->flags |= POLYCALL_CONTEXT_FLAG_LOCKED;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Unlock a context
 polycall_core_error_t polycall_context_unlock(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if not locked
     if (!(ctx_ref->flags & POLYCALL_CONTEXT_FLAG_LOCKED)) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Unlock context
     ctx_ref->flags &= ~POLYCALL_CONTEXT_FLAG_LOCKED;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Share a context with another component
 polycall_core_error_t polycall_context_share(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref,
     const char* component
 ) {
     if (!core_ctx || !ctx_ref || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if isolated
     if (ctx_ref->flags & POLYCALL_CONTEXT_FLAG_ISOLATED) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Mark as shared
     ctx_ref->flags |= POLYCALL_CONTEXT_FLAG_SHARED;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Unshare a context
 polycall_core_error_t polycall_context_unshare(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if shared
     if (!(ctx_ref->flags & POLYCALL_CONTEXT_FLAG_SHARED)) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_SUCCESS; // Already not shared
     }
     
     // Mark as not shared
     ctx_ref->flags &= ~POLYCALL_CONTEXT_FLAG_SHARED;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Isolate a context
 polycall_core_error_t polycall_context_isolate(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref
 ) {
     if (!core_ctx || !ctx_ref) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if shared
     if (ctx_ref->flags & POLYCALL_CONTEXT_FLAG_SHARED) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Mark as isolated
     ctx_ref->flags |= POLYCALL_CONTEXT_FLAG_ISOLATED;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     // Notify listeners
     notify_listeners(ctx_ref);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Register a context listener
 polycall_core_error_t polycall_context_register_listener(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref,
     void (*listener)(polycall_context_ref_t* ctx_ref, void* user_data),
     void* user_data
 ) {
     if (!core_ctx || !ctx_ref || !listener) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Check if we have space for another listener
     if (ctx_ref->listener_count >= MAX_LISTENERS) {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Add listener
     ctx_ref->listeners[ctx_ref->listener_count].listener = listener;
     ctx_ref->listeners[ctx_ref->listener_count].user_data = user_data;
     ctx_ref->listener_count++;
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Unregister a context listener
 polycall_core_error_t polycall_context_unregister_listener(
     polycall_core_context_t* core_ctx,
     polycall_context_ref_t* ctx_ref,
     void (*listener)(polycall_context_ref_t* ctx_ref, void* user_data),
     void* user_data
 ) {
     if (!core_ctx || !ctx_ref || !listener) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&ctx_ref->lock);
     
     // Find and remove listener
     size_t index = MAX_LISTENERS;
     for (size_t i = 0; i < ctx_ref->listener_count; i++) {
         if (ctx_ref->listeners[i].listener == listener &&
             ctx_ref->listeners[i].user_data == user_data) {
             index = i;
             break;
         }
     }
     
     if (index < MAX_LISTENERS) {
         // Shift remaining listeners
         for (size_t i = index; i < ctx_ref->listener_count - 1; i++) {
             ctx_ref->listeners[i] = ctx_ref->listeners[i + 1];
         }
         ctx_ref->listener_count--;
     } else {
         pthread_mutex_unlock(&ctx_ref->lock);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_unlock(&ctx_ref->lock);
     
     return POLYCALL_CORE_SUCCESS;
 }