"""
Governance Reporter Module
"""

from pathlib import Path
from datetime import datetime
import logging

from .checker import GovernanceChecker

logger = logging.getLogger(__name__)

class GovernanceReporter:
    """Unified governance reporting system"""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        self.checker = GovernanceChecker(project_root)
        
    def generate_comprehensive_report(self) -> str:
        """Generate comprehensive governance report"""
        
        check_results = self.checker.run_comprehensive_check()
        status = self.checker.get_governance_status()
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        report = f"""# Sinphasé Governance Report

**Generated:** {timestamp}  
**Project:** {check_results['project_root']}  
**Status:** {check_results['compliance_status']}

## Executive Summary

- **Total Cost:** {check_results['total_cost']:.3f}
- **Threshold:** {check_results['threshold']}
- **Violations:** {check_results['violations']}
- **Compliance:** {"✅ PASS" if check_results['compliance_status'] == 'PASS' else '❌ FAIL'}

## Metrics

| Metric | Value | Status |
|--------|-------|--------|
"""
        
        for metric, data in status.items():
            report += f"| {metric.title()} | {data['value']} | {data['status']} |\n"
        
        if check_results['violations'] > 0:
            report += "\n## Violations\n\n"
            for i, violation in enumerate(check_results['violation_details'], 1):
                report += f"{i}. {violation}\n"
        
        return report
