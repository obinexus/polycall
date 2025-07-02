#!/usr/bin/env python3
# scripts/build_command_tests.py
# Comprehensive test compilation and linking script for LibPolyCall

import os
import sys
import subprocess
import glob
import argparse
import shutil
import logging
from typing import List, Dict, Optional, Tuple

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger('build_command_tests')

def ensure_directory_exists(directory: str) -> None:
    """Ensure the specified directory exists, creating it if necessary."""
    if not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)
        logger.info(f"Created directory: {directory}")

def detect_compiler() -> str:
    """Detect the appropriate C compiler to use."""
    if shutil.which("gcc"):
        return "gcc"
    elif shutil.which("clang"):
        return "clang"
    else:
        logger.warning("Neither gcc nor clang found, defaulting to 'cc'")
        return "cc"

def run_command(cmd: str, cwd: Optional[str] = None) -> Tuple[int, str, str]:
    """Run a command and return exit code, stdout, and stderr."""
    process = subprocess.Popen(
        cmd, 
        shell=True, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        cwd=cwd,
        universal_newlines=True
    )
    stdout, stderr = process.communicate()
    return process.returncode, stdout, stderr

def compile_source_to_object(
    compiler: str,
    source_file: str,
    obj_dir: str,
    include_flags: str,
    extra_flags: str = ""
) -> Optional[str]:
    """
    Compile a source file to an object file.
    
    Returns the path to the object file if successful, None otherwise.
    """
    # Create base name and object file path
    base_name = os.path.basename(source_file)[:-2]  # Remove .c
    obj_file = os.path.join(obj_dir, f"{base_name}.o")
    
    # Create the object directory structure if it doesn't exist
    ensure_directory_exists(os.path.dirname(obj_file))
    
    # Construct the compile command
    cmd = f"{compiler} -c {source_file} -o {obj_file} {include_flags} {extra_flags} -DUNIT_TESTING"
    
    # Execute the compile command
    logger.info(f"Compiling {source_file} to {obj_file}")
    returncode, stdout, stderr = run_command(cmd)
    
    if returncode != 0:
        logger.error(f"Compilation failed for {source_file}")
        logger.error(f"Command: {cmd}")
        logger.error(f"Error: {stderr}")
        return None
    
    logger.info(f"Successfully compiled {source_file} to {obj_file}")
    return obj_file

def build_test_executable(
    compiler: str,
    test_file: str,
    obj_files: List[str],
    bin_dir: str,
    include_flags: str,
    lib_flags: str
) -> Optional[str]:
    """
    Build a test executable from a test file and object files.
    
    Returns the path to the executable if successful, None otherwise.
    """
    # Create base name and executable path
    base_name = os.path.basename(test_file)[:-2]  # Remove .c
    test_bin = os.path.join(bin_dir, f"{base_name}")
    
    # Create the bin directory structure if it doesn't exist
    ensure_directory_exists(os.path.dirname(test_bin))
    
    # Construct the link command
    obj_files_str = " ".join(obj_files)
    cmd = f"{compiler} {test_file} {obj_files_str} -o {test_bin} {include_flags} {lib_flags}"
    
    # Execute the link command
    logger.info(f"Building test executable: {test_bin}")
    returncode, stdout, stderr = run_command(cmd)
    
    if returncode != 0:
        logger.error(f"Link failed for {test_file}")
        logger.error(f"Command: {cmd}")
        logger.error(f"Error: {stderr}")
        return None
    
    logger.info(f"Successfully built test executable: {test_bin}")
    return test_bin

def collect_dependencies(source_file: str, include_dirs: List[str]) -> List[str]:
    """
    Analyze a source file to determine its dependencies.
    Returns a list of source files that it depends on.
    """
    # This is a simplified implementation
    # A real implementation would parse include statements and map them to actual files
    dependencies = []
    
    with open(source_file, 'r') as f:
        for line in f:
            if line.strip().startswith('#include "'):
                # Extract the include file
                include_file = line.strip().split('"')[1]
                
                # Look for the include file in include directories
                for include_dir in include_dirs:
                    potential_path = os.path.join(include_dir, include_file)
                    if os.path.exists(potential_path):
                        dependencies.append(potential_path)
                        break
    
    return dependencies

def find_source_for_test(test_file: str, source_dirs: List[str]) -> Optional[str]:
    """Find the corresponding source file for a test file."""
    # Extract the base name without _test.c
    if test_file.endswith('_test.c'):
        base_name = os.path.basename(test_file)[:-7]  # Remove _test.c
    else:
        base_name = os.path.basename(test_file)[:-2]  # Just remove .c
    
    # Look for a matching source file in source directories
    for source_dir in source_dirs:
        potential_path = os.path.join(source_dir, f"{base_name}.c")
        if os.path.exists(potential_path):
            return potential_path
        
        # Also check subdirectories
        for root, dirs, files in os.walk(source_dir):
            potential_path = os.path.join(root, f"{base_name}.c")
            if os.path.exists(potential_path):
                return potential_path
    
    return None

def process_accessibility_files(
    compiler: str, 
    source_dir: str, 
    obj_dir: str, 
    include_dirs: List[str],
    include_flags: str
) -> List[str]:
    """Process all accessibility module source files and return compiled object files."""
    accessibility_dir = os.path.join(source_dir, 'core', 'accessibility')
    if not os.path.exists(accessibility_dir):
        logger.warning(f"Accessibility directory not found: {accessibility_dir}")
        return []
    
    accessibility_sources = glob.glob(f'{accessibility_dir}/*.c')
    obj_files = []
    
    for source_file in accessibility_sources:
        obj_file = compile_source_to_object(
            compiler,
            source_file,
            obj_dir,
            include_flags
        )
        if obj_file:
            obj_files.append(obj_file)
    
    return obj_files

def process_command_files(
    compiler: str,
    source_dir: str,
    obj_dir: str,
    include_dirs: List[str],
    include_flags: str
) -> Dict[str, str]:
    """
    Process all command files and return a dictionary mapping 
    command name to object file path.
    """
    command_dir = os.path.join(source_dir, 'cli', 'commands')
    command_files = glob.glob(f'{command_dir}/*.c')
    command_objs = {}
    
    # Also compile the core command.c file
    core_command_file = os.path.join(source_dir, 'cli', 'command.c')
    if os.path.exists(core_command_file):
        obj_file = compile_source_to_object(
            compiler,
            core_command_file,
            obj_dir,
            include_flags
        )
        if obj_file:
            command_objs['command'] = obj_file
    
    for command_file in command_files:
        base_name = os.path.basename(command_file)[:-2]  # Remove .c
        obj_file = compile_source_to_object(
            compiler,
            command_file,
            obj_dir,
            include_flags
        )
        if obj_file:
            command_objs[base_name] = obj_file
    
    return command_objs

def process_test_files(
    compiler: str,
    test_dir: str,
    source_dirs: List[str],
    obj_dir: str,
    bin_dir: str,
    command_objs: Dict[str, str],
    include_flags: str,
    lib_flags: str
) -> List[str]:
    """Process all test files and return paths to built executables."""
    test_files = glob.glob(f'{test_dir}/**/*_test.c', recursive=True)
    built_executables = []
    
    for test_file in test_files:
        # Find the corresponding source file
        source_file = find_source_for_test(test_file, source_dirs)
        
        if not source_file:
            logger.warning(f"No source file found for test: {test_file}")
            continue
        
        # Determine which command objects to link with
        base_name = os.path.basename(source_file)[:-2]  # Remove .c
        required_objs = []
        
        # Always include the core command object if available
        if 'command' in command_objs:
            required_objs.append(command_objs['command'])
        
        # Include the specific command object if available
        if base_name in command_objs:
            required_objs.append(command_objs[base_name])
        
        # If this is an accessibility test, include all accessibility objects
        if 'accessibility' in base_name:
            accessibility_objs = glob.glob(f'{obj_dir}/core/accessibility/*.o')
            required_objs.extend(accessibility_objs)
        
        # Build the test executable
        executable = build_test_executable(
            compiler,
            test_file,
            required_objs,
            bin_dir,
            include_flags,
            lib_flags
        )
        
        if executable:
            built_executables.append(executable)
    
    return built_executables

def run_tests(executables: List[str]) -> bool:
    """Run all test executables and return True if all tests pass."""
    all_passed = True
    
    for executable in executables:
        logger.info(f"Running test: {executable}")
        returncode, stdout, stderr = run_command(executable)
        
        if returncode != 0:
            logger.error(f"Test failed: {executable}")
            logger.error(f"Output: {stdout}")
            logger.error(f"Error: {stderr}")
            all_passed = False
        else:
            logger.info(f"Test passed: {executable}")
    
    return all_passed

def create_ci_artifacts(
    executables: List[str],
    ci_dir: str
) -> None:
    """Create CI artifacts for test executables."""
    ensure_directory_exists(ci_dir)
    
    # Create a test list file
    test_list_file = os.path.join(ci_dir, 'test_list.txt')
    with open(test_list_file, 'w') as f:
        for executable in executables:
            f.write(f"{executable}\n")
    
    # Create a run tests script
    run_tests_script = os.path.join(ci_dir, 'run_tests.sh')
    with open(run_tests_script, 'w') as f:
        f.write("#!/bin/sh\n")
        f.write("# Auto-generated test runner script\n\n")
        f.write("FAILURES=0\n\n")
        
        for executable in executables:
            f.write(f"echo \"Running {os.path.basename(executable)}...\"\n")
            f.write(f"{executable}\n")
            f.write("if [ $? -ne 0 ]; then\n")
            f.write("    echo \"FAILED: ${executable}\"\n")
            f.write("    FAILURES=$((FAILURES+1))\n")
            f.write("else\n")
            f.write("    echo \"PASSED: ${executable}\"\n")
            f.write("fi\n")
            f.write("echo \"\"\n\n")
        
        f.write("if [ $FAILURES -eq 0 ]; then\n")
        f.write("    echo \"All tests passed!\"\n")
        f.write("    exit 0\n")
        f.write("else\n")
        f.write("    echo \"$FAILURES test(s) failed!\"\n")
        f.write("    exit 1\n")
        f.write("fi\n")
    
    # Make the script executable
    os.chmod(run_tests_script, 0o755)
    logger.info(f"Created CI artifacts in {ci_dir}")

def main():
    parser = argparse.ArgumentParser(description='Build command tests for LibPolyCall')
    parser.add_argument('--source-dir', default='src', help='Root source directory')
    parser.add_argument('--test-dir', default='tests', help='Root test directory')
    parser.add_argument('--obj-dir', default='obj', help='Output directory for object files')
    parser.add_argument('--bin-dir', default='bin/tests', help='Output directory for test binaries')
    parser.add_argument('--ci-dir', default='ci', help='Output directory for CI artifacts')
    parser.add_argument('--include-dirs', default='include,src', help='Comma-separated list of include directories')
    parser.add_argument('--lib-dirs', default='.', help='Comma-separated list of library directories')
    parser.add_argument('--libs', default='polycall_core', help='Comma-separated list of libraries to link against')
    parser.add_argument('--run-tests', action='store_true', help='Run tests after building')
    parser.add_argument('--create-ci-artifacts', action='store_true', help='Create CI artifacts')
    parser.add_argument('--verbose', '-v', action='store_true', help='Enable verbose output')
    parser.add_argument('--clean', action='store_true', help='Clean build directories before building')
    args = parser.parse_args()
    
    # Set log level based on verbosity
    if args.verbose:
        logger.setLevel(logging.DEBUG)
    
    # Clean build directories if requested
    if args.clean:
        logger.info("Cleaning build directories")
        if os.path.exists(args.obj_dir):
            shutil.rmtree(args.obj_dir)
        if os.path.exists(args.bin_dir):
            shutil.rmtree(args.bin_dir)
    
    # Create output directories if they don't exist
    ensure_directory_exists(args.obj_dir)
    ensure_directory_exists(args.bin_dir)
    
    # Detect compiler
    compiler = detect_compiler()
    logger.info(f"Using compiler: {compiler}")
    
    # Parse include directories
    include_dirs = args.include_dirs.split(',')
    include_flags = ' '.join([f'-I{dir}' for dir in include_dirs])
    
    # Parse library directories and libraries
    lib_dirs = args.lib_dirs.split(',')
    libs = args.libs.split(',')
    lib_flags = ' '.join([f'-L{dir}' for dir in lib_dirs] + [f'-l{lib}' for lib in libs])
    
    # Process accessibility files
    accessibility_objs = process_accessibility_files(
        compiler,
        args.source_dir,
        args.obj_dir,
        include_dirs,
        include_flags
    )
    
    # Process command files
    command_objs = process_command_files(
        compiler,
        args.source_dir,
        args.obj_dir,
        include_dirs,
        include_flags
    )
    
    # Resolve source directories to search for matching source files
    source_dirs = [args.source_dir]
    source_dirs.extend([os.path.join(args.source_dir, subdir) for subdir in ['cli', 'core']])
    
    # Process test files
    built_executables = process_test_files(
        compiler,
        args.test_dir,
        source_dirs,
        args.obj_dir,
        args.bin_dir,
        command_objs,
        include_flags,
        lib_flags
    )
    
    # Create CI artifacts if requested
    if args.create_ci_artifacts:
        create_ci_artifacts(
            built_executables,
            args.ci_dir
        )
    
    # Run tests if requested
    if args.run_tests:
        all_passed = run_tests(built_executables)
        if all_passed:
            logger.info("All tests passed!")
            return 0
        else:
            logger.error("Some tests failed.")
            return 1
    
    logger.info(f"Built {len(built_executables)} test executables")
    return 0

if __name__ == '__main__':
    sys.exit(main())