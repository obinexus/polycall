/**
#include "polycall/core/config/polycallfile/parser.h"

 * @file parser.c
 * @brief Parser implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file implements the recursive descent parser for the configuration
 * language, building an AST from tokens.
 */

 #include "polycall/core/config/polycallfile/parser.h"

 /**
  * @brief Create a new parser
  * 
  * @param tokenizer The tokenizer to use
  * @return polycall_parser_t* The created parser
  */
 polycall_parser_t* polycall_parser_create(polycall_tokenizer_t* tokenizer) {
     if (!tokenizer) {
         return NULL;
     }
     
     polycall_parser_t* parser = (polycall_parser_t*)malloc(sizeof(polycall_parser_t));
     if (!parser) {
         return NULL;
     }
     
     parser->tokenizer = tokenizer;
     parser->current_token = NULL;
     parser->previous_token = NULL;
     parser->had_error = false;
     parser->panic_mode = false;
     
     return parser;
 }
 
 /**
  * @brief Free a parser
  * 
  * @param parser The parser to free
  */
 void polycall_parser_destroy(polycall_parser_t* parser) {
     if (!parser) {
         return;
     }
     
     // Free the current token if it exists
     if (parser->current_token) {
         polycall_token_destroy(parser->current_token);
     }
     
     // Free the previous token if it exists
     if (parser->previous_token) {
         polycall_token_destroy(parser->previous_token);
     }
     
     free(parser);
 }
 
 /**
  * @brief Advance to the next token
  * 
  * @param parser The parser
  * @return polycall_token_t* The consumed token
  */
 static polycall_token_t* advance(polycall_parser_t* parser) {
     if (parser->current_token) {
         // Free the previous token if it exists
         if (parser->previous_token) {
             polycall_token_destroy(parser->previous_token);
         }
         
         // Move the current token to previous
         parser->previous_token = parser->current_token;
         parser->current_token = NULL;
     }
     
     // Get the next token from the tokenizer
     parser->current_token = polycall_tokenizer_next_token(parser->tokenizer);
     return parser->previous_token;
 }
 
 /**
  * @brief Check if the current token has the expected type
  * 
  * @param parser The parser
  * @param type The expected token type
  * @return bool True if the current token has the expected type, false otherwise
  */
 static bool check(polycall_parser_t* parser, polycall_token_type_t type) {
     if (!parser->current_token) {
         advance(parser);
     }
     
     return parser->current_token && parser->current_token->type == type;
 }
 
 /**
  * @brief Consume a token if it has the expected type
  * 
  * @param parser The parser
  * @param type The expected token type
  * @param error_message The error message to report if the token doesn't match
  * @return bool True if the token was consumed, false otherwise
  */
 static bool consume(polycall_parser_t* parser, polycall_token_type_t type, const char* error_message) {
     if (check(parser, type)) {
         advance(parser);
         return true;
     }
     
     // Error: unexpected token
     parser->had_error = true;
     if (!parser->panic_mode) {
         fprintf(stderr, "Parse error at line %d, column %d: %s\n", 
                 parser->current_token ? parser->current_token->line : 0,
                 parser->current_token ? parser->current_token->column : 0,
                 error_message);
     }
     
     return false;
 }
 
 /**
  * @brief Synchronize after an error
  * 
  * @param parser The parser
  */
 static void synchronize(polycall_parser_t* parser) {
     parser->panic_mode = false;
     
     while (parser->current_token && parser->current_token->type != TOKEN_EOF) {
         // Synchronize at statement boundaries
         if (parser->previous_token && parser->previous_token->type == TOKEN_SEMICOLON) {
             return;
         }
         
         switch (parser->current_token->type) {
             case TOKEN_LEFT_BRACE:
             case TOKEN_RIGHT_BRACE:
                 return;
             default:
                 break;
         }
         
         advance(parser);
     }
 }
 
 /**
  * @brief Report an error
  * 
  * @param parser The parser
  * @param message The error message
  */
 static void error(polycall_parser_t* parser, const char* message) {
     if (!parser->panic_mode) {
         parser->had_error = true;
         parser->panic_mode = true;
         
         fprintf(stderr, "Parse error at line %d, column %d: %s\n", 
                 parser->current_token ? parser->current_token->line : 0,
                 parser->current_token ? parser->current_token->column : 0,
                 message);
     }
 }
 
 /**
  * @brief Parse a value (string, number, boolean, null, array)
  * 
  * @param parser The parser
  * @return polycall_ast_node_t* The parsed value node, or NULL on error
  */
 static polycall_ast_node_t* parse_value(polycall_parser_t* parser) {
     if (!parser->current_token) {
         advance(parser);
     }
     
     if (!parser->current_token) {
         error(parser, "Unexpected end of file");
         return NULL;
     }
     
     polycall_ast_node_t* value_node = NULL;
     
     switch (parser->current_token->type) {
         case TOKEN_STRING:
             value_node = polycall_ast_node_create(NODE_VALUE_STRING, parser->current_token->lexeme);
             advance(parser);
             break;
             
         case TOKEN_NUMBER:
             value_node = polycall_ast_node_create(NODE_VALUE_NUMBER, parser->current_token->lexeme);
             advance(parser);
             break;
             
         case TOKEN_TRUE:
             value_node = polycall_ast_node_create(NODE_VALUE_BOOLEAN, "true");
             advance(parser);
             break;
             
         case TOKEN_FALSE:
             value_node = polycall_ast_node_create(NODE_VALUE_BOOLEAN, "false");
             advance(parser);
             break;
             
         case TOKEN_NULL:
             value_node = polycall_ast_node_create(NODE_VALUE_NULL, "null");
             advance(parser);
             break;
             
         case TOKEN_LEFT_BRACKET: {
             // Parse an array
             advance(parser);  // Consume '['
             
             value_node = polycall_ast_node_create(NODE_VALUE_ARRAY, "array");
             if (!value_node) {
                 error(parser, "Failed to create array node");
                 return NULL;
             }
             
             // Parse array elements
             if (!check(parser, TOKEN_RIGHT_BRACKET)) {
                 do {
                     polycall_ast_node_t* element = parse_value(parser);
                     if (!element) {
                         polycall_ast_node_destroy(value_node);
                         return NULL;
                     }
                     
                     if (!polycall_ast_node_add_child(value_node, element)) {
                         error(parser, "Failed to add array element");
                         polycall_ast_node_destroy(element);
                         polycall_ast_node_destroy(value_node);
                         return NULL;
                     }
                     
                 } while (check(parser, TOKEN_COMMA) && advance(parser));
             }
             
             if (!consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after array elements")) {
                 polycall_ast_node_destroy(value_node);
                 return NULL;
             }
             
             break;
         }
             
         default:
             error(parser, "Expected value");
             return NULL;
     }
     
     return value_node;
 }
 
 /**
  * @brief Parse a statement (name = value)
  * 
  * @param parser The parser
  * @return polycall_ast_node_t* The parsed statement node, or NULL on error
  */
 static polycall_ast_node_t* parse_statement(polycall_parser_t* parser) {
     if (!parser->current_token) {
         advance(parser);
     }
     
     // Statement must start with an identifier
     if (!check(parser, TOKEN_IDENTIFIER)) {
         error(parser, "Expected identifier at start of statement");
         return NULL;
     }
     
     // Create statement node
     polycall_ast_node_t* statement_node = polycall_ast_node_create(
         NODE_STATEMENT, 
         parser->current_token->lexeme
     );
     
     if (!statement_node) {
         error(parser, "Failed to create statement node");
         return NULL;
     }
     
     advance(parser);  // Consume identifier
     
     // Expect equals sign
     if (!consume(parser, TOKEN_EQUALS, "Expected '=' after identifier")) {
         polycall_ast_node_destroy(statement_node);
         return NULL;
     }
     
     // Parse the value
     polycall_ast_node_t* value_node = parse_value(parser);
     if (!value_node) {
         polycall_ast_node_destroy(statement_node);
         return NULL;
     }
     
     // Add value as child of statement
     if (!polycall_ast_node_add_child(statement_node, value_node)) {
         error(parser, "Failed to add value to statement");
         polycall_ast_node_destroy(value_node);
         polycall_ast_node_destroy(statement_node);
         return NULL;
     }
     
     // Optional semicolon
     if (check(parser, TOKEN_SEMICOLON)) {
         advance(parser);
     }
     
     return statement_node;
 }
 
 /**
  * @brief Parse a section (identifier { ... })
  * 
  * @param parser The parser
  * @return polycall_ast_node_t* The parsed section node, or NULL on error
  */
 static polycall_ast_node_t* parse_section(polycall_parser_t* parser) {
     if (!parser->current_token) {
         advance(parser);
     }
     
     // Section must start with an identifier
     if (!check(parser, TOKEN_IDENTIFIER)) {
         error(parser, "Expected identifier at start of section");
         return NULL;
     }
     
     // Create section node
     polycall_ast_node_t* section_node = polycall_ast_node_create(
         NODE_SECTION, 
         parser->current_token->lexeme
     );
     
     if (!section_node) {
         error(parser, "Failed to create section node");
         return NULL;
     }
     
     advance(parser);  // Consume identifier
     
     // Expect opening brace
     if (!consume(parser, TOKEN_LEFT_BRACE, "Expected '{' after section name")) {
         polycall_ast_node_destroy(section_node);
         return NULL;
     }
     
     // Parse section contents
     while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
         polycall_ast_node_t* child_node = NULL;
         
         // Peek at the next token to determine if this is a nested section or a statement
         if (check(parser, TOKEN_IDENTIFIER)) {
             // Look ahead for a left brace to determine if this is a nested section
             bool is_section = false;
             
             // Save the current token
             polycall_token_t* saved_token = parser->current_token;
             parser->current_token = NULL;
             
             // Get the next token (after the identifier)
             polycall_token_t* next_token = polycall_tokenizer_next_token(parser->tokenizer);
             
             if (next_token && next_token->type == TOKEN_LEFT_BRACE) {
                 is_section = true;
             }
             
             // Put the tokens back
             polycall_token_destroy(next_token);
             parser->current_token = saved_token;
             
             if (is_section) {
                 child_node = parse_section(parser);
             } else {
                 child_node = parse_statement(parser);
             }
         } else if (check(parser, TOKEN_DIRECTIVE)) {
             // Parse directive (like @define or @import)
             child_node = parse_directive(parser);
         } else {
             error(parser, "Expected identifier or directive");
             polycall_ast_node_destroy(section_node);
             return NULL;
         }
         
         if (!child_node) {
             polycall_ast_node_destroy(section_node);
             return NULL;
         }
         
         // Add the child node to the section
         if (!polycall_ast_node_add_child(section_node, child_node)) {
             error(parser, "Failed to add child to section");
             polycall_ast_node_destroy(child_node);
             polycall_ast_node_destroy(section_node);
             return NULL;
         }
     }
     
     // Expect closing brace
     if (!consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after section contents")) {
         polycall_ast_node_destroy(section_node);
         return NULL;
     }
     
     return section_node;
 }
 
 /**
  * @brief Parse a directive (like @define or @import)
  * 
  * @param parser The parser
  * @return polycall_ast_node_t* The parsed directive node, or NULL on error
  */
 static polycall_ast_node_t* parse_directive(polycall_parser_t* parser) {
     if (!parser->current_token) {
         advance(parser);
     }
     
     // Directive must start with @
     if (!check(parser, TOKEN_DIRECTIVE)) {
         error(parser, "Expected directive");
         return NULL;
     }
     
     // Create directive node
     polycall_ast_node_t* directive_node = polycall_ast_node_create(
         NODE_DIRECTIVE, 
         parser->current_token->lexeme
     );
     
     if (!directive_node) {
         error(parser, "Failed to create directive node");
         return NULL;
     }
     
     advance(parser);  // Consume directive
     
     // Parse directive arguments based on directive type
     if (strcmp(directive_node->name, "define") == 0) {
         // @define macro_name value
         
         // Expect identifier (macro name)
         if (!check(parser, TOKEN_IDENTIFIER)) {
             error(parser, "Expected identifier after @define");
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         // Create macro name node
         polycall_ast_node_t* name_node = polycall_ast_node_create(
             NODE_IDENTIFIER, 
             parser->current_token->lexeme
         );
         
         if (!name_node) {
             error(parser, "Failed to create macro name node");
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         advance(parser);  // Consume identifier
         
         // Add name as child of directive
         if (!polycall_ast_node_add_child(directive_node, name_node)) {
             error(parser, "Failed to add name to directive");
             polycall_ast_node_destroy(name_node);
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         // Parse the value
         polycall_ast_node_t* value_node = parse_value(parser);
         if (!value_node) {
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         // Add value as child of directive
         if (!polycall_ast_node_add_child(directive_node, value_node)) {
             error(parser, "Failed to add value to directive");
             polycall_ast_node_destroy(value_node);
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
     }
     else if (strcmp(directive_node->name, "import") == 0) {
         // @import "filename"
         
         // Expect string (filename)
         if (!check(parser, TOKEN_STRING)) {
             error(parser, "Expected string after @import");
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         // Create filename node
         polycall_ast_node_t* filename_node = polycall_ast_node_create(
             NODE_VALUE_STRING, 
             parser->current_token->lexeme
         );
         
         if (!filename_node) {
             error(parser, "Failed to create filename node");
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
         
         advance(parser);  // Consume string
         
         // Add filename as child of directive
         if (!polycall_ast_node_add_child(directive_node, filename_node)) {
             error(parser, "Failed to add filename to directive");
             polycall_ast_node_destroy(filename_node);
             polycall_ast_node_destroy(directive_node);
             return NULL;
         }
     }
     else {
         // Unknown directive
         error(parser, "Unknown directive");
         polycall_ast_node_destroy(directive_node);
         return NULL;
     }
     
     // Optional semicolon
     if (check(parser, TOKEN_SEMICOLON)) {
         advance(parser);
     }
     
     return directive_node;
 }
 
 /**
  * @brief Parse the configuration file
  * 
  * @param parser The parser
  * @return polycall_ast_t* The parsed AST, or NULL on error
  */
 static polycall_ast_t* parse_config(polycall_parser_t* parser) {
     // Create an AST with a root node
     polycall_ast_t* ast = polycall_ast_create();
     if (!ast) {
         error(parser, "Failed to create AST");
         return NULL;
     }
     
     // Get the first token
     if (!parser->current_token) {
         advance(parser);
     }
     
     // Parse declarations until EOF
     while (!check(parser, TOKEN_EOF)) {
         polycall_ast_node_t* node = NULL;
         
         // Skip comments
         if (check(parser, TOKEN_COMMENT)) {
             advance(parser);
             continue;
         }
         
         // Determine what kind of declaration this is
         if (check(parser, TOKEN_DIRECTIVE)) {
             node = parse_directive(parser);
         }
         else if (check(parser, TOKEN_IDENTIFIER)) {
             // Look ahead for a left brace to determine if this is a section
             bool is_section = false;
             
             // Save the current token
             polycall_token_t* saved_token = parser->current_token;
             parser->current_token = NULL;
             
             // Get the next token (after the identifier)
             polycall_token_t* next_token = polycall_tokenizer_next_token(parser->tokenizer);
             
             if (next_token && next_token->type == TOKEN_LEFT_BRACE) {
                 is_section = true;
             }
             
             // Put the tokens back
             polycall_token_destroy(next_token);
             parser->current_token = saved_token;
             
             if (is_section) {
                 node = parse_section(parser);
             } else {
                 node = parse_statement(parser);
             }
         }
         else {
             error(parser, "Expected directive, statement, or section");
             polycall_ast_destroy(ast);
             return NULL;
         }
         
         if (!node) {
             polycall_ast_destroy(ast);
             return NULL;
         }
         
         // Add the node to the root
         if (!polycall_ast_node_add_child(ast->root, node)) {
             error(parser, "Failed to add node to root");
             polycall_ast_node_destroy(node);
             polycall_ast_destroy(ast);
             return NULL;
         }
     }
     
     return ast;
 }
 
 /**
  * @brief Parse tokens into an AST
  * 
  * @param parser The parser
  * @return polycall_ast_t* The parsed AST
  */
 polycall_ast_t* polycall_parser_parse(polycall_parser_t* parser) {
     if (!parser) {
         return NULL;
     }
     
     parser->had_error = false;
     parser->panic_mode = false;
     
     return parse_config(parser);
 }