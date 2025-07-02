#!/bin/bash
# remove-zone-identifier.sh
# Remove all Windows Zone.Identifier files from include/polycall recursively

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd -P)"
TARGET_DIR="$PROJECT_ROOT/include/polycall"

if [ -d "$TARGET_DIR" ]; then
	find "$TARGET_DIR" -type f -name "*Zone.Identifier" -exec rm -v {} + || true
	echo "All Zone.Identifier files removed from $TARGET_DIR."
else
	echo "Directory $TARGET_DIR does not exist."
fi

# Also remove Zone.Identifier files in the script's directory
SCRIPT_ROOT="$(cd "$(dirname "$0")" && pwd -P)"
if [ -d "$SCRIPT_ROOT" ]; then
	find "$SCRIPT_ROOT" -maxdepth 1 -type f -name "*Zone.Identifier" -exec rm -v {} + || true
	echo "All Zone.Identifier files removed from $SCRIPT_ROOT."
else
	echo "Script directory $SCRIPT_ROOT does not exist."
fi
