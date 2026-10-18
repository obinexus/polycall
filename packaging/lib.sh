# Shared helpers for packaging/*.sh. POSIX sh; source it, do not run it.
#   . "$(dirname "$0")/../lib.sh"     (from packaging/<dir>/<script>.sh)

# repository root = nearest ancestor of the calling script that holds
# include/polycall_export.h
REPO_ROOT=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
while [ "$REPO_ROOT" != / ] && [ ! -f "$REPO_ROOT/include/polycall_export.h" ]; do
  REPO_ROOT=$(dirname "$REPO_ROOT")
done
[ -f "$REPO_ROOT/include/polycall_export.h" ] || { echo "cannot locate the polycall source tree" >&2; exit 1; }
SUITES_TSV="$REPO_ROOT/packaging/suites.tsv"

die()  { printf '%s: error: %s\n' "${0##*/}" "$*" >&2; exit 1; }
info() { printf '%s: %s\n' "${0##*/}" "$*" >&2; }

# suite_rows: the data rows of suites.tsv (no comments, no blank lines)
suite_rows() { grep -v '^[[:space:]]*#' "$SUITES_TSV" | grep -v '^[[:space:]]*$'; }

# suite_codenames: every supported codename, one per line
suite_codenames() { suite_rows | cut -f1; }

# suite_field CODENAME FIELD  (FIELD: image | suffix | archs | name)
suite_field() {
  _col=
  case "$2" in image) _col=2 ;; suffix) _col=3 ;; archs) _col=4 ;; name) _col=5 ;;
    *) die "suite_field: unknown field $2" ;; esac
  suite_rows | awk -F '\t' -v c="$1" -v n="$_col" '$1 == c { print $n; found=1 } END { exit !found }'
}

# upstream_version: the canonical version (POLYCALL_ABI_VERSION_STRING)
upstream_version() {
  sed -n 's/^#define POLYCALL_ABI_VERSION_STRING "\(.*\)"$/\1/p' "$REPO_ROOT/include/polycall_export.h"
}

# debian_version: version of the newest debian/changelog entry, e.g. 1.0.0-1
debian_version() {
  sed -n '1s/^[^ ]* (\([^)]*\)).*/\1/p' "$REPO_ROOT/debian/changelog"
}

# check_versions: debian/changelog must package the version the header declares
check_versions() {
  _up=$(upstream_version); _deb=$(debian_version)
  [ -n "$_up" ] || die "cannot read POLYCALL_ABI_VERSION_STRING from include/polycall_export.h"
  [ -n "$_deb" ] || die "cannot read the version from debian/changelog"
  [ "${_deb%-*}" = "$_up" ] || die "debian/changelog packages ${_deb%-*} but include/polycall_export.h declares $_up; add a changelog entry"
}

# documented_fingerprint: the signing-key fingerprint README.md tells users
# to verify (between the apt-key-fingerprint markers); empty if none yet
documented_fingerprint() {
  sed -n 's#.*<!-- apt-key-fingerprint -->\([0-9A-F]\{40\}\)<!-- /apt-key-fingerprint -->.*#\1#p' \
    "$REPO_ROOT/README.md" | sort -u
}

# key_fingerprint FILE: primary-key fingerprint of an OpenPGP public key file
key_fingerprint() {
  gpg --batch --show-keys --with-colons "$1" 2>/dev/null \
    | awk -F: '$1 == "pub" { want = 1; next } want && $1 == "fpr" { print $10; exit }'
}
