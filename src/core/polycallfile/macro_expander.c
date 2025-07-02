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
#include "polycall/core/config/polycallfile/macro_expander.h"

 * @file macro_expander.c
 * @brief Macro expansion implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's architecture design
 *
 * This file implements the macro registration, resolution, and expansion logic.
 */

 #include "polycall/core/config/polycallfile/macro_expander.h"

 
 /**
  * @brief Create a new macro definition
  * 
  * @param name The macro name
  * @param expansion The macro expansion (AST node)
  * @param is_parameterized Whether the macro is parameterized
  * @return polycall_macro_def_t* The created macro definition
  */
 static polycall_macro_def_t* create_macro_def(const char* name, 
                                             polycall_ast_node_t* expansion,
                                             bool is_parameterized) {
     polycall_macro_def_t* def = (polycall_macro_def_t*)malloc(sizeof(polycall_macro_def_t));
     if (!def) {
         return NULL;
     }
     
     def->name = strdup(name);
     if (!def->name) {
         free(def);
         return NULL;
     }
     
     def->expansion = expansion;
     def->params = NULL;
     def->param_count = 0;
     def->param_capacity = 0;
     def->is_parameterized = is_parameterized;
     
     return def;
 }
 
 /**
  * @brief Destroy a macro definition
  * 
  * @param def The macro definition to destroy
  */
 static void destroy_macro_def(polycall_macro_def_t* def) {
     if (!def) {
         return;
     }
     
     if (def->name) {
         free(def->name);
     }
     
     if (def->expansion) {
         polycall_ast_node_destroy(def->expansion);
     }
     
     if (def->params) {
         for (int i = 0; i < def->param_count; i++) {
             if (def->params[i].name) {
                 free(def->params[i].name);
             }
             if (def->params[i].default_value) {
                 free(def->params[i].default_value);
             }
         }
         free(def->params);
     }
     
     free(def);
 }
 
 /**
  * @brief Initialize a macro expander
  * 
  * @return polycall_macro_expander_t* The created macro expander
  */
 polycall_macro_expander_t* polycall_macro_expander_create(void) {
     polycall_macro_expander_t* expander = 
         (polycall_macro_expander_t*)malloc(sizeof(polycall_macro_expander_t));
     if (!expander) {
         return NULL;
     }
     
     expander->macros = NULL;
     expander->macro_count = 0;
     expander->macro_capacity = 0;
     expander->scope.global_scope_end = 0;
     expander->scope.track_scopes = false;
     
     return expander;
 }
 
 /**
  * @brief Destroy a macro expander
  * 
  * @param expander The macro expander to destroy
  */
 void polycall_macro_expander_destroy(polycall_macro_expander_t* expander) {
     if (!expander) {
         return;
     }
     
     if (expander->macros) {
         for (int i = 0; i < expander->macro_count; i++) {
             destroy_macro_def(&expander->macros[i]);
         }
         free(expander->macros);
     }
     
     free(expander);
 }
 
 /**
  * @brief Ensure capacity for additional macros
  * 
  * @param expander The macro expander
  * @return bool True if successful, false otherwise
  */
 static bool ensure_macro_capacity(polycall_macro_expander_t* expander) {
     if (expander->macro_count == expander->macro_capacity) {
         int new_capacity = expander->macro_capacity == 0 ? 8 : expander->macro_capacity * 2;
         polycall_macro_def_t* new_macros = 
             (polycall_macro_def_t*)realloc(expander->macros, 
                                          new_capacity * sizeof(polycall_macro_def_t));
         
         if (!new_macros) {
             return false;
         }
         
         expander->macros = new_macros;
         expander->macro_capacity = new_capacity;
     }
     
     return true;
 }
 
 /**
  * @brief Create a simple value node for a macro
  * 
  * @param value The string value
  * @return polycall_ast_node_t* The created node
  */
 static polycall_ast_node_t* create_value_node(const char* value) {
     // Simple heuristic to determine value type
     if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
         return polycall_ast_node_create(NODE_VALUE_BOOLEAN, value);
     } else if (value[0] >= '0' && value[0] <= '9') {
         return polycall_ast_node_create(NODE_VALUE_NUMBER, value);
     } else {
         return polycall_ast_node_create(NODE_VALUE_STRING, value);
     }
 }
 
 /**
  * @brief Register a simple macro (direct substitution)
  * 
  * @param expander The macro expander
  * @param name The macro name
  * @param value The macro value as a string (will be parsed)
  * @return bool True if successful, false otherwise
  */
 bool polycall_macro_register(polycall_macro_expander_t* expander, 
                             const char* name, const char* value) {
     if (!expander || !name || !value) {
         return false;
     }
     
     // Check if macro already exists
     if (polycall_macro_find(expander, name)) {
         return false;  // Already exists
     }
     
     // Create a simple AST node for the value
     polycall_ast_node_t* expansion = create_value_node(value);
     if (!expansion) {
         return false;
     }
     
     // Ensure capacity
     if (!ensure_macro_capacity(expander)) {
         polycall_ast_node_destroy(expansion);
         return false;
     }
     
     // Create the macro definition
     polycall_macro_def_t* def = create_macro_def(name, expansion, false);
     if (!def) {
         polycall_ast_node_destroy(expansion);
         return false;
     }
     
     // Add the macro to the expander
     expander->macros[expander->macro_count++] = *def;
     
     // Update global scope end if tracking scopes
     if (expander->scope.track_scopes) {
         expander->scope.global_scope_end = expander->macro_count;
     }
     
     free(def);  // We've copied its contents, so we can free the struct
     
     return true;
 }
 
 /**
  * @brief Parse a parameterized macro pattern into an AST
  * 
  * Note: In a complete implementation, this would use the parser
  * to properly parse the pattern. For simplicity, we'll create a
  * basic node structure directly.
  * 
  * @param pattern The macro pattern
  * @return polycall_ast_node_t* The created AST node
  */
 static polycall_ast_node_t* parse_macro_pattern(const char* pattern) {
     // For simplicity, just create a string node
     // In a real implementation, this would be parsed into a proper AST
     return polycall_ast_node_create(NODE_VALUE_STRING, pattern);
 }
 
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
                                           const char** param_names, int param_count) {
     if (!expander || !name || !pattern || (param_count > 0 && !param_names)) {
         return false;
     }
     
     // Check if macro already exists
     if (polycall_macro_find(expander, name)) {
         return false;  // Already exists
     }
     
     // Parse the pattern into an AST
     polycall_ast_node_t* expansion = parse_macro_pattern(pattern);
     if (!expansion) {
         return false;
     }
     
     // Ensure capacity
     if (!ensure_macro_capacity(expander)) {
         polycall_ast_node_destroy(expansion);
         return false;
     }
     
     // Create the macro definition
     polycall_macro_def_t* def = create_macro_def(name, expansion, true);
     if (!def) {
         polycall_ast_node_destroy(expansion);
         return false;
     }
     
     // Allocate parameters
     def->params = (polycall_macro_param_t*)malloc(param_count * sizeof(polycall_macro_param_t));
     if (!def->params) {
         destroy_macro_def(def);
         return false;
     }
     
     // Initialize parameters
     def->param_count = param_count;
     def->param_capacity = param_count;
     
     for (int i = 0; i < param_count; i++) {
         def->params[i].name = strdup(param_names[i]);
         def->params[i].default_value = NULL;  // No default values for now
         
         if (!def->params[i].name) {
             destroy_macro_def(def);
             return false;
         }
     }
     
     // Add the macro to the expander
     expander->macros[expander->macro_count++] = *def;
     
     // Update global scope end if tracking scopes
     if (expander->scope.track_scopes) {
         expander->scope.global_scope_end = expander->macro_count;
     }
     
     free(def);  // We've copied its contents, so we can free the struct
     
     return true;
 }
 
 /**
  * @brief Find a macro by name
  * 
  * @param expander The macro expander
  * @param name The macro name
  * @return polycall_macro_def_t* The found macro, or NULL if not found
  */
 polycall_macro_def_t* polycall_macro_find(polycall_macro_expander_t* expander, const char* name) {
     if (!expander || !name) {
         return NULL;
     }
     
     for (int i = 0; i < expander->macro_count; i++) {
         if (strcmp(expander->macros[i].name, name) == 0) {
             return &expander->macros[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Enter a new scope
  * 
  * @param expander The macro expander
  */
 void polycall_macro_enter_scope(polycall_macro_expander_t* expander) {
     if (!expander) {
         return;
     }
     
     expander->scope.track_scopes = true;
 }
 
 /**
  * @brief Exit the current scope
  * 
  * @param expander The macro expander
  */
 void polycall_macro_exit_scope(polycall_macro_expander_t* expander) {
     if (!expander || !expander->scope.track_scopes) {
         return;
     }
     
     // Remove all macros defined in the current scope
     int to_remove = expander->macro_count - expander->scope.global_scope_end;
     
     for (int i = 0; i < to_remove; i++) {
         int idx = expander->macro_count - 1;
         // Free resources for the macro
         if (expander->macros[idx].name) {
             free(expander->macros[idx].name);
             expander->macros[idx].name = NULL;
         }
         
         if (expander->macros[idx].expansion) {
             polycall_ast_node_destroy(expander->macros[idx].expansion);
             expander->macros[idx].expansion = NULL;
         }
         
         // Free parameters
         if (expander->macros[idx].params) {
             for (int j = 0; j < expander->macros[idx].param_count; j++) {
                 if (expander->macros[idx].params[j].name) {
                     free(expander->macros[idx].params[j].name);
                 }
                 if (expander->macros[idx].params[j].default_value) {
                     free(expander->macros[idx].params[j].default_value);
                 }
             }
             free(expander->macros[idx].params);
             expander->macros[idx].params = NULL;
         }
         
         expander->macro_count--;
     }
 }
 
 /**
  * @brief Clone an AST node
  * 
  * @param node The node to clone
  * @return polycall_ast_node_t* The cloned node
  */
 static polycall_ast_node_t* clone_ast_node(polycall_ast_node_t* node) {
     if (!node) {
         return NULL;
     }
     
     polycall_ast_node_t* clone = polycall_ast_node_create(node->type, node->name);
     if (!clone) {
         return NULL;
     }
     
     // Clone children
     for (int i = 0; i < node->child_count; i++) {
         polycall_ast_node_t* child_clone = clone_ast_node(node->children[i]);
         if (!child_clone) {
             polycall_ast_node_destroy(clone);
             return NULL;
         }
         
         if (!polycall_ast_node_add_child(clone, child_clone)) {
             polycall_ast_node_destroy(child_clone);
             polycall_ast_node_destroy(clone);
             return NULL;
         }
     }
     
     return clone;
 }
 
 /**
  * @brief Check if a node contains a macro reference
  * 
  * @param node The node to check
  * @param expander The macro expander
  * @return const char* The macro name if found, NULL otherwise
  */
 static const char* find_macro_reference(polycall_ast_node_t* node, 
                                        polycall_macro_expander_t* expander) {
     if (!node || !expander) {
         return NULL;
     }
     
     // Check if the node is a reference to a macro
     // For now, we only handle simple cases where the node name directly matches a macro name
     if (node->type == NODE_IDENTIFIER) {
         for (int i = 0; i < expander->macro_count; i++) {
             if (strcmp(node->name, expander->macros[i].name) == 0) {
                 return expander->macros[i].name;
             }
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Expand macros in a node
  * 
  * @param expander The macro expander
  * @param node The node to process
  * @return polycall_ast_node_t* The expanded node
  */
 polycall_ast_node_t* polycall_macro_expand_node(polycall_macro_expander_t* expander, 
                                               polycall_ast_node_t* node) {
     if (!expander || !node) {
         return node;
     }
     
     // Check if this node is a macro reference
     const char* macro_name = find_macro_reference(node, expander);
     if (macro_name) {
         polycall_macro_def_t* macro = polycall_macro_find(expander, macro_name);
         if (macro) {
             // Clone the expansion to avoid modifying the original
             return clone_ast_node(macro->expansion);
         }
     }
     
     // Process children recursively
     for (int i = 0; i < node->child_count; i++) {
         polycall_ast_node_t* expanded_child = 
             polycall_macro_expand_node(expander, node->children[i]);
         
         if (expanded_child != node->children[i]) {
             // Replace the child with the expanded one
             polycall_ast_node_destroy(node->children[i]);
             node->children[i] = expanded_child;
         }
     }
     
     return node;
 }
 
 /**
  * @brief Expand all macros in an AST
  * 
  * @param expander The macro expander
  * @param ast The AST to process
  * @return bool True if successful, false otherwise
  */
 bool polycall_macro_expand_ast(polycall_macro_expander_t* expander, polycall_ast_t* ast) {
     if (!expander || !ast || !ast->root) {
         return false;
     }
     
     // Expand macros in the root node and its children
     polycall_ast_node_t* expanded_root = 
         polycall_macro_expand_node(expander, ast->root);
     
     if (expanded_root != ast->root) {
         polycall_ast_node_destroy(ast->root);
         ast->root = expanded_root;
     }
     
     return true;
 }
