#!/usr/bin/env python3
"""
LibPolyCall Include Path Standardizer

This script standardizes include paths across the LibPolyCall project according to the 
official module hierarchy structure. It ensures consistent pathing in both header files
and implementation files, following the proper module organization.

The script implements context-aware path resolution to correctly handle:
1. Direct includes with single-file references (polycall_error.h)
2. Implementation files that may use relative paths
3. Public interface files that should use fully-qualified paths

Usage:
  python standardize_includes.py [options]

Author: Implementation based on Nnamdi Okpala's design specifications
"""

import os
import re
import sys
import argparse
import shutil
import logging
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Set, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-standardizer')

class IncludePathStandardizer:
    """Standardizes include paths in LibPolyCall source files."""
    
    def __init__(self, root_dir='.', backup=True, dry_run=False, file_list=None, 
                 unified_header="polycall.h", public_namespace="polycall"):
        self.root_dir = Path(root_dir)
        self.src_dir = self.root_dir / 'src'
        self.include_dir = self.root_dir / 'include'
        self.dry_run = dry_run
        self.backup_dir = None
        self.file_list = file_list
        self.unified_header = unified_header
        self.public_namespace = public_namespace
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.include_fixes = 0
        
        # Define the module mappings for different contexts
        self._initialize_module_maps()
        
        # Create backup if needed
        if backup and not dry_run:
            self.create_backup()
    
    def _initialize_module_maps(self):
        """Initialize module mappings for different contexts."""
        # Public includes for header files
        self.public_module_map = {
            'auth': f'{self.public_namespace}/core/auth',
            'cli': f'{self.public_namespace}/cli',
            'config': f'{self.public_namespace}/core/config',
            'context': f'{self.public_namespace}/core/context',
            'error': f'{self.public_namespace}/core/error',
            'memory': f'{self.public_namespace}/core/memory',
            'polycall': f'{self.public_namespace}/core/polycall',
            'micro': f'{self.public_namespace}/core/micro',
            'edge': f'{self.public_namespace}/core/edge',
            'ffi': f'{self.public_namespace}/core/ffi',
            'protocol': f'{self.public_namespace}/core/protocol',
            'network': f'{self.public_namespace}/core/network',
            'parser': f'{self.public_namespace}/core/parser',
            'telemetry': f'{self.public_namespace}/core/telemetry',
        }
        
        # Implementation includes for source files
        self.implementation_module_map = {
            'auth': 'core/polycall/auth',
            'cli': 'core/polycall/cli',
            'config': 'core/polycall/config',
            'context': 'core/polycall/context',
            'error': 'core/polycall/error',
            'memory': 'core/polycall/memory',
            'polycall': 'core/polycall',
            'micro': 'core/micro',
            'edge': 'core/edge',
            'ffi': 'core/ffi',
            'protocol': 'core/protocol',
            'network': 'network',
            'parser': 'parser',
            'telemetry': 'core/telemetry',
        }
        
        # Mapping of direct includes to their fully-qualified paths
        self.direct_include_map = {
            'polycall_core.h': f'{self.public_namespace}/core/polycall/polycall_core.h',
            'polycall_error.h': f'{self.public_namespace}/core/polycall/polycall_error.h',
            'polycall_context.h': f'{self.public_namespace}/core/polycall/polycall_context.h',
            'polycall_memory.h': f'{self.public_namespace}/core/polycall/polycall_memory.h',
            'polycall_types.h': f'{self.public_namespace}/core/polycall/polycall_types.h',
            'ffi_core.h': f'{self.public_namespace}/core/ffi/ffi_core.h',
            'network_client.h': f'{self.public_namespace}/core/network/network_client.h',
            'network_server.h': f'{self.public_namespace}/core/network/network_server.h',
        }
    
    def create_backup(self):
        """Create a backup of all source code files before modification."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.backup_dir = self.root_dir / f"backup_{timestamp}"
        self.backup_dir.mkdir(exist_ok=True)
        
        logger.info(f"Creating backup in {self.backup_dir}")
        
        # Back up src directory
        if self.src_dir.exists():
            for src_file in self.src_dir.glob('**/*.?'):
                if src_file.suffix in ['.c', '.h', '.cpp', '.hpp']:
                    rel_path = src_file.relative_to(self.root_dir)
                    dest_file = self.backup_dir / rel_path
                    dest_file.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src_file, dest_file)
        
        # Back up include directory
        if self.include_dir.exists():
            for inc_file in self.include_dir.glob('**/*.h'):
                rel_path = inc_file.relative_to(self.root_dir)
                dest_file = self.backup_dir / rel_path
                dest_file.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(inc_file, dest_file)
        
        logger.info(f"Backup completed successfully")
    
    def find_all_source_files(self):
        """Find all source and header files in the project."""
        # If a specific file list was provided, use that
        if self.file_list:
            with open(self.file_list, 'r') as f:
                return [Path(line.strip()) for line in f if line.strip()]
                
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
    
    def is_implementation_file(self, file_path):
        """
        Determine if a file is an implementation file.
        
        Implementation files are in the src/ directory and may use
        different include path conventions than public headers.
        """
        return str(file_path).startswith(str(self.src_dir))
    
    def is_public_header(self, file_path):
        """
        Determine if a file is a public header file.
        
        Public headers are in the include/ directory and should use
        fully qualified paths.
        """
        return str(file_path).startswith(str(self.include_dir))
    
    def get_context_path(self, file_path):
        """
        Get the context path for a file.
        
        This determines what module the file belongs to, which affects
        how include paths should be handled.
        """
        # Check if in src directory
        if self.is_implementation_file(file_path):
            rel_path = file_path.relative_to(self.src_dir)
            components = str(rel_path).split(os.sep)
            
            # Get module from path
            if len(components) > 0:
                return components[0]
        
        # Check if in include directory
        elif self.is_public_header(file_path):
            rel_path = file_path.relative_to(self.include_dir)
            components = str(rel_path).split(os.sep)
            
            # Skip the public namespace (e.g., polycall)
            if len(components) > 1 and components[0] == self.public_namespace:
                if len(components) > 2 and components[1] == 'core':
                    return components[2]  # Return core module name
                return components[1]  # Return module name
        
        return None
    
    def resolve_include_path(self, include_path, file_path):
        """
        Resolve the correct include path based on file context.
        
        Args:
            include_path: The original include path
            file_path: Path of the file containing the include
        
        Returns:
            The standardized include path
        """
        # Context-aware path resolution
        file_context = self.get_context_path(file_path)
        is_implementation = self.is_implementation_file(file_path)
        
        # Strip quotes from include path
        clean_path = include_path.strip('"')
        
        # Determine which module map to use
        module_map = self.implementation_module_map if is_implementation else self.public_module_map
        
        # Detect direct includes of polycall files
        for direct_name, qualified_path in self.direct_include_map.items():
            if clean_path == direct_name:
                return qualified_path
        
        # Handle module path corrections
        for module, correct_path in module_map.items():
            prefix = f"{module}/"
            if clean_path.startswith(prefix):
                corrected_path = correct_path + clean_path[len(prefix)-1:]
                return corrected_path
        
        # For polycall modules, check direct module references
        if clean_path.startswith("polycall/"):
            if is_implementation:
                return "core/polycall/" + clean_path[len("polycall/"):]
            else:
                return f"{self.public_namespace}/core/polycall/" + clean_path[len("polycall/"):]
        
        # Fix duplicate core prefixes
        if clean_path.startswith("core/core/"):
            return clean_path.replace("core/core/", "core/")
        
        # Default case: return the original path
        return clean_path
    
    def standardize_include_in_file(self, file_path):
        """Standardize include paths in a single file."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
            
            original_content = content
            fixed_content = content
            
            # Track all fixes applied
            fixes_applied = []
            
            # Find all include statements
            include_pattern = re.compile(r'#include\s+"([^"]+)"')
            includes = include_pattern.findall(content)
            
            for include_path in includes:
                # Determine the correct path based on context
                resolved_path = self.resolve_include_path(include_path, file_path)
                
                if resolved_path != include_path:
                    # Create replacement pattern
                    pattern = re.escape(f'#include "{include_path}"')
                    replacement = f'#include "{resolved_path}"'
                    
                    # Apply the replacement
                    fixed_content = re.sub(pattern, replacement, fixed_content)
                    fixes_applied.append(f"Fixed: {include_path} → {resolved_path}")
            
            # Apply changes if needed
            if fixed_content != original_content:
                self.files_modified += 1
                self.include_fixes += len(fixes_applied)
                
                if self.dry_run:
                    logger.info(f"Would modify {file_path} ({len(fixes_applied)} fixes)")
                    for fix in fixes_applied:
                        logger.info(f"  - {fix}")
                else:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(fixed_content)
                    logger.info(f"Modified {file_path} ({len(fixes_applied)} fixes)")
                    for fix in fixes_applied:
                        logger.debug(f"  - {fix}")
                
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def process_all_files(self):
        """Process all source files in the project."""
        all_files = self.find_all_source_files()
        logger.info(f"Found {len(all_files)} source files to process")
        
        for file_path in all_files:
            self.files_processed += 1
            self.standardize_include_in_file(file_path)
            
            # Log progress for large codebases
            if self.files_processed % 100 == 0:
                logger.info(f"Processed {self.files_processed}/{len(all_files)} files")
        
        # Log summary
        logger.info(f"Process completed. Statistics:")
        logger.info(f"  Files processed: {self.files_processed}")
        logger.info(f"  Files modified: {self.files_modified}")
        logger.info(f"  Include fixes applied: {self.include_fixes}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
    
    def verify_includes(self):
        """Verify include paths without making changes."""
        all_files = self.find_all_source_files()
        logger.info(f"Verifying include paths in {len(all_files)} files")
        
        issues_found = 0
        
        for file_path in all_files:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                    content = f.read()
                
                # Extract all include paths
                include_pattern = re.compile(r'#include\s+"([^"]+)"')
                includes = include_pattern.findall(content)
                
                # Check each include path
                file_issues = []
                for include_path in includes:
                    resolved_path = self.resolve_include_path(include_path, file_path)
                    if resolved_path != include_path:
                        file_issues.append(f"{include_path} → should be: {resolved_path}")
                
                if file_issues:
                    issues_found += len(file_issues)
                    logger.warning(f"Issues in {file_path}:")
                    for issue in file_issues:
                        logger.warning(f"  - {issue}")
            
            except Exception as e:
                logger.error(f"Error verifying {file_path}: {e}")
        
        logger.info(f"Verification complete. Found {issues_found} issues in {len(all_files)} files")
        return issues_found == 0
    
    def check_unified_header(self):
        """Check if the unified header exists and is up to date."""
        unified_header_path = self.include_dir / self.unified_header
        
        if not unified_header_path.exists():
            logger.warning(f"Unified header not found: {unified_header_path}")
            return False
        
        logger.info(f"Unified header exists: {unified_header_path}")
        return True
    
    def show_guide(self):
        """Show the include path usage guide."""
        guide = f"""
LibPolyCall Include Path Standards
---------------------------------

1. Standard Include Path Format:

   Unified API Include:
   #include "{self.unified_header}"  (Provides access to all functionality)
   
   Public Interface Includes:
   #include "{self.public_namespace}/core/polycall/polycall_core.h"
   #include "{self.public_namespace}/core/auth/polycall_auth_context.h"
   
   Implementation Includes:
   #include "core/polycall/polycall_core.h"
   #include "core/polycall/polycall_context.h"

2. Common mistakes to avoid:

   ❌ #include "polycall_error.h"
   ✅ #include "{self.public_namespace}/core/polycall/polycall_error.h"
   
   ❌ #include "core/core/polycall/file.h"
   ✅ #include "core/polycall/file.h"
   
   ❌ #include "polycall/auth/auth.h"
   ✅ #include "{self.public_namespace}/core/polycall/auth/auth.h"
   
   ❌ #include "ffi/ffi_core.h"
   ✅ #include "{self.public_namespace}/core/ffi/ffi_core.h"
   
   ❌ #include "micro/compute.h"
   ✅ #include "{self.public_namespace}/core/micro/compute.h"

3. Tools:

   Fix includes:
   ./standardize_includes.py
   
   Check only (no changes):
   ./standardize_includes.py --dry-run
   
   Verify includes:
   ./standardize_includes.py --verify
        """
        print(guide)


def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Include Path Standardizer")
    parser.add_argument("--root", default=".", help="Project root directory")
    parser.add_argument("--no-backup", action="store_true", help="Skip backup creation")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    parser.add_argument("--verify", action="store_true", help="Verify include paths without making changes")
    parser.add_argument("--show-guide", action="store_true", help="Show the include path usage guide")
    parser.add_argument("--file-list", help="File containing a list of files to process")
    parser.add_argument("--output-dir", help="Directory to output corrected files to (instead of modifying in place)")
    parser.add_argument("--unified-header", default="polycall.h", help="Name of the unified header file")
    parser.add_argument("--public-namespace", default="polycall", help="Public namespace for include paths")
    
    args = parser.parse_args()
    
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    
    if args.show_guide:
        standardizer = IncludePathStandardizer(
            unified_header=args.unified_header,
            public_namespace=args.public_namespace
        )
        standardizer.show_guide()
        return 0
    
    standardizer = IncludePathStandardizer(
        root_dir=args.root,
        backup=not args.no_backup,
        dry_run=args.dry_run,
        file_list=args.file_list,
        unified_header=args.unified_header,
        public_namespace=args.public_namespace
    )
    
    if args.verify:
        # Check for unified header first
        standardizer.check_unified_header()
        
        # Verify include paths
        success = standardizer.verify_includes()
        return 0 if success else 1
    else:
        standardizer.process_all_files()
        return 0

if __name__ == "__main__":
    sys.exit(main())