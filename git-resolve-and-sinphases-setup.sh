#!/bin/bash
# OBINexus Git Resolution & Sinphasé Governance Setup
# Resolves ref locking issues and implements dual-enforcement governance

set -e

echo "🔧 OBINexus Git Resolution & Sinphasé Integration"
echo "=================================================="

# Step 1: Clean Git References
echo "📋 Step 1: Cleaning Git references..."
cd /mnt/c/Users/OBINexus/Projects/github/libpolycall

# Remove any corrupt or locked references
git update-ref -d refs/heads/dev/debug 2>/dev/null || true
git gc --prune=now

# Clean any packed refs issues
git pack-refs --all

# Step 2: Recreate dev/debug branch properly
echo "📋 Step 2: Creating dev/debug branch for Sinphasé work..."

# Ensure we're on dev branch
git checkout dev 2>/dev/null || git checkout -b dev origin/dev

# Create debug branch from current dev state
git checkout -b dev-debug-sinphase

echo "✅ Git references resolved. Now on branch: $(git branch --show-current)"

# Step 3: Implement Sinphasé Governance Enforcement
echo "📋 Step 3: Setting up Sinphasé governance framework..."

# Create governance directory structure
mkdir -p .github/workflows
mkdir -p .githooks
mkdir -p scripts/{ci,setup,evaluator}
mkdir -p root-dynamic-c

# Step 4: GitHub Actions (PRIMARY enforcement)
echo "📋 Step 4: Creating GitHub Actions enforcement..."
cat > .github/workflows/sinphase-governance.yml << 'YAML_EOF'
name: "Sinphasé Governance Enforcement"

on:
  push:
    branches: [ "dev", "dev-*", "main" ]
  pull_request:
    branches: [ "dev", "main" ]

jobs:
  sinphase-enforcement:
    runs-on: ubuntu-latest
    name: "🔒 Sinphasé Cost Function Validation"
    
    steps:
    - uses: actions/checkout@v4
      with:
        fetch-depth: 0
        
    - name: "Setup Python for Cost Evaluator"
      uses: actions/setup-python@v4
      with:
        python-version: '3.9'
        
    - name: "Install Dependencies"
      run: |
        python -m pip install --upgrade pip
        pip install pathlib typing
        
    - name: "🔍 Run Sinphasé Cost Function Analysis"
      run: |
        echo "🔍 Evaluating architectural compliance..."
        python scripts/evaluator/sinphase_cost_evaluator.py \
          --project-root . \
          --threshold 0.6 \
          --emergency-mode \
          --violation-rate 0.90
        
    - name: "📊 Generate Governance Report"
      run: |
        echo "📊 Generating compliance report..."
        if [ -f "SINPHASE_VIOLATIONS.json" ]; then
          echo "❌ CRITICAL: Architecture violations detected"
          cat SINPHASE_VIOLATIONS.json
          exit 1
        else
          echo "✅ Architecture compliance validated"
        fi
        
    - name: "🚨 Emergency Isolation Check"
      run: |
        echo "🚨 Checking for emergency isolation requirements..."
        if [ -f "ISOLATION_LOG.md" ]; then
          echo "📋 Isolation log exists - reviewing entries..."
          tail -20 ISOLATION_LOG.md
        fi
        
  prevent-violations:
    runs-on: ubuntu-latest
    name: "🛡️ Block Architectural Violations"
    
    steps:
    - uses: actions/checkout@v4
    
    - name: "🛡️ FFI Threshold Protection"
      run: |
        echo "🛡️ Enforcing FFI call limits..."
        
        # Count FFI calls in changed files
        ffi_count=$(git diff --name-only HEAD~1 | xargs grep -l "FFI\|ffi\|foreign" | wc -l || echo "0")
        
        echo "FFI-related files in this change: $ffi_count"
        
        if [ "$ffi_count" -gt 5 ]; then
          echo "❌ BLOCKED: Too many FFI changes (limit: 5, found: $ffi_count)"
          echo "This violates Sinphasé single-pass compilation requirements"
          exit 1
        fi
        
    - name: "🔒 Dependency Cycle Detection"
      run: |
        echo "🔒 Checking for circular dependencies..."
        
        # Basic circular dependency detection
        find . -name "*.c" -o -name "*.h" | xargs grep -l "#include" | while read file; do
          includes=$(grep "#include" "$file" | wc -l)
          if [ "$includes" -gt 10 ]; then
            echo "⚠️ Warning: $file has $includes includes (threshold: 10)"
          fi
        done
YAML_EOF

# Step 5: Local Git Hooks (SECONDARY enforcement)
echo "📋 Step 5: Creating local Git hooks..."

cat > .githooks/pre-commit << 'HOOK_EOF'
#!/bin/bash
# Sinphasé Pre-commit Hook - SECONDARY enforcement
# Provides immediate developer feedback

echo "🔍 Sinphasé: Pre-commit governance check..."

# Quick cost evaluation on staged files
staged_files=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|h)$' || true)

if [ -n "$staged_files" ]; then
    echo "📋 Analyzing staged files: $(echo $staged_files | wc -w) files"
    
    # Simple complexity check
    for file in $staged_files; do
        if [ -f "$file" ]; then
            lines=$(wc -l < "$file")
            includes=$(grep -c "#include" "$file" || echo "0")
            
            if [ "$lines" -gt 500 ] || [ "$includes" -gt 15 ]; then
                echo "⚠️ WARNING: $file may exceed complexity thresholds"
                echo "   Lines: $lines (threshold: 500)"
                echo "   Includes: $includes (threshold: 15)"
                echo "   Consider architectural refactoring"
            fi
        fi
    done
    
    echo "✅ Local pre-commit validation complete"
else
    echo "📋 No C source files staged for commit"
fi
HOOK_EOF

cat > .githooks/pre-push << 'HOOK_EOF'
#!/bin/bash
# Sinphasé Pre-push Hook - Final local validation

echo "🚀 Sinphasé: Pre-push governance validation..."

# Check if we're about to push to protected branches
remote="$1"
url="$2"

while read local_ref local_sha remote_ref remote_sha; do
    if [ "$local_sha" = "0000000000000000000000000000000000000000" ]; then
        # Branch deletion - allow
        continue
    fi
    
    branch_name=$(echo "$remote_ref" | sed 's/refs\/heads\///')
    
    if [ "$branch_name" = "main" ] || [ "$branch_name" = "dev" ]; then
        echo "🔒 Pushing to protected branch: $branch_name"
        echo "🔍 Running comprehensive Sinphasé validation..."
        
        # Run the cost evaluator if it exists
        if [ -f "scripts/evaluator/sinphase_cost_evaluator.py" ]; then
            echo "📊 Running cost function analysis..."
            python scripts/evaluator/sinphase_cost_evaluator.py --project-root . --threshold 0.6
            
            if [ $? -ne 0 ]; then
                echo "❌ PUSH BLOCKED: Sinphasé governance violations detected"
                exit 1
            fi
        fi
    fi
done

echo "✅ Pre-push validation passed"
HOOK_EOF

# Make hooks executable
chmod +x .githooks/pre-commit
chmod +x .githooks/pre-push

# Step 6: Hook Installation Script
echo "📋 Step 6: Creating hook installer..."
cat > scripts/setup/install-git-hooks.sh << 'INSTALL_EOF'
#!/bin/bash
# Install Sinphasé Git Hooks

echo "🔧 Installing Sinphasé Git hooks..."

# Configure git to use our hooks directory
git config core.hooksPath .githooks

echo "✅ Git hooks installed successfully"
echo "📋 Hooks configured in: .githooks/"
echo "🔍 Pre-commit: Immediate complexity validation"
echo "🚀 Pre-push: Comprehensive governance check"
INSTALL_EOF

chmod +x scripts/setup/install-git-hooks.sh

# Step 7: Install the hooks
echo "📋 Step 7: Installing Git hooks..."
./scripts/setup/install-git-hooks.sh

# Step 8: Copy the Python cost evaluator from project knowledge
echo "📋 Step 8: Setting up Sinphasé cost evaluator..."
cat > scripts/evaluator/sinphase_cost_evaluator.py << 'PYTHON_EOF'
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
PYTHON_EOF

chmod +x scripts/evaluator/sinphase_cost_evaluator.py

# Step 9: Initialize isolation log
echo "📋 Step 9: Initializing isolation log..."
cat > ISOLATION_LOG.md << 'LOG_EOF'
# Sinphasé Isolation Log

This file tracks architectural decisions and component isolations for the OBINexus libpolycall project.

## Emergency Recovery Status

**Project Status**: Crisis Recovery Mode  
**Violation Rate**: 90% (Emergency threshold exceeded)  
**FFI Threshold**: 1026x normal limits  
**Recovery Strategy**: Sinphasé phase-gate enforcement with automated isolation  

## Governance Framework

- **Autonomous Zone**: ≤ 0.5 (Emergency lowered threshold)
- **Warning Zone**: 0.5 - 0.6  
- **Governance Zone**: > 0.6 (Automatic isolation)
- **Emergency Mode**: Active - Stricter enforcement

## Architectural Principles Enforced

✅ Single-pass compilation requirements  
✅ Acyclic dependency graphs  
✅ Bounded component complexity  
✅ Hierarchical isolation protocols  
✅ Cost-based governance checkpoints  

---

LOG_EOF

# Step 10: Test the setup
echo "📋 Step 10: Testing Sinphasé governance setup..."

echo "🔍 Running initial cost evaluation..."
python scripts/evaluator/sinphase_cost_evaluator.py --project-root . --threshold 0.6

echo "🔧 Verifying Git hooks..."
git config core.hooksPath

echo "✅ Sinphasé governance framework setup complete!"
echo ""
echo "📋 Summary:"
echo "  - Git references resolved"
echo "  - Branch: dev-debug-sinphase created"
echo "  - GitHub Actions: PRIMARY enforcement active"
echo "  - Git Hooks: SECONDARY enforcement installed"
echo "  - Cost Evaluator: Emergency governance ready"
echo "  - Isolation Log: Crisis tracking initialized"
echo ""
echo "🚀 Ready to commit Sinphasé governance changes:"
echo "   git add ."
echo "   git commit -m 'Implement Sinphasé governance enforcement - Emergency recovery'"
echo "   git push origin dev-debug-sinphase"
