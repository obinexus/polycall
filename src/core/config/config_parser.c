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
#include "polycall/core/config/parser/config_parser.h"

 * @file config_parser.c
 * @brief Configuration parser implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file implements the top-level interface for the configuration parser.
 */

#include "polycall/core/config/parser/config_parser.h"
#include "polycall/core/config/polycallfile/ast.h"
#include "polycall/core/config/polycallfile/parser.h"
#include "polycall/core/polycall/config/polycallfile/config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Parse a configuration file
 *
 * @param filename The configuration file path
 * @return polycall_config_t* The parsed configuration, or NULL if an error
 * occurred
 */
polycall_config_t *polycall_parse_config_file(const char *filename) {
  // Open the file
  FILE *file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Read the file contents
  char *source = (char *)malloc(file_size + 1);
  if (!source) {
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(source, 1, file_size, file);
  source[bytes_read] = '\0';

  fclose(file);

  // Parse the file
  polycall_config_t *config = polycall_parse_config_string(source);

  free(source);

  return config;
}

/**
 * @brief Parse a configuration string
 *
 * @param source The configuration string
 * @return polycall_config_t* The parsed configuration, or NULL if an error
 * occurred
 */
polycall_config_t *polycall_parse_config_string(const char *source) {
  // Create a tokenizer
  polycall_tokenizer_t *tokenizer = polycall_tokenizer_create(source);
  if (!tokenizer) {
    return NULL;
  }

  // Create a parser
  polycall_parser_t *parser = polycall_parser_create(tokenizer);
  if (!parser) {
    polycall_tokenizer_destroy(tokenizer);
    return NULL;
  }

  // Parse the tokens into an AST
  polycall_ast_t *ast = polycall_parser_parse(parser);

  // Check for errors
  bool had_error = parser->had_error;

  // Clean up
  polycall_parser_destroy(parser);
  polycall_tokenizer_destroy(tokenizer);

  if (had_error || !ast) {
    if (ast) {
      polycall_ast_destroy(ast);
    }
    return NULL;
  }

  // Convert the AST to a configuration structure
  polycall_config_t *config = polycall_ast_to_config(ast);

  // Clean up
  polycall_ast_destroy(ast);

  return config;
}

/**
 * @brief Convert an AST to a configuration structure
 *
 * @param ast The AST to convert
 * @return polycall_config_t* The converted configuration
 */
polycall_config_t *polycall_ast_to_config(polycall_ast_t *ast) {
  // Create a new configuration
  polycall_config_t *config =
      (polycall_config_t *)malloc(sizeof(polycall_config_t));
  if (!config) {
    return NULL;
  }

  // Initialize configuration
  memset(config, 0, sizeof(polycall_config_t));

  // TODO: Convert the AST to configuration structures
  // For now, return an empty configuration

  return config;
}

/**
 * @brief Free a configuration
 *
 * @param config The configuration to free
 */
void polycall_config_destroy(polycall_config_t *config) {
  if (!config) {
    return;
  }

  // TODO: Free any allocated resources in the configuration

  free(config);
}

/**
 * @brief Initialize the configuration parser components
 *
 * @param parser The configuration parser
 * @return bool True if successful, false otherwise
 */
static bool initialize_parser_components(polycall_config_parser_t *parser) {
  // Create macro expander
  parser->macro_expander = polycall_macro_expander_create();
  if (!parser->macro_expander) {
    return false;
  }

  // Create expression evaluator (will be initialized after we have an AST)
  parser->expression_evaluator = NULL;

  return true;
}

/**
 * @brief Process configuration directives in the AST
 *
 * @param parser The configuration parser
 * @param ast The AST to process
 * @return bool True if successful, false otherwise
 */
static bool process_directives(polycall_config_parser_t *parser,
                               polycall_ast_t *ast) {
  if (!parser || !ast || !ast->root) {
    return false;
  }

  // First pass: Process @define directives
  for (int i = 0; i < ast->root->child_count; i++) {
    polycall_ast_node_t *child = ast->root->children[i];

    if (child->type == NODE_DIRECTIVE && strcmp(child->name, "define") == 0 &&
        child->child_count >= 2) {

      // Get macro name and value
      const char *name = child->children[0]->name;
      polycall_ast_node_t *value = child->children[1];

      // Register the macro
      if (value->type == NODE_VALUE_STRING ||
          value->type == NODE_VALUE_NUMBER ||
          value->type == NODE_VALUE_BOOLEAN) {
        polycall_macro_register(parser->macro_expander, name, value->name);
      } else {
        // More complex macro (needs proper implementation)
        // For now, just register it as a string
        polycall_macro_register(parser->macro_expander, name, "complex_macro");
      }
    }
  }

  // Second pass: Process imports
  for (int i = 0; i < ast->root->child_count; i++) {
    polycall_ast_node_t *child = ast->root->children[i];

    if (child->type == NODE_DIRECTIVE && strcmp(child->name, "import") == 0 &&
        child->child_count >= 1) {

      // Get filename
      const char *filename = child->children[0]->name;

      // Import the file (placeholder)
      // In a real implementation, this would load and parse the file
      // and merge its AST with the current one
      printf("Import: %s\n", filename);
    }
  }

  // Third pass: Expand macros
  if (!polycall_macro_expand_ast(parser->macro_expander, ast)) {
    return false;
  }

  // Now that we have an AST with expanded macros, we can create the expression
  // evaluator
  parser->expression_evaluator =
      polycall_expression_evaluator_create(ast, true);
  if (!parser->expression_evaluator) {
    return false;
  }

  // Fourth pass: Process conditionals
  process_conditionals(parser, ast->root);

  return true;
}

/**
 * @brief Process conditional directives
 *
 * @param parser The configuration parser
 * @param node The node to process
 * @return bool True if the node should be kept, false if it should be removed
 */
static bool process_conditionals(polycall_config_parser_t *parser,
                                 polycall_ast_node_t *node) {
  if (!parser || !node) {
    return true;
  }

  // Process children first
  for (int i = 0; i < node->child_count; i++) {
    polycall_ast_node_t *child = node->children[i];

    if (child->type == NODE_DIRECTIVE_IF) {
      // Evaluate the condition
      if (child->child_count >= 1) {
        polycall_value_t result = polycall_expression_evaluate(
            parser->expression_evaluator, child->children[0]);

        bool condition = polycall_value_as_boolean(result);

        if (condition) {
          // Condition is true, keep the if block
          if (child->child_count >= 2) {
            // Process the if block
            process_conditionals(parser, child->children[1]);
          }

          // Remove the else block, if present
          if (child->child_count >= 3) {
            polycall_ast_node_destroy(child->children[2]);
            child->children[2] = NULL;
            child->child_count = 2;
          }
        } else {
          // Condition is false, remove the if block
          if (child->child_count >= 2) {
            polycall_ast_node_destroy(child->children[1]);
            child->children[1] = NULL;
          }

          // Keep the else block, if present
          if (child->child_count >= 3) {
            // Process the else block
            process_conditionals(parser, child->children[2]);

            // Move the else block to the second position
            child->children[1] = child->children[2];
            child->children[2] = NULL;
            child->child_count = 2;
          } else {
            // No else block, remove the if node entirely
            polycall_ast_node_destroy(child);

            // Remove from parent
            for (int j = i; j < node->child_count - 1; j++) {
              node->children[j] = node->children[j + 1];
            }

            node->child_count--;
            i--; // Adjust index after removal
          }
        }
      }
    } else if (child->type == NODE_DIRECTIVE_FOR) {
      // Process for loops (placeholder)
      // In a real implementation, this would expand the loop
      // and create copies of the loop body for each iteration
      printf("For loop encountered (not yet implemented)\n");
    } else {
      // Recursively process other nodes
      process_conditionals(parser, child);
    }
  }

  return true;
}

/**
 * @brief Parse a configuration file
 *
 * @param filename The configuration file path
 * @return polycall_config_t* The parsed configuration
 */
polycall_config_t *polycall_parse_config_file(const char *filename) {
  // Create parser context
  polycall_config_parser_t *parser =
      (polycall_config_parser_t *)malloc(sizeof(polycall_config_parser_t));
  if (!parser) {
    return NULL;
  }

  // Initialize parser components
  if (!initialize_parser_components(parser)) {
    polycall_config_parser_destroy(parser);
    return NULL;
  }

  // Open the file
  FILE *file = fopen(filename, "r");
  if (!file) {
    polycall_config_parser_destroy(parser);
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Read the file contents
  char *source = (char *)malloc(file_size + 1);
  if (!source) {
    fclose(file);
    polycall_config_parser_destroy(parser);
    return NULL;
  }

  size_t bytes_read = fread(source, 1, file_size, file);
  source[bytes_read] = '\0';

  fclose(file);

  // Parse the source
  polycall_config_t *config = polycall_parse_config_string(parser, source);

  // Clean up
  free(source);
  polycall_config_parser_destroy(parser);

  return config;
}

/**
 * @brief Parse a configuration string
 *
 * @param parser The configuration parser
 * @param source The configuration string
 * @return polycall_config_t* The parsed configuration
 */
polycall_config_t *
polycall_parse_config_string(polycall_config_parser_t *parser,
                             const char *source) {
  if (!parser || !source) {
    return NULL;
  }

  // Create tokenizer
  polycall_tokenizer_t *tokenizer = polycall_tokenizer_create(source);
  if (!tokenizer) {
    return NULL;
  }

  // Create parser
  polycall_parser_t *syntax_parser = polycall_parser_create(tokenizer);
  if (!syntax_parser) {
    polycall_tokenizer_destroy(tokenizer);
    return NULL;
  }

  // Parse tokens into AST
  polycall_ast_t *ast = polycall_parser_parse(syntax_parser);

  // Check for errors
  bool had_error = syntax_parser->had_error;

  // Clean up syntax parser and tokenizer
  polycall_parser_destroy(syntax_parser);
  polycall_tokenizer_destroy(tokenizer);

  if (had_error || !ast) {
    if (ast) {
      polycall_ast_destroy(ast);
    }
    return NULL;
  }

  // Process directives
  if (!process_directives(parser, ast)) {
    polycall_ast_destroy(ast);
    return NULL;
  }

  // Convert AST to configuration
  polycall_config_t *config = polycall_ast_to_config(ast);

  // Clean up
  polycall_ast_destroy(ast);

  return config;
}

/**
 * @brief Destroy a configuration parser
 *
 * @param parser The parser to destroy
 */
void polycall_config_parser_destroy(polycall_config_parser_t *parser) {
  if (!parser) {
    return;
  }

  if (parser->macro_expander) {
    polycall_macro_expander_destroy(parser->macro_expander);
  }

  if (parser->expression_evaluator) {
    polycall_expression_evaluator_destroy(parser->expression_evaluator);
  }

  free(parser);
}
