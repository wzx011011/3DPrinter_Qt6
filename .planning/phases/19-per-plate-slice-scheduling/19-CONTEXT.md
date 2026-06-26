# Phase 19: Per-Plate Slice Scheduling - Context

**Gathered:** 2026-06-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace the "clone global model + hand-patch 3 keys" slice config injection with a full per-plate config merge, so arbitrary per-plate overrides (filament_maps, print_compatible_*, layer sequences, etc.) are honored during slicing — not just curr_bed_type/print_sequence/spiral_mode. Also fix the `plateScopedOptionValue`/`setPlateScopedOptionValue` HAS_LIBSLIC3R stubs so per-plate config is readable/writable end-to-end.

**Key finding (changes scope from the plan estimate):** Qt6's `SliceService` already creates a fresh stack-local `Slic3r::Print print;` per `startSlicePlate` call (SliceService.cpp:405), applies the per-plate cloned model + config (`print.apply(*modelForSlice, config)` at :450), slices, and exports. This **already implements per-plate Print isolation** (PLATE-11's core) — each plate gets an independent Print object with its own model + config. The upstream `m_print_list` (map<int,PrintBase*>) is for reuse/caching, not isolation; Qt6's stack-local approach is semantically equivalent for correctness.

So the **actual core work is PLATE-10**: replace the 3-hardcoded-key patch (SliceService.cpp:379-403) with a full `plateConfig.apply()` merge of the per-plate DynamicPrintConfig. PLATE-11 is largely already satisfied (verify + document).

**In scope:** (a) fix `plateScopedOptionValue`/`setPlateScopedOptionValue` HAS stubs to read/write PartPlate::config(); (b) SliceService: replace 3-key patch with full per-plate config merge; (c) tests.
**Out of scope:** Print-object reuse/caching (upstream m_print_list) — Qt6's stack-local per-slice Print is equivalent for correctness; caching is a perf optimization, deferred. Per-plate wipe-tower geometry (Phase 18 deferred; needs plate origins). Per-plate slice state machine (ready_for_slice/slice_result_valid) enforcement beyond the Phase 17 printable/locked filters — deferred.

</domain>

<decisions>
## Implementation Decisions

### PLATE-11: stack-local Print is the per-plate context (D-14)
- **D-14:** Qt6's existing stack-local `Slic3r::Print print;` per `startSlicePlate` call IS the per-plate slice context. Each plate slices with its own Print (independent model clone + config apply). This satisfies PLATE-11's "per-plate slice context" requirement at the correctness level — the upstream `m_print_list` reuse-map is a caching optimization, not a correctness mechanism. **No Print-creation refactor needed.** Document this equivalence in the VERIFICATION + SUMMARY.

### PLATE-10: full per-plate config merge via DynamicPrintConfig::apply (D-15)
- **D-15:** In SliceService's config-build path (SliceService.cpp:379-403), replace the 3-hardcoded-key patch with a full merge of the per-plate DynamicPrintConfig: after building the base config from presets, call `config.apply(plateConfig)` (or `plateConfig.apply(config)` — match upstream's override-direction: per-plate overrides take precedence over presets, so apply the plate config ONTO the preset config, i.e. `config.apply(plateConfig)` where apply() means "merge other into this with other winning").
  - Source truth: upstream `update_slice_context_to_current_plate` (PartPlate.hpp:868) applies `PartPlate::config()` into the slicing config.
  - This means ALL per-plate overrides (curr_bed_type, print_sequence, spiral_mode, AND any other keys the user set via setPlateScopedOptionValue) are honored — fixing the gap-analysis issue #3 (arbitrary overrides dropped).
  - The per-plate config is accessed via a new ProjectServiceMock method `plateDynamicConfig(int plateIdx)` returning `const Slic3r::DynamicPrintConfig*` (under HAS_LIBSLIC3R), so SliceService doesn't reach into PartPlate internals.

### plateScopedOptionValue / setPlateScopedOptionValue stub fix (D-16)
- **D-16:** Under HAS_LIBSLIC3R, `plateScopedOptionValue(plateIdx, key, fallback)` reads `PartPlate::config().option(key)` and converts to QVariant (int/double/bool/string per the ConfigOption type). `setPlateScopedOptionValue(plateIdx, key, value)` writes into `PartPlate::config().option(key, true)` converting from QVariant. This replaces the `return fallbackValue; // TODO` stub at ProjectServiceMock.cpp and the `return false` at setPlateScopedOptionValue.
  - The QVariant↔ConfigOption conversion is the bridge deferred from Phase 16 (D-04). It must handle: ConfigOptionInt (getInt/setInt), ConfigOptionFloat (getDouble/setDouble), ConfigOptionBool (getBool/setBool), ConfigOptionString (getString/setString). Fall back to the option's serialize/deserialize for unknown types.

### Claude's Discretion
- Exact `apply()` direction (test which makes plate-override win over preset; match upstream).
- QVariant→ConfigOption type dispatch (use ConfigOptionInt/Float/Bool/String by typeid or a small switch).
- Whether to add a `plateConfigKeys(plateIdx)` Q_INVOKABLE (for QML to enumerate override keys) — optional, defer if not needed by Phase 19 tests.

</decisions>

<canonical_refs>
## Canonical References

### Upstream Source Truth
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:868` — `update_slice_context_to_current_plate(BackgroundSlicingProcess&)` (the per-plate config-apply mechanism; D-15 mirrors its config merge).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:487` — `update_slice_context(BackgroundSlicingProcess&)`.
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp` — slice-all loop calling select_plate + update_slice_context per plate (the orchestration Qt6 already mirrors via requestSliceAll).

### Phase 16/18 Outputs (the foundation)
- `src/core/model/PartPlate.h` — `config()` returns `DynamicPrintConfig&` under HAS_LIBSLIC3R (D-04). This is the config to merge.
- `src/core/services/SliceService.cpp:379-403` — the 3-key patch being replaced (D-15).
- `src/core/services/SliceService.cpp:405,450` — stack-local `Slic3r::Print print;` + `print.apply(*modelForSlice, config)` (D-14: already per-plate).
- `src/core/services/ProjectServiceMock.cpp` `plateScopedOptionValue`/`setPlateScopedOptionValue` — the stubs being fixed (D-16).

### Rules
- `.codex/rules/source-truth-migration.md` — do NOT modify libslic3r; status terms; "do not mark complete merely because a method exists."

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `Slic3r::DynamicPrintConfig::apply(other)` — the merge API (libslic3r; used by upstream update_slice_context).
- `PartPlate::config()` (Phase 16 D-04) — the per-plate config source.
- `SliceService::requestSliceAll`/`continueSliceAllQueue` (Phase 17) — already iterates non-locked+printable plates, calling startSlicePlate per plate. No change needed.

### Integration Points
- `print.apply(*modelForSlice, config)` at SliceService.cpp:450 — the config is what gets sliced; D-15 ensures it's the full per-plate-merged config.
- EditorViewModel/QML read per-plate config via plateScopedOptionValue — D-16 makes that work end-to-end.

</code_context>

<deferred>
## Deferred Ideas

- **m_print_list reuse/caching** (upstream PartPlateList::m_print_list) — perf optimization, not correctness. Qt6's stack-local per-slice Print is equivalent.
- **Per-plate wipe-tower geometry** — needs plate origins (Phase 18 deferred).
- **Per-plate slice state machine enforcement** (can_slice / ready_for_slice beyond locked/printable) — Phase 17 covers the user-facing filters; deeper state-machine is future.
- **plateConfigKeys Q_INVOKABLE** for QML enumeration — optional.

</deferred>

---

*Phase: 19-per-plate-slice-scheduling*
*Context gathered: 2026-06-26*
