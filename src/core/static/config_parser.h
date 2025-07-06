#ifndef POLYCALL_CONFIG_PARSER_CONFIG_PARSER_H_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POLYCALL_CONFIG_PARSER_CONFIG_PARSER_H_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct polycall_config polycall_config_t;
typedef struct polycall_config_parser polycall_config_parser_t;
typedef struct polycall_ast polycall_ast_t;

// Configuration value types
typedef enum {
  CONFIG_TYPE_INT,
  CONFIG_TYPE_UINT,
  CONFIG_TYPE_BOOL,
  CONFIG_TYPE_STRING,
  CONFIG_TYPE_FLOAT,
  CONFIG_TYPE_DOUBLE
} config_value_type_t;

// Configuration entry structure
typedef struct config_entry {
  config_value_type_t type;
  char *key;
  union {
    int int_value;
    unsigned int uint_value;
    bool bool_value;
    char *string_value;
    float float_value;
    double double_value;
  } value;
  char *description;
  struct config_entry *next; // Pointer to the next entry in the list
  struct config_entry *prev; // Pointer to the previous entry in the list
  char section[64];          // Section name

} config_entry_t;

/**
 * @brief Parse a configuration file
 *
 * @param filename The configuration file path
 * @return polycall_config_t* The parsed configuration, or NULL if an error
 * occurred
 */
polycall_config_t *polycall_parse_config_file(const char *filename);

/**
 * @brief Parse a configuration string
 *
 * @param parser The configuration parser
 * @param source The configuration string
 * @return polycall_config_t* The parsed configuration
 */
polycall_config_t *
polycall_parse_config_string(polycall_config_parser_t *parser,
                             const char *source);

/**
 * @brief Convert an AST to a configuration structure
 *
 * @param ast The AST to convert
 * @return polycall_config_t* The converted configuration
 */
polycall_config_t *polycall_ast_to_config(polycall_ast_t *ast);

/**
 * @brief Free a configuration
 *
 * @param config The configuration to free
 */
void polycall_config_destroy(polycall_config_t *config);

/**
 * @brief Destroy a configuration parser
 *
 * @param parser The parser to destroy
 */
void polycall_config_parser_destroy(polycall_config_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_PARSER_CONFIG_PARSER_H_H */
