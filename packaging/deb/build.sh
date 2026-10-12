#!/bin/sh
# Build the polycall binary packages (polycall, libpolycall1, libpolycall-dev)
# for the Debian/Ubuntu release this runs on, from a pristine copy of the
# source tree, then lint them.
#
# Run as root inside a clean container of a release listed in
# packaging/suites.tsv (CI uses the job container; locally e.g.
#   docker run --rm -v "$PWD:/src:ro" -v "$PWD/dist:/dist" debian:12 \
#     sh /src/packaging/deb/build.sh --out /dist
# ). Installs its own build dependencies.
#
# usage: build.sh [--out DIR] [--no-lintian] [--nocheck]
#   --out DIR     results go to DIR/<codename>/<arch>/ (default: ./dist)
#   --no-lintian  skip lintian
#   --nocheck     skip the upstream test suite (DEB_BUILD_OPTIONS=nocheck)
#
# The Debian version is <debian/changelog version>~<suite suffix>, e.g.
# 1.0.0-1~deb12, so every release gets its own build with dependencies
# computed against that release.
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"

OUT=$(pwd)/dist
LINTIAN=1
NOCHECK=''
while [ $# -gt 0 ]; do
  case "$1" in
    --out) OUT=$2; shift 2 ;;
    --out=*) OUT=${1#*=}; shift ;;
    --no-lintian) LINTIAN=; shift ;;
    --nocheck) NOCHECK=1; shift ;;
    -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done
mkdir -p "$OUT"; OUT=$(CDPATH='' cd -- "$OUT" && pwd)

[ -r /etc/os-release ] || die "/etc/os-release missing; run this inside a Debian/Ubuntu container"
# shellcheck source=/dev/null
. /etc/os-release
CODENAME=${VERSION_CODENAME:-}
SUFFIX=$(suite_field "$CODENAME" suffix) \
  || die "release '${PRETTY_NAME:-unknown}' (codename '$CODENAME') is not listed in packaging/suites.tsv"
check_versions
UPSTREAM=$(upstream_version)
BASE_VERSION=$(debian_version)
VERSION="${BASE_VERSION}~${SUFFIX}"
info "building polycall $VERSION for $(suite_field "$CODENAME" name)"

export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -y -q --no-install-recommends dpkg-dev ca-certificates

# pristine copy of the tree: never build in (or write to) the checkout
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
SRC="$WORK/polycall-$UPSTREAM"
mkdir -p "$SRC"
( cd "$REPO_ROOT" && tar -cf - --exclude=./.git --exclude=./build --exclude=./dist \
    --exclude=./obj-* --exclude=./.polycall . ) | ( cd "$SRC" && tar -xf - )
chmod 0755 "$SRC/debian/rules" "$SRC"/debian/tests/cli-smoke "$SRC"/debian/tests/library-consumers
# CMake treats a source directory holding a CMakeCache.txt as a build tree, so
# debhelper's out-of-source `cmake ..` would refuse to configure.
[ ! -e "$SRC/CMakeCache.txt" ] || die "CMakeCache.txt is in the source root (in-source CMake configure output); remove it from the repository"

# Retarget the newest changelog entry at this release (version and
# distribution only). Its text and date stay as written, so the build is
# reproducible: SOURCE_DATE_EPOCH comes from that date.
sed -i "1s/^polycall ([^)]*) [^;]*;/polycall ($VERSION) $CODENAME;/" "$SRC/debian/changelog"
[ "$(cd "$SRC" && dpkg-parsechangelog -S Version)" = "$VERSION" ] || die "could not set the changelog version"

PROFILES=${NOCHECK:+nocheck}
apt-get build-dep -y -q --no-install-recommends ${PROFILES:+-P "$PROFILES"} "$SRC"
[ -n "$LINTIAN" ] && apt-get install -y -q --no-install-recommends lintian

JOBS=$(nproc 2>/dev/null || echo 2)
export DEB_BUILD_OPTIONS="noautodbgsym parallel=$JOBS${NOCHECK:+ nocheck}"
( cd "$SRC" && dpkg-buildpackage -b -us -uc ${PROFILES:+-P"$PROFILES"} )

ARCH=$(dpkg --print-architecture)
CHANGES="$WORK/polycall_${VERSION}_${ARCH}.changes"
[ -f "$CHANGES" ] || die "dpkg-buildpackage produced no $CHANGES"

# the staged install (debian/tmp, via DESTDIR) must have produced exactly the
# files the three packages ship -- dh_missing already enforces this; show it
for deb in "$WORK"/*.deb; do
  printf '\n== %s\n' "${deb##*/}"
  dpkg-deb --field "$deb" Package Version Architecture Depends Multi-Arch
  dpkg-deb --contents "$deb" | awk '{ print "   " $1 " " $6 ($7 ? " " $7 " " $8 : "") }'
done
contents=$(dpkg-deb --contents "$WORK/polycall_${VERSION}_${ARCH}.deb")
printf '%s\n' "$contents" | grep -q ' \./usr/bin/polycall$' \
  || die "polycall package does not ship /usr/bin/polycall"

if [ -n "$LINTIAN" ]; then
  printf '\n== lintian %s\n' "$(lintian --version)"
  # Older lintian releases do not know the newest Standards-Version yet.
  lintian --info --display-info --fail-on error,warning \
    --suppress-tags newer-standards-version "$CHANGES"
  echo "lintian: no errors or warnings"
fi

DEST="$OUT/$CODENAME/$ARCH"
rm -rf "$DEST"; mkdir -p "$DEST"
cp "$WORK"/*.deb "$CHANGES" "$WORK"/*.buildinfo "$DEST/"
info "packages for $CODENAME/$ARCH are in $DEST:"
ls -l "$DEST" >&2
