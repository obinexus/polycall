#!/usr/bin/env python3
"""
LibPolyCall Comprehensive Include Path Fixer

This script performs a comprehensive fix of all include path issues in the LibPolyCall codebase,
specifically addressing complex nested patterns like "polycall/core/polycall/core/auth/..."
that are not correctly handled by the existing scripts.

Usage:
  python fix_all_paths.py --project-root /path/to/libpolycall
                         [--dry-run]
                         [--verbose]

Author: Implementation for OBINexusComputing
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

class ComprehensivePathFixer:
    """Comprehensively fixes all types of include path issues in LibPolyCall code."""
    
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
        
        # Problem tracking
        self.problematic_files = []
        
        # Complex patterns that need direct fixing
        self.direct_patterns = [
            # Pattern for polycall/core/polycall/core/X/...
            (r'polycall/core/polycall/core/([^"/]+)/([^"]+)', r'polycall/core/\1/\2'),
            
            # Pattern for polycall/core/polycall/polycall/...
            (r'polycall/core/polycall/polycall/([^"]+)', r'polycall/core/polycall/\1'),
            
            # Pattern for nested core segments
            (r'polycall/core/core/([^"]+)', r'polycall/core/\1'),
            (r'core/core/([^"]+)', r'core/\1'),
            
            # Pattern for duplicated polycall segments
            (r'polycall/polycall/([^"]+)', r'polycall/\1'),
            
            # Multiple consecutive polycall/core/polycall patterns (really deep nesting)
            (r'polycall/core/polycall/core/polycall/core/polycall/([^"]+)', r'polycall/core/polycall/\1'),
            (r'polycall/core/polycall/core/polycall/([^"]+)', r'polycall/core/polycall/\1'),
            
            # Direct corrections for specific modules
            (r'polycall/core/polycall/core/auth/([^"]+)', r'polycall/core/auth/\1'),
            (r'polycall/core/polycall/core/config/([^"]+)', r'polycall/core/config/\1'),
            (r'polycall/core/polycall/core/edge/([^"]+)', r'polycall/core/edge/\1'),
            (r'polycall/core/polycall/core/ffi/([^"]+)', r'polycall/core/ffi/\1'),
            (r'polycall/core/polycall/core/micro/([^"]+)', r'polycall/core/micro/\1'),
            (r'polycall/core/polycall/core/network/([^"]+)', r'polycall/core/network/\1'),
            (r'polycall/core/polycall/core/protocol/([^"]+)', r'polycall/core/protocol/\1'),
            (r'polycall/core/polycall/core/telemetry/([^"]+)', r'polycall/core/telemetry/\1'),
            
            # CLI specific corrections
            (r'polycall/core/polycall/cli/([^"]+)', r'polycall/cli/\1'),
        ]
    
    def find_source_files(self) -> List[Path]:
        """Find all C/C++ source and header files in the project."""
        source_files = []
        
        # Search in src directory
        if self.src_dir.exists():
            for ext in ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx']:
                source_files.extend(self.src_dir.glob(f"**/*{ext}"))
        
        # Search in include directory
        if self.include_dir.exists():
            for ext in ['.h', '.hpp', '.hxx']:
                source_files.extend(self.include_dir.glob(f"**/*{ext}"))
        
        return sorted(source_files)
    
    def fix_include_paths(self, file_path: Path) -> bool:
        """
        Fix all include paths in a single file.
        Returns True if file was modified.
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            fixed_content = content
            
            # Find all include statements with quotes
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            
            # Process all includes to check and fix paths
            includes = include_pattern.findall(content)
            fixes_applied = 0
            
            # Record all matches for replacement
            all_replacements = []
            
            for include_path in includes:
                fixed_path = include_path
                
                # Apply each direct pattern fix
                for pattern, replacement in self.direct_patterns:
                    # Check if this pattern applies to this path
                    if re.search(pattern, fixed_path):
                        updated_path = re.sub(pattern, replacement, fixed_path)
                        
                        if updated_path != fixed_path:
                            logger.debug(f"Fixing path in {file_path}: {fixed_path} → {updated_path}")
                            fixed_path = updated_path
                            fixes_applied += 1
                
                # If path was fixed, add to replacements list
                if fixed_path != include_path:
                    all_replacements.append((include_path, fixed_path))
            
            # Apply all replacements
            for old_path, new_path in all_replacements:
                old_include = f'#include "{old_path}"'
                new_include = f'#include "{new_path}"'
                fixed_content = fixed_content.replace(old_include, new_include)
            
            # Apply changes if needed
            if fixed_content != original_content:
                self.files_modified += 1
                self.paths_fixed += fixes_applied
                
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(fixed_content)
                    logger.info(f"Fixed {fixes_applied} paths in {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would fix {fixes_applied} paths in {file_path}")
                
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            self.problematic_files.append((str(file_path), str(e)))
            return False
    
    def run(self):
        """Run comprehensive path fixing across all files."""
        source_files = self.find_source_files()
        logger.info(f"Found {len(source_files)} source files to process")
        
        for file_path in source_files:
            self.files_processed += 1
            self.fix_include_paths(file_path)
            
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
        
        # Report problematic files
        if self.problematic_files:
            logger.warning("\n=== Problematic Files ===")
            for file_path, error in self.problematic_files:
                logger.warning(f"  {file_path}: {error}")
        
        # Next steps suggestion
        logger.info("\n=== Next Steps ===")
        logger.info("1. Run 'make validate-includes-direct' to check if all issues were fixed")
        logger.info("2. Build the project to verify the fixes resolved compilation errors")
        logger.info("3. If issues remain, consider running the script again with --verbose")

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Comprehensive Include Path Fixer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    fixer = ComprehensivePathFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    fixer.run()

if __name__ == "__main__":
    main()