#!/usr/bin/env python3
"""
LibPolyCall Nested Path Include Corrector

This script targets and fixes a specific nested include path pattern that's causing build failures.
It focuses on fixing deeply nested paths that need more targeted resolution beyond the standard
path fixing script.

Usage:
  python fix_nested_paths.py --project-root /path/to/libpolycall [--dry-run] [--verbose]

"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-nested-path-fixer')

class NestedPathFixer:
    """Fixes nested module path patterns in include statements."""
    
    def __init__(self, project_root: str, dry_run: bool = False, verbose: bool = False):
        self.project_root = Path(project_root)
        self.include_dir = self.project_root / "include"
        self.src_dir = self.project_root / "src"
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.includes_fixed = 0
        
        # Specific pattern replacements for nested paths
        self.nested_patterns = [
            # Fix nested core/polycall/core paths with module-specific replacements
            (r'polycall/core/polycall/core/auth/([^"]+)', r'polycall/core/auth/\1'),
            (r'polycall/core/polycall/core/config/([^"]+)', r'polycall/core/config/\1'),
            (r'polycall/core/polycall/core/edge/([^"]+)', r'polycall/core/edge/\1'),
            (r'polycall/core/polycall/core/ffi/([^"]+)', r'polycall/core/ffi/\1'),
            (r'polycall/core/polycall/core/micro/([^"]+)', r'polycall/core/micro/\1'),
            (r'polycall/core/polycall/core/network/([^"]+)', r'polycall/core/network/\1'),
            (r'polycall/core/polycall/core/protocol/([^"]+)', r'polycall/core/protocol/\1'),
            (r'polycall/core/polycall/core/telemetry/([^"]+)', r'polycall/core/telemetry/\1'),
            
            # Generic pattern for core/polycall/core module
            (r'polycall/core/polycall/core/([^/]+)/([^"]+)', r'polycall/core/\1/\2'),
            
            # Fix cases where polycall_X is incorrectly placed under a different module
            (r'polycall/core/polycall/core/polycall_([^"]+)', r'polycall/core/polycall/polycall_\1'),
            
            # Fix module placeholders directly under polycall
            (r'polycall/auth/([^"]+)', r'polycall/core/auth/\1'),
            (r'polycall/config/([^"]+)', r'polycall/core/config/\1'),
            (r'polycall/network/([^"]+)', r'polycall/core/network/\1'),
            (r'polycall/ffi/([^"]+)', r'polycall/core/ffi/\1'),
            
            # Fix recursive nesting by flattening to direct paths
            (r'core/core/([^"]+)', r'core/\1'),
            (r'polycall/polycall/([^"]+)', r'polycall/\1'),
            
            # Fix direct polycall_*.h references
            (r'"polycall_([^"]+\.h)"', r'"polycall/core/polycall/polycall_\1"'),
            
            # Handle specific auth references  
            (r'"polycall_auth_([^"]+\.h)"', r'"polycall/core/auth/polycall_auth_\1"'),
            
            # Specifically targeting the polycall_auth_context.h issue
            (r'"../polycall/polycall_([^"]+)"', r'"polycall/core/polycall/polycall_\1"'),
        ]
    
    def find_all_files(self) -> list:
        """Find all source and header files in the project."""
        all_files = []
        
        # Find header files in include directory
        if self.include_dir.exists():
            for header_file in self.include_dir.glob('**/*.h'):
                all_files.append(header_file)
        
        # Find source files in src directory
        if self.src_dir.exists():
            for source_file in self.src_dir.glob('**/*.c'):
                all_files.append(source_file)
            for source_file in self.src_dir.glob('**/*.cpp'):
                all_files.append(source_file)
        
        return sorted(all_files)
    
    def fix_file(self, file_path: Path) -> bool:
        """
        Fix nested include paths in a single file.
        Returns True if the file was modified.
        """
        logger.debug(f"Processing file: {file_path}")
        self.files_processed += 1
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            modified_content = content
            
            # Track modifications
            changes_made = False
            fixes = []
            
            # Find all include statements
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            include_matches = include_pattern.findall(content)
            
            for include_path in include_matches:
                # Save original for comparison
                original_include = include_path
                fixed_include = include_path
                
                # Apply each pattern replacement in sequence
                for pattern, replacement in self.nested_patterns:
                    if re.search(pattern, fixed_include):
                        fixed_include = re.sub(pattern, replacement, fixed_include)
                        
                # Only proceed if the path actually changed
                if fixed_include != original_include:
                    # Create pattern to match the full include statement
                    pattern_in_file = f'#include\\s+"{re.escape(original_include)}"'
                    replacement_in_file = f'#include "{fixed_include}"'
                    
                    # Apply the replacement
                    new_content = re.sub(pattern_in_file, replacement_in_file, modified_content)
                    if new_content != modified_content:
                        modified_content = new_content
                        changes_made = True
                        self.includes_fixed += 1
                        fixes.append(f"{original_include} → {fixed_include}")
            
            # Write changes if modified
            if changes_made and modified_content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    self.files_modified += 1
                    logger.info(f"Fixed {len(fixes)} nested path patterns in {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would fix {len(fixes)} nested path patterns in {file_path}")
                
                # Log the specific fixes
                for fix in fixes:
                    logger.debug(f"  Fixed: {fix}")
                
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def run(self) -> bool:
        """Run the nested path fixer on all files."""
        files = self.find_all_files()
        logger.info(f"Found {len(files)} files to process")
        
        for file_path in files:
            self.fix_file(file_path)
        
        # Print summary
        logger.info("\n=== Nested Path Fixer Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Includes fixed: {self.includes_fixed}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
        
        return self.includes_fixed > 0

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Nested Path Include Corrector")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    fixer = NestedPathFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    success = fixer.run()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())