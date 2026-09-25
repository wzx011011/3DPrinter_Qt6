# Architecture Research

**Domain:** Qt6/QML slicer — v4.5 backlog-closure workstreams (filament-map, wipe-tower Option B mesh, CLI fixtures, D3D12, GLGizmoMeasure)
**Researched:** 2026-07-12
**Confidence:** HIGH (all integration points read from current source with file:line anchors; upstream enums/APIs verified against `third_party/OrcaSlicer`)

## Standard Architecture

### System Overview

The v4.5 workstreams all plug into the same three-layer pipeline established by v3.8-v4.4. No new layers are introduced. The diagram below shows where each workstream lands.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              QML Presentation                                │
│  PreparePage.qml · AssemblePage.qml · FilamentGroupPopup (WS1, NEW)          │
│  GLViewport bindings (id viewport3d) ← editorVm Q_PROPERTYs                  │
│  WS3 argv fixtures drive this layer via QCommandLineParser → BackendContext  │
├──────────────────────────────────────────────────────────────────────────────┤
│                              ViewModels (core/)                              │
│  EditorViewModel · PreviewViewModel · ConfigViewModel                        │
│  WS1: onFilamentMapReady slot + filamentMap* Q_PROPERTYs (mirrors WTREAD)    │
│  WS5: GLGizmoMeasure activation state + picking result Q_PROPERTYs           │
│  EditorViewModel receives services via constructor pointers (never owns)     │
├──────────────────────────────────────────────────────────────────────────────┤
│                              Services (core/)                                │
│  SliceService (slice worker, readback point) · ProjectServiceMock (Model*)   │
│  WS1: filament-map readback inside worker (mirrors wipe-tower capture)       │
│  WS2: wipe_tower_mesh_data readback inside worker (extends WTREAD capture)   │
│  WS5: per-volume ITS accessor on ProjectServiceMock (NEW)                    │
├──────────────────────────────────────────────────────────────────────────────┤
│                       Rendering (qml_gui/Renderer + core/rendering)          │
│  RhiViewport → RhiViewportRenderer (D3D11 default) · SoftwareViewport        │
│  WS2: GizmoGeometry::buildWipeTowerMeshVertices (NEW) + uploadWipeTowerBuffer │
│  WS4: RhiBackendSelector D3D12 debug-layer + crash isolation                 │
│  WS5: SceneRaycaster port (NEW, core/rendering) + Measure::Measuring consume │
│  GizmoGeometry / AssemblyMeasureGeometry / AssembleViewDataPool (pure-CPU)   │
├──────────────────────────────────────────────────────────────────────────────┤
│                       Entry Point (qml_gui/main_qml.cpp)                     │
│  QGuiApplication → RhiBackendSelector → BackendContext (composition root)    │
│  WS3: QCommandLineParser extensions + applyStartupOpenRequests → backend     │
│  QCommandLineParser · CrashHandlerWin · qmlRegisterType<RhiViewport>         │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | v4.5 Workstream Touch |
|-----------|----------------|------------------------|
| `BackendContext` | Composition root: owns all services/viewmodels, exposes to QML as `backend`, dispatches startup deep links | WS3 (applyStartupOpenRequests consumer), all WS receive services through it |
| `SliceService` | Slice engine bridge: runs libslic3r `Print` in a worker, captures results by value, emits on GUI thread | WS1 (filament-map readback), WS2 (mesh readback) — both mirror the Phase 100 wipe-tower capture pattern |
| `EditorViewModel` | Prepare-page state: gizmos, plates, wipe-tower, slice-result bridge | WS1 (onFilamentMapReady + Q_PROPERTYs), WS5 (GizmoMeasure activation) |
| `ProjectServiceMock` | Model/plate store, holds `Slic3r::Model*` under `#ifdef HAS_LIBSLIC3R`, exposes `plateListMut/plateListConst` test seam + filament-map API | WS1 (filament-map mode widening), WS5 (per-volume ITS accessor) |
| `RhiViewportRenderer` | D3D11 QRhi rendering of meshes, gizmos, wipe tower, picking | WS2 (uploadWipeTowerBuffer mesh path), WS5 (raycaster hook for picking) |
| `GizmoGeometry` | Pure-CPU gizmo/wipe-tower vertex builders, no GL/RHI | WS2 (new buildWipeTowerMeshVertices helper) |
| `RhiBackendSelector` | D3D11-first / D3D12-explicit-opt-in probe, sets `QQuickWindow::setGraphicsApi` | WS4 (D3D12 debug layer + crash root-cause surface) |
| `AssembleViewDataPool` | Caches per-object data for AssembleView gizmos; `ModelObjectsClipper` slot reserved at bit 4 | WS5 (register the deferred clipper resource) |

## Recommended Project Structure

```
src/
├── core/
│   ├── model/PartPlate.h          # WS1: m_filament_map_mode widen 2→4 (line 273)
│   ├── rendering/
│   │   ├── GizmoGeometry.{h,cpp}  # WS2: NEW buildWipeTowerMeshVertices + ITS→GizmoVertex adapter
│   │   ├── AssemblyMeasureGeometry.{h,cpp}  # WS5: existing AABB distance — keep as fallback
│   │   ├── AssembleViewDataPool.{h,cpp}     # WS5: register ModelObjectsClipper (bit 4)
│   │   ├── SceneRaycaster.{h,cpp}           # WS5: NEW port of upstream SceneRaycaster.hpp
│   │   └── MeshRaycaster.{h,cpp}            # WS5: NEW port of upstream MeshUtils.hpp MeshRaycaster
│   ├── services/
│   │   ├── SliceService.{h,cpp}   # WS1+WS2: extend worker capture + new signals
│   │   └── ProjectServiceMock.{h,cpp}  # WS1: filament-map mode API; WS5: per-volume ITS accessor
│   └── viewmodels/
│       └── EditorViewModel.{h,cpp} # WS1: onFilamentMapReady + Q_PROPERTYs; WS5: Measure state
├── qml_gui/
│   ├── main_qml.cpp               # WS3: QCommandLineParser extensions + routing
│   ├── BackendContext.{h,cpp}     # WS3: fixture-application entry; deep-link dispatch
│   ├── components/
│   │   └── FilamentGroupPopup.qml # WS1: NEW popup consuming filament-map Q_PROPERTYs
│   ├── pages/
│   │   ├── PreparePage.qml        # WS1: filament-map GLViewport/overlay bindings (~:1670 pattern)
│   │   └── AssemblePage.qml       # WS5: GizmoMeasure overlay/picking wiring
│   └── Renderer/
│       ├── RhiBackendSelector.{h,cpp}  # WS4: D3D12 enableDebugLayer + crash isolation
│       └── RhiViewportRenderer.{h,cpp} # WS2: uploadWipeTowerBuffer mesh branch; WS5: raycaster hook
└── tests/
    └── ViewModelSmokeTests.cpp    # all WS: QSignalSpy + QMetaObject::invokeMethod pattern (Phase 100 model)
```

### Structure Rationale

- **core/rendering stays pure-CPU + libslic3r-only** (no Qt6::Quick / qrhi.h), matching the GizmoVertex.h split established in v3.8. WS2's mesh adapter and WS5's raycaster live here so they remain unit-testable like `AssemblyMeasureGeometry`.
- **Per-volume ITS accessor lives on ProjectServiceMock** (not EditorViewModel) because the model is the source of truth and the accessor needs `#ifdef HAS_LIBSLIC3R` (mirrors the existing `cloneCurrentPlateModel()` / `rawModel()` pattern at `ProjectServiceMock.h:106-108`).
- **FilamentGroupPopup.qml is new under components/** because the existing `FilamentSlot.qml` is a single-slot widget, not a plate-level popup. The popup binds to EditorViewModel Q_PROPERTYs (no service access from QML — enforced by the AGENTS.md QML-boundaries rule).

## Architectural Patterns

### Pattern 1: Print-lifetime-bounded capture-by-value readback (the v4.4 WTREAD pattern — WS1 + WS2 both reuse it)

**What:** The `Print` object lives only inside the `SliceService` worker lambda (valid from `SliceService.cpp:514` `activePrint_.store(&print)` through `:664` `activePrint_.store(nullptr)`). Any readback must capture plain values into a worker-local POD struct before the lambda exits; no `Print*` may escape.

**When to use:** WS1 (filament map: `print.get_filament_maps()` / `get_filament_map_mode()` at upstream `Print.hpp:996-997`) and WS2 (wipe-tower mesh: `print.wipe_tower_data().wipe_tower_mesh_data` at upstream `Print.hpp:766`). Both read the same `Print` in the same window as Phase 100's wipe-tower dims (`SliceService.cpp:642-659`).

**Trade-offs:** Pros — deterministic, thread-safe, no dangling pointers, unit-testable via `QMetaObject::invokeMethod` + `qRegisterMetaType`. Cons — captures are copies; for WS2's mesh the copy is non-trivial (an `indexed_triangle_set`), so the POD struct must own its memory.

**Example (WS1, mirroring `SliceService.cpp:642-659` + `WipeTowerGeometry` at `SliceService.h:42`):**
```cpp
// SliceService.h — new POD struct alongside WipeTowerGeometry
struct FilamentMapResult {
  bool valid = false;
  int mode = 0;                       // widened: 0=fmmAutoForFlush, 1=fmmAutoForMatch,
                                      //          2=fmmManual, 3=fmmDefault (upstream enum order)
  QList<int> maps;                    // maps[i] = extruder for filament i (1-based, upstream)
  int plateIndex = -1;                // which plate this result is for
};
// SliceService.cpp worker — between print.process() and activePrint_.store(nullptr)
if (print.has_wipe_tower() || /* multi-extruder */) {
  const auto maps = print.get_filament_maps();      // upstream Print.hpp:996
  const auto mode = print.get_filament_map_mode();  // upstream Print.hpp:997
  capturedFilamentMap.valid = true;
  capturedFilamentMap.mode = static_cast<int>(mode); // FilamentMapMode → int
  capturedFilamentMap.maps = QList<int>(maps.begin(), maps.end());
}
```

### Pattern 2: Data-driven Q_PROPERTY gate (WTREAD-02 model — WS1 UI side)

**What:** Read-only Q_PROPERTYs on EditorViewModel, fed by a private slot connected via PMF, with a single NOTIFY signal. The slot enforces the validity gate (force-hide on invalid) and leaves dims untouched on the invalid path so stale-but-real values persist.

**When to use:** WS1 — `onFilamentMapReady(const FilamentMapResult &)` mirrors `onWipeTowerGeometryReady(const WipeTowerGeometry &)` exactly (slot declared in `private slots:` immediately before `signals:` — see the Phase 100 C2248 lesson in `100-01-SUMMARY.md`).

**Trade-offs:** Pros — QML never calls WRITE setters (data flows libslic3r → service → viewmodel → QML, matching how bed dims flow from project state). Cons — the slot block placement is load-bearing; misplacement silently re-classifies subsequent members private (Phase 100 build-fix `c45db36`).

### Pattern 3: Pure-CPU builder feeding dynamic-size RHI buffer rebuild (WS2)

**What:** `GizmoGeometry` builders emit `QVector<GizmoVertex>` (no RHI calls); `RhiViewportRenderer::uploadWipeTowerBuffer` owns the GPU upload and already does dynamic-size rebuilds via `ensureBuffer` + `m_wipeTowerDirty` (`RhiViewportRenderer.cpp:1082-1095`).

**When to use:** WS2 Option B — the new `buildWipeTowerMeshVertices(const indexed_triangle_set&)` returns a variable-size `QVector<GizmoVertex>` (the mesh can have hundreds of faces). The existing `uploadWipeTowerBuffer` already handles variable size: it computes `byteSize` from `vertices.size() * sizeof(Vertex)` and calls `ensureBuffer(..., byteSize, ...)` which reallocates when the size grows. **No structural change to `uploadWipeTowerBuffer` is required** — only the call site at `:1075-1079` switches from `buildWipeTowerVertices(box dims)` to `buildWipeTowerMeshVertices(its)` when the mesh is present.

**Trade-offs:** Pros — the buffer-rebuild infrastructure is reused verbatim. Cons — Option B coexists with Option A (the box path), gated on whether `wipe_tower_mesh_data` was `std::nullopt` (its `clear()` resets it to nullopt at upstream `Print.hpp:776`). The renderer needs a second branch, not a replacement.

### Pattern 4: argv fixture → QCommandLineParser → BackendContext deep-link (WS3)

**What:** `main_qml.cpp:170-205` (`parseStartupOpenRequest`) already defines `--open-page`, `--open-dialog`, `--load-model`, `--skip-first-run`. `applyStartupOpenRequests` (`main_qml.cpp:207-257`) dispatches them via `QTimer::singleShot(0, &backend, ...)` onto BackendContext methods (`backend.requestSelectTab`, `backend.topbarImportModel`).

**When to use:** WS3 — the existing options ARE the fixture integration point. WS3 extends them with: (a) `--slice` (auto-slice after load, for evidence capture without a click), (b) `--open-page prepare` + `--load-model <multi-material.3mf>` + `--slice` to deterministically produce a wipe-tower, (c) `--gizmo <name>` to pre-activate a gizmo (for GLGizmoMeasure evidence). Fixtures reach BackendContext through the same `applyStartupOpenRequests` path — no new transport layer.

**Trade-offs:** Pros — no new fixture framework; the deep-link machinery is production code, so fixtures exercise the real load/slice/render path. Cons — fixtures are argv-driven, so they cannot assert outcomes (they only set up state); assertions live in `tests/` QML-source-audit fallback tests (the Phase 101 `wipeTowerRealDimsReachRendererPipeline` pattern).

## Data Flow

### Request Flow: WS1 Auto Filament-Map (libslic3r → QML)

```
print.process() succeeds (SliceService.cpp:590)
    ↓
[INSIDE WORKER] print.get_filament_maps() + get_filament_map_mode()  (Print.hpp:996-997)
    ↓ capture by value into FilamentMapResult POD (mirrors WipeTowerGeometry capture)
activePrint_.store(nullptr) (:664) — Print invalidated, POD survives
    ↓
emit sliceFinished(...) + emit filamentMapReady(capturedFilamentMap) on success branch
    ↓ (queued connection, GUI thread)
EditorViewModel::onFilamentMapReady(slot, EditorViewModel.cpp:5095 pattern)
    ↓ applies validity gate, stores m_filamentMapMode/m_filamentMaps, emits filamentMapChanged
Q_PROPERTY filamentMapMode + filamentMaps NOTIFY filamentMapChanged
    ↓ (QML binding, PreparePage.qml ~:1670 pattern)
FilamentGroupPopup.qml reads editorVm.filamentMaps → renders recommended groups
```

### Request Flow: WS2 Option B Mesh (readback → RHI)

```
[INSIDE WORKER, same window as WS1]
print.wipe_tower_data().wipe_tower_mesh_data  (Print.hpp:766, std::optional)
    ↓ if has_value(): capture real_wipe_tower_mesh + real_brim_mesh ITS into WipeTowerGeometry
    ↓ (extend WipeTowerGeometry POD with QVector<GizmoVertex> meshVerts OR a captured ITS)
emit wipeTowerGeometryReady(geo) — same signal as v4.4 (SliceService.h:167)
    ↓
EditorViewModel::onWipeTowerGeometryReady → m_showWipeTower + dims + meshVerts
    ↓ (PreparePage.qml:1670-1675 GLViewport bindings — unchanged, plus new meshVerts binding)
RhiViewportRenderer::synchronize pulls Q_PROPERTYs (:171-189)
    ↓ m_wipeTowerDirty = true when mesh changes
uploadWipeTowerBuffer (:1064-1095)
    ↓ if meshVerts present: buildWipeTowerMeshVertices(its) ELSE buildWipeTowerVertices(box)
    ↓ ensureBuffer reallocates (dynamic-size path already correct)
renderWipeTower (:1894-1908) via m_translucentFillPipeline — unchanged
```

### Request Flow: WS5 GLGizmoMeasure (picking → measurement)

```
User click in RhiViewport → mouse coords
    ↓
SceneRaycaster::hit(mouse, camera) (NEW port, core/rendering)
    ↓ per-volume MeshRaycaster (built from per-volume ITS exposed by ProjectServiceMock)
HitResult { type=Volume, raycaster_id, position, normal }
    ↓
Measure::Measuring* get_measuring_of_mesh(volume) — instantiate, don't reimplement
    ↓ (upstream Measure.hpp:119 — explicit Measuring(const indexed_triangle_set&))
MeasurementResult { feature, ... } → EditorViewModel Q_PROPERTYs → AssemblePage.qml overlay
```

### State Management

- All mutable UI state stays in ViewModels as Q_PROPERTY members with explicit NOTIFY (AGENTS.md rule).
- Domain data (models, plates, volumes, filament maps) stays in `ProjectServiceMock` / `PartPlateList`; viewmodels read it via the injected pointer.
- The slice worker captures results by value into POD structs (`WipeTowerGeometry`, new `FilamentMapResult`) — no shared mutable state crosses the worker/GUI-thread boundary.

### Key Data Flows

1. **WS1 filament-map:** libslic3r `Print::get_filament_maps()` → `SliceService` worker capture → `filamentMapReady` signal → `EditorViewModel` slot → Q_PROPERTY → `FilamentGroupPopup.qml`. Exactly the Phase 100 WTREAD chain with a different payload.
2. **WS2 mesh:** same chain as v4.4 wipe-tower dims, but the payload is an ITS (variable-size) instead of 6 floats. The `uploadWipeTowerBuffer` dynamic-size path already handles it.
3. **WS3 fixtures:** argv → `QCommandLineParser` → `StartupOpenRequest` struct → `applyStartupOpenRequests` → BackendContext methods. Fixtures are production deep-link code, not a parallel test transport.
4. **WS4 D3D12:** `RhiBackendSelector::probeBackend` creates a `QRhiD3D12InitParams` (`RhiBackendSelector.cpp:22`) — the `enableDebugLayer` field on that struct is the debug-layer enable point. Crash root-cause needs a minimal D3D12 repro harness (separate from the app path).
5. **WS5 GLGizmoMeasure:** `ProjectServiceMock` per-volume ITS accessor (NEW) → `MeshRaycaster` per volume (NEW port) → `SceneRaycaster` aggregation (NEW port) → `Measure::Measuring` (upstream, instantiate) → EditorViewModel measurement Q_PROPERTYs.

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Single-plate, single-material | All five workstreams no-op gracefully (WS1 gate `valid=false`, WS2 `wipe_tower_mesh_data=nullopt` falls back to Option A box, WS5 needs >=2 volumes). No special-casing. |
| Multi-plate (typical, 4-16 plates) | WS1+WS2 must respect per-plate `Print` lifetime — the current `SliceService` worker slices one plate at a time (Phase 100 readback already per-plate-correct). WS3 `--slice` fixture should target a specific plate index. |
| Large meshes (WS5 raycaster) | Per-volume `MeshRaycaster` build cost is O(faces); build lazily on first gizmo activation, cache in a `std::map<volumeId, shared_ptr<MeshRaycaster>>` mirroring upstream `m_mesh_raycaster_map` (`GLGizmoMeasure.hpp:193`). |

### Scaling Priorities

1. **First bottleneck (WS2 mesh memory):** the WS2 `WipeTowerGeometry` POD currently holds 8 floats (32 bytes). Adding a captured ITS can add kilobytes-to-megabytes. Capture as a pre-flattened `QVector<GizmoVertex>` (post-`convex_hull_3d`, post-ITS→vertex conversion) inside the worker, so the GUI thread receives ready-to-upload vertices, not a raw ITS — keeps the cross-thread payload a single flat buffer.
2. **Second bottleneck (WS5 raycaster rebuild on scene edit):** invalidate the raycaster map on object add/remove/transform-end (mirror upstream `SceneRaycaster::remove_raycasters(EType::Volume)`), not on every mouse move.

## Anti-Patterns

### Anti-Pattern 1: Capturing `Print*` or `WipeTowerData*` for GUI-thread use (WS1/WS2)

**What people do:** Store `print.wipe_tower_data()` pointer or `&print` in a member, read it later in a slot.
**Why it's wrong:** The `Print` is destroyed when the worker lambda exits (`SliceService.cpp:664`). The pointer dangles. This is exactly Frozen Decision 1 (Phase 99) — the v4.4 invariant.
**Do this instead:** Capture by value into a POD struct inside the worker, before `activePrint_.store(nullptr)`. WS2's mesh capture must copy the vertex data (flatten to `QVector<GizmoVertex>` or deep-copy the ITS), not hold a reference.

### Anti-Pattern 2: Widening the filament-map enum in only one layer (WS1)

**What people do:** Widen `PartPlate.h:273` `m_filament_map_mode` to support 4 values but forget the QML layer, or the 3MF persistence layer.
**Why it's wrong:** The mode round-trips through `PartPlate` → `ProjectServiceMock::setPlateFilamentMap` → 3MF `filament_map_mode` attribute (`bbs_3mf.cpp:4447` writes it as the enum name string) → reload. A partial widening corrupts the persisted value.
**Do this instead:** Widen all four surfaces atomically: (1) `PartPlate.h:273` raw int with updated comment citing upstream `fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault`; (2) `ProjectServiceMock` filament-map API accepts 0-3; (3) EditorViewModel Q_PROPERTY `filamentMapMode` exposes 0-3; (4) `FilamentGroupPopup.qml` renders all 4 modes. The 3MF layer already persists by enum-name-string (upstream), so it is forward-compatible if the enum names match.

### Anti-Pattern 3: Replacing `uploadWipeTowerBuffer` instead of branching it (WS2)

**What people do:** Rewrite `uploadWipeTowerBuffer` to only handle the mesh, deleting the box path.
**Why it's wrong:** `wipe_tower_mesh_data` is `std::optional` and resets to `std::nullopt` on `clear()` (upstream `Print.hpp:776`). Some configs (certain brim/rib settings) produce no mesh. The box path is the required fallback.
**Do this instead:** Add a branch at `RhiViewportRenderer.cpp:1073-1080`: if `m_wipeTowerMeshVerts` is non-empty, call `buildWipeTowerMeshVertices`-flattened verts; else fall through to the existing `buildWipeTowerVertices` box. Option A and Option B coexist, gated on whether the mesh was captured.

### Anti-Pattern 4: Implementing Measure math instead of instantiating `Measure::Measuring` (WS5)

**What people do:** Reimplement point-on-plane, edge-distance, angle-between-faces math in Qt6.
**Why it's wrong:** `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119` ships a complete `Measuring` class with `MeasuringImpl` (pimpl). The project constraint (AGENTS.md) is "instantiate, don't reimplement" libslic3r.
**Do this instead:** Port `SceneRaycaster` + `MeshRaycaster` (the picking layer, which is GUI-side and wxWidgets-coupled, so it must be ported), but call `Measure::Measuring(its)` directly. The Qt6 work is wiring (raycaster → volume → `Measuring` instance → result struct → Q_PROPERTY), not geometry math.

## Integration Points

### Workstream 1: Auto Filament-Map Recommendation

| Integration Point | File:Line | New vs Modified | Notes |
|---|---|---|---|
| Filament-map readback in worker | `src/core/services/SliceService.cpp:642-659` (alongside WTREAD capture) | **Modified** — add `print.get_filament_maps()` + `get_filament_map_mode()` reads | Mirrors Phase 100 wipe-tower capture exactly; new `FilamentMapResult` POD in `SliceService.h` next to `WipeTowerGeometry:42` |
| New signal | `src/core/services/SliceService.h:167` (`wipeTowerGeometryReady`) | **New** — `filamentMapReady(const FilamentMapResult&)` | Same pattern, emitted on slice success branch |
| Enum widening (Qt core) | `src/core/model/PartPlate.h:273` (`m_filament_map_mode = 0`) | **Modified** — comment updates 0-3 values citing upstream `FilamentMapMode` (`PrintConfig.hpp:424`) | Raw int stays raw int (round-trips through 3MF by enum-name); only the documented range widens |
| ViewModel slot + Q_PROPERTYs | `src/core/viewmodels/EditorViewModel.h:829` (`onWipeTowerGeometryReady`) + `:719-724` getters | **Modified** — add `onFilamentMapReady` slot in same `private slots:` block + `filamentMapMode`/`filamentMaps` Q_PROPERTYs | PMF connect in `EditorViewModel.cpp:2225` pattern; NOTIFY `filamentMapChanged` |
| ProjectServiceMock API | `src/core/services/ProjectServiceMock.h:149-151` (`setPlateFilamentMap`) | **Modified** — accept mode 0-3; current 2-value comment updates | 3MF persistence at `bbs_3mf.cpp:4447` writes enum-name-string, forward-compatible |
| QML popup | `src/qml_gui/components/FilamentGroupPopup.qml` | **New** | Binds to `editorVm.filamentMaps`; no service access from QML (AGENTS.md rule) |
| QML bindings | `src/qml_gui/pages/PreparePage.qml:1670-1675` (WTREAD binding pattern) | **Modified** — add filament-map overlay/popup trigger bindings | Same `root.editorVm ? root.editorVm.X : default` null-guard pattern |

**Upstream anchors:** `Print.hpp:996-997` (`get_filament_maps`, `get_filament_map_mode`), `Print.cpp:2484-2493` (auto-compute via `ToolOrdering::get_recommended_filament_maps`), `PrintConfig.hpp:424` (`enum FilamentMapMode { fmmAutoForFlush, fmmAutoForMatch, fmmManual, fmmDefault }`), `FilamentGroup.hpp:119` (`class FilamentGroup` — the auto-compute engine libslic3r already calls during `print.process()`).

### Workstream 2: Option B Real Wipe-Tower Mesh

| Integration Point | File:Line | New vs Modified | Notes |
|---|---|---|---|
| Mesh readback in worker | `src/core/services/SliceService.cpp:647` (`print.wipe_tower_data()`) | **Modified** — also read `.wipe_tower_mesh_data` (std::optional) when present | Same capture window; flatten `real_wipe_tower_mesh` + `real_brim_mesh` to `QVector<GizmoVertex>` via `convex_hull_3d()` before capture |
| WipeTowerGeometry extension | `src/core/services/SliceService.h:42` | **Modified** — add `QVector<GizmoVertex> meshVerts` (empty = Option A fallback) | Keep POD semantics; the vertex vector is the cross-thread payload |
| NEW mesh vertex builder | `src/core/rendering/GizmoGeometry.h:74` (`buildWipeTowerVertices`) | **New** — `buildWipeTowerMeshVertices(const QVector<GizmoVertex>& capturedVerts)` OR an ITS→GizmoVertex adapter | Pure-CPU helper; the convex_hull_3d + ITS-to-vertex flattening happens in the worker (libslic3r side), so this builder just hands through pre-flattened verts (keeps core/rendering libslic3r-free) |
| uploadWipeTowerBuffer branch | `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1073-1080` | **Modified** — branch on `m_wipeTowerMeshVerts` non-empty | `ensureBuffer` + `byteSize = vertices.size() * sizeof(Vertex)` at `:1082` already handles variable size — no buffer-layer change |
| Coexist gate | `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1075-1079` | **Modified** — `if (meshVerts non-empty) useMesh else useBox` | Option A retained as fallback when `wipe_tower_mesh_data == nullopt` |

**Decision: NEW helper, not extend `buildWipeTowerVertices`.** `buildWipeTowerVertices` emits a fixed 36-vertex box from 5 floats (signature is `(x,z,width,depth,height)`). A mesh path needs a different signature (variable vertex count, no dimension args). Adding an overload would overload on unrelated signatures. A sibling `buildWipeTowerMeshVertices` is cleaner and keeps the Option A box path byte-for-byte unchanged (the Phase 101 regression-lock contract).

**Decision: Option B coexists with Option A, does not replace it.** Upstream ships both paths (`3DScene.cpp:840 load_wipe_tower_preview` box default + `:887 load_real_wipe_tower_preview` mesh upgrade). The mesh is `std::optional` and resets to `nullopt`. The box is the required fallback.

**Re-opens Phase 99 Frozen Decision 2 (WT-RENDER-UPGRADE).** Option B was LOCKED future per `99-GAP-MATRIX.md`. WS2 implementation must cite this re-opening.

**Upstream anchors:** `Print.hpp:745-746` (`real_wipe_tower_mesh`, `real_brim_mesh` inside `WipeTowerMeshData`), `Print.hpp:766` (`std::optional<WipeTowerMeshData> wipe_tower_mesh_data`), `Print.hpp:776` (`clear()` resets to nullopt), `3DScene.cpp:887-925` (`load_real_wipe_tower_preview`), `TriangleMesh.hpp:136` (`convex_hull_3d()`).

### Workstream 3: CLI Fixtures + Deterministic argv GUI Fixture Loading

| Integration Point | File:Line | New vs Modified | Notes |
|---|---|---|---|
| QCommandLineParser options | `src/qml_gui/main_qml.cpp:177-196` (existing options) | **Modified** — add `--slice` (auto-slice after load), `--gizmo <name>` (pre-activate), `--plate <idx>` (select plate), `--filament-map-mode <m>` (WS1 evidence) | All additive; existing 4 options untouched |
| StartupOpenRequest struct | `src/qml_gui/main_qml.cpp:199-204` (`request.page/dialogs/modelPaths/skipFirstRun`) | **Modified** — add `shouldSlice`, `gizmo`, `plateIndex`, `filamentMapMode` fields | Same struct, extended |
| applyStartupOpenRequests dispatch | `src/qml_gui/main_qml.cpp:207-257` | **Modified** — dispatch new fields to BackendContext methods | Same `QTimer::singleShot(0, &backend, ...)` pattern |
| BackendContext methods | `src/qml_gui/BackendContext.h` (composition root) | **Modified** — add `requestSliceForPlate(idx)`, `activateGizmo(name)` forwarders to EditorViewModel/SliceService | BackendContext is the sole argv→service bridge |
| Fixture reach to BackendContext | `src/qml_gui/main_qml.cpp:349` (`applyStartupOpenRequests(startupOpenRequest, backend)`) | **Already exists** — this is the integration point | No new transport; fixtures ARE the deep-link path |

**Fixtures reach BackendContext via the existing `applyStartupOpenRequests` call** at `main_qml.cpp:349`. No new fixture-to-backend transport layer is needed — the v4.1 deep-link infrastructure (built exactly for the Windows-capture-API blocker) is the fixture infrastructure.

**Upstream anchors:** none — this is Qt6-only infrastructure (upstream has no equivalent; the recurring Windows capture-API blocker is Qt6-specific).

### Workstream 4: D3D12 Crash Root-Cause + Backend Promotion

| Integration Point | File:Line | New vs Modified | Notes |
|---|---|---|---|
| Debug-layer enable | `src/qml_gui/Renderer/RhiBackendSelector.cpp:22` (`QRhiD3D12InitParams d3d12Params;`) | **Modified** — set `d3d12Params.enableDebugLayer = true` when `OWZX_RHI_RENDERER=d3d12-debug` (new value) or env-gated | `QRhiD3D12InitParams` has an `enableDebugLayer` field (Qt RHI API); currently left default (false) |
| Crash investigation surface | `src/qml_gui/Renderer/RhiBackendSelector.cpp:63-87` (`probeBackend`) | **Modified** — extend probe to actually run a minimal render (current probe only creates the QRhi, doesn't render) | The crash is in "prepare render" (`:39-41` comment, Phase 26 isolation), which happens after probe, in the live QQuickRhiItem. Needs a separate minimal D3D12 repro harness. |
| Backend selection policy | `src/qml_gui/Renderer/RhiBackendSelector.cpp:37-46` (`defaultWindowsCandidates`) | **Modified (conditional)** — only if root-cause succeeds: reorder or promote D3D12 | Today D3D11 is first ("auto"), D3D12 explicit. Promotion = making D3D12 safe enough to be a candidate, NOT making it default (PROJECT.md Constraint: D3D12 default requires backend crash resolution) |
| Graphics API set | `src/qml_gui/main_qml.cpp:267-268` (`QQuickWindow::setGraphicsApi`) | **Unchanged** unless promotion lands | The selector output drives this; no direct change |
| Vulkan readiness | n/a | **Evaluation only** | PROJECT.md: Qt SDK lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`; Vulkan is future until a Vulkan-enabled Qt SDK is available. No code change in WS4. |

**Debug-layer enable goes in `RhiBackendSelector`, not `main_qml`.** The `QRhiD3D12InitParams` instance lives in `RhiProbeOwner` at `RhiBackendSelector.cpp:19-24`. That is the single point where the params struct is constructed before `QRhi::create(D3D12, params)` at `:80`.

**"Backend promotion" architecture:** the selector already supports promotion by reordering `defaultWindowsCandidates()` (`:42-45`). Promotion does NOT mean making D3D12 the unconditional default — it means the probe + a smoke render succeeds, so a user requesting `d3d12` (or a future `auto` that tries D3D12 first) does not crash. PROJECT.md explicitly keeps "D3D12 default before crash resolved" out of scope.

### Workstream 5: Full GLGizmoMeasure Feature-Picking Engine + AssembleViewDataPool Clipper

| Integration Point | File:Line | New vs Modified | Notes |
|---|---|---|---|
| Per-volume ITS accessor | `src/core/services/ProjectServiceMock.h:106-108` (`cloneCurrentPlateModel` / `rawModel`) | **New** — `QByteArray volumeMeshData(int objIdx, int volIdx) const` or `indexed_triangle_set_view` under `#ifdef HAS_LIBSLIC3R` | The current `meshData()` (`ProjectServiceMock.h:269`) is per-OBJECT (flattened), not per-volume. GLGizmoMeasure needs per-volume ITS for `m_mesh_raycaster_map`. Model* is already held; the accessor walks `model_->objects[obj]->volumes[vol]->mesh()`. |
| MeshRaycaster port | `src/core/rendering/MeshRaycaster.{h,cpp}` | **New** — port of `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp` `MeshRaycaster` | Pure CPU ray-triangle intersection; wraps an `indexed_triangle_set`. No Qt6::Quick / qrhi.h. |
| SceneRaycaster port | `src/core/rendering/SceneRaycaster.{h,cpp}` | **New** — port of `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp` | Aggregates per-volume MeshRaycasters; `hit(mouse, camera)` returns the nearest. EType/EIdBase enums port verbatim. |
| Measure::Measuring consumption | (call site in EditorViewModel or a new MeasureService) | **New** — `#include <libslic3r/Measure.hpp>`; `Measure::Measuring measuring(its)` | Instantiate, don't reimplement (AGENTS.md). Upstream `Measure.hpp:119`. |
| Gizmo integration | `src/qml_gui/Renderer/RhiViewport.h:88` (`GizmoMeasure = 3`) | **Unchanged enum** — the slot exists; the engine behind it is what's missing | Phase 92 already has `GizmoAssemblyMeasure = 19` (`:109`) as the sibling for AssembleView; both consume the same SceneRaycaster. |
| AssembleViewDataPool clipper | `src/core/rendering/AssembleViewDataPool.h` (`ModelObjectsClipper = 1 << 4`) | **Modified** — register the deferred resource (slot already reserved at bit 4 per the Phase 93 deferral note) | Needs per-volume ITS (same dependency as the raycaster) + a `MeshClipper` port. Same unblocking condition as the raycaster. |
| Picking hook | `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (picking path) | **Modified** — route pick events through SceneRaycaster when `m_gizmoMode == GizmoMeasure || GizmoAssemblyMeasure` | Current picking is gizmo-grabber-based; feature-picking needs the raycaster path. |

**Critical dependency: per-volume ITS exposure must land before the raycaster port.** The current `meshData()` flattens all volumes of an object into one buffer (`ProjectServiceMock.cpp:325` "per-volume emission" comment refers to batching, not separate volume access). `SceneRaycaster::m_volumes` (`SceneRaycaster.hpp:71-72`) needs one raycaster per volume. The new accessor on ProjectServiceMock unblocks both WS5's raycaster AND the AssembleViewDataPool clipper (same per-volume ITS dependency noted in `AssembleViewDataPool.h` bit-4 deferral comment).

**GLGizmoMeasure integrates with the existing `GizmoMeasure = 3` and `GizmoAssemblyMeasure = 19` enums.** No enum change needed — the slots exist (Phase 92 added 19). The work is the engine behind the slots, not new enum values.

**Upstream anchors:** `SceneRaycaster.hpp` (full port target, 126 lines), `MeshUtils.hpp` (`MeshRaycaster` class, port target), `Measure.hpp:119` (`Measuring` class — instantiate), `GLGizmoMeasure.hpp:159` (`m_mesh_measure_map`), `GLGizmoMeasure.hpp:193` (`m_mesh_raycaster_map`), `GLGizmosCommon.hpp:268` (AssembleViewDataID).

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| SliceService worker ↔ GUI thread | POD-by-value + queued signal (Frozen Decision 1) | WS1 + WS2 must respect this; the `Print*` never escapes the worker |
| EditorViewModel ↔ RhiViewport | Q_PROPERTY bindings in PreparePage.qml (no direct pointer) | WS2 mesh flows through the same WTREAD Q_PROPERTY channel |
| core/rendering ↔ qml_gui/Renderer | GizmoGeometry builders (pure CPU) → RhiViewportRenderer upload | WS2's new builder + WS5's raycaster both stay libslic3r-free in core/rendering (GizmoVertex.h split) |
| ProjectServiceMock ↔ EditorViewModel | Injected raw pointer (non-owning) | WS5 per-volume ITS accessor added here; EditorViewModel forwards to raycaster |
| BackendContext ↔ QML deep links | `backend` context property + Q_INVOKABLE methods | WS3 fixtures dispatch through this boundary |

## Suggested Build Order (dependency-aware)

```
Phase A (parallel, no deps):
  WS3 CLI fixtures     — unblocks evidence capture for all other WS (highest ROI first)
  WS4 D3D12 debug-layer — investigation, no other WS depends on the outcome

Phase B (parallel after WS3, no inter-deps):
  WS1 Auto filament-map readback (libslic3r already auto-computes; readback + UI only)
  WS2 Option B wipe-tower mesh readback (extends v4.4 WTREAD; re-opens Phase 99 FD2)

Phase C (after WS1, WS2 — needs the worker-capture + evidence):
  WS1 UI: FilamentGroupPopup.qml + enum widening (PartPlate.h, ProjectServiceMock, EditorViewModel, QML)
  WS2 mesh: GizmoGeometry::buildWipeTowerMeshVertices + uploadWipeTowerBuffer branch

Phase D (after WS2 mesh path proven — shares variable-size buffer lessons):
  (WS2 finalization — convex_hull_3d + nullopt fallback testing)

Phase E (independent track, after WS3 only):
  WS5 step 1: per-volume ITS accessor on ProjectServiceMock (unblocks both raycaster + clipper)
  WS5 step 2: MeshRaycaster port (pure CPU)
  WS5 step 3: SceneRaycaster port (aggregates MeshRaycaster)
  WS5 step 4: Measure::Measuring consumption + EditorViewModel measurement Q_PROPERTYs
  WS5 step 5: AssembleViewDataPool ModelObjectsClipper registration (bit 4)
```

**Dependency rationale:**
- **WS3 first** because the Windows-capture-API blocker has recurred across v3.8-v4.4; deterministic argv fixtures unblock evidence for every other WS. WS3 is also the lowest-risk (purely additive argv options).
- **WS1 readback before WS1 UI** because the slot contract (POD shape, signal) must exist before the popup can bind. Mirrors Phase 100 (readback) → Phase 101 (rendering) ordering.
- **WS2 after WS1** because both read back from the same worker window; landing WS1 first establishes the second capture-by-value precedent, reducing WS2 risk. WS2's ITS extension to `WipeTowerGeometry` then looks like a known pattern.
- **WS5 step-1 (per-volume ITS) before everything else in WS5** because both the raycaster and the AssembleViewDataPool clipper share that exact dependency (documented in `AssembleViewDataPool.h` bit-4 deferral note).
- **WS4 is independent** — it is investigation-heavy and may not produce a "feature." Schedule it in parallel so it does not block the deliverable WS.

## Sources

- `.planning/milestones/v4.4-phases/100-wipe-tower-geometry-readback/100-01-SUMMARY.md` — the WTREAD readback pattern to mirror for WS1
- `.planning/milestones/v4.4-phases/101-wipe-tower-real-rendering-upgrade/101-01-SUMMARY.md` — Option A baseline + Option B LOCKED-future contract
- `.planning/milestones/v4.4-phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md` — Frozen Decisions (WS2 re-opens FD2)
- `AGENTS.md` — architecture rules (business logic in core/, QML presents only, BackendContext composition root, instantiate-don't-reimplement libslic3r)
- `src/core/services/SliceService.cpp:490-665` — the slice worker (WS1+WS2 capture window)
- `src/core/services/SliceService.h:42-62` — `WipeTowerGeometry` POD (WS1 model + WS2 extension target)
- `src/core/viewmodels/EditorViewModel.h:717-724,822-829` — wipe-tower getters + `onWipeTowerGeometryReady` slot (WS1 mirror target)
- `src/core/model/PartPlate.h:267-273` — `m_filament_map_mode` 2-value raw int (WS1 widening surface)
- `src/core/services/ProjectServiceMock.h:106-108,149-151,269` — `rawModel`, filament-map API, `meshData` per-object (WS1 API + WS5 per-volume accessor target)
- `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-533` — `buildWipeTowerVertices` (WS2 sibling helper target)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1097` — `uploadWipeTowerBuffer` (WS2 branch target; dynamic-size path already correct)
- `src/qml_gui/main_qml.cpp:170-257,265-268,349` — QCommandLineParser + applyStartupOpenRequests + RhiBackendSelector (WS3 integration point)
- `src/qml_gui/Renderer/RhiBackendSelector.cpp:19-24,37-46,63-87` — `QRhiD3D12InitParams`, candidate order, probeBackend (WS4 debug-layer + crash surface)
- `src/qml_gui/Renderer/RhiViewport.h:84-110` — GizmoMode enum incl. `GizmoMeasure=3`, `GizmoAssemblyMeasure=19` (WS5 slots exist)
- `src/core/rendering/AssembleViewDataPool.h` — bit-4 `ModelObjectsClipper` reservation (WS5 clipper target)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:745-746,766,776,996-997` — `WipeTowerMeshData`, `wipe_tower_mesh_data`, `get_filament_maps/mode`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424` — `enum FilamentMapMode { fmmAutoForFlush, fmmAutoForMatch, fmmManual, fmmDefault }` (4-value, WS1 target)
- `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119` — `Measuring` class (WS5 instantiate, don't reimplement)
- `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp` — WS5 port target (full file, 126 lines)
- `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp` — `MeshRaycaster` WS5 port target
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.hpp:159,193` — `m_mesh_measure_map`, `m_mesh_raycaster_map` (WS5 data-structure model)
- `third_party/OrcaSlicer/src/libslic3r/FilamentGroup.hpp:119` — `class FilamentGroup` (WS1 auto-compute engine, already invoked by libslic3r)

---
*Architecture research for: v4.5 backlog-closure workstreams (filament-map, wipe-tower Option B, CLI fixtures, D3D12, GLGizmoMeasure)*
*Researched: 2026-07-12*
