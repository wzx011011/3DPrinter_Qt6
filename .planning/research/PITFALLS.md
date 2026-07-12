# Pitfalls Research

**Domain:** Adding 5 workstreams to OWzx Slicer (OrcaSlicer Qt6/QML migration) — auto filament-map, Option B real wipe-tower mesh, CLI argv fixtures, D3D12 root cause, full GLGizmoMeasure engine + AssembleViewDataPool clipper
**Researched:** 2026-07-12
**Confidence:** HIGH (rooted in v4.4 Phase 100/101 evidence, v4.3 Phase 97 evidence, upstream Print.cpp:2484-2491 / 3DScene.cpp:887-925 source anchors, and the existing Qt6 RHI/SliceService code)

This document is specific to THIS codebase's history. It is NOT a generic 3D-slicer pitfalls list. Each pitfall cites the v4.4/v4.3 incident or upstream seam that motivates it.

---

## Critical Pitfalls

### Pitfall 1: Filament-map readback forgets the capture-by-value invariant (re-creates the v4.4 `Print*` lifetime hazard)

**What goes wrong:**
The implementer reads `print.get_filament_maps()` from `SliceService` worker and stashes a `Print*` or `std::vector<int>*` pointer on `SliceService`/`EditorViewModel` for the GUI thread to dereference. The pointer dangles because `activePrint_.store(nullptr)` is immediately followed by the `print` local going out of scope at the end of the worker `try` block (`SliceService.cpp:664-665`). The GUI-thread slot dereferences freed memory — a use-after-free that may not crash immediately but corrupts adjacent state.

**Why it happens:**
Phase 100 established the `WipeTowerGeometry` POD capture-by-value invariant precisely because the alternative (stashing `Print*`/`WipeTowerData*`) was the obvious-but-wrong shape. Frozen Decision 1 / D-11 in Phase 100 SUMMARY is a direct response to this hazard. A new implementer who did not live Phase 100 will reach for a pointer because "the GUI needs live access to the map." The auto-recommended map is even more tempting to expose by reference because it is a `std::vector<int>` that "feels cheap to point at."

**How to avoid:**
Mirror Phase 100 exactly. Declare a POD struct in `SliceService.h` (parallel to `WipeTowerGeometry`):
```cpp
struct FilamentMapReadback {
  bool valid = false;
  std::vector<int> filamentMaps;      // from print.get_filament_maps() after process()
  int mode = 0;                        // widened to upstream 4-value enum (see Pitfall 2)
};
```
Capture it by value into the worker-local `capturedFilamentMap` between `print.process()` (`SliceService.cpp:590`) and `activePrint_.store(nullptr)` (`:664`). Deliver it by value into the `sliceFinished` queued lambda capture list, then emit a new `filamentMapReady` signal on the success branch (mirrors `wipeTowerGeometryReady` at `:805`). No `Print*` may escape. The smoke-test pattern at `tests/ViewModelSmokeTests.cpp:3949-4031` (QMetaObject::invokeMethod + qRegisterMetaType) ports directly.

**Warning signs:**
- A `Print*` or `DynamicPrintConfig*` member added to `SliceService`/`EditorViewModel` for "lazy" reads.
- A readback signal carrying a `const std::vector<int>&` argument (cross-thread dangling reference) instead of a value.
- An `emit` of the filament-map signal placed BEFORE `activePrint_.store(nullptr)` (the Print can still be torn down before the queued lambda runs).
- A `grep` for `get_filament_maps` outside `SliceService.cpp` worker block (it must only be called inside the worker `try`, between `process()` and `store(nullptr)`).

**Phase to address:**
The filament-map readback phase (the analog of Phase 100 — likely the first or second phase of the filament-map workstream). The Phase 100 SUMMARY's `key-decisions` and `patterns-established` sections are the template; the new phase SUMMARY must repeat the capture-by-value self-check verbatim with `FilamentMapReadback` substituted.

---

### Pitfall 2: Widening the filament-map mode enum from 2-value to 4-value silently breaks existing callers

**What goes wrong:**
The current Qt6 `PartPlate::m_filament_map_mode` is documented as `0=Auto, 1=Manual` (`src/core/model/PartPlate.h:271`). Upstream `FilamentMapMode` (`PrintConfig.hpp:424-429`) has FOUR values: `fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2, fmmDefault=3`. If the widening renames/renumbers the Qt enum and misses callers, three concrete breaks fire:

1. **Persistence round-trip desync:** `ProjectServiceMock.cpp:4995-4997` writes `filamentMapMode()` to `pd->config["filament_map_mode"]` via `opt->setInt(mode)`, and `:5413-5417` reads it back from `plate->config.option("filament_map_mode")`. If the integer stored under the old convention (1=Manual) is re-read under the new convention (1=fmmAutoForMatch), every previously-saved 3MF "Manual" plate silently becomes "AutoForMatch." This is a data-corruption bug that only surfaces on reload.
2. **SliceService override precedence:** `SliceService.cpp:500-509` applies the per-plate config last (`config.apply(*plateCfg)` with plate winning). A plate persisted with old-value `1` now means AutoForMatch, so the libslic3r auto-computation path (`Print.cpp:2487 if (map_mode < fmmManual)`) fires for what the user thought was a manual plate.
3. **FilamentGroupPopup UI:** if the popup only ever set `mode=1` for "Manual," it now sets AutoForMatch and the manual assignment UI is unreachable.

**Why it happens:**
"Auto" in the Qt6 enum was always ambiguous — upstream has TWO auto modes (AutoForFlush, AutoForMatch) distinguished by optimization target. The v3.2 FMAP-04 deferral deliberately collapsed both into `0=Auto` because the auto engine was not implemented. Re-opening FMAP-04 requires splitting `0` into `0`/`1`, which shifts `Manual` from `1` to `2`. A `grep` for `setFilamentMapMode(1)` and `m_filament_map_mode = 1` returns few hits and looks safe; the real hazard is persisted state on disk, not source code.

**How to avoid:**
- Treat the enum as a v3.2-v4.5 data-migration problem, not a code-rename. Add a read-time normalization: when loading a 3MF whose `filament_map_mode` was written under the 2-value convention, map legacy `1` (Manual) → `2` (fmmManual). The cleanest signal for "is this a legacy file" is the absence of any `fmmAutoForMatch`/`fmmDefault` write in the same metadata block — but a simpler heuristic is "if mode > max legacy value (1) AND the file was written by an older version." Bump the project/3MF schema version if one exists.
- Audit ALL writers and readers before changing the enum: `grep -rn "filament_map_mode\|filamentMapMode\|setFilamentMapMode\|m_filament_map_mode" src/ third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp`. The upstream writer (`bbs_3mf.cpp:7964-7967`) uses `ConfigOptionEnum<FilamentMapMode>::get_enum_names()[getInt()]` — string-keyed, so upstream files are safe. The Qt6 `setInt` path at `ProjectServiceMock.cpp:4997` is the raw-int hazard.
- Add a round-trip test (model the `thumbnailSaveReloadRoundTrip` test at `tests/PartPlateTests.cpp` after the Phase 97 fix): write a Manual plate, reload, assert `filamentMapMode()==2`.

**Warning signs:**
- The enum change touches `PartPlate.h` only and ships without a 3MF read-side migration.
- A reload of a pre-v4.5 project file shows Manual plates flipped to Auto.
- `ProjectServiceMock.cpp:4997` still calls `opt->setInt(p->filamentMapMode())` with no comment about the value-shift.
- The FilamentGroupPopup QML uses integer literals (`mode === 1`) instead of named enum constants exposed via `Q_ENUM`.

**Phase to address:**
The enum-widening phase (a dedicated phase BEFORE the auto-computation readback — likely the first phase of the filament-map workstream). The migration must land before any auto-mode code, because auto-mode code will write the new values and entrench the new convention before legacy files are migrated.

---

### Pitfall 3: Option B ITS regression — replacing the Option A box breaks the v4.4-frozen `buildWipeTowerVertices` contract

**What goes wrong:**
v4.4 shipped Option A as a FROZEN baseline (Phase 99 Frozen Decision 2; Phase 101 locked the 36-vertex contract). The temptation when adding Option B is to modify `GizmoGeometry::buildWipeTowerVertices` (`GizmoGeometry.cpp:487-535`) to emit the real mesh, or to modify `RhiViewportRenderer::uploadWipeTowerBuffer` (`:1064-1097`) to take an ITS. Both paths risk regressing the v4.4 baseline that the Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent` test guards. Concrete regressions observed-or-likely:

1. **Vertex format mismatch:** Option A uses `GizmoVertex` (position+color); Option B's `TriangleMesh::its` uses `indexed_triangle_set` (separate vertex + index arrays). If `uploadWipeTowerBuffer` is changed to consume ITS without keeping the Option A path, the SoftwareViewport fallback (`SoftwareViewport.h:236-240`) which consumes `GizmoVertex` breaks, or the two paths diverge and the SoftwareViewport box no longer mirrors the RHI mesh.
2. **Buffer-size miscalculation:** Option A computes `byteSize = vertices.size() * sizeof(Vertex)` (`:1082`). Option B's ITS has `vertices.size()` positions + `indices.size()/3` triangles; a naive `byteSize = its.vertices.size() * sizeof(Vertex)` forgets the index buffer. The `ensureBuffer` call (`:1083`) succeeds for the vertex buffer but the index buffer is never allocated → garbage draw or GPU fault.
3. **Capture-by-value violation:** Option B reads `wipe_tower_data().wipe_tower_mesh_data->real_wipe_tower_mesh`. That field is `std::optional<WipeTowerMeshData>` (`Print.hpp:766`) holding a `TriangleMesh` (heap-allocated vertices). If the readback stashes a reference to the `TriangleMesh` and delivers it across threads, it dangles when `Print` is torn down — the SAME Frozen-Decision-1 hazard as Pitfall 1, just for mesh data instead of scalars. The mesh must be deep-copied (`TriangleMesh` copy ctor) into the POD-style readback struct.

**Why it happens:**
The Option A baseline is so simple (36 fixed vertices) that "just swap in the mesh" looks like a 10-line change. The Phase 99/100/101 decision trail explicitly flagged Option B as needing "ITS vertex format extension" — but a new implementer reads that as "add a code path," not "add a parallel code path and keep Option A as the fallback." The `wipe_tower_mesh_data == std::nullopt` case (`Print.hpp:776 clear()` resets it) is also easy to forget — if a single-material slice clears the optional, the readback must fall back to Option A, not crash on `->real_wipe_tower_mesh`.

**How to avoid:**
- Keep `buildWipeTowerVertices` UNTOUCHED as the Option A fallback. Add a NEW `buildWipeTowerMeshVertices(const TriangleMesh&)` (or `buildWipeTowerMeshFromIts(const indexed_triangle_set&)`) that mirrors `load_real_wipe_tower_preview` (`3DScene.cpp:887-925`): merge `real_wipe_tower_mesh` + `real_brim_mesh` if render_brim, then `mesh.convex_hull_3d()` (`:914`), then emit vertices. The convex hull is the upstream convention — it gives the tower shell, not the raw toolpath mesh.
- Add a NEW upload function `uploadWipeTowerMeshBuffer` (do not overload `uploadWipeTowerBuffer`). Gate on a flag (`m_useWipeTowerMesh`) set by the readback when `wipe_tower_mesh_data.has_value()` AND a feature flag is on. Default OFF so v4.4 baseline behavior is bit-identical until explicitly enabled.
- Deep-copy the mesh in the worker: `capturedGeometry.mesh = wtData.wipe_tower_mesh_data->real_wipe_tower_mesh;` (TriangleMesh is copyable). The POD-by-value invariant extends to "POD-or-value-type-by-value." Document that `TriangleMesh` copy is non-trivial (heap alloc) but bounded by tower size (kilobytes, not megabytes).
- Extend the Phase 102 regression test to assert the Option A 36-vertex path STILL fires when `wipe_tower_mesh_data == nullopt` (single-material / cleared). Add a parallel Option B anchor test gated on the feature flag.

**Warning signs:**
- `buildWipeTowerVertices` signature changes (the Phase 101 W1 fix comment at `:465-471` documents the center-convention contract — any signature change risks re-breaking W1).
- `uploadWipeTowerBuffer` body changes shape (the `ensureBuffer` + `byteSize` pattern at `:1082-1085` is load-bearing for the buffer-sizing pitfall).
- A `const TriangleMesh&` member added to `EditorViewModel` or `SliceService` (capture-by-reference — same hazard as Pitfall 1).
- The `wipe_tower_mesh_data == std::nullopt` branch is missing from the readback.
- No convex_hull_3d call (raw toolpath mesh rendered instead of the shell — visually wrong, upstream always hulls).
- SoftwareViewport and RhiViewport paths diverge (the v4.4 closure required them to mirror — `PROJECT.md:22`).

**Phase to address:**
The Option B render phase (the analog of v4.4 Phase 101). The Option A fallback preservation belongs in the same phase as the Option B path — they must ship together so the fallback is tested, not retrofitted. The ITS buffer-sizing discipline belongs here too. The capture-by-value-for-mesh invariant belongs in the readback sub-phase (analog of Phase 100) that feeds Option B.

---

### Pitfall 4: argv fixture loading fires before QML engine / BackendContext is ready (race condition)

**What goes wrong:**
The current `--load-model` deep link (`main_qml.cpp:232-238`) calls `backend.topbarImportModel(modelPath)` inside a `QTimer::singleShot(0, &backend, ...)`. This is correct TODAY because the single-shot is queued onto the Qt event loop AFTER `QQmlApplicationEngine::load` returns and the QML tree is built. But when extending fixtures to drive deterministic visual-evidence capture (FIXTURE-02 — the recurring Windows-capture-API blocker workaround), three races appear:

1. **QML not yet loaded:** `QQmlApplicationEngine` load is asynchronous-ish; the `singleShot(0)` fires on the first event-loop iteration, which may run BEFORE the QML root `ApplicationWindow` has finished `Component.onCompleted`. `topbarImportModel` calls into `EditorViewModel` which assumes the QML viewport exists for mesh-blob delivery. If the viewport is not yet instantiated, the model loads into the viewmodel but the renderer's `synchronize` has not run yet — the first frame renders empty, then the model pops in on frame 2. For deterministic screenshots this is a flake.
2. **RHI backend not yet initialized:** `selectRhiBackendFromEnvironment` (`RhiBackendSelector.cpp:114`) probes the backend BEFORE `QGuiApplication` exists in some orderings. If a fixture sets `OWZX_RHI_RENDERER=d3d12` to repro a D3D12 crash, the probe runs at `main_qml.cpp:265` (before `QGuiApplication app` at `:288`). That is correct today, but if a fixture wants to capture the D3D12 crash signature it must NOT swallow the probe failure — the current `probeBackend` (`:63`) returns false on `QRhi::create` failure and the selector falls through to software. A crash fixture needs the failure to propagate.
3. **BackendContext async service init:** `BackendContext` constructor (`main_qml.cpp:307`) is synchronous, but its services (`ProjectServiceMock`, `SliceService`) may have lazy/deferred initialization. If `topbarImportModel` runs before `ProjectServiceMock` has its libslic3r `Model*` allocated, the import silently no-ops or crashes on a null `Model*`.

**Why it happens:**
The `singleShot(0)` trick works because Qt event-loop ordering is deterministic in the common case. Fixture authors assume "the app is ready" means "the C++ objects are constructed" — but for screenshot determinism, "ready" means "the GPU has rendered at least one frame with the model uploaded," which is several event-loop iterations later. The current code has no readiness signal.

**How to avoid:**
- Add a readiness gate: defer the fixture's `--load-model`/`--open-page` handling until BOTH (a) `QQmlApplicationEngine::objectCreated` has fired AND (b) the first `QQuickWindow::frameSwapped` has signaled. Connect to both signals and only then invoke `applyStartupOpenRequests`. This guarantees the QML tree + first GPU frame are done.
- For D3D12 crash fixtures specifically: do NOT use the `singleShot(0)` path. The crash surface is at first-render (`RhiViewportRenderer.cpp:281-298` BUG-V31-1 comment), which happens inside `frameSwapped`'s predecessor (`beforeRenderPassRecording`). A crash fixture must let the renderer attempt the pass and capture the minidump — the `CrashHandlerWin::install` at `main_qml.cpp:295` already routes to `crash_dumps/`. The fixture just asserts "minidump file exists with the expected exception code after exit."
- For BackendContext service readiness: assert in `topbarImportModel` that `projectService_` is non-null and its `Model*` is allocated (`HAS_LIBSLIC3R` path). Return false (not crash) if not ready — the existing `topbarImportModel` return-bool contract (`BackendContext.h:361`) supports this.
- Document the event-loop ordering in the fixture phase SUMMARY: `QGuiApplication` → `selectRhiBackendFromEnvironment` → `BackendContext` ctor → `QQmlApplicationEngine::load` → `objectCreated` → first `frameSwapped` → fixture actions.

**Warning signs:**
- A fixture calls `topbarImportModel` directly in `main()` (before the engine loads) instead of via `singleShot`.
- Screenshots intermittently show an empty viewport on the first capture (race #1).
- D3D12 crash fixtures pass on D3D11 and silently skip on D3D12 because the probe fell through to software (race #2).
- `topbarImportModel` returns false intermittently with no log explaining why (race #3 — services not ready).
- The fixture relies on `QTest::qWait(N)` instead of a signal gate (flaky under load).

**Phase to address:**
The CLI fixtures phase (workstream 3). The readiness gate is the first deliverable — every subsequent fixture depends on it. The D3D12 crash-repro sub-fixture belongs in the D3D12 workstream but uses the fixture infrastructure, so the fixture phase must ship first or the two phases must coordinate on the readiness contract.

---

### Pitfall 5: D3D12 debug layer enabled in release builds — leaks GPU validation overhead into production

**What goes wrong:**
The D3D12 investigation phase will want the D3D12 DEBUG LAYER (`ID3D12Device::SetFeatureMasking` / `QRhiD3D12InitParams::enableDebugLayer`) to triage the BUG-V31-1 crash (`RhiViewportRenderer.cpp:283-285`: "beginPass-after-resourceUpdate ... D3D12 strictly enforces command buffer ordering and segfaults"). If the debug-layer enable is gated only on `QRhi::D3D12` selection (not on `#ifdef QT_DEBUG` / CMake build type), it ships in the Release build and causes:

1. **20-40% GPU perf regression** in production for any user who sets `OWZX_RHI_RENDERER=d3d12`.
2. **Spurious validation warnings** spammed to the diagnostic log (`QML_DEBUG_LOG`) on every frame, masking real errors.
3. **A false sense of "D3D12 works"** — the debug layer sometimes papers over undefined-behavior that the release layer crashes on, so the investigation concludes "fixed" when it is not.

**Why it happens:**
The D3D12 investigation is investigation-heavy (per `PROJECT.md:61` — "may not produce a clean feature output"). The natural workflow is "enable debug layer, repro crash, read the validation messages." Once the crash is understood, the debug-layer call is easy to forget in the cleanup. The `QRhiD3D12InitParams` struct (`RhiBackendSelector.cpp:21`) has the enable flag as a plain member with no build-type guard.

**How to avoid:**
- Gate any debug-layer enable behind `#ifndef QT_NO_DEBUG` (or a CMake `OWZX_D3D12_DEBUG` option that defaults OFF). The `RhiBackendSelector.cpp:22` `QRhiD3D12InitParams d3d12Params;` is the seam — set `d3d12Params.enableDebugLayer = true` only inside the `#ifdef`.
- Treat the BUG-V31-1 comment as the leading hypothesis, not the confirmed root cause. The comment says `beginPass`-after-`resourceUpdate` is "undefined in QRhi; D3D12 strictly enforces command buffer ordering." But the code at `:286-296` ALREADY merged the camera-uniform upload into a pre-`beginPass` batch (`:295 uploadCameraUniform(updates, ...)` before `:298 cb->beginPass`). So either the fix is incomplete (another upload site still fires after beginPass) OR the crash has a different root cause (descriptor-heap exhaustion, root-signature mismatch, the 256-byte cbuffer alignment at `:1424-1428`). The investigation phase must distinguish these — do not ship "the debug layer confirmed V31-1" without isolating which upload site actually triggers.
- Add a runtime gate: if `OWZX_RHI_RENDERER=d3d12` AND build is Release AND no `OWZX_D3D12_DEBUG` env var, log a warning that the debug layer is off and crashes will not be annotated. This makes the off-state visible.
- Vulkan is blocked by the Qt SDK feature flag (`PROJECT.md:143`: `vulkan` under `QT_DISABLED_PUBLIC_FEATURES` in `Qt6GuiTargets.cmake`). Do NOT attempt to "unblock" Vulkan by recompiling Qt in the D3D12 phase — that is out of scope. Document Vulkan as "blocked at SDK level, not at app level" so the investigation does not waste cycles on it.

**Warning signs:**
- `enableDebugLayer = true` appears without an `#ifdef` guard.
- Release-build frame times regress after the D3D12 phase ships.
- The phase SUMMARY claims "D3D12 root cause found" but cites only the BUG-V31-1 comment (which predates the phase) rather than a new isolation.
- A Vulkan probe is added to `RhiBackendSelector.cpp` (scope creep — Vulkan is SDK-blocked).
- `defaultWindowsCandidates()` (`RhiBackendSelector.cpp:37-46`) is reordered to put D3D12 first (re-introduces the original v3.2 startup crash for default-`auto` users).

**Phase to address:**
The D3D12 investigation phase (workstream 4). Split into two sub-phases: (a) root-cause isolation (debug layer ON, gated to Debug builds only, reproduce + minidump analysis), (b) backend-promotion evaluation (decide whether D3D12 can become default or stays opt-in). The release-build leak prevention belongs in sub-phase (b) as a hard gate — do not promote without proving the debug layer is off in Release.

---

### Pitfall 6: GLGizmoMeasure ITS exposure — `mesh() const` returns a reference that detaches on copy, breaking the `Measuring` lifetime contract

**What goes wrong:**
The Qt6 codebase TODAY has ZERO per-volume `indexed_triangle_set` (ITS) exposure. Search confirms: `src/core/rendering/AssemblyMeasureGeometry.cpp:23,43` explicitly says it computes "without ITS raycasting"; `EditorViewModel.h:114` documents the deferral. The full GLGizmoMeasure engine (`Measure.hpp:122 explicit Measuring(const indexed_triangle_set&)`) requires an ITS per volume. The natural port is to expose `mesh()` on a volume/object and construct `Measure::Measuring(mesh.its)`. Two lifetime bugs fire:

1. **Detach-on-copy:** if `mesh()` returns `const TriangleMesh&` and the caller writes `Measure::Measuring m(volume.mesh().its);`, the `its` is a reference into the `TriangleMesh` owned by the volume. If ANY operation triggers a detach (Qt implicit-share deep-copy, or a `setMesh`/`transform` call), the `Measuring` object's `its` reference dangles. `Measuring`'s ctor (`Measure.hpp:122`) takes `const indexed_triangle_set&` and (per the pimpl `MeasuringImpl`) likely retains it — the lifetime is "as long as the source ITS lives," which the Qt6 codebase has no contract for.
2. **Rebuild timing:** `Measuring` precomputes plane/edge/circle features in its ctor (the `MeasuringImpl` pimpl at `Measure.hpp:143` is non-trivial — it indexes the mesh). If the volume is transformed (move/rotate/scale) and the `Measuring` object is NOT rebuilt, `get_feature()` returns features in the OLD local frame. Upstream handles this by reconstructing `Measuring` on mesh change (`GLGizmoMeasure.cpp` rebuilds when `m_curr_measuring` volume changes, ~`:615`). The Qt6 EditorViewModel has no "mesh changed" signal today.

**Why it happens:**
The Measure engine was designed for upstream's `GLVolume::mesh_raycaster` lifecycle (one raycaster per volume, rebuilt on mesh change). The Qt6 renderer uses flat `QVector<Vertex>` buffers, not ITS — there is no per-volume ITS to hand to `Measuring`. The port requires introducing an ITS cache layer that does not exist, and the lifetime rules for that cache are not yet defined. The `SurfaceFeature` copy ctor (`Measure.hpp:36-43`) also has a manual `clone()` that copies only `m_*` privates but shallow-copies `volume` (a `void*`!), `plane_indices` (a `std::vector<int>*`!), and `world_tran` — so copying a `SurfaceFeature` across the libslic3r→Qt boundary without understanding these raw pointers is a latent UAF.

**How to avoid:**
- Define an explicit ITS ownership contract BEFORE writing the engine. Recommended: a per-volume `std::shared_ptr<indexed_triangle_set>` held by `EditorViewModel` (or a `VolumeMeshCache` service), updated when the volume's mesh changes. `Measure::Measuring` is constructed from the shared_ptr's `*` and documented to live no longer than the shared_ptr. The `SurfaceFeature::volume` `void*` should be set to the source `EditorViewModel` volume index (cast to `void*`) for back-mapping, NOT to a libslic3r `ModelVolume*` (which the Qt6 side does not own).
- Gate `Measuring` reconstruction on a mesh-changed signal. Add `Q_INVOKABLE void EditorViewModel::onVolumeMeshChanged(int volumeIdx)` that invalidates the cached `Measuring` for that volume. Wire it to every mutation that changes the mesh (load, transform-if-baked, boolean, cut, simplify, drill). Transient transforms (move/rotate/scale without apply) should NOT rebuild the mesh — instead apply the inverse world transform to the hit point (mirrors `GLGizmoMeasure.cpp:677-702` `m_curr_feature->world_tran`).
- Do NOT port the upstream `MeshRaycaster`-per-volume pattern (`GLGizmoMeasure.cpp:183,188,526,552`) verbatim — it creates one raycaster per renderable primitive (sphere, cylinder, plane grippers) AND one per scene volume. On the Qt6 side, reuse the existing `ObjectPicking::pickSourceObject` (`src/core/rendering/ObjectPicking.cpp:109`) ray-AABB for the coarse volume hit, then do the per-triangle ITS raycast ONLY for the hit volume, not all volumes per frame.

**Warning signs:**
- A `Measuring` member on `EditorViewModel` constructed once in the ctor and never rebuilt.
- `mesh()` returns by value (deep-copy per call — slow) or by non-const reference (detachable).
- `SurfaceFeature` instances cross the libslic3r→Qt thread boundary with `volume`/`plane_indices` raw pointers still pointing at libslic3r-owned memory.
- Per-frame raycasting iterates all volumes instead of the coarse-picked one (frame-time spike with >5 objects).
- A transform (move) triggers a full `Measuring` rebuild (the mesh did not change — only the world transform).

**Phase to address:**
The GLGizmoMeasure engine phase (workstream 5). The ITS ownership contract must be the FIRST deliverable of the phase (a `VolumeMeshCache` design doc or header), before any feature-picking code. The per-volume rebuild signal belongs in the same phase, wired to the existing EditorViewModel mutation surface. The AssembleViewDataPool `ModelObjectsClipper` resource (the `AssembleViewDataID::ModelObjectsClipper = 1 << 4` slot reserved at `AssembleViewDataPool.h:44`) shares the same ITS dependency and should be addressed in the same phase or a paired phase — both need the cache.

---

### Pitfall 7: Raycaster performance — per-frame ITS raycasts melt the frame rate at >5 volumes

**What goes wrong:**
The upstream `GLGizmoMeasure.cpp:600-615` loops `m_mesh_raycaster_map` and calls `raycaster->unproject_on_mesh(...)` on EVERY volume PER MOUSE MOVE event. On the upstream side this is tolerable because `MeshRaycaster` uses a BVH built once per volume. If the Qt6 port builds the BVH per-frame (because the `Measuring` object is reconstructed per-frame, or because no BVH cache exists), frame time goes from ~5ms to ~50ms+ with 5+ objects. The symptom is a sluggish cursor and eventually a frozen UI on complex scenes.

**Why it happens:**
`Measure::Measuring`'s pimpl (`MeasuringImpl`) builds internal acceleration structures in its ctor. If the port reconstructs `Measuring` on every mouse move (mistakenly thinking the volume changed when only the cursor moved), the BVH rebuild dominates. Alternatively, if the port calls `unproject_on_mesh` on every volume instead of coarse-picking first, the per-volume cost multiplies by volume count. The existing Qt6 `ObjectPicking::pickSourceObject` (`src/core/rendering/ObjectPicking.cpp:109`) already does ray-AABB coarse picking — but it returns only the source object, not the triangle hit, so the temptation is to skip it and raycast everything.

**How to avoid:**
- Two-stage picking, mandatory: (1) `ObjectPicking::pickSourceObject` (ray-AABB, cheap, returns volume index), (2) per-triangle ITS raycast ONLY on the picked volume's cached `Measuring`. This bounds the per-frame cost to one BVH traversal, not N.
- Cache the `Measuring` per volume, keyed by volume index + mesh revision. Rebuild only when the mesh changes (Pitfall 6 signal), not on cursor move. The cursor-move handler should look up the cached `Measuring` for the currently-picked volume and call `get_feature()` — no reconstruction.
- Add a frame-time regression test: render a scene with N=10 objects at a fixed cursor position, assert frame time < 16ms. The absence of this test is how upstream tolerates the per-volume loop — Qt6 cannot, because the ITS layer is new and unvalidated.

**Warning signs:**
- `new Measure::Measuring(...)` appears inside a mouse-move handler or a per-frame render path.
- `m_mesh_raycaster_map`-style loop iterates all volumes on every mouse event.
- No coarse-pick (ray-AABB) before the per-triangle raycast.
- Frame time scales linearly with object count in the measurement gizmo.
- A BVH build appears in a hot path (search for `MeshRaycaster(` constructions outside of volume-mesh-change handlers).

**Phase to address:**
The GLGizmoMeasure engine phase (workstream 5), as a performance acceptance criterion. The two-stage-pick design belongs in the engine's first design doc; the frame-time regression test belongs in the verification sub-phase (analog of v4.4 Phase 102).

---

### Pitfall 8: FilamentGroupPopup state management — the v2.1 placeholder ships as "real" with no backing Q_INVOKABLE

**What goes wrong:**
The current `FilamentGroupPopup` in `src/qml_gui/BBLTopbar.qml:396-397` is documented as a visual placeholder: "仅视觉，无 Q_INVOKABLE 调用，无状态" (visual only, no Q_INVOKABLE call, no state), with a `TODO(v2.1): implement FilamentGroupPopup`. When the filament-map workstream lights up the popup, the temptation is to add QML-only state (a `property var filamentMap`) and drive the assignment from QML. This violates the project's core architecture rule (`AGENTS.md` / `PROJECT.md:154`): "business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring." The popup then:

1. Diverges from `PartPlate::setFilamentMaps` (`PartPlate.h:201`) — the QML state and the C++ `m_filament_maps` drift, and the 3MF write path (`ProjectServiceMock.cpp:4995`) persists the C++ state, silently losing the QML edits.
2. Has no validation (upstream enforces physical-unprintable constraints — `Print.cpp:3061 get_physical_unprintable_filaments`). QML-only state skips this.
3. Cannot be unit-tested (the ViewModelSmokeTests pattern requires Q_INVOKABLE methods on a viewmodel).

**Why it happens:**
The popup is small UI, and QML-local state is the path of least resistance. The `BBLTopbar.qml:396` comment explicitly flags the placeholder, but a new implementer may read "implement FilamentGroupPopup" as "build the QML UI" rather than "build the viewmodel API + wire QML to it."

**How to avoid:**
- Define the popup's contract on `EditorViewModel` (or a new `FilamentMapViewModel`): `Q_INVOKABLE QList<int> plateFilamentMaps(int plateIdx)`, `Q_INVOKABLE bool setPlateFilamentMaps(int plateIdx, const QList<int>& maps)`, `Q_INVOKABLE int plateFilamentMapMode(int plateIdx)` (already exists at `ProjectServiceMock.h:150`), `Q_INVOKABLE bool setPlateFilamentMapMode(int plateIdx, int mode)`. These delegate to `ProjectServiceMock` which owns `PartPlate::setFilamentMaps/setFilamentMapMode`.
- The popup QML reads via the viewmodel Q_PROPERTYs / Q_INVOKABLE and writes via the setter. No `property var filamentMap` in QML — the popup is pure presentation.
- Expose the auto-recommended map (post-slice) as a separate Q_PROPERTY `recommendedFilamentMaps` on `EditorViewModel` driven by the `filamentMapReady` signal (Pitfall 1), so the popup can show "Recommended (AutoForFlush): [2,1,3]" with an "Apply" button that calls `setPlateFilamentMaps`.
- Add a ViewModelSmokeTests case: set manual maps, read back, assert round-trip. Model it on the existing `wipeTowerGeometryReadbackAppliesValidAndInvalidGate` test.

**Warning signs:**
- A `property var` or `property list<int>` in `FilamentGroupPopup.qml` holding the map state.
- The popup calls a method not present on any viewmodel (pure QML logic).
- No ViewModelSmokeTests case for the popup's set/get round-trip.
- The 3MF write path (`ProjectServiceMock.cpp:4995`) is not touched by the filament-map workstream (the popup edits never reach persistence).
- The popup has no mode selector (AutoForFlush / AutoForMatch / Manual) — only a binary Auto/Manual (the unwidened enum, Pitfall 2).

**Phase to address:**
The filament-map UI phase (the second half of workstream 1, after the readback + enum-widening phases). The viewmodel API must be designed before the QML popup is built — same ordering rule that governed v4.1 settings restoration.

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Stash `Print*` on SliceService for "lazy" filament-map reads | Avoids defining a POD readback struct | Use-after-free when Print is torn down; re-creates v4.4 Frozen-Decision-1 hazard | Never — Phase 100 established the invariant for exactly this reason |
| Modify `buildWipeTowerVertices` to emit Option B mesh | One function to change instead of two | Regresses the v4.4-frozen Option A baseline; breaks SoftwareViewport mirror | Never — add a parallel builder + upload path |
| Drive FilamentGroupPopup from QML-local `property var` | No viewmodel changes needed | State diverges from `PartPlate::m_filament_maps`; 3MF persistence loses edits; violates AGENTS.md architecture rule | Never — viewmodel API is mandatory |
| Enable D3D12 debug layer unconditionally | Always-on validation messages | 20-40% Release-build GPU perf regression; false "fixed" signal | Debug builds only, `#ifdef`-gated |
| Reconstruct `Measuring` per mouse-move | No cache-invalidation logic needed | BVH rebuild dominates frame time; UI freezes at >5 volumes | Never — cache per volume, rebuild on mesh-change signal only |
| Port `m_mesh_raycaster_map` loop verbatim (raycast all volumes per mouse move) | Faithful to upstream | Qt6 has no per-volume raycaster cache; cost multiplies by volume count | Never on Qt6 — two-stage pick (coarse AABB then per-triangle) is mandatory |
| Skip the 3MF legacy-enum migration (assume all files are new) | No read-side normalization code | Pre-v4.5 "Manual" plates reload as "AutoForMatch" | Never if any pre-v4.5 project files exist; otherwise acceptable with a schema-version gate |
| Use `QTest::qWait(N)` for fixture readiness instead of a signal gate | Simpler code | Flaky under load; CI failures that don't repro locally | Never for screenshot fixtures — signal-gate on `frameSwapped` |
| Deep-copy the wipe-tower `TriangleMesh` on every slice even when mesh unused | Avoids `optional` plumbing | Wasted heap alloc per slice (~kB, minor) | Acceptable as a v4.5 simplification if profiling confirms it's cheap |

## Integration Gotchas

Common mistakes when connecting to external services (libslic3r, QRhi, Qt QML engine).

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| libslic3r `Print` (filament-map readback) | Calling `print.get_filament_maps()` AFTER `activePrint_.store(nullptr)` | Read between `print.process()` (`:590`) and `activePrint_.store(nullptr)` (`:664`), capture by value into a POD, deliver via queued signal — mirror Phase 100 `WipeTowerGeometry` |
| libslic3r `Print::wipe_tower_data().wipe_tower_mesh_data` (Option B) | Assuming `wipe_tower_mesh_data` is always populated; dereferencing `->real_wipe_tower_mesh` without `has_value()` check | It is `std::optional` and `clear()` resets it to `nullopt` (`Print.hpp:776`). Single-material / no-wipe-tower slices have no mesh — fall back to Option A |
| libslic3r `ConfigOptionBool` (filament-map mode persistence) | Calling `opt->getInt()` on a `ConfigOptionBool` option (e.g. if any bool option is reused for mode flags) | `ConfigOptionBool` does NOT override `getInt` — it throws "Calling ConfigOption::getInt on a non-int ConfigOption" (Phase 97 THUMBRT-01 hit this for `spiral_mode` at `ProjectServiceMock.cpp:617-623`). Use `option<ConfigOptionBool>(...)->value` |
| libslic3r `FilamentMapMode` enum (widening) | Renaming the Qt6 enum without a 3MF read-side migration | Upstream uses string-keyed `ConfigOptionEnum<FilamentMapMode>::get_enum_names()` (`bbs_3mf.cpp:7965-7967`) — safe. Qt6 uses raw `setInt` (`ProjectServiceMock.cpp:4997`) — must normalize legacy `1`→`2` on read |
| libslic3r `Measure::Measuring` (GLGizmoMeasure) | Constructing `Measuring(mesh.its)` where `mesh` is a `TriangleMesh` that may detach | Hold the ITS in a `shared_ptr<indexed_triangle_set>` owned by the viewmodel; `Measuring` lifetime ≤ shared_ptr lifetime |
| libslic3r `Measure::SurfaceFeature` (cross-boundary copy) | Copying `SurfaceFeature` across threads with `volume` (`void*`) / `plane_indices` (`vector<int>*`) still pointing at libslic3r memory | Zero out or repoint the raw pointers to Qt-owned handles before crossing the boundary; the copy ctor (`Measure.hpp:36-43`) shallow-copies them |
| QRhi D3D12 backend | Enabling the debug layer in Release; reordering candidates to make D3D12 default | Gate debug layer on `#ifdef QT_DEBUG`; keep D3D11-first in `defaultWindowsCandidates()` (`RhiBackendSelector.cpp:37-46`) — the v3.2 D3D12 startup crash is unresolved |
| QRhi D3D12 command-buffer ordering (BUG-V31-1) | Assuming any upload-after-beginPass is safe because D3D11 tolerates it | All resource updates MUST go into a batch passed to `beginPass` (`RhiViewportRenderer.cpp:281-298`). Audit every `uploadStaticBuffer` call site for ordering |
| QML engine (argv fixture loading) | Calling `topbarImportModel` before `QQmlApplicationEngine::objectCreated` fires | Defer fixture actions until `objectCreated` AND first `frameSwapped` both signaled — guarantees QML tree + first GPU frame done |
| BackendContext services (argv fixtures) | Assuming services are ready synchronously in the `BackendContext` ctor | `topbarImportModel` must assert `projectService_` non-null + `Model*` allocated; return false (not crash) if not ready |
| 256-byte cbuffer alignment (D3D12) | Allocating a cbuffer sized to the struct, not padded to 256 | D3D12 requires 256-byte alignment (HLSL cbuffer); D3D11 tolerates the unpadded size — this ONLY manifests under D3D12 (`RhiViewportRenderer.cpp:1424-1428`). Always round up |

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Per-frame `Measuring` reconstruction (GLGizmoMeasure) | Frame time spikes when cursor is over the viewport; UI feels sluggish | Cache `Measuring` per volume, rebuild only on mesh-change signal | >1 volume or >60Hz mouse events |
| Per-frame full-scene ITS raycast (no coarse pick) | Frame time scales linearly with volume count | Two-stage pick: `ObjectPicking::pickSourceObject` (ray-AABB) then per-triangle on the hit volume only | >3 volumes |
| `convex_hull_3d` called per-slice rebuild (Option B) | Slice-complete-to-render gap grows with tower complexity | Compute the hull ONCE in the worker, deliver the hull mesh in the readback (not the raw mesh); cache until next slice | Acceptable per-slice (offline), NOT per-frame |
| Unconditional `wipeTowerGeometryChanged` emit (v4.4 Phase 100 I2, recurring) | Six QML binding re-evals + `m_wipeTowerDirty` rebuild per slice even when unchanged | Short-circuit `if (changed) emit;` in the slot | Minor at 1 slice/event; matters if filament-map readback fires more frequently |
| Filament-map auto-computation on every plate-config tweak | Re-slice latency on every UI interaction | Only auto-compute during `print.process()` (upstream behavior — `Print.cpp:2487`); do NOT trigger a pre-slice "recommendation" preview unless explicitly scoped | Multi-plate projects with frequent config edits |
| D3D12 debug layer in Release | 20-40% frame-time regression for D3D12-opt-in users | `#ifdef`-gate the enable; add `OWZX_D3D12_DEBUG` env var for opt-in | Any Release build with `OWZX_RHI_RENDERER=d3d12` |
| argv fixture `QTest::qWait` polling | CI flakes under load; 5-50% failure rate | Signal-gate on `objectCreated` + `frameSwapped` | CI runners under load |
| ITS deep-copy on every cross-thread hop (Option B mesh) | Heap alloc + vertex copy per slice | Acceptable (kB-scale, once per slice); do NOT copy per-frame | Matters only if the mesh readback is mistakenly per-frame |

## Security Mistakes

Domain-specific issues beyond general security (this is an offline desktop slicer, so the surface is narrow — focused on crash-stability and file-format integrity).

| Mistake | Risk | Prevention |
|---------|------|------------|
| D3D12 crash ships untriaged, default-`auto` users hit it | App crash on startup for any user whose env selects D3D12; data loss if a project is mid-edit | Keep D3D11-first in `defaultWindowsCandidates()`; never promote D3D12 without the root-cause phase closing |
| 3MF legacy-enum migration skipped | Silent data corruption (Manual plates become AutoForMatch) | Read-side normalization on every 3MF load; schema-version gate |
| `SurfaceFeature::volume` `void*` crosses the boundary with a libslic3r pointer | UAF if the Qt6 side dereferences it after the libslic3r object is gone | Repoint to a Qt-owned volume index (cast to `void*`); document as opaque handle |
| `--load-model` fixture accepts arbitrary paths without normalization | Path-traversal if fixtures ever read from untrusted input (low risk today — fixtures are dev-only) | Canonicalize/validate paths even in fixture mode; the cost is trivial |
| Crash-handler minidumps contain model/project data | PII leakage if minidumps are ever uploaded (no upload path today, but the D3D12 crash fixture generates minidumps) | Document minidump sensitivity; never auto-upload; the `crash_dumps/` dir (`main_qml.cpp:292`) is local-only |

## UX Pitfalls

User-experience mistakes specific to these 5 workstreams.

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| FilamentGroupPopup shows only Auto/Manual after the 4-value widening | User cannot pick AutoForFlush vs AutoForMatch (different optimization targets) | Expose all 4 modes in the popup; default to `fmmAutoForFlush` (upstream `AppConfig.cpp:453-454` default) |
| Auto-recommended map applied silently without an "Apply" affordance | User's manual map overwritten on next slice; confusion | Show recommended map as a preview; require explicit Apply (`setPlateFilamentMaps`). Never auto-overwrite a Manual-mode plate |
| Option B wipe-tower mesh appears without the brim/rib that Option A omitted | User thinks the tower changed shape (it didn't — it gained fidelity) | Document the visual change in release notes; the brim/rib/cone base is the upstream-fidelity gain, not a regression |
| D3D12 promoted to default before root cause is fixed | Random startup crashes for default users | Keep D3D11 default until the D3D12 phase closes with a confirmed root cause AND a passing crash-fixture |
| GLGizmoMeasure feature highlight lags the cursor by a frame | Feels broken; user clicks the wrong feature | Cache `Measuring` per volume (Pitfall 6/7) so feature lookup is sub-frame; never reconstruct on mouse-move |
| argv fixtures documented as "user-facing" deep links | Users discover `--load-model` and file bugs when it does not load instantly | Document fixtures as dev/test-only in `--help`; the user-facing path is the file dialog |

## "Looks Done But Isn't" Checklist

Things that appear complete but are missing critical pieces — verify each during execution.

- [ ] **Filament-map readback:** Often missing the capture-by-value invariant — verify zero `Print*` escapes the worker (grep `Print*` in `SliceService.cpp` queued-lambda capture list; mirror Phase 100 self-check)
- [ ] **Filament-map readback:** Often missing the `valid=false` gate path — verify that when `get_filament_maps()` returns empty (single-extruder), the slot forces the popup to a safe state without leaking stale data (mirror WTREAD-02)
- [ ] **Enum widening:** Often missing the 3MF legacy-file migration — verify a pre-v4.5 project reloads with Manual plates still Manual (round-trip test)
- [ ] **Enum widening:** Often missing FilamentGroupPopup mode-selector coverage — verify all 4 modes are selectable, not just Auto/Manual
- [ ] **Option B mesh:** Often missing the `wipe_tower_mesh_data == nullopt` fallback — verify single-material slice still renders Option A box (or no tower, per `has_wipe_tower()`)
- [ ] **Option B mesh:** Often missing the convex_hull_3d call — verify the rendered mesh is the hull, not the raw toolpath (visual diff against upstream)
- [ ] **Option B mesh:** Often missing the index buffer in `byteSize` — verify `ensureBuffer` is called for BOTH vertex and index buffers
- [ ] **Option B mesh:** Often missing SoftwareViewport mirror — verify the Software path renders the same mesh (v4.4 closure required mirror parity)
- [ ] **argv fixtures:** Often missing the readiness gate — verify the fixture waits for `objectCreated` + `frameSwapped`, not just `singleShot(0)`
- [ ] **argv fixtures:** Often missing the D3D12-crash-repro sub-case — verify a minidump is generated and has the expected exception code
- [ ] **D3D12:** Often missing the Release-build debug-layer gate — verify `enableDebugLayer` is `#ifdef`-guarded
- [ ] **D3D12:** Often missing root-cause isolation distinct from the BUG-V31-1 comment — verify the phase identifies WHICH upload site triggers (the comment's fix is already in place at `:286-296`, so a NEW crash needs a NEW root cause)
- [ ] **D3D12:** Often missing Vulkan-is-SDK-blocked documentation — verify the phase does NOT attempt to unblock Vulkan
- [ ] **GLGizmoMeasure:** Often missing the per-volume ITS cache — verify a `shared_ptr<indexed_triangle_set>` ownership layer exists before any `Measuring` construction
- [ ] **GLGizmoMeasure:** Often missing the mesh-change rebuild signal — verify transforms that bake the mesh trigger `Measuring` rebuild; transient transforms do NOT
- [ ] **GLGizmoMeasure:** Often missing two-stage picking — verify `ObjectPicking::pickSourceObject` runs before any per-triangle ITS raycast
- [ ] **GLGizmoMeasure:** Often missing the `SurfaceFeature` raw-pointer scrub — verify `volume`/`plane_indices` are repointed to Qt-owned handles before crossing the boundary
- [ ] **AssembleViewDataPool clipper:** Often missing the `ModelObjectsClipper` registration — verify the `AssembleViewDataID::ModelObjectsClipper = 1 << 4` slot (`AssembleViewDataPool.h:44`) is registered in the pool ctor and has a getter
- [ ] **FilamentGroupPopup:** Often missing the viewmodel API — verify Q_INVOKABLE set/get methods exist on a viewmodel, not QML-local state
- [ ] **All workstreams:** Often missing the ViewModelSmokeTests round-trip case — verify each new viewmodel API has a set→get→assert test modeled on `wipeTowerGeometryReadbackAppliesValidAndInvalidGate`

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Filament-map `Print*` escapes worker (Pitfall 1) | MEDIUM | Revert to POD capture-by-value; add the Phase 100 self-check to the phase SUMMARY; rerun smoke test with a deliberate delayed-emit to force the race |
| Enum-widening corrupted persisted files (Pitfall 2) | HIGH | Write a one-way migration: scan 3MFs, detect legacy `filament_map_mode=1` written by pre-v4.5, rewrite as `2`; bump schema version. Data recovery from backups if migration is not deterministic |
| Option A baseline regressed (Pitfall 3) | LOW | Restore `buildWipeTowerVertices` + `uploadWipeTowerBuffer` from v4.4; move Option B to a NEW function; rerun Phase 102 anchor test |
| argv fixture race flakes CI (Pitfall 4) | LOW | Add the `objectCreated` + `frameSwapped` gate; replace `qWait` with signal-wait; mark flaky tests as `ExpectFail` until gated |
| D3D12 debug layer shipped in Release (Pitfall 5) | LOW | Add the `#ifdef` gate; rebuild Release; verify frame-time regression test passes |
| `Measuring` UAF from detach (Pitfall 6) | MEDIUM | Introduce the `shared_ptr<ITS>` cache; reconstruct all `Measuring` instances from the cache; add a LSAN/ASAN run to catch the residual UAF |
| Raycaster frame-time spike (Pitfall 7) | MEDIUM | Add the coarse-pick stage; cache `Measuring` per volume; add the frame-time regression test to prevent recurrence |
| FilamentGroupPopup QML-only state (Pitfall 8) | LOW | Move state to viewmodel; rewire QML to Q_INVOKABLE; add the round-trip smoke test |

## Pitfall-to-Phase Mapping

How the v4.5 roadmap phases should address these pitfalls. Phase numbers are suggested (the actual v4.5 phases start at 103 per `PROJECT.md:63`).

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Pitfall 1 — Filament-map capture-by-value | Filament-map readback phase (analog of v4.4 Phase 100) | Grep `Print*` in `SliceService.cpp` queued-lambda capture list = 0 hits; smoke test mirrors `wipeTowerGeometryReadbackAppliesValidAndInvalidGate` |
| Pitfall 2 — Enum-widening 3MF migration | Enum-widening phase (FIRST phase of filament-map workstream, before auto-computation) | Round-trip test: write Manual plate, reload, assert mode==2; legacy-file fixture if available |
| Pitfall 3 — Option A baseline regression | Option B render phase (analog of v4.4 Phase 101) | Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent` test still passes unchanged; Option A path fires when `wipe_tower_mesh_data==nullopt` |
| Pitfall 4 — argv fixture startup race | CLI fixtures phase (workstream 3) — readiness gate is the FIRST deliverable | Screenshot fixture captures the model on the first attempt, no `qWait`; D3D12 crash fixture generates a minidump with the expected exception code |
| Pitfall 5 — D3D12 debug-layer Release leak | D3D12 investigation phase (workstream 4) — split into root-cause + promotion sub-phases | `enableDebugLayer` is `#ifdef`-gated; Release frame-time test passes; root cause is a NEW isolation, not just the BUG-V31-1 comment |
| Pitfall 6 — GLGizmoMeasure ITS lifetime | GLGizmoMeasure engine phase (workstream 5) — ITS cache contract is the FIRST deliverable | `shared_ptr<ITS>` cache exists; `Measuring` rebuilt only on mesh-change signal; `SurfaceFeature` raw pointers scrubbed at boundary |
| Pitfall 7 — Raycaster per-frame cost | GLGizmoMeasure engine phase (workstream 5) — performance acceptance in the verification sub-phase | Frame-time regression test: N=10 volumes, cursor over viewport, frame time < 16ms |
| Pitfall 8 — FilamentGroupPopup QML-only state | Filament-map UI phase (second half of workstream 1) | Popup has no `property var` state; Q_INVOKABLE set/get on viewmodel; round-trip smoke test passes |

## Cross-Workstream Dependencies

Pitfalls that span multiple workstreams and must be coordinated by the roadmapper.

| Dependency | Upstream Phase | Downstream Phase | Coordination Rule |
|------------|----------------|------------------|-------------------|
| Readback POD pattern (Pitfall 1) | Filament-map readback phase | Option B mesh readback (Pitfall 3) | Option B mesh readback reuses the SAME capture-by-value invariant; if filament-map ships first, Option B inherits the pattern. If they ship in parallel, BOTH must cite Phase 100. |
| ITS cache (Pitfall 6) | GLGizmoMeasure engine phase | AssembleViewDataPool ModelObjectsClipper (Pitfall 6, same workstream) | The `shared_ptr<ITS>` cache is shared between the Measure engine and the Clipper resource. They MUST be in the same phase or paired phases with a shared design doc. |
| argv fixture readiness gate (Pitfall 4) | CLI fixtures phase | D3D12 crash-repro sub-fixture (Pitfall 5) | The D3D12 crash fixture uses the fixture infrastructure's readiness contract. Fixtures phase ships first OR the two phases agree on the gate contract in their PLAN.md files. |
| Enum widening (Pitfall 2) | Filament-map enum phase | FilamentGroupPopup UI (Pitfall 8) | The popup's mode-selector UI depends on the 4-value enum. Enum phase MUST ship first; popup phase consumes the widened enum. |
| Option A fallback (Pitfall 3) | Option B render phase | (none — internal to Option B) | The Option A fallback and Option B path ship in the SAME phase so the fallback is tested, not retrofitted. |

## Sources

- **v4.4 Phase 100 SUMMARY** (`.planning/milestones/v4.4-phases/100-wipe-tower-geometry-readback/100-01-SUMMARY.md`) — the capture-by-value invariant (Frozen Decision 1), the `WipeTowerGeometry` POD pattern, the `wipeTowerGeometryReady` signal wiring, the C2248 `private slots:` placement bug, the libslic3r-reconfigure timeout pattern. Direct template for Pitfall 1.
- **v4.4 Phase 100 REVIEW** (`.planning/milestones/v4.4-phases/100-wipe-tower-geometry-readback/100-REVIEW.md`) — W1 corner-vs-center coordinate mismatch (the geometric-convention pitfall pattern), I1 captured-but-unused struct fields, I2 unconditional NOTIFY emit. Motivates Pitfall 3's "do not change `buildWipeTowerVertices` signature" and Pitfall 1's "capture by value including mesh."
- **v4.3 Phase 97 REVIEW** (`.planning/milestones/v4.3-phases/97-thumbnail-save-reload-round-trip/97-REVIEW.md`) — the `ConfigOptionBool` does NOT override `getInt` gotcha (THUMBRT-01), the in-loop `setThumbnail`-before-`arrangeObjects` ordering bug. Motivates the Integration Gotchas `ConfigOptionBool` row and the enum-widening read-side discipline (Pitfall 2).
- **PROJECT.md** (`.planning/PROJECT.md`) — constraints (OpenVDB unavailable, FFmpeg unavailable, CGAL available), the v4.4 carry-forward list (Option B LOCKED, D3D12 deferred, GLGizmoMeasure deferred, CLI fixtures deferred), the Vulkan SDK feature-flag block (`:143`), the architecture rule (business logic in C++, QML is presentation). Motivates Pitfall 5 (Vulkan scope), Pitfall 8 (architecture rule).
- **STATE.md** (`.planning/STATE.md`) — deferred items table, the recurring Windows-capture-API blocker (FIXTURE-02), the recurring build-timeout pattern. Motivates Pitfall 4 (fixtures are the workaround for the capture blocker).
- **AGENTS.md** — build rules (canonical command, single build dir), source-truth migration rules, the QML-boundaries rule. Motivates Pitfall 8.
- **SliceService.cpp:490-673** — the worker `try` block showing the Print lifetime window (`:513 Slic3r::Print print` → `:590 print.process()` → `:642-659 wipe-tower capture` → `:664 activePrint_.store(nullptr)`). The filament-map readback (Pitfall 1) and Option B mesh readback (Pitfall 3) MUST fit in this window.
- **RhiBackendSelector.cpp** — the D3D11-first policy (`:37-46`), the D3D12 crash comment (`:39-41`), the probeBackend function (`:63-87`), the `QRhiD3D12InitParams` member (`:22`). Motivates Pitfall 5.
- **RhiViewportRenderer.cpp:1064-1097** — `uploadWipeTowerBuffer` (the Option A upload path that must not regress), `:281-298` BUG-V31-1 comment (the D3D12 command-buffer-ordering crash), `:1424-1428` 256-byte cbuffer alignment. Motivates Pitfall 3 and Pitfall 5.
- **GizmoGeometry.cpp:449-535** — `buildWipeTowerVertices` (the Option A 36-vertex box baseline), the Option B LOCKED-future-upgrade comment block (`:473-486`), the center-convention input contract (`:465-471`, the W1 fix). Motivates Pitfall 3.
- **Measure.hpp** — `Measuring(const indexed_triangle_set&)` ctor (`:122`), the pimpl `MeasuringImpl` (`:143`), `SurfaceFeature` copy ctor with raw `void* volume` / `vector<int>* plane_indices` (`:36-43, 95-99`). Motivates Pitfall 6.
- **GLGizmoMeasure.cpp** (upstream) — the `m_mesh_raycaster_map` per-volume loop (`:600-615`), the `unproject_on_mesh` per-mouse-move pattern (`:606`), the `m_curr_measuring` rebuild on volume change (`:615`), the `MeshRaycaster`-per-primitive construction (`:183,188,526,552`). Motivates Pitfall 6 and Pitfall 7.
- **3DScene.cpp:887-925** (upstream) — `load_real_wipe_tower_preview`: the `mesh.convex_hull_3d()` call (`:914`), the `real_wipe_tower_mesh` + `real_brim_mesh` merge (`:906-909`), the `v.is_wipe_tower = true` flag (`:923`). Direct template for Option B (Pitfall 3).
- **Print.cpp:2484-2491** (upstream) — the auto-filament-map computation inside `print.process()`: `if (map_mode < fmmManual) { filament_maps = ToolOrdering::get_recommended_filament_maps(...); update_filament_maps_to_config(filament_maps); }`. Confirms the auto-map is written back into config during slicing, NOT a separate output — motivates the readback pattern (Pitfall 1) and the enum-widening precedence (Pitfall 2).
- **Print.hpp:740-790** (upstream) — `WipeTowerData` struct: `wipe_tower_mesh_data` is `std::optional<WipeTowerMeshData>` (`:766`), `clear()` resets it to `nullopt` (`:776`), the struct is non-copyable (`:782-783` delete). Motivates the Option B nullopt-fallback (Pitfall 3).
- **PrintConfig.hpp:424-429** (upstream) — the 4-value `FilamentMapMode` enum (`fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2, fmmDefault=3`). Direct source for Pitfall 2.
- **ProjectServiceMock.cpp:610-627, 4991-4997, 5409-5420** — the Phase 97 `spiral_mode` `ConfigOptionBool` fix, the `filament_maps` 3MF write path (`setInt`), the `filament_maps` 3MF read path. Direct evidence for the Integration Gotchas and Pitfall 2.
- **PartPlate.h:200-273** — the current 2-value Qt6 enum (`0=Auto, 1=Manual`), the `setFilamentMaps/setFilamentMapMode` API. The widening target (Pitfall 2) and the FilamentGroupPopup backing API (Pitfall 8).
- **AssembleViewDataPool.h** — the reserved `ModelObjectsClipper = 1 << 4` slot (`:44`), the deferred-resource documentation (`:23-28`), the `needs per-volume ITS` note. Motivates Pitfall 6's cross-workstream dependency with the Clipper.
- **BBLTopbar.qml:396-397** — the FilamentGroupPopup visual-only placeholder with `TODO(v2.1)`. Motivates Pitfall 8.
- **main_qml.cpp:177-257, 259-320** — the `--load-model`/`--open-page`/`--open-dialog` argv parsing, the `singleShot(0)` readiness trick, the RHI backend selection order. Direct evidence for Pitfall 4.

---
*Pitfalls research for: adding 5 workstreams (filament-map, Option B mesh, CLI fixtures, D3D12, GLGizmoMeasure) to OWzx Slicer post-v4.4*
*Researched: 2026-07-12*
