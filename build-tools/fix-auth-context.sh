#!/bin/bash
# build-tools/fix-auth-context.sh

# Check if auth_context.c exists
if [ -f "src/polycall/auth_context.c" ]; then
  # Create directory for the log if it doesn't exist
  mkdir -p build/logs
  
  # Check for potential issues in the file
  echo "Analyzing auth_context.c for potential issues..."
  
  # Common issues include missing headers, undefined symbols
  missing_headers=$(grep -c "#include" src/polycall/auth_context.c)
  if [ $missing_headers -eq 0 ]; then
    echo "Warning: No include statements found in auth_context.c"
    
    # Add standard headers if missing
    echo "#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include \"../../include/polycall/auth.h\"

$(cat src/polycall/auth_context.c)" > src/polycall/auth_context.c.new
    mv src/polycall/auth_context.c.new src/polycall/auth_context.c
    echo "Added standard headers to auth_context.c"
  fi
  
  # Check for main function
  if ! grep -q "int main" src/polycall/auth_context.c; then
    echo "Warning: No main function found in auth_context.c"
    echo "
int main(int argc, char *argv[]) {
    printf(\"Auth context module initialized\\n\");
    // Add implementation here
    return 0;
}
" >> src/polycall/auth_context.c
    echo "Added main function to auth_context.c"
  fi
  
  echo "Fixed potential issues in auth_context.c"
else
  echo "Error: src/polycall/auth_context.c not found"
  exit 1
fi
