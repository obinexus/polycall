#!/usr/bin/env python3
"""
PolyCall Setup Orchestrator - Unified cross-platform setup system
Implements Biafran color palette and systematic project organization
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
    """Biafran flag color palette for terminal output"""
    # Based on Biafran flag colors
    RED = '\033[91m'      # Top stripe
    BLACK = '\033[90m'    # Middle stripe with rising sun
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

class PolyCallSetup:
    def __init__(self):
        self.root_dir = Path.cwd()
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
            for item in self.isolated_dir.rglob("*"):
                if item.is_file():
                    is_essential = any(item.match(pattern) for pattern in essential_patterns)
                    if not is_essential:
                        dest = archive_dir / item.relative_to(self.isolated_dir)
                        dest.parent.mkdir(parents=True, exist_ok=True)
                        shutil.move(str(item), str(dest))
            
            print(self.colors.success(f"Archived {len(list(archive_dir.rglob('*')))} files"))
    
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
    Write-Host "║" -NoNewline -ForegroundColor Black
    Write-Host " ☀ " -NoNewline -ForegroundColor Yellow
    Write-Host $Text.PadLeft(27).PadRight(54) -NoNewline -ForegroundColor White
    Write-Host "║" -ForegroundColor Black
    Write-Host ("=" * 60) -ForegroundColor Green
}

Write-BiafranBanner "POLYCALL SETUP - WINDOWS"

# Check prerequisites
Write-Host "`n[CHECK] Verifying prerequisites..." -ForegroundColor Yellow

# Check for MinGW/MSYS2
if (!(Get-Command gcc -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] GCC not found. Please install MinGW or MSYS2" -ForegroundColor Red
    exit 1
}

# Check for CMake
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "[ERROR] CMake not found. Please install CMake" -ForegroundColor Red
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
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "`n[BUILD] Compiling PolyCall..." -ForegroundColor Yellow
mingw32-make -j4
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Compilation failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location
Write-Host "`n[SUCCESS] PolyCall built successfully!" -ForegroundColor Green
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
    echo -e "${RED}${'='*60}${RESET}"
    echo -e "${BLACK}║${YELLOW} ☀ ${BOLD}$1${RESET}${BLACK}║${RESET}"
    echo -e "${GREEN}${'='*60}${RESET}"
}

biafran_banner "POLYCALL SETUP - UNIX/LINUX/MAC"

# Detect OS
OS=$(uname -s)
echo -e "${YELLOW}[DETECT]${RESET} Operating System: $OS"

# Check prerequisites
echo -e "\\n${YELLOW}[CHECK]${RESET} Verifying prerequisites..."

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}[ERROR]${RESET} $1 not found. Please install $1"
        exit 1
    fi
}

check_command gcc
check_command cmake
check_command make

echo -e "${GREEN}[OK]${RESET} Prerequisites verified"

# Fix header issues
echo -e "\\n${YELLOW}[FIX]${RESET} Applying emergency header fixes..."
if [ -f "scripts/adhoc/emergency_header_fix.sh" ]; then
    bash scripts/adhoc/emergency_header_fix.sh
fi

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

echo -e "\\n${YELLOW}[BUILD]${RESET} Configuring CMake..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${RESET} CMake configuration failed"
    exit 1
fi

echo -e "\\n${YELLOW}[BUILD]${RESET} Compiling PolyCall..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${RESET} Compilation failed"
    exit 1
fi

cd ..
echo -e "\\n${GREEN}[SUCCESS]${RESET} PolyCall built successfully!"

# Install hooks
echo -e "\\n${YELLOW}[HOOKS]${RESET} Installing git hooks..."
if [ -d ".git" ]; then
    cp scripts/hooks/* .git/hooks/ 2>/dev/null || true
    chmod +x .git/hooks/* 2>/dev/null || true
    echo -e "${GREEN}[OK]${RESET} Git hooks installed"
fi
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

echo "${RED}============================================================${RESET}"
echo "                    POLYCALL SETUP - POSIX                    "
echo "${GREEN}============================================================${RESET}"

# Check for required commands
echo ""
echo "${YELLOW}[CHECK]${RESET} Verifying prerequisites..."

for cmd in gcc cmake make; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "${RED}[ERROR]${RESET} $cmd not found"
        exit 1
    fi
done

echo "${GREEN}[OK]${RESET} Prerequisites verified"

# Build
if [ ! -d "build" ]; then
    mkdir build
fi

cd build || exit 1
cmake .. || exit 1
make || exit 1
cd ..

echo ""
echo "${GREEN}[SUCCESS]${RESET} PolyCall built successfully!"
'''
        
        script_path.write_text(content)
        script_path.chmod(0o755)
        print(self.colors.success(f"Generated: {script_path}"))
    
    def fix_violations(self):
        """Apply fixes from fix_violations_report.json"""
        print(self.colors.warning("Applying violation fixes..."))
        
        report_path = self.root_dir / "fix_violations_report.json"
        if not report_path.exists():
            print(self.colors.error("fix_violations_report.json not found"))
            return
        
        with open(report_path) as f:
            report = json.load(f)
        
        # Apply recommendations
        for rec in report.get("recommendations", []):
            print(self.colors.info(f"Recommendation: {rec}"))
        
        print(self.colors.success("Violation fixes reviewed"))
    
    def generate_qa_compliance_script(self):
        """Generate QA compliance verification script"""
        script_path = self.scripts_dir / "qa_compliance.py"
        
        content = '''#!/usr/bin/env python3
"""QA Compliance Verification Script"""

import os
import json
from pathlib import Path

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
            
            results["modules"][module] = {
                "has_tests": has_tests,
                "has_docs": has_docs,
                "compliant": has_tests and has_docs
            }
            
            if not (has_tests and has_docs):
                results["compliance"] = False
    
    # Write report
    with open("qa_compliance_report.json", "w") as f:
        json.dump(results, f, indent=2)
    
    return results["compliance"]

if __name__ == "__main__":
    if verify_compliance():
        print("✓ QA Compliance: PASSED")
        exit(0)
    else:
        print("✗ QA Compliance: FAILED")
        exit(1)
'''
        
        script_path.write_text(content)
        script_path.chmod(0o755)
        print(self.colors.success(f"Generated: {script_path}"))
    
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
            
        except Exception as e:
            print(self.colors.error(f"\nSetup failed: {e}"))
            return 1
        
        return 0

if __name__ == "__main__":
    setup = PolyCallSetup()
    sys.exit(setup.run())
