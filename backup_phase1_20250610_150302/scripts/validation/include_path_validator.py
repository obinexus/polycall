#!/usr/bin/env python3
"""
LibPolyCall Include Path Validator and Fixer

This script analyzes and fixes invalid include paths in C/C++ source files by:
1. Scanning build error logs for "No such file or directory" errors
2. Mapping incorrect include paths to correct ones based on project structure
3. Detecting and fixing paths that reference non-existent files
4. Verifying include path corrections against the actual filesystem

Usage:
  python include_path_validator.py --project-root /path/to/libpolycall --error-log build_errors.txt [--dry-run]

Author: Implementation for OBINexusComputing based on standardization requirements
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
logger = logging.getLogger('libpolycall-path-validator')

class IncludePathValidator:
    """Validates and fixes include paths in LibPolyCall codebase."""
    
    def __init__(self, project_root: str, error_log: str, dry_run: bool = False, verbose: bool = False):
        self.project_root = Path(project_root)
        self.error_log = error_log
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Project structure paths
        self.include_dir = self.project_root / "include"
        self.src_dir = self.project_root / "src"
        
        # Statistics
        self.errors_found = 0
        self.files_processed = 0
        self.files_modified = 0
        self.paths_corrected = 0
        
        # Mapping of incorrect paths to correct ones
        self.path_corrections = {}
        
        # Files with errors and their fixes
        self.file_fixes: Dict[str, Dict[str, str]] = {}  # file -> {wrong_path -> correct_path}
        
        # Initialize common path corrections based on project structure
        self._initialize_path_corrections()
        
    def _initialize_path_corrections(self):
        """Initialize path corrections based on common patterns and project structure."""
        # Common path corrections based on observed errors
        self.path_corrections = {
            # Fix core/polycall/config/* paths that should be core/config/*
            r"core/polycall/config/factory/([^\"]+)": r"core/config/factory/\1",
            r"core/polycall/config/parser/([^\"]+)": r"core/config/parser/\1",
            r"core/polycall/config/([^/\"]+)": r"core/config/\1",
            
            # Fix paths that incorrectly use polycall/ instead of core/polycall/
            r"^polycall/([^\"]+)": r"core/polycall/\1",
            
            # Fix missing core/ prefix
            r"^config/([^\"]+)": r"core/config/\1",
            r"^edge/([^\"]+)": r"core/edge/\1",
            r"^network/([^\"]+)": r"core/network/\1",
            r"^protocol/([^\"]+)": r"core/protocol/\1",
            r"^ffi/([^\"]+)": r"core/ffi/\1"
        }
        
    def parse_error_log(self):
        """Parse the compilation error log to find missing and incorrect includes."""
        logger.info(f"Parsing error log: {self.error_log}")
        
        # Pattern for missing file errors
        missing_file_pattern = re.compile(r'([^:]+):(\d+):(\d+):\s+fatal error:\s+([^:]+):\s+No such file or directory')
        
        # Pattern for unknown type errors
        unknown_type_pattern = re.compile(r'([^:]+):(\d+):(\d+):\s+error:\s+unknown type name \'([^\']+)\'')
        
        with open(self.error_log, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                # Check for missing include file errors
                missing_match = missing_file_pattern.search(line)
                if missing_match:
                    self.errors_found += 1
                    source_file = missing_match.group(1)
                    missing_include = missing_match.group(4)
                    
                    logger.debug(f"Found missing include: {missing_include} in {source_file}")
                    
                    if source_file not in self.file_fixes:
                        self.file_fixes[source_file] = {}
                    
                    # Try to find a correct path for the missing include
                    correct_path = self._find_correct_include_path(missing_include)
                    if correct_path:
                        self.file_fixes[source_file][missing_include] = correct_path
                        logger.debug(f"  Mapped to: {correct_path}")
                
                # Check for unknown type errors (might need include file)
                # This is secondary to the direct file errors
                unknown_match = unknown_type_pattern.search(line)
                if unknown_match:
                    self.errors_found += 1
                    source_file = unknown_match.group(1)
                    unknown_type = unknown_match.group(4)
                    
                    logger.debug(f"Found unknown type: {unknown_type} in {source_file}")
                    
                    # For now, we're focusing on the direct file errors
                    # We could expand this to map types to headers
        
        logger.info(f"Found {self.errors_found} include errors across {len(self.file_fixes)} files")
    
    def _find_correct_include_path(self, missing_include: str) -> Optional[str]:
        """
        Find the correct path for a missing include.
        Returns the corrected path or None if no correction is found.
        """
        # First, try to apply known path corrections
        for pattern, replacement in self.path_corrections.items():
            if re.match(pattern, missing_include):
                corrected = re.sub(pattern, replacement, missing_include)
                logger.debug(f"Applied pattern correction: {missing_include} -> {corrected}")
                
                # Verify the corrected path exists
                if self._verify_include_path(corrected):
                    return corrected
        
        # If no pattern match, try to find the file in the project
        # Strip quotes if present
        clean_path = missing_include.strip('"')
        
        # Check if the include exists in include/ directory
        include_path = self.include_dir / clean_path
        if include_path.exists():
            return clean_path
        
        # Check if it's a header in src/ directory
        src_path = self.src_dir / clean_path
        if src_path.exists():
            return clean_path
        
        # Try to find by filename only (last component)
        filename = os.path.basename(clean_path)
        
        # Search in include directory
        for found_path in self.include_dir.glob(f"**/{filename}"):
            rel_path = found_path.relative_to(self.include_dir)
            logger.debug(f"Found potential match: {rel_path}")
            return str(rel_path)
            
        # Search in src directory
        for found_path in self.src_dir.glob(f"**/{filename}"):
            rel_path = found_path.relative_to(self.src_dir)
            if found_path.suffix in ['.h', '.hpp']:
                logger.debug(f"Found potential match in src: {rel_path}")
                return str(rel_path)
        
        # Could not find a correction
        logger.debug(f"Could not find correction for: {missing_include}")
        return None
    
    def _verify_include_path(self, include_path: str) -> bool:
        """Verify that an include path exists in the project."""
        # Strip quotes if present
        clean_path = include_path.strip('"')
        
        # Check in include/ directory
        include_file = self.include_dir / clean_path
        if include_file.exists():
            return True
        
        # Check in src/ directory
        src_file = self.src_dir / clean_path
        if src_file.exists():
            return True
        
        return False
    
    def fix_files(self):
        """Fix the include paths in all files that need fixing."""
        for file_path, fixes in self.file_fixes.items():
            if not fixes:
                continue
                
            self.files_processed += 1
            self.fix_file(file_path, fixes)
    
    def fix_file(self, file_path: str, fixes: Dict[str, str]):
        """Fix include paths in a single file."""
        if not os.path.exists(file_path):
            logger.warning(f"File not found: {file_path}")
            return
            
        logger.info(f"Processing file: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            modified_content = content
            
            # Apply all fixes to the file
            for wrong_path, correct_path in fixes.items():
                # Escape special characters in the path for regex
                escaped_wrong_path = re.escape(wrong_path)
                
                # Create pattern that matches the include statement
                pattern = f'#include\\s+[<"]({escaped_wrong_path})[>"]'
                
                # Determine whether to use quotes or angle brackets
                if '<' in wrong_path and '>' in wrong_path:
                    replacement = f'#include <{correct_path}>'
                else:
                    replacement = f'#include "{correct_path}"'
                
                # Apply the replacement
                new_content = re.sub(pattern, replacement, modified_content)
                
                # If no match found, try without the escaped pattern
                if new_content == modified_content:
                    direct_pattern = f'#include\\s+"({wrong_path})"'
                    new_content = re.sub(direct_pattern, f'#include "{correct_path}"', modified_content)
                
                if new_content != modified_content:
                    logger.debug(f"  Fixed: {wrong_path} -> {correct_path}")
                    modified_content = new_content
                    self.paths_corrected += 1
            
            # Check if changes were made
            if modified_content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    self.files_modified += 1
                    logger.info(f"Fixed {len(fixes)} include paths in {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would fix {len(fixes)} include paths in {file_path}")
                    for wrong, correct in fixes.items():
                        logger.info(f"  {wrong} -> {correct}")
            else:
                logger.warning(f"No changes made to {file_path} despite identified fixes")
        
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
    
    def scan_for_header_paths(self):
        """
        Scan the project to build a map of header files and their locations.
        This helps with finding correct paths for includes.
        """
        logger.info("Scanning project for header files...")
        
        self.header_map = {}
        
        # Scan include directory
        for header_file in self.include_dir.glob("**/*.h"):
            rel_path = header_file.relative_to(self.include_dir)
            filename = header_file.name
            
            if filename not in self.header_map:
                self.header_map[filename] = []
            
            self.header_map[filename].append(str(rel_path))
            logger.debug(f"Found header: {filename} at {rel_path}")
        
        # Scan src directory
        for header_file in self.src_dir.glob("**/*.h"):
            rel_path = header_file.relative_to(self.src_dir)
            filename = header_file.name
            
            if filename not in self.header_map:
                self.header_map[filename] = []
            
            self.header_map[filename].append(str(rel_path))
            logger.debug(f"Found header in src: {filename} at {rel_path}")
        
        logger.info(f"Found {len(self.header_map)} unique header files")
        
        # Find potential duplicates (headers with the same name in different locations)
        duplicates = {name: paths for name, paths in self.header_map.items() if len(paths) > 1}
        if duplicates:
            logger.warning(f"Found {len(duplicates)} headers with multiple locations:")
            for name, paths in duplicates.items():
                logger.warning(f"  {name}: {', '.join(paths)}")
    
    def run(self):
        """Run the include path validator and fixer."""
        self.scan_for_header_paths()
        self.parse_error_log()
        self.fix_files()
        
        # Print summary
        logger.info("Finished processing. Summary:")
        logger.info(f"  Files processed: {self.files_processed}")
        logger.info(f"  Files modified: {self.files_modified}")
        logger.info(f"  Errors found: {self.errors_found}")
        logger.info(f"  Paths corrected: {self.paths_corrected}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")

def main():
    parser = argparse.ArgumentParser(
        description="LibPolyCall Include Path Validator and Fixer"
    )
    parser.add_argument(
        "--project-root", 
        required=True,
        help="Root directory of LibPolyCall project"
    )
    parser.add_argument(
        "--error-log", 
        required=True,
        help="Compilation error log file"
    )
    parser.add_argument(
        "--dry-run", 
        action="store_true",
        help="Show what would be changed without modifying files"
    )
    parser.add_argument(
        "--verbose", 
        action="store_true",
        help="Enable verbose logging"
    )
    
    args = parser.parse_args()
    
    validator = IncludePathValidator(
        project_root=args.project_root,
        error_log=args.error_log,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    validator.run()

if __name__ == "__main__":
    main()