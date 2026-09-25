# Phase 16: PartPlate Data Model Foundation - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Introduce a Qt6-native `PartPlate`-equivalent value object and `PartPlateList`-equivalent container under a new `src/core/model/` layer, and migrate `ProjectServiceMock`'s plate storage off the parallel-vector shell (`int plateCount_` + 9 scattered `QList` vectors) onto the new data model as the single source of truth.

**No new user-visible features.** Existing plate operations (add/delete/rename/lock/select) must preserve their current QML/viewmodel behavior (PLATE-06). This phase is the foundation every later v3.0 phase (17 lifecycle, 18 3MF persistence, 19 slice scheduling) builds on.

**In scope:** PartPlate + PartPlateList classes; migration of ProjectServiceMock storage; re-backing existing plate API on the new model; unit tests for the new model.
**Out of scope:** clone/duplicate/reorder/per-plate-printable (Phase 17); 3MF write path (Phase 18); slice context refactor (Phase 19); any GL/wx rendering code (~65% of upstream PartPlate.cpp is irrelevant to Qt6); any new UI.

</domain>

<decisions>
## Implementation Decisions

All four decisions share a single direction: **complete source-truth alignment, no half-measures**. The user consistently chose the most thorough upstream-faithful option, accepting larger Phase 16 change surface in exchange for no deferred migration debt.

### Class Location & Boundaries
- **D-01:** New `PartPlate` and `PartPlateList` classes live in a **new `src/core/model/` directory** as pure domain objects (POD/value types, NO Qt signals/Q_PROPERTY/Q_OBJECT). Files: `src/core/model/PartPlate.h/.cpp` + `src/core/model/PartPlateList.h/.cpp`.
  - `ProjectServiceMock` owns a `PartPlateList` member and acts as the Qt adaptation layer (exposes Q_PROPERTY/Q_INVOKABLE that read from the model).
  - This separates domain logic from Qt wiring, enables independent unit testing of the model, and aligns with ARCHITECTURE.md "business logic in core/".
- **D-02:** `PartPlate` mirrors upstream `PartPlate.hpp:77-557` field structure for the data/lifecycle/IO subset ONLY. **Exclude** all GL rendering fields (`m_triangles`, `GLModel`, `PickingModel`, `m_quadric`, `m_hover_id`, textures), wxWidgets fields (`wxCoord`, `wxGetApp`), and cereal serialization (Qt6 does its own persistence). Include: plate_index, name, geometry (origin/width/depth/height), instance-pair membership, config, locked, printable, slice-state-machine flags (ready_for_slice/slice_result_valid/apply_invalid).

### Member Relationship Granularity
- **D-03:** PartPlate uses **instance-level membership** (`std::set<std::pair<int,int>>` of object+instance, mirroring upstream `obj_to_instance_set` at `PartPlate.hpp:93`). This is complete source-truth alignment — can express "some instances of one object on plate A, others on plate B" and "instance outside plate" tracking.
  - Consequence: existing `plateObjectIndices(QList<int>)` API semantics must be redesigned to surface instance-level info (or provide a derived object-index view on top of instance pairs). Load path must parse the `objects_and_instances` instance dimension from 3MF `PlateData`.
  - The deferred "instance_outside_set" (`PartPlate.hpp:94`) may be added now or in Phase 17 — planner's discretion, but the `obj_to_instance_set` structure must land in Phase 16.

### Per-Plate Config Map Form
- **D-04:** PartPlate holds a **native `Slic3r::DynamicPrintConfig m_config`** (mirroring upstream `PartPlate.hpp:159`), NOT a `QHash<QString,QVariant>`. The existing `m_mockPlateOverrides` (`ProjectServiceMock.h:328`) is removed.
  - `setPlateScopedOptionValue`/`plateScopedOptionValue` write/read the DynamicPrintConfig directly (fixing the `return false` stub at `ProjectServiceMock.cpp:1336`).
  - This makes Phase 18 3MF persistence native (upstream `PlateData::config` is DynamicPrintConfig), and Phase 19 slice config merge a native `apply()` operation.
  - QML cannot read DynamicPrintConfig directly — `ProjectServiceMock` provides a QVariant-string adaptation view (key list + value-as-QVariant for the Q_PROPERTY surface), but the config truth is DynamicPrintConfig.

### Migration Strategy
- **D-05:** **Big-bang refactor** — Phase 16 deletes all 9 parallel `QList` vectors (`plateNames_`, `plateLockedStates_`, `plateBedTypes_`, `platePrintSequences_`, `plateSpiralModes_`, `plateFirstLayerSeqChoices_`, `plateFirstLayerSeqOrders_`, `plateOtherLayersSeqChoices_`, `plateOtherLayersSeqEntries_`) and `plateCount_`. `PartPlateList` becomes the single source of truth. Q_PROPERTY (`plateCount`/`currentPlateIndex`) and ~30 Q_INVOKABLE methods are rewritten to read directly from `PartPlateList`.
  - **No bridge/cache layer.** The user explicitly rejected the gradual bridge approach to avoid leaving a temporary two-layer sync debt.
  - **Risk accepted:** single large change surface (~30 methods + load path at `ProjectServiceMock.cpp:4768-4799`), requires close canonical-verify guarding.

### Phase 16 Risk Posture (emergent from the four decisions)
- The combined choices mean Phase 16 touches: new model layer, instance-level semantics (redesigning plateObjectIndices API), DynamicPrintConfig integration, removal of all parallel vectors, and rewriting ~30 Q_INVOKABLE — all in one phase.
- **Planner MUST structure for test-first:** write `PartPlate`/`PartPlateList` unit tests BEFORE migrating `ProjectServiceMock`, and run `auto_verify_with_vcvars.ps1` frequently during the migration.
- If the planner judges the scope too large for one phase, the acceptable split (per scope rules) is: keep all four decisions intact but split Phase 16 into sub-plans (16-01 model+tests, 16-02 ProjectServiceMock migration) — do NOT defer any of D-01..D-05 to a later phase.

### Claude's Discretion
- Exact `PartPlate` C++ field names (mirror upstream `m_` prefix per CONVENTIONS.md, or Qt-style — match the new `src/core/model/` idiom the planner establishes).
- Whether `instance_outside_set` lands in Phase 16 or Phase 17 (the `obj_to_instance_set` structure is required now; the "outside" tracking is adjacent).
- Internal structure of `PartPlateList` (vector of unique_ptr, vector of values, etc.) — match what makes the instance-pair semantics cleanest.
- How `plateObjectIndices` Q_INVOKABLE adapts to instance-level truth (derived view, deprecation + new method, etc.) — preserve current QML callers' behavior.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Migration Rules (governs all v3.0 work)
- `.codex/rules/source-truth-migration.md` — source truth = OrcaSlicer; status terms (Real/Hybrid/Mock/Blocked/Placeholder); "do not mark complete merely because a class exists"; required verification via canonical build command.

### Upstream Source Truth (the data model to mirror)
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:77-557` — `class PartPlate` full field structure (D-02/D-03/D-04 mirror this).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:93` — `obj_to_instance_set` (`std::set<std::pair<int,int>>`) — the instance-level membership D-03 implements.
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:159` — `DynamicPrintConfig m_config` — the config form D-04 implements.
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:559-937` — `class PartPlateList` (m_plate_list vector, m_current_plate, m_print_list, MAX_PLATE_COUNT=36).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp` — lifecycle/IO methods (4407 create_plate, 4484 duplicate_plate [Phase 17], 4554 delete_plate, 6160 store_to_3mf_structure [Phase 18], 6229 load_from_3mf_structure [Phase 18]).

### Gap Analysis (quantified the migration)
- `.planning/audits/2026-06-25-partplate-assembleview-gap.md` — §1 data model gap, §6 3MF round-trip gap, §7 complexity (7727 LOC upstream, ~2700 must migrate).

### Current Qt6 Implementation (what gets migrated)
- `src/core/services/ProjectServiceMock.h:336-361` — the 9 parallel QList vectors + plateCount_ being removed (D-05).
- `src/core/services/ProjectServiceMock.h:49-128` — existing plate Q_PROPERTY/Q_INVOKABLE surface to re-back on PartPlateList (PLATE-06).
- `src/core/services/ProjectServiceMock.cpp:1312-1342` — the stubbed plateScopedOptionValue/setPlateScopedOptionValue (D-04 fixes the `return false` at :1336).
- `src/core/services/ProjectServiceMock.cpp:4768-4799` — load path plate reconstruction (must adapt to instance-level + DynamicPrintConfig).

### Project Conventions
- `AGENTS.md` / `CONVENTIONS.md` — naming (PascalCase classes, `m_` or `_` member prefix, final classes), 2-space indent, `#ifdef HAS_LIBSLIC3R` guards, Q_PROPERTY+NOTIFY patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `Slic3r::DynamicPrintConfig` (libslic3r) — the config type D-04 uses natively; already available via HAS_LIBSLIC3R, used elsewhere in libslic3r.
- `Slic3r::Model` / `ModelObject` / `ModelInstance` (`ProjectServiceMock.h:372` holds `Slic3r::Model* model_`) — instance-pair membership (D-03) indexes into this model's object/instance space.
- Existing `MockLayerSeqEntry` struct (`ProjectServiceMock.h:356-360`) — layer-sequence data shape; moves into PartPlate or stays as a helper type.

### Established Patterns
- **Service-as-Qt-adapter pattern:** `ProjectServiceMock` already exposes libslic3r types through Q_PROPERTY/Q_INVOKABLE with `#ifdef HAS_LIBSLIC3R` guards — D-01 extends this pattern (PartPlateList is the domain object, ProjectServiceMock adapts it to Qt).
- **`final` QObject classes with single `stateChanged()` signal** (CONVENTIONS.md) — but PartPlate/PartPlateList are NOT QObjects (D-01: pure domain objects), so this pattern applies only to ProjectServiceMock's adaptation layer.
- **Test-first risk mitigation:** the user's v2.9 retrospective noted "deterministic test coverage replacing transport confidence" as a winning pattern — Phase 16 should apply it to the new model.

### Integration Points
- `EditorViewModel` reads plate state from `ProjectServiceMock` — its Q_PROPERTY bindings must keep working through the migration (PLATE-06).
- `SliceService::cloneCurrentPlateModel` (`SliceService.cpp:261`) and `startSlicePlate` (`:760`) read plate membership — will be reworked in Phase 19 but must not break in Phase 16.
- 3MF load path (`ProjectServiceMock.cpp:4731` `read_from_archive` → `:4776-4799` reconstruction) — adapts to instance-level + DynamicPrintConfig in Phase 16 (so Phase 18's write path has a matching read path).

</code_context>

<specifics>
## Specific Ideas

- The user's consistent theme across all four decisions: **"do it completely and correctly now, don't leave structural debt for later."** This is a strong signal — when the planner faces a "quick vs correct" trade-off within Phase 16 scope, choose correct.
- No specific UI/UX/behavior references — Phase 16 has no user-visible surface, so this is expected.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within Phase 16 scope. The four gray areas were all implementation-shaping decisions, not scope additions.

Adjacent work correctly belongs to later phases:
- clone/duplicate/reorder/per-plate-printable → Phase 17 (PLATE-03/04/05)
- 3MF write path (store_to_3mf_structure) → Phase 18 (PLATE-07/08/09)
- slice context refactor → Phase 19 (PLATE-10/11)
- AssembleView → v3.1 (PLATE-15)

</deferred>

---

*Phase: 16-PartPlate Data Model Foundation*
*Context gathered: 2026-06-25*
