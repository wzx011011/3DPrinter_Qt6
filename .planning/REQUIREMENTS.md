# Requirements: OWzx Slicer v4.5 Backlog Closure

**Defined:** 2026-07-12
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

**Milestone goal:** Clear the deferred-backlog in one cycle — wipe-tower Option B mesh, auto filament-map, CLI fixtures, D3D12 root cause, and the GLGizmoMeasure engine — extending v4.4's slice/UI work and unblocking long-deferred items.

**Phases continue from 103.**

## v4.5 Requirements

### WS1 — Auto Filament-Map Recommendation

- [ ] **FMAP-01**: After a successful slice, Qt6 reads back the auto-recommended per-plate filament map from libslic3r (`Print::get_filament_maps()` consumed inside the SliceService worker between `print.process()` and `activePrint_.store(nullptr)`, captured by value — no `Print*` escape; mirrors v4.4 WTREAD-01).
- [ ] **FMAP-02**: The Qt6 filament-map mode enum is widened from 2-value (`Auto/Manual`) to upstream's 4-value (`fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault`), with 3MF persistence read-side migration so pre-v4.5 "Manual" plates do not silently reload as "AutoForMatch" (the `setInt` raw-int hazard at `ProjectServiceMock.cpp:4997`).
- [ ] **FMAP-03**: A `FilamentGroupPopup` UI surfaces the 3 selectable modes (AutoForFlush "Filament-Saving", AutoForMatch "Convenience", Manual "Custom") with the auto-recommended map preview; `fmmDefault` is the per-plate "inherit from global" sentinel (NOT a 4th radio button).
- [ ] **FMAP-04**: An automated test asserts the auto-recommended map round-trips through save→reload (mirrors Phase 97 thumbnail round-trip pattern) and that the `fmmDefault` inheritance resolves correctly.

### WS2 — Option B Real Wipe-Tower Mesh

- [ ] **WTMESH-01**: When `Print::wipe_tower_data().wipe_tower_mesh_data` is populated (post-slice, multi-material), the renderer draws a real mesh (brim + rib/cone base + true non-convex footprint) via `wipe_tower_mesh_data->real_wipe_tower_mesh` + `real_brim_mesh` + `convex_hull_3d`, mirroring upstream `3DScene.cpp:887-925 load_real_wipe_tower_preview`.
- [ ] **WTMESH-02**: The Option A dimensioned box (v4.4 Phase 99 Frozen Decision 2) is preserved as the fallback when `wipe_tower_mesh_data == std::nullopt` (single-material / pre-slice) — Option B is a parallel path, NOT a modification of the frozen `buildWipeTowerVertices` / `uploadWipeTowerBuffer`.
- [ ] **WTMESH-03**: The SoftwareViewport fallback renderer mirrors the Option B mesh (parallel to v4.4 Phase 101 QPainter box consistency).
- [ ] **WTMESH-04**: An automated test + source audit confirm Option A does not regress (v4.4 regression ctest still passes) and Option B fires only when the mesh is populated.

### WS3 — CLI Fixtures + argv GUI Fixture Loading

- [ ] **FIXTURE-01**: A multi-material fixture model is added to `tests/data/` (current `test_model.stl` is single-material) so multi-material-dependent features (wipe-tower, filament-map, AMS) can be exercised deterministically.
- [ ] **FIXTURE-02**: The existing argv plumbing (`--load-model`, `--open-page`, `--open-dialog`, `--skip-first-run` at `main_qml.cpp:170-257`) is gated on `QQmlApplicationEngine::objectCreated` + `QQuickWindow::frameSwapped` (not the current `singleShot(0)` trick) so screenshots are deterministic — closing the long-standing Windows-capture-API runtime-evidence blocker.
- [ ] **FIXTURE-03**: argv fixture recipes (documented combos of `--load-model X --open-page Y`) cover the major GUI states (Prepare empty, Prepare with model, Preview, AssembleView, settings dialogs) so visual verification can reach them without simulated clicks.
- [ ] **FIXTURE-04**: The argv fixtures are documented as test-evidence plumbing, NOT a user-facing deep-link product feature (anti-feature: shipping as product).

### WS4 — D3D12 Crash Root Cause + Backend Readiness

- [ ] **D3D12-01**: The D3D12 debug layer is enabled behind an env flag in `RhiBackendSelector.cpp` (before `QRhi::create`) so the startup `0xc0000005` access violation (reproducible via `OWZX_RHI_RENDERER=d3d12`) can be triaged with debug output.
- [ ] **D3D12-02**: The D3D12 crash root cause is identified (investigation; the BUG-V31-1 comment is a leading hypothesis, not confirmed — the cited fix is already in place at `RhiViewportRenderer.cpp:286-296`, so a new crash needs new isolation). Time-boxed; "fixed" = root cause documented + stable opt-in.
- [ ] **D3D12-03**: Default-backend promotion to D3D12 is explicitly out of scope (stays opt-in until root cause is resolved); Vulkan backend is documented as SDK-blocked (Qt disables `vulkan` feature).

### WS5 — Full GLGizmoMeasure Engine + AssembleViewDataPool Clipper

- [ ] **MEASURE-01**: A per-volume ITS (indexed_triangle_set) accessor is added to `ProjectServiceMock` (current `meshData()` is per-object-flattened) — this is the cross-workstream dependency that also unblocks the AssembleViewDataPool `ModelObjectsClipper`.
- [ ] **MEASURE-02**: A `SceneRaycaster` + `MeshRaycaster` are ported from upstream into `src/core/rendering/` as pure-CPU helpers (matching the `GizmoGeometry`/`ObjectPicking` pattern), with two-stage pick (coarse AABB via `ObjectPicking::pickSourceObject`, then per-triangle ITS on the hit volume only) for mouse-move performance.
- [ ] **MEASURE-03**: `Measure::Measuring` is instantiated per-volume (not reimplemented) to produce real measurements: angle, direct/perpendicular distance, distance XYZ — replacing the current AABB stub at `AssemblyMeasureGeometry`.
- [ ] **MEASURE-04**: The GLGizmoMeasure snap UX (Point/Edge/Circle/Plane feature picks; Shift toggles FeatureSelection vs PointSelection) is wired through the raycaster + `Measuring`, with `SurfaceFeature` boundary scrubbing (raw `void* volume` / `vector<int>* plane_indices` from libslic3r must not escape into Qt).
- [ ] **MEASURE-05**: An automated test + source audit confirm the measurement readouts are real (not the AABB stub) and the per-volume ITS lifetime is correct (no UAF across the libslic3r→Qt boundary).

## Future Requirements (deferred)

- **MEASURE-06**: Assembly-mode transformation actions (Parallel/Coincidence/Reverse-rotation per upstream `Measure.hpp:186-200 AssemblyAction`) — documented as P3 future, NOT in v4.5 scope. Ship the measurement readouts first (MEASURE-01..05); Assembly actions need a stable feature-picking foundation.
- Full PLATE-09 save/reload state assertions — partially addressed by v4.5 WS3 fixture work; complete coverage remains future.
- Full i18n translation coverage beyond strings touched by active workflows.

## Out of Scope

- **Changing libslic3r slicing algorithms** as part of GUI migration work. The migration rewrites the GUI layer only; libslic3r slicing algorithms are preserved unchanged.
- **Adding product behavior not mapped to OrcaSlicer upstream** or explicitly documented as an OWzx-only decision.
- **Creating alternate build directories or using non-canonical build scripts** (canonical: `scripts/auto_verify_with_vcvars.ps1`, single build dir `build/`).
- **fmmDefault as a 4th popup radio button** — `fmmDefault` is a per-plate "inherit from global" sentinel resolved by `PartPlate::get_real_filament_map_mode` (`PartPlate.cpp:317-328`), deliberately not in the serialized enum map (`PrintConfig.cpp:580-583`). Anti-feature.
- **Option B rendering per-extruder color slabs AND mesh together** — upstream chose silhouette over stripes (`model_per_colors.resize(1)` at `3DScene.cpp:911`). Do not render both.
- **argv fixtures shipped as a user-facing deep-link product feature** — these are OWzx-only test-evidence plumbing (upstream has no `--open-page`/`--open-dialog`/`--load-model`; upstream argv is CLI-only `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`). Anti-feature.
- **D3D12 default-backend promotion before root cause is resolved** — stays opt-in (`OWZX_RHI_RENDERER=d3d12`) until D3D12-02 is closed.
- **Assembly-actions-before-engine** — MEASURE-06 deferred until MEASURE-01..05 ship the stable feature-picking foundation.
- **Vulkan production backend** — Qt SDK disables the `vulkan` feature; Vulkan is evaluation-only, not a v4.5 deliverable.
- **OpenVDB-dependent features** (hollow/support paint gizmos) — OpenVDB unavailable; none of the 5 v4.5 workstreams need it.
- **LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows** — removed from forward scope unless the user explicitly reopens them.

## Traceability

Each active REQ-ID is mapped to exactly one phase. Phases continue from 103 (v4.4 ended at Phase 102). MEASURE-06 is deferred and NOT mapped.

| REQ-ID | Phase | Phase Name | Notes |
|--------|-------|------------|-------|
| FMAP-01 | 108 | Filament-Map Auto Recommendation Readback | Mirrors v4.4 WTREAD-01 capture-by-value; Pitfall 1 |
| FMAP-02 | 107 | Filament-Map Mode Enum Widening And 3MF Migration | MUST precede FMAP-01 (Pitfall 2 — raw `setInt` migration hazard at `ProjectServiceMock.cpp:4997`) |
| FMAP-03 | 110 | Filament-Map Popup UI And Mode Surfacing | Needs 107 (enum) + 108 (readback) first; Pitfall 8 (no QML-local state) |
| FMAP-04 | 111 | Filament-Map Save-Reload Round-Trip | Ships last in WS1; mirrors Phase 97 thumbnail pattern |
| WTMESH-01 | 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | Real mesh via `wipe_tower_mesh_data` + `convex_hull_3d`; re-opens Phase 99 Frozen Decision 2 |
| WTMESH-02 | 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | Option A fallback preserved byte-for-byte (Pitfall 3); co-ships with WTMESH-01 |
| WTMESH-03 | 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | SoftwareViewport mirror (v4.4 Phase 101 parity) |
| WTMESH-04 | 116 | v4.5 Verification And Cross-Workstream Regression | Option A regression-lock + Option B fires-only-when-populated audit |
| FIXTURE-01 | 104 | CLI Fixture Recipes And Multi-Material Model | Multi-material fixture in `tests/data/` |
| FIXTURE-02 | 103 | CLI Fixture Readiness Gate | `objectCreated` + `frameSwapped` gate; closes the Windows-capture-API blocker (Pitfall 4) |
| FIXTURE-03 | 104 | CLI Fixture Recipes And Multi-Material Model | Documented argv recipes per screenshot target |
| FIXTURE-04 | 104 | CLI Fixture Recipes And Multi-Material Model | Anti-feature: test-evidence plumbing, NOT user-facing deep link |
| D3D12-01 | 105 | D3D12 Debug Layer Wiring | Env-gated, Debug-build-only (Pitfall 5 — no Release leak) |
| D3D12-02 | 106 | D3D12 Crash Root-Cause And Backend Readiness | TIME-BOXED investigation; "fixed" = root cause documented + stable opt-in |
| D3D12-03 | 106 | D3D12 Crash Root-Cause And Backend Readiness | Documentation: default promotion out of scope; Vulkan SDK-blocked |
| MEASURE-01 | 112 | Per-Volume ITS Accessor And Mesh Cache | Cross-workstream dep — unblocks raycaster + AssembleViewDataPool clipper; Pitfall 6 (ITS ownership contract first) |
| MEASURE-02 | 113 | Scene And Mesh Raycaster Port | Pure-CPU port; two-stage pick mandatory (Pitfall 7) |
| MEASURE-03 | 114 | Measure Engine Instantiation And Feature Readouts | Instantiate `Measure::Measuring`, don't reimplement (AGENTS.md) |
| MEASURE-04 | 115 | GLGizmoMeasure Snap UX And Feature Picking | Point/Edge/Circle/Plane picks; Shift toggle; `SurfaceFeature` raw-pointer scrub |
| MEASURE-05 | 116 | v4.5 Verification And Cross-Workstream Regression | Real-readout assertion + ITS lifetime + frame-time < 16ms (Pitfall 7) |

### Coverage Validation

- **Active requirements mapped:** 20/20 (FMAP-01..04, WTMESH-01..04, FIXTURE-01..04, D3D12-01..03, MEASURE-01..05).
- **Deferred (NOT mapped):** MEASURE-06 (Assembly-mode transformation actions — P3 future).
- **Phase range:** 103-116 (14 phases; continues from v4.4 Phase 102, no reset).
- **Each REQ-ID maps to exactly one phase** (WTMESH-01/02/03 share Phase 109 by design — Option A fallback + Option B path + SoftwareViewport mirror must ship together per Pitfall 3; FIXTURE-01/03/04 share Phase 104; D3D12-02/03 share Phase 106; WTMESH-04 + MEASURE-05 share the verification Phase 116).

### Phase-to-Requirement Reverse Map

| Phase | Phase Name | Workstream | Requirements |
|-------|------------|------------|--------------|
| 103 | CLI Fixture Readiness Gate | WS3 | FIXTURE-02 |
| 104 | CLI Fixture Recipes And Multi-Material Model | WS3 | FIXTURE-01, FIXTURE-03, FIXTURE-04 |
| 105 | D3D12 Debug Layer Wiring | WS4 | D3D12-01 |
| 106 | D3D12 Crash Root-Cause And Backend Readiness | WS4 | D3D12-02, D3D12-03 |
| 107 | Filament-Map Mode Enum Widening And 3MF Migration | WS1 | FMAP-02 |
| 108 | Filament-Map Auto Recommendation Readback | WS1 | FMAP-01 |
| 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | WS2 | WTMESH-01, WTMESH-02, WTMESH-03 |
| 110 | Filament-Map Popup UI And Mode Surfacing | WS1 | FMAP-03 |
| 111 | Filament-Map Save-Reload Round-Trip | WS1 | FMAP-04 |
| 112 | Per-Volume ITS Accessor And Mesh Cache | WS5 | MEASURE-01 |
| 113 | Scene And Mesh Raycaster Port | WS5 | MEASURE-02 |
| 114 | Measure Engine Instantiation And Feature Readouts | WS5 | MEASURE-03 |
| 115 | GLGizmoMeasure Snap UX And Feature Picking | WS5 | MEASURE-04 |
| 116 | v4.5 Verification And Cross-Workstream Regression | cross-WS | WTMESH-04, MEASURE-05 |

---

*20 active requirements + 1 deferred (MEASURE-06) across 5 workstreams. Phases 103-116 (14 phases). Traceability filled by the roadmapper on 2026-07-12.*
