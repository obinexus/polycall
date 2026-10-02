#!/bin/sh
# Create (or refresh) a signed APT repository from .deb files.
#
# Usage: packaging/apt-repo.sh REPO_DIR KEY_ID DEB...
#   GNUPGHOME selects the keyring holding KEY_ID's secret key. The key is
#   never written into the repository; only its public part is exported
#   (polycall-archive-keyring.gpg, for apt's signed-by=).
#
# Layout (suite "stable", component "main"):
#   pool/main/p/polycall/*.deb
#   dists/stable/{Release,InRelease,Release.gpg}
#   dists/stable/main/binary-<arch>/Packages{,.gz}
# Clients:
#   deb [signed-by=/usr/share/keyrings/polycall-archive-keyring.gpg] <URL> stable main
#
# Requires apt-utils (apt-ftparchive) and gnupg. Publishing the directory
# (any static HTTPS host) is a separate, owner-approved step.
set -eu
REPO=${1:?usage: apt-repo.sh REPO_DIR KEY_ID DEB...}
KEY=${2:?usage: apt-repo.sh REPO_DIR KEY_ID DEB...}
shift 2
[ $# -gt 0 ] || { echo "apt-repo: no .deb files given" >&2; exit 2; }
SUITE=stable
COMP=main

mkdir -p "$REPO/pool/$COMP/p/polycall"
for d in "$@"; do
  case $d in *.deb) cp "$d" "$REPO/pool/$COMP/p/polycall/" ;; esac
done

cd "$REPO"
ARCHS=$(for d in pool/$COMP/p/polycall/*.deb; do dpkg-deb -f "$d" Architecture; done |
  grep -v '^all$' | sort -u | tr '\n' ' ')
for a in $ARCHS; do
  mkdir -p "dists/$SUITE/$COMP/binary-$a"
  apt-ftparchive --arch "$a" packages "pool/$COMP" > "dists/$SUITE/$COMP/binary-$a/Packages"
  gzip -9nf -k "dists/$SUITE/$COMP/binary-$a/Packages"
done
apt-ftparchive \
  -o APT::FTPArchive::Release::Origin=OBINexus \
  -o APT::FTPArchive::Release::Label=polycall \
  -o APT::FTPArchive::Release::Suite=$SUITE \
  -o APT::FTPArchive::Release::Codename=$SUITE \
  -o "APT::FTPArchive::Release::Architectures=$ARCHS" \
  -o APT::FTPArchive::Release::Components=$COMP \
  release "dists/$SUITE" > "dists/$SUITE/Release.tmp"
mv "dists/$SUITE/Release.tmp" "dists/$SUITE/Release"
rm -f "dists/$SUITE/InRelease" "dists/$SUITE/Release.gpg"
gpg --batch --yes --local-user "$KEY" --clearsign -o "dists/$SUITE/InRelease" "dists/$SUITE/Release"
gpg --batch --yes --local-user "$KEY" --armor --detach-sign -o "dists/$SUITE/Release.gpg" "dists/$SUITE/Release"
gpg --batch --yes --export "$KEY" > polycall-archive-keyring.gpg
echo "apt-repo: $REPO signed by $KEY ($ARCHS)"
