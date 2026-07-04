---
gsd_state_version: 1.0
milestone: v3.8
milestone_name: RHI Gizmo Parity
status: planning
last_updated: "2026-07-04T02:50:00+08:00"
last_activity: "2026-07-04 - v3.7 closed with residual gaps carried to v3.8; v3.8 milestone defined (RHI gizmo/pick/cut/wipe parity + GLViewport retirement)"
progress:
  total_phases: 9
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
stopped_at: v3.8 milestone defined; ready to discuss Phase 65
---

# Project State

**Milestone:** v3.8 - RHI Gizmo Parity
**Status:** Planning
**Next step:** Start Phase 65, `Gizmo Math Extraction + Unit Tests`.

## Current Position

Phase: 65 (not started)
Plan: Not started
Status: Ready to discuss
Last activity: 2026-07-04 - v3.7 closed with residual visual gaps carried
forward. v3.8 milestone defined: port gizmo (move/rotate/scale/cut), wipe
tower, cut plane, and precise object picking from GLViewportRenderer to
RhiViewportRenderer so the default D3D11 RHI path is functionally complete
and GLViewportRenderer (2285 lines) can be retired. Also: RHI pipeline
optimizations landed this session (preview-range cache, uniform buffer
256-byte alignment, MSAA-triggered depth test + highlight no-depth-write
pipeline).

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
| 65 | Gizmo math extraction + unit tests | Pending |
| 66 | Gizmo geometry builders port | Pending |
| 67 | RHI gizmo state wiring | Pending |
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
