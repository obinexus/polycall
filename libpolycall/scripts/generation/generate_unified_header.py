#!/usr/bin/env python3
"""
LibPolyCall Unified Header Generator

This script automatically generates a unified header file (polycall.h) 
that includes all component headers in the project. It creates a single entry point
for library users, supporting the #include <polycall.h> pattern for simplified access.

Usage:
  python generate_unified_header.py --project-root PROJECT_ROOT 
                                    --output OUTPUT_PATH
                                    [--template TEMPLATE_PATH]
                                    [--component-dirs DIR1 DIR2 ...]

Author: Implementation based on Nnamdi Okpala's design specifications
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import List, Dict, Set

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('unified-header-generator')

class UnifiedHeaderGenerator:
    """Generates a unified header file from component headers."""
    
    def __init__(self, project_root: str, output_path: str, 
                 component_dirs: List[str] = None, template_path: str = None,
                 verbose: bool = False):
        self.project_root = Path(project_root)
        self.output_path = Path(output_path)
        self.component_dirs = component_dirs or self._default_component_dirs()
        self.template_path = template_path
        
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.headers_found = 0
        self.components_processed = 0
    
    def _default_component_dirs(self) -> List[str]:
        """Define default component directories if none provided."""
        return [
            "include/polycall/core/polycall",
            "include/polycall/core/auth",
            "include/polycall/core/config",
            "include/polycall/core/edge",
            "include/polycall/core/ffi",
            "include/polycall/core/micro",
            "include/polycall/core/network",
            "include/polycall/core/protocol",
            "include/polycall/core/telemetry",
            "include/polycall/cli"
        ]
    
    def scan_component_headers(self) -> Dict[str, List[str]]:
        """Scan component directories for public headers."""
        component_headers = {}
        
        for component_dir in self.component_dirs:
            component_path = self.project_root / component_dir
            if not component_path.exists():
                logger.warning(f"Component directory not found: {component_path}")
                continue
            
            # Extract component name from path
            component_name = component_dir.split('/')[-1]
            component_headers[component_name] = []
            
            # Find all headers in this component that are not internal/private
            for header_file in component_path.glob("**/*.h"):
                # Skip internal or private headers
                if "_internal" in str(header_file) or "private" in str(header_file):
                    continue
                
                # Get path relative to project root
                rel_path = header_file.relative_to(self.project_root)
                component_headers[component_name].append(str(rel_path))
                self.headers_found += 1
                logger.debug(f"Found header: {rel_path}")
            
            # Sort headers for consistency
            component_headers[component_name].sort()
            self.components_processed += 1
            
        return component_headers
    
    def generate_header_content(self, component_headers: Dict[str, List[str]]) -> str:
        """Generate the content for the unified header."""
        if self.template_path:
            # Use template if provided
            try:
                with open(self.template_path, 'r', encoding='utf-8') as f:
                    template_content = f.read()
                
                # Generate include statements by component
                component_includes = []
                for component, headers in sorted(component_headers.items()):
                    if headers:
                        component_includes.append(f"/* {component.upper()} Component */")
                        for header in headers:
                            component_includes.append(f'#include "{header}"')
                        component_includes.append("")  # Empty line between components
                
                # Replace placeholder with include statements
                unified_content = template_content.replace(
                    "@COMPONENT_INCLUDES@", 
                    "\n".join(component_includes)
                )
                
                return unified_content
            except Exception as e:
                logger.error(f"Error using template {self.template_path}: {e}")
                # Fall back to basic header generation
        
        # Generate basic header
        header_name = os.path.basename(self.output_path)
        guard_name = f"{header_name.upper().replace('.', '_')}"
        
        unified_content = [
            f"/**",
            f" * @file {header_name}",
            f" * @brief Unified public API for LibPolyCall",
            f" *",
            f" * This header provides access to all LibPolyCall functionality.",
            f" * For component-specific functionality, consider including only",
            f" * the headers you need.",
            f" *",
            f" * Generated automatically by {os.path.basename(__file__)}",
            f" */",
            "",
            f"#ifndef {guard_name}",
            f"#define {guard_name}",
            "",
            f"/* Version information */",
            f"#define POLYCALL_VERSION_MAJOR 1",
            f"#define POLYCALL_VERSION_MINOR 1",
            f"#define POLYCALL_VERSION_PATCH 0",
            f"#define POLYCALL_VERSION_STRING \"1.1.0\"",
            ""
        ]
        
        # Add component includes
        for component, headers in sorted(component_headers.items()):
            if headers:
                unified_content.append(f"/* {component.upper()} Component */")
                for header in headers:
                    unified_content.append(f'#include "{header}"')
                unified_content.append("")  # Empty line between components
        
        # Close include guard
        unified_content.append(f"#endif /* {guard_name} */")
        
        return "\n".join(unified_content)
    
    def write_unified_header(self, content: str) -> None:
        """Write the unified header to the output path."""
        # Ensure the output directory exists
        os.makedirs(os.path.dirname(self.output_path), exist_ok=True)
        
        with open(self.output_path, 'w', encoding='utf-8') as f:
            f.write(content)
        
        logger.info(f"Unified header written to {self.output_path}")
    
    def generate(self) -> bool:
        """Generate the unified header file."""
        logger.info(f"Scanning project: {self.project_root}")
        component_headers = self.scan_component_headers()
        
        if not self.headers_found:
            logger.error("No headers found in component directories")
            return False
        
        logger.info(f"Found {self.headers_found} headers across {self.components_processed} components")
        
        content = self.generate_header_content(component_headers)
        self.write_unified_header(content)
        
        logger.info("Unified header generation complete")
        return True

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Unified Header Generator")
    parser.add_argument("--project-root", required=True, help="Project root directory")
    parser.add_argument("--output", required=True, help="Output path for the unified header")
    parser.add_argument("--template", help="Template file path (optional)")
    parser.add_argument("--component-dirs", nargs='+', help="Component directories to scan (optional)")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    generator = UnifiedHeaderGenerator(
        project_root=args.project_root,
        output_path=args.output,
        component_dirs=args.component_dirs,
        template_path=args.template,
        verbose=args.verbose
    )
    
    success = generator.generate()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()