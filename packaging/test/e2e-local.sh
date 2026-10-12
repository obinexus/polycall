#!/bin/sh
# End-to-end test of the APT repository workflow without publishing anything:
#
#   1. sign a repository built from freshly built packages with a throw-away
#      key (packaging/apt/update-repo.sh -- the same script releases use)
#   2. serve it over HTTPS from an nginx container (throw-away test CA)
#   3. in a pristine container of the target release, follow the README:
#      fetch + verify the key, add the Signed-By source, apt update,
#      apt install polycall, run the CLI smoke test, build against
#      libpolycall-dev (packaging/test/client-check.sh)
#   4. publish a newer (repacked) version, apt upgrade, smoke test again,
#      downgrade/upgrade path, then remove, purge and drop the repository
#
# usage: e2e-local.sh --dist DIR --suite CODENAME [--arch ARCH] [--keep]
#   --dist DIR   packaging/deb/build.sh output (DIR/<codename>/<arch>/*.deb)
#   --arch ARCH  Debian architecture of the client (default: docker host's)
#   --keep       leave the containers, volume and network for inspection
#
# Needs only a docker CLI (no bind mounts, so it also works against a remote
# or Docker Desktop daemon).
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"

DIST='' SUITE='' ARCH='' KEEP=''
while [ $# -gt 0 ]; do
  case "$1" in
    --dist) DIST=$2; shift 2 ;;
    --suite) SUITE=$2; shift 2 ;;
    --arch) ARCH=$2; shift 2 ;;
    --keep) KEEP=1; shift ;;
    -h|--help) sed -n '2,22p' "$0"; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done
[ -n "$DIST" ] && [ -n "$SUITE" ] || die "--dist and --suite are required"
IMAGE=$(suite_field "$SUITE" image) || die "unknown release $SUITE"
if [ -z "$ARCH" ]; then
  case "$(docker version -f '{{.Server.Arch}}')" in
    amd64|x86_64) ARCH=amd64 ;; arm64|aarch64) ARCH=arm64 ;; *) die "set --arch" ;;
  esac
fi
[ -d "$DIST/$SUITE/$ARCH" ] || die "no packages in $DIST/$SUITE/$ARCH (run packaging/deb/build.sh first)"
TOOLS_IMAGE=debian:13
WEB_IMAGE=nginx:1.28
HOST=apt.polycall.test
URL="https://$HOST/apt"

ID="pce2e-$SUITE-$ARCH-$$"
NET=$ID-net WWW=$ID-www TOOLS=$ID-tools WEB=$ID-web CLIENT=$ID-client
TMP=$(mktemp -d)
cleanup() {
  rc=$?
  if [ -n "$KEEP" ]; then info "kept $TOOLS $WEB $CLIENT, volume $WWW, network $NET"
  else
    docker rm -f "$TOOLS" "$WEB" "$CLIENT" >/dev/null 2>&1 || true
    docker volume rm "$WWW" >/dev/null 2>&1 || true
    docker network rm "$NET" >/dev/null 2>&1 || true
  fi
  rm -rf "$TMP"
  exit $rc
}
trap cleanup EXIT INT TERM
step() { printf '\n######## %s\n' "$*"; }

step "network, volume, tools container ($TOOLS_IMAGE)"
docker network create "$NET" >/dev/null
docker volume create "$WWW" >/dev/null
docker run -d --name "$TOOLS" --network "$NET" -v "$WWW:/srv/www" "$TOOLS_IMAGE" sleep infinity >/dev/null
docker exec "$TOOLS" sh -c 'export DEBIAN_FRONTEND=noninteractive
  apt-get update -qq && apt-get install -y -qq --no-install-recommends \
    apt-utils gnupg gpgv xz-utils openssl ca-certificates dpkg >/dev/null
  mkdir -p /work/src /work/incoming /srv/www/site /srv/www/tls'
( cd "$REPO_ROOT" && tar -cf - packaging include debian tests/consumers README.md ) \
  | docker cp - "$TOOLS:/work/src"
( cd "$DIST" && tar -cf - "$SUITE/$ARCH" ) | docker cp - "$TOOLS:/work/incoming"

step "throw-away signing key and TLS test CA"
docker exec "$TOOLS" sh -euc '
  export GNUPGHOME=/work/gnupg; mkdir -m 0700 -p "$GNUPGHOME"
  gpg --batch --pinentry-mode loopback --passphrase "" \
    --quick-generate-key "polycall e2e throw-away key <e2e@polycall.invalid>" ed25519 sign 1d 2>/dev/null
  gpg --batch --with-colons --list-secret-keys | awk -F: "\$1 == \"fpr\" { print \$10; exit }" >/work/fpr
  cd /srv/www/tls
  openssl req -x509 -newkey rsa:2048 -nodes -days 2 -subj "/CN=polycall e2e test CA" \
    -keyout ca.key -out ca.crt 2>/dev/null
  printf "subjectAltName=DNS:'"$HOST"'\nextendedKeyUsage=serverAuth\nbasicConstraints=CA:FALSE\n" >ext.cnf
  openssl req -newkey rsa:2048 -nodes -subj "/CN='"$HOST"'" -keyout server.key -out server.csr 2>/dev/null
  openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -days 2 \
    -extfile ext.cnf -out server.crt 2>/dev/null'
FPR=$(docker exec "$TOOLS" cat /work/fpr)
info "throw-away key $FPR"

step "signed repository from the built packages"
docker exec "$TOOLS" sh -euc "export GNUPGHOME=/work/gnupg
  sh /work/src/packaging/apt/update-repo.sh --repo /srv/www/site/apt \
    --incoming /work/incoming --key $FPR --url $URL
  find /srv/www/site/apt -type f | sort"
VERSION=$(docker exec "$TOOLS" sh -c "dpkg-deb -f /work/incoming/$SUITE/$ARCH/polycall_*.deb Version")
info "published polycall $VERSION for $SUITE/$ARCH"

step "HTTPS server ($WEB_IMAGE) as $HOST"
cat >"$TMP/default.conf" <<EOF
server {
    listen 443 ssl;
    server_name $HOST;
    ssl_certificate     /srv/www/tls/server.crt;
    ssl_certificate_key /srv/www/tls/server.key;
    root /srv/www/site;
    location / { autoindex on; }
}
EOF
docker create --name "$WEB" --network "$NET" --network-alias "$HOST" \
  -v "$WWW:/srv/www:ro" "$WEB_IMAGE" >/dev/null
( cd "$TMP" && tar -cf - default.conf ) | docker cp - "$WEB:/etc/nginx/conf.d"
docker start "$WEB" >/dev/null

step "pristine client: $IMAGE ($ARCH)"
docker run -d --platform "linux/$ARCH" --name "$CLIENT" --network "$NET" "$IMAGE" sleep infinity >/dev/null
docker exec "$CLIENT" mkdir -p /src
( cd "$REPO_ROOT" && tar -cf - packaging/test debian/tests tests/consumers ) | docker cp - "$CLIENT:/src"
docker exec "$TOOLS" cat /srv/www/tls/ca.crt | docker exec -i "$CLIENT" sh -c 'cat >/tmp/polycall-e2e-ca.crt'

client() {
  docker exec -e POLYCALL_APT_URL="$URL" -e POLYCALL_APT_FINGERPRINT="$FPR" \
    -e POLYCALL_TEST_CA=/tmp/polycall-e2e-ca.crt -e POLYCALL_SRC=/src \
    -e POLYCALL_EXPECT_VERSION="$VERSION" "$@"
}
client "$CLIENT" sh /src/packaging/test/client-check.sh setup install dev

step "publish a newer version and upgrade"
NEXT="$VERSION+e2e1"
docker exec "$TOOLS" sh -euc "export GNUPGHOME=/work/gnupg
  sh /work/src/packaging/test/repack-version.sh /work/incoming/$SUITE/$ARCH /work/next/$SUITE/$ARCH e2e1
  sh /work/src/packaging/apt/update-repo.sh --repo /srv/www/site/apt \
    --incoming /work/next --key $FPR --url $URL"
client -e POLYCALL_EXPECT_UPGRADE="$NEXT" "$CLIENT" sh /src/packaging/test/client-check.sh upgrade oldest-first remove

step "PASS: $SUITE/$ARCH -- install, libraries, upgrade, downgrade+upgrade, remove, purge"
