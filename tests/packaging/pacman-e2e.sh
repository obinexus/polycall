#!/bin/sh
# End-to-end pacman distribution test (needs docker):
#   source tarball of HEAD -> packaging/arch/PKGBUILD stamped with its real
#   checksum -> makepkg (with check()) as a non-root user, pkgrel 1 and 2 ->
#   signed packages + signed repository databases (throwaway key generated
#   inside the builder container) -> tests/packaging/pacman-client.sh in a
#   fresh container.
# Usage: tests/packaging/pacman-e2e.sh OUTDIR
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
mkdir -p "${1:?usage: pacman-e2e.sh OUTDIR}"
OUT=$(cd "$1" && pwd)
IMAGE=${POLYCALL_ARCH_IMAGE:-archlinux:base-devel}
hostpath() { if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else printf '%s' "$1"; fi; }
export MSYS_NO_PATHCONV=1

rm -rf "$OUT/arch-repo-v1" "$OUT/arch-repo-v2"
TAR=$(sh "$ROOT/packaging/make-source-tarball.sh" HEAD "$OUT" | awk '{print $2}')
TARNAME=$(basename "$TAR")
UPV=${TARNAME#polycall-}; UPV=${UPV%.tar.gz}
SUM=$(sha256sum "$TAR" | awk '{print $1}')
echo "== source: $TARNAME ($SUM)"

docker run --rm -v "$(hostpath "$OUT"):/out" "$IMAGE" sh -c '
  set -eu
  pacman-key --init >/dev/null 2>&1; pacman-key --populate archlinux >/dev/null 2>&1
  pacman -Syu --noconfirm --needed cmake python >/dev/null
  useradd -m builder
  for rel in 1 2; do
    d=/home/builder/p$rel; mkdir -p $d
    tar -xzf /out/'"$TARNAME"' -C /tmp polycall-'"$UPV"'/packaging/arch/PKGBUILD
    sed -e "s/^sha256sums=(.SKIP.)$/sha256sums=('"'"''"$SUM"''"'"')/" -e "s/^pkgrel=.*/pkgrel=$rel/" \
      /tmp/polycall-'"$UPV"'/packaging/arch/PKGBUILD > $d/PKGBUILD
    grep -q "^sha256sums=(.'"$SUM"'.)$" $d/PKGBUILD
    cp /out/'"$TARNAME"' $d/
    chown -R builder $d
    su builder -c "cd $d && makepkg --noconfirm --cleanbuild" > /out/makepkg-$rel.log 2>&1
  done
  export GNUPGHOME=$(mktemp -d)
  gpg --batch --passphrase "" --quick-gen-key "polycall test archive (throwaway) <noreply@invalid>" ed25519 sign 1d 2>/dev/null
  FPR=$(gpg --list-keys --with-colons | awk -F: "/^fpr/{print \$10; exit}")
  for v in 1 2; do
    r=/out/arch-repo-v$v; mkdir -p $r
    cp /home/builder/p$v/polycall-'"$UPV"'-$v-*.pkg.tar.zst $r/
    for p in $r/*.pkg.tar.zst; do gpg --batch --local-user "$FPR" --detach-sign "$p"; done
    repo-add --sign --key "$FPR" $r/polycall.db.tar.gz $r/*.pkg.tar.zst >/dev/null
    gpg --batch --armor --export "$FPR" > $r/polycall-archive.asc
  done
  echo "$FPR" > /out/arch-key.fpr
  rm -rf "$GNUPGHOME"
' >"$OUT/arch-build.log" 2>&1 || { echo "build failed:"; tail -30 "$OUT/arch-build.log"; tail -40 "$OUT/makepkg-1.log" 2>/dev/null; exit 1; }
FPR=$(cat "$OUT/arch-key.fpr")

log="$OUT/client-archlinux.log"
if docker run --rm -v "$(hostpath "$OUT/arch-repo-v1"):/repo-v1:ro" -v "$(hostpath "$OUT/arch-repo-v2"):/repo-v2:ro" \
     -v "$(hostpath "$HERE"):/t:ro" "$IMAGE" sh /t/pacman-client.sh "$UPV" "$FPR" >"$log" 2>&1; then
  echo "PASS  $IMAGE  ($(tail -1 "$log"))"
else
  echo "FAIL  $IMAGE  (see $log)"; grep '^FAIL' "$log" || tail -20 "$log"; exit 1
fi
