# Project Research Summary

**Project:** OWzx Slicer - OrcaSlicer Qt6/QML Migration
**Domain:** Qt6/QML desktop slicer (Windows) — milestone v4.5 Backlog Closure
**Researched:** 2026-07-12
**Confidence:** HIGH (all integration points cited at file:line in current source + upstream `third_party/OrcaSlicer`; MEDIUM only on D3D12 root cause which is investigation output, not pre-known)

This is the single document the v4.5 roadmapper + plan-phases consult. It synthesizes STACK.md, FEATURES.md, ARCHITECTURE.md, and PITFALLS.md into roadmap-ready decisions for the 5 backlog-closure workstreams. Read this first; the four detail docs are reference.

---

## Executive Summary

v4.5 closes the deferred backlog in one cycle via 5 workstreams: (1) auto filament-map recommendation, (2) Option B real wipe-tower mesh, (3) CLI fixtures + argv GUI fixture loading, (4) D3D12 crash root cause + backend readiness, and (5) the full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper. **The headline finding: ZERO new external libraries are needed** — all 5 workstreams are served by existing libslic3r APIs (`wipe_tower_mesh_data`, `get_recommended_filament_maps`, `Measure::Measuring`, `SceneRaycaster`/`MeshRaycaster`), existing Qt 6.10 modules already linked (`QCommandLineParser`, `QRhiD3D12InitParams`, `QtQuick.Controls`), and two local code extensions (a `GizmoVertex` ITS-capable builder + a per-volume `SceneRaycaster` port). OpenVDB/FFmpeg/OCCT-as-mesh-boolean are NOT touched by any workstream.

**The recommended approach reuses v4.4's proven capture-by-value readback pattern as the backbone for WS1 + WS2**, and ports (not reimplements) upstream libslic3r engine code for WS5. Two cross-workstream dependencies dominate the sequencing: (a) the per-volume ITS accessor on `ProjectServiceMock` unblocks BOTH WS5's Measure raycaster AND the AssembleViewDataPool `ModelObjectsClipper`, and (b) the argv fixture readiness gate from WS3 is the SAME gate WS4 needs to repro the D3D12 crash deterministically. The suggested build order exploits both: WS3 (cheapest unblocker) + WS4 (independent investigation) parallel → WS1+WS2 readback (parallel, share worker-capture pattern) → WS1 UI + WS2 mesh renderer → WS5 step-1 (per-volume ITS) → WS5 steps 2-5 (raycaster → Measuring → clipper).

**Key risks and mitigations:** the filament-map enum widening (2-value → 4-value) has a 3MF persistence migration hazard (raw `setInt` at `ProjectServiceMock.cpp:4997` would silently reload pre-v4.5 "Manual" plates as "AutoForMatch" without read-side migration); Option B must NOT regress the v4.4-frozen Option A baseline (add a parallel path, do not modify `buildWipeTowerVertices`); the D3D12 work is HIGH/OPEN-ENDED complexity and must be time-boxed (it may not produce a clean feature); `Measure::Measuring` requires a per-volume ITS ownership contract (`shared_ptr<indexed_triangle_set>`) defined BEFORE any feature-picking code, or the `SurfaceFeature` raw-pointer fields (`void* volume`, `vector<int>* plane_indices`) become latent UAFs across the libslic3r→Qt boundary. The full 8-pitfall matrix is in §4.

---

## Stack Additions

### NONE — confirmed explicitly

| # | Workstream | New dependency needed? | Verdict |
|---|---|---|---|
| 1 | Auto filament-map recommendation | **No** | libslic3r `ToolOrdering::get_recommended_filament_maps` (`ToolOrdering.hpp:244`) + `Print::get_filament_map_mode()` (`Print.hpp:997`) already available; widen Qt enum 2→4 value; no new Qt module (`FilamentGroupPopup` uses existing `QtQuick.Controls` + `QtQuick.Layouts` + `Cx*` controls) |
| 2 | Option B real wipe-tower mesh | **No** | libslic3r `wipe_tower_mesh_data` (`Print.hpp:766`) + `TriangleMesh::convex_hull_3d()` (`TriangleMesh.hpp:136`, qhull-backed) available; extend `GizmoVertex` layout LOCALLY (no format change — only vertex count/positions become arbitrary) |
| 3 | CLI fixtures + argv GUI loading | **No** | `QCommandLineParser`/`QCommandLineOption` ALREADY wired in `main_qml.cpp:170-203`; fixture work is additive options + recipes + multi-material fixture model, NOT a new library |
| 4 | D3D12 crash root cause + backend eval | **Qt6 RHI debug API only (investigation tooling, NOT shipped)** | `QRhiD3D12InitParams` already used in `RhiBackendSelector.cpp:22`; debug layers + PIX are developer-machine tooling (env-gated), NOT build dependencies. Vulkan SDK-BLOCKED (`vulkan` under `QT_DISABLED_PUBLIC_FEATURES` in `Qt6GuiTargets.cmake`) |
| 5 | Full GLGizmoMeasure engine + clipper | **No** | Upstream `SceneRaycaster`/`MeshRaycaster` (`MeshUtils.hpp`) + `Measure::Measuring` (`Measure.hpp:119`) all compile under the existing libslic3r target; PORT (do not add) into `src/core/rendering/`. Eigen already linked |

**Existing technologies (DO NOT re-add — all validated through v4.4):**
- **Qt 6.10** (Qml/Quick/QuickControls2/Gui/GuiPrivate/OpenGL/Concurrent/Network/Test/LinguistTools/ShaderTools) — GUI shell, RHI viewport, viewmodels, tests. `CMakeLists.txt:63,66`
- **libslic3r v7.0.1 from-source** — sole slicing engine (Print, WipeTowerData, ToolOrdering, Measure, TriangleMesh, indexed_triangle_set). Out-of-Scope to change
- **QRhi (Qt GUI private)** — default D3D11 renderer; D3D12 candidate backend via `RhiBackendSelector.cpp`
- **qhull/qhullcpp** — convex hull for `TriangleMesh::convex_hull_3d()` (WS2 source). Already linked via libslic3r deps
- **Eigen 5.0** — raycaster + `Measure::Measuring` math. Already linked

**What NOT to use (confirmed by STACK.md "What NOT to Use"):** OpenVDB (unavailable; only for removed-scope hollow/support paint gizmos — NONE of the 5 WS need it); FFmpeg/WebRTC/MetaRTC (media scope removed); Vulkan QRhi backend on production SDK (SDK-blocked); D3D12 as default backend (known crash — explicit opt-in only); custom argv parser (reinvents what `QCommandLineParser` already provides); OCCT-based mesh boolean (CGAL 5.4 < 5.6 required; use qhull `convex_hull_3d` matching upstream `3DScene.cpp:914`); `Qt6::Quick3D`/`Charts`/`DataVisualization`/`Labs*` (not linked, `FilamentGroupPopup` needs none); PIX/`WinPixEventRuntime` as shipped dependency (capture tooling belongs to dev machine).

---

## Feature Decisions Per Workstream

Source of truth: `third_party/OrcaSlicer/` user-visible behavior. Each workstream inherits upstream behavior; nothing here invents new product capability.

### WS1 — Auto Filament-Map Recommendation (Priority P1)

| Class | Decision | Upstream anchor |
|---|---|---|
| **Table stakes** | 4-value enum parity (`fmmAutoForFlush`/`fmmAutoForMatch`/`fmmManual`/`fmmDefault`); per-plate vs global resolution via `get_real_filament_map_mode`; "Convenience Mode" connection-gating (disabled unless printer connected); manual mode passthrough (already works) | `PrintConfig.hpp:424-429`; `PartPlate.cpp:317-328`; `FilamentGroupPopup.cpp:242-254` |
| **Differentiator** | NONE — pure source-truth parity. Matching OrcaSlicer/Bambu Studio is the bar, not an edge | — |
| **Anti-feature** | Exposing `fmmDefault` as a 4th user-visible radio button. Upstream keeps it out of `mode_list` (`FilamentGroupPopup.hpp:52`) — it is a per-plate "inherit from global" resolution sentinel, NOT a user-selectable mode. The popup has exactly 3 radio buttons | `FilamentGroupPopup.hpp:52`; `PrintConfig.cpp:580-583` (no `"Default"` string in serialization map) |

### WS2 — Option B Real Wipe-Tower Mesh (Priority P1, re-opens Phase 99 Frozen Decision 2)

| Class | Decision | Upstream anchor |
|---|---|---|
| **Table stakes** | Show the brim (users expect to see the prime-tower brim they will actually print); rib/cone base when `wipe_tower_wall_type == wtwRib`; true non-convex footprint for plate-collision margining | `Print.cpp:5070-5094 construct_mesh`; `3DScene.cpp:887-925 load_real_wipe_tower_preview` |
| **Differentiator** | NONE — pure parity | — |
| **Anti-feature** | Per-extruder color slabs ON the real mesh. Upstream's `load_real_wipe_tower_preview` deliberately drops per-extruder slabs (`model_per_colors.resize(1)` at `:911`) because real mesh geometry is incompatible with slab-splitting. Upstream chose silhouette over stripes for the real path — do NOT try to render both | `3DScene.cpp:897-902,911` |

### WS3 — CLI Fixtures + argv GUI Fixture Loading (Priority P1, cheapest unblocker)

| Class | Decision | Upstream anchor |
|---|---|---|
| **Table stakes** | Deterministic model fixtures (single + multi-material); one argv recipe per screenshot target; FIXTURE-02 3MF round-trip assertions (unblocked by v4.3 shared writer) | OWzx-only pattern (no upstream equivalent) |
| **Differentiator** | NONE — this is test infrastructure, correctly classified as enabling work, not product capability | — |
| **Anti-feature** | Shipping these flags to end users as a "deep link" product feature. Upstream OrcaSlicer has NO `--open-page`/`--open-dialog`/`--load-model` (its CLI is `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`). These are test-evidence plumbing; exposing them in user-facing help/docs invents product behavior upstream doesn't have | `OrcaSlicer.cpp:7122-7216` |

### WS4 — D3D12 Crash Root Cause + Backend Readiness (Priority P2/P3, HIGH/OPEN-ENDED, time-box)

| Class | Decision | Upstream anchor |
|---|---|---|
| **Table stakes** | NONE for end users. D3D11 is the verified default; users get a working renderer today. D3D12 is invisible unless they set `OWZX_RHI_RENDERER=d3d12` | N/A (upstream is raw OpenGL) |
| **Differentiator** | NONE directly — a working D3D12/Vulkan path is platform investment that unlocks future heavy-rendering features (larger G-code scenes, MSAA), not product capability | — |
| **Anti-feature** | Promoting D3D12 to default before the crash is root-caused (PROJECT.md Out-of-Scope forbids this explicitly). Also anti-feature: treating "D3D12 doesn't crash on my machine" as success without understanding WHY — the crash is environment-sensitive and could regress. Also anti-feature: attempting to unblock Vulkan by recompiling Qt (SDK-level block, out of scope) | `RhiBackendSelector.cpp:37-46 defaultWindowsCandidates`; `Qt6GuiTargets.cmake QT_DISABLED_PUBLIC_FEATURES` |

### WS5 — Full GLGizmoMeasure Feature-Picking Engine + AssembleViewDataPool Clipper (Priority P2)

| Class | Decision | Upstream anchor |
|---|---|---|
| **Table stakes** | Point-to-point distance; edge length; angle between two edges; Distance XYZ. The current AABB stub (`AssemblyMeasureGeometry.h:23-33`) passes screenshot-parity but fails "click a real feature" | `Measure.hpp:148-180 MeasurementResult`; `GLGizmoMeasure.cpp:1990-2048` |
| **Different stakes** | Circle measurement (center/radius/normal); Plane measurement; Assembly-mode transformation actions (Parallel / Center coincidence / Reverse rotation). Genuinely advanced capability worth inheriting | `Measure.hpp:186-200 AssemblyAction`; `GLGizmoMeasure.cpp:2055-2099+` |
| **Differentiator** | NONE vs upstream — parity. vs other slicers: the Assembly-mode transformation actions are a genuinely advanced capability | — |
| **Anti-feature** | Porting the Assembly-mode transformation actions (Parallel/Coincidence/etc.) BEFORE the basic measure engine works. Those depend on full transformation/undo-redo integration that is scope-creep. Ship measure readouts first (v4.5); defer Assembly actions (v4.6+) | `Measure.hpp:186-200` |

---

## Architecture Integration Points (file:line anchors per workstream)

All v4.5 workstreams plug into the same three-layer pipeline (QML Presentation → ViewModels → Services + Rendering) established v3.8-v4.4. No new layers.

### WS1 — Auto Filament-Map

| Integration Point | File:Line | New/Mod |
|---|---|---|
| Filament-map readback in worker (mirrors Phase 100 WTREAD) | `src/core/services/SliceService.cpp:642-659` (alongside WTREAD capture) | **Modified** |
| New `FilamentMapResult` POD | `src/core/services/SliceService.h:42` (next to `WipeTowerGeometry`) | **New** |
| New signal `filamentMapReady` | `src/core/services/SliceService.h:167` (`wipeTowerGeometryReady`) | **New** |
| Enum widening (Qt core) | `src/core/model/PartPlate.h:273` (`m_filament_map_mode = 0`, currently 2-value comment) | **Modified** |
| ViewModel slot + Q_PROPERTYs | `src/core/viewmodels/EditorViewModel.h:829` (`onWipeTowerGeometryReady`) + `:719-724` getters | **Modified** |
| ProjectServiceMock API (accept mode 0-3) | `src/core/services/ProjectServiceMock.h:149-151` (`setPlateFilamentMap`) + `:4997` (`setInt` — the migration hazard) | **Modified** |
| QML popup | `src/qml_gui/components/FilamentGroupPopup.qml` (replaces `BBLTopbar.qml:396-397` placeholder) | **New** |
| QML bindings | `src/qml_gui/pages/PreparePage.qml:1670-1675` (WTREAD binding pattern) | **Modified** |

**Upstream anchors:** `Print.hpp:996-997` (`get_filament_maps`, `get_filament_map_mode`); `Print.cpp:2484-2493` (auto-compute); `PrintConfig.hpp:424` (4-value enum); `FilamentGroup.hpp:119` (auto-compute engine already invoked by libslic3r during `print.process()`).

### WS2 — Option B Real Wipe-Tower Mesh (re-opens Phase 99 Frozen Decision 2)

| Integration Point | File:Line | New/Mod |
|---|---|---|
| Mesh readback in worker (extends WTREAD capture) | `src/core/services/SliceService.cpp:647` (`print.wipe_tower_data()`) | **Modified** |
| WipeTowerGeometry extension (add `QVector<GizmoVertex> meshVerts`) | `src/core/services/SliceService.h:42` | **Modified** |
| NEW mesh vertex builder (sibling, NOT overload of `buildWipeTowerVertices`) | `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-535` | **New** |
| uploadWipeTowerBuffer branch (gated on meshVerts non-empty) | `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1073-1080` (`ensureBuffer` + `byteSize` at `:1082` already handles variable size) | **Modified** |
| Coexist gate (Option A fallback when `wipe_tower_mesh_data == nullopt`) | `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1075-1079` | **Modified** |

**Decision:** NEW helper `buildWipeTowerMeshVertices`, NOT extend `buildWipeTowerVertices`. The latter emits a fixed 36-vertex box from 5 floats; a mesh path needs a different signature. A sibling keeps the Option A box path byte-for-byte unchanged (Phase 101 regression-lock contract).

**Decision:** Option B coexists with Option A (does not replace). Upstream ships both paths gated at `GLCanvas3D.cpp:2911`. The mesh is `std::optional` and resets to nullopt on `clear()` (`Print.hpp:776`).

**Upstream anchors:** `Print.hpp:745-746,766,776`; `3DScene.cpp:887-925`; `TriangleMesh.hpp:122,136` (`merge()` + `convex_hull_3d()`).

### WS3 — CLI Fixtures + argv GUI Fixture Loading

| Integration Point | File:Line | New/Mod |
|---|---|---|
| QCommandLineParser options (add `--slice`, `--gizmo <name>`, `--plate <idx>`, `--filament-map-mode <m>`) | `src/qml_gui/main_qml.cpp:177-196` (existing 4 options untouched) | **Modified** |
| StartupOpenRequest struct (add `shouldSlice`, `gizmo`, `plateIndex`, `filamentMapMode`) | `src/qml_gui/main_qml.cpp:199-204` | **Modified** |
| applyStartupOpenRequests dispatch | `src/qml_gui/main_qml.cpp:207-257` (same `QTimer::singleShot(0, &backend, ...)` pattern) | **Modified** |
| BackendContext forwarders (`requestSliceForPlate(idx)`, `activateGizmo(name)`) | `src/qml_gui/BackendContext.h` (composition root) | **Modified** |
| Fixture-to-backend transport | `src/qml_gui/main_qml.cpp:349` (`applyStartupOpenRequests(startupOpenRequest, backend)`) | **Already exists** — fixtures ARE the deep-link path |

**Critical:** the argv plumbing ALREADY EXISTS (shipped v4.1/v4.2). WS3 is fixtures + recipes + multi-material fixture model + readiness gate, NOT new flags or a new transport layer.

### WS4 — D3D12 Crash Root-Cause + Backend Readiness

| Integration Point | File:Line | New/Mod |
|---|---|---|
| Debug-layer enable (env-gated, NOT in Release) | `src/qml_gui/Renderer/RhiBackendSelector.cpp:22` (`QRhiD3D12InitParams d3d12Params;`) | **Modified** |
| Crash investigation (probe needs to actually run a minimal render — current probe only creates QRhi) | `src/qml_gui/Renderer/RhiBackendSelector.cpp:63-87` (`probeBackend`) | **Modified** |
| Backend selection policy (ONLY if root-cause succeeds) | `src/qml_gui/Renderer/RhiBackendSelector.cpp:37-46` (`defaultWindowsCandidates` — D3D11-first today) | **Modified (conditional)** |
| Graphics API set | `src/qml_gui/main_qml.cpp:267-268` (`QQuickWindow::setGraphicsApi`) | **Unchanged** unless promotion lands |
| Vulkan readiness | n/a | **Evaluation only** — SDK-blocked, no code change |

**Debug-layer enable goes in `RhiBackendSelector`, not `main_qml`.** The `QRhiD3D12InitParams` instance lives in `RhiProbeOwner` at `RhiBackendSelector.cpp:19-24` — the single point before `QRhi::create(D3D12, params)` at `:80`.

### WS5 — Full GLGizmoMeasure Engine + AssembleViewDataPool Clipper

| Integration Point | File:Line | New/Mod |
|---|---|---|
| **Per-volume ITS accessor (CROSS-WORKSTREAM — unblocks raycaster AND clipper)** | `src/core/services/ProjectServiceMock.h:106-108` (`cloneCurrentPlateModel`/`rawModel`) — current `meshData()` (`:269`) is per-OBJECT-flattened, NOT per-volume | **New** |
| MeshRaycaster port (pure CPU) | `src/core/rendering/MeshRaycaster.{h,cpp}` | **New** (port of `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp`) |
| SceneRaycaster port (aggregates MeshRaycaster) | `src/core/rendering/SceneRaycaster.{h,cpp}` | **New** (port of `SceneRaycaster.hpp`, 126 lines) |
| `Measure::Measuring` consumption (instantiate, don't reimplement) | call site in EditorViewModel or new MeasureService | **New** (`#include <libslic3r/Measure.hpp>`) |
| Gizmo integration | `src/qml_gui/Renderer/RhiViewport.h:88` (`GizmoMeasure = 3`) + `:109` (`GizmoAssemblyMeasure = 19`) | **Unchanged** — slots ALREADY exist (Phase 92). The engine behind them is what's missing |
| AssembleViewDataPool clipper | `src/core/rendering/AssembleViewDataPool.h` (`ModelObjectsClipper = 1 << 4`) — slot reserved, resource NOT registered | **Modified** (register the deferred resource) |
| Picking hook (route through SceneRaycaster when gizmo active) | `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (picking path) | **Modified** |

**Critical:** the per-volume ITS accessor must land BEFORE the raycaster port. `SceneRaycaster::m_volumes` needs one raycaster per volume; the current `meshData()` flattens all volumes into one buffer. The new accessor unblocks both WS5's raycaster AND the AssembleViewDataPool clipper (same per-volume ITS dependency).

**Upstream anchors:** `SceneRaycaster.hpp` (port target); `MeshUtils.hpp:178,208,237,241` (`MeshRaycaster::unproject_on_mesh`/`closest_hit`); `Measure.hpp:119-140` (`Measuring` — instantiate); `GLGizmoMeasure.hpp:79,159,193` (`PickRaycaster = SceneRaycasterItem`, `m_mesh_measure_map`, `m_mesh_raycaster_map`).

---

## Top Pitfalls (the 8 critical ones)

Each pitfall: what goes wrong → how to avoid → phase to address. Full detail in PITFALLS.md.

### Pitfall 1 — Filament-map readback forgets the capture-by-value invariant (re-creates v4.4 `Print*` lifetime hazard)

**What:** Implementer stashes `Print*`/`std::vector<int>*` on SliceService/EditorViewModel for "lazy" GUI-thread reads. The pointer dangles because `activePrint_.store(nullptr)` is immediately followed by `print` going out of scope (`SliceService.cpp:664-665`). Use-after-free that may not crash immediately but corrupts adjacent state.

**Prevention:** Mirror Phase 100 exactly. Declare a `FilamentMapResult` POD in `SliceService.h` next to `WipeTowerGeometry:42`. Capture by value between `print.process()` (`:590`) and `activePrint_.store(nullptr)` (`:664`). Deliver via queued `filamentMapReady` signal (mirrors `wipeTowerGeometryReady` at `:805`). No `Print*` may escape. Smoke-test pattern at `tests/ViewModelSmokeTests.cpp:3949-4031` ports directly.

**Phase route:** Filament-map readback phase (analog of v4.4 Phase 100 — likely first/second phase of WS1). Phase 100 SUMMARY's `key-decisions` and `patterns-established` sections are the template.

### Pitfall 2 — Widening the filament-map enum 2→4 silently breaks persisted 3MF files

**What:** Qt6 `ProjectServiceMock.cpp:4997` writes `filamentMapMode()` via raw `opt->setInt(mode)`, and `:5413-5417` reads it back. If old-value `1` (Manual under the 2-value convention) is re-read under the new 4-value convention (`1=fmmAutoForMatch`), every pre-v4.5 "Manual" plate silently becomes "AutoForMatch" on reload. This is data corruption that only surfaces on reload. (Upstream `bbs_3mf.cpp:7964-7967` uses string-keyed `ConfigOptionEnum` — safe; the Qt6 `setInt` path is the hazard.)

**Prevention:** Treat the enum as a v3.2→v4.5 data-migration problem, not a code-rename. Add read-time normalization: when loading a 3MF written under the 2-value convention, map legacy `1` (Manual) → `2` (fmmManual). Audit ALL writers/readers: `grep -rn "filament_map_mode\|filamentMapMode\|setFilamentMapMode\|m_filament_map_mode"`. Add a round-trip test modeled on `thumbnailSaveReloadRoundTrip`.

**Phase route:** Enum-widening phase (FIRST phase of WS1, BEFORE auto-computation readback — so auto-mode code doesn't entrench the new convention before legacy files are migrated).

### Pitfall 3 — Option B ITS regression: replacing Option A box breaks the v4.4-frozen contract

**What:** Temptation to modify `buildWipeTowerVertices` (`GizmoGeometry.cpp:487-535`) to emit the real mesh, or modify `uploadWipeTowerBuffer` (`:1064-1097`) to take an ITS. Both risk regressing the v4.4-frozen Option A baseline that Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent` guards. Concrete regressions: vertex format mismatch with SoftwareViewport; buffer-size miscalc (forgetting the index buffer); capture-by-value violation (mesh must be deep-copied, not referenced).

**Prevention:** Keep `buildWipeTowerVertices` UNTOUCHED. Add NEW `buildWipeTowerMeshVertices(const QVector<GizmoVertex>& capturedVerts)`. Add NEW `uploadWipeTowerMeshBuffer` (do not overload). Gate on `m_useWipeTowerMesh` set when `wipe_tower_mesh_data.has_value()` AND feature flag on. Default OFF so v4.4 baseline is bit-identical until explicitly enabled. Deep-copy the mesh in the worker (`TriangleMesh` is copyable). Extend Phase 102 test to assert Option A 36-vertex path STILL fires when `wipe_tower_mesh_data == nullopt`.

**Phase route:** Option B render phase (analog of v4.4 Phase 101). Option A fallback preservation ships in the SAME phase as Option B path — they must ship together so the fallback is tested, not retrofitted.

### Pitfall 4 — argv fixture loading fires before QML engine / BackendContext is ready (race condition)

**What:** Three races: (1) `QQmlApplicationEngine::load` is async-ish; `singleShot(0)` may fire BEFORE `Component.onCompleted` — first frame renders empty, model pops in on frame 2 (flake for screenshots); (2) RHI backend probe runs at `main_qml.cpp:265` before `QGuiApplication` — D3D12 crash fixtures need the failure to propagate, but current `probeBackend` falls through to software; (3) BackendContext services may have lazy init — `topbarImportModel` no-ops or crashes on null `Model*`.

**Prevention:** Add a readiness gate: defer fixture handling until BOTH `QQmlApplicationEngine::objectCreated` AND first `QQuickWindow::frameSwapped` have fired. For D3D12 crash fixtures, do NOT use `singleShot(0)` — let the renderer attempt the pass and capture the minidump (`CrashHandlerWin::install` at `main_qml.cpp:295` already routes to `crash_dumps/`). Document event-loop ordering: `QGuiApplication` → `selectRhiBackendFromEnvironment` → `BackendContext` ctor → `QQmlApplicationEngine::load` → `objectCreated` → first `frameSwapped` → fixture actions.

**Phase route:** CLI fixtures phase (WS3). Readiness gate is the FIRST deliverable — every subsequent fixture depends on it. The D3D12 crash-repro sub-fixture belongs in WS4 but uses the fixture infrastructure, so WS3 must ship first OR the two phases coordinate on the gate contract.

### Pitfall 5 — D3D12 debug layer enabled in Release — leaks GPU validation overhead into production

**What:** Debug layer (`QRhiD3D12InitParams::enableDebugLayer`) gated only on `QRhi::D3D12` selection ships in Release and causes: (1) 20-40% GPU perf regression for `OWZX_RHI_RENDERER=d3d12` users; (2) spurious validation warnings masking real errors; (3) false "D3D12 works" signal (debug layer sometimes papers over undefined behavior the release layer crashes on).

**Prevention:** Gate any debug-layer enable behind `#ifndef QT_NO_DEBUG` (or CMake `OWZX_D3D12_DEBUG` defaulting OFF). The `RhiBackendSelector.cpp:22 QRhiD3D12InitParams d3d12Params;` is the seam. Treat the BUG-V31-1 comment (`RhiViewportRenderer.cpp:283-285`) as a LEADING HYPOTHESIS, not confirmed root cause — the fix at `:286-296` already merged the camera-uniform upload into a pre-`beginPass` batch, so a NEW crash needs a NEW root cause (descriptor-heap exhaustion, root-signature mismatch, 256-byte cbuffer alignment at `:1424-1428`). Add runtime gate: if `OWZX_RHI_RENDERER=d3d12` AND Release AND no `OWZX_D3D12_DEBUG`, log that crashes will not be annotated.

**Phase route:** D3D12 investigation phase (WS4). Split into: (a) root-cause isolation (debug layer ON, Debug-build-gated, reproduce + minidump analysis); (b) backend-promotion evaluation (decide opt-in vs default). Release-build leak prevention is a hard gate in (b) — do not promote without proving the debug layer is off in Release.

### Pitfall 6 — GLGizmoMeasure ITS exposure: `mesh() const` reference detaches on copy, breaking the `Measuring` lifetime contract

**What:** Qt6 has ZERO per-volume ITS exposure today (`AssemblyMeasureGeometry.cpp:23,43` explicitly says "without ITS raycasting"). The port requires introducing an ITS cache layer with undefined lifetime rules. Two bugs fire: (1) detach-on-copy — if `mesh()` returns `const TriangleMesh&` and the caller writes `Measure::Measuring m(volume.mesh().its);`, any detach dangles the `its` reference; `Measuring`'s pimpl likely retains it; (2) rebuild timing — `Measuring` precomputes features in its ctor; if the volume is transformed and `Measuring` is NOT rebuilt, `get_feature()` returns features in the OLD local frame. Additionally, `SurfaceFeature`'s copy ctor (`Measure.hpp:36-43`) shallow-copies `volume` (`void*`!) and `plane_indices` (`vector<int>*`!) — copying across the boundary without scrubbing is latent UAF.

**Prevention:** Define an explicit ITS ownership contract BEFORE writing the engine. Recommended: per-volume `std::shared_ptr<indexed_triangle_set>` held by EditorViewModel (or a `VolumeMeshCache` service), updated when the volume's mesh changes. `Measuring` constructed from the shared_ptr's `*`, documented to live no longer than the shared_ptr. The `SurfaceFeature::volume void*` must be set to the Qt-side volume index (cast to `void*`), NOT a libslic3r `ModelVolume*`. Gate `Measuring` reconstruction on a mesh-changed signal; transient transforms (move/rotate/scale without apply) apply inverse world transform to the hit point (mirrors `GLGizmoMeasure.cpp:677-702`), NOT a rebuild.

**Phase route:** GLGizmoMeasure engine phase (WS5). ITS ownership contract is the FIRST deliverable (a `VolumeMeshCache` design doc or header), before any feature-picking code. The AssembleViewDataPool `ModelObjectsClipper` shares the same ITS dependency and must be in the same phase or a paired phase.

### Pitfall 7 — Raycaster performance: per-frame ITS raycasts melt the frame rate at >5 volumes

**What:** Upstream `GLGizmoMeasure.cpp:600-615` loops `m_mesh_raycaster_map` and calls `unproject_on_mesh(...)` on EVERY volume PER MOUSE MOVE. Tolerable upstream because `MeshRaycaster` uses a BVH built once. If the Qt6 port builds the BVH per-frame (reconstructing `Measuring`, or no BVH cache), frame time goes ~5ms → ~50ms+ with 5+ objects. Symptom: sluggish cursor, frozen UI on complex scenes.

**Prevention:** Two-stage picking MANDATORY: (1) `ObjectPicking::pickSourceObject` (`src/core/rendering/ObjectPicking.cpp:109`) — ray-AABB, cheap, returns volume index; (2) per-triangle ITS raycast ONLY on the picked volume's cached `Measuring`. Bounds per-frame cost to one BVH traversal, not N. Cache `Measuring` per volume, keyed by volume index + mesh revision. Rebuild only on mesh change (Pitfall 6 signal), not cursor move. Add a frame-time regression test: N=10 objects at a fixed cursor position, assert frame time < 16ms.

**Phase route:** GLGizmoMeasure engine phase (WS5), as a performance acceptance criterion. Two-stage-pick design in the engine's first design doc; frame-time regression test in the verification sub-phase (analog of v4.4 Phase 102).

### Pitfall 8 — FilamentGroupPopup ships as QML-only state with no backing Q_INVOKABLE

**What:** Current `FilamentGroupPopup` in `BBLTopbar.qml:396-397` is a documented visual placeholder ("仅视觉，无 Q_INVOKABLE 调用，无状态"). Temptation to add QML-only state (`property var filamentMap`) drives assignment from QML — violates AGENTS.md (business rules in C++, QML is presentation). Popup then: diverges from `PartPlate::setFilamentMaps` (QML state drifts from C++ `m_filament_maps`, 3MF write path persists C++ state, silently loses QML edits); has no validation (upstream enforces physical-unprintable constraints `Print.cpp:3061`); cannot be unit-tested.

**Prevention:** Define the popup contract on EditorViewModel: `Q_INVOKABLE QList<int> plateFilamentMaps(int)`, `Q_INVOKABLE bool setPlateFilamentMaps(int, const QList<int>&)`, `Q_INVOKABLE int plateFilamentMapMode(int)`, `Q_INVOKABLE bool setPlateFilamentMapMode(int, int)`. Delegates to ProjectServiceMock which owns `PartPlate::setFilamentMaps/setFilamentMapMode`. Popup QML reads via viewmodel Q_PROPERTYs/Q_INVOKABLE and writes via setter. No `property var` in QML. Expose auto-recommended map as separate Q_PROPERTY `recommendedFilamentMaps` driven by `filamentMapReady` (Pitfall 1), with explicit "Apply" button. Add ViewModelSmokeTests case modeled on `wipeTowerGeometryReadbackAppliesValidAndInvalidGate`.

**Phase route:** Filament-map UI phase (second half of WS1, AFTER readback + enum-widening phases). The viewmodel API must be designed BEFORE the QML popup is built — same ordering rule that governed v4.1 settings restoration.

---

## Suggested Build Order (dependency-aware phase sequencing for the roadmapper)

Phases start at 103 per PROJECT.md. The sequencing exploits two cross-workstream dependencies: (a) the per-volume ITS accessor unblocks WS5 raycaster + clipper together; (b) the argv fixture readiness gate is shared between WS3 and WS4's crash-repro.

```
Phase A (parallel, no deps):
  WS3 CLI fixtures + readiness gate
    — unblocks evidence capture for all other WS (highest ROI first)
    — readiness gate (objectCreated + frameSwapped) is the FIRST deliverable
  WS4 D3D12 debug-layer wiring + crash isolation (time-boxed)
    — investigation, no other WS depends on the outcome
    — can run independently so it does NOT gate deliverable WS

Phase B (parallel after WS3 readiness gate, no inter-deps):
  WS1 Auto filament-map readback (libslic3r already auto-computes; readback + signal only)
    — mirror Phase 100 WTREAD capture-by-value
  WS2 Option B wipe-tower mesh readback (extends v4.4 WTREAD; re-opens Phase 99 FD 2)
    — capture wipe_tower_mesh_data + convex_hull_3d in worker

Phase C (after WS1+WS2 readback — needs worker-capture + evidence):
  WS1 UI: enum widening (PartPlate.h, ProjectServiceMock, EditorViewModel, 3MF migration)
    + FilamentGroupPopup.qml
  WS2 mesh renderer: GizmoGeometry::buildWipeTowerMeshVertices
    + uploadWipeTowerBuffer branch (Option A fallback preserved)

Phase D (independent track, after WS3 only):
  WS5 step 1: per-volume ITS accessor on ProjectServiceMock
    — CROSS-WORKSTREAM: unblocks BOTH raycaster + AssembleViewDataPool clipper
  WS5 step 2: MeshRaycaster port (pure CPU, unit-testable)
  WS5 step 3: SceneRaycaster port (aggregates MeshRaycaster)
  WS5 step 4: Measure::Measuring consumption + EditorViewModel measurement Q_PROPERTYs
    — ship measure readouts (distance/angle/XYZ) first
  WS5 step 5: AssembleViewDataPool ModelObjectsClipper registration (bit 4)
    — same ITS cache as step 1; paired phase or shared design doc
```

### Phase Ordering Rationale

- **WS3 first** because the Windows-capture-API blocker has recurred across v3.8-v4.4; deterministic argv fixtures unblock evidence for every other WS. WS3 is also the lowest-risk (purely additive argv options + recipes). Its readiness gate (`objectCreated` + `frameSwapped`) is the SHARED contract WS4 needs for crash-repro.
- **WS4 in parallel with WS3** because no other WS blocks on D3D12 (all run on D3D11 today). It is investigation-heavy and may not produce a clean feature — schedule it so it does NOT gate the deliverable WS. Time-box it.
- **WS1 readback before WS1 UI** because the slot contract (POD shape, signal) must exist before the popup can bind. Mirrors Phase 100 (readback) → Phase 101 (rendering) ordering.
- **WS2 after WS1 readback** because both read from the same worker window; landing WS1 first establishes the second capture-by-value precedent, reducing WS2 risk. WS2's ITS extension to `WipeTowerGeometry` then looks like a known pattern.
- **WS1 UI + WS2 mesh in Phase C (parallel)** because both consume the Phase B readback payloads and don't depend on each other.
- **WS5 step-1 (per-volume ITS) before everything else in WS5** because both the raycaster and the AssembleViewDataPool clipper share that exact dependency (documented in `AssembleViewDataPool.h` bit-4 deferral note). Building it once serves both.
- **WS5 steps 2-5 sequential** because each builds on the previous (raycaster → aggregator → Measuring → clipper). Ship measure readouts (step 4) before deferring Assembly transformation actions to v4.6+.

### Research Flags

Phases likely needing deeper research during planning:
- **WS4 D3D12 root cause:** investigation-heavy; may need PIX capture + debug-layer triage. The BUG-V31-1 comment is a leading hypothesis, not confirmed — the fix at `:286-296` is already in place, so a new crash needs a new root cause.
- **WS5 step-1 per-volume ITS ownership contract:** the `shared_ptr<indexed_triangle_set>` cache design + mesh-changed signal wiring needs a design doc BEFORE feature-picking code (Pitfall 6).
- **WS2 re-opens Phase 99 Frozen Decision 2:** the `wipe_tower_mesh_data` capture design (not just dims) must be re-audited against the v4.4 frozen decision.

Phases with standard patterns (skip deep research):
- **WS1 readback + UI:** direct mirror of v4.4 Phase 100/101 WTREAD pattern.
- **WS3 fixtures:** argv plumbing already exists; additive options + recipes only.
- **WS5 steps 2-5:** ports of upstream `MeshRaycaster`/`SceneRaycaster`/`Measuring` with the project's established `GizmoGeometry`/`GizmoMath`/`ObjectPicking` pure-CPU-testable pattern.

---

## Anti-Features (consolidated list)

The roadmapper and plan-phases MUST treat these as explicit non-goals. Each would either corrupt source-truth parity, regress a frozen baseline, or invent product behavior upstream doesn't have.

1. **`fmmDefault` as a 4th user-visible radio button in FilamentGroupPopup** (WS1). Upstream keeps it out of `mode_list` (`FilamentGroupPopup.hpp:52`) — it is a per-plate "inherit from global" resolution sentinel, not a user-selectable mode. The popup has exactly 3 radio buttons (AutoForFlush / AutoForMatch / Manual).

2. **Option B trying to render per-extruder color slabs AND the real mesh** (WS2). Upstream's `load_real_wipe_tower_preview` deliberately drops per-extruder slabs (`model_per_colors.resize(1)` at `:911`) because real mesh geometry is incompatible with slab-splitting. Upstream chose silhouette over stripes for the real path.

3. **argv fixtures shipped as a user-facing "deep link" product feature** (WS3). Upstream OrcaSlicer has NO `--open-page`/`--open-dialog`/`--load-model` (its CLI is `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`). These are test-evidence plumbing for the recurring Windows-capture-API blocker, not product capability. Do not expose in user-facing help/docs.

4. **D3D12 default promotion before the crash is root-caused** (WS4). PROJECT.md Out-of-Scope forbids this explicitly. Also: treating "D3D12 doesn't crash on my machine" as success without understanding WHY (environment-sensitive, could regress); attempting to unblock Vulkan by recompiling Qt (SDK-level block, out of scope); enabling the D3D12 debug layer in Release builds (20-40% perf regression + false "fixed" signal).

5. **Porting Assembly-mode transformation actions (Parallel/Coincidence/etc.) before the basic GLGizmoMeasure engine works** (WS5). Those depend on full transformation/undo-redo integration that is scope-creep for the measure engine. Ship measure readouts (distance/angle/XYZ) first in v4.5; defer Assembly actions to v4.6+.

6. **Replacing `buildWipeTowerVertices` / `uploadWipeTowerBuffer` instead of branching them** (WS2). Option A is the v4.4-frozen fallback for when `wipe_tower_mesh_data == nullopt` (single-material / cleared). Add parallel functions; do not modify the frozen baseline.

7. **Driving FilamentGroupPopup from QML-local `property var` state** (WS1). Violates AGENTS.md (business rules in C++, QML is presentation). State would diverge from `PartPlate::m_filament_maps`; 3MF persistence would lose edits. ViewModel Q_INVOKABLE API is mandatory.

8. **Reimplementing `Measure::Measuring` math instead of instantiating it** (WS5). `Measure.hpp:119` ships a complete `Measuring` class with `MeasuringImpl` pimpl. AGENTS.md: "instantiate, don't reimplement" libslic3r. The Qt6 work is wiring (raycaster → volume → `Measuring` instance → result struct → Q_PROPERTY), not geometry math.

9. **Stashing `Print*` / `WipeTowerData*` / `TriangleMesh*` for GUI-thread reads** (WS1/WS2). Re-creates v4.4 Frozen-Decision-1 hazard. The `Print` is destroyed when the worker lambda exits. Capture by value into POD structs.

10. **Porting the upstream `m_mesh_raycaster_map` per-volume-per-mouse-move loop verbatim** (WS5). Qt6 has no per-volume raycaster cache; cost multiplies by volume count. Two-stage picking (ray-AABB coarse then per-triangle on hit volume only) is mandatory.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | **HIGH** | All 5 workstreams verified to need zero new external libraries; all integration points cited at file:line in current `src/` + upstream `third_party/OrcaSlicer`. Qt6.10 + libslic3r v7.0.1 APIs confirmed. Vulkan SDK block confirmed in `Qt6GuiTargets.cmake` |
| Features | **HIGH** for WS1/2/3/5; **MEDIUM** for WS4 | WS1/2/3/5 cite upstream file:line for every behavior. WS4 (D3D12) crash is reproducible and characterized, but root cause is investigation output not pre-known |
| Architecture | **HIGH** | All integration points read from current source with file:line anchors. The v4.4 WTREAD pattern (the backbone for WS1+WS2) is proven. The per-volume ITS accessor dependency (WS5) is confirmed against current `meshData()` per-object-flattening behavior |
| Pitfalls | **HIGH** | All 8 pitfalls rooted in v4.4 Phase 100/101 evidence, v4.3 Phase 97 evidence, upstream `Print.cpp:2484-2491` / `3DScene.cpp:887-925` anchors, and existing Qt6 RHI/SliceService code |

**Overall confidence:** **HIGH** (WS4 root cause is the only MEDIUM area, and it is correctly scoped as time-boxed investigation)

### Gaps to Address

- **WS4 D3D12 root cause (OPEN):** the crash is characterized (`0xc0000005` at startup with `OWZX_RHI_RENDERER=d3d12`) but the root cause is unknown. BUG-V31-1 comment is a leading hypothesis, not confirmed — the fix at `RhiViewportRenderer.cpp:286-296` is already in place, so a new crash needs a new root cause (descriptor-heap exhaustion, root-signature mismatch, 256-byte cbuffer alignment at `:1424-1428`). Handle by time-boxing WS4 and producing incremental value (debug-layer wiring → crash signature → root-cause hypothesis → fix attempt → stability test), not committing to a fixed feature deliverable up front.
- **WS5 step-1 per-volume ITS ownership contract (DESIGN NEEDED):** the `shared_ptr<indexed_triangle_set>` cache + mesh-changed signal design must be the FIRST deliverable of WS5, before any feature-picking code. Handle by requiring a `VolumeMeshCache` design doc or header as the first plan of the WS5 phase.
- **WS2 re-opens Phase 99 Frozen Decision 2 (RE-AUDIT NEEDED):** the `wipe_tower_mesh_data` capture design (not just dims) must be re-audited against the v4.4 frozen decision. Handle by citing the re-opening in the WS2 phase PLAN.md and producing a new audit matrix.

---

## Sources

### Primary (HIGH confidence — codebase + upstream source truth)
- `E:/ai/3DPrinter_Qt6/.planning/research/STACK.md` — stack verdict per workstream (zero new libs)
- `E:/ai/3DPrinter_Qt6/.planning/research/FEATURES.md` — feature decisions per workstream
- `E:/ai/3DPrinter_Qt6/.planning/research/ARCHITECTURE.md` — integration points + patterns
- `E:/ai/3DPrinter_Qt6/.planning/research/PITFALLS.md` — the 8 critical pitfalls
- `E:/ai/3DPrinter_Qt6/.planning/PROJECT.md` — project context, constraints, validated/active/future scope
- `E:/ai/3DPrinter_Qt6/.planning/MILESTONES.md` — v4.4 (just closed) + prior milestone history
- `src/core/services/SliceService.cpp:490-665` — the slice worker (WS1+WS2 capture window)
- `src/core/services/SliceService.h:42-62,167` — `WipeTowerGeometry` POD + `wipeTowerGeometryReady` signal (WS1 model + WS2 extension target)
- `src/core/model/PartPlate.h:202-203,268-273` — 2-value filament map mode (WS1 widening surface)
- `src/core/services/ProjectServiceMock.cpp:610-627,4991-4997,5409-5420` — filament-map API + 3MF write/read paths (WS1 migration hazard)
- `src/core/rendering/GizmoGeometry.cpp:449-535` — `buildWipeTowerVertices` Option A box (WS2 sibling target)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1097,281-298,1424-1428` — wipe-tower upload + BUG-V31-1 + 256-byte alignment (WS2 + WS4)
- `src/qml_gui/main_qml.cpp:170-257,265-268,295,307,349` — QCommandLineParser + applyStartupOpenRequests + CrashHandlerWin + BackendContext (WS3)
- `src/qml_gui/Renderer/RhiBackendSelector.cpp:19-24,37-46,63-87,114` — D3D12 probe + candidate order + `QRhiD3D12InitParams` (WS4)
- `src/qml_gui/Renderer/RhiViewport.h:84-110` — GizmoMode enum incl. `GizmoMeasure=3`, `GizmoAssemblyMeasure=19` (WS5 slots ALREADY exist)
- `src/core/rendering/AssembleViewDataPool.h` — bit-4 `ModelObjectsClipper` reservation (WS5 clipper target)
- `src/core/rendering/AssemblyMeasureGeometry.h:18-33` — AABB measure stub (WS5 gap evidence)
- `src/qml_gui/BBLTopbar.qml:396-397` — FilamentGroupPopup placeholder TODO (WS1)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-790,996-997` — `WipeTowerData`/`WipeTowerMeshData` + `get_filament_maps`/`get_filament_map_mode`
- `third_party/OrcaSlicer/src/libslic3r/Print.cpp:2484-2493,5070-5094` — auto filament-map compute + `construct_mesh`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424-429` — 4-value `FilamentMapMode` enum
- `third_party/OrcaSlicer/src/libslic3r/GCode/ToolOrdering.hpp:244` + `.cpp:1107-1214` — `get_recommended_filament_maps`
- `third_party/OrcaSlicer/src/libslic3r/TriangleMesh.hpp:122,136` — `merge()` + `convex_hull_3d()`
- `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:16-238` — `Measuring` + `MeasurementResult` + `SurfaceFeature` (with raw `void*`/`vector*` hazards)
- `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:840-925` — `load_wipe_tower_preview` (box) + `load_real_wipe_tower_preview` (mesh, hull at `:914`)
- `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp` — WS5 port target (126 lines)
- `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp:178,208,237,241` — `MeshRaycaster` WS5 port target
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.hpp:79,159,193` — `PickRaycaster`, `m_mesh_measure_map`, `m_mesh_raycaster_map`
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:317-328` — `get_real_filament_map_mode` (`fmmDefault` resolution)
- `third_party/OrcaSlicer/src/slic3r/GUI/FilamentGroupPopup.hpp:21-80,52` + `.cpp:16-47,89-208,242-312` — popup structure + 3-mode list + connection-gating
- `third_party/OrcaSlicer/src/OrcaSlicer.cpp:7122-7216` — upstream CLI (NO GUI deep-link flags — WS3 framing)

### Secondary (MEDIUM confidence — Qt docs confirmation)
- [QCommandLineParser Class — Qt 6.8 docs](https://doc.qt.io/qt-6.8/zh/qcommandlineparser.html) — stable API across Qt 6.x/6.10 (already proven in `main_qml.cpp:201-202`)

### Tertiary (LOW confidence — needs validation during implementation)
- `.planning/debug/qrhi-d3d12-crash.md` — D3D12 crash characterization (`0xc0000005`); root cause is investigation output

---
*Research completed: 2026-07-12*
*Ready for roadmap: yes*
