---
gsd_state_version: 1.0
milestone: v3.1
milestone_name: QRhi High-Performance Prepare/Preview Rendering
status: executing
last_updated: "2026-06-27T06:28:44.000Z"
last_activity: 2026-06-27 -- Phase 24 Plan 03 complete
progress:
  total_phases: 6
  completed_phases: 1
  total_plans: 7
  completed_plans: 6
  percent: 43
---

# Project State

**Milestone:** v3.1 - QRhi High-Performance Prepare/Preview Rendering
**Status:** Ready to execute
**Next step:** `$gsd-execute-phase 24`

## Current Position

Phase: 24
Plan: 24-04-PLAN.md
Status: Ready
Last activity: 2026-06-27 -- Phase 24 Plan 03 complete

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-06-27)

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Phase 24 — prepare scene data and plate rendering

## Active Milestone Summary

**Goal:** Establish the Qt-native high-performance rendering foundation for Prepare and Preview using QRhi with D3D12-first/D3D11 fallback.

**Phases:**

- Phase 23: QRhi Renderer Foundation And Backend Gate
- Phase 24: Prepare Scene Data And Plate Rendering
- Phase 25: Prepare Model Mesh Rendering And Camera Interaction
- Phase 26: Preview G-Code GPU Pipeline
- Phase 27: Preview Interaction And Performance Gate
- Phase 28: Fallback, Verification, Reviews, And Handoff

**Requirements:** 26 active requirements in `.planning/REQUIREMENTS.md`, all mapped to phases.

## Latest Shipped Milestone

**v3.0 PartPlate Core** - shipped 2026-06-26.

- Phases 16-22 (5 mainline + 2 review-driven), 7/7 plans complete, 14/14 requirements satisfied.
- Canonical verification: `auto_verify_with_vcvars.ps1` exited 0; QML UI audit + E2E pipeline passed.
- Explicit smoke: `ViewModelSmokeTests.exe` reported 44 passed, 0 failed, 1 skipped; `QmlUiAuditTests.exe` reported 8 passed.
- Two review cycles (code Phase 21 + UI Phase 22): 4 P0/P1 findings all fixed and regression-guarded.
- Audit: `.planning/milestones/v3.0-MILESTONE-AUDIT.md` status `tech_debt`, review-clean.
- Archives: `.planning/milestones/v3.0-{ROADMAP,REQUIREMENTS,MILESTONE-AUDIT}.md`.
- Git tag: `v3.0`.

## Deferred Items

Items acknowledged and deferred at v3.0/v3.1 planning.

| Category | Item | Target |
|---|---|---|
| feature | AssembleView non-placeholder bird's-eye multi-plate layout | v3.2 |
| feature | Multi-plate arrangement, wipe-tower geometry, multi-thumbnail kinds, filament-map UI | v3.2 |
| feature | Preset System completion, upstream-compatible bundles, CreatePresetsDialog | v3.3 |
| vulkan | Replace/rebuild Qt 6.10 with public QtGui Vulkan support and rerun backend benchmark | future spike |
| test_debt | PLATE-09 full round-trip test needs a real-model fixture so store_bbs_3mf has geometry | v3.2 candidate |
| code_quality | canSlice() state-machine field unused; wire or remove | v3.2 candidate |
| code_quality | bed-type dual-source (PartPlate field + config key); document or consolidate | v3.2 candidate |
| perf | Canonical full-verify slow; ninja incremental miss on test Q_OBJECT changes | v3.1+ opportunistic |
| perf | m_print_list caching from upstream Print reuse-map | future |
| portability | `.Codex` path casing diverges from git-tracked lowercase `.codex` | future cleanup |
| feature | ModelMall/Home WebView + publish blocked on QtWebEngine/policy | v3.4 |
| feature | Cloud account/login | v3.4 |
| live_verification | MQTT/FTP/RTSP live hardware verification | manual/future |

## Handoff

Discuss the next v3.1 phase:

```text
$gsd-discuss-phase 24
```

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|---|---|---|---|
| 260627-04s | Document upstream OrcaSlicer issue avoidance constraints | 2026-06-27 | pending | [260627-04s-document-upstream-orcaslicer-issue-avoid](./quick/260627-04s-document-upstream-orcaslicer-issue-avoid/) |
