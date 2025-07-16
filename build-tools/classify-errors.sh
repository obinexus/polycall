#!/bin/bash
# build-tools/classify-errors.sh

ERROR_COUNT=$1
OUTPUT_FILE=$2

if [ -z "$ERROR_COUNT" ]; then
    echo "Usage: classify-errors.sh <error_count> [output_file]"
    exit 1
fi

# Write count to file for status reporting
echo "$ERROR_COUNT" > build/errors/core/count.txt

# Classify according to fault grades
if [ "$ERROR_COUNT" -le 3 ]; then
    echo "STATE_OK" > build/errors/core/state.txt
    exit 0
elif [ "$ERROR_COUNT" -le 6 ]; then
    echo "STATE_CRITICAL" > build/errors/core/state.txt
    # Continue but warn
    echo "⚠️ STATE_CRITICAL: $ERROR_COUNT errors detected (allowed: 4-6)"
    exit 0
elif [ "$ERROR_COUNT" -le 9 ]; then
    echo "STATE_DANGER" > build/errors/core/state.txt
    echo "🔴 STATE_DANGER: $ERROR_COUNT errors detected (allowed: 7-9, requires QA)"
    if [ "$BUILD_MODE" != "qa" ]; then
        echo "Set BUILD_MODE=qa to proceed with this build"
        exit 1
    fi
    exit 0
else
    echo "STATE_PANIC" > build/errors/core/state.txt
    echo "🔥 STATE_PANIC: $ERROR_COUNT errors detected (max: 9)"
    # Always exit with error in panic state
    exit 1
fi
