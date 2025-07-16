#!/bin/bash
# scripts/cli/polycall-branch.sh

ACTION=$1
TARGET=${2:-"dev-main"}

case "$ACTION" in
  create)
    ./scripts/fix_branch.sh
    ;;
  merge)
    ./scripts/hooks/post-divergence-merge.sh $TARGET
    ;;
  status)
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    if [[ $BRANCH =~ ^diverge/ ]]; then
      echo "[polycall:BRANCH] Currently on divergent branch: $BRANCH"
      if [ -f "build/metadata/build-grade.txt" ]; then
        echo "[polycall:BRANCH] Current fault grade: $(cat build/metadata/build-grade.txt)"
      else
        echo "[polycall:BRANCH] No fault grade file found"
      fi
    else
      echo "[polycall:BRANCH] Not on a divergent branch. Current: $BRANCH"
    fi
    ;;
  list)
    echo "[polycall:BRANCH] Active divergent branches:"
    git branch | grep "^[[:space:]]*diverge/" | sed 's/^[[:space:]]*/  /'
    ;;
  *)
    echo "Usage: polycall-branch.sh <action> [target-branch]"
    echo "Actions:"
    echo "  create    - Create a new divergent branch"
    echo "  merge     - Merge current divergent branch to target"
    echo "  status    - Show current branch status"
    echo "  list      - List all divergent branches"
    echo ""
    echo "Example: polycall-branch.sh merge dev-main"
    exit 1
    ;;
esac
