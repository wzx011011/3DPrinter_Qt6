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
| ASM-SHELL | `shotScreen/装配页.png` shows a 4-region AssembleView chrome (top bar, left settings sidebar, central 3D canvas, bottom controls) distinct from Prepare/Preview shells. | `Plater.qml:102-115` hosts only a placeholder `Item` (id `assembleSlot`) with a `Text` reading "装配视图暂不可用"; no AssemblePage.qml exists. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 90 | ASMSHELL-01 | _Pending task 89-01-02_ |
| ASM-CANVAS | `shotScreen/装配页.png` shows a central 3D canvas rendering the assembly model with top-right view controls; this is the third `GLCanvas3D` host after View3D and Preview. | No AssembleView canvas host exists on the Qt side; the RHI `GLViewport`/`GLViewportRenderer` path serves Prepare/Preview only. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | Critical | Phase 90 | ASMSHELL-02 | _Pending task 89-01-02_ |
| ASM-EXPLODE-SLIDER | `shotScreen/装配页_爆炸.png` shows the "爆炸比例" (Explosion Ratio) slider raised to `3.00`; default state at `0.00`/`1.0` per `shotScreen/装配页.png`. | No explosion-ratio control exists on the Qt side; upstream `m_explosion_ratio` has no Qt mirror. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 91 | ASMEXPLODE-01 | _Pending task 89-01-02_ |
| ASM-EXPLODE-RENDER | `shotScreen/装配页_爆炸.png` shows multi-part volumes separated radially at Explosion Ratio = 3.00 with yellow dashed connector guide lines between parts. | No per-volume separation rendering or connector guide lines exist on the Qt side. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | Critical | Phase 91 | ASMEXPLODE-02 | _Pending task 89-01-02_ |
| ASM-ASSEMBLY-GIZMO | `shotScreen/装配页_测量.png` shows the Assembly measurement gizmo active (Ctrl+Y), invoking ONLY_ASSEMBLY measure mode on the AssembleView canvas. | No `GLGizmoAssembly`/Assembly gizmo mode exists on the Qt side; the RHI gizmo set covers move/rotate/scale/cut for Prepare only. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 92 | ASMMEASURE-01 | _Pending task 89-01-02_ |
| ASM-MEASURE-OVERLAY | `shotScreen/装配页_测量.png` shows a right-side "测量" panel, white dashed dimension lines with arrowheads, teal measurement-value boxes (e.g. `90.000°`), and "选中 N 平面" plane-selection indicators. | No measurement overlay, dimension lines, value boxes, or measurement panel exist on the Qt side. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | Medium | Phase 92 | ASMMEASURE-02 | _Pending task 89-01-02_ |
| ASM-ASSEMBLY-INFO | `shotScreen/装配页.png` shows a bottom-right "装配体信息" (Assembly Info) panel with volume and dimensions for the assembly. | No assembly-info panel component exists on the Qt side. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | Medium | Phase 90 | ASMSHELL-01 | _Pending task 89-01-02_ |
| ASM-DATA-POOL | Upstream `AssembleViewDataID`/`AssembleViewDataPool` cache per-object data (model objects info, clipper) for the AssembleView gizmos; not directly screenshot-visible but required by the Assembly gizmo. | No AssembleView data pool plumbing exists on the Qt side. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 93 | ASMROUTE-02 | _Pending task 89-01-02_ |
| ASM-PLATER-ROUTING | Upstream `Plater.cpp` branches selection, undo/redo, and gizmo routing on `CanvasAssembleView`; the Qt side must mirror this so Prepare/Preview stay unchanged while AssembleView gets its own routing. | `BackendContext.h:199` `ViewMode::AssembleView = 2` enum entry and `BackendContext.cpp:362,376` `kLastVm` boundary exist as the routing anchor, but no real canvas host is wired behind them. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | Critical | Phase 90 | ASMROUTE-01 | _Pending task 89-01-02_ |
| ASM-NAVIGATION | `shotScreen/装配页.png` is reached from the application navigation; upstream AssembleView is a peer of View3D/Preview. | `src/qml_gui/main.qml` has no navigation entry that toggles to AssembleView; the only entry is the `vmAssembleView` enum value consumed by the placeholder. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 90 | ASMSHELL-01 | _Pending task 89-01-02_ |
| ASM-CLEANUP | The placeholder `assembleSlot` Item and "装配视图暂不可用" Text must be removed once a real AssembleView host lands; stale imports/resources/tests must not remain. | `Plater.qml:102-115` placeholder `Item` (id `assembleSlot`) + "装配视图暂不可用" `Text` are the removal candidates; no AssemblePage.qml/AssembleViewModel exist yet to leave stale. | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | _Pending task 89-01-02_ | High | Phase 93 | ASMVERIFY-01 | _Pending task 89-01-02_ |
