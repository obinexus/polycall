#!/usr/bin/env python3
"""Sinphasé Cost Calculator - Minimal Implementation"""

from pathlib import Path
from typing import Dict

def calculate_project_costs(project_root: Path) -> Dict[str, float]:
    """Calculate basic costs for project components."""
    costs = {}
    
    # Find source directories
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
                
                # Simple cost calculation
                c_files = list(component_dir.glob("**/*.c"))
                cost = len(c_files) * 0.1  # Basic cost estimate
                
                relative_path = str(component_dir.relative_to(project_root))
                costs[relative_path] = cost
    
    return costs

class SinphaseCostCalculator:
    """Minimal cost calculator implementation."""
    
    def __init__(self):
        pass
    
    def calculate_component_cost(self, component_path: Path):
        """Calculate cost for component."""
        return {"component": component_path.name, "cost": 0.1}
