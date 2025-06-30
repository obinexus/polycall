#!/bin/bash
# standardize_paths.sh - LibPolyCall Include Path Standardization Automation Script
# 
# This script systematically standardizes include paths across the LibPolyCall codebase
# by executing the Python standardization scripts in the optimal sequence.
#
# Usage: ./standardize_paths.sh [OPTIONS]
#   Options:
#     --project-root DIR   Specify project root directory (default: current directory)
#     --verbose            Enable verbose logging
#     --no-backup          Skip backup creation
#     --dry-run            Show what would be changed without modifying files
#     --help               Display this help message
#
# Author: Nnamdi Okpala, OBINexusComputing

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
LOG_FILE="${LOG_DIR}/standardization_${TIMESTAMP}.log"

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
      echo "Usage: ./standardize_paths.sh [OPTIONS]"
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

log "Starting LibPolyCall include path standardization process"
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

# Step 2: Basic standardization
log "Step 1/5: Performing basic include standardization..."
CMD="python ${SCRIPTS_DIR}/standardize_includes.py --root ${PROJECT_ROOT} ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 3: Enhanced pattern-specific fixes
log "Step 2/5: Applying enhanced pattern-specific fixes..."
CMD="python ${SCRIPTS_DIR}/enhanced_fix_includes.py --project-root ${PROJECT_ROOT} --fix-polycall-paths ${VERBOSE} ${DRY_RUN}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

# Step 4: Validate includes
log "Step 3/5: Validating include paths..."
VALIDATION_OUTPUT="${LOG_DIR}/validation_${TIMESTAMP}.txt"
CMD="python ${SCRIPTS_DIR}/validate_includes.py --root ${PROJECT_ROOT} --standard polycall"
log "Executing: ${CMD}"
eval "${CMD}" > "${VALIDATION_OUTPUT}" 2>&1
cat "${VALIDATION_OUTPUT}" >> "${LOG_FILE}"

# Check if there are still issues
ISSUES_COUNT=$(grep -c "Issues in" "${VALIDATION_OUTPUT}" || true)
if [ "${ISSUES_COUNT}" -gt 0 ]; then
  log "Found ${ISSUES_COUNT} files with remaining issues"
  
  # Step 5: Fix any remaining issues based on validation
  log "Step 4/5: Fixing remaining validation issues..."
  CMD="python ${SCRIPTS_DIR}/include_path_validator.py --project-root ${PROJECT_ROOT} --error-log ${VALIDATION_OUTPUT} ${DRY_RUN} ${VERBOSE}"
  log "Executing: ${CMD}"
  eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"
else
  log "No validation issues found, skipping additional fixes"
fi

# Step 6: Generate unified header
log "Step 5/5: Generating unified header..."
UNIFIED_HEADER="${PROJECT_ROOT}/include/polycall.h"
CMD="python ${SCRIPTS_DIR}/generate_unified_header.py --project-root ${PROJECT_ROOT} --output ${UNIFIED_HEADER} ${VERBOSE}"
log "Executing: ${CMD}"
eval "${CMD}" 2>&1 | tee -a "${LOG_FILE}"

log "Path standardization process completed"
log "Log file: ${LOG_FILE}"

# Final status report
if [ -z "${DRY_RUN}" ]; then
  log "Summary of changes:"
  log "  - Basic standardization completed"
  log "  - Enhanced pattern fixes applied"
  log "  - Validation issues addressed: ${ISSUES_COUNT}"
  log "  - Unified header generated: ${UNIFIED_HEADER}"
  
  log "Recommendation: Build the project and fix any remaining build errors:"
  log "  mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR} && cmake .. && make 2> build_errors.txt"
  log "  python ${SCRIPTS_DIR}/header_include_fixer.py build_errors.txt --verbose"
else
  log "Dry run completed. No files were modified."
  log "Run without --dry-run to apply changes."
fi

echo ""
echo "Path standardization completed successfully."
echo "See ${LOG_FILE} for details."