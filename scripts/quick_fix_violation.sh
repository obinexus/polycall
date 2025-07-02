#!/bin/bash
# Quick fix for OBINexus PolyCall module classification violations

set -euo pipefail

PROJECT_ROOT="${1:-.}"
cd "$PROJECT_ROOT"

echo "=== Quick Fix for Module Classification ==="

# Step 1: Fix the core/core directory issue first
if [ -d "src/core/core" ]; then
    echo "Fixing core/core directory structure..."
    
    # Move directories up one level
    for dir in src/core/core/*/; do
        if [ -d "$dir" ]; then
            dirname=$(basename "$dir")
            dest="src/core/$dirname"
            
            if [ -d "$dest" ]; then
                # Merge with existing
                echo "  Merging $dirname..."
                cp -r "$dir"* "$dest/" 2>/dev/null || true
            else
                # Move entire directory
                echo "  Moving $dirname..."
                mv "$dir" "$dest"
            fi
        fi
    done
    
    # Remove empty core/core
    rmdir src/core/core 2>/dev/null || true
    echo "✓ Fixed directory structure"
fi

# Step 2: Update the migration enforcer script directly
echo "Updating migration enforcer..."

ENFORCER="scripts/polycall_migration_enforcer.py"

if [ -f "$ENFORCER" ]; then
    # Create a temporary Python script to update the enforcer
    cat > /tmp/fix_enforcer.py << 'PYTHON_EOF'
import sys
import re

with open(sys.argv[1], 'r') as f:
    content = f.read()

# Update command modules - remove protocol, network, auth
content = re.sub(
    r'self\.command_modules = \{[^}]+\}',
    '''self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo",
            "accessibility", "repl", "ignore", "doctor"
        }''',
    content,
    flags=re.DOTALL
)

# Update infrastructure modules - add protocol, network, auth
content = re.sub(
    r'self\.infrastructure_modules = \{[^}]+\}',
    '''self.infrastructure_modules = {
            "base", "polycall", "common", "memory", "error", "context",
            "config", "parser", "schema", "factory",
            "protocol", "network", "auth", "ffi", "bridges", "hotwire"
        }''',
    content,
    flags=re.DOTALL
)

with open(sys.argv[1], 'w') as f:
    f.write(content)

print("✓ Updated module classifications")
PYTHON_EOF

    python3 /tmp/fix_enforcer.py "$ENFORCER"
    rm /tmp/fix_enforcer.py
fi

# Step 3: Create ARCHITECTURE.md
echo "Creating architecture documentation..."

cat > ARCHITECTURE.md << 'EOF'
# OBINexus PolyCall Architecture

## Module Classification

### Infrastructure Modules (Layered Dependencies)

**Layer 0 - Base** (no dependencies):
- `base` - Core memory, error, context management
- `common` - Shared utilities
- `polycall` - Core runtime

**Layer 1 - Config** (depends on Layer 0):
- `config` - Configuration management
- `parser` - Config parsing
- `schema` - Config schemas
- `factory` - Object factories

**Layer 2 - Protocol** (depends on Layers 0-1):
- `protocol` - Communication protocols

**Layer 3 - Network** (depends on Layers 0-2):
- `network` - Network communication, connection pools

**Layer 4 - Auth** (depends on Layers 0-3):
- `auth` - Authentication, authorization, security

**Additional Infrastructure**:
- `ffi` - Foreign Function Interface
- `bridges` - Language bridges
- `hotwire` - Hot-wiring subsystem

### Command Modules (Isolated)
These can depend on ANY infrastructure module but NOT on each other:
- `micro` - Micro component management
- `telemetry` - Telemetry and metrics
- `guid` - GUID generation and tracking
- `edge` - Edge computing features
- `crypto` - Cryptographic operations
- `topo` - Topology management
- `accessibility` - Accessibility features
- `repl` - REPL interface
- `doctor` - System diagnostics
- `ignore` - Ignore file handling

## Dependency Rules
1. Infrastructure modules follow layer hierarchy
2. Command modules can use any infrastructure
3. Command modules CANNOT depend on other commands
4. No circular dependencies allowed

Updated: $(date)
EOF

echo "✓ Created ARCHITECTURE.md"

# Step 4: Run validation
echo ""
echo "Running validation..."
if python3 scripts/polycall_migration_enforcer.py . --check-only; then
    echo ""
    echo "✅ All violations fixed!"
    echo ""
    echo "Next steps:"
    echo "1. Review ARCHITECTURE.md for the module structure"
    echo "2. Run: make clean && make build"
    echo "3. Test: ./build/bin/polycall help"
else
    echo ""
    echo "⚠️  Some issues may remain - likely missing source files"
    echo "Run the full migration to generate missing files:"
    echo "  ./scripts/enforce_command_purity.sh full"
fi

# Create a summary file
cat > module_fix_summary.txt << EOF
OBINexus Module Classification Fix
==================================
Date: $(date)

Changes Applied:
1. Reclassified protocol, network, auth as infrastructure
2. Fixed core/core directory structure
3. Updated migration enforcer configuration
4. Created architecture documentation

Infrastructure Modules:
- base, common, polycall (Layer 0)
- config, parser, schema, factory (Layer 1)  
- protocol (Layer 2)
- network (Layer 3)
- auth (Layer 4)
- ffi, bridges, hotwire

Command Modules:
- micro, telemetry, guid, edge, crypto, topo
- accessibility, repl, doctor, ignore

Dependency Rule:
Commands can depend on infrastructure, but not on each other.
EOF

echo ""
echo "Summary saved to: module_fix_summary.txt"
