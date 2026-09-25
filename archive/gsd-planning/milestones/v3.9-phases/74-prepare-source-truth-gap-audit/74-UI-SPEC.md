# Phase 74 UI Contract: Prepare Page Gap Audit

**Phase:** 74 - Prepare Source-Truth Gap Audit
**Status:** Contract for downstream v3.9 UI work
**Target:** `shotScreen/准备页.png`
**Current evidence:** `.planning/milestones/prepare-gap-current-20260704.png`

## Product Surface

The Prepare page is a dense desktop slicer workspace. It should feel like a
work-focused CAD/CAM tool: compact, scan-friendly, predictable, and visually
quiet. It must not look like a marketing interface or a generic dashboard.

## Layout Contract

| Region | Target Contract | Downstream Owner |
|---|---|---|
| Top shell | Compact dark shell with Prepare selected, file/edit controls, and slice/export actions in the top-right area. | Phase 76 |
| Left sidebar | Narrow compact parameter sidebar with printer, filament, process preset, process tabs, and inline option rows. | Phase 75 |
| Object list | Upstream-style object/volume tree and context actions; no decorative oversized toolbar chrome. | Phase 76 |
| Viewport | Large grey workspace with bed/model as the visual focus; tool overlays must not dominate the scene. | Phase 77 |
| Toolbars | Compact icon-first controls near the viewport/bed, with stable disabled states and tooltips. | Phase 77 |
| Plate bar | Plate selection/add/status must align with the bottom/bed-adjacent target layout. | Phase 76 |
| Slice/status | Slice/export readiness and status must be honest and placed consistently with the target. | Phase 76 |

## Visual Rules For Implementation Phases

- Use icons for tool actions where an icon exists; do not use large text glyph
  buttons for common tool commands when a symbol is expected.
- Keep card radius at 8px or less. The current 12px pill/card style is a visual
  mismatch for this domain.
- Do not nest cards inside cards in the Prepare sidebar.
- Do not scale font size with viewport width.
- The left sidebar must be dense but readable; option rows should not become
  oversized dashboard cards.
- Every visible option label must be user-facing and upstream-mapped. Raw keys
  like `Layer height`, `print`, or `default` are not acceptable final UI labels
  when the target displays localized setting names.
- Disabled/blocked controls must be honestly disabled with a reason; visible
  controls must not be no-op.
- Toolbars and floating panels must have stable dimensions so hover/disabled
  states do not shift the viewport layout.

## Required States

Each downstream phase must cover at least these states for its owned region:

- empty project,
- one model loaded,
- one selected model,
- slicing not available,
- slicing available,
- slicing in progress,
- blocked/disabled tool with a visible reason.

## Verification Expectations

- Source audit: every changed visible control maps to an upstream source file or
  a documented block.
- QML audit: no visible placeholder copy, raw internal labels, or dead controls
  remain in the owned region.
- Runtime evidence: Phase 78 must capture the Prepare page and compare it
  against `shotScreen/准备页.png`.
