#!/bin/bash
# OBINexus Makefile Recursion Diagnostic Tool
# Identifies and reports recursion issues in makefiles

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "=== OBINexus Makefile Recursion Diagnostic ==="
echo "Analyzing makefile structure for recursion issues..."
echo

# Function to check for recursion guards
check_recursion_guards() {
    echo -e "${BLUE}Checking recursion guards...${NC}"
    
    for makefile in Makefile*; do
        if [ -f "$makefile" ]; then
            if grep -q "ifndef.*_INCLUDED" "$makefile"; then
                echo -e "  ${GREEN}✓${NC} $makefile has recursion guard"
            else
                echo -e "  ${RED}✗${NC} $makefile missing recursion guard"
            fi
        fi
    done
    echo
}

# Function to check for problematic make calls
check_make_calls() {
    echo -e "${BLUE}Checking for problematic make calls...${NC}"
    
    for makefile in Makefile*; do
        if [ -f "$makefile" ]; then
            echo "  Analyzing $makefile:"
            
            # Check for $(MAKE) -C patterns (can cause recursion)
            if grep -q '\$(MAKE).*-C' "$makefile"; then
                echo -e "    ${YELLOW}⚠${NC}  Found \$(MAKE) -C pattern (line $(grep -n '\$(MAKE).*-C' "$makefile" | cut -d: -f1 | head -1))"
            fi
            
            # Check for recursive includes
            if grep -q '^include.*Makefile' "$makefile"; then
                echo -e "    ${YELLOW}⚠${NC}  Found include statement (line $(grep -n '^include.*Makefile' "$makefile" | cut -d: -f1 | head -1))"
            fi
            
            # Check for proper SUBMAKE pattern
            if grep -q 'SUBMAKE.*--no-print-directory' "$makefile"; then
                echo -e "    ${GREEN}✓${NC} Uses proper SUBMAKE pattern"
            fi
        fi
    done
    echo
}

# Function to test actual recursion
test_recursion() {
    echo -e "${BLUE}Testing for actual recursion...${NC}"
    
    targets=("all" "build" "test" "clean")
    
    for target in "${targets[@]}"; do
        echo "  Testing 'make $target':"
        
        # Capture make dry-run output
        output=$(make -n $target 2>&1 || true)
        
        # Count "Entering directory" occurrences
        enter_count=$(echo "$output" | grep -c "Entering directory" || true)
        
        if [ $enter_count -eq 0 ]; then
            echo -e "    ${GREEN}✓${NC} No directory changes"
        elif [ $enter_count -eq 1 ]; then
            echo -e "    ${GREEN}✓${NC} Single directory entry (normal)"
        else
            echo -e "    ${RED}✗${NC} Multiple directory entries ($enter_count) - RECURSION DETECTED"
            
            # Show which makefiles are involved
            echo "$output" | grep "make\[" | head -5 | sed 's/^/      /'
        fi
    done
    echo
}

# Function to generate fix recommendations
generate_recommendations() {
    echo -e "${BLUE}Recommendations:${NC}"
    echo
    
    # Check if fixes are needed
    needs_fix=false
    
    for makefile in Makefile*; do
        if [ -f "$makefile" ]; then
            if ! grep -q "ifndef.*_INCLUDED" "$makefile"; then
                needs_fix=true
                echo "1. Add recursion guard to $makefile:"
                echo "   ifndef $(echo $makefile | tr 'a-z.' 'A-Z_')_INCLUDED"
                echo "   $(echo $makefile | tr 'a-z.' 'A-Z_')_INCLUDED := 1"
                echo "   # ... makefile content ..."
                echo "   endif"
                echo
            fi
        fi
    done
    
    if grep -q '\$(MAKE).*-C' Makefile* 2>/dev/null; then
        needs_fix=true
        echo "2. Replace recursive make calls:"
        echo "   Old: \$(MAKE) -C \$(SRC_DIR)/component"
        echo "   New: @\$(SUBMAKE) Makefile.component target"
        echo
    fi
    
    if ! grep -q 'SUBMAKE.*--no-print-directory' Makefile 2>/dev/null; then
        needs_fix=true
        echo "3. Add SUBMAKE definition to root Makefile:"
        echo "   SUBMAKE = \$(MAKE) --no-print-directory -f"
        echo
    fi
    
    if [ "$needs_fix" = false ]; then
        echo -e "${GREEN}No fixes needed! Your makefiles are properly configured.${NC}"
    fi
}

# Function to create backup
create_backup() {
    echo -e "${BLUE}Creating backup of makefiles...${NC}"
    
    backup_dir="makefile_backup_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$backup_dir"
    
    for makefile in Makefile*; do
        if [ -f "$makefile" ]; then
            cp "$makefile" "$backup_dir/"
            echo "  Backed up $makefile"
        fi
    done
    
    echo -e "${GREEN}Backup created in $backup_dir/${NC}"
    echo
}

# Main diagnostic flow
main() {
    # Create backup first
    create_backup
    
    # Run diagnostics
    check_recursion_guards
    check_make_calls
    test_recursion
    generate_recommendations
    
    echo
    echo "=== Diagnostic Complete ==="
    echo "To apply the fixed makefiles:"
    echo "1. Review the recommendations above"
    echo "2. Use the provided fixed Makefile artifacts"
    echo "3. Test with: ./test-makefiles.sh"
    echo
    echo "Your original makefiles are backed up and safe."
}

# Run main
main
