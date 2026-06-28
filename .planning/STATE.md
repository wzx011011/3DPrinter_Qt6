---
gsd_state_version: 1.0
milestone: v3.3
milestone_name: Slice Preview Main Flow MVP
status: in_progress
last_updated: "2026-06-28T05:27:00.000Z"
last_activity: 2026-06-28 -- Phase 33 slice-to-preview navigation gate implemented
progress:
  total_phases: 4
  completed_phases: 1
  total_plans: 1
  completed_plans: 1
  percent: 25
---

# Project State

**Milestone:** v3.3 - Slice Preview Main Flow MVP
**Status:** In progress
**Next step:** execute Phase 34, the G-code Preview parser MVP.

## Current Position

Phase: 34 - Not started
Plan: add parser fixture coverage for extrusion modes, reset, layers, travel/extrude split, and tool changes
Status: Phase 33 complete; parser MVP next
Last activity: 2026-06-28 -- Phase 33 implemented automatic Preview navigation after slice completion and added E2E coverage.

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Load model -> slice -> enter Preview -> render non-empty D3D11 QRhi G-code preview.

## Latest Shipped Milestone

**v3.2 Multi-Plate Data Polish** - audited 2026-06-28, status `tech_debt`.

- Phases 29-32 complete.
- Requirements: 8/10 complete; `THUMB-02` and `FIXTURE-02` partial because the shared 3MF writer integration path is deferred.
- Details: `.planning/v3.2-MILESTONE-AUDIT.md` and `.planning/milestones/v3.2-MILESTONE-AUDIT.md`.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 33 | Slice-to-Preview Navigation Gate | Complete |
| 34 | G-code Preview Parser MVP | Not started |
| 35 | D3D11 Preview Rendering Interaction | Not started |
| 36 | Verification and Handoff | Not started |

## Phase 33 Evidence

- Commit: `5a4d37f feat: switch to preview after slicing completes`.
- Runtime behavior: `BackendContext` now handles `SliceService::sliceFinished` by posting the slicing-complete notification and calling `requestSelectTab(tpPreview)`, reusing the existing tab/viewMode linkage.
- Regression: `E2EWorkflowTests::test_backend_switches_to_preview_after_slice` first failed with `ctx.currentPage() == 1`, then passed after the implementation.
- Canonical verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; PrepareScene, PartPlate, QML UI audit, and E2E pipeline tests passed.

## Deferred Items

| Category | Item | Target |
|---|---|---|
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | v3.4+ |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | v3.4+ |
| future | AssembleView | v3.4+ |
| future | Auto filament-map recommendation | v3.4+ |
| future | Wipe-tower geometry + rendering | v3.4+ |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | TBD |
| opportunistic | D3D12 crash root cause | TBD |

## Handoff

Start Phase 34 with a narrow TDD loop:

```text
$gsd-plan-phase 34
$gsd-execute-phase 34 --interactive
```
