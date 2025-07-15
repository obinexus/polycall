#!/bin/bash
# OBINexus LibPolyCall v2 - Release Preparation Script
# Purpose: Prepare project for version tagging and release

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
RELEASE_TYPE=${1:-"patch"}
CODENAME=${2:-"aegis"}

log() { echo "[RELEASE-PREP] $*" >&2; }

prepare_release() {
    local release_type="$1"
    local codename="$2"
    
    cd "$PROJECT_ROOT"
    
    log "Preparing $release_type release with codename: $codename"
    
    # Validate clean state
    if [[ -n "$(git status --porcelain)" ]]; then
        echo "Error: Working directory not clean. Commit or stash changes."
        exit 1
    fi
    
    # Calculate new version
    local current_version
    current_version=$(jq -r '.version | "\(.major).\(.minor).\(.patch)"' config/versions/current.json)
    
    local new_version
    case "$release_type" in
        "major")
            new_version=$(echo "$current_version" | awk -F. '{print ($1+1)".0.0"}')
            ;;
        "minor")
            new_version=$(echo "$current_version" | awk -F. '{print $1"."($2+1)".0"}')
            ;;
        "patch")
            new_version=$(echo "$current_version" | awk -F. '{print $1"."$2"."($3+1)}')
            ;;
        *)
            echo "Error: Invalid release type. Use: major, minor, or patch"
            exit 1
            ;;
    esac
    
    log "Version transition: $current_version → $new_version"
    
    # Update version file
    jq --arg version "$new_version" --arg codename "$codename" '
        .version.major = ($version | split(".")[0] | tonumber) |
        .version.minor = ($version | split(".")[1] | tonumber) |
        .version.patch = ($version | split(".")[2] | tonumber) |
        .codename = $codename |
        .status = "release-candidate" |
        .last_updated = now | strftime("%Y-%m-%dT%H:%M:%SZ")
    ' config/versions/current.json > config/versions/current.json.tmp
    
    mv config/versions/current.json.tmp config/versions/current.json
    
    # Generate changelog
    generate_changelog "$new_version" "$codename"
    
    # Update documentation
    update_version_documentation "$new_version" "$codename"
    
    log "Release preparation complete"
    echo "Next steps:"
    echo "1. Review changes: git diff"
    echo "2. Commit changes: git add . && git commit -m 'Prepare release v$new_version-$codename'"
    echo "3. Create release: ./scripts/release/create-tag.sh $new_version $codename"
}

generate_changelog() {
    local version="$1"
    local codename="$2"
    
    local changelog_file="docs/changelogs/v${version}-${codename}.md"
    
    cat > "$changelog_file" << EOL
# Changelog - v${version} (${codename^})

## Release Information
- **Version**: v${version}-${codename}
- **Release Date**: $(date -u +%Y-%m-%d)
- **Branch**: $(git rev-parse --abbrev-ref HEAD)
- **Commit**: $(git rev-parse --short HEAD)

## Major Changes

### 🚀 Features
- Unified directory realignment implementation
- Comprehensive validation script framework
- Git version management system

### 🔧 Improvements
- Component isolation architecture established
- Build system integration prepared
- Performance monitoring implemented

### 🐛 Bug Fixes
- Directory structure compliance issues resolved
- Include path references updated
- Validation pipeline inconsistencies addressed

### 📚 Documentation
- Project state documentation comprehensive
- Naming standards established
- Version management procedures documented

### 🔒 Security
- Architectural boundary enforcement implemented
- Validation pipeline security checks active

## Breaking Changes
None in this release.

## Migration Guide
No migration required for this release.

## Known Issues
- Build system dependencies (Meson/Ninja) require installation
- Some validation warnings present (non-blocking)

## Next Release
Focus on POLYCALL_UGLY module sinphase optimization and build performance improvements.
EOL
    
    log "Changelog generated: $changelog_file"
}

update_version_documentation() {
    local version="$1"
    local codename="$2"
    
    # Update README version badge
    if [[ -f "README.md" ]]; then
        sed -i "s/version-[^-]*-/version-$version-/" README.md || true
    fi
    
    # Update project documentation
    cat > docs/releases/v${version}-${codename}.md << EOL
# Release Notes - v${version} (${codename^})

## Overview
This release represents a significant milestone in the OBINexus LibPolyCall v2 
development, establishing the foundational architecture and tooling framework
for the Aegis project progression.

## Target Audience
- **Developers**: Enhanced development workflow with validation tools
- **DevOps**: Improved build system integration and monitoring
- **Architects**: Clear component isolation and dependency management

## Installation

### From Source
\`\`\`bash
git clone https://github.com/obinexus/polycall.git
cd polycall
git checkout v${version}-${codename}
make build
\`\`\`

### Dependencies
- GCC 11+ or Clang 13+
- Make 4.0+
- Optional: Meson 0.60+, Ninja 1.10+

## Architecture Highlights

### Component Isolation
- Clear separation between core, CLI, FFI, and binding modules
- Architectural boundary enforcement through validation tools
- Sinphase governance framework for coupling management

### Build System
- Makefile-based primary build system
- Meson/Ninja integration prepared for performance optimization
- Comprehensive validation and testing pipeline

### Quality Assurance
- Automated orphan file detection
- Configuration validation framework
- Build regression monitoring
- Performance baseline establishment

## Performance Characteristics
- Target build time improvement: 60% (with Meson/Ninja)
- Sinphase governance target: ≤ 0.5 (architectural coupling)
- Validation coverage: 100% (critical path coverage)

## Compatibility
- **Platforms**: Linux (primary), macOS, Windows (MSYS2)
- **Compilers**: GCC 11+, Clang 13+, MSVC 2019+ (Windows)
- **Standards**: ISO C11, POSIX.1-2017

## Support
- **Documentation**: docs.obinexus.io/libpolycall
- **Issues**: github.com/obinexus/polycall/issues
- **Community**: OBINexus Engineering Team
EOL
    
    log "Release documentation updated"
}

# Execute release preparation
prepare_release "$RELEASE_TYPE" "$CODENAME"
