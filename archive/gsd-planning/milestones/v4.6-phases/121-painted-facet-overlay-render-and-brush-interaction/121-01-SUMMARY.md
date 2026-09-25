# Phase 121 Summary: Painted-Facet Overlay Render And Brush Interaction

**Phase:** 121 (WS2, PAINT-02 + PAINT-03)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed PAINT-02 (overlay render on QRhi + Software mirror) + PAINT-03 (brush interaction). Reused mesh pipeline (m_fillPipeline + rhi_viewport.qsb, zero new shaders).

## Changes
- EditorViewModel.h/.cpp: paintOverlayData Q_PROPERTY (flattens painted facets to world-transformed byte stream) + extrudersColors Q_PROPERTY (MMU).
- RhiViewport.h/.cpp: paintOverlayData + brush Q_PROPERTYs (radius/cursorType/paintState/screen pos/button state); emitPaintPickIfActive reads Q_PROPERTYs (no more hardcoded 2.0); brush cursor tracking in mouse/hover events.
- RhiViewportRenderer.h/.cpp: m_paintOverlayBuffer + uploadPaintOverlayBuffer (state→color map, Y→Z swap) + renderPaintOverlay (gate {6,7,10}, m_fillPipeline, after model mesh); m_brushCursorBuffer + uploadBrushCursorBuffer (inverse-MVP + bed-plane intersect) + renderBrushCursor (m_translucentFillPipeline).
- GizmoGeometry.h/.cpp: buildBrushSphereVertices (UV sphere 1152 verts).
- SoftwareViewport.h/.cpp: paintOverlayData + painted faces appended to depth-sorted faces vector.
- PreparePage.qml: full Support paint panel (tool/radius/cursor-type) + paintOverlayData/brush bindings + _activeBrush* helpers.
- QmlUiAuditTests.cpp: paintedFacetOverlayAndBrushInteraction slot.

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=7288.
