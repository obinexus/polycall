#!/usr/bin/env python3
"""
Sinphasé Environment-Aware Governance Check
Unified entry point for all governance operations
"""

import sys
import argparse
from pathlib import Path

# Import environment-aware runner
sys.path.insert(0, str(Path(__file__).parent))
from environments.governance_runner import SinphaseGovernanceRunner, Environment

def main():
    parser = argparse.ArgumentParser(description="Sinphasé Governance Check")
    parser.add_argument("--environment", choices=[e.value for e in Environment])
    parser.add_argument("--fail-on-violations", action="store_true")
    parser.add_argument("--project-root", default=".")
    
    args = parser.parse_args()
    
    environment = Environment(args.environment) if args.environment else None
    runner = SinphaseGovernanceRunner(Path(args.project_root), environment)
    
    success, results = runner.run_governance_check(args.fail_on_violations)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
