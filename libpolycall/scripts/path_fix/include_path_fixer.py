#!/usr/bin/env python3
"""
LibPolyCall Enhanced Include Path Fixer - Modified for Root Prefix Recognition

This script performs comprehensive analysis and correction of non-standard include paths
in the LibPolyCall codebase, with particular focus on correcting paths that previously
worked with a direct "core/" or "cli/" prefix.

Usage:
  python fix_includes.py --project-root /path/to/project 
                       [--dry-run] 
                       [--verbose]
                       [--fix-polycall-paths]

Author: Modified from Nnamdi Okpala's design (OBINexusComputing)
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
logger = logging.getLogger('libpolycall-include-fixer')

# Define include path conventions for LibPolyCall with ROOT_PREFIX adjustment
ROOT_PREFIX = "polycall/"

# Define direct replacements for common patterns
DIRECT_REPLACEMENTS = {
    # Fix duplicate paths with core/polycall repetition
    r'polycall/core/polycall/core/polycall/': r'polycall/core/polycall/',
    r'polycall/core/polycall/core/': r'polycall/core/',
    
    # Fix extreme case with triple polycall
    r'polycall/core/polycall/core/polycall/polycall/': r'polycall/core/polycall/',
    
    # Standard conversions for direct module includes
    r'"core/([^"]+)"': r'"polycall/core/\1"',
    r'"cli/([^"]+)"': r'"polycall/cli/\1"',
    
    # Fix any remaining duplicate segments
    r'polycall/polycall/': r'polycall/',
    r'core/core/': r'core/',
    
    # Direct common polycall header files
    r'"polycall_core\.h"': r'"polycall/core/polycall/polycall_core.h"',
    r'"polycall_error\.h"': r'"polycall/core/polycall/polycall_error.h"',
    r'"polycall_memory\.h"': r'"polycall/core/polycall/polycall_memory.h"',
    r'"polycall_context\.h"': r'"polycall/core/polycall/polycall_context.h"',
}

class EnhancedIncludePathFixer:
    """Fixes non-standard include paths in the LibPolyCall codebase with advanced analysis."""
    
    def __init__(self, project_root: str, dry_run: bool = False, 
                 verbose: bool = False, fix_polycall_paths: bool = False):
        self.project_root = Path(project_root)
        self.dry_run = dry_run
        self.fix_polycall_paths = fix_polycall_paths
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Project structure paths
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.issues_fixed = 0
        self.polycall_paths_fixed = 0
        
        # Error tracking
        self.problematic_files = []
    
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
    
    def fix_include_paths(self, file_path: Path) -> int:
        """
        Fix include paths in a single file based on defined patterns.
        Returns the number of issues fixed.
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            new_content = content
            
            # Find all include statements with quotes
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            
            # Collect all include statements for analysis
            includes = include_pattern.findall(content)
            fixes_applied = 0
            
            # Process each include statement using direct replacements
            for pattern, replacement_pattern in DIRECT_REPLACEMENTS.items():
                # Skip patterns that don't apply to include statements
                if 'include' not in pattern and not pattern.startswith('"'):
                    # Apply to entire content
                    new_content_iteration = re.sub(pattern, replacement_pattern, new_content)
                    if new_content_iteration != new_content:
                        fixes_applied += new_content.count(pattern.replace('\\', ''))
                        new_content = new_content_iteration
                        logger.debug(f"Applied pattern replacement in {file_path}: {pattern} → {replacement_pattern}")
                else:
                    # For include statements with patterns
                    for include_path in includes:
                        # Create pattern that matches this specific include inside #include
                        modified_pattern = pattern.replace('"', '')
                        if re.search(modified_pattern, include_path):
                            old_include = f'#include "{include_path}"'
                            # Remove escape characters for replacement
                            clean_pattern = pattern.replace('\\', '')
                            clean_replacement = replacement_pattern.replace('\\', '')
                            
                            # Apply the replacement
                            new_include_path = re.sub(modified_pattern, clean_replacement, include_path)
                            new_include = f'#include "{new_include_path}"'
                            
                            # Only apply if actually changing something
                            if old_include != new_include:
                                new_content = new_content.replace(old_include, new_include)
                                fixes_applied += 1
                                logger.debug(f"Fixed include in {file_path}: {include_path} → {new_include_path}")
            
            # Check if changes were made
            if new_content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(new_content)
                
                self.files_modified += 1
                self.issues_fixed += fixes_applied
                
                logger.info(f"{'[DRY RUN] Would fix' if self.dry_run else 'Fixed'} {fixes_applied} include paths in {file_path}")
            
            return fixes_applied
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            self.problematic_files.append((str(file_path), str(e)))
            return 0
    
    def process_file(self, file_path: Path) -> None:
        """Process a single file with all available fixers."""
        self.files_processed += 1
        
        # Apply fixes to this file
        issues_fixed = self.fix_include_paths(file_path)
        
        # Log progress periodically
        if self.files_processed % 50 == 0:
            logger.info(f"Processed {self.files_processed} files")
    
    def run(self):
        """Run all include path fixers on the codebase."""
        source_files = self.find_source_files()
        logger.info(f"Found {len(source_files)} source files to process")
        
        for file_path in source_files:
            self.process_file(file_path)
        
        # Print summary
        logger.info("\n=== Fix Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Issues fixed: {self.issues_fixed}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
        
        # Report problematic files
        if self.problematic_files:
            logger.info("\n=== Problematic Files ===")
            for file_path, error in self.problematic_files:
                logger.info(f"  {file_path}: {error}")

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Enhanced Include Path Fixer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    parser.add_argument("--fix-polycall-paths", action="store_true", 
                        help="Fix polycall/ paths missing correct module hierarchy")
    
    args = parser.parse_args()
    
    fixer = EnhancedIncludePathFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose,
        fix_polycall_paths=args.fix_polycall_paths
    )
    
    fixer.run()

if __name__ == "__main__":
    main()