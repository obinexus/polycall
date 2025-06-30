#!/usr/bin/env python3
"""
Remove Invalid #include Statements for Missing Headers

Scans all .c and .h files in the src/ and include/ directories
and removes any #include directives that point to non-existent files.

Usage:
    1. Navigate to the project root directory
    2. Run: python3 scripts/remove_missing_includes.py

The script will:
- Check all .c and .h files in src/ and include/ directories
- Remove #include statements for missing header files
- Print removed includes for reference
- Modify files directly, so use version control
"""

import os
from pathlib import Path
import re

PROJECT_ROOT = Path(__file__).resolve().parent
SRC_DIR = PROJECT_ROOT / "src"
INCLUDE_DIR = PROJECT_ROOT / "include"

INCLUDE_PATTERN = re.compile(r'#\s*include\s+"([^"]+)"')


def header_exists(include_path: str) -> bool:
    """Check if the include path exists in the src or include directory."""
    return any(
        (base / include_path).exists()
        for base in [SRC_DIR, INCLUDE_DIR]
    )


def clean_includes_in_file(file_path: Path):
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    cleaned_lines = []
    removed_count = 0
    
    for line in lines:
        match = INCLUDE_PATTERN.search(line)
        if match:
            include_path = match.group(1)
            if not header_exists(include_path):
                print(f"Removing invalid include in {file_path}: {include_path}")
                removed_count += 1
                continue  # Skip this line
        cleaned_lines.append(line)

    if removed_count > 0:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(cleaned_lines)


def process_directory(directory: Path):
    for ext in ['.c', '.h']:
        for file in directory.rglob(f'*{ext}'):
            clean_includes_in_file(file)


def main():
    print("Scanning and cleaning includes in src/ and include/...\n")
    process_directory(SRC_DIR)
    process_directory(INCLUDE_DIR)
    print("\nInclude cleaning complete.")


if __name__ == '__main__':
    main()
# This script is intended to be run from the root of the project directory.
# It will modify the source files directly, so make sure to back up your files
# or use version control before running it.
# The script will print out the files and includes it removes for your reference.

