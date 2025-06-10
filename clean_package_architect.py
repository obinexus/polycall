#!/usr/bin/env python3
"""
Sinphasé Package Architect - Minimal Working Implementation
Clean syntax with deterministic module resolution
"""

import os
import sys
from pathlib import Path

def create_package_structure(project_root: Path):
    """Create minimal but functional package structure."""
    print("🏗️ Creating Sinphasé package structure...")
    
    # Create package hierarchy
    package_root = project_root / "sinphase_governance"
    package_root.mkdir(exist_ok=True)
    
    # Core package directories
    dirs = [
        "core",
        "core/evaluator", 
        "core/detector",
        "core/reporter",
        "core/config",
        "cli"
    ]
    
    for dir_name in dirs:
        dir_path = package_root / dir_name
        dir_path.mkdir(parents=True, exist_ok=True)
        
        # Create __init__.py for package recognition
        init_file = dir_path / "__init__.py"
        init_file.write_text(f'"""Sinphasé {dir_name.split("/")[-1]} module."""\n')
        print(f"✅ Created: {dir_name}")
    
    # Create main package __init__.py
    main_init = package_root / "__init__.py"
    main_init.write_text('''"""
Sinphasé Governance Framework
Enterprise-grade governance for software architecture
"""

__version__ = "2.1.0"
__author__ = "OBINexus Computing"
''')
    
    print("✅ Package structure created successfully")
    return package_root

def create_minimal_modules(package_root: Path):
    """Create minimal functional modules for immediate testing."""
    print("📦 Creating minimal functional modules...")
    
    # Cost calculator module
    cost_calc_content = '''#!/usr/bin/env python3
"""Sinphasé Cost Calculator - Minimal Implementation"""

from pathlib import Path
from typing import Dict

def calculate_project_costs(project_root: Path) -> Dict[str, float]:
    """Calculate basic costs for project components."""
    costs = {}
    
    # Find source directories
    src_dirs = [
        project_root / "src",
        project_root / "libpolycall" / "src"
    ]
    
    for src_dir in src_dirs:
        if not src_dir.exists():
            continue
            
        for component_dir in src_dir.rglob("*"):
            if (component_dir.is_dir() and 
                any(component_dir.glob("*.c")) and
                "test" not in component_dir.name.lower()):
                
                # Simple cost calculation
                c_files = list(component_dir.glob("**/*.c"))
                cost = len(c_files) * 0.1  # Basic cost estimate
                
                relative_path = str(component_dir.relative_to(project_root))
                costs[relative_path] = cost
    
    return costs

class SinphaseCostCalculator:
    """Minimal cost calculator implementation."""
    
    def __init__(self):
        pass
    
    def calculate_component_cost(self, component_path: Path):
        """Calculate cost for component."""
        return {"component": component_path.name, "cost": 0.1}
'''
    
    cost_calc_file = package_root / "core/evaluator/cost_calculator.py"
    cost_calc_file.write_text(cost_calc_content)
    
    # Violation detector module
    violation_content = '''#!/usr/bin/env python3
"""Sinphasé Violation Detector - Minimal Implementation"""

from typing import Dict, List, Tuple
from dataclasses import dataclass

@dataclass
class ViolationSummary:
    """Basic violation summary."""
    total_files: int
    total_violations: int
    violation_percentage: float
    emergency_action_required: bool

class SinphaseViolationDetector:
    """Minimal violation detector implementation."""
    
    def __init__(self):
        pass
    
    def detect_violations(self, cost_results: Dict[str, float], threshold: float = 0.6):
        """Detect basic violations."""
        violations = []
        
        for file_path, cost in cost_results.items():
            if cost > threshold:
                violations.append({
                    "file_path": file_path,
                    "cost": cost,
                    "threshold": threshold
                })
        
        summary = ViolationSummary(
            total_files=len(cost_results),
            total_violations=len(violations),
            violation_percentage=len(violations) / len(cost_results) * 100 if cost_results else 0,
            emergency_action_required=len(violations) > 5
        )
        
        return violations, summary
'''
    
    violation_file = package_root / "core/detector/violation_scanner.py"
    violation_file.write_text(violation_content)
    
    # CLI module
    cli_content = '''#!/usr/bin/env python3
"""Sinphasé CLI - Minimal Implementation"""

import sys
import argparse
from pathlib import Path

try:
    from sinphase_governance.core.evaluator.cost_calculator import calculate_project_costs
    from sinphase_governance.core.detector.violation_scanner import SinphaseViolationDetector
except ImportError:
    print("⚠️ Package not properly installed, using relative imports")
    sys.path.insert(0, str(Path(__file__).parent.parent))
    from core.evaluator.cost_calculator import calculate_project_costs
    from core.detector.violation_scanner import SinphaseViolationDetector

def main():
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(description="Sinphasé Governance Framework")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--threshold", type=float, default=0.6, help="Governance threshold")
    
    args = parser.parse_args()
    
    project_root = Path(args.project_root).resolve()
    
    print(f"🔍 Sinphasé Governance Analysis")
    print(f"Project Root: {project_root}")
    print(f"Threshold: {args.threshold}")
    print()
    
    # Execute analysis
    detector = SinphaseViolationDetector()
    cost_results = calculate_project_costs(project_root)
    
    if not cost_results:
        print("⚠️ No components found for analysis")
        return
    
    violations, summary = detector.detect_violations(cost_results, args.threshold)
    
    # Display results
    print(f"📊 Analysis Results:")
    print(f"  Total Components: {summary.total_files}")
    print(f"  Violations: {summary.total_violations}")
    print(f"  Violation Rate: {summary.violation_percentage:.1f}%")
    
    if summary.emergency_action_required:
        print("🚨 EMERGENCY ACTION REQUIRED")
    
    if violations:
        print("\\n🔍 Violations:")
        for violation in violations[:5]:  # Show first 5
            print(f"  • {violation['file_path']}: {violation['cost']:.3f}")
    else:
        print("\\n✅ No violations detected")

if __name__ == "__main__":
    main()
'''
    
    cli_file = package_root / "cli/main.py"
    cli_file.write_text(cli_content)
    
    print("✅ Minimal modules created successfully")

def create_setup_files(project_root: Path):
    """Create basic setup files for package installation."""
    print("📋 Creating setup files...")
    
    # Basic setup.py
    setup_content = '''#!/usr/bin/env python3
"""Sinphasé Governance Framework Setup"""

from setuptools import setup, find_packages

setup(
    name="sinphase-governance",
    version="2.1.0",
    description="Enterprise governance framework",
    packages=find_packages(),
    python_requires=">=3.8",
    entry_points={
        "console_scripts": [
            "sinphase=sinphase_governance.cli.main:main",
        ],
    },
)
'''
    
    setup_file = project_root / "setup.py"
    setup_file.write_text(setup_content)
    
    print("✅ Setup files created")

def main():
    """Execute minimal package architecture implementation."""
    if len(sys.argv) < 2:
        print("Usage: python clean_package_architect.py <project_root>")
        sys.exit(1)
    
    project_root = Path(sys.argv[1]).resolve()
    
    if not project_root.exists():
        print(f"❌ Project root does not exist: {project_root}")
        sys.exit(1)
    
    print(f"🏗️ Sinphasé Clean Package Architecture Implementation")
    print(f"Project Root: {project_root}")
    print()
    
    try:
        # Create package structure
        package_root = create_package_structure(project_root)
        
        # Create minimal modules
        create_minimal_modules(package_root)
        
        # Create setup files
        create_setup_files(project_root)
        
        print("\\n✅ Package architecture implementation completed!")
        print("\\nValidation Commands:")
        print(f"  cd {project_root}")
        print("  python -c \"import sinphase_governance; print('✅ Package import successful')\"")
        print("  python -m sinphase_governance.cli.main --project-root .")
        
        # Immediate validation
        print("\\n🧪 Testing package import...")
        sys.path.insert(0, str(project_root))
        
        try:
            import sinphase_governance
            print("✅ Package import test successful!")
        except ImportError as e:
            print(f"⚠️ Package import test failed: {e}")
            print("   (This is expected - install with 'pip install -e .' for full functionality)")
        
    except Exception as e:
        print(f"❌ Implementation failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
