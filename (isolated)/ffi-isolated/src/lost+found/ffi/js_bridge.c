/**
 * @file js_bridge.c
 * @brief JavaScript language bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the JavaScript language bridge for LibPolyCall FFI, providing
 * the ability to register and call JavaScript functions from other languages,
 * and to call functions in other languages from JavaScript.
 */

 #include "polycall/core/ffi/js_bridge.h"
 #include "polycall/core/ffi/ffi_core.h"
 #include "polycall/core/ffi/type_system.h"
 #include "polycall/core/ffi/memory_bridge.h"
 #include "polycall/core/ffi/security.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdlib.h>
 #include <string.h>
 #include <pthread.h>
 
 
 /**
  * @brief Initialize the JavaScript language bridge
  */
 polycall_core_error_t polycall_js_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t** js_bridge,
     const polycall_js_bridge_config_t* config
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 
     // Allocate memory for the JavaScript bridge
     polycall_js_bridge_t* new_bridge = polycall_core_malloc(ctx, sizeof(polycall_js_bridge_t));
     if (!new_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate JavaScript bridge");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 
     // Initialize bridge with configuration
     new_bridge->core_ctx = ctx;
     new_bridge->ffi_ctx = ffi_ctx;
     new_bridge->runtime_type = config->runtime_type;
     new_bridge->runtime_handle = config->runtime_handle;
     new_bridge->enable_promise_integration = config->enable_promise_integration;
     new_bridge->enable_callback_conversion = config->enable_callback_conversion;
     new_bridge->enable_object_proxying = config->enable_object_proxying;
     new_bridge->enable_exception_translation = config->enable_exception_translation;
     new_bridge->max_string_length = config->max_string_length;
     new_bridge->user_data = config->user_data;
 
     // Initialize mutex for runtime access
     if (pthread_mutex_init(&new_bridge->runtime_mutex, NULL) != 0) {
         polycall_core_free(ctx, new_bridge);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize JavaScript bridge mutex");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize function registry
     polycall_core_error_t result = init_js_function_registry(ctx, &new_bridge->function_registry, 256);
     if (result != POLYCALL_CORE_SUCCESS) {
         pthread_mutex_destroy(&new_bridge->runtime_mutex);
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     // Initialize promise registry if promise integration is enabled
     if (new_bridge->enable_promise_integration) {
         result = init_promise_registry(ctx, &new_bridge->promise_registry, 64);
         if (result != POLYCALL_CORE_SUCCESS) {
             cleanup_js_function_registry(ctx, &new_bridge->function_registry);
             pthread_mutex_destroy(&new_bridge->runtime_mutex);
             polycall_core_free(ctx, new_bridge);
             return result;
         }
     }
     
     // Setup runtime adapter based on the JavaScript runtime type
     result = setup_runtime_adapter(new_bridge);
     if (result != POLYCALL_CORE_SUCCESS) {
         if (new_bridge->enable_promise_integration) {
             cleanup_promise_registry(ctx, &new_bridge->promise_registry);
         }
         cleanup_js_function_registry(ctx, &new_bridge->function_registry);
         pthread_mutex_destroy(&new_bridge->runtime_mutex);
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     // Initialize the JavaScript runtime if needed
     if (new_bridge->adapter.initialize_runtime && !new_bridge->adapter.initialize_runtime(new_bridge->runtime_handle)) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to initialize JavaScript runtime");
         
         if (new_bridge->enable_promise_integration) {
             cleanup_promise_registry(ctx, &new_bridge->promise_registry);
         }
         cleanup_js_function_registry(ctx, &new_bridge->function_registry);
         pthread_mutex_destroy(&new_bridge->runtime_mutex);
         polycall_core_free(ctx, new_bridge);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Set up language bridge interface
     new_bridge->bridge_interface.language_name = "javascript";
     new_bridge->bridge_interface.version = "1.0.0";
     new_bridge->bridge_interface.convert_to_native = js_convert_to_native;
     new_bridge->bridge_interface.convert_from_native = js_convert_from_native;
     new_bridge->bridge_interface.register_function = js_register_function;
     new_bridge->bridge_interface.call_function = js_call_function;
     new_bridge->bridge_interface.acquire_memory = js_acquire_memory;
     new_bridge->bridge_interface.release_memory = js_release_memory;
     new_bridge->bridge_interface.handle_exception = js_handle_exception;
     new_bridge->bridge_interface.initialize = js_initialize;
     new_bridge->bridge_interface.cleanup = js_cleanup;
     new_bridge->bridge_interface.user_data = new_bridge;
     
     *js_bridge = new_bridge;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up JavaScript language bridge
  */
 void polycall_js_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge
 ) {
     if (!ctx || !js_bridge) {
         return;
     }
     
     // Clean up JavaScript runtime if necessary
     if (js_bridge->adapter.cleanup_runtime) {
         js_bridge->adapter.cleanup_runtime(js_bridge->runtime_handle);
     }
     
     // Clean up promise registry if used
     if (js_bridge->enable_promise_integration) {
         cleanup_promise_registry(ctx, &js_bridge->promise_registry);
     }
     
     // Clean up function registry
     cleanup_js_function_registry(ctx, &js_bridge->function_registry);
     
     // Destroy mutex
     pthread_mutex_destroy(&js_bridge->runtime_mutex);
     
     // Free bridge structure
     polycall_core_free(ctx, js_bridge);
 }
 
 /**
  * @brief Register a JavaScript function with the FFI system
  */
 polycall_core_error_t polycall_js_bridge_register_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     const char* function_name,
     void* js_function,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !function_name || !js_function || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Verify JavaScript function
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     bool is_function = js_bridge->adapter.is_function(js_bridge->runtime_handle, js_function);
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     
     if (!is_function) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Provided JavaScript object is not a function");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Lock function registry
     pthread_mutex_lock(&js_bridge->function_registry.mutex);
     
     // Check if function already exists
     if (find_js_function(&js_bridge->function_registry, function_name)) {
         pthread_mutex_unlock(&js_bridge->function_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "JavaScript function %s already registered", function_name);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (js_bridge->function_registry.count >= js_bridge->function_registry.capacity) {
         pthread_mutex_unlock(&js_bridge->function_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "JavaScript function registry capacity exceeded");
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Add function to registry
     js_function_t* func = &js_bridge->function_registry.functions[js_bridge->function_registry.count];
     
     // Duplicate function name
     func->name = polycall_core_malloc(ctx, strlen(function_name) + 1);
     if (!func->name) {
         pthread_mutex_unlock(&js_bridge->function_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for function name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy(func->name, function_name);
     
     // Retain JavaScript function reference
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     js_bridge->adapter.retain_value(js_bridge->runtime_handle, js_function);
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     
     // Set other fields
     func->js_function = js_function;
     memcpy(&func->signature, signature, sizeof(ffi_signature_t));
     func->flags = flags;
     
     // Increment count
     js_bridge->function_registry.count++;
     
     pthread_mutex_unlock(&js_bridge->function_registry.mutex);
     
     // Register with FFI system
     polycall_core_error_t result = polycall_ffi_expose_function(
         ctx, ffi_ctx, function_name, js_function, signature, "javascript", flags);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Clean up if FFI registration fails
         pthread_mutex_lock(&js_bridge->function_registry.mutex);
         
         // Decrement count
         js_bridge->function_registry.count--;
         
         // Release function reference
         pthread_mutex_lock(&js_bridge->runtime_mutex);
         js_bridge->adapter.release_value(js_bridge->runtime_handle, js_function);
         pthread_mutex_unlock(&js_bridge->runtime_mutex);
         
         // Free name
         polycall_core_free(ctx, func->name);
         
         pthread_mutex_unlock(&js_bridge->function_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to expose JavaScript function to FFI system");
     }
     
     return result;
 }
 
 /**
  * @brief Call a JavaScript function through the FFI system
  */
 polycall_core_error_t polycall_js_bridge_call_function(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !function_name || (!args && arg_count > 0) || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Find function
     pthread_mutex_lock(&js_bridge->function_registry.mutex);
     js_function_t* func = find_js_function(&js_bridge->function_registry, function_name);
     
     if (!func) {
         pthread_mutex_unlock(&js_bridge->function_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "JavaScript function %s not found", function_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Make local copies to safely use after unlocking
     void* js_function = func->js_function;
     
     pthread_mutex_unlock(&js_bridge->function_registry.mutex);
     
     // Convert FFI arguments to JavaScript values
     void** js_args = NULL;
     if (arg_count > 0) {
         js_args = polycall_core_malloc(ctx, arg_count * sizeof(void*));
         if (!js_args) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate memory for JavaScript arguments");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Convert each argument
         for (size_t i = 0; i < arg_count; i++) {
             polycall_core_error_t error = convert_ffi_to_js_value(ctx, js_bridge, &args[i], &js_args[i]);
             if (error != POLYCALL_CORE_SUCCESS) {
                 // Clean up already converted arguments
                 for (size_t j = 0; j < i; j++) {
                     pthread_mutex_lock(&js_bridge->runtime_mutex);
                     js_bridge->adapter.release_value(js_bridge->runtime_handle, js_args[j]);
                     pthread_mutex_unlock(&js_bridge->runtime_mutex);
                 }
                 
                 polycall_core_free(ctx, js_args);
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   error,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to convert FFI argument %zu to JavaScript value", i);
                 return error;
             }
         }
     }
     
     // Call JavaScript function
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     
     void* js_result = js_bridge->adapter.call_function(
         js_bridge->runtime_handle, js_function, NULL, js_args, arg_count);
     
     // Check for exceptions
     bool has_exception = js_bridge->adapter.has_exception(js_bridge->runtime_handle);
     void* exception = NULL;
     
     if (has_exception) {
         exception = js_bridge->adapter.get_exception(js_bridge->runtime_handle);
         js_bridge->adapter.clear_exception(js_bridge->runtime_handle);
     }
     
     // Clean up arguments
     if (js_args) {
         for (size_t i = 0; i < arg_count; i++) {
             js_bridge->adapter.release_value(js_bridge->runtime_handle, js_args[i]);
         }
     }
     
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     
     if (js_args) {
         polycall_core_free(ctx, js_args);
     }
     
     // Handle exceptions
     if (has_exception) {
         char error_message[256] = "JavaScript exception";
         
         if (js_bridge->enable_exception_translation && exception) {
             pthread_mutex_lock(&js_bridge->runtime_mutex);
             
             size_t msg_len = 0;
             char* exception_msg = js_bridge->adapter.get_exception_message(
                 js_bridge->runtime_handle, exception, &msg_len);
             
             if (exception_msg && msg_len > 0) {
                 size_t copy_len = msg_len < 255 ? msg_len : 255;
                 strncpy(error_message, exception_msg, copy_len);
                 error_message[copy_len] = '\0';
                 
                 // Free message if necessary (depends on runtime adapter)
                 if (js_bridge->adapter.release_value) {
                     js_bridge->adapter.release_value(js_bridge->runtime_handle, exception_msg);
                 }
             }
             
             // Release exception object
             js_bridge->adapter.release_value(js_bridge->runtime_handle, exception);
             
             pthread_mutex_unlock(&js_bridge->runtime_mutex);
         }
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_EXECUTION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "JavaScript execution failed: %s", error_message);
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     // Convert JavaScript result to FFI value
     if (!js_result) {
         // If no result (undefined or execution error), set a null result
         result->type = POLYCALL_FFI_TYPE_VOID;
         memset(&result->value, 0, sizeof(result->value));
     } else {
         polycall_core_error_t error = convert_js_to_ffi_value(
             ctx, js_bridge, js_result, POLYCALL_FFI_TYPE_VOID, result);
         
         // Release JavaScript result
         pthread_mutex_lock(&js_bridge->runtime_mutex);
         js_bridge->adapter.release_value(js_bridge->runtime_handle, js_result);
         pthread_mutex_unlock(&js_bridge->runtime_mutex);
         
         if (error != POLYCALL_CORE_SUCCESS) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               error,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to convert JavaScript result to FFI value");
             return error;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert FFI value to JavaScript value
  */
 polycall_core_error_t polycall_js_bridge_to_js_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     const ffi_value_t* ffi_value,
     void** js_value
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !ffi_value || !js_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return convert_ffi_to_js_value(ctx, js_bridge, ffi_value, js_value);
 }
 
 /**
  * @brief Convert JavaScript value to FFI value
  */
 polycall_core_error_t polycall_js_bridge_from_js_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     void* js_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !js_value || !ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return convert_js_to_ffi_value(ctx, js_bridge, js_value, expected_type, ffi_value);
 }
 
 /**
  * @brief Setup Promise handling for asynchronous operations
  */
 polycall_core_error_t polycall_js_bridge_setup_promise(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     const char* async_function_name,
     void* callback_fn,
     void* user_data
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !async_function_name || !callback_fn) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Check if promise integration is enabled
     if (!js_bridge->enable_promise_integration) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Promise integration is not enabled in this JavaScript bridge");
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Lock promise registry
     pthread_mutex_lock(&js_bridge->promise_registry.mutex);
     
     // Check if callback for this function already exists
     if (find_promise_callback(&js_bridge->promise_registry, async_function_name)) {
         pthread_mutex_unlock(&js_bridge->promise_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Promise callback for function %s already registered", async_function_name);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (js_bridge->promise_registry.count >= js_bridge->promise_registry.capacity) {
         pthread_mutex_unlock(&js_bridge->promise_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Promise callback registry capacity exceeded");
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Add callback to registry
     promise_callback_t* callback = &js_bridge->promise_registry.callbacks[js_bridge->promise_registry.count];
     
     // Duplicate function name
     callback->function_name = polycall_core_malloc(ctx, strlen(async_function_name) + 1);
     if (!callback->function_name) {
         pthread_mutex_unlock(&js_bridge->promise_registry.mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate memory for async function name");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy(callback->function_name, async_function_name);
     callback->callback_fn = callback_fn;
     callback->user_data = user_data;
     
     // Increment count
     js_bridge->promise_registry.count++;
     
     pthread_mutex_unlock(&js_bridge->promise_registry.mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Handle JavaScript exception
  */
 polycall_core_error_t polycall_js_bridge_handle_exception(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     void* js_exception,
     char* error_message,
     size_t message_size
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !js_exception || !error_message || message_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     
     // Extract exception message
     size_t msg_len = 0;
     char* exception_msg = js_bridge->adapter.get_exception_message(
         js_bridge->runtime_handle, js_exception, &msg_len);
     
     // Copy message to output buffer
     if (exception_msg && msg_len > 0) {
         size_t copy_len = msg_len < (message_size - 1) ? msg_len : (message_size - 1);
         strncpy(error_message, exception_msg, copy_len);
         error_message[copy_len] = '\0';
         
         // Free message if necessary (depends on runtime adapter)
         if (js_bridge->adapter.release_value) {
             js_bridge->adapter.release_value(js_bridge->runtime_handle, exception_msg);
         }
     } else {
         // Fallback message
         strncpy(error_message, "Unknown JavaScript exception", message_size - 1);
         error_message[message_size - 1] = '\0';
     }
     
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get language bridge interface for JavaScript
  */
 polycall_core_error_t polycall_js_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_js_bridge_t* js_bridge,
     language_bridge_t* bridge
 ) {
     if (!ctx || !ffi_ctx || !js_bridge || !bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     memcpy(bridge, &js_bridge->bridge_interface, sizeof(language_bridge_t));
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default JavaScript bridge configuration
  */
 polycall_js_bridge_config_t polycall_js_bridge_create_default_config(void) {
     polycall_js_bridge_config_t config;
     
     config.runtime_type = POLYCALL_JS_RUNTIME_NODE;  // Default to Node.js
     config.runtime_handle = NULL;                    // Must be provided by the caller
     config.enable_promise_integration = true;
     config.enable_callback_conversion = true;
     config.enable_object_proxying = false;           // Disabled by default for security
     config.enable_exception_translation = true;
     config.max_string_length = 1024 * 1024;          // 1MB default
     config.user_data = NULL;
     
     return config;
 }
 
 /*---------------------------------------------------------------------------*/
 /* Internal helper functions */
 /*---------------------------------------------------------------------------*/
 
 /**
  * @brief Initialize JavaScript function registry
  */
 static polycall_core_error_t init_js_function_registry(
     polycall_core_context_t* ctx,
     js_function_registry_t* registry,
     size_t capacity
 ) {
     // Allocate function array
     registry->functions = polycall_core_malloc(ctx, capacity * sizeof(js_function_t));
     if (!registry->functions) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     registry->count = 0;
     registry->capacity = capacity;
     
     // Initialize mutex
     if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
         polycall_core_free(ctx, registry->functions);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up JavaScript function registry
  */
 static void cleanup_js_function_registry(
     polycall_core_context_t* ctx,
     js_function_registry_t* registry
 ) {
     if (!registry->functions) {
         return;
     }
     
     // Clean up each function entry
     for (size_t i = 0; i < registry->count; i++) {
         polycall_core_free(ctx, registry->functions[i].name);
         // Note: JavaScript function references are released elsewhere
     }
     
     // Free array
     polycall_core_free(ctx, registry->functions);
     registry->functions = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     // Destroy mutex
     pthread_mutex_destroy(&registry->mutex);
 }
 
 /**
  * @brief Initialize promise registry
  */
 static polycall_core_error_t init_promise_registry(
     polycall_core_context_t* ctx,
     promise_registry_t* registry,
     size_t capacity
 ) {
     // Allocate callback array
     registry->callbacks = polycall_core_malloc(ctx, capacity * sizeof(promise_callback_t));
     if (!registry->callbacks) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     registry->count = 0;
     registry->capacity = capacity;
     
     // Initialize mutex
     if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
         polycall_core_free(ctx, registry->callbacks);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up promise registry
  */
 static void cleanup_promise_registry(
     polycall_core_context_t* ctx,
     promise_registry_t* registry
 ) {
     if (!registry->callbacks) {
         return;
     }
     
     // Clean up each callback entry
     for (size_t i = 0; i < registry->count; i++) {
         polycall_core_free(ctx, registry->callbacks[i].function_name);
     }
     
     // Free array
     polycall_core_free(ctx, registry->callbacks);
     registry->callbacks = NULL;
     registry->count = 0;
     registry->capacity = 0;
     
     // Destroy mutex
     pthread_mutex_destroy(&registry->mutex);
 }
 
 /**
  * @brief Find JavaScript function by name
  */
 static js_function_t* find_js_function(
     js_function_registry_t* registry,
     const char* name
 ) {
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->functions[i].name, name) == 0) {
             return &registry->functions[i];
         }
     }
     return NULL;
 }
 
 /**
  * @brief Find promise callback by function name
  */
 static promise_callback_t* find_promise_callback(
     promise_registry_t* registry,
     const char* name
 ) {
     for (size_t i = 0; i < registry->count; i++) {
         if (strcmp(registry->callbacks[i].function_name, name) == 0) {
             return &registry->callbacks[i];
         }
     }
     return NULL;
 }
 
 /**
  * @brief Setup runtime adapter based on JavaScript runtime type
  */
 static polycall_core_error_t setup_runtime_adapter(
     polycall_js_bridge_t* js_bridge
 ) {
     switch (js_bridge->runtime_type) {
         case POLYCALL_JS_RUNTIME_NODE:
             return setup_node_adapter(js_bridge);
         
         case POLYCALL_JS_RUNTIME_V8:
             return setup_v8_adapter(js_bridge);
         
         case POLYCALL_JS_RUNTIME_WEBKIT:
             return setup_webkit_adapter(js_bridge);
         
         case POLYCALL_JS_RUNTIME_SPIDERMONKEY:
             return setup_spidermonkey_adapter(js_bridge);
         
         case POLYCALL_JS_RUNTIME_QUICKJS:
             return setup_quickjs_adapter(js_bridge);
         
         case POLYCALL_JS_RUNTIME_CUSTOM:
             return setup_custom_adapter(js_bridge);
         
         default:
             POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Unsupported JavaScript runtime type: %d", js_bridge->runtime_type);
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
 }
 
 /**
  * @brief Convert FFI value to JavaScript value
  */
 static polycall_core_error_t convert_ffi_to_js_value(
     polycall_core_context_t* ctx,
     polycall_js_bridge_t* js_bridge,
     const ffi_value_t* ffi_value,
     void** js_value
 ) {
     if (!ctx || !js_bridge || !ffi_value || !js_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     
     // Convert based on FFI type
     switch (ffi_value->type) {
         case POLYCALL_FFI_TYPE_BOOL:
             *js_value = js_bridge->adapter.create_boolean(js_bridge->runtime_handle, ffi_value->value.bool_value);
             break;
             
         case POLYCALL_FFI_TYPE_CHAR:
         case POLYCALL_FFI_TYPE_INT8:
         case POLYCALL_FFI_TYPE_UINT8:
         case POLYCALL_FFI_TYPE_INT16:
         case POLYCALL_FFI_TYPE_UINT16:
         case POLYCALL_FFI_TYPE_INT32:
         case POLYCALL_FFI_TYPE_UINT32:
         case POLYCALL_FFI_TYPE_INT64:
         case POLYCALL_FFI_TYPE_UINT64:
         case POLYCALL_FFI_TYPE_FLOAT:
         case POLYCALL_FFI_TYPE_DOUBLE:
             // Convert numeric types
             double number_value = 0.0;
             
             switch (ffi_value->type) {
                 case POLYCALL_FFI_TYPE_CHAR:    number_value = (double)ffi_value->value.char_value; break;
                 case POLYCALL_FFI_TYPE_INT8:    number_value = (double)ffi_value->value.int8_value; break;
                 case POLYCALL_FFI_TYPE_UINT8:   number_value = (double)ffi_value->value.uint8_value; break;
                 case POLYCALL_FFI_TYPE_INT16:   number_value = (double)ffi_value->value.int16_value; break;
                 case POLYCALL_FFI_TYPE_UINT16:  number_value = (double)ffi_value->value.uint16_value; break;
                 case POLYCALL_FFI_TYPE_INT32:   number_value = (double)ffi_value->value.int32_value; break;
                 case POLYCALL_FFI_TYPE_UINT32:  number_value = (double)ffi_value->value.uint32_value; break;
                 case POLYCALL_FFI_TYPE_INT64:   number_value = (double)ffi_value->value.int64_value; break;
                 case POLYCALL_FFI_TYPE_UINT64:  number_value = (double)ffi_value->value.uint64_value; break;
                 case POLYCALL_FFI_TYPE_FLOAT:   number_value = (double)ffi_value->value.float_value; break;
                 case POLYCALL_FFI_TYPE_DOUBLE:  number_value = ffi_value->value.double_value; break;
                 default: break; // Should never reach here
             }
             
             *js_value = js_bridge->adapter.create_number(js_bridge->runtime_handle, number_value);
             break;
             
         case POLYCALL_FFI_TYPE_STRING:
             if (ffi_value->value.string_value) {
                 size_t str_len = strlen(ffi_value->value.string_value);
                 // Limit string length if needed
                 if (js_bridge->max_string_length > 0 && str_len > js_bridge->max_string_length) {
                     str_len = js_bridge->max_string_length;
                 }
                 *js_value = js_bridge->adapter.create_string(
                     js_bridge->runtime_handle, ffi_value->value.string_value, str_len);
             } else {
                 // Null string converts to JavaScript null
                 *js_value = js_bridge->adapter.create_null(js_bridge->runtime_handle);
             }
             break;
             
         case POLYCALL_FFI_TYPE_POINTER:
             // Pointers require special handling - we wrap them in an object
             {
                 // Create object to wrap pointer
                 void* obj = js_bridge->adapter.create_object(js_bridge->runtime_handle);
                 
                 // Create a string property name
                 void* prop_name = js_bridge->adapter.create_string(
                     js_bridge->runtime_handle, "address", 7);
                 
                 // Create a number for the pointer address
                 uintptr_t ptr_value = (uintptr_t)ffi_value->value.pointer_value;
                 void* ptr_js_value = js_bridge->adapter.create_number(
                     js_bridge->runtime_handle, (double)ptr_value);
                 
                 // Set the property
                 // Note: This is a simplified approach; actual implementation would depend on
                 // the specific JavaScript runtime adapter's object property manipulation API
                 // For now, we'll assume we have a function to set object properties
 
                 // Release temporary values
                 js_bridge->adapter.release_value(js_bridge->runtime_handle, prop_name);
                 js_bridge->adapter.release_value(js_bridge->runtime_handle, ptr_js_value);
                 
                 *js_value = obj;
             }
             break;
             
         case POLYCALL_FFI_TYPE_VOID:
             // Void corresponds to JavaScript undefined
             *js_value = NULL;
             break;
             
         case POLYCALL_FFI_TYPE_STRUCT:
             // Convert struct to JavaScript object
             if (ffi_value->value.struct_value && ffi_value->type_info) {
                 // This is a simplified implementation
                 // A real implementation would use the struct field information
                 // to create a proper JavaScript object with corresponding properties
                 *js_value = js_bridge->adapter.create_object(js_bridge->runtime_handle);
             } else {
                 *js_value = js_bridge->adapter.create_null(js_bridge->runtime_handle);
             }
             break;
             
         case POLYCALL_FFI_TYPE_ARRAY:
             // Convert array to JavaScript array
             if (ffi_value->value.array_value && ffi_value->type_info) {
                 // This is a simplified implementation
                 // A real implementation would iterate through the array elements
                 // and convert each one to the corresponding JavaScript value
                 size_t array_length = ffi_value->type_info->details.array_info.element_count;
                 *js_value = js_bridge->adapter.create_array(js_bridge->runtime_handle, array_length);
             } else {
                 *js_value = js_bridge->adapter.create_array(js_bridge->runtime_handle, 0);
             }
             break;
             
         case POLYCALL_FFI_TYPE_CALLBACK:
             // For callbacks, create a JavaScript function that will invoke the native callback
             if (js_bridge->enable_callback_conversion && ffi_value->value.callback_value) {
                 // In a real implementation, we would create a JavaScript function that,
                 // when called, would invoke the native callback with converted arguments
                 // For now, we'll just create a placeholder function
                 *js_value = js_bridge->adapter.create_function(
                     js_bridge->runtime_handle, ffi_value->value.callback_value, NULL);
             } else {
                 *js_value = js_bridge->adapter.create_null(js_bridge->runtime_handle);
             }
             break;
             
         case POLYCALL_FFI_TYPE_OBJECT:
             // For generic objects, we'll need to proxy them
             if (js_bridge->enable_object_proxying && ffi_value->value.object_value) {
                 // In a real implementation, we would create a proxy object that
                 // handles property access and method calls on the native object
                 // For now, we'll just create a placeholder object
                 *js_value = js_bridge->adapter.create_object(js_bridge->runtime_handle);
             } else {
                 *js_value = js_bridge->adapter.create_null(js_bridge->runtime_handle);
             }
             break;
             
         default:
             // Unsupported type - return null
             *js_value = js_bridge->adapter.create_null(js_bridge->runtime_handle);
             pthread_mutex_unlock(&js_bridge->runtime_mutex);
             
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Unsupported FFI type for conversion to JavaScript: %d", ffi_value->type);
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Check if the conversion was successful
     if (!*js_value && ffi_value->type != POLYCALL_FFI_TYPE_VOID) {
         pthread_mutex_unlock(&js_bridge->runtime_mutex);
         
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CONVERSION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to convert FFI value to JavaScript value");
         return POLYCALL_CORE_ERROR_CONVERSION_FAILED;
     }
     
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert JavaScript value to FFI value
  */
 static polycall_core_error_t convert_js_to_ffi_value(
     polycall_core_context_t* ctx,
     polycall_js_bridge_t* js_bridge,
     void* js_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ctx || !js_bridge || !js_value || !ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize FFI value
     memset(ffi_value, 0, sizeof(ffi_value_t));
     
     pthread_mutex_lock(&js_bridge->runtime_mutex);
     
     // Determine JavaScript value type
     bool is_number = js_bridge->adapter.is_number(js_bridge->runtime_handle, js_value);
     bool is_string = js_bridge->adapter.is_string(js_bridge->runtime_handle, js_value);
     bool is_boolean = js_bridge->adapter.is_boolean(js_bridge->runtime_handle, js_value);
     bool is_object = js_bridge->adapter.is_object(js_bridge->runtime_handle, js_value);
     bool is_null = js_bridge->adapter.is_null(js_bridge->runtime_handle, js_value);
     bool is_undefined = js_bridge->adapter.is_undefined(js_bridge->runtime_handle, js_value);
     bool is_array = js_bridge->adapter.is_array(js_bridge->runtime_handle, js_value);
     bool is_function = js_bridge->adapter.is_function(js_bridge->runtime_handle, js_value);
     
     // Handle conversion based on expected type or JavaScript type
     if (expected_type == POLYCALL_FFI_TYPE_VOID) {
         // If no expected type is specified, infer from JavaScript type
         if (is_boolean) {
             ffi_value->type = POLYCALL_FFI_TYPE_BOOL;
             ffi_value->value.bool_value = js_bridge->adapter.get_boolean(js_bridge->runtime_handle, js_value);
         } else if (is_number) {
             ffi_value->type = POLYCALL_FFI_TYPE_DOUBLE;
             ffi_value->value.double_value = js_bridge->adapter.get_number(js_bridge->runtime_handle, js_value);
         } else if (is_string) {
             size_t str_len = 0;
             char* str_value = js_bridge->adapter.get_string(js_bridge->runtime_handle, js_value, &str_len);
             
             if (str_value && str_len > 0) {
                 // Allocate memory for the string
                 char* str_copy = polycall_core_malloc(ctx, str_len + 1);
                 if (!str_copy) {
                     if (js_bridge->adapter.release_value) {
                         js_bridge->adapter.release_value(js_bridge->runtime_handle, str_value);
                     }
                     
                     pthread_mutex_unlock(&js_bridge->runtime_mutex);
                     return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                 }
                 
                 // Copy string data
                 memcpy(str_copy, str_value, str_len);
                 str_copy[str_len] = '\0';
                 
                 ffi_value->type = POLYCALL_FFI_TYPE_STRING;
                 ffi_value->value.string_value = str_copy;
                 
                 // Free JavaScript string if necessary
                 if (js_bridge->adapter.release_value) {
                     js_bridge->adapter.release_value(js_bridge->runtime_handle, str_value);
                 }
             } else {
                 // Empty or null string
                 ffi_value->type = POLYCALL_FFI_TYPE_STRING;
                 ffi_value->value.string_value = NULL;
             }
         } else if (is_null || is_undefined) {
             ffi_value->type = POLYCALL_FFI_TYPE_VOID;
         } else if (is_array) {
             // Convert JavaScript array to FFI array
             // This is a simplified implementation
             ffi_value->type = POLYCALL_FFI_TYPE_ARRAY;
             ffi_value->value.array_value = NULL;  // Actual implementation would create array
         } else if (is_function) {
             // Convert JavaScript function to FFI callback
             ffi_value->type = POLYCALL_FFI_TYPE_CALLBACK;
             ffi_value->value.callback_value = js_value;  // This is a simplification
         } else if (is_object) {
             // Convert JavaScript object to FFI struct or object
             ffi_value->type = POLYCALL_FFI_TYPE_OBJECT;
             ffi_value->value.object_value = js_value;  // This is a simplification
         } else {
             // Unknown type - default to void
             ffi_value->type = POLYCALL_FFI_TYPE_VOID;
         }
     } else {
         // Convert to the expected type
         ffi_value->type = expected_type;
         
         switch (expected_type) {
             case POLYCALL_FFI_TYPE_BOOL:
                 // Convert to boolean
                 if (is_boolean) {
                     ffi_value->value.bool_value = js_bridge->adapter.get_boolean(js_bridge->runtime_handle, js_value);
                 } else if (is_number) {
                     double num_val = js_bridge->adapter.get_number(js_bridge->runtime_handle, js_value);
                     ffi_value->value.bool_value = (num_val != 0.0);
                 } else if (is_string) {
                     // Non-empty string is true
                     size_t str_len = 0;
                     js_bridge->adapter.get_string(js_bridge->runtime_handle, js_value, &str_len);
                     ffi_value->value.bool_value = (str_len > 0);
                 } else if (is_null || is_undefined) {
                     ffi_value->value.bool_value = false;
                 } else {
                     // Objects, arrays, and functions are true
                     ffi_value->value.bool_value = true;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_CHAR:
             case POLYCALL_FFI_TYPE_INT8:
             case POLYCALL_FFI_TYPE_UINT8:
             case POLYCALL_FFI_TYPE_INT16:
             case POLYCALL_FFI_TYPE_UINT16:
             case POLYCALL_FFI_TYPE_INT32:
             case POLYCALL_FFI_TYPE_UINT32:
             case POLYCALL_FFI_TYPE_INT64:
             case POLYCALL_FFI_TYPE_UINT64:
             case POLYCALL_FFI_TYPE_FLOAT:
             case POLYCALL_FFI_TYPE_DOUBLE:
                 // Convert to numeric types
                 double num_val = 0.0;
                 
                 if (is_number) {
                     num_val = js_bridge->adapter.get_number(js_bridge->runtime_handle, js_value);
                 } else if (is_boolean) {
                     num_val = js_bridge->adapter.get_boolean(js_bridge->runtime_handle, js_value) ? 1.0 : 0.0;
                 } else if (is_string) {
                     // Try to parse string as number (simplified)
                     size_t str_len = 0;
                     char* str_value = js_bridge->adapter.get_string(js_bridge->runtime_handle, js_value, &str_len);
                     
                     if (str_value && str_len > 0) {
                         num_val = atof(str_value);
                         
                         // Free JavaScript string if necessary
                         if (js_bridge->adapter.release_value) {
                             js_bridge->adapter.release_value(js_bridge->runtime_handle, str_value);
                         }
                     }
                 }
                 
                 // Assign to the appropriate numeric field
                 switch (expected_type) {
                     case POLYCALL_FFI_TYPE_CHAR:    ffi_value->value.char_value = (char)num_val; break;
                     case POLYCALL_FFI_TYPE_INT8:    ffi_value->value.int8_value = (int8_t)num_val; break;
                     case POLYCALL_FFI_TYPE_UINT8:   ffi_value->value.uint8_value = (uint8_t)num_val; break;
                     case POLYCALL_FFI_TYPE_INT16:   ffi_value->value.int16_value = (int16_t)num_val; break;
                     case POLYCALL_FFI_TYPE_UINT16:  ffi_value->value.uint16_value = (uint16_t)num_val; break;
                     case POLYCALL_FFI_TYPE_INT32:   ffi_value->value.int32_value = (int32_t)num_val; break;
                     case POLYCALL_FFI_TYPE_UINT32:  ffi_value->value.uint32_value = (uint32_t)num_val; break;
                     case POLYCALL_FFI_TYPE_INT64:   ffi_value->value.int64_value = (int64_t)num_val; break;
                     case POLYCALL_FFI_TYPE_UINT64:  ffi_value->value.uint64_value = (uint64_t)num_val; break;
                     case POLYCALL_FFI_TYPE_FLOAT:   ffi_value->value.float_value = (float)num_val; break;
                     case POLYCALL_FFI_TYPE_DOUBLE:  ffi_value->value.double_value = num_val; break;
                     default: break;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_STRING:
                 // Convert to string
                 if (is_string) {
                     // Get JavaScript string
                     size_t str_len = 0;
                     char* str_value = js_bridge->adapter.get_string(js_bridge->runtime_handle, js_value, &str_len);
                     
                     if (str_value && str_len > 0) {
                         // Allocate memory for the string
                         char* str_copy = polycall_core_malloc(ctx, str_len + 1);
                         if (!str_copy) {
                             if (js_bridge->adapter.release_value) {
                                 js_bridge->adapter.release_value(js_bridge->runtime_handle, str_value);
                             }
                             
                             pthread_mutex_unlock(&js_bridge->runtime_mutex);
                             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                         }
                         
                         // Copy string data
                         memcpy(str_copy, str_value, str_len);
                         str_copy[str_len] = '\0';
                         
                         ffi_value->value.string_value = str_copy;
                         
                         // Free JavaScript string if necessary
                         if (js_bridge->adapter.release_value) {
                             js_bridge->adapter.release_value(js_bridge->runtime_handle, str_value);
                         }
                     } else {
                         // Empty string
                         ffi_value->value.string_value = NULL;
                     }
                 } else if (is_number) {
                     // Convert number to string
                     double num_val = js_bridge->adapter.get_number(js_bridge->runtime_handle, js_value);
                     
                     // Allocate a buffer for the number string (20 chars should be enough for most numbers)
                     char* str_copy = polycall_core_malloc(ctx, 32);
                     if (!str_copy) {
                         pthread_mutex_unlock(&js_bridge->runtime_mutex);
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                     
                     // Format the number as string
                     snprintf(str_copy, 32, "%g", num_val);
                     ffi_value->value.string_value = str_copy;
                 } else if (is_boolean) {
                     // Convert boolean to "true" or "false" string
                     bool bool_val = js_bridge->adapter.get_boolean(js_bridge->runtime_handle, js_value);
                     const char* bool_str = bool_val ? "true" : "false";
                     
                     // Allocate and copy the string
                     size_t str_len = strlen(bool_str);
                     char* str_copy = polycall_core_malloc(ctx, str_len + 1);
                     if (!str_copy) {
                         pthread_mutex_unlock(&js_bridge->runtime_mutex);
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                     
                     memcpy(str_copy, bool_str, str_len + 1);
                     ffi_value->value.string_value = str_copy;
                 } else if (is_null || is_undefined) {
                     // Null or undefined becomes NULL string
                     ffi_value->value.string_value = NULL;
                 } else {
                     // Objects, arrays, functions - just use a placeholder string
                     const char* obj_str = "[object Object]";
                     size_t str_len = strlen(obj_str);
                     
                     char* str_copy = polycall_core_malloc(ctx, str_len + 1);
                     if (!str_copy) {
                         pthread_mutex_unlock(&js_bridge->runtime_mutex);
                         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
                     }
                     
                     memcpy(str_copy, obj_str, str_len + 1);
                     ffi_value->value.string_value = str_copy;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_POINTER:
                 // Convert to pointer - mainly for wrapped pointers in objects
                 if (is_object) {
                     // Try to extract pointer value from object
                     // This is a simplification - actual implementation would depend on
                     // how pointers are wrapped in JavaScript objects
                     void* ptr_value = NULL;
                     ffi_value->value.pointer_value = ptr_value;
                 } else {
                     // Non-object values can't be converted to pointers
                     ffi_value->value.pointer_value = NULL;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_STRUCT:
                 // Convert to struct - for JavaScript objects
                 if (is_object && !is_array && !is_null && !is_function) {
                     // This is a simplified implementation
                     // A real implementation would extract object properties
                     // and populate the struct fields accordingly
                     ffi_value->value.struct_value = NULL;
                 } else {
                     // Non-object values can't be converted to structs
                     ffi_value->value.struct_value = NULL;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_ARRAY:
                 // Convert to array - for JavaScript arrays
                 if (is_array) {
                     // This is a simplified implementation
                     // A real implementation would extract array elements
                     // and convert them to the appropriate FFI values
                     ffi_value->value.array_value = NULL;
                 } else {
                     // Non-array values can't be converted to arrays
                     ffi_value->value.array_value = NULL;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_CALLBACK:
                 // Convert to callback - for JavaScript functions
                 if (is_function && js_bridge->enable_callback_conversion) {
                     // This is a simplified implementation
                     // A real implementation would create a C function that calls back
                     // to the JavaScript function with converted arguments
                     ffi_value->value.callback_value = js_value;
                 } else {
                     // Non-function values can't be converted to callbacks
                     ffi_value->value.callback_value = NULL;
                 }
                 break;
                 
             case POLYCALL_FFI_TYPE_VOID:
                 // No conversion needed for void
                 break;
                 
             default:
                 // Unsupported type
                 pthread_mutex_unlock(&js_bridge->runtime_mutex);
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Unsupported FFI type for conversion from JavaScript: %d", expected_type);
                 return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
         }
     }
     
     pthread_mutex_unlock(&js_bridge->runtime_mutex);
     return POLYCALL_CORE_SUCCESS;
 }
 
 /*---------------------------------------------------------------------------*/
 /* JavaScript runtime adapter implementations */
 /*---------------------------------------------------------------------------*/
 
 /**
  * @brief Set up Node.js runtime adapter
  */
 static polycall_core_error_t setup_node_adapter(polycall_js_bridge_t* js_bridge) {
     // Node.js adapter implementation
     // This is a placeholder - actual implementation would use Node API (N-API)
     POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "Node.js adapter not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Set up V8 runtime adapter
  */
 static polycall_core_error_t setup_v8_adapter(polycall_js_bridge_t* js_bridge) {
     // V8 adapter implementation
     // This is a placeholder - actual implementation would use V8 API
     POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "V8 adapter not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Set up WebKit/JavaScriptCore runtime adapter
  */
 static polycall_core_error_t setup_webkit_adapter(polycall_js_bridge_t* js_bridge) {
     // WebKit/JavaScriptCore adapter implementation
     // This is a placeholder - actual implementation would use JSC API
     POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "WebKit/JavaScriptCore adapter not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Set up SpiderMonkey runtime adapter
  */
 static polycall_core_error_t setup_spidermonkey_adapter(polycall_js_bridge_t* js_bridge) {
     // SpiderMonkey adapter implementation
     // This is a placeholder - actual implementation would use SpiderMonkey API
     POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "SpiderMonkey adapter not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Set up QuickJS runtime adapter
  */
 static polycall_core_error_t setup_quickjs_adapter(polycall_js_bridge_t* js_bridge) {
     // QuickJS adapter implementation
     // This is a placeholder - actual implementation would use QuickJS API
     POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "QuickJS adapter not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Set up custom runtime adapter
  */
 static polycall_core_error_t setup_custom_adapter(polycall_js_bridge_t* js_bridge) {
     // Custom adapter implementation
     // This assumes that the runtime_handle is already initialized with custom adapter functions
     // and just needs to be assigned to the bridge adapter
     
     // Custom adapter should be provided by the caller
     if (!js_bridge->runtime_handle) {
         POLYCALL_ERROR_SET(js_bridge->core_ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Custom JavaScript runtime adapter requires a valid runtime handle");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // The custom runtime adapter should be pre-configured with function pointers
     // that match the js_runtime_adapter_t structure
     
     // Since we can't make assumptions about the custom runtime's API,
     // we need to verify that the required functions are available
     
     // This would involve checking function pointers in the runtime_handle
     // to ensure they're not NULL
     
     // For now, we'll just return success
     return POLYCALL_CORE_SUCCESS;
 }
 
 /*---------------------------------------------------------------------------*/
 /* Language bridge interface implementation */
 /*---------------------------------------------------------------------------*/
 
 /**
  * @brief Convert FFI value to native JavaScript value
  */
 static polycall_core_error_t js_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 ) {
     // This function is called by the FFI system to convert an FFI value to a native value
     // In the case of JavaScript, this means converting to a JavaScript engine-specific value
     
     // Implementation would depend on the specific JavaScript runtime
     // and how it represents values in memory
     
     // For now, we'll just return a generic error
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "Direct FFI-to-native JavaScript conversion not supported");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Convert native JavaScript value to FFI value
  */
 static polycall_core_error_t js_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 ) {
     // This function is called by the FFI system to convert a native value to an FFI value
     // In the case of JavaScript, this means converting from a JavaScript engine-specific value
     
     // Implementation would depend on the specific JavaScript runtime
     // and how it represents values in memory
     
     // For now, we'll just return a generic error
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "Direct native JavaScript-to-FFI conversion not supported");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Register a function for JavaScript
  */
 static polycall_core_error_t js_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     // This is called when another language wants to register a function for JavaScript to call
     
     // Get the JavaScript bridge from the user_data field
     polycall_js_bridge_t* js_bridge = NULL;
     
     // In reality, we'd need to find the js_bridge instance, perhaps from a global registry
     // or from ctx/function_ptr (depends on how the FFI system is implemented)
     
     if (!js_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find JavaScript bridge instance");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Implementation would create a JavaScript function that wraps the native function
     // Then register that JavaScript function in the JavaScript runtime
     
     // For now, we'll just return a generic error
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "Registering native functions for JavaScript not yet implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 /**
  * @brief Call a JavaScript function
  */
 static polycall_core_error_t js_call_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     // This is called when another language wants to call a JavaScript function
     
     // Get the JavaScript bridge from the user_data field
     polycall_js_bridge_t* js_bridge = NULL;
     
     // In reality, we'd need to find the js_bridge instance, perhaps from a global registry
     // or from ctx/function_name (depends on how the FFI system is implemented)
     
     if (!js_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find JavaScript bridge instance");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Call the function using our utility function
     return polycall_js_bridge_call_function(ctx, js_bridge->ffi_ctx, js_bridge, function_name, args, arg_count, result);
 }
 
 /**
  * @brief Acquire memory for JavaScript
  */
 static polycall_core_error_t js_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 ) {
     // This is called when JavaScript needs to acquire memory allocated by another language
     
     // Get the JavaScript bridge from the user_data field
     polycall_js_bridge_t* js_bridge = NULL;
     
     // In reality, we'd need to find the js_bridge instance, perhaps from a global registry
     // or from ctx/ptr (depends on how the FFI system is implemented)
     
     if (!js_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find JavaScript bridge instance");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // For JavaScript, we could create an ArrayBuffer that views the native memory
     // or we could copy the memory to a JavaScript ArrayBuffer
     
     // For now, we'll just return success (no-op)
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Release memory for JavaScript
  */
 static polycall_core_error_t js_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 ) {
     // This is called when JavaScript needs to release memory it acquired from another language
     
     // Get the JavaScript bridge from the user_data field
     polycall_js_bridge_t* js_bridge = NULL;
     
     // In reality, we'd need to find the js_bridge instance, perhaps from a global registry
     // or from ctx/ptr (depends on how the FFI system is implemented)
     
     if (!js_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find JavaScript bridge instance");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // For JavaScript, we would need to release any ArrayBuffer that's viewing the native memory
     
     // For now, we'll just return success (no-op)
     return POLYCALL_CORE_SUCCESS;
 }
 
/**
 * @brief Handle JavaScript exception
 */
static polycall_core_error_t js_handle_exception(
    polycall_core_context_t* ctx,
    void* exception,
    char* message,
    size_t message_size
) {
    // This is called when a JavaScript exception needs to be handled
    
    // Get the JavaScript bridge from the user_data field
    polycall_js_bridge_t* js_bridge = NULL;
    
    // In reality, we'd need to find the js_bridge instance, perhaps from a global registry
    // or from ctx (depends on how the FFI system is implemented)
    if (ctx && ctx->user_data) {
        js_bridge = (polycall_js_bridge_t*)ctx->user_data;
    }
    
    if (!js_bridge) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to find JavaScript bridge instance");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Use the bridge's handle_exception function
    return polycall_js_bridge_handle_exception(
        ctx, js_bridge->ffi_ctx, js_bridge, exception, message, message_size);
}

/**
 * @brief Get JavaScript environment for current thread
 */
static polycall_core_error_t js_get_env(
    polycall_core_context_t* ctx,
    void** js_env
) {
    if (!ctx || !js_env) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get the JavaScript bridge from the user_data field
    polycall_js_bridge_t* js_bridge = NULL;
    
    // In a real implementation, we'd likely retrieve the bridge from a registry
    // For this implementation, we assume it's stored in the context's user_data
    if (ctx && ctx->user_data) {
        js_bridge = (polycall_js_bridge_t*)ctx->user_data;
    }
    
    if (!js_bridge) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR, 
                          "Failed to find JavaScript bridge instance");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    // Lock the runtime mutex to ensure thread safety
    pthread_mutex_lock(&js_bridge->runtime_mutex);
    
    // Get the environment handle
    *js_env = js_bridge->runtime_handle;
    
    // Unlock the mutex
    pthread_mutex_unlock(&js_bridge->runtime_mutex);
    
    if (!*js_env) {
        POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI,
                          POLYCALL_CORE_ERROR_INVALID_STATE,
                          POLYCALL_ERROR_SEVERITY_ERROR,
                          "JavaScript environment handle is NULL");
        return POLYCALL_CORE_ERROR_INVALID_STATE;
    }
    
    return POLYCALL_CORE_SUCCESS;
}