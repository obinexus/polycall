#!/bin/sh
# End-to-end APT distribution test (needs docker):
#   source tarball of HEAD  ->  .debs built as a non-root user on Debian 12
#   (X.Y.Z-1 and an X.Y.Z-2 upgrade)  ->  two signed repositories, signed by
#   a throwaway key generated inside the builder container (it never touches
#   the host)  ->  tests/packaging/apt-client.sh in fresh client containers.
#
# Usage: tests/packaging/apt-e2e.sh OUTDIR [CLIENT_IMAGE...]
#   default clients: debian:12 debian:13 ubuntu:24.04
#   OUTDIR receives the tarball, debs-1/, debs-2/, repo-v1/, repo-v2/ and
#   one log per stage. Exit 0 only if every stage and client passed.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
mkdir -p "${1:?usage: apt-e2e.sh OUTDIR [CLIENT_IMAGE...]}"
OUT=$(cd "$1" && pwd)
shift
CLIENTS=${*:-debian:12 debian:13 ubuntu:24.04}
BUILDER=${POLYCALL_DEB_BUILDER:-debian:12}

hostpath() { if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else printf '%s' "$1"; fi; }
export MSYS_NO_PATHCONV=1

rm -rf "$OUT/debs-1" "$OUT/debs-2" "$OUT/repo-v1" "$OUT/repo-v2"
TAR=$(sh "$ROOT/packaging/make-source-tarball.sh" HEAD "$OUT" | awk '{print $2}')
TARNAME=$(basename "$TAR")
UPV=${TARNAME#polycall-}; UPV=${UPV%.tar.gz}
echo "== source: $TARNAME"

echo "== build (.debs, signed repositories) on $BUILDER"
docker run --rm -v "$(hostpath "$OUT"):/out" "$BUILDER" sh -c '
  set -eu
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -qq
  apt-get install -y -qq build-essential debhelper cmake python3 pkg-config file apt-utils gnupg >/dev/null
  useradd -m builder
  mkdir /home/builder/w && tar -xzf /out/'"$TARNAME"' -C /home/builder/w
  chown -R builder /home/builder/w
  S=/home/builder/w/polycall-'"$UPV"'
  su builder -c "sh $S/packaging/build-deb.sh /out/'"$TARNAME"' /home/builder/debs-1" >/out/build-1.log 2>&1
  su builder -c "POLYCALL_DEB_VERSION='"$UPV"'-2 sh $S/packaging/build-deb.sh /out/'"$TARNAME"' /home/builder/debs-2" >/out/build-2.log 2>&1
  cp -r /home/builder/debs-1 /home/builder/debs-2 /out/
  export GNUPGHOME=$(mktemp -d)
  gpg --batch --passphrase "" --quick-gen-key "polycall test archive (throwaway) <noreply@invalid>" ed25519 sign 1d 2>/dev/null
  KEY=$(gpg --list-keys --with-colons | awk -F: "/^fpr/{print \$10; exit}")
  sh $S/packaging/apt-repo.sh /out/repo-v1 "$KEY" /out/debs-1/*.deb
  sh $S/packaging/apt-repo.sh /out/repo-v2 "$KEY" /out/debs-1/*.deb /out/debs-2/*.deb
  rm -rf "$GNUPGHOME"
' >"$OUT/build.log" 2>&1 || { echo "build failed:"; tail -40 "$OUT/build.log"; tail -40 "$OUT/build-1.log" 2>/dev/null; exit 1; }
tail -2 "$OUT/build.log"

RC=0
for img in $CLIENTS; do
  log="$OUT/client-$(echo "$img" | tr ':/' '__').log"
  if docker run --rm -v "$(hostpath "$OUT/repo-v1"):/repo-v1:ro" -v "$(hostpath "$OUT/repo-v2"):/repo-v2:ro" \
       -v "$(hostpath "$HERE"):/t:ro" "$img" sh /t/apt-client.sh "$UPV" >"$log" 2>&1; then
    echo "PASS  $img  ($(tail -1 "$log"))"
  else
    echo "FAIL  $img  (see $log)"; grep '^FAIL' "$log" || tail -20 "$log"; RC=1
  fi
done
exit $RC
