#!/usr/bin/env python3
"""
LibPolyCall Include Path Standardization Tool

This comprehensive script handles:
1. Standardizing include paths across include and src directories
2. Validating compliance with standard path structure
3. Reporting on the standardization process
4. Checking physical file paths for correctness

This is intended to be integrated into the build process to maintain
consistent include path standards throughout the LibPolyCall project.

Author: Based on Nnamdi Okpala's design (OBINexusComputing)
"""

import os
import re
import sys
import argparse
import logging
import json
from pathlib import Path
from typing import List, Dict, Set, Tuple, Optional
from collections import defaultdict
from datetime import datetime

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-standardizer')

class IncludeStandardizer:
    """Standardizes and validates include paths in the LibPolyCall project."""
    
    def __init__(self, project_root: str, verbose: bool = False, 
                 dry_run: bool = False, output_dir: Optional[str] = None,
                 backup: bool = True):
        self.project_root = Path(project_root)
        self.include_dir = self.project_root / "include"
        self.src_dir = self.project_root / "src"
        self.dry_run = dry_run
        self.backup = backup
        
        # Set output directory (for modified files) if specified
        self.output_dir = Path(output_dir) if output_dir else None
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.includes_modified = 0
        self.valid_includes = 0
        self.invalid_includes = 0
        
        # Backup directory
        self.backup_dir = None
        
        # Track issues by type for reporting
        self.issue_types = defaultdict(int)
        self.issues_by_file = defaultdict(list)
        
        # Define standard patterns
        self._init_patterns()
        
        # Create backup if needed
        if self.backup and not self.dry_run:
            self._create_backup()
    
    def _init_patterns(self):
        """Initialize all patterns for standardization and validation."""
        # Valid include path patterns
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
            r"^polycall/core/accessibility/.*\.h$",
            r"^polycall/cli/.*\.h$",
            
            # Root includes
            r"^polycall/polycall\.h$",
            r"^polycall/polycall_config\.h$",
            
            # System includes (always valid)
            r"^<.*>$",
            
            # Relative paths within same module
            r"^[^/]+\.h$",
            r"^\.\./(core|auth|config|edge|ffi|micro|network|protocol|telemetry)/.*\.h$"
        ]
        
        # Invalid patterns to identify specific issues
        self.invalid_patterns = {
            "duplicate_core": r"^core/core/.*\.h$",
            "duplicate_polycall": r"^polycall/polycall/.*\.h$",
            "nested_paths": r"^polycall/core/polycall/core/.*\.h$",
            "direct_include": r"^polycall_.*\.h$",
            "missing_polycall_prefix_core": r"^core/.*\.h$",
            "missing_polycall_prefix_module": r"^(auth|config|ffi|protocol|network|micro|edge|telemetry)/.*\.h$",
            "wrong_module_path": r"^polycall/core/polycall/(auth|config|ffi|protocol|network|micro|edge|telemetry)/.*\.h$"
        }
        
        # Transformation patterns for standardization
        self.transforms = [
            # Fix nested path problems
            # Fix CLI relative paths
            (r"^\.\./(command\.h)$", r"polycall/cli/\1"),
            (r"^\.\./(repl\.h)$", r"polycall/cli/\1"),
            (r"^\.\./(commands/.+\.h)$", r"polycall/cli/\1"),
            # Fix CLI relative paths
            (r"^\.\./(command\.h)$", r"polycall/cli/\1"),
            (r"^\.\./(repl\.h)$", r"polycall/cli/\1"),
            (r"^\.\./(commands/.+\.h)$", r"polycall/cli/\1"),
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
            (r"^core/accessibility/", r"polycall/core/accessibility/"),
            
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
            # Fix core.h and polycall.h paths
            (r"^core/core\.h$", r"polycall/core/polycall/core.h"),
            (r"^core/polycall\.h$", r"polycall/core/polycall/polycall.h"),
            # Fix direct includes
            (r"^polycall_([^/]+\.h)$", r"polycall/core/polycall/polycall_\1"),
        ]
        
        # Direct header mappings for common files
        self.direct_mappings = {
            "polycall_core.h": "polycall/core/polycall/polycall_core.h",
            "polycall_error.h": "polycall/core/polycall/polycall_error.h",
            "polycall_context.h": "polycall/core/polycall/polycall_context.h",
            "polycall_memory.h": "polycall/core/polycall/polycall_memory.h",
            "ffi_core.h": "polycall/core/ffi/ffi_core.h",
            "network.h": "polycall/core/network/network.h",
            "protocol.h": "polycall/core/protocol/protocol.h",
            "accessibility_colors.h": "polycall/core/accessibility/accessibility_colors.h",
            "accessibility_config.h": "polycall/core/accessibility/accessibility_config.h",
            "accessibility_interface.h": "polycall/core/accessibility/accessibility_interface.h",
            "protocol.h": "polycall/core/protocol/protocol.h"
        }
    
    def _create_backup(self):
        """Create a backup of source files before modification."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.backup_dir = self.project_root / f"backup_{timestamp}"
        self.backup_dir.mkdir(exist_ok=True)
        
        logger.info(f"Creating backup in {self.backup_dir}")
        
        # Backup include directory files
        self._backup_directory(self.include_dir)
        
        # Backup src directory files
        self._backup_directory(self.src_dir)
        
        logger.info(f"Backup completed")
    
    def _backup_directory(self, directory: Path):
        """
        Create a backup of all .h and .c files in the specified directory.
        """
        if not directory.exists():
            return
            
        for file_ext in ['.c', '.cpp', '.cc', '.h', '.hpp']:
            for file_path in directory.glob(f"**/*{file_ext}"):
                rel_path = file_path.relative_to(self.project_root)
                backup_path = self.backup_dir / rel_path
                backup_path.parent.mkdir(parents=True, exist_ok=True)
                
                try:
                    with open(file_path, 'rb') as src, open(backup_path, 'wb') as dest:
                        dest.write(src.read())
                except Exception as e:
                    logger.error(f"Error backing up {file_path}: {e}")
    
    def find_all_files(self) -> List[Path]:
        """Find all source and header files in the project."""
        all_files = []
        
        # Find source files
        if self.src_dir.exists():
            for src_file in self.src_dir.glob('**/*.?'):
                if src_file.suffix in ['.c', '.h', '.cpp', '.hpp']:
                    all_files.append(src_file)
        
        # Find header files
        if self.include_dir.exists():
            for inc_file in self.include_dir.glob('**/*.h'):
                all_files.append(inc_file)
        
        return sorted(all_files)
    
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
    
    def standardize_include_path(self, include_path: str) -> str:
        """Convert a non-standard include path to the standard format."""
        # Apply transformation patterns
        fixed_path = include_path
        
        # Handle relative paths based on file location
        if fixed_path.startswith('../'):
            # Keep relative paths within the same module hierarchy
            return fixed_path
        elif not fixed_path.startswith('polycall/') and '/' not in fixed_path:
            # Local include in same directory - keep as is
            return fixed_path
            
        # Apply standard transformations
        for pattern, replacement in self.transforms:
            if re.match(pattern, fixed_path):
                prev_path = fixed_path
                fixed_path = re.sub(pattern, replacement, fixed_path)
                if prev_path != fixed_path:
                    logger.debug(f"Fixed: {prev_path} → {fixed_path}")
                    
        
                if prev_path != fixed_path:
                    logger.debug(f"Fixed: {prev_path} → {fixed_path}")
        
        return fixed_path
    
    def process_file(self, file_path: Path) -> bool:
        """
        Process a single file to standardize include paths.
        Returns True if file was modified.
        """
        logger.debug(f"Processing file: {file_path}")
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
                if self.is_valid_include(include_path):
                    self.valid_includes += 1
                    continue
                
                # Track the issue type
                issue_type = self.get_issue_type(include_path)
                self.issue_types[issue_type] += 1
                self.invalid_includes += 1
                
                # Store the issue for reporting
                rel_path = str(file_path.relative_to(self.project_root))
                self.issues_by_file[rel_path].append({
                    "path": include_path,
                    "issue_type": issue_type
                })
                
                # Standardize the path
                standardized_path = self.standardize_include_path(include_path)
                
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
                    # Determine output path
                    output_path = file_path
                    if self.output_dir:
                        rel_path = file_path.relative_to(self.project_root)
                        output_path = self.output_dir / rel_path
                        output_path.parent.mkdir(parents=True, exist_ok=True)
                    
                    with open(output_path, 'w', encoding='utf-8') as f:
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
        """Run the standardization process across all files."""
        all_files = self.find_all_files()
        logger.info(f"Found {len(all_files)} files to process")
        
        if not all_files:
            logger.error("No files found to process")
            return False
        
        for file_path in all_files:
            self.process_file(file_path)
            
            # Log progress for large codebases
            if self.files_processed % 100 == 0:
                logger.info(f"Processed {self.files_processed}/{len(all_files)} files")
        
        # Print summary and validation report
        self.print_report()
        
        # Check for remaining issues
        remaining_issues = self.invalid_includes - self.includes_modified
        return remaining_issues == 0
    
    def print_report(self):
        """Print a detailed report of the standardization process."""
        logger.info("\n=== Standardization Report ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Total includes analyzed: {self.valid_includes + self.invalid_includes}")
        logger.info(f"Valid includes found: {self.valid_includes}")
        logger.info(f"Invalid includes found: {self.invalid_includes}")
        logger.info(f"Includes modified: {self.includes_modified}")
        
        remaining = self.invalid_includes - self.includes_modified
        if remaining > 0:
            logger.warning(f"Remaining issues: {remaining} includes could not be automatically fixed")
        
        if self.invalid_includes > 0:
            logger.info("\nIssue breakdown:")
            for issue_type, count in sorted(self.issue_types.items(), key=lambda x: x[1], reverse=True):
                logger.info(f"  {issue_type}: {count} occurrences")
            
            # Print details of files with the most issues (up to 5)
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
        
        if self.dry_run:
            logger.info("\nThis was a dry run. No files were actually modified.")
    
    def write_json_report(self, report_file: str):
        """Write a JSON report with standardization results."""
        report = {
            "summary": {
                "files_processed": self.files_processed,
                "files_modified": self.files_modified,
                "valid_includes": self.valid_includes,
                "invalid_includes": self.invalid_includes,
                "includes_modified": self.includes_modified,
                "remaining_issues": self.invalid_includes - self.includes_modified,
                "timestamp": datetime.now().isoformat()
            },
            "issue_types": dict(self.issue_types),
            "issues_by_file": self.issues_by_file
        }
        
        try:
            with open(report_file, 'w', encoding='utf-8') as f:
                json.dump(report, f, indent=2)
            
            logger.info(f"JSON report written to {report_file}")
            
        except Exception as e:
            logger.error(f"Error writing JSON report: {e}")
    
    def validate_only(self) -> bool:
        """Run validation without making changes."""
        all_files = self.find_all_files()
        logger.info(f"Validating {len(all_files)} files")
        
        for file_path in all_files:
            self.files_processed += 1
            
            try:
                with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                    content = f.read()
                
                # Find all include statements
                include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
                matches = include_pattern.findall(content)
                
                # Check each include
                for include_path in matches:
                    if self.is_valid_include(include_path):
                        self.valid_includes += 1
                    else:
                        self.invalid_includes += 1
                        
                        # Track the issue
                        issue_type = self.get_issue_type(include_path)
                        self.issue_types[issue_type] += 1
                        
                        # Store for reporting
                        rel_path = str(file_path.relative_to(self.project_root))
                        self.issues_by_file[rel_path].append({
                            "path": include_path,
                            "issue_type": issue_type
                        })
            
            except Exception as e:
                logger.error(f"Error validating {file_path}: {e}")
        
        # Print validation report
        self.print_report()
        
        return self.invalid_includes == 0

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Standardizer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    parser.add_argument("--validate-only", action="store_true", help="Only validate includes without fixing")
    parser.add_argument("--no-backup", action="store_true", help="Skip creating backup files")
    parser.add_argument("--output-dir", help="Directory to output modified files (instead of modifying in place)")
    parser.add_argument("--json-report", help="Write JSON report to specified file")
    
    args = parser.parse_args()
    
    standardizer = IncludeStandardizer(
        project_root=args.project_root,
        verbose=args.verbose,
        dry_run=args.dry_run,
        output_dir=args.output_dir,
        backup=not args.no_backup
    )
    
    if args.validate_only:
        result = standardizer.validate_only()
    else:
        result = standardizer.run()
    
    if args.json_report:
        standardizer.write_json_report(args.json_report)
    
    return 0 if result else 1

if __name__ == "__main__":
    sys.exit(main())
    
    