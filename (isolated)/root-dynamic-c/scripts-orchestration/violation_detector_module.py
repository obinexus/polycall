#!/usr/bin/env python3
"""
Sinphasé Violation Detection Module
Standalone threshold validation and violation detection logic

Author: OBINexus Computing - Sinphasé Governance Framework
Version: 2.0.0 (Decoupled Architecture)
"""

import json
import logging
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, asdict
from datetime import datetime
from enum import Enum

class ViolationSeverity(Enum):
    """Violation severity levels."""
    INFO = "info"
    WARNING = "warning"
    CRITICAL = "critical"
    EMERGENCY = "emergency"

class GovernanceZone(Enum):
    """Sinphasé governance zones."""
    AUTONOMOUS = "autonomous"      # C ≤ 0.5
    WARNING = "warning"           # 0.5 < C ≤ threshold
    GOVERNANCE = "governance"     # C > threshold

@dataclass
class ViolationDetails:
    """Detailed information about a governance violation."""
    file_path: str
    cost: float
    threshold: float
    violation_ratio: float
    severity: ViolationSeverity
    zone: GovernanceZone
    message: str
    timestamp: datetime
    
    def to_dict(self) -> Dict:
        return {
            "file_path": self.file_path,
            "cost": self.cost,
            "threshold": self.threshold,
            "violation_ratio": self.violation_ratio,
            "severity": self.severity.value,
            "zone": self.zone.value,
            "message": self.message,
            "timestamp": self.timestamp.isoformat()
        }

@dataclass
class ViolationSummary:
    """Summary of violations across a project."""
    total_files: int
    total_violations: int
    critical_violations: int
    warning_violations: int
    autonomous_count: int
    violation_percentage: float
    emergency_action_required: bool
    timestamp: datetime
    
    def to_dict(self) -> Dict:
        return asdict(self)

class SinphaseViolationDetector:
    """
    Standalone violation detection engine for Sinphasé governance.
    
    Implements threshold checking, severity classification, and
    violation reporting without external dependencies.
    """
    
    def __init__(self, config: Optional[Dict] = None):
        """Initialize violation detector with configuration."""
        self.config = config or self._default_config()
        self.logger = self._setup_logging()
        
    def _default_config(self) -> Dict:
        """Default violation detection configuration."""
        return {
            "thresholds": {
                "autonomous_limit": 0.5,
                "governance_threshold": 0.6,
                "critical_multiplier": 1.5,
                "emergency_multiplier": 2.0
            },
            "severity_rules": {
                "critical_violation_count": 10,
                "emergency_percentage": 15.0,
                "warning_ratio": 1.2
            },
            "isolation_rules": {
                "critical_threshold_multiplier": 1.5,
                "emergency_threshold_multiplier": 2.0,
                "isolation_required_count": 5
            }
        }
    
    def _setup_logging(self) -> logging.Logger:
        """Setup logger for violation detector."""
        logger = logging.getLogger('sinphase.violation_detector')
        logger.setLevel(logging.INFO)
        
        if not logger.handlers:
            handler = logging.StreamHandler()
            formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
            )
            handler.setFormatter(formatter)
            logger.addHandler(handler)
        
        return logger
    
    def detect_violations(
        self, 
        cost_results: Dict[str, float],
        threshold: Optional[float] = None
    ) -> Tuple[List[ViolationDetails], ViolationSummary]:
        """
        Detect violations from cost analysis results.
        
        Args:
            cost_results: Dictionary mapping file paths to costs
            threshold: Override governance threshold
            
        Returns:
            Tuple of (violations_list, summary)
        """
        if threshold is None:
            threshold = self.config["thresholds"]["governance_threshold"]
        
        violations = []
        autonomous_count = 0
        warning_count = 0
        
        for file_path, cost in cost_results.items():
            zone, severity = self._classify_violation(cost, threshold)
            
            if zone == GovernanceZone.AUTONOMOUS:
                autonomous_count += 1
            elif zone == GovernanceZone.WARNING:
                warning_count += 1
            elif zone == GovernanceZone.GOVERNANCE:
                violation = self._create_violation_details(
                    file_path, cost, threshold, severity, zone
                )
                violations.append(violation)
        
        # Create summary
        summary = self._create_violation_summary(
            len(cost_results), violations, autonomous_count, warning_count
        )
        
        return violations, summary
    
    def _classify_violation(
        self, 
        cost: float, 
        threshold: float
    ) -> Tuple[GovernanceZone, ViolationSeverity]:
        """
        Classify a cost value into governance zone and severity.
        
        Args:
            cost: Component cost value
            threshold: Governance threshold
            
        Returns:
            Tuple of (zone, severity)
        """
        autonomous_limit = self.config["thresholds"]["autonomous_limit"]
        critical_multiplier = self.config["thresholds"]["critical_multiplier"]
        emergency_multiplier = self.config["thresholds"]["emergency_multiplier"]
        
        if cost <= autonomous_limit:
            return GovernanceZone.AUTONOMOUS, ViolationSeverity.INFO
        elif cost <= threshold:
            return GovernanceZone.WARNING, ViolationSeverity.WARNING
        else:
            # In governance zone - determine severity
            violation_ratio = cost / threshold
            
            if violation_ratio >= emergency_multiplier:
                return GovernanceZone.GOVERNANCE, ViolationSeverity.EMERGENCY
            elif violation_ratio >= critical_multiplier:
                return GovernanceZone.GOVERNANCE, ViolationSeverity.CRITICAL
            else:
                return GovernanceZone.GOVERNANCE, ViolationSeverity.WARNING
    
    def _create_violation_details(
        self,
        file_path: str,
        cost: float,
        threshold: float,
        severity: ViolationSeverity,
        zone: GovernanceZone
    ) -> ViolationDetails:
        """Create detailed violation information."""
        violation_ratio = cost / threshold
        
        # Generate appropriate message
        if severity == ViolationSeverity.EMERGENCY:
            message = f"EMERGENCY: Cost {cost:.4f} is {violation_ratio:.2f}x threshold - IMMEDIATE ISOLATION REQUIRED"
        elif severity == ViolationSeverity.CRITICAL:
            message = f"CRITICAL: Cost {cost:.4f} exceeds {violation_ratio:.2f}x threshold - ISOLATION RECOMMENDED"
        else:
            message = f"Violation: Cost {cost:.4f} exceeds governance threshold {threshold}"
        
        return ViolationDetails(
            file_path=file_path,
            cost=cost,
            threshold=threshold,
            violation_ratio=violation_ratio,
            severity=severity,
            zone=zone,
            message=message,
            timestamp=datetime.now()
        )
    
    def _create_violation_summary(
        self,
        total_files: int,
        violations: List[ViolationDetails],
        autonomous_count: int,
        warning_count: int
    ) -> ViolationSummary:
        """Create violation summary statistics."""
        total_violations = len(violations)
        
        # Count by severity
        critical_violations = len([v for v in violations 
                                 if v.severity in [ViolationSeverity.CRITICAL, ViolationSeverity.EMERGENCY]])
        warning_violations = len([v for v in violations 
                                if v.severity == ViolationSeverity.WARNING])
        
        # Calculate violation percentage
        violation_percentage = (total_violations / total_files * 100) if total_files > 0 else 0
        
        # Determine if emergency action is required
        emergency_action_required = (
            critical_violations >= self.config["severity_rules"]["critical_violation_count"] or
            violation_percentage >= self.config["severity_rules"]["emergency_percentage"]
        )
        
        return ViolationSummary(
            total_files=total_files,
            total_violations=total_violations,
            critical_violations=critical_violations,
            warning_violations=warning_violations,
            autonomous_count=autonomous_count,
            violation_percentage=round(violation_percentage, 2),
            emergency_action_required=emergency_action_required,
            timestamp=datetime.now()
        )
    
    def identify_isolation_candidates(
        self, 
        violations: List[ViolationDetails]
    ) -> Tuple[List[str], List[str]]:
        """
        Identify files that require isolation based on violation severity.
        
        Args:
            violations: List of violation details
            
        Returns:
            Tuple of (tier1_critical, tier2_moderate) file paths
        """
        tier1_critical = []
        tier2_moderate = []
        
        critical_threshold_multiplier = self.config["isolation_rules"]["critical_threshold_multiplier"]
        emergency_threshold_multiplier = self.config["isolation_rules"]["emergency_threshold_multiplier"]
        
        for violation in violations:
            if violation.severity == ViolationSeverity.EMERGENCY:
                tier1_critical.append(violation.file_path)
            elif (violation.severity == ViolationSeverity.CRITICAL or 
                  violation.violation_ratio >= critical_threshold_multiplier):
                tier1_critical.append(violation.file_path)
            elif violation.violation_ratio >= 1.0:  # Any governance zone violation
                tier2_moderate.append(violation.file_path)
        
        return tier1_critical, tier2_moderate
    
    def generate_violation_report(
        self, 
        violations: List[ViolationDetails], 
        summary: ViolationSummary
    ) -> Dict:
        """
        Generate comprehensive violation report.
        
        Args:
            violations: List of violation details
            summary: Violation summary
            
        Returns:
            Complete violation report dictionary
        """
        # Group violations by severity
        violations_by_severity = {}
        for severity in ViolationSeverity:
            violations_by_severity[severity.value] = [
                v.to_dict() for v in violations if v.severity == severity
            ]
        
        # Identify isolation candidates
        tier1_critical, tier2_moderate = self.identify_isolation_candidates(violations)
        
        # Generate recommendations
        recommendations = self._generate_recommendations(summary, violations)
        
        return {
            "timestamp": datetime.now().isoformat(),
            "summary": summary.to_dict(),
            "violations_by_severity": violations_by_severity,
            "isolation_candidates": {
                "tier1_critical": tier1_critical,
                "tier2_moderate": tier2_moderate
            },
            "all_violations": [v.to_dict() for v in violations],
            "recommendations": recommendations
        }
    
    def _generate_recommendations(
        self, 
        summary: ViolationSummary, 
        violations: List[ViolationDetails]
    ) -> List[str]:
        """Generate actionable recommendations based on violations."""
        recommendations = []
        
        if summary.emergency_action_required:
            recommendations.append(
                "🚨 EMERGENCY ACTION REQUIRED: Implement immediate component isolation"
            )
            recommendations.append(
                "📋 Isolate critical violators to emergency containment directory"
            )
        
        if summary.critical_violations > 0:
            recommendations.append(
                f"🔴 Address {summary.critical_violations} critical violations immediately"
            )
            recommendations.append(
                "🛠️ Implement architectural redesign for violating components"
            )
        
        if summary.violation_percentage > 10:
            recommendations.append(
                "📈 High violation rate detected - review governance thresholds"
            )
            recommendations.append(
                "🔧 Consider implementing automated refactoring tools"
            )
        
        if summary.warning_violations > 0:
            recommendations.append(
                f"⚠️ Monitor {summary.warning_violations} warning-level violations"
            )
        
        # Add preventive recommendations
        recommendations.extend([
            "✅ Implement governance hooks to prevent future violations",
            "📊 Set up continuous compliance monitoring",
            "🎯 Establish component-specific governance policies"
        ])
        
        return recommendations

# Example usage and testing
if __name__ == "__main__":
    import sys
    
    detector = SinphaseViolationDetector()
    
    # Example cost results (normally from cost evaluator)
    example_costs = {
        "src/core/ffi/ffi_config.c": 1.2237,
        "src/core/ffi/c_bridge.c": 1.095,
        "src/core/ffi/python_bridge.c": 1.0833,
        "src/core/auth/auth_policy.c": 0.6146,
        "src/core/config/config_factory.c": 0.7266,
        "src/components/simple_component.c": 0.35,
        "src/components/basic_module.c": 0.45
    }
    
    violations, summary = detector.detect_violations(example_costs)
    
    report = detector.generate_violation_report(violations, summary)
    print(json.dumps(report, indent=2))