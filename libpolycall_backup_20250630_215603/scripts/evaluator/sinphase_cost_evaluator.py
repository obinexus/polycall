#!/usr/bin/env python3
"""
Enhanced Sinphasé Cost Function Automation - OBINexus LibPolyCall Integration
Implements Zero Trust governance, DOP adapter integration, and GUID telemetry
"""

import os
import re
import json
import sys
import hashlib
import time
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple, NamedTuple, Optional
from dataclasses import dataclass, asdict
from enum import Enum
import argparse
import logging

# Configure logging for LibPolyCall integration
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

class ComponentType(Enum):
    """DOP Component types aligned with polycall_dop_component_type_t"""
    REACT = 1
    VUE = 2
    NODE = 3
    PYTHON = 4
    WASM = 5
    C_NATIVE = 6
    FFI_BRIDGE = 7
    CUSTOM = 99

class IsolationLevel(Enum):
    """Security isolation levels for Zero Trust integration"""
    NONE = 0
    SANDBOX = 1
    CONTAINER = 2
    VM = 3

class ViolationSeverity(Enum):
    """Violation severity levels aligned with polycall_error_severity_t"""
    INFO = 0
    WARNING = 1
    ERROR = 2
    CRITICAL = 3
    FATAL = 4

@dataclass
class SinphaseMetrics:
    """Enhanced metrics with LibPolyCall integration"""
    include_depth: int
    function_calls: int
    external_deps: int
    ffi_calls: int
    complexity: float
    link_deps: int
    circular_penalty: float
    temporal_pressure: float
    memory_allocations: int
    security_boundaries: int
    zero_trust_violations: int
    protocol_state_changes: int

@dataclass
class ComponentAnalysis:
    """Component analysis with DOP adapter integration"""
    component_path: str
    component_type: ComponentType
    isolation_level: IsolationLevel
    cost: float
    metrics: SinphaseMetrics
    phase_state: str
    violation_details: List[str]
    isolation_required: bool
    security_policy_violations: List[str]
    guid: str  # GUID for telemetry tracking

@dataclass
class SecurityPolicy:
    """Zero Trust security policy configuration"""
    max_ffi_calls: int = 10
    max_external_deps: int = 5
    max_memory_allocations: int = 100
    allowed_protocols: List[str] = None
    isolation_enforcement: bool = True
    audit_required: bool = True

class EnhancedSinphaseCostEvaluator:
    """Enhanced Sinphasé governance with LibPolyCall integration"""
    
    # Enhanced weight configuration for LibPolyCall architecture
    BASE_WEIGHTS = {
        'include_depth': 0.12,
        'function_calls': 0.15,
        'external_deps': 0.20,
        'ffi_calls': 0.25,      # Critical for FFI threshold compliance
        'complexity': 0.18,
        'memory_allocations': 0.10,
        'security_boundaries': 0.15,
        'zero_trust_violations': 0.30,  # Highest weight for security
        'protocol_state_changes': 0.08
    }
    
    # Dynamic threshold adjustment based on component type
    TYPE_MULTIPLIERS = {
        ComponentType.FFI_BRIDGE: 1.5,     # Stricter for FFI components
        ComponentType.C_NATIVE: 1.2,       # Core C components
        ComponentType.PYTHON: 1.0,         # Standard
        ComponentType.NODE: 1.1,           # Slightly stricter
        ComponentType.REACT: 0.9,          # More lenient for UI
        ComponentType.VUE: 0.9,
        ComponentType.WASM: 1.3,           # WASM requires careful monitoring
        ComponentType.CUSTOM: 1.0
    }
    
    def __init__(self, project_root: str, threshold: float = 0.6, 
                 security_policy: Optional[SecurityPolicy] = None):
        self.project_root = Path(project_root)
        self.threshold = threshold
        self.security_policy = security_policy or SecurityPolicy()
        self.violations = []
        self.telemetry_data = []
        
        # Initialize LibPolyCall integration
        self._init_polycall_integration()
        
    def _init_polycall_integration(self):
        """Initialize integration with LibPolyCall core systems"""
        logger.info("Initializing LibPolyCall integration...")
        
        # Check for polycall configuration files
        polycallfile = self.project_root / "Polycallfile"
        polycallrc = self.project_root / ".polycallrc"
        
        self.has_polycall_config = polycallfile.exists() or polycallrc.exists()
        
        if self.has_polycall_config:
            logger.info("Found LibPolyCall configuration files")
            self._load_polycall_config()
        else:
            logger.warning("No LibPolyCall configuration found - using defaults")
            
    def _load_polycall_config(self):
        """Load LibPolyCall configuration for enhanced analysis"""
        # Parse .polycallrc for runtime settings
        polycallrc = self.project_root / ".polycallrc"
        if polycallrc.exists():
            try:
                with open(polycallrc, 'r') as f:
                    config_content = f.read()
                    # Extract memory limits, CPU quotas, etc.
                    self._parse_polycallrc(config_content)
            except Exception as e:
                logger.error(f"Failed to parse .polycallrc: {e}")
                
    def _parse_polycallrc(self, content: str):
        """Parse .polycallrc configuration for policy enforcement"""
        # Extract memory limits for component analysis
        memory_match = re.search(r'memory_limit[:\s]+(\d+)', content)
        if memory_match:
            self.security_policy.max_memory_allocations = int(memory_match.group(1))
            
        # Extract allowed protocols
        protocol_matches = re.findall(r'allowed_protocol[:\s]+([^\n\r]+)', content)
        if protocol_matches:
            self.security_policy.allowed_protocols = protocol_matches
            
    def _generate_guid(self, component_path: str) -> str:
        """Generate GUID for telemetry tracking using SHA-256"""
        path_str = str(component_path)
        timestamp = str(int(time.time()))
        guid_input = f"{path_str}:{timestamp}"
        return hashlib.sha256(guid_input.encode()).hexdigest()[:16]
        
    def _detect_component_type(self, file_path: Path) -> ComponentType:
        """Detect component type based on file analysis"""
        file_ext = file_path.suffix.lower()
        file_content = self._read_file_safe(file_path)
        
        # File extension mapping
        if file_ext in ['.c', '.h']:
            if 'ffi' in file_content.lower() or 'extern' in file_content:
                return ComponentType.FFI_BRIDGE
            return ComponentType.C_NATIVE
        elif file_ext in ['.js', '.jsx']:
            if 'react' in file_content.lower():
                return ComponentType.REACT
            return ComponentType.NODE
        elif file_ext == '.py':
            return ComponentType.PYTHON
        elif file_ext == '.wasm':
            return ComponentType.WASM
        elif file_ext in ['.vue']:
            return ComponentType.VUE
        else:
            return ComponentType.CUSTOM
            
    def _determine_isolation_level(self, component_type: ComponentType, 
                                 security_violations: int) -> IsolationLevel:
        """Determine required isolation level based on component analysis"""
        if security_violations > 5:
            return IsolationLevel.VM
        elif component_type == ComponentType.FFI_BRIDGE:
            return IsolationLevel.CONTAINER
        elif security_violations > 2:
            return IsolationLevel.CONTAINER
        elif security_violations > 0:
            return IsolationLevel.SANDBOX
        else:
            return IsolationLevel.NONE
            
    def _read_file_safe(self, file_path: Path) -> str:
        """Safely read file with encoding fallback"""
        try:
            return file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception as e:
            logger.warning(f"Failed to read {file_path}: {e}")
            return ""
            
    def _analyze_enhanced_metrics(self, file_path: Path, content: str) -> SinphaseMetrics:
        """Enhanced metrics analysis with LibPolyCall integration"""
        lines = len(content.splitlines())
        
        # Basic metrics
        includes = len(re.findall(r'#include|import|require', content))
        functions = len(re.findall(r'\w+\s*\([^)]*\)\s*{', content))
        
        # Enhanced FFI analysis
        ffi_patterns = [
            r'ffi|FFI|extern\s+["\w]',
            r'dlopen|dlsym|GetProcAddress',
            r'ctypes|cffi',
            r'polycall_.*_init|polycall_.*_call'
        ]
        ffi_calls = sum(len(re.findall(pattern, content, re.IGNORECASE)) 
                       for pattern in ffi_patterns)
        
        # Memory allocation detection
        memory_patterns = [
            r'malloc|calloc|realloc|free',
            r'new\s+\w+|delete\s+',
            r'polycall_memory_.*',
            r'alloc|dealloc'
        ]
        memory_allocations = sum(len(re.findall(pattern, content, re.IGNORECASE))
                               for pattern in memory_patterns)
        
        # Security boundary analysis
        security_patterns = [
            r'polycall_dop_.*_init',
            r'polycall_security_.*',
            r'zero_trust|Zero.*Trust',
            r'isolation.*boundary'
        ]
        security_boundaries = sum(len(re.findall(pattern, content, re.IGNORECASE))
                                for pattern in security_patterns)
        
        # Zero Trust violation detection
        violation_patterns = [
            r'trust.*bypass|bypass.*trust',
            r'unsafe.*cast|cast.*unsafe',
            r'raw.*pointer|pointer.*raw',
            r'system\s*\(|exec\s*\(',
            r'eval\s*\(|setTimeout.*eval'
        ]
        zero_trust_violations = sum(len(re.findall(pattern, content, re.IGNORECASE))
                                  for pattern in violation_patterns)
        
        # Protocol state change detection
        state_patterns = [
            r'state.*transition|transition.*state',
            r'polycall_protocol_.*',
            r'context.*change|change.*context'
        ]
        protocol_state_changes = sum(len(re.findall(pattern, content, re.IGNORECASE))
                                   for pattern in state_patterns)
        
        # Calculate complexity metrics
        complexity = (lines / 1000.0) + (functions / 100.0)
        external_deps = len(re.findall(r'extern|import.*from|require\(', content))
        link_deps = len(re.findall(r'link|\.so|\.dll|\.dylib', content))
        
        # Circular dependency detection (simplified)
        circular_penalty = 0.1 if includes > 10 and functions > 20 else 0.0
        
        # Temporal pressure (based on recent changes)
        temporal_pressure = 0.05  # Placeholder - could be calculated from git history
        
        return SinphaseMetrics(
            include_depth=includes,
            function_calls=functions,
            external_deps=external_deps,
            ffi_calls=ffi_calls,
            complexity=complexity,
            link_deps=link_deps,
            circular_penalty=circular_penalty,
            temporal_pressure=temporal_pressure,
            memory_allocations=memory_allocations,
            security_boundaries=security_boundaries,
            zero_trust_violations=zero_trust_violations,
            protocol_state_changes=protocol_state_changes
        )
        
    def _calculate_enhanced_cost(self, metrics: SinphaseMetrics, 
                               component_type: ComponentType) -> float:
        """Enhanced cost calculation with LibPolyCall integration"""
        
        # Normalize metrics to 0-1 range
        normalized_metrics = {
            'include_depth': min(metrics.include_depth / 20.0, 1.0),
            'function_calls': min(metrics.function_calls / 50.0, 1.0),
            'external_deps': min(metrics.external_deps / 10.0, 1.0),
            'ffi_calls': min(metrics.ffi_calls / self.security_policy.max_ffi_calls, 1.0),
            'complexity': min(metrics.complexity, 1.0),
            'memory_allocations': min(metrics.memory_allocations / self.security_policy.max_memory_allocations, 1.0),
            'security_boundaries': min(metrics.security_boundaries / 5.0, 1.0),
            'zero_trust_violations': min(metrics.zero_trust_violations / 3.0, 1.0),
            'protocol_state_changes': min(metrics.protocol_state_changes / 10.0, 1.0)
        }
        
        # Calculate weighted cost
        weighted_cost = sum(
            normalized_metrics[key] * self.BASE_WEIGHTS[key]
            for key in normalized_metrics.keys() if key in self.BASE_WEIGHTS
        )
        
        # Apply component type multiplier
        type_multiplier = self.TYPE_MULTIPLIERS.get(component_type, 1.0)
        final_cost = weighted_cost * type_multiplier
        
        # Add penalties
        final_cost += metrics.circular_penalty + metrics.temporal_pressure
        
        return min(final_cost, 2.0)  # Cap at 2.0
        
    def _analyze_security_violations(self, metrics: SinphaseMetrics, 
                                   component_type: ComponentType) -> List[str]:
        """Analyze security policy violations"""
        violations = []
        
        if metrics.ffi_calls > self.security_policy.max_ffi_calls:
            violations.append(f"FFI call limit exceeded: {metrics.ffi_calls} > {self.security_policy.max_ffi_calls}")
            
        if metrics.external_deps > self.security_policy.max_external_deps:
            violations.append(f"External dependency limit exceeded: {metrics.external_deps} > {self.security_policy.max_external_deps}")
            
        if metrics.memory_allocations > self.security_policy.max_memory_allocations:
            violations.append(f"Memory allocation limit exceeded: {metrics.memory_allocations} > {self.security_policy.max_memory_allocations}")
            
        if metrics.zero_trust_violations > 0:
            violations.append(f"Zero Trust violations detected: {metrics.zero_trust_violations}")
            
        return violations
        
    def evaluate_component(self, file_path: Path) -> ComponentAnalysis:
        """Evaluate a single component with enhanced analysis"""
        content = self._read_file_safe(file_path)
        if not content:
            return None
            
        # Component analysis
        component_type = self._detect_component_type(file_path)
        metrics = self._analyze_enhanced_metrics(file_path, content)
        cost = self._calculate_enhanced_cost(metrics, component_type)
        
        # Security analysis
        security_violations = self._analyze_security_violations(metrics, component_type)
        isolation_level = self._determine_isolation_level(component_type, len(security_violations))
        
        # Generate GUID for telemetry
        guid = self._generate_guid(file_path)
        
        # Determine violation details
        violation_details = []
        if cost > self.threshold:
            violation_details.append(f"Cost threshold exceeded: {cost:.3f} > {self.threshold}")
        
        violation_details.extend(security_violations)
        
        return ComponentAnalysis(
            component_path=str(file_path.relative_to(self.project_root)),
            component_type=component_type,
            isolation_level=isolation_level,
            cost=cost,
            metrics=metrics,
            phase_state="ANALYZED",
            violation_details=violation_details,
            isolation_required=isolation_level != IsolationLevel.NONE,
            security_policy_violations=security_violations,
            guid=guid
        )
        
    def evaluate_project(self, emergency_mode: bool = False) -> bool:
        """Enhanced project evaluation with LibPolyCall integration"""
        logger.info(f"🔍 Evaluating project: {self.project_root}")
        logger.info(f"📊 Governance threshold: {self.threshold}")
        logger.info(f"🔒 Zero Trust enforcement: {self.security_policy.isolation_enforcement}")
        
        if emergency_mode:
            logger.warning("🚨 EMERGENCY MODE: Stricter enforcement activated")
            self.threshold = min(self.threshold * 0.8, 0.4)  # 20% reduction
            
        # Find source files with enhanced patterns
        file_patterns = ['*.c', '*.h', '*.py', '*.js', '*.jsx', '*.vue', '*.wasm']
        source_files: list[Path] = []
        for pattern in file_patterns:
            source_files.extend(self.project_root.rglob(pattern))
            
        total_files = len(source_files)
        logger.info(f"📋 Analyzing {total_files} source files...")
        
        violations = 0
        critical_violations = 0
        components = []
        
        for source_file in source_files:
            analysis = self.evaluate_component(source_file)
            if analysis:
                components.append(analysis)
                
                if analysis.violation_details:
                    violations += 1
                    
                    # Check for critical violations
                    if analysis.metrics.zero_trust_violations > 0 or analysis.cost > (self.threshold * 1.5):
                        critical_violations += 1
                        logger.error(f"❌ CRITICAL VIOLATION: {analysis.component_path} (cost: {analysis.cost:.3f})")
                    else:
                        logger.warning(f"⚠️  VIOLATION: {analysis.component_path} (cost: {analysis.cost:.3f})")
                        
        violation_rate = violations / total_files if total_files > 0 else 0
        critical_rate = critical_violations / total_files if total_files > 0 else 0
        
        logger.info(f"📊 Violation rate: {violation_rate:.1%} ({violations}/{total_files})")
        logger.info(f"🚨 Critical violation rate: {critical_rate:.1%} ({critical_violations}/{total_files})")
        
        # Write comprehensive reports
        self._write_enhanced_violation_report(components, violation_rate, critical_rate)
        self._write_telemetry_data(components)
        
        # Determine overall compliance
        compliance_threshold = 0.1  # 10% violation rate acceptable
        critical_threshold = 0.05   # 5% critical violation rate acceptable
        
        if critical_rate > critical_threshold:
            logger.error("🚨 CRITICAL: Unacceptable critical violation rate")
            return False
        elif violation_rate > compliance_threshold:
            logger.warning("⚠️  WARNING: High violation rate - monitoring required")
            return not emergency_mode  # Fail in emergency mode
        else:
            logger.info("✅ Project complies with enhanced Sinphasé governance")
            return True
            
    def _write_enhanced_violation_report(self, components: List[ComponentAnalysis], 
                                       violation_rate: float, critical_rate: float):
        """Write enhanced violation report with LibPolyCall integration"""
        report_file = self.project_root / "SINPHASE_VIOLATIONS_ENHANCED.json"
        
        # Aggregate statistics
        stats = {
            'total_components': len(components),
            'violation_rate': violation_rate,
            'critical_rate': critical_rate,
            'component_type_distribution': {},
            'isolation_requirements': {},
            'security_policy_summary': asdict(self.security_policy)
        }
        
        # Calculate component type distribution
        for component in components:
            comp_type = component.component_type.name
            if comp_type not in stats['component_type_distribution']:
                stats['component_type_distribution'][comp_type] = 0
            stats['component_type_distribution'][comp_type] += 1
            
            # Calculate isolation level distribution
            isolation_level = component.isolation_level.name
            if isolation_level not in stats['isolation_requirements']:
                stats['isolation_requirements'][isolation_level] = 0
            stats['isolation_requirements'][isolation_level] += 1
            
        report = {
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'project_root': str(self.project_root),
            'threshold': self.threshold,
            'polycall_integration': self.has_polycall_config,
            'statistics': stats,
            'components': [asdict(comp) for comp in components if comp.violation_details],
            'recommendations': self._generate_recommendations(components),
            'emergency_action_required': critical_rate > 0.05
        }
        
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2, default=str)
            
        logger.info(f"📋 Enhanced violation report written: {report_file}")
        
    def _write_telemetry_data(self, components: List[ComponentAnalysis]):
        """Write telemetry data for LibPolyCall GUID system"""
        telemetry_file = self.project_root / "SINPHASE_TELEMETRY.json"
        
        telemetry_data = {
            'evaluation_timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'guid_mappings': {comp.guid: comp.component_path for comp in components},
            'component_metrics': {
                comp.guid: asdict(comp.metrics) for comp in components
            },
            'security_events': [
                {
                    'guid': comp.guid,
                    'component': comp.component_path,
                    'violations': comp.security_policy_violations,
                    'isolation_level': comp.isolation_level.name
                }
                for comp in components if comp.security_policy_violations
            ]
        }
        
        with open(telemetry_file, 'w') as f:
            json.dump(telemetry_data, f, indent=2, default=str)
            
        logger.info(f"📊 Telemetry data written: {telemetry_file}")
        
    def _generate_recommendations(self, components: List[ComponentAnalysis]) -> List[str]:
        """Generate actionable recommendations based on analysis"""
        recommendations = []
        
        # Component-specific recommendations
        ffi_components = [c for c in components if c.component_type == ComponentType.FFI_BRIDGE and c.violation_details]
        if ffi_components:
            recommendations.append("FFI Bridge components require immediate isolation review")
            
        critical_components = [c for c in components if c.metrics.zero_trust_violations > 0]
        if critical_components:
            recommendations.append("Zero Trust violations detected - security audit required")
            
        high_cost_components = [c for c in components if c.cost > (self.threshold * 1.5)]
        if high_cost_components:
            recommendations.append("High-cost components should be refactored or isolated")
            
        # Architecture recommendations
        if len([c for c in components if c.isolation_level == IsolationLevel.VM]) > 0:
            recommendations.append("VM-level isolation required - consider containerization strategy")
            
        return recommendations

def main():
    """Main execution with enhanced argument parsing"""
    parser = argparse.ArgumentParser(
        description='Enhanced Sinphasé Cost Function Evaluator with LibPolyCall Integration'
    )
    parser.add_argument('--project-root', default='.', 
                       help='Project root directory')
    parser.add_argument('--threshold', type=float, default=0.6, 
                       help='Governance threshold (0.0-2.0)')
    parser.add_argument('--emergency-mode', action='store_true', 
                       help='Enable emergency mode with stricter enforcement')
    parser.add_argument('--max-ffi-calls', type=int, default=10,
                       help='Maximum allowed FFI calls per component')
    parser.add_argument('--max-external-deps', type=int, default=5,
                       help='Maximum allowed external dependencies')
    parser.add_argument('--max-memory-allocations', type=int, default=100,
                       help='Maximum allowed memory allocations')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Enable verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
        
    # Configure security policy
    security_policy = SecurityPolicy(
        max_ffi_calls=args.max_ffi_calls,
        max_external_deps=args.max_external_deps,
        max_memory_allocations=args.max_memory_allocations,
        isolation_enforcement=True,
        audit_required=True
    )
    
    # Initialize enhanced evaluator
    evaluator = EnhancedSinphaseCostEvaluator(
        args.project_root, 
        args.threshold,
        security_policy
    )
    
    try:
        if evaluator.evaluate_project(args.emergency_mode):
            logger.info("✅ Project complies with enhanced Sinphasé governance")
            sys.exit(0)
        else:
            logger.error("❌ Project violates enhanced Sinphasé governance")
            sys.exit(1)
    except Exception as e:
        logger.error(f"💥 Evaluation failed: {e}")
        sys.exit(2)

if __name__ == '__main__':
    main()