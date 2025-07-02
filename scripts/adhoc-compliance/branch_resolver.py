#!/usr/bin/env python3
"""
LibPolyCall Branch Resolver

This script analyzes and resolves differences between dev_temp and main branches,
specifically focusing on fixing issues in the src/core/polycall directory.
It creates a new branch with the resolved codebase.

Author: Nnamdi Okpala
"""

import os
import sys
import subprocess
import argparse
import tempfile
import shutil
from datetime import datetime

def run_command(cmd, cwd=None, capture_output=True):
    """Run a command and return the result"""
    try:
        result = subprocess.run(
            cmd, 
            cwd=cwd, 
            check=True, 
            capture_output=capture_output, 
            text=True
        )
        return result.stdout if capture_output else None
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {' '.join(cmd)}")
        print(f"Error details: {e.stderr}")
        sys.exit(1)

def get_branch_commit(branch):
    """Get the commit hash of a branch"""
    return run_command(['git', 'rev-parse', branch]).strip()

def create_backup(repo_path, branch_name):
    """Create a backup of the current state"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = os.path.join(repo_path, f"backup_{branch_name}_{timestamp}")
    
    # Backup key directories
    for directory in ["src/core/polycall", "src/core/ffi"]:
        source_dir = os.path.join(repo_path, directory)
        if os.path.exists(source_dir):
            target_dir = os.path.join(backup_dir, directory)
            os.makedirs(os.path.dirname(target_dir), exist_ok=True)
            shutil.copytree(source_dir, target_dir)
            print(f"Backed up {directory} to {backup_dir}")
    
    return backup_dir

def analyze_differences(repo_path, source_branch, target_branch, focus_path):
    """Analyze key differences between branches for a specific path"""
    print(f"\nAnalyzing differences in {focus_path} between {source_branch} and {target_branch}...")
    
    # Get files that differ in the focus path
    diff_output = run_command([
        'git', 'diff', '--name-status', f'{target_branch}..{source_branch}', '--', focus_path
    ], cwd=repo_path)
    
    changed_files = []
    for line in diff_output.splitlines():
        if not line.strip():
            continue
        status, file_path = line.split(maxsplit=1)
        changed_files.append((status, file_path))
    
    return changed_files

def create_resolution_branch(repo_path, source_branch, target_branch, focus_paths):
    """Create a resolution branch from target branch with changes from source branch"""
    # Create a new branch name
    resolution_branch = f"resolution_{source_branch}_{datetime.now().strftime('%Y%m%d_%H%M')}"
    
    # Create and checkout the resolution branch from target branch
    run_command(['git', 'checkout', '-b', resolution_branch, target_branch], cwd=repo_path, capture_output=False)
    print(f"\nCreated resolution branch '{resolution_branch}' from {target_branch}")
    
    # For each focus path, analyze and apply changes
    all_changed_files = []
    for focus_path in focus_paths:
        changed_files = analyze_differences(repo_path, source_branch, target_branch, focus_path)
        all_changed_files.extend(changed_files)
        
        if not changed_files:
            print(f"No differences found in {focus_path}")
            continue
        
        print(f"\nFound {len(changed_files)} different files in {focus_path}:")
        for status, file_path in changed_files:
            status_desc = {
                'M': 'Modified',
                'A': 'Added',
                'D': 'Deleted',
                'R': 'Renamed'
            }.get(status[0], status)
            print(f"  {status_desc}: {file_path}")
    
    if not all_changed_files:
        print("No differences to resolve. Exiting.")
        run_command(['git', 'checkout', target_branch], cwd=repo_path, capture_output=False)
        run_command(['git', 'branch', '-D', resolution_branch], cwd=repo_path, capture_output=False)
        return None
    
    # Create a temporary directory to extract files from source branch
    with tempfile.TemporaryDirectory() as temp_dir:
        source_commit = get_branch_commit(source_branch)
        
        # Extract and apply changes from each changed file
        for status, file_path in all_changed_files:
            if status[0] == 'D':
                # Handle deleted files
                print(f"Removing {file_path}...")
                if os.path.exists(os.path.join(repo_path, file_path)):
                    run_command(['git', 'rm', file_path], cwd=repo_path, capture_output=False)
            else:
                # Extract file from source branch
                try:
                    file_content = run_command(['git', 'show', f'{source_commit}:{file_path}'], cwd=repo_path)
                    
                    # Ensure directory exists
                    os.makedirs(os.path.dirname(os.path.join(repo_path, file_path)), exist_ok=True)
                    
                    # Write file content
                    with open(os.path.join(repo_path, file_path), 'w') as f:
                        f.write(file_content)
                    
                    # Stage file
                    run_command(['git', 'add', file_path], cwd=repo_path, capture_output=False)
                    print(f"Applied changes to {file_path}")
                except Exception as e:
                    print(f"Error applying changes to {file_path}: {str(e)}")
        
        # Commit the changes
        commit_message = f"fix: Resolve issues in {', '.join(focus_paths)} by merging from {source_branch}\n\n"
        commit_message += "This commit resolves codebase issues by applying targeted fixes from the working branch.\n"
        commit_message += f"Source: {source_branch} ({source_commit})\n"
        
        run_command(['git', 'commit', '-m', commit_message], cwd=repo_path, capture_output=False)
        print(f"\nCommitted changes to {resolution_branch}")
    
    return resolution_branch

def resolve_absolute_path(repo_path, path):
    """Convert a relative path to absolute path based on repo root"""
    if os.path.isabs(path):
        return path
    return os.path.normpath(os.path.join(repo_path, path))

def validate_paths(repo_path, focus_paths):
    """Validate and convert paths to absolute paths"""
    absolute_paths = []
    for path in focus_paths:
        abs_path = resolve_absolute_path(repo_path, path)
        if not os.path.exists(abs_path):
            print(f"Warning: Path does not exist: {abs_path}")
        absolute_paths.append(abs_path)
    return absolute_paths


def main():
    parser = argparse.ArgumentParser(description="Resolve differences between LibPolyCall branches")
    parser.add_argument("--repo", default=".", help="Path to the repository (default: current directory)")
    parser.add_argument("--source", default="dev_temp", help="Source branch with working code (default: dev_temp)")
    parser.add_argument("--target", default="main", help="Target branch to fix (default: main)")
    parser.add_argument("--focus", nargs="+", default=["src/core/polycall", "src/core/ffi"], 
                        help="Focus paths to analyze and fix (default: src/core/polycall src/core/ffi)")
    parser.add_argument("--backup", action="store_true", help="Create backup before making changes")
    args = parser.parse_args()
    
    # Validate repository path
    repo_path = os.path.abspath(args.repo)
    if not os.path.isdir(os.path.join(repo_path, ".git")):
        print(f"Error: {repo_path} is not a git repository")
        sys.exit(1)
    
    # Validate branches
    for branch in [args.source, args.target]:
        try:
            run_command(['git', 'rev-parse', '--verify', branch], cwd=repo_path)
        except:
            print(f"Error: Branch '{branch}' does not exist")
            sys.exit(1)
    
    print(f"LibPolyCall Branch Resolver")
    print(f"===========================")
    print(f"Repository: {repo_path}")
    print(f"Source branch (working code): {args.source}")
    print(f"Target branch (to fix): {args.target}")
    print(f"Focus paths: {', '.join(args.focus)}")
    
    # Create backup if requested
    if args.backup:
        backup_dir = create_backup(repo_path, args.target)
        print(f"Created backup at {backup_dir}")
    
    # Create resolution branch
    resolution_branch = create_resolution_branch(repo_path, args.source, args.target, args.focus)
    
    if resolution_branch:
        print(f"\nResolution complete! New branch created: {resolution_branch}")
        print(f"To test the resolved code, run:")
        print(f"  git checkout {resolution_branch}")
        print(f"  cmake -B build && cmake --build build")
        print(f"\nTo merge the resolved code into {args.target}, run:")
        print(f"  git checkout {args.target}")
        print(f"  git merge {resolution_branch}")
    else:
        print(f"\nNo resolution was necessary.")

if __name__ == "__main__":
    main()