# OBINexus LibPolyCall v2 - Naming Standards & Conventions

## Version Naming Convention

### Semantic Versioning Structure
```
<major>.<minor>.<patch>[-<prerelease>][+<build>]
```

### Examples
- **Development**: `2.0.0-dev+a1b2c3d`
- **Alpha**: `2.0.0-alpha.1+a1b2c3d`
- **Beta**: `2.0.0-beta.1+a1b2c3d`
- **Release Candidate**: `2.0.0-rc.1+a1b2c3d`
- **Stable**: `2.0.0+a1b2c3d`

### Codename Integration
- **Primary**: `v2.0.0-aegis`
- **Alpha**: `v2.0.0-alpha.1-aegis`
- **Beta**: `v2.0.0-beta.1-aegis`

## Branch Naming Convention

### Main Branches
- `main`: Stable production releases only
- `develop`: Integration branch for features
- `release/v2.0.0-aegis`: Release preparation branch

### Feature Branches
- `feature/polycall-ugly-gating`: Specific feature implementation
- `feature/ffi-interface-abstraction`: Component-specific development
- `feature/sinphase-optimization`: Architecture improvement

### Hotfix Branches
- `hotfix/v2.0.1-critical-fix`: Critical production fixes
- `hotfix/v2.0.1-security-patch`: Security-related fixes

### Experimental Branches
- `experimental/webassembly-integration`: Proof-of-concept work
- `experimental/performance-optimization`: Experimental improvements

## File and Directory Naming

### Source Code Files
- **C Source**: `snake_case.c` (e.g., `polycall_protocol.c`)
- **C Headers**: `snake_case.h` (e.g., `polycall_interface.h`)
- **Makefiles**: `Makefile.<purpose>` (e.g., `Makefile.build`)

### Configuration Files
- **Project Config**: `polycall.polycallfile`
- **Environment Config**: `polycall.<env>.cfg` (e.g., `polycall.prod.cfg`)
- **Schema Files**: `<component>_schema.json`

### Documentation Files
- **Architecture**: `PascalCase.md` (e.g., `ComponentArchitecture.md`)
- **Guides**: `kebab-case.md` (e.g., `installation-guide.md`)
- **Specifications**: `UPPERCASE.md` (e.g., `API_SPECIFICATION.md`)

### Script Files
- **Validation Scripts**: `##-kebab-case.sh` (e.g., `01-clear-bin.sh`)
- **Utility Scripts**: `kebab-case.sh` (e.g., `version-management.sh`)
- **Build Scripts**: `build-<purpose>.sh` (e.g., `build-release.sh`)

## Tag Naming Convention

### Release Tags
- **Stable**: `v2.0.0` or `v2.0.0-aegis`
- **Pre-release**: `v2.0.0-alpha.1` or `v2.0.0-alpha.1-aegis`

### Development Tags
- **Milestone**: `milestone/phase-2-complete`
- **Integration**: `integration/polycall-ugly-complete`

### Archive Tags
- **Legacy**: `legacy/v1.x-final`
- **Backup**: `backup/pre-realignment-20250716`

## Component Naming Standards

### Module Names
- **Core**: `polycall_core`, `polycall_protocol`, `polycall_state`
- **CLI**: `polycall_cli`, `polycall_main`, `polycall_args`
- **FFI**: `polycall_ffi_<language>` (e.g., `polycall_ffi_rust`)
- **Bindings**: `polycall_bindings_<language>` (e.g., `polycall_bindings_python`)

### Library Names
- **Shared**: `libpolycall.so`, `libpolycall_core.so`
- **Static**: `libpolycall.a`, `libpolycall_ffi.a`
- **Dynamic**: `polycall_runtime.dll` (Windows)

### Test Names
- **Unit**: `test_<module>_<function>.c`
- **Integration**: `integration_<component>_<scenario>.c`
- **Performance**: `perf_<operation>_benchmark.c`

## Consistency Guidelines

### Abbreviations
- **Configuration**: `config` (not `cfg` or `conf`)
- **Interface**: `interface` (not `iface` or `intf`)
- **Implementation**: `impl` (acceptable abbreviation)
- **Specification**: `spec` (acceptable abbreviation)

### Separators
- **Files**: Use underscores for C files, hyphens for scripts/docs
- **Directories**: Use hyphens for multi-word directories
- **Variables**: Use snake_case in C, camelCase in higher-level languages
- **Constants**: Use UPPER_SNAKE_CASE

### Language-Specific Conventions
- **C**: Follow ISO C11 standards with snake_case
- **Python**: Follow PEP 8 with snake_case
- **JavaScript**: Follow ESLint standards with camelCase
- **Rust**: Follow rustfmt standards with snake_case
- **Makefiles**: Use UPPER_CASE for variables, lowercase for targets

This naming standard ensures consistency across the entire OBINexus ecosystem
and facilitates automated tooling, documentation generation, and team collaboration.
