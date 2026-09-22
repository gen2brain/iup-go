#!/bin/bash
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "${1:-docs}" && pwd)
VERSION=$(tr -d '[:space:]' < "$HERE/../../iup/external/VERSION" | cut -d. -f1,2)
OUT=${2:-_site}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

rm -rf "$OUT"
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
cp -r "$HERE/assets" "$OUT/assets"
for d in images figures; do
  [ -d "$SRC/$d" ] && cp -r "$SRC/$d" "$OUT/$d"
done

pandoc -f gfm -t html5 --lua-filter "$HERE/nav.lua" -M navprefix="" "$SRC/README.md" -o "$TMP/nav0.html"
pandoc -f gfm -t html5 --lua-filter "$HERE/nav.lua" -M navprefix="../" "$SRC/README.md" -o "$TMP/nav1.html"

cd "$SRC"
find . -name '*.md' | sed 's|^\./||' | while read -r rel; do
  case "$rel" in
    */*) root="../"; nav="$TMP/nav1.html" ;;
    *)   root="";    nav="$TMP/nav0.html" ;;
  esac
  if [ "$rel" = "README.md" ]; then out="$OUT/index.html"; else out="$OUT/${rel%.md}.html"; fi
  mkdir -p "$(dirname "$out")"
  pandoc -f gfm -t html5 --template "$HERE/template.html" --toc --toc-depth=4 \
    --lua-filter "$HERE/links.lua" -M docpath="$rel" -V root="$root" -V version="$VERSION" \
    --include-before-body "$nav" "$rel" -o "$out"
done

pagefind --site "$OUT" --output-subdir pagefind
