#include "GizmoMath.h"

#include <cfloat>
#include <cmath>

// GizmoMath implementation - math ported VERBATIM from GLViewportRenderer.cpp
// (Phase 65 extraction). The only changes are the member->parameter
// substitutions documented in 65-01-PLAN.md Task 1; the floating-point
// operations, constants, and control flow are bit-identical so the GL path
// retains its original behavior.
//
// Source locations in GLViewportRenderer.cpp at HEAD:
//   computeRay         lines 1250-1266
//   rayXZIntersect     lines 1271-1278
//   rayToAxisT         lines 1346-1359
//   pickMoveAxis       lines 1364-1418
//   computeRotateAngle lines 1491-1517
//   pickRotateAxis     lines 1630-1670
//   pickScaleAxis      lines 1899-1950

// ===========================================================================
// computeRay
// ===========================================================================
std::pair<QVector3D, QVector3D>
GizmoMath::computeRay(float sx, float sy, const QSize &viewSize,
                     const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix)
{
  const float w = float(viewSize.width()), h = float(viewSize.height());
  if (w <= 0 || h <= 0)
    return {{}, {1, 0, 0}};
  QMatrix4x4 invPV = (projMatrix * viewMatrix).inverted();
  float ndcX = 2.f * sx / w - 1.f, ndcY = 1.f - 2.f * sy / h;
  QVector4D nearH = invPV * QVector4D(ndcX, ndcY, -1.f, 1.f);
  QVector4D farH = invPV * QVector4D(ndcX, ndcY, 1.f, 1.f);
  nearH /= nearH.w();
  farH /= farH.w();
  QVector3D orig = nearH.toVector3D();
  return {orig, (farH.toVector3D() - orig).normalized()};
}

// ===========================================================================
// rayXZIntersect
// ===========================================================================
QVector3D GizmoMath::rayXZIntersect(float sx, float sy, const QSize &viewSize,
                                    const QMatrix4x4 &projMatrix,
                                    const QMatrix4x4 &viewMatrix)
{
  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  if (std::abs(dir.y()) < 1e-6f)
    return {0, 0, 0};
  float t = -orig.y() / dir.y();
  return orig + t * dir;
}

// ===========================================================================
// rayToAxisT  -- closest t along axisDir at gizmoCenter from screen ray
// ===========================================================================
float GizmoMath::rayToAxisT(const QVector3D &orig, const QVector3D &dir,
                            const QVector3D &axisDir,
                            const QVector3D &gizmoCenter)
{
  // Solve shortest distance between two lines:
  // L1: orig + t*dir   L2: gizmoCenter + s*axisDir
  QVector3D w = orig - gizmoCenter;
  float b = QVector3D::dotProduct(dir, axisDir);
  float e = QVector3D::dotProduct(axisDir, w);
  float d = QVector3D::dotProduct(dir, w);
  float denom = 1.f - b * b;
  if (std::abs(denom) < 1e-8f)
    return e; // parallel
  return (e - b * d) / denom;
}

// ===========================================================================
// pickMoveAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
// ===========================================================================
int GizmoMath::pickMoveAxis(float sx, float sy, const QSize &viewSize,
                           const QMatrix4x4 &projMatrix,
                           const QMatrix4x4 &viewMatrix,
                           const QVector3D &gizmoCenter,
                           const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  // Compute gizmo scale for proximity threshold
  float dist = (gizmoCenter - cameraEye).length();
  float scale = std::max(dist * 0.15f, 5.f);
  // Threshold = 8% of arrow length in world units
  float thresh = scale * 0.08f;

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);

  static const QVector3D kAxes[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;
  for (int ax = 0; ax < 3; ax++)
  {
    // Closest distance from ray to axis line segment
    QVector3D axDir = kAxes[ax];
    QVector3D w = orig - gizmoCenter;
    float b = QVector3D::dotProduct(dir, axDir);
    float e = QVector3D::dotProduct(axDir, w);
    float d = QVector3D::dotProduct(dir, w);
    float denom = 1.f - b * b;
    float t_ray, s_axis;
    if (std::abs(denom) < 1e-8f)
    {
      t_ray = 0;
      s_axis = e;
    }
    else
    {
      t_ray = (b * e - d) / denom;
      s_axis = (e - b * d) / denom;
    }
    if (t_ray < 0)
      continue;      // behind camera
    s_axis /= scale; // normalise to [0..1] range
    if (s_axis < 0 || s_axis > 1.0f)
      continue; // outside arrow length

    QVector3D p1 = orig + t_ray * dir;
    QVector3D p2 = gizmoCenter + s_axis * scale * axDir;
    float dist2 = (p1 - p2).length();
    if (dist2 < thresh && dist2 < bestDist)
    {
      bestDist = dist2;
      best = ax + 1;
    }
  }
  return best;
}

// ===========================================================================
// computeRotateAngle  -- compute angle for rotate gizmo interaction
// ===========================================================================
float GizmoMath::computeRotateAngle(float sx, float sy, int axis,
                                   const QSize &viewSize,
                                   const QMatrix4x4 &projMatrix,
                                   const QMatrix4x4 &viewMatrix,
                                   const QVector3D &gizmoCenter,
                                   float rotateStartAngle)
{
  // Project mouse position onto the rotation plane and compute angle
  QVector3D planeNormal;
  if (axis == 1) planeNormal = QVector3D(1, 0, 0);
  else if (axis == 2) planeNormal = QVector3D(0, 1, 0);
  else planeNormal = QVector3D(0, 0, 1);

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);
  float denom = QVector3D::dotProduct(dir, planeNormal);
  if (std::abs(denom) < 1e-6f)
    return rotateStartAngle; // ray parallel to plane

  float t = QVector3D::dotProduct(gizmoCenter - orig, planeNormal) / denom;
  QVector3D hit = orig + t * dir;
  QVector3D v = hit - gizmoCenter;

  // Compute angle on the rotation plane
  // Reference axis: the "right" direction on this plane
  QVector3D ref;
  if (axis == 1) ref = QVector3D(0, 1, 0); // Y-axis as reference for X rotation
  else if (axis == 2) ref = QVector3D(1, 0, 0); // X-axis as reference for Y rotation
  else ref = QVector3D(1, 0, 0); // X-axis as reference for Z rotation

  return std::atan2(QVector3D::dotProduct(QVector3D::crossProduct(ref, v).normalized(), planeNormal),
                    QVector3D::dotProduct(ref, v));
}

// ===========================================================================
// pickRotateAxis  -- returns 1/2/3 for X/Y/Z ring, 0 if no hit
// ===========================================================================
int GizmoMath::pickRotateAxis(float sx, float sy, const QSize &viewSize,
                             const QMatrix4x4 &projMatrix,
                             const QMatrix4x4 &viewMatrix,
                             const QVector3D &gizmoCenter,
                             const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  float dist = (gizmoCenter - cameraEye).length();
  float scale = std::max(dist * 0.15f, 5.f);
  float thresh = scale * 0.10f; // picking tolerance

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);

  // For each ring, project ray onto ring plane and check distance to ring
  static const QVector3D kNormals[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;

  for (int ax = 0; ax < 3; ax++)
  {
    const QVector3D &n = kNormals[ax];
    float denom = QVector3D::dotProduct(dir, n);
    if (std::abs(denom) < 1e-6f)
      continue; // ray parallel to plane

    float t = QVector3D::dotProduct(gizmoCenter - orig, n) / denom;
    if (t < 0)
      continue; // behind camera

    QVector3D hit = orig + t * dir;
    float ringDist = (hit - gizmoCenter).length();
    float tubeDist = std::abs(ringDist - scale * 0.7f); // 0.7 = majorR

    if (tubeDist < thresh && tubeDist < bestDist)
    {
      bestDist = tubeDist;
      best = ax + 1;
    }
  }
  return best;
}

// ===========================================================================
// pickScaleAxis  -- returns 1/2/3 for X/Y/Z, 0 if no hit
// ===========================================================================
int GizmoMath::pickScaleAxis(float sx, float sy, const QSize &viewSize,
                            const QMatrix4x4 &projMatrix,
                            const QMatrix4x4 &viewMatrix,
                            const QVector3D &gizmoCenter,
                            const QVector3D &cameraEye, bool hasSelection)
{
  if (!hasSelection)
    return 0;

  float dist = (gizmoCenter - cameraEye).length();
  float scale = std::max(dist * 0.15f, 5.f);
  float thresh = scale * 0.10f;

  auto [orig, dir] = computeRay(sx, sy, viewSize, projMatrix, viewMatrix);

  static const QVector3D kAxes[3] = {
      QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};

  float bestDist = FLT_MAX;
  int best = 0;
  for (int ax = 0; ax < 3; ax++)
  {
    QVector3D axDir = kAxes[ax];
    QVector3D w = orig - gizmoCenter;
    float b = QVector3D::dotProduct(dir, axDir);
    float e = QVector3D::dotProduct(axDir, w);
    float d = QVector3D::dotProduct(dir, w);
    float denom = 1.f - b * b;
    float t_ray, s_axis;
    if (std::abs(denom) < 1e-8f)
    {
      t_ray = 0;
      s_axis = e;
    }
    else
    {
      t_ray = (b * e - d) / denom;
      s_axis = (e - b * d) / denom;
    }
    if (t_ray < 0)
      continue;
    s_axis /= scale;
    if (s_axis < 0 || s_axis > 1.0f)
      continue;

    QVector3D p1 = orig + t_ray * dir;
    QVector3D p2 = gizmoCenter + s_axis * scale * axDir;
    float d2 = (p1 - p2).length();
    if (d2 < thresh && d2 < bestDist)
    {
      bestDist = d2;
      best = ax + 1;
    }
  }
  return best;
}
