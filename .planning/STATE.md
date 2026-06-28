---
gsd_state_version: 1.0
milestone: v3.3
milestone_name: Slice Preview Main Flow MVP
status: ready_for_audit
last_updated: "2026-06-28T08:20:00.000Z"
last_activity: 2026-06-28 -- Phase 36 verification and handoff complete
progress:
  total_phases: 4
  completed_phases: 4
  total_plans: 4
  completed_plans: 4
  percent: 100
---

# Project State

**Milestone:** v3.3 - Slice Preview Main Flow MVP
**Status:** Ready for milestone audit after manual UAT
**Next step:** test the running app, then run the v3.3 milestone audit/complete lifecycle.

## Current Position

Phase: 36 - Complete
Plan: final verification, UAT notes, review, and handoff
Status: Complete
Last activity: 2026-06-28 -- Phase 36 verification and handoff complete

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
| 35 | D3D11 Preview Rendering Interaction | Complete |
| 36 | Verification and Handoff | Complete |

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

## Phase 35 Evidence

- Commits: `3c34615 test(35-01): add failing audit for preview rhi draw range`, `f8af356 feat(35-01): use exact preview draw spans`.
- Runtime behavior: `RhiViewportRenderer` now builds per-segment draw spans from packed layer/move indices and clamps Preview draw range exactly instead of scaling by vertex count.
- Regression: `QmlUiAuditTests::previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans` first failed under the canonical command, then passed after implementation.
- Canonical verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; PrepareScene, PartPlate, QML UI audit, and E2E pipeline tests passed.
- Startup diagnostics: latest QRhi auto startup selected D3D11 (`selected=d3d11`).

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
| polish | Preview marker at exact final move position | v3.4+ |

## Handoff

v3.3 phases are complete and ready for user UAT in the launched app. After UAT, run the milestone lifecycle:

```text
$gsd-audit-milestone
$gsd-complete-milestone v3.3
```
