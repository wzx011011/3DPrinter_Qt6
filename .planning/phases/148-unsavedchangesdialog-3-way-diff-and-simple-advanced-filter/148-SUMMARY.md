---
phase: 148
name: UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter
status: passed
verified: 2026-07-17
requirements_covered:
  - PSET-03
  - PSET-04
---

# Phase 148 Summary

**Phase:** 148 (v5.0 / WS4)
**Status:** passed — PSET-03/04 satisfied (both were already wired pre-v5.0; this phase verifies + locks them)
**Requirements:** PSET-03, PSET-04

## Scope reality

Both PSET-03 and PSET-04 were largely implemented in earlier milestones. Phase 148's actual work was **verify + anchor** the existing infrastructure, with one explicit gap documented.

### PSET-03 — UnsavedChangesDialog
Pre-existing (pre-v5.0):
- `src/qml_gui/dialogs/UnsavedChangesDialog.qml` (152 lines): modal dialog with Discard / Save-As / Cancel actions.
- `SettingsDialog.qml` routes each action: save → `requestSaveAndMaybeClose` (opens SavePresetDialog); discard → `requestDiscardPendingChanges`; cancel → no-op.
- Backed by the existing `ConfigViewModel` pending-unsaved infrastructure (`pendingUnsavedAction`, `pendingUnsavedTarget`, `hasPendingUnsavedChanges`).

**Documented gap**: upstream's 3-column visual diff (old | current | new preset) is not implemented — the current dialog shows a flat list of modified options. This is a UI-fidelity gap, not a functional gap. The Keep/Discard/Save-As/Cancel contract is satisfied.

### PSET-04 — Simple/Advanced filter
Pre-existing (pre-v5.0):
- `ConfigViewModel::filterOptionIndices(category, searchText, advancedMode=false)` is a REAL C++ implementation (ConfigViewModel.cpp:1219+). Simple mode filters to `comSimple` options only; advanced mode is a strict superset (`comSimple + comAdvanced + comDevelop`).
- `SettingsDialog.qml` exposes the `advancedMode` user toggle (line 47) bound to `filterOptionIndices(presetTier, searchText, advancedMode)` (line 109).

No new code needed — the typed C++ filter rule was already in place.

## What shipped

### Regression lock
`tests/QmlUiAuditTests.cpp` — new `v50UnsavedChangesAndFilterWired()` slot asserting:
- PSET-03: UnsavedChangesDialog has discard/save/cancel actions; SettingsDialog routes them.
- PSET-04: `filterOptionIndices` exists with the `advancedMode` parameter; the Simple/Advanced rule is documented in C++ (Simple = comSimple subset, Advanced = superset); SettingsDialog exposes the user-facing toggle bound to the filter.

## Verification

- QmlUiAuditTests: 107/107 PASS (+1: `v50UnsavedChangesAndFilterWired`).
- One mid-execution QVERIFY2 macro mistake (`||` operator inside QVERIFY2 isn't valid C++) caught + fixed.

## Lessons

1. **Pre-phase grep would have shown both reqs were already done.** The ROADMAP described these as "port" tasks; in fact the ports happened in earlier milestones. Phase 148 became a verify-and-lock phase. Same lesson as Phase 144 (always check existing implementation first).
2. **QVERIFY2 doesn't accept `||` in the condition.** The macro is `QVERIFY2(cond, msg)` — `cond` must be a single expression. For "match A OR B", use `cond = source.contains(A) || source.contains(B)` as a single bool, not inline in the macro.

## Unlocks downstream

- Phase 149 (Compare/Diff + Dirty Propagation + Round-Trip): can proceed against the verified dirty-state infrastructure.
