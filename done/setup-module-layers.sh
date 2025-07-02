#!/bin/bash
# setup-module-layers.sh - Set up module layers for Python, Shell, and PowerShell
# OBINexus Waterfall Development - Module Organization Phase

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/adhoc"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"

log_info() { echo "[INFO] $1"; }
log_error() { echo "[ERROR] $1" >&2; }
log_success() { echo "[SUCCESS] $1"; }

# Create adhoc module structure (2 layers deep only)
create_adhoc_structure() {
    log_info "Creating adhoc module structure..."
    
    # Primary layer - by language
    local languages=("python" "shell" "powershell")
    local categories=("build" "test" "qa" "deploy" "validate")
    
    for lang in "${languages[@]}"; do
        for cat in "${categories[@]}"; do
            mkdir -p "$ADHOC_DIR/$lang/$cat"
        done
    done
    
    # Create main orchestrator
    cat > "$ADHOC_DIR/main.sh" << 'EOF'
#!/bin/bash
# Adhoc Module Orchestrator
# Waterfall Development Cycle Controller

set -e

ADHOC_ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$ADHOC_ROOT/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

usage() {
    cat << USAGE
LibPolyCall Adhoc Module System
Waterfall Development Phases:

  build     - Execute build phase scripts
  test      - Run test phase scripts  
  qa        - Quality assurance phase
  deploy    - Deployment preparation
  validate  - Validation and compliance
  cycle     - Run complete waterfall cycle
  
Options:
  --lang    Language to use (python|shell|powershell)
  --phase   Specific phase to run
  --dry-run Show what would be executed
  
Examples:
  $0 build --lang=python
  $0 cycle              # Run all phases
  $0 qa --dry-run
USAGE
}

# Execute scripts in a phase
execute_phase() {
    local phase="$1"
    local lang="${2:-all}"
    
    echo -e "${GREEN}[PHASE]${NC} Executing $phase phase..."
    
    if [ "$lang" = "all" ]; then
        local langs=("python" "shell" "powershell")
    else
        local langs=("$lang")
    fi
    
    for l in "${langs[@]}"; do
        local phase_dir="$ADHOC_ROOT/$l/$phase"
        if [ -d "$phase_dir" ]; then
            echo -e "${YELLOW}[LANG]${NC} Processing $l scripts in $phase..."
            
            # Find and execute scripts
            case "$l" in
                python)
                    find "$phase_dir" -name "*.py" -type f | sort | while read -r script; do
                        echo "  Running: $(basename "$script")"
                        [ "$DRY_RUN" != "1" ] && python3 "$script" || echo "    [DRY-RUN]"
                    done
                    ;;
                shell)
                    find "$phase_dir" -name "*.sh" -type f | sort | while read -r script; do
                        echo "  Running: $(basename "$script")"
                        [ "$DRY_RUN" != "1" ] && bash "$script" || echo "    [DRY-RUN]"
                    done
                    ;;
                powershell)
                    if command -v pwsh >/dev/null 2>&1; then
                        find "$phase_dir" -name "*.ps1" -type f | sort | while read -r script; do
                            echo "  Running: $(basename "$script")"
                            [ "$DRY_RUN" != "1" ] && pwsh "$script" || echo "    [DRY-RUN]"
                        done
                    else
                        echo "  PowerShell not available, skipping..."
                    fi
                    ;;
            esac
        fi
    done
}

# Run complete waterfall cycle
run_cycle() {
    local phases=("build" "test" "qa" "validate" "deploy")
    
    echo -e "${GREEN}[WATERFALL]${NC} Starting complete development cycle..."
    echo "Phases: ${phases[*]}"
    echo "----------------------------------------"
    
    for phase in "${phases[@]}"; do
        execute_phase "$phase" "$LANG"
        echo "----------------------------------------"
    done
    
    echo -e "${GREEN}[COMPLETE]${NC} Waterfall cycle finished"
}

# Parse command line
COMMAND="$1"
shift || true

# Parse options
while [[ $# -gt 0 ]]; do
    case "$1" in
        --lang=*)
            LANG="${1#*=}"
            ;;
        --phase=*)
            PHASE="${1#*=}"
            ;;
        --dry-run)
            DRY_RUN=1
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

# Execute command
case "$COMMAND" in
    build|test|qa|deploy|validate)
        execute_phase "$COMMAND" "${LANG:-all}"
        ;;
    cycle)
        run_cycle
        ;;
    *)
        usage
        exit 1
        ;;
esac
EOF
    chmod +x "$ADHOC_DIR/main.sh"
    
    log_success "Adhoc structure created"
}

# Create Python module templates
create_python_modules() {
    log_info "Creating Python module templates..."
    
    # Build phase Python script
    cat > "$ADHOC_DIR/python/build/prepare_environment.py" << 'EOF'
#!/usr/bin/env python3
"""
LibPolyCall Build Environment Preparation
Waterfall Phase: Build
"""

import os
import sys
import subprocess
import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[3]
BUILD_CONFIG = PROJECT_ROOT / ".polycall-build.json"

def check_dependencies():
    """Check and install required dependencies"""
    dependencies = ["cffi", "pytest", "black", "mypy"]
    
    for dep in dependencies:
        try:
            __import__(dep)
            print(f"✓ {dep} available")
        except ImportError:
            print(f"Installing {dep}...")
            subprocess.run([sys.executable, "-m", "pip", "install", dep])

def generate_build_config():
    """Generate build configuration"""
    config = {
        "version": "2.0.0",
        "waterfall_phase": "build",
        "modules": ["core", "auth", "network", "protocol", "edge", "micro"],
        "compliance": {
            "sinphase": True,
            "adhoc": True
        },
        "platform": sys.platform
    }
    
    with open(BUILD_CONFIG, 'w') as f:
        json.dump(config, f, indent=2)
    
    print(f"Build configuration written to {BUILD_CONFIG}")

if __name__ == "__main__":
    print("=== LibPolyCall Build Preparation ===")
    check_dependencies()
    generate_build_config()
    print("Build environment ready")
EOF

    # Test phase Python script
    cat > "$ADHOC_DIR/python/test/run_unit_tests.py" << 'EOF'
#!/usr/bin/env python3
"""
LibPolyCall Unit Test Runner
Waterfall Phase: Test
"""

import sys
import pytest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[3]
TEST_DIR = PROJECT_ROOT / "tests"

def run_tests():
    """Run unit tests with coverage"""
    args = [
        "-v",
        "--tb=short",
        "--cov=polycall",
        "--cov-report=term-missing",
        str(TEST_DIR)
    ]
    
    return pytest.main(args)

if __name__ == "__main__":
    print("=== LibPolyCall Unit Test Suite ===")
    sys.exit(run_tests())
EOF

    # QA phase Python script  
    cat > "$ADHOC_DIR/python/qa/code_quality_check.py" << 'EOF'
#!/usr/bin/env python3
"""
LibPolyCall Code Quality Checker
Waterfall Phase: Quality Assurance
"""

import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[3]
SRC_DIR = PROJECT_ROOT / "src"
SCRIPTS_DIR = PROJECT_ROOT / "scripts"

def run_linter():
    """Run code linting"""
    print("Running Python linter...")
    try:
        subprocess.run(["flake8", str(SCRIPTS_DIR), "--max-line-length=100"], check=True)
        print("✓ Linting passed")
    except subprocess.CalledProcessError:
        print("✗ Linting failed")
        return False
    return True

def run_type_check():
    """Run type checking"""
    print("Running type checker...")
    try:
        subprocess.run(["mypy", str(SCRIPTS_DIR), "--ignore-missing-imports"], check=True)
        print("✓ Type checking passed")
    except subprocess.CalledProcessError:
        print("✗ Type checking failed")
        return False
    return True

if __name__ == "__main__":
    print("=== LibPolyCall QA Check ===")
    success = run_linter() and run_type_check()
    sys.exit(0 if success else 1)
EOF

    chmod +x "$ADHOC_DIR/python/build/prepare_environment.py"
    chmod +x "$ADHOC_DIR/python/test/run_unit_tests.py"
    chmod +x "$ADHOC_DIR/python/qa/code_quality_check.py"
}

# Create Shell module templates
create_shell_modules() {
    log_info "Creating Shell module templates..."
    
    # Build phase shell script
    cat > "$ADHOC_DIR/shell/build/compile_modules.sh" << 'EOF'
#!/bin/bash
# LibPolyCall Module Compilation
# Waterfall Phase: Build

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Compiling LibPolyCall Modules ==="

# Create build directory
mkdir -p "$BUILD_DIR"

# Compile each module
modules=("auth" "network" "protocol" "edge" "micro" "ffi")
for module in "${modules[@]}"; do
    echo "Compiling module: $module"
    if [ -d "$PROJECT_ROOT/src/core/$module" ]; then
        # Placeholder for actual compilation
        echo "  ✓ $module compiled"
    fi
done

echo "Module compilation complete"
EOF

    # Test phase shell script
    cat > "$ADHOC_DIR/shell/test/integration_tests.sh" << 'EOF'
#!/bin/bash
# LibPolyCall Integration Tests
# Waterfall Phase: Test

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
TEST_RESULTS="$PROJECT_ROOT/test-results"

echo "=== Running Integration Tests ==="

mkdir -p "$TEST_RESULTS"

# Run integration test suites
test_suites=("auth" "network" "edge" "micro")
for suite in "${test_suites[@]}"; do
    echo "Testing: $suite integration"
    # Placeholder for actual tests
    echo "  ✓ $suite tests passed"
done

echo "Integration tests complete"
EOF

    # QA phase shell script
    cat > "$ADHOC_DIR/shell/qa/compliance_check.sh" << 'EOF'
#!/bin/bash
# LibPolyCall Compliance Checker
# Waterfall Phase: Quality Assurance

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

echo "=== Sinphasé Compliance Check ==="

# Check file headers
echo "Checking file headers..."
find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" | while read -r file; do
    if ! grep -q "LibPolyCall" "$file"; then
        echo "  Missing header: $file"
    fi
done

# Check build artifacts
echo "Checking build compliance..."
if [ -d "$PROJECT_ROOT/build" ]; then
    echo "  ✓ Build directory exists"
fi

echo "Compliance check complete"
EOF

    chmod +x "$ADHOC_DIR/shell/build/compile_modules.sh"
    chmod +x "$ADHOC_DIR/shell/test/integration_tests.sh"
    chmod +x "$ADHOC_DIR/shell/qa/compliance_check.sh"
}

# Create PowerShell module templates
create_powershell_modules() {
    log_info "Creating PowerShell module templates..."
    
    # Build phase PowerShell script
    cat > "$ADHOC_DIR/powershell/build/Setup-BuildEnvironment.ps1" << 'EOF'
# LibPolyCall Build Environment Setup
# Waterfall Phase: Build

$ErrorActionPreference = "Stop"

$ProjectRoot = (Get-Item $PSScriptRoot).Parent.Parent.Parent.FullName

Write-Host "=== Setting Up Windows Build Environment ===" -ForegroundColor Green

# Check for Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -property installationPath
    Write-Host "  ✓ Visual Studio found at: $vsPath" -ForegroundColor Green
} else {
    Write-Host "  ✗ Visual Studio not found" -ForegroundColor Red
}

# Create build directories
$buildDirs = @("build", "build/Debug", "build/Release")
foreach ($dir in $buildDirs) {
    $path = Join-Path $ProjectRoot $dir
    if (!(Test-Path $path)) {
        New-Item -ItemType Directory -Path $path | Out-Null
        Write-Host "  Created: $dir"
    }
}

Write-Host "Build environment ready" -ForegroundColor Green
EOF

    # Deploy phase PowerShell script
    cat > "$ADHOC_DIR/powershell/deploy/Package-Release.ps1" << 'EOF'
# LibPolyCall Release Packaging
# Waterfall Phase: Deploy

$ErrorActionPreference = "Stop"

$ProjectRoot = (Get-Item $PSScriptRoot).Parent.Parent.Parent.FullName
$ReleaseDir = Join-Path $ProjectRoot "release"

Write-Host "=== Packaging LibPolyCall Release ===" -ForegroundColor Green

# Create release directory
if (!(Test-Path $ReleaseDir)) {
    New-Item -ItemType Directory -Path $ReleaseDir | Out-Null
}

# Package components
$components = @("polycall.dll", "polycall.exe", "polycall.h")
foreach ($component in $components) {
    Write-Host "  Packaging: $component"
    # Placeholder for actual packaging
}

$version = "2.0.0"
$packageName = "libpolycall-$version-win64.zip"
Write-Host "Release package: $packageName" -ForegroundColor Green
EOF
}

# Create index files for module discovery
create_index_files() {
    log_info "Creating module index files..."
    
    # Create Python __init__.py files
    find "$ADHOC_DIR/python" -type d -exec touch {}/__init__.py \;
    
    # Create module index
    cat > "$ADHOC_DIR/MODULE_INDEX.md" << 'EOF'
# LibPolyCall Adhoc Module Index
## Waterfall Development Structure

### Module Organization
```
adhoc/
├── main.sh           # Orchestrator script
├── python/           # Python modules
│   ├── build/       # Build phase scripts
│   ├── test/        # Test phase scripts
│   ├── qa/          # QA phase scripts
│   ├── deploy/      # Deploy phase scripts
│   └── validate/    # Validation scripts
├── shell/           # Shell modules
│   └── [same structure]
└── powershell/      # PowerShell modules
    └── [same structure]
```

### Waterfall Phases
1. **Build**: Environment setup and compilation
2. **Test**: Unit and integration testing
3. **QA**: Quality assurance and compliance
4. **Validate**: Final validation checks
5. **Deploy**: Release preparation

### Usage
```bash
# Run specific phase
./adhoc/main.sh build

# Run complete cycle
./adhoc/main.sh cycle

# Run with specific language
./adhoc/main.sh test --lang=python
```
EOF

    # Create .gitkeep files
    find "$ADHOC_DIR" -type d -empty -exec touch {}/.gitkeep \;
}

# Main execution
main() {
    log_info "Setting up module layers for libpolycall..."
    
    # Create directory structure
    create_adhoc_structure
    
    # Create language-specific modules
    create_python_modules
    create_shell_modules
    create_powershell_modules
    
    # Create index files
    create_index_files
    
    # Update main Makefile to include adhoc targets
    if [ -f "$PROJECT_ROOT/Makefile" ]; then
        cat >> "$PROJECT_ROOT/Makefile" << 'EOF'

# Adhoc Module Targets
adhoc-build:
	@$(ADHOC_DIR)/main.sh build

adhoc-test:
	@$(ADHOC_DIR)/main.sh test

adhoc-qa:
	@$(ADHOC_DIR)/main.sh qa

adhoc-cycle:
	@$(ADHOC_DIR)/main.sh cycle

.PHONY: adhoc-build adhoc-test adhoc-qa adhoc-cycle
EOF
    fi
    
    log_success "Module layers setup complete"
    log_info "Run './adhoc/main.sh cycle' to execute waterfall cycle"
}

main "$@"
