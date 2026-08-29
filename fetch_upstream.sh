#!/bin/sh
# fetch_upstream.sh - download and unpack the upstream Common Source Code
# Project sources this port builds against.
#
# The repo tracks only the port itself (Makefile, src/sdl/, three patched core
# files). Everything else - the VM cores, the rest of the shared framework,
# resources - comes from Toshiya Takeda's upstream archive. This script
# downloads it and unpacks it into the repo WITHOUT touching any file git
# tracks, so the port's patched core files stay as committed.
#
#   ./fetch_upstream.sh            # then: make
#
# Needs: curl, rsync, git, and a 7z extractor (brew install sevenzip,
# or: sudo port install 7zip).
#
# The port was built against the 2025-11-09 upstream snapshot (the exact core
# edits are recorded in core-vs-upstream-20251109.patch). Upstream updates the
# archive in place; a newer snapshot will most likely still build, but if the
# checksum warning below fires and the build breaks, that patch shows what the
# port expects.
set -e
cd "$(dirname "$0")"

URL="http://takeda-toshiya.my.coocan.jp/common/source.7z"
KNOWN_SHA256="63d598a29cabf2d9212bd8a1699d8bcdddfc2416e711c20581a0d665c96f2399"

# find a 7z extractor
for z in 7zz 7z 7za; do
	if command -v "$z" >/dev/null 2>&1; then
		SEVENZIP=$z
		break
	fi
done
[ -n "$SEVENZIP" ] || { echo "error: no 7z extractor found (brew install sevenzip)" >&2; exit 1; }
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || { echo "error: run from a git checkout of this repo" >&2; exit 1; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "==> downloading $URL"
curl -fL -o "$tmp/source.7z" "$URL"

sum=$(shasum -a 256 "$tmp/source.7z" | awk '{print $1}')
if [ "$sum" != "$KNOWN_SHA256" ]; then
	echo "warning: archive checksum differs from the snapshot this port was tested with" >&2
	echo "         (upstream updates source.7z in place; continuing anyway)" >&2
fi

echo "==> extracting"
"$SEVENZIP" x -y -o"$tmp/x" "$tmp/source.7z" >/dev/null

# archive root = the directory that contains src/common.cpp
root=$(dirname "$(dirname "$(find "$tmp/x" -path '*/src/common.cpp' | head -1)")")
[ -f "$root/src/common.cpp" ] || { echo "error: unexpected archive layout" >&2; exit 1; }

# unpack into the repo, skipping every git-tracked file (keeps the patched
# core files and the port's own tree exactly as committed)
echo "==> unpacking into $(pwd) (git-tracked files untouched)"
git ls-files | sed 's,^,/,' > "$tmp/exclude"
rsync -a --exclude-from="$tmp/exclude" "$root"/ ./

echo "==> done. next: make   (see README.md)"
