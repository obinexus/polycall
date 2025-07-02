#!/bin/bash
# LibPolyCall Static Analysis Script using cppcheck
# This script performs comprehensive static analysis on the LibPolyCall codebase
# using cppcheck with various configurations and generates formatted reports.

# ANSI color codes for better output readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Banner
echo -e "${BLUE}"
echo "=================================================================="
echo "  LibPolyCall Static Code Analysis"
echo "  Comprehensive Codebase Health Check"
echo "=================================================================="
echo -e "${NC}"

# Configuration variables - modify as needed
PROJECT_ROOT="."
INCLUDE_DIR="include"
SRC_DIR="src"
REPORT_DIR="reports/cppcheck"
SUPPRESSION_FILE=".cppcheck_suppressions"
LOG_FILE="${REPORT_DIR}/cppcheck_full.log"
XML_REPORT="${REPORT_DIR}/cppcheck_report.xml"
HTML_REPORT_DIR="${REPORT_DIR}/html"
ERROR_SUMMARY="${REPORT_DIR}/error_summary.txt"
MODULE_REPORT_DIR="${REPORT_DIR}/modules"

# Cppcheck options
CPPCHECK_STD="c11" # C standard to use
CPPCHECK_PLATFORM="unix64" # Target platform
CPPCHECK_ENABLE="all,warning,style,performance,portability,information,missingInclude,unusedFunction"
CPPCHECK_INCONCLUSIVE="--inconclusive" # Report even when not 100% sure
CPPCHECK_MAX_CONFIGS="--max-configs=25" # Maximum configurations to check
CPPCHECK_FORCE="--force" # Force checking all configurations

# Modules to analyze
MODULES=(
    "core"
    "core/ffi"
    "core/micro"
    "core/edge"
    "core/protocol"
    "core/auth"
    "core/config"
    "core/polycall"
    "network"
    "parser"
)

# Function to print section headers
section() {
    echo -e "\n${CYAN}==== $1 ====${NC}"
}

# Function to print success messages
success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print warning messages
warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Function to print error messages
error() {
    echo -e "${RED}✗ $1${NC}"
}

# Function to check if cppcheck is installed
check_cppcheck() {
    section "Checking Dependencies"
    
    if ! command -v cppcheck &> /dev/null; then
        error "cppcheck is not installed. Please install it first."
        echo "Installation instructions:"
        echo "  Ubuntu/Debian: sudo apt-get install cppcheck"
        echo "  RedHat/CentOS: sudo yum install cppcheck"
        echo "  macOS: brew install cppcheck"
        echo "  Windows: Download from http://cppcheck.sourceforge.net/"
        exit 1
    fi
    
    if ! command -v cppcheck-htmlreport &> /dev/null; then
        warning "cppcheck-htmlreport is not available. HTML reports will be skipped."
        GENERATE_HTML=false
    else
        GENERATE_HTML=true
    fi
    
    success "cppcheck found: $(cppcheck --version)"
}

# Function to create necessary directories
create_directories() {
    section "Creating Directories"
    
    mkdir -p "${REPORT_DIR}"
    mkdir -p "${HTML_REPORT_DIR}"
    mkdir -p "${MODULE_REPORT_DIR}"
    
    for module in "${MODULES[@]}"; do
        mkdir -p "${MODULE_REPORT_DIR}/${module}"
    done
    
    success "Created report directories"
}

# Function to create suppression file if it doesn't exist
create_suppression_file() {
    section "Setting Up Suppression File"
    
    if [ ! -f "${SUPPRESSION_FILE}" ]; then
        echo "Creating suppression file: ${SUPPRESSION_FILE}"
        cat > "${SUPPRESSION_FILE}" << EOF
// Suppress warnings for external libraries
*:${INCLUDE_DIR}/external/*

// Suppress specific warnings
// Specific function or identifiers for which warnings should be suppressed
variableScope:${SRC_DIR}/core/polycall/polycall.c
unusedFunction:${SRC_DIR}/core/ffi/ffi_core.c

// Suppress specific error ids
unmatchedSuppression
missingIncludeSystem
EOF
        success "Created suppression file with default suppressions"
    else
        success "Using existing suppression file"
    fi
}

# Function to count lines of code
count_lines() {
    section "Code Statistics"
    
    C_FILES=$(find "${SRC_DIR}" "${INCLUDE_DIR}" -name "*.c" -o -name "*.h" | wc -l)
    LINES=$(find "${SRC_DIR}" "${INCLUDE_DIR}" -name "*.c" -o -name "*.h" -exec cat {} \; | wc -l)
    
    echo "Total C/H files: ${C_FILES}"
    echo "Total lines of code: ${LINES}"
}

# Function to run cppcheck on a specific module
analyze_module() {
    local module=$1
    local module_path="${SRC_DIR}/${module}"
    local module_report="${MODULE_REPORT_DIR}/${module}/report.txt"
    local module_xml="${MODULE_REPORT_DIR}/${module}/report.xml"
    
    echo "Analyzing module: ${module}"
    
    # Run cppcheck on the module
    cppcheck --std=${CPPCHECK_STD} \
             --platform=${CPPCHECK_PLATFORM} \
             --enable=${CPPCHECK_ENABLE} \
             ${CPPCHECK_INCONCLUSIVE} \
             ${CPPCHECK_FORCE} \
             ${CPPCHECK_MAX_CONFIGS} \
             --suppressions-list=${SUPPRESSION_FILE} \
             --error-exitcode=0 \
             --inline-suppr \
             -j $(nproc) \
             --xml \
             --xml-version=2 \
             -I "${INCLUDE_DIR}" \
             "${module_path}" 2> "${module_xml}"
    
    # Convert XML to readable text
    echo "Module: ${module}" > "${module_report}"
    echo "Path: ${module_path}" >> "${module_report}"
    echo "Date: $(date)" >> "${module_report}"
    echo "=================================================" >> "${module_report}"
    
    # Process XML using grep and sed to extract readable error information
    grep -A 2 "<error " "${module_xml}" | \
        sed -e 's/<error //' -e 's/<\/error>//' -e 's/<location //' -e 's/\/>//' | \
        sed -e 's/file="/File: /' -e 's/" line="/  Line: /' -e 's/" id="/  Error: /' -e 's/" severity="/  Severity: /' -e 's/" msg="/  Message: /' -e 's/".*//' | \
        sed '/^--$/d' >> "${module_report}"
    
    # Count issues by severity
    local error_count=$(grep -c 'Severity: error' "${module_report}")
    local warning_count=$(grep -c 'Severity: warning' "${module_report}")
    local style_count=$(grep -c 'Severity: style' "${module_report}")
    local performance_count=$(grep -c 'Severity: performance' "${module_report}")
    local information_count=$(grep -c 'Severity: information' "${module_report}")
    
    # Generate HTML report if possible
    if [ "${GENERATE_HTML}" = true ]; then
        cppcheck-htmlreport --file="${module_xml}" --report-dir="${MODULE_REPORT_DIR}/${module}/html" --source-dir=.
    fi
    
    # Print summary
    if [ $error_count -gt 0 ]; then
        error "Module ${module}: ${error_count} errors, ${warning_count} warnings, ${style_count} style issues, ${performance_count} performance issues, ${information_count} information"
    elif [ $warning_count -gt 0 ]; then
        warning "Module ${module}: ${error_count} errors, ${warning_count} warnings, ${style_count} style issues, ${performance_count} performance issues, ${information_count} information"
    else
        success "Module ${module}: ${error_count} errors, ${warning_count} warnings, ${style_count} style issues, ${performance_count} performance issues, ${information_count} information"
    fi
    
    # Write to summary
    echo "Module ${module}: ${error_count} errors, ${warning_count} warnings, ${style_count} style issues, ${performance_count} performance issues, ${information_count} information" >> "${ERROR_SUMMARY}"
    
    return $error_count
}

# Function to run cppcheck on all source files
analyze_full_codebase() {
    section "Analyzing Full Codebase"
    
    echo "Running cppcheck on all source files..."
    
    # Run cppcheck on the entire codebase
    cppcheck --std=${CPPCHECK_STD} \
             --platform=${CPPCHECK_PLATFORM} \
             --enable=${CPPCHECK_ENABLE} \
             ${CPPCHECK_INCONCLUSIVE} \
             ${CPPCHECK_FORCE} \
             ${CPPCHECK_MAX_CONFIGS} \
             --suppressions-list=${SUPPRESSION_FILE} \
             --error-exitcode=0 \
             --inline-suppr \
             -j $(nproc) \
             --xml \
             --xml-version=2 \
             -I "${INCLUDE_DIR}" \
             "${SRC_DIR}" 2> "${XML_REPORT}"
    
    # Generate text report
    echo "LibPolyCall Full Codebase Analysis" > "${LOG_FILE}"
    echo "Date: $(date)" >> "${LOG_FILE}"
    echo "=================================================" >> "${LOG_FILE}"
    
    # Process XML using grep and sed to extract readable error information
    grep -A 2 "<error " "${XML_REPORT}" | \
        sed -e 's/<error //' -e 's/<\/error>//' -e 's/<location //' -e 's/\/>//' | \
        sed -e 's/file="/File: /' -e 's/" line="/  Line: /' -e 's/" id="/  Error: /' -e 's/" severity="/  Severity: /' -e 's/" msg="/  Message: /' -e 's/".*//' | \
        sed '/^--$/d' >> "${LOG_FILE}"
    
    # Generate HTML report if possible
    if [ "${GENERATE_HTML}" = true ]; then
        echo "Generating HTML report..."
        cppcheck-htmlreport --file="${XML_REPORT}" --report-dir="${HTML_REPORT_DIR}" --source-dir=.
        success "HTML report generated in ${HTML_REPORT_DIR}"
    fi
    
    # Count issues by severity for the full codebase
    local error_count=$(grep -c 'Severity: error' "${LOG_FILE}")
    local warning_count=$(grep -c 'Severity: warning' "${LOG_FILE}")
    local style_count=$(grep -c 'Severity: style' "${LOG_FILE}")
    local performance_count=$(grep -c 'Severity: performance' "${LOG_FILE}")
    local information_count=$(grep -c 'Severity: information' "${LOG_FILE}")
    
    # Log to summary file
    echo "Total Issues:" > "${ERROR_SUMMARY}"
    echo "  Errors: ${error_count}" >> "${ERROR_SUMMARY}"
    echo "  Warnings: ${warning_count}" >> "${ERROR_SUMMARY}"
    echo "  Style: ${style_count}" >> "${ERROR_SUMMARY}"
    echo "  Performance: ${performance_count}" >> "${ERROR_SUMMARY}"
    echo "  Information: ${information_count}" >> "${ERROR_SUMMARY}"
    echo "" >> "${ERROR_SUMMARY}"
    echo "Module Summary:" >> "${ERROR_SUMMARY}"
    
    # Print summary
    echo ""
    echo "Analysis complete!"
    echo "  Errors: ${error_count}"
    echo "  Warnings: ${warning_count}"
    echo "  Style: ${style_count}"
    echo "  Performance: ${performance_count}"
    echo "  Information: ${information_count}"
    echo ""
    echo "Full report available in: ${LOG_FILE}"
    if [ "${GENERATE_HTML}" = true ]; then
        echo "HTML report available in: ${HTML_REPORT_DIR}/index.html"
    fi
}

# Function to analyze each module separately
analyze_modules() {
    section "Analyzing Individual Modules"
    
    local error_modules=0
    
    for module in "${MODULES[@]}"; do
        analyze_module "${module}"
        if [ $? -gt 0 ]; then
            error_modules=$((error_modules + 1))
        fi
    done
    
    echo ""
    if [ $error_modules -gt 0 ]; then
        error "${error_modules} modules have errors"
    else
        success "All modules analyzed successfully"
    fi
    
    echo "Module reports available in: ${MODULE_REPORT_DIR}"
}

# Function to search for TODO/FIXME comments
find_todos() {
    section "Finding TODO/FIXME Comments"
    
    local todo_file="${REPORT_DIR}/todos.txt"
    
    echo "Searching for TODO/FIXME comments..."
    echo "TODO and FIXME Comments" > "${todo_file}"
    echo "======================" >> "${todo_file}"
    echo "" >> "${todo_file}"
    
    # Find TODO comments
    echo "TODO Comments:" >> "${todo_file}"
    grep -rn "TODO" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | \
        sed 's/^/  /' >> "${todo_file}"
    
    echo "" >> "${todo_file}"
    
    # Find FIXME comments
    echo "FIXME Comments:" >> "${todo_file}"
    grep -rn "FIXME" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | \
        sed 's/^/  /' >> "${todo_file}"
    
    # Count TODO/FIXME comments
    local todo_count=$(grep -r "TODO" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | wc -l)
    local fixme_count=$(grep -r "FIXME" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | wc -l)
    
    if [ $todo_count -gt 0 ] || [ $fixme_count -gt 0 ]; then
        warning "Found ${todo_count} TODO and ${fixme_count} FIXME comments"
    else
        success "No TODO/FIXME comments found"
    fi
    
    echo "TODO/FIXME report available in: ${todo_file}"
}

# Function to analyze compiler compatibility
check_compiler_compatibility() {
    section "Checking Compiler Compatibility"
    
    local compat_report="${REPORT_DIR}/compiler_compatibility.txt"
    
    echo "LibPolyCall Compiler Compatibility Check" > "${compat_report}"
    echo "====================================" >> "${compat_report}"
    echo "Date: $(date)" >> "${compat_report}"
    echo "" >> "${compat_report}"
    
    # Check if GCC is available
    if command -v gcc &> /dev/null; then
        gcc_version=$(gcc --version | head -n 1)
        echo "GCC: ${gcc_version}" >> "${compat_report}"
        
        # Test compilation with GCC
        echo "" >> "${compat_report}"
        echo "GCC Compilation Test:" >> "${compat_report}"
        gcc -c -Wall -Wextra -Werror -I"${INCLUDE_DIR}" -o /dev/null "${SRC_DIR}/core/polycall/polycall.c" 2>> "${compat_report}" || true
    else
        echo "GCC: Not installed" >> "${compat_report}"
    fi
    
    # Check if Clang is available
    if command -v clang &> /dev/null; then
        clang_version=$(clang --version | head -n 1)
        echo "" >> "${compat_report}"
        echo "Clang: ${clang_version}" >> "${compat_report}"
        
        # Test compilation with Clang
        echo "" >> "${compat_report}"
        echo "Clang Compilation Test:" >> "${compat_report}"
        clang -c -Wall -Wextra -Werror -I"${INCLUDE_DIR}" -o /dev/null "${SRC_DIR}/core/polycall/polycall.c" 2>> "${compat_report}" || true
    else
        echo "" >> "${compat_report}"
        echo "Clang: Not installed" >> "${compat_report}"
    fi
    
    success "Compiler compatibility report generated: ${compat_report}"
}

# Function to generate a final summary report
generate_summary() {
    section "Generating Summary Report"
    
    local summary_file="${REPORT_DIR}/analysis_summary.txt"
    
    echo "LibPolyCall Static Analysis Summary" > "${summary_file}"
    echo "=================================" >> "${summary_file}"
    echo "Date: $(date)" >> "${summary_file}"
    echo "" >> "${summary_file}"
    
    # Add code statistics
    echo "Code Statistics:" >> "${summary_file}"
    echo "  Files: $(find "${SRC_DIR}" "${INCLUDE_DIR}" -name "*.c" -o -name "*.h" | wc -l)" >> "${summary_file}"
    echo "  Lines: $(find "${SRC_DIR}" "${INCLUDE_DIR}" -name "*.c" -o -name "*.h" -exec cat {} \; | wc -l)" >> "${summary_file}"
    echo "" >> "${summary_file}"
    
    # Add issue summary
    echo "Issue Summary:" >> "${summary_file}"
    cat "${ERROR_SUMMARY}" >> "${summary_file}"
    echo "" >> "${summary_file}"
    
    # Add TODO/FIXME summary
    echo "TODO/FIXME Summary:" >> "${summary_file}"
    echo "  TODO Comments: $(grep -r "TODO" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | wc -l)" >> "${summary_file}"
    echo "  FIXME Comments: $(grep -r "FIXME" --include="*.c" --include="*.h" "${SRC_DIR}" "${INCLUDE_DIR}" | wc -l)" >> "${summary_file}"
    echo "" >> "${summary_file}"
    
    # Add error rate percentage
    total_lines=$(find "${SRC_DIR}" "${INCLUDE_DIR}" -name "*.c" -o -name "*.h" -exec cat {} \; | wc -l)
    total_issues=$(( $(grep -c 'Severity: error' "${LOG_FILE}") + $(grep -c 'Severity: warning' "${LOG_FILE}") ))
    
    if [ "${total_lines}" -gt 0 ]; then
        issue_rate=$(echo "scale=4; ${total_issues} * 100 / ${total_lines}" | bc)
        echo "Issue Rate: ${issue_rate}% (${total_issues} issues per ${total_lines} lines)" >> "${summary_file}"
    else
        echo "Issue Rate: N/A (no code found)" >> "${summary_file}"
    fi
    
    success "Summary report generated: ${summary_file}"
}

# Main execution flow
main() {
    # Switch to project root directory
    cd "${PROJECT_ROOT}"
    
    # Check if cppcheck is installed
    check_cppcheck
    
    # Create necessary directories
    create_directories
    
    # Create suppression file if it doesn't exist
    create_suppression_file
    
    # Count lines of code
    count_lines
    
    # Analyze full codebase
    analyze_full_codebase
    
    # Analyze individual modules
    analyze_modules
    
    # Find TODO/FIXME comments
    find_todos
    
    # Check compiler compatibility
    check_compiler_compatibility
    
    # Generate summary report
    generate_summary
    
    section "Analysis Complete"
    echo "All reports have been generated in: ${REPORT_DIR}"
    
    # Exit with error code if issues were found
    error_count=$(grep -c 'Severity: error' "${LOG_FILE}")
    if [ $error_count -gt 0 ]; then
        error "Analysis found ${error_count} errors. Please fix them before proceeding."
        exit 1
    else
        success "No critical errors found. The codebase looks good!"
        exit 0
    fi
}

# Run the main function
main