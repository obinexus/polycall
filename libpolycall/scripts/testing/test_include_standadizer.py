#!/usr/bin/env python3
"""
LibPolyCall Test Include Standardizer

This script specifically handles standardizing include paths in test files,
ensuring proper mapping between test files in tests/unit/ and their
corresponding implementation files in src/core/.

It works alongside the existing standardize_includes.py but focuses
exclusively on test-specific include patterns.
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Set, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-test-standardizer')

class TestIncludeStandardizer:
    """Standardizes include paths in LibPolyCall test files."""
    
    def __init__(self, project_root: str, verbose: bool = False, 
                 dry_run: bool = False):
        self.project_root = Path(project_root)
        self.tests_dir = self.project_root / "tests" / "unit"
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.includes_modified = 0
        
        # Initialize patterns
        self._init_patterns()
    
    def _init_patterns(self):
        """Initialize patterns for test-specific include standardization."""
        # For mapping component source files to test includes
        self.component_map = {
            "polycall": "polycall/core/polycall",
            "auth": "polycall/core/auth",
            "protocol": "polycall/core/protocol",
            "ffi": "polycall/core/ffi",
            "edge": "polycall/core/edge",
            "micro": "polycall/core/micro",
            "telemetry": "polycall/core/telemetry",
            "network": "polycall/core/network",
            "config": "polycall/core/config",
        }
        
        # Include patterns to standardize
        self.test_patterns = [
            # Convert relative paths to standard paths
            (r"^\.\.\/\.\.\/src\/core\/([^\/]+)\/(.+\.h)$", 
             lambda m: f"{self.component_map.get(m.group(1), f'polycall/core/{m.group(1)}')}/{m.group(2)}"),
            
            # Convert direct component references
            (r"^([^\/]+)\/(.+\.h)$", 
             lambda m: f"{self.component_map.get(m.group(1), f'polycall/core/{m.group(1)}')}/{m.group(2)}"),
            
            # Fix mock includes
            (r"^mock_(.+)\.h$", 
             lambda m: f"tests/mocks/mock_{m.group(1)}.h"),
            
            # Fix test framework includes
            (r"^unit_tests?_framework\.h$", 
             lambda m: f"tests/framework/unit_test_framework.h"),
        ]
        
        # Direct mappings for common test headers
        self.test_header_map = {
            "unit_tests_framework.h": "tests/framework/unit_test_framework.h",
            "unit_test_framework.h": "tests/framework/unit_test_framework.h",
            "mock_core.h": "tests/mocks/mock_core.h",
            "test_utils.h": "tests/utils/test_utils.h",
        }
    
    def find_test_files(self) -> List[Path]:
        """Find all test files in the tests/unit directory."""
        test_files = []
        
        if self.tests_dir.exists():
            for file_ext in ['.c', '.cpp', '.h']:
                for test_file in self.tests_dir.glob(f'**/*{file_ext}'):
                    test_files.append(test_file)
        
        return sorted(test_files)
    
    def standardize_include_path(self, include_path: str, test_file: Path) -> str:
        """
        Convert a non-standard include path to the standard format for test files.
        Takes into account the test file's location to correctly resolve relative paths.
        """
        # Check direct mappings first
        if include_path in self.test_header_map:
            return self.test_header_map[include_path]
        
        # Determine the component based on the test file path
        rel_path = test_file.relative_to(self.tests_dir)
        parts = list(rel_path.parts)
        component = parts[0] if parts else None
        
        # Try to infer the component from the file path if possible
        if not component and len(parts) > 0:
            # Extract component from test filename (e.g., test_polycall_core.c -> polycall)
            file_name = parts[-1]
            if file_name.startswith("test_"):
                component_match = re.match(r'test_([^_]+)_?.*\.', file_name)
                if component_match:
                    component = component_match.group(1)
        
        # Apply transformation patterns
        for pattern, replacement in self.test_patterns:
            match = re.match(pattern, include_path)
            if match:
                if callable(replacement):
                    return replacement(match)
                else:
                    return re.sub(pattern, replacement, include_path)
        
        # If we couldn't transform, try to add the component prefix
        if component and not include_path.startswith("polycall/"):
            standard_component = self.component_map.get(component, f"polycall/core/{component}")
            return f"{standard_component}/{include_path}"
        
        return include_path
    
    def process_test_file(self, file_path: Path) -> bool:
        """
        Process a single test file to standardize include paths.
        Returns True if file was modified.
        """
        logger.debug(f"Processing test file: {file_path}")
        self.files_processed += 1
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            
            # Find all include statements
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            matches = include_pattern.findall(content)
            
            if not matches:
                return False
            
            # Process each include
            changes_made = False
            for include_path in matches:
                # Skip system includes
                if include_path.startswith("<") and include_path.endswith(">"):
                    continue
                    
                # Standardize the path
                standardized_path = self.standardize_include_path(include_path, file_path)
                
                if standardized_path != include_path:
                    # Create pattern with proper escaping for replacement
                    old_include = f'#include "{include_path}"'
                    new_include = f'#include "{standardized_path}"'
                    
                    content = content.replace(old_include, new_include)
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
        """Run the standardization process across all test files."""
        test_files = self.find_test_files()
        logger.info(f"Found {len(test_files)} test files to process")
        
        if not test_files:
            logger.error("No test files found to process")
            return False
        
        for file_path in test_files:
            self.process_test_file(file_path)
        
        # Print summary
        logger.info("\n=== Standardization Report ===")
        logger.info(f"Test files processed: {self.files_processed}")
        logger.info(f"Test files modified: {self.files_modified}")
        logger.info(f"Includes modified: {self.includes_modified}")
        
        if self.dry_run:
            logger.info("\nThis was a dry run. No files were actually modified.")
        
        return True

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Test Include Standardizer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    standardizer = TestIncludeStandardizer(
        project_root=args.project_root,
        verbose=args.verbose,
        dry_run=args.dry_run
    )
    
    result = standardizer.run()
    return 0 if result else 1

if __name__ == "__main__":
    sys.exit(main())