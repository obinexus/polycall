#!/usr/bin/env python3
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
        print("\n🔍 Violations:")
        for violation in violations[:5]:  # Show first 5
            print(f"  • {violation['file_path']}: {violation['cost']:.3f}")
    else:
        print("\n✅ No violations detected")

if __name__ == "__main__":
    main()
