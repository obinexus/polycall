/* Standard library includes */
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file python_bridge_dop.c
 * @brief Python FFI bridge with DOP adapter integration
 *
 * Example implementation showing how to use the DOP adapter with Python
 * binding.
 */

#include "polycall/core/ffi/dop_adapter.h"
#include "polycall/core/ffi/python_bridge.h"
#include "polycall/core/polycall/polycall_logger.h"
#include "polycall/core/polycall/polycall_memory.h"

#include <Python.h>
#include <string.h>

// Forward declarations
static void *python_object_clone(void *data);
static void *python_object_to_object(void *data);
static void *python_object_merge(void *data, void *other);
static bool python_object_equals(void *data, void *other);
static void python_object_free(void *data);

static void *python_process_data(void *data);
static const char *python_get_behavior_id(void);
static const char *python_get_description(void);

static bool validate_string_length(const void *value, const void *context);
static bool validate_number_range(const void *value, const void *context);

/**
 * @brief Python object wrapper for DOP operations
 */
typedef struct {
  PyObject *py_object;
  char *object_type;
} python_dop_object_t;

/**
 * Initializes the Python bridge with DOP adapter support
 *
 * @return POLYCALL_SUCCESS or error code
 */
polycall_result_t polycall_python_bridge_init_dop(void) {
  // Initialize Python interpreter if not already initialized
  if (!Py_IsInitialized()) {
    Py_Initialize();
  }

  polycall_logger_log(POLYCALL_LOG_LEVEL_INFO,
                      "Initialized Python bridge with DOP adapter support");

  return POLYCALL_SUCCESS;
}

/**
 * Creates a DOP adapter for a Python object
 *
 * @param py_object The Python object to adapt
 * @param adapter_name Name for the adapter
 * @return A new DOP adapter or NULL on error
 */
polycall_dop_adapter_t *
polycall_python_create_adapter(PyObject *py_object, const char *adapter_name) {
  if (!py_object || !adapter_name) {
    return NULL;
  }

  // Create Python object wrapper
  python_dop_object_t *wrapper =
      polycall_memory_alloc(sizeof(python_dop_object_t));
  if (!wrapper) {
    return NULL;
  }

  wrapper->py_object = py_object;
  Py_INCREF(py_object);

  // Get Python object type
  PyObject *py_type = PyObject_Type(py_object);
  PyObject *py_type_name = PyObject_GetAttrString(py_type, "__name__");
  const char *type_name = PyUnicode_AsUTF8(py_type_name);
  wrapper->object_type = polycall_memory_strdup(type_name);

  Py_DECREF(py_type_name);
  Py_DECREF(py_type);

  if (!wrapper->object_type) {
    Py_DECREF(py_object);
    polycall_memory_free(wrapper);
    return NULL;
  }

  // Create data model
  polycall_dop_data_model_t *data_model = polycall_dop_data_model_create(
      wrapper, python_object_clone, python_object_to_object,
      python_object_merge, python_object_equals, python_object_free);

  if (!data_model) {
    Py_DECREF(py_object);
    polycall_memory_free(wrapper->object_type);
    polycall_memory_free(wrapper);
    return NULL;
  }

  // Create behavior model
  polycall_dop_behavior_model_t *behavior_model =
      polycall_dop_behavior_model_create(
          python_process_data, python_get_behavior_id, python_get_description);

  if (!behavior_model) {
    polycall_dop_data_model_destroy(data_model);
    return NULL;
  }

  // Create validator for Python objects
  polycall_component_validator_t *validator =
      polycall_component_validator_create("PythonValidator");

  if (!validator) {
    polycall_dop_behavior_model_destroy(behavior_model);
    polycall_dop_data_model_destroy(data_model);
    return NULL;
  }

  // Add some sample validation constraints
  polycall_component_validator_add_constraint(
      validator, "title", POLYCALL_DOP_TYPE_STRING, true,
      validate_string_length, "Title must be at least 3 characters long");

  polycall_component_validator_add_constraint(
      validator, "count", POLYCALL_DOP_TYPE_NUMBER, false,
      validate_number_range, "Count must be between 0 and 100");

  // Create DOP adapter
  polycall_dop_adapter_t *adapter = polycall_dop_adapter_create(
      data_model, behavior_model, validator, adapter_name);

  if (!adapter) {
    polycall_component_validator_destroy(validator);
    polycall_dop_behavior_model_destroy(behavior_model);
    polycall_dop_data_model_destroy(data_model);
    return NULL;
  }

  return adapter;
}

/**
 * Converts a Python object to a functional representation
 *
 * @param py_object The Python object
 * @param adapter_name Name for the adapter
 * @return A new Python function object or NULL on error
 */
PyObject *polycall_python_to_functional(PyObject *py_object,
                                        const char *adapter_name) {
  if (!py_object || !adapter_name) {
    return NULL;
  }

  // Create DOP adapter for Python object
  polycall_dop_adapter_t *adapter =
      polycall_python_create_adapter(py_object, adapter_name);

  if (!adapter) {
    return NULL;
  }

  // Convert to functional representation
  void *func_ptr = polycall_dop_adapter_to_functional(adapter);
  if (!func_ptr) {
    polycall_dop_adapter_destroy(adapter);
    return NULL;
  }

  // In a real implementation, func_ptr would be used to create a Python
  // function For this example, we'll just create a simple Python function
  // object

  PyObject *py_func = PyCFunction_New(NULL, NULL);

  // Clean up adapter
  polycall_dop_adapter_destroy(adapter);

  return py_func;
}

/**
 * Converts a Python object to an OOP representation
 *
 * @param py_object The Python object
 * @param adapter_name Name for the adapter
 * @return A new Python class object or NULL on error
 */
PyObject *polycall_python_to_oop(PyObject *py_object,
                                 const char *adapter_name) {
  if (!py_object || !adapter_name) {
    return NULL;
  }

  // Create DOP adapter for Python object
  polycall_dop_adapter_t *adapter =
      polycall_python_create_adapter(py_object, adapter_name);

  if (!adapter) {
    return NULL;
  }

  // Convert to OOP representation
  void *oop_ptr = polycall_dop_adapter_to_oop(adapter);
  if (!oop_ptr) {
    polycall_dop_adapter_destroy(adapter);
    return NULL;
  }

  // In a real implementation, oop_ptr would be used to create a Python class
  // For this example, we'll just create a simple Python class object

  PyObject *py_type = PyType_Type.tp_alloc(&PyType_Type, 0);

  // Clean up adapter
  polycall_dop_adapter_destroy(adapter);

  return py_type;
}

/**
 * Validates a Python object using the component validator
 *
 * @param py_object The Python object to validate
 * @param validator_name Name of the validator
 * @return POLYCALL_SUCCESS or error code
 */
polycall_result_t polycall_python_validate(PyObject *py_object,
                                           const char *validator_name) {
  if (!py_object || !validator_name) {
    return POLYCALL_ERROR_INVALID_PARAMETER;
  }

  // Create Python object wrapper
  python_dop_object_t *wrapper =
      polycall_memory_alloc(sizeof(python_dop_object_t));
  if (!wrapper) {
    return POLYCALL_ERROR_OUT_OF_MEMORY;
  }

  wrapper->py_object = py_object;
  Py_INCREF(py_object);

  // Get Python object type
  PyObject *py_type = PyObject_Type(py_object);
  PyObject *py_type_name = PyObject_GetAttrString(py_type, "__name__");
  const char *type_name = PyUnicode_AsUTF8(py_type_name);
  wrapper->object_type = polycall_memory_strdup(type_name);

  Py_DECREF(py_type_name);
  Py_DECREF(py_type);

  if (!wrapper->object_type) {
    Py_DECREF(py_object);
    polycall_memory_free(wrapper);
    return POLYCALL_ERROR_OUT_OF_MEMORY;
  }

  // Create validator
  polycall_component_validator_t *validator =
      polycall_component_validator_create(validator_name);

  if (!validator) {
    Py_DECREF(py_object);
    polycall_memory_free(wrapper->object_type);
    polycall_memory_free(wrapper);
    return POLYCALL_ERROR_OUT_OF_MEMORY;
  }

  // Add validation constraints (simplified example)
  polycall_component_validator_add_constraint(
      validator, "title", POLYCALL_DOP_TYPE_STRING, true,
      validate_string_length, "Title must be at least 3 characters long");

  polycall_component_validator_add_constraint(
      validator, "count", POLYCALL_DOP_TYPE_NUMBER, false,
      validate_number_range, "Count must be between 0 and 100");

  // Validate Python object
  polycall_validation_error_t error;
  polycall_result_t result = polycall_component_validator_validate(
      validator, (polycall_dop_object_t *)wrapper, &error);

  // Clean up
  polycall_component_validator_destroy(validator);
  Py_DECREF(py_object);
  polycall_memory_free(wrapper->object_type);
  polycall_memory_free(wrapper);

  if (result != POLYCALL_SUCCESS) {
    polycall_logger_log(POLYCALL_LOG_LEVEL_ERROR, "Validation failed: %s",
                        error.message);
  }

  return result;
}

/**
 * Cleans up Python bridge DOP adapter resources
 */
void polycall_python_bridge_cleanup_dop(void) {
  // Finalize Python interpreter if we initialized it
  // Note: This should be done carefully, especially in an embedded scenario
  if (Py_IsInitialized()) {
    Py_Finalize();
  }

  polycall_logger_log(POLYCALL_LOG_LEVEL_INFO,
                      "Cleaned up Python bridge DOP adapter resources");
}

// --- Implementation of internal functions ---

static void *python_object_clone(void *data) {
  python_dop_object_t *original = (python_dop_object_t *)data;
  if (!original) {
    return NULL;
  }

  python_dop_object_t *clone =
      polycall_memory_alloc(sizeof(python_dop_object_t));
  if (!clone) {
    return NULL;
  }

  clone->py_object = original->py_object;
  Py_INCREF(clone->py_object);

  clone->object_type = polycall_memory_strdup(original->object_type);
  if (!clone->object_type) {
    Py_DECREF(clone->py_object);
    polycall_memory_free(clone);
    return NULL;
  }

  return clone;
}

static void *python_object_to_object(void *data) {
  // Convert Python object to a serializable representation
  // This would be implemented based on the specific needs
  return data; // Simplified for example
}

static void *python_object_merge(void *data, void *other) {
  python_dop_object_t *obj1 = (python_dop_object_t *)data;
  python_dop_object_t *obj2 = (python_dop_object_t *)other;

  if (!obj1 || !obj2) {
    return NULL;
  }

  // This would merge two Python objects using Python's dict.update() or similar
  // For example, calling PyDict_Merge() if both objects are dictionaries

  // Simplified for example:
  return python_object_clone(data);
}

static bool python_object_equals(void *data, void *other) {
  python_dop_object_t *obj1 = (python_dop_object_t *)data;
  python_dop_object_t *obj2 = (python_dop_object_t *)other;

  if (!obj1 || !obj2) {
    return false;
  }

  // Use Python's equality comparison
  int result =
      PyObject_RichCompareBool(obj1->py_object, obj2->py_object, Py_EQ);

  return result == 1;
}

static void python_object_free(void *data) {
  python_dop_object_t *obj = (python_dop_object_t *)data;
  if (!obj) {
    return;
  }

  if (obj->py_object) {
    Py_DECREF(obj->py_object);
  }

  if (obj->object_type) {
    polycall_memory_free(obj->object_type);
  }

  polycall_memory_free(obj);
}

static void *python_process_data(void *data) {
  python_dop_object_t *obj = (python_dop_object_t *)data;
  if (!obj || !obj->py_object) {
    return NULL;
  }

  // Example processing: call a 'process' method if available
  if (PyObject_HasAttrString(obj->py_object, "process")) {
    PyObject *process_method =
        PyObject_GetAttrString(obj->py_object, "process");
    if (process_method && PyCallable_Check(process_method)) {
      PyObject *result = PyObject_CallObject(process_method, NULL);
      Py_DECREF(process_method);

      if (result) {
        // Wrap the result in another python_dop_object_t
        python_dop_object_t *result_wrapper =
            polycall_memory_alloc(sizeof(python_dop_object_t));
        if (!result_wrapper) {
          Py_DECREF(result);
          return NULL;
        }

        result_wrapper->py_object = result;
        // Don't increment reference here since we're transferring ownership

        // Get result object type
        PyObject *py_type = PyObject_Type(result);
        PyObject *py_type_name = PyObject_GetAttrString(py_type, "__name__");
        const char *type_name = PyUnicode_AsUTF8(py_type_name);
        result_wrapper->object_type = polycall_memory_strdup(type_name);

        Py_DECREF(py_type_name);
        Py_DECREF(py_type);

        if (!result_wrapper->object_type) {
          Py_DECREF(result);
          polycall_memory_free(result_wrapper);
          return NULL;
        }

        return result_wrapper;
      }
    }
  }

  // Default: clone the original object
  return python_object_clone(data);
}

static const char *python_get_behavior_id(void) {
  return "python.dop.behavior";
}

static const char *python_get_description(void) {
  return "Python DOP Behavior Model";
}

static bool validate_string_length(const void *value, const void *context) {
  if (!value) {
    return false;
  }

  python_dop_object_t *obj = (python_dop_object_t *)value;

  // Check if the Python object is a string
  if (!PyUnicode_Check(obj->py_object)) {
    return false;
  }

  // Get string length
  Py_ssize_t length = PyUnicode_GET_LENGTH(obj->py_object);

  // Validate minimum length (example: 3 characters)
  return length >= 3;
}

static bool validate_number_range(const void *value, const void *context) {
  if (!value) {
    return true; // Not required, so NULL is valid
  }

  python_dop_object_t *obj = (python_dop_object_t *)value;

  // Check if the Python object is a number
  if (!PyLong_Check(obj->py_object) && !PyFloat_Check(obj->py_object)) {
    return false;
  }

  double num_value;
  if (PyLong_Check(obj->py_object)) {
    num_value = (double)PyLong_AsLong(obj->py_object);
  } else {
    num_value = PyFloat_AsDouble(obj->py_object);
  }

  // Validate range (example: between 0 and 100)
  return num_value >= 0.0 && num_value <= 100.0;
}
