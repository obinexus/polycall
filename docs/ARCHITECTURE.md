# OBINexus PolyCall Architecture

## Module Classification (Updated)

### Infrastructure Modules
These form the foundation that command modules can depend on:

**Core Infrastructure**:
- `base` - Memory, error, context management
- `common` - Shared utilities
- `polycall` - Core runtime

**Configuration Layer**:
- `config` - Configuration management
- `parser` - Config parsing
- `schema` - Config schemas
- `factory` - Object factories

**Communication Layer**:
- `protocol` - Communication protocols
- `network` - Network communication
- `auth` - Authentication and security

**Service Layer**:
- `accessibility` - Accessibility services for all modules
- `repl` - REPL infrastructure used by commands
- `ffi` - Foreign Function Interface
- `bridges` - Language bridges
- `hotwire` - Hot-wiring subsystem

### Command Modules
Isolated modules that implement specific functionality:
- `micro` - Micro component management
- `telemetry` - Telemetry and metrics
- `guid` - GUID generation
- `edge` - Edge computing
- `crypto` - Cryptographic operations
- `topo` - Topology management
- `doctor` - System diagnostics
- `ignore` - Ignore file handling

## Dependency Rules
1. Infrastructure modules can depend on other infrastructure following the layer hierarchy
2. Command modules can depend on any infrastructure module
3. Command modules CANNOT depend on other command modules
4. No circular dependencies allowed

## Recent Changes
- Moved `accessibility` and `repl` to infrastructure (they provide services)
- Fixed include paths for proper module structure
- Resolved circular dependencies

Updated: $(date)
