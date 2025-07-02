#!/bin/bash
# Enhanced Refactoring Script with Sinphasé Policy Enforcement
# OBINexus Aegis Project - Ad-hoc Compliance Module Refactoring
# Collaboration: Nnamdi Okpala

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"
POLICY_DIR="$PROJECT_ROOT/scripts/policies"
REFACTOR_LOG="$PROJECT_ROOT/logs/refactor_$(date +%Y%m%d_%H%M%S).log"

# Configuration
POLICY_MODE=false
DRY_RUN=false
MODULES=""
VERBOSE=false

# Parse arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --policy-mode)
                POLICY_MODE=true
                shift
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --modules)
                MODULES="$2"
                shift 2
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

show_usage() {
    cat << EOF
Enhanced Refactoring Script - Sinphasé Policy Enforcement

Usage: $0 [OPTIONS]

Options:
    --policy-mode    Enable full Sinphasé policy enforcement
    --dry-run        Preview changes without applying them
    --modules LIST   Comma-separated list of modules to refactor
                     (default: all modules)
    --verbose, -v    Enable verbose output
    --help, -h       Show this help message

Modules:
    core/auth        Authentication module
    core/edge        Edge computing module
    core/ffi         Foreign Function Interface
    core/micro       Microservices module
    core/network     Network module
    core/protocol    Protocol module
    core/telemetry   Telemetry module
    cli/*            All CLI components

Examples:
    # Refactor all modules with policy enforcement
    $0 --policy-mode

    # Dry run for specific modules
    $0 --dry-run --modules "core/auth,core/ffi"

    # Verbose refactoring of CLI components
    $0 --verbose --modules "cli/*"
EOF
}

# Logging functions
log() {
    local level="$1"
    shift
    local message="$@"
    local timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    echo "[$timestamp] [$level] $message" | tee -a "$REFACTOR_LOG"
}

log_info() {
    log "INFO" "$@"
}

log_warn() {
    log "WARN" "$@"
}

log_error() {
    log "ERROR" "$@"
}

log_debug() {
    if [ "$VERBOSE" = true ]; then
        log "DEBUG" "$@"
    fi
}

# Initialize refactoring environment
init_refactor() {
    log_info "=== Enhanced Refactoring Session ==="
    log_info "Policy Mode: $POLICY_MODE"
    log_info "Dry Run: $DRY_RUN"
    log_info "Log File: $REFACTOR_LOG"
    
    # Create necessary directories
    mkdir -p "$(dirname "$REFACTOR_LOG")"
    mkdir -p "$PROJECT_ROOT/backup/refactor_$(date +%Y%m%d)"
    
    # Initialize execution context if policy mode
    if [ "$POLICY_MODE" = true ]; then
        EXEC_ID="refactor_$(date +%Y%m%d_%H%M%S)_$$"
        export SINPHASE_EXEC_ID="$EXEC_ID"
        
        # Start trace session
        if [ -f "$ADHOC_DIR/tracer-root.sh" ]; then
            "$ADHOC_DIR/tracer-root.sh" start "$EXEC_ID" "$0" "strict"
        fi
    fi
}

# Detect modules to refactor
detect_modules() {
    if [ -n "$MODULES" ]; then
        # Use specified modules
        echo "$MODULES" | tr ',' '\n'
    else
        # Auto-detect all modules
        find "$PROJECT_ROOT/src/core" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort
        find "$PROJECT_ROOT/src/cli" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sed 's/^/cli\//' | sort
    fi
}

# Pre-refactor validation
validate_module() {
    local module="$1"
    local module_path=""
    
    # Determine module path
    if [[ "$module" == cli/* ]]; then
        module_path="$PROJECT_ROOT/src/$module"
    else
        module_path="$PROJECT_ROOT/src/core/$module"
    fi
    
    log_info "Validating module: $module"
    
    # Check module exists
    if [ ! -d "$module_path" ]; then
        log_error "Module directory not found: $module_path"
        return 1
    fi
    
    # Check for source files
    local file_count=$(find "$module_path" -name "*.c" -o -name "*.h" | wc -l)
    if [ "$file_count" -eq 0 ]; then
        log_warn "No source files found in module: $module"
        return 1
    fi
    
    log_debug "Found $file_count source files in $module"
    
    # Policy validation if enabled
    if [ "$POLICY_MODE" = true ] && [ -f "$POLICY_DIR/pre/validate_script.sh" ]; then
        log_debug "Running policy validation for $module"
        if ! "$POLICY_DIR/pre/validate_script.sh" "$module_path" >/dev/null 2>&1; then
            log_warn "Module failed policy validation: $module"
        fi
    fi
    
    return 0
}

# Core refactoring operations
refactor_module() {
    local module="$1"
    local module_path=""
    local include_path=""
    
    # Determine paths
    if [[ "$module" == cli/* ]]; then
        module_path="$PROJECT_ROOT/src/$module"
        include_path="$PROJECT_ROOT/include/polycall/$module"
    else
        module_path="$PROJECT_ROOT/src/core/$module"
        include_path="$PROJECT_ROOT/include/polycall/core/$module"
    fi
    
    log_info "Refactoring module: $module"
    
    # Create backup if not dry run
    if [ "$DRY_RUN" = false ]; then
        backup_module "$module" "$module_path"
    fi
    
    # Execute refactoring operations
    refactor_includes "$module_path" "$include_path"
    refactor_structure "$module_path"
    refactor_naming "$module_path"
    refactor_dependencies "$module_path"
    
    # Apply Sinphasé principles
    if [ "$POLICY_MODE" = true ]; then
        apply_sinphase_principles "$module_path"
    fi
    
    log_info "Module refactoring complete: $module"
}

# Backup module before refactoring
backup_module() {
    local module="$1"
    local module_path="$2"
    local backup_dir="$PROJECT_ROOT/backup/refactor_$(date +%Y%m%d)/$module"
    
    log_debug "Creating backup: $backup_dir"
    mkdir -p "$backup_dir"
    cp -r "$module_path"/* "$backup_dir/" 2>/dev/null || true
}

# Refactor include paths
refactor_includes() {
    local src_path="$1"
    local include_path="$2"
    
    log_debug "Refactoring includes in $src_path"
    
    # Find all C source files
    find "$src_path" -name "*.c" -o -name "*.h" | while read -r file; do
        if [ "$DRY_RUN" = true ]; then
            log_info "[DRY RUN] Would refactor includes in: $file"
            grep -E '^#include\s+"' "$file" | head -5 || true
        else
            # Fix include paths to use standard format
            sed -i.bak \
                -e 's|#include\s*"\.\./\.\./include/polycall/|#include "polycall/|g' \
                -e 's|#include\s*"\.\./\.\./\.\./include/polycall/|#include "polycall/|g' \
                -e 's|#include\s*"include/polycall/|#include "polycall/|g' \
                -e 's|#include\s*<polycall\.h>|#include "polycall/polycall.h"|g' \
                "$file"
            
            # Remove backup if changes were successful
            if diff -q "$file" "$file.bak" >/dev/null 2>&1; then
                rm -f "$file.bak"
            else
                log_debug "Updated includes in: $(basename "$file")"
            fi
        fi
    done
}

# Refactor module structure
refactor_structure() {
    local module_path="$1"
    
    log_debug "Refactoring structure in $module_path"
    
    # Ensure proper directory structure
    local required_dirs="internal tests docs"
    
    for dir in $required_dirs; do
        if [ ! -d "$module_path/$dir" ]; then
            if [ "$DRY_RUN" = true ]; then
                log_info "[DRY RUN] Would create directory: $module_path/$dir"
            else
                mkdir -p "$module_path/$dir"
                log_debug "Created directory: $module_path/$dir"
            fi
        fi
    done
    
    # Move internal headers if needed
    find "$module_path" -name "*_internal.h" -o -name "*_private.h" | while read -r file; do
        local basename=$(basename "$file")
        local target="$module_path/internal/$basename"
        
        if [ "$file" != "$target" ]; then
            if [ "$DRY_RUN" = true ]; then
                log_info "[DRY RUN] Would move internal header: $file -> internal/"
            else
                mv "$file" "$target"
                log_debug "Moved internal header: $basename"
            fi
        fi
    done
}

# Refactor naming conventions
refactor_naming() {
    local module_path="$1"
    local module_name=$(basename "$module_path")
    
    log_debug "Refactoring naming conventions in $module_path"
    
    # Ensure consistent prefixing
    find "$module_path" -name "*.c" -o -name "*.h" | while read -r file; do
        if [ "$DRY_RUN" = true ]; then
            # Check for functions without proper prefix
            if grep -E "^[a-zA-Z_]+ [a-z][a-zA-Z0-9_]*\(" "$file" | grep -v "^static" | grep -v "polycall_" | head -3; then
                log_info "[DRY RUN] Found unprefixed functions in: $(basename "$file")"
            fi
        else
            # This would be too invasive for automatic refactoring
            # Just log for manual review
            local unprefixed=$(grep -E "^[a-zA-Z_]+ [a-z][a-zA-Z0-9_]*\(" "$file" | grep -v "^static" | grep -v "polycall_" | wc -l)
            if [ "$unprefixed" -gt 0 ]; then
                log_warn "Manual review needed: $unprefixed unprefixed functions in $(basename "$file")"
            fi
        fi
    done
}

# Refactor dependencies
refactor_dependencies() {
    local module_path="$1"
    
    log_debug "Analyzing dependencies for $module_path"
    
    # Create dependency graph
    local dep_file="$module_path/dependencies.md"
    
    if [ "$DRY_RUN" = true ]; then
        log_info "[DRY RUN] Would generate dependency analysis: $dep_file"
    else
        {
            echo "# Module Dependencies"
            echo "Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
            echo
            echo "## Internal Dependencies"
            
            # Analyze internal includes
            grep -h '#include "polycall/' "$module_path"/*.c 2>/dev/null | \
                sed 's/#include "//' | sed 's/"//' | sort | uniq | \
                while read -r inc; do
                    echo "- $inc"
                done
            
            echo
            echo "## External Dependencies"
            
            # Analyze system includes
            grep -h '#include <' "$module_path"/*.c 2>/dev/null | \
                sed 's/#include <//' | sed 's/>//' | sort | uniq | \
                while read -r inc; do
                    echo "- $inc"
                done
        } > "$dep_file"
        
        log_debug "Generated dependency analysis: $dep_file"
    fi
}

# Apply Sinphasé principles
apply_sinphase_principles() {
    local module_path="$1"
    
    log_info "Applying Sinphasé principles to $module_path"
    
    # Principle 1: Zero-trust boundaries
    apply_zero_trust "$module_path"
    
    # Principle 2: Contract enforcement
    apply_contracts "$module_path"
    
    # Principle 3: Audit trails
    apply_audit_trails "$module_path"
    
    # Principle 4: Isolation enforcement
    apply_isolation "$module_path"
}

# Apply zero-trust boundaries
apply_zero_trust() {
    local module_path="$1"
    local boundary_file="$module_path/internal/boundaries.h"
    
    if [ "$DRY_RUN" = true ]; then
        log_info "[DRY RUN] Would create zero-trust boundaries: $boundary_file"
    else
        mkdir -p "$(dirname "$boundary_file")"
        cat > "$boundary_file" << 'EOF'
/*
 * Zero-Trust Boundaries - Sinphasé Enforcement
 * Auto-generated by enhanced refactoring
 * OBINexus Aegis Project
 */

#ifndef POLYCALL_BOUNDARIES_H
#define POLYCALL_BOUNDARIES_H

/* Input validation macros */
#define VALIDATE_PTR(ptr) do { \
    if (!(ptr)) { \
        return POLYCALL_ERROR_INVALID_PARAM; \
    } \
} while(0)

#define VALIDATE_RANGE(val, min, max) do { \
    if ((val) < (min) || (val) > (max)) { \
        return POLYCALL_ERROR_OUT_OF_RANGE; \
    } \
} while(0)

/* Trust boundary markers */
#define TRUST_BOUNDARY_ENTER() /* TODO: Implement */
#define TRUST_BOUNDARY_EXIT()  /* TODO: Implement */

#endif /* POLYCALL_BOUNDARIES_H */
EOF
        log_debug "Created zero-trust boundaries header"
    fi
}

# Apply contract enforcement
apply_contracts() {
    local module_path="$1"
    local contract_file="$module_path/internal/contracts.h"
    
    if [ "$DRY_RUN" = true ]; then
        log_info "[DRY RUN] Would create contract definitions: $contract_file"
    else
        mkdir -p "$(dirname "$contract_file")"
        cat > "$contract_file" << 'EOF'
/*
 * Contract Enforcement - Sinphasé Principles
 * Auto-generated by enhanced refactoring
 * OBINexus Aegis Project
 */

#ifndef POLYCALL_CONTRACTS_H
#define POLYCALL_CONTRACTS_H

/* Pre-condition macros */
#define REQUIRES(cond) do { \
    if (!(cond)) { \
        return POLYCALL_ERROR_PRECONDITION_FAILED; \
    } \
} while(0)

/* Post-condition macros */
#define ENSURES(cond) do { \
    if (!(cond)) { \
        return POLYCALL_ERROR_POSTCONDITION_FAILED; \
    } \
} while(0)

/* Invariant checks */
#define INVARIANT(cond) assert(cond)

#endif /* POLYCALL_CONTRACTS_H */
EOF
        log_debug "Created contract enforcement header"
    fi
}

# Apply audit trails
apply_audit_trails() {
    local module_path="$1"
    local audit_file="$module_path/internal/audit.h"
    
    if [ "$DRY_RUN" = true ]; then
        log_info "[DRY RUN] Would create audit trail system: $audit_file"
    else
        mkdir -p "$(dirname "$audit_file")"
        cat > "$audit_file" << 'EOF'
/*
 * Audit Trail System - Sinphasé Compliance
 * Auto-generated by enhanced refactoring
 * OBINexus Aegis Project
 */

#ifndef POLYCALL_AUDIT_H
#define POLYCALL_AUDIT_H

/* Audit event types */
typedef enum {
    AUDIT_ACCESS,
    AUDIT_MODIFY,
    AUDIT_ERROR,
    AUDIT_SECURITY
} audit_event_type_t;

/* Audit macros */
#define AUDIT_LOG(type, msg) /* TODO: Implement */
#define AUDIT_ENTER(func) AUDIT_LOG(AUDIT_ACCESS, #func " entered")
#define AUDIT_EXIT(func) AUDIT_LOG(AUDIT_ACCESS, #func " exited")

#endif /* POLYCALL_AUDIT_H */
EOF
        log_debug "Created audit trail header"
    fi
}

# Apply isolation enforcement
apply_isolation() {
    local module_path="$1"
    local isolation_file="$module_path/internal/isolation.h"
    
    if [ "$DRY_RUN" = true ]; then
        log_info "[DRY RUN] Would create isolation enforcement: $isolation_file"
    else
        mkdir -p "$(dirname "$isolation_file")"
        cat > "$isolation_file" << 'EOF'
/*
 * Isolation Enforcement - Sinphasé Architecture
 * Auto-generated by enhanced refactoring
 * OBINexus Aegis Project
 */

#ifndef POLYCALL_ISOLATION_H
#define POLYCALL_ISOLATION_H

/* Module isolation context */
typedef struct {
    void* private_data;
    size_t data_size;
    uint32_t access_flags;
} isolation_context_t;

/* Isolation macros */
#define ISOLATE_BEGIN() /* TODO: Implement */
#define ISOLATE_END()   /* TODO: Implement */

#endif /* POLYCALL_ISOLATION_H */
EOF
        log_debug "Created isolation enforcement header"
    fi
}

# Generate refactoring report
generate_report() {
    log_info "Generating refactoring report..."
    
    local report_file="$PROJECT_ROOT/reports/refactor_$(date +%Y%m%d_%H%M%S).md"
    mkdir -p "$(dirname "$report_file")"
    
    {
        echo "# LibPolyCall Refactoring Report"
        echo "Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
        echo
        echo "## Configuration"
        echo "- Policy Mode: $POLICY_MODE"
        echo "- Dry Run: $DRY_RUN"
        echo "- Modules: ${MODULES:-all}"
        echo
        echo "## Modules Processed"
        
        # List processed modules
        detect_modules | while read -r module; do
            echo "- $module"
        done
        
        echo
        echo "## Actions Taken"
        echo "See log file for details: $REFACTOR_LOG"
        
        if [ "$POLICY_MODE" = true ]; then
            echo
            echo "## Sinphasé Principles Applied"
            echo "- Zero-trust boundaries"
            echo "- Contract enforcement"
            echo "- Audit trails"
            echo "- Isolation enforcement"
        fi
        
        echo
        echo "## Recommendations"
        echo "1. Review generated headers in internal/ directories"
        echo "2. Implement TODO markers in generated code"
        echo "3. Run validation suite after refactoring"
        echo "4. Update CMakeLists.txt files as needed"
        
    } > "$report_file"
    
    log_info "Report generated: $report_file"
}

# Cleanup and finalization
cleanup() {
    log_info "Cleaning up refactoring session..."
    
    # End trace session if policy mode
    if [ "$POLICY_MODE" = true ] && [ -n "$EXEC_ID" ]; then
        if [ -f "$ADHOC_DIR/tracer-root.sh" ]; then
            "$ADHOC_DIR/tracer-root.sh" end "$EXEC_ID" "0"
        fi
    fi
    
    # Remove temporary files
    find "$PROJECT_ROOT" -name "*.bak" -mtime -1 -delete 2>/dev/null || true
    
    log_info "Refactoring session complete"
}

# Main execution
main() {
    parse_args "$@"
    
    # Initialize
    init_refactor
    
    # Process modules
    local modules_processed=0
    local modules_failed=0
    
    detect_modules | while read -r module; do
        if validate_module "$module"; then
            refactor_module "$module"
            ((modules_processed++))
        else
            log_error "Skipping invalid module: $module"
            ((modules_failed++))
        fi
    done
    
    # Generate report
    generate_report
    
    # Cleanup
    cleanup
    
    # Final summary
    log_info "=== Refactoring Summary ==="
    log_info "Modules processed: $modules_processed"
    log_info "Modules failed: $modules_failed"
    log_info "Log file: $REFACTOR_LOG"
    
    if [ "$modules_failed" -gt 0 ]; then
        exit 1
    fi
}

# Trap for cleanup on exit
trap cleanup EXIT

# Execute main
main "$@"
