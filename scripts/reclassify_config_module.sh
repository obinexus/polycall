#!/bin/bash
# Reclassify config module as infrastructure for OBINexus PolyCall

set -euo pipefail

PROJECT_ROOT="${1:-.}"
cd "$PROJECT_ROOT"

echo "=== Reclassifying Config Module as Infrastructure ==="
echo "Project root: $PROJECT_ROOT"

# Step 1: Update CMakeLists.txt
echo "Updating CMakeLists.txt..."
if [ -f "CMakeLists.txt" ]; then
    # Move config from COMMANDS to infrastructure
    sed -i 's/set(COMMANDS micro telemetry guid edge crypto topo)/set(COMMANDS micro telemetry guid edge crypto topo auth network protocol accessibility)/' CMakeLists.txt
    
    # Add config to infrastructure libraries
    sed -i '/^# Base Infrastructure Library/,/^add_library/ {
        /set(BASE_SOURCES/,/)/ {
            s|)$|\n    src/core/config/config_parser.c\n    src/core/config/config_factory.c\n    src/core/config/config_container.c\n    src/core/config/config_error.c\n    src/core/config/config_registry.c\n)|
        }
    }' CMakeLists.txt
    
    echo "✓ Updated CMakeLists.txt"
fi

# Step 2: Update migration enforcer
echo "Updating migration enforcer..."
ENFORCER_SCRIPT="scripts/polycall_migration_enforcer.py"
if [ -f "$ENFORCER_SCRIPT" ]; then
    # Create a Python script to update the enforcer
    cat > /tmp/update_enforcer.py << 'EOF'
import sys
import re

with open(sys.argv[1], 'r') as f:
    content = f.read()

# Remove config from command modules
content = re.sub(
    r'(self\.command_modules = \{[^}]+)"config",?\s*',
    r'\1',
    content
)

# Add config to infrastructure modules
content = re.sub(
    r'(self\.infrastructure_modules = \{[^}]+)\}',
    r'\1, "config", "parser", "schema", "factory"}',
    content
)

with open(sys.argv[1], 'w') as f:
    f.write(content)
EOF

    python3 /tmp/update_enforcer.py "$ENFORCER_SCRIPT"
    rm /tmp/update_enforcer.py
    echo "✓ Updated migration enforcer"
fi

# Step 3: Create config infrastructure headers if missing
echo "Ensuring config infrastructure headers..."
mkdir -p include/polycall/config

# Create a base config header that modules can safely include
cat > include/polycall/config/base_config.h << 'EOF'
/*
 * base_config.h
 * Base configuration infrastructure for PolyCall
 * This is safe for all modules to include
 */

#ifndef POLYCALL_BASE_CONFIG_H
#define POLYCALL_BASE_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

typedef struct polycall_config polycall_config_t;

// Configuration API that all modules can use
polycall_config_t* polycall_config_create(void);
void polycall_config_destroy(polycall_config_t* config);
int polycall_config_set(polycall_config_t* config, const char* key, const char* value);
const char* polycall_config_get(polycall_config_t* config, const char* key);
bool polycall_config_get_bool(polycall_config_t* config, const char* key, bool default_value);
int polycall_config_get_int(polycall_config_t* config, const char* key, int default_value);

#endif /* POLYCALL_BASE_CONFIG_H */
EOF

echo "✓ Created base config header"

# Step 4: Fix module-specific config includes
echo "Fixing module-specific config includes..."

# Find all config-related includes and fix them
find src/core -name "*.c" -type f | while read -r file; do
    # Skip files in the config directory itself
    if [[ "$file" == *"/config/"* ]]; then
        continue
    fi
    
    # Check if file has config includes
    if grep -q "#include.*config" "$file"; then
        echo "  Checking: $file"
        
        # Create a temporary file
        tmp_file="${file}.tmp"
        
        # Replace config includes
        sed -E \
            -e 's|#include\s*["<]config/([^">]+)[">]|#include "polycall/config/\1"|g' \
            -e 's|#include\s*["<]\.\./(\.\./)*/config/|#include "polycall/config/|g' \
            -e 's|#include\s*["<]polycall_config\.h[">]|#include "polycall/config/base_config.h"|g' \
            "$file" > "$tmp_file"
        
        # Only update if changes were made
        if ! cmp -s "$file" "$tmp_file"; then
            mv "$tmp_file" "$file"
            echo "    ✓ Fixed includes in $(basename "$file")"
        else
            rm "$tmp_file"
        fi
    fi
done

# Step 5: Create a migration summary
echo
echo "=== Migration Summary ==="
echo "1. Config module reclassified as infrastructure"
echo "2. Command modules can now safely depend on config"
echo "3. Base config header created for common usage"
echo "4. Module includes have been updated"
echo
echo "Next steps:"
echo "1. Run: python3 scripts/fix_polycall_violations.py $PROJECT_ROOT"
echo "2. Run: ./scripts/enforce_command_purity.sh check"
echo "3. If all checks pass, run: make build"

# Create a marker file
echo "$(date): Config module reclassified as infrastructure" > .config_reclassified
