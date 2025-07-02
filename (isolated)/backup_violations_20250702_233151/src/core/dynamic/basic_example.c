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
 * @file basic_example.c
 * @brief Basic example of using LibPolyCall
 * 
 * This example demonstrates how to initialize LibPolyCall,
 * use various components, and properly clean up resources.
 */

#include <polycall.h>
#include <string.h>

// Error handling helper function
void check_error(int result, const char* operation) {
    if (result != 0) {
        fprintf(stderr, "Error during %s: %s (code: %d)\n", 
                operation, polycall_error_string(result), result);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    int result;
    
    printf("LibPolyCall Basic Example\n");
    printf("=========================\n\n");
    
    // Step 1: Initialize LibPolyCall
    printf("Initializing LibPolyCall...\n");
    result = polycall_init(NULL);
    check_error(result, "initialization");
    
    // Print version information
    printf("Using LibPolyCall version: %s\n\n", polycall_get_version());
    
    // Step 2: Create a main context
    printf("Creating main context...\n");
    polycall_context_t* context = polycall_context_create();
    if (!context) {
        fprintf(stderr, "Failed to create context\n");
        polycall_shutdown();
        return EXIT_FAILURE;
    }
    
    // Step 3: Set up authentication (optional)
    if (argc > 1 && strcmp(argv[1], "--auth") == 0) {
        printf("Setting up authentication...\n");
        
        // Create authentication context
        polycall_auth_context_t* auth_ctx = polycall_auth_context_create();
        if (!auth_ctx) {
            fprintf(stderr, "Failed to create auth context\n");
            polycall_context_destroy(context);
            polycall_shutdown();
            return EXIT_FAILURE;
        }
        
        // Attach authentication to main context
        result = polycall_context_set_auth(context, auth_ctx);
        check_error(result, "attaching authentication");
        
        printf("Authentication configured successfully\n\n");
    }
    
    // Step 4: Create a network endpoint (optional)
    if (argc > 1 && strcmp(argv[1], "--network") == 0) {
        printf("Setting up network endpoint...\n");
        
        // Create a network endpoint
        polycall_network_endpoint_t* endpoint = polycall_network_endpoint_create();
        if (!endpoint) {
            fprintf(stderr, "Failed to create network endpoint\n");
            polycall_context_destroy(context);
            polycall_shutdown();
            return EXIT_FAILURE;
        }
        
        // Configure the endpoint
        result = polycall_network_endpoint_configure(endpoint, "localhost", 8080);
        check_error(result, "configuring network endpoint");
        
        // Attach to context
        result = polycall_context_set_network(context, endpoint);
        check_error(result, "attaching network endpoint");
        
        printf("Network endpoint configured successfully\n\n");
    }
    
    // Step 5: Execute some operations using the context
    printf("Executing sample operations...\n");
    
    // Generate a token (just as an example)
    char token_buffer[256];
    result = polycall_context_generate_token(context, token_buffer, sizeof(token_buffer));
    if (result == 0) {
        printf("Generated token: %s\n", token_buffer);
    } else {
        printf("Token generation not available in this configuration\n");
    }
    
    // Step 6: Clean up resources
    printf("\nCleaning up resources...\n");
    polycall_context_destroy(context);
    
    // Step 7: Shut down LibPolyCall
    printf("Shutting down LibPolyCall...\n");
    result = polycall_shutdown();
    check_error(result, "shutdown");
    
    printf("Example completed successfully\n");
    return EXIT_SUCCESS;
}
