#!/usr/bin/env python3
"""
Test Suite for LibPolyCall Include Path Standardizer

This script tests the functionality of the standardize_includes.py script
to ensure it correctly transforms include paths according to the project's
standards.

Author: Implementation based on Nnamdi Okpala's design specifications
"""

import os
import sys
import tempfile
import unittest
from pathlib import Path

# Import the standardizer module
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from demo.standardize_includes import IncludePathStandardizer

class TestIncludeStandardization(unittest.TestCase):
    """Test cases for include path standardization."""
    
    def setUp(self):
        # Create temp directory for test files
        self.temp_dir = tempfile.TemporaryDirectory()
        self.test_dir = Path(self.temp_dir.name)
        
        # Create some test files with problematic includes
        self.test_files = {
            "test1.c": """
            #include "core/core/polycall/polycall_context.h"
            #include "polycall/auth/polycall_auth.h"
            """,
            
            "test2.h": """
            #include "polycall_error.h"
            #include "ffi/ffi_core.h"
            """,
            
            "test3.c": """
            #include "protocol/polycall_protocol_context.h"
            #include "micro/compute_core.h"
            """,
            
            "test4.c": """
            #include <stdio.h>
            #include <stdlib.h>
            
            #include "network/network_client.h"
            #include "parser/message_parser.h"
            """
        }
        
        # Write test files
        for filename, content in self.test_files.items():
            file_path = self.test_dir / filename
            with open(file_path, 'w') as f:
                f.write(content)
        
        # Create standardizer instance
        self.standardizer = IncludePathStandardizer(
            root_dir=self.test_dir,
            backup=False,
            dry_run=False
        )
    
    def tearDown(self):
        # Remove temp directory
        self.temp_dir.cleanup()
    
    def test_duplicate_core_fix(self):
        """Test that duplicate 'core/core/' is fixed to 'core/'."""
        file_path = self.test_dir / "test1.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/polycall/polycall_context.h"', content)
        self.assertNotIn('#include "core/core/polycall/polycall_context.h"', content)
    
    def test_polycall_path_fix(self):
        """Test that 'polycall/' is fixed to 'core/polycall/'."""
        file_path = self.test_dir / "test1.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/polycall/auth/polycall_auth.h"', content)
        self.assertNotIn('#include "polycall/auth/polycall_auth.h"', content)
    
    def test_direct_include_fix(self):
        """Test that direct includes like 'polycall_error.h' are fixed."""
        file_path = self.test_dir / "test2.h"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/polycall/polycall_error.h"', content)
        self.assertNotIn('#include "polycall_error.h"', content)
    
    def test_ffi_path_fix(self):
        """Test that 'ffi/' is fixed to 'core/ffi/'."""
        file_path = self.test_dir / "test2.h"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/ffi/ffi_core.h"', content)
        self.assertNotIn('#include "ffi/ffi_core.h"', content)
    
    def test_protocol_path_fix(self):
        """Test that 'protocol/' is fixed to 'core/protocol/'."""
        file_path = self.test_dir / "test3.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/protocol/polycall_protocol_context.h"', content)
        self.assertNotIn('#include "protocol/polycall_protocol_context.h"', content)
    
    def test_micro_path_fix(self):
        """Test that 'micro/' is fixed to 'core/micro/'."""
        file_path = self.test_dir / "test3.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "core/micro/compute_core.h"', content)
        self.assertNotIn('#include "micro/compute_core.h"', content)
    
    def test_network_path_preserved(self):
        """Test that 'network/' is preserved as is (not a core module)."""
        file_path = self.test_dir / "test4.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "network/network_client.h"', content)
        self.assertNotIn('#include "core/network/network_client.h"', content)
    
    def test_parser_path_preserved(self):
        """Test that 'parser/' is preserved as is (not a core module)."""
        file_path = self.test_dir / "test4.c"
        self.standardizer.standardize_include_in_file(file_path)
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        self.assertIn('#include "parser/message_parser.h"', content)
        self.assertNotIn('#include "core/parser/message_parser.h"', content)
    
    def test_all_files_processed(self):
        """Test that all files are processed and statistics are updated."""
        self.standardizer.process_all_files()
        
        self.assertEqual(self.standardizer.files_processed, 4)
        self.assertEqual(self.standardizer.files_modified, 3)  # test4.c should not be modified
        self.assertTrue(self.standardizer.include_fixes > 0)

    def test_verify_includes(self):
        """Test verification functionality."""
        # Before standardization - should find issues
        is_valid = self.standardizer.verify_includes()
        self.assertFalse(is_valid)
        
        # Standardize includes
        self.standardizer.process_all_files()
        
        # After standardization - should pass verification
        is_valid = self.standardizer.verify_includes()
        self.assertTrue(is_valid)

if __name__ == "__main__":
    unittest.main()