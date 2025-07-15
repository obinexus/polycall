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
#include "polycall/core/config/polycallfile/tokenizer.h"

 * @file tokenizer.c
 * @brief Tokenizer implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file implements a tokenizer (lexical analyzer) for the configuration
 * language, breaking down the source text into tokens for parsing.
 */

 #include "polycall/core/config/polycallfile/tokenizer.h"

 
 /**
  * @brief Initialize a tokenizer with source text
  * 
  * @param source The source text to tokenize
  * @return polycall_tokenizer_t* The created tokenizer
  */
 polycall_tokenizer_t* polycall_tokenizer_create(const char* source) {
     if (!source) {
         return NULL;
     }
     
     polycall_tokenizer_t* tokenizer = (polycall_tokenizer_t*)malloc(sizeof(polycall_tokenizer_t));
     if (!tokenizer) {
         return NULL;
     }
     
     tokenizer->source = source;
     tokenizer->length = strlen(source);
     tokenizer->start = 0;
     tokenizer->current = 0;
     tokenizer->line = 1;
     tokenizer->column = 1;
     
     return tokenizer;
 }
 
 /**
  * @brief Free a tokenizer
  * 
  * @param tokenizer The tokenizer to free
  */
 void polycall_tokenizer_destroy(polycall_tokenizer_t* tokenizer) {
     if (tokenizer) {
         free(tokenizer);
     }
 }
 
 /**
  * @brief Check if we've reached the end of the source
  * 
  * @param tokenizer The tokenizer
  * @return bool True if at end, false otherwise
  */
 static bool is_at_end(polycall_tokenizer_t* tokenizer) {
     return tokenizer->current >= tokenizer->length;
 }
 
 /**
  * @brief Peek at the current character without advancing
  * 
  * @param tokenizer The tokenizer
  * @return char The current character
  */
 static char peek(polycall_tokenizer_t* tokenizer) {
     if (is_at_end(tokenizer)) {
         return '\0';
     }
     return tokenizer->source[tokenizer->current];
 }
 
 /**
  * @brief Peek at the next character without advancing
  * 
  * @param tokenizer The tokenizer
  * @return char The next character
  */
 static char peek_next(polycall_tokenizer_t* tokenizer) {
     if (tokenizer->current + 1 >= tokenizer->length) {
         return '\0';
     }
     return tokenizer->source[tokenizer->current + 1];
 }
 
 /**
  * @brief Advance to the next character
  * 
  * @param tokenizer The tokenizer
  * @return char The consumed character
  */
 static char advance(polycall_tokenizer_t* tokenizer) {
     char c = tokenizer->source[tokenizer->current++];
     tokenizer->column++;
     
     if (c == '\n') {
         tokenizer->line++;
         tokenizer->column = 1;
     }
     
     return c;
 }
 
 /**
  * @brief Check if the current character matches the expected one, and advance if so
  * 
  * @param tokenizer The tokenizer
  * @param expected The expected character
  * @return bool True if matched, false otherwise
  */
 static bool match(polycall_tokenizer_t* tokenizer, char expected) {
     if (is_at_end(tokenizer) || tokenizer->source[tokenizer->current] != expected) {
         return false;
     }
     
     tokenizer->current++;
     tokenizer->column++;
     return true;
 }
 
 /**
  * @brief Create a token
  * 
  * @param tokenizer The tokenizer
  * @param type The token type
  * @return polycall_token_t* The created token
  */
 static polycall_token_t* make_token(polycall_tokenizer_t* tokenizer, polycall_token_type_t type) {
     size_t length = tokenizer->current - tokenizer->start;
     char* lexeme = (char*)malloc(length + 1);
     if (!lexeme) {
         return NULL;
     }
     
     strncpy(lexeme, &tokenizer->source[tokenizer->start], length);
     lexeme[length] = '\0';
     
     polycall_token_t* token = polycall_token_create(
         type, 
         lexeme, 
         tokenizer->line, 
         tokenizer->column - length
     );
     
     free(lexeme);
     return token;
 }
 
 /**
  * @brief Create an error token
  * 
  * @param tokenizer The tokenizer
  * @param message The error message
  * @return polycall_token_t* The created error token
  */
 static polycall_token_t* error_token(polycall_tokenizer_t* tokenizer, const char* message) {
     return polycall_token_create(
         TOKEN_ERROR, 
         message, 
         tokenizer->line, 
         tokenizer->column - (tokenizer->current - tokenizer->start)
     );
 }
 
 /**
  * @brief Skip whitespace characters
  * 
  * @param tokenizer The tokenizer
  */
 static void skip_whitespace(polycall_tokenizer_t* tokenizer) {
     while (1) {
         char c = peek(tokenizer);
         
         switch (c) {
             case ' ':
             case '\t':
             case '\r':
                 advance(tokenizer);
                 break;
                 
             case '\n':
                 advance(tokenizer);
                 break;
                 
             default:
                 return;
         }
     }
 }
 
 /**
  * @brief Process a comment
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The comment token
  */
 static polycall_token_t* handle_comment(polycall_tokenizer_t* tokenizer) {
     // Skip the '#'
     advance(tokenizer);
     
     // Read until the end of the line
     while (peek(tokenizer) != '\n' && !is_at_end(tokenizer)) {
         advance(tokenizer);
     }
     
     return make_token(tokenizer, TOKEN_COMMENT);
 }
 
 /**
  * @brief Process a string literal
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The string token
  */
 static polycall_token_t* handle_string(polycall_tokenizer_t* tokenizer) {
     // Skip the opening quote
     char quote = advance(tokenizer);
     tokenizer->start = tokenizer->current;
     
     // Read until the closing quote
     while (peek(tokenizer) != quote && !is_at_end(tokenizer)) {
         if (peek(tokenizer) == '\\' && peek_next(tokenizer) == quote) {
             // Handle escaped quote
             advance(tokenizer);
         }
         advance(tokenizer);
     }
     
     // Unterminated string
     if (is_at_end(tokenizer)) {
         return error_token(tokenizer, "Unterminated string");
     }
     
     // Create the token
     size_t length = tokenizer->current - tokenizer->start;
     char* string_content = (char*)malloc(length + 1);
     if (!string_content) {
         return error_token(tokenizer, "Memory allocation failed");
     }
     
     strncpy(string_content, &tokenizer->source[tokenizer->start], length);
     string_content[length] = '\0';
     
     // Skip the closing quote
     advance(tokenizer);
     
     polycall_token_t* token = polycall_token_create(
         TOKEN_STRING, 
         string_content, 
         tokenizer->line, 
         tokenizer->column - length - 2  // -2 for quotes
     );
     
     free(string_content);
     return token;
 }
 
 /**
  * @brief Check if a character is a valid identifier start
  * 
  * @param c The character
  * @return bool True if valid, false otherwise
  */
 static bool is_alpha(char c) {
     return (c >= 'a' && c <= 'z') || 
            (c >= 'A' && c <= 'Z') || 
            c == '_';
 }
 
 /**
  * @brief Check if a character is a valid identifier character
  * 
  * @param c The character
  * @return bool True if valid, false otherwise
  */
 static bool is_alpha_numeric(char c) {
     return is_alpha(c) || (c >= '0' && c <= '9');
 }
 
 /**
  * @brief Process an identifier or keyword
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The identifier or keyword token
  */
 static polycall_token_t* handle_identifier(polycall_tokenizer_t* tokenizer) {
     while (is_alpha_numeric(peek(tokenizer))) {
         advance(tokenizer);
     }
     
     // Check for keywords
     size_t length = tokenizer->current - tokenizer->start;
     
     if (length == 4 && strncmp(&tokenizer->source[tokenizer->start], "true", 4) == 0) {
         return make_token(tokenizer, TOKEN_TRUE);
     }
     else if (length == 5 && strncmp(&tokenizer->source[tokenizer->start], "false", 5) == 0) {
         return make_token(tokenizer, TOKEN_FALSE);
     }
     else if (length == 4 && strncmp(&tokenizer->source[tokenizer->start], "null", 4) == 0) {
         return make_token(tokenizer, TOKEN_NULL);
     }
     
     return make_token(tokenizer, TOKEN_IDENTIFIER);
 }
 
 /**
  * @brief Process a number literal
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The number token
  */
 static polycall_token_t* handle_number(polycall_tokenizer_t* tokenizer) {
     // Read the integer part
     while (isdigit(peek(tokenizer))) {
         advance(tokenizer);
     }
     
     // Look for a decimal point
     if (peek(tokenizer) == '.' && isdigit(peek_next(tokenizer))) {
         // Consume the '.'
         advance(tokenizer);
         
         // Read the fractional part
         while (isdigit(peek(tokenizer))) {
             advance(tokenizer);
         }
     }
     
     // Check for unit suffixes (MB, GB, ms, etc.)
     if (is_alpha(peek(tokenizer))) {
         const char* suffix_start = &tokenizer->source[tokenizer->current];
         
         // Read the first character
         advance(tokenizer);
         
         // Allow a second character for common suffixes
         if (is_alpha(peek(tokenizer))) {
             advance(tokenizer);
         }
         
         // Allow a third character for common suffixes (KiB, MiB, etc.)
         if (is_alpha(peek(tokenizer))) {
             advance(tokenizer);
         }
     }
     
     return make_token(tokenizer, TOKEN_NUMBER);
 }
 
 /**
  * @brief Process a directive (like @define or @import)
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The directive token
  */
 static polycall_token_t* handle_directive(polycall_tokenizer_t* tokenizer) {
     // Skip the '@'
     advance(tokenizer);
     tokenizer->start = tokenizer->current;
     
     // Read the directive name (like "define", "import", etc.)
     while (is_alpha_numeric(peek(tokenizer))) {
         advance(tokenizer);
     }
     
     return make_token(tokenizer, TOKEN_DIRECTIVE);
 }
 
 /**
  * @brief Get the next token from the source
  * 
  * @param tokenizer The tokenizer
  * @return polycall_token_t* The next token
  */
 polycall_token_t* polycall_tokenizer_next_token(polycall_tokenizer_t* tokenizer) {
     if (!tokenizer) {
         return NULL;
     }
     
     // Skip whitespace
     skip_whitespace(tokenizer);
     
     // Mark the start of the token
     tokenizer->start = tokenizer->current;
     
     // Check for EOF
     if (is_at_end(tokenizer)) {
         return polycall_token_create(TOKEN_EOF, "", tokenizer->line, tokenizer->column);
     }
     
     // Get the next character
     char c = advance(tokenizer);
     
     // Identify the token type
     switch (c) {
         // Single-character tokens
         case '{': return make_token(tokenizer, TOKEN_LEFT_BRACE);
         case '}': return make_token(tokenizer, TOKEN_RIGHT_BRACE);
         case '[': return make_token(tokenizer, TOKEN_LEFT_BRACKET);
         case ']': return make_token(tokenizer, TOKEN_RIGHT_BRACKET);
         case ',': return make_token(tokenizer, TOKEN_COMMA);
         case '.': return make_token(tokenizer, TOKEN_DOT);
         case ';': return make_token(tokenizer, TOKEN_SEMICOLON);
         case '=': return make_token(tokenizer, TOKEN_EQUALS);
         
         // Comments
         case '#': return handle_comment(tokenizer);
         
         // Strings
         case '"':
         case '\'': return handle_string(tokenizer);
         
         // Directives
         case '@': return handle_directive(tokenizer);
         
         // Other characters
         default:
             if (isdigit(c)) {
                 return handle_number(tokenizer);
             }
             else if (is_alpha(c)) {
                 return handle_identifier(tokenizer);
             }
             else {
                 return error_token(tokenizer, "Unexpected character");
             }
     }
 }
 
 /**
  * @brief Tokenize the entire source and return an array of tokens
  * 
  * @param tokenizer The tokenizer
  * @param token_count Pointer to receive the number of tokens
  * @return polycall_token_t** Array of tokens
  */
 polycall_token_t** polycall_tokenizer_tokenize_all(polycall_tokenizer_t* tokenizer, size_t* token_count) {
     if (!tokenizer || !token_count) {
         return NULL;
     }
     
     // Initial capacity
     size_t capacity = 16;
     size_t count = 0;
     
     polycall_token_t** tokens = (polycall_token_t**)malloc(capacity * sizeof(polycall_token_t*));
     if (!tokens) {
         *token_count = 0;
         return NULL;
     }
     
     // Tokenize until EOF
     while (1) {
         polycall_token_t* token = polycall_tokenizer_next_token(tokenizer);
         
         if (!token) {
             // Error during tokenization
             for (size_t i = 0; i < count; i++) {
                 polycall_token_destroy(tokens[i]);
             }
             free(tokens);
             *token_count = 0;
             return NULL;
         }
         
         // Check if we need to expand the array
         if (count == capacity) {
             capacity *= 2;
             polycall_token_t** new_tokens = (polycall_token_t**)realloc(
                 tokens, capacity * sizeof(polycall_token_t*)
             );
             
             if (!new_tokens) {
                 // Allocation failure
                 for (size_t i = 0; i < count; i++) {
                     polycall_token_destroy(tokens[i]);
                 }
                 free(tokens);
                 polycall_token_destroy(token);
                 *token_count = 0;
                 return NULL;
             }
             
             tokens = new_tokens;
         }
         
         // Add the token to the array
         tokens[count++] = token;
         
         // Stop at EOF
         if (token->type == TOKEN_EOF) {
             break;
         }
     }
     
     *token_count = count;
     return tokens;
 }
