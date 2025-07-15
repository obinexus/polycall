#!/bin/bash
# tools/validate_purity.sh - OBINexus Command Purity Validator
# Ensures strict isolation between command modules

set -euo pipefail

COMMANDS="micro telemetry guid edge crypto topo"
VIOLATIONS=0

echo "=== OBINexus Command Purity Validation ==="
echo "Checking for cross-command dependencies..."

# Function to check for illegal includes
check_command_includes() {
    local cmd=$1
    local cmd_dir="src/core/commands/$cmd"
    
    if [[ ! -d "$cmd_dir" ]]; then
        echo "Warning: Command directory $cmd_dir not found"
        return
    fi
    
    echo "Checking $cmd command..."
    
    # Check all C and H files in command directory
    for file in "$cmd_dir"/*.{c,h}; do
        if [[ ! -f "$file" ]]; then
            continue
        fi
        
        # Check for includes from other commands
        for other_cmd in $COMMANDS; do
            if [[ "$other_cmd" == "$cmd" ]]; then
                continue
            fi
            
            # Search for illegal includes
            if grep -q "#include.*commands/$other_cmd" "$file" 2>/dev/null; then
                echo "  ❌ VIOLATION: $file includes from $other_cmd command"
                ((VIOLATIONS++))
            fi
            
            # Search for direct references to other command symbols
            if grep -qE "${other_cmd}_command|${other_cmd}_execute|${other_cmd}_init" "$file" 2>/dev/null; then
                echo "  ❌ VIOLATION: $file references $other_cmd command symbols"
                ((VIOLATIONS++))
            fi
        done
        
        # Verify only allowed includes
        allowed_patterns=(
            "base/"
            "protocol/"
            "network/"
            "auth/"
            "command_interface.h"
            "<std"
            "<sys/"
            "<unistd.h>"
            "<string.h>"
            "<errno.h>"
        )
        
        # Extract all includes
        includes=$(grep "^#include" "$file" 2>/dev/null || true)
        
        while IFS= read -r include; do
            if [[ -z "$include" ]]; then
                continue
            fi
            
            valid=0
            for pattern in "${allowed_patterns[@]}"; do
                if [[ "$include" =~ $pattern ]]; then
                    valid=1
                    break
                fi
            done
            
            if [[ $valid -eq 0 ]] && [[ "$include" =~ commands/ ]]; then
                echo "  ⚠️  Suspicious include in $file: $include"
            fi
        done <<< "$includes"
    done
}

# Check each command for violations
for cmd in $COMMANDS; do
    check_command_includes "$cmd"
done

echo ""
echo "=== Checking CLI command handlers ==="

# Verify CLI handlers don't directly call command implementations
for cli_file in src/cli/commands/cli_*.c; do
    if [[ ! -f "$cli_file" ]]; then
        continue
    fi
    
    echo "Checking $(basename "$cli_file")..."
    
    # Should only use registry_execute_command
    if grep -qE "micro_execute|telemetry_execute|guid_execute|edge_execute|crypto_execute|topo_execute" "$cli_file" 2>/dev/null; then
        echo "  ❌ VIOLATION: Direct command execution found"
        ((VIOLATIONS++))
    fi
    
    # Should use registry pattern
    if ! grep -q "registry_execute_command" "$cli_file" 2>/dev/null; then
        echo "  ⚠️  Warning: No registry usage detected"
    fi
done

echo ""
echo "=== Checking bridge implementations ==="

# Verify bridges don't reference commands
for bridge_file in src/core/bridges/*.c; do
    if [[ ! -f "$bridge_file" ]]; then
        continue
    fi
    
    echo "Checking $(basename "$bridge_file")..."
    
    for cmd in $COMMANDS; do
        if grep -qE "#include.*commands/$cmd|${cmd}_command|${cmd}_execute" "$bridge_file" 2>/dev/null; then
            echo "  ❌ VIOLATION: Bridge references $cmd command"
            ((VIOLATIONS++))
        fi
    done
done

echo ""
echo "=== Checking circular dependencies ==="

# Use nm to check for undefined symbols that might indicate circular deps
if command -v nm >/dev/null 2>&1; then
    for cmd in $COMMANDS; do
        lib="build/libpolycall_cmd_${cmd}.a"
        if [[ -f "$lib" ]]; then
            echo "Analyzing $cmd library symbols..."
            
            # Check for symbols from other commands
            undefined=$(nm -u "$lib" 2>/dev/null | grep -E "micro_|telemetry_|guid_|edge_|crypto_|topo_" || true)
            
            for other_cmd in $COMMANDS; do
                if [[ "$other_cmd" == "$cmd" ]]; then
                    continue
                fi
                
                if echo "$undefined" | grep -q "${other_cmd}_"; then
                    echo "  ❌ VIOLATION: $cmd depends on undefined ${other_cmd} symbols"
                    ((VIOLATIONS++))
                fi
            done
        fi
    done
fi

echo ""
echo "=== Summary ==="

if [[ $VIOLATIONS -eq 0 ]]; then
    echo "✅ Command purity validation PASSED"
    echo "All commands are properly isolated"
    exit 0
else
    echo "❌ Command purity validation FAILED"
    echo "Found $VIOLATIONS violation(s)"
    echo ""
    echo "To fix violations:"
    echo "1. Remove direct includes between commands"
    echo "2. Use the command registry for all command invocations"
    echo "3. Ensure commands only depend on base/protocol/network/auth layers"
    echo "4. Move shared functionality to appropriate infrastructure layers"
    exit 1
fi
