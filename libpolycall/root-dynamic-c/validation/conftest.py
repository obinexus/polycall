"""
Pytest configuration and fixtures
"""

import pytest
from pathlib import Path
import tempfile

@pytest.fixture
def temp_project():
    """Create temporary project structure for testing"""
    with tempfile.TemporaryDirectory() as temp_dir:
        project_root = Path(temp_dir) / "test_project"
        project_root.mkdir()
        
        # Create basic project structure
        (project_root / "src").mkdir()
        (project_root / "include").mkdir()
        
        # Create sample C files with proper content
        main_c_content = """#include <stdio.h>

int main() {
    printf("Hello, World!\\n");
    return 0;
}"""
        
        test_h_content = """#pragma once

// Test header file
"""
        
        (project_root / "src" / "main.c").write_text(main_c_content)
        (project_root / "include" / "test.h").write_text(test_h_content)
        
        yield project_root
