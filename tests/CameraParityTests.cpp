// P14 (CAM-PARITY): regression lock for the upstream camera behavior
// migration (Slic3r::GUI::Camera). Each test cites the upstream function it
// pins, and the default-orientation test pins the legacy analytic frame so
// the quaternion rewrite introduces no visual shift.
#include <QtTest/QtTest>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QtMath>
#include <algorithm>

#include "qml_gui/Renderer/CameraController.h"

namespace
{
constexpr float kPi = 3.14159265358979f;

// The pre-rewrite analytic frame (azimuth/elevation + fixed-up lookAt). The
// quaternion camera must reproduce it outside the poles.
QMatrix4x4 legacyViewMatrix(float azimuthDeg, float elevationDeg, float distance,
                            const QVector3D &target)
{
  const float az = qDegreesToRadians(azimuthDeg);
  const float el = qDegreesToRadians(elevationDeg);
  const QVector3D dir(qCos(el) * qSin(az), qSin(el), qCos(el) * qCos(az));
  QMatrix4x4 mat;
  mat.lookAt(target + dir * distance, target, QVector3D(0, 1, 0));
  return mat;
}

bool matNear(const QMatrix4x4 &a, const QMatrix4x4 &b, float eps)
{
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      if (std::abs(a(r, c) - b(r, c)) > eps)
        return false;
  return true;
}
} // namespace

class CameraParityTests : public QObject
{
  Q_OBJECT

private slots:
  void defaultOrientationMatchesLegacyAnalytic();
  void constrainedOrbitClampsAtExactlyPlus90();
  void constrainedOrbitRecoversFromPole();
  void constrainedOrbitKeepsHorizonLevel();
  void freeTrackballIsUnboundedAndRolls();
  void recoverFromFreeCameraRestoresHorizon();
  void orbitAroundPivotPreservesPivotDistance();
  void zoomIsMultiplicativePerNotch();
  void zoomBoundsFollowSceneBox();
  void translateWorldClampsTargetTo3xSceneBox();
  void plateViewMatchesUpstreamTopFront();
  void groundPointOnPlaneRoundTrip();
};

void CameraParityTests::defaultOrientationMatchesLegacyAnalytic()
{
  // The v5.12 analytic default (azimuth 45, elevation 35, distance 380,
  // target 110,0,110) must survive the quaternion rewrite unchanged so no
  // view shifts on upgrade.
  CameraController cam;
  QVERIFY(matNear(cam.viewMatrix(),
                  legacyViewMatrix(45.0f, 35.0f, 380.0f,
                                   QVector3D(110, 0, 110)),
                  1e-3f));
  // And an arbitrary mid-range orientation.
  cam.setOrientation(213.0f, -57.0f);
  QVERIFY(matNear(cam.viewMatrix(),
                  legacyViewMatrix(213.0f, -57.0f, cam.distance(),
                                   cam.target()),
                  1e-3f));
}

void CameraParityTests::constrainedOrbitClampsAtExactlyPlus90()
{
  // Upstream rotate_on_sphere clamp (Camera.cpp:376-385): zenit stops at
  // exactly 90 degrees -- straight top-down is reachable.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  cam.orbit(0.0f, 1000.0f);
  QCOMPARE(cam.elevation(), 90.0f);
  // Camera straight above the target looking straight down.
  const QVector3D radial = cam.eye() - cam.target();
  QVERIFY(qAbs(radial.x()) < 1e-3f);
  QVERIFY(qAbs(radial.z()) < 1e-3f);
  QVERIFY(radial.y() > 0.0f);
  QVERIFY(matNear(cam.viewMatrix(), cam.viewMatrix(), 0.0f));
  // Further upward orbit keeps the exact pole without flipping.
  cam.orbit(30.0f, 50.0f);
  QCOMPARE(cam.elevation(), 90.0f);
  // Upstream delta adjustment (Camera.cpp:378): the applied pitch is
  // trimmed so a downward stroke pulls back the full amount.
  cam.orbit(0.0f, -30.0f);
  QVERIFY(qAbs(cam.elevation() - 60.0f) < 1e-3f);
}

void CameraParityTests::constrainedOrbitRecoversFromPole()
{
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  cam.orbit(0.0f, -1000.0f);
  QCOMPARE(cam.elevation(), -90.0f);
  // The azimuth bookkeeping keeps composing through the pole
  // (legacy m_azimuth += dAzimuth): 45 + 45 = 90.
  cam.orbit(45.0f, 45.0f);
  QVERIFY(qAbs(cam.elevation() - -45.0f) < 1e-3f);
  QVERIFY(qAbs(cam.azimuth() - 90.0f) < 1e-3f);
}

void CameraParityTests::constrainedOrbitKeepsHorizonLevel()
{
  // Constrained orbit must keep the right vector parallel to the horizon
  // (upstream constrained camera never rolls).
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  for (int i = 0; i < 200; ++i) {
    cam.orbit(7.0f, 1.3f);
    QVERIFY(std::abs(cam.rightVector().y()) < 1e-3f);
  }
}

void CameraParityTests::freeTrackballIsUnboundedAndRolls()
{
  // Upstream rotate_local_around_target (Camera.cpp:409-420): no limits,
  // roll is allowed, distance stays fixed.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  const float distance0 = cam.distance();
  bool rolled = false;
  for (int i = 0; i < 400; ++i) {
    cam.rotateLocalAroundTarget(0.05f, 0.03f);
    QCOMPARE(cam.distance(), distance0);
    if (std::abs(cam.rightVector().y()) > 0.2f)
      rolled = true;
  }
  QVERIFY(rolled);
  // Elevation derived from the rolled orientation saturates but the
  // orientation itself stays a normalized rotation (eye stays on the sphere).
  const QVector3D radial = cam.eye() - cam.target();
  QVERIFY(qAbs(radial.length() - distance0) < 1e-2f);
}

void CameraParityTests::recoverFromFreeCameraRestoresHorizon()
{
  // Upstream recover_from_free_camera (Camera.hpp:154-158): right vector
  // back on the horizon, eye/target/distance untouched.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  for (int i = 0; i < 120; ++i)
    cam.rotateLocalAroundTarget(0.06f, 0.04f);
  QVERIFY(std::abs(cam.rightVector().y()) > 1e-4f);
  const QVector3D eye0 = cam.eye();
  cam.recoverFromFreeCamera();
  QVERIFY(std::abs(cam.rightVector().y()) < 1e-4f);
  QVERIFY((cam.eye() - eye0).length() < 1e-3f);
}

void CameraParityTests::orbitAroundPivotPreservesPivotDistance()
{
  // Upstream rotate_on_sphere_with_target (Camera.cpp:351-370): eye and
  // target rotate rigidly about the pivot; pivot distances are preserved.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  const QVector3D pivot(130.0f, 12.0f, 90.0f);
  const float eyePivot0 = (cam.eye() - pivot).length();
  const float targetPivot0 = (cam.target() - pivot).length();
  for (int i = 0; i < 50; ++i)
    cam.orbitAround(3.0f, 0.7f, pivot);
  QVERIFY(std::abs((cam.eye() - pivot).length() - eyePivot0) < 1e-2f);
  QVERIFY(std::abs((cam.target() - pivot).length() - targetPivot0) < 1e-2f);
  // Zenit of the radial still clamps to +-90 (upstream apply_limits).
  cam.orbitAround(0.0f, 500.0f, pivot);
  const float el = qRadiansToDegrees(
      qAsin(qBound(-1.0f, (cam.eye() - pivot).normalized().y(), 1.0f)));
  QVERIFY(el <= 90.0f + 1e-3f);
}

void CameraParityTests::zoomIsMultiplicativePerNotch()
{
  // Upstream update_zoom (Camera.cpp:83): one notch (delta=1) scales zoom by
  // 1/(1-0.1); distance runs inverse, so distance multiplies by (1-0.1).
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  const float d0 = cam.distance();
  cam.zoom(1.0f);
  QVERIFY(std::abs(cam.distance() - d0 * 0.9f) < 1e-2f);
  // Delta clamps to +-4 (Camera.cpp:83).
  cam.zoom(50.0f);
  QVERIFY(std::abs(cam.distance() - d0 * 0.9f * (1.0f - 4.0f * 0.1f)) < 1e-2f);
}

void CameraParityTests::zoomBoundsFollowSceneBox()
{
  // Upstream set_zoom bounds (Camera.cpp:75-84): closest = radius/250
  // (floored at the 2mm near-plane margin documented in clampDistance),
  // farthest = radius / (0.2 * fit) ~= 5x radius for the mapped scene box.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  for (int i = 0; i < 500; ++i)
    cam.zoom(1.0f);
  const float radius = 0.5f * std::sqrt(220.0f * 220.0f + 220.0f * 220.0f
                                        + 120.0f * 120.0f);
  QVERIFY(cam.distance() <= std::max(2.0f, radius / 250.0f) + 1e-2f);
  for (int i = 0; i < 5000; ++i)
    cam.zoom(-1.0f);
  QVERIFY(cam.distance() <= radius / 0.2f + 1e-2f);
  QVERIFY(cam.distance() >= std::max(2.0f, radius / 250.0f) - 1e-2f);
}

void CameraParityTests::translateWorldClampsTargetTo3xSceneBox()
{
  // Upstream validate_target (Camera.cpp:662-674): target clamped into the
  // scene box enlarged 3x around its center.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  cam.translateWorld(QVector3D(10000.0f, -5000.0f, 10000.0f));
  const QVector3D t = cam.target();
  // 3x box: center (110,60,110), half extents (330,180,330).
  QVERIFY(t.x() <= 110.0f + 330.0f + 1e-3f);
  QVERIFY(t.x() >= 110.0f - 330.0f - 1e-3f);
  QVERIFY(t.y() >= 60.0f - 180.0f - 1e-3f);
  QVERIFY(t.z() <= 110.0f + 330.0f + 1e-3f);
  QCOMPARE(t.y(), 60.0f - 180.0f);
}

void CameraParityTests::plateViewMatchesUpstreamTopFront()
{
  // Upstream Camera::select_view("plate") (Camera.cpp:102-106) is the
  // top-front oblique view, distinct from the isometric default. The Qt
  // scene frame maps the upstream vertical axis to Y and bed depth to Z.
  CameraController cam;
  const QVector3D target = cam.target();
  cam.viewPlate();
  const QVector3D eyeDirection = (cam.eye() - target).normalized();
  QVERIFY(std::abs(eyeDirection.x()) < 1e-3f);
  QVERIFY(std::abs(eyeDirection.y() - std::sqrt(0.5f)) < 1e-3f);
  QVERIFY(std::abs(eyeDirection.z() - std::sqrt(0.5f)) < 1e-3f);
  QVERIFY(std::abs(cam.elevation() - 45.0f) < 1e-3f);
}

void CameraParityTests::groundPointOnPlaneRoundTrip()
{
  // The pan/zoom-to-mouse pick (upstream _mouse_to_3d with z=0): a known
  // ground point, projected to the screen and picked back, must round-trip.
  CameraController cam;
  cam.setSceneExtent(220.0f, 220.0f, 120.0f);
  const QSizeF viewport(800.0, 600.0);
  const QMatrix4x4 viewProj =
      cam.projMatrix(float(viewport.width() / viewport.height()))
      * cam.viewMatrix();
  const QVector3D ground(150.0f, 0.0f, 60.0f);
  const QVector4D clip = viewProj * QVector4D(ground, 1.0f);
  if (clip.w() <= 0.0f)
    QSKIP("point behind camera for the default view");
  const QPointF screen(float((clip.x() / clip.w() + 1.0) * 0.5 * viewport.width()),
                       float((1.0 - clip.y() / clip.w()) * 0.5 * viewport.height()));
  QVector3D picked;
  QVERIFY(CameraController::groundPointOnPlane(viewProj, viewport, screen,
                                               &picked));
  QVERIFY((picked - ground).length() < 1e-2f);
  // A level camera (eye on the bed plane, elevation 0) shoots rays parallel
  // to the plane: no pick.
  CameraController levelCam;
  levelCam.setOrientation(0.0f, 0.0f);
  QVERIFY(levelCam.target().y() == 0.0f);
  QVERIFY(std::abs(levelCam.eye().y()) < 1e-3f);
  const QMatrix4x4 levelViewProj =
      levelCam.projMatrix(float(viewport.width() / viewport.height()))
      * levelCam.viewMatrix();
  QVector3D ignored;
  QCOMPARE(CameraController::groundPointOnPlane(levelViewProj, viewport,
                                                QPointF(400, 300), &ignored),
           false);
}

QTEST_MAIN(CameraParityTests)
#include "CameraParityTests.moc"
