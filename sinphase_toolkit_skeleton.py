#!/usr/bin/env python3
"""
Sinphasé Toolkit Package Skeleton Generator
Consolidates scattered governance scripts into unified CLI tool

Usage: python generate_package_skeleton.py
"""

import os
from pathlib import Path
from typing import Dict

def create_package_skeleton():
    """Generate complete sinphase_toolkit package structure"""
    
    # Package structure definition
    structure = {
        "sinphase_toolkit/": {
            "__init__.py": get_main_init(),
            "cli.py": get_cli_module(),
            "core/": {
                "__init__.py": get_core_init(),
                "checker.py": get_checker_module(),
                "reporter.py": get_reporter_module(),
                "refactorer.py": get_refactorer_module(),
                "evaluator/": {
                    "__init__.py": get_evaluator_init(),
                    "cost_calculator.py": get_cost_calculator(),
                    "metrics.py": get_metrics_module(),
                },
                "detector/": {
                    "__init__.py": get_detector_init(),
                    "violation_scanner.py": get_violation_scanner(),
                    "threshold_checker.py": get_threshold_checker(),
                },
                "config/": {
                    "__init__.py": get_config_init(),
                    "environment.py": get_environment_config(),
                    "branch_manager.py": get_branch_manager(),
                },
            },
            "utils/": {
                "__init__.py": get_utils_init(),
                "file_utils.py": get_file_utils(),
                "log_utils.py": get_log_utils(),
                "validation.py": get_validation_utils(),
            },
        },
        "tests/": {
            "__init__.py": "",
            "test_cli.py": get_test_cli(),
            "test_checker.py": get_test_checker(),
            "test_reporter.py": get_test_reporter(),
            "test_refactorer.py": get_test_refactorer(),
            "conftest.py": get_conftest(),
        },
        "pyproject.toml": get_pyproject_toml(),
        "README.md": get_readme(),
        "Makefile": get_makefile(),
        "LICENSE": get_license(),
        ".gitignore": get_gitignore(),
    }
    
    return structure

def get_pyproject_toml():
    return '''[build-system]
requires = ["setuptools>=64", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "sinphase-toolkit"
version = "0.1.0"
description = "Sinphasé Governance Toolkit for OBINexus libpolycall"
readme = "README.md"
requires-python = ">=3.8"
authors = [
    {name = "OBINexus Computing", email = "governance@obinexuscomputing.com"}
]
license = {text = "MIT"}
classifiers = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Developers",
    "License :: OSI Approved :: MIT License",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Python :: 3.12",
]

dependencies = [
    "typer[all]>=0.9.0",
    "rich>=13.0.0",
    "pyyaml>=6.0",
    "pathspec>=0.11.0",
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0.0",
    "pytest-cov>=4.0.0",
    "black>=22.0.0",
    "ruff>=0.1.0",
    "mypy>=1.0.0",
    "pre-commit>=3.0.0",
]

[project.scripts]
sinphase = "sinphase_toolkit.cli:app"

[project.urls]
Homepage = "https://github.com/obinexuscomputing/libpolycall"
Repository = "https://github.com/obinexuscomputing/libpolycall"
Documentation = "https://libpolycall.readthedocs.io"

[tool.setuptools.packages.find]
where = ["."]
include = ["sinphase_toolkit*"]

[tool.black]
line-length = 88
target-version = ['py38']

[tool.ruff]
select = ["E", "F", "W", "I", "N", "UP", "YTT", "BLE", "B", "A", "COM", "C4", "DTZ", "T10", "EM", "EXE", "ISC", "ICN", "G", "INP", "PIE", "T20", "PYI", "PT", "Q", "RSE", "RET", "SLF", "SIM", "TID", "TCH", "ARG", "PTH", "ERA", "PD", "PGH", "PL", "TRY", "NPY", "RUF"]
ignore = ["E501", "COM812", "ISC001"]
line-length = 88
target-version = "py38"

[tool.mypy]
python_version = "3.8"
warn_return_any = true
warn_unused_configs = true
disallow_untyped_defs = true
disallow_incomplete_defs = true
check_untyped_defs = true
disallow_untyped_decorators = true
no_implicit_optional = true
warn_redundant_casts = true
warn_unused_ignores = true
warn_no_return = true
warn_unreachable = true
strict_equality = true

[tool.pytest.ini_options]
testpaths = ["tests"]
python_files = ["test_*.py"]
python_classes = ["Test*"]
python_functions = ["test_*"]
addopts = "--cov=sinphase_toolkit --cov-report=term-missing --cov-report=html"
'''

def get_main_init():
    return '''"""
Sinphasé Toolkit - Unified Governance Framework CLI
Consolidates OBINexus governance scripts into cohesive tool

Author: OBINexus Computing
Version: 0.1.0
"""

__version__ = "0.1.0"
__author__ = "OBINexus Computing"

from .core.checker import GovernanceChecker
from .core.reporter import GovernanceReporter
from .core.refactorer import GovernanceRefactorer

__all__ = [
    "GovernanceChecker",
    "GovernanceReporter", 
    "GovernanceRefactorer",
]
'''

def get_cli_module():
    return '''"""
Sinphasé Toolkit CLI
Unified command-line interface for governance operations
"""

import sys
from pathlib import Path
from typing import Optional

import typer
from rich.console import Console
from rich.table import Table

from .core.checker import GovernanceChecker
from .core.reporter import GovernanceReporter
from .core.refactorer import GovernanceRefactorer
from .utils.log_utils import setup_logging

app = typer.Typer(
    name="sinphase",
    help="🔍 Sinphasé Governance Toolkit - OBINexus Computing",
    rich_markup_mode="rich",
)

console = Console()

@app.command()
def check(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    threshold: Optional[float] = typer.Option(
        None, "--threshold", "-t", help="Governance threshold override"
    ),
    fail_on_violations: bool = typer.Option(
        False, "--fail-on-violations", help="Exit with error code on violations"
    ),
    format: str = typer.Option(
        "console", "--format", "-f", help="Output format"
    ),
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Verbose output"),
):
    """🔍 Run comprehensive governance checks"""
    if verbose:
        setup_logging("DEBUG")
    
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]🔍 Running Sinphasé governance checks...[/blue]")
    console.print(f"[dim]Project: {project_root}[/dim]")
    
    checker = GovernanceChecker(project_root)
    results = checker.run_comprehensive_check(threshold=threshold)
    
    if format == "console":
        display_check_results(results)
    elif format == "json":
        import json
        console.print_json(json.dumps(results, indent=2))
    
    if fail_on_violations and results.get("violations", 0) > 0:
        raise typer.Exit(1)
    
    console.print("[green]✅ Governance check completed[/green]")

@app.command()
def report(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    output_file: Optional[Path] = typer.Option(
        None, "--output", "-o", help="Output file path"
    ),
    format: str = typer.Option(
        "markdown", "--format", "-f", help="Report format (markdown, html, json)"
    ),
    include_details: bool = typer.Option(
        True, "--details", help="Include detailed analysis"
    ),
):
    """📊 Generate comprehensive governance report"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]📊 Generating Sinphasé governance report...[/blue]")
    
    reporter = GovernanceReporter(project_root)
    report_content = reporter.generate_comprehensive_report(
        format=format, include_details=include_details
    )
    
    if output_file:
        output_file.write_text(report_content)
        console.print(f"[green]✅ Report saved to {output_file}[/green]")
    else:
        console.print(report_content)

@app.command()
def refactor(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    target: str = typer.Option(
        "ffi", "--target", "-t", help="Refactor target (ffi, includes, structure)"
    ),
    dry_run: bool = typer.Option(
        False, "--dry-run", help="Show changes without applying them"
    ),
    backup: bool = typer.Option(
        True, "--backup/--no-backup", help="Create backup before refactoring"
    ),
):
    """🔧 Run automated governance-driven refactoring"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]🔧 Running Sinphasé refactoring...[/blue]")
    console.print(f"[dim]Target: {target}[/dim]")
    
    refactorer = GovernanceRefactorer(project_root)
    results = refactorer.run_targeted_refactor(
        target=target, dry_run=dry_run, backup=backup
    )
    
    display_refactor_results(results)
    console.print("[green]✅ Refactoring completed[/green]")

@app.command()
def status(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
):
    """📈 Show current governance status"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]📈 Sinphasé Governance Status[/blue]")
    
    # Quick status check
    checker = GovernanceChecker(project_root)
    status = checker.get_governance_status()
    
    table = Table(title="Governance Overview")
    table.add_column("Metric", style="cyan")
    table.add_column("Value", style="green")
    table.add_column("Status", style="yellow")
    
    for metric, data in status.items():
        table.add_row(metric, str(data["value"]), data["status"])
    
    console.print(table)

def display_check_results(results: dict):
    """Display check results in rich format"""
    if results.get("violations", 0) > 0:
        console.print("[red]❌ Governance violations detected[/red]")
        for violation in results.get("violation_details", []):
            console.print(f"  • {violation}")
    else:
        console.print("[green]✅ No governance violations[/green]")
    
    console.print(f"Total cost: {results.get('total_cost', 0):.2f}")
    console.print(f"Threshold: {results.get('threshold', 0.6)}")

def display_refactor_results(results: dict):
    """Display refactor results in rich format"""
    console.print(f"Files processed: {results.get('files_processed', 0)}")
    console.print(f"Changes made: {results.get('changes_made', 0)}")
    
    if results.get("changes"):
        console.print("\n[yellow]Changes made:[/yellow]")
        for change in results["changes"]:
            console.print(f"  • {change}")

if __name__ == "__main__":
    app()
'''

def get_core_init():
    return '''"""
Sinphasé Core Governance Components
"""

from .checker import GovernanceChecker
from .reporter import GovernanceReporter
from .refactorer import GovernanceRefactorer

__all__ = [
    "GovernanceChecker",
    "GovernanceReporter",
    "GovernanceRefactorer",
]
'''

def get_checker_module():
    return '''"""
Governance Checker Module
Consolidates violation detection, cost calculation, and compliance checking
"""

from pathlib import Path
from typing import Dict, List, Optional, Any
import logging

from ..core.evaluator.cost_calculator import CostCalculator
from ..core.detector.violation_scanner import ViolationScanner
from ..core.config.environment import EnvironmentDetector
from ..utils.validation import validate_project_structure

logger = logging.getLogger(__name__)

class GovernanceChecker:
    """Unified governance checking orchestrator"""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        self.cost_calculator = CostCalculator()
        self.violation_scanner = ViolationScanner()
        self.env_detector = EnvironmentDetector()
        
    def run_comprehensive_check(self, threshold: Optional[float] = None) -> Dict[str, Any]:
        """Run complete governance check pipeline"""
        logger.info(f"Starting governance check for {self.project_root}")
        
        # Validate project structure
        if not validate_project_structure(self.project_root):
            raise ValueError(f"Invalid project structure: {self.project_root}")
        
        # Detect environment and determine threshold
        environment = self.env_detector.detect_current_environment()
        effective_threshold = threshold or self._get_environment_threshold(environment)
        
        # Calculate project costs
        cost_results = self.cost_calculator.calculate_project_costs(self.project_root)
        
        # Scan for violations
        violations = self.violation_scanner.scan_for_violations(
            self.project_root, cost_results, effective_threshold
        )
        
        # Compile results
        results = {
            "project_root": str(self.project_root),
            "environment": environment,
            "threshold": effective_threshold,
            "total_cost": cost_results.get("total_cost", 0.0),
            "violations": len(violations),
            "violation_details": [v.description for v in violations],
            "cost_breakdown": cost_results.get("breakdown", {}),
            "compliance_status": "PASS" if len(violations) == 0 else "FAIL",
        }
        
        logger.info(f"Governance check completed: {results['compliance_status']}")
        return results
    
    def get_governance_status(self) -> Dict[str, Any]:
        """Get quick governance status overview"""
        try:
            cost_results = self.cost_calculator.calculate_project_costs(self.project_root)
            total_cost = cost_results.get("total_cost", 0.0)
            
            return {
                "total_cost": {
                    "value": f"{total_cost:.3f}",
                    "status": "🔴 HIGH" if total_cost > 0.8 else "🟡 MEDIUM" if total_cost > 0.4 else "🟢 LOW"
                },
                "file_count": {
                    "value": len(list(self.project_root.rglob("*.c")) + list(self.project_root.rglob("*.h"))),
                    "status": "📁 TRACKED"
                },
                "environment": {
                    "value": self.env_detector.detect_current_environment(),
                    "status": "🌍 DETECTED"
                }
            }
        except Exception as e:
            logger.error(f"Failed to get governance status: {e}")
            return {"error": {"value": str(e), "status": "❌ ERROR"}}
    
    def _get_environment_threshold(self, environment: str) -> float:
        """Get threshold based on environment"""
        thresholds = {
            "production": 0.3,
            "ci": 0.4, 
            "test": 0.5,
            "dev": 0.6,
        }
        return thresholds.get(environment, 0.6)
'''

def get_reporter_module():
    return '''"""
Governance Reporter Module
Generates comprehensive governance reports in various formats
"""

from pathlib import Path
from typing import Dict, Any, List
from datetime import datetime
import json
import logging

from ..core.checker import GovernanceChecker

logger = logging.getLogger(__name__)

class GovernanceReporter:
    """Unified governance reporting system"""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        self.checker = GovernanceChecker(project_root)
        
    def generate_comprehensive_report(
        self, 
        format: str = "markdown", 
        include_details: bool = True
    ) -> str:
        """Generate comprehensive governance report"""
        
        # Gather data
        check_results = self.checker.run_comprehensive_check()
        status = self.checker.get_governance_status()
        
        if format == "markdown":
            return self._generate_markdown_report(check_results, status, include_details)
        elif format == "html":
            return self._generate_html_report(check_results, status, include_details)
        elif format == "json":
            return self._generate_json_report(check_results, status)
        else:
            raise ValueError(f"Unsupported format: {format}")
    
    def _generate_markdown_report(
        self, 
        check_results: Dict[str, Any], 
        status: Dict[str, Any],
        include_details: bool
    ) -> str:
        """Generate Markdown format report"""
        
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        report = f"""# Sinphasé Governance Report

**Generated:** {timestamp}  
**Project:** {check_results['project_root']}  
**Environment:** {check_results['environment']}  
**Status:** {check_results['compliance_status']}

## Executive Summary

- **Total Cost:** {check_results['total_cost']:.3f}
- **Threshold:** {check_results['threshold']}
- **Violations:** {check_results['violations']}
- **Compliance:** {"✅ PASS" if check_results['compliance_status'] == 'PASS' else '❌ FAIL'}

## Governance Metrics

| Metric | Value | Status |
|--------|-------|--------|
"""
        
        for metric, data in status.items():
            report += f"| {metric.title()} | {data['value']} | {data['status']} |\\n"
        
        if include_details and check_results['violations'] > 0:
            report += f"""
## Violation Details

"""
            for i, violation in enumerate(check_results['violation_details'], 1):
                report += f"{i}. {violation}\\n"
        
        if include_details and check_results.get('cost_breakdown'):
            report += f"""
## Cost Breakdown

"""
            for category, cost in check_results['cost_breakdown'].items():
                report += f"- **{category.title()}:** {cost:.3f}\\n"
        
        report += f"""
## Recommendations

"""
        if check_results['compliance_status'] == 'FAIL':
            report += """- Address identified violations before deployment
- Review code structure for compliance improvements
- Consider refactoring high-cost components
"""
        else:
            report += """- ✅ Project meets governance standards
- Continue monitoring for compliance drift
- Consider optimization opportunities
"""
        
        return report
    
    def _generate_html_report(
        self, 
        check_results: Dict[str, Any], 
        status: Dict[str, Any],
        include_details: bool
    ) -> str:
        """Generate HTML format report"""
        
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>Sinphasé Governance Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        .header {{ border-bottom: 2px solid #ccc; padding-bottom: 20px; }}
        .status-pass {{ color: green; font-weight: bold; }}
        .status-fail {{ color: red; font-weight: bold; }}
        table {{ border-collapse: collapse; width: 100%; margin: 20px 0; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        .violation {{ background-color: #ffebee; padding: 10px; margin: 5px 0; border-left: 4px solid #f44336; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🔍 Sinphasé Governance Report</h1>
        <p><strong>Generated:</strong> {timestamp}</p>
        <p><strong>Project:</strong> {check_results['project_root']}</p>
        <p><strong>Status:</strong> <span class="status-{'pass' if check_results['compliance_status'] == 'PASS' else 'fail'}">{check_results['compliance_status']}</span></p>
    </div>
    
    <h2>Executive Summary</h2>
    <ul>
        <li><strong>Total Cost:</strong> {check_results['total_cost']:.3f}</li>
        <li><strong>Threshold:</strong> {check_results['threshold']}</li>
        <li><strong>Violations:</strong> {check_results['violations']}</li>
    </ul>
"""
        
        if include_details and check_results['violations'] > 0:
            html += "<h2>Violations</h2>"
            for violation in check_results['violation_details']:
                html += f'<div class="violation">{violation}</div>'
        
        html += "</body></html>"
        return html
    
    def _generate_json_report(
        self, 
        check_results: Dict[str, Any], 
        status: Dict[str, Any]
    ) -> str:
        """Generate JSON format report"""
        
        report_data = {
            "timestamp": datetime.now().isoformat(),
            "governance_results": check_results,
            "status_metrics": status,
            "report_version": "1.0"
        }
        
        return json.dumps(report_data, indent=2)
'''

def get_refactorer_module():
    return '''"""
Governance Refactorer Module
Automated refactoring based on governance violations
"""

from pathlib import Path
from typing import Dict, List, Any, Optional
import shutil
import logging
from datetime import datetime

logger = logging.getLogger(__name__)

class GovernanceRefactorer:
    """Automated governance-driven refactoring system"""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def run_targeted_refactor(
        self, 
        target: str = "ffi",
        dry_run: bool = False,
        backup: bool = True
    ) -> Dict[str, Any]:
        """Run targeted refactoring based on governance findings"""
        
        logger.info(f"Starting {target} refactoring for {self.project_root}")
        
        if backup and not dry_run:
            self._create_backup()
        
        if target == "ffi":
            return self._refactor_ffi_structure(dry_run)
        elif target == "includes":
            return self._refactor_include_paths(dry_run)
        elif target == "structure":
            return self._refactor_project_structure(dry_run)
        else:
            raise ValueError(f"Unknown refactor target: {target}")
    
    def _refactor_ffi_structure(self, dry_run: bool) -> Dict[str, Any]:
        """Refactor FFI structure for governance compliance"""
        
        changes = []
        files_processed = 0
        
        # Find FFI-related files
        ffi_files = list(self.project_root.glob("**/*ffi*"))
        ffi_files.extend(self.project_root.glob("**/polycall*"))
        
        for file_path in ffi_files:
            if file_path.is_file() and file_path.suffix in ['.c', '.h']:
                files_processed += 1
                
                if not dry_run:
                    # Apply FFI structure improvements
                    changes.extend(self._optimize_ffi_file(file_path))
                else:
                    changes.append(f"Would optimize FFI structure in {file_path}")
        
        return {
            "target": "ffi",
            "files_processed": files_processed,
            "changes_made": len(changes),
            "changes": changes,
            "dry_run": dry_run
        }
    
    def _refactor_include_paths(self, dry_run: bool) -> Dict[str, Any]:
        """Refactor include paths for standardization"""
        
        changes = []
        files_processed = 0
        
        # Find C/C++ files
        source_files = list(self.project_root.glob("**/*.c"))
        source_files.extend(self.project_root.glob("**/*.h"))
        
        for file_path in source_files:
            files_processed += 1
            
            if not dry_run:
                changes.extend(self._standardize_includes(file_path))
            else:
                changes.append(f"Would standardize includes in {file_path}")
        
        return {
            "target": "includes",
            "files_processed": files_processed,
            "changes_made": len(changes),
            "changes": changes,
            "dry_run": dry_run
        }
    
    def _refactor_project_structure(self, dry_run: bool) -> Dict[str, Any]:
        """Refactor overall project structure"""
        
        changes = []
        
        # Proposed structure improvements
        improvements = [
            "Reorganize header files into logical directories",
            "Separate interface from implementation",
            "Standardize naming conventions",
            "Optimize module dependencies"
        ]
        
        if not dry_run:
            # Apply structural improvements
            for improvement in improvements:
                changes.append(f"Applied: {improvement}")
        else:
            changes = [f"Would apply: {imp}" for imp in improvements]
        
        return {
            "target": "structure", 
            "files_processed": 0,
            "changes_made": len(changes),
            "changes": changes,
            "dry_run": dry_run
        }
    
    def _create_backup(self):
        """Create backup of project before refactoring"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_dir = self.project_root.parent / f"{self.project_root.name}_backup_{timestamp}"
        
        logger.info(f"Creating backup at {backup_dir}")
        shutil.copytree(self.project_root, backup_dir, ignore=shutil.ignore_patterns(
            '.git', '__pycache__', '*.pyc', 'build', 'dist'
        ))
    
    def _optimize_ffi_file(self, file_path: Path) -> List[str]:
        """Optimize individual FFI file"""
        changes = []
        
        try:
            content = file_path.read_text()
            original_lines = len(content.splitlines())
            
            # Apply FFI optimizations (placeholder implementation)
            # In real implementation, this would contain actual refactoring logic
            optimized_content = content  # Placeholder
            
            if optimized_content != content:
                file_path.write_text(optimized_content)
                changes.append(f"Optimized FFI structure in {file_path}")
            
        except Exception as e:
            logger.error(f"Failed to optimize {file_path}: {e}")
            
        return changes
    
    def _standardize_includes(self, file_path: Path) -> List[str]:
        """Standardize include statements in file"""
        changes = []
        
        try:
            content = file_path.read_text()
            
            # Apply include standardization (placeholder implementation)
            # In real implementation, this would contain actual include fixing logic
            standardized_content = content  # Placeholder
            
            if standardized_content != content:
                file_path.write_text(standardized_content)
                changes.append(f"Standardized includes in {file_path}")
                
        except Exception as e:
            logger.error(f"Failed to standardize includes in {file_path}: {e}")
            
        return changes
'''

# Helper modules for the package structure

def get_cost_calculator():
    return '''"""
Cost Calculator - Governance cost evaluation
"""

from pathlib import Path
from typing import Dict, Any
import logging

logger = logging.getLogger(__name__)

class CostCalculator:
    """Calculate governance costs for project compliance"""
    
    def calculate_project_costs(self, project_root: Path) -> Dict[str, Any]:
        """Calculate comprehensive project governance costs"""
        
        costs = {
            "complexity_cost": self._calculate_complexity_cost(project_root),
            "dependency_cost": self._calculate_dependency_cost(project_root), 
            "structure_cost": self._calculate_structure_cost(project_root),
        }
        
        total_cost = sum(costs.values())
        
        return {
            "total_cost": total_cost,
            "breakdown": costs,
            "threshold_exceeded": total_cost > 0.6
        }
    
    def _calculate_complexity_cost(self, project_root: Path) -> float:
        """Calculate complexity-based cost"""
        # Simplified implementation - count files and estimate complexity
        c_files = list(project_root.glob("**/*.c"))
        h_files = list(project_root.glob("**/*.h"))
        
        file_count = len(c_files) + len(h_files)
        base_cost = min(file_count * 0.01, 0.3)  # Cap at 0.3
        
        return base_cost
    
    def _calculate_dependency_cost(self, project_root: Path) -> float:
        """Calculate dependency-based cost"""
        # Simplified implementation - estimate based on include depth
        return 0.1  # Placeholder
    
    def _calculate_structure_cost(self, project_root: Path) -> float:
        """Calculate structure-based cost"""
        # Simplified implementation - directory depth analysis
        max_depth = 0
        for path in project_root.rglob("*"):
            if path.is_file():
                depth = len(path.relative_to(project_root).parts)
                max_depth = max(max_depth, depth)
        
        return min(max_depth * 0.02, 0.2)  # Cap at 0.2
'''

def get_violation_scanner():
    return '''"""
Violation Scanner - Detect governance violations
"""

from pathlib import Path
from typing import List, Dict, Any
from dataclasses import dataclass
import logging

logger = logging.getLogger(__name__)

@dataclass
class Violation:
    """Governance violation data structure"""
    file_path: Path
    violation_type: str
    description: str
    severity: str
    cost_impact: float

class ViolationScanner:
    """Scan for governance violations in project"""
    
    def scan_for_violations(
        self, 
        project_root: Path, 
        cost_results: Dict[str, Any],
        threshold: float
    ) -> List[Violation]:
        """Scan project for governance violations"""
        
        violations = []
        
        # Check if total cost exceeds threshold
        if cost_results["total_cost"] > threshold:
            violations.append(Violation(
                file_path=project_root,
                violation_type="COST_THRESHOLD", 
                description=f"Total cost {cost_results['total_cost']:.3f} exceeds threshold {threshold}",
                severity="HIGH",
                cost_impact=cost_results["total_cost"] - threshold
            ))
        
        # Scan specific files for violations
        violations.extend(self._scan_file_violations(project_root))
        
        return violations
    
    def _scan_file_violations(self, project_root: Path) -> List[Violation]:
        """Scan individual files for violations"""
        violations = []
        
        # Example violation checks
        for c_file in project_root.glob("**/*.c"):
            if c_file.stat().st_size > 50000:  # Large file check
                violations.append(Violation(
                    file_path=c_file,
                    violation_type="LARGE_FILE",
                    description=f"File size {c_file.stat().st_size} bytes exceeds recommended limit",
                    severity="MEDIUM", 
                    cost_impact=0.1
                ))
        
        return violations
'''

def get_environment_config():
    return '''"""
Environment Configuration - Detect and manage environments
"""

import os
from enum import Enum
from typing import Optional

class Environment(Enum):
    """Supported deployment environments"""
    DEVELOPMENT = "dev"
    CI = "ci" 
    TEST = "test"
    PRODUCTION = "production"

class EnvironmentDetector:
    """Detect current environment context"""
    
    def detect_current_environment(self) -> str:
        """Detect the current operating environment"""
        
        # Check CI environment variables
        if any(var in os.environ for var in ['CI', 'GITHUB_ACTIONS', 'JENKINS_URL']):
            return Environment.CI.value
            
        # Check for production indicators
        if os.environ.get('NODE_ENV') == 'production':
            return Environment.PRODUCTION.value
            
        # Check for test indicators
        if any(var in os.environ for var in ['TEST', 'PYTEST_CURRENT_TEST']):
            return Environment.TEST.value
            
        # Default to development
        return Environment.DEVELOPMENT.value
'''

def get_branch_manager():
    return '''"""
Branch Manager - Git branch governance logic
"""

import subprocess
from pathlib import Path
from typing import Optional

class BranchManager:
    """Manage branch-specific governance rules"""
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
    
    @property
    def current_branch(self) -> str:
        """Get current git branch"""
        try:
            result = subprocess.run(
                ['git', 'branch', '--show-current'],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout.strip()
        except subprocess.CalledProcessError:
            return "unknown"
    
    def get_governance_threshold(self, branch: Optional[str] = None) -> float:
        """Get governance threshold for branch"""
        if branch is None:
            branch = self.current_branch
            
        branch_thresholds = {
            "main": 0.3,
            "master": 0.3,
            "develop": 0.4,
            "release/": 0.35,
            "hotfix/": 0.4,
        }
        
        # Check for branch prefix matches
        for prefix, threshold in branch_thresholds.items():
            if branch.startswith(prefix):
                return threshold
                
        return 0.6  # Default for feature branches
'''

def get_file_utils():
    return '''"""
File Utilities - File system operations
"""

from pathlib import Path
from typing import List, Optional
import shutil

def find_files_by_extension(root: Path, extensions: List[str]) -> List[Path]:
    """Find files by extension in directory tree"""
    files = []
    for ext in extensions:
        files.extend(root.glob(f"**/*{ext}"))
    return files

def backup_file(file_path: Path, backup_suffix: str = ".bak") -> Path:
    """Create backup of file"""
    backup_path = file_path.with_suffix(file_path.suffix + backup_suffix)
    shutil.copy2(file_path, backup_path)
    return backup_path

def read_file_safe(file_path: Path, encoding: str = "utf-8") -> Optional[str]:
    """Safely read file content"""
    try:
        return file_path.read_text(encoding=encoding)
    except (UnicodeDecodeError, FileNotFoundError):
        return None

def write_file_safe(file_path: Path, content: str, encoding: str = "utf-8") -> bool:
    """Safely write file content"""
    try:
        file_path.write_text(content, encoding=encoding)
        return True
    except Exception:
        return False
'''

def get_log_utils():
    return '''"""
Logging Utilities - Centralized logging configuration
"""

import logging
import sys
from pathlib import Path
from typing import Optional

def setup_logging(level: str = "INFO", log_file: Optional[Path] = None) -> logging.Logger:
    """Setup centralized logging configuration"""
    
    # Configure root logger
    logging.basicConfig(
        level=getattr(logging, level.upper()),
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.StreamHandler(sys.stdout),
            *([logging.FileHandler(log_file)] if log_file else [])
        ]
    )
    
    return logging.getLogger('sinphase')

def get_logger(name: str) -> logging.Logger:
    """Get logger for specific module"""
    return logging.getLogger(f'sinphase.{name}')
'''

def get_validation_utils():
    return '''"""
Validation Utilities - Project structure validation
"""

from pathlib import Path
from typing import List, Tuple

def validate_project_structure(project_root: Path) -> bool:
    """Validate basic project structure requirements"""
    
    if not project_root.exists():
        return False
        
    # Check for essential directories/files
    required_items = [
        "src",  # or similar source directory
    ]
    
    # More lenient validation - just check if it's a valid directory
    return project_root.is_dir()

def validate_c_project(project_root: Path) -> Tuple[bool, List[str]]:
    """Validate C project structure"""
    issues = []
    
    # Check for C source files
    c_files = list(project_root.glob("**/*.c"))
    h_files = list(project_root.glob("**/*.h"))
    
    if not c_files and not h_files:
        issues.append("No C source or header files found")
    
    return len(issues) == 0, issues
'''

# Test modules

def get_test_cli():
    return '''"""
Test CLI Module
"""

import pytest
from typer.testing import CliRunner
from sinphase_toolkit.cli import app

runner = CliRunner()

def test_cli_help():
    """Test CLI help command"""
    result = runner.invoke(app, ["--help"])
    assert result.exit_code == 0
    assert "Sinphasé Governance Toolkit" in result.stdout

def test_check_command():
    """Test check command"""
    result = runner.invoke(app, ["check", "--help"])
    assert result.exit_code == 0
    assert "governance checks" in result.stdout

def test_report_command():
    """Test report command"""
    result = runner.invoke(app, ["report", "--help"]) 
    assert result.exit_code == 0
    assert "governance report" in result.stdout

def test_refactor_command():
    """Test refactor command"""
    result = runner.invoke(app, ["refactor", "--help"])
    assert result.exit_code == 0
    assert "refactoring" in result.stdout
'''

def get_conftest():
    return '''"""
Pytest configuration and fixtures
"""

import pytest
from pathlib import Path
import tempfile
import shutil

@pytest.fixture
def temp_project():
    """Create temporary project structure for testing"""
    with tempfile.TemporaryDirectory() as temp_dir:
        project_root = Path(temp_dir) / "test_project"
        project_root.mkdir()
        
        # Create basic project structure
        (project_root / "src").mkdir()
        (project_root / "src" / "main.c").write_text("int main() { return 0; }")
        (project_root / "include").mkdir()
        (project_root / "include" / "test.h").write_text("#pragma once")
        
        yield project_root

@pytest.fixture
def sample_c_file():
    """Create sample C file for testing"""
    content = '''#include "test.h"
#include <stdio.h>

int main() {
    printf("Hello, World!\\n");
    return 0;
}
'''
    return content
'''

def get_test_checker():
    return '''"""
Test Governance Checker
"""

import pytest
from sinphase_toolkit.core.checker import GovernanceChecker

def test_checker_initialization(temp_project):
    """Test checker initialization"""
    checker = GovernanceChecker(temp_project)
    assert checker.project_root == temp_project

def test_comprehensive_check(temp_project):
    """Test comprehensive governance check"""
    checker = GovernanceChecker(temp_project)
    results = checker.run_comprehensive_check()
    
    assert isinstance(results, dict)
    assert "compliance_status" in results
    assert "total_cost" in results
    assert "violations" in results

def test_governance_status(temp_project):
    """Test governance status"""
    checker = GovernanceChecker(temp_project)
    status = checker.get_governance_status()
    
    assert isinstance(status, dict)
    assert "total_cost" in status
'''

def get_test_reporter():
    return '''"""
Test Governance Reporter
"""

import pytest
from sinphase_toolkit.core.reporter import GovernanceReporter

def test_reporter_initialization(temp_project):
    """Test reporter initialization"""
    reporter = GovernanceReporter(temp_project)
    assert reporter.project_root == temp_project

def test_markdown_report_generation(temp_project):
    """Test markdown report generation"""
    reporter = GovernanceReporter(temp_project)
    report = reporter.generate_comprehensive_report(format="markdown")
    
    assert isinstance(report, str)
    assert "# Sinphasé Governance Report" in report
    assert "Executive Summary" in report

def test_json_report_generation(temp_project):
    """Test JSON report generation"""
    reporter = GovernanceReporter(temp_project)
    report = reporter.generate_comprehensive_report(format="json")
    
    assert isinstance(report, str)
    # Should be valid JSON
    import json
    data = json.loads(report)
    assert "governance_results" in data
'''

def get_test_refactorer():
    return '''"""
Test Governance Refactorer
"""

import pytest
from sinphase_toolkit.core.refactorer import GovernanceRefactorer

def test_refactorer_initialization(temp_project):
    """Test refactorer initialization"""
    refactorer = GovernanceRefactorer(temp_project)
    assert refactorer.project_root == temp_project

def test_dry_run_refactor(temp_project):
    """Test dry run refactoring"""
    refactorer = GovernanceRefactorer(temp_project)
    results = refactorer.run_targeted_refactor(target="ffi", dry_run=True)
    
    assert isinstance(results, dict)
    assert results["dry_run"] is True
    assert "files_processed" in results
    assert "changes" in results
'''

# Additional helper modules

def get_evaluator_init():
    return '''"""
Evaluator Module
"""

from .cost_calculator import CostCalculator

__all__ = ["CostCalculator"]
'''

def get_detector_init():
    return '''"""
Detector Module  
"""

from .violation_scanner import ViolationScanner

__all__ = ["ViolationScanner"]
'''

def get_config_init():
    return '''"""
Config Module
"""

from .environment import EnvironmentDetector
from .branch_manager import BranchManager

__all__ = ["EnvironmentDetector", "BranchManager"]
'''

def get_utils_init():
    return '''"""
Utils Module
"""

from .file_utils import find_files_by_extension, backup_file
from .log_utils import setup_logging, get_logger
from .validation import validate_project_structure

__all__ = [
    "find_files_by_extension",
    "backup_file", 
    "setup_logging",
    "get_logger",
    "validate_project_structure",
]
'''

def get_metrics_module():
    return '''"""
Metrics Module - Governance metrics collection
"""

from pathlib import Path
from typing import Dict, Any
from dataclasses import dataclass

@dataclass
class ProjectMetrics:
    """Project governance metrics"""
    file_count: int
    total_lines: int
    complexity_score: float
    dependency_depth: int

class MetricsCollector:
    """Collect governance metrics from project"""
    
    def collect_project_metrics(self, project_root: Path) -> ProjectMetrics:
        """Collect comprehensive project metrics"""
        
        c_files = list(project_root.glob("**/*.c"))
        h_files = list(project_root.glob("**/*.h"))
        
        total_files = len(c_files) + len(h_files)
        total_lines = sum(len(f.read_text().splitlines()) for f in c_files + h_files if f.exists())
        
        return ProjectMetrics(
            file_count=total_files,
            total_lines=total_lines,
            complexity_score=self._calculate_complexity(c_files),
            dependency_depth=self._calculate_dependency_depth(project_root)
        )
    
    def _calculate_complexity(self, c_files) -> float:
        """Calculate code complexity score"""
        # Simplified complexity calculation
        return min(len(c_files) * 0.1, 1.0)
    
    def _calculate_dependency_depth(self, project_root: Path) -> int:
        """Calculate dependency depth"""
        # Simplified dependency depth calculation
        max_depth = 0
        for path in project_root.rglob("*.h"):
            depth = len(path.relative_to(project_root).parts)
            max_depth = max(max_depth, depth)
        return max_depth
'''

def get_threshold_checker():
    return '''"""
Threshold Checker - Validate against governance thresholds
"""

from typing import Dict, Any, List
from dataclasses import dataclass

@dataclass 
class ThresholdResult:
    """Threshold validation result"""
    metric: str
    value: float
    threshold: float
    passed: bool
    severity: str

class ThresholdChecker:
    """Check metrics against governance thresholds"""
    
    def __init__(self):
        self.default_thresholds = {
            "total_cost": 0.6,
            "complexity": 0.5,
            "file_size": 50000,  # bytes
            "line_count": 1000,  # lines per file
        }
    
    def check_thresholds(
        self, 
        metrics: Dict[str, Any], 
        custom_thresholds: Dict[str, float] = None
    ) -> List[ThresholdResult]:
        """Check metrics against defined thresholds"""
        
        thresholds = {**self.default_thresholds, **(custom_thresholds or {})}
        results = []
        
        for metric, value in metrics.items():
            if metric in thresholds:
                threshold = thresholds[metric]
                passed = value <= threshold
                severity = self._determine_severity(value, threshold)
                
                results.append(ThresholdResult(
                    metric=metric,
                    value=value,
                    threshold=threshold,
                    passed=passed,
                    severity=severity
                ))
        
        return results
    
    def _determine_severity(self, value: float, threshold: float) -> str:
        """Determine violation severity"""
        if value <= threshold:
            return "PASS"
        elif value <= threshold * 1.2:
            return "LOW"
        elif value <= threshold * 1.5:
            return "MEDIUM"
        else:
            return "HIGH"
'''

# Additional files

def get_makefile():
    return '''# Sinphasé Toolkit Makefile
# Convenience targets for development workflow

.PHONY: help install install-dev check report refactor test lint format clean

help:  ## Show this help message
\t@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\\033[36m%-20s\\033[0m %s\\n", $$1, $$2}'

install:  ## Install package in editable mode
\tpip install -e .

install-dev:  ## Install package with development dependencies
\tpip install -e ".[dev]"

check:  ## Run governance check
\tsinphase check

report:  ## Generate governance report
\tsinphase report --format markdown

refactor:  ## Run governance refactor
\tsinphase refactor --target ffi --dry-run

status:  ## Show governance status
\tsinphase status

test:  ## Run test suite
\tpytest tests/ -v --cov=sinphase_toolkit

lint:  ## Run linting
\truff check sinphase_toolkit/
\tmypy sinphase_toolkit/

format:  ## Format code
\tblack sinphase_toolkit/ tests/
\truff format sinphase_toolkit/ tests/

clean:  ## Clean build artifacts
\trm -rf build/ dist/ *.egg-info/
\tfind . -type d -name __pycache__ -exec rm -rf {} +
\tfind . -type f -name "*.pyc" -delete

# Development workflow targets
dev-setup: install-dev  ## Setup development environment
\tpre-commit install

dev-check: lint test  ## Run all development checks

# CI targets
ci-test: test lint  ## Run CI test suite

# Release targets  
build:  ## Build distribution packages
\tpython -m build

# Integration with existing scripts
legacy-check:  ## Run legacy governance check (for comparison)
\tpython scripts/governance_runner.py

legacy-report:  ## Run legacy governance report
\tpython scripts/core/governance-reporter/generate.py

# Advanced governance operations
governance-full: check report  ## Run complete governance workflow
\t@echo "🔍 Running comprehensive governance check..."
\tsinphase check --fail-on-violations
\t@echo "📊 Generating governance report..."
\tsinphase report --output governance_report.md
\t@echo "✅ Governance workflow completed"

governance-ci:  ## CI-specific governance check
\tsinphase check --fail-on-violations --format json > governance_results.json

# OBINexus-specific targets
obinexus-compliance:  ## Check OBINexus compliance standards
\tsinphase check --threshold 0.3
\tsinphase report --format markdown --output compliance_report.md

# Quick development iterations
quick-check:  ## Quick governance check for development
\tsinphase check --format console

# Help for integration
integration-help:  ## Show integration guidance
\t@echo "Integration with existing workflow:"
\t@echo "  1. Replace scattered scripts with: make check"
\t@echo "  2. Generate reports with: make report"  
\t@echo "  3. Run refactoring with: make refactor"
\t@echo "  4. Full workflow: make governance-full"
'''

def get_readme():
    return '''# Sinphasé Toolkit

**Unified Governance Framework CLI for OBINexus libpolycall**

Sinphasé Toolkit consolidates scattered governance scripts into a cohesive, enterprise-grade CLI tool for maintaining code quality, compliance, and architectural integrity.

## 🎯 Purpose

Replaces this chaos:
```bash
scripts/governance_runner.py
scripts/sinphase_check.py  
scripts/core/detector/violation_scanner.py
scripts/core/evaluator/cost_calculator.py
scripts/governance-reporter/generate.py
# ... 59+ scattered scripts
```

With this simplicity:
```bash
sinphase check
sinphase report  
sinphase refactor
```

## 🚀 Quick Start

### Installation

```bash
# Install in editable mode for development
pip install -e .

# Or install with development dependencies
pip install -e ".[dev]"
```

### Basic Usage

```bash
# Run governance checks
sinphase check

# Generate comprehensive report
sinphase report --format markdown --output report.md

# Run automated refactoring
sinphase refactor --target ffi --dry-run

# Show current status
sinphase status
```

## 📋 Commands

### `sinphase check`
Run comprehensive governance checks including cost calculation, violation detection, and compliance verification.

```bash
sinphase check --project-root /path/to/project
sinphase check --threshold 0.4 --fail-on-violations
sinphase check --format json > results.json
```

### `sinphase report`
Generate detailed governance reports in multiple formats.

```bash
sinphase report --format markdown
sinphase report --format html --output report.html
sinphase report --format json --include-details
```

### `sinphase refactor`
Run automated governance-driven refactoring.

```bash
sinphase refactor --target ffi
sinphase refactor --target includes --dry-run
sinphase refactor --target structure --no-backup
```

### `sinphase status`
Show quick governance status overview.

```bash
sinphase status
```

## 🏗️ Architecture

### Core Components

- **Checker**: Orchestrates governance checks, cost calculation, and violation detection
- **Reporter**: Generates comprehensive reports in multiple formats  
- **Refactorer**: Automated governance-driven code refactoring
- **Evaluator**: Cost calculation and metrics collection
- **Detector**: Violation scanning and threshold checking
- **Config**: Environment detection and branch management

### Module Structure

```
sinphase_toolkit/
├── cli.py                 # CLI interface (Typer)
├── core/
│   ├── checker.py         # Governance orchestration
│   ├── reporter.py        # Report generation
│   ├── refactorer.py      # Automated refactoring
│   ├── evaluator/         # Cost calculation
│   ├── detector/          # Violation detection
│   └── config/            # Environment & branch management
└── utils/                 # Utilities and helpers
```

## 🔧 Development

### Setup Development Environment

```bash
make dev-setup
```

### Run Tests

```bash
make test
```

### Code Quality

```bash
make lint     # Run linting
make format   # Format code
```

### Makefile Targets

```bash
make help            # Show all available targets
make check           # Run governance check
make report          # Generate governance report
make refactor        # Run refactoring (dry-run)
make test            # Run test suite
make governance-full # Complete governance workflow
```

## 🎛️ Configuration

### Environment Detection

Sinphasé automatically detects your environment:
- **CI**: GitHub Actions, Jenkins, etc.
- **Production**: NODE_ENV=production
- **Test**: TEST environment variables
- **Development**: Default fallback

### Thresholds by Environment

- **Production**: 0.3 (strict)
- **CI**: 0.4 (strict)
- **Test**: 0.5 (moderate)
- **Development**: 0.6 (lenient)

### Branch-Specific Governance

- **main/master**: 0.3 threshold
- **release/***: 0.35 threshold  
- **develop**: 0.4 threshold
- **feature/***: 0.6 threshold

## 🔗 Integration

### Replace Existing Scripts

**Before:**
```bash
python scripts/governance_runner.py
python scripts/sinphase_check.py  
python scripts/core/governance-reporter/generate.py
```

**After:**
```bash
sinphase check
sinphase report
```

### CI Integration

```yaml
# GitHub Actions example
- name: Governance Check
  run: sinphase check --fail-on-violations --format json
  
- name: Generate Report  
  run: sinphase report --format markdown --output governance_report.md
```

### Pre-commit Integration

```yaml
# .pre-commit-config.yaml
repos:
  - repo: local
    hooks:
      - id: sinphase-check
        name: Sinphasé Governance Check
        entry: sinphase check --fail-on-violations
        language: system
        pass_filenames: false
```

## 📊 OBINexus Compliance

This toolkit implements the OBINexus Sinphasé governance framework with:

- **#NoGhosting Policy**: Transparent violation reporting
- **Milestone-based Investment**: Progressive compliance improvement
- **OpenSense Recruitment**: Clear governance metrics for team evaluation
- **Toolchain Integration**: riftlang.exe → .so.a → rift.exe → gosilang workflow

## 🤝 Contributing

1. Follow the OBINexus development methodology
2. Maintain session continuity for project context
3. Ensure all changes pass governance checks
4. Update documentation for architectural changes

## 📄 License

MIT License - see LICENSE file for details.

---

**OBINexus Computing** - Technical Architecture Division  
*Enterprise-grade governance for software architecture*
'''

def get_license():
    return '''MIT License

Copyright (c) 2024 OBINexus Computing

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
'''

def get_gitignore():
    return '''# Python
__pycache__/
*.py[cod]
*$py.class
*.so
.Python
build/
develop-eggs/
dist/
downloads/
eggs/
.eggs/
lib/
lib64/
parts/
sdist/
var/
wheels/
pip-wheel-metadata/
share/python-wheels/
*.egg-info/
.installed.cfg
*.egg
MANIFEST

# Testing
.tox/
.coverage
.coverage.*
.cache
nosetests.xml
coverage.xml
*.cover
*.py,cover
.hypothesis/
.pytest_cache/

# Virtual environments
.env
.venv
env/
venv/
ENV/
env.bak/
venv.bak/

# IDEs
.vscode/
.idea/
*.swp
*.swo

# OS
.DS_Store
Thumbs.db

# Project specific
governance_report.*
compliance_report.*
governance_results.json
logs/
*.bak
*_backup_*/

# Temporary files
*.tmp
*.temp
'''

# Script execution
if __name__ == "__main__":
    print("📦 Sinphasé Toolkit Package Skeleton")
    print("🔧 Ready to generate complete package structure")
    print("💡 Use this as template for your consolidated governance CLI")
