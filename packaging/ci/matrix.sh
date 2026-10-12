#!/bin/sh
# Print the GitHub Actions matrix for packaging/suites.tsv as compact JSON:
# one {codename, image, arch, name, suffix, runner} entry per release and
# architecture. arm64 entries run on GitHub's native arm64 runners.
set -eu
# shellcheck source=packaging/lib.sh
. "$(dirname "$0")/../lib.sh"
command -v jq >/dev/null 2>&1 || die "jq is required"
suite_rows | awk -F '\t' '{
  n = split($4, a, " ")
  for (i = 1; i <= n; i++) printf "%s\t%s\t%s\t%s\t%s\n", $1, $2, a[i], $5, $3
}' | jq -R -s -c '
  split("\n") | map(select(length > 0) | split("\t") | {
    codename: .[0], image: .[1], arch: .[2], name: .[3], suffix: .[4],
    runner: (if .[2] == "arm64" then "ubuntu-24.04-arm" else "ubuntu-24.04" end)
  })'
