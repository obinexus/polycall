#!/bin/bash
# fix-adhoc-compliance.sh - Ensure adhoc scripts comply with build system
# OBINexus Waterfall Development - Compliance Integration Phase

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/adhoc"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"
COMPLIANCE_DIR="$PROJECT_ROOT/.compliance"

log_info() { echo "[INFO] $1"; }
log_error() { echo "[ERROR] $1" >&2; }
log_success() { echo "[SUCCESS] $1"; }
log_warn() { echo "[WARN] $1"; }

# Initialize compliance framework
init_compliance() {
    log_info "Initializing adhoc compliance framework..."
    
    mkdir -p "$COMPLIANCE_DIR"/{policies,validators,reports,traces}
    
    # Create compliance policy
    cat > "$COMPLIANCE_DIR/adhoc-policy.json" << 'EOF'
{
  "version": "2.0.0",
  "policy": "sinphase-governance",
  "rules": {
    "script_headers": {
      "required": true,
      "pattern": "^#!/.*\\n# .*\\n# .*Phase:.*"
    },
    "error_handling": {
      "set_e": true,
      "trap_errors": false
    },
    "logging": {
      "required": true,
      "functions": ["log_info", "log_error", "log_success"]
    },
    "project_root": {
      "required": true,
      "pattern": "PROJECT_ROOT=.*"
    },
    "file_permissions": {
      "executable": true,
      "mode": "755"
    }
  },
  "validators": {
    "python": {
      "linter": "flake8",
      "formatter": "black",
      "type_checker": "mypy"
    },
    "shell": {
      "linter": "shellcheck",
      "formatter": "shfmt"
    },
    "powershell": {
      "linter": "PSScriptAnalyzer"
    }
  }
}
EOF

    log_success "Compliance framework initialized"
}

# Create script validator
create_validator() {
    cat > "$COMPLIANCE_DIR/validate_script.sh" << 'EOF'
#!/bin/bash
# Adhoc Script Validator
# Validates scripts against compliance policy

set -e

SCRIPT="$1"
COMPLIANCE_DIR="$(cd "$(dirname "$0")" && pwd)"
POLICY_FILE="$COMPLIANCE_DIR/adhoc-policy.json"

# Detect script type
detect_type() {
    case "${1##*.}" in
        py) echo "python" ;;
        sh) echo "shell" ;;
        ps1) echo "powershell" ;;
        *) echo "unknown" ;;
    esac
}

# Validate script header
check_header() {
    local script="$1"
    local first_lines=$(head -n 3 "$script")
    
    # Check shebang
    if ! head -n 1 "$script" | grep -q "^#!"; then
        echo "ERROR: Missing shebang line"
        return 1
    fi
    
    # Check description and phase
    if ! echo "$first_lines" | grep -q "Phase:"; then
        echo "ERROR: Missing phase declaration"
        return 1
    fi
    
    return 0
}

# Validate error handling
check_error_handling() {
    local script="$1"
    local type=$(detect_type "$script")
    
    case "$type" in
        shell)
            if ! grep -q "set -e" "$script"; then
                echo "WARN: Missing 'set -e' for error handling"
            fi
            ;;
        python)
            if ! grep -q "try:" "$script"; then
                echo "WARN: No exception handling found"
            fi
            ;;
    esac
}

# Validate logging
check_logging() {
    local script="$1"
    
    if ! grep -qE "(log_info|print|Write-Host)" "$script"; then
        echo "WARN: No logging functions found"
    fi
}

# Check file permissions
check_permissions() {
    local script="$1"
    
    if [ ! -x "$script" ]; then
        echo "ERROR: Script not executable"
        return 1
    fi
    
    return 0
}

# Run language-specific linters
run_linter() {
    local script="$1"
    local type=$(detect_type "$script")
    
    case "$type" in
        python)
            if command -v flake8 >/dev/null 2>&1; then
                flake8 "$script" --max-line-length=100 || echo "WARN: Linting issues found"
            fi
            ;;
        shell)
            if command -v shellcheck >/dev/null 2>&1; then
                shellcheck "$script" || echo "WARN: ShellCheck issues found"
            fi
            ;;
    esac
}

# Main validation
echo "Validating: $SCRIPT"
echo "Type: $(detect_type "$SCRIPT")"
echo "-----------------------------------"

ERRORS=0

# Run checks
check_header "$SCRIPT" || ((ERRORS++))
check_error_handling "$SCRIPT"
check_logging "$SCRIPT"
check_permissions "$SCRIPT" || ((ERRORS++))
run_linter "$SCRIPT"

if [ $ERRORS -eq 0 ]; then
    echo "✓ Validation PASSED"
    exit 0
else
    echo "✗ Validation FAILED ($ERRORS errors)"
    exit 1
fi
EOF
    chmod +x "$COMPLIANCE_DIR/validate_script.sh"
}

# Create compliance enforcer
create_enforcer() {
    cat > "$COMPLIANCE_DIR/enforce_compliance.sh" << 'EOF'
#!/bin/bash
# Compliance Enforcer
# Automatically fixes common compliance issues

set -e

SCRIPT="$1"
BACKUP="${SCRIPT}.backup"

# Create backup
cp "$SCRIPT" "$BACKUP"

# Fix script header
fix_header() {
    local script="$1"
    local type="${script##*.}"
    local phase=$(basename "$(dirname "$script")")
    
    # Check if header exists
    if ! head -n 3 "$script" | grep -q "Phase:"; then
        # Create proper header
        local shebang=$(head -n 1 "$script")
        local content=$(tail -n +2 "$script")
        
        cat > "$script" << HEADER
$shebang
# $(basename "$script" | sed 's/\.[^.]*$//' | sed 's/_/ /g' | sed 's/\b\(.\)/\u\1/g')
# Waterfall Phase: ${phase^}

HEADER
        echo "$content" >> "$script"
    fi
}

# Add error handling
add_error_handling() {
    local script="$1"
    local type="${script##*.}"
    
    case "$type" in
        sh)
            if ! grep -q "set -e" "$script"; then
                sed -i '2i\set -e\n' "$script"
            fi
            ;;
        py)
            # Python error handling is more complex, skip auto-fix
            ;;
    esac
}

# Add project root detection
add_project_root() {
    local script="$1"
    local type="${script##*.}"
    
    case "$type" in
        sh)
            if ! grep -q "PROJECT_ROOT=" "$script"; then
                sed -i '/^set -e/a\\nPROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"' "$script"
            fi
            ;;
        py)
            if ! grep -q "PROJECT_ROOT" "$script"; then
                sed -i '/^import/a\\nfrom pathlib import Path\nPROJECT_ROOT = Path(__file__).resolve().parents[3]' "$script"
            fi
            ;;
    esac
}

# Fix permissions
fix_permissions() {
    local script="$1"
    chmod +x "$script"
}

# Apply fixes
echo "Enforcing compliance on: $SCRIPT"
fix_header "$SCRIPT"
add_error_handling "$SCRIPT"
add_project_root "$SCRIPT"
fix_permissions "$SCRIPT"

echo "✓ Compliance enforced"
echo "Backup saved to: $BACKUP"
EOF
    chmod +x "$COMPLIANCE_DIR/enforce_compliance.sh"
}

# Create trace system initializer
create_trace_system() {
    log_info "Creating adhoc trace system..."
    
    cat > "$COMPLIANCE_DIR/init_trace.sh" << 'EOF'
#!/bin/bash
# Adhoc Trace System Initializer
# Tracks script execution for compliance

set -e

TRACE_DIR="$(cd "$(dirname "$0")" && pwd)/traces"
TRACE_ID="$(date +%Y%m%d_%H%M%S)_$$"
TRACE_FILE="$TRACE_DIR/trace_$TRACE_ID.log"

mkdir -p "$TRACE_DIR"

# Create trace wrapper
cat > "$TRACE_DIR/trace_wrapper.sh" << 'WRAPPER'
#!/bin/bash
# Trace Wrapper
# Usage: trace_wrapper.sh <script> [args...]

SCRIPT="$1"
shift
TRACE_FILE="$TRACE_DIR/trace_$(date +%Y%m%d_%H%M%S)_$$.log"

{
    echo "=== TRACE START ==="
    echo "Script: $SCRIPT"
    echo "Args: $*"
    echo "Time: $(date)"
    echo "User: $USER"
    echo "PWD: $PWD"
    echo "==================="
    echo ""
    
    # Execute script with timing
    time "$SCRIPT" "$@" 2>&1
    EXIT_CODE=$?
    
    echo ""
    echo "==================="
    echo "Exit Code: $EXIT_CODE"
    echo "Time: $(date)"
    echo "=== TRACE END ==="
} | tee "$TRACE_FILE"

exit $EXIT_CODE
WRAPPER
chmod +x "$TRACE_DIR/trace_wrapper.sh"

echo "Trace system initialized"
echo "Use: $TRACE_DIR/trace_wrapper.sh <script> [args...]"
EOF
    chmod +x "$COMPLIANCE_DIR/init_trace.sh"
    
    # Run trace initializer
    "$COMPLIANCE_DIR/init_trace.sh"
}

# Create compliance report generator
create_report_generator() {
    cat > "$COMPLIANCE_DIR/generate_report.py" << 'EOF'
#!/usr/bin/env python3
"""
Adhoc Compliance Report Generator
Generates compliance reports for all adhoc scripts
"""

import os
import json
import subprocess
from pathlib import Path
from datetime import datetime

COMPLIANCE_DIR = Path(__file__).parent
PROJECT_ROOT = COMPLIANCE_DIR.parent
ADHOC_DIR = PROJECT_ROOT / "adhoc"

def scan_scripts():
    """Find all scripts in adhoc directory"""
    scripts = []
    for ext in ["*.py", "*.sh", "*.ps1"]:
        scripts.extend(ADHOC_DIR.rglob(ext))
    return scripts

def validate_script(script_path):
    """Validate a single script"""
    validator = COMPLIANCE_DIR / "validate_script.sh"
    result = subprocess.run(
        [str(validator), str(script_path)],
        capture_output=True,
        text=True
    )
    
    return {
        "path": str(script_path.relative_to(PROJECT_ROOT)),
        "valid": result.returncode == 0,
        "output": result.stdout + result.stderr
    }

def generate_report():
    """Generate compliance report"""
    scripts = scan_scripts()
    results = []
    
    print(f"Scanning {len(scripts)} scripts...")
    
    for script in scripts:
        print(f"  Validating: {script.name}")
        results.append(validate_script(script))
    
    # Calculate statistics
    total = len(results)
    valid = sum(1 for r in results if r["valid"])
    invalid = total - valid
    
    # Generate report
    report = {
        "timestamp": datetime.now().isoformat(),
        "summary": {
            "total_scripts": total,
            "valid": valid,
            "invalid": invalid,
            "compliance_rate": f"{(valid/total*100):.1f}%" if total > 0 else "N/A"
        },
        "details": results
    }
    
    # Save report
    report_file = COMPLIANCE_DIR / "reports" / f"compliance_{datetime.now():%Y%m%d_%H%M%S}.json"
    report_file.parent.mkdir(exist_ok=True)
    
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\nCompliance Report:")
    print(f"  Total Scripts: {total}")
    print(f"  Valid: {valid}")
    print(f"  Invalid: {invalid}")
    print(f"  Compliance Rate: {report['summary']['compliance_rate']}")
    print(f"\nReport saved to: {report_file}")
    
    return report

if __name__ == "__main__":
    generate_report()
EOF
    chmod +x "$COMPLIANCE_DIR/generate_report.py"
}

# Fix existing scripts
fix_existing_scripts() {
    log_info "Fixing existing adhoc scripts for compliance..."
    
    # Find all scripts
    local scripts=()
    while IFS= read -r -d '' script; do
        scripts+=("$script")
    done < <(find "$ADHOC_DIR" -type f \( -name "*.sh" -o -name "*.py" -o -name "*.ps1" \) -print0 2>/dev/null)
    
    local fixed=0
    local failed=0
    
    for script in "${scripts[@]}"; do
        echo -n "  Fixing: $(basename "$script")... "
        if "$COMPLIANCE_DIR/enforce_compliance.sh" "$script" >/dev/null 2>&1; then
            echo "✓"
            ((fixed++))
        else
            echo "✗"
            ((failed++))
        fi
    done
    
    log_info "Fixed $fixed scripts, $failed failed"
}

# Update Makefile with compliance targets
update_makefile() {
    log_info "Adding compliance targets to Makefile..."
    
    cat >> "$PROJECT_ROOT/Makefile" << 'EOF'

# Adhoc Compliance Targets
COMPLIANCE_DIR = .compliance

compliance-init:
	@bash scripts/fix-adhoc-compliance.sh

compliance-check:
	@echo "Running compliance check on adhoc scripts..."
	@python3 $(COMPLIANCE_DIR)/generate_report.py

compliance-fix:
	@echo "Fixing compliance issues..."
	@find adhoc -type f \( -name "*.sh" -o -name "*.py" \) -exec $(COMPLIANCE_DIR)/enforce_compliance.sh {} \;

compliance-validate: compliance-check
	@echo "Validation complete"

trace-init:
	@$(COMPLIANCE_DIR)/init_trace.sh

# Policy injection (Sinphasé governance)
policy-inject:
	@echo "Injecting Sinphasé policies..."
	@cp $(COMPLIANCE_DIR)/adhoc-policy.json $(ADHOC_DIR)/
	@echo "✓ Policies injected"

.PHONY: compliance-init compliance-check compliance-fix compliance-validate trace-init policy-inject
EOF
}

# Main execution
main() {
    log_info "Setting up adhoc compliance system..."
    
    # Initialize compliance framework
    init_compliance
    
    # Create validator
    create_validator
    
    # Create enforcer
    create_enforcer
    
    # Create trace system
    create_trace_system
    
    # Create report generator
    create_report_generator
    
    # Fix existing scripts
    if [ -d "$ADHOC_DIR" ]; then
        fix_existing_scripts
    fi
    
    # Update Makefile
    update_makefile
    
    log_success "Adhoc compliance system established"
    log_info "Run 'make compliance-check' to validate scripts"
    log_info "Run 'make compliance-fix' to auto-fix issues"
}

main "$@"
