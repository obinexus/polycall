#!/usr/bin/env python3
"""
Sinphasé Violation Detection Module
Functional implementation for threshold violation detection

Author: OBINexus Computing - Sinphasé Framework
"""

from typing import Dict, List, Tuple
from dataclasses import dataclass
from enum import Enum

class ViolationSeverity(Enum):
    """Violation severity classification."""
    WARNING = "warning"
    CRITICAL = "critical" 
    EMERGENCY = "emergency"

@dataclass
class Violation:
    """Violation details."""
    file_path: str
    cost: float
    threshold: float
    severity: ViolationSeverity
    violation_ratio: float
    
    def to_dict(self) -> Dict:
        return {
            "file": self.file_path,
            "cost": self.cost,
            "threshold": self.threshold,
            "severity": self.severity.value,
            "ratio": self.violation_ratio
        }

@dataclass
class ViolationSummary:
    """Summary of violation analysis."""
    total_files: int
    total_violations: int
    critical_violations: int
    violation_percentage: float
    emergency_action_required: bool
    
    def to_dict(self) -> Dict:
        return {
            "total_files": self.total_files,
            "total_violations": self.total_violations,
            "critical_violations": self.critical_violations,
            "violation_percentage": self.violation_percentage,
            "emergency_action_required": self.emergency_action_required
        }

class SinphaseViolationDetector:
    """
    Functional violation detector implementing Sinphasé governance.
    """
    
    def __init__(self, config: Dict = None):
        self.config = config or {
            "governance_threshold": 0.6,
            "critical_multiplier": 1.5,
            "emergency_multiplier": 2.0
        }
    
    def detect_violations(self, 
                         cost_results: Dict[str, float],
                         threshold: float = None) -> Tuple[List[Violation], ViolationSummary]:
        """Detect violations from cost analysis results."""
        if threshold is None:
            threshold = self.config["governance_threshold"]
        
        violations = []
        
        for file_path, cost in cost_results.items():
            if cost > threshold:
                severity = self._classify_severity(cost, threshold)
                violation = Violation(
                    file_path=file_path,
                    cost=cost,
                    threshold=threshold,
                    severity=severity,
                    violation_ratio=cost / threshold
                )
                violations.append(violation)
        
        # Create summary
        total_files = len(cost_results)
        critical_violations = len([v for v in violations 
                                 if v.severity in [ViolationSeverity.CRITICAL, ViolationSeverity.EMERGENCY]])
        violation_percentage = (len(violations) / total_files * 100) if total_files > 0 else 0
        emergency_action_required = critical_violations >= 5 or violation_percentage >= 15.0
        
        summary = ViolationSummary(
            total_files=total_files,
            total_violations=len(violations),
            critical_violations=critical_violations,
            violation_percentage=round(violation_percentage, 2),
            emergency_action_required=emergency_action_required
        )
        
        return violations, summary
    
    def _classify_severity(self, cost: float, threshold: float) -> ViolationSeverity:
        """Classify violation severity based on cost ratio."""
        ratio = cost / threshold
        
        if ratio >= self.config["emergency_multiplier"]:
            return ViolationSeverity.EMERGENCY
        elif ratio >= self.config["critical_multiplier"]:
            return ViolationSeverity.CRITICAL
        else:
            return ViolationSeverity.WARNING

if __name__ == "__main__":
    # Test functionality
    detector = SinphaseViolationDetector()
    test_costs = {
        "src/component1": 0.4,
        "src/component2": 0.8,  # Violation
        "src/component3": 1.2   # Critical violation
    }
    
    violations, summary = detector.detect_violations(test_costs)
    print(f"Violations: {len(violations)}, Critical: {summary.critical_violations}")
