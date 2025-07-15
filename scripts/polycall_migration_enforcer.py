#!/usr/bin/env python3
"""
OBINexus PolyCall Migration Enforcer
Ensures command purity and header/source mirroring for all modules
"""

import os
import re
import sys
import json
import shutil
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Set, Tuple

class PolyCallMigrationEnforcer:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.include_dir = self.project_root / "include" / "polycall"
        self.src_dir = self.project_root / "src"
        self.cli_dir = self.src_dir / "cli"
        self.core_dir = self.src_dir / "core"
        
        # Command modules that must remain isolated
        self.command_modules = {
            "micro", "telemetry", "guid", "edge", "crypto", "topo",
            "repl", "ignore", "doctor"
        }
        
        # Infrastructure modules that commands can depend on
        self.infrastructure_modules = {
            "base", "polycall", "common", "memory", "error", "context",
            "config", "parser", "schema", "factory",
            "protocol", "network", "auth", "ffi", "bridges", "hotwire"
        , "config", "parser", "schema", "factory", "accessibility", "repl"}
        
        self.violations = []
        self.fixes_applied = []
        
    def run_migration(self):
        """Main migration process"""
        print("=== OBINexus PolyCall Migration Enforcer ===")
        print(f"Project Root: {self.project_root}")
        print()
        
        # Step 1: Validate directory structure
        if not self._validate_structure():
            return False
            
        # Step 2: Enforce header/source mirroring
        print("\n[1/5] Enforcing header/source mirroring...")
        self._enforce_header_source_mirror()
        
        # Step 3: Check command purity
        print("\n[2/5] Checking command module purity...")
        self._check_command_purity()
        
        # Step 4: Generate IoC wrapper for CLI
        print("\n[3/5] Generating IoC command registry...")
        self._generate_ioc_registry()
        
        # Step 5: Update build configuration
        print("\n[4/5] Updating build configuration...")
        self._update_build_config()
        
        # Step 6: Generate compliance report
        print("\n[5/5] Generating compliance report...")
        self._generate_report()
        
        return len(self.violations) == 0
        
    def _validate_structure(self) -> bool:
        """Validate required directory structure exists"""
        required_dirs = [
            self.include_dir,
            self.src_dir,
            self.cli_dir,
            self.core_dir
        ]
        
        for dir_path in required_dirs:
            if not dir_path.exists():
                print(f"ERROR: Required directory missing: {dir_path}")
                return False
                
        return True
        
    def _enforce_header_source_mirror(self):
        """Ensure every .h file has corresponding .c file with proper structure"""
        header_map = self._build_header_map()
        
        for module_name, headers in header_map.items():
            module_src_dir = self.core_dir / module_name
            
            # Create module source directory if needed
            if not module_src_dir.exists():
                module_src_dir.mkdir(parents=True)
                print(f"  Created source directory: {module_src_dir}")
                
            for header_file in headers:
                # Determine corresponding source file
                source_name = header_file.stem + ".c"
                source_file = module_src_dir / source_name
                
                if not source_file.exists():
                    # Generate skeleton source file
                    self._generate_source_skeleton(header_file, source_file)
                    self.fixes_applied.append(f"Generated: {source_file}")
                else:
                    # Validate source matches header
                    self._validate_source_header_match(header_file, source_file)
                    
    def _build_header_map(self) -> Dict[str, List[Path]]:
        """Build map of module -> header files"""
        header_map = {}
        
        for header_path in self.include_dir.rglob("*.h"):
            # Extract module name from path
            relative_path = header_path.relative_to(self.include_dir)
            parts = relative_path.parts
            
            if len(parts) > 1:
                module_name = parts[0]
            else:
                module_name = "polycall"
                
            if module_name not in header_map:
                header_map[module_name] = []
            header_map[module_name].append(header_path)
            
        return header_map
        
    def _generate_source_skeleton(self, header_file: Path, source_file: Path):
        """Generate skeleton C file for header"""
        header_name = header_file.name
        module_name = header_file.parent.name
        
        # Extract function declarations from header
        functions = self._extract_functions(header_file)
        
        content = f"""/*
 * {source_file.name}
 * Implementation for {header_name}
 * Module: {module_name}
 * Generated by OBINexus Migration Enforcer
 */

#include "{self._get_include_path(header_file)}"
#include <stdlib.h>
#include <string.h>

"""
        
        # Add function implementations
        for func in functions:
            content += self._generate_function_stub(func)
            
        source_file.write_text(content)
        print(f"  Generated: {source_file}")
        
    def _extract_functions(self, header_file: Path) -> List[str]:
        """Extract function declarations from header"""
        content = header_file.read_text()
        
        # Simple regex to find function declarations
        func_pattern = r'^\s*(?:static\s+)?(?:inline\s+)?(?:const\s+)?(?:\w+\s+)*\*?\s*(\w+)\s*\([^)]*\)\s*;'
        functions = re.findall(func_pattern, content, re.MULTILINE)
        
        return functions
        
    def _generate_function_stub(self, func_name: str) -> str:
        """Generate stub implementation for function"""
        return f"""
// TODO: Implement {func_name}
void {func_name}_stub(void) {{
    // Implementation pending
}}

"""
        
    def _get_include_path(self, header_file: Path) -> str:
        """Get proper include path for header"""
        try:
            relative = header_file.relative_to(self.include_dir)
            return f"polycall/{relative}".replace('\\', '/')
        except:
            return header_file.name
            
    def _validate_source_header_match(self, header_file: Path, source_file: Path):
        """Validate that source file properly includes its header"""
        source_content = source_file.read_text()
        include_path = self._get_include_path(header_file)
        
        if f'#include "{include_path}"' not in source_content:
            self.violations.append(f"{source_file} does not include {include_path}")
            
    def _check_command_purity(self):
        """Check for cross-dependencies between command modules"""
        for module in self.command_modules:
            module_dir = self.core_dir / module
            if not module_dir.exists():
                continue
                
            for source_file in module_dir.glob("*.c"):
                self._check_file_dependencies(source_file, module)
                
    def _check_file_dependencies(self, source_file: Path, module: str):
        """Check single file for improper dependencies"""
        content = source_file.read_text()
        
        # Find all includes
        include_pattern = r'#include\s*[<"]([^>"]+)[>"]'
        includes = re.findall(include_pattern, content)
        
        for include in includes:
            # Check if it includes another command module
            for other_module in self.command_modules:
                if other_module != module and other_module in include:
                    self.violations.append(
                        f"Command purity violation: {source_file} "
                        f"(module: {module}) includes {other_module}"
                    )
                    
    def _generate_ioc_registry(self):
        """Generate IoC command registry for CLI"""
        registry_file = self.cli_dir / "command_registry_ioc.c"
        header_file = self.cli_dir / "command_registry_ioc.h"
        
        # Generate header
        header_content = """/*
 * command_registry_ioc.h
 * IoC Command Registry for PolyCall CLI
 * Auto-generated by OBINexus Migration Enforcer
 */

#ifndef POLYCALL_COMMAND_REGISTRY_IOC_H
#define POLYCALL_COMMAND_REGISTRY_IOC_H

#include <stddef.h>

typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char** argv);
} polycall_command_t;

typedef struct {
    polycall_command_t* commands;
    size_t count;
    size_t capacity;
} polycall_registry_t;

// IoC Registry API
polycall_registry_t* polycall_registry_create(void);
void polycall_registry_destroy(polycall_registry_t* registry);
int polycall_registry_register(polycall_registry_t* registry, 
                               const char* name, 
                               const char* description,
                               int (*handler)(int, char**));
int polycall_registry_execute(polycall_registry_t* registry,
                              const char* command,
                              int argc, char** argv);
void polycall_registry_list(polycall_registry_t* registry);

// Command module initializers
void polycall_register_micro_commands(polycall_registry_t* registry);
void polycall_register_telemetry_commands(polycall_registry_t* registry);
void polycall_register_edge_commands(polycall_registry_t* registry);
void polycall_register_auth_commands(polycall_registry_t* registry);
void polycall_register_config_commands(polycall_registry_t* registry);
void polycall_register_network_commands(polycall_registry_t* registry);

#endif /* POLYCALL_COMMAND_REGISTRY_IOC_H */
"""
        
        # Generate implementation
        impl_content = """/*
 * command_registry_ioc.c
 * IoC Command Registry Implementation
 * Auto-generated by OBINexus Migration Enforcer
 */

#include "command_registry_ioc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CAPACITY 16

polycall_registry_t* polycall_registry_create(void) {
    polycall_registry_t* registry = malloc(sizeof(polycall_registry_t));
    if (!registry) return NULL;
    
    registry->commands = malloc(sizeof(polycall_command_t) * INITIAL_CAPACITY);
    if (!registry->commands) {
        free(registry);
        return NULL;
    }
    
    registry->count = 0;
    registry->capacity = INITIAL_CAPACITY;
    return registry;
}

void polycall_registry_destroy(polycall_registry_t* registry) {
    if (!registry) return;
    
    if (registry->commands) {
        for (size_t i = 0; i < registry->count; i++) {
            free((char*)registry->commands[i].name);
            free((char*)registry->commands[i].description);
        }
        free(registry->commands);
    }
    free(registry);
}

int polycall_registry_register(polycall_registry_t* registry,
                               const char* name,
                               const char* description,
                               int (*handler)(int, char**)) {
    if (!registry || !name || !handler) return -1;
    
    // Expand capacity if needed
    if (registry->count >= registry->capacity) {
        size_t new_capacity = registry->capacity * 2;
        polycall_command_t* new_commands = realloc(registry->commands,
                                                    sizeof(polycall_command_t) * new_capacity);
        if (!new_commands) return -1;
        
        registry->commands = new_commands;
        registry->capacity = new_capacity;
    }
    
    // Add command
    registry->commands[registry->count].name = strdup(name);
    registry->commands[registry->count].description = description ? strdup(description) : strdup("");
    registry->commands[registry->count].handler = handler;
    registry->count++;
    
    return 0;
}

int polycall_registry_execute(polycall_registry_t* registry,
                              const char* command,
                              int argc, char** argv) {
    if (!registry || !command) return -1;
    
    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->commands[i].name, command) == 0) {
            return registry->commands[i].handler(argc, argv);
        }
    }
    
    fprintf(stderr, "Unknown command: %s\\n", command);
    return -1;
}

void polycall_registry_list(polycall_registry_t* registry) {
    if (!registry) return;
    
    printf("Available commands:\\n");
    for (size_t i = 0; i < registry->count; i++) {
        printf("  %-20s %s\\n", 
               registry->commands[i].name,
               registry->commands[i].description);
    }
}
"""
        
        header_file.write_text(header_content)
        registry_file.write_text(impl_content)
        
        self.fixes_applied.append(f"Generated IoC registry: {registry_file}")
        self.fixes_applied.append(f"Generated IoC header: {header_file}")
        
        # Generate main.c template
        self._generate_main_template()
        
    def _generate_main_template(self):
        """Generate IoC-based main.c template"""
        main_file = self.cli_dir / "main_ioc.c"
        
        content = """/*
 * main_ioc.c
 * PolyCall CLI Main Driver with IoC
 * Auto-generated by OBINexus Migration Enforcer
 */

#include "command_registry_ioc.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\\n", argv[0]);
        fprintf(stderr, "Use '%s help' for available commands\\n", argv[0]);
        return 1;
    }
    
    // Create IoC registry
    polycall_registry_t* registry = polycall_registry_create();
    if (!registry) {
        fprintf(stderr, "Failed to create command registry\\n");
        return 1;
    }
    
    // Register all command modules
    polycall_register_micro_commands(registry);
    polycall_register_telemetry_commands(registry);
    polycall_register_edge_commands(registry);
    polycall_register_auth_commands(registry);
    polycall_register_config_commands(registry);
    polycall_register_network_commands(registry);
    
    // Handle help command
    if (strcmp(argv[1], "help") == 0) {
        polycall_registry_list(registry);
        polycall_registry_destroy(registry);
        return 0;
    }
    
    // Execute command
    int result = polycall_registry_execute(registry, argv[1], argc - 2, argv + 2);
    
    // Cleanup
    polycall_registry_destroy(registry);
    return result;
}
"""
        
        main_file.write_text(content)
        self.fixes_applied.append(f"Generated IoC main: {main_file}")
        
    def _update_build_config(self):
        """Update Makefile to build in root/build/"""
        makefile_update = """
# Additional build targets for command purity
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj

# Ensure build directories exist
$(shell mkdir -p $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR))

# Library targets
LIBPOLYCALL_STATIC = $(LIB_DIR)/libpolycall.a
LIBPOLYCALL_SHARED = $(LIB_DIR)/libpolycall.so

# Main executable
POLYCALL_BIN = $(BIN_DIR)/polycall

# Command purity validation
validate-purity:
\t@echo "Validating command module purity..."
\t@python3 scripts/polycall_migration_enforcer.py --check-only

# Build static library
$(LIBPOLYCALL_STATIC): $(ALL_OBJECTS)
\t@echo "Building static library..."
\t@$(AR) rcs $@ $^

# Build shared library
$(LIBPOLYCALL_SHARED): $(ALL_OBJECTS)
\t@echo "Building shared library..."
\t@$(CC) -shared -o $@ $^ $(LDFLAGS)

# Build executable with IoC
$(POLYCALL_BIN): $(OBJ_DIR)/cli/main_ioc.o $(OBJ_DIR)/cli/command_registry_ioc.o $(LIBPOLYCALL_STATIC)
\t@echo "Building polycall executable..."
\t@$(CC) -o $@ $^ $(LDFLAGS)

# Install to build directory
install-local: $(POLYCALL_BIN) $(LIBPOLYCALL_STATIC) $(LIBPOLYCALL_SHARED)
\t@echo "Installation complete in $(BUILD_DIR)"
"""
        
        # Save build configuration
        build_mk = self.project_root / "Makefile.purity"
        build_mk.write_text(makefile_update)
        self.fixes_applied.append(f"Generated: {build_mk}")
        
    def _generate_report(self):
        """Generate migration compliance report"""
        report_dir = self.project_root / "reports"
        report_dir.mkdir(exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_file = report_dir / f"migration_report_{timestamp}.json"
        
        report = {
            "timestamp": timestamp,
            "project_root": str(self.project_root),
            "command_modules": list(self.command_modules),
            "infrastructure_modules": list(self.infrastructure_modules),
            "violations": self.violations,
            "fixes_applied": self.fixes_applied,
            "status": "PASSED" if len(self.violations) == 0 else "FAILED"
        }
        
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
            
        print(f"\nReport saved to: {report_file}")
        
        # Print summary
        print("\n=== Migration Summary ===")
        print(f"Violations found: {len(self.violations)}")
        print(f"Fixes applied: {len(self.fixes_applied)}")
        print(f"Status: {report['status']}")
        
        if self.violations:
            print("\nViolations:")
            for v in self.violations[:5]:  # Show first 5
                print(f"  - {v}")
            if len(self.violations) > 5:
                print(f"  ... and {len(self.violations) - 5} more")


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: python polycall_migration_enforcer.py <project_root> [--check-only]")
        sys.exit(1)
        
    project_root = sys.argv[1]
    check_only = "--check-only" in sys.argv
    
    enforcer = PolyCallMigrationEnforcer(project_root)
    
    if check_only:
        # Only check for violations
        enforcer._check_command_purity()
        if enforcer.violations:
            print("Command purity violations found!")
            for v in enforcer.violations:
                print(f"  - {v}")
            sys.exit(1)
        else:
            print("No command purity violations found.")
            sys.exit(0)
    else:
        # Run full migration
        success = enforcer.run_migration()
        sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()