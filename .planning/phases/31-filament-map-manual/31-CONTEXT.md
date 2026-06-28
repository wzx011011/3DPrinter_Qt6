# Phase 31: Filament Map (Manual) - Context

**Gathered:** 2026-06-28
**Status:** Complete (FMAP-01/02/03 implemented; FMAP-04 Auto deferred to v3.3+)
**Mode:** Smart Discuss (autonomous) — decisions made inline (backend phase, no grey areas requiring user input beyond the documented FMAP-01/02/03 scope)

<domain>
## Phase Boundary

Add per-plate manual filament→extruder mapping (FMAP-01 domain fields, FMAP-02 3MF persistence, FMAP-03 manual assignment API). Auto recommendation (FMAP-04) deferred to v3.3+ (blocked on ToolOrdering).

**In scope:** FMAP-01 (PartPlate filament_maps + filament_map_mode), FMAP-02 (3MF round-trip via PlateData::filament_maps), FMAP-03 (Q_INVOKABLE setPlateFilamentMap/plateFilamentMaps/plateFilamentMapMode).

**Out of scope:** FMAP-04 Auto recommendation (v3.3+, ToolOrdering), full QML UI binding (the Q_INVOKABLE API satisfies Manual mode; a dedicated QML filament-map editor widget is a refinement for a UI phase).
</domain>

<decisions>
- **D-31-1:** filament_maps is `std::vector<int>` on PartPlate (1-based extruder indices, matching upstream PlateData::filament_maps at bbs_3mf.hpp:98). filament_map_mode is int (0=Auto, 1=Manual, mirroring upstream FilamentMapMode).
- **D-31-2:** FMAP-02 persistence via buildPlateDataList populates pd->filament_maps + writes filament_map_mode as a config key (so it round-trips). Load path extracts both into pendingPlateFilamentMaps_/pendingPlateFilamentMapMode_ and applies in the rebuild.
- **D-31-3:** FMAP-03 Manual mode via Q_INVOKABLE setPlateFilamentMap(plateIndex, mode, maps) + plateFilamentMaps/plateFilamentMapMode getters. No new QML widget — the Q_INVOKABLE API is the Manual-mode surface; QML can bind it.
</decisions>
</decisions>

---

*Phase: 31-Filament Map (Manual)*
