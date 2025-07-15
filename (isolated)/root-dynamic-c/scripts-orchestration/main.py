#!/usr/bin/env python3
"""
Sinphasé CLI Interface
Command-line interface for enterprise governance execution
"""

import sys
import argparse
from pathlib import Path

# Import core governance modules using absolute package imports
from sinphase_governance.core.evaluator.cost_calculator import calculate_project_costs
from sinphase_governance.core.detector.violation_scanner import SinphaseViolationDetector
from sinphase_governance.core.reporter.report_generator import SinphaseReportGenerator
from sinphase_governance.core.config.environment import EnvironmentDetector, BranchManager

def execute_governance_analysis():
    """
    Execute comprehensive Sinphasé governance analysis.
    
    Technical Implementation:
    - Implements environment-aware governance policy application
    - Provides branch-specific threshold adjustment
    - Supports multiple output formats for CI/CD integration
    """
    parser = argparse.ArgumentParser(
        description="Sinphasé Governance Framework - Enterprise Architecture Compliance",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  sinphase --project-root . --format console
  sinphase --threshold 0.8 --format json --no-fail
  sinphase --project-root /path/to/project --format markdown
        """
    )
    
    parser.add_argument(
        "--project-root", 
        default=".", 
        help="Project root directory path (default: current directory)"
    )
    parser.add_argument(
        "--threshold", 
        type=float, 
        help="Override governance threshold (default: environment-aware)"
    )
    parser.add_argument(
        "--format", 
        choices=["console", "json", "markdown"], 
        default="console",
        help="Report output format (default: console)"
    )
    parser.add_argument(
        "--fail-on-violations", 
        action="store_true",
        help="Exit with error code on governance violations"
    )
    parser.add_argument(
        "--no-fail", 
        action="store_true",
        help="Never exit with error code (override environment policy)"
    )
    parser.add_argument(
        "--verbose", 
        action="store_true",
        help="Enable verbose output for debugging"
    )
    
    args = parser.parse_args()
    
    # Initialize governance components
    project_root = Path(args.project_root).resolve()
    environment_detector = EnvironmentDetector()
    branch_manager = BranchManager()
    violation_detector = SinphaseViolationDetector()
    report_generator = SinphaseReportGenerator()
    
    # Determine execution environment and governance parameters
    environment = environment_detector.detect_environment()
    
    if args.threshold:
        threshold = args.threshold
    else:
        base_threshold = branch_manager.get_environment_base_threshold(environment)
        threshold = branch_manager.calculate_governance_threshold(base_threshold)
    
    if args.verbose:
        print(f"🔧 Governance Execution Configuration:")
        print(f"   Project Root: {project_root}")
        print(f"   Environment: {environment.value}")
        print(f"   Branch: {branch_manager.current_branch}")
        print(f"   Threshold: {threshold}")
        print()
    
    print(f"🔍 Executing Sinphasé Governance Analysis")
    print(f"Environment: {environment.value} | Branch: {branch_manager.current_branch} | Threshold: {threshold}")
    print()
    
    try:
        # Execute cost analysis
        cost_results = calculate_project_costs(project_root)
        
        if not cost_results:
            print("⚠️  No components found for analysis")
            sys.exit(0)
        
        # Execute violation detection
        violations, summary = violation_detector.detect_violations(cost_results, threshold)
        
        # Generate and display report
        report = report_generator.generate_report(violations, summary, args.format)
        print(report)
        
        # Determine exit behavior based on environment and arguments
        should_fail = _determine_failure_policy(args, environment, summary)
        
        if should_fail and summary.total_violations > 0:
            if args.verbose:
                print("\n❌ Exiting with error code due to governance violations")
            sys.exit(1)
        else:
            if args.verbose:
                print("\n✅ Governance analysis completed successfully")
            sys.exit(0)
    
    except Exception as e:
        print(f"❌ Governance analysis failed: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(2)

def _determine_failure_policy(args, environment, summary) -> bool:
    """Determine whether governance violations should cause process failure."""
    # Explicit command-line overrides
    if args.no_fail:
        return False
    if args.fail_on_violations:
        return True
    
    # Environment-specific failure policies
    environment_policies = {
        EnvironmentDetector.Environment.DEVELOPMENT: False,     # No failure in development
        EnvironmentDetector.Environment.CI_CD: True,           # Fail in CI/CD
        EnvironmentDetector.Environment.TEST: False,           # No failure in test
        EnvironmentDetector.Environment.PRODUCTION: True       # Always fail in production
    }
    
    base_policy = environment_policies.get(environment, True)
    
    # Always fail on emergency violations regardless of environment
    if summary.emergency_action_required:
        return True
    
    return base_policy

def main():
    """Main entry point for CLI execution."""
    execute_governance_analysis()

if __name__ == "__main__":
    main()
