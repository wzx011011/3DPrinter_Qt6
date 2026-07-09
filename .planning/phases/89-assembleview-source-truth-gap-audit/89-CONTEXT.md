# Phase 89: AssembleView Source-Truth Gap Audit - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning
**Source:** Pre-planning exploration + milestone requirements

<domain>
## Phase Boundary

Phase 89 is a read-only audit phase for the v4.2 AssembleView Source-Truth
Restoration milestone. It freezes the AssembleView region map before
implementation by mapping the AssembleView surface to:

- the three target screenshots (`shotScreen/装配页.png`,
  `shotScreen/装配页_爆炸.png`, `shotScreen/装配页_测量.png`),
- OrcaSlicer source-truth files (behavior truth),
- current Qt/QML/C++ targets (the gap),
- replacement decisions,
- downstream phase ownership (Phase 90-93),
- requirements, and verification expectations.

This phase does not modify production UI or renderer source code. It produces
the canonical v4.2 AssembleView gap matrix (`89-GAP-MATRIX.md`) that Phase 90,
Phase 91, Phase 92, and Phase 93 execute against.

In scope:

- AssembleView default layout audit using `shotScreen/装配页.png`.
- Explosion ratio / assembly rendering audit using `shotScreen/装配页_爆炸.png`.
- Assembly measurement gizmo audit using `shotScreen/装配页_测量.png`.
- Upstream AssembleView source-truth mapping (`GUI_Preview.hpp`, `GLCanvas3D.hpp`,
  `GLGizmoAssembly.hpp`, `GLGizmosCommon.hpp`, `Plater.cpp`).
- Current Qt placeholder reconciliation (`Plater.qml:104-117`,
  `BackendContext.h:199`) into a replacement plan.
- Explicit out-of-scope classification for Arrange (already implemented).

Out of scope:

- Production QML/C++ changes.
- Arrange / auto-arrangement (already fully implemented).
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).
- D3D12/Vulkan backend promotion.
- Runtime visual proof (owned by Phase 93).

</domain>

<decisions>
## Implementation Decisions

### Deliverable shape
- The deliverable is `89-GAP-MATRIX.md` modeled after the Phase 84 settings gap
  matrix (`84-GAP-MATRIX.md`) and the Phase 79 Preview region matrix.
- The matrix must use a multi-column table: Region | Target Observation |
  Current Evidence | Qt Targets | Upstream Source | Decision | Gap | Severity |
  Owner | Requirement | Verification.
- Region IDs use an `ASM-` prefix (e.g., `ASM-SHELL`, `ASM-CANVAS`,
  `ASM-EXPLODE-SLIDER`, `ASM-EXPLODE-RENDER`, `ASM-ASSEMBLY-GIZMO`,
  `ASM-MEASURE-OVERLAY`, `ASM-ASSEMBLY-INFO`, `ASM-DATA-POOL`,
  `ASM-PLATER-ROUTING`, `ASM-NAVIGATION`, `ASM-CLEANUP`).

### Visual truth (from screenshot analysis)
- `shotScreen/装配页.png` — AssembleView default layout: 4-region chrome (top
  bar, left settings sidebar, central 3D canvas, bottom controls), bottom
  "爆炸比例" (Explosion Ratio) slider at 0.00, selection mode dropdown,
  bottom-right "装配体信息" (Assembly Info) panel with volume/dimensions,
  canvas top-right view controls.
- `shotScreen/装配页_爆炸.png` — Explosion Ratio = 3.00: multi-part volumes
  separated radially with yellow dashed connector guide lines between parts;
  Assembly Info panel values unchanged.
- `shotScreen/装配页_测量.png` — Assembly measurement gizmo active: right-side
  "测量" (Measurement) panel, white dashed dimension lines with arrowheads,
  teal measurement-value boxes (e.g. `90.000°`), plane-selection indicators
  ("选中 N 平面").

### Behavior truth (upstream source anchors — from pre-planning exploration)
- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` —
  `class AssembleView : public wxPanel` (third GLCanvas3D host; holds
  `wxGLCanvas* m_canvas_widget` + `GLCanvas3D* m_canvas`; exposes
  `get_canvas3d()`, `render()`, `reload_scene()`, `select_view()`).
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:509-513` —
  `enum ECanvasType { ..., CanvasAssembleView = 2 }`.
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596,770-771` —
  `mutable float m_explosion_ratio = 1.0;` plus
  `get_explosion_ratio()` / `reset_explosion_ratio()`.
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9` —
  `class GLGizmoAssembly : public GLGizmoMeasure` (`Ctrl+Y`, `ONLY_ASSEMBLY`
  measure mode; activability depends on `explosion_ratio ≈ 1.0` on assemble
  canvas and ≥2 volumes selected).
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.cpp:25-68` —
  constructor sets `m_measure_mode = EMeasureMode::ONLY_ASSEMBLY`.
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268,274,299` —
  `enum class AssembleViewDataID`, `AssembleViewDataPool`,
  `AssembleViewDataBase` (per-object cached data for the view).
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4959` —
  `assemble_view = new AssembleView(panel_3d, bed, model, config, &background_process);`
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4431` —
  `bool is_assemble_view_show() const { return current_panel == assemble_view; }`
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:7322,11601,11635,11744-11745,11823-11835` —
  AssembleView branches in undo/redo, selection clear, and gizmo routing
  (`get_current_canvas3D()->get_canvas_type() == CanvasAssembleView ?
   assemble_view->get_canvas3d() : view3D->get_canvas3d()`).

### Current Qt state (the gap — from pre-planning exploration)
- `src/qml_gui/pages/Plater.qml:102-115` — placeholder `Item` (id `assembleSlot`)
  with `Text { text: qsTr("装配视图暂不可用") }`; comment at line 103:
  "上游 AssembleView 在 v2.0 为 Out of Scope，仅保留枚举入口".
- `src/qml_gui/BackendContext.h:159` — `Q_PROPERTY(int vmAssembleView ...)`.
- `src/qml_gui/BackendContext.h:193-199` — `enum ViewMode { ..., AssembleView = 2 }`.
- `src/qml_gui/BackendContext.h:227` — `int vmAssembleView() const`.
- `src/qml_gui/BackendContext.cpp:362,376` — `kLastVm` boundary uses AssembleView.
- **Not present on Qt side:** no AssemblePage.qml, no AssembleViewModel, no
  explosion-ratio support, no Assembly gizmo, no data pool, no navigation entry
  in main.qml toggling to AssembleView, no `GLGizmoAssembly`/Assembly gizmo mode.
- **Already complete (NOT in scope):** Arrange (`ProjectServiceMock.cpp:2521-2592`
  calls real `Slic3r::arrange_objects`; `EditorViewModel.h:595-656` arrange
  settings properties; `PreparePage.qml:419,573,1398-1545` full arrange UI).
  Also `assembleObjects` (multi-part merge, `EditorViewModel.cpp:3217`) is
  distinct and already implemented.

### Phase ownership routing (from ROADMAP)
- Phase 89 (this phase): gap matrix + placeholder reconciliation plan (ASMAUDIT-01, ASMAUDIT-02).
- Phase 90: shell + canvas host + navigation + Plater routing (ASMSHELL-01, ASMSHELL-02, ASMROUTE-01).
- Phase 91: explosion ratio + assembly rendering (ASMEXPLODE-01, ASMEXPLODE-02).
- Phase 92: assembly measurement gizmo (ASMMEASURE-01, ASMMEASURE-02).
- Phase 93: data pool + verification + cleanup (ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02).

### Out-of-scope explicit classification
- Arrange (auto-arrangement): already complete, must be classified as
  out-of-scope in the matrix so no later phase re-touches it.
- `assembleObjects` (multi-part merge): already implemented, out-of-scope.
- All removed network/device/cloud scope: out-of-scope.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Prior gap-audit pattern (template to replicate)
- `.planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` — canonical multi-column settings gap matrix; the structure Phase 89 replicates.
- `.planning/phases/84-settings-source-truth-gap-audit/84-CONTEXT.md` — prior audit context for boundary/decisions shape.

### Upstream AssembleView source (behavior truth)
- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` — `class AssembleView : public wxPanel`.
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:509-513` — `ECanvasType::CanvasAssembleView`.
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596,770-771` — `m_explosion_ratio` + accessors.
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9` — `GLGizmoAssembly : public GLGizmoMeasure`.
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.cpp:25-68` — `ONLY_ASSEMBLY` mode.
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268,274,299` — `AssembleViewDataID`/`AssembleViewDataPool`.
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4959,4431,7322,11601,11635,11744,11823` — instantiation + CanvasAssembleView branching.

### Current Qt placeholder (the gap)
- `src/qml_gui/pages/Plater.qml:102-115` — placeholder slot.
- `src/qml_gui/BackendContext.h:159,193-199,227` — ViewMode enum + accessor.
- `src/qml_gui/BackendContext.cpp:362,376` — kLastVm boundary.

### Screenshots (visual truth)
- `shotScreen/装配页.png` — default layout.
- `shotScreen/装配页_爆炸.png` — explosion ratio = 3.00.
- `shotScreen/装配页_测量.png` — measurement gizmo active.

### Project rules
- `.codex/rules/source-truth-migration.md` — canonical migration rules.
- `.planning/REQUIREMENTS.md` — v4.2 requirement definitions.
- `.planning/ROADMAP.md` — Phase 89-93 structure.

</canonical_refs>

<deferred>
## Deferred Ideas

- Runtime visual proof is deferred to Phase 93.
- Implementation of any canvas host / gizmo / rendering is deferred to Phase 90-93.
- Phase 89 produces only the gap matrix + placeholder reconciliation plan.

</deferred>

---

*Phase: 89-assembleview-source-truth-gap-audit*
*Context gathered: 2026-07-09 from pre-planning exploration + milestone requirements*
