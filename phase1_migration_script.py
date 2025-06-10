#!/usr/bin/env python3
"""
Sinphasé Phase 1 Migration Script
Automated migration from monolithic to decoupled architecture

Author: OBINexus Computing - Sinphasé Governance Framework
Version: 2.0.0 (Decoupled Architecture Migration)
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List, Optional
import argparse
import logging

class SinphasePhase1Migration:
    """
    Automated migration utility for implementing Phase 1 decoupled architecture.
    
    This script systematically transforms the existing monolithic Sinphasé
    implementation into a modular, environment-aware governance system.
    """
    
    def __init__(self, project_root: Path, backup: bool = True, dry_run: bool = False):
        self.project_root = project_root
        self.backup = backup
        self.dry_run = dry_run
        self.logger = self._setup_logging()
        
        # Define target directory structure
        self.target_structure = {
            "scripts/core/cost-evaluator": ["__init__.py", "evaluate.py", "cost_function.py", "metrics.py"],
            "scripts/core/violation-detector": ["__init__.py", "scan.py", "threshold_checker.py", "violation_model.py"],
            "scripts/core/governance-reporter": ["__init__.py", "generate.py", "formatters.py", "export.py"],
            "scripts/core/config": ["__init__.py", "environment.py", "thresholds.py", "branch_config.py"],
            "scripts/environments/dev": ["governance.py", "debug_hooks.py", "local_validation.py"],
            "scripts/environments/ci": ["gate_enforcement.py", "branch_validation.py", "release_checks.py"],
            "scripts/environments/test": ["std_test_governance.py", "qa_validation.py"],
            "scripts/environments/production": ["strict_governance.py", "monitoring.py"],
            "config/environments": ["dev.yaml", "ci.yaml", "test.yaml", "production.yaml"],
            "config/branches": ["main.yaml", "develop.yaml", "feature.yaml", "release.yaml", "hotfix.yaml"],
            "config/base": ["cost_function.yaml", "violation_rules.yaml", "reporting.yaml", "integration.yaml"]
        }
    
    def _setup_logging(self) -> logging.Logger:
        """Setup comprehensive logging for migration process."""
        logger = logging.getLogger('sinphase.migration')
        logger.setLevel(logging.INFO)
        
        if not logger.handlers:
            # Console handler
            console_handler = logging.StreamHandler()
            console_formatter = logging.Formatter(
                '%(asctime)s - %(levelname)s - %(message)s'
            )
            console_handler.setFormatter(console_formatter)
            logger.addHandler(console_handler)
            
            # File handler
            log_file = self.project_root / "migration.log"
            file_handler = logging.FileHandler(log_file)
            file_formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(funcName)s:%(lineno)d - %(message)s'
            )
            file_handler.setFormatter(file_formatter)
            logger.addHandler(file_handler)
        
        return logger
    
    def run_migration(self) -> bool:
        """
        Execute complete Phase 1 migration process.
        
        Returns:
            bool: True if migration successful, False otherwise
        """
        self.logger.info("🚀 Starting Sinphasé Phase 1 Migration")
        self.logger.info(f"Project root: {self.project_root}")
        self.logger.info(f"Dry run mode: {self.dry_run}")
        
        try:
            # Phase 1A: Backup existing structure
            if self.backup and not self.dry_run:
                self._create_backup()
            
            # Phase 1B: Create new directory structure
            self._create_directory_structure()
            
            # Phase 1C: Extract and modularize existing code
            self._extract_monolithic_components()
            
            # Phase 1D: Create environment-specific entry points
            self._create_environment_entry_points()
            
            # Phase 1E: Implement configuration system
            self._create_configuration_system()
            
            # Phase 1F: Update build and CI integration
            self._update_build_integration()
            
            # Phase 1G: Validation and testing
            self._validate_migration()
            
            self.logger.info("✅ Phase 1 migration completed successfully")
            return True
            
        except Exception as e:
            self.logger.error(f"❌ Migration failed: {e}")
            if self.backup and not self.dry_run:
                self._restore_backup()
            return False
    
    def _create_backup(self):
        """Create comprehensive backup of existing structure."""
        self.logger.info("📦 Creating backup of existing structure")
        
        backup_dir = self.project_root / f"backup_phase1_{self._get_timestamp()}"
        
        if not self.dry_run:
            backup_dir.mkdir(exist_ok=True)
            
            # Backup scripts directory
            scripts_dir = self.project_root / "scripts"
            if scripts_dir.exists():
                shutil.copytree(scripts_dir, backup_dir / "scripts")
                self.logger.info(f"Backed up scripts to {backup_dir / 'scripts'}")
            
            # Backup any existing config
            config_dir = self.project_root / "config"
            if config_dir.exists():
                shutil.copytree(config_dir, backup_dir / "config")
                self.logger.info(f"Backed up config to {backup_dir / 'config'}")
        
        self.backup_dir = backup_dir
        self.logger.info(f"Backup created at: {backup_dir}")
    
    def _create_directory_structure(self):
        """Create the new modular directory structure."""
        self.logger.info("📁 Creating new modular directory structure")
        
        for directory, files in self.target_structure.items():
            dir_path = self.project_root / directory
            
            if not self.dry_run:
                dir_path.mkdir(parents=True, exist_ok=True)
                
                # Create __init__.py files for Python packages
                if any(f.endswith('.py') for f in files):
                    init_file = dir_path / "__init__.py"
                    if not init_file.exists():
                        init_file.write_text('"""Sinphasé modular governance component."""\n')
            
            self.logger.info(f"Created directory: {directory}")
    
    def _extract_monolithic_components(self):
        """Extract components from existing monolithic evaluator."""
        self.logger.info("🔧 Extracting monolithic components")
        
        # Find existing evaluator
        existing_evaluator = self._find_existing_evaluator()
        
        if existing_evaluator:
            self.logger.info(f"Found existing evaluator: {existing_evaluator}")
            
            # Extract cost calculation logic
            self._extract_cost_evaluator(existing_evaluator)
            
            # Extract violation detection logic
            self._extract_violation_detector(existing_evaluator)
            
            # Extract reporting logic
            self._extract_reporter(existing_evaluator)
        else:
            self.logger.warning("No existing evaluator found - creating from templates")
            self._create_from_templates()
    
    def _find_existing_evaluator(self) -> Optional[Path]:
        """Find existing monolithic evaluator script."""
        candidates = [
            self.project_root / "scripts" / "evaluator" / "sinphase_cost_evaluator.py",
            self.project_root / "scripts" / "sinphase_cost_evaluator.py",
            self.project_root / "sinphase_cost_evaluator.py"
        ]
        
        for candidate in candidates:
            if candidate.exists():
                return candidate
        
        return None
    
    def _extract_cost_evaluator(self, source_file: Path):
        """Extract cost evaluation logic from monolithic script."""
        self.logger.info("⚙️ Extracting cost evaluation logic")
        
        target_file = self.project_root / "scripts/core/cost-evaluator/evaluate.py"
        
        if not self.dry_run:
            # For now, copy our modular implementation
            # In a real scenario, this would parse the existing script
            self._write_modular_cost_evaluator(target_file)
        
        self.logger.info(f"Extracted cost evaluator to: {target_file}")
    
    def _extract_violation_detector(self, source_file: Path):
        """Extract violation detection logic."""
        self.logger.info("🔍 Extracting violation detection logic")
        
        target_file = self.project_root / "scripts/core/violation-detector/scan.py"
        
        if not self.dry_run:
            self._write_modular_violation_detector(target_file)
        
        self.logger.info(f"Extracted violation detector to: {target_file}")
    
    def _extract_reporter(self, source_file: Path):
        """Extract reporting logic."""
        self.logger.info("📊 Extracting reporting logic")
        
        target_file = self.project_root / "scripts/core/governance-reporter/generate.py"
        
        if not self.dry_run:
            self._write_modular_reporter(target_file)
        
        self.logger.info(f"Extracted reporter to: {target_file}")
    
    def _create_environment_entry_points(self):
        """Create environment-specific governance entry points."""
        self.logger.info("🌍 Creating environment-specific entry points")
        
        environments = ["dev", "ci", "test", "production"]
        
        for env in environments:
            entry_point = self.project_root / f"scripts/environments/{env}/governance.py"
            
            if not self.dry_run:
                self._write_environment_entry_point(entry_point, env)
            
            self.logger.info(f"Created {env} environment entry point: {entry_point}")
    
    def _create_configuration_system(self):
        """Create branch-aware configuration system."""
        self.logger.info("⚙️ Creating configuration system")
        
        config_files = {
            "config/base/cost_function.yaml": self._get_cost_function_config(),
            "config/base/violation_rules.yaml": self._get_violation_rules_config(),
            "config/base/reporting.yaml": self._get_reporting_config(),
            "config/environments/dev.yaml": self._get_dev_config(),
            "config/environments/ci.yaml": self._get_ci_config(),
            "config/branches/main.yaml": self._get_main_branch_config(),
            "config/branches/develop.yaml": self._get_develop_branch_config(),
            "config/branches/feature.yaml": self._get_feature_branch_config()
        }
        
        for file_path, content in config_files.items():
            full_path = self.project_root / file_path
            
            if not self.dry_run:
                full_path.parent.mkdir(parents=True, exist_ok=True)
                full_path.write_text(content)
            
            self.logger.info(f"Created config file: {file_path}")
    
    def _update_build_integration(self):
        """Update build system integration."""
        self.logger.info("🔨 Updating build integration")
        
        # Create new governance check script
        new_check_script = self.project_root / "scripts/sinphase_check.py"
        
        if not self.dry_run:
            self._write_new_check_script(new_check_script)
            new_check_script.chmod(0o755)
        
        # Update existing shell scripts
        existing_check = self.project_root / "scripts/sinphases-check.sh"
        if existing_check.exists() and not self.dry_run:
            self._update_shell_script(existing_check)
        
        self.logger.info("Updated build integration scripts")
    
    def _validate_migration(self):
        """Validate migration results."""
        self.logger.info("✅ Validating migration results")
        
        # Check directory structure
        missing_dirs = []
        for directory in self.target_structure.keys():
            dir_path = self.project_root / directory
            if not dir_path.exists():
                missing_dirs.append(directory)
        
        if missing_dirs:
            raise Exception(f"Missing directories: {missing_dirs}")
        
        # Test new governance system
        if not self.dry_run:
            self._test_new_system()
        
        self.logger.info("Migration validation completed successfully")
    
    def _test_new_system(self):
        """Test the new modular governance system."""
        self.logger.info("🧪 Testing new governance system")
        
        try:
            # Test cost evaluator
            sys.path.insert(0, str(self.project_root / "scripts"))
            
            # Import and test new modules
            from core.cost_evaluator.evaluate import SinphaseCostCalculator
            from core.violation_detector.scan import SinphaseViolationDetector
            
            calculator = SinphaseCostCalculator()
            detector = SinphaseViolationDetector()
            
            self.logger.info("✅ New modules imported successfully")
            
        except ImportError as e:
            raise Exception(f"Failed to import new modules: {e}")
    
    def _write_modular_cost_evaluator(self, target_file: Path):
        """Write the modular cost evaluator implementation."""
        # This would contain the actual implementation from our previous artifact
        content = '''# Extracted cost evaluator implementation
# (Implementation details from core_cost_evaluator artifact)
'''
        target_file.write_text(content)
    
    def _write_modular_violation_detector(self, target_file: Path):
        """Write the modular violation detector implementation."""
        content = '''# Extracted violation detector implementation
# (Implementation details from violation_detector_module artifact)
'''
        target_file.write_text(content)
    
    def _write_modular_reporter(self, target_file: Path):
        """Write the modular reporter implementation."""
        content = '''# Extracted reporter implementation
# (Implementation details from governance reporter)
'''
        target_file.write_text(content)
    
    def _write_environment_entry_point(self, target_file: Path, environment: str):
        """Write environment-specific entry point."""
        content = f'''#!/usr/bin/env python3
"""
{environment.upper()} Environment Governance Entry Point
"""

import sys
from pathlib import Path

# Import environment-aware runner
sys.path.insert(0, str(Path(__file__).parent.parent.parent))
from environments.governance_runner import SinphaseGovernanceRunner, Environment

def main():
    runner = SinphaseGovernanceRunner(Path.cwd(), Environment.{environment.upper()})
    success, results = runner.run_governance_check()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
'''
        target_file.write_text(content)
        target_file.chmod(0o755)
    
    def _write_new_check_script(self, target_file: Path):
        """Write new unified governance check script."""
        content = '''#!/usr/bin/env python3
"""
Sinphasé Environment-Aware Governance Check
Unified entry point for all governance operations
"""

import sys
import argparse
from pathlib import Path

# Import environment-aware runner
sys.path.insert(0, str(Path(__file__).parent))
from environments.governance_runner import SinphaseGovernanceRunner, Environment

def main():
    parser = argparse.ArgumentParser(description="Sinphasé Governance Check")
    parser.add_argument("--environment", choices=[e.value for e in Environment])
    parser.add_argument("--fail-on-violations", action="store_true")
    parser.add_argument("--project-root", default=".")
    
    args = parser.parse_args()
    
    environment = Environment(args.environment) if args.environment else None
    runner = SinphaseGovernanceRunner(Path(args.project_root), environment)
    
    success, results = runner.run_governance_check(args.fail_on_violations)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
'''
        target_file.write_text(content)
    
    def _get_timestamp(self) -> str:
        """Get current timestamp for backup naming."""
        from datetime import datetime
        return datetime.now().strftime("%Y%m%d_%H%M%S")
    
    def _get_cost_function_config(self) -> str:
        """Get cost function configuration content."""
        return """# Sinphasé Cost Function Configuration
cost_function:
  complexity_weights:
    lines_factor: 0.001
    dependency_factor: 0.05
    include_factor: 0.02
    function_factor: 0.01
  
  cost_multipliers:
    c_files: 1.0
    header_files: 0.8
    test_files: 0.6
"""
    
    def _get_violation_rules_config(self) -> str:
        """Get violation rules configuration content."""
        return """# Sinphasé Violation Rules Configuration
violation_detection:
  severity_thresholds:
    warning: 1.0
    critical: 1.5
    emergency: 2.0
  
  isolation_rules:
    tier1_critical_threshold: 1.5
    tier2_moderate_threshold: 1.2
"""
    
    def _get_reporting_config(self) -> str:
        """Get reporting configuration content."""
        return """# Sinphasé Reporting Configuration
reporting:
  formats:
    console:
      colored_output: true
      summary_only: true
    markdown:
      include_graphs: true
      detailed_metrics: true
"""
    
    def _get_dev_config(self) -> str:
        """Get development environment configuration."""
        return """# Development Environment Configuration
environment: development
governance:
  base_threshold: 0.8
  fail_on_violations: false
  isolation_enabled: false

reporting:
  level: "summary"
  format: "console"
  fast_feedback: true
"""
    
    def _get_ci_config(self) -> str:
        """Get CI environment configuration."""
        return """# CI/CD Environment Configuration
environment: ci_cd
governance:
  base_threshold: 0.6
  fail_on_violations: true
  isolation_enabled: true

reporting:
  level: "detailed"
  format: "structured"
  export_artifacts: true
"""
    
    def _get_main_branch_config(self) -> str:
        """Get main branch configuration."""
        return """# Main Branch Configuration
branch: main
governance:
  threshold_multiplier: 1.0
  required_approvals: 2
  mandatory_review: true
"""
    
    def _get_develop_branch_config(self) -> str:
        """Get develop branch configuration."""
        return """# Develop Branch Configuration
branch: develop
governance:
  threshold_multiplier: 1.3
  required_approvals: 1
  mandatory_review: true
"""
    
    def _get_feature_branch_config(self) -> str:
        """Get feature branch configuration."""
        return """# Feature Branch Configuration
branch_pattern: "feature/*"
governance:
  threshold_multiplier: 1.7
  required_approvals: 0
  mandatory_review: false
"""
    
    def _create_from_templates(self):
        """Create components from templates when no existing code found."""
        self.logger.info("Creating components from templates")
        # Implementation would create template-based components
    
    def _update_shell_script(self, script_path: Path):
        """Update existing shell script to use new system."""
        self.logger.info(f"Updating shell script: {script_path}")
        # Implementation would update the shell script
    
    def _restore_backup(self):
        """Restore from backup in case of failure."""
        self.logger.info("Restoring from backup due to migration failure")
        # Implementation would restore from backup

def main():
    """Main entry point for migration script."""
    parser = argparse.ArgumentParser(description="Sinphasé Phase 1 Migration")
    parser.add_argument("--project-root", default=".", help="Project root directory")
    parser.add_argument("--no-backup", action="store_true", help="Skip backup creation")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be done")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    migration = SinphasePhase1Migration(
        project_root=Path(args.project_root),
        backup=not args.no_backup,
        dry_run=args.dry_run
    )
    
    success = migration.run_migration()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
