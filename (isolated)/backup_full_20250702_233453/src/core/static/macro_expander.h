/**
 * @file macro_expander.h
 * @brief Macro expansion system for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's architecture design
 *
 * This file defines the interfaces for macro registration, resolution, and expansion
 * within the configuration parser.
 */

 #ifndef POLYCALL_CONFIG_POLYCALLFILE_MACRO_EXPANDER_H_H
 #define POLYCALL_CONFIG_POLYCALLFILE_MACRO_EXPANDER_H_H
 
 #include "ast.h"
 #include <stdbool.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 /**
  * @brief Macro parameter structure
  */
 typedef struct {
     char* name;          // Parameter name
     char* default_value; // Default value (can be NULL)
 } polycall_macro_param_t;
 
 /**
  * @brief Macro definition structure
  */
 typedef struct {
     char* name;                      // Macro name
     polycall_ast_node_t* expansion;  // Macro expansion (AST node or subtree)
     polycall_macro_param_t* params;  // Parameter list (for parameterized macros)
     int param_count;                 // Number of parameters
     int param_capacity;              // Parameter capacity
     bool is_parameterized;           // Whether the macro is parameterized
 } polycall_macro_def_t;
 
 /**
  * @brief Macro expander context
  */
 typedef struct {
     polycall_macro_def_t* macros;    // Macro definitions
     int macro_count;                 // Number of macros
     int macro_capacity;              // Macro capacity
     
     // Scope management
     struct {
         int global_scope_end;        // Index marking the end of the global scope
         bool track_scopes;           // Whether to track scopes
     } scope;
 } polycall_macro_expander_t;
 
 /**
  * @brief Initialize a macro expander
  * 
  * @return polycall_macro_expander_t* The created macro expander
  */
 polycall_macro_expander_t* polycall_macro_expander_create(void);
 
 /**
  * @brief Destroy a macro expander
  * 
  * @param expander The macro expander to destroy
  */
 void polycall_macro_expander_destroy(polycall_macro_expander_t* expander);
 
 /**
  * @brief Register a simple macro (direct substitution)
  * 
  * @param expander The macro expander
  * @param name The macro name
  * @param value The macro value as a string (will be parsed)
  * @return bool True if successful, false otherwise
  */
 bool polycall_macro_register(polycall_macro_expander_t* expander, 
                             const char* name, const char* value);
 
 /**
  * @brief Register a parameterized macro
  * 
  * @param expander The macro expander
  * @param name The macro name
  * @param pattern The macro pattern
  * @param param_names Array of parameter names
  * @param param_count Number of parameters
  * @return bool True if successful, false otherwise
  */
 bool polycall_macro_register_parameterized(polycall_macro_expander_t* expander,
                                           const char* name, const char* pattern,
                                           const char** param_names, int param_count);
 
 /**
  * @brief Find a macro by name
  * 
  * @param expander The macro expander
  * @param name The macro name
  * @return polycall_macro_def_t* The found macro, or NULL if not found
  */
 polycall_macro_def_t* polycall_macro_find(polycall_macro_expander_t* expander, const char* name);
 
 /**
  * @brief Enter a new scope
  * 
  * @param expander The macro expander
  */
 void polycall_macro_enter_scope(polycall_macro_expander_t* expander);
 
 /**
  * @brief Exit the current scope
  * 
  * @param expander The macro expander
  */
 void polycall_macro_exit_scope(polycall_macro_expander_t* expander);
 
 /**
  * @brief Apply macro expansion to an AST
  * 
  * @param expander The macro expander
  * @param node The AST node to process
  * @return polycall_ast_node_t* The expanded AST node
  */
 polycall_ast_node_t* polycall_macro_expand_node(polycall_macro_expander_t* expander, 
                                               polycall_ast_node_t* node);
 
 /**
  * @brief Expand all macros in an AST
  * 
  * @param expander The macro expander
  * @param ast The AST to process
  * @return bool True if successful, false otherwise
  */
 bool polycall_macro_expand_ast(polycall_macro_expander_t* expander, polycall_ast_t* ast);
 
 #endif /* POLYCALL_CONFIG_POLYCALLFILE_MACRO_EXPANDER_H_H */