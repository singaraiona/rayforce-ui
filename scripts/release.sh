#!/usr/bin/env bash
set -euo pipefail

VERSION_FILE="include/rfui/version.h"

# --- Parse current version ---
major=$(grep '#define RFUI_VERSION_MAJOR' "$VERSION_FILE" | awk '{print $3}')
minor=$(grep '#define RFUI_VERSION_MINOR' "$VERSION_FILE" | awk '{print $3}')
patch=$(grep '#define RFUI_VERSION_PATCH' "$VERSION_FILE" | awk '{print $3}')
old="$major.$minor.$patch"

# --- Bump ---
component="${1:-patch}"
case "$component" in
  major) major=$((major + 1)); minor=0; patch=0 ;;
  minor) minor=$((minor + 1)); patch=0 ;;
  patch) patch=$((patch + 1)) ;;
  *) echo "Usage: $0 [major|minor|patch]" >&2; exit 1 ;;
esac
new="$major.$minor.$patch"
echo "Bumping $old → $new"

# --- Changelog entry ---
tmpfile=$(mktemp)
trap 'rm -f "$tmpfile"' EXIT

if [ -t 0 ] && [ -n "${EDITOR:-}" ]; then
  echo "# Enter changelog for v$new (lines starting with # are removed)" > "$tmpfile"
  $EDITOR "$tmpfile"
  entry=$(grep -v '^#' "$tmpfile" || true)
else
  echo "Enter changelog (Ctrl-D when done):"
  entry=$(cat)
fi

if [ -z "$entry" ]; then
  echo "Empty changelog, aborting." >&2
  exit 1
fi

# --- Prepend to CHANGELOG.md ---
header="## v$new — $(date +%Y-%m-%d)"
{
  head -1 CHANGELOG.md
  echo ""
  echo "$header"
  echo ""
  echo "$entry"
  tail -n +2 CHANGELOG.md
} > CHANGELOG.md.tmp
mv CHANGELOG.md.tmp CHANGELOG.md

# --- Update version.h ---
sed -i "s/#define RFUI_VERSION_MAJOR .*/#define RFUI_VERSION_MAJOR $major/" "$VERSION_FILE"
sed -i "s/#define RFUI_VERSION_MINOR .*/#define RFUI_VERSION_MINOR $minor/" "$VERSION_FILE"
sed -i "s/#define RFUI_VERSION_PATCH .*/#define RFUI_VERSION_PATCH $patch/" "$VERSION_FILE"

# --- Commit and tag ---
git add CHANGELOG.md "$VERSION_FILE"
git commit -m "release: v$new"
git tag "v$new"

echo ""
echo "Tagged v$new"
echo "Run: git push && git push --tags"
