/**
 * @file js_bridge.h
 * @brief JavaScript language bridge for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the JavaScript language bridge for LibPolyCall FFI,
 * providing an interface for JavaScript code to interact with other languages
 * through the FFI system.
 */

#ifndef POLYCALL_FFI_JS_BRIDGE_H_H
#define POLYCALL_FFI_JS_BRIDGE_H_H

#include "polycall/core/ffi/ffi_core.h"
#include "polycall/core/polycall/polycall_core.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JavaScript bridge handle (opaque)
 */
typedef struct polycall_js_bridge polycall_js_bridge_t;

/**
 * @brief JavaScript runtime type enumeration
 */
typedef enum {
  POLYCALL_JS_RUNTIME_NODE = 0,     /**< Node.js runtime */
  POLYCALL_JS_RUNTIME_V8,           /**< V8 JavaScript engine */
  POLYCALL_JS_RUNTIME_WEBKIT,       /**< WebKit JavaScript engine */
  POLYCALL_JS_RUNTIME_SPIDERMONKEY, /**< SpiderMonkey JavaScript engine */
  POLYCALL_JS_RUNTIME_QUICKJS,      /**< QuickJS JavaScript engine */
  POLYCALL_JS_RUNTIME_CUSTOM,       /**< Custom JavaScript runtime */
} polycall_js_runtime_type_t;

/**
 * @brief JavaScript bridge configuration
 */
typedef struct {
  polycall_js_runtime_type_t runtime_type; /**< JavaScript runtime type */
  void *runtime_handle;                    /**< JavaScript runtime handle */
  bool enable_promise_integration;         /**< Enable Promise integration */
  bool enable_callback_conversion; /**< Enable function callback conversion */
  bool enable_object_proxying; /**< Enable object proxying between runtimes */
  bool enable_exception_translation; /**< Enable JavaScript exception
                                        translation */
  size_t max_string_length; /**< Maximum string length for conversions */
  void *user_data;          /**< User data */
} polycall_js_bridge_config_t;

/**
 * @brief Initialize the JavaScript language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge Pointer to receive JavaScript bridge handle
 * @param config Bridge configuration
 * @return Error code
 */
polycall_core_error_t
polycall_js_bridge_init(polycall_core_context_t *ctx,
                        polycall_ffi_context_t *ffi_ctx,
                        polycall_js_bridge_t **js_bridge,
                        const polycall_js_bridge_config_t *config);

/**
 * @brief Clean up JavaScript language bridge
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle to clean up
 */
void polycall_js_bridge_cleanup(polycall_core_context_t *ctx,
                                polycall_ffi_context_t *ffi_ctx,
                                polycall_js_bridge_t *js_bridge);

/**
 * @brief Register a JavaScript function with the FFI system
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param function_name Function name
 * @param js_function JavaScript function object
 * @param signature Function signature
 * @param flags Function flags
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_register_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, const char *function_name,
    void *js_function, ffi_signature_t *signature, uint32_t flags);

/**
 * @brief Call a JavaScript function through the FFI system
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param function_name Function name
 * @param args Function arguments
 * @param arg_count Argument count
 * @param result Pointer to receive function result
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_call_function(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, const char *function_name,
    ffi_value_t *args, size_t arg_count, ffi_value_t *result);

/**
 * @brief Convert FFI value to JavaScript value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param ffi_value FFI value to convert
 * @param js_value Pointer to receive JavaScript value
 * @return Error code
 */
polycall_core_error_t
polycall_js_bridge_to_js_value(polycall_core_context_t *ctx,
                               polycall_ffi_context_t *ffi_ctx,
                               polycall_js_bridge_t *js_bridge,
                               const ffi_value_t *ffi_value, void **js_value);

/**
 * @brief Convert JavaScript value to FFI value
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param js_value JavaScript value to convert
 * @param expected_type Expected FFI type
 * @param ffi_value Pointer to receive FFI value
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_from_js_value(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, void *js_value,
    polycall_ffi_type_t expected_type, ffi_value_t *ffi_value);

/**
 * @brief Set up Promise handling for asynchronous operations
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param async_function_name Function name
 * @param callback_fn Callback function
 * @param user_data User data for callback
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_setup_promise(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, const char *async_function_name,
    void *callback_fn, void *user_data);

/**
 * @brief Handle JavaScript exception
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param js_exception JavaScript exception object
 * @param error_message Buffer to receive error message
 * @param message_size Size of error message buffer
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_handle_exception(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, void *js_exception, char *error_message,
    size_t message_size);

/**
 * @brief Get language bridge interface for JavaScript
 *
 * @param ctx Core context
 * @param ffi_ctx FFI context
 * @param js_bridge JavaScript bridge handle
 * @param bridge Pointer to receive language bridge interface
 * @return Error code
 */
polycall_core_error_t polycall_js_bridge_get_interface(
    polycall_core_context_t *ctx, polycall_ffi_context_t *ffi_ctx,
    polycall_js_bridge_t *js_bridge, language_bridge_t *bridge);

/**
 * @brief Create a default JavaScript bridge configuration
 *
 * @return Default configuration
 */
polycall_js_bridge_config_t polycall_js_bridge_create_default_config(void);

// Define error sources if not already defined
#ifndef POLYCALL_FFI_JS_BRIDGE_H_H
#define POLYCALL_FFI_JS_BRIDGE_H_H
#endif

// JavaScript function entry
typedef struct {
  char *name;                // Function name
  void *js_function;         // JavaScript function object
  ffi_signature_t signature; // Function signature
  uint32_t flags;            // Function flags
} js_function_t;

// JavaScript function registry
typedef struct {
  js_function_t *functions; // Array of functions
  size_t count;             // Current number of functions
  size_t capacity;          // Maximum number of functions
  pthread_mutex_t mutex;    // Thread safety mutex
} js_function_registry_t;

// Promise callback entry
typedef struct {
  char *function_name; // Async function name
  void *callback_fn;   // Callback function
  void *user_data;     // User data for callback
} promise_callback_t;

// Promise registry
typedef struct {
  promise_callback_t *callbacks; // Array of callbacks
  size_t count;                  // Current number of callbacks
  size_t capacity;               // Maximum number of callbacks
  pthread_mutex_t mutex;         // Thread safety mutex
} promise_registry_t;

// Runtime adapter function pointers
typedef struct {
  // JavaScript value creation/manipulation
  void *(*create_number)(void *runtime, double value);
  void *(*create_string)(void *runtime, const char *value, size_t length);
  void *(*create_boolean)(void *runtime, bool value);
  void *(*create_object)(void *runtime);
  void *(*create_null)(void *runtime);
  void *(*create_array)(void *runtime, size_t length);
  void *(*create_array_buffer)(void *runtime, const void *data, size_t length);

  // JavaScript value extraction
  bool (*get_boolean)(void *runtime, void *value);
  double (*get_number)(void *runtime, void *value);
  char *(*get_string)(void *runtime, void *value, size_t *length);
  void *(*get_object_property)(void *runtime, void *object,
                               const char *property);
  void *(*get_array_element)(void *runtime, void *array, size_t index);
  void *(*get_array_buffer_data)(void *runtime, void *buffer, size_t *length);

  // JavaScript value type checking
  bool (*is_number)(void *runtime, void *value);
  bool (*is_string)(void *runtime, void *value);
  bool (*is_boolean)(void *runtime, void *value);
  bool (*is_object)(void *runtime, void *value);
  bool (*is_null)(void *runtime, void *value);
  bool (*is_undefined)(void *runtime, void *value);
  bool (*is_array)(void *runtime, void *value);
  bool (*is_array_buffer)(void *runtime, void *value);
  bool (*is_function)(void *runtime, void *value);

  // JavaScript function handling
  void *(*call_function)(void *runtime, void *function, void *this_obj,
                         void **args, size_t arg_count);
  void *(*create_function)(void *runtime, void *native_function,
                           void *user_data);
  void *(*create_promise)(void *runtime, void *executor, void *resolve,
                          void *reject);

  // JavaScript exception handling
  bool (*has_exception)(void *runtime);
  void *(*get_exception)(void *runtime);
  void (*clear_exception)(void *runtime);
  char *(*get_exception_message)(void *runtime, void *exception,
                                 size_t *length);

  // Memory management
  void (*release_value)(void *runtime, void *value);
  void (*retain_value)(void *runtime, void *value);
  void (*trigger_gc)(void *runtime);

  // Runtime lifecycle
  bool (*initialize_runtime)(void *runtime);
  void (*cleanup_runtime)(void *runtime);
} js_runtime_adapter_t;

// JavaScript bridge structure
struct polycall_js_bridge {
  polycall_core_context_t *core_ctx;
  polycall_ffi_context_t *ffi_ctx;
  polycall_js_runtime_type_t runtime_type;
  void *runtime_handle;
  js_runtime_adapter_t adapter;
  js_function_registry_t function_registry;
  promise_registry_t promise_registry;
  bool enable_promise_integration;
  bool enable_callback_conversion;
  bool enable_object_proxying;
  bool enable_exception_translation;
  size_t max_string_length;
  void *user_data;
  language_bridge_t bridge_interface;
  pthread_mutex_t runtime_mutex;
};

// Forward declarations for language bridge functions
static polycall_core_error_t js_convert_to_native(polycall_core_context_t *ctx,
                                                  const ffi_value_t *src,
                                                  void *dest,
                                                  ffi_type_info_t *dest_type);

static polycall_core_error_t
js_convert_from_native(polycall_core_context_t *ctx, const void *src,
                       ffi_type_info_t *src_type, ffi_value_t *dest);

static polycall_core_error_t js_register_function(polycall_core_context_t *ctx,
                                                  const char *function_name,
                                                  void *function_ptr,
                                                  ffi_signature_t *signature,
                                                  uint32_t flags);

static polycall_core_error_t
js_call_function(polycall_core_context_t *ctx, const char *function_name,
                 ffi_value_t *args, size_t arg_count, ffi_value_t *result);

static polycall_core_error_t js_acquire_memory(polycall_core_context_t *ctx,
                                               void *ptr, size_t size);

static polycall_core_error_t js_release_memory(polycall_core_context_t *ctx,
                                               void *ptr);

static polycall_core_error_t js_handle_exception(polycall_core_context_t *ctx,
                                                 void *exception, char *message,
                                                 size_t message_size);

static polycall_core_error_t js_initialize(polycall_core_context_t *ctx);

static void js_cleanup(polycall_core_context_t *ctx);

// Forward declarations for internal helper functions
static js_function_t *find_js_function(js_function_registry_t *registry,
                                       const char *name);
static promise_callback_t *find_promise_callback(promise_registry_t *registry,
                                                 const char *name);
static polycall_core_error_t
init_js_function_registry(polycall_core_context_t *ctx,
                          js_function_registry_t *registry, size_t capacity);
static void cleanup_js_function_registry(polycall_core_context_t *ctx,
                                         js_function_registry_t *registry);
static polycall_core_error_t init_promise_registry(polycall_core_context_t *ctx,
                                                   promise_registry_t *registry,
                                                   size_t capacity);
static void cleanup_promise_registry(polycall_core_context_t *ctx,
                                     promise_registry_t *registry);
static polycall_core_error_t
setup_runtime_adapter(polycall_js_bridge_t *js_bridge);

// Runtime adapter implementations for supported JavaScript engines
static polycall_core_error_t
setup_node_adapter(polycall_js_bridge_t *js_bridge);
static polycall_core_error_t setup_v8_adapter(polycall_js_bridge_t *js_bridge);
static polycall_core_error_t
setup_webkit_adapter(polycall_js_bridge_t *js_bridge);
static polycall_core_error_t
setup_spidermonkey_adapter(polycall_js_bridge_t *js_bridge);
static polycall_core_error_t
setup_quickjs_adapter(polycall_js_bridge_t *js_bridge);
static polycall_core_error_t
setup_custom_adapter(polycall_js_bridge_t *js_bridge);

// Type conversion helpers
static polycall_core_error_t convert_js_to_ffi_value(
    polycall_core_context_t *ctx, polycall_js_bridge_t *js_bridge,
    void *js_value, polycall_ffi_type_t expected_type, ffi_value_t *ffi_value);

static polycall_core_error_t
convert_ffi_to_js_value(polycall_core_context_t *ctx,
                        polycall_js_bridge_t *js_bridge,
                        const ffi_value_t *ffi_value, void **js_value);
#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_FFI_JS_BRIDGE_H_H */