#!/bin/sh
# Verify the PUBLISHED repository from a clean container of a supported
# release, exactly as a user would set it up from README.md.
#
# usage: verify-published.sh [BASE_VERSION] [WAIT_SECONDS]
#   BASE_VERSION  Debian base version that must be installable, e.g. 1.0.0-1
#                 (the per-release suffix is added); empty = newest published
#   WAIT_SECONDS  how long to wait for BASE_VERSION to appear (CDN/deploy lag)
#
# The expected key fingerprint is read from README.md -- the value users are
# told to compare -- so a README/key mismatch fails here too.
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"
# shellcheck source=/dev/null
. /etc/os-release
suffix=$(suite_field "$VERSION_CODENAME" suffix) || die "$PRETTY_NAME is not a supported release"
fpr=$(documented_fingerprint)
[ -n "$fpr" ] || die "README.md documents no signing-key fingerprint"
[ "$(printf '%s\n' "$fpr" | wc -l)" -eq 1 ] || die "README.md documents more than one fingerprint"
export POLYCALL_APT_FINGERPRINT="$fpr"
export POLYCALL_APT_URL="${POLYCALL_APT_URL:-https://obinexus.github.io/polycall/apt}"
if [ -n "${1:-}" ]; then
  export POLYCALL_EXPECT_VERSION="$1~$suffix"
  export POLYCALL_WAIT_SECONDS="${2:-0}"
fi
exec sh "$REPO_ROOT/packaging/test/client-check.sh" setup install dev oldest-first remove
