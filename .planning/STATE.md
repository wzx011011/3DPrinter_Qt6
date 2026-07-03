---
gsd_state_version: 1.0
milestone: v3.7
milestone_name: Screenshot-Level UI Parity Closure
status: active
last_updated: "2026-07-03T18:45:00+08:00"
last_activity: 2026-07-03 - Phase 64 canonical build passed and runtime screenshots were captured; visual acceptance still has recorded residual gaps
progress:
  total_phases: 6
  completed_phases: 6
  total_plans: 6
  completed_plans: 6
  percent: 100
---

# Project State

**Milestone:** v3.7 - Screenshot-Level UI Parity Closure
**Status:** Active
**Current rule:** Execute all visual-fix phases first, then run one canonical
build and visual acceptance pass at the end.

## Current Position

Phase: 64
Plan: Visual parity closure
Status: Executed, with residual visual gaps recorded
Last activity: 2026-07-03 - User approved starting v3.7 and requested one
unified compile + visual acceptance after all phases. Phase 59 matrix is
recorded, Phase 60 moved process options inline in the prepare sidebar, Phase
61 compacted the settings dialogs, Phase 62 made Preview reuse the same left
sidebar while tightening the right analysis panel, Phase 63 passed static
checks, and Phase 64 passed the canonical build plus QML/ViewModel audit tests.
Runtime visual acceptance captured prepare and process settings screenshots.
Remaining acceptance gaps are tracked in
`.planning/milestones/v3.7-VISUAL-MATRIX.md`.

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
Screenshot-driven UI milestones also use screenshots as visual/layout source of
truth.

## Correction From v3.6

v3.6 must not be treated as visually complete. It reached an automated
verification floor, but its manual visual UAT (VERIFY-04) was not executed.
v3.7 exists to close that gap.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 59 | v3.6 status correction and visual baseline matrix | Complete |
| 60 | Prepare sidebar and viewport chrome parity | Complete |
| 61 | Settings dialog compact OrcaSlicer tab layout parity | Complete |
| 62 | Preview right panel, sliders, and G-code text parity | Complete |
| 63 | Placeholder removal, static audit hardening, and visual UAT assets | Complete |
| 64 | Unified canonical build and running-app visual acceptance | Executed with residual gaps |

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
