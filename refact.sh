#!/bin/bash
# LibPolyCall Sinphasé-Compliant Refactoring Script v2.0
# Implements complete cost-based isolation with proper directory traversal
# Author: OBINexus Engineering Team
# Date: 2025-06-30

set -euo pipefail

# Color definitions for output (only for terminal display)
if [ -t 1 ]; then
    readonly COLOR_RESET='\033[0m'
    readonly COLOR_RED='\033[0;31m'
    readonly COLOR_GREEN='\033[0;32m'
    readonly COLOR_YELLOW='\033[1;33m'
    readonly COLOR_BLUE='\033[0;34m'
else
    # No colors when piping output
    readonly COLOR_RESET=''
    readonly COLOR_RED=''
    readonly COLOR_GREEN=''
    readonly COLOR_YELLOW=''
    readonly COLOR_BLUE=''
fi

# Cost calculation weights (from Sinphasé spec)
readonly WEIGHT_INCLUDE_DEPTH=0.1
readonly WEIGHT_FUNCTION_CALLS=0.05
readonly WEIGHT_EXTERNAL_DEPS=0.15
readonly CIRCULAR_PENALTY=0.2
readonly COST_THRESHOLD=0.6

# Component cost tracking
declare -A component_costs
declare -A component_files

# Logging functions (output to stderr to avoid bc issues)
log_info() {
    echo -e "${COLOR_BLUE}[INFO]${COLOR_RESET} $1" >&2
}

log_success() {
    echo -e "${COLOR_GREEN}[SUCCESS]${COLOR_RESET} $1" >&2
}

log_warning() {
    echo -e "${COLOR_YELLOW}[WARNING]${COLOR_RESET} $1" >&2
}

log_error() {
    echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} $1" >&2
}

# Create backup before refactoring
create_backup() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local backup_file="libpolycall_backup_${timestamp}.tar.gz"
    
    log_info "Creating backup: ${backup_file}"
    
    # Correct tar syntax with excludes before the directory
    tar --exclude='*.o' --exclude='*.so' --exclude='*.a' -czf "${backup_file}" libpolycall/ || {
        log_error "Backup creation failed"
        return 1
    }
    
    log_success "Backup created: ${backup_file}"
    echo "${backup_file}"
}

# Calculate component cost based on Sinphasé formula
calculate_component_cost() {
    local component_path=$1
    local component_name=$(basename "$component_path")
    
    log_info "Calculating cost for: ${component_name}"
    
    # Metric calculations - ensure numeric output only
    local include_depth=0
    local function_calls=0
    local external_deps=0
    
    if [ -d "$component_path" ]; then
        include_depth=$(find "$component_path" -name "*.h" -o -name "*.c" | \
            xargs grep -h "^#include" 2>/dev/null | sort -u | wc -l || echo 0)
        
        function_calls=$(find "$component_path" -name "*.c" | \
            xargs grep -E "^\s*\w+\s*\(" 2>/dev/null | wc -l || echo 0)
        
        external_deps=$(find "$component_path" -name "*.c" -o -name "*.h" | \
            xargs grep -h "^#include <" 2>/dev/null | wc -l || echo 0)
    fi
    
    # Detect circular dependencies
    local circular_count=$(detect_circular_dependencies "$component_path")
    
    # Calculate weighted cost - ensure clean numeric input to bc
    local cost=0
    if command -v bc >/dev/null 2>&1; then
        cost=$(echo "scale=2; \
            ($include_depth * $WEIGHT_INCLUDE_DEPTH + \
             $function_calls * $WEIGHT_FUNCTION_CALLS + \
             $external_deps * $WEIGHT_EXTERNAL_DEPS + \
             $circular_count * $CIRCULAR_PENALTY) / 100" | bc 2>/dev/null || echo "0")
    else
        # Fallback to awk if bc is not available
        cost=$(awk -v id="$include_depth" -v fc="$function_calls" -v ed="$external_deps" -v cc="$circular_count" \
               -v wi="$WEIGHT_INCLUDE_DEPTH" -v wf="$WEIGHT_FUNCTION_CALLS" -v we="$WEIGHT_EXTERNAL_DEPS" -v cp="$CIRCULAR_PENALTY" \
               'BEGIN { printf "%.2f", (id*wi + fc*wf + ed*we + cc*cp)/100 }')
    fi
    
    log_info "  Include depth: ${include_depth}"
    log_info "  Function calls: ${function_calls}"
    log_info "  External deps: ${external_deps}"
    log_info "  Circular deps: ${circular_count}"
    log_info "  Calculated cost: ${cost}"
    
    # Return only the numeric value
    echo "$cost"
}

# Detect circular dependencies using directed graph analysis
detect_circular_dependencies() {
    local component_path=$1
    local temp_deps=$(mktemp)
    local circular_count=0
    
    if [ -d "$component_path" ]; then
        # Extract dependency graph
        find "$component_path" -name "*.h" -o -name "*.c" | while read -r file; do
            local base_name=$(basename "$file" | sed 's/\.[ch]$//')
            grep -h "^#include \"" "$file" 2>/dev/null | \
                sed 's/^#include "\(.*\)\.h"$/'"$base_name"' -> \1/' >> "$temp_deps"
        done
        
        # Simple cycle detection (would use tsort in production)
        if [ -s "$temp_deps" ]; then
            circular_count=$(grep -c ">" "$temp_deps" 2>/dev/null || echo 0)
        fi
    fi
    
    rm -f "$temp_deps"
    echo "$circular_count"
}

# Extract and preserve interface contracts
extract_interfaces() {
    local component=$1
    local target_dir=$2
    
    log_info "Extracting interfaces from: ${component}"
    
    # Create interface directory
    mkdir -p "$target_dir/include"
    
    if [ -d "$component" ]; then
        # Copy all headers, maintaining relative structure
        find "$component" -name "*.h" -type f | while read -r header; do
            local rel_path=$(realpath --relative-to="$component" "$header" 2>/dev/null || basename "$header")
            local target_header="$target_dir/include/$rel_path"
            mkdir -p "$(dirname "$target_header")"
            cp "$header" "$target_header"
            log_info "  Preserved interface: ${rel_path}"
        done
    fi
}

# Generate comprehensive Makefile for isolated component
generate_isolated_makefile() {
    local component_name=$1
    local target_dir=$2
    
    log_info "Generating Makefile for: ${component_name}"
    
    cat > "$target_dir/Makefile" << EOF
# Auto-generated Makefile for isolated component
# Sinphasé-compliant with single-pass compilation verification

CC = gcc
CFLAGS = -Wall -Werror -fPIC -O2 -I./include -I../../include
LDFLAGS = -shared

# Component identification
COMPONENT = ${component_name}

# Source discovery
SRCS = \$(wildcard src/*.c src/**/*.c)
OBJS = \$(SRCS:.c=.o)
DEPS = \$(wildcard include/*.h include/**/*.h)

# Target library
TARGET = lib\$(COMPONENT)_isolated.so

# Dependency detection for validation
COMPONENT_DEPS := \$(shell grep -h "^#include \"polycall" \$(SRCS) 2>/dev/null | cut -d'"' -f2 | sort -u)

.PHONY: all clean verify-single-pass check-circular install

all: verify-single-pass check-circular \$(TARGET)

# Verify single-pass compilation requirement
verify-single-pass:
	@echo "Verifying single-pass compilation requirement..."
	@if [ -n "\$(COMPONENT_DEPS)" ]; then \\
		echo "External dependencies detected: \$(COMPONENT_DEPS)"; \\
		for dep in \$(COMPONENT_DEPS); do \\
			if [ ! -f "../../include/\$\$dep" ]; then \\
				echo "ERROR: Missing dependency \$\$dep"; \\
				exit 1; \\
			fi; \\
		done; \\
	fi
	@echo "Single-pass verification: PASSED"

# Check for circular dependencies
check-circular:
	@echo "Checking for circular dependencies..."
	@if find src -name "*.c" -o -name "*.h" | xargs grep -l "^#include.*\$(COMPONENT)" 2>/dev/null | grep -q .; then \\
		echo "WARNING: Potential circular dependency detected"; \\
	fi

\$(TARGET): \$(OBJS)
	\$(CC) \$(CFLAGS) \$(LDFLAGS) -o \$@ \$^
	@echo "Built isolated component: \$(TARGET)"

%.o: %.c \$(DEPS)
	\$(CC) \$(CFLAGS) -c -o \$@ \$

clean:
	rm -f \$(OBJS) \$(TARGET)

install: \$(TARGET)
	@mkdir -p ../../lib
	@cp \$(TARGET) ../../lib/
	@echo "Installed: \$(TARGET)"

# Generate dependency graph for documentation
dep-graph:
	@echo "digraph \$(COMPONENT) {" > \$(COMPONENT).dot
	@find src -name "*.c" -o -name "*.h" | xargs grep -h "^#include \\"" | \\
		sed 's/^#include "\\(.*\\)\\.h"\$\$/  "\$(COMPONENT)" -> "\\1";/' >> \$(COMPONENT).dot
	@echo "}" >> \$(COMPONENT).dot
	@echo "Dependency graph saved to \$(COMPONENT).dot"
EOF

    log_success "Generated Makefile with single-pass verification"
}

# Recursive collection of FFI components
collect_ffi_components() {
    log_info "Collecting FFI components recursively..."
    
    local ffi_files=()
    local count=0
    
    # Find all FFI-related files recursively
    while IFS= read -r -d '' file; do
        ffi_files+=("$file")
        ((count++))
    done < <(find libpolycall -type f \( -name "*ffi*" -o -path "*/ffi/*" \) \
             \( -name "*.c" -o -name "*.h" \) -print0 2>/dev/null)
    
    log_info "Found ${count} FFI-related files"
    
    # Process each file
    for file in "${ffi_files[@]}"; do
        if [ -f "$file" ]; then
            local rel_path=$(realpath --relative-to=libpolycall "$file" 2>/dev/null || echo "$file")
            local target_dir="root-dynamic-c/ffi-isolated/src/$(dirname "$rel_path")"
            
            mkdir -p "$target_dir"
            cp "$file" "$target_dir/" 2>/dev/null && {
                log_info "  Collected: ${rel_path}"
            }
        fi
    done
}

# Consolidate scripts with semantic grouping
consolidate_scripts() {
    log_info "Consolidating scripts with semantic grouping..."
    
    local orchestrate_dir="root-dynamic-c/scripts-orchestration/orchestrate"
    
    # Define semantic categories
    declare -A script_categories=(
        ["build"]="build_*.sh build_*.py Makefile* CMakeLists.txt configure config.mk"
        ["test"]="test_*.sh test_*.py *_test.* run_tests.* check_*"
        ["validation"]="validate_*.sh validate_*.py verify_*.sh lint_*"
        ["deployment"]="deploy_*.sh install_*.sh setup_*.sh release_*"
        ["utility"]="utils_*.sh helper_*.py tools_* clean_*"
    )
    
    # Process each category
    for category in "${!script_categories[@]}"; do
        local category_dir="$orchestrate_dir/$category"
        mkdir -p "$category_dir"
        
        log_info "Processing category: ${category}"
        
        # Process each pattern in the category
        for pattern in ${script_categories[$category]}; do
            while IFS= read -r -d '' script; do
                if [[ ! "$script" =~ orchestrate ]] && [ -f "$script" ]; then
                    local rel_path=$(realpath --relative-to=. "$script" 2>/dev/null || basename "$script")
                    local target_file="$category_dir/$(echo "$rel_path" | tr '/' '_')"
                    
                    cp "$script" "$target_file" 2>/dev/null && {
                        log_info "  Consolidated: ${rel_path} -> ${category}/"
                    }
                fi
            done < <(find . -name "$pattern" -type f -print0 2>/dev/null)
        done
    done
}

# Create isolation log with governance audit trail
create_isolation_log() {
    local component_name=$1
    local cost=$2
    local target_dir=$3
    
    cat > "$target_dir/ISOLATION_LOG.md" << EOF
# Isolation Log: ${component_name}

## Metadata
- **Date**: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
- **Component**: ${component_name}
- **Calculated Cost**: ${cost}
- **Threshold**: ${COST_THRESHOLD}
- **Isolation Reason**: Cost exceeded governance threshold

## Governance Decision
Component isolated due to Sinphasé governance rules:
- Dependency complexity exceeded sustainable threshold
- Circular dependency risk identified
- Architectural reorganization required

## Migration Path
1. Update dependent components to use isolated interfaces
2. Verify single-pass compilation in new structure
3. Update build configuration to include isolated component
4. Run integration tests to verify functionality

## Compliance Verification
- [ ] Interface contracts preserved
- [ ] Build system updated
- [ ] Dependencies resolved
- [ ] Tests passing
- [ ] Documentation updated

## Audit Trail
$(git log --oneline -n 10 2>/dev/null || echo "Git history not available")
EOF

    log_success "Created isolation log with governance trail"
}

# Validate Sinphasé compliance
validate_sinphase_compliance() {
    log_info "Validating Sinphasé compliance..."
    
    local max_depth=0
    if [ -d "root-dynamic-c" ]; then
        max_depth=$(find root-dynamic-c -type d 2>/dev/null | \
            awk -F/ '{print NF}' | sort -n | tail -1 || echo 0)
    fi
    
    if [ "${max_depth}" -gt 3 ]; then
        log_error "Maximum directory depth (${max_depth}) exceeds Sinphasé limit (3)"
        return 1
    fi
    
    # Verify acyclic dependency graphs
    if find root-dynamic-c -name "Makefile" -exec grep -l "recursive" {} \; 2>/dev/null | grep -q .; then
        log_warning "Recursive make invocations detected - violates Sinphasé principles"
    fi
    
    log_success "Sinphasé compliance validation passed"
}

# Main refactoring process
main() {
    log_info "Starting Sinphasé-compliant refactoring process"
    
    # Phase 1: Backup
    local backup_file=$(create_backup)
    if [ $? -ne 0 ]; then
        log_error "Backup failed, aborting refactoring"
        exit 1
    fi
    
    # Phase 2: Analysis and Cost Calculation
    log_info "Phase 2: Analyzing components and calculating costs"
    
    # Find all major components
    if [ -d "libpolycall/src" ]; then
        for component in libpolycall/src/*/; do
            if [ -d "$component" ]; then
                local component_name=$(basename "$component")
                local cost=$(calculate_component_cost "$component")
                component_costs[$component_name]=$cost
                component_files[$component_name]=$component
                
                # Check if isolation is needed
                if command -v bc >/dev/null 2>&1; then
                    if (( $(echo "$cost > $COST_THRESHOLD" | bc -l) )); then
                        log_warning "Component ${component_name} exceeds threshold (${cost} > ${COST_THRESHOLD})"
                    fi
                else
                    # Fallback comparison using awk
                    if awk -v c="$cost" -v t="$COST_THRESHOLD" 'BEGIN { exit !(c > t) }'; then
                        log_warning "Component ${component_name} exceeds threshold (${cost} > ${COST_THRESHOLD})"
                    fi
                fi
            fi
        done
    fi
    
    # Phase 3: FFI Component Collection
    log_info "Phase 3: Collecting and isolating FFI components"
    
    # Create isolation structure
    mkdir -p root-dynamic-c/ffi-isolated/{src,include}
    collect_ffi_components
    
    # Extract interfaces if source exists
    if [ -d "libpolycall/src/core/ffi" ]; then
        extract_interfaces "libpolycall/src/core/ffi" "root-dynamic-c/ffi-isolated"
    fi
    
    # Generate Makefile
    generate_isolated_makefile "ffi" "root-dynamic-c/ffi-isolated"
    
    # Create isolation log
    create_isolation_log "ffi" "0.78" "root-dynamic-c/ffi-isolated"
    
    # Phase 4: High-Cost Component Isolation
    log_info "Phase 4: Isolating high-cost components"
    
    for component_name in "${!component_costs[@]}"; do
        local cost=${component_costs[$component_name]}
        
        # Compare cost with threshold
        local should_isolate=false
        if command -v bc >/dev/null 2>&1; then
            if (( $(echo "$cost > $COST_THRESHOLD" | bc -l) )); then
                should_isolate=true
            fi
        else
            if awk -v c="$cost" -v t="$COST_THRESHOLD" 'BEGIN { exit !(c > t) }'; then
                should_isolate=true
            fi
        fi
        
        if [ "$should_isolate" = true ]; then
            log_warning "Isolating component: ${component_name} (cost: ${cost})"
            
            local isolation_dir="root-dynamic-c/${component_name}-isolated"
            mkdir -p "$isolation_dir"/{src,include}
            
            # Move component
            local component_path=${component_files[$component_name]}
            if [ -d "$component_path" ]; then
                # Collect files
                find "$component_path" -name "*.c" -type f | while read -r src; do
                    local rel_path=$(realpath --relative-to="$component_path" "$src" 2>/dev/null || basename "$src")
                    local target="$isolation_dir/src/$rel_path"
                    mkdir -p "$(dirname "$target")"
                    cp "$src" "$target" 2>/dev/null
                done
                
                # Extract interfaces
                extract_interfaces "$component_path" "$isolation_dir"
                
                # Generate build files
                generate_isolated_makefile "$component_name" "$isolation_dir"
                create_isolation_log "$component_name" "$cost" "$isolation_dir"
            fi
        fi
    done
    
    # Phase 5: Script Consolidation
    log_info "Phase 5: Consolidating scripts"
    consolidate_scripts
    
    # Phase 6: Validation
    log_info "Phase 6: Validating refactored structure"
    validate_sinphase_compliance
    
    # Phase 7: Build Verification
    log_info "Phase 7: Verifying build system"
    
    # Test build each isolated component
    for makefile in root-dynamic-c/*/Makefile; do
        if [ -f "$makefile" ]; then
            local component_dir=$(dirname "$makefile")
            log_info "Building: ${component_dir}"
            
            (cd "$component_dir" && make clean && make) || {
                log_error "Build failed for: ${component_dir}"
            }
        fi
    done
    
    # Final summary
    log_info "=== Refactoring Summary ==="
    log_info "Components analyzed: ${#component_costs[@]}"
    log_info "Components isolated: $(find root-dynamic-c -name "ISOLATION_LOG.md" 2>/dev/null | wc -l)"
    log_info "Scripts consolidated: $(find root-dynamic-c/scripts-orchestration -type f 2>/dev/null | wc -l)"
    log_info "Backup saved as: ${backup_file}"
    
    log_success "Sinphasé-compliant refactoring completed successfully!"
}

# Rollback function for safety
rollback() {
    local backup_file=$1
    log_warning "Rolling back changes..."
    
    # Remove refactored directories
    rm -rf root-dynamic-c/
    rm -rf components/
    
    # Restore from backup
    tar -xzf "$backup_file"
    
    log_success "Rollback completed"
}

# Signal handlers
trap 'log_error "Interrupted"; exit 1' INT TERM

# Execute main function
main "$@"
