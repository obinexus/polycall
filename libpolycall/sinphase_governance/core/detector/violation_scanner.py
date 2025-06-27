#!/usr/bin/env python3
"""
Sinphasé Violation Detector Module
Production-grade threshold violation detection and severity classification
"""

from typing import Dict, List, Tuple
from dataclasses import dataclass
from enum import Enum

class ViolationSeverity(Enum):
    """Governance violation severity classification."""
    WARNING = "warning"
    CRITICAL = "critical" 
    EMERGENCY = "emergency"

@dataclass
class GovernanceViolation:
    """Comprehensive governance violation analysis result."""
    file_path: str
    cost: float
    threshold: float
    severity: ViolationSeverity
    violation_ratio: float
    
    def to_dict(self) -> Dict:
        return {
            "file_path": self.file_path,
            "cost": self.cost,
            "threshold": self.threshold,
            "severity": self.severity.value,
            "violation_ratio": round(self.violation_ratio, 3)
        }

@dataclass
class ViolationSummary:
    """Comprehensive violation analysis summary for enterprise reporting."""
    total_files: int
    total_violations: int
    critical_violations: int
    emergency_violations: int
    violation_percentage: float
    emergency_action_required: bool
    autonomous_components: int
    
    def to_dict(self) -> Dict:
        return {
            "total_files": self.total_files,
            "total_violations": self.total_violations,
            "critical_violations": self.critical_violations,
            "emergency_violations": self.emergency_violations,
            "violation_percentage": self.violation_percentage,
            "emergency_action_required": self.emergency_action_required,
            "autonomous_components": self.autonomous_components
        }

class SinphaseViolationDetector:
    """
    Enterprise-grade violation detection engine for Sinphasé governance.
    
    Technical Implementation:
    - Implements multi-tier severity classification
    - Provides emergency action triggering based on configurable thresholds
    - Supports enterprise compliance reporting requirements
    """
    
    def __init__(self, config: Dict = None):
        self.config = config or self._get_default_config()
    
    def _get_default_config(self) -> Dict:
        """Default violation detection configuration."""
        return {
            "governance_threshold": 0.6,
            "autonomous_threshold": 0.5,
            "critical_multiplier": 1.5,
            "emergency_multiplier": 2.0,
            "emergency_threshold_count": 5,
            "emergency_percentage": 15.0
        }
    
    def detect_violations(self, 
                         cost_results: Dict[str, float],
                         threshold: float = None) -> Tuple[List[GovernanceViolation], ViolationSummary]:
        """
        Execute comprehensive violation detection analysis.
        
        Args:
            cost_results: Dictionary mapping component paths to cost values
            threshold: Override governance threshold (optional)
            
        Returns:
            Tuple of (violations_list, comprehensive_summary)
        """
        if threshold is None:
            threshold = self.config["governance_threshold"]
        
        violations = []
        autonomous_count = 0
        
        # Analyze each component for governance violations
        for file_path, cost in cost_results.items():
            if cost <= self.config["autonomous_threshold"]:
                autonomous_count += 1
            elif cost > threshold:
                severity = self._classify_violation_severity(cost, threshold)
                violation = GovernanceViolation(
                    file_path=file_path,
                    cost=cost,
                    threshold=threshold,
                    severity=severity,
                    violation_ratio=cost / threshold
                )
                violations.append(violation)
        
        # Generate comprehensive summary
        summary = self._generate_comprehensive_summary(violations, len(cost_results), autonomous_count)
        
        return violations, summary
    
    def _classify_violation_severity(self, cost: float, threshold: float) -> ViolationSeverity:
        """Classify violation severity using enterprise governance criteria."""
        violation_ratio = cost / threshold
        
        if violation_ratio >= self.config["emergency_multiplier"]:
            return ViolationSeverity.EMERGENCY
        elif violation_ratio >= self.config["critical_multiplier"]:
            return ViolationSeverity.CRITICAL
        else:
            return ViolationSeverity.WARNING
    
    def _generate_comprehensive_summary(self, 
                                      violations: List[GovernanceViolation], 
                                      total_files: int, 
                                      autonomous_count: int) -> ViolationSummary:
        """Generate comprehensive violation summary for enterprise reporting."""
        critical_violations = len([v for v in violations 
                                 if v.severity == ViolationSeverity.CRITICAL])
        emergency_violations = len([v for v in violations 
                                  if v.severity == ViolationSeverity.EMERGENCY])
        
        violation_percentage = (len(violations) / total_files * 100) if total_files > 0 else 0
        
        # Determine emergency action requirement
        emergency_action_required = (
            emergency_violations >= self.config["emergency_threshold_count"] or
            violation_percentage >= self.config["emergency_percentage"]
        )
        
        return ViolationSummary(
            total_files=total_files,
            total_violations=len(violations),
            critical_violations=critical_violations,
            emergency_violations=emergency_violations,
            violation_percentage=round(violation_percentage, 2),
            emergency_action_required=emergency_action_required,
            autonomous_components=autonomous_count
        )
