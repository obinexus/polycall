#!/usr/bin/env python3
"""
Sinphasé Report Generator Module
Production-grade governance reporting with multi-format output support
"""

import json
from typing import Dict, List, Any
from datetime import datetime

class SinphaseReportGenerator:
    """
    Enterprise-grade report generator for Sinphasé governance results.
    
    Technical Implementation:
    - Supports multiple output formats (console, JSON, Markdown)
    - Provides enterprise compliance reporting
    - Implements structured data export for CI/CD integration
    """
    
    def __init__(self):
        self.generation_timestamp = datetime.now()
    
    def generate_report(self, 
                       violations: List[Any],
                       summary: Any,
                       format_type: str = "console") -> str:
        """
        Generate governance report in specified format.
        
        Args:
            violations: List of governance violations
            summary: Violation summary statistics
            format_type: Output format (console, json, markdown)
            
        Returns:
            Formatted report string
        """
        if format_type == "json":
            return self._generate_json_report(violations, summary)
        elif format_type == "markdown":
            return self._generate_markdown_report(violations, summary)
        else:
            return self._generate_console_report(violations, summary)
    
    def _generate_console_report(self, violations: List[Any], summary: Any) -> str:
        """Generate console-optimized governance report."""
        lines = [
            "\n📊 Sinphasé Governance Analysis Results",
            "=" * 50,
            f"Analysis Timestamp: {self.generation_timestamp.strftime('%Y-%m-%d %H:%M:%S')}",
            "",
            "📈 Summary Statistics:",
            f"  Total Components Analyzed: {summary.total_files}",
            f"  🟢 Autonomous Components: {summary.autonomous_components}",
            f"  🔴 Governance Violations: {summary.total_violations}",
            f"  ⚠️  Critical Violations: {summary.critical_violations}",
            f"  🚨 Emergency Violations: {summary.emergency_violations}",
            f"  📊 Violation Rate: {summary.violation_percentage}%",
            ""
        ]
        
        if summary.emergency_action_required:
            lines.extend([
                "🚨 EMERGENCY ACTION REQUIRED",
                "Critical governance threshold exceeded - immediate intervention needed",
                ""
            ])
        
        if violations:
            lines.append("🔍 Detailed Violation Analysis:")
            for violation in violations[:10]:  # Limit console output
                severity_icon = {"warning": "⚠️", "critical": "🔴", "emergency": "🚨"}
                icon = severity_icon.get(violation.severity.value, "❓")
                lines.append(f"  {icon} {violation.file_path}")
                lines.append(f"     Cost: {violation.cost:.4f} | Threshold: {violation.threshold}")
                lines.append(f"     Ratio: {violation.violation_ratio:.2f}x | Severity: {violation.severity.value}")
                lines.append("")
            
            if len(violations) > 10:
                lines.append(f"... and {len(violations) - 10} additional violations")
        else:
            lines.append("✅ No governance violations detected")
        
        return "\n".join(lines)
    
    def _generate_json_report(self, violations: List[Any], summary: Any) -> str:
        """Generate JSON-structured governance report for CI/CD integration."""
        report_data = {
            "metadata": {
                "generated_at": self.generation_timestamp.isoformat(),
                "generator": "Sinphasé Governance Framework v2.1.0",
                "format_version": "1.0"
            },
            "summary": summary.to_dict(),
            "violations": [violation.to_dict() for violation in violations],
            "compliance_status": {
                "compliant": summary.total_violations == 0,
                "emergency_action_required": summary.emergency_action_required,
                "governance_level": self._determine_governance_level(summary)
            }
        }
        
        return json.dumps(report_data, indent=2, sort_keys=True)
    
    def _generate_markdown_report(self, violations: List[Any], summary: Any) -> str:
        """Generate Markdown-formatted governance report for documentation."""
        md_content = [
            "# Sinphasé Governance Analysis Report",
            f"**Generated:** {self.generation_timestamp.strftime('%Y-%m-%d %H:%M:%S')}",
            "",
            "## Executive Summary",
            f"- **Total Components:** {summary.total_files}",
            f"- **Autonomous Components:** {summary.autonomous_components}",
            f"- **Governance Violations:** {summary.total_violations}",
            f"- **Critical Violations:** {summary.critical_violations}",
            f"- **Emergency Violations:** {summary.emergency_violations}",
            f"- **Violation Rate:** {summary.violation_percentage}%",
            ""
        ]
        
        if summary.emergency_action_required:
            md_content.extend([
                "## 🚨 Emergency Action Required",
                "Critical governance thresholds have been exceeded. Immediate architectural intervention is required.",
                ""
            ])
        
        if violations:
            md_content.extend([
                "## Detailed Violation Analysis",
                "| Component | Cost | Threshold | Ratio | Severity |",
                "|-----------|------|-----------|-------|----------|"
            ])
            
            for violation in violations:
                severity_badge = f"![{violation.severity.value}](https://img.shields.io/badge/-{violation.severity.value}-red)"
                row = f"| `{violation.file_path}` | {violation.cost:.4f} | {violation.threshold} | {violation.violation_ratio:.2f}x | {severity_badge} |"
                md_content.append(row)
        else:
            md_content.extend([
                "## ✅ Compliance Status",
                "All components comply with Sinphasé governance requirements."
            ])
        
        return "\n".join(md_content)
    
    def _determine_governance_level(self, summary: Any) -> str:
        """Determine overall governance compliance level."""
        if summary.emergency_action_required:
            return "EMERGENCY"
        elif summary.critical_violations > 0:
            return "CRITICAL"
        elif summary.total_violations > 0:
            return "WARNING"
        else:
            return "COMPLIANT"
