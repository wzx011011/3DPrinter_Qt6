#pragma once

// GizmoVertex - shared POD vertex layout used by both the RHI mesh path
// (RhiViewportRenderer) and the gizmo geometry builders (GizmoGeometry).
//
// Extracted in Phase 66 so GizmoGeometry.{h,cpp} can produce vertices with
// the same layout the RHI renderer consumes WITHOUT pulling in the heavy
// RhiViewportRenderer.h (which transitively requires Qt6::Quick / qrhi.h).
//
// Layout: position (3 floats) + color RGBA (4 floats) = 7 floats, 28 bytes.
// Keep this struct POD and free of any Qt/RHI includes.
struct GizmoVertex
{
  float x;
  float y;
  float z;
  float r;
  float g;
  float b;
  float a;
};
static_assert(sizeof(GizmoVertex) == 7 * sizeof(float),
              "GizmoVertex must be exactly 7 floats (28 bytes) - matches "
              "RhiViewportRenderer::Vertex and the RHI vertex-input layout.");
