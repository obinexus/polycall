#!/bin/bash
# build-tools/generate-manifest.sh

# Create the topology manifest
generate_manifest() {
    cat > build/metadata/build-manifest.json << EOF
{
    "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "components": {
        "polycall_auth_context": {
            "status": "$(./build-tools/status-check.sh $(grep -c "warning:" build/logs/auth_context.log 2>/dev/null || echo "0"))",
            "fallback": "build/bin/auth_context_previous"
        }
    },
    "topologies": {
        "star": {
            "central_node": "build-orchestrator",
            "leaf_nodes": ["polycall_auth_context", "freebsd_gui_adapter"],
            "health": "$(./build-tools/status-check.sh $(grep -c "warning:" build/logs/star-*.log 2>/dev/null || echo "0") "star")"
        },
        "bus": {
            "shared_channels": ["stderr", "syslog", "telemetry"],
            "health": "$(./build-tools/status-check.sh $(grep -c "warning:" build/logs/bus-*.log 2>/dev/null || echo "0") "bus")"
        },
        "p2p": {
            "nodes": ["kernel_module_1", "kernel_module_2", "syscall_bridge"],
            "health": "$(./build-tools/status-check.sh $(grep -c "warning:" build/logs/p2p-*.log 2>/dev/null || echo "0") "p2p")"
        },
        "ring": {
            "nodes": ["compile_worker_1", "compile_worker_2", "compile_worker_3"],
            "health": "$(./build-tools/status-check.sh $(grep -c "warning:" build/logs/ring-*.log 2>/dev/null || echo "0") "ring")"
        }
    }
}
EOF
}

# Create directory if it doesn't exist
mkdir -p build/metadata

# Generate the manifest
generate_manifest

echo "✅ Build manifest generated: build/metadata/build-manifest.json"
