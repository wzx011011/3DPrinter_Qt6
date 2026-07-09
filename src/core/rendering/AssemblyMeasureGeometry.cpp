#include "AssemblyMeasureGeometry.h"

#include <algorithm>
#include <cmath>

namespace AssemblyMeasureGeometry
{

QVector3D center(const PrepareSceneData::ModelBounds &b)
{
  // Midpoint of the world-space AABB. Matches GL path's m_gizmoCenter
  // derivation (object bbox center). Used by both distance and overlay anchors.
  return QVector3D((b.minX + b.maxX) * 0.5f,
                   (b.minY + b.maxY) * 0.5f,
                   (b.minZ + b.maxZ) * 0.5f);
}

QVector3D longestAxis(const PrepareSceneData::ModelBounds &b)
{
  // Phase 92 (ASMMEASURE-02): the AABB axis whose extent is largest stands in
  // for the upstream "dominant feature direction" (an edge/plane normal). This
  // is the load-bearing simplification: two perpendicular longest axes produce
  // the screenshot's 90.000° angle without the full ITS feature extraction.
  const float dx = b.maxX - b.minX;
  const float dy = b.maxY - b.minY;
  const float dz = b.maxZ - b.minZ;
  if (dx >= dy && dx >= dz)
    return QVector3D(1.0f, 0.0f, 0.0f);
  if (dy >= dx && dy >= dz)
    return QVector3D(0.0f, 1.0f, 0.0f);
  return QVector3D(0.0f, 0.0f, 1.0f);
}

AssemblyMeasureResult measure(const PrepareSceneData::ModelBounds &a,
                              const PrepareSceneData::ModelBounds &b)
{
  // Phase 92 (ASMMEASURE-02): the common multi-volume measurement. Mirrors the
  // upstream point-point distance + edge-edge angle branches
  // (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoMeasure.cpp:1157-1286
  // render_dimensioning + third_party/OrcaSlicer/src/libslic3r/Measure.hpp:
  // 147-180 DistAndPoints / AngleAndEdges). The simplification uses AABB
  // centers (for distance) and longest-AABB-axis directions (for angle) instead
  // of picked features — produces the screenshot values without ITS raycasting.
  AssemblyMeasureResult result;
  // Degenerate AABB (non-positive extent on all axes) → invalid (no measurement).
  const auto degenerate = [](const PrepareSceneData::ModelBounds &bb) {
    const float ex = bb.maxX - bb.minX;
    const float ey = bb.maxY - bb.minY;
    const float ez = bb.maxZ - bb.minZ;
    return ex <= 0.0f && ey <= 0.0f && ez <= 0.0f;
  };
  if (degenerate(a) || degenerate(b))
    return result;  // valid stays false

  result.centerA = center(a);
  result.centerB = center(b);
  const QVector3D delta = result.centerB - result.centerA;
  result.distanceXyz = delta;
  result.distance = delta.length();
  result.axisA = longestAxis(a);
  result.axisB = longestAxis(b);
  // Angle between the two longest-axis directions (degrees). Clamp the dot
  // product to [-1, 1] to absorb float error (acos domain is strict).
  const float rawDot = QVector3D::dotProduct(result.axisA, result.axisB);
  const float dot = std::clamp(rawDot, -1.0f, 1.0f);
  result.angleDeg = std::acos(dot) * 180.0f / float(M_PI);
  result.valid = true;
  return result;
}

QString formatDistance(float mm)
{
  // Upstream format_double uses %.3f (GLGizmoMeasure.cpp:24); units are mm
  // (GLGizmoMeasure.cpp:1250 distance-string suffix). 3-decimal precision
  // matches the screenshot value-box style.
  return QString::number(mm, 'f', 3) + QStringLiteral(" mm");
}

QString formatAngle(float deg)
{
  // Upstream formats the radian->degree value with format_double + the degree
  // glyph (GLGizmoMeasure.cpp:1558). 3-decimal precision matches 90.000°.
  return QString::number(deg, 'f', 3) + QStringLiteral("\u00b0");
}

}  // namespace AssemblyMeasureGeometry
