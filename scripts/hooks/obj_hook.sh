#!/bin/bash
# obj_hook.sh - Object file generation hook for QA validation
# Ensures successful compilation tracking with -MMD

set -euo pipefail

OBJ_FILE="$1"
SRC_FILE="$2"
BUILD_MODE="${3:-debug}"

# Extract module information
MODULE=$(basename "${SRC_FILE}" .c)
MODULE_DIR=$(dirname "${SRC_FILE}")

# QA validation log
QA_LOG="${BUILD_BASE}/qa/reports/compilation_$(date +%Y%m%d).log"
mkdir -p "$(dirname "${QA_LOG}")"

# Log successful compilation
echo "[$(date +%Y-%m-%d\ %H:%M:%S)] SUCCESS: ${MODULE} (${BUILD_MODE})" >> "${QA_LOG}"

# Verify dependency file generation
DEP_FILE="${OBJ_FILE%.o}.d"
if [[ -f "${DEP_FILE}" ]]; then
    echo "  Dependencies tracked: $(wc -l < "${DEP_FILE}") includes" >> "${QA_LOG}"
else
    echo "  WARNING: No dependency file generated" >> "${QA_LOG}"
fi

# Check for code quality markers
if command -v cppcheck &> /dev/null; then
    cppcheck --quiet --enable=warning,style \
             --suppress=missingIncludeSystem \
             "${SRC_FILE}" 2>&1 | tee -a "${QA_LOG}"
fi

# Update module compilation matrix
MATRIX_FILE="${BUILD_BASE}/qa/reports/compilation_matrix.json"
python3 << EOF
import json
import os
from datetime import datetime

matrix_file = '${MATRIX_FILE}'
module = '${MODULE}'
mode = '${BUILD_MODE}'

# Load or create matrix
if os.path.exists(matrix_file):
    with open(matrix_file, 'r') as f:
        matrix = json.load(f)
else:
    matrix = {'modules': {}}

# Update module entry
if module not in matrix['modules']:
    matrix['modules'][module] = {}

matrix['modules'][module][mode] = {
    'compiled': True,
    'timestamp': datetime.utcnow().isoformat(),
    'object': '${OBJ_FILE}',
    'source': '${SRC_FILE}'
}

# Save matrix
os.makedirs(os.path.dirname(matrix_file), exist_ok=True)
with open(matrix_file, 'w') as f:
    json.dump(matrix, f, indent=2)
EOF
