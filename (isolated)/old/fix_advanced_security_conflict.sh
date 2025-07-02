#!/bin/bash
# Fix advanced_security.c file conflicts in OBINexus PolyCall

set -euo pipefail

PROJECT_ROOT="${1:-.}"
cd "$PROJECT_ROOT"

echo "=== Fixing advanced_security.c Conflicts ==="
echo

# The issue: advanced_security.c exists in multiple modules
# Solution: It should only exist in the auth module (infrastructure)

# Step 1: Check where advanced_security.c files exist
echo "Checking for advanced_security.c files..."
SECURITY_FILES=$(find src/core -name "advanced_security.c" -type f 2>/dev/null || true)

if [ -z "$SECURITY_FILES" ]; then
    echo "No advanced_security.c files found"
    exit 0
fi

echo "Found advanced_security files:"
echo "$SECURITY_FILES" | while read -r file; do
    echo "  - $file"
done
echo

# Step 2: Ensure auth module has the canonical version
AUTH_SECURITY="src/core/auth/advanced_security.c"
EDGE_SECURITY="src/core/edge/advanced_security.c"

if [ -f "$EDGE_SECURITY" ]; then
    echo "Moving advanced_security from edge to auth module..."
    
    if [ -f "$AUTH_SECURITY" ]; then
        # Auth already has it, just remove from edge
        echo "  Auth module already has advanced_security.c"
        echo "  Removing duplicate from edge module..."
        rm "$EDGE_SECURITY"
    else
        # Move from edge to auth
        mkdir -p "$(dirname "$AUTH_SECURITY")"
        mv "$EDGE_SECURITY" "$AUTH_SECURITY"
        echo "  Moved to auth module"
    fi
    
    # Update any edge files that reference advanced_security
    echo "  Updating edge module references..."
    find src/core/edge -name "*.c" -type f | while read -r file; do
        # Replace local includes with auth module includes
        sed -i 's|#include "advanced_security\.h"|#include "polycall/auth/advanced_security.h"|g' "$file"
        sed -i 's|#include "edge/advanced_security\.h"|#include "polycall/auth/advanced_security.h"|g' "$file"
    done
fi

# Step 3: Fix the auth module's advanced_security.c to not violate purity
if [ -f "$AUTH_SECURITY" ]; then
    echo "Fixing includes in auth/advanced_security.c..."
    
    # Create a backup
    cp "$AUTH_SECURITY" "${AUTH_SECURITY}.bak"
    
    # Since auth is infrastructure, it can include protocol
    # But let's make sure the includes are correct
    sed -i 's|#include\s*["<]protocol/|#include "polycall/protocol/|g' "$AUTH_SECURITY"
    sed -i 's|#include\s*["<]network/|#include "polycall/network/|g' "$AUTH_SECURITY"
    
    echo "  ✓ Fixed include paths"
fi

# Step 4: Create/update the advanced_security.h header
AUTH_SECURITY_H="include/polycall/auth/advanced_security.h"
mkdir -p "$(dirname "$AUTH_SECURITY_H")"

if [ ! -f "$AUTH_SECURITY_H" ]; then
    echo "Creating advanced_security.h header..."
    cat > "$AUTH_SECURITY_H" << 'EOF'
/*
 * advanced_security.h
 * Advanced security features for PolyCall
 * Part of the auth infrastructure module
 */

#ifndef POLYCALL_ADVANCED_SECURITY_H
#define POLYCALL_ADVANCED_SECURITY_H

#include <stddef.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct polycall_security_context polycall_security_context_t;
typedef struct polycall_security_policy polycall_security_policy_t;

/* Security context management */
polycall_security_context_t* polycall_security_create_context(void);
void polycall_security_destroy_context(polycall_security_context_t* ctx);

/* Policy management */
polycall_security_policy_t* polycall_security_create_policy(const char* name);
void polycall_security_destroy_policy(polycall_security_policy_t* policy);
int polycall_security_apply_policy(polycall_security_context_t* ctx,
                                   polycall_security_policy_t* policy);

/* Security operations */
int polycall_security_validate_token(polycall_security_context_t* ctx,
                                     const char* token);
int polycall_security_encrypt_data(polycall_security_context_t* ctx,
                                   const void* data, size_t data_len,
                                   void** encrypted, size_t* encrypted_len);
int polycall_security_decrypt_data(polycall_security_context_t* ctx,
                                   const void* encrypted, size_t encrypted_len,
                                   void** data, size_t* data_len);

/* Audit and compliance */
void polycall_security_audit_log(polycall_security_context_t* ctx,
                                 const char* action, const char* details);
bool polycall_security_check_compliance(polycall_security_context_t* ctx);

#endif /* POLYCALL_ADVANCED_SECURITY_H */
EOF
    echo "  ✓ Created header file"
fi

# Step 5: Remove any edge module security header
EDGE_SECURITY_H="include/polycall/edge/advanced_security.h"
if [ -f "$EDGE_SECURITY_H" ]; then
    echo "Removing duplicate header from edge module..."
    rm "$EDGE_SECURITY_H"
fi

# Step 6: Update edge module to use auth security
echo "Updating edge module to use auth infrastructure..."
find src/core/edge -name "*.c" -type f | while read -r file; do
    if grep -q "advanced_security" "$file" 2>/dev/null; then
        echo "  Updating: $(basename "$file")"
        # Update to use auth module's security
        sed -i 's|#include\s*["<]advanced_security\.h[">]|#include "polycall/auth/advanced_security.h"|g' "$file"
        sed -i 's|#include\s*["<]edge/advanced_security\.h[">]|#include "polycall/auth/advanced_security.h"|g' "$file"
    fi
done

echo
echo "✓ Fixed advanced_security.c conflicts"
echo
echo "Summary:"
echo "- advanced_security.c now exists only in auth module"
echo "- auth is an infrastructure module (can be used by commands)"
echo "- edge module updated to use auth's security features"
echo
echo "Next: Run the quick fix to update module classifications:"
echo "  ./scripts/quick_fix_violations.sh"
