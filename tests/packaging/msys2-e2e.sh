#!/usr/bin/env bash
# MSYS2 package test, run from an MSYS2 shell (MSYSTEM=UCRT64 by default):
#   release tarball -> packaging/msys2/PKGBUILD stamped with its checksum ->
#   makepkg-mingw (pkgrel 1 with check(); pkgrel 2 for the upgrade) ->
#   packages + repository database signed with a throwaway key ->
#   install / use / upgrade / remove in an ISOLATED pacman root
#   (--root/--dbpath/--gpgdir/--cachedir all under OUTDIR).
# The MSYS2 installation itself is only read: no package is installed into
# it, and its package list is compared before and after.
#
# Usage: tests/packaging/msys2-e2e.sh polycall-X.Y.Z.tar.gz OUTDIR
# Needs, already installed: makepkg-mingw, gnupg, and the PKGBUILD's
# makedepends/checkdepends for the environment.
set -u
TARBALL=$(realpath "${1:?usage: msys2-e2e.sh TARBALL OUTDIR}")
mkdir -p "${2:?usage: msys2-e2e.sh TARBALL OUTDIR}"
OUT=$(realpath "$2")
HERE=$(cd "$(dirname "$0")" && pwd)
ENVN=${MINGW_ARCH:-ucrt64}
PFX=$MINGW_PACKAGE_PREFIX
[ -n "$PFX" ] && [ -n "$MINGW_PREFIX" ] || { echo "run from an MSYS2 MinGW shell (MSYSTEM=UCRT64)"; exit 2; }
PKG=$PFX-polycall
PASS=0; FAIL=0
ok()  { echo "PASS  $*"; PASS=$((PASS + 1)); }
bad() { echo "FAIL  $*"; FAIL=$((FAIL + 1)); }
check() { if eval "$1"; then ok "$2"; else bad "$2"; fi; }

TARNAME=$(basename "$TARBALL")
UPV=${TARNAME#polycall-}; UPV=${UPV%.tar.gz}
SUM=$(sha256sum < "$TARBALL"); SUM=${SUM%% *}
BEFORE=$(pacman -Q | sha256sum)
[ -n "${POLYCALL_E2E_MAKEPKG_FLAGS:-}" ] && echo "NOTE  makepkg-mingw extra flags: $POLYCALL_E2E_MAKEPKG_FLAGS (node: $(command -v node || echo none))"
rm -rf "$OUT/build" "$OUT/repo-v1" "$OUT/repo-v2" "$OUT/root" "$OUT/gnupg-signer"

# ---- build ---------------------------------------------------------------------
for rel in 1 2; do
  d="$OUT/build/r$rel"; mkdir -p "$d"
  tar -xzf "$TARBALL" -O "polycall-$UPV/packaging/msys2/PKGBUILD" |
    sed -e "s/^sha256sums=('SKIP')$/sha256sums=('$SUM')/" -e "s/^pkgrel=.*/pkgrel=$rel/" > "$d/PKGBUILD"
  grep -q "^sha256sums=('$SUM')$" "$d/PKGBUILD" || { echo "PKGBUILD stamping failed"; exit 1; }
  cp "$TARBALL" "$d/"
  # pkgrel 2 is the identical source rebuilt for the upgrade test: --nocheck
  nc=; [ $rel = 2 ] && nc=--nocheck
  # POLYCALL_E2E_MAKEPKG_FLAGS=--nodeps: for a local MSYS2 without the
  # checkdepends packages, with those tools provided on PATH instead (reported)
  ( cd "$d" && MINGW_ARCH=$ENVN makepkg-mingw --noconfirm --cleanbuild $nc ${POLYCALL_E2E_MAKEPKG_FLAGS:-} ) > "$OUT/makepkg-$rel.log" 2>&1
  check '[ $? -eq 0 ] && ls "$d"/$PKG-$UPV-$rel-any.pkg.tar.zst >/dev/null 2>&1' \
    "makepkg-mingw pkgrel=$rel$( [ $rel = 1 ] && echo ' (with check())')"
done
grep -q "100% tests passed" "$OUT/makepkg-1.log" && ok "check(): ctest 100% passed" || bad "check(): ctest did not pass"

# ---- sign (throwaway key, kept under OUTDIR, deleted at the end) ----------------
export GNUPGHOME="$OUT/gnupg-signer"; mkdir -p "$GNUPGHOME"; chmod 700 "$GNUPGHOME"
gpg --batch --passphrase "" --quick-gen-key "polycall test archive (throwaway) <noreply@invalid>" ed25519 sign 1d 2>/dev/null
FPR=$(gpg --list-keys --with-colons | awk -F: '/^fpr/{print $10; exit}')
for v in 1 2; do
  r="$OUT/repo-v$v"; mkdir -p "$r"
  cp "$OUT/build/r$v"/$PKG-$UPV-$v-any.pkg.tar.zst "$r/"
  for p in "$r"/*.pkg.tar.zst; do gpg --batch --local-user "$FPR" --detach-sign "$p"; done
  repo-add --sign --key "$FPR" "$r/polycall.db.tar.gz" "$r"/*.pkg.tar.zst >/dev/null 2>&1
  gpg --batch --armor --export "$FPR" > "$r/polycall-archive.asc"
done
unset GNUPGHOME

# ---- isolated pacman root ----------------------------------------------------------
R="$OUT/root"; mkdir -p "$R/var/lib/pacman" "$R/cache" "$R/gnupg" "$R/hooks"
P() { pacman --root "$R" --dbpath "$R/var/lib/pacman" --gpgdir "$R/gnupg" --cachedir "$R/cache" \
             --hookdir "$R/hooks" --logfile "$R/pacman.log" --config "$R/pacman.conf" "$@"; }
use_repo() {
  printf '[options]\nArchitecture = auto\nSigLevel = Required DatabaseRequired\n\n[polycall]\nServer = file://%s\n' \
    "$(cygpath -m "$1")" > "$R/pacman.conf"
}
pacman-key --gpgdir "$R/gnupg" --init >/dev/null 2>&1

use_repo "$OUT/repo-v1"
P -Sy --noconfirm >"$OUT/s1.log" 2>&1
check '[ $? -ne 0 ]' "the repository is refused before its key is trusted"
pacman-key --gpgdir "$R/gnupg" --add "$OUT/repo-v1/polycall-archive.asc" >/dev/null 2>&1
pacman-key --gpgdir "$R/gnupg" --lsign-key "$FPR" >/dev/null 2>&1
cp -r "$OUT/repo-v1" "$OUT/repo-tampered"
for f in "$OUT/repo-tampered"/*.pkg.tar.zst; do printf x >> "$f"; done
use_repo "$OUT/repo-tampered"
P -Sy --noconfirm $PKG >"$OUT/s2.log" 2>&1
check '[ $? -ne 0 ] && [ ! -e "$R$MINGW_PREFIX/bin/polycall.exe" ]' "a package altered after signing is refused"
rm -rf "$OUT/repo-tampered" "$R/cache"/*

use_repo "$OUT/repo-v1"
P -Sy --noconfirm $PKG >"$OUT/i1.log" 2>&1
check '[ $? -eq 0 ]' "pacman -S $PKG (isolated root)"
B="$R$MINGW_PREFIX"
check '[ "$("$B/bin/polycall.exe" --version)" = "polycall $UPV" ]' "installed polycall.exe --version = $UPV"
check '[ -f "$B/bin/libpolycall.dll" ] && [ -f "$B/lib/libpolycall.dll.a" ] && [ -f "$B/lib/libpolycall.a" ] && [ -f "$B/include/polycall/polycall.h" ]' \
  "MinGW layout: bin/libpolycall.dll, lib/libpolycall.dll.a, lib/libpolycall.a, include/polycall"
check '[ ! -e "$B/lib/systemd" ] && [ ! -e "$R/usr/lib/systemd" ]' "no systemd unit in the MSYS2 package"
deps=$(objdump -p "$B/bin/libpolycall.dll" | awk '/DLL Name/{print tolower($3)}' | grep -v '^api-ms-win-\|^kernel32.dll$\|^ws2_32.dll$' || true)
check '[ -z "$deps" ]' "libpolycall.dll needs only system DLLs${deps:+ (also: $deps)}"

# relocated install: consumers find it through the relocatable .pc and CMake files
W="$OUT/consumer"; rm -rf "$W"; mkdir -p "$W"
cat > "$W/consumer.c" <<'EOF'
#include <polycall/polycall.h>
#include <stdio.h>
int main(void) {
    char v[32];
    if (polycall_ffi_abi_version() != 1) return 1;
    if (polycall_ffi_version(v, (int)sizeof v) <= 0) return 2;
    printf("%s\n", v);
    return 0;
}
EOF
( cd "$W" && PKG_CONFIG_PATH="$B/lib/pkgconfig" PKG_CONFIG_LIBDIR="$B/lib/pkgconfig" \
    gcc -Wall -Werror consumer.c $(PKG_CONFIG_PATH="$B/lib/pkgconfig" PKG_CONFIG_LIBDIR="$B/lib/pkgconfig" pkg-config --cflags --libs polycall) -o shared.exe &&
  PATH="$B/bin:$PATH" ./shared.exe ) > "$W/o1" 2>&1
check '[ "$(tr -d "\r" < "$W/o1")" = "$UPV" ]' "pkg-config consumer against the relocated root"
mkdir -p "$W/cm" && printf '%s\n' 'cmake_minimum_required(VERSION 3.20)' 'project(c LANGUAGES C)' \
  'find_package(polycall 1.1 REQUIRED)' 'add_executable(cm ../consumer.c)' \
  'target_link_libraries(cm PRIVATE polycall::polycall)' > "$W/cm/CMakeLists.txt"
( cd "$W/cm" && cmake -G Ninja -S . -B b -DCMAKE_PREFIX_PATH="$(cygpath -m "$B")" >/dev/null &&
  cmake --build b >/dev/null && PATH="$B/bin:$PATH" ./b/cm.exe ) > "$W/o2" 2>&1
check '[ "$(tr -d "\r" < "$W/o2")" = "$UPV" ]' "CMake find_package(polycall) consumer against the relocated root"

mkdir -p "$W/d" && printf 'network start\ndaemon_endpoint=127.0.0.1:0\nauth_token_env=POLYCALL_DEV_TOKEN\n' > "$W/d/Polycallfile"
( "$B/bin/polycall.exe" daemon start -t 10000 "$W/d/Polycallfile" && "$B/bin/polycall.exe" daemon status "$W/d/Polycallfile" &&
  "$B/bin/polycall.exe" daemon stop "$W/d/Polycallfile" && ! "$B/bin/polycall.exe" daemon status "$W/d/Polycallfile" ) > "$W/d.log" 2>&1
check '[ $? -eq 0 ] && grep -q "\"status\":\"ok\"" "$W/d.log"' "daemon start / status (health ok) / stop from the package"

use_repo "$OUT/repo-v2"
P -Syu --noconfirm >"$OUT/u1.log" 2>&1
V=$(P -Q $PKG 2>/dev/null | awk '{print $2}')
check '[ "$V" = "$UPV-2" ]' "pacman -Syu upgrades to $V"
check '[ "$(PATH="$B/bin:$PATH" "$W/shared.exe" | tr -d "\r")" = "$UPV" ]' "existing consumer runs against the upgraded DLL"
P -R --noconfirm $PKG >"$OUT/r1.log" 2>&1
check '[ $? -eq 0 ] && [ ! -e "$B/bin/polycall.exe" ] && [ ! -e "$B/bin/libpolycall.dll" ] && [ ! -d "$B/include/polycall" ]' \
  "pacman -R removes every packaged file"

AFTER=$(pacman -Q | sha256sum)
check '[ "$BEFORE" = "$AFTER" ] && [ ! -e "$MINGW_PREFIX/bin/polycall.exe" ]' \
  "the MSYS2 installation's own package set is unchanged"
rm -rf "$OUT/gnupg-signer"
echo "== MSYS2 $ENVN: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
