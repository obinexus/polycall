#!/usr/bin/env python3
"""
Sinphasé Cost Function Automation - OBINexus Implementation
Emergency governance for 90% violation rate and 1026x FFI threshold
"""

import os
import re
import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple, NamedTuple
from dataclasses import dataclass
import argparse

class SinphaseMetrics(NamedTuple):
    include_depth: int
    function_calls: int
    external_deps: int
    complexity: float
    link_deps: int
    circular_penalty: float
    temporal_pressure: float

@dataclass
class ComponentAnalysis:
    component_path: str
    cost: float
    metrics: SinphaseMetrics
    phase_state: str
    violation_details: List[str]
    isolation_required: bool

class SinphaseCostEvaluator:
    """Emergency Sinphasé governance for crisis recovery"""
    
    # Crisis mode: Stricter thresholds
    WEIGHTS = {
        'include_depth': 0.15,
        'function_calls': 0.20,
        'external_deps': 0.25,
        'complexity': 0.30,
        'link_deps': 0.10
    }
    
    def __init__(self, project_root: str, threshold: float = 0.6):
        self.project_root = Path(project_root)
        self.threshold = threshold
        self.violations = []
        
    def evaluate_project(self, emergency_mode=False):
        """Evaluate entire project for violations"""
        print(f"🔍 Evaluating project: {self.project_root}")
        print(f"📊 Governance threshold: {self.threshold}")
        
        if emergency_mode:
            print("🚨 EMERGENCY MODE: Stricter enforcement")
            self.threshold = 0.5  # Lower threshold in emergency
            
        # Find all C source files
        c_files = list(self.project_root.rglob("*.c"))
        h_files = list(self.project_root.rglob("*.h"))
        
        total_files = len(c_files) + len(h_files)
        print(f"📋 Analyzing {total_files} source files...")
        
        violations = 0
        for source_file in c_files + h_files:
            cost = self._calculate_file_cost(source_file)
            
            if cost > self.threshold:
                violations += 1
                violation = {
                    'file': str(source_file.relative_to(self.project_root)),
                    'cost': cost,
                    'threshold': self.threshold,
                    'violation_ratio': cost / self.threshold
                }
                self.violations.append(violation)
                print(f"❌ VIOLATION: {violation['file']} (cost: {cost:.3f})")
                
        violation_rate = violations / total_files if total_files > 0 else 0
        print(f"📊 Violation rate: {violation_rate:.1%} ({violations}/{total_files})")
        
        if violation_rate > 0.1:  # 10% violation rate is critical
            print("🚨 CRITICAL: High violation rate detected")
            self._write_violation_report()
            return False
            
        return True
        
    def _calculate_file_cost(self, file_path: Path) -> float:
        """Calculate Sinphasé cost for a single file"""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
        except:
            return 0.0
            
        # Basic metrics
        lines = len(content.splitlines())
        includes = len(re.findall(r'#include', content))
        functions = len(re.findall(r'\w+\s*\([^)]*\)\s*{', content))
        ffi_calls = len(re.findall(r'ffi|FFI|extern', content))
        
        # Sinphasé cost calculation
        complexity = lines / 1000.0  # Normalize by typical file size
        include_depth = min(includes / 20.0, 1.0)  # Cap at 1.0
        function_density = min(functions / 50.0, 1.0)  # Cap at 1.0
        external_penalty = min(ffi_calls / 10.0, 1.0)  # FFI penalty
        
        weighted_cost = (
            include_depth * self.WEIGHTS['include_depth'] +
            function_density * self.WEIGHTS['function_calls'] +
            external_penalty * self.WEIGHTS['external_deps'] +
            complexity * self.WEIGHTS['complexity']
        )
        
        return min(weighted_cost, 2.0)  # Cap total cost
        
    def _write_violation_report(self):
        """Write violation report for CI/CD"""
        report_file = self.project_root / "SINPHASE_VIOLATIONS.json"
        
        report = {
            'timestamp': str(Path().absolute()),
            'threshold': self.threshold,
            'total_violations': len(self.violations),
            'violations': self.violations,
            'emergency_action_required': True
        }
        
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
            
        print(f"📋 Violation report written: {report_file}")

def main():
    parser = argparse.ArgumentParser(description='Sinphasé Cost Function Evaluator')
    parser.add_argument('--project-root', default='.', help='Project root directory')
    parser.add_argument('--threshold', type=float, default=0.6, help='Governance threshold')
    parser.add_argument('--emergency-mode', action='store_true', help='Enable emergency mode')
    parser.add_argument('--violation-rate', type=float, help='Expected violation rate for testing')
    
    args = parser.parse_args()
    
    evaluator = SinphaseCostEvaluator(args.project_root, args.threshold)
    
    if evaluator.evaluate_project(args.emergency_mode):
        print("✅ Project complies with Sinphasé governance")
        sys.exit(0)
    else:
        print("❌ Project violates Sinphasé governance")
        sys.exit(1)

if __name__ == '__main__':
    main()
