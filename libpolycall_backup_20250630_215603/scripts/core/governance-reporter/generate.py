#!/usr/bin/env python3
"""
Sinphasé Governance Reporter Module
Functional implementation for governance reporting

Author: OBINexus Computing - Sinphasé Framework
"""

import json
from typing import Dict, List, Any
from datetime import datetime

class SinphaseReportGenerator:
    """
    Functional report generator for Sinphasé governance results.
    """
    
    def __init__(self):
        self.timestamp = datetime.now()
    
    def generate_report(self, 
                       violations: List[Any],
                       summary: Any,
                       format_type: str = "markdown") -> str:
        """Generate governance report in specified format."""
        if format_type == "json":
            return self._generate_json_report(violations, summary)
        elif format_type == "markdown":
            return self._generate_markdown_report(violations, summary)
        else:
            return self._generate_console_report(violations, summary)
    
    def _generate_json_report(self, violations: List[Any], summary: Any) -> str:
        """Generate JSON format report."""
        report = {
            "timestamp": self.timestamp.isoformat(),
            "summary": summary.to_dict(),
            "violations": [v.to_dict() for v in violations]
        }
        return json.dumps(report, indent=2)
    
    def _generate_markdown_report(self, violations: List[Any], summary: Any) -> str:
        """Generate Markdown format report."""
        md_content = f"""# Sinphasé Governance Report
Generated: {self.timestamp.strftime('%Y-%m-%d %H:%M:%S')}

## Summary
- **Total Files Analyzed**: {summary.total_files}
- **Total Violations**: {summary.total_violations}
- **Critical Violations**: {summary.critical_violations}
- **Violation Rate**: {summary.violation_percentage}%
- **Emergency Action Required**: {'Yes' if summary.emergency_action_required else 'No'}

## Violations
"""
        
        if violations:
            for violation in violations:
                md_content += f"""
### {violation.file_path}
- **Cost**: {violation.cost}
- **Threshold**: {violation.threshold}
- **Severity**: {violation.severity.value}
- **Violation Ratio**: {violation.violation_ratio:.2f}x
"""
        else:
            md_content += "\n✅ No violations detected."
        
        return md_content
    
    def _generate_console_report(self, violations: List[Any], summary: Any) -> str:
        """Generate console-friendly report."""
        lines = [
            f"📊 Sinphasé Governance Results:",
            f"  Total Files: {summary.total_files}",
            f"  Violations: {summary.total_violations}",
            f"  Critical: {summary.critical_violations}",
            f"  Rate: {summary.violation_percentage}%"
        ]
        
        if summary.emergency_action_required:
            lines.append("🚨 EMERGENCY ACTION REQUIRED")
        
        return "\n".join(lines)

if __name__ == "__main__":
    # Test functionality
    generator = SinphaseReportGenerator()
    print("Governance reporter initialized successfully")
