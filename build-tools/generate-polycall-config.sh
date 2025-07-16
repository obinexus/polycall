# build-tools/generate-polycall-config.sh
#!/bin/bash

# Generate .Polycallfile from build manifest
echo "Generating Polycall configuration..."

# Extract build state
ERROR_COUNT=$(cat build/errors/core/count.txt)

# Determine isolation level based on error state
if [ "$ERROR_COUNT" -le 3 ]; then
    ISOLATION="standard"
elif [ "$ERROR_COUNT" -le 6 ]; then
    ISOLATION="elevated"
else
    ISOLATION="maximum"
fi

# Generate configuration
cat > config.Polycallfile << EOF
# Generated Polycall Configuration
# Error count: $ERROR_COUNT
# Isolation level: $ISOLATION

server node 8080:8084
tls_enabled=true

micro core_service {
  port=3005:8085
  data_scope=$ISOLATION
  allowed_connections=system_api
  max_memory=512M
}
EOF

echo "✅ Polycall configuration generated"
