"""File Utilities"""
from pathlib import Path
from typing import List

def find_files_by_extension(root: Path, extensions: List[str]) -> List[Path]:
    """Find files by extension"""
    files = []
    for ext in extensions:
        files.extend(root.glob(f"**/*{ext}"))
    return files
