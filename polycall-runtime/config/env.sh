#!/bin/bash
# PolyCall Environment Configuration
# Source this file to set up PolyCall environment

export POLYCALL_HOME="/home/obinexus/projects/pkg/polycall/polycall-runtime"
export POLYCALL_CONFIG="/home/obinexus/projects/pkg/polycall/polycall-runtime/config/polycall.yaml"
export POLYCALL_CRYPTO_SEED="[0;34m[INFO][0m Generating cryptographic seed...
wRzxEFIRrwMRJ8UzJAP4SAjfJtySoFpQHbKwo/tG/1c="
export POLYCALL_TELEMETRY=1
export POLYCALL_LOG_DIR="/home/obinexus/projects/pkg/polycall/polycall-runtime/logs"

# Add PolyCall to PATH
export PATH="/home/obinexus/projects/pkg/polycall/polycall-runtime/bin:${PATH}"

# Library path
if [[ "$(uname)" == "Darwin" ]]; then
    export DYLD_LIBRARY_PATH="/home/obinexus/projects/pkg/polycall/polycall-runtime/lib:${DYLD_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="/home/obinexus/projects/pkg/polycall/polycall-runtime/lib:${LD_LIBRARY_PATH}"
fi

echo "PolyCall environment configured"
echo "  POLYCALL_HOME: ${POLYCALL_HOME}"
echo "  Crypto seed: [REDACTED]"
