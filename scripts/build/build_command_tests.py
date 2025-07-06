#!/usr/bin/env python3
"""
Build Command Tests for LibPolyCall

Usage:
    ./build_command_tests.py [--source-dir DIR] [--obj-dir DIR] [--test-dir DIR] [--include-dirs DIRS]

Options:
    --source-dir DIR    Source directory for commands [default: src/cli/commands]
def main():
    try:
        parser = argparse.ArgumentParser(description='Build command tests for LibPolyCall',
                                       formatter_class=argparse.RawDescriptionHelpFormatter)
    --include-dirs DIRS Comma-separated list of include directories [default: include,src]
"""

import os
import sys
import subprocess
import glob
import argparse

def main():
    parser = argparse.ArgumentParser(description='Build command tests for LibPolyCall')
    parser.add_argument('--source-dir', default='src/cli/commands', help='Source directory for commands')
    parser.add_argument('--obj-dir', default='obj/commands', help='Output directory for object files')
    parser.add_argument('--test-dir', default='tests/unit/cli/commands', help='Directory containing test files')
    parser.add_argument('--include-dirs', default='include,src', help='Comma-separated list of include directories')
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    os.makedirs(args.obj_dir, exist_ok=True)
    
    # Get include directories
    include_dirs = args.include_dirs.split(',')
    include_flags = ' '.join([f'-I{dir}' for dir in include_dirs])
    
    # Compile each command file to object file
    command_files = glob.glob(f'{args.source_dir}/*.c')
    for command_file in command_files:
        base_name = os.path.basename(command_file)[:-2]  # Remove .c
        obj_file = f'{args.obj_dir}/{base_name}.o'
        
        cmd = f'gcc -c {command_file} -o {obj_file} {include_flags} -DUNIT_TESTING'
        print(f'Compiling {command_file} to {obj_file}...')
        subprocess.run(cmd, shell=True, check=True)
    
    # Build unit tests if they exist
    test_files = glob.glob(f'{args.test_dir}/*_test.c')
    for test_file in test_files:
        base_name = os.path.basename(test_file)[:-7]  # Remove _test.c
        obj_file = f'{args.obj_dir}/{base_name}.o'
        test_bin = f'bin/tests/{base_name}_test'
        
        os.makedirs(os.path.dirname(test_bin), exist_ok=True)
        
        # Check if corresponding object file exists
        if not os.path.exists(obj_file):
            print(f'Warning: No object file found for {test_file}')
            continue
        
        # Link test executable
        cmd = f'gcc {test_file} {obj_file} -o {test_bin} {include_flags} -L. -lpolycall_core -lpolycall_accessibility'
        print(f'Building test {test_bin}...')
        subprocess.run(cmd, shell=True, check=True)

if __name__ == '__main__':
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"Error: Build failed with exit code {e.returncode}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)