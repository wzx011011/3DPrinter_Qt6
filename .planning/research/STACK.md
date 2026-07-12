# Stack Research

**Domain:** Qt6/QML desktop slicer (OrcaSlicer migration) — v4.5 backlog closure (5 workstreams)
**Researched:** 2026-07-12
**Confidence:** HIGH

This research answers the 5 stack questions for the NEW v4.5 workstreams only.
It does NOT re-research validated v4.4-and-earlier capabilities (documented in
PROJECT.md "Validated" + the v4.4 99-GAP-MATRIX Frozen Decisions).

## TL;DR — Stack Verdict Per Workstream

| # | Workstream | New dependency needed? | Verdict |
|---|---|---|---|
| 1 | Auto filament-map recommendation | **No** | libslic3r `ToolOrdering::get_recommended_filament_maps` + `Print::get_filament_map_mode()` already available; widen Qt enum 2→4 value; no new Qt module |
| 2 | Option B real wipe-tower mesh | **No** | libslic3r `wipe_tower_mesh_data` + `TriangleMesh::convex_hull_3d()` available; extend `GizmoVertex` layout (no new lib) |
| 3 | CLI fixtures + argv GUI loading | **No** | `QCommandLineParser`/`QCommandLineOption` ALREADY wired in `main_qml.cpp:170-203`; fixture work is additive options, not a new library |
| 4 | D3D12 crash root cause + backend eval | **Qt6 RHI debug API only** | `QRhiD3D12InitParams` already used in `RhiBackendSelector.cpp`; debug layers + PIX are investigation tooling (enable via env/SDK), NOT shipped dependencies |
| 5 | Full GLGizmoMeasure engine + clipper | **No** | Upstream `SceneRaycaster`/`MeshRaycaster` (MeshUtils.hpp) + `Measure::Measuring` (libslic3r/Measure.hpp) all compile under the existing libslic3r target; PORT, do not add |

**Net: zero new external libraries.** All 5 workstreams are served by (a)
existing libslic3r APIs, (b) existing Qt 6.10 modules already linked, and (c)
two local code extensions (GizmoVertex ITS layout + the scene-raycaster port).
OpenVDB/FFmpeg constraints are NOT touched by any workstream.

---

## Recommended Stack

### Core Technologies (already validated — DO NOT re-add)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Qt 6.10 (Qml/Quick/QuickControls2/Gui/GuiPrivate/OpenGL/Concurrent/Network/Test/LinguistTools/ShaderTools) | 6.10 | GUI shell, RHI viewport, viewmodels, tests | Project standard (`CMakeLists.txt:63,66`); QCommandLineParser + QQuickRhiItem live here |
| libslic3r (from-source build) | v7.0.1 upstream | Slicing engine: Print, WipeTowerData, ToolOrdering, Measure, TriangleMesh, indexed_triangle_set | Sole slicing engine; preserved unchanged (PROJECT.md Out-of-Scope) |
| QRhi (Qt GUI private + rhi/qrhi.h) | Qt 6.10 bundled | Default D3D11 renderer (owns gizmo/pick/cut/wipe/Preview/thumbnail) | v3.8 default; D3D12 candidate backend via `RhiBackendSelector.cpp` |
| CGAL (libslic3r_cgal) | 5.4 (have; 5.6+ needed for some mesh ops) | Computational geometry used by libslic3r | Available per PROJECT.md; NOT consumed by any v4.5 workstream directly (convex_hull_3d uses qhull, not CGAL) |
| qhull / qhullcpp | upstream prebuilt | Convex hull for `TriangleMesh::convex_hull_3d()` (Option B source) | Already linked via libslic3r deps; no action |

### Supporting Libraries (per-workstream, all already available)

| Library / API | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `QCommandLineParser` + `QCommandLineOption` | Qt 6.10 (Qt Core) | argv fixture loading for deterministic GUI deep links | Workstream 3 ONLY — already wired (`main_qml.cpp:172-203`); add new options for repeatable screenshot fixtures |
| `Print::wipe_tower_data().wipe_tower_mesh_data` (`std::optional<WipeTowerMeshData>`) | libslic3r v7.0.1 (`Print.hpp:742-747,766`) | Real wipe-tower + brim mesh (Option B source) | Workstream 2 ONLY — extract `real_wipe_tower_mesh` + `real_brim_mesh` `TriangleMesh` |
| `TriangleMesh::convex_hull_3d()` / `TriangleMesh::merge()` | libslic3r (`TriangleMesh.hpp:122,136`) | Build hull from merged wipe+brim mesh (mirrors upstream `3DScene.cpp:908-914`) | Workstream 2 ONLY — the upstream Option B algorithm |
| `TriangleMesh::its` (`indexed_triangle_set`) | libslic3r (`MeshDAO.hpp`/`TriangleMesh.hpp`) | Per-triangle vertex+index data for arbitrary meshes | Workstream 2 (wipe mesh upload) + Workstream 5 (per-volume ITS for Measure::Measuring) |
| `ToolOrdering::get_recommended_filament_maps()` | libslic3r (`ToolOrdering.hpp:244`, impl `:1107`) | Auto-compute per-plate filament map | Workstream 1 ONLY — libslic3r already computes this during slicing; Qt side reads back |
| `Print::get_filament_map_mode()` | libslic3r (`Print.hpp:997`) | Returns current `FilamentMapMode` enum | Workstream 1 ONLY — enum widening reference |
| `SceneRaycaster` / `MeshRaycaster` | upstream GUI (`SceneRaycaster.hpp` / `MeshUtils.hpp`) | Camera-ray → triangle hit for feature picking | Workstream 5 ONLY — PORT from upstream, not a dependency add |
| `Measure::Measuring` (constructed from `indexed_triangle_set`) | libslic3r (`Measure.hpp:119-140`) | Point/edge/circle/plane feature extraction | Workstream 5 ONLY — already compiled into libslic3r; Qt side instantiates |
| `QRhiD3D12InitParams` | Qt 6.10 RHI (`rhi/qrhi_platform.h`) | D3D12 backend probe/selection | Workstream 4 ONLY — already used in `RhiBackendSelector.cpp:22` |

### Development Tools (investigation-only, NOT shipped)

| Tool | Purpose | Notes |
|------|---------|-------|
| D3D12 Debug Layer | Enable `D3D12GetDebugInterface`-level device-removal diagnostics for the startup crash | Enable via the Windows "Graphics Tools" optional feature + the PIX-on-Windows target in `QRhiD3D12InitParams`; runtime toggle, not a build dependency. NONE of this is wired in `src/` today (grep `enableDebugLayer`/`PIX`/`DebugLayer` returns zero hits in `src/`) — Workstream 4 wires it behind an env flag |
| PIX on Windows | GPU frame capture + D3D12 device-removal reason | Run against `build/OWzxSlicer.exe` with `OWZX_RHI_RENDERER=d3d12`; the existing `RhiBackendSelector::probeBackend` already creates a D3D12 `QRhi` probe, so a PIX capture is mechanically possible once the debug layer is enabled. Not integrated |
| Vulkan SDK (Vulkan backend readiness eval only) | Evaluate whether a Vulkan-enabled Qt SDK could replace D3D11 default | Currently BLOCKED: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES` (PROJECT.md Context). A Vulkan eval needs a custom Qt build with Vulkan enabled; do NOT attempt on the production SDK |
| Windows Graphics Tools (dxcap.exe / DXGI_INFO_QUEUE) | Capture API + DXGI error surfacing for the recurring Windows-capture-API blocker (FIXTURE-02) | The argv fixtures from Workstream 3 are the project's chosen workaround (PROJECT.md Key Decisions); Graphics Tools is supplementary |

## Installation

```bash
# Core — NO INSTALL NEEDED. All components are already linked:
#   Qt6 components: CMakeLists.txt:63,66 (find_package Qt6 REQUIRED COMPONENTS
#     Qml Quick QuickControls2 LinguistTools OpenGL Concurrent Test Gui
#     GuiPrivate ShaderTools)
#   Qt6 link targets: CMakeLists.txt:265-266,278-279 (owzx_app_core + OWzxSlicer)
#   libslic3r: cmake/BuildLibslic3rFromSource.cmake (from upstream source)
#   QRhi private headers: Qt6::GuiPrivate already linked

# Workstream 4 only — investigation tooling (NOT a build dependency):
#   1. Windows "Graphics Tools" optional feature (Settings > Apps > Optional
#      features > Add "Graphics Tools") — enables D3D12 debug layer + dxcap.
#   2. PIX on Windows (Microsoft Store) — GPU capture; launch against
#      build/OWzxSlicer.exe with OWZX_RHI_RENDERER=d3d12.

# Verification build (ONLY canonical command per AGENTS.md):
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Workstream-by-Workstream Stack Detail

### Workstream 1 — Auto filament-map recommendation

**Question answered: does any new Qt module / library need to be added?** No.

- **libslic3r compute path:** `Print::process()` already calls
  `ToolOrdering::get_recommended_filament_maps(...)` when
  `get_filament_map_mode() < fmmManual` (`Print.cpp:2485-2488`). The result is
  the auto-computed per-plate filament map. Qt does NOT re-implement the
  algorithm — it reads the result back.
- **Enum widening (2-value → 4-value).** Current Qt: `PartPlate.h:273`
  documents `m_filament_map_mode` as `0=Auto, 1=Manual` (2-value). Upstream
  truth (`PrintConfig.hpp:424-429`) is a **4-value** enum:
  ```cpp
  enum FilamentMapMode {
      fmmAutoForFlush,   // 0
      fmmAutoForMatch,   // 1
      fmmManual,         // 2
      fmmDefault         // 3
  };
  ```
  (The milestone-context "4-value" claim is confirmed.) The Qt enum/int is
  widened to carry all 4 values; serialization already writes the config key
  as a string via `ConfigOptionEnum<FilamentMapMode>` in
  `bbs_3mf.cpp:7964-7967`, so round-trip compatibility is preserved by
  mapping the widened int to the upstream enum-name string.
- **FilamentGroupPopup UI modules:** the popup is a plain QML component.
  QML imports already in use project-wide (grep `^import`) are:
  `QtQuick`, `QtQuick.Controls`, `QtQuick.Layouts`, `QtQuick.Dialogs`,
  `QtQuick.Window`, `OWzxGL 1.0`, and project-relative control/component
  dirs. `FilamentGroupPopup` needs NO new Qt module — `QtQuick.Controls`
  (Popup/ComboBox/TextField), `QtQuick.Layouts` (GridLayout), and the
  existing `controls/` Cx* components (CxButton/CxComboBox/CxTextField) cover
  it. `BBLTopbar.qml:396-397` already documents the popup as a TODO placeholder
  that becomes the real implementation here.
- **Readback integration point:** `SliceService.cpp` worker already reads
  `print.has_wipe_tower()` / `print.wipe_tower_data()` post-slice (Phase 100,
  commit f829f72). The auto-filament-map readback follows the same
  capture-by-value pattern (Frozen Decision 1 invariant: no `Print*` escapes
  the worker). Add a sibling POD field + signal; mirror the v4.4
  `wipeTowerGeometryReady` wiring.

### Workstream 2 — Option B real wipe-tower mesh

**Question answered: ITS vertex format situation for GizmoGeometry + RhiViewportRenderer.**

- **libslic3r source is fully available.** `wipe_tower_mesh_data`
  (`Print.hpp:766`, `std::optional<WipeTowerMeshData>`) is populated by
  `WipeTowerData::construct_mesh(...)` (`Print.cpp:5070-5087`) which builds
  `real_wipe_tower_mesh` + `real_brim_mesh` as `TriangleMesh` objects.
  `wipe_tower_mesh_data` MAY be `std::nullopt` (its `clear()` resets it,
  `Print.hpp:776`) — the Option B path MUST handle nullopt by falling back to
  the Option A box (v4.4 baseline, preserved).
- **The ITS extension is a LOCAL code change, not a library add.** Current
  `buildWipeTowerVertices` (`GizmoGeometry.cpp:487-535`) emits a fixed
  36-vertex `{x,y,z,r,g,b,a}` (7-float / 28-byte `GizmoVertex`) box. Option B
  needs a builder that accepts an arbitrary `indexed_triangle_set` and emits
  the same `GizmoVertex` layout (one vertex per triangle-corner, color baked
  in upstream-style per-extruder). The layout (`GizmoVertex.h`, shared with
  `RhiViewportRenderer::Vertex` typealias at `RhiViewportRenderer.h:29`) does
  NOT change — only the vertex COUNT and POSITIONS become arbitrary. The
  upload path (`uploadWipeTowerBuffer`, `RhiViewportRenderer.cpp:1064-1095`)
  already uses `QRhiBuffer` with dynamic size + `m_wipeTowerDirty` rebuild,
  so an arbitrary-vertex buffer is structurally supported; the change is
  feeding ITS vertices instead of the 36-vertex box array.
- **Upstream algorithm (source-truth to mirror).** `3DScene.cpp:887-925
  load_real_wipe_tower_preview`:
  - `:891` early-return if `wt_mesh.its.vertices.empty()`
  - `:906-909` `auto mesh = wt_mesh; if (render_brim) mesh.merge(brim_mesh);`
  - `:914` `TriangleMesh wipe_tower_shell = mesh.convex_hull_3d();`
  - `:915` `v.model.init_from(wipe_tower_shell);`
  - `:916` raycaster built from the hull
  
  The Qt mirror: read `wipe_tower_mesh_data->real_wipe_tower_mesh` +
  `real_brim_mesh` in the SliceService worker, capture the `its` (vertices +
  indices) by value into the WipeTowerGeometry POD (extend it), build the
  convex hull on the GUI thread OR capture the hull `its` directly (lower
  risk — avoids re-entrant libslic3r calls off-worker), feed the hull `its`
  into a new `buildWipeTowerVerticesFromIts(...)` in `GizmoGeometry`, upload
  via the existing `m_wipeTowerBuffer` path. `TriangleMesh::convex_hull_3d()`
  (`TriangleMesh.hpp:136`) and `TriangleMesh::merge()` (`:122`) are the two
  libslic3r calls; both use qhull (available).
- **SoftwareViewport mirror:** the parallel QPainter path
  (`SoftwareViewport.cpp:207-253`) draws a box today. Option B can either
  (a) keep the box approximation on the software path (acceptable — software
  is the fallback, not the default), or (b) draw a convex-hull polygon.
  Recommend (a) for v4.5; the RHI path is the source-truth surface.

### Workstream 3 — CLI fixtures + deterministic argv GUI fixture loading

**Question answered: does Qt6.10 have argv parsing utilities suitable for GUI fixture loading, or should we use QCommandLineOption?**

**Use `QCommandLineParser` + `QCommandLineOption` — they are ALREADY wired.**

- **Current wiring** (`main_qml.cpp:170-203`, shipped v4.1/v4.2): four options
  are parsed before the QML engine loads:
  - `--open-page <name>` (single value; routed via `startupPageRoutes()` :123-136)
  - `--open-dialog <name>` (REPEATABLE via `parser.values()` :201; routed via
    `startupDialogRoutes()` :138-168 — printer/filament/process settings,
    config-wizard, bed-shape, ams, firmware, speed-limit, wipe-tower,
    print-host, plugin-manager, lite-mode)
  - `--load-model <path>` (REPEATABLE via `parser.values()` :202; loaded then
    navigates to Prepare)
  - `--skip-first-run` (boolean flag via `parser.isSet()` :203)
- **Why QCommandLineParser is the right choice (not custom):**
  - It is the Qt-standard argv parser (Qt Core, no extra module); stable API
    across Qt 5.x/6.x/6.10 (Qt docs QCommandLineParser Class).
  - `QCommandLineOption::setNames()` gives multi-value (`--open-dialog a
    --open-dialog b`) which is exactly what fixture repeatable-dialog loading
    needs — already proven working in v4.2 (`parser.values(openDialogOption)`).
  - `parser.process(app)` integrates with Qt's own `--help`/`--version` (added
    via `addHelpOption()`/`addVersionOption()` at :174-175) and Qt's internal
    arg handling (e.g. `QSG_RHI_*` env-style plumbing).
  - Custom argv parsing would reinvent option-name normalization (the project
    already has `normalizeStartupToken()` at :105-110 for `_`↔`-` + case fold),
    repeatable-value collection, and help/version — all already solved.
- **Workstream 3 work is ADDITIVE options + fixture model files, not a parser rewrite.**
  - Add new fixture options (e.g. `--fixture-dir`, `--auto-screenshot`,
    `--exit-after-load`) to the existing `parseStartupOpenRequest()` :170 block.
  - The FIXTURE-02 Windows-capture-API workaround (recurring blocker) is
    unblocked by v4.3's shared `store_bbs_3mf` writer (PROJECT.md Future);
    the fixture 3MFs already load via the existing `--load-model` path.
- **Alternative considered — QCoreApplication::arguments() + manual parse:**
  rejected. It loses help/version integration, normalization, and
  multi-value collection; the existing parser already provides all three.

### Workstream 4 — D3D12 crash root cause + backend readiness evaluation

**Question answered: what D3D12 debugging tooling exists, and is any of it already wired?**

- **Already wired:**
  - `RhiBackendSelector.h/.cpp` — env-driven backend selection
    (`OWZX_RHI_RENDERER=d3d11|d3d12|auto`). Default Windows candidates are
    D3D11-first (D3D12 has the known startup segfault), explicit D3D12 via
    `OWZX_RHI_RENDERER=d3d12` (`RhiBackendSelector.cpp:38-46`).
  - `QRhiD3D12InitParams d3d12Params` + `QRhiD3D11InitParams d3d11Params`
    in `RhiProbeOwner` (`RhiBackendSelector.cpp:21-25`).
  - `RhiViewport.cpp:47` `setSampleCount(4)` (MSAA on the default D3D11 path;
    relevant because the D3D12 crash may be depth-stencil/sample-related).
  - Probe + diagnostics logging (`RhiBackendSelection::diagnostics()`) emits
    `attempts=[d3d11:ok,d3d12:failed(...)]` style strings — already useful
    for the crash investigation.
- **NOT wired (Workstream 4 adds these behind an env flag, NOT shipped):**
  - **D3D12 Debug Layer.** Zero references to `enableDebugLayer` / `DebugLayer`
    / `D3D12GetDebugInterface` in `src/` (grep confirmed). To enable: set
    `QRhiD3D12InitParams::enableDebugLayer = true` (Qt RHI D3D12 init param)
    conditionally when a debug env var is set, before `QRhi::create(D3D12,...)`.
    Requires the Windows "Graphics Tools" optional feature installed.
  - **PIX on Windows.** Zero references to `PIX`/`WinPixEventRuntime` in
    `src/` or `CMakeLists.txt` (grep confirmed — the only `PIX` hits are
    `AV_PIX_FMT_*` in CameraStream, unrelated). PIX is a runtime capture tool
    launched externally against `build/OWzxSlicer.exe`; no code integration
    needed for a crash capture (device-removal reason is surfaced via the
    debug layer). Optional `ID3D12Device::SetStablePowerState` +
    `BeginEvent`/`EndEvent` annotations can be added later if capture
    readability matters.
  - **DXGI_INFO_QUEUE** (for detailed DXGI errors). Not wired; available via
    `IDXGIInfoQueue` once the debug layer is on.
- **Vulkan backend readiness.** BLOCKED on the Qt SDK, not on app code:
  `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under
  `QT_DISABLED_PUBLIC_FEATURES` (PROJECT.md Context). A Vulkan eval requires a
  custom Qt 6.10 build with Vulkan enabled — out of scope for the production
  SDK. Do NOT add `QRhi Vulkan` plumbing expecting it to work; it will fail
  at `QRhi::create(Vulkan,...)` on the current SDK.

### Workstream 5 — Full GLGizmoMeasure engine + AssembleViewDataPool clipper

**Question answered: what scene-raycaster infrastructure exists for feature-picking?**

**Status: NO raycaster infrastructure exists in `src/`. It must be PORTED from upstream.**

- **Current state (grep confirmed):** zero `MeshRaycaster` / `SceneRaycaster`
  / `raycast` / `unproject` references in `src/`. The existing
  `ObjectPicking.h` (consumed by `RhiViewport.cpp:5`) is a SCREEN-SPACE pick
  (the v3.8 RHI picking path), NOT a triangle-ray intersection. Feature
  picking (point/edge/circle/plane) needs true mesh raycasting.
- **Upstream source-truth (all compile under the existing libslic3r target):**
  - `SceneRaycaster` (`third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp`)
    — typed hit results (Bed/Volume/Gizmo/FallbackGizmo), ID encoding,
    `hit(mouse_pos, camera, clipping_plane)`. Depends on `MeshRaycaster`.
  - `MeshRaycaster` (`third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp`)
    — `unproject_on_mesh(...)` (:178) and `closest_hit(...)` (:208).
    Constructed from a `TriangleMesh` (which exposes `.its`).
  - `Measure::Measuring` (`third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119`)
    — `explicit Measuring(const indexed_triangle_set& its)` (:122). This is
    the feature-extraction engine (point/edge/circle/plane). ALREADY compiled
    into the from-source libslic3r target (`Measure.cpp` is part of the
    libslic3r build). The Qt side needs only to instantiate it with the
    per-volume `its`.
  - `GLGizmoMeasure.hpp` shows the integration:
    `using PickRaycaster = SceneRaycasterItem;` (:79),
    `std::map<GLVolume*, std::shared_ptr<Measure::Measuring>> m_mesh_measure_map;`
    (:159) — one `Measuring` per volume, keyed by the volume.
- **Per-volume ITS access in Qt6.** `ProjectServiceMock.cpp` already accesses
  `vol->mesh().its` (e.g. :2188, :3010, :3371) and constructs
  `indexed_triangle_set` locally (:2267, :2683, :3922). The pattern for
  Workstream 5: extract each selected volume's `its`, hand it to a Qt-side
  `Measure::Measuring` wrapper, run `MeshRaycaster::unproject_on_mesh` against
  the mouse ray to get the hit point/triangle, then query the `Measuring`
  object for the nearest feature.
- **Port, not add.** The `SceneRaycaster` + `MeshRaycaster` code is
  wxWidgets/OpenGL-flavored upstream but the ray-triangle math is pure
  (Eigen + libslic3r `TriangleMesh`). The port produces a Qt-side
  `SceneRaycaster` in `src/core/rendering/` (pure CPU, unit-testable, matching
  the v3.8 `GizmoGeometry`/`GizmoMath`/`ObjectPicking` pattern). NO new
  external library — only Eigen (already linked) + libslic3r headers.
- **AssembleViewDataPool clipper.** `AssembleViewDataPool.h` reserves the
  `ModelObjectsClipper` enum slot (bit 4) but the resource is intentionally
  NOT registered (comment :40-44). The upstream `MeshClipper` lives in
  `GLGizmosCommon.hpp` (not a standalone file; grep found no separate
  Clipper file). Porting it needs per-volume ITS — the SAME dependency as the
  Measure engine — so Workstream 5 unblocks both. OpenVDB is NOT involved
  (MeshClipper operates on `indexed_triangle_set`, not VDB; the OpenVDB
  dependency is only for the hollow/support paint gizmos, which are removed
  scope).

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| `QCommandLineParser` (Workstream 3) | Custom `argv` parse / `QCoreApplication::arguments()` loop | Never for this project — the parser is already wired, handles multi-value + normalization + help/version. Custom only if a fixture needed pre-`QGuiApplication` mode-switching (GUI vs CLI), which none of the 5 workstreams require |
| Option B hull-on-capture in worker (Workstream 2) | Hull-on-GUI-thread | If the hull compute is cheap and avoids storing two `TriangleMesh` copies in the POD; GUI-thread compute risks frame hitches for large towers. Recommend capture-in-worker for the v4.4 Frozen-Decision-1 invariant consistency |
| Port `SceneRaycaster` to `src/core/rendering/` (Workstream 5) | Call upstream `SceneRaycaster` directly via a thin shim | Rejected: upstream is wxWidgets-coupled (`GLVolume*`, `Camera` GUI type, ImGui overlays). The port isolates the math into the project's pure-CPU-testable rendering layer (matches the established `GizmoGeometry` pattern) |
| D3D12 debug layer via env flag (Workstream 4) | Hardcode debug-layer-on in Debug builds | Rejected: debug layer has runtime perf cost and requires Graphics Tools installed; gate behind an explicit env so production Debug builds stay fast |
| Convex hull via `TriangleMesh::convex_hull_3d()` (Workstream 2, qhull) | CGAL convex hull | Use qhull (the upstream choice at `3DScene.cpp:914`). CGAL 5.4 is available but the hull op is not in the CGAL-version-blocked set; qhull matches upstream exactly |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| OpenVDB | Unavailable (link failure, PROJECT.md Constraints). Required ONLY for hollow/support paint gizmos (HMS), which are removed scope. NONE of the 5 v4.5 workstreams need it — confirmed: Workstream 5's `Measure::Measuring` + `MeshClipper` operate on `indexed_triangle_set`, not VDB | `indexed_triangle_set` + qhull `convex_hull_3d` |
| FFmpeg / WebRTC / MetaRTC | Unavailable (PROJECT.md Constraints). Camera/device/network scope removed | N/A — no v4.5 workstream touches media streams |
| Vulkan QRhi backend (production) | Qt SDK disables public Vulkan support (`QT_DISABLED_PUBLIC_FEATURES` includes `vulkan`). `QRhi::create(Vulkan,...)` will fail | D3D11 (default) + D3D12 (Workstream 4 investigation). A Vulkan eval needs a custom Qt build, out of scope |
| D3D12 as default backend | Known startup segfault (PROJECT.md Key Decisions). Remains explicit opt-in via `OWZX_RHI_RENDERER=d3d12` | D3D11 default; D3D12 only for the Workstream 4 investigation |
| Custom argv parser | Reinvents normalization + multi-value + help/version already provided by `QCommandLineParser` | `QCommandLineParser` (already wired at `main_qml.cpp:172`) |
| OCCT-based mesh boolean for Option B hull | OCCT is load-time-linked (38 TK DLLs) but hull ops use CGAL/qhull upstream; OCCT is excluded for mesh boolean/cut-surface due to CGAL version (5.4 < 5.6 required) | `TriangleMesh::convex_hull_3d()` (qhull, matches upstream `3DScene.cpp:914`) |
| `Qt6::Quick3D` / `Qt6::Charts` / `Qt6::DataVisualization` / `Qt6::Labs*` | Not linked (`CMakeLists.txt:63,66` lists only Qml/Quick/QuickControls2/LinguistTools/OpenGL/Concurrent/Test/Gui/GuiPrivate/ShaderTools). FilamentGroupPopup needs none of them | Standard `QtQuick.Controls` + `QtQuick.Layouts` for the popup |
| PIX/WinPixEventRuntime as a shipped dependency | Capture tooling belongs to the developer machine, not the shipped app | Launch PIX externally against `build/OWzxSlicer.exe`; enable D3D12 debug layer via env flag in Debug only |

## Stack Patterns by Variant

**If the workstream touches libslic3r readback (1, 2):**
- Use the capture-by-value POD pattern (Frozen Decision 1 invariant): no
  `Print*` / `WipeTowerData*` / `TriangleMesh*` escapes the SliceService
  worker. Extend the existing `WipeTowerGeometry` POD (SliceService.h:42) or
  add a sibling POD + signal mirroring `wipeTowerGeometryReady`.
- Because the `Print` is valid only between `activePrint_.store(&print)`
  (`SliceService.cpp:508`) and `activePrint_.store(nullptr)` (`:625/:629/:634`).

**If the workstream adds RHI-rendered geometry (2):**
- Reuse the `GizmoVertex` layout (7-float/28-byte) — do NOT invent a new
  vertex format. The `RhiViewportRenderer::Vertex` typealias
  (`RhiViewportRenderer.h:29`) and every gizmo/cut/wipe builder share it.
- For arbitrary ITS meshes, the buffer upload path (`uploadWipeTowerBuffer`)
  already handles dynamic size; only the vertex SOURCE changes (ITS →
  GizmoVertex with baked color).

**If the workstream needs triangle-level picking (5):**
- Port `SceneRaycaster` + `MeshRaycaster` into `src/core/rendering/` as
  pure-CPU helpers (no QRhi, no libslic3r except `TriangleMesh`/`its`).
  Unit-test them like `GizmoGeometry`/`GizmoMath`/`ObjectPicking`.
- Instantiate `Measure::Measuring` (libslic3r) per-volume with the volume's
  `its`. Do NOT reimplement feature extraction.

**If the workstream adds argv options (3):**
- Extend `parseStartupOpenRequest()` (`main_qml.cpp:170`) — do NOT add a
  second parser. New fixture options use `QCommandLineOption` +
  `parser.addOption()` + `parser.values()`/`isSet()`, matching the 4
  existing options.

**If the workstream debugs D3D12 (4):**
- Gate debug-layer enable behind an env flag (e.g. `OWZX_D3D12_DEBUG=1`)
  read in `RhiBackendSelector.cpp` before `QRhi::create`. Do NOT enable in
  Release. PIX capture is external; do not link `WinPixEventRuntime`.

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| Qt 6.10 `QCommandLineParser` | Qt 5.x+ (stable API) | `QCommandLineOption::setNames()` multi-value works on 6.10; already proven in `main_qml.cpp:201-202` (`parser.values`) |
| Qt 6.10 `QRhiD3D12InitParams` | Qt 6.6+ (D3D12 backend stable in 6.x) | `enableDebugLayer` field available; needs Graphics Tools installed at runtime |
| Qt 6.10 `QQuickRhiItem` + `setSampleCount(4)` | Qt 6.6+ | `RhiViewport.cpp:47` MSAA trigger for depth-stencil; D3D12 crash may relate |
| libslic3r `wipe_tower_mesh_data` / `construct_mesh` | OrcaSlicer v7.0.1 upstream | `Print.cpp:5070-5087` builds the mesh; `Print.hpp:766` holds the optional |
| libslic3r `TriangleMesh::convex_hull_3d()` | OrcaSlicer v7.0.1 (qhull-backed) | `TriangleMesh.hpp:136`; same call upstream uses at `3DScene.cpp:914` |
| libslic3r `Measure::Measuring(const indexed_triangle_set&)` | OrcaSlicer v7.0.1 | `Measure.hpp:122`; already compiled into the from-source libslic3r target |
| libslic3r `ToolOrdering::get_recommended_filament_maps` | OrcaSlicer v7.0.1 | `ToolOrdering.hpp:244`; called inside `Print::process()` (`Print.cpp:2488`) |
| FilamentMapMode enum (4-value) | OrcaSlicer v7.0.1 (`PrintConfig.hpp:424-429`) | Qt int-widening 2→4 value; serialization via `ConfigOptionEnum` string names preserves round-trip |
| Eigen | 5.0 (via `find_package(Eigen3 5.0)` in BuildDepsFromSource.cmake:20) | Required by Measure::Measuring + raycaster math; already linked |

## Sources

- **Codebase (PRIMARY — all line citations verified 2026-07-12):**
  - `E:/ai/3DPrinter_Qt6/src/qml_gui/main_qml.cpp:1-290` — argv parser + deep-link routing already wired
  - `E:/ai/3DPrinter_Qt6/src/qml_gui/Renderer/RhiBackendSelector.cpp:1-120` — D3D11/D3D12 probe + `QRhiD3D12InitParams` already used
  - `E:/ai/3DPrinter_Qt6/src/qml_gui/Renderer/RhiViewport.cpp:47` — `setSampleCount(4)` D3D11 default
  - `E:/ai/3DPrinter_Qt6/src/qml_gui/Renderer/RhiViewportRenderer.h:29,57,129,143,152,177,216-222` — wipe-tower buffer/upload state (Option B target)
  - `E:/ai/3DPrinter_Qt6/src/core/rendering/GizmoGeometry.cpp:449-535` — `buildWipeTowerVertices` Option A box (Option B needs ITS sibling)
  - `E:/ai/3DPrinter_Qt6/src/core/rendering/AssembleViewDataPool.h:25-44` — `ModelObjectsClipper` slot reserved but unregistered (Workstream 5)
  - `E:/ai/3DPrinter_Qt6/src/core/services/ProjectServiceMock.cpp:2188,2267,2683,3010,3371,3922` — per-volume `its` access pattern (Workstream 5 reuse)
  - `E:/ai/3DPrinter_Qt6/src/core/model/PartPlate.h:202-203,268-273` — 2-value filament map mode (widen to 4)
  - `E:/ai/3DPrinter_Qt6/CMakeLists.txt:63,66,265-266,278-279` — Qt6 components + link targets (confirm no Quick3D/Charts/Labs)
  - `E:/ai/3DPrinter_Qt6/src/qml_gui/BBLTopbar.qml:396-397` — FilamentGroupPopup placeholder TODO (Workstream 1)
  - `E:/ai/3DPrinter_Qt6/.planning/milestones/v4.4-phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md` — Option A locked baseline, Option B future (Frozen Decision 2)
  - `E:/ai/3DPrinter_Qt6/.planning/milestones/v4.4-phases/100-wipe-tower-geometry-readback/100-01-SUMMARY.md` — v4.4 readback POD pattern (Frozen Decision 1 invariant to extend)
- **Upstream source-truth (PRIMARY):**
  - `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` — `WipeTowerData` + `WipeTowerMeshData` (real_wipe_tower_mesh / real_brim_mesh / wipe_tower_mesh_data optional)
  - `third_party/OrcaSlicer/src/libslic3r/Print.cpp:5070-5087` — `WipeTowerData::construct_mesh` (populates wipe_tower_mesh_data)
  - `third_party/OrcaSlicer/src/libslic3r/Print.cpp:2485-2488,3056` — auto filament-map compute + `get_filament_map_mode()`
  - `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424-429` — `FilamentMapMode` 4-value enum
  - `third_party/OrcaSlicer/src/libslic3r/GCode/ToolOrdering.hpp:244` + `.cpp:1107` — `get_recommended_filament_maps`
  - `third_party/OrcaSlicer/src/libslic3r/Measure.hpp:119-140` — `Measure::Measuring(const indexed_triangle_set&)` (feature engine)
  - `third_party/OrcaSlicer/src/libslic3r/TriangleMesh.hpp:122,136` — `merge()` + `convex_hull_3d()`
  - `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:840-925` — `load_wipe_tower_preview` (Option A) + `load_real_wipe_tower_preview` (Option B, hull at :914)
  - `third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp` — `SceneRaycaster` + `SceneRaycasterItem` + `HitResult` (Workstream 5 port source)
  - `third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp:178,208,237,241` — `MeshRaycaster::unproject_on_mesh` / `closest_hit`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.hpp:79,159` — `PickRaycaster = SceneRaycasterItem` + per-volume `Measuring` map
- **External docs (Qt argv parsing — confirmation only):**
  - [QCommandLineParser Class — Qt 6.8 docs](https://doc.qt.io/qt-6.8/zh/qcommandlineparser.html) — stable API across Qt 6.x/6.10 (HIGH confidence)
  - [QCommandLineParser GUI-vs-CLI pattern — Qt Forum](https://forum.qt.io/topic/53298/qcommandlineparser-to-select-gui-or-non-gui-mode) — confirms parse-before-QGuiApplication for mode switching (not needed here; app is GUI-only)
- **Project constraints (PRIMARY):**
  - `.planning/PROJECT.md` Constraints + Context — OpenVDB/FFmpeg unavailable, CGAL available, D3D11 default, Vulkan SDK-blocked
  - `AGENTS.md` — build command, source-truth rules

---
*Stack research for: v4.5 backlog closure (5 workstreams — auto filament-map, Option B wipe-tower mesh, CLI fixtures, D3D12 investigation, GLGizmoMeasure engine)*
*Researched: 2026-07-12*
