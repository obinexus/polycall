# Integration Guide for Polycall Ignore System

## Overview

This document outlines the implementation and integration approach for the Polycall ignore system, which provides functionality to exclude specific files and directories from being processed by the Polycall configuration system. The ignore system is inspired by the `.gitignore` pattern mechanism and follows a zero-trust architecture approach.

## Architecture Components

The implementation consists of the following key components:

1. **Core Ignore Framework** (`polycall_ignore`)
   - Generic pattern matching implementation
   - File loading and pattern management
   - Path normalization and testing

2. **Binding Configuration Integration** (`binding_config`)
   - Integration with `.polycallrc` configuration
   - Support for `.polycallrc.ignore` files
   - Path validation during configuration operations

3. **Polycallfile Integration** (`polycallfile_ignore`)
   - Specialized implementation for `config.Polycallfile`
   - Support for `config.Polycallfile.ignore` files
   - Default patterns for zero-trust security

## Directory Structure

```
src/core/config/
├── ignore/
│   ├── polycall_ignore.c
│   ├── polycall_ignore.h
│   └── CMakeLists.txt
├── polycallrc/
│   ├── binding_config.c
│   ├── binding_config.h
│   └── CMakeLists.txt
└── polycallfile/
    ├── ignore/
    │   ├── polycallfile_ignore.c
    │   ├── polycallfile_ignore.h
    │   └── CMakeLists.txt
    ├── ast.c
    ├── expression_evaluator.c
    ├── macro_expander.c
    ├── parser.c
    ├── token.c
    ├── tokenizer.c
    └── CMakeLists.txt
```

## Integration Steps

### 1. Core Context Integration

The ignore system integrates with the core context for memory management and error handling. Both `polycall_ignore` and `polycallfile_ignore` receive the core context during initialization:

```c
polycall_core_error_t polycall_ignore_context_init(
    polycall_core_context_t* core_ctx,
    polycall_ignore_context_t** ignore_ctx,
    bool case_sensitive
);
```

### 2. Build Integration

Update the main `CMakeLists.txt` to include the new ignore module:

```cmake
# In the main CMakeLists.txt
add_subdirectory(src/core/config/ignore)
target_link_libraries(polycall_core PRIVATE polycall_ignore)
```

Ensure the binding_config module links against the ignore module:

```cmake
# In src/core/config/polycallrc/CMakeLists.txt
target_link_libraries(polycall_rc PRIVATE polycall_ignore)
```

Add the polycallfile_ignore module to the polycallfile build:

```cmake
# In src/core/config/polycallfile/CMakeLists.txt
add_subdirectory(ignore)
target_link_libraries(polycallfile PRIVATE polycallfile_ignore)
```

### 3. Configuration System Integration

1. **Binding Configuration Integration**

   The binding configuration system now checks for `.polycallrc.ignore` files when loading/saving configuration files:

   ```c
   polycall_core_error_t polycall_binding_config_load(
       polycall_binding_config_context_t* config_ctx,
       const char* file_path
   ) {
       // Existing loading code...
       
       // Load ignore patterns if configured
       if (config_ctx->use_ignore_patterns && config_ctx->ignore_ctx) {
           // Find and load .polycallrc.ignore
           // ...
       }
       
       // Continue with loading process
   }
   ```

2. **Polycallfile Integration**

   The Polycallfile system performs similar integration, but with its specialized ignore system:

   ```c
   bool polycallfile_parser_should_process_file(
       polycallfile_parser_t* parser,
       const char* file_path
   ) {
       // Check if the file should be ignored
       if (parser->ignore_ctx && 
           polycallfile_ignore_should_ignore(parser->ignore_ctx, file_path)) {
           return false;
       }
       return true;
   }
   ```

### 4. Zero-Trust Security Integration

The ignore system plays a crucial role in the zero-trust architecture by preventing sensitive files from being processed by the configuration system:

1. Default patterns exclude credential files, secrets, and certificates
2. The system honors user-defined patterns for additional security constraints
3. Configuration paths are validated against ignore patterns before being written to

## Default Ignore Patterns

The system includes default ignore patterns that focus on security and operational concerns:

1. **Security-sensitive files**:
   - Certificate files (*.pem, *.crt, *.key)
   - Credential files (credentials.json, secrets.json)
   - Private key files

2. **Development artifacts**:
   - Version control directories (.git)
   - IDE configuration files
   - Compiled binaries and caches

3. **Temporary files**:
   - Backup files and logs
   - Temporary directories
   - System-specific files like .DS_Store

## Usage Recommendations

1. **Application Integration**

   When integrating the ignore system with application code:

   ```c
   // Initialize the ignore system
   polycallfile_ignore_context_t* ignore_ctx = NULL;
   polycallfile_ignore_init(core_ctx, &ignore_ctx, false);
   
   // Load default patterns and user patterns
   polycallfile_ignore_add_defaults(ignore_ctx);
   polycallfile_ignore_load_file(ignore_ctx, "config.Polycallfile.ignore");
   
   // Check files before processing
   if (!polycallfile_ignore_should_ignore(ignore_ctx, file_path)) {
       // Process the file
   }
   ```

2. **Custom Ignore Files**

   Create specialized ignore files for different environments:

   ```
   # Production.Polycallfile.ignore
   # Strict security settings for production
   **/test/
   **/mock/
   **/local_settings.json
   ```

3. **Validating Configurations**

   Always validate file paths against the ignore system before reading or writing:

   ```c
   if (should_read_config(file_path) && 
       !polycallfile_ignore_should_ignore(ignore_ctx, file_path)) {
       // Safe to read configuration
   }
   ```

## Debugging

The ignore system provides access to the loaded patterns for debugging purposes:

```c
size_t pattern_count = polycallfile_ignore_get_pattern_count(ignore_ctx);
printf("Loaded %zu ignore patterns:\n", pattern_count);

for (size_t i = 0; i < pattern_count; i++) {
    const char* pattern = polycallfile_ignore_get_pattern(ignore_ctx, i);
    printf("  %zu: %s\n", i, pattern);
}
```

## Next Steps

1. Enhance the ignore system to support hierarchical ignore files (similar to `.gitignore` in subdirectories)
2. Add pattern negation support with exclamation point prefix (`!pattern`)
3. Implement centralized ignore pattern management for the core Polycall runtime