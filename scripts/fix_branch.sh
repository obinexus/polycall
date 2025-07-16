#!/bin/bash
# scripts/fix_branch.sh

GRADE_FILE="build/metadata/build-grade.txt"

if [ ! -f "$GRADE_FILE" ]; then
  echo "[polycall:QA] ❌ Cannot find build-grade.txt, running QA suite..."
  ./scripts/qa_suite_run.sh
  
  if [ ! -f "$GRADE_FILE" ]; then
    echo "[polycall:QA] ❌ Failed to generate build grade, cannot proceed." >&2
    exit 1
  fi
fi

FAULT_GRADE=$(cat "$GRADE_FILE")

if [ $FAULT_GRADE -lt 7 ]; then
  echo "[polycall:QA] 🟢 Fault grade ${FAULT_GRADE} is acceptable, no divergence needed." >&2
  exit 0
fi

# Create timestamped branch name
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M")
BRANCH_NAME="diverge/${TIMESTAMP}"

# Create and switch to divergent branch
git checkout -b ${BRANCH_NAME}

if [ $? -ne 0 ]; then
  echo "[polycall:QA] ❌ Failed to create divergent branch. Resolve git issues first." >&2
  exit 1
fi

# Output warning to stderr for visibility
echo "🔴 QA Grade BLOCKED (${FAULT_GRADE}). Diverged to: ${BRANCH_NAME}" >&2

# Stage any changes
git add -A

echo "[polycall:QA] ✅ Divergence complete. You're now on branch: ${BRANCH_NAME}"
echo "[polycall:QA] Fix the issues, then commit and push to continue."

exit 0
