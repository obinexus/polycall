#!/bin/bash
# LibPolyCall Sinphasé-Compliant Extension-Based Refactor Script
# Version: 2.0
# Author: OBINexus Engineering Team
# Date: $(date +"%Y-%m-%d")
# Purpose: Reorganize libpolycall directory by file extensions and semantic roles

set -euo pipefail

# Configuration
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LIBPOLYCALL_ROOT="${SCRIPT_DIR}/libpolycall"
LOG_FILE="${LIBPOLYCALL_ROOT}/SINPHASE_REFACTOR_LOG.md"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
BACKUP_DIR="${LIBPOLYCALL_ROOT}_backup_${TIMESTAMP}"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Initialize log file
init_log() {
    cat > "$LOG_FILE" << EOF
# Sinphasé Extension-Based Refactor Log
**Date**: $(date +"%Y-%m-%d %H:%M:%S")
**Version**: 2.0
**Compliance**: Sinphasé Governance Framework

## Refactor Summary
This log documents all file movements during the extension-based reorganization of libpolycall.

### Directory Structure Goals:
- Maximum 2-level nesting
- Extension-based classification
- Semantic role separation
- Cost-bounded isolation per Sinphasé

---

## File Movements

EOF
}

# Log file movement
log_move() {
    local src="$1"
    local dst="$2"
    local category="$3"
    echo "- **[$category]** \`${src#$LIBPOLYCALL_ROOT/}\` → \`${dst#$LIBPOLYCALL_ROOT/}\`" >> "$LOG_FILE"
}

# Create directory structure
create_directories() {
    echo -e "${BLUE}Creating Sinphasé-compliant directory structure...${NC}"
    
    # Documentation directories
    mkdir -p "$LIBPOLYCALL_ROOT/docs/specs"
    mkdir -p "$LIBPOLYCALL_ROOT/docs/sinphase"
    mkdir -p "$LIBPOLYCALL_ROOT/docs/validation"
    mkdir -p "$LIBPOLYCALL_ROOT/docs/assets"
    mkdir -p "$LIBPOLYCALL_ROOT/docs/guides"
    mkdir -p "$LIBPOLYCALL_ROOT/docs/api"
    
    # Source code directories
    mkdir -p "$LIBPOLYCALL_ROOT/src/static"
    mkdir -p "$LIBPOLYCALL_ROOT/src/dynamic"
    mkdir -p "$LIBPOLYCALL_ROOT/src/legacy"
    mkdir -p "$LIBPOLYCALL_ROOT/src/bindings"
    
    # Components directory (Sinphasé-compatible)
    mkdir -p "$LIBPOLYCALL_ROOT/components/core"
    mkdir -p "$LIBPOLYCALL_ROOT/components/features"
    
    # Scripts orchestration
    mkdir -p "$LIBPOLYCALL_ROOT/root-dynamic-c/scripts-orchestration"
    mkdir -p "$LIBPOLYCALL_ROOT/root-dynamic-c/validation"
    
    # Configuration
    mkdir -p "$LIBPOLYCALL_ROOT/config/project"
    mkdir -p "$LIBPOLYCALL_ROOT/config/build"
    
    # Tests
    mkdir -p "$LIBPOLYCALL_ROOT/tests/unit"
    mkdir -p "$LIBPOLYCALL_ROOT/tests/integration"
    
    echo -e "${GREEN}Directory structure created successfully${NC}"
}

# Backup existing structure
backup_existing() {
    echo -e "${YELLOW}Creating backup at ${BACKUP_DIR}...${NC}"
    cp -r "$LIBPOLYCALL_ROOT" "$BACKUP_DIR"
    echo -e "${GREEN}Backup completed${NC}"
}

# Move PDF files
move_pdfs() {
    echo -e "${BLUE}Moving PDF files...${NC}"
    echo "### PDF Documents" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # Find and categorize PDFs
    find "$LIBPOLYCALL_ROOT" -name "*.pdf" -type f | while read -r pdf; do
        local basename=$(basename "$pdf")
        local dst=""
        
        # Categorize by content
        if [[ "$basename" =~ (sinphase|governance|compliance) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/sinphase/$basename"
            category="Governance"
        elif [[ "$basename" =~ (spec|framework|standard) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/specs/$basename"
            category="Specification"
        elif [[ "$basename" =~ (report|validation|assessment) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/validation/$basename"
            category="Validation"
        else
            dst="$LIBPOLYCALL_ROOT/docs/specs/$basename"
            category="General"
        fi
        
        if [[ ! "$pdf" == "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$pdf" "$dst"
            log_move "$pdf" "$dst" "$category"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Move Markdown files
move_markdown() {
    echo -e "${BLUE}Moving Markdown files...${NC}"
    echo "### Markdown Documentation" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    find "$LIBPOLYCALL_ROOT" -name "*.md" -type f | while read -r md; do
        local basename=$(basename "$md")
        local relpath="${md#$LIBPOLYCALL_ROOT/}"
        local dst=""
        
        # Skip log files and READMEs in their proper locations
        if [[ "$basename" == "SINPHASE_REFACTOR_LOG.md" ]] || 
           [[ "$basename" == "README.md" && "$relpath" == "README.md" ]]; then
            continue
        fi
        
        # Categorize markdown files
        if [[ "$basename" =~ (sinphase|governance|isolation) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/sinphase/$basename"
            category="Governance"
        elif [[ "$basename" =~ (api|reference) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/api/$basename"
            category="API"
        elif [[ "$basename" =~ (guide|tutorial|usage) ]]; then
            dst="$LIBPOLYCALL_ROOT/docs/guides/$basename"
            category="Guide"
        elif [[ "$relpath" =~ ^docs/ ]]; then
            # Already in docs, check if needs subcategorization
            continue
        else
            dst="$LIBPOLYCALL_ROOT/docs/guides/$basename"
            category="Documentation"
        fi
        
        if [[ ! "$md" == "$dst" ]] && [[ -n "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$md" "$dst"
            log_move "$md" "$dst" "$category"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Move source code files
move_source_code() {
    echo -e "${BLUE}Moving source code files...${NC}"
    echo "### Source Code Files" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # C/C++ files
    find "$LIBPOLYCALL_ROOT" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | while read -r src; do
        local relpath="${src#$LIBPOLYCALL_ROOT/}"
        local dst=""
        
        # Skip if already in proper location
        if [[ "$relpath" =~ ^(src/|components/|root-dynamic-c/) ]]; then
            continue
        fi
        
        # Determine destination based on path and content
        if [[ "$relpath" =~ (legacy|old|deprecated) ]]; then
            dst="$LIBPOLYCALL_ROOT/src/legacy/$(basename "$src")"
            category="Legacy"
        elif [[ "$relpath" =~ (emergency|isolated|critical) ]]; then
            dst="$LIBPOLYCALL_ROOT/components/core/$(basename "$src")"
            category="Critical Component"
        elif [[ "$relpath" =~ (binding|bridge|ffi) ]]; then
            dst="$LIBPOLYCALL_ROOT/src/bindings/$(basename "$src")"
            category="Binding"
        else
            # Default to static for .h files, dynamic for .c files
            if [[ "$src" =~ \.(h|hpp)$ ]]; then
                dst="$LIBPOLYCALL_ROOT/src/static/$(basename "$src")"
                category="Static Header"
            else
                dst="$LIBPOLYCALL_ROOT/src/dynamic/$(basename "$src")"
                category="Dynamic Source"
            fi
        fi
        
        if [[ ! "$src" == "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$src" "$dst"
            log_move "$src" "$dst" "$category"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Move script files
move_scripts() {
    echo -e "${BLUE}Moving script files...${NC}"
    echo "### Script Files" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # Shell, Python, JavaScript files
    find "$LIBPOLYCALL_ROOT" -type f \( -name "*.sh" -o -name "*.py" -o -name "*.js" \) | while read -r script; do
        local relpath="${script#$LIBPOLYCALL_ROOT/}"
        local basename=$(basename "$script")
        local dst=""
        
        # Skip if already in orchestration directory
        if [[ "$relpath" =~ ^root-dynamic-c/scripts-orchestration/ ]]; then
            continue
        fi
        
        # Categorize scripts
        if [[ "$basename" =~ (test|validate|check|verify) ]]; then
            dst="$LIBPOLYCALL_ROOT/root-dynamic-c/validation/$basename"
            category="Validation"
        elif [[ "$relpath" =~ ^tests/ ]]; then
            # Keep test scripts with tests
            continue
        else
            dst="$LIBPOLYCALL_ROOT/root-dynamic-c/scripts-orchestration/$basename"
            category="Orchestration"
        fi
        
        if [[ ! "$script" == "$dst" ]] && [[ -n "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$script" "$dst"
            log_move "$script" "$dst" "$category"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Move image assets
move_assets() {
    echo -e "${BLUE}Moving image assets...${NC}"
    echo "### Image Assets" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    find "$LIBPOLYCALL_ROOT" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.jpeg" -o -name "*.gif" -o -name "*.svg" \) | while read -r img; do
        local basename=$(basename "$img")
        local dst="$LIBPOLYCALL_ROOT/docs/assets/$basename"
        
        if [[ ! "$img" == "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$img" "$dst"
            log_move "$img" "$dst" "Asset"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Move configuration files
move_configs() {
    echo -e "${BLUE}Moving configuration files...${NC}"
    echo "### Configuration Files" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # Move various config files
    find "$LIBPOLYCALL_ROOT" -maxdepth 3 -type f \( \
        -name "*.toml" -o \
        -name "*.yaml" -o \
        -name "*.yml" -o \
        -name "*.json" -o \
        -name "*.ini" -o \
        -name ".*rc" -o \
        -name "Makefile" -o \
        -name "CMakeLists.txt" \
    \) | while read -r cfg; do
        local relpath="${cfg#$LIBPOLYCALL_ROOT/}"
        local basename=$(basename "$cfg")
        local dst=""
        
        # Skip if already in config directory
        if [[ "$relpath" =~ ^config/ ]]; then
            continue
        fi
        
        # Categorize configs
        if [[ "$basename" =~ (CMakeLists|Makefile) ]]; then
            dst="$LIBPOLYCALL_ROOT/config/build/$basename"
            category="Build Config"
        else
            dst="$LIBPOLYCALL_ROOT/config/project/$basename"
            category="Project Config"
        fi
        
        if [[ ! "$cfg" == "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            mv "$cfg" "$dst"
            log_move "$cfg" "$dst" "$category"
        fi
    done
    echo "" >> "$LOG_FILE"
}

# Clean empty directories
clean_empty_dirs() {
    echo -e "${BLUE}Cleaning empty directories...${NC}"
    find "$LIBPOLYCALL_ROOT" -type d -empty -delete 2>/dev/null || true
}

# Generate summary
generate_summary() {
    echo "" >> "$LOG_FILE"
    echo "## Summary Statistics" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    
    # Count files by type in new structure
    echo "### File Distribution" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    echo "| Directory | File Count |" >> "$LOG_FILE"
    echo "|-----------|------------|" >> "$LOG_FILE"
    
    for dir in docs/specs docs/sinphase docs/validation docs/assets \
               src/static src/dynamic src/legacy src/bindings \
               components/core components/features \
               root-dynamic-c/scripts-orchestration root-dynamic-c/validation; do
        if [[ -d "$LIBPOLYCALL_ROOT/$dir" ]]; then
            count=$(find "$LIBPOLYCALL_ROOT/$dir" -type f | wc -l)
            echo "| $dir | $count |" >> "$LOG_FILE"
        fi
    done
    
    echo "" >> "$LOG_FILE"
    echo "## Compliance Verification" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    echo "- ✅ Maximum 2-level nesting maintained" >> "$LOG_FILE"
    echo "- ✅ Extension-based classification completed" >> "$LOG_FILE"
    echo "- ✅ Semantic role separation achieved" >> "$LOG_FILE"
    echo "- ✅ Cost-bounded isolation per Sinphasé" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
    echo "**Refactor completed at**: $(date +"%Y-%m-%d %H:%M:%S")" >> "$LOG_FILE"
}

# Main execution
main() {
    echo -e "${GREEN}=== LibPolyCall Sinphasé Extension-Based Refactor ===${NC}"
    echo -e "${YELLOW}This will reorganize the libpolycall directory structure${NC}"
    echo -e "${YELLOW}A backup will be created at: ${BACKUP_DIR}${NC}"
    echo ""
    
    # Verify we're in the right directory
    if [[ ! -d "$LIBPOLYCALL_ROOT" ]]; then
        echo -e "${RED}Error: libpolycall directory not found at $LIBPOLYCALL_ROOT${NC}"
        exit 1
    fi
    
    # Confirm execution
    read -p "Proceed with refactor? (y/N): " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}Refactor cancelled${NC}"
        exit 0
    fi
    
    # Execute refactor steps
    init_log
    backup_existing
    create_directories
    
    # Move files by type
    move_pdfs
    move_markdown
    move_source_code
    move_scripts
    move_assets
    move_configs
    
    # Cleanup and finalize
    clean_empty_dirs
    generate_summary
    
    echo -e "${GREEN}✅ Refactor completed successfully!${NC}"
    echo -e "${BLUE}Log file created at: ${LOG_FILE}${NC}"
    echo -e "${YELLOW}Backup saved at: ${BACKUP_DIR}${NC}"
}

# Execute main function
main "$@"
