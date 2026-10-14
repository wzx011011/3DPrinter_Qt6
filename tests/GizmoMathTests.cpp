// GizmoMathTests - pure-math unit tests for the gizmo pick/drag library.
//
// Phase 65 (GMATH-01/02/03). Covers the seven GizmoMath functions with
// deterministic, hand-derived inputs and expected outputs. The expected
// values are computed independently of GizmoMath (by hand, on paper) so the
// tests genuinely verify the math rather than mirroring it.
//
// AUTOMOC caveat (single-file QtTest with cpp-internal Q_OBJECT, see
// tests/ViewModelSmokeTests.cpp:1-10): after editing private slots here,
// delete build/GizmoMathTests_autogen/timestamp before any incremental
// rebuild, or re-run cmake configure (the canonical verify script does this
// automatically).

#include <QtTest>
#include <QMatrix4x4>
#include <QSize>
#include <QVector3D>
#include <QVector4D>

#include "core/rendering/GizmoMath.h"

#include <cmath>

namespace
{
  // Float comparison helper with explicit tolerance (qFuzzyCompare is
  // unreliable around zero). Used for ray origins/directions/t-values.
  bool approx(float a, float b, float eps = 1e-4f)
  {
    return std::abs(a - b) <= eps;
  }

  bool approxVec(const QVector3D &a, const QVector3D &b, float eps = 1e-4f)
  {
    return approx(a.x(), b.x(), eps) && approx(a.y(), b.y(), eps) &&
           approx(a.z(), b.z(), eps);
  }

  // QString::arg(float) was removed in Qt 6, so message formatting for
  // QVERIFY2 descriptions goes through this helper. Returns a QByteArray
  // suitable for direct use as the description argument (no qPrintable needed
  // since QVERIFY2 accepts const char*).
  QByteArray msg(const char *fmt, float a, float b = 0.f, float c = 0.f)
  {
    return QString::asprintf(fmt, double(a), double(b), double(c)).toLocal8Bit();
  }

  // A camera looking down -Z from eye=(0,0,eyeZ) toward the origin, up=+Y.
  // viewMatrix = translate(0, 0, -eyeZ). projMatrix = perspective(fovY deg,
  // aspect, near, far). This is the simplest setup for hand-derived rays:
  // the screen-center ray runs along -Z through the origin.
  struct LookDownZ
  {
    QSize viewSize{800, 600};
    float eyeZ = 5.f;
    float fovY = 45.f;
    float aspect = 800.f / 600.f;
    float nearZ = 0.1f;
    float farZ = 100.f;

    QMatrix4x4 projMatrix() const
    {
      QMatrix4x4 p;
      p.perspective(fovY, aspect, nearZ, farZ);
      return p;
    }
    QMatrix4x4 viewMatrix() const
    {
      QMatrix4x4 v;
      v.translate(0.f, 0.f, -eyeZ);
      return v;
    }
    QVector3D eye() const { return {0.f, 0.f, eyeZ}; }
    float sxCenter() const { return float(viewSize.width()) / 2.f; }
    float syCenter() const { return float(viewSize.height()) / 2.f; }
  };

  // Projects a world point to widget pixel coordinates through the same
  // proj*view chain computeRay inverts, so a pick test can aim the cursor at
  // an exact world-space body point (X3 grabber-picking tests).
  QPointF worldToScreen(const QVector3D &world, const QMatrix4x4 &proj,
                        const QMatrix4x4 &view, const QSize &viewSize)
  {
    const QVector4D clip = proj * view * QVector4D(world, 1.0f);
    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();
    return QPointF(float((ndcX + 1.0) * 0.5 * viewSize.width()),
                   float((1.0 - ndcY) * 0.5 * viewSize.height()));
  }
}

class GizmoMathTests final : public QObject
{
  Q_OBJECT

private slots:
  // ---- computeRay ----
  void testComputeRayCenterIsotropic();
  void testComputeRayDegenerateViewport();

  // ---- rayXZIntersect ----
  void testRayXZIntersectStraightDown();
  void testRayXZIntersectParallelRay();

  // ---- rayToAxisT ----
  void testRayToAxisTPerpendicular();
  void testRayToAxisTParallelLines();

  // ---- pickMoveAxis ----
  void testPickMoveAxisNoSelection();
  void testPickMoveAxisHitsXAxis();
  void testPickMoveAxisMissesWhenFarOffAxis();

  // ---- pickRotateAxis ----
  void testPickRotateAxisNoSelection();
  void testPickRotateAxisHitsXRing();

  // ---- pickScaleAxis ----
  void testPickScaleAxisNoSelection();
  void testPickScaleAxisHitsXBoxHandle();

  // ---- computeRotateAngle ----
  void testComputeRotateAngleReferenceIsZero();
  void testComputeRotateAngleParallelPlaneFallback();
};

// ===========================================================================
// computeRay
// ===========================================================================

void GizmoMathTests::testComputeRayCenterIsotropic()
{
  // Camera at eye=(0,0,5) looking down -Z. Screen center cursor (400, 300).
  // The ray should originate on the near plane (z just below 5) and point
  // along -Z. Origin: computeRay unprojects NDC z=-1 (near plane) so origin
  // sits at eye.z - small epsilon; direction is (0,0,-1).
  LookDownZ cam;
  auto [orig, dir] = GizmoMath::computeRay(cam.sxCenter(), cam.syCenter(),
                                           cam.viewSize, cam.projMatrix(),
                                           cam.viewMatrix());

  // Direction is straight down -Z (within float epsilon).
  QVERIFY2(approxVec(dir, {0.f, 0.f, -1.f}),
           msg("center ray dir should be (0,0,-1), got (%f,%f,%f)",
               dir.x(), dir.y(), dir.z()));

  // Origin is on the near plane in front of the eye. With near=0.1 and
  // eye.z=5, the near plane in world space is at z = eye.z - (far-near)*...
  // Easier check: origin.z must be strictly less than eyeZ (5.0) and
  // greater than eyeZ - 1 (well inside the frustum).
  QVERIFY2(orig.z() < cam.eyeZ,
           msg("orig.z (%f) must be < eyeZ (%f)", orig.z(), cam.eyeZ));
  QVERIFY2(orig.z() > cam.eyeZ - 1.f,
           msg("orig.z (%f) must be > eyeZ-1 (%f)", orig.z(), cam.eyeZ - 1.f));
  // Origin x,y ~ 0 (the center ray passes through the view axis).
  QVERIFY2(approx(orig.x(), 0.f),
           msg("orig.x should be ~0, got %f", orig.x()));
  QVERIFY2(approx(orig.y(), 0.f),
           msg("orig.y should be ~0, got %f", orig.y()));
}

void GizmoMathTests::testComputeRayDegenerateViewport()
{
  // w=0 or h=0 -> early return {{0,0,0},{1,0,0}}.
  QSize zero{0, 0};
  LookDownZ cam;
  auto [orig, dir] = GizmoMath::computeRay(400.f, 300.f, zero,
                                           cam.projMatrix(), cam.viewMatrix());
  QVERIFY2(approxVec(orig, {0.f, 0.f, 0.f}),
           "degenerate viewport must return zero origin");
  QVERIFY2(approxVec(dir, {1.f, 0.f, 0.f}),
           "degenerate viewport must return (1,0,0) direction");
}

// ===========================================================================
// rayXZIntersect
// ===========================================================================

void GizmoMathTests::testRayXZIntersectStraightDown()
{
  // Camera looking straight DOWN (-Y): eye=(0,5,0), center=origin,
  // up=(0,0,1). Build the proj/view by hand so the screen-center ray
  // points along -Y and crosses Y=0 at the origin.
  QSize view{800, 600};
  QMatrix4x4 proj;
  proj.perspective(45.f, 800.f / 600.f, 0.1f, 100.f);
  // View matrix: lookAt(eye=(0,5,0), center=(0,0,0), up=(0,0,1)).
  QMatrix4x4 viewM;
  viewM.lookAt(QVector3D(0, 5, 0), QVector3D(0, 0, 0), QVector3D(0, 0, 1));

  float sxCenter = 400.f, syCenter = 300.f;
  QVector3D hit = GizmoMath::rayXZIntersect(sxCenter, syCenter, view, proj, viewM);
  // The center ray hits the Y=0 plane somewhere; y must be ~0.
  QVERIFY2(approx(hit.y(), 0.f),
           msg("hit.y should be ~0, got %f", hit.y()));
  QVERIFY2(std::isfinite(hit.x()) && std::isfinite(hit.z()),
           msg("hit.x/z must be finite, got (%f,%f)", hit.x(), hit.z()));
}

void GizmoMathTests::testRayXZIntersectParallelRay()
{
  // Construct a ray that is parallel to the Y=0 plane (dir.y() == 0).
  // Use a camera looking along +X so the center ray has zero Y component.
  // Easier: call rayXZIntersect with a camera whose center ray is horizontal.
  QSize view{800, 600};
  QMatrix4x4 proj;
  proj.perspective(45.f, 800.f / 600.f, 0.1f, 100.f);
  // lookAt(eye=(5,0,0), center=(0,0,0), up=(0,1,0)) -> center ray along -X.
  QMatrix4x4 viewM;
  viewM.lookAt(QVector3D(5, 0, 0), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

  QVector3D hit = GizmoMath::rayXZIntersect(400.f, 300.f, view, proj, viewM);
  // Ray parallel to Y=0 -> function returns {0,0,0}.
  QVERIFY2(approxVec(hit, {0.f, 0.f, 0.f}),
           msg("parallel ray should return origin, got (%f,%f,%f)",
               hit.x(), hit.y(), hit.z()));
}

// ===========================================================================
// rayToAxisT
// ===========================================================================

void GizmoMathTests::testRayToAxisTPerpendicular()
{
  // orig=(0,0,0), dir=(0,0,1), axisDir=(1,0,0), gizmoCenter=(0,0,0).
  // w = orig-gizmoCenter = (0,0,0); b=dot(dir,axisDir)=0; e=dot(axisDir,w)=0;
  // d=dot(dir,w)=0; denom=1-0=1; t = (e - b*d)/denom = 0.
  float t = GizmoMath::rayToAxisT(QVector3D(0, 0, 0), QVector3D(0, 0, 1),
                                  QVector3D(1, 0, 0), QVector3D(0, 0, 0));
  QVERIFY2(approx(t, 0.f),
           msg("perpendicular ray through gizmo center: t should be 0, got %f", t));
}

void GizmoMathTests::testRayToAxisTParallelLines()
{
  // dir=(1,0,0), axisDir=(1,0,0) -> denom = 1 - 1*1 = 0 -> parallel branch.
  // Returns e = dot(axisDir, w) where w = orig - gizmoCenter.
  // orig=(2,0,0), gizmoCenter=(0,0,0) -> w=(2,0,0); e = dot((1,0,0),(2,0,0)) = 2.
  float t = GizmoMath::rayToAxisT(QVector3D(2, 0, 0), QVector3D(1, 0, 0),
                                  QVector3D(1, 0, 0), QVector3D(0, 0, 0));
  QVERIFY2(approx(t, 2.f),
           msg("parallel ray returns e=2, got %f", t));
}

// ===========================================================================
// pickMoveAxis
// ===========================================================================

void GizmoMathTests::testPickMoveAxisNoSelection()
{
  LookDownZ cam;
  QVector3D gizmoCenter{0, 0, 0};
  int ax = GizmoMath::pickMoveAxis(cam.sxCenter(), cam.syCenter(), cam.viewSize,
                                   cam.projMatrix(), cam.viewMatrix(),
                                   gizmoCenter, cam.eye(),
                                   /*hasSelection=*/false);
  QCOMPARE(ax, 0);
}

void GizmoMathTests::testPickMoveAxisHitsXAxis()
{
  // X3: exact grabber-body picking - only the drawn CONE at the arrow tip
  // (local axial [0.78, 1.0], base radius 0.055) is hittable; the shaft line
  // is a dashed-line primitive, not a grabber (upstream GLGizmoMove.cpp cone
  // grabbers + render_grabber_connection). Rig: eyeZ=40 -> scale =
  // max(40*0.15, 5) = 6.0, cone spans axial [4.68, 6.0] with radius 0.33 -> 0.
  // The cursor aims at a point inside the X cone body (axial 5.34, radius
  // there ~0.165), projected to screen through the same proj*view chain the
  // pick ray inverts. (A sweep from the screen center would first cross the
  // Z cone - the +Z body points at this camera - so the pick must be pinned
  // to an exact body point instead.)
  LookDownZ cam;
  cam.eyeZ = 40.f;
  QVector3D gizmoCenter{0, 0, 0};
  const QPointF cursor = worldToScreen(QVector3D(5.34f, 0.05f, 0.0f),
                                       cam.projMatrix(), cam.viewMatrix(),
                                       cam.viewSize);
  const int ax = GizmoMath::pickMoveAxis(float(cursor.x()), float(cursor.y()),
                                         cam.viewSize, cam.projMatrix(),
                                         cam.viewMatrix(), gizmoCenter,
                                         cam.eye(), /*hasSelection=*/true);
  QCOMPARE(ax, 1);
}

void GizmoMathTests::testPickMoveAxisMissesWhenFarOffAxis()
{
  // Cursor at a screen corner far from any axis arrow. With hasSelection=true
  // and gizmoCenter=origin, a cursor at the very corner of an 800x600 viewport
  // looking down -Z should not hit any axis (the rays skim past the arrows).
  // Note: this depends on the camera FOV; with fov=45 and eye=5, the
  // half-extent at z=0 is tan(22.5)*5 ~ 2.07, so the X arrow (length 5) DOES
  // extend beyond the view frustum at z=0. A corner ray (e.g. top-right)
  // heads toward (2.07, 1.55, 0) which is well off all three axes -> 0.
  LookDownZ cam;
  QVector3D gizmoCenter{0, 0, 0};
  // Top-right corner cursor.
  int ax = GizmoMath::pickMoveAxis(float(cam.viewSize.width() - 1),
                                   float(cam.viewSize.height() - 1),
                                   cam.viewSize, cam.projMatrix(),
                                   cam.viewMatrix(), gizmoCenter, cam.eye(),
                                   /*hasSelection=*/true);
  QCOMPARE(ax, 0);
}

// ===========================================================================
// pickRotateAxis
// ===========================================================================

void GizmoMathTests::testPickRotateAxisNoSelection()
{
  LookDownZ cam;
  QVector3D gizmoCenter{0, 0, 0};
  int ax = GizmoMath::pickRotateAxis(cam.sxCenter(), cam.syCenter(),
                                     cam.viewSize, cam.projMatrix(),
                                     cam.viewMatrix(), gizmoCenter, cam.eye(),
                                     /*hasSelection=*/false);
  QCOMPARE(ax, 0);
}

void GizmoMathTests::testPickRotateAxisHitsXRing()
{
  // Rotate rings: axis 1=X (YZ plane, normal +X), axis 2=Y (XZ plane),
  // axis 3=Z (XY plane, normal +Z). Ring geometry: majorR 0.7, minorR 0.035
  // gizmo-local; with fov=90 and eyeZ=5 the scale is 5.0, so the Z ring
  // center circle has radius 3.5 and tube radius 0.175, well inside the
  // z=0-plane half-extent tan(45)*5 = 5.
  //
  // X3: exact body picking - the cursor aims at a point on the Z ring's
  // center circle at 45 degrees (2.475, 2.475, 0), inside the tube and 3.5
  // away from the X/Y ring circles, projected to screen through the same
  // proj*view chain the pick ray inverts. (A sweep from the screen center
  // would first run straight through the X ring tube at (0,0,3.5) - the ring
  // crossing the view axis - so the pick is pinned to an exact body point.)
  QSize view{800, 600};
  QMatrix4x4 proj;
  proj.perspective(90.f, 800.f / 600.f, 0.1f, 100.f);
  QMatrix4x4 viewM;
  viewM.translate(0.f, 0.f, -5.f);
  QVector3D eye{0.f, 0.f, 5.f};
  QVector3D gizmoCenter{0, 0, 0};

  const QPointF cursor = worldToScreen(QVector3D(2.4749f, 2.4749f, 0.0f),
                                       proj, viewM, view);
  const int ax = GizmoMath::pickRotateAxis(float(cursor.x()), float(cursor.y()),
                                           view, proj, viewM, gizmoCenter,
                                           eye, /*hasSelection=*/true);
  QCOMPARE(ax, 3);
}

// ===========================================================================
// pickScaleAxis
// ===========================================================================

void GizmoMathTests::testPickScaleAxisNoSelection()
{
  LookDownZ cam;
  QVector3D gizmoCenter{0, 0, 0};
  int ax = GizmoMath::pickScaleAxis(cam.sxCenter(), cam.syCenter(),
                                    cam.viewSize, cam.projMatrix(),
                                    cam.viewMatrix(), gizmoCenter, cam.eye(),
                                    /*hasSelection=*/false);
  QCOMPARE(ax, 0);
}

void GizmoMathTests::testPickScaleAxisHitsXBoxHandle()
{
  // X3: exact grabber-body picking - the scale grabber is the drawn BOX at
  // the end of each shaft (0.08 cubed centered at 0.78, gizmo-local); the
  // shaft line is not pickable. Rig: eyeZ=40 -> scale 6.0, box centered at
  // (4.68, 0, 0) with 0.24 half extents. The cursor aims at the box interior
  // (4.68, 0.1, 0.1), projected to screen through the same proj*view chain
  // the pick ray inverts. (A sweep from the screen center would first cross
  // the Z box - the +Z body points at this camera.)
  LookDownZ cam;
  cam.eyeZ = 40.f;
  QVector3D gizmoCenter{0, 0, 0};
  const QPointF cursor = worldToScreen(QVector3D(4.68f, 0.10f, 0.10f),
                                       cam.projMatrix(), cam.viewMatrix(),
                                       cam.viewSize);
  const int ax = GizmoMath::pickScaleAxis(float(cursor.x()), float(cursor.y()),
                                          cam.viewSize, cam.projMatrix(),
                                          cam.viewMatrix(), gizmoCenter,
                                          cam.eye(), /*hasSelection=*/true);
  QCOMPARE(ax, 1);
}

// ===========================================================================
// computeRotateAngle
// ===========================================================================

void GizmoMathTests::testComputeRotateAngleReferenceIsZero()
{
  // axis=3 (Z plane, normal=(0,0,1), ref=(1,0,0)). A cursor whose ray hits
  // the +X axis at (1, 0, 0) yields v = hit - gizmoCenter = (1,0,0).
  // angle = atan2(dot(cross(ref,v).normalized, normal), dot(ref,v))
  //       = atan2(dot((0,0,1).normalized, (0,0,1)), 1)
  //       = atan2(1, 1) = pi/4.
  // Wait: cross((1,0,0),(1,0,0)) = (0,0,0); .normalized() of zero vector is
  // undefined (Qt returns (0,0,0) or NaN). Re-derive with a v that is NOT
  // parallel to ref: aim at (1, 1, 0) instead.
  //
  // v = (1,1,0); cross(ref, v) = cross((1,0,0),(1,1,0)) = (0*0-0*1, 0*1-1*0, 1*1-0*1) = (0,0,1).
  // .normalized() = (0,0,1); dot((0,0,1),(0,0,1)) = 1; dot(ref,v) = 1.
  // angle = atan2(1, 1) = pi/4.
  //
  // To hit world (1,1,0) we need a cursor whose ray passes through it. With
  // eye=(0,0,5) looking down -Z, screen pixel for (1,1,0) is found by
  // scanning sx (rightward for +X) and sy (upward for +Y) until the ray
  // passes near (1,1,0). Use computeRay to find the closest pixel.
  LookDownZ cam;
  QVector3D gizmoCenter{0, 0, 0};
  // Find the pixel whose ray passes closest to world (1,1,0).
  // Step 1 pixel for sub-2px sampling accuracy so the angle assertion can
  // stay tight (5e-3 rad).
  float bestSx = -1.f, bestSy = -1.f, bestDist = 1e9f;
  for (int sx = 0; sx < cam.viewSize.width(); sx += 1)
  {
    for (int sy = 0; sy < cam.viewSize.height(); sy += 1)
    {
      auto [o, d] = GizmoMath::computeRay(float(sx), float(sy), cam.viewSize,
                                          cam.projMatrix(), cam.viewMatrix());
      // Closest point on ray to target (1,1,0).
      QVector3D target{1.f, 1.f, 0.f};
      QVector3D w = target - o;
      float t = QVector3D::dotProduct(w, d);
      if (t < 0) continue;
      QVector3D closest = o + t * d;
      float dist = (closest - target).length();
      if (dist < bestDist)
      {
        bestDist = dist;
        bestSx = float(sx);
        bestSy = float(sy);
      }
    }
  }
  QVERIFY2(bestDist < 0.1f,
           msg("could not find a pixel near (1,1,0); best dist %f", bestDist));
  float angle = GizmoMath::computeRotateAngle(bestSx, bestSy, /*axis=*/3,
                                              cam.viewSize, cam.projMatrix(),
                                              cam.viewMatrix(), gizmoCenter,
                                              /*rotateStartAngle=*/0.f);
  QVERIFY2(approx(angle, float(M_PI) / 4.f, 5e-3f),
           msg("angle should be pi/4 (~0.7854) within sampling tolerance, got %f", angle));
}

void GizmoMathTests::testComputeRotateAngleParallelPlaneFallback()
{
  // Construct a ray parallel to the Z rotation plane (normal=(0,0,1)).
  // A camera looking along +Z has its center ray parallel to the plane
  // normal -> denom = dot(dir, normal) ~ +-1 (NOT parallel; we need denom~0).
  // We need dir with zero Z component. Camera looking along +X has center
  // ray along -X (dir=( -1,0,0 )), so dot(dir,(0,0,1)) = 0 -> parallel branch
  // -> returns rotateStartAngle.
  QSize view{800, 600};
  QMatrix4x4 proj;
  proj.perspective(45.f, 800.f / 600.f, 0.1f, 100.f);
  QMatrix4x4 viewM;
  viewM.lookAt(QVector3D(5, 0, 0), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

  QVector3D gizmoCenter{0, 0, 0};
  const float startAngle = 1.234f;
  float angle = GizmoMath::computeRotateAngle(400.f, 300.f, /*axis=*/3, view,
                                              proj, viewM, gizmoCenter,
                                              startAngle);
  QVERIFY2(approx(angle, startAngle),
           msg("parallel plane should return startAngle=%f, got %f", startAngle, angle));
}

QTEST_MAIN(GizmoMathTests)
#include "GizmoMathTests.moc"
