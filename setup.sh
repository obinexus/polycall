#!/bin/bash
# LibPolyCall v2 Setup Script - Unix/Linux
# Generated: 2025-07-02 22:26:00
# OBINexus Aegis Project - Sinphasé Governance

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Project paths
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]})"/../.. && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"

echo -e "${BLUE}=== LibPolyCall v2 Build Setup ===${NC}"
echo "Project Root: $PROJECT_ROOT"

# Check dependencies
echo -e "\n${YELLOW}Checking dependencies...${NC}"
MISSING_DEPS=""
for dep in gcc make ar python3; do
    if ! command -v $dep &> /dev/null; then
        MISSING_DEPS="$MISSING_DEPS $dep"
        echo -e "${RED}✗ $dep not found${NC}"
    else
        echo -e "${GREEN}✓ $dep found${NC}"
    fi
done

if [ -n "$MISSING_DEPS" ]; then
    echo -e "\n${RED}Error: Missing dependencies:$MISSING_DEPS${NC}"
    echo "Please install missing dependencies before continuing."
    exit 1
fi

# Create build directories
echo -e "\n${YELLOW}Creating build directories...${NC}"
mkdir -p "$BUILD_DIR/obj/core" "$BUILD_DIR/obj/cli"
mkdir -p "$BUILD_DIR/lib" "$BUILD_DIR/bin/debug" "$BUILD_DIR/bin/prod"
mkdir -p "$BUILD_DIR/include/polycall"

# Fix include paths
echo -e "\n${YELLOW}Fixing include paths...${NC}"
if [ -f "$SCRIPTS_DIR/build/fix_include_paths.py" ]; then
    python3 "$SCRIPTS_DIR/build/fix_include_paths.py" --project-root "$PROJECT_ROOT"
else
    echo -e "${YELLOW}Include path fixer not found, skipping...${NC}"
fi

# Stage headers
echo -e "\n${YELLOW}Staging headers...${NC}"
if [ -d "$PROJECT_ROOT/include/polycall" ]; then
    cp -r "$PROJECT_ROOT/include/polycall" "$BUILD_DIR/include/"
    echo -e "${GREEN}✓ Headers staged${NC}"
fi

# Run build orchestrator
echo -e "\n${YELLOW}Running build orchestrator...${NC}"
if [ -f "$SCRIPTS_DIR/build/build_orchestrator.py" ]; then
    python3 "$SCRIPTS_DIR/build/build_orchestrator.py" \
        --project-root "$PROJECT_ROOT" \
        --config debug \
        --verbose
else
    echo -e "${RED}Build orchestrator not found!${NC}"
    exit 1
fi

echo -e "\n${GREEN}=== Setup Complete ===${NC}"
echo "Build artifacts location: $BUILD_DIR"
echo "Run 'make build' to compile the project"
