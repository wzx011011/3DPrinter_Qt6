#pragma once

#include <QMatrix4x4>
#include <QSize>
#include <QVector3D>
#include <QVector4D>
#include <utility>

// GizmoMath - pure-math gizmo pick/drag functions, extracted from
// GLViewportRenderer (Phase 65, GMATH-01/02/03). Zero render dependencies
// (no GL / RHI / QtOpenGL includes) so the same math is reusable from the
// RHI path without dragging in OpenGL.
//
// Every function is fully parameterized: callers pass viewport size, camera
// matrices, gizmo center, etc. explicitly. There is no member state.
//
// Parameter-order convention:
//   (screen coords, viewport, camera matrices, gizmo state, ...rest)
// i.e. computeRay(sx, sy, viewSize, projMatrix, viewMatrix)
//      pickMoveAxis(sx, sy, viewSize, projMatrix, viewMatrix,
//                   gizmoCenter, cameraEye, hasSelection)
//      rayToAxisT(orig, dir, axisDir, gizmoCenter)
//      computeRotateAngle(sx, sy, axis, viewSize, projMatrix, viewMatrix,
//                         gizmoCenter, rotateStartAngle)
//
// Scope (Phase 65): the seven gizmo pick/drag functions only. Object picking
// lives in ObjectPicking (Phase 72) so RHI can use the same source-truth ray.
class GizmoMath
{
public:
  // Screen-space (sx, sy in widget pixels, origin top-left) -> world-space
  // ray (origin on near plane, normalized direction).
  // Returns {{0,0,0}, {1,0,0}} when viewSize is degenerate (w<=0 || h<=0).
  static std::pair<QVector3D, QVector3D> computeRay(
      float sx, float sy, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix);

  // World point where the screen ray crosses the Y=0 plane.
  // Returns {0,0,0} if the ray is parallel to the Y=0 plane
  // (abs(dir.y()) < 1e-6f).
  static QVector3D rayXZIntersect(
      float sx, float sy, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix);

  // Closest parametric t along axisDir (line through gizmoCenter) for the
  // screen ray (orig, dir). Used by move + scale drag.
  // Returns e (=dot(axisDir, orig-gizmoCenter)) when the ray and axis are
  // parallel (denom < 1e-8f).
  static float rayToAxisT(
      const QVector3D &orig, const QVector3D &dir,
      const QVector3D &axisDir, const QVector3D &gizmoCenter);

  // Which move-arrow axis (1=X, 2=Y, 3=Z, 0=none) the cursor hits.
  // hasSelection=false forces return 0 (mirrors the m_meshBatches.empty()
  // early-return guard in the GL original).
  static int pickMoveAxis(
      float sx, float sy, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix,
      const QVector3D &gizmoCenter, const QVector3D &cameraEye,
      bool hasSelection);

  // Which rotate-ring axis (1=X, 2=Y, 3=Z, 0=none) the cursor hits.
  static int pickRotateAxis(
      float sx, float sy, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix,
      const QVector3D &gizmoCenter, const QVector3D &cameraEye,
      bool hasSelection);

  // Which scale-shaft axis (1=X, 2=Y, 3=Z, 0=none) the cursor hits.
  // (The GL original returns 1/2/3 only; uniform (4) is reserved for a
  // future center handle and not exercised by the current drag loop.)
  static int pickScaleAxis(
      float sx, float sy, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix,
      const QVector3D &gizmoCenter, const QVector3D &cameraEye,
      bool hasSelection);

  // Angle (radians) of the cursor projected onto the rotation plane
  // perpendicular to `axis` (1=X, 2=Y, 3=Z), measured from a reference axis
  // on that plane around gizmoCenter. Returns rotateStartAngle when the ray
  // is parallel to the rotation plane (abs(denom) < 1e-6f).
  static float computeRotateAngle(
      float sx, float sy, int axis, const QSize &viewSize,
      const QMatrix4x4 &projMatrix, const QMatrix4x4 &viewMatrix,
      const QVector3D &gizmoCenter, float rotateStartAngle);

private:
  GizmoMath() = delete; // static-only utility
};
