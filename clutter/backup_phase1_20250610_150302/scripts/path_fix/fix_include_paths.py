#!/usr/bin/env python3
"""
LibPolyCall Include Path Standardizer

This script standardizes include paths in header files within the LibPolyCall project.
It specifically targets:
1. Removing non-standard includes
2. Converting to absolute paths from project root
3. Validating and checking updates

Author: Based on Nnamdi Okpala's design (OBINexusComputing)
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Set, Tuple

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-include-standardizer')

class IncludePathStandardizer:
    """Standardizes include paths in LibPolyCall header files."""
    
    def __init__(self, project_root: str, dry_run: bool = False, verbose: bool = False):
        self.project_root = Path(project_root)
        self.include_dir = self.project_root / "include"
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.includes_modified = 0
        
        # Define standard include path patterns
        self.valid_prefixes = [
            "polycall/core/polycall/",
            "polycall/core/auth/",
            "polycall/core/config/",
            "polycall/core/edge/",
            "polycall/core/ffi/",
            "polycall/core/micro/",
            "polycall/core/network/",
            "polycall/core/protocol/",
            "polycall/core/telemetry/",
            "polycall/cli/"
        ]
        
        # Non-standard path patterns to fix
        self.path_fixes = [
            # Fix duplicate nested paths
            (r"polycall/core/polycall/core/polycall/", r"polycall/core/polycall/"),
            (r"polycall/core/polycall/core/", r"polycall/core/"),
            (r"core/core/", r"core/"),
            (r"polycall/polycall/", r"polycall/"),
            
            # Fix incorrect module placement
            (r"polycall/core/polycall/auth/", r"polycall/core/auth/"),
            (r"polycall/core/polycall/config/", r"polycall/core/config/"),
            (r"polycall/core/polycall/edge/", r"polycall/core/edge/"),
            (r"polycall/core/polycall/ffi/", r"polycall/core/ffi/"),
            (r"polycall/core/polycall/micro/", r"polycall/core/micro/"),
            (r"polycall/core/polycall/network/", r"polycall/core/network/"),
            (r"polycall/core/polycall/protocol/", r"polycall/core/protocol/"),
            (r"polycall/core/polycall/telemetry/", r"polycall/core/telemetry/"),
            (r"polycall/core/polycall/cli/", r"polycall/cli/"),
            
            # Convert from core/* to polycall/core/*
            (r"^core/polycall/", r"polycall/core/polycall/"),
            (r"^core/auth/", r"polycall/core/auth/"),
            (r"^core/config/", r"polycall/core/config/"),
            (r"^core/edge/", r"polycall/core/edge/"),
            (r"^core/ffi/", r"polycall/core/ffi/"),
            (r"^core/micro/", r"polycall/core/micro/"),
            (r"^core/network/", r"polycall/core/network/"),
            (r"^core/protocol/", r"polycall/core/protocol/"),
            (r"^core/telemetry/", r"polycall/core/telemetry/"),
            
            # Convert from direct module to polycall/core/*
            (r"^auth/", r"polycall/core/auth/"),
            (r"^config/", r"polycall/core/config/"),
            (r"^edge/", r"polycall/core/edge/"),
            (r"^ffi/", r"polycall/core/ffi/"),
            (r"^micro/", r"polycall/core/micro/"),
            (r"^network/", r"polycall/core/network/"),
            (r"^protocol/", r"polycall/core/protocol/"),
            (r"^telemetry/", r"polycall/core/telemetry/"),
            (r"^cli/", r"polycall/cli/"),
            
            # Fix direct includes
            (r"^polycall_([^/]+\.h)$", r"polycall/core/polycall/polycall_\1"),
        ]
    
    def find_header_files(self) -> List[Path]:
        """Find all header files in the include directory."""
        if not self.include_dir.exists():
            logger.error(f"Include directory not found: {self.include_dir}")
            return []
        
        header_files = list(self.include_dir.glob("**/*.h"))
        logger.info(f"Found {len(header_files)} header files")
        return header_files
    
    def is_valid_include(self, include_path: str) -> bool:
        """Check if an include path follows standard patterns."""
        # System includes are always valid
        if include_path.startswith("<") and include_path.endswith(">"):
            return True
        
        # Check against valid prefixes
        for prefix in self.valid_prefixes:
            if include_path.startswith(prefix):
                return True
        
        return False
    
    def standardize_include_path(self, include_path: str) -> str:
        """Convert a non-standard include path to the standard format."""
        fixed_path = include_path
        
        # Apply all path fixes in sequence
        for pattern, replacement in self.path_fixes:
            if re.match(pattern, fixed_path):
                prev_path = fixed_path
                fixed_path = re.sub(pattern, replacement, fixed_path)
                if prev_path != fixed_path:
                    logger.debug(f"Fixed: {prev_path} → {fixed_path}")
        
        return fixed_path
    
    def process_file(self, file_path: Path) -> bool:
        """Process a single header file to standardize include paths."""
        logger.debug(f"Processing file: {file_path}")
        self.files_processed += 1
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # Find all include statements
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            matches = include_pattern.findall(content)
            
            if not matches:
                logger.debug(f"No includes found in {file_path}")
                return False
            
            # Process and standardize each include path
            changes_made = False
            for include_path in matches:
                if not self.is_valid_include(include_path):
                    standardized_path = self.standardize_include_path(include_path)
                    
                    # Only replace if the path was actually changed
                    if standardized_path != include_path:
                        # Create pattern with proper escaping for replacement
                        pattern = re.escape(f'#include "{include_path}"')
                        replacement = f'#include "{standardized_path}"'
                        content = re.sub(pattern, replacement, content)
                        changes_made = True
                        self.includes_modified += 1
                        logger.debug(f"  Replaced: {include_path} → {standardized_path}")
            
            # Save changes if modified
            if changes_made and content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    self.files_modified += 1
                    logger.info(f"Updated includes in {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would update includes in {file_path}")
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def run(self) -> bool:
        """Run the standardization process on all header files."""
        header_files = self.find_header_files()
        if not header_files:
            logger.error("No header files found")
            return False
        
        for file_path in header_files:
            self.process_file(file_path)
        
        # Print summary
        logger.info("=== Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Includes modified: {self.includes_modified}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
        
        return True
    
    def validate_includes(self) -> bool:
        """Validate all includes to ensure they follow standard patterns."""
        logger.info("Validating include paths...")
        header_files = self.find_header_files()
        
        valid = True
        issues_found = 0
        
        for file_path in header_files:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Find all include statements
                include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
                matches = include_pattern.findall(content)
                
                # Check each include path
                for include_path in matches:
                    if not self.is_valid_include(include_path):
                        logger.warning(f"Invalid include in {file_path}: {include_path}")
                        valid = False
                        issues_found += 1
            
            except Exception as e:
                logger.error(f"Error validating {file_path}: {e}")
                valid = False
        
        if valid:
            logger.info("All includes follow standard patterns")
        else:
            logger.warning(f"Found {issues_found} non-standard includes")
        
        return valid

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Standardizer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    parser.add_argument("--validate-only", action="store_true", help="Only validate includes without fixing")
    
    args = parser.parse_args()
    
    standardizer = IncludePathStandardizer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    if args.validate_only:
        success = standardizer.validate_includes()
    else:
        success = standardizer.run()
        if success:
            # Validate after fixing
            standardizer.validate_includes()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())