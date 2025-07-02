#!/usr/bin/env python3
"""
LibPolyCall Include Path Validator

This script validates that all include paths in LibPolyCall header files comply 
with the standardized include path structure. It reports non-compliant includes
and provides detailed validation metrics.

Author: Based on Nnamdi Okpala's design (OBINexusComputing)
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Set, Tuple, Optional
from collections import defaultdict

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-include-validator')

class IncludePathValidator:
    """Validates include paths in LibPolyCall header files."""
    
    def __init__(self, project_root: str, verbose: bool = False, report_file: Optional[str] = None):
        self.project_root = Path(project_root)
        self.include_dir = self.project_root / "include"
        self.report_file = report_file
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_with_issues = 0
        self.total_includes = 0
        self.non_compliant_includes = 0
        
        # Track issues by type for reporting
        self.issue_types = defaultdict(int)
        self.issues_by_file = defaultdict(list)
        
        # Define standard include path patterns
        self.valid_patterns = [
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
        ]
        
        # Invalid patterns to detect specific issues
        self.invalid_patterns = {
            "duplicate_core": r"^core/core/.*\.h$",
            "duplicate_polycall": r"^polycall/polycall/.*\.h$",
            "nested_paths": r"^polycall/core/polycall/core/.*\.h$",
            "direct_include": r"^polycall_.*\.h$",
            "missing_polycall_prefix_core": r"^core/.*\.h$",
            "missing_polycall_prefix_module": r"^(auth|config|ffi|protocol|network|micro|edge|telemetry)/.*\.h$",
            "wrong_module_path": r"^polycall/core/polycall/(auth|config|ffi|protocol|network|micro|edge|telemetry)/.*\.h$"
        }
    
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
        # Check against valid patterns
        for pattern in self.valid_patterns:
            if re.match(pattern, include_path):
                return True
        
        return False
    
    def get_issue_type(self, include_path: str) -> str:
        """Identify the specific issue with a non-compliant include path."""
        for issue_type, pattern in self.invalid_patterns.items():
            if re.match(pattern, include_path):
                return issue_type
        
        return "unknown"
    
    def validate_file(self, file_path: Path) -> Tuple[bool, List[Dict]]:
        """
        Validate include paths in a single file.
        Returns (is_valid, list of issues).
        """
        logger.debug(f"Validating file: {file_path}")
        
        issues = []
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Find all include statements
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            matches = include_pattern.findall(content)
            
            self.total_includes += len(matches)
            
            # Check each include path
            for include_path in matches:
                if not self.is_valid_include(include_path):
                    issue_type = self.get_issue_type(include_path)
                    issues.append({
                        "path": include_path,
                        "issue_type": issue_type,
                        "file": str(file_path.relative_to(self.project_root))
                    })
                    
                    self.issue_types[issue_type] += 1
                    self.non_compliant_includes += 1
            
            if issues:
                self.issues_by_file[str(file_path.relative_to(self.project_root))] = issues
            
            return len(issues) == 0, issues
            
        except Exception as e:
            logger.error(f"Error validating {file_path}: {e}")
            return False, [{"path": "", "issue_type": "error", "file": str(file_path)}]
    
    def validate_all(self) -> bool:
        """Validate all header files."""
        header_files = self.find_header_files()
        if not header_files:
            logger.error("No header files found")
            return False
        
        all_valid = True
        
        for file_path in header_files:
            self.files_processed += 1
            is_valid, issues = self.validate_file(file_path)
            
            if not is_valid:
                all_valid = False
                self.files_with_issues += 1
        
        # Generate report
        self.print_validation_report()
        
        if self.report_file:
            self.write_report_file()
        
        return all_valid
    
    def print_validation_report(self):
        """Print a validation report to the console."""
        logger.info("\n=== Include Path Validation Report ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files with issues: {self.files_with_issues}")
        logger.info(f"Total includes analyzed: {self.total_includes}")
        logger.info(f"Non-compliant includes: {self.non_compliant_includes}")
        
        if self.non_compliant_includes > 0:
            logger.info("\nIssue breakdown:")
            for issue_type, count in sorted(self.issue_types.items(), key=lambda x: x[1], reverse=True):
                logger.info(f"  {issue_type}: {count} occurrences")
            
            # Print details of the top 5 files with the most issues
            logger.info("\nTop files with issues:")
            sorted_files = sorted(self.issues_by_file.items(), 
                                 key=lambda x: len(x[1]), 
                                 reverse=True)
            
            for file_path, issues in sorted_files[:5]:
                logger.info(f"  {file_path}: {len(issues)} issues")
                # Show up to 3 example issues per file
                for issue in issues[:3]:
                    logger.info(f"    - {issue['path']} ({issue['issue_type']})")
                if len(issues) > 3:
                    logger.info(f"    - ... and {len(issues) - 3} more")
    
    def write_report_file(self):
        """Write a detailed validation report to a file."""
        try:
            with open(self.report_file, 'w', encoding='utf-8') as f:
                f.write("# LibPolyCall Include Path Validation Report\n\n")
                f.write(f"- Files processed: {self.files_processed}\n")
                f.write(f"- Files with issues: {self.files_with_issues}\n")
                f.write(f"- Total includes analyzed: {self.total_includes}\n")
                f.write(f"- Non-compliant includes: {self.non_compliant_includes}\n\n")
                
                if self.non_compliant_includes > 0:
                    f.write("## Issue breakdown\n\n")
                    for issue_type, count in sorted(self.issue_types.items(), 
                                                  key=lambda x: x[1], 
                                                  reverse=True):
                        f.write(f"- {issue_type}: {count} occurrences\n")
                    
                    f.write("\n## All non-compliant includes\n\n")
                    for file_path, issues in sorted(self.issues_by_file.items()):
                        f.write(f"### {file_path}\n\n")
                        for issue in issues:
                            f.write(f"- `{issue['path']}` ({issue['issue_type']})\n")
                        f.write("\n")
                
                f.write("\n## Validation Rules\n\n")
                f.write("### Valid patterns\n\n")
                for pattern in self.valid_patterns:
                    f.write(f"- `{pattern}`\n")
                
                f.write("\n### Invalid patterns\n\n")
                for issue_type, pattern in self.invalid_patterns.items():
                    f.write(f"- {issue_type}: `{pattern}`\n")
            
            logger.info(f"Detailed report written to {self.report_file}")
            
        except Exception as e:
            logger.error(f"Error writing report file: {e}")
    
    def check_header_paths(self):
        """
        Check that the actual header files exist in the expected locations
        based on their include paths.
        """
        logger.info("Checking physical header file paths...")
        header_files = self.find_header_files()
        issues = 0
        
        # Map from relative path to actual file
        path_map = {}
        for file_path in header_files:
            rel_path = str(file_path.relative_to(self.include_dir))
            path_map[rel_path] = file_path
        
        # Check each include reference
        for file_path in header_files:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Find all include statements 
                include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
                matches = include_pattern.findall(content)
                
                for include_path in matches:
                    # Skip system includes
                    if include_path.startswith("<") and include_path.endswith(">"):
                        continue
                    
                    # Skip if already known to be non-standard
                    if not self.is_valid_include(include_path):
                        continue
                    
                    # Check if file exists at expected location
                    if include_path not in path_map:
                        logger.warning(f"Header not found: {include_path} (referenced in {file_path})")
                        issues += 1
            
            except Exception as e:
                logger.error(f"Error checking header in {file_path}: {e}")
        
        if issues == 0:
            logger.info("All referenced headers exist at their expected locations")
        else:
            logger.warning(f"Found {issues} references to headers that don't exist at expected locations")
        
        return issues == 0

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Validator")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    parser.add_argument("--report", help="Write detailed report to specified file")
    parser.add_argument("--check-paths", action="store_true", help="Check physical header file paths")
    
    args = parser.parse_args()
    
    validator = IncludePathValidator(
        project_root=args.project_root,
        verbose=args.verbose,
        report_file=args.report
    )
    
    # Validate includes
    validation_result = validator.validate_all()
    
    # Optionally check physical paths
    path_result = True
    if args.check_paths:
        path_result = validator.check_header_paths()
    
    # Exit with appropriate status code
    return 0 if validation_result and path_result else 1

if __name__ == "__main__":
    sys.exit(main())