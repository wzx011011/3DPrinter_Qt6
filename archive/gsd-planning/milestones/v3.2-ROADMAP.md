# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0 PartPlate Core** — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1 QRhi Rendering** — Phases 23-28 (shipped 2026-06-28)
- 🚧 **v3.2 Multi-Plate Data Polish** — Phases 29-32 (in progress)

## Active Milestone: v3.2 Multi-Plate Data Polish

**Goal:** Polish the PartPlate data model with multi-plate arrangement grid, thumbnail persistence, manual filament-map, and the real-model 3MF round-trip test fixture. AssembleView deferred (XL).

**Requirements:** 10 total (ARRANGE-01..03, THUMB-01..02, FMAP-01..03, FIXTURE-01..02).

## Phases

### Phase 29: Multi-Plate Arrangement Grid

**Goal:** Add plate-grid geometry to PartPlateList (cols, stride, compute_plate_index) and wire arrangeObjects to distribute across plates.
**Depends on:** (none — v3.0 PartPlate model + v3.1 QRhi complete)
**Requirements:** ARRANGE-01, ARRANGE-02, ARRANGE-03

### Phase 30: Thumbnail Persistence

**Goal:** Add top-down 2D footprint thumbnail variant; persist thumbnail data to 3MF.
**Depends on:** (none)
**Requirements:** THUMB-01, THUMB-02

### Phase 31: Filament Map (Manual)

**Goal:** Add filament_maps/filament_map_mode to PartPlate; 3MF round-trip; simple QML Manual-mode UI.
**Depends on:** (none)
**Requirements:** FMAP-01, FMAP-02, FMAP-03

### Phase 32: Test Fixture + Verification

**Goal:** Commit a real-model test fixture; close PLATE-09 round-trip test; milestone verification.
**Depends on:** Phases 29-31
**Requirements:** FIXTURE-01, FIXTURE-02

## Past Milestones

- ✅ **v3.1 QRhi Rendering** — Phases 23-28 (shipped 2026-06-28). Details: `.planning/milestones/v3.1-ROADMAP.md`.
- ✅ **v3.0 PartPlate Core** — Phases 16-22 (shipped 2026-06-26). Details: `.planning/milestones/v3.0-ROADMAP.md`.
- ✅ **v2.9** — Phases 10-15 (shipped 2026-06-25). Details: `.planning/milestones/v2.9-ROADMAP.md`.

## Next Step

Phase 29 is the foundation. Start with:

```text
/gsd-discuss-phase 29
```

---

*Last updated: 2026-06-28 via `/gsd-new-milestone v3.2 Multi-Plate Data Polish`.*
