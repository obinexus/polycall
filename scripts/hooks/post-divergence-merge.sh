#!/bin/bash
# scripts/hooks/post-divergence-merge.sh
# This script handles merging of divergent branches back to main after QA validation

set -e

BRANCH_NAME=$(git rev-parse --abbrev-ref HEAD)
TARGET_BRANCH=${1:-"dev-main"}  # Default target is dev-main
GRADE_FILE="build/metadata/build-grade.txt"

# Verify we're on a divergent branch
if [[ ! $BRANCH_NAME =~ ^diverge/ ]]; then
  echo "[polycall:MERGE] ❌ Not on a divergent branch. Current: $BRANCH_NAME"
  exit 1
fi

# Run QA validation to ensure fixes are effective
if [ ! -f "$GRADE_FILE" ]; then
  echo "[polycall:MERGE] Running QA validation..."
  ./scripts/qa_suite_run.sh
fi

# Read current fault grade
FAULT_GRADE=$(cat "$GRADE_FILE")

if [ $FAULT_GRADE -ge 7 ]; then
  echo "[polycall:MERGE] ❌ Fault grade still in danger zone: $FAULT_GRADE"
  echo "[polycall:MERGE] Fix the issues before attempting to merge"
  exit 1
fi

echo "[polycall:MERGE] ✅ Fault grade acceptable: $FAULT_GRADE"

# Capture changes to apply to target branch
PATCH_FILE=$(mktemp /tmp/polycall-merge-XXXXXX.patch)
git format-patch --stdout $(git merge-base HEAD $TARGET_BRANCH)..HEAD > $PATCH_FILE

echo "[polycall:MERGE] Created patch file: $PATCH_FILE"

# Switch to target branch
git checkout $TARGET_BRANCH
if [ $? -ne 0 ]; then
  echo "[polycall:MERGE] ❌ Failed to switch to $TARGET_BRANCH"
  exit 1
fi

# Apply changes
echo "[polycall:MERGE] Applying changes to $TARGET_BRANCH..."
git am $PATCH_FILE

if [ $? -ne 0 ]; then
  echo "[polycall:MERGE] ❌ Merge conflict detected. Aborting merge."
  git am --abort
  echo "[polycall:MERGE] Original patch saved at: $PATCH_FILE"
  echo "[polycall:MERGE] Resolve conflicts manually:"
  echo "  1. git checkout $TARGET_BRANCH"
  echo "  2. git checkout $BRANCH_NAME -- <conflicting-files>"
  echo "  3. Resolve conflicts and commit"
  exit 1
fi

echo "[polycall:MERGE] ✅ Successfully merged changes from $BRANCH_NAME to $TARGET_BRANCH"
echo "[polycall:MERGE] Running QA validation on $TARGET_BRANCH..."

# Run QA validation on target branch
./scripts/qa_suite_run.sh

# Read target branch fault grade
FAULT_GRADE=$(cat "$GRADE_FILE")

if [ $FAULT_GRADE -ge 7 ]; then
  echo "[polycall:MERGE] ⚠️ Warning: Target branch now has fault grade: $FAULT_GRADE"
  echo "[polycall:MERGE] Consider reverting the merge: git reset --hard HEAD^"
else
  echo "[polycall:MERGE] ✅ Target branch QA validation passed with grade: $FAULT_GRADE"
  echo "[polycall:MERGE] Safe to push changes to remote"
fi

# Clean up
rm $PATCH_FILE
