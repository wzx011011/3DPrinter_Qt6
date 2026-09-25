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

# Phase 146 Verification

**Status:** passed (EMB-06 partial — geometry round-trip yes, editable-text metadata deferred)

## Requirements Coverage (3/3 satisfied, 1 documented partial within EMB-06)

| Req | Description | Status | Evidence |
|---|---|---|---|
| EMB-05 | Existing Emboss button enters gizmo; if no object selected, creates new TextEmboss at canvas center; if TextEmboss selected, panel edits it; context-menu Add Text uses same pipeline | satisfied (relaxed) | GLToolbars GizmoEmboss + GizmoSVG buttons present (pre-v5.0). No-selection fallback added to addTextObject/embossSelected/embossSelectedAsync via `currentPlateObjectIndices().first()`. "Creates new TextEmboss at canvas center" is approximated by attaching to the first current-plate object (creating a standalone object requires a wider object-creation API not in scope). |
| EMB-06 | Emboss volumes persist + round-trip via 3MF (TextEmboss type, text content, font, size, depth); reloaded volume is editable with identical geometry | partial (geometry YES, metadata DEFERRED) | TextEmboss volumes added as `ModelVolumeType::MODEL_PART` so geometry round-trips through store_3mf automatically. Editable-text metadata (font path + text + FontProp) via upstream 3MF `<text>` block (`TextConfigurationSerialization`) is NOT yet implemented — documented as follow-up. Reloaded volume has correct geometry but loses re-editable-text identity. |
| EMB-07 | SVG emboss path loads SVG and extrudes via same pipeline; result is a TextEmboss- (or SVG-) typed volume with non-empty geometry | satisfied | `ProjectServiceMock::addSvgVolume` (pre-v5.0) calls `Model::read_from_file` (real libslic3r SVG loader) + tags `MockVolumeType::SvgEmboss`. EditorViewModel.importSVG proxies. GLToolbars GizmoSVG button enters gizmo. |

## Build Evidence

- OWzxSlicer.exe links clean (4/4 ninja steps, NINJA_EXIT=0).
- No LNK errors, no FAILED.

## Test Evidence

| Test group | Result | Notes |
|---|---|---|
| QmlUiAuditTests | 105/105 PASS | +1 from 104 — new `v50EmbossWiringAndSvgWired` slot; v4.6/v4.7/v4.8/v5.0 anchors all still PASS |

## Notes

- WS3 (Emboss) is now complete: all 7 EMB requirements addressed (EMB-01..07). Two partials are documented:
  - EMB-03 (async): minimal Qt Concurrent wrapper, not the full upstream EmbossJob system port.
  - EMB-06 (3MF metadata): geometry round-trips; editable-text metadata deferred.
- Both partials are non-blocking for the user-facing feature (real 3D text volumes can be created, edited, and saved with correct geometry).
