#!/bin/bash
# Tracer Root Script - Execution Monitoring and Compliance Tracking
# Sinphasé Governance - OBINexus Aegis Project Phase 2

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TRACE_DIR="$PROJECT_ROOT/logs/trace"
TRACE_DB="$TRACE_DIR/trace.db"

# Initialize trace database
init_trace_db() {
    echo "Initializing trace database..."
    
    # Create SQLite database for trace storage
    cat > "$TRACE_DIR/init.sql" << 'EOF'
CREATE TABLE IF NOT EXISTS executions (
    id TEXT PRIMARY KEY,
    script_path TEXT NOT NULL,
    script_type TEXT,
    start_time TIMESTAMP,
    end_time TIMESTAMP,
    exit_code INTEGER,
    duration_ms INTEGER,
    user TEXT,
    hostname TEXT,
    compliance_level TEXT,
    policy_violations INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS trace_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    execution_id TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    event_type TEXT,
    event_data TEXT,
    severity TEXT,
    FOREIGN KEY (execution_id) REFERENCES executions(id)
);

CREATE TABLE IF NOT EXISTS policy_violations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    execution_id TEXT,
    policy_name TEXT,
    violation_type TEXT,
    description TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (execution_id) REFERENCES executions(id)
);

CREATE TABLE IF NOT EXISTS metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    execution_id TEXT,
    metric_name TEXT,
    metric_value REAL,
    unit TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (execution_id) REFERENCES executions(id)
);

CREATE INDEX IF NOT EXISTS idx_executions_time ON executions(start_time);
CREATE INDEX IF NOT EXISTS idx_trace_events_exec ON trace_events(execution_id);
CREATE INDEX IF NOT EXISTS idx_violations_exec ON policy_violations(execution_id);
EOF
    
    sqlite3 "$TRACE_DB" < "$TRACE_DIR/init.sql"
    echo "Trace database initialized: $TRACE_DB"
}

# Start trace session
start_trace() {
    local exec_id="$1"
    local script_path="$2"
    local compliance_level="${3:-standard}"
    
    local script_type="unknown"
    if [[ "$script_path" == *.py ]]; then
        script_type="python"
    elif [[ "$script_path" == *.sh ]]; then
        script_type="shell"
    elif [[ "$script_path" == *cmake* ]]; then
        script_type="cmake"
    fi
    
    sqlite3 "$TRACE_DB" << EOF
INSERT INTO executions (
    id, script_path, script_type, start_time, 
    user, hostname, compliance_level
) VALUES (
    '$exec_id',
    '$script_path',
    '$script_type',
    datetime('now'),
    '$(whoami)',
    '$(hostname)',
    '$compliance_level'
);
EOF
    
    echo "$exec_id"
}

# End trace session
end_trace() {
    local exec_id="$1"
    local exit_code="$2"
    
    sqlite3 "$TRACE_DB" << EOF
UPDATE executions 
SET end_time = datetime('now'),
    exit_code = $exit_code,
    duration_ms = (
        SELECT (julianday(datetime('now')) - julianday(start_time)) * 86400000
        FROM executions WHERE id = '$exec_id'
    )
WHERE id = '$exec_id';
EOF
}

# Log trace event
log_event() {
    local exec_id="$1"
    local event_type="$2"
    local event_data="$3"
    local severity="${4:-info}"
    
    sqlite3 "$TRACE_DB" << EOF
INSERT INTO trace_events (execution_id, event_type, event_data, severity)
VALUES ('$exec_id', '$event_type', '$event_data', '$severity');
EOF
}

# Log policy violation
log_violation() {
    local exec_id="$1"
    local policy_name="$2"
    local violation_type="$3"
    local description="$4"
    
    sqlite3 "$TRACE_DB" << EOF
INSERT INTO policy_violations (execution_id, policy_name, violation_type, description)
VALUES ('$exec_id', '$policy_name', '$violation_type', '$description');

UPDATE executions 
SET policy_violations = policy_violations + 1
WHERE id = '$exec_id';
EOF
}

# Log metric
log_metric() {
    local exec_id="$1"
    local metric_name="$2"
    local metric_value="$3"
    local unit="${4:-count}"
    
    sqlite3 "$TRACE_DB" << EOF
INSERT INTO metrics (execution_id, metric_name, metric_value, unit)
VALUES ('$exec_id', '$metric_name', $metric_value, '$unit');
EOF
}

# Generate trace report
generate_report() {
    local exec_id="$1"
    
    echo "=== Execution Trace Report ==="
    echo "Execution ID: $exec_id"
    echo
    
    # Execution summary
    sqlite3 -header -column "$TRACE_DB" << EOF
SELECT 
    script_path,
    script_type,
    start_time,
    end_time,
    duration_ms,
    exit_code,
    compliance_level,
    policy_violations
FROM executions
WHERE id = '$exec_id';
EOF
    
    echo
    echo "=== Trace Events ==="
    sqlite3 -header -column "$TRACE_DB" << EOF
SELECT 
    timestamp,
    event_type,
    event_data,
    severity
FROM trace_events
WHERE execution_id = '$exec_id'
ORDER BY timestamp;
EOF
    
    # Check for violations
    local violation_count=$(sqlite3 "$TRACE_DB" "SELECT COUNT(*) FROM policy_violations WHERE execution_id = '$exec_id';")
    
    if [ "$violation_count" -gt 0 ]; then
        echo
        echo "=== Policy Violations ==="
        sqlite3 -header -column "$TRACE_DB" << EOF
SELECT 
    policy_name,
    violation_type,
    description,
    timestamp
FROM policy_violations
WHERE execution_id = '$exec_id'
ORDER BY timestamp;
EOF
    fi
    
    echo
    echo "=== Metrics ==="
    sqlite3 -header -column "$TRACE_DB" << EOF
SELECT 
    metric_name,
    metric_value,
    unit,
    timestamp
FROM metrics
WHERE execution_id = '$exec_id'
ORDER BY timestamp;
EOF
}

# Real-time monitoring
monitor_executions() {
    echo "Starting real-time execution monitoring..."
    echo "Press Ctrl+C to stop"
    
    while true; do
        clear
        echo "=== Active Executions ==="
        sqlite3 -header -column "$TRACE_DB" << 'EOF'
SELECT 
    id,
    script_path,
    start_time,
    CASE 
        WHEN end_time IS NULL THEN 'RUNNING'
        ELSE 'COMPLETED'
    END as status,
    compliance_level,
    policy_violations
FROM executions
WHERE datetime(start_time) > datetime('now', '-1 hour')
ORDER BY start_time DESC
LIMIT 20;
EOF
        
        echo
        echo "=== Recent Policy Violations ==="
        sqlite3 -header -column "$TRACE_DB" << 'EOF'
SELECT 
    e.script_path,
    v.policy_name,
    v.violation_type,
    v.timestamp
FROM policy_violations v
JOIN executions e ON v.execution_id = e.id
WHERE datetime(v.timestamp) > datetime('now', '-1 hour')
ORDER BY v.timestamp DESC
LIMIT 10;
EOF
        
        sleep 5
    done
}

# Compliance statistics
compliance_stats() {
    echo "=== Compliance Statistics ==="
    
    sqlite3 -header -column "$TRACE_DB" << 'EOF'
SELECT 
    compliance_level,
    COUNT(*) as total_executions,
    SUM(CASE WHEN exit_code = 0 THEN 1 ELSE 0 END) as successful,
    SUM(CASE WHEN exit_code != 0 THEN 1 ELSE 0 END) as failed,
    SUM(policy_violations) as total_violations,
    ROUND(AVG(duration_ms), 2) as avg_duration_ms
FROM executions
GROUP BY compliance_level;
EOF
    
    echo
    echo "=== Most Common Violations ==="
    sqlite3 -header -column "$TRACE_DB" << 'EOF'
SELECT 
    policy_name,
    violation_type,
    COUNT(*) as occurrence_count
FROM policy_violations
GROUP BY policy_name, violation_type
ORDER BY occurrence_count DESC
LIMIT 10;
EOF
}

# Export traces
export_traces() {
    local output_dir="${1:-$TRACE_DIR/exports}"
    local timestamp=$(date +%Y%m%d_%H%M%S)
    
    mkdir -p "$output_dir"
    
    echo "Exporting traces to $output_dir..."
    
    # Export as CSV
    sqlite3 -header -csv "$TRACE_DB" "SELECT * FROM executions;" > "$output_dir/executions_$timestamp.csv"
    sqlite3 -header -csv "$TRACE_DB" "SELECT * FROM trace_events;" > "$output_dir/trace_events_$timestamp.csv"
    sqlite3 -header -csv "$TRACE_DB" "SELECT * FROM policy_violations;" > "$output_dir/violations_$timestamp.csv"
    sqlite3 -header -csv "$TRACE_DB" "SELECT * FROM metrics;" > "$output_dir/metrics_$timestamp.csv"
    
    # Create summary report
    {
        echo "# Trace Export Summary"
        echo "Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
        echo
        compliance_stats
    } > "$output_dir/summary_$timestamp.md"
    
    echo "Export complete: $output_dir"
}

# Clean old traces
clean_traces() {
    local days="${1:-30}"
    
    echo "Cleaning traces older than $days days..."
    
    local deleted=$(sqlite3 "$TRACE_DB" << EOF
DELETE FROM trace_events 
WHERE execution_id IN (
    SELECT id FROM executions 
    WHERE datetime(start_time) < datetime('now', '-$days days')
);

DELETE FROM policy_violations 
WHERE execution_id IN (
    SELECT id FROM executions 
    WHERE datetime(start_time) < datetime('now', '-$days days')
);

DELETE FROM metrics 
WHERE execution_id IN (
    SELECT id FROM executions 
    WHERE datetime(start_time) < datetime('now', '-$days days')
);

DELETE FROM executions 
WHERE datetime(start_time) < datetime('now', '-$days days');

SELECT changes();
EOF
)
    
    echo "Deleted $deleted old trace records"
    
    # Vacuum database
    sqlite3 "$TRACE_DB" "VACUUM;"
    echo "Database optimized"
}

# Main command handler
main() {
    local command="${1:-help}"
    shift || true
    
    case "$command" in
        init)
            mkdir -p "$TRACE_DIR"/{execution,compliance,audit,exports}
            init_trace_db
            ;;
        start)
            start_trace "$@"
            ;;
        end)
            end_trace "$@"
            ;;
        event)
            log_event "$@"
            ;;
        violation)
            log_violation "$@"
            ;;
        metric)
            log_metric "$@"
            ;;
        report)
            generate_report "$@"
            ;;
        monitor)
            monitor_executions
            ;;
        stats)
            compliance_stats
            ;;
        export)
            export_traces "$@"
            ;;
        clean)
            clean_traces "$@"
            ;;
        help|*)
            echo "Tracer Root System - Execution Monitoring"
            echo ""
            echo "Usage: $0 <command> [args]"
            echo ""
            echo "Commands:"
            echo "  init              Initialize trace database"
            echo "  start <id> <script> [level]  Start trace session"
            echo "  end <id> <code>   End trace session"
            echo "  event <id> <type> <data> [severity]  Log event"
            echo "  violation <id> <policy> <type> <desc> Log violation"
            echo "  metric <id> <name> <value> [unit]    Log metric"
            echo "  report <id>       Generate execution report"
            echo "  monitor           Real-time monitoring"
            echo "  stats             Show compliance statistics"
            echo "  export [dir]      Export traces"
            echo "  clean [days]      Clean old traces"
            ;;
    esac
}

main "$@"