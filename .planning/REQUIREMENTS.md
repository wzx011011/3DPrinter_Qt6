# Requirements: OWzx Slicer v4.2 AssembleView Source-Truth Restoration

**Defined:** 2026-07-09
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

**Visual/layout truth (screenshots):**
- `shotScreen/装配页.png` — AssembleView default layout: 4-region chrome (top bar, left settings sidebar, central canvas, bottom controls), bottom "爆炸比例" (Explosion Ratio) slider at 0.00, selection mode dropdown, bottom-right "装配体信息" (Assembly Info) panel with volume/dimensions, canvas top-right view controls (zoom/rotate/pan/reset/explode/layers/settings).
- `shotScreen/装配页_爆炸.png` — Explosion Ratio = 3.00: multi-part volumes separated radially with yellow dashed connector guide lines between parts; Assembly Info panel values unchanged.
- `shotScreen/装配页_测量.png` — Assembly measurement gizmo active: right-side "测量" (Measurement) panel, white dashed dimension lines with arrowheads, teal measurement-value boxes (e.g. `90.000°`), plane-selection indicators ("选中 N 平面").

## v4.2 Requirements

### Source-Truth Inventory

- [x] **ASMAUDIT-01**: The AssembleView (assembly view) surface has a current inventory that maps screenshot-visible regions and upstream capabilities (explosion ratio, Assembly gizmo, data pool, Plater routing) to OrcaSlicer source files, Qt targets, replacement decisions, and verification evidence.
- [x] **ASMAUDIT-02**: The existing `Plater.qml` placeholder (`AssembleView = 2` enum + "装配视图暂不可用" stub) is reconciled into a replacement plan with explicit removal of the placeholder code path.

### Page Shell And Navigation

- [x] **ASMSHELL-01**: User can navigate to a real AssembleView page (replacing the `Plater.qml` placeholder) from the intended entry point(s), and the page renders as a screenshot-aligned canvas host with no leftover "temporarily unavailable" text.
- [x] **ASMSHELL-02**: AssembleView is registered as the third `GLCanvas3D`-equivalent canvas host (mirroring `CanvasAssembleView = 2`) and coexists with Prepare/Preview without breaking navigation, payload, or view-mode state.

### Explosion Ratio And Assembly Rendering

- [x] **ASMEXPLODE-01**: User can adjust an explosion-ratio control that separates volumes of multi-part objects for assembly inspection, mirroring upstream `m_explosion_ratio` behavior (default 1.0, reset capability).
- [x] **ASMEXPLODE-02**: AssembleView renders all loaded model volumes on the default RHI/D3D11 path with correct per-volume separation at the current explosion ratio, without regressing Prepare/Preview rendering.

### Assembly Measurement Gizmo

- [x] **ASMMEASURE-01**: User can invoke the Assembly measurement gizmo (`Ctrl+Y`) on the AssembleView canvas, mirroring `GLGizmoAssembly` activability rules (explosion ratio near 1.0, multi-volume selection).
- [x] **ASMMEASURE-02**: The Assembly gizmo measures distances/relations between selected volumes using the upstream `ONLY_ASSEMBLY` measure mode, rendering measurement overlays anchored to the right geometry.

### AssembleView Data And Plater Routing

- [x] **ASMROUTE-01**: AssembleView selection, undo/redo, and gizmo routing branch correctly on `CanvasAssembleView` (mirroring upstream `Plater.cpp` conditionals) so Prepare/Preview behavior is unchanged while AssembleView gets its own routing.
- [x] **ASMROUTE-02**: AssembleView data pool plumbing (`AssembleViewDataID` / `AssembleViewDataPool`) caches per-object data needed by the view without leaking into Prepare/Preview state.

### Verification And Cleanup

- [x] **ASMVERIFY-01**: Replaced AssembleView placeholder leaves no stale files, imports, resource entries, tests, or disconnected code paths; source/QML audits cover region mapping, canvas-type routing, explosion-ratio wiring, and gizmo anchors.
- [x] **ASMVERIFY-02**: The canonical verifier passes, `build/OWzxSlicer.exe` launches, AssembleView is reachable at runtime, and visual evidence is recorded against the target screenshot.

## Future Requirements

### Adjacent Local/Offline Work

- **AUTO-FILAMENT-FUTURE-01**: Auto filament-map recommendation and wipe-tower geometry/rendering as a dedicated milestone.
- **THUMB-FUTURE-01**: Real thumbnail capture and 3MF pixel round-trip.
- **FIXTURE-FUTURE-01**: Add missing CLI fixtures (`hotend.stl`, `Block20XY.stl`) and deterministic GUI fixture loading for visual screenshots.
- **BACKEND-FUTURE-01**: Resolve the D3D12 QRhi crash and evaluate Vulkan only after an SDK/runtime path exists.

### Removed Product Scope

- **NETWORK-REMOVED-01**: LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows are not future requirements unless the user explicitly reopens them.

## Out of Scope

Explicitly excluded to keep v4.2 focused.

| Feature | Reason |
|---|---|
| Arrange / auto-arrangement | Already fully implemented (`Slic3r::arrange_objects` via libnest2d + full settings UI in PreparePage). Arrange is a separate upstream concern invoked from the View3D canvas, not part of AssembleView's essence. |
| `assembleObjects` (merge-into-multi-part-object) | Already implemented in EditorViewModel; distinct from the AssembleView canvas. |
| Device, cloud print, Monitor, ModelMall/Home WebView/cloud, live camera/network, and printer-connected hardware workflows | Removed from forward product scope by user direction on 2026-07-07. |
| D3D12 or Vulkan backend promotion | Renderer backend work is blocked/future and not required for AssembleView on D3D11. |
| libslic3r slicing algorithm changes | GUI/canvas restoration must not change slicing engine behavior. |
| New product behavior not mapped to OrcaSlicer upstream | Violates the project core value. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| ASMAUDIT-01 | Phase 89 | Complete |
| ASMAUDIT-02 | Phase 89 | Complete |
| ASMSHELL-01 | Phase 90 | Complete |
| ASMSHELL-02 | Phase 90 | Complete |
| ASMROUTE-01 | Phase 90 | Complete |
| ASMEXPLODE-01 | Phase 91 | Complete |
| ASMEXPLODE-02 | Phase 91 | Complete |
| ASMMEASURE-01 | Phase 92 | Complete |
| ASMMEASURE-02 | Phase 92 | Complete |
| ASMROUTE-02 | Phase 93 | Complete |
| ASMVERIFY-01 | Phase 93 | Complete |
| ASMVERIFY-02 | Phase 93 | Complete |

**Coverage:**
- v4.2 requirements: 12 total
- Mapped to phases: 12
- Unmapped: 0

---
*Requirements defined: 2026-07-09*
*Last updated: 2026-07-09 after roadmap creation (Phases 89-93)*
