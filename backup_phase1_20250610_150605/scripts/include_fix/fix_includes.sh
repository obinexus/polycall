#!/bin/bash
#
# LibPolyCall Include Path Standardization Script
#
# This script runs the complete include path standardization process:
# 1. Validates current include paths to identify issues
# 2. Standardizes all include paths to follow LibPolyCall conventions
# 3. Generates a report on the changes made
# 4. Verifies the changes fixed the issues
#
# Usage: ./fix_includes.sh [--no-backup] [--dry-run] [--verbose]
#

set -e  # Exit on error
# Implementation Details:
# ----------------------
# The standardization process follows this precise workflow:
#
# 1. Module Path Mapping:
#    - Maps common modules to their correct paths (e.g., auth → polycall/core/auth)
#    - Ensures consistent include structure across the codebase
#
# 2. Direct File Mappings:
#    - Applies direct mappings for frequently included headers
#      (e.g., polycall_error.h → polycall/core/polycall/polycall_error.h)
#    - Handles special cases with dedicated rules
#
# 3. Path Normalization:
#    - Fixes relative includes by normalizing paths
#    - Removes unnecessary path components (like "../" or "./")
#
# 4. Nested Path Correction:
#    - Uses pattern-matching to identify and fix complex nested paths
#    - Addresses patterns like polycall/core/polycall/core/auth/ which are
#      particularly problematic in the auth module
#
# 5. Filesystem Verification:
#    - Verifies changes against the filesystem to ensure headers exist
#    - Validates that the standardized paths point to actual files
#
# This multi-step approach ensures both correctness and consistency across
# the entire codebase, while handling the unique requirements of LibPolyCall.

# Properly determine the project root (parent of the scripts directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SCRIPT_DIR}")"
LOGS_DIR="${PROJECT_ROOT}/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Default options
DRY_RUN=""
VERBOSE=""
BACKUP=""
HELP=false

# Process command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run)
      DRY_RUN="--dry-run"
      shift
      ;;
    --no-backup)
      BACKUP="--no-backup"
      shift
      ;;
    --verbose)
      VERBOSE="--verbose"
      shift
      ;;
    --help)
      HELP=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      HELP=true
      shift
      ;;
  esac
done

# Display help if requested
if $HELP; then
  echo "LibPolyCall Include Path Standardization Script"
  echo ""
  echo "Usage: $0 [OPTIONS]"
  echo ""
  echo "Options:"
  echo "  --dry-run      Show what would be changed without modifying files"
  echo "  --no-backup    Skip creating backup files"
  echo "  --verbose      Enable verbose logging"
  echo "  --help         Display this help message"
  echo ""
  exit 0
fi

# Make sure the logs directory exists
mkdir -p "${LOGS_DIR}"

# Log file for this run
LOG_FILE="${LOGS_DIR}/include_fix_${TIMESTAMP}.log"
# JSON report file
REPORT_FILE="${LOGS_DIR}/include_report_${TIMESTAMP}.json"

# Log function to write to both console and log file
log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "${LOG_FILE}"
}

log "Starting LibPolyCall include path standardization"
log "Project root: ${PROJECT_ROOT}"
log "Scripts directory: ${SCRIPT_DIR}"

# Step 1: Validate current include paths to identify issues
log "Step 1/4: Validating current include paths"
VALIDATION_FILE="${LOGS_DIR}/validation_before_${TIMESTAMP}.log"
python "${SCRIPT_DIR}/validate_include_paths.py" \
  --project-root "${PROJECT_ROOT}" \
  --verbose \
  --report "${VALIDATION_FILE}" \
  2>&1 | tee -a "${LOG_FILE}"

# Step 2: Standardize all include paths
log "Step 2/4: Standardizing include paths"
python "${SCRIPT_DIR}/standardize_includes.py" \
  --project-root "${PROJECT_ROOT}" \
  ${DRY_RUN} \
  ${VERBOSE} \
  ${BACKUP} \
  --json-report "${REPORT_FILE}" \
  2>&1 | tee -a "${LOG_FILE}"

# Exit if this was a dry run (no need to verify changes)
if [[ -n "${DRY_RUN}" ]]; then
  log "Dry run completed. No files were modified."
  log "See ${LOG_FILE} for details"
  exit 0
fi

# Step 3: Verify the changes fixed the issues
log "Step 3/4: Verifying changes"
VALIDATION_AFTER="${LOGS_DIR}/validation_after_${TIMESTAMP}.log"
python "${SCRIPT_DIR}/validate_include_paths.py" \
  --project-root "${PROJECT_ROOT}" \
  --verbose \
  --report "${VALIDATION_AFTER}" \
  2>&1 | tee -a "${LOG_FILE}"

# Step 4: Fix any remaining special cases with the targeted fixer
log "Step 4/4: Fixing any remaining issues"
python "${SCRIPT_DIR}/fix_include_paths.py" \
  --project-root "${PROJECT_ROOT}" \
  ${VERBOSE} \
  2>&1 | tee -a "${LOG_FILE}"

# Step 5: Fix implementation-specific includes
log "Step 5/5: Fixing implementation-specific includes"
python "${SCRIPT_DIR}/fix_implementation_includes.py" \
  --project-root "${PROJECT_ROOT}" \
  ${DRY_RUN} \
  ${VERBOSE} \
  2>&1 | tee -a "${LOG_FILE}"

# Final verification
FINAL_VALIDATION="${LOGS_DIR}/validation_final_${TIMESTAMP}.log"
python "${SCRIPT_DIR}/validate_include_paths.py" \
  --project-root "${PROJECT_ROOT}" \
  --verbose \
  --report "${FINAL_VALIDATION}" \
  2>&1 | tee -a "${LOG_FILE}"

# Print summary
log "Include path standardization completed"
log "Summary:"
log "  - Initial validation: ${VALIDATION_FILE}"
log "  - Standardization report: ${REPORT_FILE}"
log "  - Final validation: ${FINAL_VALIDATION}"
log "  - Complete log: ${LOG_FILE}"

echo ""
echo "Process completed. See ${LOG_FILE} for full details."

# Check if there are remaining issues
if [ -f "${FINAL_VALIDATION}" ]; then
  REMAINING_ISSUES=$(grep -c "Non-compliant includes:" "${FINAL_VALIDATION}" 2>/dev/null || echo "0")
  REMAINING_ISSUES=$(echo "${REMAINING_ISSUES}" | cut -d':' -f2 | tr -d ' ')
  
  if [[ -z "${REMAINING_ISSUES}" || "${REMAINING_ISSUES}" == "0" ]]; then
    log "SUCCESS: All include paths now follow the standard format"
    exit 0
  else
    log "WARNING: There are still ${REMAINING_ISSUES} non-compliant includes"
    exit 1
  fi
else
  log "WARNING: Final validation file not found"
  exit 1
