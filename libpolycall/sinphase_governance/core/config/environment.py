#!/usr/bin/env python3
"""
Sinphasé Configuration Management
Environment detection and branch-aware configuration for enterprise deployment
"""

import os
import subprocess
from pathlib import Path
from typing import Dict, Optional
from enum import Enum

class Environment(Enum):
    """Development environment classification for governance policy application."""
    DEVELOPMENT = "development"
    CI_CD = "ci_cd"
    TEST = "test"
    PRODUCTION = "production"

class EnvironmentDetector:
    """
    Production-grade environment detection system.
    
    Technical Implementation:
    - Detects CI/CD environments through standard environment variables
    - Identifies production deployments through deployment indicators
    - Provides fallback detection mechanisms for complex environments
    """
    
    @staticmethod
    def detect_environment() -> Environment:
        """
        Detect current execution environment using enterprise detection criteria.
        
        Returns:
            Environment: Detected environment classification
        """
        # CI/CD environment detection through standard variables
        ci_indicators = [
            'CI', 'CONTINUOUS_INTEGRATION', 'GITHUB_ACTIONS',
            'JENKINS_URL', 'GITLAB_CI', 'TRAVIS', 'AZURE_DEVOPS',
            'BUILDKITE', 'CIRCLE_CI', 'TEAMCITY_VERSION'
        ]
        
        if any(os.getenv(var) for var in ci_indicators):
            return Environment.CI_CD
        
        # Production environment detection
        production_indicators = [
            'PRODUCTION', 'PROD', 'DEPLOY_ENV=production',
            'NODE_ENV=production', 'ENVIRONMENT=prod'
        ]
        
        if any(os.getenv(var) or os.getenv('DEPLOY_ENV') == 'production' 
               for var in production_indicators):
            return Environment.PRODUCTION
        
        # Test environment detection
        test_indicators = ['TEST', 'TESTING', 'QA']
        if (any(os.getenv(var) for var in test_indicators) or
            any(indicator in os.getcwd().lower() for indicator in ['test', 'testing', 'qa'])):
            return Environment.TEST
        
        # Default to development environment
        return Environment.DEVELOPMENT

class BranchManager:
    """
    Git branch-aware configuration management for governance policy adaptation.
    
    Technical Implementation:
    - Retrieves current branch through git subprocess execution
    - Applies branch-specific governance threshold multipliers
    - Supports feature branch pattern recognition for development flexibility
    """
    
    def __init__(self):
        self.current_branch = self._retrieve_current_branch()
        self.branch_configuration = self._initialize_branch_configuration()
    
    def _retrieve_current_branch(self) -> str:
        """Retrieve current git branch through subprocess execution."""
        try:
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                capture_output=True, 
                text=True, 
                check=True, 
                timeout=10
            )
            return result.stdout.strip()
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError):
            return "unknown"
    
    def _initialize_branch_configuration(self) -> Dict[str, float]:
        """Initialize branch-specific governance threshold multipliers."""
        return {
            "main": 1.0,        # Strict governance for production branch
            "master": 1.0,      # Strict governance for production branch
            "develop": 1.3,     # Moderate governance for development integration
            "staging": 1.1,     # Moderate-strict governance for staging
            "release": 1.0,     # Strict governance for release preparation
            "hotfix": 1.0,      # Strict governance for emergency fixes
            "bugfix": 1.2,      # Moderate governance for bug resolution
        }
    
    def calculate_threshold_multiplier(self) -> float:
        """
        Calculate governance threshold multiplier for current branch context.
        
        Returns:
            float: Threshold multiplier for current branch
        """
        # Feature branch pattern recognition for development flexibility
        feature_patterns = [
            'feature/', 'feat/', 'bug/', 'fix/', 'chore/', 
            'docs/', 'style/', 'refactor/', 'perf/', 'test/'
        ]
        
        if any(pattern in self.current_branch.lower() for pattern in feature_patterns):
            return 1.7  # Permissive governance for feature development
        
        # Apply configured multiplier or use moderate default
        return self.branch_configuration.get(self.current_branch, 1.3)
    
    def calculate_governance_threshold(self, base_threshold: float = 0.6) -> float:
        """
        Calculate final governance threshold for current execution context.
        
        Args:
            base_threshold: Base governance threshold value
            
        Returns:
            float: Adjusted governance threshold for current context
        """
        multiplier = self.calculate_threshold_multiplier()
        adjusted_threshold = base_threshold * multiplier
        return round(adjusted_threshold, 3)
    
    def get_environment_base_threshold(self, environment: Environment) -> float:
        """
        Get environment-specific base governance threshold.
        
        Args:
            environment: Current execution environment
            
        Returns:
            float: Environment-appropriate base threshold
        """
        environment_thresholds = {
            Environment.DEVELOPMENT: 0.8,    # Relaxed for development
            Environment.CI_CD: 0.6,          # Standard for CI/CD
            Environment.TEST: 0.7,           # Moderate for testing
            Environment.PRODUCTION: 0.5      # Strict for production
        }
        
        return environment_thresholds.get(environment, 0.6)
