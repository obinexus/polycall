# LibPolyCall

LibPolyCall is a powerful cross-language communication framework designed to facilitate seamless integration between different programming languages, microservices, and edge computing environments.

## Overview

LibPolyCall provides a unified interface for cross-language function calls, resource management, and distributed computing, allowing developers to build robust, secure, and efficient applications that leverage multiple programming languages and computing environments.

Version: 1.1.0

## Key Features

- **Cross-Language Communication**: Seamless FFI (Foreign Function Interface) between different programming languages
- **Resource Management**: Comprehensive tracking and limiting of memory, CPU, and I/O usage
- **Micro Command Architecture**: Lightweight components for microservice environments
- **Edge Computing Support**: Specialized components for edge computing scenarios
- **Protocol Implementation**: Standardized communication protocols
- **Network Integration**: Robust networking capabilities
- **Security Features**: Built-in security patterns and isolation mechanisms

## Architecture

LibPolyCall is built around a modular architecture:

- **Core Module**: Foundation layer providing context management, error handling, and memory management
- **FFI Module**: Foreign Function Interface for cross-language communication
- **Protocol Module**: Communication protocol implementations
- **Network Module**: Network communication capabilities
- **Micro Command**: Lightweight command execution framework for microservices
- **Edge Command**: Specialized command framework for edge computing environments

## Installation

### Prerequisites

- C11-compatible compiler
- CMake 3.13 or higher
- pthreads library

```bash
git clone https://github.com/obinexus/libpolycall.git
cd libpolycall
mkdir build && cd build
cmake ..
make
make install
```

## Technical Specification: PolyCall Protocol

### Introduction
The PolyCall Protocol is a language-agnostic middleware system developed by OBINexus Computing. It acts as a proxy service designed to mediate, standardize, and optimize cross-language function calls using a polling-based architecture. The system accommodates both interactive and non-interactive use cases through configuration-driven and REPL (Read-Eval-Print Loop) modes.

### Core Concepts
- **Middleware Proxy:** PolyCall acts as a middle layer between the client (end user or machine) and the target service (written in C, Python, Go, Java, etc.).
- **Polling Engine:** Instead of waiting for direct function calls, the system uses a continuous polling loop to detect and respond to changes in requests or data availability.
- **Unified Communication Layer:** Abstracts the differences between programming languages by using a declarative configuration system and a stateless message-passing model.

### Workflow Overview
The full lifecycle from end-user request to response delivery involves:

**Step 1: End-User Initiates a Request**
- An end-user invokes a command or submits a structured request (via the CLI, REPL, or a configuration file).
- The request is serialized and pushed to the polling engine via a queue-like interface.

**Step 2: Polling Middleware Detects Changes**
- The middleware continuously polls for changes in:
    - Configuration files (`.polycallrc`, `Polycallfile`)
    - Registered endpoints
    - Queued commands
- When a change is detected, the middleware routes the message to the appropriate subsystem (micro, edge, protocol, etc.).

**Step 3: Protocol Dispatching**
- The `protocol_commands` module handles dispatching based on defined port mappings and capabilities.
- It interprets the request according to the `.polycallrc` adapter for the destination language.
- If the adapter requires dynamic linking, PolyCall resolves and loads the required bridge (e.g., Python, JVM, etc.).

**Step 4: Execution and Monitoring**
- The target function is invoked.
- Middleware telemetry systems monitor the state (e.g., latency, response, error code).
- Results are serialized and pushed back to the originating context (e.g., client REPL, file, socket).

**Step 5: Response Delivery and State Update**
- Results are returned through the same polling cycle.
- The originating context is notified.
- PolyCall optionally updates config/state files if auto-sync is enabled.

### Protocol and Configuration Files
- **Polycallfile:** Contains global server definitions, port mappings, TLS settings, and network constraints.
- **.polycallrc:** Defines per-language runtime settings such as:
    - Execution permissions
    - Memory and CPU quotas
    - Endpoint behavior
    - Bridge loader settings

### Proxy Behavior
- **Stateless Proxying:** Each function call is stateless. Previous requests do not impact current logic.
- **Security Features:** Supports zero-trust validation, role isolation (micro), and encryption.
- **Dynamic Resolution:** Bridges are resolved only when a request is routed, ensuring low idle cost.

### Key Modules
- `protocol_commands.c`: Command dispatcher
- `network_commands.c`: Network setup and endpoint listing
- `micro_commands.c`: Micro-isolation command handler
- `polycall_protocol_context.c`: Manages context for each communication round
- `telemetry_commands.c`: Logs, metrics, and diagnostics

### Summary
PolyCall functions as a proxy system using middleware polling to detect, process, and return cross-language requests. The design ensures low latency, isolated execution, and secure communication between heterogeneous systems. It supports both human and machine interfaces, enabling scalable and dynamic interoperability for modern distributed applications.
