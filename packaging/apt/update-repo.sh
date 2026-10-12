#!/bin/sh
# Add .deb files to the polycall APT repository tree, regenerate the indexes
# for every supported release, and sign them.
#
# usage: update-repo.sh --repo DIR --incoming DIR --key FINGERPRINT
#                       [--public-key FILE] [--passphrase-file FILE]
#                       [--url URL]
#
#   --repo DIR        repository root (created if missing). Layout:
#                       pool/<codename>/main/p/polycall/*.deb
#                       dists/<codename>/{InRelease,Release,Release.gpg}
#                       dists/<codename>/main/binary-<arch>/Packages{,.gz,.xz}
#                       polycall-archive-keyring.{gpg,asc}, index.html
#   --incoming DIR    packaging/deb/build.sh output: <codename>/<arch>/*.deb
#                     (may be empty to only re-index and re-sign)
#   --key FPR         fingerprint of the signing key in $GNUPGHOME
#   --public-key FILE armored public key to publish (default: export FPR);
#                     must contain exactly the signing key
#   --passphrase-file FILE  passphrase for the signing key, if it has one
#   --url URL         public URL of --repo, used in index.html
#
# Needs: apt-utils (apt-ftparchive), gnupg, gpgv, xz-utils, dpkg.
# A version that is already in the pool is never replaced: re-adding an
# identical file is a no-op, a different file with the same name is an error
# (bump the Debian revision instead).
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"

REPO='' INCOMING='' FPR='' PUBKEY='' PASSFILE='' URL=https://obinexus.github.io/polycall/apt
while [ $# -gt 0 ]; do
  case "$1" in
    --repo) REPO=$2; shift 2 ;;
    --incoming) INCOMING=$2; shift 2 ;;
    --key) FPR=$2; shift 2 ;;
    --public-key) PUBKEY=$2; shift 2 ;;
    --passphrase-file) PASSFILE=$2; shift 2 ;;
    --url) URL=$2; shift 2 ;;
    -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
    *) die "unknown argument: $1" ;;
  esac
done
[ -n "$REPO" ] && [ -n "$INCOMING" ] && [ -n "$FPR" ] || die "--repo, --incoming and --key are required"
for t in apt-ftparchive gpg gpgv xz dpkg-deb; do
  command -v "$t" >/dev/null 2>&1 || die "$t not found (install apt-utils gnupg gpgv xz-utils dpkg)"
done
gpg --batch --list-secret-keys "$FPR" >/dev/null 2>&1 || die "no secret key $FPR in ${GNUPGHOME:-~/.gnupg}"
mkdir -p "$REPO"; REPO=$(CDPATH='' cd -- "$REPO" && pwd)
[ -d "$INCOMING" ] || die "--incoming $INCOMING is not a directory"
INCOMING=$(CDPATH='' cd -- "$INCOMING" && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT

# ---------------------------------------------------------------- add packages
added=0
for deb in $(find "$INCOMING" -name '*.deb' -type f | sort); do
  rel=${deb#"$INCOMING"/}
  codename=${rel%%/*}
  suffix=$(suite_field "$codename" suffix) || die "$rel: '$codename' is not a release in packaging/suites.tsv"
  pkg=$(dpkg-deb --field "$deb" Package)
  ver=$(dpkg-deb --field "$deb" Version)
  arch=$(dpkg-deb --field "$deb" Architecture)
  src=$(dpkg-deb --field "$deb" Source | cut -d' ' -f1); src=${src:-$pkg}
  case " $(suite_field "$codename" archs) all " in *" $arch "*) ;;
    *) die "$rel: architecture $arch is not enabled for $codename in packaging/suites.tsv" ;; esac
  case "$ver" in *"~$suffix"|*"~$suffix"+*) ;;
    *) die "$rel: version $ver was not built for $codename (expected a ~$suffix suffix)" ;; esac
  case "$src" in lib?*) initial=$(printf '%s' "$src" | cut -c1-4) ;; *) initial=$(printf '%s' "$src" | cut -c1) ;; esac
  dest="$REPO/pool/$codename/main/$initial/$src/${pkg}_$(printf '%s' "$ver" | sed 's/^[0-9]*://')_$arch.deb"
  if [ -e "$dest" ]; then
    cmp -s "$deb" "$dest" || die "$pkg $ver ($arch) is already published for $codename with different contents; bump the Debian revision"
    info "already published: ${dest#"$REPO"/}"
    continue
  fi
  mkdir -p "$(dirname "$dest")"
  cp "$deb" "$dest"; chmod 0644 "$dest"
  info "added: ${dest#"$REPO"/}"
  added=$((added + 1))
done
info "$added new package file(s)"

# ---------------------------------------------------------------- public key
if [ -n "$PUBKEY" ]; then
  [ "$(key_fingerprint "$PUBKEY")" = "$FPR" ] || die "--public-key $PUBKEY is not the key $FPR"
  cp "$PUBKEY" "$TMP/key.asc"
else
  gpg --batch --armor --export-options export-minimal --export "$FPR" >"$TMP/key.asc"
fi
[ "$(gpg --batch --show-keys --with-colons "$TMP/key.asc" | grep -c '^pub:')" -eq 1 ] \
  || die "the published keyring must contain exactly one key"
gpg --batch --yes --dearmor --output "$TMP/key.gpg" "$TMP/key.asc"
install -m 0644 "$TMP/key.asc" "$REPO/polycall-archive-keyring.asc"
install -m 0644 "$TMP/key.gpg" "$REPO/polycall-archive-keyring.gpg"

# ---------------------------------------------------------------- indexes
sign() { # sign IN OUT MODE(clearsign|detach-sign)
  gpg --batch --yes --pinentry-mode loopback ${PASSFILE:+--passphrase-file "$PASSFILE"} \
    --local-user "$FPR" --digest-algo SHA512 --armor "--$3" --output "$2" "$1"
}

for codename in $(suite_codenames); do
  pool="pool/$codename/main"
  [ -d "$REPO/$pool" ] || continue
  dist="$REPO/dists/$codename"
  # hashes the current Release points at stay reachable by-hash for one more
  # generation, so clients mid-update (or behind a CDN cache) never see a
  # hash mismatch
  keep="$TMP/keep-$codename"
  if [ -f "$dist/Release" ]; then
    awk '/^SHA(256|512):/ { on = 1; next } /^[^ ]/ { on = 0 } on { print $1 }' "$dist/Release" >"$keep"
  else
    : >"$keep"
  fi
  rm -f "$dist/Release" "$dist/InRelease" "$dist/Release.gpg"

  archs=$(find "$REPO/$pool" -name '*.deb' | sed 's/.*_\([^_]*\)\.deb$/\1/' | sort -u | grep -v '^all$' | tr '\n' ' ')
  archs=${archs% }
  for arch in $archs; do
    bdir="$dist/main/binary-$arch"
    mkdir -p "$bdir"
    ( cd "$REPO" && apt-ftparchive --arch "$arch" packages "$pool" ) >"$bdir/Packages"
    [ -s "$bdir/Packages" ] || die "empty Packages index for $codename/$arch"
    gzip -9n <"$bdir/Packages" >"$bdir/Packages.gz"
    xz -9e <"$bdir/Packages" >"$bdir/Packages.xz"
  done

  name=$(suite_field "$codename" name)
  ( cd "$dist" && apt-ftparchive \
      -o APT::FTPArchive::DoByHash=true \
      -o APT::FTPArchive::Release::MD5=false \
      -o APT::FTPArchive::Release::SHA1=false \
      -o APT::FTPArchive::Release::Origin=polycall \
      -o APT::FTPArchive::Release::Label=polycall \
      -o APT::FTPArchive::Release::Suite="$codename" \
      -o APT::FTPArchive::Release::Codename="$codename" \
      -o APT::FTPArchive::Release::Architectures="$archs" \
      -o APT::FTPArchive::Release::Components=main \
      -o APT::FTPArchive::Release::Description="polycall for $name - third-party repository, not part of Debian or Ubuntu" \
      release . ) >"$TMP/Release"
  mv "$TMP/Release" "$dist/Release"
  sign "$dist/Release" "$dist/InRelease" clearsign
  sign "$dist/Release" "$dist/Release.gpg" detach-sign

  # prune by-hash entries referenced by neither this Release nor the last
  awk '/^SHA(256|512):/ { on = 1; next } /^[^ ]/ { on = 0 } on { print $1 }' "$dist/Release" >>"$keep"
  find "$dist" -path '*/by-hash/*' -type f | while read -r f; do
    grep -qx "${f##*/}" "$keep" || rm -f "$f"
  done

  # verify what was just written, the way apt will
  gpgv --keyring "$REPO/polycall-archive-keyring.gpg" "$dist/InRelease" 2>"$TMP/gpgv.log" \
    || { cat "$TMP/gpgv.log" >&2; die "InRelease for $codename does not verify"; }
  gpgv --keyring "$REPO/polycall-archive-keyring.gpg" "$dist/Release.gpg" "$dist/Release" 2>"$TMP/gpgv.log" \
    || { cat "$TMP/gpgv.log" >&2; die "Release.gpg for $codename does not verify"; }
  awk '/^SHA256:/ { on = 1; next } /^[^ ]/ { on = 0 } on { print $1 "  " $3 }' "$dist/Release" \
    | ( cd "$dist" && sha256sum --check --quiet --strict ) || die "Release checksums for $codename are wrong"
  info "signed dists/$codename ($archs)"
done

# ---------------------------------------------------------------- landing page
{
  cat <<EOF
<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>polycall APT repository</title>
<style>body{font:16px/1.5 system-ui,sans-serif;max-width:46rem;margin:2rem auto;padding:0 1rem}
pre{background:#f4f4f4;padding:.75rem;overflow-x:auto}code{font-size:.92em}</style>
</head><body>
<h1>polycall APT repository</h1>
<p>A third-party APT repository for <a href="https://github.com/obinexus/polycall">polycall</a>,
maintained by the polycall project. It is <strong>not</strong> part of Debian or Ubuntu and is not
reviewed or supported by either distribution.</p>
<p>Signing key fingerprint: <code>$FPR</code></p>
<p>Setup, fingerprint verification, upgrade and removal instructions are in the
<a href="https://github.com/obinexus/polycall#install-on-debian-and-ubuntu">project README</a>.</p>
<h2>Releases</h2>
<ul>
EOF
  for codename in $(suite_codenames); do
    [ -f "$REPO/dists/$codename/InRelease" ] || continue
    printf '<li><code>%s</code> &mdash; %s</li>\n' "$codename" "$(suite_field "$codename" name)"
  done
  cat <<EOF
</ul>
<p>Repository URL: <code>$URL</code></p>
</body></html>
EOF
} >"$REPO/index.html"
info "repository updated: $REPO"
