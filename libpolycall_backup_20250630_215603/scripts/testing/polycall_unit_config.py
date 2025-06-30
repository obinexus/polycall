#!/usr/bin/env python3
"""
LibPolyCall Unit Test Configuration Script

This script automates test discovery, build configuration, and dependency management
for the LibPolyCall unit testing framework. It follows the AAA (Arrange, Act, Assert)
pattern and integrates with the CMake build system.

Author: OBINexusComputing
"""

import os
import sys
import glob
import shutil
import subprocess
import argparse
import logging
from pathlib import Path


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('libpolycall-test-config')


class LibPolycallTestConfig:
    """Manages LibPolyCall test configuration and builds."""

    def __init__(self, project_root, build_dir=None, verbose=False):
        """
        Initialize test configuration with project paths.
        
        Args:
            project_root: Root directory of the LibPolyCall project
            build_dir: Output directory for built artifacts (default: <project_root>/build)
            verbose: Enable verbose output
        """
        self.project_root = Path(project_root).resolve()
        self.build_dir = Path(build_dir).resolve() if build_dir else self.project_root / 'build'
        self.src_dir = self.project_root / 'src'
        self.tests_dir = self.project_root / 'tests'
        self.unit_tests_dir = self.tests_dir / 'unit'
        self.test_build_dir = self.build_dir / 'tests'
        
        # Configure logging verbosity
        if verbose:
            logger.setLevel(logging.DEBUG)

        # Validate directories
        self._validate_directories()

    def _validate_directories(self):
        """Validate that required directories exist."""
        required_dirs = [
            self.project_root,
            self.src_dir,
            self.tests_dir,
            self.unit_tests_dir
        ]
        
        for directory in required_dirs:
            if not directory.exists():
                logger.error(f"Required directory not found: {directory}")
                sys.exit(1)
        
        # Create build directories if they don't exist
        os.makedirs(self.build_dir, exist_ok=True)
        os.makedirs(self.test_build_dir, exist_ok=True)

    def discover_modules(self):
        """
        Discover all modules in the src directory that have corresponding test modules.
        
        Returns:
            List of module paths relative to src directory
        """
        modules = []
        
        # Find all subdirectories in src/core
        core_dir = self.src_dir / 'core'
        if core_dir.exists():
            for item in core_dir.iterdir():
                if item.is_dir():
                    test_dir = self.unit_tests_dir / 'core' / item.name
                    if test_dir.exists():
                        modules.append(f"core/{item.name}")
        
        # Find all subdirectories in src/cli
        cli_dir = self.src_dir / 'cli'
        if cli_dir.exists():
            for item in cli_dir.iterdir():
                if item.is_dir():
                    test_dir = self.unit_tests_dir / 'cli' / item.name
                    if test_dir.exists():
                        modules.append(f"cli/{item.name}")
        
        logger.info(f"Discovered {len(modules)} testable modules")
        logger.debug(f"Modules: {modules}")
        
        return modules

    def generate_test_cmake(self, modules):
        """
        Generate CMakeLists.txt files for test modules.
        
        Args:
            modules: List of module paths
        """
        # Generate root CMakeLists.txt for tests
        root_cmake_path = self.tests_dir / 'CMakeLists.txt'
        with open(root_cmake_path, 'w') as f:
            f.write(self._generate_root_cmake_content(modules))
        
        logger.info(f"Generated root CMakeLists.txt at {root_cmake_path}")
        
        # Generate CMakeLists.txt for unit tests
        unit_cmake_path = self.unit_tests_dir / 'CMakeLists.txt'
        with open(unit_cmake_path, 'w') as f:
            f.write(self._generate_unit_cmake_content(modules))
        
        logger.info(f"Generated unit tests CMakeLists.txt at {unit_cmake_path}")
        
        # Generate CMakeLists.txt for each module
        for module in modules:
            module_dir = self.unit_tests_dir / Path(module)
            test_files = list(module_dir.glob('test_*.c'))
            
            if not test_files:
                logger.warning(f"No test files found in {module_dir}")
                continue
            
            module_cmake_path = module_dir / 'CMakeLists.txt'
            with open(module_cmake_path, 'w') as f:
                f.write(self._generate_module_cmake_content(module, test_files))
            
            logger.info(f"Generated module CMakeLists.txt at {module_cmake_path}")

    def _generate_root_cmake_content(self, modules):
        """Generate content for the root CMakeLists.txt file."""
        return f"""# Auto-generated CMakeLists.txt for LibPolyCall tests
cmake_minimum_required(VERSION 3.10)

# Include the parent project for finding libraries
get_filename_component(PARENT_DIR ${{CMAKE_CURRENT_SOURCE_DIR}} DIRECTORY)
include_directories(${{PARENT_DIR}}/include)
include_directories(${{PARENT_DIR}}/src)
include_directories(${{CMAKE_CURRENT_SOURCE_DIR}}/framework)

# Set test compile flags
set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} -Wall -Wextra -g")

# Add unit tests subdirectory
add_subdirectory(unit)

# Create test target that runs all tests
add_custom_target(test
    COMMAND echo "Running all tests..."
    DEPENDS unit_tests
)

# Print test status
add_custom_command(TARGET test POST_BUILD
    COMMAND echo "All tests completed"
)
"""

    def _generate_unit_cmake_content(self, modules):
        """Generate content for the unit tests CMakeLists.txt file."""
        module_subdirs = '\n'.join([f'add_subdirectory({module})' for module in modules])
        
        return f"""# Auto-generated CMakeLists.txt for unit tests
cmake_minimum_required(VERSION 3.10)

# Create unit test target
add_custom_target(unit_tests)

# Add core modules
add_subdirectory(core)

# Create a target to run all unit tests
add_custom_target(run_unit_tests
    COMMAND echo "Running unit tests..."
    DEPENDS unit_tests
)
"""

    def _generate_module_cmake_content(self, module, test_files):
        """Generate content for a module's CMakeLists.txt file."""
        module_path_parts = module.split('/')
        module_type = module_path_parts[0]  # core or cli
        module_name = module_path_parts[1]  # actual module name
        
        # Generate targets for each test file
        targets = []
        for test_file in test_files:
            test_name = test_file.stem  # filename without extension
            target_name = f"{module_name}_{test_name}"
            
            targets.append({
                'name': target_name,
                'file': test_file.name,
                'module': module_name
            })
        
        # Build CMake content
        content = f"""# Auto-generated CMakeLists.txt for {module} tests
cmake_minimum_required(VERSION 3.10)

# Find the module library
set(MODULE_LIB_NAME {module_name})
"""
        
        # Add targets
        for target in targets:
            content += f"""
# Target for {target['name']}
add_executable({target['name']} {target['file']})
target_include_directories({target['name']} PRIVATE
    ${{CMAKE_CURRENT_SOURCE_DIR}}/../../../include
    ${{CMAKE_CURRENT_SOURCE_DIR}}/../../../src
    ${{CMAKE_CURRENT_SOURCE_DIR}}/../../framework
)
target_link_libraries({target['name']} ${{MODULE_LIB_NAME}})
add_dependencies(unit_tests {target['name']})

# Custom command to run the test
add_custom_command(TARGET {target['name']} POST_BUILD
    COMMAND echo "Running {target['name']}..."
    COMMAND {target['name']}
    WORKING_DIRECTORY ${{CMAKE_CURRENT_BINARY_DIR}}
)
"""
        
        return content

    def copy_build_artifacts(self):
        """Copy build artifacts from the main build to the test build directory."""
        # Find all .o and .a files in the build directory
        object_files = list(self.build_dir.glob('**/*.o'))
        library_files = list(self.build_dir.glob('**/*.a'))
        all_artifacts = object_files + library_files
        
        for artifact in all_artifacts:
            # Get relative path from build dir
            rel_path = artifact.relative_to(self.build_dir)
            # Create target path in test build dir
            target_path = self.test_build_dir / rel_path
            # Create parent directories if they don't exist
            os.makedirs(target_path.parent, exist_ok=True)
            
            # Copy the file
            shutil.copy2(artifact, target_path)
            logger.debug(f"Copied {artifact} to {target_path}")
        
        logger.info(f"Copied {len(all_artifacts)} build artifacts to test build directory")

    def build_tests(self):
        """Build all tests using CMake."""
        os.makedirs(self.test_build_dir, exist_ok=True)
        
        # Run CMake to generate build files
        cmake_cmd = [
            'cmake',
            '-S', str(self.tests_dir),
            '-B', str(self.test_build_dir)
        ]
        
        logger.info("Running CMake to generate test build files")
        result = subprocess.run(cmake_cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            logger.error(f"CMake configuration failed:\n{result.stderr}")
            return False
        
        # Build tests
        build_cmd = [
            'cmake',
            '--build', str(self.test_build_dir),
            '--target', 'unit_tests'
        ]
        
        logger.info("Building unit tests")
        result = subprocess.run(build_cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            logger.error(f"Test build failed:\n{result.stderr}")
            return False
        
        logger.info("Test build completed successfully")
        return True

    def run_tests(self):
        """Run all built tests."""
        run_cmd = [
            'cmake',
            '--build', str(self.test_build_dir),
            '--target', 'run_unit_tests'
        ]
        
        logger.info("Running unit tests")
        result = subprocess.run(run_cmd, capture_output=False, text=True)
        
        if result.returncode != 0:
            logger.error("One or more tests failed")
            return False
        
        logger.info("All tests passed")
        return True

    def configure_and_build(self):
        """Run the full test configuration and build process."""
        # Discover modules with tests
        modules = self.discover_modules()
        
        # Generate CMake files
        self.generate_test_cmake(modules)
        
        # Copy build artifacts
        self.copy_build_artifacts()
        
        # Build tests
        if not self.build_tests():
            return False
        
        return True


def main():
    """Main entry point for the script."""
    parser = argparse.ArgumentParser(description='LibPolyCall Unit Test Configuration')
    parser.add_argument('--project-root', required=True, help='Project root directory')
    parser.add_argument('--build-dir', help='Build directory')
    parser.add_argument('--run', action='store_true', help='Run tests after building')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    test_config = LibPolycallTestConfig(
        project_root=args.project_root,
        build_dir=args.build_dir,
        verbose=args.verbose
    )
    
    success = test_config.configure_and_build()
    
    if success and args.run:
        success = test_config.run_tests()
    
    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())