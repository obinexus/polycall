#!/bin/bash
# fix_all_includes.sh - LibPolyCall Comprehensive Include Path Fix Script
# 
# This script executes all include path fixing scripts in sequence to efficiently
# resolve all types of include path issues in the codebase.
#
# Usage: ./fix_all_includes.sh [OPTIONS]
#   Options:
#     --project-root DIR   Specify project root directory (default: current directory)
#     --verbose            Enable verbose logging
#     --no-backup          Skip backup creation
#     --dry-run            Show what would be changed without modifying files
#     --help               Display this help message
#
# Author: OBINexusComputing

set -e  # Exit on error

# Default values
PROJECT_ROOT="$(pwd)"
SCRIPTS_DIR="$(pwd)/scripts"
VERBOSE=""
BACKUP="true"
DRY_RUN=""
BUILD_DIR="$(pwd)/build"

# Timestamp for logs
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="${PROJECT_ROOT}/logs"
LOG_FILE="${LOG_DIR}/fix_all_includes_${TIMESTAMP}.log"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root)
      PROJECT_ROOT="$2"
      shift 2
      ;;
    --verbose)
      VERBOSE="--verbose"
      shift
      ;;
    --no-backup)
      BACKUP=""
      shift
      ;;
    --dry-run)
      DRY_RUN="--dry-run"
      shift
      ;;
    --help)
      echo "Usage: ./fix_all_includes.sh [OPTIONS]"
      echo "Options:"
      echo "  --project-root DIR   Specify project root directory (default: current directory)"
      echo "  --verbose            Enable verbose logging"
      echo "  --no-backup          Skip backup creation"
      echo "  --dry-run            Show what would be changed without modifying files"
      echo "  --help               Display this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Use --help for usage information"
      exit 1
      ;;
  esac
done

# Create log directory if it doesn't exist
mkdir -p "${LOG_DIR}"

# Log function
log() {
  echo "[$(date +%Y-%m-%d\ %H:%M:%S)] $1" | tee -a "${LOG_FILE}"
}

log "Starting LibPolyCall comprehensive include path fix process"
log "Project root: ${PROJECT_ROOT}"
log "Using scripts from: ${SCRIPTS_DIR}"

# Validate scripts directory
if [ ! -d "${SCRIPTS_DIR}" ]; then
  log "ERROR: Scripts directory not found: ${SCRIPTS_DIR}"
  exit 1
fi

# Step 1: Create backup if enabled
if [ -n "${BACKUP}" ] && [ -z "${DRY_RUN}" ]; then
  BACKUP_DIR="${PROJECT_ROOT}/backup_${TIMESTAMP}"
  log "Creating backup in ${BACKUP_DIR}..."
  mkdir -p "${BACKUP_DIR}"
  
  # Backup src directory
  if [ -d "${PROJECT_ROOT}/src" ]; then
    log "Backing up src directory..."
    cp -r "${PROJECT_ROOT}/src" "${BACKUP_DIR}/"
  fi
  
  # Backup include directory
  if [ -d "${PROJECT_ROOT}/include" ]; then
    log "Backing up include directory..."
    cp -r "${PROJECT_ROOT}/include" "${BACKUP_DIR}/"
  fi
  
  log "Backup completed successfully"
else
  log "Backup skipped"
fi

# Step 2: Basic path standardization with standardize_includes.py
log "Step 1/6: Running basic include standardization..."
CMD="python ${SCRIPTS_DIR}/standardize_includes.py --root ${PROJECT_ROOT} ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 3: Fix problematic polycall/ paths
log "Step 2/6: Fixing problematic polycall/ paths..."
CMD="python ${SCRIPTS_DIR}/fix_polycall_paths.py --project-root ${PROJECT_ROOT} ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 4: Fix complex nested paths
log "Step 3/7: Running fix_all_paths.py for complex nested patterns..."
CMD="python ${SCRIPTS_DIR}/fix_all_paths.py --project-root ${PROJECT_ROOT} ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 5: Fix nested path includes specifically
log "Step 4/7: Fixing nested path include patterns..."
CMD="python ${SCRIPTS_DIR}/fix_nested_path_includes.py --project-root ${PROJECT_ROOT} ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 6: Validate and list remaining issues
log "Step 5/7: Validating include paths..."
VALIDATION_OUTPUT="${LOG_DIR}/validation_${TIMESTAMP}.txt"
CMD="python ${SCRIPTS_DIR}/validate_includes.py --root ${PROJECT_ROOT} --standard polycall"
log "Executing: ${CMD}"
eval "${CMD}" > "${VALIDATION_OUTPUT}" 2>&1
cat "${VALIDATION_OUTPUT}" >> "${LOG_FILE}"

# Check for validation issues
ISSUES_COUNT=$(grep -c "Issues in" "${VALIDATION_OUTPUT}" || true)
if [ "${ISSUES_COUNT}" -gt 0 ]; then
  # Step 7: Fix any remaining issues
  log "Step 6/7: Fixing remaining validation issues..."
  CMD="python ${SCRIPTS_DIR}/include_path_validator.py --project-root ${PROJECT_ROOT} --error-log ${VALIDATION_OUTPUT} ${DRY_RUN} ${VERBOSE}"
  log "Executing: ${CMD}"
  eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"
else
  log "No validation issues found!"
fi

# Step 8: Generate unified header
log "Step 7/7: Generating unified header..."
UNIFIED_HEADER="${BUILD_DIR}/include/polycall.h"
mkdir -p $(dirname ${UNIFIED_HEADER})
CMD="python ${SCRIPTS_DIR}/generate_unified_header.py --project-root ${PROJECT_ROOT} --output ${UNIFIED_HEADER} ${VERBOSE}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Final validation to confirm all issues resolved
log "Running final validation check..."
FINAL_VALIDATION="${LOG_DIR}/final_validation_${TIMESTAMP}.txt"
CMD="python ${SCRIPTS_DIR}/validate_includes.py --root ${PROJECT_ROOT} --standard polycall"
log "Executing: ${CMD}"
eval "${CMD}" > "${FINAL_VALIDATION}" 2>&1

# Check remaining issues
REMAINING_ISSUES=$(grep -c "Issues in" "${FINAL_VALIDATION}" || true)
if [ "${REMAINING_ISSUES}" -gt 0 ]; then
  log "WARNING: ${REMAINING_ISSUES} files still have include path issues after fixes"
  log "Review ${FINAL_VALIDATION} for details"
else
  log "SUCCESS: All include path issues have been resolved!"
fi

log "Comprehensive include path fix process completed"
log "Log file: ${LOG_FILE}"

# Final status report
if [ -z "${DRY_RUN}" ]; then
  log "Summary of changes:"
  log "  - Basic standardization completed"
  log "  - Problematic polycall/ paths fixed"
  log "  - Complex nested patterns addressed"
  log "  - Validation issues addressed: ${ISSUES_COUNT}"
  log "  - Unified header generated: ${UNIFIED_HEADER}"
  log "  - Remaining issues: ${REMAINING_ISSUES}"
  
  if [ "${REMAINING_ISSUES}" -gt 0 ]; then
    log "Recommendation: Manual inspection of remaining issues:"
    log "  cat ${FINAL_VALIDATION}"
  else
    log "Recommendation: Build the project to confirm all include path issues are resolved:"
    log "  mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR} && cmake .. && make"
  fi
else
  log "Dry run completed. No files were modified."
  log "Run without --dry-run to apply changes."
fi

echo ""
echo "Path standardization completed."
echo "See ${LOG_FILE} for details."
echo "Final validation report: ${FINAL_VALIDATION}"

# Return appropriate exit code
if [ "${REMAINING_ISSUES}" -gt 0 ]; then
  exit 1
else
  exit 0
fi