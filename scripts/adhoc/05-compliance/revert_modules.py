import os
import subprocess
import argparse
import shutil

def revert_module_to_commit(repo_path, module_path, target_commit):
    """Revert a specific module to a known good commit."""
    # Check if the module path exists
    if not os.path.exists(os.path.join(repo_path, module_path)):
        print(f"Warning: Module path '{module_path}' does not exist in the current repo")
        
    # First, get the files in the module
    try:
        result = subprocess.run(
            ["git", "ls-tree", "-r", "--name-only", target_commit, module_path],
            cwd=repo_path,
            stdout=subprocess.PIPE,
            text=True,
            check=True
        )
        module_files = result.stdout.splitlines()
    except subprocess.CalledProcessError as e:
        print(f"Error listing files: {e}")
        return False
    
    if not module_files:
        print(f"No files found in module '{module_path}' at commit {target_commit}")
        return False
    
    # Create a temporary directory to restore files
    temp_dir = os.path.join(repo_path, "_temp_restore")
    os.makedirs(temp_dir, exist_ok=True)
    
    try:
        # Checkout the module files from the target commit
        for file_path in module_files:
            # Extract the file name and create any needed subdirectories
            file_name = os.path.basename(file_path)
            file_dir = os.path.dirname(file_path)
            temp_file_dir = os.path.join(temp_dir, file_dir)
            os.makedirs(temp_file_dir, exist_ok=True)
            
            # Get the file content from the target commit
            try:
                result = subprocess.run(
                    ["git", "show", f"{target_commit}:{file_path}"],
                    cwd=repo_path,
                    stdout=subprocess.PIPE,
                    check=True
                )
                
                # Write the content to a temp file
                with open(os.path.join(temp_file_dir, file_name), 'wb') as f:
                    f.write(result.stdout)
                
                print(f"Retrieved {file_path} from commit {target_commit}")
            except subprocess.CalledProcessError as e:
                print(f"Error retrieving {file_path}: {e}")
                continue
        
        # Copy the restored files back to the working directory
        for file_path in module_files:
            temp_file = os.path.join(temp_dir, file_path)
            dest_file = os.path.join(repo_path, file_path)
            
            # Ensure destination directory exists
            os.makedirs(os.path.dirname(dest_file), exist_ok=True)
            
            if os.path.exists(temp_file):
                shutil.copy2(temp_file, dest_file)
                print(f"Restored {file_path}")
        
        return True
    
    finally:
        # Clean up temporary directory
        shutil.rmtree(temp_dir, ignore_errors=True)

def main():
    parser = argparse.ArgumentParser(description="Revert specific modules to a known good commit.")
    parser.add_argument("repo_path", help="Path to the Git repository")
    parser.add_argument("target_commit", help="Target commit hash to restore from")
    parser.add_argument("--modules", nargs='+', default=["src/core/automaton", "include/core/automaton"],
                       help="Module paths to revert (default: automaton module)")
    
    args = parser.parse_args()
    
    success = True
    for module in args.modules:
        print(f"Reverting module {module} to commit {args.target_commit}...")
        if not revert_module_to_commit(args.repo_path, module, args.target_commit):
            success = False
            print(f"Failed to revert module {module}")
    
    if success:
        print("\nAll modules successfully reverted. You should review the changes and commit them.")
    else:
        print("\nSome modules could not be reverted. Please check the errors above.")

if __name__ == "__main__":
    main()