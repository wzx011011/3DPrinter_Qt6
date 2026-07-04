---
gsd_state_version: 1.0
milestone: v3.8
milestone_name: RHI Gizmo Parity
status: ready_to_plan
last_updated: "2026-07-04T20:30:39+08:00"
last_activity: "2026-07-04 - Phase 69 complete (move gizmo pick + drag interaction: RHI axis picking, per-axis drag deltas, C++ viewmodel translation, one undo entry per drag, canonical verifier passed)."
progress:
  total_phases: 18
  completed_phases: 14
  total_plans: 5
  completed_plans: 5
  percent: 78
stopped_at: Phase 69 complete (1/1) - ready to plan Phase 70
---

# Project State

**Milestone:** v3.8 - RHI Gizmo Parity
**Status:** Ready to plan
**Next step:** Plan Phase 70, `Rotate + Scale Gizmos`.

## Current Position

Phase: 70
Plan: Not started
Status: Phase 69 complete; ready to plan Phase 70
Last activity: 2026-07-04 - Phase 69 shipped RHI move-gizmo axis picking and
dragging. `RhiViewport` uses `GizmoMath::pickMoveAxis`, `computeRay`, and
`rayToAxisT` for axis selection and per-frame world deltas; drag events are
consumed so camera orbit does not trigger. `EditorViewModel` applies deltas,
invalidates stale slice results, and coalesces the whole drag into one undo
entry. `PreparePage.qml` stays a thin signal bridge. Added focused viewmodel
and QML audit coverage. Canonical verification passed with
`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

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
| 68 | Move gizmo RHI render | Complete (visual verification deferred to Phase 73) |
| 69 | Move gizmo pick + drag interaction | Complete |
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
