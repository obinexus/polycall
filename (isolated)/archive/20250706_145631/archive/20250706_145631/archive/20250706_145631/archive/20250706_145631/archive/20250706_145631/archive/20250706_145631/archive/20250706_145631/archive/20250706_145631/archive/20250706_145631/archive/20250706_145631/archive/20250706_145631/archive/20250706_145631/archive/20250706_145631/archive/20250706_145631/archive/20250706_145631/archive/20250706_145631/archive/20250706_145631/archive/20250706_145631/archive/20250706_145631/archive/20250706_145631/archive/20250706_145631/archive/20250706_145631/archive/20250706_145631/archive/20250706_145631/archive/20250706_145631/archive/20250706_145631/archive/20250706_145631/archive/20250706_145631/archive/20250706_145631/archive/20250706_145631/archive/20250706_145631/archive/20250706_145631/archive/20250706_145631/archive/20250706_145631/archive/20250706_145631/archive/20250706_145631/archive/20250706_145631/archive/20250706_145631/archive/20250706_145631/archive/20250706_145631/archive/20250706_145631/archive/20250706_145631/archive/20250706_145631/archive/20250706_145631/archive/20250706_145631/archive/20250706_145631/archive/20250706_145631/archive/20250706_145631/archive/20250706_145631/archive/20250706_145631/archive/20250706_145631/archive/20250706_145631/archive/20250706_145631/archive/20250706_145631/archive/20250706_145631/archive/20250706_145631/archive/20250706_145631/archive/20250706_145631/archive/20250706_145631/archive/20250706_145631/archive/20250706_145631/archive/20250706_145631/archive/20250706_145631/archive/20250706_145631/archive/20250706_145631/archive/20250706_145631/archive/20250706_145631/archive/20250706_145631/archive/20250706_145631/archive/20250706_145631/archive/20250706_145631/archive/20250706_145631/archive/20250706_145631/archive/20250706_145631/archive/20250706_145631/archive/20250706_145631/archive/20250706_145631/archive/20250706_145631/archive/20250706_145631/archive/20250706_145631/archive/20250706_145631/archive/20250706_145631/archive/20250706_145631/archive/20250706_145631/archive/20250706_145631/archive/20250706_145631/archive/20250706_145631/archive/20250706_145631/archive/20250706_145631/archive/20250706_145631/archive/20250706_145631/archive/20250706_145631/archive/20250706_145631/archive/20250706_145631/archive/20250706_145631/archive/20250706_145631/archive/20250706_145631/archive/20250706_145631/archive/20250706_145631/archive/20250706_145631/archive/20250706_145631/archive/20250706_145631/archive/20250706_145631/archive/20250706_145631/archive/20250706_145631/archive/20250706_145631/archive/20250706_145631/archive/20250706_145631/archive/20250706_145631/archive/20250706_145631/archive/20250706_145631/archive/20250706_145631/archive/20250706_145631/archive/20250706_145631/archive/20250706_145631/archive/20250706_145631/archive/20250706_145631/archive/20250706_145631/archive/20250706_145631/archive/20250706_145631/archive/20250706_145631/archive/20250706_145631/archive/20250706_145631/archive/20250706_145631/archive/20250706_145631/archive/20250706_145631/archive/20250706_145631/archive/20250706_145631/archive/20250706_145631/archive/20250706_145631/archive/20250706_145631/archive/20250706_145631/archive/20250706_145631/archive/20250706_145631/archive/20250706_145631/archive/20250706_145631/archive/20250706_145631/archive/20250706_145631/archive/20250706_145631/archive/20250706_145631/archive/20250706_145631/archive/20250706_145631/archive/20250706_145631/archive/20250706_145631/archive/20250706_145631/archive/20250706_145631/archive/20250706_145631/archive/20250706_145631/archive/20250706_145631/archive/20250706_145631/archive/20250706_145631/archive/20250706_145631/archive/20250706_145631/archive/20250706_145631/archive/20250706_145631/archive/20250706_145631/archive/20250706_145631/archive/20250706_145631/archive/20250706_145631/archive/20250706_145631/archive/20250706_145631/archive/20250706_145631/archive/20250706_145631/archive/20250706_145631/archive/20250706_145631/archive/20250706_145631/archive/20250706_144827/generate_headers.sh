#!/bin/bash
# Generate header files for all C sources

for dir in base protocol network auth bridges polycall; do
    for cfile in src/core/$dir/*.c; do
        if [[ -f "$cfile" ]]; then
            hfile="${cfile%.c}.h"
            if [[ ! -f "$hfile" ]]; then
                echo "Generating $hfile..."
                # Extract function signatures and create header
                basename=$(basename "$cfile" .c)
                upper=$(echo "$basename" | tr '[:lower:]' '[:upper:]')
                
                cat > "$hfile" << HEADER
#ifndef POLYCALL_${upper}_H
#define POLYCALL_${upper}_H

#include <stdint.h>
#include <stddef.h>

// TODO: Add function declarations from $cfile

#endif // POLYCALL_${upper}_H
HEADER
            fi
        fi
    done
done
