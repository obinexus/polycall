/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 */

 #ifndef POLYCALL_CONFIG_POLYCALLFILE_AST_H_H
 #define POLYCALL_CONFIG_POLYCALLFILE_AST_H_H
 
 #include <stdbool.h>
 #include <stddef.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
 /**
  * @brief AST node types
  */
 typedef enum {
     NODE_SECTION,            // Section (e.g., network {})
     NODE_STATEMENT,          // Statement (e.g., port = 8080)
     NODE_VALUE_STRING,       // String value
     NODE_VALUE_NUMBER,       // Numeric value
     NODE_VALUE_BOOLEAN,      // Boolean value
     NODE_VALUE_NULL,         // Null value
     NODE_VALUE_ARRAY,        // Array value
     NODE_DIRECTIVE,           // Directive (e.g., @define)
     NODE_IDENTIFIER,         // Identifier (e.g., variable name)
    NODE_COMMENT,           // Comment
    NODE_EXPRESSION_BINARY, // Binary expression (e.g., a + b)
    NODE_EXPRESSION_UNARY,  // Unary expression (e.g., -a)
    NODE_ERROR              // Error node

 } polycall_ast_node_type_t;
 
 /**
  * @brief AST node structure (forward declaration)
  */
 typedef struct polycall_ast_node polycall_ast_node_t;
 
 /**
  * @brief AST node structure
  */
 struct polycall_ast_node {
     polycall_ast_node_type_t type;  // Node type
     char* name;                      // Node name
     polycall_ast_node_t* parent;    // Parent node
     polycall_ast_node_t** children; // Child nodes
     int child_count;                // Number of children
     int child_capacity;             // Capacity of children array
 };
 
 /**
  * @brief AST structure
  */
 typedef struct {
     polycall_ast_node_t* root;      // Root node
 } polycall_ast_t;
 
 /**
  * @brief Create a new AST node
  * 
  * @param type The node type
  * @param name The node name
  * @return polycall_ast_node_t* The created node
  */
 polycall_ast_node_t* polycall_ast_node_create(polycall_ast_node_type_t type, const char* name);
 
 /**
  * @brief Free an AST node and all its children
  * 
  * @param node The node to free
  */
 void polycall_ast_node_destroy(polycall_ast_node_t* node);
 
 /**
  * @brief Add a child node to a parent node
  * 
  * @param parent The parent node
  * @param child The child node
  * @return bool True if successful, false otherwise
  */
 bool polycall_ast_node_add_child(polycall_ast_node_t* parent, polycall_ast_node_t* child);
 
 /**
  * @brief Find a child node by name
  * 
  * @param parent The parent node
  * @param name The name to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_node_find_child(polycall_ast_node_t* parent, const char* name);
 
 /**
  * @brief Find a node by path (e.g., "network.tls.enabled")
  * 
  * @param root The root node
  * @param path The path to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_node_find_path(polycall_ast_node_t* root, const char* path);
 
 /**
  * @brief Create a new AST
  * 
  * @return polycall_ast_t* The created AST
  */
 polycall_ast_t* polycall_ast_create();
 
 /**
  * @brief Free an AST
  * 
  * @param ast The AST to free
  */
 void polycall_ast_destroy(polycall_ast_t* ast);
 
 /**
  * @brief Find a node by path in the AST
  * 
  * @param ast The AST
  * @param path The path to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_find_node(polycall_ast_t* ast, const char* path);
 
 #endif /* POLYCALL_CONFIG_POLYCALLFILE_AST_H_H */