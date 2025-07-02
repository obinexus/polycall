#!/usr/bin/env python3
"""
Sinphasé Main Governance Runner
Unified entry point for environment-aware governance

Author: OBINexus Computing - Sinphasé Framework
"""

import sys
import argparse
from pathlib import Path

# Add scripts directory to Python path
scripts_dir = Path(__file__).parent
sys.path.insert(0, str(scripts_dir))

from core.cost_evaluator.evaluate import calculate_project_costs
from core.violation_detector.scan import SinphaseViolationDetector
from core.governance_reporter.generate import SinphaseReportGenerator
from core.config.environment import EnvironmentDetector, BranchConfig, Environment

class SinphaseGovernanceRunner:
    """Main governance execution engine."""
    
    def __init__(self, project_root: Path, environment: Environment = None):
        self.project_root = project_root
        self.environment = environment or EnvironmentDetector.detect_environment()
        self.branch_config = BranchConfig()
        
        self.detector = SinphaseViolationDetector()
        self.reporter = SinphaseReportGenerator()
    
    def run_governance_check(self, fail_on_violations: bool = None) -> tuple:
        """Execute complete governance check."""
        print(f"🔍 Running Sinphasé governance check (Environment: {self.environment.value})")
        
        # Get environment-specific threshold
        base_threshold = self._get_environment_threshold()
        threshold = self.branch_config.get_threshold_for_branch(base_threshold)
        
        print(f"📊 Governance threshold: {threshold} (Branch: {self.branch_config.current_branch})")
        
        # Calculate project costs
        cost_results = calculate_project_costs(self.project_root)
        
        # Detect violations
        violations, summary = self.detector.detect_violations(cost_results, threshold)
        
        # Generate and display report
        report = self.reporter.generate_report(violations, summary, "console")
        print(report)
        
        # Determine success
        should_fail = self._should_fail_on_violations(fail_on_violations, summary)
        success = not (should_fail and summary.total_violations > 0)
        
        return success, {
            "environment": self.environment.value,
            "branch": self.branch_config.current_branch,
            "threshold": threshold,
            "violations": summary.total_violations,
            "success": success
        }
    
    def _get_environment_threshold(self) -> float:
        """Get base threshold for environment."""
        thresholds = {
            Environment.DEVELOPMENT: 0.8,
            Environment.CI_CD: 0.6,
            Environment.TEST: 0.7,
            Environment.PRODUCTION: 0.6
        }
        return thresholds.get(self.environment, 0.6)
    
    def _should_fail_on_violations(self, override: bool, summary) -> bool:
        """Determine if violations should cause failure."""
        if override is not None:
            return override
        
        # Environment-specific failure policies
        policies = {
            Environment.DEVELOPMENT: False,
            Environment.CI_CD: True,
            Environment.TEST: False,
            Environment.PRODUCTION: True
        }
        
        base_policy = policies.get(self.environment, True)
        
        # Always fail on emergency violations
        if summary.emergency_action_required:
            return True
        
        return base_policy

def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Sinphasé Governance Check")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--environment", choices=[e.value for e in Environment])
    parser.add_argument("--fail-on-violations", action="store_true")
    parser.add_argument("--no-fail", action="store_true")
    
    args = parser.parse_args()
    
    environment = None
    if args.environment:
        environment = Environment(args.environment)
    
    fail_on_violations = None
    if args.fail_on_violations:
        fail_on_violations = True
    elif args.no_fail:
        fail_on_violations = False
    
    runner = SinphaseGovernanceRunner(Path(args.project_root), environment)
    success, results = runner.run_governance_check(fail_on_violations)
    
    print(f"\n📋 Result: {'SUCCESS' if success else 'FAILURE'}")
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
