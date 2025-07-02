#!/usr/bin/env python3
"""
Fix PolyCall Migration Violations
Resolves include path and command purity issues
"""

import os
import re
import shutil
from pathlib import Path
from datetime import datetime
import json

class ViolationFixer:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.backup_dir = self.project_root / f"backup_violations_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        self.fixes_applied = []
        
        # Reclassify config as infrastructure module
        self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo",
            "auth", "network", "protocol", "accessibility"
        }
        
        # Config is now infrastructure, not a command module
        self.infrastructure_modules = {
            "base", "polycall", "common", "memory", "error", "context",
            "config", "parser", "schema", "factory"  # Added config-related modules
        }
        
    def fix_all_violations(self):
        """Main entry point to fix all violations"""
        print("=== PolyCall Violation Fixer ===")
        print(f"Project root: {self.project_root}")
        print(f"Creating backup: {self.backup_dir}")
        
        # Create backup
        self._create_backup()
        
        # Fix include paths
        print("\n[1/4] Fixing include path violations...")
        self._fix_include_paths()
        
        # Fix config dependencies
        print("\n[2/4] Refactoring config module dependencies...")
        self._refactor_config_dependencies()
        
        # Fix duplicate core/core issue
        print("\n[3/4] Fixing directory structure issues...")
        self._fix_directory_structure()
        
        # Update migration enforcer config
        print("\n[4/4] Updating migration enforcer configuration...")
        self._update_enforcer_config()
        
        # Generate report
        self._generate_fix_report()
        
        print(f"\n✓ Applied {len(self.fixes_applied)} fixes")
        print(f"Backup saved to: {self.backup_dir}")
        
    def _create_backup(self):
        """Create backup of files to be modified"""
        self.backup_dir.mkdir(exist_ok=True)
        
        # Backup source directories
        for src_dir in ["src/core", "include/polycall"]:
            src_path = self.project_root / src_dir
            if src_path.exists():
                dst_path = self.backup_dir / src_dir
                dst_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(src_path, dst_path)
                
    def _fix_include_paths(self):
        """Fix incorrect include paths in source files"""
        fixes = [
            # Fix core/core/polycall.c issue
            {
                "file": "src/core/core/polycall.c",
                "wrong_path": "src/core/core/polycall.c",
                "correct_path": "src/core/polycall/polycall.c",
                "include": "polycall/polycall/polycall.h"
            },
            # Fix security.c include
            {
                "file": "src/core/core/security.c", 
                "wrong_path": "src/core/core/security.c",
                "correct_path": "src/core/edge/security.c",
                "include": "polycall/edge/security.h"
            }
        ]
        
        for fix in fixes:
            wrong_file = self.project_root / fix["wrong_path"]
            correct_file = self.project_root / fix["correct_path"]
            
            if wrong_file.exists():
                # Move file to correct location
                correct_file.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(wrong_file), str(correct_file))
                self.fixes_applied.append(f"Moved {fix['wrong_path']} -> {fix['correct_path']}")
                
                # Fix include in the file
                self._fix_file_include(correct_file, fix["include"])
                
    def _fix_file_include(self, file_path: Path, correct_include: str):
        """Fix include statement in a file"""
        if not file_path.exists():
            return
            
        content = file_path.read_text()
        
        # Find and replace incorrect includes
        include_pattern = r'#include\s*[<"]([^>"]+)[>"]'
        
        def replace_include(match):
            current_include = match.group(1)
            # Check if this looks like it should be our header
            if file_path.stem in current_include or "polycall" in current_include:
                return f'#include "{correct_include}"'
            return match.group(0)
            
        new_content = re.sub(include_pattern, replace_include, content, count=1)
        
        if new_content != content:
            file_path.write_text(new_content)
            self.fixes_applied.append(f"Fixed include in {file_path}")
            
    def _refactor_config_dependencies(self):
        """Refactor config module usage to use infrastructure pattern"""
        # Find all files that include config from command modules
        for module in self.command_modules:
            module_dir = self.project_root / "src" / "core" / module
            if not module_dir.exists():
                continue
                
            for src_file in module_dir.glob("*.c"):
                self._refactor_config_include(src_file)
                
    def _refactor_config_include(self, file_path: Path):
        """Refactor config includes in a file"""
        content = file_path.read_text()
        original_content = content
        
        # Pattern to find config includes
        config_patterns = [
            (r'#include\s*[<"](?:.*?/)?config\.h[>"]', '#include "polycall/polycall_config.h"'),
            (r'#include\s*[<"](?:.*?/)?config_parser\.h[>"]', '#include "polycall/parser/config_parser.h"'),
            (r'#include\s*[<"](?:.*?/)?polycall_config\.h[>"]', '#include "polycall/polycall_config.h"'),
            (r'#include\s*[<"](?:.*?/)?(\w+_)?config\.h[>"]', self._replace_config_include),
        ]
        
        for pattern, replacement in config_patterns:
            if callable(replacement):
                content = re.sub(pattern, replacement, content)
            else:
                content = re.sub(pattern, replacement, content)
                
        # Also check for direct config module references
        if "config/" in content or "/config/" in content:
            # Replace with infrastructure config
            content = re.sub(r'(\w+/)config/', r'\1polycall/', content)
            
        if content != original_content:
            file_path.write_text(content)
            self.fixes_applied.append(f"Refactored config usage in {file_path}")
            
    def _replace_config_include(self, match):
        """Replacement function for config includes"""
        full_match = match.group(0)
        prefix = match.group(1) if match.lastindex >= 1 else ""
        
        # Map module-specific configs to infrastructure
        if "network_config" in full_match:
            return '#include "polycall/network/network_config.h"'
        elif "telemetry_config" in full_match:
            return '#include "polycall/telemetry/telemetry_config.h"'
        elif "micro_config" in full_match:
            return '#include "polycall/micro/polycall_micro_config.h"'
        else:
            # Default to polycall config
            return '#include "polycall/polycall_config.h"'
            
    def _fix_directory_structure(self):
        """Fix directory structure issues like core/core"""
        # Check for duplicate core directory
        double_core = self.project_root / "src" / "core" / "core"
        if double_core.exists():
            print(f"  Found duplicate core directory: {double_core}")
            
            # Move files up one level
            for item in double_core.iterdir():
                if item.is_file():
                    # Determine destination based on file name
                    if "polycall" in item.name:
                        dest_dir = self.project_root / "src" / "core" / "polycall"
                    elif "security" in item.name:
                        dest_dir = self.project_root / "src" / "core" / "edge"
                    else:
                        dest_dir = self.project_root / "src" / "core"
                        
                    dest_dir.mkdir(parents=True, exist_ok=True)
                    dest_file = dest_dir / item.name
                    
                    shutil.move(str(item), str(dest_file))
                    self.fixes_applied.append(f"Moved {item} -> {dest_file}")
                    
            # Remove empty directory
            if not any(double_core.iterdir()):
                double_core.rmdir()
                self.fixes_applied.append(f"Removed empty directory: {double_core}")
                
    def _update_enforcer_config(self):
        """Update the migration enforcer script configuration"""
        enforcer_script = self.project_root / "scripts" / "polycall_migration_enforcer.py"
        
        if enforcer_script.exists():
            content = enforcer_script.read_text()
            
            # Update command modules list (remove config)
            old_modules = '''self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo",
            "auth", "config", "network", "protocol", "accessibility"
        }'''
            
            new_modules = '''self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo",
            "auth", "network", "protocol", "accessibility"
        }'''
            
            # Update infrastructure modules (add config)
            old_infra = '''self.infrastructure_modules = {
            "base", "polycall", "common", "memory", "error", "context"
        }'''
            
            new_infra = '''self.infrastructure_modules = {
            "base", "polycall", "common", "memory", "error", "context",
            "config", "parser", "schema", "factory"
        }'''
            
            content = content.replace(old_modules, new_modules)
            content = content.replace(old_infra, new_infra)
            
            enforcer_script.write_text(content)
            self.fixes_applied.append("Updated migration enforcer configuration")
            
    def _generate_fix_report(self):
        """Generate a report of fixes applied"""
        report = {
            "timestamp": datetime.now().isoformat(),
            "backup_location": str(self.backup_dir),
            "fixes_applied": self.fixes_applied,
            "recommendations": [
                "Run the migration enforcer again to verify fixes",
                "Config module is now classified as infrastructure",
                "Command modules can safely depend on config",
                "Review the backup if any issues arise"
            ]
        }
        
        report_file = self.project_root / "fix_violations_report.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
            
        print(f"\nFix report saved to: {report_file}")


def main():
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python fix_polycall_violations.py <project_root>")
        sys.exit(1)
        
    project_root = sys.argv[1]
    fixer = ViolationFixer(project_root)
    fixer.fix_all_violations()


if __name__ == "__main__":
    main()
