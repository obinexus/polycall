#!/bin/bash
# enexus-search.sh - Recursive project health scanner
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERBOSE=false
FIX_MODE=false

declare -a SEARCH_DIRS=("src" "config" "lib" "docs")
declare -a ROGUE_PATTERNS=(
    "FIXME"
    "TODO" 
    "HACK"
    "XXX"
    "BUG"
    "DEPRECATED"
    "REMOVE"
    "DELETE"
)

declare -a BROKEN_PATTERNS=(
    '#include\s*[<"]\s*$'           # Incomplete includes
    '#include\s*[<"][^>"]*[^h][<"]' # Suspicious include paths
    'printf\s*\('                  # Debug printf statements
    'fprintf\s*\(\s*stderr'        # Debug error output
    '\/\*.*\*\/'                   # Inline comments (potential cleanup needed)
)

log() {
    [[ "$VERBOSE" == "true" ]] && echo "[ENEXUS-SEARCH] $*" >&2 || true
}

warn() {
    echo "[ENEXUS-SEARCH] WARNING: $*" >&2
}

error() {
    echo "[ENEXUS-SEARCH] ERROR: $*" >&2
}

scan_rogue_symbols() {
    local total_issues=0
    
    for dir in "${SEARCH_DIRS[@]}"; do
        if [[ ! -d "$dir" ]]; then
            log "Skipping non-existent directory: $dir"
            continue
        fi
        
        log "Scanning directory: $dir"
        
        for pattern in "${ROGUE_PATTERNS[@]}"; do
            local matches
            matches=$(find "$dir" -name "*.c" -o -name "*.h" -o -name "*.cfg" | \
                     xargs grep -Hn "$pattern" 2>/dev/null || true)
            
            if [[ -n "$matches" ]]; then
                warn "Found $pattern markers:"
                echo "$matches" | while IFS= read -r match; do
                    echo "  $match"
                    ((total_issues++))
                done
                echo ""
            fi
        done
    done
    
    return $total_issues
}

scan_broken_links() {
    local total_broken=0
    
    for dir in "${SEARCH_DIRS[@]}"; do
        if [[ ! -d "$dir" ]]; then
            continue
        fi
        
        # Check for broken includes
        find "$dir" -name "*.c" -o -name "*.h" | while IFS= read -r file; do
            local includes
            includes=$(grep -n '#include' "$file" 2>/dev/null || true)
            
            if [[ -n "$includes" ]]; then
                echo "$includes" | while IFS= read -r line; do
                    local line_num header_path
                    line_num=$(echo "$line" | cut -d: -f1)
                    header_path=$(echo "$line" | sed -E 's/.*#include\s*[<"]([^>"]*)[>"].*/\1/')
                    
                    # Check if header exists in lib/shared or standard locations
                    if [[ ! -f "lib/shared/$header_path" && ! -f "/usr/include/$header_path" ]]; then
                        error "Broken include in $file:$line_num - $header_path not found"
                        ((total_broken++))
                    fi
                done
            fi
        done
    done
    
    return $total_broken
}

check_orphan_configs() {
    # Look for configuration files that don't match expected schemas
    find config/ -name "*.cfg" -o -name "*.json" -o -name "*.yaml" 2>/dev/null | \
    while IFS= read -r config_file; do
        local basename_file
        basename_file=$(basename "$config_file")
        
        # Check if config has corresponding schema
        local schema_file="config/schemas/${basename_file%.*}.schema.json"
        if [[ ! -f "$schema_file" ]]; then
            warn "Configuration file lacks schema: $config_file (expected: $schema_file)"
        fi
    done
}

auto_fix_issues() {
    if [[ "$FIX_MODE" == "false" ]]; then
        return 0
    fi
    
    log "Auto-fixing common issues..."
    
    # Remove trailing whitespace
    find "${SEARCH_DIRS[@]}" -name "*.c" -o -name "*.h" | \
    xargs sed -i 's/[[:space:]]*$//' 2>/dev/null || true
    
    # Fix common include formatting
    find "${SEARCH_DIRS[@]}" -name "*.c" -o -name "*.h" | \
    xargs sed -i 's/#include\s*<\s*\([^>]*\)\s*>/#include <\1>/' 2>/dev/null || true
    
    log "Auto-fix completed"
}

generate_report() {
    local rogue_count broken_count
    
    echo "=== ENEXUS-SEARCH PROJECT HEALTH REPORT ==="
    echo "Timestamp: $(date)"
    echo "Scan directories: ${SEARCH_DIRS[*]}"
    echo ""
    
    # Scan for rogue symbols
    rogue_count=$(scan_rogue_symbols) || true
    
    # Scan for broken links
    broken_count=$(scan_broken_links) || true
    
    # Check orphan configs
    check_orphan_configs
    
    # Auto-fix if requested
    auto_fix_issues
    
    echo ""
    echo "=== SUMMARY ==="
    echo "Rogue symbols found: $rogue_count"
    echo "Broken links found: $broken_count"
    
    if [[ $((rogue_count + broken_count)) -gt 0 ]]; then
        error "Project health check failed"
        return 1
    else
        log "✅ Project health check passed"
        return 0
    fi
}

main() {
    case "${1:-scan}" in
        --help|-h)
            echo "Usage: $0 [scan|fix|verbose]"
            echo "  scan     - Run health check (default)"
            echo "  fix      - Run health check and auto-fix issues"
            echo "  verbose  - Enable verbose output"
            exit 0
            ;;
        fix)
            FIX_MODE=true
            ;;
        verbose)
            VERBOSE=true
            ;;
    esac
    
    generate_report
}

main "$@"
