#!/bin/sh
# Clean-room APT client test. Runs as root in a FRESH Debian/Ubuntu container
# with two signed repositories mounted read-only:
#   /repo-v1  polycall X.Y.Z-1
#   /repo-v2  the same plus X.Y.Z-2 (the upgrade)
# Checks: signature enforcement, `apt install polycall`, the CLI, the daemon
# as a non-root user, the systemd unit is installed but not enabled, a C
# consumer through pkg-config (shared and static) and CMake, upgrade with a
# locally modified conffile, remove, purge.
# Usage: apt-client.sh EXPECTED_UPSTREAM_VERSION
set -u
UPV=${1:?usage: apt-client.sh X.Y.Z}
PASS=0; FAIL=0
ok()  { echo "PASS  $*"; PASS=$((PASS + 1)); }
bad() { echo "FAIL  $*"; FAIL=$((FAIL + 1)); }
check() { if eval "$1"; then ok "$2"; else bad "$2"; fi; }
export DEBIAN_FRONTEND=noninteractive
. /etc/os-release
echo "== client: $PRETTY_NAME"

KR=/usr/share/keyrings/polycall-archive-keyring.gpg
LIST=/etc/apt/sources.list.d/polycall.list
ONLY="-o Dir::Etc::sourcelist=$LIST -o Dir::Etc::sourceparts=- -o APT::Get::List-Cleanup=0"
apt-get update -qq >/dev/null 2>&1 || { echo "cannot reach the distribution mirrors"; exit 2; }
apt-get install -y -qq gcc libc6-dev pkg-config cmake make >/dev/null 2>&1 || { echo "toolchain install failed"; exit 2; }
install -m 0644 /repo-v1/polycall-archive-keyring.gpg "$KR"

# ---- the repository signature is enforced ---------------------------------
rm -rf /tmp/tampered && cp -a /repo-v1 /tmp/tampered
sed -i 's/^Description: Polycall command line/Description: TAMPERED command line/' \
  /tmp/tampered/dists/stable/main/binary-*/Packages
gzip -9nf -k /tmp/tampered/dists/stable/main/binary-*/Packages
echo "deb [signed-by=$KR] file:/tmp/tampered stable main" > $LIST
apt-get update $ONLY >/tmp/u1 2>&1; rc=$?
check '[ $rc -ne 0 ] || grep -qi "hash sum mismatch\|File has unexpected size" /tmp/u1' \
  "a Packages index altered after signing is rejected"
rm -rf /tmp/tampered && cp -a /repo-v1 /tmp/tampered
sed -i 's/^Label: polycall$/Label: tampered/' /tmp/tampered/dists/stable/InRelease
rm -f /tmp/tampered/dists/stable/Release.gpg
apt-get update $ONLY >/tmp/u2 2>&1; rc=$?
check '[ $rc -ne 0 ] && grep -qi "signature\|BADSIG\|not signed\|invalid" /tmp/u2' \
  "an InRelease whose signed text was altered is rejected"
OTHER=$(ls /usr/share/keyrings/*archive-keyring.gpg 2>/dev/null | grep -v polycall | head -1)
echo "deb [signed-by=$OTHER] file:/repo-v1 stable main" > $LIST
apt-get update $ONLY >/tmp/u3 2>&1; rc=$?
check '[ $rc -ne 0 ] && grep -qi "NO_PUBKEY\|not signed\|signature" /tmp/u3' \
  "the repository is refused when trusted only with a different key"
rm -rf /tmp/tampered /var/lib/apt/lists/_tmp_tampered_* /var/lib/apt/lists/_repo-v1_*

# ---- install from the signed repository ------------------------------------
echo "deb [signed-by=$KR] file:/repo-v1 stable main" > $LIST
apt-get update >/tmp/u4 2>&1; rc=$?
check '[ $rc -eq 0 ] && ! grep -qi "^W:.*polycall\|^E:" /tmp/u4' "apt update verifies the signed repository"
apt-get install -y polycall >/tmp/i1 2>&1; rc=$?
check '[ $rc -eq 0 ]' "apt install polycall"
V=$(dpkg-query -W -f='${Version}' polycall 2>/dev/null)
check '[ "$V" = "$UPV-1" ]' "polycall $V installed from repo-v1"
check '[ "$(polycall --version)" = "polycall $UPV" ]' "polycall --version = $UPV"
check 'polycall --help >/dev/null 2>&1' "polycall --help"
check '[ -f /usr/lib/systemd/system/polycall.service ] || [ -f /lib/systemd/system/polycall.service ]' \
  "systemd unit installed"
check '[ ! -e /etc/systemd/system/multi-user.target.wants/polycall.service ]' \
  "unit not enabled on install"
check 'dpkg-query -W -f="\${Conffiles}" polycall | grep -q "/etc/polycall/Polycallfile"' \
  "/etc/polycall/Polycallfile is a conffile"
check '! dpkg -s libpolycall-dev >/dev/null 2>&1' "the CLI does not pull in the -dev package"

# ---- daemon as a non-root user ---------------------------------------------
id tester >/dev/null 2>&1 || useradd -m tester
su tester -c 'set -e; mkdir -p ~/d && sed "s/^daemon_endpoint=.*/daemon_endpoint=127.0.0.1:0/" /etc/polycall/Polycallfile > ~/d/Polycallfile
  polycall daemon start -t 10000 ~/d/Polycallfile && polycall daemon status ~/d/Polycallfile &&
  polycall daemon stop ~/d/Polycallfile && ! polycall daemon status ~/d/Polycallfile' >/tmp/d1 2>&1; rc=$?
check '[ $rc -eq 0 ] && grep -q "\"status\":\"ok\"" /tmp/d1' \
  "non-root: daemon start / status (health ok) / stop / status=not running"
[ $rc -eq 0 ] || cat /tmp/d1

# ---- development package: pkg-config and CMake consumers -------------------
apt-get install -y libpolycall-dev >/tmp/i2 2>&1; rc=$?
check '[ $rc -eq 0 ]' "apt install libpolycall-dev"
check '[ "$(pkg-config --modversion polycall)" = "$UPV" ]' "pkg-config --modversion polycall = $UPV"
mkdir -p /tmp/c && cat > /tmp/c/consumer.c <<'EOF'
#include <polycall/polycall.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    char v[32];
    if (polycall_ffi_abi_version() != 1) return 1;
    if (polycall_ffi_version(v, (int)sizeof v) <= 0) return 2;
    if (strcmp(polycall_strerror(POLYCALL_OK), "") == 0) return 3;
    printf("%s\n", v);
    return 0;
}
EOF
( cd /tmp/c && gcc -Wall -Werror consumer.c $(pkg-config --cflags --libs polycall) -o shared && ./shared ) >/tmp/c/o1 2>&1
check '[ "$(cat /tmp/c/o1)" = "$UPV" ]' "pkg-config shared consumer builds and runs"
LIBP=$(ldd /tmp/c/shared | awk '$1 == "libpolycall.so.1" {print $3}')
check '[ -n "$LIBP" ] && dpkg -S "$(readlink -f "$LIBP")" 2>/dev/null | grep -q "^libpolycall1:"' \
  "consumer resolves libpolycall.so.1 to the file owned by libpolycall1 ($LIBP)"
( cd /tmp/c && gcc -Wall -Werror consumer.c -static-libgcc $(pkg-config --static --cflags polycall) \
    /usr/lib/*/libpolycall.a $(pkg-config --static --libs-only-l polycall | sed 's/-lpolycall//') -o static && ./static ) >/tmp/c/o2 2>&1
check '[ "$(cat /tmp/c/o2)" = "$UPV" ] && ! ldd /tmp/c/static | grep -q libpolycall' "static consumer (libpolycall.a)"
mkdir -p /tmp/cm && cat > /tmp/cm/CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.20)
project(c LANGUAGES C)
find_package(polycall 1.1 REQUIRED)
add_executable(cmconsumer ../c/consumer.c)
target_link_libraries(cmconsumer PRIVATE polycall::polycall)
EOF
( cd /tmp/cm && cmake -S . -B b >/dev/null && cmake --build b >/dev/null && ./b/cmconsumer ) >/tmp/cm/o 2>&1
check '[ "$(cat /tmp/cm/o)" = "$UPV" ]' "CMake find_package(polycall) consumer"

# ---- upgrade with a locally modified conffile -------------------------------
echo "# local admin change" >> /etc/polycall/Polycallfile
echo "deb [signed-by=$KR] file:/repo-v2 stable main" > $LIST
apt-get update >/dev/null 2>&1
apt-get -y -o Dpkg::Options::=--force-confold upgrade >/tmp/up 2>&1; rc=$?
check '[ $rc -eq 0 ]' "apt upgrade"
for p in polycall libpolycall1 libpolycall-dev; do
  V=$(dpkg-query -W -f='${Version}' $p 2>/dev/null)
  check '[ "$V" = "$UPV-2" ]' "$p upgraded to $V"
done
check 'grep -q "local admin change" /etc/polycall/Polycallfile' "the admin's conffile change survives the upgrade"
check '[ "$(/tmp/c/shared)" = "$UPV" ]' "existing consumer still runs after the library upgrade"

# ---- remove and purge --------------------------------------------------------
apt-get remove -y polycall >/dev/null 2>&1
check '[ ! -e /usr/bin/polycall ] && [ -f /etc/polycall/Polycallfile ]' "remove: binary gone, conffile kept"
apt-get purge -y polycall >/dev/null 2>&1
check '[ ! -e /etc/polycall/Polycallfile ] && ! dpkg -s polycall >/dev/null 2>&1' "purge: conffile removed"
apt-get purge -y libpolycall-dev libpolycall1 >/dev/null 2>&1
check '! ls /usr/lib/*/libpolycall.so* >/dev/null 2>&1 && [ ! -d /usr/include/polycall ]' \
  "purge libpolycall1/-dev: library and headers removed"

echo "== $PRETTY_NAME: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
