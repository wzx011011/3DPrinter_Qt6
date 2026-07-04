#include "GizmoGeometry.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// GizmoGeometry implementation - vertex generation ported VERBATIM from
// GLViewportRenderer.cpp (Phase 66 extraction). The position math is
// identical to the GL originals; the only addition is expanding each output
// vertex into the GizmoVertex struct {x,y,z,r,g,b,a} with
// the per-axis color from kAxisColorRGB.
//
// Source locations in GLViewportRenderer.cpp at HEAD (before Phase 66):
//   buildGizmoGeometry       (move arrows):  lines 940-1077
//   buildRotateGizmoGeometry (torus rings):  lines 1434-1506
//   buildScaleGizmoGeometry  (shafts+boxes): lines 1553-1626

namespace
{
  // Hardcoded move-gizmo vertex positions, copied verbatim from
  // GLViewportRenderer.cpp:944-1059. 114 vertices total: 3 axes x 38 verts
  // (2 shaft line endpoints + 36 cone triangle verts). Per axis:
  //   shaft: indices [ax*38 + 0 .. ax*38 + 1]  (2 verts, GL_LINES)
  //   cone:  indices [ax*38 + 2 .. ax*38 + 37] (36 verts, GL_TRIANGLES)
  static const float kMoveVerts[114][3] = {
      // X axis (indices 0-37)
      {0.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, 0.05500f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, 0.04763f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, 0.02750f, 0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, -0.02750f, 0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.05500f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, -0.05500f, 0.00000f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, -0.02750f, -0.04763f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, -0.02750f, -0.04763f},
      {1.00000f, 0.00000f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.00000f, 0.00000f},
      {0.78000f, 0.05500f, 0.00000f},
      {0.78000f, 0.02750f, -0.04763f},
      // Y axis (indices 38-75)
      {0.00000f, 0.00000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.02750f, 0.78000f, 0.04763f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {-0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.05500f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, 0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.05500f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {-0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 1.00000f, 0.00000f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      {-0.02750f, 0.78000f, -0.04763f},
      {0.00000f, 1.00000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.00000f, 0.78000f, 0.00000f},
      {0.05500f, 0.78000f, 0.00000f},
      {0.02750f, 0.78000f, -0.04763f},
      // Z axis (indices 76-113)
      {0.00000f, 0.00000f, 0.00000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.02750f, 0.04763f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {-0.02750f, 0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {-0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
      {-0.02750f, -0.04763f, 0.78000f},
      {0.00000f, 0.00000f, 1.00000f},
      {0.02750f, -0.04763f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.00000f, 0.00000f, 0.78000f},
      {0.05500f, 0.00000f, 0.78000f},
      {0.02750f, -0.04763f, 0.78000f},
  };

  // Per-axis number of move-gizmo vertices (shaft + cone).
  static constexpr int kMoveVertsPerAxis = 38;
  static constexpr int kMoveShaftVertsPerAxis = 2;
  static constexpr int kMoveConeVertsPerAxis = 36;

  // Rotate torus ring parameters (verbatim from GLViewportRenderer.cpp:1438-1441).
  static constexpr int kRotateSegments = 48;
  static constexpr int kRotateSides = 6;
  static constexpr float kRotateMajorR = 0.7f;
  static constexpr float kRotateMinorR = 0.035f;
  // 6 verts per quad (2 tris * 3 verts); per ring = segments * sides * 6.
  static constexpr int kRotateVertsPerRing = kRotateSegments * kRotateSides * 6;

  // Scale shaft + box parameters (verbatim from GLViewportRenderer.cpp:1558-1560).
  static constexpr float kScaleShaftEnd = 0.7f;
  static constexpr float kScaleBoxSize = 0.08f;
  static constexpr float kScaleBoxCenter = 0.78f;
  static constexpr int kScaleShaftVertsPerAxis = 2;
  static constexpr int kScaleBoxVerts = 36; // 6 faces * 2 tris * 3 verts

  // Append a single Vertex with the per-axis color baked in.
  inline void appendVert(QVector<GizmoVertex> &out,
                         float x, float y, float z, int axisIndex)
  {
    const float *rgb = GizmoGeometry::kAxisColorRGB[axisIndex];
    out.append({x, y, z, rgb[0], rgb[1], rgb[2], 1.0f});
  }
} // namespace

// ===========================================================================
// buildMoveGizmoVertices
// ===========================================================================
QVector<GizmoVertex>
GizmoGeometry::buildMoveGizmoVertices(GizmoGeometryOffsets *out)
{
  QVector<GizmoVertex> verts;
  verts.reserve(114);

  for (int i = 0; i < 114; ++i)
  {
    int axisIndex = i / kMoveVertsPerAxis; // 0=X, 1=Y, 2=Z
    appendVert(verts, kMoveVerts[i][0], kMoveVerts[i][1], kMoveVerts[i][2], axisIndex);
  }

  if (out)
  {
    for (int ax = 0; ax < 3; ++ax)
    {
      out->shaftBase[ax] = ax * kMoveVertsPerAxis;
      out->coneBase[ax] = ax * kMoveVertsPerAxis + kMoveShaftVertsPerAxis;
    }
    out->shaftVertCount = kMoveShaftVertsPerAxis;
    out->coneVertCount = kMoveConeVertsPerAxis;
  }
  return verts;
}

// ===========================================================================
// buildRotateGizmoVertices  -- ported verbatim from GLViewportRenderer.cpp:1448-1495
// ===========================================================================
QVector<GizmoVertex>
GizmoGeometry::buildRotateGizmoVertices(GizmoGeometryOffsets *out)
{
  QVector<GizmoVertex> verts;
  verts.reserve(3 * kRotateVertsPerRing * int(sizeof(GizmoVertex)));

  // Generate a torus ring in a specific plane.
  // axisIndex: 0=X (YZ plane), 1=Y (XZ plane), 2=Z (XY plane)
  auto addRing = [&](int axisIndex) -> int
  {
    int baseVertex = verts.size();
    for (int i = 0; i < kRotateSegments; i++)
    {
      float theta1 = 2.f * float(M_PI) * i / kRotateSegments;
      float theta2 = 2.f * float(M_PI) * (i + 1) / kRotateSegments;

      for (int j = 0; j < kRotateSides; j++)
      {
        float phi1 = 2.f * float(M_PI) * j / kRotateSides;
        float phi2 = 2.f * float(M_PI) * (j + 1) / kRotateSides;

        auto point = [&](float theta, float phi) -> void
        {
          float r = kRotateMajorR + kRotateMinorR * std::cos(phi);
          float y = kRotateMinorR * std::sin(phi);
          // Base ring in XZ plane
          float x = r * std::cos(theta);
          float z = r * std::sin(theta);
          // Rotate to target plane
          if (axisIndex == 0)
            appendVert(verts, y, x, z, axisIndex); // X ring: YZ plane
          else if (axisIndex == 1)
            appendVert(verts, x, y, z, axisIndex); // Y ring: XZ plane
          else
            appendVert(verts, x, z, y, axisIndex); // Z ring: XY plane
        };

        // Triangle 1: p00, p10, p11
        point(theta1, phi1);
        point(theta2, phi1);
        point(theta2, phi2);
        // Triangle 2: p00, p11, p01
        point(theta1, phi1);
        point(theta2, phi2);
        point(theta1, phi2);
      }
    }
    return baseVertex;
  };

  if (out)
  {
    out->ringBase[0] = addRing(0); // X ring
    out->ringBase[1] = addRing(1); // Y ring
    out->ringBase[2] = addRing(2); // Z ring
    out->ringVertCount = kRotateVertsPerRing;
  }
  else
  {
    addRing(0);
    addRing(1);
    addRing(2);
  }
  return verts;
}

// ===========================================================================
// buildScaleGizmoVertices  -- ported verbatim from GLViewportRenderer.cpp:1563-1614
// ===========================================================================
QVector<GizmoVertex>
GizmoGeometry::buildScaleGizmoVertices(GizmoGeometryOffsets *out)
{
  QVector<GizmoVertex> verts;
  verts.reserve(3 * (kScaleShaftVertsPerAxis + kScaleBoxVerts));

  static const float kAxes[3][3] = {
      {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

  // Generate a unit cube centered at `center` with size `size` (all verts
  // colored with the current axis color). 36 verts = 6 faces * 2 tris * 3.
  auto addCube = [&](const float *center, const float *size, int axisIndex)
  {
    float hx = size[0] * 0.5f, hy = size[1] * 0.5f, hz = size[2] * 0.5f;
    float cx = center[0], cy = center[1], cz = center[2];

    // 8 corners
    float c[8][3] = {
        {cx - hx, cy - hy, cz - hz}, {cx + hx, cy - hy, cz - hz},
        {cx + hx, cy + hy, cz - hz}, {cx - hx, cy + hy, cz - hz},
        {cx - hx, cy - hy, cz + hz}, {cx + hx, cy - hy, cz + hz},
        {cx + hx, cy + hy, cz + hz}, {cx - hx, cy + hy, cz + hz}};

    // 12 triangles (6 faces). Each face: 4 corner indices -> 2 tris.
    int faces[6][4] = {
        {0, 1, 2, 3}, // -Z
        {4, 6, 5, 7}, // +Z
        {0, 4, 5, 1}, // -Y
        {2, 6, 7, 3}, // +Y
        {0, 3, 7, 4}, // -X
        {1, 5, 6, 2}, // +X
    };

    for (auto &f : faces)
    {
      // Tri 1: a b c
      appendVert(verts, c[f[0]][0], c[f[0]][1], c[f[0]][2], axisIndex);
      appendVert(verts, c[f[1]][0], c[f[1]][1], c[f[1]][2], axisIndex);
      appendVert(verts, c[f[2]][0], c[f[2]][1], c[f[2]][2], axisIndex);
      // Tri 2: a c d
      appendVert(verts, c[f[0]][0], c[f[0]][1], c[f[0]][2], axisIndex);
      appendVert(verts, c[f[2]][0], c[f[2]][1], c[f[2]][2], axisIndex);
      appendVert(verts, c[f[3]][0], c[f[3]][1], c[f[3]][2], axisIndex);
    }
  };

  for (int ax = 0; ax < 3; ax++)
  {
    if (out)
      out->shaftBase[ax] = verts.size();

    // Shaft line (origin to shaftEnd along the axis).
    appendVert(verts, 0.f, 0.f, 0.f, ax);
    appendVert(verts, kAxes[ax][0] * kScaleShaftEnd,
               kAxes[ax][1] * kScaleShaftEnd,
               kAxes[ax][2] * kScaleShaftEnd, ax);

    if (out)
      out->boxBase[ax] = verts.size();

    // Box handle at end of axis.
    float center[3] = {
        kAxes[ax][0] * kScaleBoxCenter,
        kAxes[ax][1] * kScaleBoxCenter,
        kAxes[ax][2] * kScaleBoxCenter};
    float size[3] = {kScaleBoxSize, kScaleBoxSize, kScaleBoxSize};
    addCube(center, size, ax);
  }

  if (out)
  {
    out->shaftVertCount = kScaleShaftVertsPerAxis;
    out->boxVertCount = kScaleBoxVerts;
  }
  return verts;
}
