#!/usr/bin/env python3
"""
Sinphasé Environment-Specific Entry Points
Decoupled governance runners for different development environments

Author: OBINexus Computing - Sinphasé Governance Framework
Version: 2.0.0 (Decoupled Architecture)
"""

import os
import sys
import json
import argparse
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from enum import Enum

# Import our standalone modules
sys.path.insert(0, str(Path(__file__).parent.parent / "core"))
from cost_evaluator.evaluate import SinphaseCostCalculator
from violation_detector.scan import SinphaseViolationDetector
from governance_reporter.generate import SinphaseReportGenerator

class Environment(Enum):
    """Development environment types."""
    DEVELOPMENT = "dev"
    CI_CD = "ci"
    TEST = "test"
    PRODUCTION = "prod"

@dataclass
class EnvironmentConfig:
    """Environment-specific configuration."""
    name: str
    governance_threshold: float
    fail_on_violations: bool
    isolation_enabled: bool
    reporting_level: str
    fast_feedback: bool
    
    @classmethod
    def from_dict(cls, data: Dict) -> 'EnvironmentConfig':
        return cls(**data)

class EnvironmentDetector:
    """Detect current execution environment."""
    
    @staticmethod
    def detect_environment() -> Environment:
        """Auto-detect environment based on context."""
        # Check CI environment variables
        ci_indicators = [
            'CI', 'CONTINUOUS_INTEGRATION', 'GITHUB_ACTIONS',
            'JENKINS_URL', 'GITLAB_CI', 'TRAVIS'
        ]
        
        if any(os.getenv(var) for var in ci_indicators):
            return Environment.CI_CD
        
        # Check test environment indicators
        if any(indicator in sys.argv[0].lower() for indicator in ['test', 'pytest', 'unittest']):
            return Environment.TEST
        
        # Check production indicators
        if os.getenv('PRODUCTION') or os.getenv('DEPLOY_ENV') == 'production':
            return Environment.PRODUCTION
        
        # Default to development
        return Environment.DEVELOPMENT

class BranchAwareConfig:
    """Git branch-aware configuration management."""
    
    def __init__(self, config_dir: Path):
        self.config_dir = config_dir
        self.current_branch = self._get_current_branch()
    
    def _get_current_branch(self) -> str:
        """Get current git branch name."""
        try:
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True, text=True, check=True
            )
            return result.stdout.strip()
        except (subprocess.CalledProcessError, FileNotFoundError):
            return "unknown"
    
    def get_branch_threshold(self, environment: Environment) -> float:
        """Get governance threshold for current branch and environment."""
        # Branch-specific threshold mappings
        branch_thresholds = {
            "main": 0.6,       # Strict governance for main
            "master": 0.6,     # Strict governance for master
            "develop": 0.8,    # Moderate governance for develop
            "staging": 0.7,    # Moderate-strict for staging
            "release": 0.6,    # Strict for release branches
            "hotfix": 0.6,     # Strict for hotfixes
        }
        
        # Feature branch patterns (more permissive)
        if any(pattern in self.current_branch for pattern in ['feature/', 'feat/', 'bug/', 'fix/']):
            base_threshold = 1.0
        else:
            base_threshold = branch_thresholds.get(self.current_branch, 0.8)
        
        # Environment adjustments
        environment_multipliers = {
            Environment.DEVELOPMENT: 1.2,  # More permissive in dev
            Environment.TEST: 1.1,         # Slightly permissive in test
            Environment.CI_CD: 1.0,        # Standard in CI
            Environment.PRODUCTION: 0.9    # Stricter in production
        }
        
        return base_threshold * environment_multipliers.get(environment, 1.0)

class SinphaseGovernanceRunner:
    """Main governance runner with environment awareness."""
    
    def __init__(self, project_root: Path, environment: Optional[Environment] = None):
        self.project_root = project_root
        self.environment = environment or EnvironmentDetector.detect_environment()
        self.config_dir = project_root / "config"
        self.branch_config = BranchAwareConfig(self.config_dir)
        
        # Initialize standalone modules
        self.cost_calculator = SinphaseCostCalculator()
        self.violation_detector = SinphaseViolationDetector()
        self.report_generator = SinphaseReportGenerator()
        
    def run_governance_check(self, 
                           fail_on_violations: Optional[bool] = None,
                           output_format: str = "markdown") -> Tuple[bool, Dict]:
        """
        Run complete governance check with environment awareness.
        
        Args:
            fail_on_violations: Override environment default
            output_format: Report output format
            
        Returns:
            Tuple of (success, results_dict)
        """
        print(f"🔍 Running Sinphasé governance check (Environment: {self.environment.value})")
        
        # Get environment-specific configuration
        threshold = self.branch_config.get_branch_threshold(self.environment)
        print(f"📊 Governance threshold: {threshold} (Branch: {self.branch_config.current_branch})")
        
        # Calculate costs for all components
        cost_results = self._calculate_project_costs()
        
        # Detect violations
        violations, summary = self.violation_detector.detect_violations(cost_results, threshold)
        
        # Generate report
        report = self.violation_detector.generate_violation_report(violations, summary)
        
        # Environment-specific reporting
        self._output_environment_report(report, output_format)
        
        # Determine success based on environment
        should_fail = self._should_fail_on_violations(fail_on_violations, summary)
        success = not (should_fail and summary.total_violations > 0)
        
        return success, {
            "environment": self.environment.value,
            "branch": self.branch_config.current_branch,
            "threshold": threshold,
            "summary": summary.to_dict(),
            "violations": len(violations),
            "success": success
        }
    
    def _calculate_project_costs(self) -> Dict[str, float]:
        """Calculate costs for all project components."""
        cost_results = {}
        
        # Find all C source directories
        src_dirs = [
            self.project_root / "src",
            self.project_root / "libpolycall" / "src",
        ]
        
        for src_dir in src_dirs:
            if not src_dir.exists():
                continue
                
            # Find component directories
            for component_dir in src_dir.rglob("*"):
                if (component_dir.is_dir() and 
                    any(component_dir.glob("*.c")) and
                    "test" not in component_dir.name.lower()):
                    
                    try:
                        component_cost = self.cost_calculator.calculate_component_cost(component_dir)
                        cost_results[str(component_dir.relative_to(self.project_root))] = component_cost.cost
                    except Exception as e:
                        print(f"⚠️ Warning: Could not calculate cost for {component_dir}: {e}")
        
        return cost_results
    
    def _should_fail_on_violations(self, 
                                  override: Optional[bool], 
                                  summary) -> bool:
        """Determine if violations should cause failure."""
        if override is not None:
            return override
        
        # Environment-specific failure policies
        environment_policies = {
            Environment.DEVELOPMENT: False,     # Don't fail in dev
            Environment.TEST: False,           # Don't fail in test
            Environment.CI_CD: True,           # Fail in CI
            Environment.PRODUCTION: True       # Always fail in prod
        }
        
        base_policy = environment_policies.get(self.environment, True)
        
        # Always fail on emergency violations regardless of environment
        if summary.emergency_action_required:
            return True
        
        return base_policy
    
    def _output_environment_report(self, report: Dict, format_type: str):
        """Output report in environment-appropriate format."""
        if self.environment == Environment.DEVELOPMENT:
            # Development: Concise, actionable output
            print(f"\n📊 Governance Results:")
            print(f"  🟢 Autonomous: {report['summary']['autonomous_count']} components")
            print(f"  🟡 Warning: {report['summary']['warning_violations']} violations")
            print(f"  🔴 Governance: {report['summary']['total_violations']} violations")
            
            if report['summary']['total_violations'] > 0:
                print(f"\n🔧 Quick fixes needed for {report['summary']['total_violations']} components")
                
        elif self.environment == Environment.CI_CD:
            # CI/CD: Structured output for pipeline processing
            print(f"\n::group::Sinphasé Governance Results")
            print(f"Violations: {report['summary']['total_violations']}")
            print(f"Critical: {report['summary']['critical_violations']}")
            print(f"Emergency: {report['summary']['emergency_action_required']}")
            print(f"::endgroup::")
            
            # Output violations for CI tools
            for violation in report['all_violations']:
                severity = violation['severity'].upper()
                print(f"::{severity}::{violation['file_path']} - Cost {violation['cost']} exceeds threshold")
                
        else:
            # Test/Production: Full detailed report
            if format_type == "json":
                print(json.dumps(report, indent=2))
            else:
                self.report_generator.generate_markdown_report(report)

def main():
    """Main entry point for environment-specific governance."""
    parser = argparse.ArgumentParser(description="Sinphasé Environment-Aware Governance")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--environment", choices=[e.value for e in Environment], 
                       help="Override environment detection")
    parser.add_argument("--fail-on-violations", action="store_true", 
                       help="Fail on any violations")
    parser.add_argument("--no-fail", action="store_true", 
                       help="Never fail on violations")
    parser.add_argument("--format", choices=["markdown", "json"], default="markdown",
                       help="Output format")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    # Determine environment
    environment = None
    if args.environment:
        environment = Environment(args.environment)
    
    # Determine failure policy
    fail_on_violations = None
    if args.fail_on_violations:
        fail_on_violations = True
    elif args.no_fail:
        fail_on_violations = False
    
    # Run governance check
    runner = SinphaseGovernanceRunner(Path(args.project_root), environment)
    success, results = runner.run_governance_check(fail_on_violations, args.format)
    
    if args.verbose:
        print(f"\n📋 Execution Summary:")
        print(f"  Environment: {results['environment']}")
        print(f"  Branch: {results['branch']}")
        print(f"  Threshold: {results['threshold']}")
        print(f"  Success: {results['success']}")
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()