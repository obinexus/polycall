#!/bin/sh
# Build the Debian binary packages (libpolycall1, libpolycall-dev, polycall)
# from a release source tarball made by packaging/make-source-tarball.sh.
#
# Usage: packaging/build-deb.sh polycall-X.Y.Z.tar.gz OUTDIR
# Run on Debian/Ubuntu as a normal user, with the Build-Depends of
# packaging/debian/control installed (dpkg-checkbuilddeps verifies them).
#
# POLYCALL_DEB_VERSION=X.Y.Z-N builds that Debian revision instead of the
# one in packaging/debian/changelog (a changelog entry is added for it);
# the upstream part must match the tarball.
set -eu
TARBALL=$(realpath "${1:?usage: build-deb.sh polycall-X.Y.Z.tar.gz OUTDIR}")
mkdir -p "${2:?usage: build-deb.sh polycall-X.Y.Z.tar.gz OUTDIR}"
OUT=$(realpath "$2")
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

tar -xzf "$TARBALL" -C "$WORK"
set -- "$WORK"/polycall-*
[ $# -eq 1 ] && [ -d "$1" ] || { echo "build-deb: unexpected tarball layout" >&2; exit 1; }
SRC=$1
UP=${SRC##*/polycall-}
cp -r "$SRC/packaging/debian" "$SRC/debian"

if [ -n "${POLYCALL_DEB_VERSION:-}" ]; then
  [ "${POLYCALL_DEB_VERSION%-*}" = "$UP" ] || {
    echo "build-deb: POLYCALL_DEB_VERSION $POLYCALL_DEB_VERSION does not match source $UP" >&2; exit 1; }
  {
    echo "polycall ($POLYCALL_DEB_VERSION) unstable; urgency=medium"
    echo
    echo "  * Rebuild as $POLYCALL_DEB_VERSION."
    echo
    echo " -- $(sed -n 's/^Maintainer: //p' "$SRC/debian/control")  $(date -R)"
    echo
    cat "$SRC/debian/changelog"
  } > "$WORK/changelog" && mv "$WORK/changelog" "$SRC/debian/changelog"
fi
CHV=$(dpkg-parsechangelog -l "$SRC/debian/changelog" -S Version)
[ "${CHV%-*}" = "$UP" ] || { echo "build-deb: changelog $CHV does not match source $UP" >&2; exit 1; }

cp "$TARBALL" "$WORK/polycall_$UP.orig.tar.gz"
( cd "$SRC" && dpkg-checkbuilddeps && dpkg-buildpackage -us -uc -b )
cp "$WORK"/*.deb "$WORK"/*.buildinfo "$WORK"/*.changes "$OUT"/
ls -l "$OUT"
