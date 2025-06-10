/**
#include "polycall/core/ffi/ffi_core.h"

 * @file ffi_core.c
 * @brief Core Foreign Function Interface module for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the core FFI functionality for LibPolyCall, enabling 
 * cross-language interoperability with the Program-First design philosophy.
 */

 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/ffi/type_system.h"
 #include "polycall/core/ffi/memory_bridge.h"
 #include "polycall/core/ffi/security.h"

   // Version string (format: Major.Minor.Patch)
   static const char* FFI_VERSION_STRING = 
   POLYCALL_FFI_VERSION_MAJOR "." 
   POLYCALL_FFI_VERSION_MINOR "." 
   POLYCALL_FFI_VERSION_PATCH;
 /**
  * @brief Initialize function registry
  */
 static ffi_registry_t* init_registry(
     polycall_core_context_t* ctx,
     size_t function_capacity
 ) {
     ffi_registry_t* registry = polycall_core_malloc(ctx, sizeof(ffi_registry_t));
     if (!registry) {
         return NULL;
     }
     
     registry->functions = polycall_core_malloc(ctx, 
                                              function_capacity * sizeof(function_entry_t));
     if (!registry->functions) {
         polycall_core_free(ctx, registry);
         return NULL;
     }
     
     registry->languages = polycall_core_malloc(ctx, 
                                              8 * sizeof(language_entry_t)); // Default 8 languages
     if (!registry->languages) {
         polycall_core_free(ctx, registry->functions);
         polycall_core_free(ctx, registry);
         return NULL;
     }
     
     registry->capacity = function_capacity;
     registry->count = 0;
     registry->language_capacity = 8;
     registry->language_count = 0;
     
     return registry;
 }
 
 /**
  * @brief Cleanup function registry
  */
 static void cleanup_registry(
     polycall_core_context_t* ctx,
     ffi_registry_t* registry
 ) {
     if (!ctx || !registry) {
         return;
     }
     
     // Free function entries
     for (size_t i = 0; i < registry->count; i++) {
         polycall_core_free(ctx, registry->functions[i].name);
         polycall_core_free(ctx, registry->functions[i].language);
         // Note: signature is freed elsewhere
     }
     
     // Free language entries
     for (size_t i = 0; i < registry->language_count; i++) {
         polycall_core_free(ctx, registry->languages[i].language);
     }
     
     polycall_core_free(ctx, registry->functions);
     polycall_core_free(ctx, registry->languages);
     polycall_core_free(ctx, registry);
 }
 
 // Implementation of public functions
 
 polycall_core_error_t polycall_ffi_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t** ffi_ctx,
     const polycall_ffi_config_t* config
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate FFI context
     polycall_ffi_context_t* new_ctx = polycall_core_malloc(ctx, sizeof(polycall_ffi_context_t));
     if (!new_ctx) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize members
     memset(new_ctx, 0, sizeof(polycall_ffi_context_t));
     new_ctx->core_ctx = ctx;
     new_ctx->flags = config->flags;
     new_ctx->user_data = config->user_data;
     
     // Initialize function registry
     new_ctx->registry = init_registry(ctx, config->function_capacity);
     if (!new_ctx->registry) {
         polycall_core_free(ctx, new_ctx);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize type system
     type_system_config_t type_config = polycall_type_create_default_config();
     type_config.type_capacity = config->type_capacity;
     
     polycall_core_error_t result = polycall_type_init(
         ctx, 
         new_ctx,
         &new_ctx->type_ctx,
         &type_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_registry(ctx, new_ctx->registry);
         polycall_core_free(ctx, new_ctx);
         return result;
     }
     
     // Initialize memory bridge
     memory_bridge_config_t mem_config = polycall_memory_bridge_create_default_config();
     mem_config.shared_pool_size = config->memory_pool_size;
     
     result = polycall_memory_bridge_init(
         ctx,
         new_ctx,
         &new_ctx->memory_mgr,
         &mem_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_type_cleanup(ctx, new_ctx, new_ctx->type_ctx);
         cleanup_registry(ctx, new_ctx->registry);
         polycall_core_free(ctx, new_ctx);
         return result;
     }
     
     // Register with context system
     polycall_context_ref_t context_ref;
     result = polycall_context_register(
         ctx,
         POLYCALL_CONTEXT_TYPE_FFI,
         new_ctx,
         &context_ref
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_memory_bridge_cleanup(ctx, new_ctx, new_ctx->memory_mgr);
         polycall_type_cleanup(ctx, new_ctx, new_ctx->type_ctx);
         cleanup_registry(ctx, new_ctx->registry);
         polycall_core_free(ctx, new_ctx);
         return result;
     }
     
     new_ctx->context_ref = context_ref;
     *ffi_ctx = new_ctx;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 void polycall_ffi_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx
 ) {
     if (!ctx || !ffi_ctx) {
         return;
     }
     
     // Unregister from context system
     polycall_context_unregister(ctx, ffi_ctx->context_ref);
     
     // Clean up components in reverse order of initialization
     if (ffi_ctx->perf_mgr) {
         polycall_performance_cleanup(ctx, ffi_ctx, ffi_ctx->perf_mgr);
     }
     
     if (ffi_ctx->security_ctx) {
         polycall_security_cleanup(ctx, ffi_ctx, ffi_ctx->security_ctx);
     }
     
     if (ffi_ctx->memory_mgr) {
         polycall_memory_bridge_cleanup(ctx, ffi_ctx, ffi_ctx->memory_mgr);
     }
     
     if (ffi_ctx->type_ctx) {
         polycall_type_cleanup(ctx, ffi_ctx, ffi_ctx->type_ctx);
     }
     
     if (ffi_ctx->registry) {
         cleanup_registry(ctx, ffi_ctx->registry);
     }
     
     // Free context itself
     polycall_core_free(ctx, ffi_ctx);
 }
 
 polycall_core_error_t polycall_ffi_register_language(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* language_name,
     language_bridge_t* bridge
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !language_name || !bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     ffi_registry_t* registry = ffi_ctx->registry;
     
     // Check if language already registered
     for (size_t i = 0; i < registry->language_count; i++) {
         if (strcmp(registry->languages[i].language, language_name) == 0) {
             return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
         }
     }
     
     // Check if we need to expand capacity
     if (registry->language_count >= registry->language_capacity) {
         size_t new_capacity = registry->language_capacity * 2;
         language_entry_t* new_languages = polycall_core_malloc(ctx,
             new_capacity * sizeof(language_entry_t));
         
         if (!new_languages) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing entries
         memcpy(new_languages, registry->languages, 
               registry->language_count * sizeof(language_entry_t));
         
         // Free old array
         polycall_core_free(ctx, registry->languages);
         
         // Update registry
         registry->languages = new_languages;
         registry->language_capacity = new_capacity;
     }
     
     // Add new language entry
     size_t name_len = strlen(language_name) + 1;
     registry->languages[registry->language_count].language = 
         polycall_core_malloc(ctx, name_len);
     
     if (!registry->languages[registry->language_count].language) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(registry->languages[registry->language_count].language, 
           language_name, name_len);
     
     // Copy bridge interface
     memcpy(&registry->languages[registry->language_count].bridge, 
           bridge, sizeof(language_bridge_t));
     
     registry->language_count++;
     
     // Initialize the language bridge if needed
     if (bridge->initialize) {
         return bridge->initialize(ctx);
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_ffi_expose_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     const char* source_language,
     uint32_t flags
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !function_name || !function_ptr || 
         !signature || !source_language) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     ffi_registry_t* registry = ffi_ctx->registry;
     
     // Check if function already registered
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->functions[i].name, function_name) == 0) {
             return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
         }
     }
     
     // Check if we need to expand capacity
     if (registry->count >= registry->capacity) {
         size_t new_capacity = registry->capacity * 2;
         function_entry_t* new_functions = polycall_core_malloc(ctx,
             new_capacity * sizeof(function_entry_t));
         
         if (!new_functions) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing entries
         memcpy(new_functions, registry->functions, 
               registry->count * sizeof(function_entry_t));
         
         // Free old array
         polycall_core_free(ctx, registry->functions);
         
         // Update registry
         registry->functions = new_functions;
         registry->capacity = new_capacity;
     }
     
     // Add new function entry
     size_t name_len = strlen(function_name) + 1;
     registry->functions[registry->count].name = 
         polycall_core_malloc(ctx, name_len);
     
     if (!registry->functions[registry->count].name) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(registry->functions[registry->count].name, 
           function_name, name_len);
     
     // Copy language name
     size_t lang_len = strlen(source_language) + 1;
     registry->functions[registry->count].language = 
         polycall_core_malloc(ctx, lang_len);
     
     if (!registry->functions[registry->count].language) {
         polycall_core_free(ctx, registry->functions[registry->count].name);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     memcpy(registry->functions[registry->count].language, 
           source_language, lang_len);
     
     // Set other fields
     registry->functions[registry->count].function_ptr = function_ptr;
     registry->functions[registry->count].signature = signature;
     registry->functions[registry->count].flags = flags;
     
     registry->count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_ffi_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result,
     const char* target_language
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !function_name || (!args && arg_count > 0) || 
         !result || !target_language) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     ffi_registry_t* registry = ffi_ctx->registry;
     
     // Find function
     function_entry_t* func_entry = NULL;
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->functions[i].name, function_name) == 0) {
             func_entry = &registry->functions[i];
             break;
         }
     }
     
     if (!func_entry) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find target language bridge
     language_bridge_t* bridge = NULL;
     for (size_t i = 0; i < registry->language_count; i++) {
         if (strcmp(registry->languages[i].language, target_language) == 0) {
             bridge = &registry->languages[i].bridge;
             break;
         }
     }
     
     if (!bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Verify security access if security context is available
     if (ffi_ctx->security_ctx) {
         security_result_t sec_result;
         polycall_core_error_t res = polycall_security_verify_access(
             ctx, ffi_ctx, ffi_ctx->security_ctx,
             function_name, target_language, NULL, &sec_result
         );
         
         if (res != POLYCALL_CORE_SUCCESS || !sec_result.allowed) {
             return POLYCALL_CORE_ERROR_PERMISSION_DENIED;
         }
     }
     
     // Perform function call
     return bridge->call_function(
         ctx, function_name, args, arg_count, result
     );
 }
 
 polycall_core_error_t polycall_ffi_register_type(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_type_info_t* type_info,
     const char* language
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !type_info || !language) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Register with type system
     return polycall_type_register(ctx, ffi_ctx, ffi_ctx->type_ctx, type_info, language);
 }
 
 polycall_core_error_t polycall_ffi_create_signature(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_type_t return_type,
     polycall_ffi_type_t* param_types,
     size_t param_count,
     ffi_signature_t** signature
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || (!param_types && param_count > 0) || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate signature
     ffi_signature_t* new_sig = polycall_core_malloc(ctx, sizeof(ffi_signature_t));
     if (!new_sig) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize signature
     memset(new_sig, 0, sizeof(ffi_signature_t));
     new_sig->return_type = return_type;
     new_sig->param_count = param_count;
     
     // Allocate parameter arrays if needed
     if (param_count > 0) {
         new_sig->param_types = polycall_core_malloc(ctx, 
                                                param_count * sizeof(polycall_ffi_type_t));
         if (!new_sig->param_types) {
             polycall_core_free(ctx, new_sig);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy parameter types
         memcpy(new_sig->param_types, param_types, 
               param_count * sizeof(polycall_ffi_type_t));
         
         // Allocate arrays for additional parameter information
         new_sig->param_type_infos = polycall_core_malloc(ctx, 
                                                      param_count * sizeof(ffi_type_info_t*));
         new_sig->param_names = polycall_core_malloc(ctx, 
                                                 param_count * sizeof(char*));
         new_sig->param_optional = polycall_core_malloc(ctx, 
                                                    param_count * sizeof(bool));
         
         if (!new_sig->param_type_infos || !new_sig->param_names || !new_sig->param_optional) {
             if (new_sig->param_type_infos)
                 polycall_core_free(ctx, new_sig->param_type_infos);
             if (new_sig->param_names)
                 polycall_core_free(ctx, new_sig->param_names);
             if (new_sig->param_optional)
                 polycall_core_free(ctx, new_sig->param_optional);
             polycall_core_free(ctx, new_sig->param_types);
             polycall_core_free(ctx, new_sig);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Initialize arrays
         memset(new_sig->param_type_infos, 0, param_count * sizeof(ffi_type_info_t*));
         memset(new_sig->param_names, 0, param_count * sizeof(char*));
         memset(new_sig->param_optional, 0, param_count * sizeof(bool));
     }
     
     *signature = new_sig;
     return POLYCALL_CORE_SUCCESS;
 }
 
 void polycall_ffi_destroy_signature(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_signature_t* signature
 ) {
     if (!ctx || !signature) {
         return;
     }
     
     // Free parameter arrays
     if (signature->param_count > 0) {
         // Free parameter names
         for (size_t i = 0; i < signature->param_count; i++) {
             if (signature->param_names[i]) {
                 polycall_core_free(ctx, (void*)signature->param_names[i]);
             }
         }
         
         // Free arrays
         if (signature->param_types)
             polycall_core_free(ctx, signature->param_types);
         if (signature->param_type_infos)
             polycall_core_free(ctx, signature->param_type_infos);
         if (signature->param_names)
             polycall_core_free(ctx, signature->param_names);
         if (signature->param_optional)
             polycall_core_free(ctx, signature->param_optional);
     }
     
     // Free return type info if needed (ownership depends on usage)
     
     // Free signature itself
     polycall_core_free(ctx, signature);
 }
 
 polycall_core_error_t polycall_ffi_create_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_ffi_type_t type,
     ffi_value_t** value
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate value
     ffi_value_t* new_value = polycall_core_malloc(ctx, sizeof(ffi_value_t));
     if (!new_value) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize value
     memset(new_value, 0, sizeof(ffi_value_t));
     new_value->type = type;
     
     *value = new_value;
     return POLYCALL_CORE_SUCCESS;
 }
 
 void polycall_ffi_destroy_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_value_t* value
 ) {
     if (!ctx || !value) {
         return;
     }
     
     // Free value data based on type
     switch (value->type) {
         case POLYCALL_FFI_TYPE_STRING:
             // Only free if we own the string (depends on usage)
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
         case POLYCALL_FFI_TYPE_ARRAY:
         case POLYCALL_FFI_TYPE_OBJECT:
             // Only free if we own the data (depends on usage)
             break;
             
         default:
             // Simple types don't need cleanup
             break;
     }
     
     // Free value itself
     polycall_core_free(ctx, value);
 }
 
 polycall_core_error_t polycall_ffi_set_value_data(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     ffi_value_t* value,
     const void* data,
     size_t size
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !value || (!data && size > 0)) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Handle data based on type
     switch (value->type) {
         case POLYCALL_FFI_TYPE_BOOL:
             if (size != sizeof(bool)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             memcpy(&value->value.bool_value, data, size);
             break;
             
         case POLYCALL_FFI_TYPE_CHAR:
             if (size != sizeof(char)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             memcpy(&value->value.char_value, data, size);
             break;
             
         case POLYCALL_FFI_TYPE_UINT8:
             if (size != sizeof(uint8_t)) {
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             memcpy(&value->value.uint8_value, data, size);
             break;
             
         // Similar cases for other primitive types...
             
         case POLYCALL_FFI_TYPE_STRING:
             // Make a copy of the string
             {
                 char* str_copy = polycall_core_malloc(ctx, size);
                 if (!str_copy) {
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 memcpy(str_copy, data, size);
                 value->value.string_value = str_copy;
             }
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             memcpy(&value->value.pointer_value, data, sizeof(void*));
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
         case POLYCALL_FFI_TYPE_ARRAY:
         case POLYCALL_FFI_TYPE_OBJECT:
             // These types require complex handling with memory manager
             // Implementation would depend on the memory bridge
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_ffi_get_value_data(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     const ffi_value_t* value,
     void** data,
     size_t* size
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !value || !data || !size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Handle data based on type
     switch (value->type) {
         case POLYCALL_FFI_TYPE_BOOL:
             *data = (void*)&value->value.bool_value;
             *size = sizeof(bool);
             break;
             
         case POLYCALL_FFI_TYPE_CHAR:
             *data = (void*)&value->value.char_value;
             *size = sizeof(char);
             break;
             
         case POLYCALL_FFI_TYPE_UINT8:
             *data = (void*)&value->value.uint8_value;
             *size = sizeof(uint8_t);
             break;
             
         // Similar cases for other primitive types...
             
         case POLYCALL_FFI_TYPE_STRING:
             *data = (void*)value->value.string_value;
             *size = strlen(value->value.string_value) + 1;
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             *data = (void*)&value->value.pointer_value;
             *size = sizeof(void*);
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
         case POLYCALL_FFI_TYPE_ARRAY:
         case POLYCALL_FFI_TYPE_OBJECT:
             // These types require complex handling with memory manager
             // Implementation would depend on the memory bridge
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_ffi_get_info(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    size_t* language_count,
    size_t* function_count,
    size_t* type_count
) {
    // Validate parameters
    if (!ctx || !ffi_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ffi_registry_t* registry = ffi_ctx->registry;
    
    // Fill out requested information
    if (language_count) {
        *language_count = registry->language_count;
    }
    
    if (function_count) {
        *function_count = registry->count;
    }
    
    if (type_count && ffi_ctx->type_ctx) {
        // Delegate to type system to get type count
        return polycall_type_get_count(ctx, ffi_ctx, ffi_ctx->type_ctx, type_count);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

const char* polycall_ffi_get_version(void) {
    return FFI_VERSION_STRING;
}

// Internal helper functions for finding language bridges and functions

/**
 * @brief Find a language bridge by name
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param language_name Language name to find
 * @return Language bridge interface or NULL if not found
 */
language_bridge_t* find_language_bridge(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const char* language_name
) {
    if (!ctx || !ffi_ctx || !language_name) {
        return NULL;
    }
    
    ffi_registry_t* registry = ffi_ctx->registry;
    
    for (size_t i = 0; i < registry->language_count; i++) {
        if (strcmp(registry->languages[i].language, language_name) == 0) {
            return &registry->languages[i].bridge;
        }
    }
    
    return NULL;
}

/**
 * @brief Find a registered function by name
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name to find
 * @return Function entry or NULL if not found
 */
function_entry_t* find_function(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const char* function_name
) {
    if (!ctx || !ffi_ctx || !function_name) {
        return NULL;
    }
    
    ffi_registry_t* registry = ffi_ctx->registry;
    
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->functions[i].name, function_name) == 0) {
            return &registry->functions[i];
        }
    }
    
    return NULL;
}

// Additional utility functions for internal use

/**
 * @brief Check if signature is compatible with arguments
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param signature Function signature
 * @param args Function arguments
 * @param arg_count Argument count
 * @return true if compatible, false otherwise
 */
static bool is_signature_compatible(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const ffi_signature_t* signature,
    const ffi_value_t* args,
    size_t arg_count
) {
    if (!signature) {
        return false;
    }
    
    // Check argument count
    if (!signature->variadic && arg_count != signature->param_count) {
        return false;
    }
    
    if (signature->variadic && arg_count < signature->param_count) {
        return false;
    }
    
    // Check argument types
    for (size_t i = 0; i < signature->param_count && i < arg_count; i++) {
        // Skip check for optional parameters if NULL is passed
        if (signature->param_optional && signature->param_optional[i] && 
            args[i].type == POLYCALL_FFI_TYPE_VOID) {
            continue;
        }
        
        // Basic type checking
        if (args[i].type != signature->param_types[i]) {
            // Check if types are compatible through conversion
            if (!polycall_type_are_compatible(
                ctx, ffi_ctx, ffi_ctx->type_ctx,
                args[i].type, signature->param_types[i])) {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief Wrapper for performance tracing of function calls
 * 
 * This function handles performance monitoring and error tracing
 * for cross-language function calls.
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name
 * @param source_language Source language
 * @param target_language Target language
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Function result
 * @return Error code
 */
static polycall_core_error_t trace_function_call(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const char* function_name,
    const char* source_language,
    const char* target_language,
    ffi_value_t* args,
    size_t arg_count,
    ffi_value_t* result
) {
    // Skip tracing if performance manager is not available
    if (!ffi_ctx->perf_mgr) {
        // Direct delegation to target language
        language_bridge_t* bridge = find_language_bridge(ctx, ffi_ctx, target_language);
        if (!bridge) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        return bridge->call_function(ctx, function_name, args, arg_count, result);
    }
    
    // Start tracing
    performance_trace_entry_t* trace_entry = NULL;
    polycall_core_error_t err = polycall_performance_trace_begin(
        ctx,
        ffi_ctx,
        ffi_ctx->perf_mgr,
        function_name,
        source_language,
        target_language,
        &trace_entry
    );
    
    if (err != POLYCALL_CORE_SUCCESS) {
        // Continue without tracing if it fails
        language_bridge_t* bridge = find_language_bridge(ctx, ffi_ctx, target_language);
        if (!bridge) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        return bridge->call_function(ctx, function_name, args, arg_count, result);
    }
    
    // Check cache if enabled
    ffi_value_t* cached_result = NULL;
    bool use_cache = polycall_performance_check_cache(
        ctx,
        ffi_ctx,
        ffi_ctx->perf_mgr,
        function_name,
        args,
        arg_count,
        &cached_result
    );
    
    if (use_cache && cached_result) {
        // Copy cached result
        memcpy(result, cached_result, sizeof(ffi_value_t));
        
        // End tracing with cached flag
        trace_entry->cached = true;
        polycall_performance_trace_end(
            ctx,
            ffi_ctx,
            ffi_ctx->perf_mgr,
            trace_entry
        );
        
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Execute function call
    language_bridge_t* bridge = find_language_bridge(ctx, ffi_ctx, target_language);
    if (!bridge) {
        polycall_performance_trace_end(ctx, ffi_ctx, ffi_ctx->perf_mgr, trace_entry);
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    err = bridge->call_function(ctx, function_name, args, arg_count, result);
    
    // Cache result if successful
    if (err == POLYCALL_CORE_SUCCESS) {
        polycall_performance_cache_result(
            ctx,
            ffi_ctx,
            ffi_ctx->perf_mgr,
            function_name,
            args,
            arg_count,
            result
        );
    }
    
    // End tracing
    polycall_performance_trace_end(ctx, ffi_ctx, ffi_ctx->perf_mgr, trace_entry);
    
    return err;
}

/**
 * @brief Utility function to duplicate a string using core memory allocation
 * 
 * @param ctx Core context
 * @param str String to duplicate
 * @return Duplicated string or NULL on failure
 */
static char* duplicate_string(polycall_core_context_t* ctx, const char* str) {
    if (!ctx || !str) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* dup = polycall_core_malloc(ctx, len);
    
    if (dup) {
        memcpy(dup, str, len);
    }
    
    return dup;
}

// Extension to handle function unregistration for module flexibility

/**
 * @brief Unregister a function from the FFI system
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name to unregister
 * @return Error code
 */
polycall_core_error_t polycall_ffi_unregister_function(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const char* function_name
) {
    // Validate parameters
    if (!ctx || !ffi_ctx || !function_name) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ffi_registry_t* registry = ffi_ctx->registry;
    
    // Find function index
    size_t index = SIZE_MAX;
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->functions[i].name, function_name) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == SIZE_MAX) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Free resources
    polycall_core_free(ctx, registry->functions[index].name);
    polycall_core_free(ctx, registry->functions[index].language);
    
    // Signature is kept by caller
    
    // Shift remaining entries
    if (index < registry->count - 1) {
        memmove(
            &registry->functions[index],
            &registry->functions[index + 1],
            (registry->count - index - 1) * sizeof(function_entry_t)
        );
    }
    
    registry->count--;
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get information about a registered function
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param function_name Function name
 * @param signature Pointer to receive function signature
 * @param source_language Pointer to receive source language
 * @param flags Pointer to receive function flags
 * @return Error code
 */
polycall_core_error_t polycall_ffi_get_function_info(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    const char* function_name,
    ffi_signature_t** signature,
    const char** source_language,
    uint32_t* flags
) {
    // Validate parameters
    if (!ctx || !ffi_ctx || !function_name) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    function_entry_t* func = find_function(ctx, ffi_ctx, function_name);
    if (!func) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Fill out requested information
    if (signature) {
        *signature = func->signature;
    }
    
    if (source_language) {
        *source_language = func->language;
    }
    
    if (flags) {
        *flags = func->flags;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Get information about registered languages
 * 
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param languages Buffer to receive language names
 * @param count Pointer to buffer size (in/out)
 * @return Error code
 */
polycall_core_error_t polycall_ffi_get_languages(
    polycall_core_context_t* ctx,
    polycall_ffi_context_t* ffi_ctx,
    char** languages,
    size_t* count
) {
    // Validate parameters
    if (!ctx || !ffi_ctx || !count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    ffi_registry_t* registry = ffi_ctx->registry;
    
    // If only count is requested
    if (!languages) {
        *count = registry->language_count;
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Return as many languages as buffer allows
    size_t to_copy = (*count < registry->language_count) ? 
                    *count : registry->language_count;
    
    for (size_t i = 0; i < to_copy; i++) {
        languages[i] = registry->languages[i].language;
    }
    
    // Return actual count
    *count = registry->language_count;
    
    return POLYCALL_CORE_SUCCESS;
}