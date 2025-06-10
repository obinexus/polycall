#!/usr/bin/env python3
"""
Sinphasé Core Cost Evaluator Module
Standalone cost calculation logic extracted from monolithic evaluator

Author: OBINexus Computing - Sinphasé Governance Framework
Version: 2.0.0 (Decoupled Architecture)
"""

import os
import sys
import json
import math
import logging
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Union
from dataclasses import dataclass, asdict
from datetime import datetime

@dataclass
class CostMetrics:
    """Cost calculation metrics for a component or file."""
    lines_of_code: int
    complexity_factor: float
    dependency_count: int
    include_complexity: float
    function_count: int
    base_cost: float
    final_cost: float
    
    def to_dict(self) -> Dict:
        return asdict(self)

@dataclass
class ComponentCost:
    """Cost analysis for a software component."""
    name: str
    path: str
    cost: float
    metrics: CostMetrics
    phase: str
    timestamp: datetime
    
    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "path": self.path,
            "cost": self.cost,
            "metrics": self.metrics.to_dict(),
            "phase": self.phase,
            "timestamp": self.timestamp.isoformat()
        }

class SinphaseCostCalculator:
    """
    Pure cost calculation engine implementing Sinphasé cost function.
    
    This class is completely standalone with no external dependencies
    beyond standard library modules.
    """
    
    def __init__(self, config: Optional[Dict] = None):
        """Initialize cost calculator with optional configuration."""
        self.config = config or self._default_config()
        self.logger = self._setup_logging()
        
    def _default_config(self) -> Dict:
        """Default cost function configuration."""
        return {
            "complexity_weights": {
                "lines_factor": 0.001,
                "dependency_factor": 0.05,
                "include_factor": 0.02,
                "function_factor": 0.01
            },
            "cost_multipliers": {
                "c_files": 1.0,
                "header_files": 0.8,
                "test_files": 0.6
            },
            "phases": {
                "DESIGN": 0.5,
                "IMPLEMENTATION": 1.0,
                "VALIDATION": 1.2,
                "DEPLOYMENT": 1.1
            }
        }
    
    def _setup_logging(self) -> logging.Logger:
        """Setup logger for cost calculator."""
        logger = logging.getLogger('sinphase.cost_calculator')
        logger.setLevel(logging.INFO)
        
        if not logger.handlers:
            handler = logging.StreamHandler()
            formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
            )
            handler.setFormatter(formatter)
            logger.addHandler(handler)
        
        return logger
    
    def calculate_file_cost(self, file_path: Path) -> ComponentCost:
        """
        Calculate Sinphasé cost for a single file.
        
        Args:
            file_path: Path to the file to analyze
            
        Returns:
            ComponentCost object with detailed cost analysis
        """
        if not file_path.exists():
            raise FileNotFoundError(f"File not found: {file_path}")
        
        try:
            content = file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception as e:
            self.logger.warning(f"Could not read {file_path}: {e}")
            return self._empty_cost(file_path)
        
        # Calculate basic metrics
        metrics = self._calculate_metrics(content, file_path)
        
        # Apply Sinphasé cost function
        cost = self._apply_cost_function(metrics, file_path)
        
        # Determine component phase
        phase = self._determine_phase(file_path)
        
        return ComponentCost(
            name=file_path.name,
            path=str(file_path),
            cost=cost,
            metrics=metrics,
            phase=phase,
            timestamp=datetime.now()
        )
    
    def calculate_component_cost(self, component_path: Path) -> ComponentCost:
        """
        Calculate aggregated cost for a component (directory).
        
        Args:
            component_path: Path to component directory
            
        Returns:
            ComponentCost object with aggregated cost analysis
        """
        if not component_path.is_dir():
            raise ValueError(f"Path is not a directory: {component_path}")
        
        c_files = list(component_path.glob("**/*.c"))
        h_files = list(component_path.glob("**/*.h"))
        
        if not c_files and not h_files:
            self.logger.warning(f"No C/H files found in {component_path}")
            return self._empty_component_cost(component_path)
        
        # Calculate costs for all files
        file_costs = []
        for file_path in c_files + h_files:
            try:
                file_cost = self.calculate_file_cost(file_path)
                file_costs.append(file_cost)
            except Exception as e:
                self.logger.error(f"Error calculating cost for {file_path}: {e}")
        
        # Aggregate metrics and costs
        return self._aggregate_component_cost(component_path, file_costs)
    
    def _calculate_metrics(self, content: str, file_path: Path) -> CostMetrics:
        """Calculate detailed metrics for file content."""
        lines = content.splitlines()
        
        # Basic metrics
        lines_of_code = len([line for line in lines if line.strip() and not line.strip().startswith('//')])
        
        # Complexity analysis
        complexity_indicators = [
            'if ', 'else', 'while ', 'for ', 'switch ', 'case ',
            '&&', '||', '?', ':', 'goto', 'break', 'continue'
        ]
        complexity_factor = sum(content.count(indicator) for indicator in complexity_indicators)
        
        # Dependency analysis
        include_lines = [line for line in lines if line.strip().startswith('#include')]
        dependency_count = len(include_lines)
        
        # Include complexity (nested includes, relative paths)
        include_complexity = self._calculate_include_complexity(include_lines)
        
        # Function count estimation
        function_count = content.count('{') - content.count('struct') - content.count('enum')
        function_count = max(0, function_count)  # Ensure non-negative
        
        # Base cost calculation
        weights = self.config["complexity_weights"]
        base_cost = (
            lines_of_code * weights["lines_factor"] +
            complexity_factor * weights["dependency_factor"] +
            dependency_count * weights["include_factor"] +
            function_count * weights["function_factor"]
        )
        
        # Apply file type multiplier
        multiplier = self._get_file_multiplier(file_path)
        final_cost = base_cost * multiplier
        
        return CostMetrics(
            lines_of_code=lines_of_code,
            complexity_factor=complexity_factor,
            dependency_count=dependency_count,
            include_complexity=include_complexity,
            function_count=function_count,
            base_cost=base_cost,
            final_cost=final_cost
        )
    
    def _calculate_include_complexity(self, include_lines: List[str]) -> float:
        """Calculate complexity score for include statements."""
        complexity = 0.0
        
        for line in include_lines:
            # Relative path includes add complexity
            if '../' in line:
                complexity += line.count('../') * 0.1
            
            # Long include paths add complexity
            if line.count('/') > 3:
                complexity += 0.05
            
            # Non-standard include patterns
            if any(pattern in line for pattern in ['polycall/', 'core/', 'ffi/']):
                complexity += 0.02
        
        return complexity
    
    def _get_file_multiplier(self, file_path: Path) -> float:
        """Get cost multiplier based on file type."""
        multipliers = self.config["cost_multipliers"]
        
        if file_path.suffix == '.c':
            if 'test' in file_path.name.lower():
                return multipliers["test_files"]
            return multipliers["c_files"]
        elif file_path.suffix == '.h':
            return multipliers["header_files"]
        else:
            return 1.0
    
    def _apply_cost_function(self, metrics: CostMetrics, file_path: Path) -> float:
        """Apply the core Sinphasé cost function."""
        base_cost = metrics.final_cost
        
        # Apply phase multiplier
        phase = self._determine_phase(file_path)
        phase_multiplier = self.config["phases"].get(phase, 1.0)
        
        # Final cost with phase adjustment
        final_cost = base_cost * phase_multiplier
        
        return round(final_cost, 4)
    
    def _determine_phase(self, file_path: Path) -> str:
        """Determine development phase based on file characteristics."""
        path_str = str(file_path).lower()
        
        if any(keyword in path_str for keyword in ['test', 'spec', 'check']):
            return "VALIDATION"
        elif any(keyword in path_str for keyword in ['deploy', 'install', 'package']):
            return "DEPLOYMENT"
        elif file_path.suffix == '.h':
            return "DESIGN"
        else:
            return "IMPLEMENTATION"
    
    def _aggregate_component_cost(self, component_path: Path, file_costs: List[ComponentCost]) -> ComponentCost:
        """Aggregate file costs into component cost."""
        if not file_costs:
            return self._empty_component_cost(component_path)
        
        total_cost = sum(fc.cost for fc in file_costs)
        
        # Aggregate metrics
        total_lines = sum(fc.metrics.lines_of_code for fc in file_costs)
        avg_complexity = sum(fc.metrics.complexity_factor for fc in file_costs) / len(file_costs)
        total_dependencies = sum(fc.metrics.dependency_count for fc in file_costs)
        avg_include_complexity = sum(fc.metrics.include_complexity for fc in file_costs) / len(file_costs)
        total_functions = sum(fc.metrics.function_count for fc in file_costs)
        
        aggregated_metrics = CostMetrics(
            lines_of_code=total_lines,
            complexity_factor=avg_complexity,
            dependency_count=total_dependencies,
            include_complexity=avg_include_complexity,
            function_count=total_functions,
            base_cost=sum(fc.metrics.base_cost for fc in file_costs),
            final_cost=total_cost
        )
        
        # Determine dominant phase
        phase_counts = {}
        for fc in file_costs:
            phase_counts[fc.phase] = phase_counts.get(fc.phase, 0) + 1
        dominant_phase = max(phase_counts, key=phase_counts.get)
        
        return ComponentCost(
            name=component_path.name,
            path=str(component_path),
            cost=round(total_cost, 4),
            metrics=aggregated_metrics,
            phase=dominant_phase,
            timestamp=datetime.now()
        )
    
    def _empty_cost(self, file_path: Path) -> ComponentCost:
        """Create empty cost result for invalid files."""
        empty_metrics = CostMetrics(0, 0.0, 0, 0.0, 0, 0.0, 0.0)
        return ComponentCost(
            name=file_path.name,
            path=str(file_path),
            cost=0.0,
            metrics=empty_metrics,
            phase="UNKNOWN",
            timestamp=datetime.now()
        )
    
    def _empty_component_cost(self, component_path: Path) -> ComponentCost:
        """Create empty cost result for invalid components."""
        empty_metrics = CostMetrics(0, 0.0, 0, 0.0, 0, 0.0, 0.0)
        return ComponentCost(
            name=component_path.name,
            path=str(component_path),
            cost=0.0,
            metrics=empty_metrics,
            phase="UNKNOWN",
            timestamp=datetime.now()
        )

# Example usage
if __name__ == "__main__":
    calculator = SinphaseCostCalculator()
    
    # Example file cost calculation
    if len(sys.argv) > 1:
        target_path = Path(sys.argv[1])
        
        if target_path.is_file():
            cost_result = calculator.calculate_file_cost(target_path)
            print(json.dumps(cost_result.to_dict(), indent=2))
        elif target_path.is_dir():
            cost_result = calculator.calculate_component_cost(target_path)
            print(json.dumps(cost_result.to_dict(), indent=2))
        else:
            print(f"Invalid path: {target_path}")
            sys.exit(1)
    else:
        print("Usage: python evaluate.py <file_or_directory_path>")
        sys.exit(1)