#!/usr/bin/env python3
"""
LibPolyCall Include Path Fixer with Path Validation

This script analyzes and fixes incorrect include paths in C/C++ source files by:
1. Scanning for incorrect include paths
2. Checking if the standardized path actually exists in the filesystem
3. Applying fixes only for paths that exist in the correct location
4. Providing detailed logging and validation reports

Usage:
  python fix_include_paths_with_validation.py --project-root /path/to/libpolycall [--error-log build_errors.txt] [--dry-run]

Author: Implementation for OBINexusComputing based on standardization requirements
"""

import os
import re
import sys
import argparse
import logging
import tempfile
import subprocess
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-path-fixer')

class IncludePathFixer:
    """Fixes and validates include paths in LibPolyCall codebase."""
    
    def __init__(self, project_root: str, error_log: Optional[str] = None, 
                 dry_run: bool = False, verbose: bool = False):
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
        self.paths_verified = 0
        self.paths_invalid = 0
        
        # Files with detected issues
        self.file_fixes: Dict[str, Dict[str, str]] = {}  # file -> {wrong_path -> correct_path}
        
        # Invalid path corrections (target doesn't exist)
        self.invalid_corrections: Dict[str, str] = {}  # wrong_path -> attempted_correction
        
        # Initialize path correction patterns
        self._initialize_correction_patterns()
        
        # Build header file index
        self.header_map = self._build_header_index()
        
    def _build_header_index(self) -> Dict[str, List[str]]:
        """
        Build an index of header files and their locations.
        This helps with finding correct paths for includes.
        """
        header_map = {}
        
        # Scan include directory
        if self.include_dir.exists():
            for header_file in self.include_dir.glob("**/*.h"):
                rel_path = str(header_file.relative_to(self.include_dir))
                filename = header_file.name
                
                if filename not in header_map:
                    header_map[filename] = []
                
                header_map[filename].append(rel_path)
                logger.debug(f"Found header: {filename} at {rel_path}")
        
        # Scan src directory for headers
        if self.src_dir.exists():
            for header_file in self.src_dir.glob("**/*.h"):
                rel_path = str(header_file.relative_to(self.src_dir))
                filename = header_file.name
                
                if filename not in header_map:
                    header_map[filename] = []
                
                header_map[filename].append(f"src/{rel_path}")
                logger.debug(f"Found header in src: {filename} at src/{rel_path}")
        
        logger.info(f"Indexed {sum(len(paths) for paths in header_map.values())} header files")
        
        # Detect potential duplicates for warning
        duplicates = {name: paths for name, paths in header_map.items() if len(paths) > 1}
        if duplicates:
            logger.warning(f"Found {len(duplicates)} headers with multiple locations:")
            for name, paths in list(duplicates.items())[:5]:  # Show only first 5
                logger.warning(f"  {name}: {', '.join(paths)}")
            if len(duplicates) > 5:
                logger.warning(f"  ... and {len(duplicates) - 5} more duplicate headers")
        
        return header_map
        
    def _initialize_correction_patterns(self):
        """Initialize path correction patterns for standard LibPolyCall structure."""
        # Direct path corrections - specific paths that need fixing
        self.direct_corrections = {
            "polycall/core/polycall/polycall_auth_context.h": "polycall/core/auth/polycall_auth_context.h",
            "polycall/core/polycall/core/polycall_auth_context.h": "polycall/core/auth/polycall_auth_context.h",
        }
        
        # Pattern-based corrections for systematic issues
        self.pattern_corrections = [
            # Fix nested module path patterns
            (r"polycall/core/polycall/core/([^/]+)/([^\"]+)", r"polycall/core/\1/\2"),
            (r"polycall/core/polycall/core/([^\"]+)", r"polycall/core/\1"),
            
            # Fix incorrect core paths
            (r"core/core/([^\"]+)", r"core/\1"),
            (r"polycall/polycall/([^\"]+)", r"polycall/\1"),
            
            # Fix incorrect module placement - specific modules
            (r"polycall/core/polycall/auth/([^\"]+)", r"polycall/core/auth/\1"),
            (r"polycall/core/polycall/config/([^\"]+)", r"polycall/core/config/\1"),
            (r"polycall/core/polycall/edge/([^\"]+)", r"polycall/core/edge/\1"),
            (r"polycall/core/polycall/ffi/([^\"]+)", r"polycall/core/ffi/\1"),
            (r"polycall/core/polycall/micro/([^\"]+)", r"polycall/core/micro/\1"),
            (r"polycall/core/polycall/network/([^\"]+)", r"polycall/core/network/\1"),
            (r"polycall/core/polycall/protocol/([^\"]+)", r"polycall/core/protocol/\1"),
            (r"polycall/core/polycall/telemetry/([^\"]+)", r"polycall/core/telemetry/\1"),
            (r"polycall/core/polycall/cli/([^\"]+)", r"polycall/cli/\1"),
        ]
    
    def parse_error_log(self):
        """Parse the compilation error log to find missing and incorrect includes."""
        if not self.error_log:
            logger.info("No error log provided, skipping error log parsing")
            return
            
        if not os.path.exists(self.error_log):
            logger.warning(f"Error log not found: {self.error_log}")
            return
            
        logger.info(f"Parsing error log: {self.error_log}")
        
        # Pattern for missing file errors
        missing_file_pattern = re.compile(r'([^:]+):(\d+):(\d+):\s+fatal error:\s+([^:]+):\s+No such file or directory')
        
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
                    correct_path = self._correct_include_path(missing_include)
                    if correct_path and self._verify_include_path(correct_path):
                        self.file_fixes[source_file][missing_include] = correct_path
                        logger.debug(f"  Mapped to: {correct_path}")
                    else:
                        logger.warning(f"Could not find valid correction for {missing_include}")
                        if correct_path:
                            self.invalid_corrections[missing_include] = correct_path
        
        logger.info(f"Found {self.errors_found} include errors across {len(self.file_fixes)} files")
    
    def scan_all_files(self):
        """Scan all source files for include path issues regardless of error log."""
        logger.info("Scanning all source files for include path issues...")
        
        source_files = []
        # Find C/C++ source files
        if self.src_dir.exists():
            for ext in ['.c', '.cpp', '.cc', '.cxx']:
                source_files.extend(list(self.src_dir.glob(f"**/*{ext}")))
        
        # Find header files
        if self.include_dir.exists():
            for ext in ['.h', '.hpp', '.hxx']:
                source_files.extend(list(self.include_dir.glob(f"**/*{ext}")))
        
        logger.info(f"Found {len(source_files)} source files to scan")
        
        # Process each file
        for file_path in source_files:
            self._scan_file_for_issues(file_path)
            
            # Progress logging
            if self.files_processed % 50 == 0:
                logger.info(f"Processed {self.files_processed}/{len(source_files)} files")
    
    def _scan_file_for_issues(self, file_path: Path):
        """Scan a single file for include path issues."""
        self.files_processed += 1
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
                
            # Find all include directives
            include_pattern = re.compile(r'#\s*include\s+"([^"]+)"')
            include_matches = include_pattern.findall(content)
            
            file_key = str(file_path)
            if file_key not in self.file_fixes:
                self.file_fixes[file_key] = {}
                
            # Check each include path
            for include_path in include_matches:
                # Skip system includes with angle brackets
                if include_path.startswith('<') and include_path.endswith('>'):
                    continue
                    
                # Check direct corrections first
                if include_path in self.direct_corrections:
                    correct_path = self.direct_corrections[include_path]
                    if self._verify_include_path(correct_path):
                        self.file_fixes[file_key][include_path] = correct_path
                        continue
                        
                # Then try pattern-based corrections
                for pattern, replacement in self.pattern_corrections:
                    if re.match(pattern, include_path):
                        corrected = re.sub(pattern, replacement, include_path)
                        if corrected != include_path and self._verify_include_path(corrected):
                            self.file_fixes[file_key][include_path] = corrected
                            break
        
        except Exception as e:
            logger.error(f"Error scanning {file_path}: {e}")
    
    def _correct_include_path(self, include_path: str) -> Optional[str]:
        """
        Apply correction patterns to an include path.
        Returns corrected path or None if no correction pattern matches.
        """
        # Clean path - remove quotes if present
        clean_path = include_path.strip('"')
        
        # Check direct corrections first
        if clean_path in self.direct_corrections:
            return self.direct_corrections[clean_path]
        
        # Try pattern-based corrections
        for pattern, replacement in self.pattern_corrections:
            if re.match(pattern, clean_path):
                corrected = re.sub(pattern, replacement, clean_path)
                if corrected != clean_path:
                    logger.debug(f"Pattern correction: {clean_path} -> {corrected}")
                    return corrected
        
        # Try to look up by filename as last resort
        filename = os.path.basename(clean_path)
        if filename in self.header_map and len(self.header_map[filename]) == 1:
            # If only one header with this name exists, use it
            return self.header_map[filename][0]
        
        return None
    
    def _verify_include_path(self, include_path: str) -> bool:
        """
        Verify that an include path exists in the project.
        Returns True if the file exists, False otherwise.
        """
        # Clean path - remove quotes if present
        clean_path = include_path.strip('"')
        
        # Check in include/ directory
        include_file = self.include_dir / clean_path
        if include_file.exists():
            self.paths_verified += 1
            return True
        
        # Check in src/ directory
        src_file = self.src_dir / clean_path
        if src_file.exists():
            self.paths_verified += 1
            return True
        
        # Path does not exist
        self.paths_invalid += 1
        logger.debug(f"Invalid path: {clean_path} - file does not exist")
        return False
    
    def fix_files(self):
        """Apply fixes to all files with identified include path issues."""
        files_to_fix = {file_path: fixes for file_path, fixes in self.file_fixes.items() 
                       if fixes and os.path.exists(file_path)}
        
        logger.info(f"Applying fixes to {len(files_to_fix)} files")
        
        for file_path, fixes in files_to_fix.items():
            self.fix_file(file_path, fixes)
    
    def fix_file(self, file_path: str, fixes: Dict[str, str]):
        """Fix include paths in a single file."""
        logger.info(f"Processing file: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            modified_content = content
            
            # Apply all fixes to the file
            for wrong_path, correct_path in fixes.items():
                # Verify the target path exists
                if not self._verify_include_path(correct_path):
                    logger.warning(f"Skipping fix: target path does not exist: {correct_path}")
                    continue
                
                # Create pattern to match the include statement
                pattern = f'#include\\s+"{re.escape(wrong_path)}"'
                replacement = f'#include "{correct_path}"'
                
                # Apply the replacement
                new_content = re.sub(pattern, replacement, modified_content)
                
                if new_content != modified_content:
                    logger.debug(f"  Fixed: {wrong_path} -> {correct_path}")
                    modified_content = new_content
                    self.paths_corrected += 1
                else:
                    logger.debug(f"  No match found for: {wrong_path} in file")
            
            # Check if changes were made
            if modified_content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    self.files_modified += 1
                    logger.info(f"Fixed include paths in: {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would fix include paths in: {file_path}")
            else:
                logger.debug(f"No changes required for: {file_path}")
        
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
    
    def generate_error_report(self):
        """Generate a detailed report of remaining issues."""
        report_lines = ["# LibPolyCall Include Path Issues Report", ""]
        
        # Add files with fixes
        report_lines.append(f"## Files with Include Path Issues: {len(self.file_fixes)}")
        report_lines.append("")
        
        for file_path, fixes in self.file_fixes.items():
            if fixes:
                report_lines.append(f"### {file_path}")
                report_lines.append("")
                for wrong_path, correct_path in fixes.items():
                    valid = self._verify_include_path(correct_path)
                    status = "✓" if valid else "✗"
                    report_lines.append(f"- `{wrong_path}` → `{correct_path}` {status}")
                report_lines.append("")
        
        # Add invalid corrections
        if self.invalid_corrections:
            report_lines.append(f"## Invalid Corrections (Target Does Not Exist): {len(self.invalid_corrections)}")
            report_lines.append("")
            for wrong_path, attempted_correction in self.invalid_corrections.items():
                report_lines.append(f"- `{wrong_path}` → `{attempted_correction}`")
            report_lines.append("")
        
        # Write report to file
        report_file = os.path.join(self.project_root, "include_issues_report.md")
        try:
            with open(report_file, 'w', encoding='utf-8') as f:
                f.write("\n".join(report_lines))
            logger.info(f"Generated detailed report: {report_file}")
        except Exception as e:
            logger.error(f"Error writing report: {e}")
    
    def run(self):
        """Run the include path fixer with validation."""
        # First, build the header index
        self.header_map = self._build_header_index()
        
        # If an error log was provided, parse it for errors
        if self.error_log:
            self.parse_error_log()
        
        # Scan all files for additional issues
        self.scan_all_files()
        
        # Apply fixes
        self.fix_files()
        
        # Generate report
        self.generate_error_report()
        
        # Print summary
        logger.info("\n=== Include Path Fix Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        if self.error_log:
            logger.info(f"Errors found in log: {self.errors_found}")
        logger.info(f"Paths corrected: {self.paths_corrected}")
        logger.info(f"Paths verified: {self.paths_verified}")
        logger.info(f"Invalid paths detected: {self.paths_invalid}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
        
        return self.files_modified > 0

def main():
    parser = argparse.ArgumentParser(
        description="LibPolyCall Include Path Fixer with Path Validation"
    )
    parser.add_argument(
        "--project-root", 
        required=True,
        help="Root directory of LibPolyCall project"
    )
    parser.add_argument(
        "--error-log", 
        help="Compilation error log file (optional)"
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
    
    fixer = IncludePathFixer(
        project_root=args.project_root,
        error_log=args.error_log,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    success = fixer.run()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())