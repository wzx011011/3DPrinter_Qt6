---
phase: 144
name: Emboss Font Loading And Text2Shapes Extrude
status: passed
verified: 2026-07-17
requirements_covered:
  - EMB-01
  - EMB-02
---

# Phase 144 Summary

**Phase:** 144 (v5.0 / WS3)
**Status:** passed — EMB-01/02 satisfied
**Requirements:** EMB-01, EMB-02

## Scope reality discovered mid-phase (positive)

Phase 144 was planned as the "LARGEST single phase" because the ROADMAP described porting the 3811-line `GLGizmoEmboss.cpp`. Exploration revealed the actual situation was much better:

**The real Emboss pipeline was already wired** before v5.0:
- `ProjectServiceMock::addTextVolume` (lines 2422+) already called:
  - `Slic3r::Emboss::create_font_file` (load font via stb_truetype — no freetype dependency)
  - `Slic3r::Emboss::text2shapes` (text → 2D polygons)
  - `Slic3r::Emboss::polygons2model` (polygons → 3D indexed triangle set)
  - Creates a real `ModelVolume` with real geometry
- This was NOT a mock — it produced real 3D text geometry.

**The only gaps vs EMB-01/02 were:**
1. Font path hardcoded to `arial.ttf` (no enumeration, no selection)
2. Height/depth hardcoded (10mm/2mm), not driven by Q_PROPERTYs

So Phase 144's actual work was **parameterization + font enumeration**, not a 3811-line port.

## What shipped

### EMB-01 — real font loading + enumeration
- `ProjectServiceMock::embossFontList()` — wraps `Slic3r::Emboss::get_font_list()` (which uses Windows registry enumeration + folder fallback). Returns a `QVariantList` of `{family, path}` maps for QML consumption.
- `ProjectServiceMock::setEmbossFont(QString)` — stores user-selected font path.
- `EditorViewModel` — new `embossFontPath` Q_PROPERTY + `embossFontList()` Q_INVOKABLE proxy.
- `addTextVolume` now uses `m_embossFontPath` (falls back to `arial.ttf`/`DejaVuSans.ttf` if empty).

### EMB-02 — parameterized text2shapes extrude
- `addTextVolume` now reads `m_embossHeight` (drives `FontProp.size_in_mm`) and `m_embossDepth` (drives `ProjectZ` depth) instead of hardcoded 10mm/2mm.
- `EditorViewModel::embossSelected()` + `addTextObject()` forward the Q_PROPERTY values to ProjectServiceMock before invoking `addTextVolume`.
- Defaults (10mm/2mm) preserved when Q_PROPERTY values are unset (backwards compatible).

### Regression lock
- `tests/QmlUiAuditTests.cpp` — new `v50EmbossParameterized()` source-audit slot asserting all EMB-01/02 anchors.

## Verification

- OWzxSlicer.exe links clean (4/4 ninja steps, NINJA_EXIT=0).
- 103/103 QmlUiAuditTests passing (+1: `v50EmbossParameterized`).
- One mid-execution typo (`m_mockVolume` missing trailing `s`) caught by build, fixed, re-verified clean.

## Lessons

1. **Always grep for existing implementation before assuming scope.** Phase 144's "3811-line port" framing was based on the upstream source-file size, not the actual Qt-side state. A 30-second grep for `text2shapes`/`polygons2model` in `ProjectServiceMock.cpp` revealed the pipeline was already wired. This is the same lesson as Phase 143 (verify integration depth) applied positively: the work was smaller than feared, not larger.
2. **stb_truetype is self-contained.** Upstream Emboss uses `imgui/imstb_truetype.h` (single-header truetype parser) — no freetype dependency to wire. This eliminated what could have been a major sub-task.
3. **Don't reformat code you're not actually changing.** My Edit to add the font-path parameter accidentally dropped the trailing `s` from `m_mockVolumes` because I rewrote the whole block instead of a minimal patch. Minimal targeted Edits reduce regression risk.

## Unlocks downstream

- Phase 145 (EmbossJob async + gizmo panel): can proceed against the parameterized pipeline.
- Phase 146 (Emboss wiring + 3MF round-trip + SVG): same.
