#!/bin/bash
# scripts/verify-build-system.sh
# Verify OBINexus build system is properly configured

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== OBINexus Build System Verification ==="
echo ""

# Function to check if something exists
check_exists() {
    local type=$1
    local path=$2
    local desc=$3
    
    if [ -$type "$path" ]; then
        echo -e "${GREEN}✓${NC} $desc"
        return 0
    else
        echo -e "${RED}✗${NC} $desc (missing: $path)"
        return 1
    fi
}

# Check makefiles
echo "Checking Makefiles..."
check_exists f "Makefile" "Root Makefile"
check_exists f "Makefile.build" "Build Makefile"
check_exists f "Makefile.projects" "Projects Makefile"
check_exists f "Makefile.purity" "Purity Makefile"
check_exists f "Makefile.spec" "Spec Makefile"
check_exists f "Makefile.vendor" "Vendor Makefile"
echo ""

# Check recursion guards
echo "Checking recursion guards..."
for makefile in Makefile.projects Makefile.purity Makefile.spec Makefile.vendor; do
    if [ -f "$makefile" ]; then
        if grep -q "ifndef.*_INCLUDED" "$makefile" && grep -q "endif.*_INCLUDED" "$makefile"; then
            echo -e "${GREEN}✓${NC} $makefile has recursion guard"
        else
            echo -e "${RED}✗${NC} $makefile missing recursion guard"
        fi
    fi
done
echo ""

# Check directories
echo "Checking directory structure..."
check_exists d "src" "Source directory"
check_exists d "src/core" "Core modules directory"
check_exists d "src/cli" "CLI directory"
check_exists d "src/command" "Command modules directory"
check_exists d "test" "Test directory"
check_exists d "build" "Build directory"
echo ""

# Check core modules
echo "Checking core modules..."
modules="base common polycall config parser schema factory protocol network auth accessibility repl ffi bridges hotwire"
for module in $modules; do
    if [ -d "src/core/$module" ] && [ -f "src/core/$module/$module.c" ]; then
        echo -e "${GREEN}✓${NC} $module module"
    else
        echo -e "${RED}✗${NC} $module module"
    fi
done
echo ""

# Check scripts
echo "Checking scripts..."
check_exists f "scripts/fix-immediate-issue.sh" "Fix script"
if [ -f "scripts/integrate-build-system.sh" ]; then
    echo -e "${GREEN}✓${NC} Integration script (correct name)"
elif [ -f "scripts/intergrate-build-system.sh" ]; then
    echo -e "${YELLOW}⚠${NC} Integration script (typo: intergrate)"
else
    echo -e "${RED}✗${NC} Integration script missing"
fi
echo ""

# Check for QA target
echo "Checking QA targets..."
if grep -q "^qa:" Makefile.spec 2>/dev/null; then
    echo -e "${GREEN}✓${NC} QA target exists in Makefile.spec"
else
    echo -e "${RED}✗${NC} QA target missing in Makefile.spec"
fi
echo ""

# Test basic make commands
echo "Testing basic make commands..."
echo -e "${YELLOW}Running: make clean${NC}"
if make clean >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} make clean works"
else
    echo -e "${RED}✗${NC} make clean failed"
fi

echo -e "${YELLOW}Running: make help${NC}"
if make help >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} make help works"
else
    echo -e "${RED}✗${NC} make help failed"
fi
echo ""

# Summary
echo "=== Summary ==="
echo "If all checks pass, you're ready to:"
echo "1. Run: make build"
echo "2. Run: ./scripts/integrate-build-system.sh"
echo "3. Run: make test-enhanced"
echo ""
echo "For any failures, run ./scripts/fix-immediate-issue.sh again"
