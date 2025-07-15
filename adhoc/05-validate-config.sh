#!/bin/bash
# 05-validate-config.sh - Lints .polycall.cfg and warns of key drift
set -euo pipefail

VERBOSE=false
DRY_RUN=false
STRICT=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose) VERBOSE=true; shift ;;
        --dry-run) DRY_RUN=true; shift ;;
        --strict) STRICT=true; shift ;;
        *) shift ;;
    esac
done

declare -a CONFIG_FILES=(
    ".polycall.cfg"
    "polycall.cfg"
    "config/polycall.cfg"
    "polycallfile"
    ".polycallfile"
)

log() { [[ "$VERBOSE" == "true" ]] && echo "[VALIDATE-CONFIG] $*" >&2 || true; }
warn() { echo "[VALIDATE-CONFIG] WARNING: $*" >&2; }
error() { echo "[VALIDATE-CONFIG] ERROR: $*" >&2; }

validate_config_syntax() {
    local config_file="$1"
    local errors=0
    
    log "Validating syntax of: $config_file"
    
    # Check for basic syntax issues
    while IFS= read -r line_num; do
        local line content
        line=$(sed -n "${line_num}p" "$config_file")
        content=$(echo "$line" | sed 's/^[[:space:]]*//' | sed 's/[[:space:]]*$//')
        
        # Skip comments and empty lines
        [[ "$content" =~ ^#.*$ ]] || [[ -z "$content" ]] && continue
        
        # Check for key=value format
        if [[ ! "$content" =~ ^[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*=[[:space:]]*.*$ ]]; then
            error "Line $line_num: Invalid syntax: $line"
            ((errors++))
        fi
        
        # Check for duplicate keys
        local key
        key=$(echo "$content" | cut -d= -f1 | sed 's/[[:space:]]*//')
        local key_count
        key_count=$(grep -n "^[[:space:]]*$key[[:space:]]*=" "$config_file" | wc -l)
        
        if [[ $key_count -gt 1 ]]; then
            warn "Key '$key' appears $key_count times"
        fi
        
    done < <(grep -n . "$config_file")
    
    return $errors
}

check_required_keys() {
    local config_file="$1"
    local missing_keys=()
    
    declare -a REQUIRED_KEYS=(
        "POLYCALL_VERSION"
        "POLYCALL_HOME"
        "POLYCALL_LOG_LEVEL"
        "POLYCALL_CACHE_DIR"
        "POLYCALL_CONFIG_FORMAT"
    )
    
    for key in "${REQUIRED_KEYS[@]}"; do
        if ! grep -q "^[[:space:]]*$key[[:space:]]*=" "$config_file"; then
            missing_keys+=("$key")
        fi
    done
    
    if [[ ${#missing_keys[@]} -gt 0 ]]; then
        error "Missing required keys: ${missing_keys[*]}"
        return 1
    fi
    
    return 0
}

check_deprecated_keys() {
    local config_file="$1"
    
    declare -a DEPRECATED_KEYS=(
        "POLYCALL_V1_COMPAT"
        "LEGACY_FFI_MODE"
        "OLD_CACHE_PATH"
        "DEPRECATED_SSL_MODE"
    )
    
    for key in "${DEPRECATED_KEYS[@]}"; do
        if grep -q "^[[:space:]]*$key[[:space:]]*=" "$config_file"; then
            warn "Deprecated key found: $key"
        fi
    done
}

main() {
    log "Starting configuration validation..."
    
    local config_found=false
    local total_errors=0
    
    for config_file in "${CONFIG_FILES[@]}"; do
        if [[ -f "$config_file" ]]; then
            config_found=true
            log "Found config file: $config_file"
            
            if [[ "$DRY_RUN" == "true" ]]; then
                echo "[DRY-RUN] Would validate: $config_file"
                continue
            fi
            
            local file_errors=0
            
            # Syntax validation
            validate_config_syntax "$config_file" || file_errors=$?
            
            # Required keys check
            check_required_keys "$config_file" || ((file_errors++))
            
            # Deprecated keys check
            check_deprecated_keys "$config_file"
            
            total_errors=$((total_errors + file_errors))
            
            if [[ $file_errors -eq 0 ]]; then
                log "✅ $config_file validation passed"
            else
                error "❌ $config_file has $file_errors errors"
            fi
        fi
    done
    
    if [[ "$config_found" == "false" ]]; then
        warn "No configuration files found"
        [[ "$STRICT" == "true" ]] && return 1
    fi
    
    if [[ $total_errors -gt 0 ]]; then
        error "Configuration validation failed with $total_errors errors"
        return 1
    fi
    
    log "Configuration validation completed successfully"
    return 0
}

main "$@"
