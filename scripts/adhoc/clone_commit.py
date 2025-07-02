import os
import subprocess
import argparse


def get_commit_range(repo_path, start_commit=None, count=10):
    """Retrieve a specific range of commit hashes and messages from the repository."""
    if start_commit:
        cmd = ["git", "log", "--pretty=format:%h %s", f"{start_commit}~{count}..{start_commit}"]
    else:
        cmd = ["git", "log", "--pretty=format:%h %s", f"-{count}"]
    
    result = subprocess.run(
        cmd,
        cwd=repo_path,
        stdout=subprocess.PIPE,
        text=True,
        stderr=subprocess.PIPE
    )
    
    if result.returncode != 0:
        print(f"Error getting commits: {result.stderr}")
        return []
    
    return result.stdout.splitlines()

def clone_specific_commits(repo_path, output_dir, start_commit=None, count=10):
    """Clone a range of commits into separate directories for analysis."""
    # Get the specified range of commits
    commits = get_commit_range(repo_path, start_commit, count)
    
    for commit in commits:
        try:
            commit_hash, commit_message = commit.split(" ", 1)
        except ValueError:
            print(f"Warning: Could not parse commit '{commit}', skipping")
            continue
            
        # Sanitize the commit message for directory naming
        safe_message = "".join(c if c.isalnum() or c in ['-', '_'] else '_' for c in commit_message[:40])
        commit_dir = os.path.join(output_dir, f"librift-{commit_hash}-{safe_message}")
        
        if os.path.exists(commit_dir):
            print(f"Directory already exists for {commit_hash}, skipping")
            continue
            
        os.makedirs(commit_dir, exist_ok=True)
        
        # Create a worktree for this commit
        try:
            subprocess.run(
                ["git", "worktree", "add", "--detach", commit_dir, commit_hash],
                cwd=repo_path,
                check=True,
                capture_output=True,
                text=True
            )
        except subprocess.CalledProcessError as e:
            print(f"Error creating worktree for {commit_hash}: {e.stderr}")
            continue
        
        print(f"Created worktree for {commit_hash} - {commit_message} at {commit_dir}")
    
    return len(commits)

def verify_automaton_module(commit_dirs):
    """Verify the integrity of the automaton module across different commits."""
    results = {}
    
    for commit_dir in commit_dirs:
        commit_hash = os.path.basename(commit_dir).split("-")[1]
        
        # Check for key files
        automaton_file = os.path.join(commit_dir, "src/core/automaton/automaton.c")
        flags_file = os.path.join(commit_dir, "src/core/automaton/flags.c")
        
        if os.path.exists(automaton_file) and os.path.exists(flags_file):
            # Perform basic integrity check - look for corruption patterns
            with open(automaton_file, 'r', encoding='utf-8', errors='replace') as f:
                content = f.read()
                corrupted = content.count('#include "core/core/') > 0 or \
                           content.count('include "core/core/core/') > 0
                
            with open(flags_file, 'r', encoding='utf-8', errors='replace') as f:
                flags_content = f.read()
                flags_corrupted = flags_content.count('#include "core/core/') > 0 or \
                                 flags_content.count('include "core/core/core/') > 0
            
            results[commit_hash] = {
                'automaton_corrupted': corrupted,
                'flags_corrupted': flags_corrupted
            }
    
    return results

def main():
    parser = argparse.ArgumentParser(description="Clone and analyze a range of commits.")
    parser.add_argument("repo_path", help="Path to the local Git repository")
    parser.add_argument("output_dir", help="Directory to output the cloned commits")
    parser.add_argument("--start", help="Starting commit hash (default: HEAD)", default="HEAD")
    parser.add_argument("--count", help="Number of commits to analyze", type=int, default=10)
    parser.add_argument("--verify", help="Verify the automaton module integrity", action="store_true")
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Clone the specified commits
    num_cloned = clone_specific_commits(args.repo_path, args.output_dir, args.start, args.count)
    print(f"Successfully cloned {num_cloned} commits")
    
    # Optionally verify the automaton module
    if args.verify:
        commit_dirs = [os.path.join(args.output_dir, d) for d in os.listdir(args.output_dir) 
                      if os.path.isdir(os.path.join(args.output_dir, d))]
        results = verify_automaton_module(commit_dirs)
        
        print("\nAutomaton Module Integrity Analysis:")
        print("------------------------------------")
        for commit, status in results.items():
            print(f"Commit {commit}:")
            print(f"  automaton.c corrupted: {status['automaton_corrupted']}")
            print(f"  flags.c corrupted: {status['flags_corrupted']}")

if __name__ == "__main__":
    main()