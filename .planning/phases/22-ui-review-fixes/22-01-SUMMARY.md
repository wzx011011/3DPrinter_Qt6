---
phase: 22-ui-review-fixes
plan: 01
subsystem: ui
tags: [ui-review, plate-menu, failure-feedback, audit-guard, honest-ui]

requires:
  - phase: 17
    provides: "Phase 17 plate context-menu items (the UI under review)"
provides:
  - "clonePlate/movePlate/setPlatePrintable menu items give failure feedback (UI-1)"
  - "QmlUiAuditTests actively guards Phase 17 menu wiring (UI-3)"
affects: []

key-files:
  created: []
  modified:
    - src/qml_gui/pages/PreparePage.qml
    - tests/QmlUiAuditTests.cpp

requirements-affected: [PLATE-03, PLATE-04, PLATE-05]

duration: 15min
completed: 2026-06-26
---

# Plan 22-01: UI Review-Driven Fixes Summary

**Fixed the 2 UI review findings: plate-op failures now give user feedback, and QmlUiAuditTests actively guards the Phase 17 menu wiring.**

## Performance
- **Duration:** ~15 min (no build-fix)
- **Files modified:** 2

## UI Findings Fixed

### UI-1: plate-op failures now show notifications
- **Before:** clonePlate/movePlate/setPlatePrintable menu items fire-and-forget the editorVm call; return value (false on failure) ignored. User clicks "克隆平板" at MAX_PLATE_COUNT → nothing happens, no feedback.
- **Fix:** `PreparePage.qml:575-604` — each onTriggered captures the return; on false, `backend.postNotification(qsTr(...), qsTr("..."), 1)` (severity 1 = warning). clone's message names the likely cause ("可能已达到最大平板数（36）"). All strings qsTr.
- **Consistency:** all 3 ops (clone, move×2, printable toggle) now give feedback, not just the high-frequency clone failure.

### UI-3: QmlUiAuditTests actively guards Phase 17 wiring
- **Before:** QmlUiAudit checked only generic honest-UI rules (no placeholder text, no empty handlers, no forbidden colors). A regression deleting a Phase 17 menu item / emptying its onTriggered would pass all 7 tests.
- **Fix:** added `plateContextMenuItemsWiredAndNonEmpty()` — asserts PreparePage.qml contains `clonePlate(`/`movePlate(`/`setPlatePrintable(` (each wired) AND has no `onTriggered: {}` anywhere. QmlUiAudit now actively guards the Phase 17 items: 8 passed (was 7).

## Task Commits
- **All tasks:** `c754692` (fix) — single commit; UI feedback + audit guard are cohesive.

## Decisions Made
- **Severity 1 (warning) for all op failures:** these are recoverable user-action failures (try again, free a plate), not system errors. postNotification (not postError) keeps the UX calm.
- **Global empty-onTriggered check** (not just plate-menu region): strengthens the Phase 14 honest-UI contract project-wide in PreparePage, not just the new items.

## Issues Encountered
- None — clean single-pass.

## Remaining UI review notes (non-blocking, v3.1 candidates)
- **UI-2 (discoverability):** Phase 17 ops are only in the right-click menu; no toolbar button/shortcut. Left for v3.1 (matches upstream's right-click-menu approach; adding affordances is a UX design call).

## Next Phase Readiness
- v3.0 is now **review-clean for both code (Phase 21) and UI (Phase 22) P0/P1 findings**. Ready for `/gsd-complete-milestone v3.0`.

---
*Phase: 22-ui-review-fixes*
*Completed: 2026-06-26*
