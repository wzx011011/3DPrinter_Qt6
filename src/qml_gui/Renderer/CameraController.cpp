#include "CameraController.h"

#include <QtMath>
#include <algorithm>

namespace
{
// Upstream ZoomUnit (Camera.cpp:23): one wheel notch scales zoom by
// 1/(1 - 1*ZoomUnit).
constexpr float kZoomUnit = 0.1f;
// Upstream max_zoom (Camera.hpp:165).
constexpr float kMaxZoom = 250.0f;
// Upstream min_zoom = 0.2 * calc_zoom_to_bounding_box_factor(scene_box)
// (Camera.hpp:166). The fit zoom sits at roughly radius-to-distance parity,
// so the distance mapping is: closest = radius / 250, farthest = 5x radius.
constexpr float kMinZoomFactor = 0.2f;

QQuaternion rotationFromAzElImpl(float azimuthDeg, float elevationDeg)
{
  // World yaw about the horizon normal, then camera-right pitch. The
  // composition reproduces the legacy analytic frame exactly:
  // eye direction = (cos(el)sin(az), sin(el), cos(el)cos(az)); the negative
  // pitch converts eye elevation into camera look-down pitch.
  return QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, azimuthDeg) *
         QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -elevationDeg);
}
} // namespace

CameraController::CameraController()
{
  // Upstream set_default_orientation (Camera.cpp:650-660): zenit 45 with a
  // 45-degree horizontal component; the Qt6 default view is azimuth 45,
  // elevation 35 (established K1C framing preserved from v5.12).
  m_rotation = rotationFromAzElImpl(45.0f, 35.0f);
}

void CameraController::setSceneExtent(float width, float depth, float height)
{
  m_sceneMinX = 0.0f;
  m_sceneMaxX = width;
  m_sceneMinY = 0.0f;
  m_sceneMaxY = std::max(100.0f, height);
  m_sceneMinZ = 0.0f;
  m_sceneMaxZ = depth;
  m_sceneRadius = 0.5f * std::sqrt(width * width + depth * depth + m_sceneMaxY * m_sceneMaxY);
}

QVector3D CameraController::clampTarget(const QVector3D &target) const
{
  // Upstream validate_target (Camera.cpp:662-674): clamp into the scene box
  // scaled 3x around its own center.
  const QVector3D center((m_sceneMinX + m_sceneMaxX) * 0.5f,
                         (m_sceneMinY + m_sceneMaxY) * 0.5f,
                         (m_sceneMinZ + m_sceneMaxZ) * 0.5f);
  QVector3D half((m_sceneMaxX - m_sceneMinX) * 1.5f,
                 (m_sceneMaxY - m_sceneMinY) * 1.5f,
                 (m_sceneMaxZ - m_sceneMinZ) * 1.5f);
  return QVector3D(qBound(center.x() - half.x(), target.x(), center.x() + half.x()),
                   qBound(center.y() - half.y(), target.y(), center.y() + half.y()),
                   qBound(center.z() - half.z(), target.z(), center.z() + half.z()));
}

QQuaternion CameraController::rotationFromAzEl(float azimuthDeg,
                                               float elevationDeg) const
{
  return rotationFromAzElImpl(azimuthDeg, elevationDeg);
}

void CameraController::setRotationAzEl(float azimuthDeg, float elevationDeg)
{
  m_rotation = rotationFromAzElImpl(azimuthDeg, elevationDeg);
}

void CameraController::orbit(float dAzimuth, float dElevation)
{
  // Upstream rotate_on_sphere (Camera.cpp:373-392): zenit clamps to exactly
  // +-90 and the overshoot is trimmed from the applied pitch so the rotation
  // stops at the pole instead of bouncing.
  if (!m_freeCamera) {
    const float el = elevation();
    const float clampedEl = qBound(-90.0f, el + dElevation, 90.0f);
    dElevation = clampedEl - el;
  }
  // World-up yaw, then camera-right pitch. Post-multiplication acts on the
  // camera-LOCAL axes, so the pitch axis is the local X (the post-multiplied
  // rotation maps it onto the world right); the negative angle converts the
  // eye-elevation delta into a look pitch so dElevation > 0 raises the eye
  // (matching the legacy m_elevation += dEl).
  m_rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, dAzimuth) * m_rotation;
  m_rotation = m_rotation * QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -dElevation);
  m_rotation.normalize();
}

void CameraController::orbitAround(float dAzimuth, float dElevation,
                                   const QVector3D &pivot)
{
  // Upstream rotate_on_sphere_with_target (Camera.cpp:351-370): the pivot
  // substitutes the orbit center; eye and target rotate rigidly about it.
  if (!m_freeCamera) {
    const QVector3D radial = eye() - pivot;
    const float el = qAsin(qBound(-1.0f, radial.normalized().y(), 1.0f));
    const float clampedEl = qBound(-90.0f, el + dElevation, 90.0f);
    dElevation = clampedEl - el;
  }
  const QQuaternion worldYaw =
      QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, dAzimuth);
  const QQuaternion pitch =
      QQuaternion::fromAxisAndAngle(rightVector(), -dElevation);
  const QQuaternion delta = worldYaw * pitch;
  m_target = pivot + delta.rotatedVector(m_target - pivot);
  m_rotation = (delta * m_rotation).normalized();
}

void CameraController::rotateLocalAroundTarget(float pitchAroundRightRad,
                                               float yawAroundUpRad)
{
  // Upstream rotate_local_around_target (Camera.cpp:409-420): one rotation
  // about the combined screen-drag axis expressed in camera-local space; no
  // limits, so the camera may roll freely.
  const QVector3D axis(pitchAroundRightRad, yawAroundUpRad, 0.0f);
  const float angle = axis.length();
  if (angle < 1e-8f)
    return;
  const QVector3D localAxis = m_rotation.conjugated().rotatedVector(axis / angle);
  m_rotation *= QQuaternion::fromAxisAndAngle(localAxis, qRadiansToDegrees(angle));
  m_rotation.normalize();
}

void CameraController::recoverFromFreeCamera()
{
  // Upstream Camera::recover_from_free_camera (Camera.hpp:154-158): when the
  // right vector left the horizon, rebuild the orientation through
  // look_at(eye, target, world up), which keeps eye/target/distance.
  if (qAbs(rightVector().y()) < 1e-6f)
    return;
  const QVector3D back = (eye() - target()).normalized();
  QVector3D right = QVector3D::crossProduct(QVector3D(0, 1, 0), back);
  if (right.lengthSquared() < 1e-8f)
    return;
  right.normalize();
  const QVector3D up = QVector3D::crossProduct(back, right);
  m_rotation = QQuaternion::fromAxes(right, up, back);
}

void CameraController::translateWorld(const QVector3D &displacement)
{
  // Upstream Camera::translate_world (Camera.hpp:135): the target moves and
  // the position follows; set_target validates against the scene box.
  m_target = clampTarget(m_target + displacement);
}

void CameraController::clampDistance()
{
  // Upstream set_zoom bounds (Camera.cpp:75-84) mapped to distance:
  // closest  = radius / max_zoom(250), floored at 2mm because the fixed
  //            perspective near plane sits at 1mm (upstream tightens the
  //            frustum per view instead).
  // farthest = radius / min_zoom(=0.2 * fit factor) ~= 5x radius.
  const float closest = std::max(2.0f, m_sceneRadius / kMaxZoom);
  const float farthest = m_sceneRadius / kMinZoomFactor;
  m_distance = qBound(closest, m_distance, farthest);
}

void CameraController::zoom(float delta)
{
  // Upstream update_zoom (Camera.cpp:83): delta clamped to +-4, zoom is
  // multiplicative (zoom /= 1 - delta*ZoomUnit); distance runs inverse to
  // zoom, so distance multiplies by the same factor.
  delta = qBound(-4.0f, delta, 4.0f);
  m_distance *= (1.0f - delta * kZoomUnit);
  clampDistance();
}

void CameraController::setOrientation(float azimuthDeg, float elevationDeg)
{
  // The quaternion orientation has no pole singularity, so +-90 is exact
  // (upstream clamps zenit to +-90 as well).
  setRotationAzEl(azimuthDeg, qBound(-90.0f, elevationDeg, 90.0f));
}

void CameraController::fitView(float cx, float cy, float cz, float radius)
{
  m_target = clampTarget(QVector3D(cx, cy, cz));
  // FOV=45°, tan(22.5°)≈0.4142; 加 20% 边距
  const float minDist = 50.f;
  const float safeRadius = std::max(0.1f, radius);
  m_distance = std::max(minDist, safeRadius / 0.4142f * 1.2f);
  clampDistance();
  setRotationAzEl(45.0f, 35.0f);
}

void CameraController::resetToDefault()
{
  m_target    = QVector3D(110.f, 0.f, 110.f);
  m_distance  = 380.0f;
  setRotationAzEl(45.0f, 35.0f);
}

void CameraController::viewTop()
{
  // Upstream select_view "top" (Camera.cpp:94-95): straight down, exact.
  setRotationAzEl(0.0f, 90.0f);
}

void CameraController::viewFront()
{
  setRotationAzEl(0.0f, 0.0f);
}

void CameraController::viewRight()
{
  setRotationAzEl(90.0f, 0.0f);
}

void CameraController::viewIso()
{
  setRotationAzEl(45.0f, 35.0f);
}

void CameraController::viewPlate()
{
  // Upstream Camera.cpp:102-106 uses topfront for the plate/default view.
  // In the Qt scene frame the bed is XZ and Y is up, so the eye is at
  // azimuth 0 with a 45-degree elevation above the bed.
  setRotationAzEl(0.0f, 45.0f);
}

// Phase 237 (VIEW-01): remaining Camera::select_view directions (upstream
// Camera.cpp:94-101). Qt6 Y-up mapping: bottom = -Y, rear = -Z, left = -X.
void CameraController::viewBottom()
{
  // Upstream select_view "bottom" (Camera.cpp:96-97): straight up from below
  // the bed, exact.
  setRotationAzEl(0.0f, -90.0f);
}

void CameraController::viewRear()
{
  setRotationAzEl(180.0f, 0.0f);
}

void CameraController::viewLeft()
{
  setRotationAzEl(-90.0f, 0.0f);
}

QVector3D CameraController::eye() const
{
  return m_target - forwardVector() * m_distance;
}

QVector3D CameraController::target() const   { return m_target; }
float     CameraController::distance() const { return m_distance; }

QVector3D CameraController::rightVector() const
{
  return m_rotation.rotatedVector(QVector3D(1, 0, 0));
}

QVector3D CameraController::upVector() const
{
  return m_rotation.rotatedVector(QVector3D(0, 1, 0));
}

QVector3D CameraController::forwardVector() const
{
  return m_rotation.rotatedVector(QVector3D(0, 0, -1));
}

float CameraController::azimuth() const
{
  // Target->eye direction (upstream zenit/azimuth convention: azimuth is the
  // horizontal angle of the eye direction measured from +Z toward +X).
  const QVector3D dir = -forwardVector();
  return qRadiansToDegrees(std::atan2(dir.x(), dir.z()));
}

float CameraController::elevation() const
{
  const QVector3D dir = -forwardVector();
  const float len = dir.length();
  if (len < 1e-8f)
    return 0.0f;
  return qRadiansToDegrees(qAsin(qBound(-1.0f, dir.y() / len, 1.0f)));
}

QMatrix4x4 CameraController::viewMatrix() const
{
  // Rotation-first view: world->camera is the conjugate rotation followed by
  // the eye translation. Equivalent to the legacy lookAt for every
  // non-degenerate orientation, and well-defined at the poles.
  QMatrix4x4 mat;
  mat.rotate(m_rotation.conjugated());
  mat.translate(-eye());
  return mat;
}

bool CameraController::groundPointOnPlane(const QMatrix4x4 &viewProj,
                                          const QSizeF &viewport,
                                          const QPointF &screen,
                                          QVector3D *outWorld)
{
  if (viewport.width() < 1.0 || viewport.height() < 1.0)
    return false;
  const float ndcX = float(2.0 * screen.x() / viewport.width() - 1.0);
  const float ndcY = float(1.0 - 2.0 * screen.y() / viewport.height());
  const QMatrix4x4 inverse = viewProj.inverted();
  const QVector4D nearPoint = inverse * QVector4D(ndcX, ndcY, -1.0f, 1.0f);
  const QVector4D farPoint = inverse * QVector4D(ndcX, ndcY, 1.0f, 1.0f);
  if (qFuzzyIsNull(nearPoint.w()) || qFuzzyIsNull(farPoint.w()))
    return false;
  // toVector3DAffine() already divides by w.
  const QVector3D p0 = nearPoint.toVector3DAffine();
  const QVector3D dir =
      farPoint.toVector3DAffine() - p0;
  if (qFuzzyIsNull(dir.y()))
    return false;
  const float t = -p0.y() / dir.y();
  if (!std::isfinite(t))
    return false;
  *outWorld = p0 + dir * t;
  return true;
}

QMatrix4x4 CameraController::projMatrix(float aspect) const
{
  QMatrix4x4 mat;
  // Phase 237 (VIEW-01): orthographic branch for the upstream View-menu
  // "Use Orthogonal View" toggle (upstream Camera EType::Ortho,
  // MainFrame.cpp:2611-2616). The ortho half-height matches the perspective
  // frustum half-height at the target distance (distance * tan(fov/2)) so the
  // toggle preserves the apparent scene size.
  if (m_useOrtho)
  {
    const float halfH = m_distance * 0.4142f; // tan(45 deg / 2)
    const float clampedAspect = std::max(0.01f, aspect);
    mat.ortho(-halfH * clampedAspect, halfH * clampedAspect,
              -halfH, halfH, 1.0f, 10000.0f);
    return mat;
  }
  mat.perspective(45.0f, std::max(0.01f, aspect), 1.0f, 10000.0f);
  return mat;
}
