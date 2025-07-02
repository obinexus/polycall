#!/bin/bash
# implement-waterfall-cycle.sh - Implement waterfall development methodology
# OBINexus Waterfall Development - Process Implementation Phase

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WATERFALL_DIR="$PROJECT_ROOT/.waterfall"
PHASE_FILE="$WATERFALL_DIR/current_phase"
GATE_LOG="$WATERFALL_DIR/phase_gates.log"

log_info() { echo "[INFO] $1"; }
log_error() { echo "[ERROR] $1" >&2; }
log_success() { echo "[SUCCESS] $1"; }
log_phase() { echo "[PHASE:$1] $2" | tee -a "$GATE_LOG"; }

# Initialize waterfall structure
init_waterfall() {
    log_info "Initializing waterfall development structure..."
    
    mkdir -p "$WATERFALL_DIR"/{phases,gates,artifacts,reports}
    
    # Define phases
    cat > "$WATERFALL_DIR/phases.json" << 'EOF'
{
  "phases": [
    {
      "id": "requirements",
      "name": "Requirements Analysis",
      "duration": "1 week",
      "gates": ["requirements_complete", "stakeholder_approval"],
      "artifacts": ["requirements.md", "use_cases.md", "api_spec.md"]
    },
    {
      "id": "design",
      "name": "System Design", 
      "duration": "2 weeks",
      "gates": ["design_review", "architecture_approval"],
      "artifacts": ["architecture.md", "component_design.md", "interface_spec.md"]
    },
    {
      "id": "implementation",
      "name": "Implementation",
      "duration": "4 weeks",
      "gates": ["code_complete", "unit_tests_pass"],
      "artifacts": ["source_code", "unit_tests", "build_scripts"]
    },
    {
      "id": "verification",
      "name": "Verification",
      "duration": "2 weeks",
      "gates": ["integration_tests_pass", "performance_baseline"],
      "artifacts": ["test_reports", "coverage_report", "performance_metrics"]
    },
    {
      "id": "maintenance",
      "name": "Maintenance",
      "duration": "ongoing",
      "gates": ["deployment_ready", "documentation_complete"],
      "artifacts": ["deployment_guide", "user_manual", "maintenance_plan"]
    }
  ]
}
EOF

    # Set initial phase
    echo "requirements" > "$PHASE_FILE"
    
    log_success "Waterfall structure initialized"
}

# Create phase gate checker
create_gate_checker() {
    cat > "$WATERFALL_DIR/check_gate.sh" << 'EOF'
#!/bin/bash
# Phase Gate Checker
# Validates phase completion criteria

WATERFALL_DIR="$(cd "$(dirname "$0")" && pwd)"
PHASE="$1"
GATE="$2"

check_requirements_complete() {
    local required_files=(
        "docs/requirements.md"
        "docs/use_cases.md"
        "docs/api_spec.md"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$PROJECT_ROOT/$file" ]; then
            echo "Missing: $file"
            return 1
        fi
    done
    return 0
}

check_design_review() {
    if [ ! -f "$PROJECT_ROOT/docs/architecture.md" ]; then
        echo "Architecture document missing"
        return 1
    fi
    
    # Check for review comments
    if [ ! -f "$WATERFALL_DIR/gates/design_review.txt" ]; then
        echo "Design review not conducted"
        return 1
    fi
    return 0
}

check_code_complete() {
    # Check if all modules exist
    local modules=("auth" "network" "protocol" "edge" "micro")
    for module in "${modules[@]}"; do
        if [ ! -d "$PROJECT_ROOT/src/core/$module" ]; then
            echo "Module missing: $module"
            return 1
        fi
    done
    return 0
}

check_unit_tests_pass() {
    # Run unit tests
    if [ -f "$PROJECT_ROOT/build/run_tests" ]; then
        "$PROJECT_ROOT/build/run_tests" > /dev/null 2>&1
        return $?
    fi
    return 1
}

check_integration_tests_pass() {
    # Check integration test results
    if [ -f "$PROJECT_ROOT/test-results/integration.xml" ]; then
        grep -q "failures=\"0\"" "$PROJECT_ROOT/test-results/integration.xml"
        return $?
    fi
    return 1
}

# Execute gate check
case "$GATE" in
    requirements_complete) check_requirements_complete ;;
    design_review) check_design_review ;;
    code_complete) check_code_complete ;;
    unit_tests_pass) check_unit_tests_pass ;;
    integration_tests_pass) check_integration_tests_pass ;;
    *)
        echo "Unknown gate: $GATE"
        exit 1
        ;;
esac
EOF
    chmod +x "$WATERFALL_DIR/check_gate.sh"
}

# Create phase transition script
create_phase_transition() {
    cat > "$WATERFALL_DIR/transition_phase.sh" << 'EOF'
#!/bin/bash
# Phase Transition Manager
# Handles waterfall phase transitions

set -e

WATERFALL_DIR="$(cd "$(dirname "$0")" && pwd)"
CURRENT_PHASE=$(cat "$WATERFALL_DIR/../current_phase" 2>/dev/null || echo "requirements")
PHASES_FILE="$WATERFALL_DIR/../phases.json"

# Get next phase
get_next_phase() {
    case "$1" in
        requirements) echo "design" ;;
        design) echo "implementation" ;;
        implementation) echo "verification" ;;
        verification) echo "maintenance" ;;
        maintenance) echo "maintenance" ;;
        *) echo "requirements" ;;
    esac
}

# Validate phase gates
validate_gates() {
    local phase="$1"
    local gates=$(jq -r ".phases[] | select(.id==\"$phase\") | .gates[]" "$PHASES_FILE")
    
    echo "Validating gates for phase: $phase"
    local all_passed=true
    
    for gate in $gates; do
        echo -n "  Checking $gate... "
        if "$WATERFALL_DIR/check_gate.sh" "$phase" "$gate"; then
            echo "PASSED"
        else
            echo "FAILED"
            all_passed=false
        fi
    done
    
    $all_passed
}

# Generate phase report
generate_phase_report() {
    local phase="$1"
    local report_file="$WATERFALL_DIR/reports/phase_${phase}_$(date +%Y%m%d).md"
    
    cat > "$report_file" << REPORT
# Phase Completion Report: ${phase^^}
Date: $(date)

## Phase Summary
- Phase: $phase
- Duration: $(jq -r ".phases[] | select(.id==\"$phase\") | .duration" "$PHASES_FILE")
- Status: Completed

## Gates Validated
$(jq -r ".phases[] | select(.id==\"$phase\") | .gates[] | \"- \" + ." "$PHASES_FILE")

## Artifacts Produced
$(jq -r ".phases[] | select(.id==\"$phase\") | .artifacts[] | \"- \" + ." "$PHASES_FILE")

## Metrics
- Files Modified: $(git diff --name-only HEAD~10 2>/dev/null | wc -l || echo "N/A")
- Tests Added: $(find tests -name "*.c" -newer "$WATERFALL_DIR/phase_start" 2>/dev/null | wc -l || echo "0")
- Documentation Updated: $(find docs -name "*.md" -newer "$WATERFALL_DIR/phase_start" 2>/dev/null | wc -l || echo "0")

## Notes
Phase completed successfully. Ready for transition to next phase.
REPORT
    
    echo "Report generated: $report_file"
}

# Main transition logic
if validate_gates "$CURRENT_PHASE"; then
    NEXT_PHASE=$(get_next_phase "$CURRENT_PHASE")
    echo "$NEXT_PHASE" > "$WATERFALL_DIR/../current_phase"
    date > "$WATERFALL_DIR/phase_start"
    
    generate_phase_report "$CURRENT_PHASE"
    
    echo "=== PHASE TRANSITION ==="
    echo "From: $CURRENT_PHASE"
    echo "To: $NEXT_PHASE"
    echo "Timestamp: $(date)"
    echo "======================="
    
    # Log transition
    echo "$(date): $CURRENT_PHASE -> $NEXT_PHASE" >> "$WATERFALL_DIR/../phase_gates.log"
else
    echo "Cannot transition: Gates not satisfied for phase $CURRENT_PHASE"
    exit 1
fi
EOF
    chmod +x "$WATERFALL_DIR/transition_phase.sh"
}

# Create waterfall dashboard
create_dashboard() {
    cat > "$PROJECT_ROOT/waterfall-status.sh" << 'EOF'
#!/bin/bash
# Waterfall Development Dashboard
# Shows current phase and progress

WATERFALL_DIR=".waterfall"
CURRENT_PHASE=$(cat "$WATERFALL_DIR/current_phase" 2>/dev/null || echo "Not initialized")

echo "╔══════════════════════════════════════════════════╗"
echo "║       LibPolyCall Waterfall Dashboard            ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""
echo "Current Phase: ${CURRENT_PHASE^^}"
echo "─────────────────────────────────────────────────────"

# Show phase progress
phases=("requirements" "design" "implementation" "verification" "maintenance")
for i in "${!phases[@]}"; do
    phase="${phases[$i]}"
    if [ "$phase" = "$CURRENT_PHASE" ]; then
        echo "→ $(printf "%2d" $((i+1))). ${phase^^} [ACTIVE]"
    elif [ $i -lt $(printf '%s\n' "${phases[@]}" | grep -n "^$CURRENT_PHASE$" | cut -d: -f1) ]; then
        echo "✓ $(printf "%2d" $((i+1))). ${phase^^} [COMPLETED]"
    else
        echo "  $(printf "%2d" $((i+1))). ${phase^^} [PENDING]"
    fi
done

echo ""
echo "Phase Gates Status:"
echo "─────────────────────────────────────────────────────"

if [ -f "$WATERFALL_DIR/phases.json" ] && [ "$CURRENT_PHASE" != "Not initialized" ]; then
    gates=$(jq -r ".phases[] | select(.id==\"$CURRENT_PHASE\") | .gates[]" "$WATERFALL_DIR/phases.json")
    for gate in $gates; do
        echo -n "  • $gate: "
        if "$WATERFALL_DIR/check_gate.sh" "$CURRENT_PHASE" "$gate" >/dev/null 2>&1; then
            echo "✓ PASSED"
        else
            echo "✗ PENDING"
        fi
    done
fi

echo ""
echo "Recent Activity:"
echo "─────────────────────────────────────────────────────"
tail -5 "$WATERFALL_DIR/phase_gates.log" 2>/dev/null || echo "  No activity recorded"

echo ""
echo "Commands:"
echo "  make waterfall-init     - Initialize waterfall process"
echo "  make waterfall-check    - Check current gates"
echo "  make waterfall-advance  - Advance to next phase"
echo "  make waterfall-report   - Generate phase report"
EOF
    chmod +x "$PROJECT_ROOT/waterfall-status.sh"
}

# Create TDD cycle scripts
create_tdd_cycle() {
    log_info "Creating TDD cycle scripts..."
    
    mkdir -p "$PROJECT_ROOT/tdd"
    
    cat > "$PROJECT_ROOT/tdd/tdd-cycle.sh" << 'EOF'
#!/bin/bash
# TDD Cycle Runner
# Red-Green-Refactor implementation

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TDD_DIR="$PROJECT_ROOT/tdd"
CYCLE_LOG="$TDD_DIR/cycle.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

run_tdd_cycle() {
    local module="$1"
    local test_file="$2"
    
    echo "Starting TDD Cycle for: $module"
    echo "════════════════════════════════════"
    
    # RED: Write failing test
    echo -e "${RED}1. RED PHASE - Write Failing Test${NC}"
    echo "   Creating test: $test_file"
    
    # Create test template if not exists
    if [ ! -f "$test_file" ]; then
        mkdir -p "$(dirname "$test_file")"
        cat > "$test_file" << 'TEST'
#include <assert.h>
#include <stdio.h>
#include "polycall.h"

void test_new_feature() {
    // This test should fail initially
    assert(0 && "Test not implemented");
}

int main() {
    test_new_feature();
    printf("All tests passed!\n");
    return 0;
}
TEST
    fi
    
    # Run test (expect failure)
    echo "   Running test (expecting failure)..."
    if gcc -o test_runner "$test_file" -I"$PROJECT_ROOT/include" 2>/dev/null && ./test_runner 2>/dev/null; then
        echo -e "${RED}   ERROR: Test passed when it should fail!${NC}"
        exit 1
    else
        echo -e "${RED}   ✓ Test fails as expected${NC}"
    fi
    
    # GREEN: Make test pass
    echo -e "\n${GREEN}2. GREEN PHASE - Make Test Pass${NC}"
    echo "   Implement minimal code to pass..."
    read -p "   Press Enter when implementation is ready..."
    
    # Run test again
    if gcc -o test_runner "$test_file" -I"$PROJECT_ROOT/include" -L"$PROJECT_ROOT/build" -lpolycall 2>/dev/null && ./test_runner; then
        echo -e "${GREEN}   ✓ Test passes!${NC}"
    else
        echo -e "${RED}   ✗ Test still failing${NC}"
        exit 1
    fi
    
    # REFACTOR: Improve code
    echo -e "\n${BLUE}3. REFACTOR PHASE - Improve Code${NC}"
    echo "   Refactor while keeping tests green..."
    read -p "   Press Enter when refactoring is complete..."
    
    # Final test run
    if gcc -o test_runner "$test_file" -I"$PROJECT_ROOT/include" -L"$PROJECT_ROOT/build" -lpolycall 2>/dev/null && ./test_runner; then
        echo -e "${GREEN}   ✓ Tests still pass after refactoring!${NC}"
    else
        echo -e "${RED}   ✗ Refactoring broke tests!${NC}"
        exit 1
    fi
    
    # Log cycle
    echo "$(date): TDD cycle completed for $module" >> "$CYCLE_LOG"
    echo -e "\n${GREEN}TDD Cycle Complete!${NC}"
}

# Parse arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <module> <test_file>"
    echo "Example: $0 auth tests/test_auth.c"
    exit 1
fi

run_tdd_cycle "$1" "$2"
EOF
    chmod +x "$PROJECT_ROOT/tdd/tdd-cycle.sh"
}

# Update Makefile with waterfall targets
update_makefile() {
    log_info "Updating Makefile with waterfall targets..."
    
    cat >> "$PROJECT_ROOT/Makefile" << 'EOF'

# Waterfall Development Targets
WATERFALL_DIR = .waterfall

waterfall-init:
	@echo "Initializing waterfall development process..."
	@bash scripts/implement-waterfall-cycle.sh

waterfall-status:
	@./waterfall-status.sh

waterfall-check:
	@echo "Checking current phase gates..."
	@bash $(WATERFALL_DIR)/check_gate.sh $$(cat $(WATERFALL_DIR)/current_phase) all

waterfall-advance:
	@echo "Attempting phase transition..."
	@bash $(WATERFALL_DIR)/transition_phase.sh

waterfall-report:
	@echo "Generating phase report..."
	@phase=$$(cat $(WATERFALL_DIR)/current_phase); \
	 echo "Report for phase: $$phase"

# TDD Targets
tdd-cycle:
	@echo "Usage: make tdd-cycle MODULE=<module> TEST=<test_file>"
	@echo "Example: make tdd-cycle MODULE=auth TEST=tests/test_auth.c"
	@[ -n "$(MODULE)" ] && [ -n "$(TEST)" ] && ./tdd/tdd-cycle.sh $(MODULE) $(TEST)

# QA Targets
qa-full:
	@echo "Running full QA cycle..."
	@$(MAKE) test
	@$(MAKE) adhoc-qa
	@$(MAKE) waterfall-check

.PHONY: waterfall-init waterfall-status waterfall-check waterfall-advance waterfall-report tdd-cycle qa-full
EOF
}

# Main execution
main() {
    log_info "Implementing waterfall development methodology..."
    
    # Initialize waterfall structure
    init_waterfall
    
    # Create gate checker
    create_gate_checker
    
    # Create phase transition script
    create_phase_transition
    
    # Create dashboard
    create_dashboard
    
    # Create TDD cycle scripts
    create_tdd_cycle
    
    # Update Makefile
    update_makefile
    
    log_success "Waterfall methodology implemented"
    log_info "Run './waterfall-status.sh' to view current status"
    log_info "Run 'make waterfall-advance' to transition phases"
}

main "$@"
