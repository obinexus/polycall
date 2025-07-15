#!/bin/bash
# OBINexus LibPolyCall v2 - Branching Strategy Implementation
# Purpose: Establish systematic branch management for different release channels

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

log() { echo "[BRANCHING] $*" >&2; }

create_release_branches() {
    cd "$PROJECT_ROOT"
    
    local current_branch
    current_branch=$(git rev-parse --abbrev-ref HEAD)
    
    log "Current branch: $current_branch"
    
    # Ensure we're on develop or main
    if [[ "$current_branch" != "develop" && "$current_branch" != "main" && "$current_branch" != "dev-main" ]]; then
        log "Switching to develop branch for release preparation"
        git checkout develop 2>/dev/null || git checkout -b develop
    fi
    
    # Create release channels if they don't exist
    create_branch_if_not_exists "experimental/polycall-v2" "Experimental development channel"
    create_branch_if_not_exists "alpha/polycall-v2" "Alpha testing channel"
    create_branch_if_not_exists "beta/polycall-v2" "Beta testing channel"
    create_branch_if_not_exists "release-candidate/polycall-v2" "Release candidate channel"
    
    log "Branch strategy implementation complete"
}

create_branch_if_not_exists() {
    local branch_name="$1"
    local description="$2"
    
    if git show-ref --verify --quiet "refs/heads/$branch_name"; then
        log "Branch exists: $branch_name"
    else
        log "Creating branch: $branch_name ($description)"
        git checkout -b "$branch_name"
        git checkout -  # Return to previous branch
    fi
}

promote_to_channel() {
    local target_channel="$1"
    local version="$2"
    local codename="$3"
    
    cd "$PROJECT_ROOT"
    
    case "$target_channel" in
        "experimental")
            promote_to_experimental "$version" "$codename"
            ;;
        "alpha")
            promote_to_alpha "$version" "$codename"
            ;;
        "beta")
            promote_to_beta "$version" "$codename"
            ;;
        "stable")
            promote_to_stable "$version" "$codename"
            ;;
        *)
            echo "Error: Invalid channel. Use: experimental, alpha, beta, stable"
            exit 1
            ;;
    esac
}

promote_to_experimental() {
    local version="$1"
    local codename="$2"
    
    log "Promoting to experimental channel: v$version-$codename"
    
    git checkout experimental/polycall-v2
    git merge develop --no-ff -m "Merge develop for experimental v$version-$codename"
    
    # Update version with experimental designation
    jq --arg version "$version" --arg codename "$codename" '
        .version.prerelease = "experimental" |
        .status = "experimental" |
        .channel = "experimental"
    ' config/versions/current.json > config/versions/current.json.tmp
    mv config/versions/current.json.tmp config/versions/current.json
    
    git add config/versions/current.json
    git commit -m "Update version for experimental v$version-$codename"
}

promote_to_alpha() {
    local version="$1"
    local codename="$2"
    
    log "Promoting to alpha channel: v$version-alpha-$codename"
    
    git checkout alpha/polycall-v2
    git merge experimental/polycall-v2 --no-ff -m "Promote experimental to alpha v$version-$codename"
    
    jq --arg version "$version" --arg codename "$codename" '
        .version.prerelease = "alpha.1" |
        .status = "alpha" |
        .channel = "alpha"
    ' config/versions/current.json > config/versions/current.json.tmp
    mv config/versions/current.json.tmp config/versions/current.json
    
    git add config/versions/current.json
    git commit -m "Update version for alpha v$version-$codename"
}

promote_to_beta() {
    local version="$1"
    local codename="$2"
    
    log "Promoting to beta channel: v$version-beta-$codename"
    
    git checkout beta/polycall-v2
    git merge alpha/polycall-v2 --no-ff -m "Promote alpha to beta v$version-$codename"
    
    jq --arg version "$version" --arg codename "$codename" '
        .version.prerelease = "beta.1" |
        .status = "beta" |
        .channel = "beta"
    ' config/versions/current.json > config/versions/current.json.tmp
    mv config/versions/current.json.tmp config/versions/current.json
    
    git add config/versions/current.json
    git commit -m "Update version for beta v$version-$codename"
}

promote_to_stable() {
    local version="$1"
    local codename="$2"
    
    log "Promoting to stable release: v$version-$codename"
    
    # Create release branch
    git checkout -b "release/v$version-$codename" beta/polycall-v2
    
    jq --arg version "$version" --arg codename "$codename" '
        .version.prerelease = null |
        .status = "stable" |
        .channel = "stable" |
        .release_date = now | strftime("%Y-%m-%d")
    ' config/versions/current.json > config/versions/current.json.tmp
    mv config/versions/current.json.tmp config/versions/current.json
    
    git add config/versions/current.json
    git commit -m "Prepare stable release v$version-$codename"
    
    # Merge to main
    git checkout main
    git merge "release/v$version-$codename" --no-ff -m "Release v$version-$codename"
}

# Usage demonstration
case "${1:-help}" in
    "init")
        create_release_branches
        ;;
    "promote")
        promote_to_channel "$2" "$3" "$4"
        ;;
    "help")
        echo "Usage: $0 <command> [args]"
        echo ""
        echo "Commands:"
        echo "  init                           Initialize branch structure"
        echo "  promote <channel> <version> <codename>  Promote to release channel"
        echo ""
        echo "Channels: experimental, alpha, beta, stable"
        echo "Example: $0 promote alpha 2.0.0 aegis"
        ;;
    *)
        echo "Unknown command: $1"
        echo "Use: $0 help"
        exit 1
        ;;
esac
