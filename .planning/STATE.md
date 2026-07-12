---
gsd_state_version: 1.0
milestone: v4.5
milestone_name: Backlog Closure
status: planning
last_updated: "2026-07-12T00:00:00.000Z"
last_activity: 2026-07-12
progress:
  total_phases: 14
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# Project State

**Milestone:** v4.5 - Backlog Closure (Mega-Milestone)
**Status:** v4.5 roadmap defined; awaiting Phase 103 planning
**Next step:** Plan Phase 103 (CLI Fixture Readiness Gate) with `/gsd-plan-phase 103`.

## Current Position

Phase: 103 (CLI Fixture Readiness Gate) — not started
Plan: —
Status: Roadmap defined; ready to plan Phase 103
Last activity: 2026-07-12 — v4.5 roadmap created (14 phases, 103-116; 20 active requirements mapped across 5 workstreams)

## Current Milestone (v4.5)

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 103 | CLI Fixture Readiness Gate | Not started | FIXTURE-02 |
| 104 | CLI Fixture Recipes And Multi-Material Model | Not started | FIXTURE-01, FIXTURE-03, FIXTURE-04 |
| 105 | D3D12 Debug Layer Wiring | Not started | D3D12-01 |
| 106 | D3D12 Crash Root-Cause And Backend Readiness (time-boxed) | Not started | D3D12-02, D3D12-03 |
| 107 | Filament-Map Mode Enum Widening And 3MF Migration | Not started | FMAP-02 |
| 108 | Filament-Map Auto Recommendation Readback | Not started | FMAP-01 |
| 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | Not started | WTMESH-01, WTMESH-02, WTMESH-03 |
| 110 | Filament-Map Popup UI And Mode Surfacing | Not started | FMAP-03 |
| 111 | Filament-Map Save-Reload Round-Trip | Not started | FMAP-04 |
| 112 | Per-Volume ITS Accessor And Mesh Cache | Not started | MEASURE-01 |
| 113 | Scene And Mesh Raycaster Port | Not started | MEASURE-02 |
| 114 | Measure Engine Instantiation And Feature Readouts | Not started | MEASURE-03 |
| 115 | GLGizmoMeasure Snap UX And Feature Picking | Not started | MEASURE-04 |
| 116 | v4.5 Verification And Cross-Workstream Regression | Not started | WTMESH-04, MEASURE-05 |

**Coverage:** 20/20 active requirements mapped to exactly one phase. MEASURE-06 (Assembly-mode transformation actions) deferred — not mapped.

## Last Completed Milestone: v4.4 Wipe-Tower Geometry Readback And Real Rendering

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 99 | Wipe-Tower Geometry Gap Audit | Complete | WTAUDIT-01, WTAUDIT-02 |
| 100 | Wipe-Tower Geometry Readback | Complete | WTREAD-01, WTREAD-02 |
| 101 | Wipe-Tower Real Rendering Upgrade | Complete | WTRENDER-01, WTRENDER-02 |
| 102 | Wipe-Tower Verification And Regression | Complete | WTVERIFY-01, WTVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-07-12)
See: `.planning/ROADMAP.md` (v4.5 roadmap — 14 phases, 103-116)
See: `.planning/REQUIREMENTS.md` (20 active v4.5 requirements + 1 deferred)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** v4.5 — backlog closure across 5 workstreams (auto filament-map, Option B wipe-tower mesh, CLI fixtures, D3D12 root cause, GLGizmoMeasure engine).

## Milestone Context (v4.5)

**Goal:** Clear the deferred backlog in one cycle across 5 workstreams, extending v4.4's slice/UI work and unblocking long-deferred items.

**Workstream map:**

| WS | Workstream | Phases | Priority | Notes |
|---|---|---|---|---|
| 1 | Auto filament-map recommendation | 107, 108, 110, 111 | P1 | Enum widening BEFORE readback (Pitfall 2); round-trip ships last |
| 2 | Option B real wipe-tower mesh | 109, 116 (regression) | P1 | Re-opens Phase 99 Frozen Decision 2; Option A fallback preserved (Pitfall 3) |
| 3 | CLI fixtures + argv GUI loading | 103, 104 | P1 | Cheapest unblocker; argv plumbing already exists (FIXTURE-02 gate is first) |
| 4 | D3D12 crash root cause + backend readiness | 105, 106 | P2/P3 | Time-boxed investigation; may not produce a clean feature; default promotion out of scope |
| 5 | Full GLGizmoMeasure engine + clipper | 112, 113, 114, 115, 116 | P2 | Per-volume ITS unblocks BOTH raycaster + clipper (Pitfall 6) |

**Key research findings (from `.planning/research/SUMMARY.md`):**

- ZERO new external libraries needed across all 5 workstreams (STACK.md confirmed).
- The v4.4 capture-by-value readback pattern (`WipeTowerGeometry` POD + `wipeTowerGeometryReady`) is the proven backbone for WS1 + WS2 readback.
- Two cross-workstream dependencies: (a) per-volume ITS accessor (Phase 112) unblocks WS5 raycaster + clipper; (b) WS3 readiness gate (Phase 103) is the same gate WS4 needs to repro the D3D12 crash.
- 8 critical pitfalls documented (`.planning/research/PITFALLS.md`): enum-widening 3MF migration (P2), Option A baseline regression (P3), D3D12 debug-layer Release leak (P5), `Measure::Measuring` ITS lifetime/UAF (P6), raycaster per-frame performance (P7).

## Out of Scope for v4.5

- MEASURE-06 Assembly-mode transformation actions (deferred — needs stable feature-picking foundation first).
- D3D12 default-backend promotion before Phase 106 root cause is resolved.
- Vulkan production backend (SDK-blocked).
- Auto filament-map, wipe-tower Option B, GLGizmoMeasure engine, CLI fixtures — all IN scope (this milestone closes them).
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.4 Wipe-tower geometry readback + Option A rendering | Shipped in v4.4 |
| active | Auto filament-map recommendation (WS1) | v4.5 Phases 107, 108, 110, 111 |
| active | Option B real wipe-tower mesh (WS2) | v4.5 Phase 109 (+ 116 regression) |
| active | CLI fixtures + argv GUI loading (WS3) | v4.5 Phases 103, 104 |
| active | D3D12 root cause + backend readiness (WS4) | v4.5 Phases 105, 106 (time-boxed) |
| active | Full GLGizmoMeasure engine + clipper (WS5) | v4.5 Phases 112-115 (+ 116 regression) |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | MEASURE-06 Assembly-mode transformation actions | Future milestone (after v4.5 measure foundation) |
| future | D3D12 default-backend promotion | After Phase 106 root cause + stability proof |
| future | Full PLATE-09 save/reload state assertions | Partially addressed by v4.5 WS3; complete coverage future |
| future | Full i18n translation coverage | Future |

## Scope Guard

- v4.5 is backlog closure across the 5 declared workstreams only.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.
- Do not promote D3D12 to default before Phase 106 closes with a confirmed root cause.
- Do not ship MEASURE-06 (Assembly transformation actions) before MEASURE-01..05 ship the stable feature-picking foundation.
- Do not expose `fmmDefault` as a 4th popup radio button, render Option B color slabs AND mesh together, or ship argv fixtures as a user-facing product feature (anti-features).

## Operator Next Steps

- Plan Phase 103 with `/gsd-plan-phase 103` (CLI Fixture Readiness Gate — Wave A, parallel with Phase 105).
- Phases 103 + 105 are parallel-safe (Wave A); Phase 104 follows 103; Phase 106 follows 105 (time-boxed).
- Phase 107 (WS1 enum widening) MUST precede Phase 108 (readback) per Pitfall 2.
- Phase 109 (WS2 Option B) readback reuses the Phase 108 capture-by-value precedent.

## Deferred Items

Items acknowledged and deferred at v4.4 milestone close (2026-07-12) and carried into v4.5 scope:

| Category | Item | Status |
|---|---|---|
| feature | Option B (real wipe-tower mesh via wipe_tower_mesh_data + convex_hull_3d) | ACTIVE in v4.5 Phase 109 (was LOCKED future per Phase 99 Frozen Decision 2) |
| feature | Auto filament-map recommendation | ACTIVE in v4.5 Phases 107-111 |
| feature | CLI fixtures + argv GUI fixture loading | ACTIVE in v4.5 Phases 103-104 |
| feature | Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | ACTIVE in v4.5 Phases 112-115 |
| feature | D3D12 crash root cause + backend readiness | ACTIVE in v4.5 Phases 105-106 (time-boxed) |
| policy (D3D12-03) | D3D12 default-backend promotion stays OUT OF SCOPE until a confirmed root cause (Phase 106-01 root-cause report at `.planning/research/D3D12-CRASH-ROOT-CAUSE.md`, time-boxed per DR-04). D3D12 remains opt-in via `OWZX_RHI_RENDERER=d3d12`; `defaultWindowsCandidates()` keeps D3D11 first (`RhiBackendSelector.cpp:56-65`, locked by the `d3d12StaysOptInBehindEnvFlag` source-audit slot). Vulkan is SDK-blocked (Qt disables the `vulkan` public feature, `PROJECT.md:143`) and is evaluation-only, NOT a v4.5 deliverable. | DEFERRED — promotion to a future milestone requires a confirmed root cause + clean repro on the original machine |
| feature | MEASURE-06 Assembly-mode transformation actions | DEFERRED to future milestone (P3) |
| process | Nyquist VALIDATION.md files (v4.4 carry-forward) | Carry-forward; v4.5 phases should produce VALIDATION.md |
| evidence | Runtime visual evidence (Windows capture API) | v4.5 WS3 argv fixtures are the chosen workaround |
