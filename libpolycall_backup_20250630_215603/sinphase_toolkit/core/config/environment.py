"""Environment Configuration"""
import os

class EnvironmentDetector:
    def detect_current_environment(self) -> str:
        """Detect current environment"""
        if os.environ.get('CI'):
            return "ci"
        return "dev"
