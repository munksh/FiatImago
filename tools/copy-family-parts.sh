#!/usr/bin/env bash
# Copies the family's shared parts from Fiat Lux into fiat imago, with the
# theme name swapped, then checks the whole tree. Run once from anywhere;
# run again whenever Lux's components change.
set -e

HERE="$(cd "$(dirname "$0")/.." && pwd)"
LUX="${LUX:-$HOME/Projects/FiatLux}"

if [ ! -d "$LUX/qml/components" ]; then
    echo "Fiat Lux not found at $LUX (run with LUX=/path/to/FiatLux)"
    exit 1
fi

mkdir -p "$HERE/qml/components" "$HERE/qml/pages/images/family"

for part in PageHead SectionLabel MunkstolenMark WordChoice LinkText FiatButton; do
    sed 's/FiatLuxTheme/FiatImagoTheme/g' "$LUX/qml/components/$part.qml" \
        > "$HERE/qml/components/$part.qml"
    echo "copied $part.qml"
done

if ls "$LUX"/qml/pages/images/family/*.png > /dev/null 2>&1; then
    cp "$LUX"/qml/pages/images/family/*.png "$HERE/qml/pages/images/family/"
    echo "copied the family icons"
else
    echo "WARNING: no family icons in $LUX/qml/pages/images/family/"
fi
cp "$HERE/icons/172x172/harbour-fiatimago.png" "$HERE/qml/pages/images/family/"
cp "$LUX/LICENSE" "$HERE/LICENSE"
echo "copied LICENSE"

cd "$HERE"
echo
echo "--- other theme names left (must be empty):"
grep -rhoE "Fiat[A-Za-z]*Theme" qml/ | grep -v "^FiatImagoTheme$" | sort -u || true

echo "--- theme names used but not defined (must be empty):"
for n in $(grep -rhoE --include=*.qml "FiatImagoTheme\.[A-Za-z_]+" qml/ | sed 's/.*\.//' | sort -u); do
    grep -qE "(property [a-z]+ $n\b|function $n\b)" qml/FiatImagoTheme.qml || echo "$n"
done

echo "--- relative imports in the copied parts (each must exist under qml/components):"
for f in PageHead SectionLabel MunkstolenMark WordChoice LinkText FiatButton; do
    grep -hoE 'import "[^"]+"' "qml/components/$f.qml" | while read -r _ path; do
        path="${path//\"/}"
        [ "$path" = ".." ] && continue
        [ -e "qml/components/$path" ] || echo "$f.qml imports $path, which is missing"
    done
done

echo "--- SectionLabel properties (AboutPage sets: text):"
grep -nE "property|alias" qml/components/SectionLabel.qml || echo "(none found)"

echo "--- launcher names of the siblings (fiat imago's .desktop says: $(grep '^Name=' harbour-fiatimago.desktop)):"
grep -h "^Name=" "$LUX"/*.desktop "$HOME"/Projects/FiatMos/*.desktop 2>/dev/null || true
