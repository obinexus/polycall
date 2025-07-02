#!/bin/bash
# Waterfall Phase Restructuring for LibPolyCall
# OBINexus Computing - Aegis Project Phase 2
# Phase: Infrastructure Reorganization

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"
ADHOC_DIR="$SCRIPTS_DIR/adhoc"

# Color codes
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== LibPolyCall Waterfall Restructuring ==="
echo "Project Root: $PROJECT_ROOT"
echo ""

# Create waterfall phase directories
create_waterfall_structure() {
    echo "Creating waterfall phase structure..."
    
    # Main waterfall phases
    local phases=(
        "01-requirements"
        "02-design"
        "03-implementation"
        "04-testing"
        "05-deployment"
        "06-maintenance"
    )
    
    for phase in "${phases[@]}"; do
        mkdir -p "$ADHOC_DIR/$phase"/{scripts,logs,reports}
        
        # Create phase-specific main.sh
        cat > "$ADHOC_DIR/$phase/main.sh" << EOF
#!/bin/bash
# $phase Phase Orchestrator
# OBINexus LibPolyCall - Waterfall Phase Controller
# Phase: ${phase#*-}

set -e

PHASE_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
PROJECT_ROOT="\$(cd "\$PHASE_DIR/../../.." && pwd)"
PHASE_NAME="${phase#*-}"

# Phase-specific logging
log_phase() { echo -e "${BLUE}[\${PHASE_NAME^^}]${NC} \$1"; }
log_success() { echo -e "${GREEN}[\${PHASE_NAME^^}]${NC} ✓ \$1"; }
log_error() { echo -e "${YELLOW}[\${PHASE_NAME^^}]${NC} ✗ \$1" >&2; }

# Execute phase scripts
execute_phase() {
    log_phase "Executing \${PHASE_NAME} phase..."
    
    # Find and execute all scripts in phase
    for script in \$PHASE_DIR/scripts/*.{sh,py}; do
        if [ -f "\$script" ]; then
            local script_name=\$(basename "\$script")
            log_phase "Running: \$script_name"
            
            case "\${script##*.}" in
                sh)
                    bash "\$script" --project-root "\$PROJECT_ROOT"
                    ;;
                py)
                    python3 "\$script" --project-root "\$PROJECT_ROOT"
                    ;;
            esac
        fi
    done
    
    log_success "\${PHASE_NAME} phase complete"
}

# Main execution
case "\${1:-run}" in
    run)
        execute_phase
        ;;
    status)
        echo "Phase: \${PHASE_NAME}"
        echo "Scripts: \$(find \$PHASE_DIR/scripts -name "*.sh" -o -name "*.py" | wc -l)"
        echo "Logs: \$(find \$PHASE_DIR/logs -name "*.log" | wc -l)"
        ;;
    *)
        echo "Usage: \$0 {run|status}"
        ;;
esac
EOF
        chmod +x "$ADHOC_DIR/$phase/main.sh"
    done
}

# Migrate existing scripts to appropriate phases
migrate_scripts() {
    echo "Migrating scripts to waterfall phases..."
    
    # Requirements phase scripts
    mkdir -p "$ADHOC_DIR/01-requirements/scripts"
    cp "$ADHOC_DIR/branch_resolver.py" "$ADHOC_DIR/01-requirements/scripts/" 2>/dev/null || true
    
    # Design phase scripts
    mkdir -p "$ADHOC_DIR/02-design/scripts"
    for script in standardize_includes.py include_path_standardizer.py generate_unified_header.py; do
        [ -f "$ADHOC_DIR/$script" ] && cp "$ADHOC_DIR/$script" "$ADHOC_DIR/02-design/scripts/"
    done
    
    # Implementation phase scripts
    mkdir -p "$ADHOC_DIR/03-implementation/scripts"
    for script in fix_*.py fix_*.sh; do
        [ -f "$ADHOC_DIR/$script" ] && cp "$ADHOC_DIR/$script" "$ADHOC_DIR/03-implementation/scripts/"
    done
    
    # Testing phase scripts
    mkdir -p "$ADHOC_DIR/04-testing/scripts"
    for script in validate_*.py validate_*.sh test_*.py compliance-check.sh; do
        [ -f "$ADHOC_DIR/$script" ] && cp "$ADHOC_DIR/$script" "$ADHOC_DIR/04-testing/scripts/"
    done
    
    # Deployment phase scripts
    mkdir -p "$ADHOC_DIR/05-deployment/scripts"
    [ -f "$ADHOC_DIR/custom_binding_configurator.sh" ] && \
        cp "$ADHOC_DIR/custom_binding_configurator.sh" "$ADHOC_DIR/05-deployment/scripts/"
}

# Create master waterfall orchestrator
create_waterfall_orchestrator() {
    cat > "$ADHOC_DIR/waterfall.sh" << 'EOF'
#!/bin/bash
# Master Waterfall Orchestrator
# OBINexus LibPolyCall - Full Lifecycle Controller
# Phase: All

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

# Color codes
BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Waterfall phases in order
PHASES=(
    "01-requirements"
    "02-design"
    "03-implementation"
    "04-testing"
    "05-deployment"
    "06-maintenance"
)

# Execute single phase
execute_phase() {
    local phase="$1"
    echo -e "${BLUE}=== Waterfall Phase: ${phase#*-} ===${NC}"
    
    if [ -f "$ADHOC_DIR/$phase/main.sh" ]; then
        "$ADHOC_DIR/$phase/main.sh" run
        return $?
    else
        echo "Phase not implemented: $phase"
        return 1
    fi
}

# Execute all phases
execute_waterfall() {
    local start_time=$(date +%s)
    local failed_phases=()
    
    echo "=== LibPolyCall Waterfall Execution ==="
    echo "Starting full waterfall cycle..."
    echo ""
    
    for phase in "${PHASES[@]}"; do
        if execute_phase "$phase"; then
            echo -e "${GREEN}✓ $phase completed${NC}"
        else
            echo -e "${RED}✗ $phase failed${NC}"
            failed_phases+=("$phase")
            
            # Stop on critical phase failure
            if [[ "$phase" =~ ^0[1-4] ]]; then
                echo "Critical phase failed. Stopping waterfall."
                break
            fi
        fi
        echo ""
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    echo "=== Waterfall Summary ==="
    echo "Duration: ${duration}s"
    echo "Failed phases: ${#failed_phases[@]}"
    
    if [ ${#failed_phases[@]} -eq 0 ]; then
        echo -e "${GREEN}✓ All phases completed successfully${NC}"
        return 0
    else
        echo -e "${RED}✗ Failed phases: ${failed_phases[*]}${NC}"
        return 1
    fi
}

# Show waterfall status
show_status() {
    echo "=== LibPolyCall Waterfall Status ==="
    echo ""
    
    for phase in "${PHASES[@]}"; do
        echo "Phase: $phase"
        if [ -f "$ADHOC_DIR/$phase/main.sh" ]; then
            "$ADHOC_DIR/$phase/main.sh" status
        else
            echo "  Status: Not implemented"
        fi
        echo ""
    done
}

# Main command dispatcher
case "${1:-help}" in
    run)
        execute_waterfall
        ;;
    phase)
        if [ -n "$2" ]; then
            execute_phase "$2"
        else
            echo "Usage: $0 phase <phase-name>"
            exit 1
        fi
        ;;
    status)
        show_status
        ;;
    help|*)
        echo "Usage: $0 {run|phase <name>|status}"
        echo ""
        echo "Commands:"
        echo "  run    - Execute full waterfall cycle"
        echo "  phase  - Execute specific phase"
        echo "  status - Show waterfall status"
        echo ""
        echo "Available phases:"
        for phase in "${PHASES[@]}"; do
            echo "  - $phase"
        done
        ;;
esac
EOF
    chmod +x "$ADHOC_DIR/waterfall.sh"
}

# Create fixed script wrappers
create_script_wrappers() {
    echo "Creating parameter-aware script wrappers..."
    
    # Create wrapper for Python scripts
    cat > "$ADHOC_DIR/python-wrapper.sh" << 'EOF'
#!/bin/bash
# Python Script Wrapper with Project Root
set -e

SCRIPT="$1"
shift

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# Add --project-root if not present
if [[ "$*" != *"--project-root"* ]]; then
    python3 "$SCRIPT" --project-root "$PROJECT_ROOT" "$@"
else
    python3 "$SCRIPT" "$@"
fi
EOF
    chmod +x "$ADHOC_DIR/python-wrapper.sh"
}

# Fix adhoc-validator.sh
fix_adhoc_validator() {
    echo "Fixing adhoc-validator.sh..."
    
    cat > "$ADHOC_DIR/adhoc-validator.sh" << 'EOF'
#!/bin/bash
# Ad-hoc Script Validator - Fixed Version
# Phase: Quality Assurance

set -e

ADHOC_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_DIR/../.." && pwd)"

echo "=== Ad-hoc Script Validation ==="

validate_script() {
    local script="$1"
    local errors=0
    
    echo "Validating: $(basename "$script")"
    
    # Check shell syntax
    if [[ "$script" == *.sh ]]; then
        if bash -n "$script" 2>/dev/null; then
            echo "  ✓ Shell syntax valid"
        else
            echo "  ✗ Shell syntax error"
            ((errors++))
        fi
    fi
    
    # Check for phase declaration
    if grep -q "Phase:" "$script"; then
        echo "  ✓ Phase declaration found"
    else
        echo "  ✗ Missing phase declaration"
        ((errors++))
    fi
    
    return $errors
}

# Validate all scripts
total_errors=0
for script in $(find "$ADHOC_DIR" -name "*.sh" -o -name "*.py" | grep -v logs | head -20); do
    validate_script "$script" || ((total_errors+=$?))
done

if [ $total_errors -eq 0 ]; then
    echo ""
    echo "✓ All scripts validated successfully"
    exit 0
else
    echo ""
    echo "✗ Found $total_errors validation errors"
    exit 1
fi
EOF
    chmod +x "$ADHOC_DIR/adhoc-validator.sh"
}

# Main execution
main() {
    create_waterfall_structure
    migrate_scripts
    create_waterfall_orchestrator
    create_script_wrappers
    fix_adhoc_validator
    
    echo ""
    echo -e "${GREEN}=== Restructuring Complete ===${NC}"
    echo "Waterfall structure created in: $ADHOC_DIR"
    echo ""
    echo "Next steps:"
    echo "1. Run: $ADHOC_DIR/waterfall.sh status"
    echo "2. Execute phases: $ADHOC_DIR/waterfall.sh run"
    echo "3. Or run specific phase: $ADHOC_DIR/waterfall.sh phase 03-implementation"
}

main "$@"
