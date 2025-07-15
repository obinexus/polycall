/**
 * @file dop_adapter.c
 * @brief Data-Oriented Programming adapter for FFI bindings
 *
 * Implements a pattern for cross-language validation and runtime
 * verification inspired by the OBIX DOP adapter pattern.
 */

#include "polycall/core/ffi/dop_adapter.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"

#include <stdlib.h>
#include <string.h>

/** Validation constraint callback type */
typedef bool (*polycall_validation_func)(const void* value,
                                         const void* context);

/** Validation constraint entry */
typedef struct {
    polycall_dop_data_type_t type;
    bool required;
    polycall_validation_func validate;
    const char* error_message;
} polycall_validation_constraint_t;

/** Component validator structure */
struct polycall_component_validator {
    polycall_validation_constraint_t* constraints;
    size_t constraint_count;
    const char* component_name;
};

/** DOP data model implementation */
struct polycall_dop_data_model {
    void* data;
    void* (*clone)(void* data);
    void* (*to_object)(void* data);
    void* (*merge)(void* data, void* other);
    bool (*equals)(void* data, void* other);
    void (*free)(void* data);
};

/** DOP behavior model implementation */
struct polycall_dop_behavior_model {
    void* (*process)(void* data);
    const char* (*get_behavior_id)(void);
    const char* (*get_description)(void);
};

/** DOP adapter implementation */
struct polycall_dop_adapter {
    polycall_dop_data_model_t* data_model;
    polycall_dop_behavior_model_t* behavior_model;
    polycall_component_validator_t* validator;
    const char* adapter_name;
};

polycall_component_validator_t* polycall_component_validator_create(
    const char* component_name) {
    if (!component_name) {
        return NULL;
    }

    polycall_component_validator_t* validator =
        polycall_memory_alloc(sizeof(polycall_component_validator_t));
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

polycall_result_t polycall_component_validator_add_constraint(
    polycall_component_validator_t* validator,
    const char* prop_name,
    polycall_dop_data_type_t type,
    bool required,
    polycall_validation_func validate,
    const char* error_message) {
    if (!validator || !prop_name || !validate || !error_message) {
        return POLYCALL_ERROR_INVALID_PARAMETER;
    }

    size_t new_count = validator->constraint_count + 1;
    polycall_validation_constraint_t* new_constraints = polycall_memory_realloc(
        validator->constraints,
        sizeof(polycall_validation_constraint_t) * new_count);

    if (!new_constraints) {
        return POLYCALL_ERROR_OUT_OF_MEMORY;
    }

    validator->constraints = new_constraints;

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

polycall_result_t polycall_component_validator_validate(
    polycall_component_validator_t* validator,
    const polycall_dop_object_t* props,
    polycall_validation_error_t* error_out) {
    if (!validator || !props) {
        return POLYCALL_ERROR_INVALID_PARAMETER;
    }

    for (size_t i = 0; i < validator->constraint_count; i++) {
        const polycall_validation_constraint_t* constraint =
            &validator->constraints[i];
        const char* prop_name = validator->constraints[i].error_message; /*
                                                                          *
                                                                          *
                                                                          * Placeholder - in a real
                                                                          * implementation this would
                                                                          * reference the property
                                                                          * name
                                                                          */
        const void* prop_value = NULL; /* Would obtain from props by name */

        if (constraint->required && !prop_value) {
            if (error_out) {
                error_out->code = "MISSING_REQUIRED_PROP";
                snprintf(error_out->message, sizeof(error_out->message),
                         "Required prop '%s' is missing", prop_name);
                error_out->source = validator->component_name;
            }
            return POLYCALL_ERROR_VALIDATION_FAILED;
        }

        if (!prop_value && !constraint->required) {
            continue;
        }

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

void polycall_component_validator_destroy(
    polycall_component_validator_t* validator) {
    if (!validator) {
        return;
    }

    for (size_t i = 0; i < validator->constraint_count; i++) {
        polycall_memory_free((void*)validator->constraints[i].error_message);
    }

    polycall_memory_free(validator->constraints);
    polycall_memory_free((void*)validator->component_name);
    polycall_memory_free(validator);
}

polycall_dop_data_model_t* polycall_dop_data_model_create(
    void* data,
    void* (*clone)(void* data),
    void* (*to_object)(void* data),
    void* (*merge)(void* data, void* other),
    bool (*equals)(void* data, void* other),
    void (*free_func)(void* data)) {
    if (!clone || !to_object || !merge || !equals || !free_func) {
        return NULL;
    }

    polycall_dop_data_model_t* model =
        polycall_memory_alloc(sizeof(polycall_dop_data_model_t));
    if (!model) {
        return NULL;
    }

    model->data = data;
    model->clone = clone;
    model->to_object = to_object;
    model->merge = merge;
    model->equals = equals;
    model->free = free_func;

    return model;
}

void polycall_dop_data_model_destroy(polycall_dop_data_model_t* model) {
    if (!model) {
        return;
    }

    if (model->data && model->free) {
        model->free(model->data);
    }

    polycall_memory_free(model);
}

polycall_dop_behavior_model_t* polycall_dop_behavior_model_create(
    void* (*process)(void* data),
    const char* (*get_behavior_id)(void),
    const char* (*get_description)(void)) {
    if (!process || !get_behavior_id || !get_description) {
        return NULL;
    }

    polycall_dop_behavior_model_t* model =
        polycall_memory_alloc(sizeof(polycall_dop_behavior_model_t));
    if (!model) {
        return NULL;
    }

    model->process = process;
    model->get_behavior_id = get_behavior_id;
    model->get_description = get_description;

    return model;
}

void polycall_dop_behavior_model_destroy(polycall_dop_behavior_model_t* model) {
    if (!model) {
        return;
    }

    polycall_memory_free(model);
}

polycall_dop_adapter_t* polycall_dop_adapter_create(
    polycall_dop_data_model_t* data_model,
    polycall_dop_behavior_model_t* behavior_model,
    polycall_component_validator_t* validator,
    const char* adapter_name) {
    if (!data_model || !behavior_model || !adapter_name) {
        return NULL;
    }

    polycall_dop_adapter_t* adapter =
        polycall_memory_alloc(sizeof(polycall_dop_adapter_t));
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

void* polycall_dop_adapter_to_functional(polycall_dop_adapter_t* adapter) {
    if (!adapter || !adapter->data_model || !adapter->behavior_model) {
        return NULL;
    }

    polycall_logger_log(POLYCALL_LOG_INFO,
                        "Converting %s to functional paradigm",
                        adapter->adapter_name);

    void* data_clone = adapter->data_model->clone(adapter->data_model->data);

    return data_clone; /* Placeholder for real functional wrapper */
}

void* polycall_dop_adapter_to_oop(polycall_dop_adapter_t* adapter) {
    if (!adapter || !adapter->data_model || !adapter->behavior_model) {
        return NULL;
    }

    polycall_logger_log(POLYCALL_LOG_INFO,
                        "Converting %s to OOP paradigm",
                        adapter->adapter_name);

    void* data_clone = adapter->data_model->clone(adapter->data_model->data);

    return data_clone; /* Placeholder for real OOP wrapper */
}

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

