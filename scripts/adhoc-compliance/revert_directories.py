#!/usr/bin/env python3
import os
import subprocess
import shutil
import argparse
import sys

def restore_directories_from_commit(repo_path, commit_hash, directories):
    """Restore specified directories from a target commit"""
    temp_dir = os.path.join(repo_path, "_temp_restore")
    os.makedirs(temp_dir, exist_ok=True)
    
    success = True
    restored_files = 0
    
    try:
        # Get all files from the specified directories in the target commit
        for directory in directories:
            try:
                result = subprocess.run(
                    ["git", "ls-tree", "-r", "--name-only", commit_hash, directory],
                    cwd=repo_path,
                    stdout=subprocess.PIPE,
                    text=True,
                    check=True
                )
                files = result.stdout.splitlines()
                
                if not files:
                    print(f"No files found in '{directory}' at commit {commit_hash}")
                    continue
                
                print(f"Found {len(files)} files in '{directory}'")
                
                for file_path in files:
                    # Extract file from target commit
                    try:
                        content_result = subprocess.run(
                            ["git", "show", f"{commit_hash}:{file_path}"],
                            cwd=repo_path,
                            stdout=subprocess.PIPE,
                            check=True
                        )
                        
                        # Prepare target directory
                        dest_file = os.path.join(repo_path, file_path)
                        dest_dir = os.path.dirname(dest_file)
                        os.makedirs(dest_dir, exist_ok=True)
                        
                        # Write file
                        with open(dest_file, 'wb') as f:
                            f.write(content_result.stdout)
                        
                        restored_files += 1
                        print(f"Restored: {file_path}")
                        
                    except subprocess.CalledProcessError as e:
                        print(f"Failed to extract {file_path}: {e}")
                        success = False
                
            except subprocess.CalledProcessError as e:
                print(f"Error listing files in '{directory}': {e}")
                success = False
        
        return success, restored_files
    
    finally:
        # Clean up temp directory
        shutil.rmtree(temp_dir, ignore_errors=True)

def fix_includes(repo_path):
    """Run the fix_includes.sh script to correct include paths"""
    fix_script = os.path.join(repo_path, "tools", "linter", "fix_includes.sh")
    
    if not os.path.exists(fix_script):
        print(f"Error: Include fix script not found at {fix_script}")
        return False
    
    try:
        # Ensure the script is executable
        os.chmod(fix_script, 0o755)
        
        # Run the script
        print("\nRunning include path fixes...")
        subprocess.run([fix_script], cwd=repo_path, check=True)
        return True
    
    except subprocess.CalledProcessError as e:
        print(f"Error running fix_includes.sh: {e}")
        return False

def sync_headers(repo_path):
    """Run the sync_headers.sh script to synchronize headers"""
    sync_script = os.path.join(repo_path, "tools", "formatter", "sync_headers.sh")
    
    if not os.path.exists(sync_script):
        print(f"Error: Header sync script not found at {sync_script}")
        return False
    
    try:
        # Ensure the script is executable
        os.chmod(sync_script, 0o755)
        
        # Run the script
        print("\nSynchronizing headers...")
        subprocess.run([sync_script], cwd=repo_path, check=True)
        return True
    
    except subprocess.CalledProcessError as e:
        print(f"Error running sync_headers.sh: {e}")
        return False

def verify_reversion(repo_path):
    """Perform basic verification of the reversion operation"""
    # Check if key directories exist
    if not os.path.exists(os.path.join(repo_path, "src")) or \
       not os.path.exists(os.path.join(repo_path, "include")):
        print("Error: Expected directories not found after reversion")
        return False
    
    # Check for automaton.h and automaton.c as indicators
    automaton_h = os.path.join(repo_path, "include", "core", "automaton", "automaton.h")
    automaton_c = os.path.join(repo_path, "src", "core", "automaton", "automaton.c")
    
    if not os.path.exists(automaton_h) or not os.path.exists(automaton_c):
        print("Warning: Key automaton files not found after reversion")
        return False
    
    return True

def main():
    parser = argparse.ArgumentParser(description="Revert src and include directories to a specific commit")
    parser.add_argument("repo_path", help="Path to the repository")
    parser.add_argument("commit_hash", help="Target commit hash to restore from")
    parser.add_argument("--skip-fixes", action="store_true", help="Skip running fix scripts after restoration")
    
    args = parser.parse_args()
    
    # Validate repository path
    if not os.path.isdir(os.path.join(args.repo_path, ".git")):
        print(f"Error: {args.repo_path} is not a valid git repository")
        sys.exit(1)
    
    # Validate commit hash
    try:
        result = subprocess.run(
            ["git", "cat-file", "-t", args.commit_hash],
            cwd=args.repo_path,
            stdout=subprocess.PIPE,
            text=True,
            check=True
        )
        obj_type = result.stdout.strip()
        if obj_type != "commit":
            print(f"Error: {args.commit_hash} is not a valid commit hash")
            sys.exit(1)
    except subprocess.CalledProcessError:
        print(f"Error: {args.commit_hash} is not a valid commit hash")
        sys.exit(1)
    
    print(f"Reverting src and include directories to commit {args.commit_hash}...")
    
    # Directories to revert
    directories = ["src", "include"]
    
    # Create a backup
    backup_dir = f"backup_{args.commit_hash}"
    print(f"Creating backup in {backup_dir}...")
    os.makedirs(backup_dir, exist_ok=True)
    
    for directory in directories:
        source_dir = os.path.join(args.repo_path, directory)
        if os.path.exists(source_dir):
            target_dir = os.path.join(backup_dir, directory)
            shutil.copytree(source_dir, target_dir, dirs_exist_ok=True)
            print(f"Backed up {directory}")
    
    # Restore directories from target commit
    success, restored_files = restore_directories_from_commit(args.repo_path, args.commit_hash, directories)
    
    if not success:
        print("\nWarning: Some files may not have been properly restored")
    
    print(f"\nRestored {restored_files} files from commit {args.commit_hash}")
    
    # Run fix scripts unless skipped
    if not args.skip_fixes:
        fix_includes(args.repo_path)
        sync_headers(args.repo_path)
    
    # Verify the reversion
    if verify_reversion(args.repo_path):
        print("\nVerification passed: Key directories and files found")
    else:
        print("\nVerification warning: Some expected files may be missing")
    
    print(f"\nOperation complete. Backup stored in {backup_dir}")
    print("You should review the changes and test compilation.")

if __name__ == "__main__":
    main()