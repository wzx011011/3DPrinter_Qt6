---
phase: 144
name: Emboss Font Loading And Text2Shapes Extrude
status: passed
verified: 2026-07-17
requirements_covered:
  - EMB-01
  - EMB-02
---

# Phase 144 Verification

**Status:** passed

## Requirements Coverage (2/2)

| Req | Description | Status | Evidence |
|---|---|---|---|
| EMB-01 | Real font loading replaces synchronous mock; system + bundled fonts enumerate and select | passed | `ProjectServiceMock::embossFontList()` wraps `Slic3r::Emboss::get_font_list` (Windows registry + folder fallback). `setEmbossFont(QString)` stores path. `addTextVolume` uses `m_embossFontPath` (falls back to arial/DejaVu). EditorViewModel exposes `embossFontPath` Q_PROPERTY + `embossFontList()` Q_INVOKABLE. Note: the "replaces synchronous mock" framing was a misunderstanding — the pre-v5.0 implementation was already real (called text2shapes + polygons2model), just hardcoded. Phase 144 added enumeration + selection on top. |
| EMB-02 | Text-to-shape via upstream `Emboss::text2shapes`; shapes extruded with user-controlled height/depth; result is a real TextEmboss volume with non-empty geometry | passed | `text2shapes` + `polygons2model` were already called; Phase 144 parameterized them via `m_embossHeight` (drives `FontProp.size_in_mm`) and `m_embossDepth` (drives `ProjectZ` depth), replacing the hardcoded 10mm/2mm. EditorViewModel forwards the Q_PROPERTY values before invoking addTextVolume. |

## Build Evidence

- OWzxSlicer.exe links clean (4/4 ninja steps, NINJA_EXIT=0).
- One mid-execution typo (`m_mockVolume` → `m_mockVolumes`) caught + fixed.
- No LNK errors, no FAILED on the final build.

## Test Evidence

| Test group | Result | Notes |
|---|---|---|
| QmlUiAuditTests | 103/103 PASS | +1 from 102 — new `v50EmbossParameterized` slot; v4.6/v4.7/v4.8/v5.0 anchors all still PASS |

## Notes

- The "real EmbossJob async pipeline" (EMB-03, planned for Phase 145) is the next piece. Phase 144's synchronous text2shapes call is fine for short text; async matters for long text or rapid edits.
- The Emboss panel UI work (EMB-04, planned for Phase 145) will surface the new `embossFontPath` + `embossFontList` to the user — Phase 144 wired the C++ side; Phase 145 wires the QML side.
