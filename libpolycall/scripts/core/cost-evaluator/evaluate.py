#!/usr/bin/env python3
"""
Sinphasé Core Cost Evaluator Module
Functional implementation for cost calculation

Author: OBINexus Computing - Sinphasé Framework
"""

import os
import math
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass

@dataclass
class CostResult:
    """Cost calculation result."""
    component_name: str
    cost: float
    lines_of_code: int
    complexity_factor: float
    
    def to_dict(self) -> Dict:
        return {
            "component": self.component_name,
            "cost": self.cost,
            "lines": self.lines_of_code,
            "complexity": self.complexity_factor
        }

class SinphaseCostCalculator:
    """
    Functional cost calculator implementing Sinphasé methodology.
    """
    
    def __init__(self, config: Optional[Dict] = None):
        self.config = config or {
            "lines_factor": 0.001,
            "complexity_factor": 0.05,
            "dependency_factor": 0.02
        }
    
    def calculate_component_cost(self, component_path: Path) -> CostResult:
        """Calculate cost for a component directory."""
        if not component_path.exists() or not component_path.is_dir():
            return CostResult(component_path.name, 0.0, 0, 0.0)
        
        # Find C source files
        c_files = list(component_path.glob("**/*.c"))
        h_files = list(component_path.glob("**/*.h"))
        
        total_lines = 0
        total_complexity = 0.0
        
        for file_path in c_files + h_files:
            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')
                lines = len([line for line in content.splitlines() 
                           if line.strip() and not line.strip().startswith('//')])
                complexity = self._calculate_complexity(content)
                
                total_lines += lines
                total_complexity += complexity
                
            except Exception:
                continue  # Skip problematic files
        
        # Apply Sinphasé cost function
        base_cost = (
            total_lines * self.config["lines_factor"] +
            total_complexity * self.config["complexity_factor"]
        )
        
        return CostResult(
            component_name=component_path.name,
            cost=round(base_cost, 4),
            lines_of_code=total_lines,
            complexity_factor=total_complexity
        )
    
    def _calculate_complexity(self, content: str) -> float:
        """Calculate complexity score for file content."""
        complexity_indicators = [
            'if ', 'else', 'while ', 'for ', 'switch ',
            '&&', '||', '?', ':', 'goto'
        ]
        
        complexity = 0.0
        for indicator in complexity_indicators:
            complexity += content.count(indicator) * 0.1
        
        # Include complexity
        includes = content.count('#include')
        complexity += includes * 0.02
        
        return complexity

def calculate_project_costs(project_root: Path) -> Dict[str, float]:
    """Calculate costs for all components in project."""
    calculator = SinphaseCostCalculator()
    costs = {}
    
    # Find component directories
    src_dirs = [
        project_root / "src",
        project_root / "libpolycall" / "src"
    ]
    
    for src_dir in src_dirs:
        if not src_dir.exists():
            continue
            
        for component_dir in src_dir.rglob("*"):
            if (component_dir.is_dir() and 
                any(component_dir.glob("*.c")) and
                "test" not in component_dir.name.lower()):
                
                result = calculator.calculate_component_cost(component_dir)
                relative_path = str(component_dir.relative_to(project_root))
                costs[relative_path] = result.cost
    
    return costs

if __name__ == "__main__":
    import json
    costs = calculate_project_costs(Path.cwd())
    print(json.dumps(costs, indent=2))
