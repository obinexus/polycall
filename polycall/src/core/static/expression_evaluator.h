/**
 * @file expression_evaluator.h
 * @brief Expression evaluation system for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's architecture design
 *
 * This file defines the interfaces for evaluating expressions within the
 * configuration parser, supporting conditional configurations.
 */

#ifndef POLYCALL_CONFIG_POLYCALLFILE_EXPRESSION_EVALUATOR_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_EXPRESSION_EVALUATOR_H_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ast.h"

/**
 * @brief Value types for expression evaluation
 */
typedef enum {
	VALUE_NULL,
	VALUE_BOOLEAN,
	VALUE_INTEGER,
	VALUE_FLOAT,
	VALUE_STRING
} polycall_value_type_t;

/**
 * @brief Value structure for expression evaluation
 */
typedef struct {
	polycall_value_type_t type;
	union {
		bool boolean;
		int64_t integer;
		double floating;
		char* string;
	} as;
} polycall_value_t;

/**
 * @brief Expression evaluation context
 */
typedef struct {
	polycall_ast_t* ast;          /**< AST containing the variable definitions */
	bool strict_mode;             /**< Whether to use strict type checking */
	char error_buffer[256];       /**< Buffer for error messages */
	bool has_error;               /**< Whether an error occurred */
} polycall_expression_evaluator_t;

/**
 * @brief Initialize an expression evaluator
 * 
 * @param ast The AST containing variable definitions
 * @param strict_mode Whether to use strict type checking
 * @return polycall_expression_evaluator_t* The created evaluator
 */
polycall_expression_evaluator_t* polycall_expression_evaluator_create(
	polycall_ast_t* ast, bool strict_mode);

/**
 * @brief Destroy an expression evaluator
 * 
 * @param evaluator The evaluator to destroy
 */
void polycall_expression_evaluator_destroy(polycall_expression_evaluator_t* evaluator);

/**
 * @brief Evaluate an expression node
 * 
 * @param evaluator The expression evaluator
 * @param node The expression node to evaluate
 * @return polycall_value_t The result of the evaluation
 */
polycall_value_t polycall_expression_evaluate(polycall_expression_evaluator_t* evaluator,
											 polycall_ast_node_t* node);

/**
 * @brief Check if the most recent evaluation had an error
 * 
 * @param evaluator The expression evaluator
 * @return bool True if an error occurred, false otherwise
 */
bool polycall_expression_has_error(polycall_expression_evaluator_t* evaluator);

/**
 * @brief Get the most recent error message
 * 
 * @param evaluator The expression evaluator
 * @return const char* The error message
 */
const char* polycall_expression_get_error(polycall_expression_evaluator_t* evaluator);

/**
 * @brief Create a boolean value
 * 
 * @param value The boolean value
 * @return polycall_value_t The created value
 */
polycall_value_t polycall_value_boolean(bool value);

/**
 * @brief Create an integer value
 * 
 * @param value The integer value
 * @return polycall_value_t The created value
 */
polycall_value_t polycall_value_integer(int64_t value);

/**
 * @brief Create a floating-point value
 * 
 * @param value The floating-point value
 * @return polycall_value_t The created value
 */
polycall_value_t polycall_value_float(double value);

/**
 * @brief Create a string value
 * 
 * @param value The string value
 * @return polycall_value_t The created value
 */
polycall_value_t polycall_value_string(const char* value);

/**
 * @brief Create a null value
 * 
 * @return polycall_value_t The created value
 */
polycall_value_t polycall_value_null(void);

/**
 * @brief Convert a value to a boolean
 * 
 * @param value The value to convert
 * @return bool The boolean representation
 */
bool polycall_value_as_boolean(polycall_value_t value);

/**
 * @brief Convert a value to an integer
 * 
 * @param value The value to convert
 * @return int64_t The integer representation
 */
int64_t polycall_value_as_integer(polycall_value_t value);

/**
 * @brief Convert a value to a floating-point number
 * 
 * @param value The value to convert
 * @return double The floating-point representation
 */
double polycall_value_as_float(polycall_value_t value);

/**
 * @brief Convert a value to a string
 * 
 * @param value The value to convert
 * @param buffer The buffer to write to
 * @param buffer_size The size of the buffer 
 * @return int The number of bytes written, or -1 on error
 */
int polycall_value_as_string(polycall_value_t value, char* buffer, size_t buffer_size);

#endif /* POLYCALL_CONFIG_POLYCALLFILE_EXPRESSION_EVALUATOR_H_H */
