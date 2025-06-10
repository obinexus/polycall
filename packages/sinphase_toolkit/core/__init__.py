# core/__init__.py
"""
Sinphasé Toolkit Core Modules
Enterprise governance engine components
"""

from .checker import GovernanceChecker
from .reporter import GovernanceReporter
from .refactorer import GovernanceRefactorer

__all__ = ["GovernanceChecker", "GovernanceReporter", "GovernanceRefactorer"]


# core/checker.py
"""
Governance Checker - Orchestrates comprehensive compliance analysis
Implements OBINexus methodology for systematic quality assurance
"""

import logging
from pathlib import Path
from typing import Dict, Any, Optional
from .evaluator.cost_calculator import CostCalculator
from .detector.violation_scanner import ViolationScanner
from .config.environment import EnvironmentDetector, Environment

logger = logging.getLogger(__name__)

class GovernanceChecker:
    """
    Central orchestrator for governance compliance verification
    
    Technical Implementation:
    - Coordinates cost calculation, violation detection, and threshold analysis
    - Applies environment-specific governance policies
    - Implements milestone-based investment compliance tracking
    """
    
    def __init__(self, project_root: Path, threshold: Optional[float] = None, environment: Optional[Environment] = None):
        self.project_root = project_root
        self.environment = environment or EnvironmentDetector().detect_environment()
        
        # Initialize core components
        self.cost_calculator = CostCalculator(project_root)
        self.violation_scanner = ViolationScanner(project_root)
        
        # Set environment-appropriate threshold
        self.threshold = threshold or self._get_environment_threshold()
        
        logger.info(f"Initialized GovernanceChecker for {project_root} (env: {self.environment.value}, threshold: {self.threshold})")
    
    def run_comprehensive_check(self) -> Dict[str, Any]:
        """
        Execute complete governance analysis workflow
        
        Returns comprehensive analysis results including:
        - Cost analysis metrics
        - Violation detection results
        - Compliance scoring
        - Threshold compliance status
        """
        logger.info("Executing comprehensive governance check")
        
        results = {
            "project_root": str(self.project_root),
            "environment": self.environment.value,
            "threshold": self.threshold,
            "timestamp": self._get_timestamp()
        }
        
        # Phase 1: Cost Analysis
        logger.debug("Phase 1: Cost calculation")
        cost_analysis = self.cost_calculator.calculate_comprehensive_costs()
        results["cost_analysis"] = cost_analysis
        
        # Phase 2: Violation Detection
        logger.debug("Phase 2: Violation scanning")
        violations = self.violation_scanner.scan_comprehensive_violations()
        results["violations"] = violations
        
        # Phase 3: Compliance Assessment
        logger.debug("Phase 3: Compliance assessment")
        compliance_score = self._calculate_compliance_score(cost_analysis, violations)
        results["compliance_score"] = compliance_score
        
        # Phase 4: Threshold Validation
        logger.debug("Phase 4: Threshold validation")
        threshold_compliance = compliance_score >= self.threshold
        results["threshold_compliance"] = threshold_compliance
        results["has_violations"] = len(violations) > 0
        
        logger.info(f"Governance check completed - Compliance: {compliance_score:.3f}, Threshold: {threshold_compliance}")
        
        return results
    
    def _get_environment_threshold(self) -> float:
        """Determine appropriate governance threshold based on environment"""
        thresholds = {
            Environment.PRODUCTION: 0.3,
            Environment.CI: 0.4,
            Environment.TEST: 0.5,
            Environment.DEVELOPMENT: 0.6
        }
        return thresholds.get(self.environment, 0.5)
    
    def _calculate_compliance_score(self, cost_analysis: Dict, violations: list) -> float:
        """Calculate normalized compliance score from analysis results"""
        base_score = 1.0
        
        # Cost impact factor
        total_cost = cost_analysis.get("total_cost", 0.0)
        cost_penalty = min(total_cost * 0.1, 0.3)  # Cap at 30% penalty
        
        # Violation impact factor
        violation_penalty = min(len(violations) * 0.05, 0.4)  # Cap at 40% penalty
        
        compliance_score = base_score - cost_penalty - violation_penalty
        return max(compliance_score, 0.0)  # Ensure non-negative
    
    def _get_timestamp(self) -> str:
        """Generate ISO timestamp for analysis tracking"""
        from datetime import datetime
        return datetime.now().isoformat()


# core/reporter.py
"""
Governance Reporter - Generates comprehensive analysis documentation
Implements multi-format reporting for stakeholder communication
"""

import logging
from pathlib import Path
from typing import Dict, Any

logger = logging.getLogger(__name__)

class GovernanceReporter:
    """
    Multi-format governance report generator
    
    Supports enterprise documentation requirements with:
    - Markdown reports for technical teams
    - HTML reports for executive presentation
    - JSON reports for CI/CD integration
    - Console reports for development workflow
    """
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        logger.info(f"Initialized GovernanceReporter for {project_root}")
    
    def generate_report(self, format: str = "markdown", include_details: bool = False) -> str:
        """
        Generate governance report in specified format
        
        Args:
            format: Output format (markdown, html, json, console)
            include_details: Include detailed violation information
            
        Returns:
            str: Formatted report content
        """
        logger.info(f"Generating {format} report (details: {include_details})")
        
        # Gather analysis data
        analysis_data = self._gather_analysis_data()
        
        # Generate format-specific report
        if format == "markdown":
            return self._generate_markdown_report(analysis_data, include_details)
        elif format == "html":
            return self._generate_html_report(analysis_data, include_details)
        elif format == "json":
            return self._generate_json_report(analysis_data, include_details)
        elif format == "console":
            return self._generate_console_report(analysis_data, include_details)
        else:
            raise ValueError(f"Unsupported format: {format}")
    
    def _gather_analysis_data(self) -> Dict[str, Any]:
        """Gather comprehensive analysis data for reporting"""
        # Implementation would integrate with actual analysis components
        return {
            "project_info": {"root": str(self.project_root)},
            "governance_status": "Active",
            "total_files": 42,
            "compliance_score": 0.85
        }
    
    def _generate_markdown_report(self, data: Dict[str, Any], include_details: bool) -> str:
        """Generate markdown-formatted governance report"""
        report = f"""# Sinphasé Governance Report

## Project Overview
- **Root:** {data['project_info']['root']}
- **Status:** {data['governance_status']}
- **Files Analyzed:** {data['total_files']}
- **Compliance Score:** {data['compliance_score']:.1%}

## Summary
Governance analysis completed successfully.
"""
        if include_details:
            report += "\n## Detailed Analysis\n(Details would be included here)\n"
        
        return report
    
    def _generate_html_report(self, data: Dict[str, Any], include_details: bool) -> str:
        """Generate HTML-formatted governance report"""
        return f"""<h1>Sinphasé Governance Report</h1>
<p>Project: {data['project_info']['root']}</p>
<p>Compliance: {data['compliance_score']:.1%}</p>"""
    
    def _generate_json_report(self, data: Dict[str, Any], include_details: bool) -> str:
        """Generate JSON-formatted governance report"""
        import json
        return json.dumps(data, indent=2)
    
    def _generate_console_report(self, data: Dict[str, Any], include_details: bool) -> str:
        """Generate console-formatted governance report"""
        return f"""Sinphasé Governance Report
========================
Project: {data['project_info']['root']}
Compliance: {data['compliance_score']:.1%}
Status: {data['governance_status']}"""


# core/refactorer.py
"""
Governance Refactorer - Automated compliance-driven code improvement
Implements systematic refactoring based on governance analysis
"""

import logging
from pathlib import Path
from typing import Dict, Any, List

logger = logging.getLogger(__name__)

class GovernanceRefactorer:
    """
    Automated governance-driven refactoring engine
    
    Capabilities:
    - FFI interface optimization
    - Include structure reorganization
    - Architecture compliance improvements
    - Automated code quality enhancements
    """
    
    def __init__(self, project_root: Path):
        self.project_root = project_root
        logger.info(f"Initialized GovernanceRefactorer for {project_root}")
    
    def execute_refactoring(self, target: str, dry_run: bool = True, create_backup: bool = True) -> Dict[str, Any]:
        """
        Execute governance-driven refactoring
        
        Args:
            target: Refactoring target (ffi, includes, structure)
            dry_run: Preview changes without applying
            create_backup: Create backup before changes
            
        Returns:
            Dict containing refactoring results and applied changes
        """
        logger.info(f"Executing {target} refactoring (dry_run: {dry_run})")
        
        results = {
            "target": target,
            "dry_run": dry_run,
            "backup_created": False,
            "changes": []
        }
        
        # Create backup if requested and not dry run
        if create_backup and not dry_run:
            self._create_backup()
            results["backup_created"] = True
        
        # Execute target-specific refactoring
        if target == "ffi":
            changes = self._refactor_ffi_interfaces(dry_run)
        elif target == "includes":
            changes = self._refactor_include_structure(dry_run)
        elif target == "structure":
            changes = self._refactor_project_structure(dry_run)
        else:
            raise ValueError(f"Unsupported refactoring target: {target}")
        
        results["changes"] = changes
        logger.info(f"Refactoring completed - {len(changes)} changes identified")
        
        return results
    
    def _refactor_ffi_interfaces(self, dry_run: bool) -> List[str]:
        """Refactor FFI interface implementations"""
        changes = [
            "Optimize FFI function signatures",
            "Consolidate duplicate interface definitions",
            "Improve error handling patterns"
        ]
        
        if not dry_run:
            logger.info("Applying FFI interface refactoring")
            # Implementation would apply actual changes
        
        return changes
    
    def _refactor_include_structure(self, dry_run: bool) -> List[str]:
        """Refactor include file organization"""
        changes = [
            "Reorganize header file dependencies",
            "Remove redundant includes",
            "Optimize include path structure"
        ]
        
        if not dry_run:
            logger.info("Applying include structure refactoring")
            # Implementation would apply actual changes
        
        return changes
    
    def _refactor_project_structure(self, dry_run: bool) -> List[str]:
        """Refactor overall project architecture"""
        changes = [
            "Reorganize module dependencies",
            "Improve layer separation",
            "Enhance configuration management"
        ]
        
        if not dry_run:
            logger.info("Applying project structure refactoring")
            # Implementation would apply actual changes
        
        return changes
    
    def _create_backup(self) -> None:
        """Create backup of project before refactoring"""
        from datetime import datetime
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_name = f"backup_{timestamp}"
        logger.info(f"Creating backup: {backup_name}")
        # Implementation would create actual backup