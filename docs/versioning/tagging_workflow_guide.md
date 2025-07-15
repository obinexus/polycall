# Git Tagging Workflow Guide

## Complete Version Management Workflow

### 1. Development Phase
```bash
# Start development on feature branch
git checkout develop
git checkout -b feature/polycall-ugly-gating

# Make changes, commit regularly
git add .
git commit -m "Implement sinphase governance optimization"

# Merge back to develop when complete
git checkout develop
git merge feature/polycall-ugly-gating --no-ff
```

### 2. Release Preparation
```bash
# Prepare release (updates version files, generates changelog)
./scripts/release/prepare-release.sh patch aegis

# Review and commit release preparation
git add .
git commit -m "Prepare release v2.0.1-aegis"
```

### 3. Channel Promotion Workflow

#### Initialize Branch Structure (First Time)
```bash
./scripts/release/branching-strategy.sh init
```

#### Experimental Channel
```bash
# Promote to experimental for internal testing
./scripts/release/branching-strategy.sh promote experimental 2.0.1 aegis

# Create experimental tag
git checkout experimental/polycall-v2
./scripts/release/create-tag.sh 2.0.1 aegis annotated
```

#### Alpha Channel
```bash
# Promote to alpha for extended testing
./scripts/release/branching-strategy.sh promote alpha 2.0.1 aegis

# Create alpha tag
git checkout alpha/polycall-v2
./scripts/release/create-tag.sh 2.0.1 aegis annotated
```

#### Beta Channel
```bash
# Promote to beta for release candidate testing
./scripts/release/branching-strategy.sh promote beta 2.0.1 aegis

# Create beta tag
git checkout beta/polycall-v2
./scripts/release/create-tag.sh 2.0.1 aegis annotated
```

#### Stable Release
```bash
# Final promotion to stable release
./scripts/release/branching-strategy.sh promote stable 2.0.1 aegis

# Create stable release tag
git checkout main
./scripts/release/create-tag.sh 2.0.1 aegis signed
```

### 4. Tag Variations and Examples

#### Standard Release Tags
```bash
# Stable release
git tag -a v2.0.0-aegis -m "Stable release v2.0.0 (Aegis)"

# With semantic versioning
git tag -a v2.0.0 -m "LibPolyCall v2.0.0 stable"

# Pre-release versions
git tag -a v2.0.0-alpha.1-aegis -m "Alpha release v2.0.0"
git tag -a v2.0.0-beta.1-aegis -m "Beta release v2.0.0"
git tag -a v2.0.0-rc.1-aegis -m "Release candidate v2.0.0"
```

#### Convenience Tags
```bash
# Latest stable in major.minor series
git tag -f v2.0-latest

# Latest by codename
git tag -f latest-aegis

# Channel-specific latest
git tag -f alpha-latest-aegis
git tag -f beta-latest-aegis
```

#### Milestone Tags
```bash
# Development milestones
git tag milestone/unified-realignment-complete
git tag milestone/polycall-ugly-optimized
git tag milestone/aegis-phase-2-complete

# Integration points
git tag integration/ffi-interface-stable
git tag integration/build-system-optimized
```

### 5. Advanced Tagging Operations

#### Signed Tags (Recommended for Releases)
```bash
# Create GPG-signed tag
git tag -s v2.0.0-aegis -m "Signed release v2.0.0 (Aegis)"

# Verify signed tag
git tag -v v2.0.0-aegis
```

#### Retroactive Tagging
```bash
# Tag specific commit
git tag -a v1.9.9-legacy abc1234 -m "Legacy version tag"

# Tag with specific date
GIT_COMMITTER_DATE="2025-01-01 12:00:00" git tag -a v2.0.0-aegis -m "Backdated tag"
```

#### Tag Management
```bash
# List all tags
git tag -l

# List tags with pattern
git tag -l "v2.0.*"
git tag -l "*aegis*"

# Show tag details
git show v2.0.0-aegis

# Delete local tag
git tag -d v2.0.0-aegis

# Delete remote tag
git push origin --delete v2.0.0-aegis
```

### 6. Release Channel Strategy

#### Channel Progression
```
Development → Experimental → Alpha → Beta → Stable
     ↓              ↓          ↓       ↓       ↓
feature/*    experimental/* alpha/* beta/*  main
```

#### Tag Naming Convention
```
Experimental: v2.0.0-experimental-aegis
Alpha:        v2.0.0-alpha.1-aegis
Beta:         v2.0.0-beta.1-aegis
RC:           v2.0.0-rc.1-aegis
Stable:       v2.0.0-aegis or v2.0.0
```

### 7. Automated Release Workflow

#### Complete Automated Release
```bash
#!/bin/bash
# Automated release script example

VERSION="2.0.1"
CODENAME="aegis"

# Prepare release
./scripts/release/prepare-release.sh patch $CODENAME

# Commit preparation
git add .
git commit -m "Prepare release v$VERSION-$CODENAME"

# Promote through channels
./scripts/release/branching-strategy.sh promote experimental $VERSION $CODENAME
./scripts/release/create-tag.sh $VERSION $CODENAME

./scripts/release/branching-strategy.sh promote alpha $VERSION $CODENAME
./scripts/release/create-tag.sh $VERSION $CODENAME

./scripts/release/branching-strategy.sh promote beta $VERSION $CODENAME
./scripts/release/create-tag.sh $VERSION $CODENAME

./scripts/release/branching-strategy.sh promote stable $VERSION $CODENAME
./scripts/release/create-tag.sh $VERSION $CODENAME signed

echo "Release v$VERSION-$CODENAME complete!"
```

### 8. Integration with Build Systems

#### Makefile Integration
```makefile
# Get version from git tag
VERSION := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "dev")
BUILD_HASH := $(shell git rev-parse --short HEAD)

version:
	@echo "Version: $(VERSION)+$(BUILD_HASH)"

tag-release:
	@./scripts/release/create-tag.sh $(VERSION) aegis annotated
```

#### CI/CD Integration
```yaml
# GitHub Actions example
name: Release
on:
  push:
    tags:
      - 'v*-aegis'

jobs:
  release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Create Release
        run: |
          VERSION=${GITHUB_REF#refs/tags/v}
          echo "Creating release for $VERSION"
```

This workflow ensures systematic version management while maintaining
flexibility for different release cadences and deployment strategies.
