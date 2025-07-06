#ifndef POLYCALL_CONFIG_SCHEMA_CONFIG_SCHEMA_H_H
#include <errno.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stddef.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdio.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdio.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdlib.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdlib.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <string.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <string.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

#define POLYCALL_CONFIG_SCHEMA_CONFIG_SCHEMA_H_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Supported field types in schema definitions
 */
typedef enum {
  SCHEMA_FIELD_STRING,      // String field
  SCHEMA_FIELD_INTEGER,     // Integer field
  SCHEMA_FIELD_FLOAT,       // Floating point field
  SCHEMA_FIELD_BOOLEAN,     // Boolean field
  SCHEMA_FIELD_ENUM,        // Enumeration field
  SCHEMA_FIELD_BITMASK,     // Bitmask field
  SCHEMA_FIELD_OBJECT,      // Nested object field
  SCHEMA_FIELD_STRING_ARRAY // String array field
} schema_field_type_t;

/**
 * @brief Schema field structure
 */
typedef struct {
  const char *name;         // Field name
  schema_field_type_t type; // Field type
  bool required;            // Whether the field is required
  const char *
      *allowed_values;       // Array of allowed string values (for string type)
  int allowed_values_count;  // Number of allowed values
  int64_t min_value;         // Minimum value (for numeric types)
  int64_t max_value;         // Maximum value (for numeric types)
  const char *regex_pattern; // Regex pattern (for string validation)
  const char *description;   // Field description for error messages
} schema_field_t;

/**
 * @brief Supported field types in schema definitions
 */
typedef enum {
  SCHEMA_FIELD_STRING,      // String field
  SCHEMA_FIELD_INTEGER,     // Integer field
  SCHEMA_FIELD_FLOAT,       // Floating point field
  SCHEMA_FIELD_BOOLEAN,     // Boolean field
  SCHEMA_FIELD_ENUM,        // Enumeration field
  SCHEMA_FIELD_BITMASK,     // Bitmask field
  SCHEMA_FIELD_OBJECT,      // Nested object field
  SCHEMA_FIELD_STRING_ARRAY // String array field
} schema_field_type_t;

/**
 * @brief Generic field constraints union
 */
typedef union {
  struct {
    int64_t min;
    int64_t max;
  } numeric;
  struct {
    const char **values;
    size_t count;
  } enumeration;
  struct {
    size_t min_len;
    size_t max_len;
    const char *pattern;
  } string;
} field_constraints_t;
/**
 * @brief Schema section structure
 */
typedef struct schema_section {
  const char *name;                // Section name
  schema_field_t *fields;          // Array of fields
  int field_count;                 // Number of fields
  struct schema_section *sections; // Array of sub-sections
  int section_count;               // Number of sub-sections
  bool allow_unknown_fields;       // Whether to allow unknown fields
} schema_section_t;

/**
 * @brief Schema context structure
 */
struct polycall_schema_context {
  polycall_core_context_t *core_ctx; // Core context
  schema_section_t *root_sections;   // Array of root sections
  int root_section_count;            // Number of root sections
  bool strict_validation;            // Whether to perform strict validation
};

// Configuration schema types
typedef enum {
  CONFIG_TYPE_NULL,
  CONFIG_TYPE_BOOLEAN,
  CONFIG_TYPE_INTEGER,
  CONFIG_TYPE_FLOAT,
  CONFIG_TYPE_STRING,
  CONFIG_TYPE_ARRAY,
  CONFIG_TYPE_OBJECT
} config_value_type_t;

// Configuration value structure
typedef struct config_value {
  config_value_type_t type;
  union {
    bool boolean;
    int64_t integer;
    double float_val;
    char *string;
    struct {
      struct config_value *items;
      size_t count;
    } array;
    struct {
      char **keys;
      struct config_value *values;
      size_t count;
    } object;
  } data;
} config_value_t;

// Function declarations
config_value_t *config_create_value(config_value_type_t type);
void config_free_value(config_value_t *value);
bool config_set_boolean(config_value_t *value, bool boolean_value);
bool config_set_integer(config_value_t *value, int64_t integer_value);
bool config_set_float(config_value_t *value, double float_value);
bool config_set_string(config_value_t *value, const char *string_value);
bool config_array_append(config_value_t *array, config_value_t *item);
bool config_object_set(config_value_t *object, const char *key,
                       config_value_t *value);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_SCHEMA_CONFIG_SCHEMA_H_H */