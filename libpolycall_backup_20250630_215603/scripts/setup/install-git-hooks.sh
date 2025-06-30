#!/bin/bash
# Install Sinphasé Git Hooks

echo "🔧 Installing Sinphasé Git hooks..."

# Configure git to use our hooks directory
git config core.hooksPath .githooks

echo "✅ Git hooks installed successfully"
echo "📋 Hooks configured in: .githooks/"
echo "🔍 Pre-commit: Immediate complexity validation"
echo "🚀 Pre-push: Comprehensive governance check"
