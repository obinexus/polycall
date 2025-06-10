#!/bin/bash
# Install Sinphasé Git Hooks
# Location: scripts/setup/install-git-hooks.sh

set -e

PROJECT_ROOT="$(git rev-parse --show-toplevel)"
HOOKS_DIR="$PROJECT_ROOT/.git/hooks"
SHARED_HOOKS_DIR="$PROJECT_ROOT/.githooks"

echo "🔧 Installing Sinphasé Git Hooks..."

# Check if shared hooks exist
if [ ! -d "$SHARED_HOOKS_DIR" ]; then
    echo "❌ Shared hooks directory not found: $SHARED_HOOKS_DIR"
    echo "   Create the .githooks directory with pre-commit and pre-push scripts"
    exit 1
fi

# Install pre-commit hook
if [ -f "$SHARED_HOOKS_DIR/pre-commit" ]; then
    cp "$SHARED_HOOKS_DIR/pre-commit" "$HOOKS_DIR/pre-commit"
    chmod +x "$HOOKS_DIR/pre-commit"
    echo "✅ Pre-commit hook installed"
else
    echo "⚠️  Pre-commit hook not found in $SHARED_HOOKS_DIR"
fi

# Install pre-push hook
if [ -f "$SHARED_HOOKS_DIR/pre-push" ]; then
    cp "$SHARED_HOOKS_DIR/pre-push" "$HOOKS_DIR/pre-push"
    chmod +x "$HOOKS_DIR/pre-push"
    echo "✅ Pre-push hook installed"
else
    echo "⚠️  Pre-push hook not found in $SHARED_HOOKS_DIR"
fi

# Configure git to use shared hooks directory (optional)
git config core.hooksPath "$SHARED_HOOKS_DIR"
echo "✅ Git configured to use shared hooks directory"

echo ""
echo "🎉 Sinphasé Git Hooks installation complete!"
echo ""
echo "📋 Installed hooks:"
echo "  • Pre-commit: Blocks commits with governance violations"
echo "  • Pre-push: Blocks pushes with governance violations"
echo ""
echo "🔧 To disable hooks temporarily:"
echo "  git commit --no-verify"
echo "  git push --no-verify"
