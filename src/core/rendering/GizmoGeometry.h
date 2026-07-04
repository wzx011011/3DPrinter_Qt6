#pragma once

#include <QVector>
#include <QVector3D>

// GizmoVertex is the shared POD vertex layout {x,y,z,r,g,b,a} (7 floats,
// 28 bytes). Defined in core/rendering/GizmoVertex.h so this geometry layer
// avoids pulling in the heavy RhiViewportRenderer.h (which requires
// Qt6::Quick / qrhi.h). RhiViewportRenderer::Vertex is a typealias for
// GizmoVertex, so the layout the RHI mesh path consumes is identical to what
// these builders produce.
#include "GizmoVertex.h"

// Per-axis vertex offsets (in vertex count, NOT bytes) for per-axis draw
// calls. Returned by the builders via out-parameter. Index 0=X, 1=Y, 2=Z.
struct GizmoGeometryOffsets
{
  int shaftBase[3] = {0, 0, 0}; // move + scale shaft line start
  int coneBase[3] = {0, 0, 0};  // move cone triangle fan start
  int boxBase[3] = {0, 0, 0};   // scale box handle start
  int ringBase[3] = {0, 0, 0};  // rotate ring start
  int shaftVertCount = 0;
  int coneVertCount = 0;
  int boxVertCount = 0;
  int ringVertCount = 0;
};

// GizmoGeometry - pure-CPU gizmo vertex generators (Phase 66, GGEO-01/02/03).
// Three builders produce QVector<GizmoVertex> with per-axis
// RGBA color baked in. Zero GL/RHI upload calls - only CPU vertex generation.
// Callers (GLViewportRenderer today, RhiViewportRenderer in Phase 68) own the
// GPU upload.
//
// Scope: gizmo primitive builders plus Phase 71 cut-plane/wipe-tower helper
// geometry. All functions are pure CPU builders with no GL/RHI calls.
class GizmoGeometry
{
public:
  // Move arrows: 3 axes x (2 shaft verts + 36 cone verts) = 114 vertices.
  // Shaft uses GL_LINES topology (2 verts per axis); cone uses GL_TRIANGLES
  // (36 verts per axis = 6 sides x 3 tris x 2 verts... actually 12 tris x 3).
  // Offsets: shaftBase[ax] = ax*38, coneBase[ax] = ax*38 + 2.
  static QVector<GizmoVertex> buildMoveGizmoVertices(
      GizmoGeometryOffsets *out = nullptr);

  // Rotate torus rings: 3 axes x (48 segments x 6 sides x 6 verts) = 5184 vertices.
  // Uses GL_TRIANGLES topology. Ring radius = majorR (0.7) + minorR (0.035).
  static QVector<GizmoVertex> buildRotateGizmoVertices(
      GizmoGeometryOffsets *out = nullptr);

  // Scale shafts + box handles: 3 axes x (2 shaft verts + 36 box verts) = 114 vertices.
  // Shaft uses GL_LINES; box uses GL_TRIANGLES.
  static QVector<GizmoVertex> buildScaleGizmoVertices(
      GizmoGeometryOffsets *out = nullptr);

  // Cut plane fill: 2 triangles (6 verts) perpendicular to cutAxis at
  // cutPosition, using selected object bounds expanded by 5% on the two
  // non-cut axes. Colors match the GL cut-plane path.
  static QVector<GizmoVertex> buildCutPlaneVertices(
      const QVector3D &boundsMin,
      const QVector3D &boundsMax,
      int cutAxis,
      float cutPosition);

  // Cut plane outline: 4 line segments (8 verts) around the same expanded
  // plane as buildCutPlaneVertices. Outline alpha is stronger than fill.
  static QVector<GizmoVertex> buildCutPlaneOutlineVertices(
      const QVector3D &boundsMin,
      const QVector3D &boundsMax,
      int cutAxis,
      float cutPosition);

  // Wipe tower: rectangular prism on the bed, 6 faces x 2 triangles x 3 verts
  // = 36 vertices. Empty if any dimension is non-positive.
  static QVector<GizmoVertex> buildWipeTowerVertices(
      float x,
      float z,
      float width,
      float depth,
      float height);

  // Axis color constants - single source of truth (X=0, Y=1, Z=2).
  // Alpha is always 1.0; callers that need transparency set it post-build.
  // Matches the GL kAxisColors[3] (GLViewportRenderer.cpp:128-132 at HEAD
  // before Phase 66 removed them).
  static constexpr float kAxisColorRGB[3][3] = {
      {0.90f, 0.18f, 0.18f}, // X red
      {0.22f, 0.80f, 0.22f}, // Y green
      {0.18f, 0.40f, 0.95f}, // Z blue
  };

private:
  GizmoGeometry() = delete; // static-only utility
};
