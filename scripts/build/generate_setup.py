#!/usr/bin/env python3
"""
LibPolyCall v2 Cross-Platform Setup Script Generator
Generates setup.sh, setup.cmd, and setup.ps1 for universal build system

Author: OBINexus Computing - Aegis Project
Version: 2.0.0
"""

import os
import sys
from pathlib import Path
from datetime import datetime

class SetupScriptGenerator:
    """Generates platform-specific setup scripts for LibPolyCall build system."""
    
    def __init__(self, project_root: str = "."):
        self.project_root = Path(project_root).resolve()
        self.timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        # Common build requirements
        self.python_requirements = [
            "build_orchestrator.py",
            "fix_include_paths.py", 
            "generate_setup.py"
        ]
        
        self.dependencies = {
            "unix": ["gcc", "make", "ar", "python3"],
            "windows": ["cl.exe", "nmake", "lib.exe", "python"]
        }
    
    def generate_bash_setup(self) -> str:
        """Generate setup.sh for Unix/Linux systems."""
        return f'''#!/bin/bash
# LibPolyCall v2 Setup Script - Unix/Linux
# Generated: {self.timestamp}
# OBINexus Aegis Project - Sinphasé Governance

set -e

# Color codes for output
RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[0;33m'
BLUE='\\033[0;34m'
NC='\\033[0m'

# Project paths
PROJECT_ROOT="$(cd "$(dirname "${{BASH_SOURCE[0]}})"/../.. && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SCRIPTS_DIR="$PROJECT_ROOT/scripts"

echo -e "${{BLUE}}=== LibPolyCall v2 Build Setup ===${{NC}}"
echo "Project Root: $PROJECT_ROOT"

# Check dependencies
echo -e "\\n${{YELLOW}}Checking dependencies...${{NC}}"
MISSING_DEPS=""
for dep in gcc make ar python3; do
    if ! command -v $dep &> /dev/null; then
        MISSING_DEPS="$MISSING_DEPS $dep"
        echo -e "${{RED}}✗ $dep not found${{NC}}"
    else
        echo -e "${{GREEN}}✓ $dep found${{NC}}"
    fi
done

if [ -n "$MISSING_DEPS" ]; then
    echo -e "\\n${{RED}}Error: Missing dependencies:$MISSING_DEPS${{NC}}"
    echo "Please install missing dependencies before continuing."
    exit 1
fi

# Create build directories
echo -e "\\n${{YELLOW}}Creating build directories...${{NC}}"
mkdir -p "$BUILD_DIR/obj/core" "$BUILD_DIR/obj/cli"
mkdir -p "$BUILD_DIR/lib" "$BUILD_DIR/bin/debug" "$BUILD_DIR/bin/prod"
mkdir -p "$BUILD_DIR/include/polycall"

# Fix include paths
echo -e "\\n${{YELLOW}}Fixing include paths...${{NC}}"
if [ -f "$SCRIPTS_DIR/build/fix_include_paths.py" ]; then
    python3 "$SCRIPTS_DIR/build/fix_include_paths.py" --project-root "$PROJECT_ROOT"
else
    echo -e "${{YELLOW}}Include path fixer not found, skipping...${{NC}}"
fi

# Stage headers
echo -e "\\n${{YELLOW}}Staging headers...${{NC}}"
if [ -d "$PROJECT_ROOT/include/polycall" ]; then
    cp -r "$PROJECT_ROOT/include/polycall" "$BUILD_DIR/include/"
    echo -e "${{GREEN}}✓ Headers staged${{NC}}"
fi

# Run build orchestrator
echo -e "\\n${{YELLOW}}Running build orchestrator...${{NC}}"
if [ -f "$SCRIPTS_DIR/build/build_orchestrator.py" ]; then
    python3 "$SCRIPTS_DIR/build/build_orchestrator.py" \\
        --project-root "$PROJECT_ROOT" \\
        --config debug \\
        --verbose
else
    echo -e "${{RED}}Build orchestrator not found!${{NC}}"
    exit 1
fi

echo -e "\\n${{GREEN}}=== Setup Complete ===${{NC}}"
echo "Build artifacts location: $BUILD_DIR"
echo "Run 'make build' to compile the project"
'''

    def generate_cmd_setup(self) -> str:
        """Generate setup.cmd for Windows Command Prompt."""
        return f'''@echo off
REM LibPolyCall v2 Setup Script - Windows CMD
REM Generated: {self.timestamp}
REM OBINexus Aegis Project - Sinphasé Governance

setlocal enabledelayedexpansion

REM Project paths
set PROJECT_ROOT=%~dp0..\..
set BUILD_DIR=%PROJECT_ROOT%\\build
set SCRIPTS_DIR=%PROJECT_ROOT%\\scripts

echo === LibPolyCall v2 Build Setup ===
echo Project Root: %PROJECT_ROOT%

REM Check dependencies
echo.
echo Checking dependencies...
set MISSING_DEPS=
where cl.exe >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! cl.exe
where nmake >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! nmake
where python >nul 2>&1 || set MISSING_DEPS=!MISSING_DEPS! python

if not "!MISSING_DEPS!"=="" (
    echo Error: Missing dependencies:!MISSING_DEPS!
    echo Please install Visual Studio Build Tools and Python
    exit /b 1
)

REM Create build directories
echo.
echo Creating build directories...
mkdir "%BUILD_DIR%\\obj\\core" 2>nul
mkdir "%BUILD_DIR%\\obj\\cli" 2>nul
mkdir "%BUILD_DIR%\\lib" 2>nul
mkdir "%BUILD_DIR%\\bin\\debug" 2>nul
mkdir "%BUILD_DIR%\\bin\\prod" 2>nul
mkdir "%BUILD_DIR%\\include\\polycall" 2>nul

REM Fix include paths
echo.
echo Fixing include paths...
if exist "%SCRIPTS_DIR%\\build\\fix_include_paths.py" (
    python "%SCRIPTS_DIR%\\build\\fix_include_paths.py" --project-root "%PROJECT_ROOT%"
) else (
    echo Include path fixer not found, skipping...
)

REM Stage headers
echo.
echo Staging headers...
if exist "%PROJECT_ROOT%\\include\\polycall" (
    xcopy /E /I /Y "%PROJECT_ROOT%\\include\\polycall" "%BUILD_DIR%\\include\\polycall"
    echo Headers staged
)

REM Run build orchestrator
echo.
echo Running build orchestrator...
if exist "%SCRIPTS_DIR%\\build\\build_orchestrator.py" (
    python "%SCRIPTS_DIR%\\build\\build_orchestrator.py" ^
        --project-root "%PROJECT_ROOT%" ^
        --config debug ^
        --verbose
) else (
    echo Build orchestrator not found!
    exit /b 1
)

echo.
echo === Setup Complete ===
echo Build artifacts location: %BUILD_DIR%
echo Run 'nmake' to compile the project

endlocal
'''

    def generate_powershell_setup(self) -> str:
        """Generate setup.ps1 for Windows PowerShell."""
        return f'''# LibPolyCall v2 Setup Script - PowerShell
# Generated: {self.timestamp}
# OBINexus Aegis Project - Sinphasé Governance

$ErrorActionPreference = "Stop"

# Color functions
function Write-Success {{ param($msg) Write-Host $msg -ForegroundColor Green }}
function Write-Warning {{ param($msg) Write-Host $msg -ForegroundColor Yellow }}
function Write-Error {{ param($msg) Write-Host $msg -ForegroundColor Red }}
function Write-Info {{ param($msg) Write-Host $msg -ForegroundColor Cyan }}

# Project paths
$ProjectRoot = (Get-Item $PSScriptRoot).Parent.Parent.FullName
$BuildDir = Join-Path $ProjectRoot "build"
$ScriptsDir = Join-Path $ProjectRoot "scripts"

Write-Info "=== LibPolyCall v2 Build Setup ==="
Write-Host "Project Root: $ProjectRoot"

# Check dependencies
Write-Info "`nChecking dependencies..."
$missingDeps = @()

$dependencies = @{{
    "cl.exe" = "Visual Studio C++ Compiler"
    "nmake" = "Visual Studio Build Tools"
    "python" = "Python 3.x"
}}

foreach ($dep in $dependencies.Keys) {{
    $found = Get-Command $dep -ErrorAction SilentlyContinue
    if (-not $found) {{
        $missingDeps += $dependencies[$dep]
        Write-Error "✗ $($dependencies[$dep]) not found"
    }} else {{
        Write-Success "✓ $($dependencies[$dep]) found"
    }}
}}

if ($missingDeps.Count -gt 0) {{
    Write-Error "`nError: Missing dependencies:"
    $missingDeps | ForEach-Object {{ Write-Error "  - $_" }}
    Write-Host "`nPlease install missing dependencies before continuing."
    exit 1
}}

# Create build directories
Write-Info "`nCreating build directories..."
$directories = @(
    "$BuildDir\\obj\\core",
    "$BuildDir\\obj\\cli",
    "$BuildDir\\lib",
    "$BuildDir\\bin\\debug",
    "$BuildDir\\bin\\prod",
    "$BuildDir\\include\\polycall"
)

foreach ($dir in $directories) {{
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}}
Write-Success "✓ Build directories created"

# Fix include paths
Write-Info "`nFixing include paths..."
$fixScript = Join-Path $ScriptsDir "build\\fix_include_paths.py"
if (Test-Path $fixScript) {{
    & python $fixScript --project-root $ProjectRoot
}} else {{
    Write-Warning "Include path fixer not found, skipping..."
}}

# Stage headers
Write-Info "`nStaging headers..."
$includeSource = Join-Path $ProjectRoot "include\\polycall"
$includeDest = Join-Path $BuildDir "include\\polycall"
if (Test-Path $includeSource) {{
    Copy-Item -Path $includeSource -Destination (Join-Path $BuildDir "include") -Recurse -Force
    Write-Success "✓ Headers staged"
}}

# Run build orchestrator
Write-Info "`nRunning build orchestrator..."
$orchestrator = Join-Path $ScriptsDir "build\\build_orchestrator.py"
if (Test-Path $orchestrator) {{
    & python $orchestrator `
        --project-root $ProjectRoot `
        --config debug `
        --verbose
}} else {{
    Write-Error "Build orchestrator not found!"
    exit 1
}}

Write-Success "`n=== Setup Complete ==="
Write-Host "Build artifacts location: $BuildDir"
Write-Host "Run 'nmake' or use Visual Studio to compile the project"
'''

    def generate_all(self):
        """Generate all platform-specific setup scripts."""
        output_dir = self.project_root / "scripts" / "build"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        scripts = {
            "setup.sh": self.generate_bash_setup(),
            "setup.cmd": self.generate_cmd_setup(),
            "setup.ps1": self.generate_powershell_setup()
        }
        
        for filename, content in scripts.items():
            filepath = output_dir / filename
            filepath.write_text(content)
            
            # Make shell script executable on Unix
            if filename.endswith('.sh'):
                os.chmod(filepath, 0o755)
            
            print(f"Generated: {filepath}")
        
        # Also create in project root for convenience
        root_scripts = ["setup.sh", "setup.cmd", "setup.ps1"]
        for script in root_scripts:
            src = output_dir / script
            dst = self.project_root / script
            if src.exists():
                dst.write_text(src.read_text())
                if script.endswith('.sh'):
                    os.chmod(dst, 0o755)
                print(f"Created root script: {dst}")

def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Generate cross-platform setup scripts for LibPolyCall"
    )
    parser.add_argument(
        "--project-root",
        default=".",
        help="Project root directory"
    )
    
    args = parser.parse_args()
    
    generator = SetupScriptGenerator(args.project_root)
    generator.generate_all()
    
    print("\n✓ Setup scripts generated successfully!")
    print("  - Unix/Linux: ./setup.sh")
    print("  - Windows CMD: setup.cmd")
    print("  - PowerShell: .\\setup.ps1")

if __name__ == "__main__":
    main()