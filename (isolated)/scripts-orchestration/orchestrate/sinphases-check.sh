#!/bin/bash
# Sinphasé CI/CD Helper Script
# Location: scripts/ci/sinphase-check.sh

set -e

PROJECT_ROOT="${1:-.}"
FAIL_ON_VIOLATIONS="${2:-true}"

cd "$PROJECT_ROOT"

echo "🔍 Running Sinphasé governance evaluation..."

# Run evaluation
python3 scripts/evaluator/sinphase_cost_evaluator.py --project-root . --report

# Check results
if [ -f "SINPHASE_GOVERNANCE_REPORT.md" ]; then
    VIOLATIONS=$(grep -c "GOVERNANCE ZONE" SINPHASE_GOVERNANCE_REPORT.md || echo "0")
    WARNING_COUNT=$(grep -c "WARNING ZONE" SINPHASE_GOVERNANCE_REPORT.md || echo "0")
    AUTONOMOUS_COUNT=$(grep -c "AUTONOMOUS ZONE" SINPHASE_GOVERNANCE_REPORT.md || echo "0")
    
    echo ""
    echo "📊 Sinphasé Governance Results:"
    echo "  🟢 Autonomous Zone: $AUTONOMOUS_COUNT components"
    echo "  🟡 Warning Zone: $WARNING_COUNT components"
    echo "  🔴 Governance Zone: $VIOLATIONS components"
    echo ""
    
    if [ "$VIOLATIONS" -gt 0 ]; then
        echo "🚨 GOVERNANCE VIOLATIONS DETECTED:"
        grep -A 1 "GOVERNANCE ZONE" SINPHASE_GOVERNANCE_REPORT.md
        echo ""
        
        if [ "$FAIL_ON_VIOLATIONS" = "true" ]; then
            echo "❌ CI/CD pipeline failing due to violations"
            exit 1
        else
            echo "⚠️  Violations detected but not failing pipeline"
        fi
    else
        echo "✅ All components comply with Sinphasé governance"
    fi
else
    echo "❌ Governance report not generated"
    exit 1
fi
