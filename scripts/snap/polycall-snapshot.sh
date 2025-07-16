#!/bin/bash
# scripts/snap/polycall-snapshot.sh

# Configuration
SNAPSHOT_BASE="/root/logs/polycall_snapshot"
SOURCE_DIR="$HOME/obinexus/pkg/polycall"
RETENTION_DAYS=14

# Create timestamp
TIMESTAMP=$(date +"%Y%m%d-%H%M%S")
SNAPSHOT_DIR="${SNAPSHOT_BASE}/${TIMESTAMP}"
mkdir -p "${SNAPSHOT_DIR}"

# Capture build artifacts
cp -r ${SOURCE_DIR}/{build,logs,test_error_*.log} "${SNAPSHOT_DIR}/" 2>/dev/null
cp -r ${SOURCE_DIR}/qa_lib "${SNAPSHOT_DIR}/" 2>/dev/null

# Capture QA reports
cp /tmp/polycall_qa_report.json "${SNAPSHOT_DIR}/" 2>/dev/null

# Capture git state
(cd ${SOURCE_DIR} && git log -1 --format="%H %an %ad %s" > "${SNAPSHOT_DIR}/git_state.txt")
(cd ${SOURCE_DIR} && git status --porcelain > "${SNAPSHOT_DIR}/git_status.txt")

# Generate snapshot manifest
cat > "${SNAPSHOT_DIR}/manifest.json" << EOF
{
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "hostname": "$(hostname)",
  "user": "$(whoami)",
  "build_grade": "$(cat ${SOURCE_DIR}/build/metadata/build-grade.txt 2>/dev/null || echo "N/A")",
  "git_commit": "$(cd ${SOURCE_DIR} && git rev-parse HEAD 2>/dev/null || echo "N/A")",
  "git_branch": "$(cd ${SOURCE_DIR} && git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "N/A")"
}
EOF

# Generate telemetry summary
TELEMETRY_FILE="${SNAPSHOT_DIR}/telemetry.txt"
echo "POLYCALL TELEMETRY SNAPSHOT: ${TIMESTAMP}" > ${TELEMETRY_FILE}
echo "=======================================" >> ${TELEMETRY_FILE}
echo "Build Grade: $(cat ${SOURCE_DIR}/build/metadata/build-grade.txt 2>/dev/null || echo "N/A")" >> ${TELEMETRY_FILE}
echo "Error Log Count: $(ls ${SOURCE_DIR}/test_error_*.log 2>/dev/null | wc -l)" >> ${TELEMETRY_FILE}
echo "Top Errors:" >> ${TELEMETRY_FILE}
grep -h "ERROR" ${SOURCE_DIR}/test_error_*.log 2>/dev/null | sort | uniq -c | sort -nr | head -5 >> ${TELEMETRY_FILE}

# Clean up old snapshots
find ${SNAPSHOT_BASE} -type d -mtime +${RETENTION_DAYS} -exec rm -rf {} \; 2>/dev/null

echo "[cron:snapshot] ✅ Snapshot created at ${SNAPSHOT_DIR}"
echo "[cron:snapshot] Telemetry data captured in ${TELEMETRY_FILE}"

# Create symlink to latest snapshot for convenience
ln -sf "${SNAPSHOT_DIR}" "${SNAPSHOT_BASE}/latest"

exit 0
