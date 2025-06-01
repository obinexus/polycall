# LibPolyCall Command System Design

## Architecture Overview

The command system follows a hierarchical structure with the following pattern:
```
./polycall [command] [subcommand] [arguments] [--flags]
```

Core components:
1. **Command Parser** - Processes input arguments, validates structure
2. **Command Registry** - Stores and retrieves command handlers
3. **Command Execution** - Dispatches to appropriate handlers
4. **Help System** - Provides contextual documentation
5. **Accessibility Layer** - Integrates with Biafran UI/UX

## Command Structure

Each command should follow a consistent structure:
- Primary commands define core functional areas
- Subcommands define specific operations
- Arguments provide required data
- Flags modify behavior

### Implementation in `command.c`

```c
/**
 * @brief Process command-line arguments and dispatch to appropriate handler
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Core context
 * @return int Exit code
 */
int process_command_line(int argc, char** argv, polycall_core_context_t* context) {
    if (argc < 2) {
        // No command specified, show help
        return show_help(NULL, context);
    }
    
    const char* command_name = argv[1];
    
    // Check for help request
    if (strcmp(command_name, "help") == 0) {
        if (argc > 2) {
            return show_help(argv[2], context);
        } else {
            return show_help(NULL, context);
        }
    }
    
    // Find and execute command
    const command_t* command = find_command(command_name);
    if (!command) {
        polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
        char error_buffer[256];
        
        if (access_ctx) {
            char error_msg[128];
            snprintf(error_msg, sizeof(error_msg), "Unknown command: %s", command_name);
            
            polycall_accessibility_format_text(
                context,
                access_ctx,
                error_msg,
                POLYCALL_TEXT_ERROR,
                POLYCALL_STYLE_NORMAL,
                error_buffer,
                sizeof(error_buffer)
            );
        } else {
            snprintf(error_buffer, sizeof(error_buffer), "Unknown command: %s", command_name);
        }
        
        fprintf(stderr, "%s\n\n", error_buffer);
        show_help(NULL, context);
        return COMMAND_ERROR_NOT_FOUND;
    }
    
    // Execute command with remaining arguments
    return command->handler(argc - 1, &argv[1], context);
}
```

## Command Registry

Enhance the existing command registry to support hierarchical commands:

```c
typedef struct {
    const char* name;                   /* Command name */
    const char* description;            /* Command description */
    const char* usage;                  /* Command usage */
    command_handler_t handler;          /* Command handler function */
    subcommand_t* subcommands;          /* Array of subcommands */
    int subcommand_count;               /* Number of subcommands */
    bool requires_context;              /* Whether command requires context */
    polycall_text_type_t text_type;     /* Text type for accessibility */
    const char* screen_reader_desc;     /* Description for screen readers */
} command_t;

typedef struct {
    const char* name;                   /* Subcommand name */
    const char* description;            /* Subcommand description */
    const char* usage;                  /* Subcommand usage */
    command_handler_t handler;          /* Subcommand handler */
    bool requires_context;              /* Whether requires context */
} subcommand_t;
```

## Primary Commands

Based on your existing files, implement these primary commands:

1. **auth** - Authentication and security management
   - **login** - Authenticate user
   - **token** - Manage authentication tokens
   - **policy** - Configure security policies

2. **config** - Configuration management
   - **show** - Display current configuration
   - **set** - Modify configuration
   - **reset** - Reset to defaults

3. **network** - Network operations
   - **server** - Manage servers
   - **client** - Manage clients
   - **status** - Show network status

4. **protocol** - Protocol operations
   - **encode** - Encode messages
   - **decode** - Decode messages
   - **validate** - Validate protocol

5. **ffi** - Foreign Function Interface
   - **list** - List available language bridges
   - **bind** - Create language binding
   - **test** - Test language interop

6. **micro** - Microservice management
   - **deploy** - Deploy microservice
   - **list** - List microservices
   - **scale** - Scale microservice

7. **edge** - Edge computing management
   - **deploy** - Deploy to edge
   - **route** - Configure routing
   - **monitor** - Monitor edge nodes

8. **telemetry** - Monitoring and metrics
   - **status** - Show telemetry status
   - **enable** - Enable telemetry
   - **disable** - Disable telemetry

9. **repl** - Interactive shell
   - Enters interactive mode without subcommands

10. **accessibility** - Accessibility settings
    - **theme** - Set UI theme
    - **colors** - Configure colors
    - **apply** - Apply environment settings

## CLI Argument Processing

Implement a robust argument parser to handle:
- Positional arguments
- Flag arguments (--flag)
- Value arguments (--param=value)
- Combined short flags (-abc)

## Help System Implementation

Create a context-aware help system:

```c
/**
 * @brief Show help for a command or general help
 * 
 * @param command_name Command name or NULL for general help
 * @param context Core context
 * @return int Exit code
 */
int show_help(const char* command_name, polycall_core_context_t* context) {
    polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
    
    if (!command_name) {
        // Show general help
        char title[256];
        
        if (access_ctx) {
            polycall_accessibility_format_text(
                context,
                access_ctx,
                "LibPolyCall Command-Line Interface",
                POLYCALL_TEXT_HEADING,
                POLYCALL_STYLE_BOLD,
                title,
                sizeof(title)
            );
            printf("%s\n\n", title);
        } else {
            printf("LibPolyCall Command-Line Interface\n\n");
        }
        
        printf("Usage: polycall [command] [subcommand] [arguments] [--flags]\n\n");
        printf("Available commands:\n");
        
        // List all commands with descriptions
        for (int i = 0; i < command_count; i++) {
            if (access_ctx) {
                char cmd_name[128];
                polycall_accessibility_format_text(
                    context,
                    access_ctx,
                    commands[i].name,
                    POLYCALL_TEXT_COMMAND,
                    POLYCALL_STYLE_NORMAL,
                    cmd_name,
                    sizeof(cmd_name)
                );
                
                char cmd_desc[256];
                polycall_accessibility_format_text(
                    context,
                    access_ctx,
                    commands[i].description,
                    POLYCALL_TEXT_NORMAL,
                    POLYCALL_STYLE_NORMAL,
                    cmd_desc,
                    sizeof(cmd_desc)
                );
                
                printf("  %-15s  %s\n", cmd_name, cmd_desc);
            } else {
                printf("  %-15s  %s\n", commands[i].name, commands[i].description);
            }
        }
        
        printf("\nFor more information on a specific command, run: polycall help [command]\n");
        return COMMAND_SUCCESS;
    }
    
    // Show help for a specific command
    const command_t* command = find_command(command_name);
    if (!command) {
        fprintf(stderr, "Unknown command: %s\n", command_name);
        return COMMAND_ERROR_NOT_FOUND;
    }
    
    // Format and display command help
    if (access_ctx) {
        char help_buffer[1024];
        polycall_accessibility_format_command_help(
            context,
            access_ctx,
            command->name,
            command->description,
            command->usage,
            help_buffer,
            sizeof(help_buffer)
        );
        printf("%s\n", help_buffer);
    } else {
        printf("Command: %s\n", command->name);
        printf("Description: %s\n", command->description);
        printf("Usage: %s\n", command->usage);
    }
    
    // Show subcommands if available
    if (command->subcommand_count > 0) {
        printf("\nAvailable subcommands:\n");
        
        for (int i = 0; i < command->subcommand_count; i++) {
            printf("  %-15s  %s\n", 
                   command->subcommands[i].name, 
                   command->subcommands[i].description);
        }
        
        printf("\nFor more information on a subcommand, run: polycall help %s [subcommand]\n", 
               command->name);
    }
    
    return COMMAND_SUCCESS;
}
```

## Accessibility Integration

Integrate with your existing Biafran UI/UX accessibility system:

1. Format all output with appropriate colors and styles
2. Support screen readers with text annotations
3. Use high-contrast themes when enabled
4. Provide verbose descriptions when needed

Example usage:

```c
// In command handler
void handle_output(const char* message, polycall_core_context_t* context, polycall_text_type_t type) {
    polycall_accessibility_context_t* access_ctx = get_accessibility_context(context);
    
    if (access_ctx) {
        char formatted_output[1024];
        polycall_accessibility_format_text(
            context,
            access_ctx,
            message,
            type,
            POLYCALL_STYLE_NORMAL,
            formatted_output,
            sizeof(formatted_output)
        );
        printf("%s\n", formatted_output);
    } else {
        printf("%s\n", message);
    }
}
```

## Required Modifications

1. Update `main.c` to use the command system
2. Enhance command handlers in each component
3. Update `CMakeLists.txt` to include the new files
4. Implement the accessibility context accessor

## Implementation Plan

1. Create core command parser
2. Update command registry to support hierarchical commands
3. Implement primary command handlers
4. Add accessibility integration
5. Update build system
6. Create comprehensive help documentation

This design ensures a cohesive, extensible command-line interface with full accessibility support via the Biafran UI/UX design system.