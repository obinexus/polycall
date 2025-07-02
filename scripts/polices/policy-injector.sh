#!/bin/bash
# Sinphasé Policy Injection System
# OBINexus Aegis Project - Runtime Policy Enforcement

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
POLICY_DIR="$SCRIPT_DIR"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"

# Initialize policy structure
init_policies() {
    echo "Initializing Sinphasé policy structure..."
    
    # Create policy directories
    mkdir -p "$POLICY_DIR"/{pre,post,runtime,templates}
    
    # Generate policy configuration
    cat > "$POLICY_DIR/policy.conf" << 'EOF'
# Sinphasé Policy Configuration
# Enforcement levels and module mapping

[global]
enforcement_level=standard
fail_on_violation=false
trace_all_executions=true
audit_mode=enabled

[modules]
# Core module policies
core.auth=strict
core.edge=standard
core.ffi=strict
core.micro=standard
core.network=standard
core.protocol=strict
core.telemetry=lenient

# CLI module policies
cli.commands=standard
cli.providers=standard
cli.repl=lenient

[policies]
# Pre-execution policies
pre.validate_script=enabled
pre.check_permissions=enabled
pre.inject_tracers=enabled
pre.resource_limits=enabled

# Post-execution policies
post.collect_metrics=enabled
post.validate_output=enabled
post.update_registry=enabled
post.cleanup_resources=enabled

# Runtime policies
runtime.monitor_resources=enabled
runtime.enforce_limits=enabled
runtime.log_activities=enabled
runtime.security_checks=enabled

[limits]
max_execution_time=300
max_memory_mb=1024
max_file_handles=256
max_thread_count=32
EOF
}

# Generate pre-execution policies
generate_pre_policies() {
    # Script validator
    cat > "$POLICY_DIR/pre/validate_script.sh" << 'EOF'
#!/bin/bash
# Pre-execution Script Validator
# Validates scripts against Sinphasé compliance rules

SCRIPT_PATH="$1"
SCRIPT_NAME="$(basename "$SCRIPT_PATH")"
VIOLATIONS=0

echo "[VALIDATE] Checking script: $SCRIPT_NAME"

# Check file attributes
if [ ! -f "$SCRIPT_PATH" ]; then
    echo "  ❌ File not found"
    exit 1
fi

if [ ! -r "$SCRIPT_PATH" ]; then
    echo "  ❌ File not readable"
    exit 1
fi

# Check for security issues
if grep -E "(rm -rf /|sudo rm|eval|exec\s+\$)" "$SCRIPT_PATH" >/dev/null 2>&1; then
    echo "  ⚠️  Potentially dangerous commands detected"
    ((VIOLATIONS++))
fi

# Check for required headers
if ! grep -q "OBINexus\|Sinphasé\|Aegis" "$SCRIPT_PATH"; then
    echo "  ⚠️  Missing project headers"
    ((VIOLATIONS++))
fi

# Language-specific checks
if [[ "$SCRIPT_PATH" == *.py ]]; then
    # Python-specific validation
    if python3 -m py_compile "$SCRIPT_PATH" 2>/dev/null; then
        echo "  ✓ Python syntax valid"
    else
        echo "  ❌ Python syntax error"
        ((VIOLATIONS++))
    fi
    
    # Check for unsafe imports
    if grep -E "import (os|subprocess|eval|exec)" "$SCRIPT_PATH" | grep -v "^#"; then
        echo "  ⚠️  Potentially unsafe imports detected"
    fi
    
elif [[ "$SCRIPT_PATH" == *.sh ]]; then
    # Shell-specific validation
    if bash -n "$SCRIPT_PATH" 2>/dev/null; then
        echo "  ✓ Shell syntax valid"
    else
        echo "  ❌ Shell syntax error"
        ((VIOLATIONS++))
    fi
    
    # Check for proper error handling
    if ! grep -q "set -e" "$SCRIPT_PATH"; then
        echo "  ⚠️  Missing 'set -e' for error handling"
    fi
fi

echo "[VALIDATE] Violations: $VIOLATIONS"
exit $VIOLATIONS
EOF
    chmod +x "$POLICY_DIR/pre/validate_script.sh"
    
    # Permission checker
    cat > "$POLICY_DIR/pre/check_permissions.sh" << 'EOF'
#!/bin/bash
# Permission and Security Checker
# Enforces execution permissions based on module

SCRIPT_PATH="$1"
MODULE="unknown"

# Determine module from path
if [[ "$SCRIPT_PATH" == *"core/auth"* ]]; then
    MODULE="core.auth"
elif [[ "$SCRIPT_PATH" == *"core/edge"* ]]; then
    MODULE="core.edge"
elif [[ "$SCRIPT_PATH" == *"core/ffi"* ]]; then
    MODULE="core.ffi"
elif [[ "$SCRIPT_PATH" == *"core/micro"* ]]; then
    MODULE="core.micro"
elif [[ "$SCRIPT_PATH" == *"core/network"* ]]; then
    MODULE="core.network"
elif [[ "$SCRIPT_PATH" == *"core/protocol"* ]]; then
    MODULE="core.protocol"
elif [[ "$SCRIPT_PATH" == *"cli/"* ]]; then
    MODULE="cli"
fi

echo "[PERMISSION] Module: $MODULE"

# Check execution context
if [ "$MODULE" == "core.auth" ] || [ "$MODULE" == "core.ffi" ]; then
    echo "[PERMISSION] High-security module - enforcing strict checks"
    
    # Verify user permissions
    if [ "$USER" != "obinexus" ] && [ "$SUDO_USER" != "obinexus" ]; then
        echo "  ⚠️  Non-authorized user executing high-security module"
    fi
fi

# Check file permissions
PERMS=$(stat -c "%a" "$SCRIPT_PATH" 2>/dev/null || stat -f "%p" "$SCRIPT_PATH" 2>/dev/null | tail -c 4)
if [ "$PERMS" -gt "755" ]; then
    echo "  ⚠️  Overly permissive file permissions: $PERMS"
fi

echo "[PERMISSION] Checks complete"
EOF
    chmod +x "$POLICY_DIR/pre/check_permissions.sh"
    
    # Tracer injector
    cat > "$POLICY_DIR/pre/inject_tracers.sh" << 'EOF'
#!/bin/bash
# Tracer Injection for Execution Monitoring
# Injects monitoring hooks into script execution

SCRIPT_PATH="$1"
EXEC_ID="$2"
TRACER_SCRIPT="$PROJECT_ROOT/scripts/adhoc/tracer-root.sh"

echo "[TRACER] Injecting execution tracers"
echo "[TRACER] Execution ID: $EXEC_ID"

# Start trace session
"$TRACER_SCRIPT" start "$EXEC_ID" "$SCRIPT_PATH" "standard"

# Log initial event
"$TRACER_SCRIPT" event "$EXEC_ID" "pre_execution" "Tracers injected" "info"

# Set up environment for tracing
export SINPHASE_EXEC_ID="$EXEC_ID"
export SINPHASE_TRACE_ENABLED="true"

echo "[TRACER] Injection complete"
EOF
    chmod +x "$POLICY_DIR/pre/inject_tracers.sh"
}

# Generate post-execution policies
generate_post_policies() {
    # Metrics collector
    cat > "$POLICY_DIR/post/collect_metrics.sh" << 'EOF'
#!/bin/bash
# Post-execution Metrics Collection
# Collects and records execution metrics

EXEC_ID="$1"
EXIT_CODE="$2"
TRACER_SCRIPT="$PROJECT_ROOT/scripts/adhoc/tracer-root.sh"

echo "[METRICS] Collecting execution metrics"

# End trace session
"$TRACER_SCRIPT" end "$EXEC_ID" "$EXIT_CODE"

# Collect resource metrics
if [ -f "/proc/$$/status" ]; then
    # Linux-specific metrics
    PEAK_MEM=$(grep "VmPeak" /proc/$$/status | awk '{print $2}')
    "$TRACER_SCRIPT" metric "$EXEC_ID" "peak_memory_kb" "$PEAK_MEM" "kilobytes"
fi

# Log completion event
"$TRACER_SCRIPT" event "$EXEC_ID" "execution_complete" "Exit code: $EXIT_CODE" "info"

echo "[METRICS] Collection complete"
EOF
    chmod +x "$POLICY_DIR/post/collect_metrics.sh"
    
    # Output validator
    cat > "$POLICY_DIR/post/validate_output.sh" << 'EOF'
#!/bin/bash
# Output Validation Policy
# Validates script output for compliance

TRACE_FILE="$1"
VIOLATIONS=0

echo "[OUTPUT] Validating execution output"

# Check for error patterns
if grep -E "(FATAL|CRITICAL|PANIC)" "$TRACE_FILE" >/dev/null 2>&1; then
    echo "  ⚠️  Critical errors detected in output"
    ((VIOLATIONS++))
fi

# Check for security leaks
if grep -E "(password|secret|token|key).*=" "$TRACE_FILE" >/dev/null 2>&1; then
    echo "  ⚠️  Potential credential exposure in output"
    ((VIOLATIONS++))
fi

# Check output size
OUTPUT_SIZE=$(wc -c < "$TRACE_FILE" 2>/dev/null || echo 0)
if [ "$OUTPUT_SIZE" -gt 10485760 ]; then  # 10MB
    echo "  ⚠️  Excessive output size: $OUTPUT_SIZE bytes"
    ((VIOLATIONS++))
fi

echo "[OUTPUT] Validation complete (violations: $VIOLATIONS)"
EOF
    chmod +x "$POLICY_DIR/post/validate_output.sh"
    
    # Registry updater
    cat > "$POLICY_DIR/post/update_registry.sh" << 'EOF'
#!/bin/bash
# Execution Registry Updater
# Updates module execution registry

EXEC_ID="$1"
SCRIPT_PATH="$2"
EXIT_CODE="$3"
REGISTRY_FILE="$PROJECT_ROOT/scripts/adhoc/modules/execution_registry.json"

echo "[REGISTRY] Updating execution registry"

# Create registry entry
TIMESTAMP=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
ENTRY=$(cat << JSON
{
  "execution_id": "$EXEC_ID",
  "script": "$SCRIPT_PATH",
  "timestamp": "$TIMESTAMP",
  "exit_code": $EXIT_CODE,
  "success": $([ $EXIT_CODE -eq 0 ] && echo "true" || echo "false")
}
JSON
)

# Append to registry (create if not exists)
if [ ! -f "$REGISTRY_FILE" ]; then
    echo "[]" > "$REGISTRY_FILE"
fi

# Add entry using jq if available, otherwise append manually
if command -v jq >/dev/null 2>&1; then
    TEMP_FILE=$(mktemp)
    jq ". += [$ENTRY]" "$REGISTRY_FILE" > "$TEMP_FILE"
    mv "$TEMP_FILE" "$REGISTRY_FILE"
else
    # Manual append (less robust)
    echo "$ENTRY" >> "${REGISTRY_FILE}.log"
fi

echo "[REGISTRY] Update complete"
EOF
    chmod +x "$POLICY_DIR/post/update_registry.sh"
}

# Generate runtime policies
generate_runtime_policies() {
    # Resource monitor
    cat > "$POLICY_DIR/runtime/monitor_resources.sh" << 'EOF'
#!/bin/bash
# Runtime Resource Monitor
# Monitors resource usage during execution

EXEC_ID="${SINPHASE_EXEC_ID:-unknown}"
MONITOR_INTERVAL=5
MONITOR_PID=$$

monitor_loop() {
    while kill -0 $MONITOR_PID 2>/dev/null; do
        # Collect current metrics
        if [ -f "/proc/$MONITOR_PID/status" ]; then
            CURRENT_MEM=$(grep "VmRSS" /proc/$MONITOR_PID/status | awk '{print $2}')
            echo "[MONITOR] Memory usage: $CURRENT_MEM KB"
        fi
        
        sleep $MONITOR_INTERVAL
    done
}

# Run monitor in background
monitor_loop &
MONITOR_JOB=$!

# Cleanup on exit
trap "kill $MONITOR_JOB 2>/dev/null" EXIT

echo "[MONITOR] Resource monitoring started (PID: $MONITOR_JOB)"
EOF
    chmod +x "$POLICY_DIR/runtime/monitor_resources.sh"
}

# Generate policy wrapper
generate_wrapper() {
    cat > "$POLICY_DIR/policy-wrapper.sh" << 'EOF'
#!/bin/bash
# Sinphasé Policy Wrapper
# Wraps script execution with full policy enforcement

set -e

SCRIPT_TO_RUN="$1"
shift
SCRIPT_ARGS="$@"

POLICY_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$POLICY_DIR/../.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"

# Source policy configuration
source "$POLICY_DIR/policy.conf" 2>/dev/null || true

# Use ad-hoc executor with policies
if [ -f "$ADHOC_DIR/adhoc-execute.sh" ]; then
    exec "$ADHOC_DIR/adhoc-execute.sh" "$SCRIPT_TO_RUN" $SCRIPT_ARGS
else
    echo "Error: Ad-hoc executor not found"
    exit 1
fi
EOF
    chmod +x "$POLICY_DIR/policy-wrapper.sh"
}

# Main initialization
main() {
    echo "=== Sinphasé Policy Injection System ==="
    echo "Initializing policy framework..."
    
    init_policies
    generate_pre_policies
    generate_post_policies  
    generate_runtime_policies
    generate_wrapper
    
    echo "✅ Policy injection system initialized"
    echo "   Configuration: $POLICY_DIR/policy.conf"
    echo "   Wrapper: $POLICY_DIR/policy-wrapper.sh"
    echo "   Pre-policies: $POLICY_DIR/pre/"
    echo "   Post-policies: $POLICY_DIR/post/"
    echo "   Runtime-policies: $POLICY_DIR/runtime/"
}

main
