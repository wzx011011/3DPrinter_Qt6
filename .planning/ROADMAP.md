# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- 🚧 **v4.5** Backlog Closure — Phases 103-116 (in progress)

## Current Milestone: v4.5 Backlog Closure (Mega-Milestone)

**Goal:** Clear the deferred backlog in one cycle — wipe-tower Option B real mesh, auto filament-map recommendation, CLI fixtures, D3D12 crash root cause, and the full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper — extending v4.4's slice/UI work and unblocking long-deferred items across 5 workstreams.

**Scope rule:** Mixed. Workstreams 1 (filament-map), 2 (Option B mesh), 3 (CLI fixtures), and 5 (GLGizmoMeasure) are local/offline. Workstream 4 (D3D12) is investigation-heavy, time-boxed, and may not produce a clean "feature" output. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

**Target features (5 workstreams, 20 active requirements + 1 deferred across 14 phases starting from 103):**
- **WS1 — Auto filament-map recommendation:** libslic3r auto-computes the per-plate filament map during slicing; Qt6 reads back + surfaces in a `FilamentGroupPopup` UI; widen the Qt6 mode enum from 2-value (Auto/Manual) to upstream 4-value (`fmmAutoForFlush`/`fmmAutoForMatch`/`fmmManual`/`fmmDefault`).
- **WS2 — Option B real wipe-tower mesh:** upgrade the rendered wipe-tower from the v4.4 Option A dimensioned box to a real mesh via `wipe_tower_mesh_data` + `convex_hull_3d` (mirrors upstream `3DScene.cpp:887-925 load_real_wipe_tower_preview`); Option A preserved as the parallel fallback. Re-opens Phase 99 Frozen Decision 2.
- **WS3 — CLI fixtures + GUI deep-link loading:** deterministic argv-based fixture loading to unblock runtime visual evidence (the recurring Windows-capture-API blocker). The argv plumbing already exists (`main_qml.cpp:170-257`); WS3 is fixtures + recipes + the `objectCreated`/`frameSwapped` readiness gate, NOT new flags.
- **WS4 — D3D12 crash root cause + backend readiness:** time-boxed investigation of the D3D12 startup `0xc0000005`; debug-layer wiring behind an env flag; default-backend promotion stays explicitly out of scope.
- **WS5 — Full GLGizmoMeasure engine + AssembleViewDataPool clipper:** port the measurement gizmo's feature-picking engine (per-volume ITS + scene raycaster + `Measure::Measuring` instantiation + snap UX); AssembleViewDataPool `ModelObjectsClipper` registration shares the per-volume ITS dependency.

**Current state (from pre-planning research):**
- Zero new external libraries needed (STACK.md confirmed): all 5 workstreams reuse existing libslic3r APIs (`wipe_tower_mesh_data`, `get_recommended_filament_maps`, `Measure::Measuring`, `SceneRaycaster`/`MeshRaycaster`) and existing Qt 6.10 modules; two local code extensions only (a `GizmoVertex` ITS-capable builder + a per-volume `SceneRaycaster` port).
- The v4.4 capture-by-value readback pattern (`WipeTowerGeometry` POD + `wipeTowerGeometryReady` signal) is the proven backbone for WS1 + WS2 readback.
- Two cross-workstream dependencies dominate sequencing: (a) the per-volume ITS accessor on `ProjectServiceMock` unblocks BOTH WS5's Measure raycaster AND the AssembleViewDataPool `ModelObjectsClipper`; (b) the argv fixture readiness gate from WS3 is the same gate WS4 needs to repro the D3D12 crash deterministically.
- 8 critical pitfalls documented (PITFALLS.md): the filament-map enum-widening 3MF migration hazard (raw `setInt` at `ProjectServiceMock.cpp:4997`), the Option A baseline regression risk, the D3D12 debug-layer Release leak, the `Measure::Measuring` ITS lifetime/UAF hazard, and the raycaster per-frame performance trap.

## Phases

- [x] Phase 103: CLI Fixture Readiness Gate (completed 2026-07-12)
- [x] Phase 104: CLI Fixture Recipes And Multi-Material Model (completed 2026-07-12)
- [x] Phase 105: D3D12 Debug Layer Wiring (completed 2026-07-12)
- [x] Phase 106: D3D12 Crash Root-Cause And Backend Readiness (time-boxed) (completed 2026-07-12)
- [x] Phase 107: Filament-Map Mode Enum Widening And 3MF Migration (completed 2026-07-12)
- [ ] Phase 108: Filament-Map Auto Recommendation Readback
- [ ] Phase 109: Option B Wipe-Tower Mesh Readback And Real Rendering
- [ ] Phase 110: Filament-Map Popup UI And Mode Surfacing
- [ ] Phase 111: Filament-Map Save-Reload Round-Trip
- [ ] Phase 112: Per-Volume ITS Accessor And Mesh Cache
- [ ] Phase 113: Scene And Mesh Raycaster Port
- [ ] Phase 114: Measure Engine Instantiation And Feature Readouts
- [ ] Phase 115: GLGizmoMeasure Snap UX And Feature Picking
- [ ] Phase 116: v4.5 Verification And Cross-Workstream Regression

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 103 | 1/1 | Complete   | 2026-07-12 |
| 104 | 1/1 | Complete   | 2026-07-12 |
| 105 | 1/1 | Complete   | 2026-07-12 |
| 106 | 1/1 | Complete   | 2026-07-12 |
| 107 | 1/1 | Complete   | 2026-07-12 |
| 108 | Filament-Map Auto Recommendation Readback | After a successful slice, read back the auto-recommended per-plate filament map from libslic3r inside the SliceService worker (between `print.process()` and `activePrint_.store(nullptr)`), captured by value into a POD, delivered via `filamentMapReady` — mirrors v4.4 WTREAD-01. | FMAP-01 |
| 109 | Option B Wipe-Tower Mesh Readback And Real Rendering | Capture `wipe_tower_mesh_data` (+ `convex_hull_3d`) in the worker, render the real mesh post-slice via a NEW parallel builder/upload path, and preserve the v4.4-frozen Option A box as the fallback when the mesh is `nullopt` — Option A and Option B coexist gated on the mesh, shipping together (re-opens Phase 99 Frozen Decision 2). | WTMESH-01, WTMESH-02, WTMESH-03 |
| 110 | Filament-Map Popup UI And Mode Surfacing | Surface the 3 selectable modes (AutoForFlush / AutoForMatch / Manual) plus the auto-recommended map preview in a `FilamentGroupPopup` QML driven by EditorViewModel Q_INVOKABLE/Q_PROPERTY APIs (no QML-local state); `fmmDefault` is the per-plate inherit sentinel, NOT a 4th radio button. | FMAP-03 |
| 111 | Filament-Map Save-Reload Round-Trip | Automated test asserting the auto-recommended map round-trips through save→reload (mirrors the Phase 97 thumbnail pattern) and that `fmmDefault` inheritance resolves correctly. Ships last in the WS1 workstream. | FMAP-04 |
| 112 | Per-Volume ITS Accessor And Mesh Cache | Add a per-volume ITS accessor to `ProjectServiceMock` (current `meshData()` is per-object-flattened) with an explicit `shared_ptr<indexed_triangle_set>` ownership contract + mesh-changed signal — the cross-workstream dependency that also unblocks the AssembleViewDataPool `ModelObjectsClipper` registration. | MEASURE-01 |
| 113 | Scene And Mesh Raycaster Port | Port `MeshRaycaster` + `SceneRaycaster` from upstream into `src/core/rendering/` as pure-CPU helpers (matching the `GizmoGeometry`/`ObjectPicking` pattern), with two-stage pick (coarse AABB via `ObjectPicking::pickSourceObject`, then per-triangle ITS on the hit volume only). | MEASURE-02 |
| 114 | Measure Engine Instantiation And Feature Readouts | Instantiate `Measure::Measuring` per-volume (not reimplement) to produce real measurements (angle, direct/perpendicular distance, distance XYZ), replacing the current AABB stub at `AssemblyMeasureGeometry`. | MEASURE-03 |
| 115 | GLGizmoMeasure Snap UX And Feature Picking | Wire the GLGizmoMeasure snap UX (Point/Edge/Circle/Plane feature picks; Shift toggles FeatureSelection vs PointSelection) through the raycaster + `Measuring`, with `SurfaceFeature` raw-pointer scrubbing at the libslic3r→Qt boundary. | MEASURE-04 |
| 116 | v4.5 Verification And Cross-Workstream Regression | Lock the milestone: automated tests + source audits confirm Option A does not regress and Option B fires only when populated (WTMESH-04), measurement readouts are real with correct ITS lifetime (MEASURE-05), the canonical verifier passes, the app launches, and Prepare/Preview/AssembleView rendering is regression-free. | WTMESH-04, MEASURE-05 |

### Build Order (parallelism guidance for the executor)

Phase numbers are sequential, but several phases are parallel-safe and may be executed concurrently:

- **Wave A (parallel, no deps):** Phase 103 (WS3 gate) + Phase 105 (WS4 debug layer). Phase 104 (WS3 fixtures) follows 103; Phase 106 (WS4 root-cause) follows 105.
- **Wave B (after 103):** Phase 107 (WS1 enum widening) — independent of WS3/WS4; MUST precede Phase 108 (Pitfall 2).
- **Wave C (parallel readback, share v4.4 worker-capture pattern):** Phase 108 (WS1 readback) + Phase 109 (WS2 readback+render; readback is the first plan within the phase).
- **Wave D (after readback):** Phase 110 (WS1 UI) — needs 107 + 108. Phase 111 (WS1 round-trip) — needs 110.
- **Wave E (after 103 only, parallel with late WS1):** Phase 112 (WS5 per-volume ITS) → Phase 113 (raycaster) → Phase 114 (Measuring) → Phase 115 (snap UX), sequential.
- **Wave F (last):** Phase 116 (verification) — needs all feature phases.

### Phase 103: CLI Fixture Readiness Gate

**Status:** Not started
**Plans:** 1/1 plans complete
**Workstream:** WS3 (CLI Fixtures)

Success criteria:
1. argv-driven fixture actions (`--load-model`, `--open-page`, `--open-dialog`) are deferred until BOTH `QQmlApplicationEngine::objectCreated` AND the first `QQuickWindow::frameSwapped` have fired, replacing the current `QTimer::singleShot(0)` trick so the first captured frame is not empty.
2. A fixture-driven model load reaches a constructed `BackendContext`/`ProjectServiceMock` (`projectService_` non-null, `Model*` allocated) and returns `false` (not crash) if a service is not yet ready.
3. The event-loop ordering (`QGuiApplication` → `selectRhiBackendFromEnvironment` → `BackendContext` ctor → `QQmlApplicationEngine::load` → `objectCreated` → first `frameSwapped` → fixture actions) is documented in the phase SUMMARY.

### Phase 104: CLI Fixture Recipes And Multi-Material Model

**Status:** Not started
**Plans:** 1/1 plans complete
**Workstream:** WS3 (CLI Fixtures)

Success criteria:
1. A multi-material fixture model exists in `tests/data/` so multi-material-dependent features (wipe-tower, filament-map, AMS) can be exercised deterministically (the current `test_model.stl` is single-material).
2. Documented argv recipes (`--load-model X --open-page Y` combos) cover the major GUI states: Prepare empty, Prepare with model, Preview, AssembleView, and the settings dialogs — each reachable without simulated clicks.
3. The argv fixtures are documented as test-evidence plumbing (anti-feature: shipping as a user-facing deep-link product feature), and the `--help` text keeps them out of the user-facing surface.

### Phase 105: D3D12 Debug Layer Wiring

**Status:** Not started
**Plans:** 1/1 plans complete
**Workstream:** WS4 (D3D12)

Success criteria:
1. The D3D12 debug layer is enabled behind an explicit env flag (e.g. `OWZX_RHI_RENDERER=d3d12-debug` or `OWZX_D3D12_DEBUG=1`) in `RhiBackendSelector.cpp` before `QRhi::create(D3D12, params)`, gated to Debug builds (`#ifndef QT_NO_DEBUG` / CMake `OWZX_D3D12_DEBUG` defaulting OFF) so it never ships in Release.
2. A runtime gate logs that crashes will not be annotated when `OWZX_RHI_RENDERER=d3d12` is set in Release without the debug env var (makes the off-state visible).
3. Reproducing with `OWZX_RHI_RENDERER=d3d12` (Debug + debug layer) surfaces validation output and/or a minidump routed via the existing `CrashHandlerWin` to `crash_dumps/`.

### Phase 106: D3D12 Crash Root-Cause And Backend Readiness

**Status:** Not started (TIME-BOXED — investigation-heavy; may not produce a clean feature)
**Plans:** 1/1 plans complete
**Workstream:** WS4 (D3D12)

Success criteria:
1. The D3D12 crash root cause is isolated as a NEW finding (distinct from the already-merged BUG-V31-1 fix at `RhiViewportRenderer.cpp:286-296`), cited at the offending code/config layer (e.g. descriptor-heap exhaustion, root-signature mismatch, 256-byte cbuffer alignment at `:1424-1428`), or the investigation is documented as inconclusive with the time-box consumed.
2. Default-backend promotion to D3D12 is explicitly confirmed out of scope (stays opt-in via `OWZX_RHI_RENDERER=d3d12` until the root cause is resolved); `defaultWindowsCandidates()` keeps D3D11-first.
3. Vulkan is documented as SDK-blocked (`vulkan` under `QT_DISABLED_PUBLIC_FEATURES`); the phase does NOT attempt to unblock Vulkan by recompiling Qt. The Release-build debug-layer-off invariant is proven (no perf regression).

### Phase 107: Filament-Map Mode Enum Widening And 3MF Migration

**Status:** Not started
**Plans:** 1/1 plans complete
**Workstream:** WS1 (Auto Filament-Map)

Success criteria:
1. The Qt6 filament-map mode enum is widened from 2-value (`0=Auto, 1=Manual`) to upstream 4-value (`0=fmmAutoForFlush, 1=fmmAutoForMatch, 2=fmmManual, 3=fmmDefault`) across all four surfaces atomically: `PartPlate.h`, `ProjectServiceMock` API, EditorViewModel Q_PROPERTY, and the QML popup enum.
2. A 3MF read-side migration normalizes legacy values so a pre-v4.5 "Manual" plate (legacy `1`) does NOT silently reload as `fmmAutoForMatch` (new `1`); all writers/readers audited via `grep -rn "filament_map_mode|filamentMapMode|setFilamentMapMode|m_filament_map_mode"`.
3. A round-trip assertion (write a Manual plate, reload, assert `filamentMapMode()==2`) is in place; `fmmDefault` is NOT added to the serialized enum-name map (upstream keeps it out as the per-plate inherit sentinel).

### Phase 108: Filament-Map Auto Recommendation Readback

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS1 (Auto Filament-Map)

Success criteria:
1. After a successful slice, the auto-recommended per-plate filament map is read from `Print::get_filament_maps()` + `get_filament_map_mode()` INSIDE the SliceService worker between `print.process()` and `activePrint_.store(nullptr)`, captured by value into a `FilamentMapResult` POD — zero `Print*` escapes the worker (grep self-check mirrors Phase 100).
2. The result is delivered to the GUI thread via a queued `filamentMapReady` signal (mirrors `wipeTowerGeometryReady`) and consumed by an `EditorViewModel::onFilamentMapReady` slot in the same `private slots:` block placement as `onWipeTowerGeometryReady`.
3. When the map is empty/invalid (single-extruder / no auto-compute), the slot enforces a `valid=false` gate that leaves the popup in a safe state without leaking stale data (mirrors WTREAD-02).

### Phase 109: Option B Wipe-Tower Mesh Readback And Real Rendering

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS2 (Option B Real Wipe-Tower Mesh) — re-opens Phase 99 Frozen Decision 2

Success criteria:
1. When `Print::wipe_tower_data().wipe_tower_mesh_data` is populated (post-slice, multi-material), the renderer draws a real mesh (brim + rib/cone base + true non-convex footprint) via a NEW parallel builder `buildWipeTowerMeshVertices` + the existing dynamic-size `uploadWipeTowerBuffer` path; the mesh is the `convex_hull_3d` hull, not the raw toolpath (mirrors `3DScene.cpp:887-925`).
2. The v4.4-frozen Option A box (`buildWipeTowerVertices` / `uploadWipeTowerBuffer` box path) is preserved BYTE-FOR-BYTE unchanged as the fallback when `wipe_tower_mesh_data == std::nullopt` (single-material / pre-slice / cleared) — Option B is a parallel branch, NOT a modification of the frozen baseline; the mesh is deep-copied by value in the worker (TriangleMesh copy ctor), no `Print*`/reference escapes.
3. The SoftwareViewport fallback renderer mirrors the Option B mesh (parallel to v4.4 Phase 101 QPainter box consistency); both paths coexist gated on whether the mesh was captured.

### Phase 110: Filament-Map Popup UI And Mode Surfacing

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS1 (Auto Filament-Map)

Success criteria:
1. A `FilamentGroupPopup` QML surfaces exactly 3 selectable radio modes (AutoForFlush "Filament-Saving" / AutoForMatch "Convenience" / Manual "Custom") with the auto-recommended map preview and an explicit "Apply" affordance; `fmmDefault` is NOT a 4th radio button (it is the per-plate inherit sentinel resolved by `get_real_filament_map_mode`).
2. The popup is driven entirely by EditorViewModel Q_INVOKABLE/Q_PROPERTY APIs (`plateFilamentMaps`, `setPlateFilamentMaps`, `plateFilamentMapMode`, `setPlateFilamentMapMode`, `recommendedFilamentMaps`) delegating to `ProjectServiceMock`/`PartPlate` — NO QML-local `property var` state (AGENTS.md QML-boundaries rule); "Convenience Mode" is connection-gated (disabled unless printer connected).
3. A ViewModelSmokeTests case modeled on `wipeTowerGeometryReadbackAppliesValidAndInvalidGate` exercises the set→get round-trip and the mode-resolution.

### Phase 111: Filament-Map Save-Reload Round-Trip

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS1 (Auto Filament-Map) — ships last in the workstream

Success criteria:
1. An automated test asserts the auto-recommended filament map round-trips through save→reload (modeled on the Phase 97 thumbnail round-trip pattern) so the reloaded `filament_maps` matches the saved auto-computed assignment.
2. The test asserts `fmmDefault` inheritance resolves correctly (a plate set to `fmmDefault` inherits the project-level mode on reload via `get_real_filament_map_mode`).
3. A pre-v4.5 legacy 3MF (if available as a fixture) reloads with Manual plates still Manual after the enum widening (Pitfall 2 migration verification).

### Phase 112: Per-Volume ITS Accessor And Mesh Cache

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS5 (GLGizmoMeasure) — cross-workstream dependency (also unblocks AssembleViewDataPool clipper)

Success criteria:
1. A per-volume `indexed_triangle_set` accessor is added to `ProjectServiceMock` (current `meshData()` is per-object-flattened), exposing `model_->objects[obj]->volumes[vol]->mesh().its` under `#ifdef HAS_LIBSLIC3R`.
2. An explicit ITS ownership contract is defined and implemented BEFORE any feature-picking code: a per-volume `std::shared_ptr<indexed_triangle_set>` cache (or `VolumeMeshCache`) held by the viewmodel, with a mesh-changed signal wired to every mesh mutation (load, baked transform, boolean, cut, simplify) so `Measure::Measuring` lifetime ≤ shared_ptr lifetime (Pitfall 6).
3. The AssembleViewDataPool `ModelObjectsClipper = 1 << 4` reserved slot is registered (paired with the same ITS cache) so the deferred clipper resource is live.

### Phase 113: Scene And Mesh Raycaster Port

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS5 (GLGizmoMeasure)

Success criteria:
1. `MeshRaycaster` + `SceneRaycaster` are ported from upstream (`MeshUtils.hpp` / `SceneRaycaster.hpp`) into `src/core/rendering/` as pure-CPU helpers (no `Qt6::Quick` / `qrhi.h`; Eigen + libslic3r `TriangleMesh` only), unit-testable like `GizmoGeometry`/`GizmoMath`/`ObjectPicking`.
2. Two-stage picking is implemented and mandatory: (1) `ObjectPicking::pickSourceObject` ray-AABB coarse pick returns the volume index; (2) per-triangle ITS raycast runs ONLY on the hit volume's cached `Measuring`/raycaster — never all volumes per mouse move (Pitfall 7).
3. `SceneRaycaster::hit(mouse, camera)` returns a typed `HitResult` (Volume/Bed/Gizmo) with the nearest triangle hit; the per-volume raycaster map is invalidated on object add/remove/transform-end (not on every mouse move).

### Phase 114: Measure Engine Instantiation And Feature Readouts

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS5 (GLGizmoMeasure)

Success criteria:
1. `Measure::Measuring` is instantiated per-volume from the cached `shared_ptr<indexed_triangle_set>` (instantiate, do NOT reimplement — AGENTS.md), producing real measurements: angle between two edges, direct/perpendicular distance, and distance XYZ — replacing the current AABB stub at `AssemblyMeasureGeometry`.
2. `Measuring` is reconstructed only on a mesh-change signal (Pitfall 6); transient transforms (move/rotate/scale without apply) apply the inverse world transform to the hit point, NOT a rebuild.
3. EditorViewModel exposes measurement-result Q_PROPERTYs fed by `Measuring::get_feature(...)`, and the AABB stub remains as a fallback for degenerate/no-ITS cases.

### Phase 115: GLGizmoMeasure Snap UX And Feature Picking

**Status:** Not started
**Plans:** 0/0
**Workstream:** WS5 (GLGizmoMeasure)

Success criteria:
1. The GLGizmoMeasure snap UX is wired through the raycaster + `Measuring`: hover detects the nearest Point/Edge/Circle/Plane feature (`get_feature`), snaps the cursor, and renders the appropriate gripper (sphere/cylinder/circle/plane) with distinct selected/hover colors.
2. Shift toggles FeatureSelection (default, whole feature) vs PointSelection (exact cursor point); Escape cancels the second then first feature; Delete restarts selection (mirrors `GLGizmoMeasure.cpp:409-442`).
3. `SurfaceFeature` raw pointers (`void* volume`, `vector<int>* plane_indices`) are scrubbed/repointed to Qt-owned volume indices before crossing the libslic3r→Qt boundary (Pitfall 6 UAF prevention).

### Phase 116: v4.5 Verification And Cross-Workstream Regression

**Status:** Not started
**Plans:** 0/0
**Workstream:** Cross-workstream verification (Phase 98/102 style)

Success criteria:
1. An automated test + source audit confirm Option A does not regress (the Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent` test still passes unchanged) AND Option B fires only when `wipe_tower_mesh_data` is populated (WTMESH-04).
2. An automated test + source audit confirm the measurement readouts are real (not the AABB stub) and the per-volume ITS lifetime is correct (no UAF across the libslic3r→Qt boundary); a frame-time regression test (N=10 volumes, cursor over viewport) asserts frame time < 16ms (MEASURE-05, Pitfall 7).
3. The canonical verifier passes, `build/OWzxSlicer.exe` launches, and Prepare/Preview/AssembleView rendering is regression-free across all 5 workstreams; runtime visual evidence is reachable via the WS3 argv fixtures.

## Deferred Backlog

- **MEASURE-06:** Assembly-mode transformation actions (Parallel / Coincidence / Reverse-rotation per upstream `Measure.hpp:186-200 AssemblyAction`) — documented as P3 future, NOT in v4.5 scope. Ship the measurement readouts first (MEASURE-01..05); Assembly actions need a stable feature-picking foundation + full transformation/undo-redo integration.
- Full PLATE-09 save/reload state assertions — partially addressed by v4.5 WS3 fixture work; complete coverage remains future.
- D3D12 promotion to default backend — only after the Phase 106 root cause is fixed and stability proven across the full Prepare/Preview/slice flow.
- Vulkan production backend — blocked on a Vulkan-enabled Qt SDK (current SDK disables `vulkan`); evaluation-only.
- Full i18n translation coverage beyond strings touched by active workflows.

## Removed Scope

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware calibration are no longer backlog items.
- `fmmDefault` as a 4th user-visible popup radio button (anti-feature — upstream keeps it out of `mode_list`).
- Option B rendering per-extruder color slabs AND mesh together (anti-feature — upstream chose silhouette over stripes, `model_per_colors.resize(1)`).
- argv fixtures shipped as a user-facing deep-link product feature (anti-feature — OWzx-only test-evidence plumbing; upstream has no `--open-page`/`--open-dialog`/`--load-model`).
- D3D12 default-backend promotion before the Phase 106 root cause is resolved (stays opt-in).
- Assembly-actions-before-engine (MEASURE-06 deferred until MEASURE-01..05 ship).
- OpenVDB-dependent features (hollow/support paint gizmos) — OpenVDB unavailable; none of the 5 v4.5 workstreams need it.
- Changing libslic3r slicing algorithms, adding unmapped product behavior, or creating alternate build directories.

## Next Step

Plan Phase 103 (CLI Fixture Readiness Gate) after this roadmap is approved:

```text
$gsd-plan-phase 103
```

---

*Last updated: 2026-07-12 after v4.5 roadmap creation (14 phases, 103-116; 20 active requirements mapped, MEASURE-06 deferred).*
