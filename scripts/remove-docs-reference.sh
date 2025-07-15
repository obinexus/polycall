#!/bin/bash
# scripts/remove-docs-reference.sh
# Remove the polycall_docs.txt reference from the fix script

echo "Removing polycall_docs.txt reference..."

# Update the fix-immediate-issues.sh script to remove the note
if [ -f "scripts/fix-immediate-issue.sh" ]; then
    # Remove the last 3 lines about polycall_docs.txt
    head -n -3 scripts/fix-immediate-issue.sh > scripts/fix-immediate-issue.sh.tmp
    mv scripts/fix-immediate-issue.sh.tmp scripts/fix-immediate-issue.sh
    chmod +x scripts/fix-immediate-issue.sh
    echo "✓ Updated fix-immediate-issue.sh"
fi

# Fix the typo in the integration script name
if [ -f "scripts/intergrate-build-system.sh" ]; then
    mv scripts/intergrate-build-system.sh scripts/integrate-build-system.sh
    echo "✓ Fixed script name typo: intergrate -> integrate"
fi

echo "✓ References fixed!"
