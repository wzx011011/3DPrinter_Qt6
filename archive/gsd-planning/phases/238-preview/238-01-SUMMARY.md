# Phase 238 Summary — Preview Completion

**Completed:** 2026-08-16
**Requirements:** PREV-01..07 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (continuation
agent run + independent re-run before commit; all 8 suites, app launch, E2E).
The first agent run hit an API quota limit after implementing PREV-01..07
(13 files) but before tests; a continuation agent audited the partial work,
closed 2 gaps + 2 real parser bugs, wrote the tests, and verified.

## Changes (16 files)

1. **PREV-01 ghost shells**: preview pass renders semi-transparent object
   meshes behind toolpaths — the prepare mesh staging reused through the new
   `m_translucentFillPipeline` (alpha, no depth write; upstream
   GCodeViewer.cpp:4023 render_shells / :3076 always-loaded). PreviewPage
   feeds meshData for the preview canvas.
2. **PREV-02 tool marker**: RhiViewportRenderer consumes the previously-dead
   markerX/Y/Z + showMarker properties; small marker geometry
   (GizmoGeometry) drawn in the preview pass (upstream Marker::render
   :306-460); buffer rebuild on position change.
3. **PREV-03 Seam/Retract/Unretract/Wipe**: parser classifies E-only moves
   (retract/unretract), G10/G11, wipe regions, and loop-start seams;
   MoveKind enum + show{Retract,Unretract,Wipe,Seam} toggles in
   VisibilityFilter (upstream options_items :913-921). Seam port is the
   faithful upstream loop-first-vertex semantics (GCodeProcessor.cpp:3310+
   record once, close on any non-wall move) — the first draft overwrote the
   flag on every wall extrude and never fired; fixed + regression-tested
   (orca_sample.gcode → exactly 1 seam).
4. **PREV-04 tick completion**: PreviewLayerRail gains a ToolChange picker
   (extruder rows with color swatches → addFilamentChangeAtLayer — first QML
   caller), a ColorChange dialog with extruder + palette (replaces the
   hardcoded extruder-1/#FF0000), tick hover tooltip (time + gcode), and a
   Jump-to-Layer dialog (upstream IMSlider.cpp:1221-1313).
5. **PREV-05 stats split + stealth + cost**: filament usage split into
   Model/Support/Flushed/Tower (flush keyed on unretract inside
   FLUSH_START..FLUSH_END, upstream GCodeProcessor.cpp:3066-3074), shown in
   StatsPanel; stealth total from the parsed silent-mode comment when
   present else the ×1.4 heuristic flagged "(估算)" via
   stealthTimeEstimated; cost from the gcode filament_cost footer (regex
   fixed to allow the real `; filament_cost = 29.99` spacing — the first
   draft's regex never matched real files), defaulting to the preset price.
6. **PREV-06 configured extruder colors**: Tool/ColorPrint modes use
   configuredExtruderColors() (config-driven, invalid-color fallback kept)
   instead of the fixed 8-color cycle; Legend rows gain per-extruder
   visibility checkboxes (toggleExtruderVisibility; upstream m_tool_visibles
   :5088).
7. **PREV-07 software fallback preview**: SoftwareViewport paints the GCV1
   segments with a QPainter orthographic projection (per-role colors,
   layer-clipped, marker as circle) — the fallback path is no longer blank.

## Tests

- PreviewParserTests +8 slots (17/17 green): move-kind classification,
  seam closed/open loop + fixture, toggle payload counts, filament split
  math, stealth comment-vs-heuristic, price/cost strings, configured colors
  (incl. fallback), extruder-visibility gating.
- QmlUiAuditTests +previewCompletionSourceAudit (145/145 green): 24
  PREV-named source assertions (marker consumption, ghost-shell branch,
  addFilamentChangeAtLayer caller, no hardcoded #FF0000, StatsPanel
  split/stealth/price, Legend toggle, SoftwareViewport GCV1).

## Honest gates / upstream deltas (documented in code)

- Retract/unretract/seam render as small tick segments, not GL_POINTS (RHI
  pipeline has no glPointSize).
- G10/G11 recorded as zero-displacement ticks (parser has no printer
  config; upstream synthesizes G1 with the configured retraction length).
- SoftwareViewport: 1px lines, circle marker (fallback-only path).
