#!/bin/sh
# Release source tarball, built from a commit: exactly the tracked files of
# that commit (git archive), never the working tree, so local build trees
# and untracked files cannot leak into a release. Reproducible: the
# timestamps come from the commit and gzip stores no name or time.
#
# Usage: packaging/make-source-tarball.sh [REV] [OUTDIR]
#   REV     commit/tag (default HEAD); OUTDIR default ./dist
# Prints the tarball path and its SHA-256. This tarball is the input of
# packaging/build-deb.sh and of the Arch/MSYS2 PKGBUILDs.
set -eu
ROOT=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
REV=${1:-HEAD}
OUT=${2:-dist}
mkdir -p "$OUT"

# the version of THAT commit, from its polycall_export.h
VER=$(git -C "$ROOT" show "$REV:include/polycall_export.h" |
  sed -n 's/^#define POLYCALL_ABI_VERSION_\(MAJOR\|MINOR\|PATCH\) \([0-9][0-9]*\).*/\2/p' |
  paste -sd. -)
case $VER in
  *.*.*) ;;
  *) echo "make-source-tarball: cannot read the version at $REV" >&2; exit 1 ;;
esac

TAR="$OUT/polycall-$VER.tar.gz"
git -C "$ROOT" archive --format=tar --prefix="polycall-$VER/" "$REV" | gzip -9n > "$TAR"
if command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$TAR"
else
  shasum -a 256 "$TAR"
fi
