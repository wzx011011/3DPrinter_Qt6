---
slug: bed-render-gaps
date: 2026-08-21
status: complete
---

# Bed render gap closure — Summary

## Outcome

Closed 3 of the 4 bed-rendering gaps from the source-truth analysis; the 4th was
re-classified as upstream dead code after deeper reading.

1. **bed_exclude_area rendering** — DONE.
   Upstream `PartPlate::render_exclude_area` (PartPlate.cpp:856-878) +
   `init_exclude_bounding_box` grouping (`index % 4 == 3`). Data flows
   printer preset (`bed_exclude_area` coPoints) -> `EditorViewModel.bedExcludeAreas`
   (flat point stream, trailing incomplete rectangle dropped) ->
   `RhiViewport::bedExcludeAreas` (bumps scene generation) ->
   `PrepareSceneData::appendExcludeFills` (fan triangulation per 4-point group,
   per-plate grid offset mirroring `set_shape` position offset), drawn inside
   the bed fill buffer between the background fill and the grid with the
   upstream grays: selected `(0.765, 0.7686, 0.7686)`, unselected `(0.9,0.9,0.9)`.
   PreviewPage does not bind the property, so its canvas keeps upstream
   `GCodeViewer::_render_bed` behavior (no exclude overlay).

2. **Height limit indicator** — DONE.
   Upstream `calc_height_limit` (PartPlate.cpp:512-561) + `render_height_limit`
   (:914, gate: print_sequence == ByObject, default mode HEIGHT_LIMIT_BOTH).
   Heights read from the printer preset keys
   `extruder_clearance_height_to_rod/_to_lid` (Plater.cpp:8151-8152); zero when
   the preset layer does not carry them (renderer keeps geometry hidden — same
   inert behavior as an unset upstream). Geometry: corner verticals
   ground->rod + rod ring in BOTTOM color `(0.4,0.4,1.0)`, verticals rod->lid +
   lid ring in TOP color `(0.6,0.6,1.0)` on a dedicated line buffer drawn only
   on non-preview canvases.

3. **In-scene plate icons/name/numbers** — RESOLVED AS ARCHITECTURE MAPPING.
   Upstream `render_icons` is the interactive plate-management UI (delete /
   orient / arrange / lock / settings / rename / move-front with hover+tooltip).
   Qt6 hosts that behavior in the QML plate cards (lock, delete, arrange,
   settings, rename are all wired there). The static number badge
   (`render_only_numbers`) duplicates the QML card identity and is consciously
   not duplicated in-scene.

4. **Assemble grabber/arrows** — NOT A GAP.
   `render_arrows` / `render_left_arrow` / `render_right_arrow` are `#if 0`
   dead code and `render_grabber` has no callers anywhere in this vendored
   upstream tree. Nothing to migrate.

## Key decisions

- Exclude-area wire format is a FLAT double list grouped per rectangle,
  exactly like upstream's single flat `Pointfs`. Nested QVariantList
  construction proved fragile through QVariant converting constructors
  (probe showed inner lists flattening to scalars), so the flat format is
  also the robust one.
- Height-limit gate lives in the PreparePage binding
  (`platePrintSequence(currentPlateIndex) === 2`) because it is per-plate
  presentation state; heights live on the viewmodel as preset-sourced data.

## Verification

- Canonical verifier `scripts/auto_verify_with_vcvars.ps1` exit 0:
  PrepareSceneData 17 (2 new), PartPlate 58, ObjectPicking 7,
  ViewModelSmoke 161, QmlUiAudit 151 (1 new audit),
  ViewportContextMenu 5, PreviewParser 17, E2E 29, app launch OK.
- Two stale-ABI crashes during bring-up were fixed by deleting the affected
  `build/CMakeFiles/<target>.dir` directories (known MSVC class-layout
  staleness pattern), NOT by changing product logic.
