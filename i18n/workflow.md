# i18n Translation Workflow

This document describes how internationalization (i18n) works in OWzx Slicer and the workflow for keeping translations up to date.

## Overview

OWzx Slicer uses Qt Linguist for i18n. User-visible strings are wrapped in `qsTr()` in QML and `tr()` in C++. The translation source files (`.ts`) live in `i18n/`, one per locale: `zh_CN`, `en`, `ja`, `ko`, `de`, `fr`.

## The Workflow

### 1. Mark strings for translation
- QML: `qsTr("Add Pause")` — already ~95% coverage.
- C++: `tr("...")` for QObject-derived classes.

### 2. Extract strings (lupdate)
After adding/changing `qsTr` strings, refresh the `.ts` files:
```bash
E:/Qt6.10/bin/lupdate.exe src/qml_gui -ts i18n/zh_CN.ts i18n/en.ts i18n/ja.ts i18n/ko.ts i18n/de.ts i18n/fr.ts -no-obsolete
```
- New strings appear as `type="unfinished"`.
- `-no-obsolete` removes strings that no longer exist in the source.

**Rule: each phase that touches `qsTr` strings MUST run lupdate to refresh the `.ts` files.** This prevents the coverage from silently rotting back to 0% finished.

### 3. Translate
Open the `.ts` file in Qt Linguist GUI (or edit the XML directly) and fill in translations. Mark as finished (remove `type="unfinished"`).

### 4. Compile (lrelease)
`lrelease` compiles `.ts` → `.qm` (binary). This is wired into the CMake build via `qt_add_translations` — the build produces `.qm` files automatically. No manual step needed.

```bash
E:/Qt6.10/bin/lrelease.exe i18n/zh_CN.ts -qm build/zh_CN.qm
```

## Current State (v4.6 Phase 127)

- **zh_CN**: v4.6-touched core strings translated (TickCode menu items, CustomGcodeDialog, paint/calibration labels). Proof-of-pipeline established.
- **ja/ko/de/fr**: refreshed via lupdate (new strings captured) but largely unfinished. Full translation of all 1600+ strings is future work — the baseline is documented, not falsely claimed as complete.
- **en**: source strings (finished when source = English).

## Adding a New Language

1. Create `i18n/<locale>.ts` (copy structure from an existing one).
2. Add to `TS_FILES` in `CMakeLists.txt` `qt_add_translations`.
3. Run lupdate + translate.
