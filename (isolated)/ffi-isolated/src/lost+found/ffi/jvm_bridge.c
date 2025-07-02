/**
 * @file jvm_bridge.c
 * @brief JVM language bridge implementation for LibPolyCall FFI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the JVM language bridge for LibPolyCall FFI, providing
 * an interface for Java and other JVM-based languages to interact with other
 * languages through the FFI system. It handles JNI integration, method registration,
 * data type conversion, and exception handling.
 */

 #include "polycall/core/ffi/jvm_bridge.h"

 #include <stdlib.h>
 #include <string.h>
 // Define error sources if not already defined
 #ifndef POLYCALL_ERROR_SOURCE_FFI
 #define POLYCALL_ERROR_SOURCE_FFI 2
 #endif
 
 // Helper functions
 
 /**
  * @brief Get JNI environment pointer for the current thread
  */
 static JNIEnv* get_jni_env(JavaVM* jvm) {
     JNIEnv* env;
     // Get the JNI environment pointer for this thread
     jint result = (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
     
     if (result == JNI_EDETACHED) {
         // Thread not attached, attach it
         result = (*jvm)->AttachCurrentThread(jvm, (void**)&env, NULL);
         if (result != JNI_OK) {
             return NULL;
         }
     } else if (result != JNI_OK) {
         return NULL;
     }
     
     return env;
 }
 
 /**
  * @brief Check for and handle Java exceptions
  */
 static polycall_core_error_t check_java_exception(
     polycall_core_context_t* ctx,
     JNIEnv* env,
     char* error_message,
     size_t message_size
 ) {
     if ((*env)->ExceptionCheck(env)) {
         jthrowable exception = (*env)->ExceptionOccurred(env);
         (*env)->ExceptionClear(env);
         
         // Get exception information
         jclass exception_class = (*env)->GetObjectClass(env, exception);
         jmethodID toString_method = (*env)->GetMethodID(env, exception_class, "toString", "()Ljava/lang/String;");
         jstring jmessage = (jstring)(*env)->CallObjectMethod(env, exception, toString_method);
         
         const char* message_chars = (*env)->GetStringUTFChars(env, jmessage, NULL);
         
         // Copy message if needed
         if (error_message && message_size > 0) {
             strncpy(error_message, message_chars, message_size - 1);
             error_message[message_size - 1] = '\0';
         }
         
         // Log error
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_EXECUTION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Java exception: %s", message_chars);
         
         // Release resources
         (*env)->ReleaseStringUTFChars(env, jmessage, message_chars);
         (*env)->DeleteLocalRef(env, jmessage);
         (*env)->DeleteLocalRef(env, exception_class);
         (*env)->DeleteLocalRef(env, exception);
         
         return POLYCALL_CORE_ERROR_EXECUTION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a JVM instance
  */
 static polycall_core_error_t create_jvm(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge
 ) {
     // Parse JVM options from configuration
     JavaVMOption* options = NULL;
     int option_count = 0;
     char* jvm_options_copy = NULL;
     
     if (jvm_bridge->config.jvm_options) {
         // Count options by counting spaces and adding 1
         jvm_options_copy = strdup(jvm_bridge->config.jvm_options);
         if (!jvm_options_copy) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Count number of options (separated by spaces)
         char* token = strtok(jvm_options_copy, " ");
         while (token) {
             option_count++;
             token = strtok(NULL, " ");
         }
         free(jvm_options_copy);
         
         // Allocate options array
         options = (JavaVMOption*)polycall_core_malloc(ctx, sizeof(JavaVMOption) * option_count);
         if (!options) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Parse options
         jvm_options_copy = strdup(jvm_bridge->config.jvm_options);
         if (!jvm_options_copy) {
             polycall_core_free(ctx, options);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         int i = 0;
         token = strtok(jvm_options_copy, " ");
         while (token) {
             options[i].optionString = token;
             i++;
             token = strtok(NULL, " ");
         }
     }
     
     // Add classpath option if provided
     JavaVMOption* classpath_option = NULL;
     if (jvm_bridge->config.classpath) {
         classpath_option = (JavaVMOption*)polycall_core_malloc(ctx, sizeof(JavaVMOption));
         if (!classpath_option) {
             if (options) polycall_core_free(ctx, options);
             if (jvm_options_copy) free(jvm_options_copy);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Allocate and format classpath string
         size_t cp_size = strlen("-Djava.class.path=") + strlen(jvm_bridge->config.classpath) + 1;
         char* cp_string = (char*)polycall_core_malloc(ctx, cp_size);
         if (!cp_string) {
             polycall_core_free(ctx, classpath_option);
             if (options) polycall_core_free(ctx, options);
             if (jvm_options_copy) free(jvm_options_copy);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         snprintf(cp_string, cp_size, "-Djava.class.path=%s", jvm_bridge->config.classpath);
         classpath_option->optionString = cp_string;
     }
     
     // Prepare JVM initialization arguments
     JavaVMInitArgs vm_args;
     vm_args.version = JNI_VERSION_1_6;
     vm_args.nOptions = option_count + (classpath_option ? 1 : 0);
     
     // Combine options and classpath
     JavaVMOption* all_options = NULL;
     if (vm_args.nOptions > 0) {
         all_options = (JavaVMOption*)polycall_core_malloc(ctx, sizeof(JavaVMOption) * vm_args.nOptions);
         if (!all_options) {
             if (classpath_option) {
                 polycall_core_free(ctx, (void*)classpath_option->optionString);
                 polycall_core_free(ctx, classpath_option);
             }
             if (options) polycall_core_free(ctx, options);
             if (jvm_options_copy) free(jvm_options_copy);
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy options
         if (options) {
             memcpy(all_options, options, sizeof(JavaVMOption) * option_count);
         }
         
         // Add classpath option
         if (classpath_option) {
             all_options[option_count] = *classpath_option;
         }
     }
     
     vm_args.options = all_options;
     vm_args.ignoreUnrecognized = JNI_TRUE;
     
     // Create the JVM
     JavaVM* jvm;
     JNIEnv* env;
     jint res = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
     
     // Clean up allocated resources
     if (all_options) polycall_core_free(ctx, all_options);
     if (classpath_option) {
         polycall_core_free(ctx, (void*)classpath_option->optionString);
         polycall_core_free(ctx, classpath_option);
     }
     if (options) polycall_core_free(ctx, options);
     if (jvm_options_copy) free(jvm_options_copy);
     
     // Handle JVM creation result
     if (res != JNI_OK) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create JVM (error %d)", res);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     jvm_bridge->jvm = jvm;
     jvm_bridge->owns_jvm = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize the JVM bridge class
  */
 static polycall_core_error_t init_bridge_class(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge
 ) {
     if (!jvm_bridge->config.bridge_class) {
         // No bridge class specified, nothing to do
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Find the bridge class
     jclass local_class = (*env)->FindClass(env, jvm_bridge->config.bridge_class);
     if (!local_class) {
         check_java_exception(ctx, env, NULL, 0);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find JVM bridge class: %s", 
                           jvm_bridge->config.bridge_class);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Create global reference to the class
     jvm_bridge->bridge_class = (*env)->NewGlobalRef(env, local_class);
     (*env)->DeleteLocalRef(env, local_class);
     
     if (!jvm_bridge->bridge_class) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create global reference to bridge class");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize method registry
  */
 static polycall_core_error_t init_method_registry(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     size_t capacity
 ) {
     // Allocate method registry
     jvm_bridge->methods = polycall_core_malloc(ctx, capacity * sizeof(java_method_entry_t));
     if (!jvm_bridge->methods) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     jvm_bridge->method_capacity = capacity;
     jvm_bridge->method_count = 0;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup method registry
  */
 static void cleanup_method_registry(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge
 ) {
     if (!jvm_bridge->methods) {
         return;
     }
     
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     
     // Free method registry entries
     for (size_t i = 0; i < jvm_bridge->method_count; i++) {
         java_method_entry_t* entry = &jvm_bridge->methods[i];
         
         // Free function name
         if (entry->function_name) {
             polycall_core_free(ctx, entry->function_name);
         }
         
         // Free method info strings
         if (entry->method_info.name) {
             polycall_core_free(ctx, (void*)entry->method_info.name);
         }
         if (entry->method_info.signature) {
             polycall_core_free(ctx, (void*)entry->method_info.signature);
         }
         if (entry->method_info.class_name) {
             polycall_core_free(ctx, (void*)entry->method_info.class_name);
         }
         
         // Delete class reference
         if (env && entry->class_ref) {
             (*env)->DeleteGlobalRef(env, entry->class_ref);
         }
         
         // Free signature (handled by FFI core)
     }
     
     // Free method registry array
     polycall_core_free(ctx, jvm_bridge->methods);
     jvm_bridge->methods = NULL;
     jvm_bridge->method_count = 0;
     jvm_bridge->method_capacity = 0;
 }
 
 /**
  * @brief Initialize callback registry
  */
 static polycall_core_error_t init_callback_registry(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     size_t capacity
 ) {
     // Allocate callback registry
     jvm_bridge->callbacks = polycall_core_malloc(ctx, capacity * sizeof(java_callback_t));
     if (!jvm_bridge->callbacks) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     jvm_bridge->callback_capacity = capacity;
     jvm_bridge->callback_count = 0;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Cleanup callback registry
  */
 static void cleanup_callback_registry(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge
 ) {
     if (!jvm_bridge->callbacks) {
         return;
     }
     
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     
     // Free callback registry entries
     for (size_t i = 0; i < jvm_bridge->callback_count; i++) {
         java_callback_t* callback = &jvm_bridge->callbacks[i];
         
         // Free callback class and method names
         if (callback->callback_class) {
             polycall_core_free(ctx, callback->callback_class);
         }
         if (callback->callback_method) {
             polycall_core_free(ctx, callback->callback_method);
         }
         
         // Delete class reference
         if (env && callback->class_ref) {
             (*env)->DeleteGlobalRef(env, callback->class_ref);
         }
         
         // Delete instance reference
         if (env && callback->instance) {
             (*env)->DeleteGlobalRef(env, callback->instance);
         }
         
         // Free signature (handled by FFI core)
     }
     
     // Free callback registry array
     polycall_core_free(ctx, jvm_bridge->callbacks);
     jvm_bridge->callbacks = NULL;
     jvm_bridge->callback_count = 0;
     jvm_bridge->callback_capacity = 0;
 }
 
 /**
  * @brief Find a registered method by name
  */
 static java_method_entry_t* find_method(
     polycall_jvm_bridge_t* jvm_bridge,
     const char* function_name
 ) {
     for (size_t i = 0; i < jvm_bridge->method_count; i++) {
         if (strcmp(jvm_bridge->methods[i].function_name, function_name) == 0) {
             return &jvm_bridge->methods[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Find a registered callback by class and method
  */
 static java_callback_t* find_callback(
     polycall_jvm_bridge_t* jvm_bridge,
     const char* callback_class,
     const char* callback_method
 ) {
     for (size_t i = 0; i < jvm_bridge->callback_count; i++) {
         if (strcmp(jvm_bridge->callbacks[i].callback_class, callback_class) == 0 &&
             strcmp(jvm_bridge->callbacks[i].callback_method, callback_method) == 0) {
             return &jvm_bridge->callbacks[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Convert FFI value to Java value
  */
 static jobject ffi_to_java_value(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     JNIEnv* env,
     const ffi_value_t* ffi_value
 ) {
     if (!ffi_value) {
         return NULL;
     }
     
     // Convert based on FFI type
     switch (ffi_value->type) {
         case POLYCALL_FFI_TYPE_VOID:
             return NULL;
             
         case POLYCALL_FFI_TYPE_BOOL: {
             // Create a java.lang.Boolean object
             jclass boolean_class = (*env)->FindClass(env, "java/lang/Boolean");
             jmethodID constructor = (*env)->GetMethodID(env, boolean_class, "<init>", "(Z)V");
             jobject result = (*env)->NewObject(env, boolean_class, constructor, (jboolean)ffi_value->value.bool_value);
             (*env)->DeleteLocalRef(env, boolean_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_CHAR: {
             // Create a java.lang.Character object
             jclass char_class = (*env)->FindClass(env, "java/lang/Character");
             jmethodID constructor = (*env)->GetMethodID(env, char_class, "<init>", "(C)V");
             jobject result = (*env)->NewObject(env, char_class, constructor, (jchar)ffi_value->value.char_value);
             (*env)->DeleteLocalRef(env, char_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_INT8:
         case POLYCALL_FFI_TYPE_INT16:
         case POLYCALL_FFI_TYPE_INT32: {
             // Create a java.lang.Integer object
             jclass integer_class = (*env)->FindClass(env, "java/lang/Integer");
             jmethodID constructor = (*env)->GetMethodID(env, integer_class, "<init>", "(I)V");
             jint value;
             
             if (ffi_value->type == POLYCALL_FFI_TYPE_INT8) {
                 value = (jint)ffi_value->value.int8_value;
             } else if (ffi_value->type == POLYCALL_FFI_TYPE_INT16) {
                 value = (jint)ffi_value->value.int16_value;
             } else {
                 value = (jint)ffi_value->value.int32_value;
             }
             
             jobject result = (*env)->NewObject(env, integer_class, constructor, value);
             (*env)->DeleteLocalRef(env, integer_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_UINT8:
         case POLYCALL_FFI_TYPE_UINT16:
         case POLYCALL_FFI_TYPE_UINT32: {
             // Create a java.lang.Integer object (Java doesn't have unsigned types)
             jclass integer_class = (*env)->FindClass(env, "java/lang/Integer");
             jmethodID constructor = (*env)->GetMethodID(env, integer_class, "<init>", "(I)V");
             jint value;
             
             if (ffi_value->type == POLYCALL_FFI_TYPE_UINT8) {
                 value = (jint)ffi_value->value.uint8_value;
             } else if (ffi_value->type == POLYCALL_FFI_TYPE_UINT16) {
                 value = (jint)ffi_value->value.uint16_value;
             } else {
                 value = (jint)ffi_value->value.uint32_value;
             }
             
             jobject result = (*env)->NewObject(env, integer_class, constructor, value);
             (*env)->DeleteLocalRef(env, integer_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_INT64: {
             // Create a java.lang.Long object
             jclass long_class = (*env)->FindClass(env, "java/lang/Long");
             jmethodID constructor = (*env)->GetMethodID(env, long_class, "<init>", "(J)V");
             jobject result = (*env)->NewObject(env, long_class, constructor, (jlong)ffi_value->value.int64_value);
             (*env)->DeleteLocalRef(env, long_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_UINT64: {
             // Create a java.lang.Long object (Java doesn't have unsigned types)
             jclass long_class = (*env)->FindClass(env, "java/lang/Long");
             jmethodID constructor = (*env)->GetMethodID(env, long_class, "<init>", "(J)V");
             jobject result = (*env)->NewObject(env, long_class, constructor, (jlong)ffi_value->value.uint64_value);
             (*env)->DeleteLocalRef(env, long_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_FLOAT: {
             // Create a java.lang.Float object
             jclass float_class = (*env)->FindClass(env, "java/lang/Float");
             jmethodID constructor = (*env)->GetMethodID(env, float_class, "<init>", "(F)V");
             jobject result = (*env)->NewObject(env, float_class, constructor, (jfloat)ffi_value->value.float_value);
             (*env)->DeleteLocalRef(env, float_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_DOUBLE: {
             // Create a java.lang.Double object
             jclass double_class = (*env)->FindClass(env, "java/lang/Double");
             jmethodID constructor = (*env)->GetMethodID(env, double_class, "<init>", "(D)V");
             jobject result = (*env)->NewObject(env, double_class, constructor, (jdouble)ffi_value->value.double_value);
             (*env)->DeleteLocalRef(env, double_class);
             return result;
         }
             
         case POLYCALL_FFI_TYPE_STRING: {
             // Create a java.lang.String object
             if (!ffi_value->value.string_value) {
                 return NULL;
             }
             return (*env)->NewStringUTF(env, ffi_value->value.string_value);
         }
             
         case POLYCALL_FFI_TYPE_POINTER: {
             // Create a java.nio.ByteBuffer for direct memory access
             if (!ffi_value->value.pointer_value) {
                 return NULL;
             }
             
             // If we have size information, use it
             size_t size = 0;
             if (ffi_value->type_info && 
                 ffi_value->type_info->details.struct_info.size > 0) {
                 size = ffi_value->type_info->details.struct_info.size;
             } else {
                 // Default to a reasonable size for direct buffers
                 size = 1024;
             }
             
             // Create a direct ByteBuffer
             return (*env)->NewDirectByteBuffer(env, ffi_value->value.pointer_value, size);
         }
             
         // Implement these based on your specific needs:
         case POLYCALL_FFI_TYPE_STRUCT:
         case POLYCALL_FFI_TYPE_ARRAY:
         case POLYCALL_FFI_TYPE_CALLBACK:
         case POLYCALL_FFI_TYPE_OBJECT:
         default: {
             // For complex types, would need custom handling
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Unsupported FFI to Java conversion for type %d", 
                               ffi_value->type);
             return NULL;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize the JVM bridge
  */
 polycall_core_error_t polycall_jvm_bridge_init(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t** jvm_bridge,
     const polycall_jvm_bridge_config_t* config
 ) {
     // Validate parameters
     if (!ctx || !ffi_ctx || !jvm_bridge || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate JVM bridge context
     polycall_jvm_bridge_t* new_bridge = polycall_core_malloc(ctx, sizeof(polycall_jvm_bridge_t));
     if (!new_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate JVM bridge context");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize fields
     memset(new_bridge, 0, sizeof(polycall_jvm_bridge_t));
     new_bridge->core_ctx = ctx;
     new_bridge->ffi_ctx = ffi_ctx;
     new_bridge->config = *config;
     
     // Initialize method registry
     polycall_core_error_t result = init_method_registry(ctx, new_bridge, 64); // Default capacity
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     // Initialize callback registry
     result = init_callback_registry(ctx, new_bridge, 32); // Default capacity
     if (result != POLYCALL_CORE_SUCCESS) {
         cleanup_method_registry(ctx, new_bridge);
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     // Use existing JVM or create a new one
     if (config->jvm_instance) {
         new_bridge->jvm = config->jvm_instance;
         new_bridge->owns_jvm = false;
     } else if (config->create_vm_if_needed) {
         result = create_jvm(ctx, new_bridge);
         if (result != POLYCALL_CORE_SUCCESS) {
             cleanup_callback_registry(ctx, new_bridge);
             cleanup_method_registry(ctx, new_bridge);
             polycall_core_free(ctx, new_bridge);
             return result;
         }
     } else {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No JVM instance provided and create_vm_if_needed is false");
         cleanup_callback_registry(ctx, new_bridge);
         cleanup_method_registry(ctx, new_bridge);
         polycall_core_free(ctx, new_bridge);
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Initialize bridge class if specified
     if (config->bridge_class) {
         result = init_bridge_class(ctx, new_bridge);
         if (result != POLYCALL_CORE_SUCCESS) {
             if (new_bridge->owns_jvm) {
                 // Destroy JVM - not implemented here as it's complex and often unnecessary
                 // JVM usually handles its own shutdown via atexit() handlers
             }
             cleanup_callback_registry(ctx, new_bridge);
             cleanup_method_registry(ctx, new_bridge);
             polycall_core_free(ctx, new_bridge);
             return result;
         }
     }
     
     // Initialize language bridge interface
     new_bridge->bridge_interface.language_name = "java";
     new_bridge->bridge_interface.version = "1.0.0";
     new_bridge->bridge_interface.convert_to_native = jvm_convert_to_native;
     new_bridge->bridge_interface.convert_from_native = jvm_convert_from_native;
     new_bridge->bridge_interface.register_function = jvm_register_function;
     new_bridge->bridge_interface.call_function = jvm_call_function;
     new_bridge->bridge_interface.acquire_memory = jvm_acquire_memory;
     new_bridge->bridge_interface.release_memory = jvm_release_memory;
     new_bridge->bridge_interface.handle_exception = jvm_handle_exception;
     new_bridge->bridge_interface.initialize = jvm_initialize;
     new_bridge->bridge_interface.cleanup = jvm_cleanup;
     new_bridge->bridge_interface.user_data = new_bridge;
     
     // Register with FFI system
     result = polycall_ffi_register_language(ctx, ffi_ctx, "java", &new_bridge->bridge_interface);
     if (result != POLYCALL_CORE_SUCCESS && result != POLYCALL_CORE_ERROR_ALREADY_INITIALIZED) {
         if (new_bridge->bridge_class) {
             JNIEnv* env = get_jni_env(new_bridge->jvm);
             if (env) {
                 (*env)->DeleteGlobalRef(env, new_bridge->bridge_class);
             }
         }
         
         if (new_bridge->owns_jvm) {
             // Destroy JVM - not implemented here
         }
         
         cleanup_callback_registry(ctx, new_bridge);
         cleanup_method_registry(ctx, new_bridge);
         polycall_core_free(ctx, new_bridge);
         return result;
     }
     
     *jvm_bridge = new_bridge;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Clean up JVM bridge
  */
 void polycall_jvm_bridge_cleanup(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge) {
         return;
     }
     
     // Clean up bridge class
     if (jvm_bridge->bridge_class) {
         JNIEnv* env = get_jni_env(jvm_bridge->jvm);
         if (env) {
             (*env)->DeleteGlobalRef(env, jvm_bridge->bridge_class);
         }
     }
     
     // Clean up registries
     cleanup_callback_registry(ctx, jvm_bridge);
     cleanup_method_registry(ctx, jvm_bridge);
     
     // Destroy JVM if we own it
     if (jvm_bridge->owns_jvm && jvm_bridge->jvm) {
         // Note: Destroying JVM is complex and often not necessary
         // JVM handles its own cleanup via atexit() handlers
         // For completeness, would use (*jvm_bridge->jvm)->DestroyJavaVM(jvm_bridge->jvm);
     }
     
     // Free bridge context
     polycall_core_free(ctx, jvm_bridge);
 }
 
 /**
  * @brief Register a Java method with the FFI system
  */
 polycall_core_error_t polycall_jvm_bridge_register_method(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* function_name,
     const java_method_signature_t* java_method,
     uint32_t flags
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !function_name || !java_method) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Check if method already registered
     if (find_method(jvm_bridge, function_name)) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Method %s already registered", function_name);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (jvm_bridge->method_count >= jvm_bridge->method_capacity) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Method registry capacity exceeded");
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Find the Java class
     jclass local_class = (*env)->FindClass(env, java_method->class_name);
     if (!local_class) {
         check_java_exception(ctx, env, NULL, 0);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find Java class: %s", java_method->class_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create global reference to the class
     jclass global_class = (*env)->NewGlobalRef(env, local_class);
     (*env)->DeleteLocalRef(env, local_class);
     
     if (!global_class) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create global reference to class");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Find the method ID
     jmethodID method_id;
     if (java_method->is_static) {
         method_id = (*env)->GetStaticMethodID(env, global_class, java_method->name, java_method->signature);
     } else {
         method_id = (*env)->GetMethodID(env, global_class, java_method->name, java_method->signature);
     }
     
     if (!method_id) {
         (*env)->DeleteGlobalRef(env, global_class);
         check_java_exception(ctx, env, NULL, 0);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find Java method: %s%s %s", 
                           java_method->is_static ? "static " : "",
                           java_method->name, java_method->signature);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create a signature for the FFI system
     // A real implementation would analyze the Java method signature to determine parameter types
     // This is a placeholder implementation
     ffi_signature_t* signature = NULL;
     polycall_core_error_t result = polycall_ffi_create_signature(
         ctx, ffi_ctx, POLYCALL_FFI_TYPE_OBJECT, NULL, 0, &signature);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         (*env)->DeleteGlobalRef(env, global_class);
         return result;
     }
     
     // Register the method
     java_method_entry_t* method = &jvm_bridge->methods[jvm_bridge->method_count];
     
     // Copy function name
     method->function_name = polycall_core_malloc(ctx, strlen(function_name) + 1);
     if (!method->function_name) {
         (*env)->DeleteGlobalRef(env, global_class);
         polycall_ffi_destroy_signature(ctx, ffi_ctx, signature);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     strcpy(method->function_name, function_name);
     
     // Copy method info
     method->method_info.name = polycall_core_malloc(ctx, strlen(java_method->name) + 1);
     method->method_info.signature = polycall_core_malloc(ctx, strlen(java_method->signature) + 1);
     method->method_info.class_name = polycall_core_malloc(ctx, strlen(java_method->class_name) + 1);
     
     if (!method->method_info.name || !method->method_info.signature || !method->method_info.class_name) {
         if (method->method_info.name) polycall_core_free(ctx, (void*)method->method_info.name);
         if (method->method_info.signature) polycall_core_free(ctx, (void*)method->method_info.signature);
         if (method->method_info.class_name) polycall_core_free(ctx, (void*)method->method_info.class_name);
         polycall_core_free(ctx, method->function_name);
         (*env)->DeleteGlobalRef(env, global_class);
         polycall_ffi_destroy_signature(ctx, ffi_ctx, signature);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy((char*)method->method_info.name, java_method->name);
     strcpy((char*)method->method_info.signature, java_method->signature);
     strcpy((char*)method->method_info.class_name, java_method->class_name);
     method->method_info.is_static = java_method->is_static;
     
     // Set method entry fields
     method->method_id = method_id;
     method->class_ref = global_class;
     method->signature = signature;
     method->flags = flags;
     
     // Increment method count
     jvm_bridge->method_count++;
     
     // Expose function to FFI system
     result = polycall_ffi_expose_function(
         ctx, ffi_ctx, function_name, method, signature, "java", flags);
     
     if (result != POLYCALL_CORE_SUCCESS) {
         // Revert method registration
         jvm_bridge->method_count--;
         polycall_core_free(ctx, (void*)method->method_info.name);
         polycall_core_free(ctx, (void*)method->method_info.signature);
         polycall_core_free(ctx, (void*)method->method_info.class_name);
         polycall_core_free(ctx, method->function_name);
         (*env)->DeleteGlobalRef(env, global_class);
         polycall_ffi_destroy_signature(ctx, ffi_ctx, signature);
         return result;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Call a Java method through the FFI system
  */
 polycall_core_error_t polycall_jvm_bridge_call_method(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !function_name || (!args && arg_count > 0) || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Find the method
     java_method_entry_t* method = find_method(jvm_bridge, function_name);
     if (!method) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Method %s not found", function_name);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert arguments to Java values
     jvalue* jargs = NULL;
     if (arg_count > 0) {
         jargs = polycall_core_malloc(ctx, arg_count * sizeof(jvalue));
         if (!jargs) {
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Failed to allocate memory for Java arguments");
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Convert each argument
         for (size_t i = 0; i < arg_count; i++) {
             // This is a simplified implementation - a real implementation would convert
             // based on Java method signature and handle various types
             jobject java_arg = ffi_to_java_value(ctx, jvm_bridge, env, &args[i]);
             if (!java_arg && args[i].type != POLYCALL_FFI_TYPE_VOID) {
                 // Clean up previously allocated arguments
                 for (size_t j = 0; j < i; j++) {
                     (*env)->DeleteLocalRef(env, jargs[j].l);
                 }
                 polycall_core_free(ctx, jargs);
                 
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_TYPE_CONVERSION_FAILED,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to convert argument %zu to Java value", i);
                 return POLYCALL_CORE_ERROR_TYPE_CONVERSION_FAILED;
             }
             
             // Store as jobject (l field of jvalue union)
             jargs[i].l = java_arg;
         }
     }
     
     // Call the Java method
     jobject java_result = NULL;
     
     if (method->method_info.is_static) {
         // Call static method
         // This is a simplified implementation - a real implementation would handle various return types
         // based on Java method signature
         java_result = (*env)->CallStaticObjectMethodA(env, method->class_ref, method->method_id, jargs);
     } else {
         // For non-static methods, we need an object instance
         // This is a placeholder - real implementation would need to handle object instances
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Non-static method calls not fully implemented");
         
         if (jargs) {
             // Clean up arguments
             for (size_t i = 0; i < arg_count; i++) {
                 (*env)->DeleteLocalRef(env, jargs[i].l);
             }
             polycall_core_free(ctx, jargs);
         }
         
         return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
     }
     
     // Check for Java exceptions
     polycall_core_error_t error = check_java_exception(ctx, env, NULL, 0);
     
     // Clean up arguments
     if (jargs) {
         for (size_t i = 0; i < arg_count; i++) {
             (*env)->DeleteLocalRef(env, jargs[i].l);
         }
         polycall_core_free(ctx, jargs);
     }
     
     if (error != POLYCALL_CORE_SUCCESS) {
         return error;
     }
     
     // Convert Java result to FFI value
     // This is a simplified implementation - a real implementation would determine
     // expected type from method signature
     error = java_to_ffi_value(ctx, jvm_bridge, env, java_result, POLYCALL_FFI_TYPE_OBJECT, result);
     
     // Clean up Java result
     if (java_result) {
         (*env)->DeleteLocalRef(env, java_result);
     }
     
     return error;
 }
 
 /**
  * @brief Convert FFI value to JVM value
  */
 polycall_core_error_t polycall_jvm_bridge_to_java_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const ffi_value_t* ffi_value,
     void* jni_env,
     void** java_value
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !ffi_value || !jni_env || !java_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert FFI value to Java value
     jobject result = ffi_to_java_value(ctx, jvm_bridge, (JNIEnv*)jni_env, ffi_value);
     
     // Check for conversion failure
     if (!result && ffi_value->type != POLYCALL_FFI_TYPE_VOID) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_TYPE_CONVERSION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to convert FFI value to Java value");
         return POLYCALL_CORE_ERROR_TYPE_CONVERSION_FAILED;
     }
     
     *java_value = result;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Convert JVM value to FFI value
  */
 polycall_core_error_t polycall_jvm_bridge_from_java_value(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void* java_value,
     void* jni_env,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !jni_env || !ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Convert Java value to FFI value
     return java_to_ffi_value(ctx, jvm_bridge, (JNIEnv*)jni_env, (jobject)java_value, expected_type, ffi_value);
 }
 
 /**
  * @brief Register a Java callback function
  */
 polycall_core_error_t polycall_jvm_bridge_register_callback(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     const char* callback_class,
     const char* callback_method,
     ffi_signature_t* signature
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !callback_class || !callback_method || !signature) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     // Check if callback already registered
     if (find_callback(jvm_bridge, callback_class, callback_method)) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_ALREADY_INITIALIZED,
                           POLYCALL_ERROR_SEVERITY_WARNING, 
                           "Callback %s.%s already registered", callback_class, callback_method);
         return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
     }
     
     // Check if registry is full
     if (jvm_bridge->callback_count >= jvm_bridge->callback_capacity) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Callback registry capacity exceeded");
         return POLYCALL_CORE_ERROR_CAPACITY_EXCEEDED;
     }
     
     // Find the Java class
     jclass local_class = (*env)->FindClass(env, callback_class);
     if (!local_class) {
         check_java_exception(ctx, env, NULL, 0);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find Java class: %s", callback_class);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create global reference to the class
     jclass global_class = (*env)->NewGlobalRef(env, local_class);
     (*env)->DeleteLocalRef(env, local_class);
     
     if (!global_class) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_OUT_OF_MEMORY,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to create global reference to callback class");
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // This implementation assumes static method for simplicity
     // For instance methods, we would need to create and keep an instance of the class
     
     // Find the method ID (assuming static method)
     // This is a simplified implementation that assumes a specific signature
     // A real implementation would generate signature from ffi_signature_t
     const char* method_signature = "([Ljava/lang/Object;)Ljava/lang/Object;";
     jmethodID method_id = (*env)->GetStaticMethodID(env, global_class, callback_method, method_signature);
     
     if (!method_id) {
         (*env)->DeleteGlobalRef(env, global_class);
         check_java_exception(ctx, env, NULL, 0);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to find Java callback method: static %s %s", 
                           callback_method, method_signature);
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Register the callback
     java_callback_t* callback = &jvm_bridge->callbacks[jvm_bridge->callback_count];
     
     // Copy callback class and method names
     callback->callback_class = polycall_core_malloc(ctx, strlen(callback_class) + 1);
     callback->callback_method = polycall_core_malloc(ctx, strlen(callback_method) + 1);
     
     if (!callback->callback_class || !callback->callback_method) {
         if (callback->callback_class) polycall_core_free(ctx, callback->callback_class);
         if (callback->callback_method) polycall_core_free(ctx, callback->callback_method);
         (*env)->DeleteGlobalRef(env, global_class);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     strcpy(callback->callback_class, callback_class);
     strcpy(callback->callback_method, callback_method);
     
     // Set callback fields
     callback->class_ref = global_class;
     callback->method_id = method_id;
     callback->signature = signature; // The caller owns the signature
     callback->instance = NULL; // For static methods, instance is NULL
     
     // Increment callback count
     jvm_bridge->callback_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Handle Java exception
  */
 polycall_core_error_t polycall_jvm_bridge_handle_exception(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void* jni_env,
     char* error_message,
     size_t message_size
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !jni_env) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     return check_java_exception(ctx, (JNIEnv*)jni_env, error_message, message_size);
 }
 
 /**
  * @brief Get JNI environment for current thread
  */
 polycall_core_error_t polycall_jvm_bridge_get_env(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     void** jni_env
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !jni_env) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     *jni_env = env;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get language bridge interface for JVM
  */
 polycall_core_error_t polycall_jvm_bridge_get_interface(
     polycall_core_context_t* ctx,
     polycall_ffi_context_t* ffi_ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     language_bridge_t* bridge
 ) {
     if (!ctx || !ffi_ctx || !jvm_bridge || !bridge) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy bridge interface
     *bridge = jvm_bridge->bridge_interface;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create a default JVM bridge configuration
  */
 polycall_jvm_bridge_config_t polycall_jvm_bridge_create_default_config(void) {
     polycall_jvm_bridge_config_t config;
     
     config.jvm_instance = NULL;
     config.create_vm_if_needed = true;
     config.classpath = NULL;
     config.jvm_options = NULL;
     config.bridge_class = NULL;
     config.enable_exception_handler = true;
     config.gc_notification = true;
     config.direct_buffer_access = true;
     config.user_data = NULL;
     
     return config;
 }
 
 // Language bridge interface implementations
 
 static polycall_core_error_t jvm_convert_to_native(
     polycall_core_context_t* ctx,
     const ffi_value_t* src,
     void* dest,
     ffi_type_info_t* dest_type
 ) {
     // Implementation would depend on specific integration needs
     // This is a placeholder - real implementation would convert FFI values to JNI types
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "JVM convert_to_native not fully implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 static polycall_core_error_t jvm_convert_from_native(
     polycall_core_context_t* ctx,
     const void* src,
     ffi_type_info_t* src_type,
     ffi_value_t* dest
 ) {
     // Implementation would depend on specific integration needs
     // This is a placeholder - real implementation would convert JNI types to FFI values
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "JVM convert_from_native not fully implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 static polycall_core_error_t jvm_register_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     void* function_ptr,
     ffi_signature_t* signature,
     uint32_t flags
 ) {
     // This would be called by the FFI core when registering a function from another language
     // that needs to be callable from Java
     
     // Implementation would depend on specific integration needs
     // This is a placeholder
     POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                       POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                       POLYCALL_ERROR_SEVERITY_ERROR, 
                       "JVM register_function not fully implemented");
     return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
 }
 
 static polycall_core_error_t jvm_call_function(
     polycall_core_context_t* ctx,
     const char* function_name,
     ffi_value_t* args,
     size_t arg_count,
     ffi_value_t* result
 ) {
     // This would be called by the FFI core when calling a Java function
     // Extract JVM bridge from user_data
     polycall_jvm_bridge_t* jvm_bridge = (polycall_jvm_bridge_t*)ctx->current_user_data;
     if (!jvm_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "JVM bridge context not available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Call the Java method
     return polycall_jvm_bridge_call_method(ctx, jvm_bridge->ffi_ctx, jvm_bridge, 
                                          function_name, args, arg_count, result);
 }
 
 static polycall_core_error_t jvm_acquire_memory(
     polycall_core_context_t* ctx,
     void* ptr,
     size_t size
 ) {
     // Implementation would depend on specific memory management needs
     // This is a placeholder
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t jvm_release_memory(
     polycall_core_context_t* ctx,
     void* ptr
 ) {
     // Implementation would depend on specific memory management needs
     // This is a placeholder
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t jvm_handle_exception(
     polycall_core_context_t* ctx,
     void* exception,
     char* message,
     size_t message_size
 ) {
     // Extract JVM bridge from user_data
     polycall_jvm_bridge_t* jvm_bridge = (polycall_jvm_bridge_t*)ctx->current_user_data;
     if (!jvm_bridge) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "JVM bridge context not available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Get JNI environment
     JNIEnv* env = get_jni_env(jvm_bridge->jvm);
     if (!env) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                           POLYCALL_CORE_ERROR_INITIALIZATION_FAILED,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to get JNI environment");
         return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
     }
     
     return check_java_exception(ctx, env, message, message_size);
 }
 
 static polycall_core_error_t jvm_initialize(
     polycall_core_context_t* ctx
 ) {
     // This would be called when the language bridge is registered with the FFI system
     // No specific initialization needed beyond what's done in polycall_jvm_bridge_init
     return POLYCALL_CORE_SUCCESS;
 }
 
 static void jvm_cleanup(
     polycall_core_context_t* ctx
 ) {
     // This would be called when the language bridge is unregistered from the FFI system
     // No specific cleanup needed beyond what's done in polycall_jvm_bridge_cleanup
 }
 
 
 /**
  * @brief Convert Java value to FFI value
  */
 static polycall_core_error_t java_to_ffi_value(
     polycall_core_context_t* ctx,
     polycall_jvm_bridge_t* jvm_bridge,
     JNIEnv* env,
     jobject java_value,
     polycall_ffi_type_t expected_type,
     ffi_value_t* ffi_value
 ) {
     if (!ffi_value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize FFI value
     ffi_value->type = expected_type;
     ffi_value->type_info = NULL;
     
     // Handle null value
     if (!java_value) {
         // Set default value based on expected type
         memset(&ffi_value->value, 0, sizeof(ffi_value->value));
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Convert based on expected FFI type
     switch (expected_type) {
         case POLYCALL_FFI_TYPE_VOID:
             // Nothing to do
             return POLYCALL_CORE_SUCCESS;
             
         case POLYCALL_FFI_TYPE_BOOL: {
             // Get boolean value from java.lang.Boolean
             jclass boolean_class = (*env)->FindClass(env, "java/lang/Boolean");
             jmethodID booleanValue = (*env)->GetMethodID(env, boolean_class, "booleanValue", "()Z");
             ffi_value->value.bool_value = (*env)->CallBooleanMethod(env, java_value, booleanValue);
             (*env)->DeleteLocalRef(env, boolean_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_CHAR: {
             // Get char value from java.lang.Character
             jclass char_class = (*env)->FindClass(env, "java/lang/Character");
             jmethodID charValue = (*env)->GetMethodID(env, char_class, "charValue", "()C");
             ffi_value->value.char_value = (*env)->CallCharMethod(env, java_value, charValue);
             (*env)->DeleteLocalRef(env, char_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_INT8:
         case POLYCALL_FFI_TYPE_INT16:
         case POLYCALL_FFI_TYPE_INT32: {
             // Get int value from java.lang.Integer or similar
             jclass integer_class = (*env)->FindClass(env, "java/lang/Integer");
             jmethodID intValue = (*env)->GetMethodID(env, integer_class, "intValue", "()I");
             jint value = (*env)->CallIntMethod(env, java_value, intValue);
             (*env)->DeleteLocalRef(env, integer_class);
             
             // Handle Java exception
             polycall_core_error_t error = check_java_exception(ctx, env, NULL, 0);
             if (error != POLYCALL_CORE_SUCCESS) {
                 return error;
             }
             
             // Set value based on expected type
             if (expected_type == POLYCALL_FFI_TYPE_INT8) {
                 ffi_value->value.int8_value = (int8_t)value;
             } else if (expected_type == POLYCALL_FFI_TYPE_INT16) {
                 ffi_value->value.int16_value = (int16_t)value;
             } else {
                 ffi_value->value.int32_value = value;
             }
             
             return POLYCALL_CORE_SUCCESS;
         }
             
         case POLYCALL_FFI_TYPE_UINT8:
         case POLYCALL_FFI_TYPE_UINT16:
         case POLYCALL_FFI_TYPE_UINT32: {
             // Get int value from java.lang.Integer (Java doesn't have unsigned types)
             jclass integer_class = (*env)->FindClass(env, "java/lang/Integer");
             jmethodID intValue = (*env)->GetMethodID(env, integer_class, "intValue", "()I");
             jint value = (*env)->CallIntMethod(env, java_value, intValue);
             (*env)->DeleteLocalRef(env, integer_class);
             
             // Handle Java exception
             polycall_core_error_t error = check_java_exception(ctx, env, NULL, 0);
             if (error != POLYCALL_CORE_SUCCESS) {
                 return error;
             }
             
             // Set value based on expected type
             if (expected_type == POLYCALL_FFI_TYPE_UINT8) {
                 ffi_value->value.uint8_value = (uint8_t)value;
             } else if (expected_type == POLYCALL_FFI_TYPE_UINT16) {
                 ffi_value->value.uint16_value = (uint16_t)value;
             } else {
                 ffi_value->value.uint32_value = (uint32_t)value;
             }
             
             return POLYCALL_CORE_SUCCESS;
         }
             
         case POLYCALL_FFI_TYPE_INT64: {
             // Get long value from java.lang.Long
             jclass long_class = (*env)->FindClass(env, "java/lang/Long");
             jmethodID longValue = (*env)->GetMethodID(env, long_class, "longValue", "()J");
             ffi_value->value.int64_value = (*env)->CallLongMethod(env, java_value, longValue);
             (*env)->DeleteLocalRef(env, long_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_UINT64: {
             // Get long value from java.lang.Long (Java doesn't have unsigned types)
             jclass long_class = (*env)->FindClass(env, "java/lang/Long");
             jmethodID longValue = (*env)->GetMethodID(env, long_class, "longValue", "()J");
             ffi_value->value.uint64_value = (uint64_t)(*env)->CallLongMethod(env, java_value, longValue);
             (*env)->DeleteLocalRef(env, long_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_FLOAT: {
             // Get float value from java.lang.Float
             jclass float_class = (*env)->FindClass(env, "java/lang/Float");
             jmethodID floatValue = (*env)->GetMethodID(env, float_class, "floatValue", "()F");
             ffi_value->value.float_value = (*env)->CallFloatMethod(env, java_value, floatValue);
             (*env)->DeleteLocalRef(env, float_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_DOUBLE: {
             // Get double value from java.lang.Double
             jclass double_class = (*env)->FindClass(env, "java/lang/Double");
             jmethodID doubleValue = (*env)->GetMethodID(env, double_class, "doubleValue", "()D");
             ffi_value->value.double_value = (*env)->CallDoubleMethod(env, java_value, doubleValue);
             (*env)->DeleteLocalRef(env, double_class);
             return check_java_exception(ctx, env, NULL, 0);
         }
             
         case POLYCALL_FFI_TYPE_STRING: {
             // Check if value is a String
             jclass string_class = (*env)->FindClass(env, "java/lang/String");
             if (!(*env)->IsInstanceOf(env, java_value, string_class)) {
                 (*env)->DeleteLocalRef(env, string_class);
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Java object is not a String");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             
             // Get UTF chars from String
             const char* str = (*env)->GetStringUTFChars(env, (jstring)java_value, NULL);
             
             // Allocate memory for the string and copy it
             size_t len = strlen(str) + 1;
             char* string_copy = polycall_core_malloc(ctx, len);
             if (!string_copy) {
                 (*env)->ReleaseStringUTFChars(env, (jstring)java_value, str);
                 (*env)->DeleteLocalRef(env, string_class);
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
             
             memcpy(string_copy, str, len);
             
             // Release JNI resources
             (*env)->ReleaseStringUTFChars(env, (jstring)java_value, str);
             (*env)->DeleteLocalRef(env, string_class);
             
             // Set string value
             ffi_value->value.string_value = string_copy;
             
             return POLYCALL_CORE_SUCCESS;
         }
             
         case POLYCALL_FFI_TYPE_POINTER: {
             // Check if value is a ByteBuffer
             jclass buffer_class = (*env)->FindClass(env, "java/nio/ByteBuffer");
             if (!(*env)->IsInstanceOf(env, java_value, buffer_class)) {
                 (*env)->DeleteLocalRef(env, buffer_class);
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_TYPE_MISMATCH,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Java object is not a ByteBuffer");
                 return POLYCALL_CORE_ERROR_TYPE_MISMATCH;
             }
             
             // Get direct buffer address
             void* buffer_addr = (*env)->GetDirectBufferAddress(env, java_value);
             if (!buffer_addr) {
                 (*env)->DeleteLocalRef(env, buffer_class);
                 POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                                   POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                                   POLYCALL_ERROR_SEVERITY_ERROR, 
                                   "Failed to get ByteBuffer address");
                 return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
             }
             
             // Set pointer value
             ffi_value->value.pointer_value = buffer_addr;
             
             (*env)->DeleteLocalRef(env, buffer_class);
             return POLYCALL_CORE_SUCCESS;
         }
             
         // Implement these based on your specific needs:
         case POLYCALL_FFI_TYPE_STRUCT:
         case POLYCALL_FFI_TYPE_ARRAY:
         case POLYCALL_FFI_TYPE_CALLBACK:
         case POLYCALL_FFI_TYPE_OBJECT:
         default: {
             // For complex types, would need custom handling
             POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_FFI, 
                               POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION,
                               POLYCALL_ERROR_SEVERITY_ERROR, 
                               "Unsupported Java to FFI conversion for type %d", 
                               expected_type);
             return POLYCALL_CORE_ERROR_UNSUPPORTED_OPERATION;
         }
     }