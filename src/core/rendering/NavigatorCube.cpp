#include "NavigatorCube.h"

#include <QQuaternion>
#include <QtMath>
#include <algorithm>

namespace NavigatorCube
{

CameraBasis cameraBasis(const QMatrix4x4 &view)
{
  // The view matrix maps world to eye space; mapping the eye-space basis
  // back through the inverse recovers the world-space camera axes.
  const QMatrix4x4 inverse = view.inverted();
  CameraBasis basis;
  basis.forward = (inverse * QVector4D(0.0f, 0.0f, -1.0f, 0.0f)).toVector3D();
  basis.up = (inverse * QVector4D(0.0f, 1.0f, 0.0f, 0.0f)).toVector3D();
  basis.right = (inverse * QVector4D(1.0f, 0.0f, 0.0f, 0.0f)).toVector3D();
  if (!basis.forward.isNull())
    basis.forward.normalize();
  return basis;
}

QPointF projectToRect(const QVector3D &cubePos, const CameraBasis &basis,
                      const RectF &rect)
{
  // Cube camera basis (ImGuizmo.cpp:2815-2822 LookAt): z along the direction
  // from the cube center toward the camera (i.e. -forward), x = up x z, y = z x x.
  // The ortho projection [-1,1] maps cube coords (span 0.5) to NDC directly.
  const QVector3D zAxis = -basis.forward;
  QVector3D xAxis = QVector3D::crossProduct(basis.up, zAxis);
  if (xAxis.lengthSquared() < 1e-8f)
    xAxis = QVector3D(1.0f, 0.0f, 0.0f);
  else
    xAxis.normalize();
  const QVector3D yAxis = QVector3D::crossProduct(zAxis, xAxis);

  const float ndcX = QVector3D::dotProduct(cubePos, xAxis);
  const float ndcY = QVector3D::dotProduct(cubePos, yAxis);
  // Item pixels are y-down; NDC y is up.
  return QPointF(rect.x + (ndcX + 1.0f) * 0.5f * rect.w,
                 rect.y + (1.0f - (ndcY + 1.0f) * 0.5f) * rect.h);
}

Hit hitTest(const QPointF &pixel, const CameraBasis &basis, const RectF &rect)
{
  Hit hit;
  if (pixel.x() < rect.x || pixel.y() < rect.y
      || pixel.x() > rect.x + rect.w || pixel.y() > rect.y + rect.h)
    return hit;

  // Pixel -> NDC (inverse of projectToRect).
  const float ndcX = float((pixel.x() - rect.x) / rect.w) * 2.0f - 1.0f;
  const float ndcY = 1.0f - float((pixel.y() - rect.y) / rect.h) * 2.0f;

  const QVector3D zAxis = -basis.forward;
  QVector3D xAxis = QVector3D::crossProduct(basis.up, zAxis);
  if (xAxis.lengthSquared() < 1e-8f)
    xAxis = QVector3D(1.0f, 0.0f, 0.0f);
  else
    xAxis.normalize();
  const QVector3D yAxis = QVector3D::crossProduct(zAxis, xAxis);

  // Parallel projection along the camera direction: every ray meets the
  // center plane at p, then continues to q = p + forward * w on a face plane
  // (negative w runs toward the camera at forward * -3).
  const QVector3D p = xAxis * ndcX + yAxis * ndcY;

  float bestW = std::numeric_limits<float>::max();
  bool found = false;
  for (int axis = 0; axis < 3; ++axis) {
    for (int sign = -1; sign <= 1; sign += 2) {
      QVector3D n = QVector3D(0, 0, 0);
      if (axis == 0) n.setX(float(sign));
      else if (axis == 1) n.setY(float(sign));
      else n.setZ(float(sign));
      // Back-face culling: only faces pointing toward the camera
      // (ImGuizmo.cpp:2878-2880).
      const float facing = QVector3D::dotProduct(n, basis.forward);
      if (facing >= 0.0f)
        continue;
      // Plane n . q = 0.5 with q = p + forward * w.
      const float denom = QVector3D::dotProduct(n, basis.forward);
      if (qFuzzyIsNull(denom))
        continue;
      const float w = (0.5f - QVector3D::dotProduct(n, p)) / denom;
      if (w >= bestW)
        continue;
      const QVector3D q = p + basis.forward * w;
      const float eps = 1e-4f;
      if (qAbs(q.x()) > 0.5f + eps || qAbs(q.y()) > 0.5f + eps
          || qAbs(q.z()) > 0.5f + eps)
        continue;
      bestW = w;
      found = true;
      hit.faceNormal = n;
      // Quantize into the 3x3x3 grid: ImGuizmo panels at cube-third steps.
      // The index counts from the POSITIVE side (snap dir = 1 - c,
      // ImGuizmo.cpp:3070-3075: a +0.5 face coordinate must yield a +1
      // direction component, so > +1/6 maps to 0 and < -1/6 maps to 2).
      const auto grid = [](float v) {
        return v > 1.0f / 6.0f ? 0 : (v < -1.0f / 6.0f ? 2 : 1);
      };
      hit.box = grid(q.x()) * 9 + grid(q.y()) * 3 + grid(q.z());
    }
  }
  if (!found)
    hit.box = -1;
  return hit;
}

QVector3D snapDirectionForBox(int box)
{
  if (box < 0 || box > 26)
    return {};
  const int cx = box / 9;
  const int cy = (box - cx * 9) / 3;
  const int cz = box % 3;
  QVector3D dir(1.0f - float(cx), 1.0f - float(cy), 1.0f - float(cz));
  if (dir.lengthSquared() < 1e-8f)
    return {};
  dir.normalize();
  return dir;
}

QVector3D dragRotateClamped(const QVector3D &dir, const QVector3D &camRight,
                            float dxRadians, float dyRadians)
{
  // ImGuizmo.cpp:3090-3096: rx around referenceUp (world +Y), ry around the
  // camera right vector, combined as roll = rx * ry.
  const QQuaternion rx = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0),
                                                        -qRadiansToDegrees(dxRadians));
  const QQuaternion ry = QQuaternion::fromAxisAndAngle(camRight.normalized(),
                                                        -qRadiansToDegrees(dyRadians));
  QVector3D newDir = (rx * ry).rotatedVector(dir);
  if (newDir.lengthSquared() < 1e-8f)
    return dir;
  newDir.normalize();

  // Horizon clamp (ImGuizmo.cpp:3097-3100): planDir = right x up projected to
  // the ground plane; remove any component pulling the direction below it.
  QVector3D planDir = QVector3D::crossProduct(camRight, QVector3D(0, 1, 0));
  planDir.setY(0.0f);
  if (planDir.lengthSquared() < 1e-8f)
    return newDir;
  planDir.normalize();
  const float dt = QVector3D::dotProduct(planDir, newDir);
  if (dt < 0.0f) {
    newDir -= planDir * dt;
    if (newDir.lengthSquared() < 1e-8f)
      return planDir;
    newDir.normalize();
  }
  return newDir;
}

void orientationForDirection(const QVector3D &dir, float &azimuthDeg,
                             float &elevationDeg)
{
  if (dir.lengthSquared() < 1e-8f) {
    azimuthDeg = 0.0f;
    elevationDeg = 0.0f;
    return;
  }
  const QVector3D d = dir.normalized();
  // CameraController::eye() derives the eye from azimuth measured from +Z
  // toward +X and elevation as the world-Y angle, so invert that here.
  if (qAbs(d.x()) < 1e-6f && qAbs(d.z()) < 1e-6f) {
    // Pure vertical: azimuth is undefined; keep the current value by
    // signaling with NaN and letting the caller preserve it.
    azimuthDeg = std::numeric_limits<float>::quiet_NaN();
    elevationDeg = d.y() > 0.0f ? 89.0f : -89.0f;
    return;
  }
  azimuthDeg = qRadiansToDegrees(float(qAtan2(double(d.x()), double(d.z()))));
  elevationDeg = qRadiansToDegrees(
      float(qAsin(double(std::clamp(d.y(), -1.0f, 1.0f)))));
  elevationDeg = std::clamp(elevationDeg, -89.0f, 89.0f);
}

} // namespace NavigatorCube
