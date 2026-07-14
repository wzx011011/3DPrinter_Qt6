# Phase 120 Summary: TriangleSelector Engine Port

**Phase:** 120 — TriangleSelector Engine Port (WS2, PAINT-01)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed PAINT-01: ported the TriangleSelector triangle-pick + adaptive-subdivide + paint-state pipeline. **TriangleSelector is REUSED (already compiled in libslic3r), not reimplemented.** Phase 120 builds the Qt6 owner (PaintEngine) + bridges 3 structural gaps. Foundation for Phase 121-123 (overlay/brush, Support/Seam, MMU).

## Key: Reuse, not reimplement
PaintEngine includes `<libslic3r/TriangleSelector.hpp>` and calls the upstream API directly (ctor/select_patch/get_facets/serialize/cursor_factory). Zero reimplementation. The GUI subclasses (GLGizmoPainterBase etc.) are GL-coupled and NOT reused — PaintEngine is the pure-C++ Qt6 equivalent.

## Changes
| File | Change |
|---|---|
| `ProjectServiceMock.h/.cpp` | volumeMeshTriangleMesh accessor (TriangleMesh aliasing shared_ptr; bridges gap 1: TriangleSelector ctor takes TriangleMesh&, not ITS). |
| `SceneRaycaster.h/.cpp` | SceneRaycasterHit.meshLocalPosition field (bridges gap 2: select_patch needs mesh-local hit; previously discarded). |
| `PaintEngine.h/.cpp` (new) | Per-volume TriangleSelector owner. ensureSelector/paintAt/getFacets/hasFacets/clearObject/serialize/deserialize. Pure C++ (held by EditorViewModel via unique_ptr). HAS_LIBSLIC3R guarded. |
| `EditorViewModel.h/.cpp` | m_paintEngine member + paintAtFacet Q_INVOKABLE (builds SceneRaycaster candidates, hitTest, paintAt). setTriangleSupportState kept as thin alias. clearPaintOnObject for gizmo exit. |
| `RhiViewport.h/.cpp` | emitPaintPickIfActive (gated on SupportPaint/SeamPaint/MmuSegmentation). mousePress + mouseMove (drag paint) wired. |
| `PreparePage.qml` | onPaintPickRequested handler. |
| `CMakeLists.txt` | PaintEngine.{cpp,h} registered in owzx_app_core. |
| `QmlUiAuditTests.cpp` | triangleSelectorEnginePorted slot (TS-01..07e). |
| `ViewModelSmokeTests.cpp` | paintEngineSelectPatchMarksFacetAndGetFacetsReturnsIt smoke (synthesized 2-triangle ITS, lib QSKIP stub). |

## Verification
- Canonical build (j6): exit 0; all targets linked (after fixing Slic3r::indexed_triangle_set → indexed_triangle_set — it's a global type, not namespaced).
- All 5 ctest groups passed (PrepareScene/PartPlate/ViewModel/QmlUiAudit/PreviewParser).
- APP_RUNNING_PID=30536.
- ViewModelSmokeTests incl. new paint smoke: passed.
- QmlUiAuditTests incl. triangleSelectorEnginePorted: passed.
- Source audit confirms TriangleSelector reused (include present, no redefinition).

## Carry-Forward
- Overlay rendering (colored facets on QRhi) → Phase 121 (PAINT-02).
- Brush interaction UI (size/type/fill) → Phase 121 (PAINT-03).
- Support/Seam slice integration → Phase 122 (PAINT-04).
- MMU segmentation → Phase 123 (PAINT-05).
- 3MF persistence (serialize/deserialize wiring) → Phase 122/123.
