#!/usr/bin/env bash
# qml_hot_reload.sh — Linux equivalent of scripts/qml_hot_reload.ps1.
#
# Launches OWzxSlicer under qmlpreview with a merged qrc->source mapping so
# trivial QML edits (colors / texts / bindings / literals — including module
# pages under `import OWzx`) hot-patch the RUNNING app in place: no rebuild,
# no restart, C++ singletons and the RHI viewport stay alive.
#
# Requires a build configured with OWZX_QML_HOT_RELOAD=ON (the CMake dev
# default; compiles the QQmlDebuggingEnabler into OWzxSlicer so qmlpreview
# can attach). CI/release builds pass =OFF — the debug hook must not ship.
#
#   Trivial edits hot-patch in place with state preserved.
#   Structural edits (add/remove objects or properties) crash the running
#   process — rerun this script to bounce the app; the fresh launch serves
#   the edited source directly.
#
# Usage:
#   bash scripts/qml_hot_reload.sh
#   bash scripts/qml_hot_reload.sh --skip-first-run --open-page preview
#
# Then edit any .qml under src/qml_gui/ and save. Ctrl+C here stops the app.

set -eu

scriptDir="$(cd "$(dirname "$0")" && pwd)"
repoRoot="$(dirname "$scriptDir")"
buildDir="$repoRoot/build"

exe="$buildDir/OWzxSlicer"
if [ ! -x "$exe" ]; then
    echo "OWzxSlicer not found at: $exe (build first)" >&2
    exit 1
fi

# Resolve qmlpreview from the Qt the build actually used (CMakeCache Qt6_DIR),
# falling back to the pinned ~/Qt tree.
qmlpreview=""
cacheFile="$buildDir/CMakeCache.txt"
if [ -f "$cacheFile" ]; then
    qt6Dir="$(sed -n 's/^Qt6_DIR:.*=\(.*\)$/\1/p' "$cacheFile" | head -1)"
    if [ -n "$qt6Dir" ]; then
        candidate="$(dirname "$(dirname "$(dirname "$qt6Dir")")")/bin/qmlpreview"
        [ -x "$candidate" ] && qmlpreview="$candidate"
    fi
fi
if [ -z "$qmlpreview" ] && [ -x "$HOME/Qt/6.12.0/gcc_64/bin/qmlpreview" ]; then
    qmlpreview="$HOME/Qt/6.12.0/gcc_64/bin/qmlpreview"
fi
if [ -z "$qmlpreview" ]; then
    echo "qmlpreview not found (looked via CMakeCache Qt6_DIR and ~/Qt/6.12.0)" >&2
    exit 1
fi

# Build the merged --resource map. Three ingredients, all required:
#   1. owzx_qml_raw_qml_0.qrc     — every .qml qrc alias -> src/qml_gui source
#   2. extra_qmldirs.qrc          — subdirectory qmldirs -> build/OWzx/<dir>
#   3. root module qmldir mapping — WITHOUT it the module resolves purely from
#      qrc and module pages never patch (verified 2026-10-18: only main.qml).
rawQrc="$buildDir/src/qml_gui/.qt/rcc/owzx_qml_raw_qml_0.qrc"
extraQrc="$buildDir/src/qml_gui/.qt/rcc/owzx_qml_raw_qml_0_extra_qmldirs.qrc"
moduleQmldir="$buildDir/OWzx/qmldir"
for p in "$rawQrc" "$extraQrc" "$moduleQmldir"; do
    if [ ! -f "$p" ]; then
        echo "Mapping input missing: $p (build first)" >&2
        exit 1
    fi
done

qrc_body() { sed -n '/<qresource/,/<\/qresource>/p' "$1" | sed '1d;$d'; }
mapFile="$buildDir/hot_reload_map.qrc"
{
    echo '<RCC>'
    echo '  <qresource prefix="/qml/OWzx/">'
    printf '    <file alias="qmldir">%s</file>\n' "$moduleQmldir"
    qrc_body "$rawQrc"
    qrc_body "$extraQrc"
    echo '  </qresource>'
    echo '</RCC>'
} > "$mapFile"
echo "[hot-reload] resource map written: $mapFile"

# qmlpreview owns the console; app args pass through after the exe path.
exec "$qmlpreview" --resource "$mapFile" "$exe" "$@"
