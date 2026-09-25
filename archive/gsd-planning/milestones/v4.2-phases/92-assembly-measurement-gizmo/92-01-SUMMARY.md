---
phase: 92-assembly-measurement-gizmo
plan: 01
subsystem: qml-gui
tags: [assembleview, measurement-gizmo, ctrl-y, glgizmoassembly, only-assembly, rhi-overlay]
requires:
  - .planning/phases/89-assembleview-source-truth-gap-audit/89-GAP-MATRIX.md
  - .planning/phases/90-assembleview-shell-and-canvas-host-restoration/90-01-SUMMARY.md
  - .planning/phases/91-explosion-ratio-and-assembly-rendering/91-01-SUMMARY.md
  - .planning/phases/92-assembly-measurement-gizmo/92-CONTEXT.md
  - .planning/phases/92-assembly-measurement-gizmo/92-01-PLAN.md
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/core/viewmodels/EditorViewModel.h
provides:
  - GizmoAssemblyMeasure = 19 enum value (distinct from GizmoMeasure = 3) (ASMMEASURE-01)
  - EditorViewModel assembly-measure gizmo state + activability gate (Ctrl+Y)
  - EditorViewModel assemblyMeasure* Q_PROPERTYs (distance/angle/xyz/plane)
  - AssemblyMeasureGeometry C++ helper (distance + angle from two AABBs)
  - RhiViewport assemblyMeasureSelected A/B selection-index properties
  - RhiViewportRenderer assembly-measure overlay (dashed line + arrowheads + teal box)
  - AssemblePage Ctrl+Y shortcut + right-side 测量 panel
  - QmlUiAuditTests measurement wiring audit slot
  - ViewModelSmokeTests activability + geometry math slots
affects: [93-assembleview-verification-and-cleanup]
tech_stack_added: []
patterns: [gizmo-mode enum gating, activability mirror of upstream on_is_activable, AABB-center distance + longest-axis angle simplification, white dashed GL_LINES + translucent-fill overlay]
requirements_completed: [ASMMEASURE-01, ASMMEASURE-02]
completed: 2026-07-09
---

# Phase 92 Plan 01 Summary

## What Changed

Phase 92 plan 01 ports the Assembly measurement gizmo (`Ctrl+Y`,
`GLGizmoAssembly` / `ONLY_ASSEMBLY` measure mode) to the Qt6/RHI
`CanvasAssembleView` host, delivering the three screenshot-visible behaviors
from `shotScreen/装配页_测量.png`: (1) a gizmo invokable via `Ctrl+Y` on
AssembleView with activability mirroring upstream `GLGizmoAssembly::
on_is_activable()`; (2) a measurement (distance + angle) between two selected
volumes computed in C++; (3) an in-world overlay (white dashed dimension line
+ arrowheads + teal value box) plus a right-side 测量 panel. This is the third
v4.2 phase to ship production C++/QML code, and it does not regress
Prepare/Preview.

Production changes (10 files):

- `src/core/viewmodels/EditorViewModel.h` + `EditorViewModel.cpp` - added 5
  assembly-measure `Q_PROPERTY`s (`assemblyMeasureGizmoActive`,
  `assemblyMeasureDistanceText`, `assemblyMeasureAngleText`,
  `assemblyMeasureDistanceXyz`, `assemblyMeasurePlaneText`, all NOTIFY
  `stateChanged`), the `Q_INVOKABLE bool activateAssemblyMeasureGizmo()` /
  `deactivateAssemblyMeasureGizmo()` pair, the
  `assemblyMeasureSelectedSourceIndices()` accessor, and a private
  `isAssemblyMeasureActivable()` helper. `availableGizmoMask()` AssembleView
  branch (which previously early-returned `0`) now returns `(1 << 19)` only
  when `m_activeCanvasType == 2 && abs(m_explosionRatio - 1.0f) < 1e-2f &&
  m_selectedSourceIndices.size() >= 2` (mirrors `GLGizmoAssembly.cpp:53-68`).
  The text accessors compute real measurements via `AssemblyMeasureGeometry`,
  sourcing the two volumes' AABBs from the cached mesh blob
  (`selectedVolumeBoundsForAssemblyMeasure()`).
- `src/core/rendering/AssemblyMeasureGeometry.h` + `AssemblyMeasureGeometry.cpp`
  (NEW) - a pure-data helper with `AssemblyMeasureResult measure(a, b)`
  computing center-to-center Euclidean distance + per-axis XYZ delta + angle
  between the two volumes' longest-AABB-axis directions, plus `longestAxis()`,
  `center()`, `formatDistance()` (3-decimal + " mm"), `formatAngle()`
  (3-decimal + degree glyph). Returns `valid=false` for degenerate AABBs. NO
  QML, NO renderer dependency — unit-testable standalone. Approximates
  upstream `Measure::MeasurementResult` (`Measure.hpp:147-180`).
- `CMakeLists.txt` - appended `AssemblyMeasureGeometry.cpp`/`.h` to the
  EXPLICIT `src/core/rendering/` source list (lines 113-114, after
  `GizmoCenter`). This directory is NOT globbed; without this edit the new TU
  is not compiled (the M-03 critical build step).
- `src/qml_gui/Renderer/RhiViewport.h` + `RhiViewport.cpp` - added
  `GizmoAssemblyMeasure = 19` to the `GizmoMode` enum (after
  `GizmoSlaSupports = 18`; distinct from `GizmoMeasure = 3` = Prepare
  Ctrl+U), and the `assemblyMeasureSelectedA`/`SelectedB` `Q_PROPERTY`s
  (WRITE + NOTIFY `assemblyMeasureSelectionChanged`) whose setters guard,
  store, emit, and call `update()` (the re-render trigger).
- `src/qml_gui/Renderer/RhiViewportRenderer.h` + `RhiViewportRenderer.cpp` -
  added 3 overlay buffers (`m_assemblyMeasureLineBuffer` / `TriBuffer` /
  `ValueBuffer`) + uploaded/bytes/count flags + the selection-index mirror
  members. `synchronize()` copies `m_assemblyMeasureSelectedA`/`B` and sets
  dirty flags on change. `uploadAssemblyMeasureBuffers()` locates the two
  selected volumes' first-batch AABBs, computes via
  `AssemblyMeasureGeometry::measure`, and builds 8 white dashed GL_LINES
  segments + white arrowhead triangles + a teal translucent value-box quad.
  `renderAssemblyMeasureOverlay()` draws the line via `m_linePipeline`, the
  arrowheads via `m_fillPipeline`, and the value box via
  `m_translucentFillPipeline`. The render gate
  (`m_canvasType == CanvasAssembleView && m_gizmoMode == 19`) is strict;
  Prepare/Preview never enter.
- `src/qml_gui/pages/AssemblePage.qml` - added a `Ctrl+Y` Shortcut toggling
  `activateAssemblyMeasureGizmo()`/`deactivateAssemblyMeasureGizmo()`, a
  right-side 测量 panel (Rectangle, `visible` bound to
  `assemblyMeasureGizmoActive`) with header, 装配测量 mode label, plane
  indicator, distance/angle rows, 关闭测量 button, and an activability hint;
  and the `assembleViewport` bindings (`gizmoMode` -> `GizmoAssemblyMeasure`
  when active else `GizmoMove`; `assemblyMeasureSelectedA`/`B` from the first
  two selected source indices). All `qsTr()` + Theme tokens (teal accent
  `#4ec9b0`); no inline business logic beyond routing.

Test changes (2 files):

- `tests/QmlUiAuditTests.cpp` - added
  `assembleViewMeasurementGizmoWiredAndOverlayRenders()` with 5 assertion
  groups (enum, viewmodel activability+state, AssemblePage shortcut+panel,
  renderer overlay gated, geometry helper exists). CJK literals as `\uXXXX`
  escapes.
- `tests/ViewModelSmokeTests.cpp` - added
  `assemblyMeasureGizmoActivabilityMirrorsUpstream()` (6 cases: default
  canvas, AssembleView <2 selected, AssembleView ratio != 1.0, the full
  activable case via `addPrimitiveToPlate` x2, deactivate, canvas-switched
  guard) and `assemblyMeasureGeometryComputesDistanceAndAngle()` (pure-math:
  distance sqrt(272) ~= 16.492, XYZ delta (16,4,0), perpendicular axes, 90
  degree angle, format suffixes, degenerate-AABB invalidity).

## Verification Outcome

- **Canonical build** (`auto_verify_with_vcvars.ps1`): compiled/linked
  `OWzxSlicer.exe` + `libslic3r_from_source.lib` + all test targets with ZERO
  errors (the new `AssemblyMeasureGeometry.cpp` compiled at `[16/243]`).
  Wrapper timed out mid-test-target-link due to the per-invocation libslic3r
  reconfigure cost (Phase 91 deviation; production compile/link is clean).
  See 92-01-VERIFICATION.md.
- **Regression suite** (`run_unit_tests_vcvars.ps1`): exit 0, all 5 suites
  pass — PrepareSceneDataTests (12/0), ViewModelSmokeTests (93/0/1skip),
  QmlUiAuditTests (64/0), PartPlateTests (48/0), PreviewParserTests (9/0).
  The 3 new slots pass.
- **Verification follow-up fix** (commit c3052f9): corrected two test-fixture
  issues found in verification — `loadFile`-replaces vs
  `addPrimitiveToPlate`-additive (the activability test needed >=2 objects),
  and `QT_TESTCASE_SOURCEDIR` path resolution (the QmlUiAudit existence check
  ran from `build/`).

## Scope Simplification (documented deviation)

Phase 92 ships a **minimal-but-correct** measurement matching
`装配页_测量.png` and the activability rules, with the internal feature-picking
engine SIMPLIFIED and DEFERRED. FULL/MATCHING: activability gate, measure
mode enum, overlay visuals, 测量 panel, Ctrl+Y shortcut. SIMPLIFIED:
distance = AABB-center to AABB-center + XYZ delta + Euclidean; angle =
longest-AABB-axis direction angle. DEFERRED (needs ITS + raycaster + data
pool from Phase 93): full per-triangle feature picking (point/edge/circle/
plane), Face-Face/Point-Point mode combo, assembly transforms (parallel/
coincidence/reverse/around-center), feature reset/re-pick, distance-XYZ
editing. The simplification is an internal-engine approximation — the
user-visible semantics (measure distance/angle between two selected volumes)
match upstream. Full table in 92-01-PLAN.md (Scope Decision) and
92-01-VERIFICATION.md.

## Key Implementation Decisions

- **`GizmoAssemblyMeasure = 19` is load-bearing.** Distinct from
  `GizmoMeasure = 3` (Prepare, Ctrl+U) — mirrors upstream `GLGizmoAssembly`
  being a separate class. The AssembleView mask returns `(1 << 19)`; the
  renderer gates on `m_gizmoMode == 19`. No existing enum value was renumbered.
- **Bounds source = parse the cached mesh blob in the viewmodel** (Option B
  from the plan). `selectedVolumeBoundsForAssemblyMeasure()` parses the TLV
  mesh blob (`m_cachedMeshData`) to extract per-volume AABBs for the first two
  selected source indices, unioning batches per `sourceObjectIndex`. This
  avoids adding a new `ProjectServiceMock` surface and reuses data already
  cached for rendering.
- **Arrowheads via `m_fillPipeline`** (not `m_gizmoTriPipeline`). The gizmoTri
  pipeline applies a gizmoCenter+scale displacement in its vertex shader that
  would offset arrowheads; `m_fillPipeline` draws raw world-space triangles
  (white opaque), which is correct for the arrowhead geometry.
- **Measurement geometry in `core/rendering/`** (AGENTS.md: business logic in
  core/). The `AssemblyMeasureGeometry` namespace is free functions with a
  pure-data struct, unit-testable without linking the renderer.

## Files

Production (8 files):
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/rendering/AssemblyMeasureGeometry.h` (NEW)
- `src/core/rendering/AssemblyMeasureGeometry.cpp` (NEW)
- `CMakeLists.txt`
- `src/qml_gui/Renderer/RhiViewport.h`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.h`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `src/qml_gui/pages/AssemblePage.qml`

Tests (2 files):
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

## Commits (10 atomic + 1 verification fix + 1 verification doc)

```
152c869 feat(92-01): add GizmoAssemblyMeasure state + activability to EditorViewModel
3221f3b feat(92-01): add GizmoAssemblyMeasure enum + selection props to RhiViewport
343e754 feat(92-01): add AssemblyMeasureGeometry C++ helper + CMakeLists (M-03)
fc35094 feat(92-01): wire AssemblyMeasureGeometry into EditorViewModel accessors
4774fcb feat(92-01): add Assembly measure overlay to RhiViewportRenderer
6124fbf feat(92-01): add Ctrl+Y shortcut + right-side 测量 panel to AssemblePage
e2913f2 feat(92-01): bind gizmoMode + assemblyMeasureSelected A/B to GLViewport
21e1e84 test(92-01): add QmlUiAuditTests slot for Assembly measure wiring audit
0500d8d test(92-01): add ViewModelSmokeTests slots for assembly measure activability + geometry
c3052f9 fix(92-01): correct two test-fixture issues found in verification
bf852b8 docs(92-01): add VERIFICATION — canonical build zero errors + 5 suites pass
```
