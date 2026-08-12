---
phase: 226-source-mapped-process-hierarchy
reviewed: 2026-08-12T00:00:00Z
depth: deep
files_reviewed: 5
files_reviewed_list:
  - src/qml_gui/Models/ConfigOptionModel.h
  - src/qml_gui/Models/ConfigOptionModel.cpp
  - src/qml_gui/dialogs/SettingsDialog.qml
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 226: Code Review Report

**Reviewed:** 2026-08-12
**Depth:** deep
**Files Reviewed:** 5
**Status:** issues_found

## Summary

The ordered projection is internally deterministic, but the schema boundary does not actually detect unmapped upstream options, and the non-libslic3r build path now exposes an empty Process hierarchy. Both cases can hide or remove user-visible settings.

## Critical Issues

### CR-01: Schema loading cannot detect unmapped Process options

**File:** `src/qml_gui/Models/ConfigOptionModel.cpp:1329-1342`
**Issue:** `loadFromUpstreamSchema()` constructs the loader input exclusively from `processManifestKeys()` and then calls `loadSchemaFromKeys()`. Consequently, any option newly present in `Slic3r::print_config_def` but missing from the manifest is never loaded at all; `def.get()` is never called for it, so there is no mismatch signal, assertion, or audit-visible failure. The phase's fail-closed contract is therefore bypassed by silently dropping schema options, and a future upstream addition can ship absent from the Process dialog without detection. The focused test only compares the rows that this restricted key list caused to load, so it cannot catch this regression.
**Fix:** Enumerate the complete print schema (or the same upstream key source used by the prior loader), compare each candidate key against `processPageGroupLookup()`, and expose/assert the unmapped set before filtering the Process projection. Keep mapped rows ordered by the manifest, but make a non-empty mismatch fail the load/test rather than silently narrowing the schema input.

## Warnings

### WR-01: Process hierarchy disappears in builds without `HAS_LIBSLIC3R`

**File:** `src/qml_gui/Models/ConfigOptionModel.cpp:1361-1376`
**Issue:** The constructor still seeds `m_options` with the built-in options when `HAS_LIBSLIC3R` is unavailable, but all three Process hierarchy invokables return empty lists in the `#else` branch. `ConfigViewModel` also skips `loadFromUpstreamSchema()` in that configuration, so a no-libslic3r/dev build opens the print settings dialog with no tabs and no rows even though it has options available. This makes the new UI path conditionally unusable and differs from the model's existing fallback behavior.
**Fix:** Provide a fallback manifest projection for the built-in options (assign their page/group metadata from the same manifest where keys exist), or explicitly disable the Process dialog with a visible error when the required backend is absent. Do not return an apparently valid empty hierarchy while retaining editable options.

---

_Reviewed: 2026-08-12_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
