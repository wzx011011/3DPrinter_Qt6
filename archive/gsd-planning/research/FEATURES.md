# Feature Research

**Domain:** v4.5 backlog closure for OWzx Slicer (OrcaSlicer Qt6/QML migration) — 5 workstreams.
**Researched:** 2026-07-12
**Confidence:** HIGH (filament-map, Option B, GLGizmoMeasure, argv fixtures — all cited at upstream file:line); MEDIUM (D3D12 — crash is reproducible and characterized, root cause is investigation output not pre-known)

**Source of truth:** `third_party/OrcaSlicer/` is the user-visible behavior truth. Existing v4.4 capabilities (real wipe-tower geometry readback + Option A dimensioned-box rendering, thumbnail capture + 3MF round-trip, AssembleView restoration incl. measurement gizmo CTRL+Y AssemblyMeasureGeometry stub, Prepare/Preview/settings dialogs, QRhi/D3D11 default renderer) are NOT re-researched — only the 5 NEW v4.5 workstreams are.

---

## Workstream Map (orientation for the 5 features below)

| # | Workstream | Upstream source truth | Existing Qt6 state | This research § |
|---|---|---|---|---|
| 1 | Auto filament-map recommendation | `Print.cpp:2485-2491` (auto-fires when `map_mode < fmmManual`), `ToolOrdering.cpp:1107-1214 get_recommended_filament_maps`, `FilamentGroupPopup.cpp`, `PartPlate.cpp:317-328 get_real_filament_map_mode` | `PartPlate.h:202-203` 2-value enum comment (`0=Auto, 1=Manual`); manual maps wired, auto deferred (FMAP-04) | §1 |
| 2 | Option B real wipe-tower mesh | `3DScene.cpp:887-925 load_real_wipe_tower_preview`, `Print.cpp:5070-5094 construct_mesh`, `GLCanvas3D.cpp:2911,2921-2933` gating | v4.4 ships Option A (dimensioned box via `buildWipeTowerVertices`); Option B LOCKED future (Phase 99 Frozen Decision 2) | §2 |
| 3 | CLI fixtures + argv GUI fixture loading | upstream has NO `--open-page`/`--open-dialog`/`--load-model` (it uses CLI `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`). The OWzx argv flags are an **OWzx-only** test-evidence pattern. | `main_qml.cpp:170-257` already parses the 4 flags; only deterministic fixtures + recipes missing | §3 |
| 4 | D3D12 crash root cause + backend readiness | (no upstream equivalent — upstream uses raw OpenGL) | `.planning/debug/qrhi-d3d12-crash.md`: `0xc0000005` access violation at startup with `OWZX_RHI_RENDERER=d3d12`; "resolved" by defaulting to D3D11 | §4 |
| 5 | Full GLGizmoMeasure feature-picking engine | `GLGizmoMeasure.cpp` + `Measure.hpp` (Point/Edge/Circle/Plane + MeasurementResult); `GLGizmoMeasure.cpp:409-442` Shift-to-point snap | Qt6 ships an AABB stub (`AssemblyMeasureGeometry.h:23-33`: center-to-center distance + longest-axis angle). Full feature-picking deferred (Phase 93). | §5 |

---

## §1 — Auto Filament-Map Recommendation

### What "auto filament-map" does (user-visible behavior)

On a multi-extruder (specifically BBL dual-nozzle) printer, each filament in the project must be assigned to a physical extruder (left/right nozzle). The **filament map** is the per-plate array `filament_maps[i] = extruder_index` that answers "which nozzle feeds filament i". Upstream computes this during slicing (`Print.cpp:2484-2491`):

```cpp
std::vector<int> filament_maps = this->get_filament_maps();
auto map_mode = get_filament_map_mode();
if (map_mode < FilamentMapMode::fmmManual) {
    filament_maps = ToolOrdering::get_recommended_filament_maps(all_filaments, this, map_mode, ...);
    ...update_filament_maps_to_config(filament_maps);
}
```

So **auto-recommendation** = libslic3r's `ToolOrdering::get_recommended_filament_maps` (`ToolOrdering.cpp:1107-1214`) reads the per-layer filament usage + flush matrix + AMS state + physical/geometric unprintable constraints, runs `FilamentGroup::calc_filament_group()` (`ToolOrdering.cpp:1204-1207`), and writes the result back into the plate config. The user sees a post-slice "which filament goes where" assignment they did not have to choose manually.

### The 4 enum values and their user-visible semantics

Declared at `PrintConfig.hpp:424-429`:
```cpp
enum FilamentMapMode {
    fmmAutoForFlush,
    fmmAutoForMatch,
    fmmManual,
    fmmDefault
};
```

**Critical serialization asymmetry:** `PrintConfig.cpp:580-583` (`s_keys_map_FilamentMapMode`) only maps 3 strings — there is **no `"Default"` string** in the enum-values map. But `PrintConfig.cpp:2500-2503` lists 4 enum_values strings including `"Default"`. `fmmDefault` is a **per-plate "inherit from global" sentinel**, not a user-selectable radio button. `PartPlate::get_real_filament_map_mode` (`PartPlate.cpp:317-328`) resolves it:

```cpp
auto mode = get_filament_map_mode();
if (FilamentMapMode::fmmDefault != mode) { return mode; }   // plate override
auto g_mode = g_config.option<...>("filament_map_mode")->value;
return g_mode;                                               // fall back to global
```

| Enum value | User-facing label (FilamentGroupPopup.cpp:91-97) | What it DOES user-visible | Popup radio button? |
|---|---|---|---|
| `fmmAutoForFlush` | **"Filament-Saving Mode"** — "Generates filament grouping for the left and right nozzles based on the most filament-saving principles to minimize waste." | libslic3r picks the nozzle assignment that **minimizes flush volume** (cost-optimal). `FGMode::FlushMode` (`ToolOrdering.cpp:1168`). Default value (`PrintConfig.cpp:2509`). | Yes (btForFlush) |
| `fmmAutoForMatch` | **"Convenience Mode"** — "Generates filament grouping for the left and right nozzles based on the printer's actual filament status, reducing the need for manual filament adjustment." | libslic3r picks the assignment that **matches what's physically loaded in the AMS** right now (so the user doesn't have to swap filaments). `FGMode::MatchMode`. **Disabled unless printer is connected** (`FilamentGroupPopup.cpp:251-254`: if `!m_connected`, force-falls-back to `fmmAutoForFlush`; `Init()` at `:242-248` greys out the radio + shows "(Sync with printer)"). | Yes (btForMatch, **connection-gated**) |
| `fmmManual` | **"Custom Mode"** — "Manually assign filament to the left or right nozzle" | libslic3r does NOT auto-compute; the user's explicit `filament_maps` array is used as-is. Opens the manual editing surface (`FilamentMapDialog`). | Yes (btManual) |
| `fmmDefault` | (no label — not user-selectable) | **Per-plate "use global setting" sentinel.** `get_real_filament_map_mode` resolves it to the project-level (`filament_map_mode`) value. This is the plate-level "inherit" state. | **No** — not in the popup's `mode_list` (`FilamentGroupPopup.hpp:52`: `{fmmAutoForFlush, fmmAutoForMatch, fmmManual}`). |

**Qt6 widening gap:** `PartPlate.h:202-203` currently comments `// 0=Auto, 1=Manual` — a 2-value model. The 4-value upstream reality is `fmmAutoForFlush` (0) / `fmmAutoForMatch` (1) / `fmmManual` (2) / `fmmDefault` (3, sentinel). v4.5 must widen the Qt6 enum to all 4, but `fmmDefault` is a resolution-layer sentinel, not a 4th radio button.

### How FilamentGroupPopup presents it

`FilamentGroupPopup` (`FilamentGroupPopup.hpp:21-80`) is a hover popup (not a modal dialog) that appears when the user hovers the slice button on a BBL dual-nozzle printer. `should_pop_up()` (`FilamentGroupPopup.cpp:16-23`) gates it: BBL vendor + `nozzle_diameter.size() > 1`. It shows:

- 3 radio buttons (the `mode_list` at `FilamentGroupPopup.hpp:52`), each with a label + grey detail line (the descriptions above).
- The "Convenience Mode" (fmmAutoForMatch) button is **disabled + shows "(Sync with printer)"** when the printer isn't connected (`FilamentGroupPopup.cpp:242-248`).
- A persistent preference (`prefered_filament_map_mode` in AppConfig, `FilamentGroupPopup.cpp:25-47`) remembers the last choice across sessions.
- Wiki + Video tutorial hyperlinks at the bottom (`FilamentGroupPopup.cpp:180-194`).
- Selection routes per-plate (when `m_sync_plate`) or globally (`FilamentGroupPopup.cpp:297-312`); "slice all" applies the mode to every plate (`:300-305`).

The popup itself does NOT compute the map — it only sets the mode. The actual auto-recommendation fires inside `Print::` during slicing.

### Table stakes / differentiator / anti-feature

- **Table stakes:** 4-value enum parity (Qt6 currently 2-value), per-plate vs global resolution (`get_real_filament_map_mode` semantics), `fmmDefault` inheritance, "Convenience Mode" connection-gating, manual mode passthrough (already works).
- **Differentiator:** none — this is pure source-truth parity. (Bambu Studio / OrcaSlicer is the reference; matching it is the bar, not a competitive edge.)
- **Anti-feature:** exposing `fmmDefault` as a 4th user-visible radio button. Upstream deliberately keeps it out of `mode_list` and out of the serialized enum map. Widening the Qt6 enum to 4 values must NOT add a 4th popup radio.

### Complexity: MEDIUM
- libslic3r already does the heavy lifting (`get_recommended_filament_maps` is upstream, unchanged). Qt6 work is: (a) widen enum + 3MF round-trip (`bbs_3mf.cpp:4443-4447,7964-7967` already serializes it), (b) read back the auto-computed map post-slice (mirrors v4.4 WT readback pattern), (c) port `FilamentGroupPopup` to QML, (d) `get_real_filament_map_mode` resolution in the viewmodel. No new slicing algorithm.

### Dependencies
- **Requires:** existing manual filament-map path (`PartPlate.h:200-203`, `ProjectServiceMock.cpp:1176` `setFilamentMapMode`), existing 3MF `filament_map_mode` round-trip.
- **Enhances:** multi-material slicing (the auto map only matters when >1 extruder).
- **Conflicts:** none.

---

## §2 — Option B Real Wipe-Tower Mesh

### What the real mesh shows that the dimensioned box (Option A) doesn't

Upstream ships **two** wipe-tower preview paths, gated at `GLCanvas3D.cpp:2911`:

```cpp
if (!current_print->is_step_done(psWipeTower) || !current_print->wipe_tower_data().wipe_tower_mesh_data) {
    // box path
    m_volumes.load_wipe_tower_preview(...);   // 3DScene.cpp:840-885
} else {
    // real mesh path
    m_volumes.load_real_wipe_tower_preview(...);  // 3DScene.cpp:887-925
}
```

| Aspect | Option A — box (`load_wipe_tower_preview`, `3DScene.cpp:840-885`) | Option B — real mesh (`load_real_wipe_tower_preview`, `3DScene.cpp:887-925`) |
|---|---|---|
| Geometry | `make_cube(width, depth, height)` (`:855`) — a single rectangular prism | `wt_mesh.convex_hull_3d()` (`:914`) from `wipe_tower_mesh_data->real_wipe_tower_mesh` + optional `real_brim_mesh` merged in (`:907-909`) |
| Per-extruder color slices | YES — splits depth into N colored slabs, one per extruder (`:868-873`, alpha 0.66) | NO per-extruder slabs — single front-extruder color only (`:897-902`) |
| Brim | NO (brim_width is a param but the box ignores it) | YES — `real_brim_mesh` is a thin (first_layer_height=0.08mm) widened skirt around the base (`Print.cpp:5077-5078`: `make_cube(width + 2*brim_width, depth + 2*brim_width, 0.08)` translated by `(-brim_width, -brim_width, 0)`); rendered when `render_brim=true` (`3DScene.cpp:907`) |
| Rib / cone base | NO | YES when `wipe_tower_wall_type == wtwRib` — `WipeTower::its_make_rib_tower` + `its_make_rib_brim` (`Print.cpp:5082-5087`), offset by `rib_offset` (`:5088-5090`) |
| Bottom polygon (for plate-fit) | implicit (the box footprint) | explicit `wipe_tower_mesh_data->bottom` polygon, used by `GLCanvas3D.cpp:2923-2929` to `move_box_inside_box` (keep tower inside plate) |

**User-visible fidelity the mesh adds:** the brim (a visible thin wider lip at the base), the rib/cone base shape (when configured), and the true non-convex footprint used for plate-collision margining. The box is a stand-in; the mesh is the real sliced shape. Note the trade-off: **the box shows per-extruder color stripes; the real mesh shows the real silhouette but only one color.** Upstream uses the real mesh only when slicing is done AND `wipe_tower_mesh_data` is populated.

### When Option B fires (gating truth)

Two conditions must BOTH hold (`GLCanvas3D.cpp:2911`):
1. `current_print->is_step_done(psWipeTower)` — the wipe-tower slice step has run.
2. `current_print->wipe_tower_data().wipe_tower_mesh_data` has a value (`std::optional<WipeTowerMeshData>`, `Print.hpp:766`).

`wipe_tower_mesh_data` is populated by `WipeTowerData::construct_mesh` (`Print.cpp:3409,3520,5070-5094`), which is called during slicing. Its `clear()` resets it to `std::nullopt` (`Print.hpp:776`), so on an unsliced plate the box path is used. **This means Option B is a post-slice-only fidelity upgrade; pre-slice the box is correct.**

### Table stakes / differentiator / anti-feature

- **Table stakes:** showing the brim (users expect to see the prime-tower brim they'll actually print). The box-only v4.4 baseline is acceptable for v4.4 but the brim is a visible fidelity gap once you're chasing source-truth.
- **Differentiator:** none — pure parity. Upstream ships it; Qt6 matching it is the bar.
- **Anti-feature:** per-extruder color slabs ON the real mesh. Upstream's `load_real_wipe_tower_preview` deliberately drops the per-extruder slabs (`model_per_colors.resize(1)` at `:911`) because the real mesh geometry is incompatible with slab-splitting. Do NOT try to render both the real silhouette AND per-extruder stripes — upstream chose silhouette over stripes for the real path.

### Complexity: MEDIUM-HIGH
- Needs ITS (indexed_triangle_set) vertex format in `GizmoGeometry` + `RhiViewportRenderer` (current `buildWipeTowerVertices` emits a fixed 36-vertex box layout, not arbitrary ITS — `GizmoGeometry.cpp:449-499`).
- Needs a non-box upload path in `RhiViewportRenderer` (mirror of `init_from(mesh.convex_hull_3d())`).
- Needs the `wipe_tower_mesh_data` (the `std::optional<WipeTowerMeshData>` itself, not just the dims) captured in the v4.4 readback path — v4.4 captures only `bbx/depth/height/position/width/brim_width/rib_offset`, NOT the mesh. This re-opens Phase 99 Frozen Decision 2.

### Dependencies
- **Requires (already built):** v4.4 wipe-tower geometry readback + `has_wipe_tower()` gate + Option A box pipeline (`buildWipeTowerVertices` + `uploadWipeTowerBuffer` + `renderWipeTower`).
- **Requires (NEW):** ITS vertex format extension in `GizmoGeometry` + `RhiViewportRenderer`; capture `wipe_tower_mesh_data` itself in the SliceService worker (not just dims).
- **Conflicts:** none — the box path stays as the pre-slice fallback (mirrors upstream gating).

---

## §3 — CLI Fixtures + Deterministic argv GUI Fixture Loading

### Upstream argv reality (important framing)

**Upstream OrcaSlicer does NOT have `--open-page`, `--open-dialog`, or `--load-model` flags.** Upstream argv is the CLI config path: `CLI::setup` (`OrcaSlicer.cpp:7122-7216`) parses `--load`, `--slice`, `--export-3mf`, positional model files, etc. via `m_config.read_cli` (`:7183`). There is no "open the Prepare page and load this model for a screenshot" flag upstream.

The 4 OWzx flags (`--open-page`, `--open-dialog`, `--load-model`, `--skip-first-run`) are an **OWzx-only test-evidence pattern**, already wired at `main_qml.cpp:170-257` (shipped in v4.1/v4.2). Workstream 3 is therefore NOT "add argv flags" — it is **add deterministic fixtures + stable recipes** that use the already-wired flags to produce reproducible visual evidence. This exists to unblock the recurring Windows capture-API blocker (the reason deep links were added in the first place, per PROJECT.md Key Decisions).

### What GUI states should be reachable via deep links

Already-reachable (main_qml.cpp:123-168 routes):
- **Pages:** home, prepare (3d/editor/plater), preview, device (monitor), multi-device, project, calibration (`startupPageRoutes`, `:123-136`).
- **Dialogs:** settings:printer / settings:filament (material) / settings:process (print), config-wizard (wizard), bed-shape (bed), ams-settings (ams), firmware, speed-limit, wipe-tower, print-host, plugin-manager (plugins), lite-mode (`startupDialogRoutes`, `:138-168`).
- **Model loading:** `--load-model <path>` (repeatable) calls `backend.topbarImportModel(path)` (`:234`).
- **First-run skip:** `--skip-first-run` marks the config wizard complete.

**Gaps workstream 3 should close (fixtures, not flags):**
- A canonical multi-material fixture model (the current `tests/data/test_model.stl` is single-material; auto filament-map + Option B wipe-tower both need a multi-extruder scene to produce visible evidence).
- Stable, documented argv recipes (a `tests/fixtures/argv/` or `.planning/` recipe list) for each screenshot target: Prepare-with-model, Preview-post-slice, settings-dialog-X, AssembleView-measure, wipe-tower-visible. Today the flags work but the recipes are tribal knowledge.
- FIXTURE-02 (referenced in PROJECT.md Future): full PLATE-09 save/reload state assertions, unblocked by v4.3's shared-writer fix. This is the deterministic 3MF round-trip fixture, not a new flag.

### Table stakes / differentiator / anti-feature

- **Table stakes (for an internal test-evidence capability):** deterministic model fixtures (single + multi-material), one argv recipe per screenshot target, FIXTURE-02 3MF round-trip assertions.
- **Differentiator:** none — this is test infrastructure, not a user-facing feature. It is correctly classified as enabling work, not product capability.
- **Anti-feature:** shipping these flags to end users as a "deep link" product feature. They are test-evidence plumbing; exposing them in user-facing help/docs would invent product behavior upstream doesn't have.

### Complexity: LOW-MEDIUM
- The argv parser is done (`main_qml.cpp:170-257`). Remaining work is fixtures (STL/3MF test assets), recipe docs, and the FIXTURE-02 round-trip assertion. No new C++ machinery.

### Dependencies
- **Requires (already built):** the 4 argv flags + route tables (`main_qml.cpp`), v4.3 shared 3MF writer.
- **Enhances:** every other workstream — workstreams 1, 2, 5 all need runtime visual evidence, which fixtures make deterministic.
- **Conflicts:** none.

---

## §4 — D3D12 Crash Root Cause + Backend Readiness

### User-visible failure mode (characterized, root cause open)

From `.planning/debug/qrhi-d3d12-crash.md` (status: "resolved" by avoidance, NOT by root cause):
- **Symptom:** launching with `OWZX_RHI_RENDERER=1` or `=d3d12` selects D3D12 (`startup_diagnostics.log`: `selected=d3d12 attempts=[d3d12:ok]`), then the app becomes unresponsive and exits.
- **Error:** `0xc0000005` access violation (minidump recorded); exit code `-1073741819`.
- **Repro:** `build/OWzxSlicer.exe` with `OWZX_RHI_RENDERER=d3d12`. Software + explicit `=d3d11` stay alive under the same timeout.
- **Current "fix":** auto policy defaults to D3D11 (`RhiBackendSelector.cpp:41`); D3D12 is explicit opt-in only.

So the **user-visible failure is a startup crash** — the window either never appears or appears then dies. Not render artifacts. This is a hard crash before the first stable frame.

### What "fixed" looks like

This is investigation-heavy and may not produce a clean feature. The success outcomes, in descending order of value:

1. **Root cause identified** — a concrete answer (e.g. depth-stencil format mismatch, sample-count mismatch, a specific QRhi D3D12 init param, a Qt 6.10 SDK bug, a driver issue). Cited at the offending code/config layer.
2. **D3D12 stable as opt-in** — `OWZX_RHI_RENDERER=d3d12` launches and stays alive through normal Prepare/Preview/slice interaction, with no render artifacts (missing geometry, black screen, flicker, Z-fighting).
3. **D3D12 promotion-ready (stretch)** — D3D12 stable enough to be a candidate default. This is NOT required for the workstream to succeed; PROJECT.md Out-of-Scope explicitly forbids making D3D12 the default before the crash is resolved.
4. **Vulkan readiness evaluation (informational)** — PROJECT.md Context notes the Qt 6.10 SDK disables public Vulkan (`QT_DISABLED_PUBLIC_FEATURES` includes `vulkan`), so Vulkan cannot be a default backend candidate. The workstream should document this, not try to enable it.

Investigation tooling available (per `.planning/research/STACK.md:211-238`): `QRhiD3D12InitParams::enableDebugLayer` (zero refs in `src/` today — must be wired behind an env flag), PIX on Windows frame capture, D3D12 debug layer (requires Windows "Graphics Tools" optional feature). None of these are shipped dependencies; they are investigation tooling.

### Table stakes / differentiator / anti-feature

- **Table stakes:** NONE for end users. D3D11 is the verified default; users get a working renderer today. D3D12 is invisible to them unless they set an env var.
- **Differentiator:** NONE directly. BUT — a working D3D12/Vulkan path is what unlocks future rendering features (e.g. larger G-code scenes, MSAA) that D3D11 may struggle with. This is platform investment, not product capability.
- **Anti-feature:** promoting D3D12 to default before the crash is root-caused. PROJECT.md Out-of-Scope forbids this explicitly. Also anti-feature: treating "D3D12 doesn't crash on my machine" as success without understanding WHY — the crash is environment-sensitive and could regress.

### Complexity: HIGH / OPEN-ENDED
- This is the only workstream flagged as investigation-heavy in PROJECT.md ("may not produce a clean 'feature' output"). A root cause could be a 1-line fix or a multi-week rabbit hole. The workstream should be time-boxed and structured to produce incremental value (debug-layer wiring → crash signature → root-cause hypothesis → fix attempt → stability test), not committed to a fixed feature deliverable up front.

### Dependencies
- **Requires (already built):** `RhiBackendSelector.cpp` (D3D12 probe already creates a `QRhi` at `:22,63-114`), the `OWZX_RHI_RENDERER` env handling, the startup diagnostics log.
- **Enhances:** nothing user-visible immediately; potentially enables future heavy-rendering features.
- **Conflicts:** none (D3D11 stays default regardless).

---

## §5 — Full GLGizmoMeasure Feature-Picking Engine

### What measurements it produces

Upstream `GLGizmoMeasure` (CTRL+U in measure mode, CTRL+Y in AssembleView per v4.2) computes a `Measure::MeasurementResult` (`Measure.hpp:167-180`) between two selected surface features. The measurement types, from the IMGUI panel (`GLGizmoMeasure.cpp:1990-2048`):

| Measurement | When shown | Source |
|---|---|---|
| **Angle** (degrees, e.g. "90.000°") | Two edges selected | `MeasurementResult.angle` → `AngleAndEdges` (`Measure.hpp:154-165`: angle + center + two edges + radius + coplanar flag). Rendered at `GLGizmoMeasure.cpp:1993-2000,1411-1419`. |
| **Distance** (mm, "Direct distance" / "Perpendicular distance") | Point-to-point, point-to-edge, etc. | `distance_infinite` (perpendicular) + `distance_strict` (direct) — `Measure.hpp:148-152,169-170`. Panel distinguishes: shows "Perpendicular distance" label only when both exist and differ (`GLGizmoMeasure.cpp:2002-2025`). |
| **Distance XYZ** ("X: .., Y: .., Z: ..") | When a per-axis decomposition is meaningful | `MeasurementResult.distance_xyz` (`Measure.hpp:171`). Panel at `:2026-2035`. |
| **(Assembly mode adds: Parallel, Center coincidence, Reverse rotation, Rotate around center, Parallel distance)** | Two planes selected in ONLY_ASSEMBLY mode | `AssemblyAction` (`Measure.hpp:186-200`) + `show_face_face_assembly_common/senior` (`GLGizmoMeasure.cpp:2055-2099+`). These are transformation actions, not just readouts. |

**Surface feature types** the engine recognizes (`Measure.hpp:16-22`): `Point` (vertex), `Edge`, `Circle` (center+radius+normal), `Plane`. Each carries an optional extra point (e.g. edge midpoint = `get_extra_point`, `Measure.hpp:70`).

### Feature-picking UX (snap behavior)

Two selection modes (`GLGizmoMeasure.hpp:94-98` `EMode`):
- **FeatureSelection** (default): click selects the whole feature under the cursor (the nearest vertex / edge / circle / plane).
- **PointSelection** (hold **Shift**): click selects the exact point under the cursor on a feature.

The snap logic (`GLGizmoMeasure.cpp:409-442` `gizmo_event`):
- `ShiftDown` → `m_mode = PointSelection` + disables scene raycasters (only the feature raycasters remain active) (`:411-416`).
- `ShiftUp` → `m_mode = FeatureSelection` + restores scene raycasters (`:418-422`).
- `Delete` → restart selection (`:423-426`).
- `Escape` → cancel second feature, then first, then exit (`:427-440`).

Feature detection: on hover, `m_curr_measuring->get_feature(face_idx, point, world_tran, only_select_plane)` (`Measure.hpp:128`) returns the feature to highlight. The engine snaps the cursor to the detected vertex/edge/circle/plane and renders a gripper (sphere for point, cylinder for edge, circle for circle, plane for plane — `GLGizmoMeasure.hpp:163-183`). Two selected features get distinct colors (SELECTED_1ST_COLOR cyan, SELECTED_2ND_COLOR magenta — `GLGizmoMeasure.hpp:27-28`); hover is green (`:30`).

**Per-volume ITS requirement:** `Measuring` is constructed per-volume on `const indexed_triangle_set&` (`Measure.hpp:122`), cached in `m_mesh_measure_map` (`GLGizmoMeasure.hpp:159`). This is the core gap — Qt6 currently has no per-volume ITS exposed to the measure path.

### Current Qt6 state (the gap)

Qt6 ships an **AABB stub** (`AssemblyMeasureGeometry.h:23-33`): center-to-center distance + per-axis XYZ delta + angle between the two volumes' longest-AABB-axis directions. This produces the screenshot-parity "装配页_测量.png" readout but is NOT the feature-picking engine — it cannot snap to a specific vertex/edge/face, cannot measure edge length, cannot measure the angle between two specific edges, etc. The stub's own header (`AssemblyMeasureGeometry.h:18-22`) documents this as a "Phase 92 simplification deferred to Phase 93 / future (needs the per-volume indexed_triangle_set + the AssembleViewDataPool)".

### Table stakes / differentiator / anti-feature

- **Table stakes:** point-to-point distance, edge length, angle between two edges, Distance XYZ — these are what "a measurement tool" means. The AABB stub passes a screenshot-parity bar but fails a "click a real feature" bar.
- **Different stakes:** Circle measurement (center/radius/normal from a cylindrical face), Plane measurement, Assembly-mode transformation actions (Parallel / Center coincidence / Reverse rotation). These are upstream differentiators that the Qt6 migration should inherit.
- **Differentiator:** none vs upstream — this is parity. vs other slicers: the assembly-mode transformation actions (snap two planes parallel, make two circle centers coincident) are a genuinely advanced capability worth inheriting.
- **Anti-feature:** porting the Assembly-mode transformation actions (Parallel/Coincidence/etc.) before the basic measure engine works. Those depend on a full transformation/undo-redo integration that is scope-creep for the measure engine itself. The measure readouts (distance/angle/XYZ) should ship first.

### Complexity: HIGH
- Needs per-volume ITS exposure (`mesh()->its` or equivalent) through the Qt6 rendering/viewmodel layer to the measure path.
- Needs a scene raycaster + per-feature raycaster infrastructure (mirrors upstream `SceneRaycasterItem` / `PickRaycaster`, `GLGizmoMeasure.hpp:79,193-194`).
- Needs `Measure::Measuring` (the ITS analysis class) constructed per-volume — this is upstream libslic3r code, reusable, but the Qt6 wiring is new.
- Needs the IMGUI-equivalent panel in QML (the readout + the assembly-action buttons).
- The AssembleViewDataPool clipper (per-volume ITS clipper) is a sub-piece — PROJECT.md Active lists it alongside the measure engine.

### Dependencies
- **Requires (already built):** AssembleView restoration (v4.2), the AABB measure stub (`AssemblyMeasureGeometry`), CTRL+Y routing to AssemblyMeasureGeometry.
- **Requires (NEW):** per-volume ITS exposure (`mesh()->its`), scene raycaster, per-feature raycaster, `Measure::Measuring` per-volume construction, QML measure panel.
- **Enhances:** AssembleView (transformation actions), Prepare measure mode (CTRL+U).
- **Conflicts:** none — the AABB stub can remain as a fallback for degenerate/no-ITS cases.

---

## Feature Dependencies

```
[Workstream 3: CLI fixtures] ──enhances──> [all other workstreams]
    (deterministic visual evidence for 1, 2, 4, 5)

[Workstream 2: Option B mesh]
    └──requires──> [v4.4 wipe-tower readback] (BUILT)
    └──requires──> [ITS vertex format in GizmoGeometry/RhiViewportRenderer] (NEW)
    └──requires──> [wipe_tower_mesh_data capture in SliceService worker] (NEW, re-opens Phase 99 FD 2)

[Workstream 5: GLGizmoMeasure engine]
    └──requires──> [per-volume ITS exposure mesh()->its] (NEW)
    └──requires──> [scene raycaster + per-feature raycaster] (NEW)
    └──requires──> [AssembleViewDataPool clipper] (sub-piece)
    └──enhances──> [AssembleView restoration] (BUILT v4.2)

[Workstream 1: filament-map]
    └──requires──> [manual filament-map path] (BUILT)
    └──requires──> [4-value enum + 3MF round-trip] (PARTIAL — 2-value today)

[Workstream 4: D3D12]
    └──requires──> [RhiBackendSelector D3D12 probe] (BUILT)
    └──requires──> [D3D12 debug-layer wiring] (NEW, env-gated)
    └──conflicts──> [D3D11 default promotion] (must NOT change default until crash root-caused)

[Workstream 2 ITS format] ──synergy──> [Workstream 5 per-volume ITS]
    (both need ITS handling in the Qt6 rendering layer; share infrastructure)
```

### Dependency Notes

- **Workstream 3 enhances all others:** every other workstream needs runtime visual evidence (auto-map result, real wipe-tower mesh, D3D12 stability, measure snap). Deterministic fixtures + argv recipes (workstream 3) are the cheapest unblocker and should land first or in parallel.
- **Workstream 2 re-opens Phase 99 Frozen Decision 2:** v4.4 explicitly LOCKED Option A as baseline and deferred Option B. Picking up Option B requires re-opening that frozen decision with a new audit (the `wipe_tower_mesh_data` capture, not just dims, must be designed).
- **Workstream 2 + Workstream 5 share ITS infrastructure:** both need the Qt6 rendering layer to handle arbitrary `indexed_triangle_set` data (Option B: wipe-tower mesh; Workstream 5: per-volume model meshes). Building the ITS format extension once serves both.
- **Workstream 4 is investigation, not a feature dependency:** no other workstream blocks on D3D12. It runs on D3D11 today. Schedule it independently; do not let it gate the others.
- **Workstream 1 + Workstream 2 are loosely coupled** (per v4.4 milestone decision): auto filament-map is multi-extruder-only and the wipe-tower only exists for multi-material. They share test fixtures (a multi-extruder scene) but not code.

---

## MVP Definition

### v4.5 Launch With (must have for the milestone to close)

- [ ] **Workstream 1 — Auto filament-map:** 4-value enum parity (`fmmAutoForFlush`/`fmmAutoForMatch`/`fmmManual`/`fmmDefault`), `get_real_filament_map_mode` per-plate/global resolution, post-slice readback of the auto-computed map, `FilamentGroupPopup` QML port with connection-gated "Convenience Mode". — closes the longest-deferred feature gap (AUTOMAP-FUTURE-01).
- [ ] **Workstream 3 — CLI fixtures:** deterministic multi-material fixture model + one argv recipe per screenshot target + FIXTURE-02 3MF round-trip assertion. — unblocks runtime visual evidence for everything else.
- [ ] **Workstream 2 — Option B real wipe-tower mesh (with explicit gate):** re-open Phase 99 FD 2, capture `wipe_tower_mesh_data`, add ITS format, render real mesh post-slice. Pre-slice keeps Option A box. — closes the v4.4 "Option B documented as future" carry-forward.

### Add After Validation (v4.5 stretch / v4.6)

- [ ] **Workstream 5 — Full GLGizmoMeasure engine (measure readouts first):** point-to-point distance, edge length, angle, Distance XYZ, snap UX (Shift-to-point). Defer Assembly transformation actions (Parallel/Coincidence) to a follow-up.
- [ ] **Workstream 4 — D3D12 root cause (time-boxed):** debug-layer wiring + crash signature + root-cause hypothesis. Promotion to default is explicitly out of scope.

### Future Consideration (v4.6+)

- [ ] **Workstream 5 Assembly-mode transformation actions** (Parallel, Center coincidence, Reverse rotation, Rotate around center) — needs full transformation/undo-redo integration; high value but scope-heavy.
- [ ] **D3D12 promotion to default** — only after root cause is fixed and stability proven across the full Prepare/Preview/slice flow.
- [ ] **Vulkan backend evaluation** — blocked on a Vulkan-enabled Qt SDK (current SDK disables it per PROJECT.md Context).

---

## Feature Prioritization Matrix

| Feature (workstream) | User Value | Implementation Cost | Priority | Rationale |
|---|---|---|---|---|
| WS1 Auto filament-map (4-value enum + popup) | HIGH (closes longest-deferred gap; multi-extruder users hit it every slice) | MEDIUM (libslic3r does the math; Qt6 wires enum + popup + readback) | **P1** | Deferred since v3.2; unblocks multi-extruder parity |
| WS3 CLI fixtures + FIXTURE-02 | HIGH (enables evidence for everything else) | LOW-MEDIUM (argv done; fixtures + recipes remain) | **P1** | Cheapest unblocker; do first or in parallel |
| WS2 Option B real wipe-tower mesh | MEDIUM (visible fidelity: brim/rib; box is acceptable v4.4 baseline) | MEDIUM-HIGH (ITS format + mesh capture + re-open FD 2) | **P1** | Closes explicit v4.4 carry-forward; shares ITS infra with WS5 |
| WS5 GLGizmoMeasure engine | HIGH (replaces AABB stub with real feature-picking) | HIGH (per-volume ITS + raycaster + QML panel) | **P2** | Ship measure readouts first; defer Assembly actions |
| WS4 D3D12 root cause | LOW (users get working D3D11 today) | HIGH / OPEN-ENDED (investigation; may not produce clean feature) | **P2/P3** | Time-box; do not let it block P1 work |

**Priority key:**
- P1: Must have for v4.5 to close (WS1, WS3, WS2)
- P2: Should have, add when P1 lands (WS5 measure readouts, WS4 investigation)
- P3: Future (WS5 Assembly actions, D3D12 promotion, Vulkan)

---

## Competitor / Source-Truth Feature Analysis

| Feature | Upstream OrcaSlicer (source truth) | Bambu Studio (lineage) | OWzx Qt6 approach |
|---|---|---|---|
| Filament map modes | 3 radio modes + `fmmDefault` inherit-sentinel (`FilamentGroupPopup.hpp:52`, `PartPlate.cpp:317-328`); auto-fires in `Print.cpp:2485-2491` | Same (shared lineage) | Widen 2-value enum to 4; port popup to QML; do NOT expose `fmmDefault` as a 4th radio |
| Wipe-tower preview | Dual-path: box (`load_wipe_tower_preview`) pre-slice + real mesh (`load_real_wipe_tower_preview`) post-slice, gated at `GLCanvas3D.cpp:2911` | Same | v4.4 ships Option A box; Option B adds real mesh post-slice, keeps box pre-slice |
| Wipe-tower mesh fidelity | Brim + optional rib/cone base (`construct_mesh`, `Print.cpp:5070-5094`); per-extruder slabs ONLY on the box path | Same | Match the gating exactly; do NOT try to put per-extruder slabs on the real mesh |
| CLI / argv | CLI `--load`/`--slice`/positional only (`OrcaSlicer.cpp:7183`); NO deep-link GUI flags | Same | OWzx-only `--open-page`/`--open-dialog`/`--load-model` (test evidence, not product); do NOT expose as user feature |
| D3D12 backend | N/A (upstream is raw OpenGL) | N/A | QRhi D3D12 investigation; D3D11 stays default; Vulkan blocked by Qt SDK |
| Measure gizmo | Full feature-picking: Point/Edge/Circle/Plane, Shift-to-point snap, Assembly transformation actions (`GLGizmoMeasure.cpp`, `Measure.hpp`) | Same (shared lineage) | Replace AABB stub (`AssemblyMeasureGeometry`) with per-volume ITS + raycaster engine; port readouts before actions |

---

## Sources

- **Upstream source truth (cited at file:line):**
  - `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424-429` — `FilamentMapMode` 4-value enum.
  - `third_party/OrcaSlicer/src/libslic3r/PrintConfig.cpp:579-583,2495-2509` — enum serialization map (3 strings, no "Default") + 4-entry def.
  - `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:317-328` — `get_real_filament_map_mode` (`fmmDefault` → global resolution).
  - `third_party/OrcaSlicer/src/slic3r/GUI/FilamentGroupPopup.hpp:21-80` + `.cpp:16-47,89-208,251-312` — popup structure, 3-mode list, connection-gating, labels.
  - `third_party/OrcaSlicer/src/libslic3r/Print.cpp:2484-2491` — auto-recommendation firing during slicing.
  - `third_party/OrcaSlicer/src/libslic3r/GCode/ToolOrdering.cpp:1107-1214` — `get_recommended_filament_maps` (FlushMode vs MatchMode).
  - `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:840-925` — `load_wipe_tower_preview` (box) + `load_real_wipe_tower_preview` (real mesh).
  - `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-787` — `WipeTowerData` + `WipeTowerMeshData` (`real_wipe_tower_mesh` + `real_brim_mesh` + `bottom`).
  - `third_party/OrcaSlicer/src/libslic3r/Print.cpp:5070-5094` — `construct_mesh` (brim + rib tower/cone base).
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp:2911,2921-2933` — Option A vs Option B gating.
  - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.hpp:1-325` + `.cpp:409-442,444-458,1990-2099` — measure gizmo structure, snap UX, shortcuts, IMGUI panel.
  - `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:16-238` — `SurfaceFeatureType`, `MeasurementResult`, `AngleAndEdges`, `AssemblyAction`.
  - `third_party/OrcaSlicer/src/OrcaSlicer.cpp:7122-7216` — upstream CLI `setup` (no GUI deep-link flags).

- **Qt6 current state (gap evidence):**
  - `src/core/model/PartPlate.h:198-203` — 2-value filament-map enum comment.
  - `src/core/rendering/AssemblyMeasureGeometry.h:18-33` — AABB measure stub + deferral note.
  - `src/qml_gui/main_qml.cpp:170-257` — already-wired argv flags + route tables.
  - `src/qml_gui/Renderer/RhiBackendSelector.cpp:22,41,63-114,114-139` — D3D12 probe + env handling.
  - `.planning/debug/qrhi-d3d12-crash.md` — D3D12 crash characterization (`0xc0000005`).
  - `.planning/milestones/v4.4-phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md` — Option A baseline + Option B Frozen Decision 2.
  - `.planning/research/STACK.md:211-238,308-317` — D3D12 debug tooling + Vulkan SDK constraint.

---
*Feature research for: v4.5 backlog closure (5 workstreams — auto filament-map, Option B wipe-tower mesh, CLI fixtures, D3D12 root cause, GLGizmoMeasure engine)*
*Researched: 2026-07-12*
