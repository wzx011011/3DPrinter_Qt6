# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9 Implementation Realignment and Stabilization** — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0 PartPlate Core** — Phases 16-22 (shipped 2026-06-26)
- 📋 **v3.1 AssembleView + Polish** — candidate (not started; awaiting `$analyzing-source-truth-gap AssembleView` then `$gsd-new-milestone`)
- 📋 **v3.2 Preset System Completion** — candidate (not started)
- 📋 **v3.3 Web, Cloud, Multi-Machine** — candidate (not started; several sub-items Blocked)

## Phases

<details>
<summary>✅ v3.0 PartPlate Core (Phases 16-22) — SHIPPED 2026-06-26</summary>

- [x] Phase 16: PartPlate Data Model Foundation (2 plans)
- [x] Phase 17: Plate Lifecycle Completion (1 plan)
- [x] Phase 18: 3MF Multi-Plate Persistence (1 plan)
- [x] Phase 19: Per-Plate Slice Scheduling (1 plan)
- [x] Phase 20: Verification and Handoff (1 plan)
- [x] Phase 21: Review-Driven Bug Fixes (1 plan, code review)
- [x] Phase 22: UI Review-Driven Fixes (1 plan)

**Requirements:** 14/14 satisfied. **Audit:** `tech_debt` (review-clean).
**Full details:** [v3.0-ROADMAP.md](milestones/v3.0-ROADMAP.md) · [v3.0-REQUIREMENTS.md](milestones/v3.0-REQUIREMENTS.md) · [v3.0-MILESTONE-AUDIT.md](milestones/v3.0-MILESTONE-AUDIT.md)

</details>

<details>
<summary>✅ v2.9 Implementation Realignment and Stabilization (Phases 10-15) — SHIPPED 2026-06-25</summary>

- [x] Phase 10-15. **Requirements:** 28/28. **Details:** `.planning/milestones/v2.9-ROADMAP.md`.

</details>

## Next Step

v3.0 is shipped. Prepare the next milestone:

```text
$analyzing-source-truth-gap AssembleView
$gsd-new-milestone
```

Candidate future scope (from v3.0 deferred items):
- **v3.1** — PLATE-15 AssembleView (bird's-eye multi-plate layout); multi-plate arrangement, wipe-tower geometry, multi-thumbnail kinds, filament-map UI (PLATE-16..19).
- **v3.2** — Preset System completion (upstream-compatible bundles, CreatePresetsDialog, dirty-state prompts).
- **v3.3** — Web (ModelMall/Home WebView, Blocked on QtWebEngine/policy); Cloud/multi-machine verified-or-blocked state.

Also consider before v3.1 (non-blocking v3.0 tech debt): normalize `.Codex` → `.codex` path casing; add a real-model fixture to close PLATE-09; investigate ninja incremental-miss issue to speed up canonical builds.

---

*Last updated: 2026-06-26 via `/gsd-complete-milestone v3.0`.*
