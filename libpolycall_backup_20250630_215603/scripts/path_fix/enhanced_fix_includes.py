#!/usr/bin/env python3
"""
LibPolyCall Enhanced Include Path Fixer

This script performs comprehensive analysis and correction of non-standard include paths
in the LibPolyCall codebase according to established module hierarchy conventions.

Usage:
  python enhanced_fix_includes.py --project-root /path/to/project 
                                 [--dry-run] 
                                 [--verbose]
                                 [--fix-polycall-paths]
                                 [--analyze-only]

Author: Implementation based on Nnamdi Okpala's design (OBINexusComputing)
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

# Define include path conventions for LibPolyCall
INCLUDE_CONVENTIONS = {
    # Standard module paths
    "polycall/core/polycall/": r"polycall/core/polycall/",
    "polycall/core/auth/": r"polycall/core/auth/",
    "polycall/core/config/": r"polycall/core/config/",
    "polycall/core/edge/": r"polycall/core/edge/",
    "polycall/core/ffi/": r"polycall/core/ffi/",
    "polycall/core/micro/": r"polycall/core/micro/",
    "polycall/core/network/": r"polycall/core/network/",
    "polycall/core/protocol/": r"polycall/core/protocol/",
    "polycall/core/telemetry/": r"polycall/core/telemetry/",
    "polycall/cli/": r"polycall/cli/",
    
    # Common patterns to fix
    "core/polycall/": r"polycall/core/polycall/",
    "core/auth/": r"polycall/core/auth/",
    "core/config/": r"polycall/core/config/",
    "core/edge/": r"polycall/core/edge/",
    "core/ffi/": r"polycall/core/ffi/",
    "core/micro/": r"polycall/core/micro/",
    "core/network/": r"polycall/core/network/",
    "core/protocol/": r"polycall/core/protocol/",
    "core/telemetry/": r"polycall/core/telemetry/",
    "cli/": r"polycall/cli/",
    
    # Specific fixes for direct includes
    "polycall_core.h": r"polycall/core/polycall/polycall_core.h",
    "polycall_error.h": r"polycall/core/polycall/polycall_error.h",
    "polycall_context.h": r"polycall/core/polycall/polycall_context.h",
    "polycall_memory.h": r"polycall/core/polycall/polycall_memory.h",
}

class EnhancedIncludePathFixer:
    """Fixes non-standard include paths in the LibPolyCall codebase with advanced analysis."""
    
    def __init__(self, project_root: str, dry_run: bool = False, 
                 verbose: bool = False, fix_polycall_paths: bool = False,
                 analyze_only: bool = False):
        self.project_root = Path(project_root)
        self.dry_run = dry_run
        self.fix_polycall_paths = fix_polycall_paths
        self.analyze_only = analyze_only
        
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
        self.error_patterns = {}
    
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
    
    def fix_basic_include_paths(self, file_path: Path) -> int:
        """
        Fix include paths in a single file based on standard patterns.
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
            
            # Process each include statement
            for include_path in includes:
                fixed_path = None
                
                # Apply fixes based on known patterns
                for pattern, replacement_prefix in INCLUDE_CONVENTIONS.items():
                    if include_path.startswith(pattern):
                        continue  # Path already follows convention
                    
                    # For direct includes (like polycall_core.h), replace the entire path
                    if pattern == include_path:
                        fixed_path = replacement_prefix
                        break
                    
                    # For path prefixes, only fix if they match exactly at beginning
                    if include_path == pattern or include_path.startswith(pattern):
                        # Extract the part after the pattern
                        suffix = include_path[len(pattern):]
                        fixed_path = f"{replacement_prefix}{suffix}"
                        break
                
                # Apply fix if one was identified
                if fixed_path and fixed_path != include_path:
                    logger.debug(f"Fixing include in {file_path}: {include_path} → {fixed_path}")
                    
                    # Create pattern that matches this specific include
                    pattern_str = f'#\\s*include\\s+"{re.escape(include_path)}"'
                    replacement = f'#include "{fixed_path}"'
                    
                    # Apply the replacement
                    new_content = re.sub(pattern_str, replacement, new_content)
                    fixes_applied += 1
            
            # Check if changes were made
            if new_content != original_content and not self.analyze_only:
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
    
    def fix_polycall_path_issues(self, file_path: Path) -> int:
        """
        Fix polycall/ path issues - specifically for non-standard module paths.
        Returns the number of fixes applied.
        """
        if not self.fix_polycall_paths:
            return 0
            
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            
            # Find problematic 'polycall/' includes that don't have core/ or cli/
            include_pattern = re.compile(r'#include\s+"(polycall/[^"]+)"')
            fixes_applied = 0
            
            # Process all matches and collect replacements to apply
            replacements = []
            for match in include_pattern.finditer(content):
                include_path = match.group(1)
                # Check if this is a direct 'polycall/' path without proper module hierarchy
                if not re.match(r'polycall/(core|cli)/', include_path):
                    correct_path = re.sub(r'^polycall/', r'polycall/core/', include_path)
                    replacements.append((include_path, correct_path))
            
            # Apply all replacements
            for old_path, new_path in replacements:
                pattern = f'#include "{re.escape(old_path)}"'
                replacement = f'#include "{new_path}"'
                content = re.sub(pattern, replacement, content)
                fixes_applied += 1
                logger.debug(f"Fixed polycall/ path in {file_path}: {old_path} → {new_path}")
            
            # Write changes if needed
            if content != original_content and fixes_applied > 0 and not self.analyze_only:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                
                self.polycall_paths_fixed += fixes_applied
                logger.info(f"{'[DRY RUN] Would fix' if self.dry_run else 'Fixed'} {fixes_applied} polycall/ path issues in {file_path}")
            
            return fixes_applied
            
        except Exception as e:
            logger.error(f"Error fixing polycall/ paths in {file_path}: {e}")
            self.problematic_files.append((str(file_path), str(e)))
            return 0
    
    def analyze_include_patterns(self, file_path: Path) -> Dict[str, int]:
        """
        Analyze include patterns in a file for diagnostic purposes.
        Returns a dictionary of pattern counts.
        """
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            # Find all include statements with quotes
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            includes = include_pattern.findall(content)
            
            patterns = {}
            for include_path in includes:
                # Check various pattern categories
                if include_path.startswith("polycall/core/"):
                    patterns["polycall/core/*"] = patterns.get("polycall/core/*", 0) + 1
                elif include_path.startswith("polycall/cli/"):
                    patterns["polycall/cli/*"] = patterns.get("polycall/cli/*", 0) + 1
                elif include_path.startswith("polycall/"):
                    patterns["polycall/* (non-standard)"] = patterns.get("polycall/* (non-standard)", 0) + 1
                elif include_path.startswith("core/"):
                    patterns["core/*"] = patterns.get("core/*", 0) + 1
                elif re.match(r"^\w+\.h$", include_path):
                    patterns["direct filename"] = patterns.get("direct filename", 0) + 1
                else:
                    patterns["other"] = patterns.get("other", 0) + 1
            
            return patterns
            
        except Exception as e:
            logger.error(f"Error analyzing {file_path}: {e}")
            return {"error": 1}
    
    def process_file(self, file_path: Path) -> None:
        """Process a single file with all available fixers."""
        self.files_processed += 1
        
        # First, fix basic include paths using the standard patterns
        basic_fixes = self.fix_basic_include_paths(file_path)
        
        # Then, fix polycall/ paths that don't have correct module structure
        polycall_fixes = self.fix_polycall_path_issues(file_path)
        
        # If we're in analysis mode, collect pattern information
        if self.analyze_only:
            patterns = self.analyze_include_patterns(file_path)
            for pattern, count in patterns.items():
                if pattern not in self.error_patterns:
                    self.error_patterns[pattern] = 0
                self.error_patterns[pattern] += count
    
    def run(self):
        """Run all include path fixers on the codebase."""
        source_files = self.find_source_files()
        logger.info(f"Found {len(source_files)} source files to process")
        
        if self.analyze_only:
            logger.info("Running in analysis mode - gathering statistics only")
        
        for file_path in source_files:
            self.process_file(file_path)
            
            # Log progress periodically
            if self.files_processed % 50 == 0:
                logger.info(f"Processed {self.files_processed}/{len(source_files)} files")
        
        # Print summary
        logger.info("\n=== Fix Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        
        if not self.analyze_only:
            logger.info(f"Files modified: {self.files_modified}")
            logger.info(f"Standard pattern issues fixed: {self.issues_fixed}")
            logger.info(f"Polycall path issues fixed: {self.polycall_paths_fixed}")
            logger.info(f"Total issues fixed: {self.issues_fixed + self.polycall_paths_fixed}")
        
            if self.dry_run:
                logger.info("This was a dry run. No files were actually modified.")
        else:
            # Report pattern analysis
            logger.info("\n=== Include Pattern Analysis ===")
            for pattern, count in sorted(self.error_patterns.items(), key=lambda x: x[1], reverse=True):
                logger.info(f"  {pattern}: {count} occurrences")
        
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
    parser.add_argument("--analyze-only", action="store_true",
                        help="Only analyze include patterns without making changes")
    
    args = parser.parse_args()
    
    fixer = EnhancedIncludePathFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose,
        fix_polycall_paths=args.fix_polycall_paths,
        analyze_only=args.analyze_only
    )
    
    fixer.run()

if __name__ == "__main__":
    main()