/**
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

 * @file token.h
 * @brief Token definitions for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 */

#ifndef POLYCALL_CONFIG_POLYCALLFILE_TOKEN_H_H
#define POLYCALL_CONFIG_POLYCALLFILE_TOKEN_H_H

 #include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
 /**
  * @brief Token types
  */
 typedef enum {
     TOKEN_IDENTIFIER,        // Identifier (e.g., variable name)
     TOKEN_STRING,            // String literal
     TOKEN_NUMBER,            // Numeric literal
     TOKEN_EQUALS,            // =
     TOKEN_LEFT_BRACE,        // {
     TOKEN_RIGHT_BRACE,       // }
     TOKEN_LEFT_BRACKET,      // [
     TOKEN_RIGHT_BRACKET,     // ]
     TOKEN_COMMA,             // ,
     TOKEN_DOT,               // .
     TOKEN_SEMICOLON,         // ;
     TOKEN_AT,                // @
     TOKEN_COMMENT,           // # comment
     TOKEN_DIRECTIVE,         // @define, @import, etc.
     TOKEN_TRUE,              // true
     TOKEN_FALSE,             // false
     TOKEN_NULL,              // null
     TOKEN_EOF,               // End of file
     TOKEN_ERROR              // Error token
 } polycall_token_type_t;
 
 /**
  * @brief Token structure
  */
 typedef struct {
     polycall_token_type_t type;  // Token type
     char* lexeme;                // Token lexeme (text)
     int line;                    // Line number
     int column;                  // Column number
 } polycall_token_t;
 
 /**
  * @brief Create a new token
  * 
  * @param type The token type
  * @param lexeme The token lexeme (text)
  * @param line Line number where token appears
  * @param column Column number where token appears
  * @return polycall_token_t* The created token
  */
 polycall_token_t* polycall_token_create(polycall_token_type_t type, const char* lexeme, 
                                       int line, int column);
 
 /**
  * @brief Free a token
  * 
  * @param token The token to free
  */
 void polycall_token_destroy(polycall_token_t* token);
 
 /**
  * @brief Get string representation of token type
  * 
  * @param type The token type
  * @return const char* String representation
  */
 const char* polycall_token_type_to_string(polycall_token_type_t type);

#ifdef __cplusplus
}
#endif

 #endif /* POLYCALL_CONFIG_POLYCALLFILE_TOKEN_H_H */