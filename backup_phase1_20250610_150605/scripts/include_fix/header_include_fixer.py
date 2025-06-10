#!/usr/bin/env python3
"""
LibPolyCall Header Include Fixer

This script analyzes compilation errors and automatically fixes missing header includes
in C/C++ source files. It identifies undefined types and adds the necessary include
statements in the appropriate location.

Usage:
  python header_include_fixer.py <error_log_file> [--dry-run] [--verbose]

Author: Implementation based on Nnamdi Okpala's design for OBINexusComputing
"""

import os
import re
import sys
import argparse
import logging
from pathlib import Path
from typing import Dict, List, Set, Tuple

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-include-fixer')

# Common type to header mappings
COMMON_TYPE_HEADERS = {
    'uint8_t': '<stdint.h>',
    'uint16_t': '<stdint.h>',
    'uint32_t': '<stdint.h>',
    'uint64_t': '<stdint.h>',
    'int8_t': '<stdint.h>',
    'int16_t': '<stdint.h>',
    'int32_t': '<stdint.h>',
    'int64_t': '<stdint.h>',
    'size_t': '<stddef.h>',
    'bool': '<stdbool.h>',
    'pthread_t': '<pthread.h>',
    'pthread_mutex_t': '<pthread.h>',
    'pthread_cond_t': '<pthread.h>',
}

# Common LibPolyCall specific types to their header files
POLYCALL_TYPE_HEADERS = {
    'polycall_core_context_t': 'core/polycall/polycall_core.h',
    'polycall_core_error_t': 'core/polycall/polycall_error.h',
    'polycall_protocol_context_t': 'core/protocol/polycall_protocol_context.h',
    'polycall_message_t': 'core/protocol/message.h',
    'polycall_network_server_t': 'core/network/network_server.h',
    'polycall_network_client_t': 'core/network/network_client.h',
    'polycall_endpoint_t': 'core/network/network_endpoint.h',
    'polycall_network_packet_t': 'core/network/network_packet.h',
    'polycall_network_event_t': 'core/network/network_event.h',
    'polycall_network_config_t': 'core/network/network_config.h',
    'polycall_network_option_t': 'core/network/network.h',
    'polycall_network_stats_t': 'core/network/network.h',
    'polycall_packet_flags_t': 'core/network/network_packet.h',
    'polycall_network_flags_t': 'core/network/network.h',
    'polycall_endpoint_type_t': 'core/network/network_endpoint.h',
    'polycall_endpoint_state_t': 'core/network/network_endpoint.h',
    'polycall_network_client_config_t': 'core/network/network_client.h',
    'polycall_network_server_config_t': 'core/network/network_server.h',
    'socket_handle_t': 'core/network/network.h',
    'server_endpoint_t': 'core/network/network_server.h',
    'client_endpoint_t': 'core/network/network_client.h',
    'worker_thread_t': 'core/network/network.h',
    'event_handler_t': 'core/network/network.h',
    'endpoint_entry_t': 'core/network/network.h',
    'client_entry_t': 'core/network/network.h',
    'server_entry_t': 'core/network/network.h',
    'endpoint_callback_t': 'core/network/network_endpoint.h',
    'packet_metadata_t': 'core/network/network_packet.h',
    'message_handler_entry_t': 'core/network/network_server.h',
}

class HeaderIncludeFixer:
    """Analyzes compilation errors and fixes missing includes in C/C++ files."""
    
    def __init__(self, error_file: str, dry_run: bool = False, verbose: bool = False):
        self.error_file = error_file
        self.dry_run = dry_run
        self.verbose = verbose
        if verbose:
            logger.setLevel(logging.DEBUG)
        
        # Statistics
        self.files_processed = 0
        self.includes_added = 0
        self.errors_found = 0
        self.files_modified = 0
        
        # Store file paths that need fixing
        self.file_fixes: Dict[str, Set[str]] = {}
        
    def parse_error_log(self) -> None:
        """Parse the compilation error log to find missing includes."""
        logger.info(f"Parsing error log: {self.error_file}")
        
        error_pattern = re.compile(r'([^:]+):(\d+):(\d+):\s+error:\s+unknown type name \'([^\']+)\'')
        suggestion_pattern = re.compile(r'note: \'[^\']+\' is defined in header \'([^\']+)\'')
        
        current_file = None
        current_type = None
        
        with open(self.error_file, 'r', encoding='utf-8') as file:
            for line in file:
                # Match error line
                error_match = error_pattern.search(line)
                if error_match:
                    self.errors_found += 1
                    current_file = error_match.group(1)
                    # line_num = int(error_match.group(2))
                    current_type = error_match.group(4)
                    
                    # Check if we know which header this type belongs to
                    if current_type in COMMON_TYPE_HEADERS:
                        header = COMMON_TYPE_HEADERS[current_type]
                        if current_file not in self.file_fixes:
                            self.file_fixes[current_file] = set()
                        self.file_fixes[current_file].add(header)
                        logger.debug(f"Found missing type {current_type} in {current_file}, needs {header}")
                    elif current_type in POLYCALL_TYPE_HEADERS:
                        header = POLYCALL_TYPE_HEADERS[current_type]
                        if current_file not in self.file_fixes:
                            self.file_fixes[current_file] = set()
                        self.file_fixes[current_file].add(header)
                        logger.debug(f"Found missing PolyCall type {current_type} in {current_file}, needs {header}")
                
                # Match suggestion line (compiler hint)
                suggestion_match = suggestion_pattern.search(line)
                if suggestion_match and current_file and current_type:
                    header = suggestion_match.group(1)
                    if current_file not in self.file_fixes:
                        self.file_fixes[current_file] = set()
                    self.file_fixes[current_file].add(header)
                    logger.debug(f"Compiler suggested {header} for type {current_type} in {current_file}")
        
        logger.info(f"Found {self.errors_found} type errors across {len(self.file_fixes)} files")
    
    def fix_files(self) -> None:
        """Add missing includes to the files that need fixing."""
        for file_path, headers in self.file_fixes.items():
            self.files_processed += 1
            self.fix_file(file_path, headers)
    
    def fix_file(self, file_path: str, headers: Set[str]) -> None:
        """Fix includes in a single file."""
        logger.info(f"Processing file: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                content = file.read()
            
            original_content = content
            
            # Find existing includes
            existing_includes = set(re.findall(r'#include\s+[<"]([^>"]+)[>"]', content))
            
            # Filter out headers that are already included
            headers_to_add = set()
            for header in headers:
                # Extract just the filename part for comparison
                if header.startswith('<') and header.endswith('>'):
                    header_name = header[1:-1]
                else:
                    header_name = header
                
                if header_name not in existing_includes:
                    headers_to_add.add(header)
            
            if not headers_to_add:
                logger.info(f"No new headers to add to {file_path}")
                return
            
            # Find position to insert new includes
            # Strategy: After the last existing include, or after header comment block
            last_include_pos = 0
            for match in re.finditer(r'#include\s+[<"][^>"]+[>"]', content):
                last_include_pos = match.end()
            
            if last_include_pos == 0:
                # No includes found, try to find the end of the header comment block
                comment_end = re.search(r'(\*/|^#ifndef|\bdefine)\s*$', content, re.MULTILINE)
                if comment_end:
                    last_include_pos = comment_end.end()
            
            # Generate include statements
            new_includes = "\n"
            for header in sorted(headers_to_add):
                if header.startswith('<'):
                    include_line = f"#include {header}\n"
                else:
                    include_line = f'#include "{header}"\n'
                new_includes += include_line
                self.includes_added += 1
            
            # Insert includes at the appropriate position
            if last_include_pos > 0:
                fixed_content = content[:last_include_pos] + new_includes + content[last_include_pos:]
            else:
                # If we couldn't find a good position, just insert at the beginning
                fixed_content = new_includes + content
            
            # Write the fixed content back to the file
            if not self.dry_run:
                with open(file_path, 'w', encoding='utf-8') as file:
                    file.write(fixed_content)
                self.files_modified += 1
                logger.info(f"Added {len(headers_to_add)} includes to {file_path}")
            else:
                logger.info(f"[DRY RUN] Would add {len(headers_to_add)} includes to {file_path}")
                for header in sorted(headers_to_add):
                    if header.startswith('<'):
                        logger.info(f"  #include {header}")
                    else:
                        logger.info(f'  #include "{header}"')
        
        except Exception as e:
            logger.error(f"Error processing {file_path}: {e}")
    
    def run(self) -> None:
        """Run the header include fixer."""
        self.parse_error_log()
        self.fix_files()
        
        # Print summary
        logger.info(f"Finished processing. Summary:")
        logger.info(f"  Files processed: {self.files_processed}")
        logger.info(f"  Files modified: {self.files_modified}")
        logger.info(f"  Errors found: {self.errors_found}")
        logger.info(f"  Includes added: {self.includes_added}")
        
        if self.dry_run:
            logger.info("This was a dry run. No files were actually modified.")

def main():
    parser = argparse.ArgumentParser(description="LibPolyCall Header Include Fixer")
    parser.add_argument("error_file", help="Compilation error log file")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be changed without modifying files")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")
    
    args = parser.parse_args()
    
    fixer = HeaderIncludeFixer(
        error_file=args.error_file,
        dry_run=args.dry_run,
        verbose=args.verbose
    )
    
    fixer.run()

if __name__ == "__main__":
    main()