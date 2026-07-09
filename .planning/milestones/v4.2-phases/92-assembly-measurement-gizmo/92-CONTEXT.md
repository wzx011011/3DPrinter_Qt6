# Phase 92: Assembly Measurement Gizmo - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 92 ports the Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly` /
`GLGizmoMeasure` ONLY_ASSEMBLY mode) to the Qt6/RHI AssembleView canvas, with
measurement overlays (dimension lines, value boxes) and the right-side "测量"
panel. This is the most technically difficult phase: it ports a measurement
geometry engine + overlay rendering from upstream wxWidgets/ImGui into Qt6/QML.

Phase 92 delivers:
- An Assembly measurement gizmo invokable via `Ctrl+Y` on the AssembleView
  canvas, mirroring `GLGizmoAssembly` activability rules (explosion ratio near
  1.0, ≥2 volumes selected).
- A measurement engine (distances/angles/relations between selected volumes)
  using the upstream `ONLY_ASSEMBLY` measure mode semantics.
- Measurement overlay rendering: white dashed dimension lines with arrowheads,
  teal measurement-value boxes (e.g. `90.000°`), plane-selection indicators
  ("选中 N 平面") — matching `shotScreen/装配页_测量.png`.
- A right-side "测量" panel in `AssemblePage.qml` showing measurement data.

Out of scope for Phase 92:
- AssembleView data pool plumbing (`AssembleViewDataID`/`AssembleViewDataPool`)
  → Phase 93 (ASMROUTE-02).
- Final verification + runtime screenshots + cleanup → Phase 93.
- Arrange / auto-arrangement (already complete) — never in scope.
- All removed network/device/cloud scope.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion (discuss skipped — use ROADMAP goal + gap matrix + codebase conventions)

This phase requires porting a non-trivial measurement geometry engine. Key
decisions are at Claude's discretion, guided by:

1. **Upstream behavior truth** — `GLGizmoAssembly` inherits `GLGizmoMeasure`
   with `ONLY_ASSEMBLY` mode. Read the upstream source thoroughly before
   deciding what to port vs. simplify:
   - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9`
     (`class GLGizmoAssembly : public GLGizmoMeasure`, `Ctrl+Y`,
     `EMeasureMode::ONLY_ASSEMBLY`)
   - `GLGizmoAssembly.cpp:25-68` (constructor + `on_is_activable()`:
     explosion_ratio ≈ 1.0 + ≥2 volumes)
   - `Gizmos/GLGizmoMeasure.hpp` / `.cpp` (the measurement geometry engine +
     overlay rendering base — this is the heavy part)
   - `Gizmos/GLGizmosCommon.hpp:268,274,299` (`AssembleViewDataID`/
     `AssembleViewDataPool` — the data source for the gizmo; Phase 93 owns
     full plumbing, but Phase 92 may need minimal data access)

2. **Qt/RHI integration** — the existing gizmo set (move/rotate/scale/cut) in
   `RhiViewportRenderer` + `EditorViewModel` is the integration pattern. The
   Assembly gizmo should follow the same architecture: a gizmo mode enum
   value + activability check + overlay rendering in the renderer + panel in
   QML. Read `src/core/viewmodels/EditorViewModel.h` (GizmoMode enum,
   availableGizmoMask, setGizmoIfAvailable) and how Prepare gizmos render in
   `RhiViewportRenderer.cpp`.

3. **Scope realism** — the upstream `GLGizmoMeasure` is large (plane selection,
   point/edge/face feature picking, distance/angle computation, ImGui imgui
   panel). Phase 92 must deliver the screenshot-visible behavior (dimension
   lines, value boxes, 测量 panel, plane selection indicators) but may simplify
   the internal measurement engine as long as: (a) distances/angles compute
   correctly for the common multi-volume case, (b) the overlay matches
   `装配页_测量.png`, (c) activability matches upstream. Any simplification
   must be documented in the SUMMARY as a deviation.

### Recommended approach (Claude's Discretion confirmed, but noted for planning)
- Add a gizmo mode enum value (e.g. `GizmoAssemblyMeasure`) to EditorViewModel,
  invokable via `Ctrl+Y`, with activability = `canvasType == AssembleView &&
  explosionRatio ≈ 1.0 && selectedVolumeCount >= 2`.
- Port the measurement geometry (point/edge/face picking + distance/angle
   computation) as a C++ helper (not in QML — business logic stays in C++ per
   project architecture).
- Render the overlay (dashed dimension lines, arrowheads, teal value boxes) in
  the `CanvasAssembleView` branch of `RhiViewportRenderer`.
- Add the right-side "测量" panel in `AssemblePage.qml`.
- If the full `GLGizmoMeasure` engine is too large for one phase, deliver a
  minimal-but-correct measurement (distance + angle between selected volumes'
  features) that matches `装配页_测量.png`, and document what was simplified.

</decisions>

<code_context>
## Existing Code Insights

### Upstream behavior truth (port targets)
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9` —
  `class GLGizmoAssembly : public GLGizmoMeasure`
- `GLGizmoAssembly.cpp:25-29` — `m_measure_mode = EMeasureMode::ONLY_ASSEMBLY`
- `GLGizmoAssembly.cpp:45-51` — `m_shortcut_key = WXK_CONTROL_Y`
- `GLGizmoAssembly.cpp:53-68` — `on_is_activable()`: explosion_ratio ≈ 1.0 on
  assemble canvas + ≥2 volumes selected
- `Gizmos/GLGizmoAssembly.hpp:32` — `on_render_input_window` (the 测量 panel)
- `Gizmos/GLGizmoMeasure.hpp/.cpp` — measurement geometry engine + overlay base
- `Gizmos/GLGizmosCommon.hpp:268,274,299` — `AssembleViewDataID`/
  `AssembleViewDataPool` (data source; Phase 93 owns full plumbing)

### Current Qt gizmo pattern (integration model)
- `src/core/viewmodels/EditorViewModel.h` — `GizmoMode` enum (GizmoMove,
  GizmoRotate, GizmoScale, GizmoCut, GizmoFlatten, GizmoSupportPaint, ...),
  `availableGizmoMask`, `setGizmoIfAvailable()`. Phase 92 adds the Assembly
  measure mode here.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` — renders existing gizmos in
  the View3D branch; Phase 92 adds Assembly measure overlay in the
  CanvasAssembleView branch.
- `src/qml_gui/pages/AssemblePage.qml` — Phase 92 adds the right-side 测量 panel.
- `src/qml_gui/pages/PreparePage.qml:149-166` — shows the `Ctrl+Y`/gizmo
  shortcut binding pattern for existing gizmos.

### Integration points
- EditorViewModel GizmoMode enum + availableGizmoMask + activability.
- RhiViewportRenderer CanvasAssembleView branch (overlay rendering).
- AssemblePage.qml (测量 panel + Ctrl+Y shortcut).
- Keyboard shortcut handling (PreparePage.qml pattern for Ctrl+Y).

### Visual truth
- `shotScreen/装配页_测量.png` — right-side 测量 panel, white dashed dimension
  lines with arrowheads, teal measurement-value boxes (e.g. `90.000°`),
  plane-selection indicators ("选中 N 平面").

</code_context>

<specifics>
## Specific Ideas

- The Assembly gizmo is activable only when `explosion_ratio ≈ 1.0` upstream.
  Since Phase 91 made explosion default to 1.0 and user-adjustable, the
  activability gate must check the current `explosionRatio` value, not assume
  the default.
- The measurement overlay must match `装配页_测量.png`: white dashed lines,
  teal value boxes, plane indicators. These are screenshot-visible and part of
  ASMMEASURE-02's success criteria.
- If porting the full `GLGizmoMeasure` feature-picking engine is too large,
  prioritize: (1) distance between two selected points/features, (2) angle
  measurement, (3) the overlay visuals, (4) the 测量 panel. Document any
  simplification.

</specifics>

<deferred>
## Deferred Ideas

- AssembleView data pool full plumbing (`AssembleViewDataID`/
  `AssembleViewDataPool`) → Phase 93 (ASMROUTE-02).
- Final verification + runtime screenshots against `装配页_测量.png` → Phase 93.
- Any measurement features beyond distance/angle/plane-selection that upstream
  supports but are not screenshot-visible → future, document as simplification.

</deferred>

---

*Phase: 92-assembly-measurement-gizmo*
*Context gathered: 2026-07-09 (discuss skipped via workflow.skip_discuss; recommended approach noted for planning)*
