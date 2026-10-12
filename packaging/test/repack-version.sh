#!/bin/sh
# repack-version.sh IN_DIR OUT_DIR TAG
# Copy every .deb in IN_DIR to OUT_DIR as version <version>+TAG, rewriting
# exact (= <version>) dependencies between them, so the end-to-end test can
# publish a strictly newer version and exercise `apt upgrade` without a
# second build. Test use only: the repacked packages are never published.
set -eu
in=${1:?IN_DIR}; out=${2:?OUT_DIR}; tag=${3:?TAG}
mkdir -p "$out"
for deb in "$in"/*.deb; do
  t=$(mktemp -d)
  dpkg-deb -R "$deb" "$t/root"
  ctl="$t/root/DEBIAN/control"
  old=$(sed -n 's/^Version: //p' "$ctl")
  new="$old+$tag"
  sed -i -e "s/^Version: .*/Version: $new/" -e "s/(= $old)/(= $new)/g" "$ctl"
  pkg=$(sed -n 's/^Package: //p' "$ctl")
  arch=$(sed -n 's/^Architecture: //p' "$ctl")
  dpkg-deb --root-owner-group -b "$t/root" "$out/${pkg}_${new}_${arch}.deb" >/dev/null
  rm -rf "$t"
  echo "repacked $pkg $old -> $new"
done
