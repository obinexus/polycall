# Debian/Ubuntu packaging and the APT repository

How `sudo apt install polycall` is produced: the Debian packaging in
`debian/`, the build/test/publish tooling in `packaging/`, and the GitHub
Actions workflows that tie them together. User-facing install instructions
are in the [README](../README.md#install-on-debian-and-ubuntu).

The repository is a **third-party APT repository** run by this project and
hosted on GitHub Pages. It is not Debian's or Ubuntu's archive and is not
reviewed or supported by either distribution. Getting `polycall` into the
official archives is a separate process (Debian: an ITP bug and a sponsor;
Ubuntu: usually via Debian) and has not been started.

## Packages

One source package, `polycall`, built with CMake (`debian/rules`, debhelper
compat 13), produces three binary packages:

| Package | Contents | Depends |
| --- | --- | --- |
| `polycall` | `/usr/bin/polycall`, `polycall(1)`, docs | `libc6` only (the CLI links the core statically, by upstream design) |
| `libpolycall1` | `libpolycall.so.1` (SONAME from `POLYCALL_ABI_VERSION_MAJOR`) | `libc6` |
| `libpolycall-dev` | headers in `/usr/include/polycall`, `libpolycall.a`, `libpolycall.so`, CMake package (`find_package(polycall)`, targets `polycall::polycall_shared` / `polycall::polycall_static`) | `libpolycall1 (= same version)` |

The split follows the ABI: the shared library carries its SONAME in its
package name, so an ABI break (major version bump) becomes `libpolycall2`
and can be co-installed. `debian/libpolycall1.symbols` lists the exported
symbols; a build fails if one disappears. When a release adds public
symbols, add them to that file with the version that introduced them.

The build runs the upstream test suite (`ctest` -> `tests/run_all.sh`; Python
is a test-only build dependency so the Python client paths run too), uses
`hardening=+all`, and stages the install with CMake's `DESTDIR` support into
`debian/tmp`; `dh_missing` fails the build if anything installed is not
packaged.

## Versions

* Upstream version: `POLYCALL_ABI_VERSION_STRING` in
  `include/polycall_export.h` (the project's single version source).
* `debian/changelog` packages it as `<upstream>-<revision>`, e.g. `1.0.0-1`.
  `packaging/deb/build.sh` refuses to build if the two disagree.
* Each supported release gets its own build with a suffix, e.g.
  `1.0.0-1~deb12`, `1.0.0-1~ubuntu24.04`. Dependencies are therefore computed
  against that release, and upgrading the OS (Debian 12 -> 13, Ubuntu 24.04
  -> 26.04) also upgrades polycall, because `~deb12 < ~deb13` and
  `~ubuntu24.04 < ~ubuntu26.04`. `~` also sorts below any official Debian
  `1.0.0-1` should one ever exist.

## Supported releases

`packaging/suites.tsv` is the single source of truth; every script and
workflow matrix reads it.

| Release | Suite | Architectures |
| --- | --- | --- |
| Debian 12 (bookworm) | `bookworm` | amd64, arm64 |
| Debian 13 (trixie) | `trixie` | amd64, arm64 |
| Ubuntu 22.04 LTS (Jammy Jellyfish) | `jammy` | amd64, arm64 |
| Ubuntu 24.04 LTS (Noble Numbat) | `noble` | amd64, arm64 |
| Ubuntu 26.04 LTS (Resolute Raccoon) | `resolute` | amd64, arm64 |

arm64 packages are built and tested on GitHub's native arm64 runners
(`ubuntu-24.04-arm`) with exactly the same steps as amd64.

To add a release, add a row and open a pull request: the packaging workflow
builds, lints and install-tests it before anything can be published. To drop
one at end of life, remove the row; its `dists/` and `pool/` entries stay on
the `apt-repo` branch (existing users keep working) until you delete them
there and re-sign by running a release.

## Repository layout and hosting

```
https://obinexus.github.io/polycall/apt/
  polycall-archive-keyring.gpg   signing key (binary) -- what users install
  polycall-archive-keyring.asc   same key, armored
  index.html                     pointer to the README instructions
  dists/<suite>/InRelease        clearsigned Release
  dists/<suite>/Release(.gpg)    Release + detached signature
  dists/<suite>/main/binary-<arch>/Packages{,.gz,.xz}
  dists/<suite>/main/binary-<arch>/by-hash/SHA256|SHA512/<hash>
  pool/<suite>/main/p/polycall/*.deb
```

* `packaging/apt/update-repo.sh` adds packages, regenerates every index with
  `apt-ftparchive`, signs `InRelease`/`Release.gpg` (SHA-512 digests), then
  verifies the result with `gpgv` and the published keyring before it
  returns. A version already in the pool is never replaced: re-adding an
  identical file is a no-op, different contents under the same version are
  an error (bump the Debian revision instead). Older versions stay in the
  pool and indexes, so `apt install polycall=<version>` can roll back.
* `Acquire-By-Hash: yes`: GitHub Pages caches files for 10 minutes, so after
  a release a client could see a new `InRelease` and a cached `Packages`.
  By-hash index URLs are unique per content, and the previous generation is
  kept, so `apt update` never hits a hash mismatch during propagation.
* The repository tree lives on the **`apt-repo` branch** (an orphan branch
  written only by `release.yml`). `pages.yml` deploys the MkDocs site, the
  Jekyll `docs/` and the `apt/` directory of that branch as one Pages
  artifact, so documentation deployments never drop the repository.
* No `Valid-Until` is set, so the indexes do not need periodic re-signing;
  freshness relies on HTTPS to GitHub Pages.

## One-time setup (maintainer)

1. **Create the signing key** on a trusted machine (needs `gpg`; `gh` for
   `--github`):

   ```bash
   sh packaging/apt/new-signing-key.sh --backup ~/polycall-apt-signing-key.asc --github
   ```

   This generates an Ed25519 signing-only key in a throw-away `GNUPGHOME`,
   writes the **public** key to `packaging/apt/polycall-archive-keyring.asc`,
   puts its fingerprint into `README.md`, writes the **secret** key only to
   the `--backup` path (outside the repository, mode 0600), and with
   `--github` stores it as the `APT_SIGNING_KEY` secret of the `apt-signing`
   environment. Keep the backup offline: losing it means rotating the key
   for every user.

2. **Commit** `packaging/apt/polycall-archive-keyring.asc` and `README.md`.
   CI refuses to publish unless the committed key, the README fingerprint
   and the CI secret are the same key (`packaging/ci/check-signing-key.sh`).

3. **Secrets** (Settings -> Environments -> `apt-signing`, created by the
   script or on the first release run):
   * `APT_SIGNING_KEY` -- armored secret key (required).
   * `APT_SIGNING_KEY_PASSPHRASE` -- only if you protected the key with one.
   Recommended: restrict the environment's deployment branches/tags to
   `v*` and `debian/*` tags, so only release runs can read the key.

4. **GitHub Pages** must stay on "GitHub Actions" as its source (it is). The
   `github-pages` environment only allows deployments from
   `polycall-cli-redesign`; `release.yml` therefore dispatches `pages.yml` on
   the default branch instead of deploying from the tag, so no change is
   needed there.

5. Recommended: protect the `apt-repo` branch against deletion and force
   pushes (a ruleset that still lets GitHub Actions push).

## Releasing

1. Bump `POLYCALL_ABI_VERSION_*` in `include/polycall_export.h` (if the code
   changed) and add a `debian/changelog` entry for `<version>-1`. Update
   `debian/libpolycall1.symbols` for new public symbols.
2. Merge to the default branch with the **Debian packages** workflow green.
3. Tag and push: `git tag v1.0.0 && git push origin v1.0.0`. For a
   packaging-only fix, add a changelog entry `1.0.0-2` and tag
   `debian/1.0.0-2`.

`release.yml` then checks the tag against the version, rebuilds and retests
everything, signs and pushes the repository, attaches the `.deb` files and
`SHA256SUMS` to the GitHub release, redeploys Pages, and finally runs
`apt-verify.yml`: every supported release, in a pristine container, follows
the README against the live URL (waiting up to 30 minutes for the new
version to appear), installs, smoke-tests, upgrades from the oldest
published version, removes and purges. A release is only done when that job
is green. `apt-verify.yml` also runs weekly. After the first release
verifies, delete the "Status" note from the README's install section.

If a release run fails after the `apt-repo` push, use **Re-run failed jobs**:
the packages built in the first attempt are reused, so the pool check sees
identical files. The builds are also reproducible (two builds of the same
tree in the same release container produce byte-identical `.deb` files), so
even a full re-run normally republishes nothing; if a distribution toolchain
update changed the bytes in between, the pool check stops the run instead of
silently replacing a published version.

## Testing locally

Everything CI does runs locally with Docker:

```bash
# build + test + lint for one release (writes dist/<suite>/<arch>/). Only
# tracked and non-ignored files go in, like a fresh checkout, so stray local
# build output (e.g. an in-source CMakeCache.txt) cannot leak into the build.
mkdir -p dist
git ls-files -co --exclude-standard -z | tar --null -T - -cf - \
  | docker run --rm -i -v "$PWD/dist:/dist" debian:12 \
      sh -c 'mkdir /src && tar -xf - -C /src && sh /src/packaging/deb/build.sh --out /dist'

# signed HTTPS repository + clean-client install/upgrade/remove test
sh packaging/test/e2e-local.sh --dist dist --suite bookworm --arch amd64

# the published repository, from a clean container
git ls-files -co --exclude-standard -z | tar --null -T - -cf - \
  | docker run --rm -i ubuntu:24.04 \
      sh -c 'mkdir /src && tar -xf - -C /src && sh /src/packaging/test/verify-published.sh'
```

Use `--platform linux/arm64` on `docker run` (and `--arch arm64` for
`e2e-local.sh`) to test arm64 under QEMU emulation on an amd64 machine.

## Key rotation, expiry and compromise

The key is created without an expiry date so that installed systems never
stop updating unexpectedly. To rotate (or after a compromise):

1. Create a new key with `new-signing-key.sh --force ...` and replace the
   `APT_SIGNING_KEY` secret; commit the new public key and README.
2. Publish a release; every index is re-signed with the new key.
3. Users must re-run the key download and fingerprint check from the README;
   until they do, `apt update` reports the repository as unsigned and keeps
   the last verified indexes. Announce this in the release notes.

After a compromise, also revoke the old key and tell users to delete
`/etc/apt/keyrings/polycall-archive-keyring.gpg` before re-importing.

## Files

| Path | Purpose |
| --- | --- |
| `debian/` | Debian packaging (control, rules, copyright, symbols, autopkgtests) |
| `docs/man/polycall.1` | manual page, installed by CMake and `make install` |
| `packaging/suites.tsv` | supported releases and architectures |
| `packaging/deb/build.sh` | build + upstream tests + lintian in a clean container |
| `packaging/apt/update-repo.sh` | add packages, index, sign, verify |
| `packaging/apt/new-signing-key.sh` | one-time signing key creation |
| `packaging/test/client-check.sh` | README steps on a clean system (install, dev, upgrade, remove) |
| `packaging/test/e2e-local.sh` | throw-away signed HTTPS repository + client-check |
| `packaging/test/verify-published.sh` | client-check against the live repository |
| `packaging/ci/*.sh` | matrix generation, tag/version and signing-key checks |
| `.github/workflows/packages.yml` | PRs: build, lint, install-test every release/arch |
| `.github/workflows/release.yml` | tags: publish the signed repository and release assets |
| `.github/workflows/apt-verify.yml` | verify the published repository (after release, weekly) |
| `.github/workflows/pages.yml` | deploy docs + `/apt/` |
