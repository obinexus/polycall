#!/bin/bash
# build-tools/status-check.sh - Fixed version

# Default to 0 if no count provided
count=${1:-0}
topology=${2:-global}

# Load thresholds from manifest if available
if [ -f "config/build/in/topology.in" ]; then
  source config/build/in/topology.in
fi

# Modified thresholds to match requirements (3-6 range)
WARNING_THRESHOLD=${WARNING_THRESHOLD:-3}
CRITICAL_THRESHOLD=${CRITICAL_THRESHOLD:-6}
PANIC_THRESHOLD=${PANIC_THRESHOLD:-9}

# Create the metadata directory if it doesn't exist
mkdir -p build/metadata

# Check against thresholds and set status
if [ "$count" -ge "$PANIC_THRESHOLD" ]; then
  echo "STATE_PANIC :: $count issues in $topology topology" > build/metadata/topology-health.log
  echo "STATE_PANIC :: Build halted - exceeds maximum threshold ❌"
  exit 3
elif [ "$count" -ge "$CRITICAL_THRESHOLD" ]; then
  echo "STATE_CRITICAL :: $count issues in $topology topology" >> build/metadata/topology-health.log
  echo "STATE_CRITICAL :: Artifact requires immediate review 🚨"
  exit 2
elif [ "$count" -ge "$WARNING_THRESHOLD" ]; then
  echo "STATE_WARNING :: $count issues in $topology topology" >> build/metadata/topology-health.log
  echo "STATE_WARNING :: Build proceeded with minor issues ⚠️"
  exit 1
else
  echo "STATE_OK :: $count issues in $topology topology" >> build/metadata/topology-health.log
  echo "STATE_OK :: Artifact stable ✅"
  exit 0
fi
