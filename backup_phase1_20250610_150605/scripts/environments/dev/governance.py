#!/usr/bin/env python3
"""
DEV Environment Governance Entry Point
"""

import sys
from pathlib import Path

# Import environment-aware runner
sys.path.insert(0, str(Path(__file__).parent.parent.parent))
from environments.governance_runner import SinphaseGovernanceRunner, Environment

def main():
    runner = SinphaseGovernanceRunner(Path.cwd(), Environment.DEV)
    success, results = runner.run_governance_check()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
