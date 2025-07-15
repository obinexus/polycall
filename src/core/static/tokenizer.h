/**
 * @file tokenizer.h
 * @brief Tokenizer definitions for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 */

#ifndef POLYCALL_CONFIG_POLYCALLFILE_TOKENIZER_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_TOKENIZER_H_H

#include "token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Tokenizer structure
 */
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Tokenizer structure
 */
typedef struct {
  const char *source; // Source text
  int length;         // Source length
  int start;          // Start of current lexeme
  int current;        // Current position in source
  int line;           // Current line
  int column;         // Current column
} polycall_tokenizer_t;

/**
 * @brief Initialize a tokenizer with source text
 *
 * @param source The source text to tokenize
 * @return polycall_tokenizer_t* The created tokenizer
 */
polycall_tokenizer_t *polycall_tokenizer_create(const char *source);

/**
 * @brief Free a tokenizer
 *
 * @param tokenizer The tokenizer to free
 */
void polycall_tokenizer_destroy(polycall_tokenizer_t *tokenizer);

/**
 * @brief Get the next token from the source
 *
 * @param tokenizer The tokenizer
 * @return polycall_token_t* The next token
 */
polycall_token_t *
polycall_tokenizer_next_token(polycall_tokenizer_t *tokenizer);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CONFIG_POLYCALLFILE_TOKENIZER_H_H */