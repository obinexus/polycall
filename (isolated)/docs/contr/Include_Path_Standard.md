# LibPolyCall Include Path Standardization Guide

## 1. Overview

This document establishes the technical standards for include path management in the LibPolyCall project. Following these conventions is essential for maintaining code integrity, preventing build errors, and ensuring consistent dependency management across the codebase.

## 2. Directory Structure

The LibPolyCall project follows a standardized directory structure:

```
libpolycall/
├── include/
│   └── polycall/             # Root include directory
│       ├── cli/              # Command-line interface components
│       ├── core/             # Core functionality modules
│       │   ├── auth/         # Authentication
│       │   ├── config/       # Configuration management
│       │   ├── edge/         # Edge computing components
│       │   ├── ffi/          # Foreign Function Interface
│       │   ├── micro/        # Microservices components
│       │   ├── network/      # Network functionality
│       │   ├── polycall/     # Core polycall functionality
│       │   ├── protocol/     # Protocol implementation
│       │   └── telemetry/    # Telemetry functionality
│       └── polycall_config.h.in # Project-wide configuration
└── src/
    ├── cli/                 # CLI implementation
    ├── core/                # Core implementation
    │   ├── auth/            # Auth implementation
    │   ├── config/          # Config implementation
    │   ├── edge/            # Edge implementation
    │   ├── ffi/             # FFI implementation
    │   ├── micro/           # Micro implementation
    │   ├── network/         # Network implementation
    │   ├── polycall/        # Core polycall implementation
    │   ├── protocol/        # Protocol implementation
    │   └── telemetry/       # Telemetry implementation
    └── polycall.c           # Main entry point
```

## 3. Include Path Standards

### 3.1. Path Format Specification

LibPolyCall enforces a strict include path format based on the module hierarchy:

#### Root-Level Includes
```c
#include "polycall/polycall_config.h"
#include "polycall/polycall.h"
```

#### Core Module Includes
```c
#include "polycall/core/polycall/polycall_context.h"
#include "polycall/core/auth/polycall_auth_context.h"
#include "polycall/core/config/global_config.h"
```

#### CLI Module Includes
```c
#include "polycall/cli/command.h"
#include "polycall/cli/repl.h"
#include "polycall/cli/commands/auth_commands.h"
```

### 3.2. Include Statement Ordering

Maintain a consistent ordering of include statements:

1. System headers (using angle brackets)
2. Third-party library headers (using angle brackets)
3. LibPolyCall headers (using quotes)

```c
// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <openssl/ssl.h>
#include <zlib.h>

// LibPolyCall headers
#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/auth/polycall_auth_context.h"
```

### 3.3. Non-compliant Patterns

The following include patterns are explicitly non-compliant:

| Non-compliant Pattern | Compliant Replacement |
|-----------------------|----------------------|
| `#include "polycall_error.h"` | `#include "polycall/core/polycall/polycall_error.h"` |
| `#include "core/polycall/error.h"` | `#include "polycall/core/polycall/polycall_error.h"` |
| `#include "auth/polycall_auth.h"` | `#include "polycall/core/auth/polycall_auth_context.h"` |
| `#include "network_client.h"` | `#include "polycall/core/network/network_client.h"` |

## 4. Implementation Guidelines

### 4.1. Header Files

When implementing header files:

- Use header guards that align with the include path
- Forward declare types where possible to minimize include dependencies
- Keep header files self-contained

```c
// polycall/core/auth/polycall_auth_context.h
#ifndef POLYCALL_CORE_AUTH_POLYCALL_AUTH_CONTEXT_H
#define POLYCALL_CORE_AUTH_POLYCALL_AUTH_CONTEXT_H

// Forward declarations
struct polycall_core_context;

// Public API declarations
polycall_result_t polycall_auth_initialize(struct polycall_core_context* ctx);

#endif /* POLYCALL_CORE_AUTH_POLYCALL_AUTH_CONTEXT_H */
```

### 4.2. Source Files

When implementing source files:

- Include the corresponding header file first
- Include dependent header files in order of dependency
- Use full paths for all includes

```c
// src/core/auth/auth_context.c
#include "polycall/core/auth/polycall_auth_context.h"  // Corresponding header first
#include "polycall/core/polycall/polycall_error.h"     // Other dependencies
#include "polycall/core/polycall/polycall_memory.h"
```

## 5. Validation Tools

LibPolyCall provides several tools to validate and fix include paths:

### 5.1. Makefile Targets

| Target | Description |
|--------|-------------|
| `validate` | Analyze include path issues without fixing |
| `validate-includes` | Validate include paths using CMake infrastructure |
| `fix` | Automatically fix include path issues |
| `dev-cycle` | Run complete workflow (build→validate→fix→rebuild) |

### 5.2. CMake Integration

Configure with include validation enabled:
```bash
cmake -DENABLE_INCLUDE_VALIDATION=ON ..
```

Use the validation target:
```bash
make validate-includes
```

### 5.3. Standalone Scripts

| Script | Purpose |
|--------|---------|
| `standardize_includes.py` | Automatically standardize include paths |
| `include_path_validator.py` | Validate and fix include paths from error logs |
| `validate_includes.py` | Comprehensive validation against standards |

Usage examples:
```bash
# Verify includes without making changes
python scripts/standardize_includes.py --dry-run

# Fix includes automatically
python scripts/standardize_includes.py

# Validate against the standard
python scripts/validate_includes.py --root . --standard polycall
```

## 6. Recommended Workflow

### 6.1. Development Workflow

1. Make code changes following the include path standards
2. Run `make validate-includes` to verify compliance
3. If issues are found, run `make fix` to automatically fix them
4. Rebuild with `make rebuild`
5. Review and test the changes

### 6.2. Integration CI Workflow

For continuous integration environments:
```bash
make ci-validation
```

This will perform validation steps suitable for automated environments.

## 7. Error Troubleshooting

### 7.1. Common Build Errors

| Error Pattern | Resolution |
|---------------|------------|
| `fatal error: polycall_error.h: No such file or directory` | Use the full path: `polycall/core/polycall/polycall_error.h` |
| `fatal error: ffi/ffi_core.h: No such file or directory` | Use the full path: `polycall/core/ffi/ffi_core.h` |

### 7.2. Error Analysis

If you encounter persistent include errors:

1. Run `make build` to generate error logs
2. Analyze with `make validate`
3. Apply fixes with `make fix`
4. If issues persist, manually review the error log: `build/release/build_errors.log`

## 8. Conclusion

Consistent include path management is critical for the LibPolyCall project's maintainability and build reliability. All contributors are expected to adhere to these standards to ensure code quality and prevent integration issues.

For additional information, please refer to the implementation details in the CMake modules and validation scripts.