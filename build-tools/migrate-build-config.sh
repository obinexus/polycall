#!/bin/bash
# build-tools/migrate-build-config.sh

# Create config structure if it doesn't exist
mkdir -p config/build/{make,cmake,meson,in}
mkdir -p config/topology/{star,bus,p2p,ring}

# Move existing build configs to organized structure
if [ -d "build/cmake" ]; then
  cp -r build/cmake/* config/build/cmake/
fi

# Create manifest template files
cat > config/build/in/base.in << EOF
# Base build configuration
BUILD_MODE=release
PARALLEL_JOBS=4
DEBUG_SYMBOLS=true
OPTIMIZATION_LEVEL=3
EOF

cat > config/build/in/topology.in << EOF
# Topology configuration
DEFAULT_TOPOLOGY=mesh
VALIDATE_TOPOLOGY=true
TOPOLOGY_HEALTH_THRESHOLD=3
CRITICAL_THRESHOLD=6
DANGER_THRESHOLD=9
PANIC_THRESHOLD=12
EOF

# Generate topology-specific manifests
for topo in star bus p2p ring; do
  cat > config/topology/$topo/build.in << EOF
# $topo topology specific settings
TOPOLOGY=$topo
ARTIFACT_PREFIX=$topo
CONNECTION_MODEL=$(echo $topo | tr '[:lower:]' '[:upper:]')
EOF
done

echo "Build configuration migration complete"
