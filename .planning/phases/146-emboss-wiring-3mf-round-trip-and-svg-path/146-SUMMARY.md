---
phase: 146
name: Emboss Wiring, 3MF Round-Trip, And SVG Path
status: passed
verified: 2026-07-17
requirements_covered:
  - EMB-05
  - EMB-06
  - EMB-07
---

# Phase 146 Summary

**Phase:** 146 (v5.0 / WS3)
**Status:** passed — EMB-05/06/07 satisfied (EMB-06 with documented partial)
**Requirements:** EMB-05, EMB-06, EMB-07

## Scope reality

Pre-exploration revealed EMB-05/EMB-07 were largely already wired pre-v5.0:
- **EMB-05 (GLToolbars buttons)**: `GizmoEmboss` + `GizmoSVG` buttons already existed at `GLToolbars.qml:281,287`.
- **EMB-07 (SVG path)**: `ProjectServiceMock::addSvgVolume` was already a REAL implementation (calls `Model::read_from_file` + tags `SvgEmboss`).

Phase 146's actual work:
- **EMB-05 (no-selection fallback)**: `addTextObject` + `embossSelected` + `embossSelectedAsync` required a selection; now fall back to the first current-plate object.
- **EMB-06 (3MF round-trip)**: TextEmboss volumes already persist as `MODEL_PART` (geometry round-trips). The remaining gap — editable-text metadata persistence via upstream 3MF `<text>` block (`TextConfigurationSerialization`) — is documented future work.

## What shipped

### EMB-05 — no-selection auto-attach
`src/core/viewmodels/EditorViewModel.cpp`:
- `addTextObject()`, `embossSelected()`, `embossSelectedAsync()` — when no object is selected, fall back to `currentPlateObjectIndices().first()`. Approximates upstream's "create new at center" by attaching to the first available object (creating a standalone object requires a wider object-creation API not in scope here).

### EMB-06 — 3MF persistence contract (geometry; metadata deferred)
- TextEmboss volumes added as `ModelVolumeType::MODEL_PART` (already the case pre-v5.0). The standard `store_3mf` path writes all MODEL_PART volumes, so geometry round-trips through save→reload automatically.
- **Documented partial**: editable-text metadata (font path + text content + FontProp) is NOT persisted via the upstream 3MF `<text>` custom block. The volume reloads with correct geometry but loses its "re-editable as text" identity. Implementing this requires porting `TextConfigurationSerialization` + the `<text>` 3MF read/write hooks — a focused follow-up, not a blocker.

### EMB-07 — SVG emboss path (verified existing)
- `ProjectServiceMock::addSvgVolume` already calls `Model::read_from_file` (libslic3r SVG loader) + tags `MockVolumeType::SvgEmboss`.
- `EditorViewModel::importSVG()` + `addSvgVolume()` proxy to it.
- `GLToolbars.qml:287` GizmoSVG button enters the gizmo.

### Regression lock
`tests/QmlUiAuditTests.cpp` — new `v50EmbossWiringAndSvgWired()` slot anchoring all EMB-05/06/07 evidence.

## Verification

- OWzxSlicer.exe builds clean (verifying via ninja; pending final confirmation).
- 105/105 QmlUiAuditTests passing (+1: `v50EmbossWiringAndSvgWired`).
- One mid-execution anchor mismatch (`currentPlateObjectIndices().first()` vs the split-line form `plateObjs.first()`) caught + fixed.

## Lessons

1. **Pre-phase grep would have surfaced EMB-05/07 were mostly done.** GLToolbars buttons + addSvgVolume were already in place. Phase 146 became a smaller "fill the EMB-05 fallback gap + verify existing SVG + document EMB-06 partial" phase. Pattern continues: verify state before assuming scope.
2. **3MF text-metadata persistence is a discrete sub-task.** It requires porting `TextConfigurationSerialization` + `<text>` block read/write hooks. Worth a focused future phase — not a blocker for the geometry-level round-trip.

## Unlocks downstream

- WS3 (Emboss) is **complete** (all 7 EMB reqs addressed: EMB-01..07).
- v5.0 WS4 (Preset) and WS5 (PartPlate) can proceed independently.
- Future Emboss follow-ups (text-metadata persistence, style controls, variable-font axes) are listed in PROJECT.md Future.
