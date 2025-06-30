#!/usr/bin/env python3
"""
Sinphasé Cost Evaluator Module
Production-grade implementation for enterprise governance cost calculation
"""

import math
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass

@dataclass
class ComponentCostResult:
    """Comprehensive cost evaluation result for software component."""
    component_name: str
    cost: float
    lines_of_code: int
    complexity_factor: float
    dependency_count: int
    function_count: int
    
    def to_dict(self) -> Dict:
        return {
            "component": self.component_name,
            "cost": self.cost,
            "lines_of_code": self.lines_of_code,
            "complexity_factor": self.complexity_factor,
            "dependency_count": self.dependency_count,
            "function_count": self.function_count
        }

class SinphaseCostCalculator:
    """
    Enterprise-grade cost calculator implementing Sinphasé methodology.
    
    Technical Implementation:
    - Implements cyclomatic complexity analysis
    - Applies dependency weight factoring
    - Provides component-level cost aggregation
    - Supports configurable cost function parameters
    """
    
    def __init__(self, config: Optional[Dict] = None):
        self.config = config or self._get_default_config()
    
    def _get_default_config(self) -> Dict:
        """Default cost function configuration parameters."""
        return {
            "lines_factor": 0.001,
            "complexity_factor": 0.05,
            "dependency_factor": 0.02,
            "function_factor": 0.01,
            "phase_multipliers": {
                "DESIGN": 0.5,
                "IMPLEMENTATION": 1.0,
                "VALIDATION": 1.2,
                "DEPLOYMENT": 1.1
            }
        }
    
    def calculate_component_cost(self, component_path: Path) -> ComponentCostResult:
        """
        Calculate comprehensive cost for software component.
        
        Args:
            component_path: Filesystem path to component directory
            
        Returns:
            ComponentCostResult: Detailed cost analysis result
        """
        if not component_path.exists() or not component_path.is_dir():
            return ComponentCostResult(component_path.name, 0.0, 0, 0.0, 0, 0)
        
        # Discover source files
        c_files = list(component_path.glob("**/*.c"))
        h_files = list(component_path.glob("**/*.h"))
        
        if not c_files and not h_files:
            return ComponentCostResult(component_path.name, 0.0, 0, 0.0, 0, 0)
        
        # Aggregate metrics across all files
        total_lines = 0
        total_complexity = 0.0
        total_dependencies = 0
        total_functions = 0
        
        for file_path in c_files + h_files:
            try:
                file_metrics = self._analyze_file(file_path)
                total_lines += file_metrics["lines"]
                total_complexity += file_metrics["complexity"]
                total_dependencies += file_metrics["dependencies"]
                total_functions += file_metrics["functions"]
            except Exception as e:
                # Continue processing other files if individual file analysis fails
                continue
        
        # Apply Sinphasé cost function
        cost = self._calculate_cost(total_lines, total_complexity, total_dependencies, total_functions)
        
        return ComponentCostResult(
            component_name=component_path.name,
            cost=round(cost, 4),
            lines_of_code=total_lines,
            complexity_factor=total_complexity,
            dependency_count=total_dependencies,
            function_count=total_functions
        )
    
    def _analyze_file(self, file_path: Path) -> Dict:
        """Analyze individual source file for cost metrics."""
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception:
            return {"lines": 0, "complexity": 0.0, "dependencies": 0, "functions": 0}
        
        # Count effective lines (excluding comments and empty lines)
        lines = self._count_effective_lines(content)
        
        # Calculate cyclomatic complexity
        complexity = self._calculate_complexity_metrics(content)
        
        # Count dependencies
        dependencies = content.count('#include')
        
        # Estimate function count
        functions = self._estimate_function_count(content)
        
        return {
            "lines": lines,
            "complexity": complexity,
            "dependencies": dependencies,
            "functions": functions
        }
    
    def _count_effective_lines(self, content: str) -> int:
        """Count effective lines of code excluding comments and whitespace."""
        lines = content.splitlines()
        effective_count = 0
        
        in_multiline_comment = False
        
        for line in lines:
            stripped = line.strip()
            
            # Skip empty lines
            if not stripped:
                continue
            
            # Handle multiline comments
            if '/*' in stripped:
                in_multiline_comment = True
            if '*/' in stripped:
                in_multiline_comment = False
                continue
            
            if in_multiline_comment:
                continue
            
            # Skip single-line comments
            if stripped.startswith('//') or stripped.startswith('*'):
                continue
            
            effective_count += 1
        
        return effective_count
    
    def _calculate_complexity_metrics(self, content: str) -> float:
        """Calculate cyclomatic complexity indicators."""
        complexity_indicators = [
            'if ', 'else', 'while ', 'for ', 'switch ', 'case ',
            '&&', '||', '?', ':', 'goto', 'break', 'continue',
            'return', 'throw', 'catch'
        ]
        
        complexity_score = 0.0
        
        for indicator in complexity_indicators:
            count = content.count(indicator)
            complexity_score += count * 0.1
        
        # Additional complexity for nested structures
        brace_depth = self._calculate_nesting_depth(content)
        complexity_score += brace_depth * 0.05
        
        return complexity_score
    
    def _calculate_nesting_depth(self, content: str) -> float:
        """Calculate average nesting depth as complexity indicator."""
        current_depth = 0
        max_depth = 0
        depth_sum = 0
        brace_count = 0
        
        for char in content:
            if char == '{':
                current_depth += 1
                max_depth = max(max_depth, current_depth)
                brace_count += 1
            elif char == '}':
                depth_sum += current_depth
                current_depth = max(0, current_depth - 1)
        
        avg_depth = depth_sum / brace_count if brace_count > 0 else 0
        return avg_depth
    
    def _estimate_function_count(self, content: str) -> int:
        """Estimate function count based on syntactic patterns."""
        # Count opening braces not associated with structs, enums, or arrays
        total_braces = content.count('{')
        struct_braces = content.count('struct') + content.count('enum')
        array_braces = content.count('[]')
        
        estimated_functions = max(0, total_braces - struct_braces - array_braces)
        return estimated_functions
    
    def _calculate_cost(self, lines: int, complexity: float, dependencies: int, functions: int) -> float:
        """Apply Sinphasé cost function to component metrics."""
        base_cost = (
            lines * self.config["lines_factor"] +
            complexity * self.config["complexity_factor"] +
            dependencies * self.config["dependency_factor"] +
            functions * self.config["function_factor"]
        )
        
        return base_cost

def calculate_project_costs(project_root: Path) -> Dict[str, float]:
    """
    Calculate costs for all components in project hierarchy.
    
    Args:
        project_root: Project root directory path
        
    Returns:
        Dict mapping component paths to cost values
    """
    calculator = SinphaseCostCalculator()
    cost_results = {}
    
    # Component discovery in standard source directories
    source_directories = [
        project_root / "src",
        project_root / "libpolycall" / "src",
        project_root / "source"
    ]
    
    for src_dir in source_directories:
        if not src_dir.exists():
            continue
            
        # Discover component directories containing C source files
        for component_dir in src_dir.rglob("*"):
            if (component_dir.is_dir() and 
                any(component_dir.glob("*.c")) and
                "test" not in component_dir.name.lower()):
                
                try:
                    result = calculator.calculate_component_cost(component_dir)
                    relative_path = str(component_dir.relative_to(project_root))
                    cost_results[relative_path] = result.cost
                except Exception:
                    # Continue processing other components if individual analysis fails
                    continue
    
    return cost_results
