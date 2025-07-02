#!/bin/bash
# Fix Script Parameter Handling for LibPolyCall
# OBINexus Computing - Aegis Project Phase 2
# Phase: Implementation Correction

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== Fixing Script Parameter Issues ==="
echo "Project Root: $PROJECT_ROOT"
echo ""

# Create universal script executor
create_universal_executor() {
    cat > "$ADHOC_DIR/execute-with-params.sh" << 'EOF'
#!/bin/bash
# Universal Script Executor with Correct Parameters
# Phase: Execution Management

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

# Extract script name and type
SCRIPT_PATH="$1"
SCRIPT_NAME="$(basename "$SCRIPT_PATH")"
SCRIPT_EXT="${SCRIPT_NAME##*.}"

# Execute based on type
case "$SCRIPT_EXT" in
    py)
        # Python scripts always need --project-root
        echo "[EXEC] Python: $SCRIPT_NAME --project-root $PROJECT_ROOT"
        python3 "$SCRIPT_PATH" --project-root "$PROJECT_ROOT"
        ;;
    sh)
        # Shell scripts can determine their own project root
        echo "[EXEC] Shell: $SCRIPT_NAME"
        bash "$SCRIPT_PATH"
        ;;
    *)
        echo "[ERROR] Unknown script type: $SCRIPT_EXT"
        exit 1
        ;;
esac
EOF
    chmod +x "$ADHOC_DIR/execute-with-params.sh"
}

# Fix the main fix_all_includes.sh script
fix_all_includes_script() {
    echo "Fixing fix_all_includes.sh..."
    
    # Create corrected version that properly uses PROJECT_ROOT
    cat > "$ADHOC_DIR/fix_all_includes_corrected.sh" << 'EOF'
#!/bin/bash
# Fixed Version: Comprehensive Include Path Fix
# Phase: Implementation

set -e

# Establish correct project root
PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Logging functions
log() { echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1"; }
log_error() { echo "[ERROR] $1" >&2; }
log_success() { echo "[SUCCESS] $1"; }

# Execute Python scripts with correct parameters
execute_python_fix() {
    local script_name="$1"
    local script_path="$SCRIPTS_DIR/adhoc/$script_name"
    
    if [ -f "$script_path" ]; then
        log "Executing: $script_name"
        python3 "$script_path" --project-root "$PROJECT_ROOT" || {
            log_error "$script_name failed"
            return 1
        }
        log_success "$script_name completed"
    else
        log_error "$script_name not found at $script_path"
        return 1
    fi
}

# Main execution
main() {
    log "Starting LibPolyCall include path fixes"
    log "Project root: $PROJECT_ROOT"
    
    # Create backup
    if [ -d "$PROJECT_ROOT/src" ] || [ -d "$PROJECT_ROOT/include" ]; then
        log "Creating backup..."
        mkdir -p "$PROJECT_ROOT/backup_$TIMESTAMP"
        [ -d "$PROJECT_ROOT/src" ] && cp -r "$PROJECT_ROOT/src" "$PROJECT_ROOT/backup_$TIMESTAMP/"
        [ -d "$PROJECT_ROOT/include" ] && cp -r "$PROJECT_ROOT/include" "$PROJECT_ROOT/backup_$TIMESTAMP/"
    fi
    
    # Execute fix scripts in order
    local scripts=(
        "standardize_includes.py"
        "fix_polycall_paths.py"
        "fix_all_paths.py"
        "fix_nested_path_includes.py"
        "fix_implementation_includes.py"
        "validate_includes.py"
    )
    
    for script in "${scripts[@]}"; do
        execute_python_fix "$script" || log_error "Skipping due to error"
    done
    
    log "Include path fix process completed"
}

main "$@"
EOF
    chmod +x "$ADHOC_DIR/fix_all_includes_corrected.sh"
}

# Create batch executor for all fix scripts
create_batch_executor() {
    cat > "$ADHOC_DIR/run-all-fixes.sh" << 'EOF'
#!/bin/bash
# Batch Fix Script Executor
# Phase: Implementation

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

echo "=== Running All Fix Scripts with Correct Parameters ==="
echo "Project Root: $PROJECT_ROOT"
echo ""

# Find all fix_*.py scripts and execute with parameters
for script in "$ADHOC_DIR"/fix_*.py; do
    if [ -f "$script" ]; then
        script_name=$(basename "$script")
        echo "Running: $script_name"
        
        # Add appropriate parameters based on script
        case "$script_name" in
            fix_includes_and_backup.py)
                python3 "$script" "$PROJECT_ROOT" --skip-backup
                ;;
            *)
                python3 "$script" --project-root "$PROJECT_ROOT"
                ;;
        esac
        
        echo "  ✓ $script_name completed"
        echo ""
    fi
done

echo "All fix scripts executed"
EOF
    chmod +x "$ADHOC_DIR/run-all-fixes.sh"
}

# Create TDD test runner for each module
create_tdd_runner() {
    cat > "$ADHOC_DIR/tdd-runner.sh" << 'EOF'
#!/bin/bash
# TDD Test Runner for LibPolyCall Modules
# Phase: Testing

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

# Core modules to test
MODULES=(
    "auth"
    "edge"
    "ffi"
    "micro"
    "network"
    "protocol"
    "telemetry"
)

# Run tests for a module
test_module() {
    local module="$1"
    echo "Testing module: $module"
    
    # Check for module test directory
    local test_dir="$PROJECT_ROOT/tests/core/$module"
    if [ -d "$test_dir" ]; then
        # Run module-specific tests
        find "$test_dir" -name "test_*.py" -exec python3 {} \;
    else
        echo "  No tests found for $module"
    fi
    
    # Validate module headers
    local include_dir="$PROJECT_ROOT/include/polycall/$module"
    if [ -d "$include_dir" ]; then
        echo "  Validating headers..."
        python3 "$ADHOC_DIR/validate_includes.py" \
            --project-root "$PROJECT_ROOT" \
            --module "$module"
    fi
}

# Main execution
case "${1:-all}" in
    all)
        echo "=== Running TDD for all modules ==="
        for module in "${MODULES[@]}"; do
            test_module "$module"
            echo ""
        done
        ;;
    module)
        if [ -n "$2" ]; then
            test_module "$2"
        else
            echo "Usage: $0 module <module-name>"
            exit 1
        fi
        ;;
    *)
        echo "Usage: $0 {all|module <name>}"
        ;;
esac
EOF
    chmod +x "$ADHOC_DIR/tdd-runner.sh"
}

# Main execution
main() {
    create_universal_executor
    fix_all_includes_script
    create_batch_executor
    create_tdd_runner
    
    echo -e "${GREEN}=== Parameter Fix Complete ===${NC}"
    echo "Created:"
    echo "  - Universal executor: $ADHOC_DIR/execute-with-params.sh"
    echo "  - Fixed includes script: $ADHOC_DIR/fix_all_includes_corrected.sh"
    echo "  - Batch executor: $ADHOC_DIR/run-all-fixes.sh"
    echo "  - TDD runner: $ADHOC_DIR/tdd-runner.sh"
    echo ""
    echo "To fix all scripts immediately, run:"
    echo "  $ADHOC_DIR/run-all-fixes.sh"
}

main "$@"
