#!/bin/bash
# prune-gitkeep.sh - Automated .gitkeep management
set -euo pipefail

find . -name ".gitkeep" -type f | while IFS= read -r gitkeep; do
    dir=$(dirname "$gitkeep")
    
    # Count non-hidden files (excluding .gitkeep itself)
    file_count=$(find "$dir" -maxdepth 1 -type f ! -name ".gitkeep" ! -name ".*" | wc -l)
    
    if [[ $file_count -gt 0 ]]; then
        echo "Removing unnecessary .gitkeep: $gitkeep (directory has $file_count files)"
        rm -f "$gitkeep"
    else
        echo "Keeping .gitkeep in empty directory: $dir"
    fi
done
