# LibPolyCall Include Path Contribution Guide

## Overview

This guide establishes standards for include paths in the LibPolyCall project. Following these conventions is essential for maintaining a consistent codebase, preventing build errors, and ensuring compatibility across the project.

## Module Hierarchy

LibPolyCall uses a structured module hierarchy:

```
libpolycall/
├── include/
│   └── core/
│       ├── polycall/         # Core functionality modules
│       │   ├── auth/         # Authentication
│       │   ├── cli/          # Command line interface
│       │   ├── config/       # Configuration
│       │   ├── context/      # Context management
│       │   ├── error/        # Error handling
│       │   └── memory/       # Memory management
│       ├── ffi/              # Foreign Function Interface
│       ├── micro/            # Microservices components
│       ├── edge/             # Edge computing components
│       └── protocol/         # Protocol implementation
└── src/
    ├── core/                 # Core implementation
    │   ├── polycall/         # Core implementation
    │   ├── ffi/              # FFI implementation 
    │   ├── micro/            # Microservices implementation
    │   ├── edge/             # Edge computing implementation
    │   └── protocol/         # Protocol implementation
    ├── network/              # Network implementation
    └── parser/               # Parser implementation
```

## Include Path Standards

### 1. Standard Include Path Format

Use these formats for include statements:

#### Core Modules
```c
#include "core/polycall/auth/polycall_auth.h"
#include "core/polycall/cli/polycall_cli.h"
#include "core/polycall/error/polycall_error.h"
```

#### Primary Modules
```c
#include "core/ffi/ffi_core.h"
#include "core/micro/compute_engine.h"
#include "core/edge/edge_router.h"
#include "core/protocol/protocol_context.h"
```

#### Other Modules
```c
#include "network/network_client.h"
#include "parser/message_parser.h"
```

### 2. Include Ordering

Maintain consistent ordering of includes:

1. Standard library includes first (alphabetical order)
2. Third-party library includes (alphabetical order)
3. Project-specific includes (hierarchical order)

```c
// Standard library includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party includes
#include <openssl/ssl.h>
#include <zlib.h>

// Project includes
#include "core/polycall/polycall_core.h"
#include "core/polycall/auth/polycall_auth.h"
#include "core/ffi/ffi_core.h"
```

### 3. Common Pitfalls to Avoid

❌ **Incorrect** | ✅ **Correct**
--------------- | --------------
`#include "polycall_error.h"` | `#include "core/polycall/polycall_error.h"`
`#include "core/core/polycall/file.h"` | `#include "core/polycall/file.h"`
`#include "polycall/auth/auth.h"` | `#include "core/polycall/auth/auth.h"`
`#include "ffi/ffi_core.h"` | `#include "core/ffi/ffi_core.h"`
`#include "micro/compute.h"` | `#include "core/micro/compute.h"`

## Automated Tools

### Include Path Standardizer

We provide a tool to help maintain consistent include paths:

```bash
# Scan and report include path issues without making changes
./standardize_includes.py --dry-run

# Fix include paths automatically
./standardize_includes.py

# Show this guide
./standardize_includes.py --show-guide
```

### Pre-commit Hook

We recommend setting up a pre-commit hook to check include paths:

1. Save this script as `.git/hooks/pre-commit`:

```bash
#!/bin/bash
./standardize_includes.py --verify
if [ $? -ne 0 ]; then
  echo "ERROR: Include path issues detected!"
  echo "Run './standardize_includes.py' to fix them"
  exit 1
fi
```

2. Make it executable:

```bash
chmod +x .git/hooks/pre-commit
```

## CMake Integration

Include path validation is integrated into our CMake build system:

```cmake
# In your CMakeLists.txt:
include(IncludePathValidation)

# Validate include paths for a specific target
polycall_validate_include_paths(TARGET your_target)

# Validate all targets
polycall_validate_all_targets()

# Add a custom validation target
add_include_path_validation_target()
```

## Tips for Development

### Adding New Files

When creating new source or header files:

1. **Header Files**: Place in the appropriate directory under `include/core/`
2. **Source Files**: Place in the corresponding directory under `src/core/`
3. **Include Guards**: Use standardized include guards:

```c
#ifndef CORE_POLYCALL_AUTH_POLYCALL_AUTH_H
#define CORE_POLYCALL_AUTH_POLYCALL_AUTH_H

// Header content here

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible declarations

#ifdef __cplusplus
}
#endif

#endif /* CORE_POLYCALL_AUTH_POLYCALL_AUTH_H */
```

### Including Your Own Headers

When including headers from your own module:

```c
// In src/core/polycall/auth/polycall_auth.c
#include "core/polycall/auth/polycall_auth.h"  // Include your own header first
// Then other includes
```

### Testing Include Paths

Run the CMake validation target to ensure correct include paths:

```bash
# Build the validation target
cmake --build . --target validate-includes
```

## Cross-Platform Considerations

On Windows, use forward slashes in include paths:

```c
// Cross-platform compatible
#include "core/polycall/auth/polycall_auth.h"

// NOT compatible with all platforms
#include "core\\polycall\\auth\\polycall_auth.h"
```

## When to Seek Exceptions

In rare cases, you may need a non-standard include approach:

1. **Platform-specific Code**: Use conditional compilation
2. **External API Compatibility**: Maintain legacy includes for API stability
3. **Complex Third-party Integration**: May require special handling

If you need an exception to these standards, please discuss with the core team.

## FAQ

### Q: Why do we need standardized include paths?

A: Standardized include paths ensure:
- Consistent code organization
- Elimination of path-related build errors
- Easier navigation for developers
- Compatibility with the build system

### Q: How do I include headers from other modules?

A: Always use the full path from the root of the include directory:

```c
// Even if you're in core/micro, to include something from core/ffi:
#include "core/ffi/ffi_core.h"
```

### Q: What if I need to refactor module organization?

A: Major reorganization requires:
1. Proposal to the core team
2. Update to this contribution guide
3. Migration plan for existing code
4. Updates to the build system

## Conclusion

Following these include path standards will help maintain consistency and quality in the LibPolyCall codebase. When in doubt, run the standardization tool or ask a team member for guidance.