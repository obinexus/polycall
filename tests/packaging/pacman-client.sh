#!/bin/sh
# Clean-room pacman client test. Runs as root in a FRESH Arch Linux container
# with two signed repositories mounted read-only:
#   /repo-v1  polycall X.Y.Z-1        /repo-v2  X.Y.Z-2 (the upgrade)
# Each holds polycall.db (signed), signed packages and polycall-archive.asc.
# Usage: pacman-client.sh EXPECTED_UPSTREAM_VERSION KEY_FINGERPRINT
set -u
UPV=${1:?usage: pacman-client.sh X.Y.Z FPR}
FPR=${2:?usage: pacman-client.sh X.Y.Z FPR}
PASS=0; FAIL=0
ok()  { echo "PASS  $*"; PASS=$((PASS + 1)); }
bad() { echo "FAIL  $*"; FAIL=$((FAIL + 1)); }
check() { if eval "$1"; then ok "$2"; else bad "$2"; fi; }
echo "== client: $(. /etc/os-release; echo "$PRETTY_NAME")"

pacman-key --init >/dev/null 2>&1
pacman-key --populate archlinux >/dev/null 2>&1
pacman -Sy --noconfirm --needed gcc pkgconf cmake make >/dev/null 2>&1 || { echo "toolchain install failed"; exit 2; }
cp /etc/pacman.conf /etc/pacman.conf.orig
use_repo() {
  cp /etc/pacman.conf.orig /etc/pacman.conf
  printf '\n[polycall]\nSigLevel = Required DatabaseRequired\nServer = file://%s\n' "$1" >> /etc/pacman.conf
}

# ---- signatures are enforced ------------------------------------------------
use_repo /repo-v1
pacman -Sy --noconfirm >/tmp/s1 2>&1; rc=$?
check '[ $rc -ne 0 ] && grep -qi "signature\|unknown trust\|invalid or corrupted" /tmp/s1' \
  "the repository is refused before its key is trusted"
pacman-key --add /repo-v1/polycall-archive.asc >/dev/null 2>&1
pacman-key --lsign-key "$FPR" >/dev/null 2>&1
rm -rf /tmp/tampered && cp -a /repo-v1 /tmp/tampered
for f in /tmp/tampered/polycall-*.pkg.tar.zst; do printf 'x' >> "$f"; done
use_repo /tmp/tampered
pacman -Sy --noconfirm polycall >/tmp/s2 2>&1; rc=$?
check '[ $rc -ne 0 ] && ! command -v polycall >/dev/null' "a package altered after signing is refused"
rm -rf /tmp/tampered /var/cache/pacman/pkg/polycall-*

# ---- install ------------------------------------------------------------------
use_repo /repo-v1
pacman -Sy --noconfirm polycall >/tmp/i1 2>&1; rc=$?
check '[ $rc -eq 0 ]' "pacman -S polycall"
V=$(pacman -Q polycall 2>/dev/null | awk '{print $2}')
check '[ "$V" = "$UPV-1" ]' "polycall $V installed from repo-v1"
check '[ "$(polycall --version)" = "polycall $UPV" ]' "polycall --version = $UPV"
check '[ -f /usr/lib/systemd/system/polycall.service ]' "systemd unit installed"
check 'pacman -Qkk polycall >/dev/null 2>&1' "pacman -Qkk: installed files match the package"
check '[ -f /usr/lib/libpolycall.so.1 ] && [ -f /usr/lib/libpolycall.a ] && [ -f /usr/include/polycall/polycall.h ]' \
  "library, static archive and headers installed"

id tester >/dev/null 2>&1 || useradd -m tester
su tester -c 'set -e; mkdir -p ~/d && sed "s/^daemon_endpoint=.*/daemon_endpoint=127.0.0.1:0/" /etc/polycall/Polycallfile > ~/d/Polycallfile
  polycall daemon start -t 10000 ~/d/Polycallfile && polycall daemon status ~/d/Polycallfile &&
  polycall daemon stop ~/d/Polycallfile && ! polycall daemon status ~/d/Polycallfile' >/tmp/d1 2>&1; rc=$?
check '[ $rc -eq 0 ] && grep -q "\"status\":\"ok\"" /tmp/d1' \
  "non-root: daemon start / status (health ok) / stop / status=not running"
[ $rc -eq 0 ] || cat /tmp/d1

mkdir -p /tmp/c && cat > /tmp/c/consumer.c <<'EOF'
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
( cd /tmp/c && gcc -Wall -Werror consumer.c $(pkg-config --cflags --libs polycall) -o shared && ./shared ) >/tmp/c/o1 2>&1
check '[ "$(cat /tmp/c/o1)" = "$UPV" ]' "pkg-config consumer builds and runs"
mkdir -p /tmp/cm && printf '%s\n' 'cmake_minimum_required(VERSION 3.20)' 'project(c LANGUAGES C)' \
  'find_package(polycall 1.1 REQUIRED)' 'add_executable(cm ../c/consumer.c)' \
  'target_link_libraries(cm PRIVATE polycall::polycall)' > /tmp/cm/CMakeLists.txt
( cd /tmp/cm && cmake -S . -B b >/dev/null && cmake --build b >/dev/null && ./b/cm ) >/tmp/cm/o 2>&1
check '[ "$(cat /tmp/cm/o)" = "$UPV" ]' "CMake find_package(polycall) consumer"

# ---- upgrade with a locally modified backup file ----------------------------
echo "# local admin change" >> /etc/polycall/Polycallfile
use_repo /repo-v2
pacman -Syu --noconfirm >/tmp/u1 2>&1; rc=$?
V=$(pacman -Q polycall 2>/dev/null | awk '{print $2}')
check '[ $rc -eq 0 ] && [ "$V" = "$UPV-2" ]' "pacman -Syu upgrades to $V"
check 'grep -q "local admin change" /etc/polycall/Polycallfile' "the admin's backup-file change survives the upgrade"
check '[ "$(/tmp/c/shared)" = "$UPV" ]' "existing consumer still runs after the upgrade"

# ---- removal -------------------------------------------------------------------
pacman -R --noconfirm polycall >/dev/null 2>&1
check '[ ! -e /usr/bin/polycall ] && [ ! -e /usr/lib/libpolycall.so.1 ] && [ -f /etc/polycall/Polycallfile.pacsave ]' \
  "pacman -R: files removed, modified config saved as .pacsave"
rm -f /etc/polycall/Polycallfile.pacsave
pacman -S --noconfirm polycall >/dev/null 2>&1 && pacman -Rn --noconfirm polycall >/dev/null 2>&1
check '[ ! -e /etc/polycall/Polycallfile ] && ! pacman -Q polycall >/dev/null 2>&1' \
  "pacman -Rn: package and its configuration removed"

echo "== Arch Linux: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
