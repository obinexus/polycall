
# Git LFS compliance validation
echo "5. Validating Git LFS Configuration..."
if ! bash scripts/validate-lfs-compliance.sh; then
    echo "❌ VIOLATION: Git LFS configuration issues detected"
    exit 3
fi
