#!/usr/bin/env python3
"""
Sinphasé Configuration Management
Environment and branch-aware configuration

Author: OBINexus Computing - Sinphasé Framework
"""

import os
import subprocess
from pathlib import Path
from typing import Dict, Optional
from enum import Enum

class Environment(Enum):
    """Development environment types."""
    DEVELOPMENT = "dev"
    CI_CD = "ci"
    TEST = "test"
    PRODUCTION = "prod"

class EnvironmentDetector:
    """Detect current execution environment."""
    
    @staticmethod
    def detect_environment() -> Environment:
        """Auto-detect environment based on context."""
        # Check CI environment variables
        ci_indicators = ['CI', 'GITHUB_ACTIONS', 'JENKINS_URL', 'GITLAB_CI']
        
        if any(os.getenv(var) for var in ci_indicators):
            return Environment.CI_CD
        
        # Check production indicators
        if os.getenv('PRODUCTION') or os.getenv('DEPLOY_ENV') == 'production':
            return Environment.PRODUCTION
        
        # Check test environment
        if 'test' in os.getcwd().lower():
            return Environment.TEST
        
        return Environment.DEVELOPMENT

class BranchConfig:
    """Git branch-aware configuration."""
    
    def __init__(self):
        self.current_branch = self._get_current_branch()
    
    def _get_current_branch(self) -> str:
        """Get current git branch name."""
        try:
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True, text=True, check=True
            )
            return result.stdout.strip()
        except:
            return "unknown"
    
    def get_threshold_for_branch(self, base_threshold: float = 0.6) -> float:
        """Get threshold adjusted for current branch."""
        branch_multipliers = {
            "main": 1.0,
            "master": 1.0,
            "develop": 1.3,
            "staging": 1.1,
            "release": 1.0,
            "hotfix": 1.0
        }
        
        # Feature branches get higher threshold
        if any(pattern in self.current_branch for pattern in ['feature/', 'feat/', 'bug/']):
            return base_threshold * 1.7
        
        multiplier = branch_multipliers.get(self.current_branch, 1.3)
        return base_threshold * multiplier

if __name__ == "__main__":
    detector = EnvironmentDetector()
    branch_config = BranchConfig()
    
    env = detector.detect_environment()
    threshold = branch_config.get_threshold_for_branch()
    
    print(f"Environment: {env.value}")
    print(f"Branch: {branch_config.current_branch}")
    print(f"Threshold: {threshold}")
