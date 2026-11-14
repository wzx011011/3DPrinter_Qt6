#!/bin/bash
# OWzx macOS bundle packaging (2026-11-11).
# Usage: scripts/package_mac_bundle.sh [build_dir]
# 1. ninja build (bundle target)  2. macdeployqt  3. QML module tree into
# Resources + import-path registration  4. libomp de-dup (Homebrew openblas
# drags libomp; two instances abort at startup)  5. ad-hoc codesign.
set -e
BUILD=${1:-/Users/maccree/wzx/ai/3DPrinter_Qt6/build-mac}
APP="$BUILD/OWzxSlicer.app"

ninja -C "$BUILD" "OWzxSlicer.app/Contents/MacOS/OWzxSlicer"
/opt/homebrew/bin/macdeployqt "$APP" || true

# libomp de-dup: openblas pulls /opt/homebrew/opt/libomp; macdeployqt copies
# it into Frameworks, and two loaded libomp instances abort at startup.
# Point every reference back at the Homebrew original and drop the copy.
rm -f "$APP/Contents/Frameworks/libomp.dylib"
rm -rf "$APP/Contents/Resources/qml"
for f in "$APP"/Contents/Frameworks/*.dylib "$APP/Contents/MacOS/OWzxSlicer"; do
  otool -L "$f" 2>/dev/null | grep -q "@executable_path/../Frameworks/libomp.dylib" && \
    install_name_tool -change "@executable_path/../Frameworks/libomp.dylib" \
      "/opt/homebrew/opt/libomp/lib/libomp.dylib" "$f"
done

# QML module tree: the engine resolves the OWzx module from DISK (build dir
# layout), not from qrc — copy it beside the executable and register the
# import path through qt.conf.
rm -rf "$APP/Contents/MacOS/qml" "$APP/Contents/MacOS/qtqml/OWzx"
mkdir -p "$APP/Contents/MacOS/qml/OWzx" "$APP/Contents/MacOS/qtqml/OWzx"
cp -R "$BUILD/OWzx/" "$APP/Contents/MacOS/qml/OWzx/"
cp -R "$BUILD/OWzx/" "$APP/Contents/MacOS/qtqml/OWzx/"
chmod -R u+rw "$APP/Contents/MacOS/qml/OWzx" "$APP/Contents/MacOS/qtqml/OWzx"
# Deploy the Qt QML plugin tree (QtQuick/Controls/... plugin dylibs) from
# Homebrew's qtdeclarative share dir beside the OWzx module — the engine
# imports QtQuick.Controls from disk (macdeployqt only walks linked
# libraries, not QML plugins referenced by qmldir).
rm -rf "$APP/Contents/MacOS/qtqml"
mkdir -p "$APP/Contents/MacOS/qtqml"
# Deploy ONLY the QML plugin dirs the app actually imports (QtTest/controls
# test subdirs carry static archives codesign can't handle), and retarget
# every copied dylib's Qt framework references to the SAME frameworks the
# main executable links (Homebrew qtbase 6.12.0_2) — the qtdeclarative
# share-dir plugins link qtbase 6.12.0_1, and two Qt binary sets in one
# process abort with "Cannot move to target thread".
for qmlmod in QtQuick QtQuickControls2 QtQuickTemplates2 QtQuick.Layouts QtQuick.Window QtQuick.Templates QtQuick.Dialogs QtQuick.Effects QtQuick.Shapes QtQuick.Pdf QtQuick.VectorImage QtMultimedia Qtlabs labs; do
  [ -d "/opt/homebrew/share/qt/qml/$qmlmod" ] && cp -RL "/opt/homebrew/share/qt/qml/$qmlmod" "$APP/Contents/MacOS/qtqml/$qmlmod"
done
[ -f "/opt/homebrew/share/qt/qml/qtquickcontrols2.conf" ] && cp "/opt/homebrew/share/qt/qml/qtquickcontrols2.conf" "$APP/Contents/MacOS/qtqml/" 2>/dev/null || true
# Retarget Qt framework references to the BUNDLE frameworks that
# macdeployqt already copied (Contents/Frameworks) — the qtdeclarative
# share-dir plugins link qtbase/qtdeclarative 6.12.0_1 Cellar paths; with
# the bundle also carrying 6.12.0_2 framework copies, two Qt binary sets
# would coexist in one process (objc duplicate-class abort + "Cannot move
# to target thread" fatal).
for dylib in $(find "$APP/Contents/MacOS/qtqml" -name "*.dylib"); do
  # Full-path entries only (space-split is safe: Homebrew install names have
  # no spaces), preserving the /Versions/A/ tail.
  otool -L "$dylib" | awk '{print $1}' | grep '^/opt/homebrew/.*\.framework/Versions/A/Qt[A-Za-z]*$' | while read -r oldpath; do
    # oldpath = <...>/X.framework/Versions/A/X — the framework DIR name is
    # two levels up from A (X.framework), NOT basename(oldpath).
    fwname=$(basename "$(dirname "$(dirname "$(dirname "$oldpath")")")")
    binname=$(basename "$oldpath")
    install_name_tool -change "$oldpath" "@executable_path/../Frameworks/$fwname/Versions/A/$binname" "$dylib"
  done
done

# The qmldir's "prefer :/qml/OWzx/" would redirect module resolution to the
# embedded qrc — the bundle exe's qrc is EMPTY (the module was designed to
# load from the build-dir disk tree). Strip the prefer line so the on-disk
# qmldir is used as-is.
sed -i '' '/prefer :\/qml\/OWzx\//d' "$APP/Contents/MacOS/qml/OWzx/qmldir"
# qt.conf relative to Contents/MacOS: point Qml2Imports at the deployed Qt
# QML plugin tree.
cat > "$APP/Contents/MacOS/qt.conf" <<CONF
[Paths]
Qml2Imports = qtqml
CONF


codesign --force --deep -s - "$APP"
codesign --verify "$APP"
echo "✅ bundle packaged: $APP"
