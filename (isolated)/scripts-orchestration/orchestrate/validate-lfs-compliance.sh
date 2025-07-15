#!/bin/bash
# LibPolyCall Git LFS Governance Validation
# OBINexus Computing - Sinphasé Framework Integration

set -euo pipefail

echo "🔍 LibPolyCall Git LFS Governance Validation"
echo "============================================="

# Check LFS installation
if ! command -v git lfs &> /dev/null; then
    echo "❌ Git LFS not installed"
    exit 1
fi

# Verify LFS initialization
if [[ ! -f .git/hooks/pre-push ]]; then
    echo "❌ Git LFS hooks not installed"
    exit 1
fi

# Check .gitattributes existence
if [[ ! -f .gitattributes ]]; then
    echo "❌ .gitattributes file missing"
    exit 1
fi

# Validate LFS tracking patterns
echo "📊 LFS Tracking Status:"
git lfs track

# Check for untracked large files
echo "🔍 Scanning for untracked large files..."
LARGE_FILES=$(find . -type f -size +1M -not -path './.git/*' -not -path './scripts/ad-hoc/*')
if [[ -n "$LARGE_FILES" ]]; then
    echo "⚠️  Large files found outside LFS:"
    echo "$LARGE_FILES"
    echo "Consider adding to LFS tracking"
fi

# LFS file count
LFS_COUNT=$(git lfs ls-files | wc -l)
echo "📈 LFS-tracked files: $LFS_COUNT"

echo "✅ Git LFS governance validation complete"
