/**
#include "polycall/core/config/polycallfile/ast.h"

 * @file ast.c
 * @brief Abstract Syntax Tree implementation for LibPolyCall configuration parser
 * @author Implementation based on Nnamdi Okpala's design
 *
 * This file implements the AST (Abstract Syntax Tree) functionality for the
 * configuration parser, providing node creation, traversal, and manipulation.
 */

 #include "polycall/core/config/polycallfile/ast.h"

 
 /**
  * @brief Create a new AST node
  * 
  * @param type The node type
  * @param name The node name
  * @return polycall_ast_node_t* The created node, or NULL if allocation fails
  */
 polycall_ast_node_t* polycall_ast_node_create(polycall_ast_node_type_t type, const char* name) {
     polycall_ast_node_t* node = (polycall_ast_node_t*)malloc(sizeof(polycall_ast_node_t));
     if (!node) {
         return NULL;
     }
     
     // Initialize basic properties
     node->type = type;
     node->name = name ? strdup(name) : NULL;
     if (name && !node->name) {
         free(node);
         return NULL;
     }
     
     node->parent = NULL;
     node->children = NULL;
     node->child_count = 0;
     node->child_capacity = 0;
     
     return node;
 }
 
 /**
  * @brief Free an AST node and all its children
  * 
  * @param node The node to free
  */
 void polycall_ast_node_destroy(polycall_ast_node_t* node) {
     if (!node) {
         return;
     }
     
     // Free name
     if (node->name) {
         free(node->name);
     }
     
     // Recursively free children
     if (node->children) {
         for (int i = 0; i < node->child_count; i++) {
             polycall_ast_node_destroy(node->children[i]);
         }
         free(node->children);
     }
     
     // Free the node itself
     free(node);
 }
 
 /**
  * @brief Ensure a node has enough capacity for a new child
  * 
  * @param parent The parent node
  * @return bool True if successful, false on allocation failure
  */
 static bool ensure_child_capacity(polycall_ast_node_t* parent) {
     if (parent->child_count == parent->child_capacity) {
         // Need to expand
         int new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
         polycall_ast_node_t** new_children = (polycall_ast_node_t**)realloc(
             parent->children, 
             new_capacity * sizeof(polycall_ast_node_t*)
         );
         
         if (!new_children) {
             return false;
         }
         
         parent->children = new_children;
         parent->child_capacity = new_capacity;
     }
     
     return true;
 }
 
 /**
  * @brief Add a child node to a parent node
  * 
  * @param parent The parent node
  * @param child The child node
  * @return bool True if successful, false otherwise
  */
 bool polycall_ast_node_add_child(polycall_ast_node_t* parent, polycall_ast_node_t* child) {
     if (!parent || !child) {
         return false;
     }
     
     // Ensure we have capacity for the new child
     if (!ensure_child_capacity(parent)) {
         return false;
     }
     
     // Add the child
     parent->children[parent->child_count++] = child;
     child->parent = parent;
     
     return true;
 }
 
 /**
  * @brief Find a child node by name
  * 
  * @param parent The parent node
  * @param name The name to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_node_find_child(polycall_ast_node_t* parent, const char* name) {
     if (!parent || !name) {
         return NULL;
     }
     
     for (int i = 0; i < parent->child_count; i++) {
         if (parent->children[i]->name && strcmp(parent->children[i]->name, name) == 0) {
             return parent->children[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Split a path string by its delimiter
  * 
  * @param path The path string
  * @param parts Array to receive the parts
  * @param max_parts Maximum number of parts to extract
  * @return int Number of parts extracted
  */
 static int split_path(const char* path, char** parts, int max_parts) {
     if (!path || !parts || max_parts <= 0) {
         return 0;
     }
     
     const char* start = path;
     const char* p = path;
     int count = 0;
     
     while (*p && count < max_parts) {
         if (*p == '.') {
             // Found a delimiter
             int len = p - start;
             if (len > 0) {
                 parts[count] = (char*)malloc(len + 1);
                 if (!parts[count]) {
                     // Cleanup on allocation failure
                     for (int i = 0; i < count; i++) {
                         free(parts[i]);
                     }
                     return 0;
                 }
                 
                 strncpy(parts[count], start, len);
                 parts[count][len] = '\0';
                 count++;
             }
             start = p + 1;
         }
         p++;
     }
     
     // Handle the last part
     if (*start && count < max_parts) {
         parts[count] = strdup(start);
         if (!parts[count]) {
             // Cleanup on allocation failure
             for (int i = 0; i < count; i++) {
                 free(parts[i]);
             }
             return 0;
         }
         count++;
     }
     
     return count;
 }
 
 /**
  * @brief Find a node by path (e.g., "network.tls.enabled")
  * 
  * @param root The root node
  * @param path The path to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_node_find_path(polycall_ast_node_t* root, const char* path) {
     if (!root || !path) {
         return NULL;
     }
     
     // Split the path into parts
     const int MAX_PARTS = 32;  // Reasonable limit for max depth
     char* parts[MAX_PARTS];
     int part_count = split_path(path, parts, MAX_PARTS);
     
     if (part_count == 0) {
         // No valid parts found, or just return root if path is empty
         return path[0] ? NULL : root;
     }
     
     // Start traversal from root
     polycall_ast_node_t* current = root;
     
     for (int i = 0; i < part_count && current; i++) {
         current = polycall_ast_node_find_child(current, parts[i]);
     }
     
     // Clean up allocated parts
     for (int i = 0; i < part_count; i++) {
         free(parts[i]);
     }
     
     return current;
 }
 
 /**
  * @brief Create a new AST
  * 
  * @return polycall_ast_t* The created AST, or NULL if allocation fails
  */
 polycall_ast_t* polycall_ast_create() {
     polycall_ast_t* ast = (polycall_ast_t*)malloc(sizeof(polycall_ast_t));
     if (!ast) {
         return NULL;
     }
     
     // Create a root node (empty section)
     ast->root = polycall_ast_node_create(NODE_SECTION, "root");
     if (!ast->root) {
         free(ast);
         return NULL;
     }
     
     return ast;
 }
 
 /**
  * @brief Free an AST
  * 
  * @param ast The AST to free
  */
 void polycall_ast_destroy(polycall_ast_t* ast) {
     if (!ast) {
         return;
     }
     
     // Free the root node (which will recursively free all children)
     if (ast->root) {
         polycall_ast_node_destroy(ast->root);
     }
     
     // Free the AST itself
     free(ast);
 }
 
 /**
  * @brief Find a node by path in the AST
  * 
  * @param ast The AST
  * @param path The path to find
  * @return polycall_ast_node_t* The found node, or NULL if not found
  */
 polycall_ast_node_t* polycall_ast_find_node(polycall_ast_t* ast, const char* path) {
     if (!ast || !ast->root || !path) {
         return NULL;
     }
     
     return polycall_ast_node_find_path(ast->root, path);
 }
 
// Helper function to print a node with indentation
void print_node(polycall_ast_node_t* node, int indent, FILE* file) {
    // Print indentation
    for (int i = 0; i < indent; i++) {
        fprintf(file, "  ");
    }
    
    // Print node type and name
    const char* type_str = "UNKNOWN";
    switch (node->type) {
        case NODE_SECTION: type_str = "SECTION"; break;
        case NODE_STATEMENT: type_str = "STATEMENT"; break;
        case NODE_VALUE_STRING: type_str = "STRING"; break;
        case NODE_VALUE_NUMBER: type_str = "NUMBER"; break;
        case NODE_VALUE_BOOLEAN: type_str = "BOOLEAN"; break;
        case NODE_VALUE_NULL: type_str = "NULL"; break;
        case NODE_VALUE_ARRAY: type_str = "ARRAY"; break;
        case NODE_DIRECTIVE: type_str = "DIRECTIVE"; break;
    }
    
    fprintf(file, "%s: %s\n", type_str, node->name ? node->name : "(null)");
    
    // Print children
    for (int i = 0; i < node->child_count; i++) {
        print_node(node->children[i], indent + 1, file);
    }
}

 /**
  * @brief Print the AST to a file (for debugging)
  * 
  * @param ast The AST to print
  * @param file The file to print to
  */
 void polycall_ast_print(polycall_ast_t* ast, FILE* file) {
     if (!ast || !ast->root || !file) {
         return;
     }
     
     
     
     // Print the root node
     print_node(ast->root, 0, file);
 }