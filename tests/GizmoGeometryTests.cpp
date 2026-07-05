// GizmoGeometryTests - snapshot tests pinning the vertex counts, per-axis
// colors, bounding ranges, and offset metadata of the three gizmo geometry
// builders (Phase 66, GGEO-01/02/03).
//
// These pin the layout so Phase 67 (RHI state wiring) and Phase 68 (RHI
// rendering) can consume the same vertices via QRhi without re-deriving the
// counts/offsets. If any snapshot drifts, the change is intentional and the
// expected values here should be updated to match.
//
// AUTOMOC caveat (single-file QtTest, see tests/GizmoMathTests.cpp:1-10):
// after editing private slots here, delete
// build/GizmoGeometryTests_autogen/timestamp before any incremental rebuild,
// or re-run cmake configure (the canonical verify script does this).

#include <QtTest>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QVector3D>

#include "core/rendering/GizmoGeometry.h"
#include "core/rendering/GizmoVertex.h"

#include <cmath>
#include <cfloat>

namespace
{
  QByteArray msg(const char *fmt, int a = 0, int b = 0, int c = 0)
  {
    return QString::asprintf(fmt, a, b, c).toLocal8Bit();
  }

  QByteArray msgF(const char *fmt, float a = 0.f, float b = 0.f, float c = 0.f)
  {
    return QString::asprintf(fmt, double(a), double(b), double(c)).toLocal8Bit();
  }

  bool approx(float a, float b, float eps = 1e-4f) { return std::abs(a - b) <= eps; }

  // Returns true if the vertex color matches kAxisColorRGB[axis] with alpha 1.
  bool colorMatchesAxis(const GizmoVertex &v, int axis)
  {
    const float *rgb = GizmoGeometry::kAxisColorRGB[axis];
    return approx(v.r, rgb[0]) && approx(v.g, rgb[1]) &&
           approx(v.b, rgb[2]) && approx(v.a, 1.0f);
  }
}

class GizmoGeometryTests final : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  // Move gizmo
  void testMoveGizmoVertexCount();
  void testMoveGizmoAxisColors();
  void testMoveGizmoBoundingBox();
  void testMoveGizmoHasNegativePerpendicularCoords();
  void testMoveGizmoOffsets();

  // Rotate gizmo
  void testRotateGizmoVertexCount();
  void testRotateGizmoAxisColors();
  void testRotateGizmoBoundingBox();
  void testRotateGizmoOffsets();

  // Scale gizmo
  void testScaleGizmoVertexCount();
  void testScaleGizmoAxisColors();
  void testScaleGizmoBoundingBox();
  void testScaleGizmoOffsets();

  // Cut plane + wipe tower
  void testCutPlaneGeometry();
  void testCutPlaneAxisColors();
  void testWipeTowerGeometry();
  void testWipeTowerRejectsInvalidDimensions();

  // Shared color constants
  void testAxisColorConstants();
};

void GizmoGeometryTests::initTestCase()
{
  // Sanity: the builders must be callable without any GL/RHI context.
  QVERIFY(GizmoGeometry::buildMoveGizmoVertices().size() > 0);
  QVERIFY(GizmoGeometry::buildRotateGizmoVertices().size() > 0);
  QVERIFY(GizmoGeometry::buildScaleGizmoVertices().size() > 0);
}

// ===========================================================================
// Move gizmo
// ===========================================================================

void GizmoGeometryTests::testMoveGizmoVertexCount()
{
  auto verts = GizmoGeometry::buildMoveGizmoVertices();
  QCOMPARE(verts.size(), 114);
}

void GizmoGeometryTests::testMoveGizmoAxisColors()
{
  auto verts = GizmoGeometry::buildMoveGizmoVertices();
  QCOMPARE(verts.size(), 114);
  // 3 axes x 38 verts each: [0..37]=X, [38..75]=Y, [76..113]=Z.
  for (int i = 0; i < 114; ++i)
  {
    int axis = i / 38;
    QVERIFY2(colorMatchesAxis(verts[i], axis),
             msg("move vert %d has wrong color for axis %d (i=%d)", i, axis, 0));
  }
}

void GizmoGeometryTests::testMoveGizmoBoundingBox()
{
  auto verts = GizmoGeometry::buildMoveGizmoVertices();
  float minxyz[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float maxxyz[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
  for (const auto &v : verts)
  {
    float coords[3] = {v.x, v.y, v.z};
    for (int k = 0; k < 3; ++k)
    {
      if (coords[k] < minxyz[k]) minxyz[k] = coords[k];
      if (coords[k] > maxxyz[k]) maxxyz[k] = coords[k];
    }
  }
  // Each axis spans [0, 1.0] on its own coordinate (shaft 0->0.78, cone tip 1.0).
  // Perpendicular cone-base coords span [-0.055, 0.055]. The origin (0,0,0) is
  // shared by all three axis shafts, so the global min on each coordinate is
  // at most 0 (origin) and at least -0.055 (perpendicular cone radius from
  // the OTHER two axes' cones). Assert the global min is within [-0.06, 0.01]
  // and the global max is ~1.0 (cone tip on the active axis).
  for (int k = 0; k < 3; ++k)
  {
    QVERIFY2(approx(maxxyz[k], 1.0f, 1e-3f),
             msgF("axis coord max should be ~1.0 (cone tip), got %f", maxxyz[k], 0, 0));
    QVERIFY2(minxyz[k] >= -0.06f && minxyz[k] <= 0.01f,
             msgF("axis coord min should be in [-0.06, 0.01] (origin or -cone radius), got %f", minxyz[k], 0, 0));
  }
}

void GizmoGeometryTests::testMoveGizmoHasNegativePerpendicularCoords()
{
  // The Y-axis cone verts (indices 38-75) carry negative X (e.g. -0.02750,
  // -0.05500) since X is a perpendicular axis for the Y shaft. Same for
  // Z-axis cone verts carrying negative X/Y. This explicitly counts negative
  // entries per coordinate to confirm the perpendicular cone radius is real.
  auto verts = GizmoGeometry::buildMoveGizmoVertices();
  QCOMPARE(verts.size(), 114);
  int negX = 0, negY = 0, negZ = 0;
  for (const auto &v : verts)
  {
    if (v.x < -1e-5f) ++negX;
    if (v.y < -1e-5f) ++negY;
    if (v.z < -1e-5f) ++negZ;
  }
  // Each axis has 12 negative-perpendicular verts (cone base radius spans
  // both sides). X is perpendicular for Y and Z axes -> ~24 negX. Same for
  // Y and Z. Assert >= 10 each (loose lower bound to allow for indexing
  // variance).
  QVERIFY2(negX >= 10, msg("expected >=10 verts with negative X (Y/Z cone radius), got %d", negX, 0, 0));
  QVERIFY2(negY >= 10, msg("expected >=10 verts with negative Y (X/Z cone radius), got %d", negY, 0, 0));
  QVERIFY2(negZ >= 10, msg("expected >=10 verts with negative Z (X/Y cone radius), got %d", negZ, 0, 0));
}

void GizmoGeometryTests::testMoveGizmoOffsets()
{
  GizmoGeometryOffsets offsets;
  auto verts = GizmoGeometry::buildMoveGizmoVertices(&offsets);
  QCOMPARE(verts.size(), 114);
  // shaftBase[ax] = ax*38; coneBase[ax] = ax*38 + 2 (matches GL lines 1061-1066).
  QCOMPARE(offsets.shaftBase[0], 0);
  QCOMPARE(offsets.coneBase[0], 2);
  QCOMPARE(offsets.shaftBase[1], 38);
  QCOMPARE(offsets.coneBase[1], 40);
  QCOMPARE(offsets.shaftBase[2], 76);
  QCOMPARE(offsets.coneBase[2], 78);
  QCOMPARE(offsets.shaftVertCount, 2);
  QCOMPARE(offsets.coneVertCount, 36);
}

// ===========================================================================
// Rotate gizmo
// ===========================================================================

void GizmoGeometryTests::testRotateGizmoVertexCount()
{
  const QFile geometrySource(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(
      QStringLiteral("src/core/rendering/GizmoGeometry.cpp")));
  QVERIFY2(geometrySource.open(QIODevice::ReadOnly | QIODevice::Text),
           "Unable to read GizmoGeometry.cpp");
  const QString source = QString::fromUtf8(geometrySource.readAll());
  QVERIFY2(source.contains(QStringLiteral("verts.reserve(3 * kRotateVertsPerRing);")),
           "Rotate gizmo reserve must use vertex count, not byte count");
  QVERIFY2(!source.contains(QStringLiteral("kRotateVertsPerRing * int(sizeof(GizmoVertex))")),
           "Rotate gizmo reserve must not multiply the vertex count by sizeof(GizmoVertex)");

  auto verts = GizmoGeometry::buildRotateGizmoVertices();
  // 3 rings x 48 segments x 6 sides x 6 verts = 5184.
  QCOMPARE(verts.size(), 5184);
}

void GizmoGeometryTests::testRotateGizmoAxisColors()
{
  auto verts = GizmoGeometry::buildRotateGizmoVertices();
  QCOMPARE(verts.size(), 5184);
  const int perRing = 5184 / 3; // 1728
  for (int i = 0; i < 5184; ++i)
  {
    int ring = i / perRing;
    QVERIFY2(colorMatchesAxis(verts[i], ring),
             msg("rotate vert %d has wrong color for ring %d (i=%d)", i, ring, 0));
  }
}

void GizmoGeometryTests::testRotateGizmoBoundingBox()
{
  auto verts = GizmoGeometry::buildRotateGizmoVertices();
  float maxAbs = 0.f;
  for (const auto &v : verts)
  {
    maxAbs = std::max(maxAbs, std::abs(v.x));
    maxAbs = std::max(maxAbs, std::abs(v.y));
    maxAbs = std::max(maxAbs, std::abs(v.z));
  }
  // majorR + minorR = 0.7 + 0.035 = 0.735.
  QVERIFY2(maxAbs <= 0.74f,
           msgF("rotate max coord magnitude should be <= 0.74 (majorR+minorR=0.735), got %f", maxAbs, 0, 0));
  QVERIFY2(maxAbs >= 0.73f,
           msgF("rotate max coord magnitude should be >= 0.73 (ring reaches majorR+minorR), got %f", maxAbs, 0, 0));
}

void GizmoGeometryTests::testRotateGizmoOffsets()
{
  GizmoGeometryOffsets offsets;
  auto verts = GizmoGeometry::buildRotateGizmoVertices(&offsets);
  QCOMPARE(verts.size(), 5184);
  QCOMPARE(offsets.ringBase[0], 0);
  QCOMPARE(offsets.ringBase[1], 1728);
  QCOMPARE(offsets.ringBase[2], 3456);
  QCOMPARE(offsets.ringVertCount, 1728);
}

// ===========================================================================
// Scale gizmo
// ===========================================================================

void GizmoGeometryTests::testScaleGizmoVertexCount()
{
  auto verts = GizmoGeometry::buildScaleGizmoVertices();
  // 3 axes x (2 shaft + 36 box) = 114.
  QCOMPARE(verts.size(), 114);
}

void GizmoGeometryTests::testScaleGizmoAxisColors()
{
  auto verts = GizmoGeometry::buildScaleGizmoVertices();
  QCOMPARE(verts.size(), 114);
  const int perAxis = 114 / 3; // 38
  for (int i = 0; i < 114; ++i)
  {
    int axis = i / perAxis;
    QVERIFY2(colorMatchesAxis(verts[i], axis),
             msg("scale vert %d has wrong color for axis %d (i=%d)", i, axis, 0));
  }
}

void GizmoGeometryTests::testScaleGizmoBoundingBox()
{
  auto verts = GizmoGeometry::buildScaleGizmoVertices();
  float minxyz[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
  float maxxyz[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
  for (const auto &v : verts)
  {
    float coords[3] = {v.x, v.y, v.z};
    for (int k = 0; k < 3; ++k)
    {
      if (coords[k] < minxyz[k]) minxyz[k] = coords[k];
      if (coords[k] > maxxyz[k]) maxxyz[k] = coords[k];
    }
  }
  // Box handle center is at 0.78 along the axis, box size 0.08, so the box
  // extends to 0.78 + 0.04 = 0.82 on the active axis. Shaft goes 0 -> 0.7.
  // Origin (0,0,0) is shared by all axes, so min is 0 on the active axis
  // and -0.04 on perpendicular (box faces). Combined min ~ -0.04.
  for (int k = 0; k < 3; ++k)
  {
    QVERIFY2(approx(maxxyz[k], 0.82f, 1e-3f),
             msgF("scale axis %d max should be ~0.82 (boxCenter+boxSize/2), got %f", maxxyz[k], 0, 0));
  }
  QVERIFY2(minxyz[0] >= -0.05f && minxyz[0] <= 0.01f,
           msgF("scale min X should be ~-0.04 or 0, got %f", minxyz[0], 0, 0));
}

void GizmoGeometryTests::testScaleGizmoOffsets()
{
  GizmoGeometryOffsets offsets;
  auto verts = GizmoGeometry::buildScaleGizmoVertices(&offsets);
  QCOMPARE(verts.size(), 114);
  // Per axis: 38 verts (2 shaft + 36 box). shaftBase[ax]=ax*38, boxBase[ax]=ax*38+2.
  QCOMPARE(offsets.shaftBase[0], 0);
  QCOMPARE(offsets.boxBase[0], 2);
  QCOMPARE(offsets.shaftBase[1], 38);
  QCOMPARE(offsets.boxBase[1], 40);
  QCOMPARE(offsets.shaftBase[2], 76);
  QCOMPARE(offsets.boxBase[2], 78);
  QCOMPARE(offsets.shaftVertCount, 2);
  QCOMPARE(offsets.boxVertCount, 36);
}

// ===========================================================================
// Cut plane + wipe tower
// ===========================================================================

void GizmoGeometryTests::testCutPlaneGeometry()
{
  const QVector3D boundsMin(-10.f, -5.f, 0.f);
  const QVector3D boundsMax(30.f, 15.f, 40.f);
  const auto fill = GizmoGeometry::buildCutPlaneVertices(boundsMin, boundsMax, 2, 12.5f);
  const auto outline = GizmoGeometry::buildCutPlaneOutlineVertices(boundsMin, boundsMax, 2, 12.5f);

  QCOMPARE(fill.size(), 6);
  QCOMPARE(outline.size(), 8);

  float minX = FLT_MAX;
  float maxX = -FLT_MAX;
  float minY = FLT_MAX;
  float maxY = -FLT_MAX;
  for (const auto &v : fill)
  {
    QVERIFY2(approx(v.z, 12.5f), msgF("cut plane z should equal cutPosition, got %f", v.z, 0, 0));
    QVERIFY2(approx(v.r, 0.3f) && approx(v.g, 0.3f) && approx(v.b, 1.0f),
             "Z cut plane fill should be blue");
    QVERIFY2(approx(v.a, 0.30f), msgF("cut plane fill alpha should be 0.30, got %f", v.a, 0, 0));
    minX = std::min(minX, v.x);
    maxX = std::max(maxX, v.x);
    minY = std::min(minY, v.y);
    maxY = std::max(maxY, v.y);
  }

  // X span 40 expands by 5 percent on each side -> [-12, 32].
  QVERIFY2(approx(minX, -12.f), msgF("cut plane min X should be -12, got %f", minX, 0, 0));
  QVERIFY2(approx(maxX, 32.f), msgF("cut plane max X should be 32, got %f", maxX, 0, 0));
  // Y span 20 expands by 5 percent on each side -> [-6, 16].
  QVERIFY2(approx(minY, -6.f), msgF("cut plane min Y should be -6, got %f", minY, 0, 0));
  QVERIFY2(approx(maxY, 16.f), msgF("cut plane max Y should be 16, got %f", maxY, 0, 0));

  for (const auto &v : outline)
  {
    QVERIFY2(approx(v.z, 12.5f), msgF("outline z should equal cutPosition, got %f", v.z, 0, 0));
    QVERIFY2(approx(v.a, 0.90f), msgF("outline alpha should be 0.90, got %f", v.a, 0, 0));
  }
}

void GizmoGeometryTests::testCutPlaneAxisColors()
{
  const QVector3D boundsMin(0.f, 0.f, 0.f);
  const QVector3D boundsMax(10.f, 20.f, 30.f);

  const auto xFill = GizmoGeometry::buildCutPlaneVertices(boundsMin, boundsMax, 0, 4.f);
  const auto yFill = GizmoGeometry::buildCutPlaneVertices(boundsMin, boundsMax, 1, 5.f);
  const auto zFill = GizmoGeometry::buildCutPlaneVertices(boundsMin, boundsMax, 2, 6.f);
  QCOMPARE(xFill.size(), 6);
  QCOMPARE(yFill.size(), 6);
  QCOMPARE(zFill.size(), 6);

  QVERIFY2(approx(xFill.first().r, 1.0f) && approx(xFill.first().g, 0.3f)
               && approx(xFill.first().b, 0.3f) && approx(xFill.first().a, 0.30f),
           "X cut fill should be red with alpha 0.30");
  QVERIFY2(approx(yFill.first().r, 0.3f) && approx(yFill.first().g, 1.0f)
               && approx(yFill.first().b, 0.3f) && approx(yFill.first().a, 0.30f),
           "Y cut fill should be green with alpha 0.30");
  QVERIFY2(approx(zFill.first().r, 0.3f) && approx(zFill.first().g, 0.3f)
               && approx(zFill.first().b, 1.0f) && approx(zFill.first().a, 0.30f),
           "Z cut fill should be blue with alpha 0.30");
}

void GizmoGeometryTests::testWipeTowerGeometry()
{
  const auto verts = GizmoGeometry::buildWipeTowerVertices(100.f, 25.f, 20.f, 10.f, 50.f);
  QCOMPARE(verts.size(), 36);

  float minX = FLT_MAX;
  float maxX = -FLT_MAX;
  float minY = FLT_MAX;
  float maxY = -FLT_MAX;
  float minZ = FLT_MAX;
  float maxZ = -FLT_MAX;
  for (const auto &v : verts)
  {
    minX = std::min(minX, v.x);
    maxX = std::max(maxX, v.x);
    minY = std::min(minY, v.y);
    maxY = std::max(maxY, v.y);
    minZ = std::min(minZ, v.z);
    maxZ = std::max(maxZ, v.z);
    QVERIFY2(approx(v.r, 0.35f) && approx(v.g, 0.60f) && approx(v.b, 0.85f),
             "wipe tower color should match GL path accent color");
    QVERIFY2(approx(v.a, 0.50f), msgF("wipe tower alpha should be 0.50, got %f", v.a, 0, 0));
  }

  QVERIFY2(approx(minX, 90.f), msgF("wipe tower min X should be 90, got %f", minX, 0, 0));
  QVERIFY2(approx(maxX, 110.f), msgF("wipe tower max X should be 110, got %f", maxX, 0, 0));
  QVERIFY2(approx(minZ, 20.f), msgF("wipe tower min Z should be 20, got %f", minZ, 0, 0));
  QVERIFY2(approx(maxZ, 30.f), msgF("wipe tower max Z should be 30, got %f", maxZ, 0, 0));
  QVERIFY2(approx(minY, -0.04f), msgF("wipe tower min Y should be -0.04, got %f", minY, 0, 0));
  QVERIFY2(approx(maxY, 49.96f), msgF("wipe tower max Y should be 49.96, got %f", maxY, 0, 0));
}

void GizmoGeometryTests::testWipeTowerRejectsInvalidDimensions()
{
  QVERIFY(GizmoGeometry::buildWipeTowerVertices(0.f, 0.f, 0.f, 10.f, 10.f).isEmpty());
  QVERIFY(GizmoGeometry::buildWipeTowerVertices(0.f, 0.f, 10.f, 0.f, 10.f).isEmpty());
  QVERIFY(GizmoGeometry::buildWipeTowerVertices(0.f, 0.f, 10.f, 10.f, 0.f).isEmpty());
  QVERIFY(GizmoGeometry::buildWipeTowerVertices(0.f, 0.f, -1.f, 10.f, 10.f).isEmpty());
}

// ===========================================================================
// Color constants
// ===========================================================================

void GizmoGeometryTests::testAxisColorConstants()
{
  // X red
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[0][0], 0.90f), "X red R should be 0.90");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[0][1], 0.18f), "X red G should be 0.18");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[0][2], 0.18f), "X red B should be 0.18");
  // Y green
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[1][0], 0.22f), "Y green R should be 0.22");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[1][1], 0.80f), "Y green G should be 0.80");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[1][2], 0.22f), "Y green B should be 0.22");
  // Z blue
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[2][0], 0.18f), "Z blue R should be 0.18");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[2][1], 0.40f), "Z blue G should be 0.40");
  QVERIFY2(approx(GizmoGeometry::kAxisColorRGB[2][2], 0.95f), "Z blue B should be 0.95");
}

QTEST_MAIN(GizmoGeometryTests)
#include "GizmoGeometryTests.moc"
