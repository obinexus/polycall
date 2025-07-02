# LibPolyCall Integration Guide

This guide explains how to integrate LibPolyCall into your project, whether using CMake, other build systems, or manually linking with the library.

## Prerequisites

- LibPolyCall installed on your system (version 1.1.0 or later)
- C compiler supporting C11 standard
- (Optional) CMake 3.13 or later for CMake integration

## Basic Usage

LibPolyCall provides a unified header `polycall.h` that includes all necessary functionality. The simplest way to use it in your code is:

```c
#include <polycall.h>

int main() {
    // Initialize LibPolyCall with default configuration
    int result = polycall_init(NULL);
    if (result != 0) {
        fprintf(stderr, "Failed to initialize LibPolyCall: %s\n", 
                polycall_error_string(result));
        return 1;
    }
    
    // Use LibPolyCall functionality here
    
    // Clean up resources
    polycall_shutdown();
    
    return 0;
}
```

## Linking with LibPolyCall

### Using CMake (Recommended)

If you're using CMake, add the following to your `CMakeLists.txt`:

```cmake
# Find LibPolyCall package
find_package(polycall 1.1 REQUIRED)

# Create your executable
add_executable(your_program your_source.c)

# Link with LibPolyCall
target_link_libraries(your_program PRIVATE polycall::polycall)
```

### Using pkg-config

LibPolyCall provides pkg-config support. You can use it in your build system:

```bash
gcc -o your_program your_source.c $(pkg-config --cflags --libs polycall)
```

### Manual Linking

You can also manually link with the library:

```bash
gcc -o your_program your_source.c -lpolycall
```

## Accessing Specific Components

While the unified header provides access to all components, you can also include specific component headers if you only need a subset of functionality:

```c
// For authentication only
#include <polycall/core/auth/polycall_auth_context.h>

// For networking only
#include <polycall/core/network/network.h>
```

## Component-Specific Libraries

Each component can also be linked separately if you only need specific functionality:

```bash
# Link only with the network component
gcc -o your_network_app your_network_app.c -lpolycall_network
```

In CMake:

```cmake
target_link_libraries(your_network_app PRIVATE polycall::network)
```

## Examples

### Basic Application

```c
#include <polycall.h>
#include <stdio.h>

int main() {
    // Initialize with default configuration
    if (polycall_init(NULL) != 0) {
        return 1;
    }
    
    // Print library version
    printf("Using LibPolyCall version: %s\n", polycall_get_version());
    
    // Create a context
    polycall_context_t* ctx = polycall_context_create();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        polycall_shutdown();
        return 1;
    }
    
    // Use the context...
    
    // Clean up
    polycall_context_destroy(ctx);
    polycall_shutdown();
    
    return 0;
}
```

### Networking Example

```c
#include <polycall.h>
#include <stdio.h>

int main() {
    // Initialize with default configuration
    if (polycall_init(NULL) != 0) {
        return 1;
    }
    
    // Create a network client
    polycall_network_client_t* client = polycall_network_client_create();
    if (!client) {
        fprintf(stderr, "Failed to create network client\n");
        polycall_shutdown();
        return 1;
    }
    
    // Connect to a server
    if (polycall_network_client_connect(client, "localhost", 8080) != 0) {
        fprintf(stderr, "Failed to connect\n");
        polycall_network_client_destroy(client);
        polycall_shutdown();
        return 1;
    }
    
    printf("Connected to server\n");
    
    // Send a message
    const char* message = "Hello, server!";
    if (polycall_network_client_send(client, message, strlen(message)) != 0) {
        fprintf(stderr, "Failed to send message\n");
    }
    
    // Clean up
    polycall_network_client_destroy(client);
    polycall_shutdown();
    
    return 0;
}
```

## Building and Linking Options

LibPolyCall provides both static and shared libraries. By default, applications will link with the shared library.

To explicitly link with the static library:

```bash
gcc -o your_program your_source.c -lpolycall_static
```

In CMake:

```cmake
target_link_libraries(your_program PRIVATE polycall::polycall_static)
```

## Troubleshooting

### Common Issues

1. **Library not found**: Ensure that LibPolyCall is properly installed and that your linker can find it.
   ```bash
   # Check library installation
   ldconfig -p | grep polycall
   ```

2. **Header not found**: Verify your include paths are correctly set.
   ```bash
   # Find header location
   find /usr/include -name polycall.h
   ```

3. **Version mismatch**: Ensure you're using the correct version of LibPolyCall.
   ```c
   printf("LibPolyCall version: %s\n", polycall_get_version());
   ```

### Getting Help

If you encounter issues, please check:

1. The documentation in the `docs/` directory
2. API reference at [https://libpolycall.obinexuscomputing.com/api](https://libpolycall.obinexuscomputing.com/api)
3. File an issue at [https://github.com/obinexuscomputing/libpolycall](https://github.com/obinexuscomputing/libpolycall)