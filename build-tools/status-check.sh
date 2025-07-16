#!/bin/bash
# build-tools/status-check.sh

# Ensure count.txt exists
if [ ! -f build/errors/core/count.txt ]; then
    echo "0" > build/errors/core/count.txt
fi

# Read error count
ERROR_COUNT=$(cat build/errors/core/count.txt)

# Default to zero if not a number
if ! [[ "$ERROR_COUNT" =~ ^[0-9]+$ ]]; then
    ERROR_COUNT=0
fi

# Determine build state
if [ "$ERROR_COUNT" -le 3 ]; then
    STATE="STATE_OK"
    ICON="🟢"
elif [ "$ERROR_COUNT" -le 6 ]; then
    STATE="STATE_CRITICAL"
    ICON="🟡"
elif [ "$ERROR_COUNT" -le 9 ]; then
    STATE="STATE_DANGER"
    ICON="🔴"
else
    STATE="STATE_PANIC"
    ICON="🔥"
fi

# Only show details in debug mode
if [ "$BUILD_MODE" = "debug" ]; then
    echo "$ICON $STATE: $ERROR_COUNT errors detected"
else
    # In non-debug mode, only show warnings for non-OK states
    if [ "$ERROR_COUNT" -gt 3 ]; then
        echo "$ICON $STATE: $ERROR_COUNT errors detected"
    fi
fi

# Return exit code based on state
if [ "$STATE" = "STATE_PANIC" ]; then
    exit 1
elif [ "$STATE" = "STATE_DANGER" ] && [ "$BUILD_MODE" != "qa" ]; then
    exit 1
else
    exit 0
fi
