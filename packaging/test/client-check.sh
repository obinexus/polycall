#!/bin/sh
# shellcheck disable=SC2032,SC2033,SC2024  # README-verbatim "sudo apt" via shell-function shims (see below)
# End-to-end check of the polycall APT repository from a clean Debian/Ubuntu
# system, following the README instructions command for command.
#
# usage: client-check.sh PHASE...
#   setup         install prerequisites, fetch + verify the key, add the source
#   install       apt update; apt install polycall; CLI smoke test
#   dev           apt install libpolycall-dev; build/run library consumers
#   upgrade       apt update; apt upgrade; expect POLYCALL_EXPECT_UPGRADE
#   oldest-first  if the repository holds older versions, install the oldest
#                 one, then upgrade to the newest (real upgrade path)
#   remove        apt remove/purge + autoremove; repository removal
#   all           setup install dev oldest-first remove
#
# environment:
#   POLYCALL_APT_URL          repository URL (default: the published one)
#   POLYCALL_APT_FINGERPRINT  expected signing-key fingerprint (required)
#   POLYCALL_EXPECT_VERSION   Debian version that `apt install` must install
#   POLYCALL_EXPECT_UPGRADE   Debian version that `upgrade` must reach
#   POLYCALL_TEST_CA          extra CA certificate to trust (local test server)
#   POLYCALL_WAIT_SECONDS     wait up to N s for POLYCALL_EXPECT_VERSION to be
#                             published before installing (default 0)
#   POLYCALL_SRC              source tree with debian/tests and tests/consumers
#                             (default: the tree this script is in)
set -eu

APT_URL=${POLYCALL_APT_URL:-https://obinexus.github.io/polycall/apt}
FPR=${POLYCALL_APT_FINGERPRINT:?set POLYCALL_APT_FINGERPRINT}
SRC=${POLYCALL_SRC:-$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd)}
KEYRING=/etc/apt/keyrings/polycall-archive-keyring.gpg
SOURCES=/etc/apt/sources.list.d/polycall.sources
export DEBIAN_FRONTEND=noninteractive
LOG=$(mktemp)

step() { printf '\n===== %s\n' "$*"; }
pass() { printf 'ok - %s\n' "$*"; }
fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }

# Runs as root in a throw-away container. The README's commands are kept
# verbatim (sudo apt ...): `sudo` runs them directly and `apt` answers yes,
# which is all that differs from an interactive session.
[ "$(id -u)" -eq 0 ] || fail "run as root in a disposable container"
sudo() { "$@"; }
apt() { command apt -y -o Dpkg::Use-Pty=0 "$@"; }

installed_version() { dpkg-query -W -f '${Version}' "$1" 2>/dev/null || true; }
status_of() { dpkg-query -W -f '${db:Status-Abbrev}' "$1" 2>/dev/null | tr -d ' ' || true; }

apt_update_clean() {
  sudo apt update >"$LOG" 2>&1 || { cat "$LOG"; fail "apt update failed"; }
  cat "$LOG"
  host=$(printf '%s' "$APT_URL" | sed 's#^[a-z]*://\([^/]*\).*#\1#')
  if grep -E '^(W|E|Err):' "$LOG" | grep -F "$host" >/dev/null; then
    fail "apt update reported problems for $host"
  fi
  if grep -E 'NO_PUBKEY|not signed|BADSIG|EXPKEYSIG|insecure|InRelease.*is not valid' "$LOG" >/dev/null; then
    fail "signature problem during apt update"
  fi
}

phase_setup() {
  step "prerequisites (README: one-time setup)"
  # shellcheck source=/dev/null
  . /etc/os-release
  echo "client: $PRETTY_NAME ($(dpkg --print-architecture)), apt $(dpkg-query -W -f '${Version}' apt)"
  sudo apt update
  sudo apt install ca-certificates curl gnupg
  if [ -n "${POLYCALL_TEST_CA:-}" ]; then
    # local HTTPS test server only; the published repository uses a public CA
    cp "$POLYCALL_TEST_CA" /usr/local/share/ca-certificates/polycall-e2e-ca.crt
    update-ca-certificates >/dev/null
  fi
  if apt-cache show polycall >/dev/null 2>&1; then
    fail "polycall is already available before adding the repository: $(apt-cache policy polycall | tr -s ' ' | head -3)"
  fi
  pass "polycall is not provided by the $PRETTY_NAME archive itself"

  if [ -n "${POLYCALL_WAIT_SECONDS:-}" ] && [ -n "${POLYCALL_EXPECT_VERSION:-}" ]; then
    step "waiting for $POLYCALL_EXPECT_VERSION to be published for $VERSION_CODENAME"
    deadline=$(( $(date +%s) + POLYCALL_WAIT_SECONDS ))
    arch=$(dpkg --print-architecture)
    until curl -fsS "$APT_URL/dists/$VERSION_CODENAME/main/binary-$arch/Packages" 2>/dev/null \
            | grep -qx "Version: $POLYCALL_EXPECT_VERSION"; do
      [ "$(date +%s)" -lt "$deadline" ] || fail "$POLYCALL_EXPECT_VERSION not published within ${POLYCALL_WAIT_SECONDS}s"
      sleep 20
    done
    pass "$POLYCALL_EXPECT_VERSION is listed"
  fi

  step "signing key (README step: dedicated keyring)"
  sudo install -d -m 0755 /etc/apt/keyrings
  curl -fsSL "$APT_URL/polycall-archive-keyring.gpg" | sudo tee "$KEYRING" > /dev/null
  gpg --show-keys "$KEYRING"
  got=$(gpg --batch --show-keys --with-colons "$KEYRING" | awk -F: '$1 == "fpr" { print $10; exit }')
  [ "$(gpg --batch --show-keys --with-colons "$KEYRING" | grep -c '^pub:')" -eq 1 ] \
    || fail "the keyring must contain exactly one key"
  [ "$got" = "$FPR" ] || fail "key fingerprint $got does not match the documented $FPR"
  pass "keyring fingerprint matches $FPR"

  step "APT source (README step: Signed-By)"
  sudo tee "$SOURCES" > /dev/null <<EOF
Types: deb
URIs: $APT_URL
Suites: $(. /etc/os-release && echo "$VERSION_CODENAME")
Components: main
Signed-By: $KEYRING
EOF
  cat "$SOURCES"
  apt_update_clean
  apt-cache policy polycall
  apt-cache policy polycall | grep -F "$APT_URL" >/dev/null || fail "polycall candidate does not come from $APT_URL"
  pass "repository configured; polycall resolvable by name"

  step "negative check: the same repository with a different Signed-By key must be rejected"
  tmpkey=$(mktemp -d)
  chmod 0755 "$tmpkey"   # apt verifies as the unprivileged _apt user
  GNUPGHOME=$tmpkey/gnupg
  mkdir -m 0700 "$GNUPGHOME"
  GNUPGHOME=$GNUPGHOME gpg --batch --pinentry-mode loopback --passphrase '' \
    --quick-generate-key "impostor <nobody@invalid>" ed25519 sign 0 >/dev/null 2>&1
  GNUPGHOME=$GNUPGHOME gpg --batch --export >"$tmpkey/wrong.gpg"
  chmod 0644 "$tmpkey/wrong.gpg"
  sed "s#^Signed-By: .*#Signed-By: $tmpkey/wrong.gpg#" "$SOURCES" >"$tmpkey/wrong.sources"
  # fresh lists dir: no previously verified index to fall back on
  mkdir -p "$tmpkey/lists/partial"
  rc=0
  apt-get update -o Dir::Etc::sourcelist="$tmpkey/wrong.sources" \
    -o Dir::Etc::sourceparts=/nonexistent -o Dir::State::Lists="$tmpkey/lists" >"$LOG" 2>&1 || rc=$?
  GNUPGHOME=$GNUPGHOME gpgconf --kill all >/dev/null 2>&1 || true
  if [ "$rc" -eq 0 ] || ! grep -E 'NO_PUBKEY|Missing key|is not signed' "$LOG"; then
    cat "$LOG"; rm -rf "$tmpkey"
    fail "apt did not reject the repository when Signed-By names another key (exit $rc)"
  fi
  if grep -q 'Permission denied' "$LOG"; then cat "$LOG"; rm -rf "$tmpkey"; fail "negative check could not read its keyring"; fi
  [ -z "$(find "$tmpkey/lists" -name '*_Packages*' -print -quit)" ] || { rm -rf "$tmpkey"; fail "an unverified Packages index was stored"; }
  rm -rf "$tmpkey"
  pass "a Signed-By keyring without the signing key is refused (apt-get update exit $rc)"
  apt_update_clean >/dev/null
}

phase_install() {
  step "install (README: sudo apt install polycall)"
  sudo apt update
  sudo apt install polycall
  v=$(installed_version polycall)
  [ -n "$v" ] || fail "polycall not installed"
  if [ -n "${POLYCALL_EXPECT_VERSION:-}" ]; then
    [ "$v" = "$POLYCALL_EXPECT_VERSION" ] || fail "installed polycall $v, expected $POLYCALL_EXPECT_VERSION"
  fi
  pass "polycall $v installed"
  apt-get check >/dev/null || fail "apt-get check reports broken dependencies"
  dpkg-query -W -f 'Depends: ${Depends}\n' polycall
  pass "dependencies satisfied"
  [ -f /usr/share/doc/polycall/copyright ] || fail "no copyright file"
  if grep -qs '^path-exclude=/usr/share/man' /etc/dpkg/dpkg.cfg.d/*; then
    # minimized images (e.g. Ubuntu's) never unpack man pages; check the package
    dl=$(mktemp -d); chown _apt "$dl" 2>/dev/null || true
    ( cd "$dl" && apt-get download -q "polycall=$v" >/dev/null )
    dpkg-deb --contents "$dl"/polycall_*.deb | grep -q '\./usr/share/man/man1/polycall\.1\.gz$' \
      || fail "the polycall package ships no man page"
    rm -rf "$dl"
    pass "copyright installed; man page shipped (this image excludes /usr/share/man)"
  else
    [ -f /usr/share/man/man1/polycall.1.gz ] || fail "no man page"
    pass "copyright and man page installed"
  fi
  POLYCALL_EXPECT_VERSION=${v%%-*} sh "$SRC/debian/tests/cli-smoke"
}

phase_dev() {
  step "development files (README: sudo apt install libpolycall-dev)"
  sudo apt install libpolycall-dev build-essential cmake
  [ "$(installed_version libpolycall1)" = "$(installed_version libpolycall-dev)" ] \
    || fail "libpolycall1 and libpolycall-dev versions differ"
  apt-mark showauto | grep -qx libpolycall1 || fail "libpolycall1 was not pulled in automatically as a dependency"
  pass "libpolycall-dev pulled in libpolycall1 $(installed_version libpolycall1)"
  ( cd "$SRC" && sh debian/tests/library-consumers )
}

phase_upgrade() {
  want=${POLYCALL_EXPECT_UPGRADE:?set POLYCALL_EXPECT_UPGRADE}
  step "upgrade to $want (README: sudo apt update && sudo apt upgrade)"
  before=$(installed_version polycall)
  sudo apt update
  sudo apt upgrade
  for p in polycall libpolycall1 libpolycall-dev; do
    [ -z "$(installed_version $p)" ] && continue
    [ "$(installed_version $p)" = "$want" ] || fail "$p is $(installed_version $p) after upgrade, expected $want"
  done
  pass "upgraded $before -> $want"
  POLYCALL_EXPECT_VERSION=${want%%-*} sh "$SRC/debian/tests/cli-smoke"
  if [ -n "$(installed_version libpolycall-dev)" ]; then ( cd "$SRC" && sh debian/tests/library-consumers ); fi
}

phase_oldest_first() {
  versions=$(apt-cache madison polycall | awk -F'|' '{ gsub(/ /, "", $2); print $2 }' | sort -u)
  n=$(printf '%s\n' "$versions" | grep -c . || true)
  newest=$(apt-cache policy polycall | awk '/Candidate:/ { print $2 }')
  if [ "$n" -lt 2 ]; then
    step "upgrade path: only $newest is published for this release; nothing older to upgrade from"
    return 0
  fi
  oldest=$(for v in $versions; do printf '%s\n' "$v"; done | while read -r v; do
    lower=1
    for w in $versions; do dpkg --compare-versions "$w" lt "$v" && lower=0; done
    [ $lower = 1 ] && echo "$v"
  done | head -1)
  step "upgrade path: install the oldest published version $oldest, then upgrade to $newest"
  sudo apt install --allow-downgrades "polycall=$oldest"
  [ "$(installed_version polycall)" = "$oldest" ] || fail "could not install polycall=$oldest"
  POLYCALL_EXPECT_VERSION=${oldest%%-*} sh "$SRC/debian/tests/cli-smoke" >/dev/null
  pass "polycall $oldest installed and working"
  step "targeted upgrade (README: sudo apt install --only-upgrade polycall)"
  sudo apt update
  sudo apt install --only-upgrade polycall
  [ "$(installed_version polycall)" = "$newest" ] || fail "polycall is $(installed_version polycall) after --only-upgrade, expected $newest"
  pass "upgraded $oldest -> $newest"
  POLYCALL_EXPECT_VERSION=${newest%%-*} sh "$SRC/debian/tests/cli-smoke"
}

phase_remove() {
  step "remove (README: sudo apt remove polycall)"
  sudo apt remove polycall
  [ ! -e /usr/bin/polycall ] || fail "/usr/bin/polycall still present after remove"
  case "$(status_of polycall)" in ""|un|rc|pn) ;; *) fail "polycall status after remove: $(status_of polycall)" ;; esac
  pass "polycall removed"

  step "purge (README: sudo apt purge polycall)"
  sudo apt purge polycall
  case "$(status_of polycall)" in ""|un|pn) ;; *) fail "polycall status after purge: $(status_of polycall)" ;; esac
  for f in /usr/share/doc/polycall /usr/share/man/man1/polycall.1.gz; do
    [ ! -e "$f" ] || fail "$f left behind after purge"
  done
  pass "polycall purged; no files left behind"

  if [ -n "$(installed_version libpolycall1)" ]; then
    step "purge the libraries (README: sudo apt purge libpolycall-dev; sudo apt autoremove)"
    sudo apt purge libpolycall-dev
    sudo apt autoremove --purge
    for p in libpolycall1 libpolycall-dev; do
      case "$(status_of $p)" in ""|un|pn) ;; *) fail "$p status after purge/autoremove: $(status_of $p)" ;; esac
    done
    if /sbin/ldconfig -p | grep -q libpolycall; then fail "libpolycall still in the ld.so cache"; fi
    pass "libraries removed; autoremove cleaned up libpolycall1"
  fi

  step "remove the repository (README: uninstall)"
  sudo rm /etc/apt/sources.list.d/polycall.sources /etc/apt/keyrings/polycall-archive-keyring.gpg
  sudo apt update
  if apt-cache show polycall >/dev/null 2>&1; then fail "polycall still resolvable after removing the repository"; fi
  pass "repository removed cleanly"
}

[ $# -gt 0 ] || { sed -n '2,26p' "$0"; exit 2; }
for phase in "$@"; do
  case "$phase" in
    setup) phase_setup ;;
    install) phase_install ;;
    dev) phase_dev ;;
    upgrade) phase_upgrade ;;
    oldest-first) phase_oldest_first ;;
    remove) phase_remove ;;
    all) phase_setup; phase_install; phase_dev; phase_oldest_first; phase_remove ;;
    *) fail "unknown phase: $phase" ;;
  esac
done
printf '\nclient-check: %s passed\n' "$*"
