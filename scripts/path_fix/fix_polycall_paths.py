#!/usr/bin/env python3
"""
LibPolyCall Targeted Path Fixer

This script specifically targets the "polycall/" path pattern issues identified
in the validation output. It resolves cases where paths start with "polycall/" 
but are missing the correct module hierarchy prefix.

Usage:
  python fix_polycall_paths.py --project-root /path/to/libpolycall
                              [--dry-run]
                              [--verbose]

Author: Implementation for OBINexusComputing based on include standardization requirements
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-path-fixer')

class PolycallPathFixer:
    """Fixes polycall/ path pattern issues in LibPolyCall code."""
    
    def __init__(self, project_root: str, dry_run: bool = False, verbose: bool = False):
        self.project_root = Path(project_root)
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Project structure paths
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.paths_fixed = 0
        
        # Module mapping
        self.module_map = {
            # Map submodule names to their correct path prefixes
            'auth': 'polycall/core/auth',
            'cli': 'polycall/cli',
            'config': 'polycall/core/config',
            'context': 'polycall/core/context',
            'error': 'polycall/core/error',
            'memory': 'polycall/core/memory',
            'polycall': 'polycall/core/polycall',
            'micro': 'polycall/core/micro',
            'edge': 'polycall/core/edge',
            'ffi': 'polycall/core/ffi',
            'protocol': 'polycall/core/protocol',
            'network': 'polycall/core/network',
            'parser': 'polycall/core/parser',
            'telemetry': 'polycall/core/telemetry',
        }
    
    def find_source_files(self) -> List[Path]:
        """Find all C/C++ source and header files in the project."""
        source_files = []
        
        # Search in src directory
        if self.src_dir.exists():
            for ext in ['.c', '.h', '.cpp', '.hpp']:
                source_files.extend(self.src_dir.glob(f"**/*{ext}"))
        
        # Search in include directory
        if self.include_dir.exists():
            for ext in ['.h', '.hpp']:
                source_files.extend(self.include_dir.glob(f"**/*{ext}"))
        
        return sorted(source_files)
    
    def fix_polycall_paths(self, file_path: Path) -> bool:
        """
        Fix problematic 'polycall/' path patterns.
        Returns True if file was modified.
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            modified_content = content
            
            # Find all include statements with quotes that contain 'polycall/'
            include_pattern = re.compile(r'#\s*include\s+"polycall/([^"]+)"')
            
            # Process all matches
            matches = include_pattern.findall(content)
            fixes_applied = 0
            
            for match in matches:
                # Check if this is a direct 'polycall/' path without proper module hierarchy
                if not re.match(r'(core|cli)/', match):
                    # Identify the module from the path
                    parts = match.split('/')
                    if parts:
                        first_component = parts[0]
                        
                        # Direct match to module map
                        if first_component in self.module_map:
                            old_path = f"polycall/{match}"
                            correct_prefix = self.module_map[first_component]
                            new_path = f"{correct_prefix}/{'/'.join(parts[1:])}"
                            
                            # Create pattern to replace this specific include
                            pattern = f'#include "{re.escape(old_path)}"'
                            replacement = f'#include "{new_path}"'
                            
                            # Apply replacement
                            new_content = modified_content.replace(pattern, replacement)
                            if new_content != modified_content:
                                modified_content = new_content
                                fixes_applied += 1
                                logger.debug(f"Fixed polycall/ path in {file_path}: {old_path} → {new_path}")
                        else:
                            # Default case - path starts with polycall/ but doesn't match a known module
                            old_path = f"polycall/{match}"
                            new_path = f"polycall/core/{match}"
                            
                            # Create pattern to replace this specific include
                            pattern = f'#include "{re.escape(old_path)}"'
                            replacement = f'#include "{new_path}"'
                            
                            # Apply replacement
                            new_content = modified_content.replace(pattern, replacement)
                            if new_content != modified_content:
                                modified_content = new_content
                                fixes_applied += 1
                                logger.debug(f"Fixed polycall/ path in {file_path}: {old_path} → {new_path}")
            
            # Write changes if needed
            if modified_content != original_content and fixes_applied > 0:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                
                self.files_modified += 1
                self.paths_fixed += fixes_applied
                logger.info(f"Fixed {fixes_applied} polycall/ path issues in {file_path}")
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def run(self):
        """Run path fixing across all files."""
        source_files = self.find_source_files()
        logger.info(f"Found {len(source_files)} source files to process")
        
        for file_path in source_files:
            self.files_processed += 1
            self.fix_polycall_paths(file_path)
            
            # Log progress periodically
            if self.files_processed % 50 == 0:
                logger.info(f"Processed {self.files_processed}/{len(source_files)} files")
        
        # Print summary
        logger.info("\n=== Fix Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Paths fixed: {self.paths_fixed}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Targeted Path Fixer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    fixer = PolycallPathFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    fixer.run()

if __name__ == "__main__":
    main()