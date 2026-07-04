---
gsd_state_version: 1.0
milestone: v3.8
milestone_name: RHI Gizmo Parity
status: in_progress
last_updated: "2026-07-04T12:30:00+08:00"
last_activity: "2026-07-04 - Phase 67 complete: RHI gizmo state pipeline wired (gizmoMode/cutAxis/cutPosition/gizmoCenter flow to RhiViewportRenderer; 7/7 tests pass); ready for Phase 68 (first visible gizmo)"
progress:
  total_phases: 9
  completed_phases: 3
  total_plans: 3
  completed_plans: 3
  percent: 33
stopped_at: Phase 67 verified; ready to discuss Phase 68
---

# Project State

**Milestone:** v3.8 - RHI Gizmo Parity
**Status:** In Progress
**Next step:** Start Phase 67, `RHI Gizmo State Wiring`.

## Current Position

Phase: 66 complete; 67 next (not started)
Plan: 66-01 complete
Status: Phase 66 verified passed; ready to discuss Phase 67
Last activity: 2026-07-04 - Phase 66 (Gizmo Geometry Builders Port) shipped.
Extracted the three gizmo geometry builders (move arrows / rotate torus /
scale shafts+boxes) into a standalone static `GizmoGeometry` class at
`src/core/rendering/`, returning `QVector<GizmoVertex>` with per-axis RGBA
color baked in. Extracted shared POD `GizmoVertex.h` so
`RhiViewportRenderer::Vertex` becomes a `using = GizmoVertex` alias (no
QtQuick dependency in the geometry layer). GLViewportRenderer delegates to
GizmoGeometry and its shaders now consume per-vertex color (location 1 ->
vColor); uGizmoColor uniform removed. 14-slot GizmoGeometryTests registered;
all 16 pass. OWzxSlicer compiles clean. Next: Phase 67 wires the RHI
renderer's synchronize() to read gizmoMode/cutAxis/cutPosition/gizmoCenter
so the gizmo state pipeline is connected on the default D3D11 path.

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
The default RHI rendering path must be functionally complete, not just
visually present.

## Carry-Forward From v3.7 (closed)

- **RHI gizmo silent-dead** (the v3.8 target): gizmoMode/cutAxis/cutPosition
  properties exist on RhiViewport but RhiViewportRenderer::synchronize never
  reads them; no gizmo renders, no pick, no drag on the default path.
  GLViewportRenderer is the only fully functional implementation, gated
  behind OWZX_OPENGL=1.
- **D3D12 segfault at setShaderResources** (QRhi D3D12 backend deep issue):
  uniform buffer 256-byte alignment fix was necessary but not sufficient.
  D3D11 stays default throughout v3.8. Documented in
  `docs/Qt版本升级与RHI渲染管线评估.md`.
- **v3.6 VERIFY-04 manual visual UAT** still pending (independent of v3.8).
- **v3.7 residual visual gaps** tracked in
  `.planning/milestones/v3.7-VISUAL-MATRIX.md`.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 65 | Gizmo math extraction + unit tests | ✓ Complete |
| 66 | Gizmo geometry builders port | ✓ Complete |
| 67 | RHI gizmo state wiring | ✓ Complete |
| 68 | Move gizmo RHI render | Pending |
| 69 | Move gizmo pick + drag interaction | Pending |
| 70 | Rotate + Scale gizmos | Pending |
| 71 | Cut plane + wipe tower | Pending |
| 72 | Precise object picking | Pending |
| 73 | Retire GLViewport + verification | Pending |

## Verification Rule

Per user instruction:

- Do not run the full canonical build after each phase.
- During phases 59-63, use source reads, encoding guard, `git diff --check`,
  and focused static checks only.
- In Phase 64, run:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- After the canonical build, launch `build/OWzxSlicer.exe` and capture
  running-app screenshots for visual acceptance.

## Target Screenshots

- `shotScreen/准备页.png` - 2560x1400
- `shotScreen/预览页.png` - 2560x1400
- `shotScreen/打印机参数设置页.png` - 736x593
- `shotScreen/材料参数设置页.png` - 736x593

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| correction | v3.6 Phase 58 manual visual UAT was not executed | Closed by v3.7 |
| carry-forward | v3.4 Phase 43 manual UAT | Before claiming v3.4 complete |
| superseded | v3.5 Phase 47-49 | Do not resume unless explicitly reopened |
| future | Device send/upload/cloud print and Monitor job lifecycle | Future milestone |
| future | AssembleView | Future source-truth milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| opportunistic | D3D12 crash root cause | Dedicated backend investigation milestone |
