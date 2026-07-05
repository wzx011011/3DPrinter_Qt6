---
gsd_state_version: 1.0
milestone: v3.8
milestone_name: RHI Gizmo Parity
status: completed
last_updated: "2026-07-05T11:01:00+08:00"
last_activity: 2026-07-05 -- Completed quick task 260705-e8x: Fix RHI gizmo depth overlay and viewport drag review findings
progress:
  total_phases: 18
  completed_phases: 18
  total_plans: 8
  completed_plans: 8
  percent: 100
---

# Project State

**Milestone:** v3.8 - RHI Gizmo Parity
**Status:** v3.8 milestone complete
**Next step:** Run milestone audit and complete v3.8 cleanup.

## Current Position

Phase: Milestone v3.8 complete
Plan: N/A
Status: Awaiting next milestone
Last activity: 2026-07-05 -- Completed quick task 260705-e8x: Fix RHI gizmo depth overlay and viewport drag review findings

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
The default RHI rendering path must be functionally complete, not just
visually present.

## Carry-Forward From v3.7 (closed)

- **RHI gizmo silent-dead**: closed by v3.8. RHI now owns the move, rotate,
  scale, cut plane, wipe tower, and precise object-picking scope on the
  default path. The legacy OpenGL `GLViewport*` and `GCodeRenderer*` files
  were removed, and `OWZX_OPENGL` no longer activates a rendering path.

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
| 70 | Rotate + Scale gizmos | Complete |
| 71 | Cut plane + wipe tower | Complete |
| 72 | Precise object picking | Complete |
| 73 | Retire GLViewport + verification | Complete |

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

## Deferred Items

Items acknowledged and deferred at milestone close on 2026-07-04:

| Category | Item | Status |
|---|---|---|
| debug | qrhi-d3d12-crash | fixing |
| quick_task | 260625-0cz-ui | missing |
| quick_task | 260702-1bn-prepare-roadmap-audit-downgrade | missing |
| quick_task | 260702-1q7-p0-collapsiblesection | missing |
| uat_gap | Phase 02 02-HUMAN-UAT.md | partial |
| uat_gap | Phase 43 43-UAT.md | pending_user |
| verification_gap | Phase 02 02-VERIFICATION.md | human_needed |
| verification_gap | Phase 68 68-VERIFICATION.md | human_needed |

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260705-e8x | Fix RHI gizmo depth overlay and viewport drag review findings | 2026-07-05 | 1ba32eb | [260705-e8x-fix-rhi-gizmo-depth-overlay-and-viewport](./quick/260705-e8x-fix-rhi-gizmo-depth-overlay-and-viewport/) |

## Operator Next Steps

- Start the next milestone with /gsd-new-milestone
