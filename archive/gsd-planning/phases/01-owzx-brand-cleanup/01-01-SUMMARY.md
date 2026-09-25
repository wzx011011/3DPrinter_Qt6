---
phase: 01-owzx-brand-cleanup
plan: 01
subsystem: QML UI + JSON resources
tags: [brand-cleanup, wave-1, string-replacement]
dependency_graph:
  requires: []
  provides: [brand-baseline-qml]
  affects: [01-02, 01-03]
tech-stack:
  added: []
  patterns: [qsTr-brand-strings]
key-files:
  created: []
  modified:
    - src/qml_gui/main.qml
    - src/qml_gui/dialogs/AboutDialog.qml
    - src/qml_gui/pages/HomePage.qml
    - src/qml_gui/pages/PreferencesPage.qml
    - src/qml_gui/pages/ModelMallPage.qml
    - src/qml_gui/dialogs/PrintHostDialog.qml
    - src/qml_gui/data/hints.json
decisions:
  - "Version aligned to OrcaSlicer 2.4.0-dev (from version.inc SoftFever_VERSION)"
  - "Commit hash removed from upstream baseline text (OrcaSlicer main is rolling)"
  - "www.creality.com replaced with www.orcaslicer.org"
  - "CrealityPrint references in hints.json rephrased to OWzx or neutral wording"
metrics:
  duration: ~3 minutes
  completed_date: 2026-06-15
---

# Phase 1 Plan 01: QML UI Brand Strings + hints.json Replacement (Wave 1)

One-line: Replaced all user-visible Creality/CrealityPrint brand strings in 7 QML/JSON files with OWzx/OrcaSlicer branding; aligned version to 2.4.0-dev.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Replace brand strings in main.qml, AboutDialog, HomePage, PreferencesPage | 5cb6a5c | main.qml, AboutDialog.qml, HomePage.qml, PreferencesPage.qml |
| 2 | Replace brand strings in ModelMallPage, PrintHostDialog, hints.json | f0168f7 | ModelMallPage.qml, PrintHostDialog.qml, hints.json |

## Changes Made

### main.qml
- Window title: `"Creality Print 7.0 - QML"` -> `"OWzx Slicer"`
- About menu item (2 occurrences): `"关于 Creality Print"` -> `"关于 OWzx"`
- About dialog title text: `"Creality Print 7.0 - QML"` -> `"OWzx Slicer"`
- Upstream attribution: `"基于 CrealityPrint v7.0.1 开源版本"` -> `"基于 OrcaSlicer 开源版本"`

### AboutDialog.qml
- Dialog title: `"关于 Creality Print"` -> `"关于 OWzx"`
- Product name: `"Creality Print"` -> `"OWzx Slicer"`
- Version: `"版本 7.0.0.0  (Qt6 QML 重写)"` -> `"版本 2.4.0-dev  (Qt6 QML)"`
- Website: `"www.creality.com"` -> `"www.orcaslicer.org"`

### HomePage.qml
- Banner: `"Creality Print 7.0"` -> `"OWzx Slicer"`
- Login dialog (2 occurrences): `"登录 Creality 账号"` -> `"登录 OWzx 账号"`
- Version footer: `"版本 7.0.0  |  Qt 6.10  |  ©2026 Creality"` -> `"版本 2.4.0-dev  |  Qt 6.10  |  OWzx"`

### PreferencesPage.qml
- Current version: `"v7.0.1 (Qt6 Edition)"` -> `"2.4.0-dev (Qt6 Edition)"`
- Upstream baseline: `"CrealityPrint v7.0.1 (0d4ac73)"` -> `"OrcaSlicer main branch"`
- Update server text: removed "Creality" brand prefix

### ModelMallPage.qml
- Status bar: `"Powered by Creality Cloud"` -> `"Powered by OrcaSlicer Cloud"`

### PrintHostDialog.qml
- Preset name: `"Creality CR-10 SE"` -> `"OrcaSlicer Default"`
- Host type: `"Creality Cloud"` -> `"OrcaSlicer Cloud"` (both property and combo model)
- Host URL: `"https://api.creality.com"` -> `"https://api.orcaslicer.org"`

### hints.json
- 4 hint entries updated: 3 changed from "CrealityPrint" to "OWzx", 1 rephrased neutrally

### ConfigWizardDialog.qml
- No changes needed: only contains upstream vendor preset name "Creality K1C 0.4 nozzle" (preserved)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical Functionality] PreferencesPage version string not in plan**
- **Found during:** Task 1
- **Issue:** Line 578 in PreferencesPage.qml contained `"当前版本：v7.0.1 (Qt6 Edition)"` which was not listed in the plan's Task 1 action items. This is a user-visible version string that should be aligned to OrcaSlicer 2.4.0-dev alongside the other version changes.
- **Fix:** Changed to `"当前版本：2.4.0-dev (Qt6 Edition)"`
- **Files modified:** src/qml_gui/pages/PreferencesPage.qml (line 578)
- **Commit:** 5cb6a5c

## Auth Gates

None encountered.

## Known Stubs

None -- this plan was pure string replacement with no stubs introduced.

## Threat Flags

None -- no new security surfaces introduced.

## Verification

- grep assertion: zero "Creality" brand strings in all 8 target files (outside vendor preset carve-outs): PASSED
- grep assertion: zero old version strings ("7.0", "7.0.0", "7.0.1") in UI files: PASSED
- ConfigWizardDialog vendor preset name "Creality K1C 0.4 nozzle" preserved: CONFIRMED
- Incremental build: NOT attempted (QML string edits do not require recompile for correctness; full build gate deferred to plan 03)
