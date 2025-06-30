# LibPolyCall FFI Module

The Foreign Function Interface (FFI) module is a core component of LibPolyCall, enabling seamless cross-language function calls with a consistent "Program-First" design philosophy. This module allows code written in different programming languages to interact with each other efficiently and securely.

## Features

- **Cross-Language Function Calls**: Call functions across language boundaries with type safety
- **Comprehensive Type System**: Convert data between different languages automatically
- **Memory Management**: Safely share memory between language runtimes
- **Zero-Trust Security**: Control function access with fine-grained permissions
- **Performance Optimization**: Caching and batching for efficient cross-language operations
- **Multiple Language Support**: C, JavaScript, Java/JVM, Python, and extensible to others

## Architecture

The FFI module is organized into several key components:

### Core Components

- **FFI Core** (`ffi_core.h/c`): Central coordination for function registration and invocation
- **Type System** (`type_system.h/c`): Type mapping and conversion between language type systems
- **Memory Bridge** (`memory_bridge.h/c`): Memory sharing and reference counting across languages
- **Security** (`security.h/c`): Access control and isolation for cross-language calls
- **Performance** (`performance.h/c`): Optimization for cross-language function calls

### Language Bridges

- **C Bridge** (`c_bridge.h/c`): Native C function integration
- **JavaScript Bridge** (`js_bridge.h/c`): JavaScript runtime integration
- **JVM Bridge** (`jvm_bridge.h/c`): Java virtual machine integration
- **Python Bridge** (`python_bridge.h/c`): Python runtime integration

## Getting Started

### Prerequisites

- C compiler with C11 support (GCC 7+ or Clang 5+)
- CMake 3.13 or higher
- Language-specific dependencies:
  - JVM Bridge: JDK with JNI headers
  - JavaScript Bridge: V8 or other JS engine development package
  - Python Bridge: Python development package

### Building the FFI Module

1. Run the setup script to install dependencies and configure the build:

```bash
./setup-ffi.sh
```

2. Build the debug version:

```bash
cd build/debug && make
```

3. Build the release version:

```bash
cd build/release && make
```

### Testing

Run the FFI module tests:

```bash
cd build/debug && ctest
```

## Usage Examples

### Exposing a C Function to JavaScript

```c
#include "core/ffi/ffi_core.h"
#include "core/ffi/c_bridge.h"

// Define a C function
int add(int a, int b) {
    return a + b;
}

int main() {
    // Initialize FFI context
    polycall_core_context_t* core_ctx;
    polycall_ffi_context_t* ffi_ctx;
    polycall_c_bridge_t* c_bridge;
    
    // Initialize core context
    polycall_core_init(&core_ctx, NULL);
    
    // Initialize FFI context
    polycall_ffi_config_t ffi_config = {0};
    ffi_config.function_capacity = 64;
    ffi_config.type_capacity = 128;
    ffi_config.memory_pool_size = 1024 * 1024; // 1MB
    polycall_ffi_init(core_ctx, &ffi_ctx, &ffi_config);
    
    // Initialize C bridge
    polycall_c_bridge_config_t c_config = polycall_c_bridge_create_default_config();
    polycall_c_bridge_init(core_ctx, ffi_ctx, &c_bridge, &c_config);
    
    // Register the add function
    polycall_c_bridge_register_function(
        core_ctx, ffi_ctx, c_bridge,
        "add", (void*)add,
        POLYCALL_FFI_TYPE_INT32,
        (polycall_ffi_type_t[]){POLYCALL_FFI_TYPE_INT32, POLYCALL_FFI_TYPE_INT32},
        2, 0
    );
    
    // Now "add" is available to call from other languages
    
    // Clean up
    polycall_c_bridge_cleanup(core_ctx, ffi_ctx, c_bridge);
    polycall_ffi_cleanup(core_ctx, ffi_ctx);
    polycall_core_cleanup(core_ctx);
    
    return 0;
}
```

### Calling a JavaScript Function from C

```c
#include "core/ffi/ffi_core.h"
#include "core/ffi/js_bridge.h"

// Callback for handling JavaScript function result
void handle_js_result(ffi_value_t* result) {
    if (result->type == POLYCALL_FFI_TYPE_INT32) {
        printf("JavaScript function returned: %d\n", result->value.int32_value);
    }
}

int main() {
    // Initialize contexts and bridges
    // ...
    
    // Prepare arguments
    ffi_value_t args[2];
    args[0].type = POLYCALL_FFI_TYPE_INT32;
    args[0].value.int32_value = 5;
    args[1].type = POLYCALL_FFI_TYPE_INT32;
    args[1].value.int32_value = 3;
    
    // Prepare result
    ffi_value_t result;
    
    // Call JavaScript function
    polycall_ffi_call_function(
        core_ctx, ffi_ctx,
        "jsMultiply", // JavaScript function name
        args, 2, &result, "javascript"
    );
    
    // Handle result
    handle_js_result(&result);
    
    // Clean up
    // ...
    
    return 0;
}
```

## Integration with Other LibPolyCall Modules

The FFI module integrates with other LibPolyCall modules:

- **Core System**: FFI registers with the Core Context system
- **Micro Command**: Components can access FFI functions securely
- **Edge Computing**: FFI calls can be routed to distributed nodes
- **Protocol System**: Remote FFI calls can be made over the network

## Contributing

Contributions to the FFI module are welcome! Here are some areas where help is needed:

- Additional language bridges (Ruby, Go, Rust, etc.)
- Performance optimizations for specific use cases
- Security enhancements and testing
- Documentation improvements
