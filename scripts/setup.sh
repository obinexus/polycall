#!/bin/sh
# LibPolyCall Universal Setup Script
# POSIX-compliant shell script for cross-platform installation
# OBINexus Computing - Aegis Project Phase 2
# Author: OBINexus Engineering Team
# Collaboration: Nnamdi Okpala

set -e  # Exit on error

# ================================================================================
# ASCII ART AND CONSTANTS
# ================================================================================

print_banner() {
    cat << 'EOF'
     _     _ _     ____       _        ____      _ _ 
    | |   (_) |__ |  _ \ ___ | |_   _ / ___|__ _| | |
    | |   | | '_ \| |_) / _ \| | | | | |   / _` | | |
    | |___| | |_) |  __/ (_) | | |_| | |__| (_| | | |
    |_____|_|_.__/|_|   \___/|_|\__, |\____\__,_|_|_|
                                |___/                 
    
    LibPolyCall - A Polymorphic Function Call Library
    OBINexus Computing | Aegis Project Phase 2
    Zero-Trust Architecture Implementation
    
EOF
    printf "%s\n" "========================================================================"
}

# Script metadata
SCRIPT_VERSION="2.0.0"
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd 2>/dev/null || echo "$SCRIPT_DIR")"

# Color codes (POSIX-compliant)
if [ -t 1 ]; then
    RED=$(printf '\033[31m')
    GREEN=$(printf '\033[32m')
    YELLOW=$(printf '\033[33m')
    BLUE=$(printf '\033[34m')
    MAGENTA=$(printf '\033[35m')
    CYAN=$(printf '\033[36m')
    RESET=$(printf '\033[0m')
    BOLD=$(printf '\033[1m')
else
    RED="" GREEN="" YELLOW="" BLUE="" MAGENTA="" CYAN="" RESET="" BOLD=""
fi

# ================================================================================
# LOGGING FUNCTIONS
# ================================================================================

log_info() {
    printf "${GREEN}[INFO]${RESET} %s\n" "$1"
}

log_warn() {
    printf "${YELLOW}[WARN]${RESET} %s\n" "$1" >&2
}

log_error() {
    printf "${RED}[ERROR]${RESET} %s\n" "$1" >&2
}

log_debug() {
    if [ "$VERBOSE" = "1" ]; then
        printf "${CYAN}[DEBUG]${RESET} %s\n" "$1"
    fi
}

log_section() {
    printf "\n${BOLD}%s${RESET}\n" "$1"
    printf "%s\n" "------------------------------------------------------------------------"
}

# ================================================================================
# PLATFORM DETECTION
# ================================================================================

detect_os() {
    OS_TYPE="unknown"
    OS_DISTRO="unknown"
    OS_VERSION="unknown"
    OS_ARCH="unknown"
    
    # Detect architecture
    OS_ARCH="$(uname -m 2>/dev/null || echo "unknown")"
    
    # Detect OS type
    case "$(uname -s 2>/dev/null)" in
        Linux*)
            OS_TYPE="linux"
            detect_linux_distro
            ;;
        Darwin*)
            OS_TYPE="macos"
            OS_DISTRO="darwin"
            OS_VERSION="$(sw_vers -productVersion 2>/dev/null || echo "unknown")"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            OS_TYPE="windows"
            detect_windows_env
            ;;
        FreeBSD*)
            OS_TYPE="freebsd"
            OS_VERSION="$(uname -r)"
            ;;
        OpenBSD*)
            OS_TYPE="openbsd"
            OS_VERSION="$(uname -r)"
            ;;
        *)
            OS_TYPE="$(uname -s 2>/dev/null || echo "unknown")"
            ;;
    esac
    
    log_debug "Detected OS: $OS_TYPE/$OS_DISTRO $OS_VERSION ($OS_ARCH)"
}

detect_linux_distro() {
    # Try /etc/os-release first (systemd standard)
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS_DISTRO="${ID:-unknown}"
        OS_VERSION="${VERSION_ID:-unknown}"
    # Try lsb_release
    elif command -v lsb_release >/dev/null 2>&1; then
        OS_DISTRO="$(lsb_release -si 2>/dev/null | tr '[:upper:]' '[:lower:]')"
        OS_VERSION="$(lsb_release -sr 2>/dev/null)"
    # Try specific distro files
    elif [ -f /etc/debian_version ]; then
        OS_DISTRO="debian"
        OS_VERSION="$(cat /etc/debian_version)"
    elif [ -f /etc/redhat-release ]; then
        OS_DISTRO="rhel"
        OS_VERSION="$(grep -oE '[0-9]+\.[0-9]+' /etc/redhat-release | head -1)"
    elif [ -f /etc/alpine-release ]; then
        OS_DISTRO="alpine"
        OS_VERSION="$(cat /etc/alpine-release)"
    fi
}

detect_windows_env() {
    # Detect specific Windows environment
    if [ -n "$MSYSTEM" ]; then
        OS_DISTRO="msys2"
        OS_VERSION="$MSYSTEM"
    elif [ -n "$MINGW_PREFIX" ]; then
        OS_DISTRO="mingw"
        OS_VERSION="$MINGW_PREFIX"
    elif [ -f /proc/version ] && grep -qi microsoft /proc/version; then
        OS_DISTRO="wsl"
        OS_VERSION="$(uname -r)"
    else
        OS_DISTRO="cygwin"
    fi
}

# ================================================================================
# DEPENDENCY CHECKING
# ================================================================================

check_command() {
    command -v "$1" >/dev/null 2>&1
}

check_python() {
    # Check for Python 3
    for py_cmd in python3 python python3.12 python3.11 python3.10 python3.9 python3.8; do
        if check_command "$py_cmd"; then
            # Verify it's Python 3
            if $py_cmd -c "import sys; sys.exit(0 if sys.version_info[0] >= 3 else 1)" 2>/dev/null; then
                PYTHON_CMD="$py_cmd"
                PYTHON_VERSION="$($py_cmd --version 2>&1 | cut -d' ' -f2)"
                log_debug "Found Python: $PYTHON_CMD ($PYTHON_VERSION)"
                return 0
            fi
        fi
    done
    return 1
}

check_dependencies() {
    log_section "Checking Dependencies"
    
    MISSING_DEPS=""
    OPTIONAL_MISSING=""
    
    # Required dependencies
    for dep in git make; do
        if check_command "$dep"; then
            log_info "✓ $dep found"
        else
            log_error "✗ $dep not found"
            MISSING_DEPS="$MISSING_DEPS $dep"
        fi
    done
    
    # Check for Python
    if check_python; then
        log_info "✓ Python 3 found: $PYTHON_CMD ($PYTHON_VERSION)"
    else
        log_error "✗ Python 3 not found"
        MISSING_DEPS="$MISSING_DEPS python3"
    fi
    
    # Check for C compiler
    COMPILER_FOUND=0
    for cc in gcc clang cc; do
        if check_command "$cc"; then
            log_info "✓ C compiler found: $cc"
            CC_CMD="$cc"
            COMPILER_FOUND=1
            break
        fi
    done
    
    if [ "$COMPILER_FOUND" -eq 0 ]; then
        log_error "✗ No C compiler found"
        MISSING_DEPS="$MISSING_DEPS compiler"
    fi
    
    # Optional dependencies
    for dep in cmake curl node cargo; do
        if check_command "$dep"; then
            log_debug "✓ Optional: $dep found"
        else
            log_debug "○ Optional: $dep not found"
            OPTIONAL_MISSING="$OPTIONAL_MISSING $dep"
        fi
    done
    
    if [ -n "$MISSING_DEPS" ]; then
        log_error "Missing required dependencies:$MISSING_DEPS"
        return 1
    fi
    
    return 0
}

# ================================================================================
# SCRIPT DISCOVERY
# ================================================================================

find_scripts() {
    log_section "Discovering Build Scripts"
    
    SCRIPTS_DIR="$PROJECT_ROOT/scripts"
    PYTHON_SCRIPTS=""
    SHELL_SCRIPTS=""
    PS1_SCRIPTS=""
    
    if [ -d "$SCRIPTS_DIR" ]; then
        log_info "Scripts directory: $SCRIPTS_DIR"
        
        # Find Python scripts
        if [ -n "$(find "$SCRIPTS_DIR" -name "*.py" -type f 2>/dev/null)" ]; then
            PYTHON_SCRIPTS=$(find "$SCRIPTS_DIR" -name "*.py" -type f | sort)
            py_count=$(echo "$PYTHON_SCRIPTS" | wc -l)
            log_info "Found $py_count Python scripts"
        fi
        
        # Find shell scripts
        if [ -n "$(find "$SCRIPTS_DIR" -name "*.sh" -type f 2>/dev/null)" ]; then
            SHELL_SCRIPTS=$(find "$SCRIPTS_DIR" -name "*.sh" -type f | sort)
            sh_count=$(echo "$SHELL_SCRIPTS" | wc -l)
            log_info "Found $sh_count shell scripts"
        fi
        
        # Find PowerShell scripts (for Windows environments)
        if [ "$OS_TYPE" = "windows" ]; then
            if [ -n "$(find "$SCRIPTS_DIR" -name "*.ps1" -type f 2>/dev/null)" ]; then
                PS1_SCRIPTS=$(find "$SCRIPTS_DIR" -name "*.ps1" -type f | sort)
                ps1_count=$(echo "$PS1_SCRIPTS" | wc -l)
                log_info "Found $ps1_count PowerShell scripts"
            fi
        fi
    else
        log_warn "Scripts directory not found: $SCRIPTS_DIR"
    fi
}

# ================================================================================
# BUILD SYSTEM DETECTION
# ================================================================================

detect_build_system() {
    log_section "Detecting Build System"
    
    BUILD_SYSTEM="unknown"
    
    # Check for various build files
    if [ -f "$PROJECT_ROOT/Makefile" ]; then
        BUILD_SYSTEM="make"
        log_info "Found Makefile - using Make build system"
    elif [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
        BUILD_SYSTEM="cmake"
        log_info "Found CMakeLists.txt - using CMake build system"
    elif [ -f "$PROJECT_ROOT/meson.build" ]; then
        BUILD_SYSTEM="meson"
        log_info "Found meson.build - using Meson build system"
    elif [ -f "$PROJECT_ROOT/setup.py" ]; then
        BUILD_SYSTEM="setuptools"
        log_info "Found setup.py - using Python setuptools"
    elif [ -f "$PROJECT_ROOT/Cargo.toml" ]; then
        BUILD_SYSTEM="cargo"
        log_info "Found Cargo.toml - using Rust Cargo"
    else
        log_warn "No recognized build system found"
    fi
}

# ================================================================================
# SETUP WIZARD EXECUTION
# ================================================================================

run_setup_wizard() {
    log_section "Running Setup Wizard"
    
    # Look for setup_wizard.py
    WIZARD_SCRIPT=""
    
    # Check common locations
    for location in \
        "$SCRIPT_DIR/setup_wizard.py" \
        "$PROJECT_ROOT/setup_wizard.py" \
        "$PROJECT_ROOT/scripts/setup_wizard.py" \
        "$PROJECT_ROOT/tools/setup_wizard.py"; do
        if [ -f "$location" ]; then
            WIZARD_SCRIPT="$location"
            break
        fi
    done
    
    if [ -z "$WIZARD_SCRIPT" ]; then
        log_warn "setup_wizard.py not found, using basic setup"
        return 1
    fi
    
    log_info "Found setup wizard: $WIZARD_SCRIPT"
    
    # Build wizard arguments
    WIZARD_ARGS=""
    
    if [ "$VERBOSE" = "1" ]; then
        WIZARD_ARGS="$WIZARD_ARGS --verbose"
    fi
    
    if [ "$SKIP_TESTS" = "1" ]; then
        WIZARD_ARGS="$WIZARD_ARGS --skip-tests"
    fi
    
    if [ "$SKIP_BUILD" = "1" ]; then
        WIZARD_ARGS="$WIZARD_ARGS --skip-build"
    fi
    
    if [ "$FORCE" = "1" ]; then
        WIZARD_ARGS="$WIZARD_ARGS --force"
    fi
    
    if [ -n "$INIT_PROJECT" ]; then
        WIZARD_ARGS="$WIZARD_ARGS --init-project $INIT_PROJECT"
    fi
    
    # Execute the wizard
    log_info "Executing: $PYTHON_CMD $WIZARD_SCRIPT $WIZARD_ARGS"
    
    if $PYTHON_CMD "$WIZARD_SCRIPT" $WIZARD_ARGS; then
        log_info "Setup wizard completed successfully"
        return 0
    else
        log_error "Setup wizard failed"
        return 1
    fi
}

# ================================================================================
# FIX SCRIPTS EXECUTION
# ================================================================================

run_fix_scripts() {
    log_section "Running Fix Scripts"
    
    # Priority order for fix scripts
    FIX_SCRIPT_PATTERNS="
        fix_all_*.py
        fix_include*.py
        fix_*paths*.py
        standardize_*.py
        validate_*.py
    "
    
    for pattern in $FIX_SCRIPT_PATTERNS; do
        for script in $(find "$SCRIPTS_DIR" -name "$pattern" -type f 2>/dev/null | sort); do
            if [ -f "$script" ] && [ -x "$script" -o -n "$PYTHON_CMD" ]; then
                script_name=$(basename "$script")
                log_info "Running: $script_name"
                
                if [ "${script##*.}" = "py" ] && [ -n "$PYTHON_CMD" ]; then
                    if $PYTHON_CMD "$script"; then
                        log_info "✓ $script_name completed"
                    else
                        log_warn "⚠ $script_name failed (continuing)"
                    fi
                elif [ "${script##*.}" = "sh" ]; then
                    if sh "$script"; then
                        log_info "✓ $script_name completed"
                    else
                        log_warn "⚠ $script_name failed (continuing)"
                    fi
                fi
            fi
        done
    done
}

# ================================================================================
# BUILD EXECUTION
# ================================================================================

run_build() {
    log_section "Building LibPolyCall"
    
    case "$BUILD_SYSTEM" in
        make)
            log_info "Running make build"
            if [ -f "$PROJECT_ROOT/Makefile" ]; then
                cd "$PROJECT_ROOT"
                
                # Clean build
                if [ "$CLEAN_BUILD" = "1" ]; then
                    log_info "Cleaning previous build..."
                    make clean || log_warn "Clean failed"
                fi
                
                # Run make targets
                for target in all install-dev; do
                    log_info "Running: make $target"
                    if make "$target"; then
                        log_info "✓ make $target succeeded"
                    else
                        log_error "✗ make $target failed"
                        [ "$FORCE" != "1" ] && return 1
                    fi
                done
            fi
            ;;
            
        cmake)
            log_info "Running CMake build"
            if check_command cmake; then
                mkdir -p "$PROJECT_ROOT/build"
                cd "$PROJECT_ROOT/build"
                
                log_info "Configuring with CMake..."
                if cmake ..; then
                    log_info "Building with CMake..."
                    cmake --build .
                else
                    log_error "CMake configuration failed"
                    return 1
                fi
            else
                log_error "CMake not found"
                return 1
            fi
            ;;
            
        setuptools)
            log_info "Running Python setuptools build"
            cd "$PROJECT_ROOT"
            if $PYTHON_CMD setup.py build; then
                log_info "✓ Python build succeeded"
            else
                log_error "✗ Python build failed"
                return 1
            fi
            ;;
            
        *)
            log_warn "No automated build available for: $BUILD_SYSTEM"
            ;;
    esac
}

# ================================================================================
# CONFIGURATION GENERATION
# ================================================================================

generate_config() {
    log_section "Generating Configuration"
    
    CONFIG_FILE="$PROJECT_ROOT/.polycallrc"
    
    cat > "$CONFIG_FILE" << EOF
{
    "version": "$SCRIPT_VERSION",
    "setup_date": "$(date -u +"%Y-%m-%d %H:%M:%S UTC")",
    "platform": {
        "os_type": "$OS_TYPE",
        "os_distro": "$OS_DISTRO",
        "os_version": "$OS_VERSION",
        "architecture": "$OS_ARCH"
    },
    "build_system": "$BUILD_SYSTEM",
    "python_command": "$PYTHON_CMD",
    "python_version": "$PYTHON_VERSION",
    "compiler": "${CC_CMD:-unknown}",
    "aegis_phase": 2
}
EOF
    
    log_info "Configuration written to: $CONFIG_FILE"
}

# ================================================================================
# MAIN EXECUTION
# ================================================================================

show_usage() {
    cat << EOF
Usage: $SCRIPT_NAME [OPTIONS]

LibPolyCall Setup Script - OBINexus Aegis Project

OPTIONS:
    -h, --help          Show this help message
    -v, --verbose       Enable verbose output
    -f, --force         Continue on errors
    -c, --clean         Clean build before compiling
    --skip-tests        Skip running tests
    --skip-build        Skip build step
    --skip-wizard       Skip Python setup wizard
    --init-project NAME Initialize project with given name
    --dry-run          Check dependencies only

EXAMPLES:
    # Standard installation
    ./$SCRIPT_NAME
    
    # Quick setup without tests
    ./$SCRIPT_NAME --skip-tests
    
    # Initialize specific project
    ./$SCRIPT_NAME --init-project aegis-demo
    
    # Verbose mode with clean build
    ./$SCRIPT_NAME -v -c

EOF
}

parse_arguments() {
    VERBOSE=0
    FORCE=0
    CLEAN_BUILD=0
    SKIP_TESTS=0
    SKIP_BUILD=0
    SKIP_WIZARD=0
    DRY_RUN=0
    INIT_PROJECT=""
    
    while [ $# -gt 0 ]; do
        case "$1" in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=1
                ;;
            -f|--force)
                FORCE=1
                ;;
            -c|--clean)
                CLEAN_BUILD=1
                ;;
            --skip-tests)
                SKIP_TESTS=1
                ;;
            --skip-build)
                SKIP_BUILD=1
                ;;
            --skip-wizard)
                SKIP_WIZARD=1
                ;;
            --dry-run)
                DRY_RUN=1
                SKIP_BUILD=1
                SKIP_WIZARD=1
                ;;
            --init-project)
                shift
                INIT_PROJECT="$1"
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
        shift
    done
}

main() {
    # Parse command line arguments
    parse_arguments "$@"
    
    # Print banner
    print_banner
    
    # Start setup process
    log_info "Starting LibPolyCall setup (v$SCRIPT_VERSION)"
    log_info "Project root: $PROJECT_ROOT"
    
    # Detect platform
    detect_os
    
    # Check dependencies
    if ! check_dependencies; then
        if [ "$FORCE" != "1" ]; then
            log_error "Missing dependencies. Install them or use --force to continue"
            exit 1
        fi
    fi
    
    # Find available scripts
    find_scripts
    
    # Detect build system
    detect_build_system
    
    # Exit if dry run
    if [ "$DRY_RUN" = "1" ]; then
        log_info "Dry run complete. No changes made."
        exit 0
    fi
    
    # Run setup wizard if available and not skipped
    if [ "$SKIP_WIZARD" != "1" ] && [ -n "$PYTHON_CMD" ]; then
        if ! run_setup_wizard; then
            log_warn "Setup wizard failed, continuing with basic setup"
        fi
    fi
    
    # Run fix scripts if found
    if [ -n "$PYTHON_SCRIPTS" ] && [ -n "$PYTHON_CMD" ]; then
        run_fix_scripts
    fi
    
    # Build project if not skipped
    if [ "$SKIP_BUILD" != "1" ]; then
        if ! run_build; then
            if [ "$FORCE" != "1" ]; then
                log_error "Build failed"
                exit 1
            fi
        fi
    fi
    
    # Generate configuration
    generate_config
    
    # Final summary
    log_section "Setup Complete"
    log_info "LibPolyCall setup completed successfully!"
    log_info "Configuration saved to: $PROJECT_ROOT/.polycallrc"
    
    if [ -n "$OPTIONAL_MISSING" ]; then
        log_info ""
        log_info "Optional tools not found:$OPTIONAL_MISSING"
        log_info "Consider installing them for full functionality"
    fi
    
    log_info ""
    log_info "Welcome to the polymorphic paradigm!"
    log_info "OBINexus Computing - Aegis Project Phase 2"
    
    exit 0
}

# Execute main function
main "$@"
