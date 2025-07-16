#!/bin/bash
# build-tools/error-isolation.sh

# Ensure target directories exist
mkdir -p build/errors/core
mkdir -p build/logs

echo "Building module: core with error isolation"

# Run make and capture all output to logs
make -C src/core 2> >(tee build/errors/core/compile_errors.log)
MAKE_EXIT=$?

# Count warnings and errors
ERROR_COUNT=$(grep -c "error:\|warning:" build/errors/core/compile_errors.log || echo "0")
echo "$ERROR_COUNT" > build/errors/core/count.txt

# Classify errors and potentially exit
./build-tools/classify-errors.sh "$ERROR_COUNT"
CLASSIFY_EXIT=$?

# Forward the exit code from classification
if [ $CLASSIFY_EXIT -ne 0 ]; then
    exit $CLASSIFY_EXIT
fi

# If make failed but classification allowed it (QA mode), still indicate issue
if [ $MAKE_EXIT -ne 0 ]; then
    echo "⚠️ Build completed with errors, but allowed to proceed due to error classification"
fi

exit $MAKE_EXIT
