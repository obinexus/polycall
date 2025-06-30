/**
 * @file dop_adapter.h
 * @brief Data-Oriented Programming adapter for FFI bindings
 * 
 * Defines interfaces for cross-language validation and runtime verification
 * inspired by the OBIX DOP adapter pattern.
 */

 #ifndef POLYCALL_FFI_DOP_ADAPTER_H_H
 #define POLYCALL_FFI_DOP_ADAPTER_H_H
 
 #include "polycall/core/polycall/polycall_error.h"
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Generic object type for DOP operations
  */
 typedef void polycall_dop_object_t;
 
 /**
  * @brief Data types supported in DOP validation
  */
 typedef enum {
     POLYCALL_DOP_TYPE_STRING,
     POLYCALL_DOP_TYPE_NUMBER,
     POLYCALL_DOP_TYPE_BOOLEAN,
     POLYCALL_DOP_TYPE_ARRAY,
     POLYCALL_DOP_TYPE_OBJECT,
     POLYCALL_DOP_TYPE_FUNCTION,
     POLYCALL_DOP_TYPE_ANY
 } polycall_dop_data_type_t;
 
 /**
  * @brief Validation error structure
  */
 typedef struct {
     const char* code;
     char message[256];
     const char* source;
 } polycall_validation_error_t;
 
 /**
  * @brief Opaque component validator structure
  */
 typedef struct polycall_component_validator polycall_component_validator_t;
 
 /**
  * @brief Opaque DOP data model structure
  */
 typedef struct polycall_dop_data_model polycall_dop_data_model_t;
 
 /**
  * @brief Opaque DOP behavior model structure
  */
 typedef struct polycall_dop_behavior_model polycall_dop_behavior_model_t;
 
 /**
  * @brief Opaque DOP adapter structure
  */
 typedef struct polycall_dop_adapter polycall_dop_adapter_t;
 
 /**
  * Creates a new component validator
  *
  * @param component_name Name of the component being validated
  * @return A new component validator or NULL on error
  */
 polycall_component_validator_t* polycall_component_validator_create(const char* component_name);
 
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
     bool (*validate)(const void* value, const void* context),
     const char* error_message
 );
 
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
 );
 
 /**
  * Destroys a component validator
  *
  * @param validator The validator to destroy
  */
 void polycall_component_validator_destroy(polycall_component_validator_t* validator);
 
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
 );
 
 /**
  * Destroys a DOP data model
  *
  * @param model The model to destroy
  */
 void polycall_dop_data_model_destroy(polycall_dop_data_model_t* model);
 
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
 );
 
 /**
  * Destroys a DOP behavior model
  *
  * @param model The model to destroy
  */
 void polycall_dop_behavior_model_destroy(polycall_dop_behavior_model_t* model);
 
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
 );
 
 /**
  * Converts an object to a functional representation
  *
  * @param adapter The DOP adapter
  * @return A functional representation as a void pointer, must be cast appropriately
  */
 void* polycall_dop_adapter_to_functional(polycall_dop_adapter_t* adapter);
 
 /**
  * Converts an object to an OOP representation
  *
  * @param adapter The DOP adapter
  * @return An OOP representation as a void pointer, must be cast appropriately
  */
 void* polycall_dop_adapter_to_oop(polycall_dop_adapter_t* adapter);
 
 /**
  * Destroys a DOP adapter
  *
  * @param adapter The adapter to destroy
  */
 void polycall_dop_adapter_destroy(polycall_dop_adapter_t* adapter);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_FFI_DOP_ADAPTER_H_H */