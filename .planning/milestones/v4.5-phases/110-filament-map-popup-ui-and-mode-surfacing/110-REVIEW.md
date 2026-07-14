---
status: APPROVED
phase: 110-filament-map-popup-ui-and-mode-surfacing
requirement: FMAP-03 + R-02
verdict: ship
counts: {blockers: 0, high: 0, medium: 0, low: 2, info: 2}
---

# Phase 110 Code Review — Filament-Map Popup UI

## Verdict: APPROVED (ship)

Clean popup, correctly-scoped anti-feature (3-modes-not-4), defensible R-02 clamp at the C++ setter boundary. Q_INVOKABLE write path preserves existing maps array. Theme-import fix verified (19 QML warnings → 0).

## Findings

| # | Severity | Finding |
|---|----------|---------|
| L-1 | low | `onToggled` immediate-apply has no confirm step (misclick silently invalidates slice result). Acceptable for a 3-option mode selector; flag for future UX pass. |
| L-2 | low | Inherit-sentinel handled as bare literal `3` in FilamentGroupPopup.qml:66 (deliberate — using `fmmDefault` name would reintroduce the token the source-audit asserts is absent). Opaque to a future reader; documented, grep-driven coupling. |
| I-1 | info | "two-way binding" comment slightly inaccurate — it's one-way declarative binding + imperative write-back via onToggled. Behavior correct, comment overstates mechanism. |
| I-2 | info | Phase 107 `filamentMapModeRoundTripManualPreserved` now overlaps the Phase 111 fmmManual leg. Both passing is conservative. The Phase 107 SCOPE NOTE is now stale (array gap closed by 111). |

## Verified

- **3-modes-not-4 anti-feature:** FilamentGroupPopup.qml declares exactly 3 mode constants (fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2). No fmmDefault constant. Repeater has exactly 3 entries. String `fmmDefault` does not appear anywhere in the file. Source-audit at QmlUiAuditTests.cpp:4419 asserts `!popup.contains("fmmDefault")`.
- **R-02 clamp:** PartPlate.h:257-260 `if (mode < 0 || mode > 3) mode = fmmDefault;` — covers [0,3], clamps out-of-range to fmmDefault. Grep-assertable. All int entry paths route through it (popup → EditorViewModel → ProjectServiceMock → PartPlate::setFilamentMapMode(int)). Write-side resolves fmmDefault to fmmAutoForFlush before persistence.
- **Maps preservation:** setPlateFilamentMapMode reads current filamentMaps() + forwards unchanged. Only mode changes.
- **QML conventions:** qsTr() on all user-visible strings; no business logic in QML; auto-preview gated on hasAutoMap.

Regression: PartPlateTests 53/53, QmlUiAuditTests 83/83 pass.
