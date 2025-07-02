/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
#include "polycall/core/config/polycallfile/token.h"

 * @file token.c
 * @brief Token implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file defines token types and structures for the configuration parser.
 */

 #include "polycall/core/config/polycallfile/token.h"

 
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
                                       int line, int column) {
     polycall_token_t* token = (polycall_token_t*)malloc(sizeof(polycall_token_t));
     if (!token) {
         return NULL;
     }
 
     token->type = type;
     token->lexeme = strdup(lexeme);
     if (!token->lexeme) {
         free(token);
         return NULL;
     }
     token->line = line;
     token->column = column;
     return token;
 }
 
 /**
  * @brief Free a token
  * 
  * @param token The token to free
  */
 void polycall_token_destroy(polycall_token_t* token) {
     if (token) {
         if (token->lexeme) {
             free(token->lexeme);
         }
         free(token);
     }
 }
 
 /**
  * @brief Get string representation of token type
  * 
  * @param type The token type
  * @return const char* String representation
  */
 const char* polycall_token_type_to_string(polycall_token_type_t type) {
     switch (type) {
         case TOKEN_IDENTIFIER:        return "IDENTIFIER";
         case TOKEN_STRING:            return "STRING";
         case TOKEN_NUMBER:            return "NUMBER";
         case TOKEN_EQUALS:            return "EQUALS";
         case TOKEN_LEFT_BRACE:        return "LEFT_BRACE";
         case TOKEN_RIGHT_BRACE:       return "RIGHT_BRACE";
         case TOKEN_LEFT_BRACKET:      return "LEFT_BRACKET";
         case TOKEN_RIGHT_BRACKET:     return "RIGHT_BRACKET";
         case TOKEN_COMMA:             return "COMMA";
         case TOKEN_DOT:               return "DOT";
         case TOKEN_SEMICOLON:         return "SEMICOLON";
         case TOKEN_AT:                return "AT";
         case TOKEN_COMMENT:           return "COMMENT";
         case TOKEN_DIRECTIVE:         return "DIRECTIVE";
         case TOKEN_TRUE:              return "TRUE";
         case TOKEN_FALSE:             return "FALSE";
         case TOKEN_NULL:              return "NULL";
         case TOKEN_EOF:               return "EOF";
         case TOKEN_ERROR:             return "ERROR";
         default:                      return "UNKNOWN";
     }
 }
