---
gsd_state_version: 1.0
milestone: v3.3
milestone_name: Slice Preview Main Flow MVP
status: in_progress
last_updated: "2026-06-28T14:52:00.000Z"
last_activity: 2026-06-28 -- Phase 34 G-code preview parser MVP implemented and verified
progress:
  total_phases: 4
  completed_phases: 2
  total_plans: 2
  completed_plans: 2
  percent: 50
---

# Project State

**Milestone:** v3.3 - Slice Preview Main Flow MVP
**Status:** In progress
**Next step:** execute Phase 35, the D3D11 Preview rendering interaction pass.

## Current Position

Phase: 35 - Not started
Plan: verify and harden D3D11 Preview rendering interaction with the parsed G-code payload
Status: Phase 34 complete; renderer interaction next
Last activity: 2026-06-28 -- Phase 34 implemented parser support for extrusion modes, reset, travel filtering, layers, and tool changes with E2E fixture coverage.

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
| 34 | G-code Preview Parser MVP | Complete |
| 35 | D3D11 Preview Rendering Interaction | Not started |
| 36 | Verification and Handoff | Not started |

## Phase 33 Evidence

- Commit: `5a4d37f feat: switch to preview after slicing completes`.
- Runtime behavior: `BackendContext` now handles `SliceService::sliceFinished` by posting the slicing-complete notification and calling `requestSelectTab(tpPreview)`, reusing the existing tab/viewMode linkage.
- Regression: `E2EWorkflowTests::test_backend_switches_to_preview_after_slice` first failed with `ctx.currentPage() == 1`, then passed after the implementation.
- Canonical verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; PrepareScene, PartPlate, QML UI audit, and E2E pipeline tests passed.

## Phase 34 Evidence

- Commit: `bda6cee feat: harden gcode preview parser`.
- Runtime behavior: `PreviewViewModel` now handles `G0`/`G1`, `M82`, `M83`, `G92 E`, Z layers, tool changes, and travel-hidden `GCV1` payload filtering.
- Regression: `E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter` first failed because the preview parser entry point did not exist, then passed after implementation.
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

Start Phase 35 with a narrow renderer interaction loop:

```text
$gsd-plan-phase 35
$gsd-execute-phase 35 --interactive
```
