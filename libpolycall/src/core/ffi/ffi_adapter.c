/**
 * @file dop_adapter.c
 * @brief Data-Oriented Programming adapter for FFI bindings
 * 
 * Implements a pattern for cross-language validation and runtime verification
 * inspired by the OBIX DOP adapter pattern.
 */

 #include "polycall/core/ffi/ffi_adapter.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_logger.h"
 
 #include <stdlib.h>
 #include <string.h>
 
 /**
  * @brief Validation constraint function signature
  */
 typedef bool (*polycall_validation_func)(const void* value, const void* context);
 
 /**
  * @brief Validation constraint structure
  */
 typedef struct {
     polycall_dop_data_type_t type;
     bool required;
     polycall_validation_func validate;
     const char* error_message;
 } polycall_validation_constraint_t;
 
 /**
  * @brief Component validator structure
  */
 struct polycall_component_validator {
     polycall_validation_constraint_t* constraints;
     size_t constraint_count;
     const char* component_name;
 };
 
 /**
  * @brief DOP data model implementation
  */
 struct polycall_dop_data_model {
     void* data;
     void* (*clone)(void* data);
     void* (*to_object)(void* data);
     void* (*merge)(void* data, void* other);
     bool (*equals)(void* data, void* other);
     void (*free)(void* data);
 };
 
 /**
  * @brief DOP behavior model implementation
  */
 struct polycall_dop_behavior_model {
     void* (*process)(void* data);
     const char* (*get_behavior_id)(void);
     const char* (*get_description)(void);
 };
 
 /**
  * @brief DOP adapter implementation
  */
 struct polycall_dop_adapter {
     polycall_dop_data_model_t* data_model;
     polycall_dop_behavior_model_t* behavior_model;
     polycall_component_validator_t* validator;
     const char* adapter_name;
 };
 
 /**
  * Creates a new component validator
  *
  * @param component_name Name of the component being validated
  * @return A new component validator or NULL on error
  */
 polycall_component_validator_t* polycall_component_validator_create(const char* component_name) {
     if (!component_name) {
         return NULL;
     }
     
     polycall_component_validator_t* validator = polycall_memory_alloc(
         sizeof(polycall_component_validator_t));
     if (!validator) {
         return NULL;
     }
     
     validator->constraints = NULL;
     validator->constraint_count = 0;
     validator->component_name = polycall_memory_strdup(component_name);
     
     if (!validator->component_name) {
         polycall_memory_free(validator);
         return NULL;
     }
     
     return validator;
 }
 
 /**
  * Adds a validation constraint to the component validator
  *
  * @param validator The component validator
  * @param prop_name Property name
  * @param type Data type
  * @param required Whether the property is required
  * @param validate Validation function
  * @param error_message Error message on validation failure
  * @return POLYCALL_SUCCESS or error code
  */
 polycall_result_t polycall_component_validator_add_constraint(
     polycall_component_validator_t* validator,
     const char* prop_name,
     polycall_dop_data_type_t type,
     bool required,
     polycall_validation_func validate,
     const char* error_message
 ) {
     if (!validator || !prop_name || !validate || !error_message) {
         return POLYCALL_ERROR_INVALID_PARAMETER;
     }
     
     // Resize constraints array
     size_t new_count = validator->constraint_count + 1;
     polycall_validation_constraint_t* new_constraints = polycall_memory_realloc(
         validator->constraints,
         sizeof(polycall_validation_constraint_t) * new_count
     );
     
     if (!new_constraints) {
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     validator->constraints = new_constraints;
     
     // Initialize new constraint
     polycall_validation_constraint_t* constraint = 
         &validator->constraints[validator->constraint_count];
     
     constraint->type = type;
     constraint->required = required;
     constraint->validate = validate;
     constraint->error_message = polycall_memory_strdup(error_message);
     
     if (!constraint->error_message) {
         return POLYCALL_ERROR_OUT_OF_MEMORY;
     }
     
     validator->constraint_count = new_count;
     return POLYCALL_SUCCESS;
 }
 
 /**
  * Validates component properties against constraints
  *
  * @param validator The component validator
  * @param props Properties to validate
  * @param error_out Optional output parameter for error details
  * @return POLYCALL_SUCCESS or error code
  */
 polycall_result_t polycall_component_validator_validate(
     polycall_component_validator_t* validator,
     const polycall_dop_object_t* props,
     polycall_validation_error_t* error_out
 ) {
     if (!validator || !props) {
         return POLYCALL_ERROR_INVALID_PARAMETER;
     }
     
     for (size_t i = 0; i < validator->constraint_count; i++) {
         const polycall_validation_constraint_t* constraint = &validator->constraints[i];
         const char* prop_name = validator->constraints[i].error_message; // Placeholder, real impl would use a prop name
         const void* prop_value = NULL; // Would get from props by name
         
         // Check if required
         if (constraint->required && !prop_value) {
             if (error_out) {
                 error_out->code = "MISSING_REQUIRED_PROP";
                 snprintf(error_out->message, sizeof(error_out->message),
                          "Required prop '%s' is missing", prop_name);
                 error_out->source = validator->component_name;
             }
             return POLYCALL_ERROR_VALIDATION_FAILED;
         }
         
         // Skip validation if not required and null
         if (!prop_value && !constraint->required) {
             continue;
         }
         
         // Perform validation
         if (!constraint->validate(prop_value, props)) {
             if (error_out) {
                 error_out->code = "VALIDATION_FAILED";
                 snprintf(error_out->message, sizeof(error_out->message),
                          "Validation failed for prop '%s': %s", 
                          prop_name, constraint->error_message);
                 error_out->source = validator->component_name;
             }
             return POLYCALL_ERROR_VALIDATION_FAILED;
         }
     }
     
     return POLYCALL_SUCCESS;
 }
 
 /**
  * Destroys a component validator
  *
  * @param validator The validator to destroy
  */
 void polycall_component_validator_destroy(polycall_component_validator_t* validator) {
     if (!validator) {
         return;
     }
     
     // Free constraint error messages
     for (size_t i = 0; i < validator->constraint_count; i++) {
         polycall_memory_free((void*)validator->constraints[i].error_message);
     }
     
     // Free constraints array
     polycall_memory_free(validator->constraints);
     
     // Free component name
     polycall_memory_free((void*)validator->component_name);
     
     // Free validator
     polycall_memory_free(validator);
 }
 
 /**
  * Creates a new DOP data model
  *
  * @param data Initial data
  * @param clone Clone function
  * @param to_object Serialization function
  * @param merge Merge function
  * @param equals Equality comparison function
  * @param free Memory release function
  * @return A new data model or NULL on error
  */
 polycall_dop_data_model_t* polycall_dop_data_model_create(
     void* data,
     void* (*clone)(void* data),
     void* (*to_object)(void* data),
     void* (*merge)(void* data, void* other),
     bool (*equals)(void* data, void* other),
     void (*free)(void* data)
 ) {
     if (!clone || !to_object || !merge || !equals || !free) {
         return NULL;
     }
     
     polycall_dop_data_model_t* model = polycall_memory_alloc(sizeof(polycall_dop_data_model_t));
     if (!model) {
         return NULL;
     }
     
     model->data = data;
     model->clone = clone;
     model->to_object = to_object;
     model->merge = merge;
     model->equals = equals;
     model->free = free;
     
     return model;
 }
 
 /**
  * Destroys a DOP data model
  *
  * @param model The model to destroy
  */
 void polycall_dop_data_model_destroy(polycall_dop_data_model_t* model) {
     if (!model) {
         return;
     }
     
     if (model->data && model->free) {
         model->free(model->data);
     }
     
     polycall_memory_free(model);
 }
 
 /**
  * Creates a new DOP behavior model
  *
  * @param process Processing function
  * @param get_behavior_id Function to get behavior ID
  * @param get_description Function to get behavior description
  * @return A new behavior model or NULL on error
  */
 polycall_dop_behavior_model_t* polycall_dop_behavior_model_create(
     void* (*process)(void* data),
     const char* (*get_behavior_id)(void),
     const char* (*get_description)(void)
 ) {
     if (!process || !get_behavior_id || !get_description) {
         return NULL;
     }
     
     polycall_dop_behavior_model_t* model = polycall_memory_alloc(
         sizeof(polycall_dop_behavior_model_t));
     if (!model) {
         return NULL;
     }
     
     model->process = process;
     model->get_behavior_id = get_behavior_id;
     model->get_description = get_description;
     
     return model;
 }
 
 /**
  * Destroys a DOP behavior model
  *
  * @param model The model to destroy
  */
 void polycall_dop_behavior_model_destroy(polycall_dop_behavior_model_t* model) {
     if (!model) {
         return;
     }
     
     polycall_memory_free(model);
 }
 
 /**
  * Creates a new DOP adapter
  *
  * @param data_model The data model
  * @param behavior_model The behavior model
  * @param validator The component validator
  * @param adapter_name Name of the adapter
  * @return A new DOP adapter or NULL on error
  */
 polycall_dop_adapter_t* polycall_dop_adapter_create(
     polycall_dop_data_model_t* data_model,
     polycall_dop_behavior_model_t* behavior_model,
     polycall_component_validator_t* validator,
     const char* adapter_name
 ) {
     if (!data_model || !behavior_model || !adapter_name) {
         return NULL;
     }
     
     polycall_dop_adapter_t* adapter = polycall_memory_alloc(sizeof(polycall_dop_adapter_t));
     if (!adapter) {
         return NULL;
     }
     
     adapter->data_model = data_model;
     adapter->behavior_model = behavior_model;
     adapter->validator = validator;
     adapter->adapter_name = polycall_memory_strdup(adapter_name);
     
     if (!adapter->adapter_name) {
         polycall_memory_free(adapter);
         return NULL;
     }
     
     return adapter;
 }
 
 /**
  * Converts an object to a functional representation
  *
  * @param adapter The DOP adapter
  * @return A functional representation as a void pointer, must be cast appropriately
  */
 void* polycall_dop_adapter_to_functional(polycall_dop_adapter_t* adapter) {
     if (!adapter || !adapter->data_model || !adapter->behavior_model) {
         return NULL;
     }
     
     // This would be implemented specifically for each FFI binding
     // Example pseudocode:
     polycall_logger_log(POLYCALL_LOG_LEVEL_INFO, 
                         "Converting %s to functional paradigm", adapter->adapter_name);
     
     // Clone the data to ensure immutability
     void* data_clone = adapter->data_model->clone(adapter->data_model->data);
     
     // Create a functional wrapper
     // Note: This would be language-specific in a real implementation
     // For example, in the Python binding, this might create a Python function object
     
     return data_clone; // Placeholder return, real impl would return a functional wrapper
 }
 
 /**
  * Converts an object to an OOP representation
  *
  * @param adapter The DOP adapter
  * @return An OOP representation as a void pointer, must be cast appropriately
  */
 void* polycall_dop_adapter_to_oop(polycall_dop_adapter_t* adapter) {
     if (!adapter || !adapter->data_model || !adapter->behavior_model) {
         return NULL;
     }
     
     // This would be implemented specifically for each FFI binding
     // Example pseudocode:
     polycall_logger_log(POLYCALL_LOG_LEVEL_INFO, 
                         "Converting %s to OOP paradigm", adapter->adapter_name);
     
     // Clone the data to ensure a clean state
     void* data_clone = adapter->data_model->clone(adapter->data_model->data);
     
     // Create an OOP wrapper
     // Note: This would be language-specific in a real implementation
     // For example, in the Python binding, this might create a Python class instance
     
     return data_clone; // Placeholder return, real impl would return an OOP wrapper
 }
 
 /**
  * Destroys a DOP adapter
  *
  * @param adapter The adapter to destroy
  */
 void polycall_dop_adapter_destroy(polycall_dop_adapter_t* adapter) {
     if (!adapter) {
         return;
     }
     
     polycall_memory_free((void*)adapter->adapter_name);
     
     if (adapter->validator) {
         polycall_component_validator_destroy(adapter->validator);
     }
     
     polycall_memory_free(adapter);
 }