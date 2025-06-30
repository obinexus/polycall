# LibPolyCall DOP (Data-Oriented Programming) Adapter

**OBINexus Computing - Aegis Project Technical Infrastructure**

Universal cross-language micro-component adapter framework implementing strict Zero Trust security enforcement and IoC compliance. Essential for banking applications requiring component isolation between trusted payment services and untrusted third-party components.

## 🏗️ Architecture Overview

The DOP Adapter provides a comprehensive framework for secure cross-language component integration:

```
┌─────────────────────────────────────────────────────────────┐
│                    DOP Adapter Framework                    │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   Banking   │  │  Ads Service│  │ Data Proc.  │         │
│  │ Component   │  │ Component   │  │ Component   │         │
│  │ (Trusted)   │  │(Untrusted)  │  │ (Standard)  │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ JavaScript  │  │   Python    │  │     C       │         │
│  │   Bridge    │  │   Bridge    │  │   Bridge    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│               Core DOP Adapter Engine                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  Security   │  │   Memory    │  │ Lifecycle   │         │
│  │ Enforcement │  │ Management  │  │ Management  │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│                 LibPolyCall Core                           │
└─────────────────────────────────────────────────────────────┘
```

## 🔒 Zero Trust Security Model

### Isolation Levels

| Level | Description | Use Case | Memory Limit | Permissions |
|-------|-------------|----------|--------------|-------------|
| **PARANOID** | Maximum isolation | Critical financial operations | 128KB | None |
| **STRICT** | High security | Payment processing | 256KB | Memory read only |
| **STANDARD** | Balanced security | General business logic | 512KB | Memory read/write, local invoke |
| **BASIC** | Light isolation | Trusted internal components | 1MB | Most operations allowed |
| **NONE** | No isolation | Development/testing only | Unlimited | All permissions |

### Banking Application Example

```c
// Payment component (trusted)
polycall_dop_security_policy_t payment_policy = {
    .isolation_level = POLYCALL_DOP_ISOLATION_STANDARD,
    .allowed_permissions = POLYCALL_DOP_PERMISSION_MEMORY_READ | 
                          POLYCALL_DOP_PERMISSION_MEMORY_WRITE |
                          POLYCALL_DOP_PERMISSION_INVOKE_LOCAL,
    .max_memory_usage = 64 * 1024 * 1024,  // 64MB
    .max_execution_time_ms = 10000
};

// Ads component (untrusted)
polycall_dop_security_policy_t ads_policy = {
    .isolation_level = POLYCALL_DOP_ISOLATION_STRICT,
    .allowed_permissions = POLYCALL_DOP_PERMISSION_MEMORY_READ,
    .max_memory_usage = 8 * 1024 * 1024,   // 8MB
    .max_execution_time_ms = 2000
};
```

## 🚀 Quick Start

### 1. Installation

```bash
# Clone the repository
git clone https://github.com/obinexuscomputing/libpolycall.git
cd libpolycall

# Build with DOP Adapter support
mkdir build && cd build
cmake .. -DBUILD_DOP_BRIDGE_JS=ON -DBUILD_DOP_BRIDGE_PYTHON=ON
make

# Install
sudo make install
```

### 2. Basic Usage (C API)

```c
#include "polycall/core/dop/polycall_dop_adapter.h"

int main() {
    // Initialize LibPolyCall core
    polycall_core_context_t* core_ctx;
    polycall_core_init(&core_ctx, NULL);
    
    // Create security policy
    polycall_dop_security_policy_t policy;
    polycall_dop_security_policy_create_default(
        POLYCALL_DOP_ISOLATION_STANDARD, &policy
    );
    
    // Initialize DOP Adapter
    polycall_dop_adapter_context_t* adapter_ctx;
    polycall_dop_adapter_initialize(core_ctx, &adapter_ctx, &policy);
    
    // Create component configuration
    polycall_dop_component_config_t config;
    polycall_dop_component_config_create_default(
        "my_component", "My Component", 
        POLYCALL_DOP_LANGUAGE_C, &config
    );
    
    // Register component
    polycall_dop_component_t* component;
    polycall_dop_component_register(adapter_ctx, &config, &component);
    
    // Invoke component method
    polycall_dop_result_t result;
    polycall_dop_invoke(adapter_ctx, "my_component", "my_method", 
                        NULL, 0, &result);
    
    // Cleanup
    polycall_dop_adapter_cleanup(adapter_ctx);
    polycall_core_cleanup(core_ctx);
    
    return 0;
}
```

### 3. JavaScript Bridge Usage

```javascript
const { DOPAdapter, DOPIsolationLevel } = require('libpolycall-dop');

async function main() {
    // Initialize adapter
    const adapter = new DOPAdapter();
    await adapter.initialize({
        security_policy: {
            isolation_level: DOPIsolationLevel.STANDARD,
            max_memory_usage: 32 * 1024 * 1024,  // 32MB
            max_execution_time_ms: 5000
        }
    });
    
    // Create component implementation
    class BankingCalculator {
        calculateInterest(principal, rate, time) {
            return principal * rate * time / 100;
        }
        
        validateAccountNumber(accountNumber) {
            return accountNumber.length >= 10 && /^\d+$/.test(accountNumber);
        }
    }
    
    // Register component
    const component = await adapter.registerComponent({
        component_id: 'banking_calculator',
        component_name: 'Banking Calculator',
        version: '1.0.0'
    }, new BankingCalculator());
    
    // Invoke methods
    const interest = await adapter.invoke('banking_calculator', 'calculateInterest', 
                                        [1000, 5.0, 2.0]);
    console.log('Interest:', interest);
    
    const isValid = await adapter.invoke('banking_calculator', 'validateAccountNumber',
                                       ['1234567890']);
    console.log('Account valid:', isValid);
    
    // Cleanup
    await adapter.cleanup();
}

main().catch(console.error);
```

### 4. Python Bridge Usage

```python
import asyncio
from polycall_dop import DOPAdapter, DOPIsolationLevel, DOPComponentConfig

class DataProcessor:
    def process_transaction(self, transaction_data):
        # Process transaction data
        return {
            'processed': True,
            'transaction_id': transaction_data.get('id'),
            'amount': transaction_data.get('amount', 0) * 1.02  # Add processing fee
        }
    
    def validate_data(self, data):
        required_fields = ['id', 'amount', 'currency']
        return all(field in data for field in required_fields)

async def main():
    # Initialize adapter
    adapter = DOPAdapter()
    await adapter.initialize({
        'security_policy': {
            'isolation_level': DOPIsolationLevel.STANDARD,
            'max_memory_usage': 32 * 1024 * 1024,
            'max_execution_time_ms': 10000
        }
    })
    
    # Register component
    config = DOPComponentConfig(
        component_id='data_processor',
        component_name='Data Processor',
        version='1.0.0'
    )
    
    component = await adapter.register_component(config, DataProcessor())
    
    # Test invocation
    test_transaction = {
        'id': 'TXN001',
        'amount': 100.0,
        'currency': 'USD'
    }
    
    result = await adapter.invoke('data_processor', 'process_transaction', 
                                [test_transaction])
    print('Processed transaction:', result)
    
    # Cleanup
    await adapter.cleanup()

if __name__ == '__main__':
    asyncio.run(main())
```

## 🔧 CLI Usage

### Basic Component Management

```bash
# Register a JavaScript banking component with strict isolation
./polycall micro bankcard_component --dop-adapter \
    --language=javascript \
    --isolation=strict \
    --memory=16 \
    --permissions=memory_read,memory_write

# Register Python data processing component
./polycall micro data_processor --dop-adapter \
    --language=python \
    --isolation=standard \
    --memory=32 \
    --timeout=10000

# Register untrusted ads service with minimal permissions
./polycall micro ads_service --dop-adapter \
    --language=javascript \
    --isolation=strict \
    --memory=8 \
    --permissions=memory_read

# List registered components
./polycall micro --dop-adapter --list

# Show adapter statistics
./polycall micro --dop-adapter --stats

# Load configuration from file
./polycall micro --dop-adapter --config=banking_components.json
```

### Configuration File Example

```json
{
  "components": [
    {
      "component_id": "payment_processor",
      "component_name": "Payment Processor",
      "language": "c",
      "version": "2.1.0",
      "security_policy": {
        "isolation_level": "standard",
        "allowed_permissions": ["memory_read", "memory_write", "invoke_local"],
        "max_memory_usage": 67108864,
        "max_execution_time_ms": 15000,
        "audit_enabled": true
      }
    },
    {
      "component_id": "ads_service",
      "component_name": "Advertisement Service",
      "language": "javascript",
      "version": "1.0.0",
      "security_policy": {
        "isolation_level": "strict",
        "allowed_permissions": ["memory_read"],
        "max_memory_usage": 8388608,
        "max_execution_time_ms": 2000,
        "audit_enabled": true
      }
    }
  ]
}
```

## 🏭 Build Configuration

### CMake Options

```bash
# Core DOP Adapter (always built)
cmake .. -DBUILD_POLYCALL_DOP_ADAPTER=ON

# Language bridges
cmake .. -DBUILD_DOP_BRIDGE_JS=ON
cmake .. -DBUILD_DOP_BRIDGE_PYTHON=ON
cmake .. -DBUILD_DOP_BRIDGE_JVM=ON

# Security options
cmake .. -DPOLYCALL_DOP_ZERO_TRUST=ON
cmake .. -DPOLYCALL_DOP_STRICT_ISOLATION=ON

# Development options
cmake .. -DBUILD_DOP_ADAPTER_TESTS=ON
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Build Targets

```bash
# Build core library
make polycall_core_dop_adapter_static
make polycall_core_dop_adapter_shared

# Build language bridges
make polycall_dop_bridge_js
make polycall_dop_bridge_python

# Build tests
make test_dop_adapter
make dop_adapter_tests

# Install
make install
```

## 🧪 Testing

### Running Tests

```bash
# Build and run all tests
make test

# Run specific test suites
./build/tests/test_dop_adapter
./build/tests/test_dop_security
./build/tests/test_dop_memory

# Run with verbose output
./build/tests/test_dop_adapter --verbose

# Run performance tests
./build/tests/test_dop_performance
```

### Test Categories

1. **Unit Tests**
   - Core adapter functionality
   - Component registration/unregistration
   - Security policy validation
   - Memory management

2. **Integration Tests**
   - Cross-language communication
   - Bridge registration and lookup
   - Component method invocation

3. **Security Tests**
   - Zero Trust enforcement
   - Isolation boundary validation
   - Permission violation detection
   - Banking scenario testing (ads vs payment isolation)

4. **Performance Tests**
   - Component creation/destruction speed
   - Memory allocation performance
   - Invocation throughput
   - Concurrent component handling

## 📚 API Reference

### Core Types

```c
// Error codes
typedef enum {
    POLYCALL_DOP_SUCCESS = 0,
    POLYCALL_DOP_ERROR_INVALID_PARAMETER,
    POLYCALL_DOP_ERROR_INVALID_STATE,
    POLYCALL_DOP_ERROR_MEMORY_ALLOCATION,
    POLYCALL_DOP_ERROR_SECURITY_VIOLATION,
    POLYCALL_DOP_ERROR_PERMISSION_DENIED,
    POLYCALL_DOP_ERROR_COMPONENT_NOT_FOUND,
    POLYCALL_DOP_ERROR_BRIDGE_UNAVAILABLE,
    POLYCALL_DOP_ERROR_ISOLATION_BREACH,
    POLYCALL_DOP_ERROR_INVOKE_FAILED,
    POLYCALL_DOP_ERROR_LIFECYCLE_VIOLATION
} polycall_dop_error_t;

// Component states
typedef enum {
    POLYCALL_DOP_COMPONENT_UNINITIALIZED = 0,
    POLYCALL_DOP_COMPONENT_INITIALIZING,
    POLYCALL_DOP_COMPONENT_READY,
    POLYCALL_DOP_COMPONENT_EXECUTING,
    POLYCALL_DOP_COMPONENT_SUSPENDED,
    POLYCALL_DOP_COMPONENT_ERROR,
    POLYCALL_DOP_COMPONENT_CLEANUP,
    POLYCALL_DOP_COMPONENT_DESTROYED
} polycall_dop_component_state_t;
```

### Core Functions

```c
// Adapter lifecycle
polycall_dop_error_t polycall_dop_adapter_initialize(
    polycall_core_context_t* core_ctx,
    polycall_dop_adapter_context_t** adapter_ctx,
    const polycall_dop_security_policy_t* default_policy
);

polycall_dop_error_t polycall_dop_adapter_cleanup(
    polycall_dop_adapter_context_t* adapter_ctx
);

// Component management
polycall_dop_error_t polycall_dop_component_register(
    polycall_dop_adapter_context_t* adapter_ctx,
    const polycall_dop_component_config_t* config,
    polycall_dop_component_t** component
);

polycall_dop_error_t polycall_dop_component_unregister(
    polycall_dop_adapter_context_t* adapter_ctx,
    polycall_dop_component_t* component
);

// Component invocation
polycall_dop_error_t polycall_dop_invoke(
    polycall_dop_adapter_context_t* adapter_ctx,
    const char* component_id,
    const char* method_name,
    const polycall_dop_value_t* parameters,
    size_t parameter_count,
    polycall_dop_result_t* result
);

// Security functions
polycall_dop_error_t polycall_dop_security_validate(
    polycall_dop_adapter_context_t* adapter_ctx,
    polycall_dop_component_t* component,
    const char* operation
);

// Memory management
polycall_dop_error_t polycall_dop_memory_allocate(
    polycall_dop_adapter_context_t* adapter_ctx,
    polycall_dop_component_t* component,
    size_t size,
    polycall_dop_permission_flags_t permissions,
    polycall_dop_memory_region_t** region
);
```

## 🔍 Debugging and Monitoring

### Audit Logging

All component operations are logged for security auditing:

```c
// Enable detailed audit logging
polycall_dop_security_policy_t policy = {
    .audit_enabled = true,
    .isolation_level = POLYCALL_DOP_ISOLATION_STRICT
};

// Audit events include:
// - Component creation/destruction
// - Method invocations
// - Security violations
// - Memory allocations
// - Permission checks
```

### Statistics and Monitoring

```c
// Get component statistics
polycall_dop_component_stats_t stats;
polycall_dop_component_get_stats(adapter_ctx, component, &stats);

printf("Invocations: %lu\n", stats.invocation_count);
printf("Execution time: %lu ns\n", stats.total_execution_time_ns);
printf("Memory usage: %lu bytes\n", stats.current_memory_usage);
printf("Security violations: %lu\n", stats.security_violations);
```

## 🏦 Banking Application Integration

### Scenario: Payment Processing with Ads

```c
// Create payment component (high trust)
polycall_dop_component_config_t payment_config = {
    .component_id = "payment_processor",
    .component_name = "Payment Processor",
    .language = POLYCALL_DOP_LANGUAGE_C,
    .security_policy = {
        .isolation_level = POLYCALL_DOP_ISOLATION_STANDARD,
        .allowed_permissions = POLYCALL_DOP_PERMISSION_MEMORY_READ |
                              POLYCALL_DOP_PERMISSION_MEMORY_WRITE |
                              POLYCALL_DOP_PERMISSION_INVOKE_LOCAL,
        .max_memory_usage = 64 * 1024 * 1024,
        .max_execution_time_ms = 10000
    }
};

// Create ads component (low trust)
polycall_dop_component_config_t ads_config = {
    .component_id = "ads_service",
    .component_name = "Advertisement Service", 
    .language = POLYCALL_DOP_LANGUAGE_JAVASCRIPT,
    .security_policy = {
        .isolation_level = POLYCALL_DOP_ISOLATION_STRICT,
        .allowed_permissions = POLYCALL_DOP_PERMISSION_MEMORY_READ,
        .max_memory_usage = 8 * 1024 * 1024,
        .max_execution_time_ms = 2000
    }
};

// Register both components
polycall_dop_component_t* payment_component;
polycall_dop_component_t* ads_component;

polycall_dop_component_register(adapter_ctx, &payment_config, &payment_component);
polycall_dop_component_register(adapter_ctx, &ads_config, &ads_component);

// Process payment (trusted operation)
polycall_dop_result_t payment_result;
polycall_dop_invoke(adapter_ctx, "payment_processor", "process_payment",
                    payment_params, param_count, &payment_result);

// Show ads (untrusted operation - cannot access payment data)
polycall_dop_result_t ads_result;
polycall_dop_invoke(adapter_ctx, "ads_service", "show_contextual_ads",
                    ads_params, ads_param_count, &ads_result);
```

## 📋 Requirements

### System Requirements

- Linux/macOS/Windows
- CMake 3.16+
- GCC 9+ or Clang 10+
- LibPolyCall Core library

### Language Bridge Requirements

- **JavaScript**: Node.js 14+ with native addon support
- **Python**: Python 3.8+ with ctypes support
- **JVM**: Java 11+ with JNI support

## 🤝 Contributing

### Development Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/obinexuscomputing/libpolycall.git

# Setup development environment
./scripts/setup-dev.sh

# Build in debug mode
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_DOP_ADAPTER_TESTS=ON
make

# Run tests
make test
```

### Code Style

- Follow LibPolyCall coding standards
- Use IoC principles throughout
- Maintain Zero Trust security model
- Document all public APIs
- Include comprehensive tests

## 📄 License

LibPolyCall DOP Adapter is part of the LibPolyCall project and is licensed under the same terms as the main project.

## 🆘 Support

- **Documentation**: [LibPolyCall Docs](https://docs.libpolycall.org)
- **Issues**: [GitHub Issues](https://github.com/obinexuscomputing/libpolycall/issues)
- **Discussions**: [GitHub Discussions](https://github.com/obinexuscomputing/libpolycall/discussions)
- **Commercial Support**: support@obinexuscomputing.com

## 🗺️ Roadmap

### Current Version (1.0.0)
- ✅ Core DOP Adapter framework
- ✅ C, JavaScript, Python bridges
- ✅ Zero Trust security model
- ✅ Memory isolation
- ✅ CLI integration

### Upcoming Features (1.1.0)
- 🔄 JVM bridge (Java, Kotlin, Scala)
- 🔄 WebAssembly bridge
- 🔄 Distributed component support
- 🔄 Advanced monitoring and telemetry

### Future Releases
- 🔄 Rust bridge
- 🔄 Go bridge
- 🔄 Real-time component migration
- 🔄 Kubernetes operator
- 🔄 Visual component designer

---

**Built with ❤️ by OBINexus Computing**

*Enabling secure, scalable, cross-language component architectures for the next generation of applications.*