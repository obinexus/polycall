#!/bin/bash
# 06-orphan-finder.sh - Detects unused C files not referenced in any Makefile
set -euo pipefail

VERBOSE=false
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose) VERBOSE=true; shift ;;
        --dry-run) DRY_RUN=true; shift ;;
        *) shift ;;
    esac
done

log() { [[ "$VERBOSE" == "true" ]] && echo "[ORPHAN-FINDER] $*" >&2 || true; }
warn() { echo "[ORPHAN-FINDER] WARNING: $*" >&2; }

find_c_files() {
    find . -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" | \
    grep -v -E '\.(git|cache|tmp)/' | \
    sort
}

find_makefiles() {
    find . -name "Makefile" -o -name "makefile" -o -name "*.mk" -o -name "CMakeLists.txt" -o -name "*.cmake" | \
    grep -v -E '\.(git|cache|tmp)/' | \
    sort
}

extract_source_references() {
    local makefile="$1"
    
    # Extract various patterns that reference source files
    grep -E '\.(c|cpp|h|hpp)' "$makefile" 2>/dev/null | \
    sed -E 's/.*([a-zA-Z_][a-zA-Z0-9_]*\.(c|cpp|h|hpp)).*/\1/g' | \
    sort -u
}

check_include_references() {
    local header="$1"
    local header_basename
    header_basename=$(basename "$header")
    
    # Check if header is included anywhere
    find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
    xargs grep -l "#include.*$header_basename" 2>/dev/null | \
    wc -l
}

main() {
    log "Starting orphan file detection..."
    
    local c_files makefiles all_referenced_files orphan_files
    
    # Get all C/C++ source and header files
    c_files=$(find_c_files)
    
    if [[ -z "$c_files" ]]; then
        log "No C/C++ source files found"
        return 0
    fi
    
    # Get all makefiles
    makefiles=$(find_makefiles)
    
    if [[ -z "$makefiles" ]]; then
        warn "No makefiles found - cannot detect orphans reliably"
        if [[ "$DRY_RUN" == "true" ]]; then
            echo "[DRY-RUN] Would list all source files as potential orphans"
        fi
        return 0
    fi
    
    # Extract all file references from makefiles
    all_referenced_files=""
    while IFS= read -r makefile; do
        log "Scanning makefile: $makefile"
        local refs
        refs=$(extract_source_references "$makefile")
        all_referenced_files="$all_referenced_files"$'\n'"$refs"
    done <<< "$makefiles"
    
    # Sort and deduplicate
    all_referenced_files=$(echo "$all_referenced_files" | sort -u | grep -v '^$')
    
    # Find orphan files
    orphan_files=""
    while IFS= read -r source_file; do
        local basename_file
        basename_file=$(basename "$source_file")
        
        # Check if file is referenced in makefiles
        if ! echo "$all_referenced_files" | grep -q "$basename_file"; then
            # For headers, also check if they're included anywhere
            if [[ "$source_file" =~ \.(h|hpp)$ ]]; then
                local include_count
                include_count=$(check_include_references "$source_file")
                if [[ $include_count -eq 0 ]]; then
                    orphan_files="$orphan_files"$'\n'"$source_file"
                else
                    log "Header $source_file not in makefiles but has $include_count includes"
                fi
            else
                orphan_files="$orphan_files"$'\n'"$source_file"
            fi
        fi
    done <<< "$c_files"
    
    # Report results
    orphan_files=$(echo "$orphan_files" | grep -v '^$' | sort)
    
    if [[ -n "$orphan_files" ]]; then
        warn "Found potential orphan files:"
        while IFS= read -r orphan; do
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Orphan detected: $orphan"
            else
                warn "  $orphan"
            fi
        done <<< "$orphan_files"
        
        echo ""
        warn "These files may be safe to remove, but verify manually before deletion"
        return 1
    else
        log "✅ No orphan files detected"
        return 0
    fi
}

main "$@"
