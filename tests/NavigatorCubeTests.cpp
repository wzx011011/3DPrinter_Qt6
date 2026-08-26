// NavigatorCubeTests - pure CPU tests for the 3D navigator cube port.
//
// v5.16 (NAVIGATOR). These tests pin the source-truth math ported from
// upstream ImGuizmo::ViewManipulate (third_party/OrcaSlicer/src/imguizmo/
// ImGuizmo.cpp:2779-3140) as hosted by GLCanvas3D::_render_3d_navigator
// (GLCanvas3D.cpp:5669-5733): the 27-box hit grid, snap directions,
// drag rotation with the horizon clamp, and the direction-to-azimuth/
// elevation mapping used to write back into CameraController.
//
// AUTOMOC caveat (single-file QtTest with cpp-internal Q_OBJECT, see
// tests/GizmoMathTests.cpp:1-12): after editing private slots here, re-run
// cmake configure (the canonical verify script does this automatically).

#include <QtTest>

#include "core/rendering/NavigatorCube.h"

#include <QtMath>
#include <cmath>

namespace
{
  // A view matrix looking from an elevated front position (matches the
  // CameraController default azimuth 45 / elevation 35 build).
  QMatrix4x4 viewFromAzEl(float azimuthDeg, float elevationDeg)
  {
    const float az = qDegreesToRadians(azimuthDeg);
    const float el = qDegreesToRadians(elevationDeg);
    const QVector3D dir(qCos(el) * qSin(az), qSin(el), qCos(el) * qCos(az));
    const QVector3D target(0.0f, 0.0f, 0.0f);
    const QVector3D eye = target + dir * 10.0f;
    QMatrix4x4 v;
    v.lookAt(eye, target, QVector3D(0, 1, 0));
    return v;
  }
}

class NavigatorCubeTests final : public QObject
{
  Q_OBJECT

private slots:
  void cameraBasisMatchesViewDirection();
  void projectToRectMapsCubeCenterAndCorners();
  void hitTestSnapsFacePanels();
  void snapDirectionForBoxCoversTwentySixDirections();
  void dragRotateClampsAtHorizon();
  void orientationForDirectionMatchesCameraControllerConvention();
};

void NavigatorCubeTests::cameraBasisMatchesViewDirection()
{
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(viewFromAzEl(45.0f, 35.0f));
  // Forward points from the eye toward the target: elevation 35 above the
  // horizon looking down.
  QVERIFY(basis.forward.y() < -0.5f && basis.forward.y() > -0.6f);
  QVERIFY(basis.forward.length() > 0.99f);
  // Up stays roughly world-up for a non-degenerate view.
  QVERIFY(basis.up.y() > 0.8f);
  // Right-handed camera frame: right x up = +z_eye = -forward.
  const float det = QVector3D::dotProduct(
      QVector3D::crossProduct(basis.right, basis.up), basis.forward);
  QVERIFY(det < -0.9f);
}

void NavigatorCubeTests::projectToRectMapsCubeCenterAndCorners()
{
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(viewFromAzEl(0.0f, 0.0f));
  // Front view: cube +X maps to rect +X, +Y maps up.
  NavigatorCube::RectF rect{100.0f, 200.0f, 128.0f, 128.0f};
  const QPointF center = NavigatorCube::projectToRect(QVector3D(0, 0, 0), basis, rect);
  QCOMPARE(center.x(), 164.0f); // rect center
  QCOMPARE(center.y(), 264.0f);
  const QPointF px = NavigatorCube::projectToRect(QVector3D(0.5f, 0, 0), basis, rect);
  const QPointF py = NavigatorCube::projectToRect(QVector3D(0, 0.5f, 0), basis, rect);
  QVERIFY(px.x() > center.x());
  QVERIFY(py.y() < center.y()); // y-down item pixels
  // The cube spans the middle half of the rect (NDC 0.25..0.75,
  // ImGuizmo identity-scale ortho with cube half extent 0.5).
  QCOMPARE(px.x(), 100.0f + 0.75f * 128.0f);
}

void NavigatorCubeTests::hitTestSnapsFacePanels()
{
  NavigatorCube::RectF rect{0.0f, 0.0f, 128.0f, 128.0f};

  // Front view (camera at +Z looking -Z): visible faces are +Z (front),
  // +X (right), +Y (top).
  const NavigatorCube::CameraBasis front =
      NavigatorCube::cameraBasis(viewFromAzEl(0.0f, 0.0f));

  // Center of the view = cube center region -> the front face center panel
  // quantizes to the face-normal direction (0,0,1) -> box grid (1,1,0)
  // (index counts from the positive side, ImGuizmo.cpp:3070-3075).
  const NavigatorCube::Hit centerHit =
      NavigatorCube::hitTest(QPointF(64.0f, 64.0f), front, rect);
  QVERIFY(centerHit.isValid());
  QCOMPARE(centerHit.box, 1 * 9 + 1 * 3 + 0);
  QCOMPARE(NavigatorCube::snapDirectionForBox(centerHit.box), QVector3D(0, 0, 1));

  // Outside the rect: no hit.
  QVERIFY(!NavigatorCube::hitTest(QPointF(200.0f, 64.0f), front, rect).isValid());

  // Top face visible from an elevated view (azimuth 0, elevation 45): the
  // upper part of the silhouette belongs to the +Y face (at exactly el 0 the
  // +Y face is edge-on and back-face culled, ImGuizmo.cpp:2878-2880).
  const NavigatorCube::CameraBasis elevated =
      NavigatorCube::cameraBasis(viewFromAzEl(0.0f, 45.0f));
  const NavigatorCube::Hit topFace =
      NavigatorCube::hitTest(QPointF(64.0f, 40.0f), elevated, rect);
  QVERIFY(topFace.isValid());
  QVERIFY(topFace.faceNormal.y() > 0.5f);
  // Its center panel snaps to the up direction (grid index 0 on +y).
  QCOMPARE(topFace.box, 1 * 9 + 0 * 3 + 1);
}

void NavigatorCubeTests::snapDirectionForBoxCoversTwentySixDirections()
{
  // The center cell is the zero direction and must be rejected.
  QVERIFY(NavigatorCube::snapDirectionForBox(1 * 9 + 1 * 3 + 1).isNull());
  QVERIFY(NavigatorCube::snapDirectionForBox(-5).isNull());
  QVERIFY(NavigatorCube::snapDirectionForBox(27).isNull());

  // All other 26 cells map to unique nonzero unit directions
  // (ImGuizmo.cpp:3070-3075: (1-cx, 1-cy, 1-cz)).
  QList<QVector3D> seen;
  for (int box = 0; box <= 26; ++box) {
    const QVector3D dir = NavigatorCube::snapDirectionForBox(box);
    if (box == 13)
      continue;
    QVERIFY2(std::abs(dir.length() - 1.0f) < 1e-5f, "snap dirs must be unit");
    bool duplicate = false;
    for (const QVector3D &prev : seen) {
      if ((prev - dir).lengthSquared() < 1e-8f)
        duplicate = true;
    }
    QVERIFY2(!duplicate, "each grid cell maps to a unique direction");
    seen.append(dir);
  }
  QCOMPARE(seen.size(), 26);
}

void NavigatorCubeTests::dragRotateClampsAtHorizon()
{
  // Dragging below the horizon must not push the direction underneath the
  // ground plane (ImGuizmo.cpp:3097-3100).
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(viewFromAzEl(0.0f, 20.0f));
  // A large downward drag (around the camera right axis).
  const QVector3D dragged = NavigatorCube::dragRotateClamped(
      basis.forward, basis.right, 0.0f, 3.0f);
  QVERIFY(dragged.y() >= -1e-5f);
  QVERIFY(dragged.length() > 0.99f);
}

void NavigatorCubeTests::orientationForDirectionMatchesCameraControllerConvention()
{
  // Front direction (0,0,1) -> azimuth 0, elevation 0 (CameraController
  // measures azimuth from +Z toward +X).
  float az = 0.0f;
  float el = 0.0f;
  NavigatorCube::orientationForDirection(QVector3D(0, 0, 1), az, el);
  QCOMPARE(az, 0.0f);
  QCOMPARE(el, 0.0f);

  // +X -> azimuth 90.
  NavigatorCube::orientationForDirection(QVector3D(1, 0, 0), az, el);
  QCOMPARE(az, 90.0f);
  QCOMPARE(el, 0.0f);

  // Up -> exact +90: the quaternion camera has no pole degeneracy, so the
  // snap reaches the exact top view (P14 CAM-PARITY, upstream select_view).
  NavigatorCube::orientationForDirection(QVector3D(0, 1, 0), az, el);
  QVERIFY(std::isnan(az)); // azimuth undefined; caller preserves it
  QCOMPARE(el, 90.0f);

  NavigatorCube::orientationForDirection(QVector3D(0, -1, 0), az, el);
  QCOMPARE(el, -90.0f);

  // Isometric direction ~ az 45 / el 35.
  NavigatorCube::orientationForDirection(
      QVector3D(1, 1, 1).normalized(), az, el);
  QCOMPARE(az, 45.0f);
  QVERIFY(qAbs(el - 35.26f) < 0.1f);
}

QTEST_MAIN(NavigatorCubeTests)
#include "NavigatorCubeTests.moc"
