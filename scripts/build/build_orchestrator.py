#!/usr/bin/env python3
"""
LibPolyCall v2 Build Orchestrator
OBINexus Aegis Project - Centralized Build System
Ensures proper include path mapping and output directory structure

Author: OBINexus Computing - Sinphasé Governance Framework
Version: 2.0.0
"""

import os
import sys
import json
import shutil
import platform
import argparse
import subprocess
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from datetime import datetime

class PolycallBuildOrchestrator:
    """Orchestrates LibPolyCall build process with standardized output structure."""
    
    def __init__(self, project_root: str, verbose: bool = False):
        self.project_root = Path(project_root).resolve()
        self.verbose = verbose
        
        # Build output structure - centralized in build/
        self.build_root = self.project_root / "build"
        self.obj_dir = self.build_root / "obj"
        self.lib_dir = self.build_root / "lib"
        self.bin_dir = self.build_root / "bin"
        self.include_staging = self.build_root / "include"
        
        # Source directories
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # Module mappings for proper include paths
        self.core_modules = [
            "auth", "accessibility", "config", "edge", "ffi",
            "micro", "network", "protocol", "telemetry", "polycall"
        ]
        
        self.cli_modules = [
            "commands", "providers", "repl", "common"
        ]
        
        # Build configurations
        self.configs = {
            "debug": {
                "cflags": "-g -O0 -DDEBUG -Wall -Wextra -fPIC",
                "output_dir": self.bin_dir / "debug"
            },
            "release": {
                "cflags": "-O3 -DNDEBUG -Wall -Wextra -fPIC",
                "output_dir": self.bin_dir / "prod"
            }
        }
        
        # Library output names (polycall, not libpolycall)
        self.lib_name = "polycall"
        self.static_lib = f"{self.lib_name}.a"
        self.shared_lib = f"{self.lib_name}.so"
        
        # Compilation tracking
        self.compilation_db = []
        
    def setup_build_directories(self):
        """Create standardized build directory structure."""
        print("=== Setting up build directories ===")
        
        # Create main build structure
        directories = [
            self.build_root,
            self.obj_dir,
            self.lib_dir,
            self.bin_dir / "debug",
            self.bin_dir / "prod",
            self.include_staging / "polycall"
        ]
        
        # Create module-specific object directories
        for module in self.core_modules:
            directories.append(self.obj_dir / "core" / module)
        
        for module in self.cli_modules:
            directories.append(self.obj_dir / "cli" / module)
        
        for dir_path in directories:
            dir_path.mkdir(parents=True, exist_ok=True)
            if self.verbose:
                print(f"  Created: {dir_path}")
    
    def fix_include_paths(self, source_file: Path) -> List[str]:
        """
        Fix include paths in source file to ensure proper polycall/ prefix.
        Returns list of fixed includes.
        """
        fixed_includes = []
        content = source_file.read_text()
        
        # Pattern to match includes
        include_pattern = r'#include\s*[<"](.+?)[>"]'
        
        # Common incorrect patterns and their fixes
        fixes = {
            r'"libpolycall/(.+)"': r'"polycall/\1"',
            r'<libpolycall/(.+)>': r'<polycall/\1>',
            r'"core/(.+)"': r'"polycall/core/\1"',
            r'"cli/(.+)"': r'"polycall/cli/\1"',
            r'"../include/(.+)"': r'"\1"',
            r'"../../include/(.+)"': r'"\1"',
        }
        
        modified = False
        for pattern, replacement in fixes.items():
            new_content = re.sub(pattern, replacement, content)
            if new_content != content:
                modified = True
                content = new_content
        
        if modified and not self.dry_run:
            source_file.write_text(content)
            fixed_includes.append(str(source_file))
        
        return fixed_includes
    
    def stage_headers(self):
        """Stage headers with proper polycall/ prefix structure."""
        print("=== Staging headers ===")
        
        if not self.include_dir.exists():
            print("  Warning: include directory not found")
            return
        
        # Copy polycall headers to staging area
        polycall_include = self.include_dir / "polycall"
        if polycall_include.exists():
            dest = self.include_staging / "polycall"
            shutil.copytree(polycall_include, dest, dirs_exist_ok=True)
            print(f"  Staged headers to: {dest}")
    
    def compile_module(self, module_type: str, module_name: str, config: str = "debug") -> bool:
        """
        Compile a specific module with proper include paths.
        Returns True if successful.
        """
        module_src = self.src_dir / module_type / module_name
        module_obj = self.obj_dir / module_type / module_name
        
        if not module_src.exists():
            if self.verbose:
                print(f"  Module source not found: {module_src}")
            return False
        
        # Find all C files in module
        c_files = list(module_src.glob("*.c"))
        if not c_files:
            return True  # No source files to compile
        
        print(f"  Compiling {module_type}/{module_name} ({len(c_files)} files)")
        
        # Compile each source file
        cc = os.environ.get("CC", "gcc")
        cflags = self.configs[config]["cflags"]
        include_flags = [
            f"-I{self.include_staging}",
            f"-I{self.include_dir}",
            f"-I{self.src_dir}"
        ]
        
        success = True
        for c_file in c_files:
            obj_file = module_obj / c_file.with_suffix(".o").name
            
            # Fix includes before compiling
            self.fix_include_paths(c_file)
            
            # Compile command
            cmd = [cc] + cflags.split() + include_flags + [
                "-c", str(c_file),
                "-o", str(obj_file)
            ]
            
            if self.verbose:
                print(f"    {' '.join(cmd)}")
            
            # Track compilation command
            self.compilation_db.append({
                "directory": str(self.project_root),
                "command": " ".join(cmd),
                "file": str(c_file)
            })
            
            # Execute compilation
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"    Error compiling {c_file.name}:")
                print(result.stderr)
                success = False
            else:
                print(f"    ✓ {c_file.name} -> {obj_file.name}")
        
        return success
    
    def link_library(self, config: str = "debug"):
        """Link all object files into static and shared libraries."""
        print("=== Linking libraries ===")
        
        # Find all object files
        obj_files = list(self.obj_dir.rglob("*.o"))
        if not obj_files:
            print("  No object files found")
            return False
        
        print(f"  Found {len(obj_files)} object files")
        
        # Create static library
        ar = os.environ.get("AR", "ar")
        static_lib_path = self.lib_dir / self.static_lib
        
        cmd = [ar, "rcs", str(static_lib_path)] + [str(f) for f in obj_files]
        if self.verbose:
            print(f"  Static library: {' '.join(cmd[:3])} ... ({len(obj_files)} objects)")
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"  ✓ Created: {static_lib_path}")
        else:
            print(f"  Error creating static library: {result.stderr}")
            return False
        
        # Create shared library
        cc = os.environ.get("CC", "gcc")
        shared_lib_path = self.lib_dir / self.shared_lib
        
        cmd = [cc, "-shared", "-o", str(shared_lib_path)] + [str(f) for f in obj_files]
        if self.verbose:
            print(f"  Shared library: {' '.join(cmd[:4])} ... ({len(obj_files)} objects)")
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"  ✓ Created: {shared_lib_path}")
        else:
            print(f"  Error creating shared library: {result.stderr}")
            return False
        
        return True
    
    def build_executable(self, config: str = "debug"):
        """Build the polycall executable."""
        print("=== Building executable ===")
        
        # Find main.c
        main_file = self.src_dir / "cli" / "main.c"
        if not main_file.exists():
            print(f"  Error: main.c not found at {main_file}")
            return False
        
        cc = os.environ.get("CC", "gcc")
        cflags = self.configs[config]["cflags"]
        output_dir = self.configs[config]["output_dir"]
        
        # Determine executable name based on platform
        exe_name = "polycall.exe" if platform.system() == "Windows" else "polycall"
        exe_path = output_dir / exe_name
        
        # Link against our library
        lib_path = self.lib_dir / self.static_lib
        
        cmd = [cc] + cflags.split() + [
            f"-I{self.include_staging}",
            f"-I{self.include_dir}",
            str(main_file),
            str(lib_path),
            "-o", str(exe_path)
        ]
        
        if self.verbose:
            print(f"  Command: {' '.join(cmd)}")
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"  ✓ Created: {exe_path}")
            return True
        else:
            print(f"  Error building executable: {result.stderr}")
            return False
    
    def generate_compilation_database(self):
        """Generate compile_commands.json for tooling support."""
        db_path = self.build_root / "compile_commands.json"
        with open(db_path, 'w') as f:
            json.dump(self.compilation_db, f, indent=2)
        print(f"  Generated: {db_path}")
    
    def build_all(self, config: str = "debug"):
        """Execute complete build process."""
        print(f"\n{'='*60}")
        print(f"LibPolyCall v2 Build Orchestrator")
        print(f"Configuration: {config}")
        print(f"Project Root: {self.project_root}")
        print(f"{'='*60}\n")
        
        # Setup
        self.setup_build_directories()
        self.stage_headers()
        
        # Compile core modules
        print("\n=== Compiling core modules ===")
        for module in self.core_modules:
            self.compile_module("core", module, config)
        
        # Compile CLI modules
        print("\n=== Compiling CLI modules ===")
        for module in self.cli_modules:
            self.compile_module("cli", module, config)
        
        # Link libraries
        if not self.link_library(config):
            return False
        
        # Build executable
        if not self.build_executable(config):
            return False
        
        # Generate compilation database
        self.generate_compilation_database()
        
        # Summary
        print(f"\n{'='*60}")
        print("Build Summary:")
        print(f"  Object files: {len(list(self.obj_dir.rglob('*.o')))}")
        print(f"  Static library: {self.lib_dir / self.static_lib}")
        print(f"  Shared library: {self.lib_dir / self.shared_lib}")
        print(f"  Executable: {self.configs[config]['output_dir'] / 'polycall'}")
        print(f"{'='*60}\n")
        
        return True
    
    def clean(self):
        """Clean all build artifacts."""
        print("=== Cleaning build artifacts ===")
        if self.build_root.exists():
            shutil.rmtree(self.build_root)
            print(f"  Removed: {self.build_root}")
        else:
            print("  Build directory already clean")

def main():
    parser = argparse.ArgumentParser(
        description="LibPolyCall v2 Build Orchestrator"
    )
    parser.add_argument(
        "--project-root",
        default=".",
        help="Project root directory (default: current directory)"
    )
    parser.add_argument(
        "--config",
        choices=["debug", "release"],
        default="debug",
        help="Build configuration (default: debug)"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build artifacts"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose output"
    )
    
    args = parser.parse_args()
    
    # Initialize orchestrator
    orchestrator = PolycallBuildOrchestrator(
        project_root=args.project_root,
        verbose=args.verbose
    )
    
    # Execute requested action
    if args.clean:
        orchestrator.clean()
    else:
        success = orchestrator.build_all(config=args.config)
        sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()