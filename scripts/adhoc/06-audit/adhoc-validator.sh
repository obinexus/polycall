#!/bin/bash
# Ad-hoc Script Validator
# Validates scripts against Sinphasé compliance rules

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

validate_script() {
    local script_path="$1"
    local script_name="$(basename "$script_path")"
    
    echo "Validating: $script_name"
    
    # Check file exists
    if [ ! -f "$script_path" ]; then
        echo "  ❌ File not found"
        return 1
    fi
    
    # Check permissions
    if [ ! -r "$script_path" ]; then
        echo "  ❌ Not readable"
        return 1
    fi
    
    # Language-specific validation
    if [[ "$script_path" == *.py ]]; then
        # Python validation
        if python3 -m py_compile "$script_path" 2>/dev/null; then
            echo "  ✓ Python syntax valid"
        else
            echo "  ❌ Python syntax error"
            return 1
        fi
    elif [[ "$script_path" == *.sh ]]; then
        # Shell validation
        if bash -n "$script_path" 2>/dev/null; then
            echo "  ✓ Shell syntax valid"
        else
            echo "  ❌ Shell syntax error"
            return 1
        fi
    fi
    
    # Check for required headers
    if grep -q "OBINexus\|Sinphasé\|Aegis" "$script_path"; then
        echo "  ✓ Project headers found"
    else
        echo "  ⚠ Missing project headers"
    fi
    
    return 0
}

# Main validation loop
main() {
    echo "=== Ad-hoc Script Validation ==="
    
    local failed=0
    local validated=0
    
    # Validate all scripts
    for script in $(find "$PROJECT_ROOT/scripts" -name "*.py" -o -name "*.sh" | grep -v adhoc | sort); do
        if validate_script "$script"; then
            ((validated++))
        else
            ((failed++))
        fi
        echo
    done
    
    echo "=== Validation Summary ==="
    echo "Validated: $validated"
    echo "Failed: $failed"
    
    if [ $failed -gt 0 ]; then
        echo "❌ Validation failed"
        exit 1
    else
        echo "✅ All scripts validated"
        exit 0
    fi
}

main
