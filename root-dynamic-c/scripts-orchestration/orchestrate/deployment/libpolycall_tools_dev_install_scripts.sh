#!/bin/bash
# install_scripts.sh - Install Include Path Standardization Scripts
# 
# This script installs or updates all the necessary scripts for standardizing
# include paths in the LibPolyCall project and adds appropriate targets to the Makefile.
#
# Usage: ./install_scripts.sh [OPTIONS]
#   Options:
#     --project-root DIR   Specify project root directory (default: current directory)
#     --help               Display this help message
#
# Author: OBINexusComputing

set -e  # Exit on error

# Default values
PROJECT_ROOT="$(pwd)"
SCRIPTS_DIR="${PROJECT_ROOT}/scripts"
MAKEFILE="${PROJECT_ROOT}/Makefile"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --project-root)
      PROJECT_ROOT="$2"
      shift 2
      ;;
    --help)
      echo "Usage: ./install_scripts.sh [OPTIONS]"
      echo "Options:"
      echo "  --project-root DIR   Specify project root directory (default: current directory)"
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

# Function to display status
status() {
  echo "[*] $1"
}

# Create scripts directory if it doesn't exist
if [ ! -d "${SCRIPTS_DIR}" ]; then
  status "Creating scripts directory: ${SCRIPTS_DIR}"
  mkdir -p "${SCRIPTS_DIR}"
fi

# List of script files to install
declare -A SCRIPT_FILES
SCRIPT_FILES[standardize_includes.py]="Basic include path standardizer"
SCRIPT_FILES[generate_unified_header.py]="Unified header generator"
SCRIPT_FILES[validate_includes.py]="Include path validator"
SCRIPT_FILES[fix_all_includes.sh]="Comprehensive include path fixing script"

# Install each script
for script in "${!SCRIPT_FILES[@]}"; do
  description="${SCRIPT_FILES[$script]}"
  status "Installing $script - $description"
  
  # Copy script to scripts directory
  if [ -f "$script" ]; then
    cp "$script" "${SCRIPTS_DIR}/"
    chmod +x "${SCRIPTS_DIR}/${script}"
    status "  Successfully installed ${script}"
  else
    status "  WARNING: Could not find ${script} in current directory"
  fi
done

# Create logs directory
mkdir -p "${PROJECT_ROOT}/logs"
status "Created logs directory: ${PROJECT_ROOT}/logs"

# Create template directory and add polycall.h.in if needed
mkdir -p "${PROJECT_ROOT}/include/polycall"
if [ ! -f "${PROJECT_ROOT}/include/polycall/polycall.h.in" ] && [ -f "polycall.h.in" ]; then
  status "Installing polycall.h.in template..."
  cp "polycall.h.in" "${PROJECT_ROOT}/include/polycall/"
  status "Successfully installed polycall.h.in template"
else
  status "Note: polycall.h.in template not found or already exists"
fi

# Update Makefile with standardization targets
if [ -f "${MAKEFILE}" ]; then
  status "Updating Makefile with standardization targets..."
  
  # Check if the standardization section already exists
  if grep -q "# Include Path Standardization Targets" "${MAKEFILE}"; then
    status "  Standardization targets already exist in Makefile, skipping..."
  else
    # Add standardization targets to Makefile
    cat >> "${MAKEFILE}" << 'EOF'

# Include Path Standardization Targets
SCRIPTS_DIR := $(shell pwd)/scripts
LOG_DIR := $(shell pwd)/logs

# Fix all include paths (comprehensive)
.PHONY: fix-all-includes
fix-all-includes:
	@echo "Running comprehensive include path fixes..."
	@$(SCRIPTS_DIR)/fix_all_includes.sh $(if $(VERBOSE),--verbose,)

# Validate include paths against standard
.PHONY: validate-includes
validate-includes:
	@echo "Validating include paths..."
	@python3 $(SCRIPTS_DIR)/validate_includes.py --root $(shell pwd) --standard polycall | tee $(LOG_DIR)/validation_$(shell date +%Y%m%d_%H%M%S).txt

# Fix include paths
.PHONY: standardize-includes
standardize-includes:
	@echo "Standardizing include paths..."
	@python3 $(SCRIPTS_DIR)/standardize_includes.py --root $(shell pwd) $(if $(VERBOSE),--verbose,)

# Generate unified header
.PHONY: unified-header
unified-header:
	@echo "Generating unified header..."
	@mkdir -p build/include
	@python3 $(SCRIPTS_DIR)/generate_unified_header.py --project-root $(shell pwd) --output build/include/polycall.h $(if $(VERBOSE),--verbose,)

# Complete include workflow
.PHONY: include-workflow
include-workflow: standardize-includes validate-includes unified-header
	@echo "Include path workflow completed."

# Show include path guide
.PHONY: include-guide
include-guide:
	@python3 $(SCRIPTS_DIR)/standardize_includes.py --show-guide
EOF

    status "  Successfully added standardization targets to Makefile"
  fi
else
  status "  WARNING: Makefile not found at ${MAKEFILE}"
fi

# Update CMakeLists.txt with include validation if present
CMAKE_ROOT="${PROJECT_ROOT}/CMakeLists.txt"
if [ -f "${CMAKE_ROOT}" ]; then
  status "Checking CMakeLists.txt for include validation..."
  
  if grep -q "ENABLE_INCLUDE_VALIDATION" "${CMAKE_ROOT}"; then
    status "  Include validation already exists in CMakeLists.txt, skipping..."
  else
    status "  Note: To enable include validation in CMake, add the snippet from cmake-integration.txt"
    status "  to your root CMakeLists.txt file."
  fi
fi

status "Installation complete!"
status "Run 'make include-guide' for usage information"
status "Or run 'make include-workflow' to fix all include path issues"