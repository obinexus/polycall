#!/bin/bash
# OBINexus LibPolyCall v2 - Version Tagging Script
# Purpose: Create systematic version tags with metadata

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

VERSION=${1:-}
CODENAME=${2:-"aegis"}
TAG_TYPE=${3:-"annotated"}
PUSH_TAGS=${PUSH_TAGS:-true}

log() { echo "[TAG-CREATE] $*" >&2; }

create_version_tag() {
    local version="$1"
    local codename="$2"
    local tag_type="$3"
    
    cd "$PROJECT_ROOT"
    
    if [[ -z "$version" ]]; then
        echo "Error: Version required"
        echo "Usage: $0 <version> [codename] [tag_type]"
        echo "Example: $0 2.0.0 aegis annotated"
        exit 1
    fi
    
    # Validate clean state
    if [[ -n "$(git status --porcelain)" ]]; then
        echo "Error: Working directory not clean"
        exit 1
    fi
    
    # Determine tag name variations
    local tag_base="v$version"
    local tag_with_codename="v$version-$codename"
    
    # Get version metadata from config
    local version_status
    version_status=$(jq -r '.status' config/versions/current.json 2>/dev/null || echo "development")
    
    local prerelease
    prerelease=$(jq -r '.version.prerelease // empty' config/versions/current.json 2>/dev/null || echo "")
    
    # Construct final tag name based on status
    local final_tag
    if [[ -n "$prerelease" ]]; then
        final_tag="$tag_base-$prerelease-$codename"
    else
        final_tag="$tag_with_codename"
    fi
    
    log "Creating tag: $final_tag (type: $tag_type)"
    
    # Generate comprehensive tag message
    local tag_message
    tag_message=$(generate_tag_message "$version" "$codename" "$version_status" "$prerelease")
    
    # Create tag based on type
    case "$tag_type" in
        "annotated")
            create_annotated_tag "$final_tag" "$tag_message"
            ;;
        "lightweight")
            create_lightweight_tag "$final_tag"
            ;;
        "signed")
            create_signed_tag "$final_tag" "$tag_message"
            ;;
        *)
            echo "Error: Invalid tag type. Use: annotated, lightweight, signed"
            exit 1
            ;;
    esac
    
    # Create additional convenience tags
    create_convenience_tags "$version" "$codename" "$version_status" "$prerelease"
    
    # Push tags if requested
    if [[ "$PUSH_TAGS" == "true" ]]; then
        push_tags_to_remote "$final_tag"
    fi
    
    # Update local tag registry
    update_tag_registry "$final_tag" "$version" "$codename" "$version_status"
    
    log "Version tagging complete: $final_tag"
}

generate_tag_message() {
    local version="$1"
    local codename="$2"
    local status="$3"
    local prerelease="$4"
    
    local build_hash
    build_hash=$(git rev-parse --short HEAD)
    
    cat << EOL
OBINexus LibPolyCall v$version ($codename^)

Release Information:
- Version: v$version$(test -n "$prerelease" && echo "-$prerelease")
- Codename: $codename^
- Status: $status
- Build: $build_hash
- Date: $(date -u +%Y-%m-%d)
- Branch: $(git rev-parse --abbrev-ref HEAD)

Project State:
- Unified realignment: Complete
- Directory structure: Target compliant
- Validation framework: Implemented
- Build system: Integration ready

Key Features:
- Component isolation architecture
- Comprehensive validation scripts
- Performance monitoring framework
- Git version management system

Next Phase:
- POLYCALL_UGLY module sinphase optimization
- Build performance improvements
- Aegis project integration

Built with OBINexus Engineering Standards
EOL
}

create_annotated_tag() {
    local tag_name="$1"
    local tag_message="$2"
    
    log "Creating annotated tag: $tag_name"
    echo "$tag_message" | git tag -a "$tag_name" -F -
}

create_lightweight_tag() {
    local tag_name="$1"
    
    log "Creating lightweight tag: $tag_name"
    git tag "$tag_name"
}

create_signed_tag() {
    local tag_name="$1"
    local tag_message="$2"
    
    log "Creating signed tag: $tag_name"
    echo "$tag_message" | git tag -s "$tag_name" -F -
}

create_convenience_tags() {
    local version="$1"
    local codename="$2"
    local status="$3"
    local prerelease="$4"
    
    # Create major.minor tag for latest patch
    local major_minor
    major_minor=$(echo "$version" | cut -d. -f1,2)
    
    if [[ "$status" == "stable" && -z "$prerelease" ]]; then
        log "Creating convenience tag: v$major_minor-latest"
        git tag -f "v$major_minor-latest"
        
        log "Creating convenience tag: latest-$codename"
        git tag -f "latest-$codename"
    fi
    
    # Create channel-specific tags
    if [[ -n "$prerelease" ]]; then
        local channel
        channel=$(echo "$prerelease" | cut -d. -f1)
        
        log "Creating channel tag: $channel-latest-$codename"
        git tag -f "$channel-latest-$codename"
    fi
}

push_tags_to_remote() {
    local primary_tag="$1"
    
    log "Pushing tags to remote repository..."
    
    # Push specific tag
    git push origin "$primary_tag"
    
    # Push all tags (includes convenience tags)
    git push origin --tags
    
    log "Tags pushed to remote"
}

update_tag_registry() {
    local tag_name="$1"
    local version="$2"
    local codename="$3"
    local status="$4"
    
    mkdir -p docs/releases/registry
    
    local registry_file="docs/releases/registry/tags.json"
    
    # Initialize registry if it doesn't exist
    if [[ ! -f "$registry_file" ]]; then
        echo '{"tags": []}' > "$registry_file"
    fi
    
    # Add tag entry
    jq --arg tag "$tag_name" \
       --arg version "$version" \
       --arg codename "$codename" \
       --arg status "$status" \
       --arg date "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
       --arg commit "$(git rev-parse HEAD)" \
       '.tags += [{
         "tag": $tag,
         "version": $version,
         "codename": $codename,
         "status": $status,
         "created": $date,
         "commit": $commit
       }]' "$registry_file" > "$registry_file.tmp"
    
    mv "$registry_file.tmp" "$registry_file"
    
    log "Tag registry updated: $registry_file"
}

# Execute tag creation
create_version_tag "$VERSION" "$CODENAME" "$TAG_TYPE"
