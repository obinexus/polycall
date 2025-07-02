#!/bin/bash
# Ad-hoc Migration Script for Dev Cycle Structure (Sinphasé Waterfall Compliance)
# Usage: ./adhoc-migration.sh [--dry-run]

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
ADHOC_DIR="$PROJECT_ROOT/scripts/adhoc"
DRY_RUN=false

if [[ "$1" == "--dry-run" ]]; then
    DRY_RUN=true
    echo "[DRY-RUN] No files will be moved. Showing planned actions:"
fi

# Map scripts to dev-cycle folders
move_script() {
    local script="$1"
    local dest_folder="$2"
    local dest="$ADHOC_DIR/$dest_folder/$(basename "$script")"
    if $DRY_RUN; then
        echo "Would move: $script -> $dest"
    else
        echo "Moving: $script -> $dest"
        mv "$script" "$dest"
    fi
}

# Requirements
for f in $ADHOC_DIR/fix_include_paths.py $ADHOC_DIR/fix_include_paths_with_validation.py $ADHOC_DIR/fix_nested_path_includes.py $ADHOC_DIR/fix_polycall_paths.py; do
    [ -f "$f" ] && move_script "$f" "01-requirements"
done

# Design
for f in $ADHOC_DIR/generate_unified_header.py $ADHOC_DIR/header_include_fixer.py $ADHOC_DIR/include_path_fixer.py; do
    [ -f "$f" ] && move_script "$f" "02-design"
done

# Implementation
for f in $ADHOC_DIR/fix_all_includes.sh $ADHOC_DIR/fix_all_paths.py $ADHOC_DIR/fix_implementation_includes.py $ADHOC_DIR/fix_includes.sh $ADHOC_DIR/fix_includes_and_backup.py $ADHOC_DIR/include_path_standardization.sh $ADHOC_DIR/include_path_standardizer.py $ADHOC_DIR/standardize_includes.py; do
    [ -f "$f" ] && move_script "$f" "03-implementation"
done

# Testing
for f in $ADHOC_DIR/test_include_standadizer.py $ADHOC_DIR/test_standardize_include.py $ADHOC_DIR/repl_test.py; do
    [ -f "$f" ] && move_script "$f" "04-testing"
done

# Compliance
for f in $ADHOC_DIR/compliance-check.sh $ADHOC_DIR/validate-lfs-compliance.sh $ADHOC_DIR/validate-libpolycall-compliance.sh $ADHOC_DIR/validate_includes.py $ADHOC_DIR/validate_include_paths.py $ADHOC_DIR/include_path_validator.py; do
    [ -f "$f" ] && move_script "$f" "05-compliance"
done

# Audit
for f in $ADHOC_DIR/tracer-root.sh $ADHOC_DIR/adhoc-validator.sh $ADHOC_DIR/adhoc-execute.sh; do
    [ -f "$f" ] && move_script "$f" "06-audit"
done

# Waterfall
for f in $ADHOC_DIR/waterfall.sh $ADHOC_DIR/main.sh; do
    [ -f "$f" ] && move_script "$f" "07-waterfall"
done

# Summary
if $DRY_RUN; then
    echo "[DRY-RUN] Migration preview complete. No files were moved."
else
    echo "[MIGRATION] Migration complete."
fi
