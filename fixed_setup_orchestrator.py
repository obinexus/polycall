#!/usr/bin/env python3
"""
PolyCall Setup Orchestrator - Fixed Version
Implements complete Biafran color palette with proper path resolution
"""

import os
import sys
import json
import shutil
import platform
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Tuple

class BiafranColors:
    """Biafran flag color palette for terminal output - Complete Implementation"""
    # Based on Biafran flag colors
    RED = '\033[91m'      # Top stripe
    BLACK = '\033[90m'    # Middle stripe  
    GREEN = '\033[92m'    # Bottom stripe
    YELLOW = '\033[93m'   # Rising sun
    
    # Additional colors for UI
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    RESET = '\033[0m'
    
    @classmethod
    def banner(cls, text: str) -> str:
        """Create a Biafran-themed banner"""
        lines = []
        lines.append(f"{cls.RED}{'═' * 60}{cls.RESET}")
        lines.append(f"{cls.BLACK}║{cls.YELLOW} ☀ {cls.BOLD}{text.center(54)}{cls.RESET}{cls.BLACK}║{cls.RESET}")
        lines.append(f"{cls.GREEN}{'═' * 60}{cls.RESET}")
        return '\n'.join(lines)
    
    @classmethod
    def success(cls, text: str) -> str:
        return f"{cls.GREEN}{cls.BOLD}✓ {text}{cls.RESET}"
    
    @classmethod
    def error(cls, text: str) -> str:
        return f"{cls.RED}{cls.BOLD}✗ {text}{cls.RESET}"
    
    @classmethod
    def warning(cls, text: str) -> str:
        return f"{cls.YELLOW}{cls.BOLD}⚠ {text}{cls.RESET}"
    
    @classmethod
    def info(cls, text: str) -> str:
        return f"{cls.BLACK}{cls.BOLD}ℹ {text}{cls.RESET}"
    
    @classmethod
    def yellow(cls, text: str) -> str:
        """Yellow text method - was missing in original implementation"""
        return f"{cls.YELLOW}{text}{cls.RESET}"
    
    @classmethod
    def bold(cls, text: str) -> str:
        """Bold text wrapper"""
        return f"{cls.BOLD}{text}{cls.RESET}"

class PolyCallSetup:
    def __init__(self):
        # Fix path resolution - go up two levels from scripts/hoc to project root
        script_dir = Path(__file__).parent
        self.root_dir = script_dir.parent.parent  # Go up to project root
        
        # Ensure we're in the correct directory
        os.chdir(self.root_dir)
        
        self.isolated_dir = self.root_dir / "(isolated)"
        self.scripts_dir = self.root_dir / "scripts"
        self.setup_dir = self.scripts_dir / "setup"
        self.platform_name = platform.system().lower()
        self.colors = BiafranColors()
        
    def print_banner(self):
        """Display the setup banner"""
        print(self.colors.banner("POLYCALL SETUP ORCHESTRATOR"))
        print(self.colors.info(f"Platform: {self.platform_name}"))
        print(self.colors.info(f"Root: {self.root_dir}"))
        print()
        
    def cleanup_isolated_directory(self):
        """Clean up the isolated directory, preserving only essential files"""
        print(self.colors.warning("Cleaning up isolated directory..."))
        
        essential_patterns = [
            "fix_violations_report.json",
            "ISOLATION_LOG.md",
            "RECOVERY_REPORT.md",
            "*_ISOLATED.c",
            "*_ISOLATED.h"
        ]
        
        if self.isolated_dir.exists():
            # Create archive directory
            archive_dir = self.isolated_dir / "archive" / datetime.now().strftime("%Y%m%d_%H%M%S")
            archive_dir.mkdir(parents=True, exist_ok=True)
            
            # Move non-essential files to archive
            archived_count = 0
            for item in self.isolated_dir.rglob("*"):
                if item.is_file():
                    is_essential = any(item.match(pattern) for pattern in essential_patterns)
                    if not is_essential and item.parent != archive_dir:
                        dest = archive_dir / item.relative_to(self.isolated_dir)
                        dest.parent.mkdir(parents=True, exist_ok=True)
                        try:
                            shutil.move(str(item), str(dest))
                            archived_count += 1
                        except Exception as e:
                            print(self.colors.warning(f"Could not move {item.name}: {e}"))
            
            print(self.colors.success(f"Archived {archived_count} files"))
    
    def organize_scripts(self):
        """Organize scripts into proper directory structure"""
        print(self.colors.warning("Organizing scripts..."))
        
        # Create setup directory structure
        self.setup_dir.mkdir(parents=True, exist_ok=True)
        
        platforms = {
            'windows': ['ps1', 'bat', 'cmd'],
            'linux': ['sh', 'bash'],
            'darwin': ['sh', 'bash'],
            'posix': ['sh']
        }
        
        # Create platform-specific directories
        for platform_name in platforms:
            platform_dir = self.setup_dir / platform_name
            platform_dir.mkdir(exist_ok=True)
        
        print(self.colors.success("Script directories organized"))
    
    def generate_platform_scripts(self):
        """Generate platform-specific setup scripts"""
        print(self.colors.warning("Generating platform setup scripts..."))
        
        # Windows PowerShell script
        self._generate_windows_script()
        
        # Linux/Mac bash script
        self._generate_unix_script()
        
        # POSIX-compliant script
        self._generate_posix_script()
        
        print(self.colors.success("Platform scripts generated"))
    
    def _generate_windows_script(self):
        """Generate Windows PowerShell setup script"""
        script_path = self.setup_dir / "windows" / "setup.ps1"
        
        content = '''# PolyCall Windows Setup Script
# Implements Biafran color scheme

$Host.UI.RawUI.BackgroundColor = "Black"
Clear-Host

function Write-BiafranBanner {
    param($Text)
    Write-Host ("=" * 60) -ForegroundColor Red
    Write-Host "║" -NoNewline -ForegroundColor DarkGray
    Write-Host " ☀ " -NoNewline -ForegroundColor Yellow
    Write-Host $Text.PadLeft(27).PadRight(54) -NoNewline -ForegroundColor White
    Write-Host "║" -ForegroundColor DarkGray
    Write-Host ("=" * 60) -ForegroundColor Green
}

Write-BiafranBanner "POLYCALL SETUP - WINDOWS"

# Check prerequisites
Write-Host "`n[CHECK] Verifying prerequisites..." -ForegroundColor Yellow

# Check for MinGW/MSYS2
if (!(Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] GCC not found. Please install MinGW or MSYS2" -ForegroundColor Red
    Write-Host "Download from: https://www.msys2.org/" -ForegroundColor Cyan
    exit 1
}

# Check for CMake
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] CMake not found. Please install CMake" -ForegroundColor Red
    Write-Host "Download from: https://cmake.org/download/" -ForegroundColor Cyan
    exit 1
}

Write-Host "[OK] Prerequisites verified" -ForegroundColor Green

# Create build directory
$BuildDir = "build"
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Write-Host "`n[BUILD] Configuring CMake..." -ForegroundColor Yellow
Push-Location $BuildDir

try {
    cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    Write-Host "`n[BUILD] Compiling PolyCall..." -ForegroundColor Yellow
    mingw32-make -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed"
    }
    
    Write-Host "`n[SUCCESS] PolyCall built successfully!" -ForegroundColor Green
    Write-Host "Executable location: $BuildDir\bin\polycall.exe" -ForegroundColor Cyan
}
catch {
    Write-Host "[ERROR] $_" -ForegroundColor Red
    exit 1
}
finally {
    Pop-Location
}

# Add to PATH suggestion
Write-Host "`n[TIP] To add PolyCall to PATH, run:" -ForegroundColor Yellow
Write-Host '$env:Path += ";' + (Get-Location).Path + '\build\bin"' -ForegroundColor Cyan
'''
        
        script_path.write_text(content)
        print(self.colors.success(f"Generated: {script_path}"))
    
    def _generate_unix_script(self):
        """Generate Unix/Linux/Mac setup script"""
        script_path = self.setup_dir / "linux" / "setup.sh"
        
        content = '''#!/bin/bash
# PolyCall Unix/Linux/Mac Setup Script
# Implements Biafran color scheme

# Biafran colors
RED='\\033[91m'
BLACK='\\033[90m'
GREEN='\\033[92m'
YELLOW='\\033[93m'
BOLD='\\033[1m'
RESET='\\033[0m'

biafran_banner() {
    echo -e "${RED}════════════════════════════════════════════════════════════${RESET}"
    echo -e "${BLACK}║${YELLOW} ☀ ${BOLD}$1${RESET}"
    echo -e "${GREEN}════════════════════════════════════════════════════════════${RESET}"
}

biafran_banner "POLYCALL SETUP - UNIX/LINUX/MAC"

# Detect OS
OS=$(uname -s)
echo -e "${YELLOW}[DETECT]${RESET} Operating System: $OS"

# Detect package manager
if command -v apt-get &> /dev/null; then
    PKG_MGR="apt-get"
    PKG_INSTALL="sudo apt-get install -y"
elif command -v yum &> /dev/null; then
    PKG_MGR="yum"
    PKG_INSTALL="sudo yum install -y"
elif command -v brew &> /dev/null; then
    PKG_MGR="brew"
    PKG_INSTALL="brew install"
else
    PKG_MGR="unknown"
fi

echo -e "${YELLOW}[DETECT]${RESET} Package Manager: $PKG_MGR"

# Check prerequisites
echo -e "\\n${YELLOW}[CHECK]${RESET} Verifying prerequisites..."

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}[ERROR]${RESET} $1 not found."
        if [ "$PKG_MGR" != "unknown" ]; then
            echo -e "${YELLOW}[TIP]${RESET} Try: $PKG_INSTALL $2"
        fi
        return 1
    fi
    return 0
}

# Check all required tools
MISSING_DEPS=0
check_command gcc "build-essential" || MISSING_DEPS=1
check_command cmake "cmake" || MISSING_DEPS=1
check_command make "make" || MISSING_DEPS=1

if [ $MISSING_DEPS -eq 1 ]; then
    echo -e "${RED}[ERROR]${RESET} Missing dependencies. Please install them and try again."
    exit 1
fi

echo -e "${GREEN}[OK]${RESET} All prerequisites verified"

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

echo -e "\\n${YELLOW}[BUILD]${RESET} Configuring CMake..."
cd "$BUILD_DIR"

if cmake .. -DCMAKE_BUILD_TYPE=Release; then
    echo -e "${GREEN}[OK]${RESET} CMake configured successfully"
else
    echo -e "${RED}[ERROR]${RESET} CMake configuration failed"
    exit 1
fi

echo -e "\\n${YELLOW}[BUILD]${RESET} Compiling PolyCall..."
if make -j$(nproc 2>/dev/null || echo 4); then
    echo -e "${GREEN}[OK]${RESET} Compilation successful"
else
    echo -e "${RED}[ERROR]${RESET} Compilation failed"
    exit 1
fi

cd ..

echo -e "\\n${GREEN}[SUCCESS]${RESET} PolyCall built successfully!"
echo -e "${YELLOW}[INFO]${RESET} Executable location: $BUILD_DIR/bin/polycall"

# Install hooks
echo -e "\\n${YELLOW}[HOOKS]${RESET} Installing git hooks..."
if [ -d ".git" ]; then
    if [ -d "scripts/hooks" ]; then
        cp scripts/hooks/* .git/hooks/ 2>/dev/null && chmod +x .git/hooks/* 2>/dev/null
        echo -e "${GREEN}[OK]${RESET} Git hooks installed"
    else
        echo -e "${YELLOW}[SKIP]${RESET} No hooks directory found"
    fi
else
    echo -e "${YELLOW}[SKIP]${RESET} Not a git repository"
fi

# Installation suggestion
echo -e "\\n${YELLOW}[TIP]${RESET} To install system-wide, run:"
echo -e "  ${BOLD}sudo make install${RESET}"
echo -e "\\n${YELLOW}[TIP]${RESET} To add to PATH for current session:"
echo -e "  ${BOLD}export PATH=\\$PATH:$(pwd)/$BUILD_DIR/bin${RESET}"
'''
        
        script_path.write_text(content)
        script_path.chmod(0o755)
        print(self.colors.success(f"Generated: {script_path}"))
    
    def _generate_posix_script(self):
        """Generate POSIX-compliant setup script"""
        script_path = self.setup_dir / "posix" / "setup.sh"
        
        content = '''#!/bin/sh
# PolyCall POSIX-compliant Setup Script
# Minimal dependencies, maximum compatibility

# POSIX color codes (may not work on all terminals)
if [ -t 1 ]; then
    RED=$(printf '\\033[31m')
    GREEN=$(printf '\\033[32m')
    YELLOW=$(printf '\\033[33m')
    RESET=$(printf '\\033[0m')
else
    RED=''
    GREEN=''
    YELLOW=''
    RESET=''
fi

echo "${RED}════════════════════════════════════════════════════════════${RESET}"
echo "                    POLYCALL SETUP - POSIX                    "
echo "${GREEN}════════════════════════════════════════════════════════════${RESET}"

# Check for required commands
echo ""
echo "${YELLOW}[CHECK]${RESET} Verifying prerequisites..."

MISSING=0
for cmd in gcc cmake make; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "${RED}[ERROR]${RESET} $cmd not found"
        MISSING=1
    else
        echo "${GREEN}[OK]${RESET} $cmd found"
    fi
done

if [ $MISSING -eq 1 ]; then
    echo ""
    echo "${RED}[ERROR]${RESET} Missing required tools. Please install them."
    exit 1
fi

echo ""
echo "${GREEN}[OK]${RESET} All prerequisites verified"

# Build
echo ""
echo "${YELLOW}[BUILD]${RESET} Creating build directory..."
if [ ! -d "build" ]; then
    mkdir build || exit 1
fi

cd build || exit 1

echo "${YELLOW}[BUILD]${RESET} Configuring..."
if cmake ..; then
    echo "${GREEN}[OK]${RESET} Configuration successful"
else
    echo "${RED}[ERROR]${RESET} Configuration failed"
    exit 1
fi

echo ""
echo "${YELLOW}[BUILD]${RESET} Compiling..."
if make; then
    echo "${GREEN}[OK]${RESET} Compilation successful"
else
    echo "${RED}[ERROR]${RESET} Compilation failed"
    exit 1
fi

cd ..

echo ""
echo "${GREEN}[SUCCESS]${RESET} PolyCall built successfully!"
echo "${YELLOW}[INFO]${RESET} Executable: build/bin/polycall"
'''
        
        script_path.write_text(content)
        script_path.chmod(0o755)
        print(self.colors.success(f"Generated: {script_path}"))
    
    def fix_violations(self):
        """Apply fixes from fix_violations_report.json"""
        print(self.colors.warning("Analyzing violation fixes..."))
        
        report_path = self.root_dir / "fix_violations_report.json"
        if not report_path.exists():
            print(self.colors.warning("No fix_violations_report.json found"))
            return
        
        try:
            with open(report_path) as f:
                report = json.load(f)
            
            # Display recommendations
            recommendations = report.get("recommendations", [])
            if recommendations:
                print(self.colors.info("Recommendations from migration enforcer:"))
                for i, rec in enumerate(recommendations, 1):
                    print(f"  {i}. {rec}")
            
            # Check if all fixes were applied
            fixes = report.get("fixes_applied", [])
            if fixes:
                print(self.colors.success(f"Previously applied {len(fixes)} fixes"))
            
        except Exception as e:
            print(self.colors.error(f"Error reading violations report: {e}"))
    
    def generate_qa_compliance_script(self):
        """Generate QA compliance verification script"""
        script_path = self.scripts_dir / "qa_compliance.py"
        
        content = '''#!/usr/bin/env python3
"""QA Compliance Verification Script"""

import os
import json
from pathlib import Path
from datetime import datetime

def verify_compliance():
    """Verify all modules comply with QA standards"""
    results = {
        "timestamp": datetime.now().isoformat(),
        "modules": {},
        "compliance": True
    }
    
    # Check each module
    modules = ["core", "cli", "auth", "network", "edge", "micro", "telemetry"]
    
    for module in modules:
        module_path = Path("src") / module
        if module_path.exists():
            # Check for test coverage
            test_path = Path("tests/unit") / module
            has_tests = test_path.exists()
            
            # Check for documentation
            doc_path = Path("docs") / module
            has_docs = doc_path.exists()
            
            # Count source files
            source_files = list(module_path.rglob("*.c"))
            
            results["modules"][module] = {
                "has_tests": has_tests,
                "has_docs": has_docs,
                "source_files": len(source_files),
                "compliant": has_tests and has_docs
            }
            
            if not (has_tests and has_docs):
                results["compliance"] = False
    
    # Write report
    with open("qa_compliance_report.json", "w") as f:
        json.dump(results, f, indent=2)
    
    # Print summary
    print("QA Compliance Report")
    print("=" * 50)
    print(f"Timestamp: {results['timestamp']}")
    print(f"Overall Compliance: {'PASSED' if results['compliance'] else 'FAILED'}")
    print("")
    print("Module Status:")
    print("-" * 50)
    
    for module, status in results["modules"].items():
        symbol = "✓" if status["compliant"] else "✗"
        print(f"{symbol} {module:<12} Tests: {str(status['has_tests']):<6} "
              f"Docs: {str(status['has_docs']):<6} "
              f"Files: {status['source_files']}")
    
    return results["compliance"]

if __name__ == "__main__":
    if verify_compliance():
        print("\\n✓ Overall QA Compliance: PASSED")
        exit(0)
    else:
        print("\\n✗ Overall QA Compliance: FAILED")
        exit(1)
'''
        
        try:
            script_path.write_text(content)
            script_path.chmod(0o755)
            print(self.colors.success(f"Generated: {script_path}"))
        except Exception as e:
            print(self.colors.error(f"Failed to create QA script: {e}"))
    
    def run(self):
        """Execute the setup orchestration"""
        self.print_banner()
        
        try:
            # Phase 1: Cleanup
            print(self.colors.yellow("\n═══ PHASE 1: CLEANUP ═══"))
            self.cleanup_isolated_directory()
            
            # Phase 2: Organization
            print(self.colors.yellow("\n═══ PHASE 2: ORGANIZATION ═══"))
            self.organize_scripts()
            
            # Phase 3: Generation
            print(self.colors.yellow("\n═══ PHASE 3: GENERATION ═══"))
            self.generate_platform_scripts()
            self.generate_qa_compliance_script()
            
            # Phase 4: Fixes
            print(self.colors.yellow("\n═══ PHASE 4: FIXES ═══"))
            self.fix_violations()
            
            # Summary
            print(self.colors.banner("SETUP COMPLETE"))
            print(self.colors.success("\nNext steps:"))
            print(f"  1. Run: {self.colors.bold}./scripts/setup/{self.platform_name}/setup.sh{self.colors.reset}")
            print(f"  2. Test: {self.colors.bold}make test{self.colors.reset}")
            print(f"  3. Verify: {self.colors.bold}python3 scripts/qa_compliance.py{self.colors.reset}")
            
            print(self.colors.info(f"\nSetup files created in: {self.setup_dir}"))
            print(self.colors.info("Platform scripts available for: Windows, Linux, Darwin, POSIX"))
            
        except Exception as e:
            print(self.colors.error(f"\nSetup failed: {e}"))
            import traceback
            traceback.print_exc()
            return 1
        
        return 0

if __name__ == "__main__":
    setup = PolyCallSetup()
    sys.exit(setup.run())
