"""
Governance Checker Module
OBINexus Computing - Core Analysis Engine
"""

from pathlib import Path
from typing import Dict, Any, Optional
import logging

logger = logging.getLogger(__name__)

class GovernanceChecker:
    """Unified governance checking orchestrator"""
    
    def __init__(self, project_root: Path, threshold: Optional[float] = None, environment=None):
        self.project_root = Path(project_root)
        self.threshold = threshold or 0.6
        self.environment = environment
        
    def run_comprehensive_check(self) -> Dict[str, Any]:
        """Run complete governance check pipeline"""
        logger.info(f"Starting governance check for {self.project_root}")
        
        # Calculate project cost
        total_cost = self._calculate_project_cost()
        
        # Check for violations
        violations = []
        if total_cost > self.threshold:
            violations.append(f"Total cost {total_cost:.3f} exceeds threshold {self.threshold}")
        
        # Assess compliance
        compliance_score = max(0.0, 1.0 - (total_cost / self.threshold))
        
        results = {
            "project_root": str(self.project_root),
            "threshold": self.threshold,
            "total_cost": total_cost,
            "violations": violations,
            "has_violations": len(violations) > 0,
            "compliance_status": "PASS" if len(violations) == 0 else "FAIL",
            "cost_analysis": {"total_cost": total_cost},
            "compliance_score": compliance_score
        }
        
        return results
    
    def get_governance_status(self) -> Dict[str, Any]:
        """Get governance status overview"""
        total_cost = self._calculate_project_cost()
        
        return {
            "total_cost": {
                "value": f"{total_cost:.3f}",
                "status": "🔴" if total_cost > 0.8 else "🟡" if total_cost > 0.4 else "🟢"
            },
            "file_count": {
                "value": str(len(list(self.project_root.rglob("*.c")) + list(self.project_root.rglob("*.h")))),
                "status": "📁"
            },
            "compliance": {
                "value": "Active",
                "status": "🎯"
            }
        }
    
    def _calculate_project_cost(self) -> float:
        """Calculate project governance cost"""
        try:
            # Find source files
            c_files = list(self.project_root.glob("**/*.c"))
            h_files = list(self.project_root.glob("**/*.h"))
            py_files = list(self.project_root.glob("**/*.py"))
            
            total_files = len(c_files) + len(h_files) + len(py_files)
            
            # Basic cost calculation based on file count and complexity
            base_cost = total_files * 0.01
            
            # Add complexity factors
            complexity_cost = 0.0
            for file_path in c_files + h_files:
                try:
                    content = file_path.read_text(encoding='utf-8', errors='ignore')
                    lines = len(content.splitlines())
                    includes = content.count('#include')
                    complexity_cost += (lines * 0.001) + (includes * 0.01)
                except:
                    continue
            
            return min(base_cost + complexity_cost, 1.0)
            
        except Exception as e:
            logger.warning(f"Cost calculation error: {e}")
            return 0.5  # Default moderate cost
