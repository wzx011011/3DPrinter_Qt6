# Phase 79 UI Contract: Preview Page Gap Audit

**Phase:** 79 - Preview Source-Truth Gap Audit
**Status:** Contract for downstream v4.0 UI work
**Target:** `shotScreen/预览页.png`
**Behavior truth:** OrcaSlicer Preview/G-code source and libvgcode

## Product Surface

The Preview page is a dense slicer analysis workspace. It must feel like a
technical inspection view for generated G-code: compact, scan-friendly,
state-rich, and stable while users drag layers, moves, camera, and visibility
controls. It must not look like a generic dashboard or a simplified mock panel.

## Layout Contract

| Region | Target Contract | Downstream Owner |
|---|---|---|
| Top shell | Preview tab selected, dark compact shell, single-plate/slice/export G-code actions placed at the top-right. | Phase 80 |
| Left sidebar | Prepare-like printer/filament/process sidebar remains visible and compact in Preview. | Phase 80 |
| Viewport | Grey workbench with bed-centered toolpath/model; empty state appears only when no Preview data exists. | Phase 80, Phase 81 |
| Right legend/statistics | Compact role table, role colors, visibility affordances, totals, estimates, and metadata. | Phase 80, Phase 82 |
| G-code text panel | Monospace bounded source-line window below the right legend/statistics panel, synchronized to current move. | Phase 80, Phase 81 |
| Vertical layer rail | Far-right layer range/current-layer control with stable numeric labels and no layout jumps. | Phase 81 |
| Bottom move bar | Playback/step control and move slider spanning the bottom workbench area. | Phase 81 |
| Color modes | Screenshot-visible color/view modes are wired to upstream `EViewType` semantics or honestly gated. | Phase 82 |

## Visual Rules For Implementation Phases

- Preserve the verified D3D11/QRhi default path. Do not promote D3D12/Vulkan in
  v4.0.
- Keep the viewport visually dominant. Panels should be dense and compact.
- Do not use nested cards for page regions. Use framed panels only for actual
  repeated/inspection surfaces.
- Use icon-first controls for repeated toolbar actions and playback where a
  common symbol exists.
- Text must fit in each control at desktop and minimum 1100px width. Long G-code
  lines and preset names must elide rather than overlap.
- All user-visible labels in touched Preview QML must be valid localized strings
  or source-truth English names where upstream itself uses English role names.
  Mojibake is not acceptable final UI.
- Disabled/blocked modes must be honestly gated with a reason; visible controls
  must not be no-op.
- Layer, move, role, and camera controls must not clear or repack the Preview
  payload unless the upstream-equivalent state actually requires it.

## Required States

Each downstream phase must cover at least these states for its owned region:

- no model/no G-code loaded,
- slicing in progress,
- preview data loaded from a slice result,
- preview data loaded from previous G-code,
- role visibility toggled,
- color/view mode changed,
- layer range moved,
- current move stepped and played,
- camera orbit/pan/zoom/fit used,
- page switch Prepare -> Preview -> Prepare.

## Verification Expectations

- Source audit: every changed visible control maps to an upstream source file or
  a documented block.
- QML audit: no visible placeholder copy, mojibake, raw internal labels, or dead
  controls remain in the owned region.
- Unit/integration tests: ViewModel and renderer invariants continue to protect
  GCV1 payload survival, role mask shape, global legend range, and current
  G-code line synchronization.
- Runtime evidence: Phase 83 must launch `build/OWzxSlicer.exe`, navigate to
  Preview, capture current visual evidence, and compare against
  `shotScreen/预览页.png`.
