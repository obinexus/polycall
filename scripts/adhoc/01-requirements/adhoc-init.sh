#!/bin/bash
# Ad-hoc Compliance Framework Initializer
# Sinphasé Governance - OBINexus Aegis Project

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"
POLICY_DIR="$PROJECT_ROOT/scripts/policies"
TRACE_DIR="$PROJECT_ROOT/logs/trace"

# Create directory structure
create_directories() {
    echo "Creating ad-hoc compliance directories..."
    mkdir -p "$ADHOC_DIR"/{modules,templates,validators}
    mkdir -p "$POLICY_DIR"/{pre,post,runtime}
    mkdir -p "$TRACE_DIR"/{execution,compliance,audit}
    
    # Create module registry
    cat > "$ADHOC_DIR/modules/registry.json" << 'EOF'
{
    "version": "2.0.0",
    "modules": {
        "core": ["auth", "edge", "ffi", "micro", "network", "protocol", "telemetry"],
        "cli": ["commands", "providers", "repl"],
        "utilities": ["fix_scripts", "validators", "generators"]
    },
    "policies": {
        "pre_execution": ["validate_script", "check_permissions", "inject_tracers"],
        "post_execution": ["collect_metrics", "validate_output", "update_registry"],
        "runtime": ["monitor_resources", "enforce_limits", "log_activities"]
    },
    "compliance_levels": {
        "strict": {"enforce_all": true, "fail_on_warning": true},
        "standard": {"enforce_all": true, "fail_on_warning": false},
        "lenient": {"enforce_all": false, "fail_on_warning": false}
    }
}
EOF
}

# Generate ad-hoc execution wrapper
generate_executor() {
    cat > "$ADHOC_DIR/adhoc-execute.sh" << 'EOF'
#!/bin/bash
# Ad-hoc Script Executor with Policy Injection
# Sinphasé Compliant Execution Wrapper

set -e

SCRIPT_PATH="$1"
shift
SCRIPT_ARGS="$@"

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"
POLICY_DIR="$PROJECT_ROOT/scripts/policies"
TRACE_DIR="$PROJECT_ROOT/logs/trace"

# Generate execution ID
EXEC_ID="exec_$(date +%Y%m%d_%H%M%S)_$$"
TRACE_FILE="$TRACE_DIR/execution/$EXEC_ID.log"

# Pre-execution policies
run_pre_policies() {
    echo "[PRE] Running pre-execution policies for $SCRIPT_PATH" | tee -a "$TRACE_FILE"
    
    # Validate script
    if [ -f "$POLICY_DIR/pre/validate_script.sh" ]; then
        bash "$POLICY_DIR/pre/validate_script.sh" "$SCRIPT_PATH" | tee -a "$TRACE_FILE"
    fi
    
    # Check permissions
    if [ -f "$POLICY_DIR/pre/check_permissions.sh" ]; then
        bash "$POLICY_DIR/pre/check_permissions.sh" "$SCRIPT_PATH" | tee -a "$TRACE_FILE"
    fi
    
    # Inject tracers
    if [ -f "$POLICY_DIR/pre/inject_tracers.sh" ]; then
        bash "$POLICY_DIR/pre/inject_tracers.sh" "$SCRIPT_PATH" "$EXEC_ID" | tee -a "$TRACE_FILE"
    fi
}

# Execute with monitoring
execute_with_monitoring() {
    echo "[EXEC] Executing: $SCRIPT_PATH $SCRIPT_ARGS" | tee -a "$TRACE_FILE"
    echo "[TIME] Start: $(date -u +"%Y-%m-%d %H:%M:%S UTC")" | tee -a "$TRACE_FILE"
    
    # Determine script type and execute
    if [[ "$SCRIPT_PATH" == *.py ]]; then
        python3 "$SCRIPT_PATH" $SCRIPT_ARGS 2>&1 | tee -a "$TRACE_FILE"
        EXIT_CODE=${PIPESTATUS[0]}
    elif [[ "$SCRIPT_PATH" == *.sh ]]; then
        bash "$SCRIPT_PATH" $SCRIPT_ARGS 2>&1 | tee -a "$TRACE_FILE"
        EXIT_CODE=${PIPESTATUS[0]}
    else
        # Direct command execution
        eval "$SCRIPT_PATH $SCRIPT_ARGS" 2>&1 | tee -a "$TRACE_FILE"
        EXIT_CODE=${PIPESTATUS[0]}
    fi
    
    echo "[TIME] End: $(date -u +"%Y-%m-%d %H:%M:%S UTC")" | tee -a "$TRACE_FILE"
    echo "[EXIT] Code: $EXIT_CODE" | tee -a "$TRACE_FILE"
    
    return $EXIT_CODE
}

# Post-execution policies
run_post_policies() {
    echo "[POST] Running post-execution policies" | tee -a "$TRACE_FILE"
    
    # Collect metrics
    if [ -f "$POLICY_DIR/post/collect_metrics.sh" ]; then
        bash "$POLICY_DIR/post/collect_metrics.sh" "$EXEC_ID" "$EXIT_CODE" | tee -a "$TRACE_FILE"
    fi
    
    # Validate output
    if [ -f "$POLICY_DIR/post/validate_output.sh" ]; then
        bash "$POLICY_DIR/post/validate_output.sh" "$TRACE_FILE" | tee -a "$TRACE_FILE"
    fi
    
    # Update registry
    if [ -f "$POLICY_DIR/post/update_registry.sh" ]; then
        bash "$POLICY_DIR/post/update_registry.sh" "$EXEC_ID" "$SCRIPT_PATH" "$EXIT_CODE" | tee -a "$TRACE_FILE"
    fi
}

# Main execution flow
main() {
    mkdir -p "$(dirname "$TRACE_FILE")"
    
    echo "=== Ad-hoc Compliant Execution ===" | tee "$TRACE_FILE"
    echo "Execution ID: $EXEC_ID" | tee -a "$TRACE_FILE"
    echo "Script: $SCRIPT_PATH" | tee -a "$TRACE_FILE"
    echo "Arguments: $SCRIPT_ARGS" | tee -a "$TRACE_FILE"
    echo "===================================" | tee -a "$TRACE_FILE"
    
    # Run pre-execution policies
    run_pre_policies
    
    # Execute with monitoring
    execute_with_monitoring
    EXIT_STATUS=$?
    
    # Run post-execution policies
    run_post_policies
    
    echo "=== Execution Complete ===" | tee -a "$TRACE_FILE"
    
    exit $EXIT_STATUS
}

main
EOF
    chmod +x "$ADHOC_DIR/adhoc-execute.sh"
}

# Generate validator script
generate_validator() {
    cat > "$ADHOC_DIR/adhoc-validator.sh" << 'EOF'
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
EOF
    chmod +x "$ADHOC_DIR/adhoc-validator.sh"
}

# Main initialization
main() {
    echo "=== Ad-hoc Compliance Framework Initialization ==="
    echo "Project root: $PROJECT_ROOT"
    
    create_directories
    generate_executor
    generate_validator
    
    # Create initialization marker
    date -u +"%Y-%m-%d %H:%M:%S UTC" > "$ADHOC_DIR/.initialized"
    
    echo "✅ Ad-hoc compliance framework initialized successfully"
    echo "   Executor: $ADHOC_DIR/adhoc-execute.sh"
    echo "   Validator: $ADHOC_DIR/adhoc-validator.sh"
    echo "   Registry: $ADHOC_DIR/modules/registry.json"
}

main