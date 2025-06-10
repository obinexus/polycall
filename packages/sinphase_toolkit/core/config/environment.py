"""Environment Configuration"""
import os
from enum import Enum

class Environment(Enum):
    DEVELOPMENT = "development"
    TEST = "test"
    CI = "ci"
    PRODUCTION = "production"

class EnvironmentDetector:
    def detect_environment(self) -> Environment:
        """Detect current environment"""
        if os.environ.get('CI'):
            return Environment.CI
        return Environment.DEVELOPMENT
