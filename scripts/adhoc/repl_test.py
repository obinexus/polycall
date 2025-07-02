#!/usr/bin/env python3
"""
LibPolyCall REPL Functionality Test

This script tests the REPL functionality of LibPolyCall,
including both interactive and non-interactive modes.

Author: Nnamdi Okpala
"""

import os
import sys
import subprocess
import argparse
import tempfile
import time
import signal

def run_command(cmd, cwd=None, timeout=10, input_data=None):
    """Run a command with timeout and optional input"""
    try:
        result = subprocess.run(
            cmd, 
            cwd=cwd, 
            input=input_data,
            timeout=timeout,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        return result
    except subprocess.TimeoutExpired:
        print(f"Command timed out after {timeout} seconds: {' '.join(cmd)}")
        return None

def test_repl_noninteractive(polycall_bin, test_dir):
    """Test non-interactive REPL mode"""
    print("\n=== Testing Non-Interactive REPL Mode ===")
    
    # Create a test script
    test_script = os.path.join(test_dir, "test_script.js")
    with open(test_script, 'w') as f:
        f.write("""
console.log("Testing LibPolyCall REPL");
const result = 42 * 2;
console.log(`Computation result: ${result}`);
process.exit(0);
""")
    
    # Run the script through polycall
    result = run_command([polycall_bin, "repl", "--script", test_script])
    
    if result and result.returncode == 0:
        print("✅ Non-interactive REPL test passed")
        print("Output:")
        print(result.stdout)
        return True
    else:
        print("❌ Non-interactive REPL test failed")
        if result:
            print("Error output:")
            print(result.stderr)
        return False

def test_repl_interactive(polycall_bin, test_dir):
    """Test interactive REPL mode with simulated input"""
    print("\n=== Testing Interactive REPL Mode ===")
    
    # Prepare input commands for interactive session
    input_commands = """
console.log("Hello from LibPolyCall interactive REPL");
const x = 10;
const y = 20;
console.log(`Sum: ${x + y}`);
.exit
"""
    
    # Run the interactive REPL
    result = run_command(
        [polycall_bin, "repl"], 
        timeout=5,
        input_data=input_commands
    )
    
    if result and result.returncode == 0:
        print("✅ Interactive REPL test passed")
        print("Output:")
        print(result.stdout)
        return True
    else:
        print("❌ Interactive REPL test failed")
        if result:
            print("Error output:")
            print(result.stderr)
        return False

def test_ffi_bindings(polycall_bin, test_dir):
    """Test FFI bindings in REPL"""
    print("\n=== Testing FFI Bindings in REPL ===")
    
    # Create a test script with FFI calls
    test_script = os.path.join(test_dir, "test_ffi.js")
    with open(test_script, 'w') as f:
        f.write("""
console.log("Testing LibPolyCall FFI bindings");
try {
    // Try to use basic FFI functionality
    const result = polycall.ffi.callFunction("polycall_core_version");
    console.log(`FFI call result: ${JSON.stringify(result)}`);
    console.log("FFI test passed");
} catch (error) {
    console.error(`FFI test failed: ${error.message}`);
    process.exit(1);
}
process.exit(0);
""")
    
    # Run the script through polycall
    result = run_command([polycall_bin, "repl", "--script", test_script])
    
    if result and result.returncode == 0:
        print("✅ FFI bindings test passed")
        print("Output:")
        print(result.stdout)
        return True
    else:
        print("❌ FFI bindings test failed")
        if result:
            print("Error output:")
            print(result.stderr)
        return False

def build_project(build_dir, cores=None):
    """Build the project using CMake"""
    print("\n=== Building LibPolyCall ===")
    
    # Determine number of cores for parallel build
    if not cores:
        try:
            cores = os.cpu_count()
        except:
            cores = 2
    
    # Create build directory if it doesn't exist
    os.makedirs(build_dir, exist_ok=True)
    
    # Configure
    config_result = run_command(
        ['cmake', '..'], 
        cwd=build_dir,
        timeout=30
    )
    
    if not config_result or config_result.returncode != 0:
        print("❌ CMake configuration failed")
        if config_result:
            print(config_result.stderr)
        return False
    
    # Build
    build_result = run_command(
        ['cmake', '--build', '.', f'-j{cores}'], 
        cwd=build_dir,
        timeout=300
    )
    
    if not build_result or build_result.returncode != 0:
        print("❌ Build failed")
        if build_result:
            print(build_result.stderr)
        return False
    
    print("✅ Build completed successfully")
    return True

def main():
    parser = argparse.ArgumentParser(description="Test LibPolyCall REPL functionality")
    parser.add_argument("--repo", default=".", help="Path to the repository (default: current directory)")
    parser.add_argument("--build-dir", default="build", help="Build directory name (default: build)")
    parser.add_argument("--polycall-bin", help="Path to polycall binary (default: auto-detect)")
    parser.add_argument("--cores", type=int, help="Number of cores to use for building")
    args = parser.parse_args()
    
    # Validate repository path
    repo_path = os.path.abspath(args.repo)
    build_path = os.path.join(repo_path, args.build_dir)
    
    if not os.path.isdir(repo_path):
        print(f"Error: Repository path {repo_path} does not exist")
        sys.exit(1)
    
    print(f"LibPolyCall REPL Functionality Test")
    print(f"===================================")
    print(f"Repository: {repo_path}")
    print(f"Build directory: {build_path}")
    
    # Build the project if necessary
    should_build = not args.polycall_bin or not os.path.exists(args.polycall_bin)
    
    if should_build:
        success = build_project(build_path, args.cores)
        if not success:
            print("Build failed. Cannot proceed with tests.")
            sys.exit(1)
    
    # Determine polycall binary path
    polycall_bin = args.polycall_bin
    if not polycall_bin:
        # Try to find the binary
        bin_path = os.path.join(build_path, "bin", "polycall")
        if os.path.exists(bin_path):
            polycall_bin = bin_path
        else:
            bin_path = os.path.join(build_path, "polycall")
            if os.path.exists(bin_path):
                polycall_bin = bin_path
            else:
                print("Error: Could not find polycall binary. Please specify with --polycall-bin")
                sys.exit(1)
    
    print(f"Using polycall binary: {polycall_bin}")
    
    # Create a temporary directory for test files
    with tempfile.TemporaryDirectory() as test_dir:
        print(f"Created temporary test directory: {test_dir}")
        
        # Run the tests
        non_interactive_result = test_repl_noninteractive(polycall_bin, test_dir)
        interactive_result = test_repl_interactive(polycall_bin, test_dir)
        ffi_result = test_ffi_bindings(polycall_bin, test_dir)
        
        # Print summary
        print("\n=== Test Summary ===")
        print(f"Non-interactive REPL test: {'PASSED' if non_interactive_result else 'FAILED'}")
        print(f"Interactive REPL test: {'PASSED' if interactive_result else 'FAILED'}")
        print(f"FFI bindings test: {'PASSED' if ffi_result else 'FAILED'}")
        
        if non_interactive_result and interactive_result and ffi_result:
            print("\n✅ All tests passed!")
            return 0
        else:
            print("\n❌ Some tests failed!")
            return 1

if __name__ == "__main__":
    sys.exit(main())