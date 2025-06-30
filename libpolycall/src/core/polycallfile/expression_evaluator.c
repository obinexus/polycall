/**
#include "polycall/core/config/polycallfile/expression_evaluator.h"

 * @file expression_evaluator.c
 * @brief Expression evaluation implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's architecture design
 *
 * This file implements expression evaluation, including arithmetic, logical,
 * and string operations.
 */

 #include "polycall/core/config/polycallfile/expression_evaluator.h"

 
 /**
  * @brief Initialize an expression evaluator
  * 
  * @param ast The AST containing variable definitions
  * @param strict_mode Whether to use strict type checking
  * @return polycall_expression_evaluator_t* The created evaluator
  */
 polycall_expression_evaluator_t* polycall_expression_evaluator_create(
     polycall_ast_t* ast, bool strict_mode) {
     polycall_expression_evaluator_t* evaluator = 
         (polycall_expression_evaluator_t*)malloc(sizeof(polycall_expression_evaluator_t));
     if (!evaluator) {
         return NULL;
     }
     
     evaluator->ast = ast;
     evaluator->strict_mode = strict_mode;
     evaluator->error_buffer[0] = '\0';
     evaluator->has_error = false;
     
     return evaluator;
 }
 
 /**
  * @brief Destroy an expression evaluator
  * 
  * @param evaluator The evaluator to destroy
  */
 void polycall_expression_evaluator_destroy(polycall_expression_evaluator_t* evaluator) {
     if (!evaluator) {
         return;
     }
     
     free(evaluator);
 }
 
 /**
  * @brief Set an error message
  * 
  * @param evaluator The expression evaluator
  * @param format The error message format
  * @param ... Format arguments
  */
 static void set_error(polycall_expression_evaluator_t* evaluator, const char* format, ...) {
     if (!evaluator) {
         return;
     }
     
     va_list args;
     va_start(args, format);
     vsnprintf(evaluator->error_buffer, sizeof(evaluator->error_buffer), format, args);
     va_end(args);
     
     evaluator->has_error = true;
 }
 
 /**
  * @brief Create a boolean value
  * 
  * @param value The boolean value
  * @return polycall_value_t The created value
  */
 polycall_value_t polycall_value_boolean(bool value) {
     polycall_value_t result;
     result.type = VALUE_BOOLEAN;
     result.as.boolean = value;
     return result;
 }
 
 /**
  * @brief Create an integer value
  * 
  * @param value The integer value
  * @return polycall_value_t The created value
  */
 polycall_value_t polycall_value_integer(int64_t value) {
     polycall_value_t result;
     result.type = VALUE_INTEGER;
     result.as.integer = value;
     return result;
 }
 
 /**
  * @brief Create a floating-point value
  * 
  * @param value The floating-point value
  * @return polycall_value_t The created value
  */
 polycall_value_t polycall_value_float(double value) {
     polycall_value_t result;
     result.type = VALUE_FLOAT;
     result.as.floating = value;
     return result;
 }
 
 /**
  * @brief Create a string value
  * 
  * @param value The string value
  * @return polycall_value_t The created value
  */
 polycall_value_t polycall_value_string(const char* value) {
     polycall_value_t result;
     result.type = VALUE_STRING;
     result.as.string = strdup(value);
     return result;
 }
 
 /**
  * @brief Create a null value
  * 
  * @return polycall_value_t The created value
  */
 polycall_value_t polycall_value_null(void) {
     polycall_value_t result;
     result.type = VALUE_NULL;
     return result;
 }
 
 /**
  * @brief Convert a value to a boolean
  * 
  * @param value The value to convert
  * @return bool The boolean representation
  */
 bool polycall_value_as_boolean(polycall_value_t value) {
     switch (value.type) {
         case VALUE_BOOLEAN:
             return value.as.boolean;
         case VALUE_INTEGER:
             return value.as.integer != 0;
         case VALUE_FLOAT:
             return value.as.floating != 0.0;
         case VALUE_STRING:
             return value.as.string != NULL && value.as.string[0] != '\0';
         case VALUE_NULL:
             return false;
         default:
             return false;
     }
 }
 
 /**
  * @brief Convert a value to an integer
  * 
  * @param value The value to convert
  * @return int64_t The integer representation
  */
 int64_t polycall_value_as_integer(polycall_value_t value) {
     switch (value.type) {
         case VALUE_BOOLEAN:
             return value.as.boolean ? 1 : 0;
         case VALUE_INTEGER:
             return value.as.integer;
         case VALUE_FLOAT:
             return (int64_t)value.as.floating;
         case VALUE_STRING:
             return value.as.string ? atoll(value.as.string) : 0;
         case VALUE_NULL:
             return 0;
         default:
             return 0;
     }
 }
 
 /**
  * @brief Convert a value to a floating-point number
  * 
  * @param value The value to convert
  * @return double The floating-point representation
  */
 double polycall_value_as_float(polycall_value_t value) {
     switch (value.type) {
         case VALUE_BOOLEAN:
             return value.as.boolean ? 1.0 : 0.0;
         case VALUE_INTEGER:
             return (double)value.as.integer;
         case VALUE_FLOAT:
             return value.as.floating;
         case VALUE_STRING:
             return value.as.string ? atof(value.as.string) : 0.0;
         case VALUE_NULL:
             return 0.0;
         default:
             return 0.0;
     }
 }
 
 /**
  * @brief Convert a value to a string
  * 
  * @param value The value to convert
  * @param buffer The buffer to write to
  * @param buffer_size The size of the buffer
  * @return int The number of bytes written, or -1 on error
  */
 int polycall_value_as_string(polycall_value_t value, char* buffer, size_t buffer_size) {
     if (!buffer || buffer_size == 0) {
         return -1;
     }
     
     switch (value.type) {
         case VALUE_BOOLEAN:
             return snprintf(buffer, buffer_size, "%s", value.as.boolean ? "true" : "false");
         case VALUE_INTEGER:
             return snprintf(buffer, buffer_size, "%lld", (long long)value.as.integer);
         case VALUE_FLOAT:
             return snprintf(buffer, buffer_size, "%g", value.as.floating);
         case VALUE_STRING:
             if (value.as.string) {
                 strncpy(buffer, value.as.string, buffer_size - 1);
                 buffer[buffer_size - 1] = '\0';
                 return strlen(buffer);
             } else {
                 buffer[0] = '\0';
                 return 0;
             }
         case VALUE_NULL:
             buffer[0] = '\0';
             return 0;
         default:
             buffer[0] = '\0';
             return 0;
     }
 }
 
 /**
  * @brief Look up a variable in the AST
  * 
  * @param evaluator The expression evaluator
  * @param name The variable name
  * @return polycall_ast_node_t* The variable node, or NULL if not found
  */
 static polycall_ast_node_t* lookup_variable(polycall_expression_evaluator_t* evaluator, 
                                           const char* name) {
     if (!evaluator || !evaluator->ast || !name) {
         return NULL;
     }
     
     return polycall_ast_find_node(evaluator->ast, name);
 }
 
 /**
  * @brief Evaluate a binary operation
  * 
  * @param evaluator The expression evaluator
  * @param left The left operand
  * @param right The right operand
  * @param op The operation
  * @return polycall_value_t The result of the operation
  */
 static polycall_value_t evaluate_binary_op(polycall_expression_evaluator_t* evaluator,
                                          polycall_value_t left, polycall_value_t right,
                                          const char* op) {
     if (!op) {
         set_error(evaluator, "Invalid operation");
         return polycall_value_null();
     }
     
     // Arithmetic operations
     if (strcmp(op, "+") == 0) {
         // Addition (or string concatenation)
         if (left.type == VALUE_STRING || right.type == VALUE_STRING) {
             // String concatenation
             char left_str[256], right_str[256];
             polycall_value_as_string(left, left_str, sizeof(left_str));
             polycall_value_as_string(right, right_str, sizeof(right_str));
             
             char* result = (char*)malloc(strlen(left_str) + strlen(right_str) + 1);
             if (!result) {
                 set_error(evaluator, "Out of memory");
                 return polycall_value_null();
             }
             
             strcpy(result, left_str);
             strcat(result, right_str);
             
             polycall_value_t value = polycall_value_string(result);
             free(result);
             return value;
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             // Floating-point addition
             return polycall_value_float(polycall_value_as_float(left) + 
                                        polycall_value_as_float(right));
         } else {
             // Integer addition
             return polycall_value_integer(polycall_value_as_integer(left) + 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "-") == 0) {
         // Subtraction
         if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_float(polycall_value_as_float(left) - 
                                        polycall_value_as_float(right));
         } else {
             return polycall_value_integer(polycall_value_as_integer(left) - 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "*") == 0) {
         // Multiplication
         if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_float(polycall_value_as_float(left) * 
                                        polycall_value_as_float(right));
         } else {
             return polycall_value_integer(polycall_value_as_integer(left) * 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "/") == 0) {
         // Division
         double right_val = polycall_value_as_float(right);
         if (right_val == 0.0) {
             set_error(evaluator, "Division by zero");
             return polycall_value_null();
         }
         
         // Always return float for division
         return polycall_value_float(polycall_value_as_float(left) / right_val);
     } else if (strcmp(op, "%") == 0) {
         // Modulo
         int64_t right_val = polycall_value_as_integer(right);
         if (right_val == 0) {
             set_error(evaluator, "Modulo by zero");
             return polycall_value_null();
         }
         
         return polycall_value_integer(polycall_value_as_integer(left) % right_val);
     }
     
     // Comparison operations
     else if (strcmp(op, "==") == 0) {
         // Equality
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) == 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(fabs(polycall_value_as_float(left) - 
                                               polycall_value_as_float(right)) < 1e-10);
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) == 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "!=") == 0) {
         // Inequality
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) != 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(fabs(polycall_value_as_float(left) - 
                                               polycall_value_as_float(right)) >= 1e-10);
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) != 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "<") == 0) {
         // Less than
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) < 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(polycall_value_as_float(left) < 
                                          polycall_value_as_float(right));
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) < 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, "<=") == 0) {
         // Less than or equal
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) <= 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(polycall_value_as_float(left) <= 
                                          polycall_value_as_float(right));
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) <= 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, ">") == 0) {
         // Greater than
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) > 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(polycall_value_as_float(left) > 
                                          polycall_value_as_float(right));
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) > 
                                          polycall_value_as_integer(right));
         }
     } else if (strcmp(op, ">=") == 0) {
         // Greater than or equal
         if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
             return polycall_value_boolean(strcmp(left.as.string, right.as.string) >= 0);
         } else if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
             return polycall_value_boolean(polycall_value_as_float(left) >= 
                                          polycall_value_as_float(right));
         } else {
             return polycall_value_boolean(polycall_value_as_integer(left) >= 
                                          polycall_value_as_integer(right));
         }
     }
     
     // Logical operations
     else if (strcmp(op, "&&") == 0) {
         // Logical AND
         return polycall_value_boolean(polycall_value_as_boolean(left) && 
                                      polycall_value_as_boolean(right));
     } else if (strcmp(op, "||") == 0) {
         // Logical OR
         return polycall_value_boolean(polycall_value_as_boolean(left) || 
                                      polycall_value_as_boolean(right));
     }
     
     // Unknown operation
     set_error(evaluator, "Unknown operation: %s", op);
     return polycall_value_null();
 }
 
 /**
  * @brief Evaluate a unary operation
  * 
  * @param evaluator The expression evaluator
  * @param operand The operand
  * @param op The operation
  * @return polycall_value_t The result of the operation
  */
 static polycall_value_t evaluate_unary_op(polycall_expression_evaluator_t* evaluator,
                                         polycall_value_t operand, const char* op) {
     if (!op) {
         set_error(evaluator, "Invalid operation");
         return polycall_value_null();
     }
     
     if (strcmp(op, "-") == 0) {
         // Negation
         if (operand.type == VALUE_FLOAT) {
             return polycall_value_float(-operand.as.floating);
         } else {
             return polycall_value_integer(-polycall_value_as_integer(operand));
         }
     } else if (strcmp(op, "!") == 0) {
         // Logical NOT
         return polycall_value_boolean(!polycall_value_as_boolean(operand));
     }
     
     // Unknown operation
     set_error(evaluator, "Unknown operation: %s", op);
     return polycall_value_null();
 }
 
 /**
  * @brief Evaluate a literal value
  * 
  * @param evaluator The expression evaluator
  * @param node The value node
  * @return polycall_value_t The evaluated value
  */
 static polycall_value_t evaluate_literal(polycall_expression_evaluator_t* evaluator,
                                        polycall_ast_node_t* node) {
     if (!node) {
         set_error(evaluator, "Invalid node");
         return polycall_value_null();
     }
     
     switch (node->type) {
         case NODE_VALUE_BOOLEAN:
             return polycall_value_boolean(strcmp(node->name, "true") == 0);
             
         case NODE_VALUE_NUMBER:
             // Check if it's an integer or float
             if (strchr(node->name, '.') != NULL) {
                 return polycall_value_float(atof(node->name));
             } else {
                 return polycall_value_integer(atoll(node->name));
             }
             
         case NODE_VALUE_STRING:
             return polycall_value_string(node->name);
             
         case NODE_VALUE_NULL:
             return polycall_value_null();
             
         default:
             set_error(evaluator, "Invalid value type");
             return polycall_value_null();
     }
 }
 
 /**
  * @brief Forward declaration for recursive evaluation
  */
 static polycall_value_t evaluate_node(polycall_expression_evaluator_t* evaluator,
                                     polycall_ast_node_t* node);
 
 /**
  * @brief Evaluate an identifier (variable reference)
  * 
  * @param evaluator The expression evaluator
  * @param node The identifier node
  * @return polycall_value_t The evaluated value
  */
 static polycall_value_t evaluate_identifier(polycall_expression_evaluator_t* evaluator,
                                           polycall_ast_node_t* node) {
     if (!node) {
         set_error(evaluator, "Invalid node");
         return polycall_value_null();
     }
     
     // Look up the variable
     polycall_ast_node_t* var_node = lookup_variable(evaluator, node->name);
     if (!var_node) {
         set_error(evaluator, "Undefined variable: %s", node->name);
         return polycall_value_null();
     }
     
     // The variable's value should be the first child of the variable node
     if (var_node->child_count < 1) {
         set_error(evaluator, "Variable has no value: %s", node->name);
         return polycall_value_null();
     }
     
     // Evaluate the variable's value
     return evaluate_node(evaluator, var_node->children[0]);
 }
 
 /**
  * @brief Evaluate a binary operation node
  * 
  * @param evaluator The expression evaluator
  * @param node The binary operation node
  * @return polycall_value_t The evaluated value
  */
 static polycall_value_t evaluate_binary_op_node(polycall_expression_evaluator_t* evaluator,
                                               polycall_ast_node_t* node) {
     if (!node || node->child_count < 3) {
         set_error(evaluator, "Invalid binary operation node");
         return polycall_value_null();
     }
     
     // The first child is the left operand
     polycall_value_t left = evaluate_node(evaluator, node->children[0]);
     if (evaluator->has_error) {
         return polycall_value_null();
     }
     
     // The second child is the operator
     const char* op = node->children[1]->name;
     
     // The third child is the right operand
     polycall_value_t right = evaluate_node(evaluator, node->children[2]);
     if (evaluator->has_error) {
         return polycall_value_null();
     }
     
     // Evaluate the operation
     return evaluate_binary_op(evaluator, left, right, op);
 }
 
 /**
  * @brief Evaluate a unary operation node
  * 
  * @param evaluator The expression evaluator
  * @param node The unary operation node
  * @return polycall_value_t The evaluated value
  */
 static polycall_value_t evaluate_unary_op_node(polycall_expression_evaluator_t* evaluator,
                                              polycall_ast_node_t* node) {
     if (!node || node->child_count < 2) {
         set_error(evaluator, "Invalid unary operation node");
         return polycall_value_null();
     }
     
     // The first child is the operator
     const char* op = node->children[0]->name;
     
     // The second child is the operand
     polycall_value_t operand = evaluate_node(evaluator, node->children[1]);
     if (evaluator->has_error) {
         return polycall_value_null();
     }
     
     // Evaluate the operation
     return evaluate_unary_op(evaluator, operand, op);
 }
 
 /**
  * @brief Evaluate a node
  * 
  * @param evaluator The expression evaluator
  * @param node The node to evaluate
  * @return polycall_value_t The evaluated value
  */
 static polycall_value_t evaluate_node(polycall_expression_evaluator_t* evaluator,
                                     polycall_ast_node_t* node) {
     if (!node) {
         set_error(evaluator, "Invalid node");
         return polycall_value_null();
     }
     
     switch (node->type) {
         case NODE_VALUE_BOOLEAN:
         case NODE_VALUE_NUMBER:
         case NODE_VALUE_STRING:
         case NODE_VALUE_NULL:
             return evaluate_literal(evaluator, node);
             
         case NODE_IDENTIFIER:
             return evaluate_identifier(evaluator, node);
             
         case NODE_EXPRESSION_BINARY:
             return evaluate_binary_op_node(evaluator, node);
             
         case NODE_EXPRESSION_UNARY:
             return evaluate_unary_op_node(evaluator, node);
             
         default:
             set_error(evaluator, "Unsupported node type for evaluation");
             return polycall_value_null();
     }
 }
 
 /**
  * @brief Evaluate an expression node
  * 
  * @param evaluator The expression evaluator
  * @param node The expression node to evaluate
  * @return polycall_value_t The result of the evaluation
  */
 polycall_value_t polycall_expression_evaluate(polycall_expression_evaluator_t* evaluator,
                                              polycall_ast_node_t* node) {
     if (!evaluator || !node) {
         if (evaluator) {
             set_error(evaluator, "Invalid expression");
         }
         return polycall_value_null();
     }
     
     // Reset error state
     evaluator->has_error = false;
     evaluator->error_buffer[0] = '\0';
     
     // Evaluate the node
     return evaluate_node(evaluator, node);
 }
 
 /**
  * @brief Check if the most recent evaluation had an error
  * 
  * @param evaluator The expression evaluator
  * @return bool True if an error occurred, false otherwise
  */
 bool polycall_expression_has_error(polycall_expression_evaluator_t* evaluator) {
     if (!evaluator) {
         return true;
     }
     
     return evaluator->has_error;
 }
 
 /**
  * @brief Get the most recent error message
  * 
  * @param evaluator The expression evaluator
  * @return const char* The error message
  */
 const char* polycall_expression_get_error(polycall_expression_evaluator_t* evaluator) {
     if (!evaluator) {
         return "Invalid evaluator";
     }
     
     return evaluator->error_buffer;
 }