#!/usr/bin/env python3
"""
LibPolyCall Implementation File Include Path Fixer

This script specifically targets and fixes include patterns in implementation files (src/),
with special handling for:
1. Relative paths (../../include/...)
2. Inconsistent module references 
3. Context-aware resolution based on file location

Author: Based on Nnamdi Okpala's design (OBINexusComputing)
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Set, Tuple, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-implementation-fixer')

class ImplementationIncludeFixer:
    """Fixes include paths in implementation files with context awareness."""
    
    def __init__(self, project_root: str, dry_run: bool = False, verbose: bool = False):
        self.project_root = Path(project_root)
        self.src_dir = self.project_root / "src"
        self.include_dir = self.project_root / "include"
        self.dry_run = dry_run
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.files_modified = 0
        self.includes_modified = 0
        
        # Define standard canonical paths
        self.module_prefix = "polycall"
        
        # Map of module names to their standardized paths
        self.module_paths = {
            "polycall": f"{self.module_prefix}/core/polycall",
            "auth": f"{self.module_prefix}/core/auth",
            "config": f"{self.module_prefix}/core/config",
            "edge": f"{self.module_prefix}/core/edge",
            "ffi": f"{self.module_prefix}/core/ffi",
            "micro": f"{self.module_prefix}/core/micro",
            "network": f"{self.module_prefix}/core/network",
            "protocol": f"{self.module_prefix}/core/protocol",
            "telemetry": f"{self.module_prefix}/core/telemetry",
            "cli": f"{self.module_prefix}/cli",
        }
        
        # Common file mappings for special cases
        self.file_mappings = {
            "polycall_error.h": f"{self.module_prefix}/core/polycall/polycall_error.h",
            "polycall_memory.h": f"{self.module_prefix}/core/polycall/polycall_memory.h",
            "polycall_context.h": f"{self.module_prefix}/core/polycall/polycall_context.h",
            "polycall_core.h": f"{self.module_prefix}/core/polycall/polycall_core.h",
            "polycall.h": f"{self.module_prefix}/core/polycall/polycall.h",
        }
        
        # Build a header file index to help with resolution
        self.header_index = self._build_header_index()
    
    def _build_header_index(self) -> Dict[str, str]:
        """
        Build an index of header files to their canonical paths.
        This helps resolve includes accurately.
        """
        header_index = {}
        
        if not self.include_dir.exists():
            logger.warning(f"Include directory not found: {self.include_dir}")
            return header_index
        
        # Scan all header files
        for header_file in self.include_dir.glob("**/*.h"):
            # Get the filename and canonical path
            filename = header_file.name
            rel_path = str(header_file.relative_to(self.include_dir))
            
            # Add to the index (filename -> full canonical path)
            header_index[filename] = rel_path
            
            # Also index by module-specific path for nested files
            if "/" in rel_path:
                module_path = rel_path.split("/", 1)[1]  # Skip the leading 'polycall/'
                if module_path not in header_index:
                    header_index[module_path] = rel_path
        
        return header_index
    
    def find_implementation_files(self) -> List[Path]:
        """Find all implementation files in the src directory."""
        if not self.src_dir.exists():
            logger.error(f"Source directory not found: {self.src_dir}")
            return []
        
        implementation_files = []
        for ext in ['.c', '.cpp', '.cc']:
            implementation_files.extend(self.src_dir.glob(f"**/*{ext}"))
        
        logger.info(f"Found {len(implementation_files)} implementation files")
        return sorted(implementation_files)
    
    def get_module_from_path(self, file_path: Path) -> Optional[str]:
        """
        Determine the module a file belongs to based on its path.
        Returns the module name or None if it can't be determined.
        """
        # Get path relative to src directory
        try:
            rel_path = file_path.relative_to(self.src_dir)
            components = str(rel_path).split(os.sep)
            
            # First component is usually the module
            if len(components) > 0:
                if components[0] in self.module_paths:
                    return components[0]
                # Handle nested paths like core/polycall/...
                elif len(components) > 1 and components[0] == "core":
                    if components[1] in self.module_paths:
                        return components[1]
            
            return None
        except:
            return None
    
    def resolve_include_path(self, include_path: str, file_path: Path) -> str:
        """
        Resolve an include path to its standardized form based on context.
        
        Args:
            include_path: The original include path
            file_path: The file containing the include
            
        Returns:
            The standardized include path
        """
        # Skip system includes
        if include_path.startswith('<') and include_path.endswith('>'):
            return include_path
            
        # Handle direct mappings first
        if include_path in self.file_mappings:
            return self.file_mappings[include_path]
        
        # Handle relative paths pointing to include directory
        if "../../include/" in include_path:
            # Extract the path after include/
            path_after_include = include_path.split("../../include/", 1)[1]
            
            # Special case for top-level includes
            if "/" not in path_after_include:
                if path_after_include in self.file_mappings:
                    return self.file_mappings[path_after_include]
                return f"{self.module_prefix}/{path_after_include}"
            
            # Handle module-specific paths
            module = path_after_include.split("/", 1)[0]
            rest_of_path = path_after_include.split("/", 1)[1] if "/" in path_after_include else ""
            
            if module in self.module_paths:
                return f"{self.module_paths[module]}/{rest_of_path}"
            
            # Default standardization for other paths
            return f"{self.module_prefix}/{path_after_include}"
        
        # Handle ../module/file.h patterns (one level up)
        if include_path.startswith("../"):
            parts = include_path[3:].split("/", 1)
            if len(parts) > 1:
                module, rest_of_path = parts
                if module in self.module_paths:
                    return f"{self.module_paths[module]}/{rest_of_path}"
        
        # Handle direct module references (module/file.h)
        direct_module_match = re.match(r'^([^/]+)/(.+)$', include_path)
        if direct_module_match:
            module, rest_of_path = direct_module_match.groups()
            if module in self.module_paths:
                return f"{self.module_paths[module]}/{rest_of_path}"
        
        # Use the header index to resolve by filename
        filename = os.path.basename(include_path)
        if filename in self.header_index:
            return self.header_index[filename]
        
        # Get the current file's module context
        file_module = self.get_module_from_path(file_path)
        
        # For local includes within the same directory, add the module prefix
        if "/" not in include_path and file_module:
            module_path = self.module_paths.get(file_module)
            if module_path:
                return f"{module_path}/{include_path}"
        
        # If we can't determine how to fix it, return the original
        return include_path
    
    def fix_includes_in_file(self, file_path: Path) -> bool:
        """
        Fix include paths in a single implementation file.
        Returns True if the file was modified.
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
            
            # Track modifications
            changes_made = False
            fixes = []
            
            # Process each include
            for include_path in matches:
                resolved_path = self.resolve_include_path(include_path, file_path)
                
                if resolved_path != include_path:
                    # Create pattern to replace this specific include
                    pattern = f'#include\\s+"{re.escape(include_path)}"'
                    replacement = f'#include "{resolved_path}"'
                    
                    # Apply the replacement
                    new_content = re.sub(pattern, replacement, content)
                    if new_content != content:
                        content = new_content
                        changes_made = True
                        self.includes_modified += 1
                        fixes.append(f"{include_path} → {resolved_path}")
            
            # Save changes if modified
            if changes_made and content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    self.files_modified += 1
                    logger.info(f"Updated {len(fixes)} includes in {file_path}")
                else:
                    logger.info(f"[DRY RUN] Would update {len(fixes)} includes in {file_path}")
                
                # Log the specific changes
                for fix in fixes:
                    logger.debug(f"  Fixed: {fix}")
                
                return True
            
            return False
            
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
            return False
    
    def run(self) -> bool:
        """Fix includes in all implementation files."""
        implementation_files = self.find_implementation_files()
        if not implementation_files:
            logger.error("No implementation files found")
            return False
        
        for file_path in implementation_files:
            self.fix_includes_in_file(file_path)
            
            # Log progress for large codebases
            if self.files_processed % 50 == 0:
                logger.info(f"Processed {self.files_processed}/{len(implementation_files)} files")
        
        # Print summary
        logger.info("\n=== Implementation Include Fix Summary ===")
        logger.info(f"Files processed: {self.files_processed}")
        logger.info(f"Files modified: {self.files_modified}")
        logger.info(f"Includes modified: {self.includes_modified}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")
        
        return True

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Implementation File Include Fixer")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    fixer = ImplementationIncludeFixer(
        project_root=args.project_root,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    success = fixer.run()
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())