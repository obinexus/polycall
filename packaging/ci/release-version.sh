#!/bin/sh
# release-version.sh TAG
# Check that a release tag names exactly the version the tree packages, and
# print the Debian base version (e.g. 1.0.0-1) on stdout.
#   vX.Y.Z           upstream release: X.Y.Z must equal POLYCALL_ABI_VERSION_STRING
#                    and the upstream part of the newest debian/changelog entry
#   debian/<version> packaging-only release (DEP-14: '~' -> '_', ':' -> '%'):
#                    <version> must equal the newest debian/changelog version
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"
tag=${1:?usage: release-version.sh TAG}
check_versions
deb=$(debian_version)
case "$tag" in
  v*)
    [ "${tag#v}" = "$(upstream_version)" ] \
      || die "tag $tag does not match the source version $(upstream_version) (include/polycall_export.h)" ;;
  debian/*)
    want=$(printf '%s' "${tag#debian/}" | tr '_%' '~:')
    [ "$want" = "$deb" ] || die "tag $tag does not match debian/changelog version $deb" ;;
  *) die "unsupported tag $tag (expected vX.Y.Z or debian/<version>)" ;;
esac
case "$deb" in *~*|*+*) die "debian/changelog version $deb must not carry a ~ or + suffix; build.sh adds the per-release suffix" ;; esac
printf '%s\n' "$deb"
