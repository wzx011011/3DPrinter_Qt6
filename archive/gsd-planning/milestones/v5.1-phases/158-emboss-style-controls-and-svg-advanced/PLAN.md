# Phase 158: Emboss Style Controls + SVG Advanced Features

**Status:** Ready to execute
**Workstream:** EMBO-F
**Requirements:** EMBO-F01, EMBO-F02

## Goal

Wire the upstream `FontProp` style axes Phase 145 surfaced minimally — boldness
slider, italic flag, use-surface option, curve-projection option — into the
existing Emboss panel, and extend the existing `addSvgVolume` path with a depth
modifier.

## Source-truth scope decision (honest deferral)

Exploration revealed upstream `Emboss.hpp` has **no `ProjectCurve` class** —
only `ProjectZ`, `ProjectScale`, `ProjectTransform` + the free function
`create_transformation_onto_surface`. Curve-projection and use-surface are
projection/placement concepts, NOT FontProp fields. So:

- **boldness + italic** → real FontProp fields (`boldness` / `skew`). Wire fully
  into text2shapes — these actually deform glyphs upstream. Low risk.
- **use-surface + curve-projection** → expose as Q_PROPERTYs + persist into
  TextConfiguration (so the 3MF round-trip carries intent). The actual geometry
  deformation is deferred with a documented TODO referencing the upstream gap
  (no upstream ProjectCurve primitive exists to call). This honors the v5.1
  "low-risk closures, no new architecture" rule.
- **SVG depth-modifier** → simple Z-scale on the imported mesh (real effect).
- **SVG curve-projection** → same scope rule as text: exposed + persisted but
  deformation deferred with a TODO.

## Plan

### Wave 1 — C++ viewmodel + service

1. `src/core/viewmodels/EditorViewModel.h/.cpp`: add 4 Q_PROPERTYs +
   setters/getters + member fields:
   - `embossBoldness` (float, range 0.0–2.0, default 0.0)
   - `embossItalic` (bool, default false)
   - `embossUseSurface` (bool, default false)
   - `embossCurveProjection` (bool, default false)
   Extend all 3 forwarding sites (sync `embossSelected`, async
   `embossSelectedAsync`, `addTextObject` fallback) to push the new fields to
   the service.

2. `src/core/services/ProjectServiceMock.h/.cpp`: add 4 setters + member fields.
   In `performEmbossVolumeAdd` + async worker:
   - Replace hardcoded `font_prop.boldness = 0.0f` with `m_embossBoldness`.
   - Set `font_prop.skew = 0.5` (italic ratio) when `m_embossItalic` is true.
   - Add boldness/italic to the async worker's value-capture list.
   Extend `attachEmbossMetadata` to persist boldness + skew into
   `tc.style.prop` so the style round-trips through 3MF (Phase 155 metadata
   path). use-surface/curve-projection intent are recorded in the volume name
   suffix (in-session affordance; no FontProp field for them upstream).

### Wave 2 — QML panel

3. `src/qml_gui/pages/PreparePage.qml`: extend the Emboss panel (line ~3313)
   with:
   - boldness slider (CxSlider, 0.0–2.0)
   - italic checkbox (CxCheckBox)
   - use-surface checkbox (CxCheckBox) + tooltip noting the upstream gap
   - curve-projection checkbox (CxCheckBox) + tooltip noting the upstream gap

### Wave 3 — SVG depth-modifier

4. `src/core/services/ProjectServiceMock.h/.cpp`: extend `addSvgVolume` to
   `addSvgVolume(int objectIndex, const QString &svgFilePath, float depthModifier = 1.0f)`.
   Apply the modifier as a Z-scale on the imported mesh. Keep backward compat
   (default 1.0 = no change).
5. `src/core/viewmodels/EditorViewModel.h/.cpp`: add a `svgDepthModifier`
   Q_PROPERTY + forward it through `importSVG()`.

### Wave 4 — Test anchor

6. `tests/QmlUiAuditTests.cpp`: add `v51EmbossStyleControlsAndSvgAdvancedWired`
   slot asserting:
   - 4 new Q_PROPERTYs in EditorViewModel.h
   - 4 new setters in ProjectServiceMock.h + member fields
   - `font_prop.boldness = m_embossBoldness` (no longer hardcoded 0.0f)
   - `font_prop.skew` set conditionally on italic
   - attachEmbossMetadata persists boldness + skew
   - `addSvgVolume` signature has the depthModifier param + applies it
   - PreparePage Emboss panel exposes the 4 new controls

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- 5/5 ctest groups PASS
- 12 v5.0 regression slots still pass
- New `v51EmbossStyleControlsAndSvgAdvancedWired` slot passes
