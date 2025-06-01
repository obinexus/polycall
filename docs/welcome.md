# LibPolyCall: Technical Documentation

## Introduction

Welcome to the LibPolyCall documentation. This comprehensive guide provides detailed instructions for implementing and utilizing LibPolyCall in your server environment. LibPolyCall enables seamless communication between services written in various programming languages through a unified protocol-based architecture, adhering to a program-first approach to interface design.

## Core Architecture

LibPolyCall implements a hierarchical state management system with inheritance, composition, and permission propagation for complex protocol state machines. The library follows a program-first design philosophy, focusing on protocol implementation rather than language-specific bindings.

### Key Features

- **Program-primary interface design**: Core functionality resides in the protocol, not in language-specific bindings
- **Hierarchical state management**: Advanced state control with inheritance and composition
- **Permission propagation**: Configurable permission inheritance models across component hierarchy
- **Cross-language compatibility**: Unified communication between multiple programming languages
- **Stateless architecture**: Clean separation between components with RESTful principles

## Installation and Configuration

### Basic Setup

1. Create the necessary configuration directories:

```bash
sudo mkdir -p /opt/polycall/services/{node,python,java,go}
```

2. Create the main configuration file in your project root:

```
# In config.Polycallfile
server node 8080:8084
server python 3001:8084
server java 3002:8082
server go 3003:8083

# Network Configuration
network start
network_timeout=5000
max_connections=1000

# Global Settings
log_directory=/var/log/polycall
workspace_root=/opt/polycall

# Security Configuration
tls_enabled=true
cert_file=/etc/polycall/cert.pem
key_file=/etc/polycall/key.pem

# Resource Limits
max_memory_per_service=1G
max_cpu_per_service=2
```

3. Create language-specific configurations:

```
# In /opt/polycall/services/node/.polycallrc
port=8080:8084
server_type=node
workspace=/opt/polycall/services/node
log_level=info
max_connections=100
supports_diagnostics=true
max_memory=512M
timeout=30
```

## Running LibPolyCall

### Non-Interactive Mode

Start the PolyCall service as a background process:

```bash
./bin/polycall -f config.Polycallfile
```

Start your language-specific binding server:

```bash
cd ../bindings/node-polycall/examples
node server.js
```

### Interactive Mode (REPL)

For direct interaction with the LibPolyCall system:

```bash
./bin/polycall
```

At the prompt, you can issue commands:

```
PolyCall CLI v1.0.0 - Type 'help' for commands
> start_network
> list_endpoints
> status
```

## Data Isolation with Micro Command

The `micro` command provides component-level data isolation, enabling strict separation between critical and non-critical services.

### Implementation Example

```
# In config.Polycallfile
micro bankcard {
  port=3005:8085
  data_scope=isolated
  allowed_connections=payment_gateway,user_db
  max_memory=512M
  tls_enforced=true
}

micro ads_service {
  port=3006:8086
  data_scope=restricted
  allowed_connections=ad_server
  max_memory=256M
  tls_enforced=false
}
```

### Technical Details

The `micro` command establishes isolated memory regions with specific permissions:

```c
polycall_memory_region_t* region = polycall_memory_create_region(
    core_ctx,
    pool,
    size,
    POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE,
    POLYCALL_MEMORY_FLAG_ISOLATED,
    "bankcard"
);
```

This creates strict memory boundaries that prevent unauthorized access between components, providing a robust security model for sensitive data.

## Zero Trust Authentication

LibPolyCall implements Zero Trust principles - never trust, always verify - through hierarchical state-based authentication.

### Authentication Flow

1. **Initial State**: All connections begin in an unauthenticated state with minimal permissions
2. **Authentication**: Credentials are verified through cryptographic protocols
3. **Permission Resolution**: Upon successful authentication, the state system resolves appropriate permissions
4. **Continuous Verification**: Every operation requires permission verification

### Implementation Details

```c
// Configure hierarchical state for authentication
polycall_hierarchical_state_config_t auth_state = {
    .name = "authenticated",
    .relationship = POLYCALL_STATE_RELATIONSHIP_PARENT,
    .parent_state = "connection_established",
    .inheritance_model = POLYCALL_PERMISSION_INHERIT_ADDITIVE,
    .permissions = { PERMISSION_READ, PERMISSION_WRITE },
    .permission_count = 2
};

// Add state to the state machine
polycall_hierarchical_state_add(
    core_ctx,
    hsm_ctx,
    &auth_state
);

// Verify permissions for every operation
bool has_permission = polycall_hierarchical_state_has_permission(
    core_ctx,
    hsm_ctx,
    "authenticated",
    PERMISSION_WRITE
);
```

## Language Binding Integration

### Node.js Example

```javascript
const PolyCall = require('polycall');

const client = new PolyCall.Client({
  port: 8084,
  hostname: 'localhost',
  timeout: 5000
});

async function connectAndSend() {
  await client.connect();
  
  const response = await client.sendCommand('/books', {
    title: 'Technical Architecture',
    author: 'Nnamdi Okpala'
  });
  
  console.log('Response:', response);
}

connectAndSend().catch(console.error);
```

### Python Example

```python
import polycall

client = polycall.Client(
    port=8084,
    hostname='localhost',
    timeout=5000
)

async def connect_and_send():
    await client.connect()
    
    response = await client.send_command('/books', {
        'title': 'Technical Architecture',
        'author': 'Nnamdi Okpala'
    })
    
    print('Response:', response)

if __name__ == "__main__":
    import asyncio
    asyncio.run(connect_and_send())
```

## Advanced State Management

The hierarchical state system enables complex state transitions with permission inheritance:

```c
// Define state relationships
polycall_hierarchical_transition_config_t transition = {
    .name = "to_auth",
    .from_state = "handshake",
    .to_state = "auth",
    .type = POLYCALL_HTRANSITION_EXTERNAL,
    .guard = auth_guard_function
};

// Add transition to state machine
polycall_hierarchical_state_add_transition(
    core_ctx,
    hsm_ctx,
    &transition
);

// Execute transition
polycall_hierarchical_state_execute_transition(
    core_ctx,
    hsm_ctx,
    "to_auth"
);
```

## Monitoring and Verification

To verify your LibPolyCall installation:

```bash
# Check running services
ps aux | grep polycall

# Monitor port mappings
netstat -tulpn | grep polycall

# Check logs for errors
tail -f /var/log/polycall/polycall.log
```

## Conclusion

This technical documentation provides an overview of LibPolyCall's capabilities and implementation details. For comprehensive API documentation, refer to the header files included in the source distribution. For additional assistance, please contact technical support at nnamdi@obinexuscomputing.com.

These technical articles provide comprehensive documentation for LibPolyCall, covering several key aspects of its architecture and implementation. Each article focuses on a specific component of the system, offering detailed implementation guidance and code examples.

Is there anything specific about these technical articles you'd like me to clarify or expand upon? Or would you like me to create additional documentation for other aspects of LibPolyCall?
