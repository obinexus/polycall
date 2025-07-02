#!/usr/bin/env python3
"""
LibPolyCall Include Path Standardizer

This script standardizes include paths across the LibPolyCall project according to the 
official module hierarchy structure. It ensures consistent pathing in both header files
and implementation files, following the proper module organization.

Usage:
  python standardize_includes.py --project-root /path/to/project [--dry-run] [--verbose]

Author: Implementation based on Nnamdi Okpala's design specifications
"""

import os
import re
import sys
import argparse
import shutil
import logging
from pathlib import Path
from datetime import datetime

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-standardizer')

class IncludePathStandardizer:
    """Standardizes include paths in LibPolyCall source files."""
    
    def __init__(self, root_dir='.', backup=True, dry_run=False):
        self.root_dir = Path(root_dir)
        self.src_dir = self.root_dir / 'src'
        self.include_dir = self.root_dir / 'include'
        self.dry_run = dry_run
        self.backup_dir = None
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.include_fixes = 0
        
        # Module mappings - defines the standardized path for each module
        self.module_map = {
            # Core modules
            'auth': 'polycall/core/auth',
            'cli': 'polycall/cli',
            'config': 'polycall/core/config',
            'context': 'polycall/core/context',
            'error': 'polycall/core/error',
            'memory': 'polycall/core/memory',
            'polycall': 'polycall/core/polycall',
            
            # Primary modules
            'micro': 'polycall/core/micro',
            'edge': 'polycall/core/edge',
            'ffi': 'polycall/core/ffi',
            'protocol': 'polycall/core/protocol',
            'network': 'polycall/core/network',
            'parser': 'polycall/core/parser',
            'telemetry': 'polycall/core/telemetry',
        }
        
        # Direct include mappings - specific filenames that need fixing
        self.direct_include_map = {
            'polycall_core.h': 'polycall/core/polycall/polycall_core.h',
            'polycall_error.h': 'polycall/core/polycall/polycall_error.h',
            'polycall_context.h': 'polycall/core/polycall/polycall_context.h',
            'polycall_memory.h': 'polycall/core/polycall/polycall_memory.h',
            'polycall_types.h': 'polycall/core/polycall/polycall_types.h',
            'ffi_core.h': 'polycall/core/ffi/ffi_core.h',
            'polycall_protocol_context.h': 'polycall/core/protocol/polycall_protocol_context.h',
            'polycall_auth_context.h': 'polycall/core/auth/polycall_auth_context.h',
        }
        
        # Create backup if needed
        if backup and not dry_run:
            self.create_backup()
    
    def create_backup(self):
        """Create a backup of all source code files before modification."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.backup_dir = self.root_dir / f"backup_{timestamp}"
        self.backup_dir.mkdir(exist_ok=True)
        
        logger.info(f"Creating backup in {self.backup_dir}")
        
        # Back up src directory
        if self.src_dir.exists():
            for src_file in self.src_dir.glob('**/*.?'):
                if src_file.suffix in ['.c', '.h', '.cpp', '.hpp']:
                    rel_path = src_file.relative_to(self.root_dir)
                    dest_file = self.backup_dir / rel_path
                    dest_file.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src_file, dest_file)
        
        # Back up include directory
        if self.include_dir.exists():
            for inc_file in self.include_dir.glob('**/*.h'):
                rel_path = inc_file.relative_to(self.root_dir)
                dest_file = self.backup_dir / rel_path
                dest_file.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(inc_file, dest_file)
        
        logger.info(f"Backup completed successfully")
    
    def find_all_source_files(self):
        """Find all source and header files in the project."""
        all_files = []
        
        # Find source files
        if self.src_dir.exists():
            for src_file in self.src_dir.glob('**/*.?'):
                if src_file.suffix in ['.c', '.h', '.cpp', '.hpp']:
                    all_files.append(src_file)
        
        # Find header files
        if self.include_dir.exists():
            for inc_file in self.include_dir.glob('**/*.h'):
                all_files.append(inc_file)
        
        return sorted(all_files)
    
    def standardize_include_in_file(self, file_path):
        """Standardize include paths in a single file."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            fixed_content = content
            
            # Track all fixes applied
            fixes_applied = []
            
            # Fix pattern 1: Direct includes (e.g., polycall_error.h)
            for direct_include, correct_path in self.direct_include_map.items():
                direct_pattern = f'#include\\s+"{direct_include}"'
                if re.search(direct_pattern, fixed_content):
                    fixed_content = re.sub(direct_pattern, f'#include "{correct_path}"', fixed_content)
                    fixes_applied.append(f"Fixed direct include: {direct_include} → {correct_path}")
            
            # Fix pattern 2: Relative includes with ../
            relative_pattern = r'#include\s+"\.\./([\w/]+)/([^"]+)"'
            for match in re.finditer(relative_pattern, fixed_content):
                full_match = match.group(0)
                module = match.group(1)
                file_name = match.group(2)
                
                if module in self.module_map:
                    replacement = f'#include "{self.module_map[module]}/{file_name}"'
                    fixed_content = fixed_content.replace(full_match, replacement)
                    fixes_applied.append(f"Fixed relative include: ../{module}/{file_name}")
            
            # Fix pattern 3: Module includes (e.g., core/polycall/file.h, auth/file.h)
            for module, correct_path in self.module_map.items():
                # Handle patterns like: "core/polycall/file.h", "auth/file.h"
                pattern1 = f'#include\\s+"core/{module}/([^"]+)"'
                pattern2 = f'#include\\s+"{module}/([^"]+)"'
                
                if re.search(pattern1, fixed_content):
                    fixed_content = re.sub(pattern1, f'#include "{correct_path}/\\1"', fixed_content)
                    fixes_applied.append(f"Fixed core/{module}/ path")
                
                if re.search(pattern2, fixed_content):
                    fixed_content = re.sub(pattern2, f'#include "{correct_path}/\\1"', fixed_content)
                    fixes_applied.append(f"Fixed {module}/ path")
            
            # Fix pattern 4: Fix nested module paths (e.g., polycall/core/polycall/core/auth/file.h)
            nested_pattern = r'polycall/core/polycall/core/([^/]+)/([^"]+)'
            if re.search(nested_pattern, fixed_content):
                fixed_content = re.sub(nested_pattern, r'polycall/core/\1/\2', fixed_content)
                fixes_applied.append("Fixed nested module path")
            
            # Fix pattern 5: Fix duplicate core paths
            duplicate_core_pattern = r'#include\s+"core/core/'
            if re.search(duplicate_core_pattern, fixed_content):
                fixed_content = re.sub(duplicate_core_pattern, r'#include "core/', fixed_content)
                fixes_applied.append("Removed duplicate core/ prefix")
            
            # Fix pattern 6: Fix duplicate polycall paths
            duplicate_polycall_pattern = r'#include\s+"polycall/polycall/'
            if re.search(duplicate_polycall_pattern, fixed_content):
                fixed_content = re.sub(duplicate_polycall_pattern, r'#include "polycall/', fixed_content)
                fixes_applied.append("Removed duplicate polycall/ prefix")
            
            # Apply changes if needed
            if fixed_content != original_content:
                self.files_modified += 1
                self.include_fixes += len(fixes_applied)
                
                if self.dry_run:
                    logger.info(f"Would modify {file_path} ({len(fixes_applied)} fixes)")
                    for fix in fixes_applied:
                        logger.info(f"  - {fix}")
                else:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(fixed_content)
                    logger.info(f"Modified {file_path} ({len(fixes_applied)} fixes)")
                    for fix in fixes_applied:
                        logger.info(f"  - {fix}")
                
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def process_all_files(self):
        """Process all source files in the project."""
        all_files = self.find_all_source_files()
        logger.info(f"Found {len(all_files)} source files to process")
        
        for file_path in all_files:
            self.files_processed += 1
            self.standardize_include_in_file(file_path)
            
            # Log progress for large codebases
            if self.files_processed % 100 == 0:
                logger.info(f"Processed {self.files_processed}/{len(all_files)} files")
        
        # Log summary
        logger.info(f"Process completed. Statistics:")
        logger.info(f"  Files processed: {self.files_processed}")
        logger.info(f"  Files modified: {self.files_modified}")
        logger.info(f"  Include fixes applied: {self.include_fixes}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Standardizer")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--no-backup", action="store_true", help="Skip backup creation")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    
    standardizer = IncludePathStandardizer(
        root_dir=args.project_root,
        backup=not args.no_backup,
        dry_run=args.dry_run
    )
    
    standardizer.process_all_files()

if __name__ == "__main__":
    main()