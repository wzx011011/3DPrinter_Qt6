# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)

## Latest Milestone: v3.2 Multi-Plate Data Polish

**Goal:** Polish the PartPlate data model with multi-plate arrangement grid, thumbnail persistence, manual filament-map, and the real-model 3MF round-trip test fixture. AssembleView deferred (XL).

**Audit status:** `tech_debt`
**Requirements:** 8/10 complete; 2 partial and deferred to v3.3+ (`THUMB-02`, `FIXTURE-02`)
**Audit:** `.planning/v3.2-MILESTONE-AUDIT.md`
**Archive:** `.planning/milestones/v3.2-ROADMAP.md`

## Phases

### Phase 29: Multi-Plate Arrangement Grid - Complete

**Goal:** Add plate-grid geometry to PartPlateList (cols, stride, compute_plate_index) and wire arrangeObjects to distribute across plates.
**Requirements:** `ARRANGE-01`, `ARRANGE-02`, `ARRANGE-03` - complete.
**Evidence:** `.planning/phases/29-multi-plate-arrangement-grid/29-VERIFICATION.md`

### Phase 30: Thumbnail Persistence - Partial / Deferred

**Goal:** Add top-down 2D footprint thumbnail variant; persist thumbnail data to 3MF.
**Requirements:** `THUMB-01` complete; `THUMB-02` partial.
**Deferred:** 3MF pixel round-trip is deferred to `THUMB-03` because the writer path is coupled to real GL capture.
**Evidence:** `.planning/phases/30-thumbnail-persistence/30-VERIFICATION.md`

### Phase 31: Filament Map (Manual) - Complete

**Goal:** Add filament_maps/filament_map_mode to PartPlate; 3MF round-trip; simple manual-mode API.
**Requirements:** `FMAP-01`, `FMAP-02`, `FMAP-03` - complete.
**Deferred:** Auto recommendation (`FMAP-04`) remains v3.3+.
**Evidence:** `.planning/phases/31-filament-map-manual/31-VERIFICATION.md`

### Phase 32: Test Fixture + Verification - Partial / Deferred

**Goal:** Commit a real-model test fixture; close PLATE-09 round-trip test; milestone verification.
**Requirements:** `FIXTURE-01` complete; `FIXTURE-02` framework complete but round-trip partial.
**Deferred:** full save/reload assertions are blocked by the same 3MF writer integration gap as `THUMB-02`.
**Evidence:** `.planning/phases/32-test-fixture-and-verification/32-VERIFICATION.md`

## Past Milestones

- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28). Details: `.planning/milestones/v3.1-ROADMAP.md`.
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26). Details: `.planning/milestones/v3.0-ROADMAP.md`.
- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25). Details: `.planning/milestones/v2.9-ROADMAP.md`.

## Next Step

No active milestone is open. Start the next milestone from the carry-forward backlog:

```text
$gsd-new-milestone
```

---

*Last updated: 2026-06-28 via planning correction after v3.2 audit.*
