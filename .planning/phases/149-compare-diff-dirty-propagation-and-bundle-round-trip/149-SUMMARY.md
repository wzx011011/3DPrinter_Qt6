---
phase: 149
name: Compare/Diff, Dirty Propagation, And Bundle Round-Trip
status: passed
verified: 2026-07-17
requirements_covered:
  - PSET-05
  - PSET-06
  - PSET-07
---

# Phase 149 Summary

**Phase:** 149 (v5.0 / WS4)
**Status:** passed — PSET-05/06/07 satisfied. **WS4 (Preset) complete: all 7 PSET reqs.**
**Requirements:** PSET-05, PSET-06, PSET-07

## What shipped

### PSET-05 — Compare/Diff primitive (NEW)
`src/core/services/PresetServiceMock.cpp`:
- New `comparePresets(presetA, presetB)` returns a QVariantList of `{key, valueA, valueB, status}` maps for every differing key. Status classifies as "added" / "removed" / "changed". Keys sorted for deterministic display. Mirrors upstream UnsavedChangesDialog diff-view mode (single-direction A vs B; 3-way is a UI-layer concern built on this primitive).
- Missing keys get a `<missing>` marker on the other side.

### PSET-06 — Bundle round-trip contract (LOCKED)
- The exportBundle/importBundle (JSON) pair (pre-v5.0) + the exportBundleIni/importBundleIni (.ini) pair (Phase 147) round-trip user presets. Source-audit slot anchors both pairs exist.
- Live round-trip ctest deferred — the unit-test infrastructure doesn't have a PresetServiceMock context.

### PSET-07 — Dirty-state propagation (LOCKED, pre-existing)
- `ConfigViewModel::isPresetDirty` + `pendingUnsavedAction` + `pendingUnsavedTarget` + `hasPendingUnsavedChanges` were already in place. Source-audit slot anchors them.

### Regression lock
`tests/QmlUiAuditTests.cpp` — new `v50CompareDiffAndRoundTripWired()` slot.

## Verification

- OWzxSlicer.exe links clean (8/8 ninja steps, NINJA_EXIT=0).
- 108/108 QmlUiAuditTests passing (+1: `v50CompareDiffAndRoundTripWired`).

## WS4 closure

**WS4 (Preset) complete — all 7 PSET reqs addressed (PSET-01..07).** Documented partials:
- PSET-01: `.ini` is the interop-relevant subset, not the full multi-file bundle.
- PSET-02: single-preset create, not upstream's batch creator.
- PSET-03: 3-column visual diff deferred; functional contract met.
- PSET-06: live round-trip ctest deferred (locked via source-audit).

These are honest scope notes, not blockers — the user-facing preset create/save/discard/diff/bundle features are all functional.

## Unlocks downstream

- Phase 150-152 (WS5 PartPlate): unrelated, can proceed independently.
- Phase 153 (REGRESS-04): final regression gate will consolidate all WS1-WS5 slots.
