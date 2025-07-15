#!/bin/bash
# Installs the comprehensive path fixer script into the LibPolyCall project structure
# and adds it to the project's Makefile for easy execution

set -e  # Exit on error

# Configuration
SCRIPT_NAME="fix_all_paths.py"
TARGET_DIR="scripts"
MAKEFILE="Makefile"

# Function to display usage information
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --project-root DIR   Specify project root directory (default: current directory)"
    echo "  --help               Display this help message"
    exit 1
}

# Default values
PROJECT_ROOT="$(pwd)"

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --project-root)
            PROJECT_ROOT="$2"
            shift 2
            ;;
        --help)
            usage
            ;;
        *)
            echo "Error: Unknown option: $1"
            usage
            ;;
    esac
done

# Ensure project root exists
if [ ! -d "$PROJECT_ROOT" ]; then
    echo "Error: Project root directory '$PROJECT_ROOT' does not exist."
    exit 1
fi

# Ensure scripts directory exists
SCRIPTS_DIR="${PROJECT_ROOT}/${TARGET_DIR}"
if [ ! -d "$SCRIPTS_DIR" ]; then
    echo "Creating scripts directory..."
    mkdir -p "$SCRIPTS_DIR"
fi

# Copy fix script to scripts directory
echo "Installing ${SCRIPT_NAME} to ${SCRIPTS_DIR}..."
cp "${SCRIPT_NAME}" "${SCRIPTS_DIR}/"
chmod +x "${SCRIPTS_DIR}/${SCRIPT_NAME}"

# Update Makefile with new target if it exists
MAKEFILE_PATH="${PROJECT_ROOT}/${MAKEFILE}"
if [ -f "$MAKEFILE_PATH" ]; then
    echo "Updating Makefile with new fix-all-paths target..."
    
    # Check if target already exists to avoid duplicate entries
    if grep -q "fix-all-paths:" "$MAKEFILE_PATH"; then
        echo "Target already exists in Makefile. Skipping..."
    else
        # Add new target to Makefile
        cat >> "$MAKEFILE_PATH" << EOF

# Fix all include path issues with comprehensive fixer
.PHONY: fix-all-paths
fix-all-paths:
	@echo "Running comprehensive include path fixer..."
	@python3 \$(SCRIPTS_DIR)/${SCRIPT_NAME} --project-root \$(PROJECT_ROOT) \$(if \$(VERBOSE),--verbose,)
	@echo "Complete path standardization completed."
	@echo "Run 'make validate-includes-direct' to verify fixes."
EOF
        echo "Makefile updated successfully."
    fi
else
    echo "Warning: Makefile not found at ${MAKEFILE_PATH}. No updates applied."
fi

echo "Installation complete!"
echo "To fix all include paths, run: make fix-all-paths"
echo "For manual execution: python3 ${SCRIPTS_DIR}/${SCRIPT_NAME} --project-root <project_root>"