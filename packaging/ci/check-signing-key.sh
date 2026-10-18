#!/bin/sh
# check-signing-key.sh [SIGNING_FINGERPRINT]
# The committed public key (packaging/apt/polycall-archive-keyring.asc), the
# fingerprint README.md tells users to verify, and -- when given -- the key
# CI signs with (the APT_SIGNING_KEY secret) must all be the same single key.
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"
key="$REPO_ROOT/packaging/apt/polycall-archive-keyring.asc"
[ -f "$key" ] || die "${key#"$REPO_ROOT"/} is missing: create the signing key with packaging/apt/new-signing-key.sh (see docs/PACKAGING.md)"
[ "$(gpg --batch --show-keys --with-colons "$key" | grep -c '^pub:')" -eq 1 ] \
  || die "${key#"$REPO_ROOT"/} must contain exactly one key"
committed=$(key_fingerprint "$key")
documented=$(documented_fingerprint)
[ "$documented" = "$committed" ] \
  || die "README.md documents fingerprint '${documented:-none}' but ${key#"$REPO_ROOT"/} is $committed"
if [ -n "${1:-}" ]; then
  [ "$1" = "$committed" ] || die "APT_SIGNING_KEY is key $1, but the committed public key is $committed"
fi
echo "signing key $committed: committed and documented in README.md${1:+; matches APT_SIGNING_KEY}"
