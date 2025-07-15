/**
 * @file parser.h
 * @brief Parser definitions for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 */

#ifndef POLYCALL_CONFIG_POLYCALLFILE_PARSER_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_PARSER_H_H

#include "ast.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Parser structure
 */
typedef struct {
  polycall_tokenizer_t *tokenizer;  // Tokenizer
  polycall_token_t *current_token;  // Current token
  polycall_token_t *previous_token; // Previous token
  bool had_error;                   // Whether an error occurred
  bool panic_mode; // Whether in panic mode (skip to synchronization point)
} polycall_parser_t;

/**
 * @brief Create a new parser
 *
 * @param tokenizer The tokenizer to use
 * @return polycall_parser_t* The created parser
 */
polycall_parser_t *polycall_parser_create(polycall_tokenizer_t *tokenizer);

/**
 * @brief Free a parser
 *
 * @param parser The parser to free
 */
void polycall_parser_destroy(polycall_parser_t *parser);

/**
 * @brief Parse tokens into an AST
 *
 * @param parser The parser
 * @return polycall_ast_t* The parsed AST
 */
polycall_ast_t *polycall_parser_parse(polycall_parser_t *parser);

#endif /* POLYCALL_CONFIG_POLYCALLFILE_PARSER_H_H */