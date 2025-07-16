#!/bin/bash
# build-tools/setup-zero-trust.sh

# Create .polycall configuration directory
mkdir -p ~/.polycall/config
mkdir -p ~/.polycall/auth

# Generate default config template
cat > ~/.polycall/config/default.toml << EOF
# Polycall Zero-Trust Configuration

[core]
protocol_version = "2.0"
enforce_zero_trust = true

[permissions]
allow_shell_exec = false
allow_network = true
allow_file_access = false

[api]
endpoints = [
  { name = "core", route = "/api/core", auth_required = true },
  { name = "health", route = "/api/health", auth_required = false }
]

[routing]
policy = "strict"
reverse_proxy = true

[runtime]
sandboxed = true
isolation_level = "high"
EOF

# Generate node identity if not exists
if [ ! -f ~/.polycall/auth/.zid ]; then
    echo "Generating node identity..."
    # In production this would use cryptographic keys
    echo "node-$(uuidgen)" > ~/.polycall/auth/.zid
fi

echo "Zero-Trust Layer initialized"
