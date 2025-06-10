# core/evaluator/__init__.py
"""Cost evaluation and metrics collection subsystem"""

from .cost_calculator import CostCalculator

__all__ = ["CostCalculator"]


# core/evaluator/cost_calculator.py
"""
Cost Calculator - Quantitative governance metric computation
Technical implementation of OBINexus cost function methodology
"""

import logging
from pathlib import Path
from typing import Dict, List, Any

logger = logging.getLogger(__name__)

class CostCalculator:
    """
    Implements comprehensive cost analysis for governance compliance
    
    Technical Architecture:
    - Multi-factor cost computation (lines, dependencies, complexity)
    - Environment-aware cost weighting
    - Milestone-based investment tracking
    """
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.cost_weights = {
            "lines_factor": 0.001,
            "dependency_factor": 0.05,
            "include_factor": 0.02,
            "function_factor": 0.01
        }
        logger.info(f"Initialized CostCalculator for {project_root}")
    
    def calculate_comprehensive_costs(self) -> Dict[str, Any]:
        """Calculate multi-dimensional cost analysis"""
        logger.debug("Calculating comprehensive project costs")
        
        source_files = self._discover_source_files()
        
        costs = {
            "file_analysis": {},
            "total_cost": 0.0,
            "cost_breakdown": {},
            "file_count": len(source_files)
        }
        
        total_cost = 0.0
        
        for file_path in source_files:
            file_cost = self._calculate_file_cost(file_path)
            costs["file_analysis"][str(file_path)] = file_cost
            total_cost += file_cost["total_cost"]
        
        costs["total_cost"] = total_cost
        costs["cost_breakdown"] = self._generate_cost_breakdown(costs["file_analysis"])
        
        logger.info(f"Cost calculation completed - Total: {total_cost:.3f}")
        return costs
    
    def calculate_total_cost(self) -> float:
        """Quick total cost calculation for status checks"""
        analysis = self.calculate_comprehensive_costs()
        return analysis["total_cost"]
    
    def _discover_source_files(self) -> List[Path]:
        """Discover analyzable source files in project"""
        patterns = ["*.c", "*.h", "*.cpp", "*.hpp", "*.py"]
        files = []
        
        for pattern in patterns:
            files.extend(self.project_root.rglob(pattern))
        
        # Filter out build artifacts and vendor code
        filtered_files = [
            f for f in files 
            if not any(part in str(f) for part in ["build", "vendor", ".git", "__pycache__"])
        ]
        
        logger.debug(f"Discovered {len(filtered_files)} source files")
        return filtered_files
    
    def _calculate_file_cost(self, file_path: Path) -> Dict[str, float]:
        """Calculate multi-factor cost for individual file"""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = content.splitlines()
            
            # Line count factor
            line_cost = len(lines) * self.cost_weights["lines_factor"]
            
            # Include/import dependency factor
            include_count = sum(1 for line in lines if line.strip().startswith(('#include', 'import ', 'from ')))
            include_cost = include_count * self.cost_weights["include_factor"]
            
            # Function definition factor (basic heuristic)
            function_count = sum(1 for line in lines if 'def ' in line or ' function ' in line or 'void ' in line)
            function_cost = function_count * self.cost_weights["function_factor"]
            
            total_cost = line_cost + include_cost + function_cost
            
            return {
                "line_cost": line_cost,
                "include_cost": include_cost,
                "function_cost": function_cost,
                "total_cost": total_cost,
                "metrics": {
                    "lines": len(lines),
                    "includes": include_count,
                    "functions": function_count
                }
            }
            
        except Exception as e:
            logger.warning(f"Error calculating cost for {file_path}: {e}")
            return {"total_cost": 0.0, "error": str(e)}
    
    def _generate_cost_breakdown(self, file_analysis: Dict) -> Dict[str, float]:
        """Generate aggregated cost breakdown by category"""
        breakdown = {
            "total_line_cost": 0.0,
            "total_include_cost": 0.0,
            "total_function_cost": 0.0
        }
        
        for file_data in file_analysis.values():
            if "error" not in file_data:
                breakdown["total_line_cost"] += file_data.get("line_cost", 0.0)
                breakdown["total_include_cost"] += file_data.get("include_cost", 0.0)
                breakdown["total_function_cost"] += file_data.get("function_cost", 0.0)
        
        return breakdown


# core/detector/__init__.py
"""Violation detection and threshold analysis subsystem"""

from .violation_scanner import ViolationScanner

__all__ = ["ViolationScanner"]


# core/detector/violation_scanner.py
"""
Violation Scanner - Systematic governance compliance verification
Implements threshold-based violation detection with severity classification
"""

import logging
from pathlib import Path
from typing import List, Dict, Any

logger = logging.getLogger(__name__)

class ViolationScanner:
    """
    Systematic violation detection engine
    
    Technical Implementation:
    - Pattern-based violation identification
    - Severity classification (warning, critical, emergency)
    - Threshold-aware compliance assessment
    """
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.violation_patterns = self._load_violation_patterns()
        logger.info(f"Initialized ViolationScanner for {project_root}")
    
    def scan_comprehensive_violations(self) -> List[Dict[str, Any]]:
        """Execute comprehensive violation scanning across project"""
        logger.debug("Scanning for governance violations")
        
        violations = []
        source_files = self._discover_source_files()
        
        for file_path in source_files:
            file_violations = self._scan_file_violations(file_path)
            violations.extend(file_violations)
        
        logger.info(f"Violation scan completed - {len(violations)} violations detected")
        return violations
    
    def scan_violations(self) -> List[Dict[str, Any]]:
        """Quick violation scan for status checks"""
        return self.scan_comprehensive_violations()
    
    def _discover_source_files(self) -> List[Path]:
        """Discover scannable source files"""
        patterns = ["*.c", "*.h", "*.cpp", "*.hpp", "*.py"]
        files = []
        
        for pattern in patterns:
            files.extend(self.project_root.rglob(pattern))
        
        # Filter out irrelevant files
        filtered_files = [
            f for f in files 
            if not any(part in str(f) for part in ["build", "vendor", ".git", "__pycache__"])
        ]
        
        return filtered_files
    
    def _scan_file_violations(self, file_path: Path) -> List[Dict[str, Any]]:
        """Scan individual file for governance violations"""
        violations = []
        
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
            lines = content.splitlines()
            
            for line_num, line in enumerate(lines, 1):
                line_violations = self._check_line_violations(line, line_num, file_path)
                violations.extend(line_violations)
                
        except Exception as e:
            logger.warning(f"Error scanning {file_path}: {e}")
            violations.append({
                "type": "scan_error",
                "file": str(file_path),
                "message": f"Failed to scan file: {e}",
                "severity": "warning"
            })
        
        return violations
    
    def _check_line_violations(self, line: str, line_num: int, file_path: Path) -> List[Dict[str, Any]]:
        """Check individual line for violation patterns"""
        violations = []
        line_stripped = line.strip()
        
        # Example violation patterns
        if len(line) > 120:
            violations.append({
                "type": "line_length",
                "file": str(file_path),
                "line": line_num,
                "message": f"Line exceeds 120 characters ({len(line)} chars)",
                "severity": "warning"
            })
        
        if "TODO" in line_stripped.upper():
            violations.append({
                "type": "todo_marker",
                "file": str(file_path),
                "line": line_num,
                "message": "TODO marker detected",
                "severity": "warning"
            })
        
        if "FIXME" in line_stripped.upper():
            violations.append({
                "type": "fixme_marker",
                "file": str(file_path),
                "line": line_num,
                "message": "FIXME marker detected",
                "severity": "critical"
            })
        
        return violations
    
    def _load_violation_patterns(self) -> Dict[str, Any]:
        """Load violation detection patterns"""
        return {
            "line_length_limit": 120,
            "complexity_threshold": 10,
            "dependency_limit": 20
        }


# core/config/__init__.py
"""Environment detection and configuration management"""

from .environment import EnvironmentDetector, Environment

__all__ = ["EnvironmentDetector", "Environment"]


# core/config/environment.py
"""
Environment Detection - Context-aware governance configuration
Implements environment-specific threshold and policy management
"""

import logging
import os
from enum import Enum
from pathlib import Path

logger = logging.getLogger(__name__)

class Environment(Enum):
    """Execution environment classification"""
    DEVELOPMENT = "development"
    TEST = "test"
    CI = "ci"
    PRODUCTION = "production"

class EnvironmentDetector:
    """
    Intelligent environment detection for context-aware governance
    
    Technical Implementation:
    - CI/CD platform detection (GitHub Actions, Jenkins, etc.)
    - Git branch analysis for environment inference
    - Environment variable assessment
    """
    
    def __init__(self):
        logger.debug("Initialized EnvironmentDetector")
    
    def detect_environment(self) -> Environment:
        """Detect current execution environment"""
        # CI/CD environment detection
        if self._is_ci_environment():
            logger.info("Detected CI/CD environment")
            return Environment.CI
        
        # Production environment detection
        if self._is_production_environment():
            logger.info("Detected production environment")
            return Environment.PRODUCTION
        
        # Test environment detection
        if self._is_test_environment():
            logger.info("Detected test environment")
            return Environment.TEST
        
        # Default to development
        logger.info("Detected development environment")
        return Environment.DEVELOPMENT
    
    def _is_ci_environment(self) -> bool:
        """Check for CI/CD environment indicators"""
        ci_indicators = [
            "CI", "CONTINUOUS_INTEGRATION",
            "GITHUB_ACTIONS", "JENKINS_URL",
            "GITLAB_CI", "BUILDKITE",
            "TRAVIS", "CIRCLECI"
        ]
        
        return any(os.getenv(indicator) for indicator in ci_indicators)
    
    def _is_production_environment(self) -> bool:
        """Check for production environment indicators"""
        prod_indicators = [
            os.getenv("NODE_ENV") == "production",
            os.getenv("ENVIRONMENT") == "production",
            os.getenv("DEPLOY_ENV") == "production"
        ]
        
        return any(prod_indicators)
    
    def _is_test_environment(self) -> bool:
        """Check for test environment indicators"""
        test_indicators = [
            os.getenv("NODE_ENV") == "test",
            os.getenv("ENVIRONMENT") == "test",
            "pytest" in os.getenv("_", "").lower()
        ]
        
        return any(test_indicators)


# utils/__init__.py
"""Utility modules for logging, file operations, and validation"""

from .log_utils import setup_logging

__all__ = ["setup_logging"]


# utils/log_utils.py
"""
Logging Configuration - Structured logging for governance operations
Implements enterprise-grade logging with environment-aware verbosity
"""

import logging
import sys
from typing import Optional

def setup_logging(level: str = "INFO", format_type: str = "detailed") -> None:
    """
    Configure structured logging for governance operations
    
    Args:
        level: Logging level (DEBUG, INFO, WARNING, ERROR)
        format_type: Format style (simple, detailed, json)
    """
    
    # Configure log level
    log_level = getattr(logging, level.upper(), logging.INFO)
    
    # Configure format based on type
    if format_type == "simple":
        log_format = "%(levelname)s: %(message)s"
    elif format_type == "json":
        log_format = '{"timestamp": "%(asctime)s", "level": "%(levelname)s", "module": "%(name)s", "message": "%(message)s"}'
    else:  # detailed
        log_format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    
    # Configure root logger
    logging.basicConfig(
        level=log_level,
        format=log_format,
        handlers=[
            logging.StreamHandler(sys.stdout)
        ]
    )
    
    # Set framework-specific log levels
    logging.getLogger("sinphase_toolkit").setLevel(log_level)
    
    logger = logging.getLogger(__name__)
    logger.info(f"Logging configured - Level: {level}, Format: {format_type}")


# utils/file_utils.py
"""File system utilities for governance operations"""

import logging
from pathlib import Path
from typing import List, Optional

logger = logging.getLogger(__name__)

def find_project_root(start_path: Optional[Path] = None) -> Path:
    """Find project root by searching for common indicators"""
    if start_path is None:
        start_path = Path.cwd()
    
    current = start_path.resolve()
    
    # Look for common project root indicators
    indicators = [".git", "pyproject.toml", "setup.py", "Makefile", "CMakeLists.txt"]
    
    while current != current.parent:
        if any((current / indicator).exists() for indicator in indicators):
            logger.debug(f"Found project root: {current}")
            return current
        current = current.parent
    
    # Fallback to start path
    logger.warning(f"No project root found, using: {start_path}")
    return start_path

def get_source_files(project_root: Path, extensions: List[str] = None) -> List[Path]:
    """Get source files matching specified extensions"""
    if extensions is None:
        extensions = [".c", ".h", ".cpp", ".hpp", ".py"]
    
    files = []
    for ext in extensions:
        files.extend(project_root.rglob(f"*{ext}"))
    
    # Filter out common non-source directories
    filtered_files = [
        f for f in files
        if not any(part in str(f) for part in ["build", "vendor", ".git", "__pycache__", "node_modules"])
    ]
    
    logger.debug(f"Found {len(filtered_files)} source files")
    return filtered_files