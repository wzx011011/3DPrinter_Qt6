# Phase 89 AssembleView Gap Matrix

**Target evidence:**

- `shotScreen/装配页.png` (AssembleView default layout)
- `shotScreen/装配页_爆炸.png` (explosion ratio = 3.00)
- `shotScreen/装配页_测量.png` (assembly measurement gizmo active)

**Scope:** AssembleView (assembly view) page shell, canvas host, explosion ratio,
assembly rendering, Assembly measurement gizmo, data pool, and Plater
`CanvasAssembleView` routing only. No LAN/device/cloud/network, Monitor,
ModelMall/Home WebView, camera streams, D3D12/Vulkan, libslic3r slicing
algorithm, Arrange, or `assembleObjects` work is in Phase 89 scope.

## Summary

Phase 89 is the v4.2 source-truth audit. Its job is to freeze the AssembleView
restoration region map before implementation. The current Qt surface exposes
only a `Plater.qml` placeholder `Item` (`assembleSlot`) showing
"装配视图暂不可用" plus a `ViewMode::AssembleView = 2` enum entry used as a
routing boundary. No real AssembleView page, canvas host, explosion-ratio
support, Assembly gizmo, data pool, or navigation entry exists on the Qt side
yet. This matrix is the canonical routing artifact for Phase 90-93.

This phase is read-only with respect to production source: it modifies
documentation only and produces no QML/C++ changes. Runtime pixel parity and
visual proof are owned by Phase 93; Phase 89 does not claim them.

## Target Evidence

The three screenshots below are the visual/layout truth for the AssembleView
restoration. Each region row in the Canonical Region Matrix cites the
screenshot(s) that observe it.

### `shotScreen/装配页.png` — AssembleView default layout

The default AssembleView layout shows a 4-region chrome: a top bar, a left
settings sidebar, a central 3D canvas, and a bottom controls row. The bottom
controls row contains a "爆炸比例" (Explosion Ratio) slider at `0.00` and a
"选择模式" (Selection Mode) dropdown. A bottom-right "装配体信息" (Assembly
Info) panel shows volume and dimensions. The canvas top-right exposes view
controls. This screenshot is the source of truth for `ASM-SHELL`,
`ASM-CANVAS`, `ASM-EXPLODE-SLIDER` (default state), `ASM-ASSEMBLY-INFO`, and
`ASM-NAVIGATION`.

### `shotScreen/装配页_爆炸.png` — Explosion Ratio = 3.00

With the Explosion Ratio raised to `3.00`, multi-part object volumes separate
radially along their assembly axes. Yellow dashed connector guide lines link
the separated parts so the assembly relationship stays readable. The Assembly
Info panel values follow the separated geometry. This screenshot is the source
of truth for `ASM-EXPLODE-SLIDER` (active state) and `ASM-EXPLODE-RENDER`.

### `shotScreen/装配页_测量.png` — Assembly measurement gizmo active

With the Assembly measurement gizmo active, a right-side "测量" (Measurement)
panel appears. White dashed dimension lines with arrowheads annotate distances
and angles between selected volumes. Teal measurement-value boxes render the
measured values (e.g. `90.000°`). Plane-selection indicators ("选中 N 平面")
mark the active measurement plane. This screenshot is the source of truth for
`ASM-ASSEMBLY-GIZMO` and `ASM-MEASURE-OVERLAY`.

## Canonical Region Matrix

| Region | Target Observation | Current Evidence | Qt Targets | Upstream Source | Decision | Gap | Severity | Owner | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|---|
| ASM-SHELL | `shotScreen/装配页.png` shows a 4-region AssembleView chrome (top bar, left settings sidebar, central 3D canvas, bottom controls) distinct from Prepare/Preview shells. | `Plater.qml:102-115` hosts only a placeholder `Item` (id `assembleSlot`) with a `Text` reading "装配视图暂不可用"; no AssemblePage.qml exists. | `src/qml_gui/pages/Plater.qml:102-115` (placeholder `assembleSlot` Item to remove); future `src/qml_gui/pages/AssemblePage.qml` (not yet present, to be added by Phase 90). | `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` (`class AssembleView : public wxPanel`; holds `wxGLCanvas* m_canvas_widget` + `GLCanvas3D* m_canvas`; exposes `get_canvas3d()`, `render()`, `reload_scene()`, `select_view()`). | Add (new AssemblePage.qml) + remove (placeholder slot). | No AssembleView page shell exists; only a placeholder Text. | High | Phase 90 | ASMSHELL-01 | Source/QML audit proving Plater.qml placeholder removed and AssemblePage.qml added; Phase 93 runtime screenshot against `shotScreen/装配页.png`. |
| ASM-CANVAS | `shotScreen/装配页.png` shows a central 3D canvas rendering the assembly model with top-right view controls; this is the third `GLCanvas3D` host after View3D and Preview. | No AssembleView canvas host exists on the Qt side; the RHI `GLViewport`/`GLViewportRenderer` path serves Prepare/Preview only. | `src/qml_gui/Renderer/GLViewport.h` / `GLViewport.cpp`; `src/qml_gui/Renderer/GLViewportRenderer.h` / `GLViewportRenderer.cpp` (extend/reuse the default D3D11 QRhi path for a third canvas host). | `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:509-513` (`enum ECanvasType { CanvasView3D = 0, CanvasPreview = 1, CanvasAssembleView = 2 }`). | Add (register AssembleView as the third `CanvasAssembleView`-equivalent canvas). | No third canvas host; `CanvasAssembleView = 2` enum has no Qt implementation. | Critical | Phase 90 | ASMSHELL-02 | Source/QML audit proving third canvas registered; Phase 93 runtime launch with AssembleView canvas rendering. |
| ASM-EXPLODE-SLIDER | `shotScreen/装配页_爆炸.png` shows the "爆炸比例" (Explosion Ratio) slider raised to `3.00`; default state at `0.00`/`1.0` per `shotScreen/装配页.png`. | No explosion-ratio control exists on the Qt side; upstream `m_explosion_ratio` has no Qt mirror. | Future `AssemblePage.qml` bottom-controls slider (not yet present); future `AssembleViewModel` (not yet present) to hold the ratio. | `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596` (`mutable float m_explosion_ratio = 1.0;`); `GLCanvas3D.hpp:770-771` (`float get_explosion_ratio()` / `void reset_explosion_ratio()`). | Add (slider + viewmodel mirror of `m_explosion_ratio`). | No explosion-ratio slider or viewmodel field; no reset capability. | High | Phase 91 | ASMEXPLODE-01 | Source audit proving `m_explosion_ratio` mirrored; Phase 93 runtime slider at 0.00 and 3.00 against screenshots. |
| ASM-EXPLODE-RENDER | `shotScreen/装配页_爆炸.png` shows multi-part volumes separated radially at Explosion Ratio = 3.00 with yellow dashed connector guide lines between parts. | No per-volume separation rendering or connector guide lines exist on the Qt side. | `src/qml_gui/Renderer/GLViewportRenderer.h` / `GLViewportRenderer.cpp` (extend rendering to apply explosion offset + connector guide lines); future `AssembleViewModel` (not yet present). | `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596` (`m_explosion_ratio`); `GLCanvas3D.hpp:770-771` (accessors) — the same anchors drive per-volume separation rendering and the connector guide lines upstream. | Add (rendering path for per-volume offset + dashed connector guides). | No explosion-driven volume separation; no yellow dashed connector guide lines. | Critical | Phase 91 | ASMEXPLODE-02 | Source audit proving rendering uses `m_explosion_ratio`; Phase 93 runtime visual against `shotScreen/装配页_爆炸.png`. |
| ASM-ASSEMBLY-GIZMO | `shotScreen/装配页_测量.png` shows the Assembly measurement gizmo active (Ctrl+Y), invoking ONLY_ASSEMBLY measure mode on the AssembleView canvas. | No `GLGizmoAssembly`/Assembly gizmo mode exists on the Qt side; the RHI gizmo set covers move/rotate/scale/cut for Prepare only. | Future Assembly gizmo controller (not yet present) on the AssembleView canvas; future `AssembleViewModel` gizmo-mode state. | `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9` (`class GLGizmoAssembly : public GLGizmoMeasure`); `Gizmos/GLGizmoAssembly.cpp:25-29` (constructor sets `m_measure_mode = EMeasureMode::ONLY_ASSEMBLY`); `GLGizmoAssembly.cpp:45-51` (`m_shortcut_key = WXK_CONTROL_Y`); `GLGizmoAssembly.cpp:53-68` (`on_is_activable()` requires `explosion_ratio ≈ 1.0` on AssembleView + ≥2 volumes selected). | Add (Assembly gizmo with ONLY_ASSEMBLY mode + Ctrl+Y activability). | No Assembly gizmo; no ONLY_ASSEMBLY measure mode; no activability gating. | High | Phase 92 | ASMMEASURE-01 | Source audit proving `GLGizmoAssembly`/`ONLY_ASSEMBLY` mirrored + activability rules; Phase 93 runtime Ctrl+Y invocation. |
| ASM-MEASURE-OVERLAY | `shotScreen/装配页_测量.png` shows a right-side "测量" panel, white dashed dimension lines with arrowheads, teal measurement-value boxes (e.g. `90.000°`), and "选中 N 平面" plane-selection indicators. | No measurement overlay, dimension lines, value boxes, or measurement panel exist on the Qt side. | Future measurement panel QML (not yet present) on the right side of AssembleView; future overlay renderer for dimension lines/value boxes. | `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:32` (`on_render_input_window`); `Gizmos/GLGizmoAssembly.cpp:25-68` (ONLY_ASSEMBLY rendering inherits `GLGizmoMeasure` overlay rendering); `Gizmos/GLGizmoMeasure.hpp` (measurement overlay/dimension rendering base). | Add (right-side 测量 panel + overlay dimension lines + teal value boxes + plane indicators). | No measurement panel, dimension lines, value boxes, or plane-selection indicators. | Medium | Phase 92 | ASMMEASURE-02 | Source audit proving overlay mirrors ONLY_ASSEMBLY rendering; Phase 93 runtime visual against `shotScreen/装配页_测量.png`. |
| ASM-ASSEMBLY-INFO | `shotScreen/装配页.png` shows a bottom-right "装配体信息" (Assembly Info) panel with volume and dimensions for the assembly. | No assembly-info panel component exists on the Qt side. | Future "装配体信息" panel component (not yet present) in AssemblePage.qml bottom-right. | `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` (AssembleView class owns the canvas that backs the assembly-info panel); upstream AssembleView renders the assembly-info values from the model. | Add (assembly-info panel component). | No assembly-info panel; no volume/dimensions display for the assembly. | Medium | Phase 90 | ASMSHELL-01 | Source/QML audit proving panel added; Phase 93 runtime visual against `shotScreen/装配页.png`. |
| ASM-DATA-POOL | Upstream `AssembleViewDataID`/`AssembleViewDataPool` cache per-object data (model objects info, clipper) for the AssembleView gizmos; not directly screenshot-visible but required by the Assembly gizmo. | No AssembleView data pool plumbing exists on the Qt side. | Future data-pool plumbing (not yet present) feeding the Assembly gizmo without leaking into Prepare/Preview state. | `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268` (`enum class AssembleViewDataID { None, ModelObjectsInfo, ModelObjectsClipper }`); `GLGizmosCommon.hpp:274` (`class AssembleViewDataPool` with `update()` + `model_objects_info()`/`model_objects_clipper()` getters); `GLGizmosCommon.hpp:299` (`class AssembleViewDataBase`). | Add (data pool mirroring `AssembleViewDataID`/`AssembleViewDataPool`). | No data pool; Assembly gizmo has no cached per-object data source. | High | Phase 93 | ASMROUTE-02 | Source audit proving data pool plumbed and isolated from Prepare/Preview state. |
| ASM-PLATER-ROUTING | Upstream `Plater.cpp` branches selection, undo/redo, and gizmo routing on `CanvasAssembleView`; the Qt side must mirror this so Prepare/Preview stay unchanged while AssembleView gets its own routing. | `BackendContext.h:199` `ViewMode::AssembleView = 2` enum entry and `BackendContext.cpp:362,376` `kLastVm` boundary exist as the routing anchor, but no real canvas host is wired behind them. | `src/qml_gui/BackendContext.h:159` (`Q_PROPERTY(int vmAssembleView ...)`); `BackendContext.h:195-200` (`enum class ViewMode { View3D = 0, Preview = 1, AssembleView = 2 }`); `BackendContext.h:227` (`int vmAssembleView() const`); `BackendContext.cpp:362,376` (`kLastVm` boundary). | `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4959` (`assemble_view = new AssembleView(...)`); `Plater.cpp:4431` (`bool is_assemble_view_show() const`); `Plater.cpp:7322` (`CanvasAssembleView` render branch); `Plater.cpp:11601,11635` (gizmo routing + snapshot branch); `Plater.cpp:11744` (undo/redo routing); `Plater.cpp:11823` (selection clear on undo/redo). | Preserve (enum entry + accessor + `kLastVm` as routing anchor) + add (real routing branches behind them in Phase 90). | Enum/accessor exist but no real canvas host or routing branches wired behind them. | Critical | Phase 90 | ASMROUTE-01 | Source audit proving `CanvasAssembleView` routing branches mirror upstream; Phase 93 runtime Prepare/Preview regression check. |
| ASM-NAVIGATION | `shotScreen/装配页.png` is reached from the application navigation; upstream AssembleView is a peer of View3D/Preview. | `src/qml_gui/main.qml` has no navigation entry that toggles to AssembleView; the only entry is the `vmAssembleView` enum value consumed by the placeholder. | `src/qml_gui/main.qml` (add navigation entry toggling to `vmAssembleView`); `src/qml_gui/pages/Plater.qml` (the placeholder slot that the navigation reveals). | `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` (AssembleView is the third panel peer of View3D/Preview); `Plater.cpp:4959` (instantiation alongside `view3D`/`preview`). | Add (navigation entry) + preserve (`vmAssembleView` enum consumed by the new entry). | No navigation entry reaches AssembleView from the application shell. | High | Phase 90 | ASMSHELL-01 | Source/QML audit proving navigation entry added; Phase 93 runtime navigation to AssembleView. |
| ASM-CLEANUP | The placeholder `assembleSlot` Item and "装配视图暂不可用" Text must be removed once a real AssembleView host lands; stale imports/resources/tests must not remain. | `Plater.qml:102-115` placeholder `Item` (id `assembleSlot`) + "装配视图暂不可用" `Text` are the removal candidates; no AssemblePage.qml/AssembleViewModel exist yet to leave stale. | `src/qml_gui/pages/Plater.qml:102-115` (placeholder `assembleSlot` Item + "装配视图暂不可用" `Text` to remove); `src/qml_gui/qml.qrc` (verify no stale entries after replacement). | Source-truth cleanup rule (No Deprecated UI Rule); `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` (the real AssembleView class that replaces the placeholder). | Remove (placeholder Item + Text) — owned by Phase 90 shell; Phase 93 verifies no stale paths remain. | Placeholder Item + Text still present; must be removed in Phase 90 and audited in Phase 93. | High | Phase 93 | ASMVERIFY-01 | Source/QML audit proving placeholder removed + no stale imports/resources/tests; Phase 93 canonical verifier. |

## Placeholder Reconciliation (ASMAUDIT-02)

The current Qt AssembleView surface is a placeholder. ASMAUDIT-02 requires it
to be reconciled into a replacement plan with explicit removal of the
placeholder code path. The reconciliation is:

**Removed in Phase 90 (replaced by a real AssembleView canvas host):**

- `src/qml_gui/pages/Plater.qml:102-115` — the placeholder `Item` (id
  `assembleSlot`) and its child `Text { text: qsTr("装配视图暂不可用") }`.
  The comment at `Plater.qml:103` ("上游 AssembleView 在 v2.0 为 Out of
  Scope，仅保留枚举入口") is also removed because v4.2 reopens AssembleView.
  These are classified `remove` in the `ASM-CLEANUP` and `ASM-SHELL` rows and
  owned by Phase 90.

**Preserved as the canvas-type routing anchor (retained, not removed):**

- `src/qml_gui/BackendContext.h:159` — `Q_PROPERTY(int vmAssembleView READ
  vmAssembleView CONSTANT)`. Retained.
- `src/qml_gui/BackendContext.h:195-200` — `enum class ViewMode { View3D = 0,
  Preview = 1, AssembleView = 2 }`. The `AssembleView = 2` entry is retained
  because it mirrors upstream `ECanvasType::CanvasAssembleView = 2`
  (`GLCanvas3D.hpp:509-513`) and is the canvas-type routing anchor.
- `src/qml_gui/BackendContext.h:227` — `int vmAssembleView() const`. Retained.
- `src/qml_gui/BackendContext.cpp:362,376` — `constexpr int kLastVm =
  static_cast<int>(ViewMode::AssembleView);` in `setCurrentViewMode` and
  `requestChangeViewMode`. The `kLastVm` boundary stays valid once Phase 90
  wires the real host behind `AssembleView = 2`; no boundary change is needed.

**Net effect:** the placeholder UI (`assembleSlot` Item + "装配视图暂不可用"
Text) is removed and replaced by a real AssembleView canvas host in Phase 90,
while the `ViewMode::AssembleView` enum entry, the `vmAssembleView` Q_PROPERTY,
and the `vmAssembleView()` accessor are preserved as the routing anchor that
the new host plugs into. Phase 93 verifies no stale placeholder paths remain.

## Out-of-Scope Classification

The following items are explicitly out of scope for Phase 89-93. No later
phase in v4.2 re-touches them unless the user explicitly reopens them.

| Item | Status | Evidence / Reason |
|---|---|---|
| Arrange (auto-arrangement) | Out of scope — already implemented | `src/core/services/ProjectServiceMock.cpp:2521-2592` (`arrangeObjects` calls real `Slic3r::arrange_objects` at line 2580 with a tolerant `VirtualBedFn`); `src/core/viewmodels/EditorViewModel.h:595-656` (arrange settings properties: `arrangeDistance`, `arrangeRotation`, `arrangeAlignY`, `arrangeMultiMaterial`, `arrangeAvoidCalibration`); `src/qml_gui/pages/PreparePage.qml:573` (`arrangeAllObjects()` call) and `1398-1545` (full arrange settings popup UI). Arrange is distinct from the AssembleView canvas and is not re-touched by Phase 90-93. |
| `assembleObjects` (multi-part merge) | Out of scope — already implemented | `src/core/viewmodels/EditorViewModel.cpp:3227` (`projectService_->assembleObjects(indices)`; requires ≥2 selected objects). This is a model merge operation, distinct from the AssembleView canvas rendering and gizmo. Already complete. |
| LAN/device/cloud/network/Monitor/ModelMall/Home WebView/camera/printer-hardware workflows | Out of scope — removed scope | Removed from forward scope by user direction on 2026-07-07. The v4.2 milestone is local/offline AssembleView UI work only. Not reintroduced unless the user explicitly reopens it. |
| D3D12/Vulkan backend promotion | Out of scope — future backend work | D3D12 has a known startup crash and remains explicit opt-in; Vulkan is disabled in the current Qt 6.10 SDK (`QT_DISABLED_PUBLIC_FEATURES` lists `vulkan`). AssembleView renders on the default RHI/D3D11 path. Backend promotion is a dedicated future investigation milestone. |
| libslic3r slicing algorithm changes | Out of scope — engine preserved | The migration rewrites the GUI layer only; libslic3r slicing algorithms are preserved unchanged. |

## v4.2 Requirement Routing

Every v4.2 requirement is routed to its owner phase and matrix region(s).

| Requirement | Owner | Matrix Region(s) |
|---|---|---|
| ASMAUDIT-01 | Phase 89 | All canonical regions in this matrix (ASM-SHELL through ASM-CLEANUP). |
| ASMAUDIT-02 | Phase 89 | Placeholder Reconciliation section above (removal of `assembleSlot` Item + "装配视图暂不可用" Text; preservation of `ViewMode::AssembleView` enum + `vmAssembleView` accessor + `kLastVm`). |
| ASMSHELL-01 | Phase 90 | ASM-SHELL, ASM-ASSEMBLY-INFO, ASM-NAVIGATION. |
| ASMSHELL-02 | Phase 90 | ASM-CANVAS. |
| ASMROUTE-01 | Phase 90 | ASM-PLATER-ROUTING. |
| ASMEXPLODE-01 | Phase 91 | ASM-EXPLODE-SLIDER. |
| ASMEXPLODE-02 | Phase 91 | ASM-EXPLODE-RENDER. |
| ASMMEASURE-01 | Phase 92 | ASM-ASSEMBLY-GIZMO. |
| ASMMEASURE-02 | Phase 92 | ASM-MEASURE-OVERLAY. |
| ASMROUTE-02 | Phase 93 | ASM-DATA-POOL. |
| ASMVERIFY-01 | Phase 93 | ASM-CLEANUP plus source/QML audits for all regions. |
| ASMVERIFY-02 | Phase 93 | Final runtime launch, AssembleView reachability, and visual evidence against all three target screenshots. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| ASMAUDIT-01 | This matrix maps all 11 AssembleView regions to the three target screenshots, OrcaSlicer upstream source files (with line anchors), current Qt targets, replacement decisions, gaps, severities, owner phases, requirement IDs, and verification routes. |
| ASMAUDIT-02 | The Placeholder Reconciliation section explicitly classifies the `Plater.qml:102-115` placeholder `assembleSlot` Item and "装配视图暂不可用" Text for removal in Phase 90, while preserving the `BackendContext.h:199` `ViewMode::AssembleView = 2` enum entry, the `BackendContext.h:159` `vmAssembleView` Q_PROPERTY, and the `BackendContext.h:227` `vmAssembleView()` accessor as the `CanvasAssembleView = 2` routing anchor. |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 90 | Replace the `Plater.qml` placeholder with a real AssembleView page/canvas host (ASM-SHELL, ASM-CANVAS, ASM-ASSEMBLY-INFO, ASM-NAVIGATION); wire `CanvasAssembleView` routing branches (ASM-PLATER-ROUTING). Preserve the `ViewMode::AssembleView` enum/accessor/`kLastVm`. |
| 91 | Implement the explosion-ratio slider and multi-part volume separation rendering with connector guide lines on the default RHI/D3D11 path (ASM-EXPLODE-SLIDER, ASM-EXPLODE-RENDER). |
| 92 | Port the Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly`/`ONLY_ASSEMBLY`) with the right-side 测量 panel and measurement overlays (ASM-ASSEMBLY-GIZMO, ASM-MEASURE-OVERLAY). |
| 93 | Wire AssembleView data pool plumbing (ASM-DATA-POOL); remove stale placeholder artifacts and run source/QML audits (ASM-CLEANUP/ASMVERIFY-01); run the canonical verifier, launch the app, and record AssembleView visual evidence against all three target screenshots (ASMVERIFY-02). |
