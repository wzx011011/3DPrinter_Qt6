# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9 Implementation Realignment and Stabilization** — Phases 10-15 (shipped 2026-06-25)
- 📋 **v3.0 PartPlate and AssembleView** — candidate (not started; awaiting `$analyzing-source-truth-gap PartPlate and AssembleView` then `$gsd-new-milestone`)
- 📋 **v3.1 Preset System Completion** — candidate (not started)
- 📋 **v3.2 Web, Cloud, Multi-Machine** — candidate (not started; several sub-items Blocked)

## Phases

<details>
<summary>✅ v2.9 Implementation Realignment and Stabilization (Phases 10-15) — SHIPPED 2026-06-25</summary>

- [x] Phase 10: Planning Truth Reset (1/1 plan) — completed 2026-06-25
- [x] Phase 11: Source Hygiene Stabilization (1/1 plan) — completed 2026-06-25
- [x] Phase 12: Calibration Closure for Implemented Modes (1/1 plan) — completed 2026-06-25
- [x] Phase 13: Hybrid Integration Verification (1/1 plan) — completed 2026-06-25
- [x] Phase 14: Visible Placeholder Triage (1/1 plan) — completed 2026-06-25
- [x] Phase 15: Verification and Handoff (1/1 plan) — completed 2026-06-25

**Requirements:** 28/28 satisfied. **Audit:** `tech_debt` (no blockers).
**Full details:** [.planning/milestones/v2.9-ROADMAP.md](milestones/v2.9-ROADMAP.md) · [v2.9-REQUIREMENTS.md](milestones/v2.9-REQUIREMENTS.md) · [v2.9-MILESTONE-AUDIT.md](milestones/v2.9-MILESTONE-AUDIT.md)

</details>

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|---|---|---|---|---|
| 10. Planning Truth Reset | v2.9 | 1/1 | Complete | 2026-06-25 |
| 11. Source Hygiene Stabilization | v2.9 | 1/1 | Complete | 2026-06-25 |
| 12. Calibration Closure for Implemented Modes | v2.9 | 1/1 | Complete | 2026-06-25 |
| 13. Hybrid Integration Verification | v2.9 | 1/1 | Complete | 2026-06-25 |
| 14. Visible Placeholder Triage | v2.9 | 1/1 | Complete | 2026-06-25 |
| 15. Verification and Handoff | v2.9 | 1/1 | Complete | 2026-06-25 |

## Next Step

v2.9 is shipped. Prepare the next milestone:

```text
$analyzing-source-truth-gap PartPlate and AssembleView
```

Then create the next milestone:

```text
$gsd-new-milestone
```

Candidate future scope (from `.planning/milestones/v2.9-REQUIREMENTS.md` "Future Requirements" + v2.9 deferred items):
- **v3.0** — PLATE-01..03: upstream multi-plate ownership, per-plate config overrides, non-placeholder AssembleView.
- **v3.1** — PRESET-01..03: upstream-compatible preset bundles, CreatePresetsDialog, dirty-state prompts.
- **v3.2** — WEB-01 ModelMall/Home WebView (Blocked on QtWebEngine/policy); CLOUD-01 cloud/multi-machine verified-or-blocked state.

Also consider before v3.0 (non-blocking v2.9 tech debt): normalize `.Codex` → `.codex` path casing for case-sensitive-FS/CI safety.

---

*Last updated: 2026-06-25 via `/gsd-complete-milestone v2.9`.*
