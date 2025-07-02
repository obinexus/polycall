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
