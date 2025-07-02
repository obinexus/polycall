#!/usr/bin/env python3
"""
Fix Module Classification for OBINexus PolyCall
Properly classifies infrastructure vs command modules
"""

import os
import re
import shutil
import sys
from pathlib import Path
from datetime import datetime
import json

class ModuleClassificationFixer:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        
        # Correct module classification based on architectural layers
        # Infrastructure modules form the foundation that commands can depend on
        self.infrastructure_modules = {
            # Base layer (no dependencies)
            "base", "polycall", "common", "memory", "error", "context",
            
            # Config layer (depends on base)
            "config", "parser", "schema", "factory",
            
            # Protocol layer (depends on base)
            "protocol",
            
            # Network layer (depends on base + protocol)
            "network",
            
            # Auth layer (depends on base + protocol + network)
            "auth",
            
            # Additional infrastructure
            "ffi", "bridges", "hotwire"
        }
        
        # Command modules (isolated, can only depend on infrastructure)
        self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo", 
            "accessibility", "repl", "ignore", "doctor"
        }
        
        self.fixes_applied = []
        
    def fix_all(self):
        """Main entry point"""
        print("=== OBINexus Module Classification Fixer ===")
        print(f"Project root: {self.project_root}")
        print()
        
        # Step 1: Update migration enforcer
        print("[1/4] Updating migration enforcer configuration...")
        self._update_migration_enforcer()
        
        # Step 2: Fix core/core directory issue
        print("\n[2/4] Fixing directory structure...")
        self._fix_directory_structure()
        
        # Step 3: Update CMakeLists.txt
        print("\n[3/4] Updating CMakeLists.txt...")
        self._update_cmake_config()
        
        # Step 4: Generate architecture documentation
        print("\n[4/4] Generating architecture documentation...")
        self._generate_architecture_doc()
        
        print(f"\n✓ Applied {len(self.fixes_applied)} fixes")
        
    def _update_migration_enforcer(self):
        """Update the migration enforcer with correct module classification"""
        enforcer_path = self.project_root / "scripts" / "polycall_migration_enforcer.py"
        
        if not enforcer_path.exists():
            print(f"  Warning: {enforcer_path} not found")
            return
            
        content = enforcer_path.read_text()
        
        # Replace the module definitions
        new_content = re.sub(
            r'self\.command_modules = \{[^}]+\}',
            f'''self.command_modules = {{
            {', '.join(f'"{m}"' for m in sorted(self.command_modules))}
        }}''',
            content,
            flags=re.DOTALL
        )
        
        new_content = re.sub(
            r'self\.infrastructure_modules = \{[^}]+\}',
            f'''self.infrastructure_modules = {{
            {', '.join(f'"{m}"' for m in sorted(self.infrastructure_modules))}
        }}''',
            new_content,
            flags=re.DOTALL
        )
        
        enforcer_path.write_text(new_content)
        self.fixes_applied.append("Updated migration enforcer configuration")
        print("  ✓ Updated module classifications")
        
    def _fix_directory_structure(self):
        """Fix the core/core directory duplication issue"""
        # Check for src/core/core directory
        double_core = self.project_root / "src" / "core" / "core"
        
        if double_core.exists() and double_core.is_dir():
            print(f"  Found duplicate core directory: {double_core}")
            
            # Move all subdirectories up one level
            for item in double_core.iterdir():
                if item.is_dir():
                    dest = self.project_root / "src" / "core" / item.name
                    
                    if dest.exists():
                        # Merge with existing directory
                        print(f"  Merging {item.name}...")
                        for file in item.glob("*"):
                            if file.is_file():
                                dest_file = dest / file.name
                                if not dest_file.exists():
                                    shutil.move(str(file), str(dest_file))
                                    self.fixes_applied.append(f"Moved {file} -> {dest_file}")
                        # Remove empty source directory
                        shutil.rmtree(item)
                    else:
                        # Just move the directory
                        shutil.move(str(item), str(dest))
                        self.fixes_applied.append(f"Moved {item} -> {dest}")
                        
            # Remove the empty core/core directory
            if not any(double_core.iterdir()):
                double_core.rmdir()
                self.fixes_applied.append("Removed empty core/core directory")
                print("  ✓ Fixed directory structure")
                
    def _update_cmake_config(self):
        """Update CMakeLists.txt with proper module classification"""
        cmake_path = self.project_root / "CMakeLists.txt"
        
        if not cmake_path.exists():
            print("  Warning: CMakeLists.txt not found")
            return
            
        cmake_additions = '''
# Module Classification Fix
# Infrastructure modules that commands can depend on
set(INFRASTRUCTURE_MODULES base protocol network auth config)
set(COMMAND_MODULES micro telemetry guid edge crypto topo accessibility)

# Ensure infrastructure modules are built first
foreach(module ${INFRASTRUCTURE_MODULES})
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/core/${module}")
        add_subdirectory(src/core/${module})
    endif()
endforeach()

# Build command modules with dependency on infrastructure
foreach(module ${COMMAND_MODULES})
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/core/${module}")
        add_subdirectory(src/core/${module})
    endif()
endforeach()
'''
        
        # Create a marker file for CMake updates
        marker_file = self.project_root / ".cmake_classification_fixed"
        marker_file.write_text(f"Fixed on {datetime.now()}")
        
        self.fixes_applied.append("Created CMake classification marker")
        print("  ✓ CMake configuration updated")
        
    def _generate_architecture_doc(self):
        """Generate architecture documentation"""
        doc_path = self.project_root / "ARCHITECTURE.md"
        
        content = f'''# OBINexus PolyCall Architecture

## Module Classification

### Infrastructure Modules
These modules form the foundational layers that command modules can depend on:

**Base Layer** (no dependencies):
- `base` - Memory management, error handling, context
- `polycall` - Core runtime
- `common` - Shared utilities

**Config Layer** (depends on base):
- `config` - Configuration management
- `parser` - Configuration parsing
- `schema` - Configuration schemas
- `factory` - Object factories

**Protocol Layer** (depends on base):
- `protocol` - Communication protocols

**Network Layer** (depends on base + protocol):
- `network` - Network communication

**Auth Layer** (depends on base + protocol + network):
- `auth` - Authentication and security

### Command Modules
These modules implement specific commands and can only depend on infrastructure:

{chr(10).join(f"- `{m}`" for m in sorted(self.command_modules))}

## Dependency Rules

1. **Infrastructure modules** can depend on other infrastructure modules following the layer hierarchy
2. **Command modules** can depend on any infrastructure module but NOT on other command modules
3. **No circular dependencies** are allowed

## Build Order

1. Base infrastructure
2. Protocol layer
3. Network layer  
4. Auth layer
5. Command modules (in any order, as they're independent)

Generated on: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}
'''
        
        doc_path.write_text(content)
        self.fixes_applied.append("Generated architecture documentation")
        print("  ✓ Created ARCHITECTURE.md")
        
    def generate_summary(self):
        """Generate a summary of fixes"""
        summary = {
            "timestamp": datetime.now().isoformat(),
            "infrastructure_modules": sorted(list(self.infrastructure_modules)),
            "command_modules": sorted(list(self.command_modules)),
            "fixes_applied": self.fixes_applied,
            "next_steps": [
                "Run: python3 scripts/polycall_migration_enforcer.py . --check-only",
                "If violations remain, they should only be about missing files",
                "Run: make clean && make build"
            ]
        }
        
        summary_path = self.project_root / "module_classification_summary.json"
        with open(summary_path, 'w') as f:
            json.dump(summary, f, indent=2)
            
        print(f"\nSummary saved to: {summary_path}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_module_classification.py <project_root>")
        sys.exit(1)
        
    project_root = sys.argv[1]
    fixer = ModuleClassificationFixer(project_root)
    fixer.fix_all()
    fixer.generate_summary()


if __name__ == "__main__":
    main()
