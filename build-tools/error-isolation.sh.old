#!/bin/bash
# error-isolation.sh - Error interception mechanism
set -euo pipefail

# 1. Create isolated error capture context
mkdir -p build/errors/{src,include,platform}

# 2. Implement error redirection mechanism
capture_errors() {
  local module=$1
  local error_log="build/errors/${module}/compile_errors.log"
  
  # Isolate stderr while preserving exit code
  {
    make -C src/${module} 2> >(tee "${error_log}")
  } || {
    local exit_code=$?
    # Classify errors without breaking build
    ./build-tools/classify-errors.sh "${error_log}" "${module}"
    return $((exit_code > 6 ? 6 : exit_code))  # Cap at 6 to prevent build termination
  }
}

# 3. Process each module independently
for module in core platform util; do
  echo "Building module: ${module} with error isolation"
  capture_errors "${module}"
done
