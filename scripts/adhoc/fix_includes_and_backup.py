#!/usr/bin/env python3
import os
import subprocess
import shutil
import argparse
import datetime
import re
import sys

def create_backup(repo_path, commit_hash=None):
    """Create a timestamped backup with optional commit reference"""
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    
    if commit_hash:
        backup_dir = os.path.join(repo_path, "backup", f"{commit_hash}_{timestamp}")
    else:
        backup_dir = os.path.join(repo_path, "backup", timestamp)
    
    os.makedirs(backup_dir, exist_ok=True)
    
    # Backup src and include directories
    for directory in ["src", "include"]:
        source_dir = os.path.join(repo_path, directory)
        if os.path.exists(source_dir):
            target_dir = os.path.join(backup_dir, directory)
            shutil.copytree(source_dir, target_dir, dirs_exist_ok=True)
            print(f"Backed up {directory} to {target_dir}")
    
    return backup_dir

def fix_includes(repo_path):
    """Fix include path issues throughout the codebase"""
    print("Fixing include paths...")
    
    # Phase 1: Find all .c files and ensure they include their corresponding .h file
    c_files = []
    for root, _, files in os.walk(os.path.join(repo_path, "src")):
        for file in files:
            if file.endswith(".c"):
                c_files.append(os.path.join(root, file))
    
    for c_file in c_files:
        # Extract module name and relative path
        rel_path = os.path.relpath(c_file, os.path.join(repo_path, "src"))
        module_name = os.path.basename(c_file)[:-2]  # Remove .c
        module_dir = os.path.dirname(rel_path)
        
        # Construct expected header path
        header_path = f"{module_dir}/{module_name}.h"
        header_include = f'#include "{header_path}"'
        
        # Check if the file exists
        if not os.path.exists(os.path.join(repo_path, "include", header_path)):
            print(f"Warning: Header file not found for {rel_path}")
            continue
        
        # Read the file content
        with open(c_file, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        # Check if the header is already included
        if header_include not in content:
            # Add the include at the top of the file
            new_content = f'{header_include}\n{content}'
            with open(c_file, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Added {header_include} to {rel_path}")
    
    # Phase 2: Fix common include path errors
    for root, _, files in os.walk(os.path.join(repo_path, "src")):
        for file in files:
            if file.endswith((".c", ".h")):
                file_path = os.path.join(root, file)
                
                with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
                    content = f.read()
                
                # Fix duplicate core prefixes and other common errors
                replacements = [
                    (r'#include "core/core/', '#include "core/'),
                    (r'#include "core/core/core/', '#include "core/'),
                    (r'#include "polycall/', '#include "core/polycall/'),
                    (r'#include "ffi/', '#include "core/ffi/')
                ]
                
                modified = False
                for pattern, replacement in replacements:
                    if re.search(pattern, content):
                        content = re.sub(pattern, replacement, content)
                        modified = True
                
                if modified:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                    print(f"Fixed include paths in {os.path.relpath(file_path, repo_path)}")
    
    return True

def verify_includes(repo_path):
    """Verify includes work by checking a sample of files"""
    # Sample critical files to check
    critical_files = [
        "src/core/polycall/polycall.c",
        "src/core/ffi/ffi_core.c"
    ]
    
    success = True
    for file_path in critical_files:
        full_path = os.path.join(repo_path, file_path)
        if not os.path.exists(full_path):
            print(f"Warning: Critical file {file_path} not found")
            success = False
            continue
            
        with open(full_path, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
            
        # Check for missing includes
        includes = re.findall(r'#include\s+"([^"]+)"', content)
        for include in includes:
            # Resolve the include path
            if include.startswith("core/"):
                include_path = os.path.join(repo_path, "include", include)
            else:
                # Try to resolve relative includes
                include_path = os.path.join(os.path.dirname(full_path), include)
                if not os.path.exists(include_path):
                    # Try in include directory
                    include_path = os.path.join(repo_path, "include", include)
            
            if not os.path.exists(include_path):
                print(f"Error: Include '{include}' in {file_path} not found")
                success = False
    
    return success

def sync_headers(repo_path):
    """Ensure all C files have corresponding headers"""
    print("Synchronizing headers...")
    
    # Find all C files and check for corresponding headers
    c_files = []
    for root, _, files in os.walk(os.path.join(repo_path, "src")):
        for file in files:
            if file.endswith(".c"):
                c_files.append(os.path.join(root, file))
    
    for c_file in c_files:
        # Get relative path and module name
        rel_path = os.path.relpath(c_file, os.path.join(repo_path, "src"))
        module_name = os.path.basename(c_file)[:-2]  # Remove .c
        module_dir = os.path.dirname(rel_path)
        
        # Path to header in include directory
        header_path = os.path.join(repo_path, "include", module_dir, f"{module_name}.h")
        
        # Create header if it doesn't exist
        if not os.path.exists(header_path):
            # Create directory if needed
            os.makedirs(os.path.dirname(header_path), exist_ok=True)
            
            # Generate header guard name
            guard_name = f"{module_dir.replace('/', '_').upper()}_{module_name.upper()}_H"
            
            # Create header content
            header_content = f"""/**
 * @file {module_name}.h
 * @brief Header file for {module_name} module
 * @author Nnamdi Okpala (OBINexusComputing)
 *
 * LibPolyCall - A Program First Data-Oriented Program Interface Implementation
 */

#ifndef {guard_name}
#define {guard_name}

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif

/* Function declarations */

#ifdef __cplusplus
}}
#endif

#endif /* {guard_name} */
"""
            # Write header file
            with open(header_path, 'w', encoding='utf-8') as f:
                f.write(header_content)
            
            print(f"Created header {os.path.relpath(header_path, repo_path)}")
    
    return True

def run_compilation_test(repo_path):
    """Run a compilation test to verify fixes worked"""
    print("Running compilation test...")
    
    try:
        result = subprocess.run(
            ["cmake", "--build", ".", "--target", "polycall_static"],
            cwd=repo_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        if result.returncode == 0:
            print("Compilation successful!")
            return True
        else:
            print("Compilation failed with errors:")
            print(result.stderr)
            
            # Extract include errors for diagnosis
            include_errors = re.findall(r'fatal error: ([^:]+): No such file or directory', result.stderr)
            if include_errors:
                print("\nMissing includes:")
                for error in include_errors:
                    print(f"  - {error}")
            
            return False
    
    except Exception as e:
        print(f"Error running compilation test: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Fix include paths and create a backup")
    parser.add_argument("repo_path", help="Path to the repository")
    parser.add_argument("--commit", help="Optional commit hash for backup reference")
    parser.add_argument("--skip-backup", action="store_true", help="Skip creating backup")
    parser.add_argument("--test", action="store_true", help="Run compilation test after fixes")
    
    args = parser.parse_args()
    
    # Verify directory is a git repository
    if not os.path.isdir(os.path.join(args.repo_path, ".git")):
        print(f"Error: {args.repo_path} is not a git repository")
        sys.exit(1)
    
    # Create backup unless skipped
    if not args.skip_backup:
        backup_dir = create_backup(args.repo_path, args.commit)
        print(f"Backup created at {backup_dir}")
    
    # Apply fixes
    fix_result = fix_includes(args.repo_path)
    if not fix_result:
        print("Warning: Include fixes may not have been fully applied")
    
    # Sync headers
    sync_result = sync_headers(args.repo_path)
    if not sync_result:
        print("Warning: Header synchronization may not have been fully completed")
    
    # Verify includes
    verify_result = verify_includes(args.repo_path)
    if not verify_result:
        print("Warning: Some includes may still be missing or incorrect")
    
    # Run compilation test if requested
    if args.test:
        test_result = run_compilation_test(args.repo_path)
        if not test_result:
            print("\nCompilation test failed. You may need to manually fix remaining issues.")
    
    print("\nOperation complete. Please review changes and test compilation.")

if __name__ == "__main__":
    main()