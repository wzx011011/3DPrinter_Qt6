# Roadmap: OWzx Slicer

## Current Milestone: v3.0 PartPlate Core

**Goal:** Replace the mock plate shell (`int plateCount_` + parallel vectors) with a real PartPlate-equivalent data model that fully round-trips multi-plate state through 3MF and supports upstream-equivalent multi-plate slice scheduling.

**Scope:** 5 phases (16-20), 14 requirements (PLATE-01..14). AssembleView deferred to v3.1.

**Gap analysis basis:** `.planning/audits/2026-06-25-partplate-assembleview-gap.md`

---

## Phases

### Phase 16: PartPlate Data Model Foundation

**Goal:** Introduce a Qt6-native `PartPlate`-equivalent value object and `PartPlateList`-equivalent container, and migrate `ProjectServiceMock`'s plate storage off the parallel-vector shell onto the new data model. No new user-visible features yet — this is the foundation every later phase builds on.

**Depends on:** (none — v2.9 baseline complete)
**Requirements:** PLATE-01, PLATE-02, PLATE-06

**Success criteria:**
1. A `PartPlate`-equivalent C++ value object exists under `src/core/` carrying: plate index, name, geometry (origin/width/depth/height), instance-pair membership (`QSet<QPair<int,int>>` or equivalent), per-plate config map, locked state, printable state, and slice-state-machine flags (ready_for_slice / slice_result_valid / apply_invalid).
2. A `PartPlateList`-equivalent container owns the plate objects and is the single source of truth for plate state in `ProjectServiceMock` (replacing `plateNames_`/`plateLockedStates_`/`plateBedTypes_`/etc. scattered vectors).
3. Existing plate operations (add/delete/rename/lock/select) are re-backed by the new data model and preserve their current QML/viewmodel behavior; `auto_verify_with_vcvars.ps1` stays green; existing smoke tests pass.

---

### Phase 17: Plate Lifecycle Completion

**Goal:** Implement the plate lifecycle operations that are currently missing entirely (clone/duplicate, reorder, per-plate printable) and wire them through to QML so users can actually use them.

**Depends on:** Phase 16 (needs the real data model)
**Requirements:** PLATE-03, PLATE-04, PLATE-05

**Success criteria:**
1. User can clone/duplicate a plate: plate state is copied and objects moved to relative positions in the new plate; instance membership rebuilt; matches upstream `PartPlateList::duplicate_plate` (`PartPlate.cpp:4484`).
2. User can reorder plates (move plate to a new index): matches upstream `move_plate_to_index` (`PartPlate.cpp:4895`); current-plate index and all references stay consistent.
3. User can mark a plate printable/non-printable; plates support upstream "shared/unprintable plate" semantics; non-printable plates are excluded from slice-all.
4. New lifecycle operations are exposed via ViewModel/QML and covered by deterministic smoke tests.

---

### Phase 18: 3MF Multi-Plate Persistence

**Goal:** Fix the v2.9-identified blocker: multi-plate state is lost on save. Implement the Qt6-side write path so all plate state round-trips through 3MF.

**Depends on:** Phase 16 (needs the real data model to read plate state from)
**Requirements:** PLATE-07, PLATE-08, PLATE-09

**Success criteria:**
1. Saving a multi-plate project (real `store_bbs_3mf` path) populates `PlateData` from the PartPlate data model before `store_bbs_3mf`, writing: names, locked, bed type, print sequence, spiral mode, filament maps, layer sequences, per-plate config overrides.
2. Loading a 3MF project restores all plate state (not just names + object membership as today), matching what was saved.
3. Deterministic round-trip test passes: build multi-plate project → save → reload → assert plate count, names, locked, bed types, sequences, overrides preserved (PLATE-09 runtime verification).

---

### Phase 19: Per-Plate Slice Scheduling

**Goal:** Replace the "clone global model + hand-patch 3 keys" slice loop with per-plate slice context that honors the full per-plate config map, producing correct G-code for multi-plate / multi-instance / per-plate-wipe-tower cases.

**Depends on:** Phase 16 (data model), Phase 18 (config overrides persisted so they're available at slice time)
**Requirements:** PLATE-10, PLATE-11

**Success criteria:**
1. Per-plate config overrides are read from the real PartPlate config map during slicing (replacing the 3-hardcoded-key patch at `SliceService.cpp:379-403`); arbitrary upstream overrides (filament_maps, print_compatible_*, layer sequences) are honored.
2. Slice-all iterates plates using per-plate slice context (matching upstream `update_slice_context_to_current_plate` semantics); locked plates skipped, empty plates handled, per-plate wipe-tower logic correct.
3. Deterministic smoke test asserts per-plate config injection and slice-all queue behavior.

---

### Phase 20: Verification and Handoff

**Goal:** Final v3.0 evidence bundle. Ensure canonical verification passes, smoke coverage is extended for the new PartPlate behavior, traceability is complete, and the next milestone (v3.1 AssembleView) is prepared.

**Depends on:** Phases 16-19
**Requirements:** PLATE-12, PLATE-13, PLATE-14

**Success criteria:**
1. `auto_verify_with_vcvars.ps1` passes after all v3.0 work: build, app smoke, QML UI audit, CLI/E2E targets, E2E pipeline all green.
2. `ViewModelSmokeTests.exe` is run explicitly and includes deterministic coverage for: PartPlate data model invariants (PLATE-01/02), clone/duplicate/reorder lifecycle (PLATE-03/04), 3MF multi-plate round-trip (PLATE-09), per-plate config override injection (PLATE-10).
3. Each completed v3.0 requirement (PLATE-01..14) has source, test, or manual-verification evidence linked in `REQUIREMENTS.md` traceability.

---

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|---|---|---|---|---|
| 16. PartPlate Data Model Foundation | v3.0 | 2/2 | Complete   | 2026-06-25 |
| 17. Plate Lifecycle Completion | v3.0 | 1/1 | Complete   | 2026-06-25 |
| 18. 3MF Multi-Plate Persistence | v3.0 | 1/1 | Complete   | 2026-06-25 |
| 19. Per-Plate Slice Scheduling | v3.0 | 1/1 | Complete   | 2026-06-26 |
| 20. Verification and Handoff | v3.0 | 0/? | Not started | — |

## Past Milestones

- ✅ **v2.9 Implementation Realignment and Stabilization** — Phases 10-15 (shipped 2026-06-25). Details: `.planning/milestones/v2.9-ROADMAP.md`.

## Future Milestones (candidates)

- 📋 **v3.1** — AssembleView (PLATE-15) + Preset System completion + multi-plate arrangement/wipe-tower/thumbnail polish (PLATE-16..19).
- 📋 **v3.2** — Web (ModelMall/WebView), Cloud/multi-machine — several sub-items Blocked.

---

## Next Step

Phase 16 is the foundation. Start with discussion to clarify the PartPlate value-object design before planning:

```text
/gsd-discuss-phase 16
```

Or skip discussion and plan directly:

```text
/gsd-plan-phase 16
```

---

*Last updated: 2026-06-25 via `/gsd-new-milestone v3.0 PartPlate Core`.*
