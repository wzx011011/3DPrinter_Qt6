#include "AssemblyMeasureGeometry.h"

#include <algorithm>
#include <cmath>

namespace AssemblyMeasureGeometry
{

QVector3D center(const PrepareSceneData::ModelBounds &b)
{
  // Midpoint of the world-space AABB. Matches GL path's m_gizmoCenter
  // derivation (object bbox center). Used by the assembly overlay anchors.
  return QVector3D((b.minX + b.maxX) * 0.5f,
                   (b.minY + b.maxY) * 0.5f,
                   (b.minZ + b.maxZ) * 0.5f);
}

AssemblyMeasureResult measure(const PrepareSceneData::ModelBounds &a,
                              const PrepareSceneData::ModelBounds &b)
{
  // X3: coarse center-to-center fallback ONLY (see the header note). The
  // Phase 92 longest-AABB-axis angle stood in for the upstream edge/plane
  // feature angle and structurally always produced 0 deg / 90.000 deg (a
  // world AABB cannot represent object rotation); that value is retired —
  // angle and feature readouts come from the MeasureEngine picked-feature
  // path (upstream Measure::get_measurement), and the renderer keeps using
  // the centers here purely to anchor its dimension leader line.
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
