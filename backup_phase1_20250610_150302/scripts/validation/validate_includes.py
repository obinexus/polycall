#!/usr/bin/env python3
"""
LibPolyCall Include Path Validator

This script validates include paths across the LibPolyCall project codebase,
checking for compliance with the project's include path standards. It can be
run from CMake or directly from the command line.

Usage:
  python validate_includes.py --root PROJECT_ROOT --standard STANDARD_NAME [--fix]

Author: Implementation based on Nnamdi Okpala's design specifications
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
logger = logging.getLogger('libpolycall-include-validator')

# Define standards
STANDARDS = {
    "polycall": {
        "valid_patterns": [
            # Standard module includes
            r"^polycall/core/polycall/.*\.h$",
            r"^polycall/core/auth/.*\.h$",
            r"^polycall/core/config/.*\.h$",
            r"^polycall/core/edge/.*\.h$",
            r"^polycall/core/ffi/.*\.h$",
            r"^polycall/core/micro/.*\.h$",
            r"^polycall/core/network/.*\.h$",
            r"^polycall/core/protocol/.*\.h$",
            r"^polycall/core/telemetry/.*\.h$",
            r"^polycall/cli/.*\.h$",
            
            # Root includes
            r"^polycall/polycall\.h$",
            r"^polycall/polycall_config\.h$",
            
            # System includes
            r"^<.*>$"
        ],
        "invalid_patterns": [
            # Missing polycall prefix
            r"^core/.*\.h$",
            r"^auth/.*\.h$",
            r"^config/.*\.h$",
            r"^protocol/.*\.h$",
            r"^network/.*\.h$",
            r"^ffi/.*\.h$",
            r"^cli/.*\.h$",
            r"^polycall_.*\.h$",
            
            # Duplicate prefixes
            r"^polycall/polycall/.*\.h$",
            r"^core/core/.*\.h$"
        ],
        "transformations": {
                    # Common transformations to fix paths
                    r"^polycall_(.*)\.h$": r"polycall/core/polycall/polycall_\1.h",
                    r"^core/polycall/(.*)\.h$": r"polycall/core/polycall/\1.h",
                    r"^ffi/(.*)\.h$": r"polycall/core/ffi/\1.h",
                    r"^protocol/(.*)\.h$": r"polycall/core/protocol/\1.h",
                    r"^config/(.*)\.h$": r"polycall/core/config/\1.h",
                    r"^network/(.*)\.h$": r"polycall/core/network/\1.h",
                    r"^auth/(.*)\.h$": r"polycall/core/auth/\1.h",
                    r"^cli/(.*)\.h$": r"polycall/cli/\1.h",
                    r"^polycall\.h$": r"polycall/polycall.h",
                    r"^src/polycall\.c$": r"src/core/polycall.c"
                }
    }
}

class IncludePathValidator:
    """Validates include paths in the LibPolyCall codebase."""
    
    def __init__(self, root_dir: str, standard: str, fix: bool = False, verbose: bool = False):
        self.root_dir = Path(root_dir)
        self.standard_name = standard
        self.fix = fix
        
        if standard not in STANDARDS:
            raise ValueError(f"Unknown standard: {standard}. Valid standards: {', '.join(STANDARDS.keys())}")
        
        self.standard = STANDARDS[standard]
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Project structure paths
        self.src_dir = self.root_dir / "src"
        self.include_dir = self.root_dir / "include"
        
        # Statistics
        self.files_processed = 0
        self.files_with_issues = 0
        self.issues_found = 0
        self.issues_fixed = 0
    
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
    
    def validate_file(self, file_path: Path) -> Tuple[bool, List[Dict]]:
        """
        Validate include paths in a single file.
        Returns (has_issues, list of issue details)
        """
        logger.debug(f"Validating file: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            # Find all include statements with quotes
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            includes = include_pattern.findall(content)
            
            issues = []
            
            for include_path in includes:
                valid = False
                
                # Check against valid patterns
                for pattern in self.standard["valid_patterns"]:
                    if re.match(pattern, include_path):
                        valid = True
                        break
                
                # Check against invalid patterns
                for pattern in self.standard["invalid_patterns"]:
                    if re.match(pattern, include_path):
                        valid = False
                        break
                
                if not valid:
                    # Try to find a fix
                    fixed_path = None
                    for pattern, replacement in self.standard["transformations"].items():
                        if re.match(pattern, include_path):
                            fixed_path = re.sub(pattern, replacement, include_path)
                            break
                    
                    issues.append({
                        "path": include_path,
                        "fixed_path": fixed_path,
                        "line_number": None  # We could calculate this if needed
                    })
            
            return len(issues) > 0, issues
        
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False, []
    
    def fix_file(self, file_path: Path, issues: List[Dict]) -> bool:
        """Fix include paths in a file based on identified issues."""
        logger.info(f"Fixing include paths in {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            fixed_content = content
            fixes_applied = 0
            
            for issue in issues:
                if issue["fixed_path"]:
                    # Escape special characters for regex
                    escaped_path = re.escape(issue["path"])
                    # Replace the problematic include
                    pattern = f'#\\s*include\\s+"({escaped_path})"'
                    replacement = f'#include "{issue["fixed_path"]}"'
                    new_content = re.sub(pattern, replacement, fixed_content)
                    
                    if new_content != fixed_content:
                        fixed_content = new_content
                        fixes_applied += 1
                        logger.debug(f"  Fixed: {issue['path']} → {issue['fixed_path']}")
            
            if fixes_applied > 0:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(fixed_content)
                self.issues_fixed += fixes_applied
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error fixing {file_path}: {e}")
            return False
    
    def validate_codebase(self) -> bool:
        """
        Validate all source files and optionally fix issues.
        Returns True if all files comply with the standard.
        """
        source_files = self.find_source_files()
        logger.info(f"Found {len(source_files)} source files to validate")
        
        all_valid = True
        
        for file_path in source_files:
            self.files_processed += 1
            has_issues, issues = self.validate_file(file_path)
            
            if has_issues:
                all_valid = False
                self.files_with_issues += 1
                self.issues_found += len(issues)
                
                logger.warning(f"Include path issues in {file_path}:")
                for issue in issues:
                    if issue["fixed_path"]:
                        logger.warning(f"  - Invalid: {issue['path']} → Should be: {issue['fixed_path']}")
                    else:
                        logger.warning(f"  - Invalid: {issue['path']} → No automatic fix available")
                
                if self.fix and any(issue["fixed_path"] for issue in issues):
                    self.fix_file(file_path, issues)
        
        # Print summary
        logger.info("=== Validation Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files with issues: {self.files_with_issues}")
        logger.info(f"Total issues found: {self.issues_found}")
        
        if self.fix:
            logger.info(f"Issues fixed: {self.issues_fixed}")
        
        return all_valid

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Validator")
    parser.add_argument("--root", required=True, help="Project root directory")
    parser.add_argument("--standard", required=True, choices=list(STANDARDS.keys()), 
                        help="Include path standard to validate against")
    parser.add_argument("--fix", action="store_true", help="Automatically fix issues when possible")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    validator = IncludePathValidator(
        root_dir=args.root,
        standard=args.standard,
        fix=args.fix,
        verbose=args.verbose
    )
    
    result = validator.validate_codebase()
    
    # Return 0 if validation passed, 1 if issues were found
    sys.exit(0 if result else 1)

if __name__ == "__main__":
    main()