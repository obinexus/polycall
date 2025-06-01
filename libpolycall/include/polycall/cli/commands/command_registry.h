// command_registry.h
#ifndef POLYCALL_COMMAND_REGISTRY_H
#define POLYCALL_COMMAND_REGISTRY_H

#include <stddef.h>

// Command execution function type
typedef int (*polycall_command_func_t)(void* container, int argc, char** argv, void* context);

typedef struct {
    polycall_command_func_t execute;  // Command execution function
    const char* name;                // Command name
    const char* description;         // Command description
    const char* usage;               // Command usage string
    const char** dependencies;       // Required services
} polycall_command_t;

// Create command registry
void* polycall_command_registry_create();

// Destroy command registry
void polycall_command_registry_destroy(void* registry);

// Register command
int polycall_command_registry_register(
    void* registry,
    const char* module,
    const polycall_command_t* command);

// Execute command
int polycall_command_registry_execute(
    void* registry,
    void* container,
    const char* module,
    const char* command,
    int argc,
    char** argv,
    void* context);

// List available commands for module
int polycall_command_registry_list(
    void* registry,
    const char* module,
    polycall_command_t** commands,
    size_t* command_count);

// List available modules
int polycall_command_registry_list_modules(
    void* registry,
    char*** modules,
    size_t* module_count);

#endif /* POLYCALL_COMMAND_REGISTRY_H */