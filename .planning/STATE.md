---
gsd_state_version: 1.0
milestone: v3.0
milestone_name: PartPlate Core
status: planning
last_updated: "2026-06-25T12:00:00+08:00"
last_activity: 2026-06-25 -- v3.0 milestone started via /gsd-new-milestone
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# Project State

**Milestone:** v3.0 - PartPlate Core
**Status:** Planning (defining requirements + roadmap)
**Next step:** After roadmap approval → `/gsd-discuss-phase 16`

## Current Position

Phase: 16 (PartPlate Data Model Foundation) — context gathered
Plan: —
Status: Ready for planning
Last activity: 2026-06-25 — Phase 16 context gathered via /gsd-discuss-phase

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-06-25)

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Replace the mock plate shell with a real PartPlate-equivalent data model that round-trips multi-plate state through 3MF and supports upstream-equivalent multi-plate slice scheduling.

## Latest Shipped Milestone

**v2.9 Implementation Realignment and Stabilization** — shipped 2026-06-25.

- Phases 10-15, 6/6 plans complete, 28/28 requirements satisfied.
- Canonical verification: `auto_verify_with_vcvars.ps1` exited 0; QML UI audit + E2E pipeline passed.
- Explicit smoke: `ViewModelSmokeTests.exe` → 32 passed; `QmlUiAuditTests.exe` → 7 passed.
- Archives: `.planning/milestones/v2.9-{ROADMAP,REQUIREMENTS,MILESTONE-AUDIT}.md`.
- Git tag: `v2.9`.

## v3.0 Context (carried from v2.9 + gap analysis)

- **Gap analysis:** `.planning/audits/2026-06-25-partplate-assembleview-gap.md` — quantified 5 gap categories (data model, lifecycle, 3MF round-trip, slice scheduling, AssembleView).
- **Key finding:** Qt6 plate surface is "shell-with-real-API but mock-backed". Biggest blocker: 3MF save path serializes only `model_`, losing all plate state (PLATE-07/09).
- **OCCT correction (2026-06-25):** AGENTS.md/CLAUDE.md were updated — OCCT is statically imported (not delay-loaded), `DelayLoadHook.cpp` is dead code, 38 TK*.dll deployed normally. PartPlate does NOT need OCCT.
- **Scope decision:** AssembleView deferred to v3.1 (depends on PartPlate data model landing first).

## Deferred Items

Items acknowledged and deferred at v2.9 milestone close (2026-06-25). Full detail in `.planning/milestones/v2.9-MILESTONE-AUDIT.md`.

| Category | Item | Target |
|---|---|---|
| **v3.1** | AssembleView non-placeholder (bird's-eye multi-plate layout) | v3.1 PLATE-15 |
| **v3.1+** | Multi-plate arrangement, wipe-tower geometry, multi-thumbnail kinds, filament map UI | v3.1 PLATE-16..19 |
| live_verification | MQTT live publish / FTP live upload / RTSP live decode (hardware fixtures) | manual, future |
| calibration | Bed Leveling, Vibration Compensation, Max Volumetric Speed | future milestone |
| feature | ModelMall/Home WebView + publish (Blocked on QtWebEngine/policy) | v3.2 WEB-01 |
| feature | Cloud account/login (Blocked/future) | v3.2 CLOUD-01 |
| feature | Upstream-compatible preset bundle (simplified JSON today) | v3.1 PRESET |
| source_hygiene | Repo-wide mojibake scan (v2.9 Phase 11 scoped to touched files) | one-shot sweep, future |
| source_hygiene | Repo-root build cruft (`resolve_addr.obj`, `test_msc.obj`, `vc140.pdb`) | cleanup, future |
| ui | BBLTopbar Account/ModelStore/Publish buttons kept hidden/disabled | revisit when features land |

**Note:** The `.Codex` (capital C) path-casing gap from v2.9 was NOT yet normalized — still Windows-safe, would 404 on case-sensitive FS. Can be picked up opportunistically in v3.0.

## Handoff

v3.0 requirements defined (`.planning/REQUIREMENTS.md`, 14 requirements PLATE-01..14). Roadmap pending. After roadmap approval:

```text
/gsd-discuss-phase 16
```
