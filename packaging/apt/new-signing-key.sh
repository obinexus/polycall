#!/bin/sh
# Create the APT repository signing key -- once, on a trusted machine.
#
#   sh packaging/apt/new-signing-key.sh --backup ~/polycall-apt-signing-key.asc [--github]
#
# What it does:
#   1. generates an Ed25519 signing-only OpenPGP key in a throw-away GNUPGHOME
#   2. writes the PUBLIC key to packaging/apt/polycall-archive-keyring.asc
#      (commit this file; CI checks every signature against it)
#   3. writes the fingerprint into README.md (the value users verify)
#   4. writes the armored SECRET key to the --backup path (mode 0600, outside
#      the repository) so you can keep an offline copy
#   5. with --github: stores the secret key as the APT_SIGNING_KEY secret of
#      the apt-signing environment via `gh secret set` (GitHub CLI, logged in
#      with admin rights on the repository)
#   6. deletes the throw-away GNUPGHOME
#
# The secret key is never written inside the repository. Without --github,
# add it yourself:  gh secret set APT_SIGNING_KEY --env apt-signing < BACKUP
#
# Options:
#   --backup FILE   where to write the armored secret key (required)
#   --uid UID       key user ID (default: "polycall APT repository <maintainer email>")
#   --expire WHEN   gpg expiry (default 0 = never; e.g. 5y). An expiring key
#                   must be extended and re-published before it expires.
#   --github        also run `gh secret set` (see step 5)
#   --repo OWNER/NAME  repository for --github (default: obinexus/polycall)
#   --force         replace an existing packaging/apt/polycall-archive-keyring.asc
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"

BACKUP='' UID_='' EXPIRE=0 GITHUB='' GH_REPO=obinexus/polycall FORCE=''
while [ $# -gt 0 ]; do
  case "$1" in
    --backup) BACKUP=$2; shift 2 ;;
    --uid) UID_=$2; shift 2 ;;
    --expire) EXPIRE=$2; shift 2 ;;
    --github) GITHUB=1; shift ;;
    --repo) GH_REPO=$2; shift 2 ;;
    --force) FORCE=1; shift ;;
    -h|--help) sed -n '2,32p' "$0"; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done
[ -n "$BACKUP" ] || die "--backup FILE is required (an offline copy of the secret key)"
case "$(CDPATH='' cd -- "$(dirname -- "$BACKUP")" 2>/dev/null && pwd)/" in
  "$REPO_ROOT"/*) die "--backup must be outside the repository" ;;
esac
[ ! -e "$BACKUP" ] || die "$BACKUP already exists"
PUB="$REPO_ROOT/packaging/apt/polycall-archive-keyring.asc"
[ ! -e "$PUB" ] || [ -n "$FORCE" ] || die "$PUB exists; a new key forces every user to re-import it. Use --force to replace it."
command -v gpg >/dev/null 2>&1 || die "gpg not found"
if [ -n "$GITHUB" ]; then command -v gh >/dev/null 2>&1 || die "--github needs the GitHub CLI (gh)"; fi
if [ -z "$UID_" ]; then
  email=$(sed -n 's/^Maintainer: .*<\(.*\)>$/\1/p' "$REPO_ROOT/debian/control")
  UID_="polycall APT repository <$email>"
fi

GNUPGHOME=$(mktemp -d); export GNUPGHOME
chmod 0700 "$GNUPGHOME"
trap 'gpgconf --kill all >/dev/null 2>&1 || true; rm -rf "$GNUPGHOME"' EXIT

gpg --batch --pinentry-mode loopback --passphrase '' \
    --quick-generate-key "$UID_" ed25519 sign "$EXPIRE" >/dev/null 2>&1
FPR=$(gpg --batch --with-colons --list-secret-keys | awk -F: '$1 == "fpr" { print $10; exit }')
[ -n "$FPR" ] || die "key generation failed"

gpg --batch --armor --export-options export-minimal --export "$FPR" >"$PUB.tmp"
mv "$PUB.tmp" "$PUB"
( umask 077; gpg --batch --pinentry-mode loopback --passphrase '' --armor \
    --export-secret-keys "$FPR" >"$BACKUP" )

# README: replace whatever fingerprint is documented with the new one
README="$REPO_ROOT/README.md"
sed -e "s#\(<!-- apt-key-fingerprint -->\)[^<]*\(<!-- /apt-key-fingerprint -->\)#\1$FPR\2#g" \
    "$README" >"$README.tmp"
mv "$README.tmp" "$README"
[ "$(documented_fingerprint)" = "$FPR" ] || die "could not find the fingerprint markers in README.md"

if [ -n "$GITHUB" ]; then
  gh secret set APT_SIGNING_KEY --env apt-signing --repo "$GH_REPO" <"$BACKUP"
  info "stored APT_SIGNING_KEY in the apt-signing environment of $GH_REPO"
fi

cat <<EOF

Signing key created.
  fingerprint : $FPR
  user ID     : $UID_
  public key  : ${PUB#"$REPO_ROOT"/}   (commit it)
  README.md   : fingerprint updated     (commit it)
  secret key  : $BACKUP   (store offline; never commit)
$( [ -n "$GITHUB" ] && echo "  CI secret   : APT_SIGNING_KEY (environment apt-signing) set" \
   || echo "  CI secret   : NOT set -- run: gh secret set APT_SIGNING_KEY --env apt-signing --repo $GH_REPO < $BACKUP" )
EOF
