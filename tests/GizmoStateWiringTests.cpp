// GizmoStateWiringTests - verifies the Phase 67 gizmoCenter computation
// (GWIRE-02): the static helper RhiViewportRenderer::computeGizmoCenterFromBatches
// must return the AABB midpoint of the batch matching the selected
// sourceObjectIndex, or origin when there is no usable selection.
//
// AUTOMOC caveat (single-file QtTest, see tests/GizmoMathTests.cpp:1-10):
// after editing private slots, delete
// build/GizmoStateWiringTests_autogen/timestamp before incremental rebuilds.

#include <QtTest>
#include <QList>
#include <QVector3D>

#include "core/rendering/GizmoCenter.h"
#include "qml_gui/Renderer/PrepareSceneData.h"

namespace
{
  PrepareSceneData::ModelBatch makeBatch(int sourceObjectIndex,
                                         float minX, float minY, float minZ,
                                         float maxX, float maxY, float maxZ)
  {
    PrepareSceneData::ModelBatch b;
    b.sourceObjectIndex = sourceObjectIndex;
    b.bounds.minX = minX; b.bounds.minY = minY; b.bounds.minZ = minZ;
    b.bounds.maxX = maxX; b.bounds.maxY = maxY; b.bounds.maxZ = maxZ;
    return b;
  }

  bool approxVec(const QVector3D &a, const QVector3D &b, float eps = 1e-4f)
  {
    return std::abs(a.x() - b.x()) <= eps &&
           std::abs(a.y() - b.y()) <= eps &&
           std::abs(a.z() - b.z()) <= eps;
  }
}

class GizmoStateWiringTests final : public QObject
{
  Q_OBJECT

private slots:
  void testNoSelectionReturnsOrigin();
  void testSelectionNotFoundReturnsOrigin();
  void testSingleBatchReturnsMidpoint();
  void testMultiBatchFindsSelected();
  void testBatchBoundsWithNegativeRanges();
};

void GizmoStateWiringTests::testNoSelectionReturnsOrigin()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(0, 0, 0, 0, 10, 20, 30));
  QVector3D center = GizmoCenter::fromSelectedBatch(-1, batches);
  QVERIFY2(approxVec(center, QVector3D(0, 0, 0)),
           "selectedSourceObjectIndex=-1 must return origin");
}

void GizmoStateWiringTests::testSelectionNotFoundReturnsOrigin()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(0, 0, 0, 0, 10, 20, 30));
  batches.append(makeBatch(1, -5, -5, -5, 5, 5, 5));
  QVector3D center = GizmoCenter::fromSelectedBatch(99, batches);
  QVERIFY2(approxVec(center, QVector3D(0, 0, 0)),
           "selected index not in batches must return origin");
}

void GizmoStateWiringTests::testSingleBatchReturnsMidpoint()
{
  QList<PrepareSceneData::ModelBatch> batches;
  // Bounds: X[0,10] Y[20,40] Z[-5,5] -> midpoint (5, 30, 0).
  batches.append(makeBatch(7, 0.f, 20.f, -5.f, 10.f, 40.f, 5.f));
  QVector3D center = GizmoCenter::fromSelectedBatch(7, batches);
  QVERIFY2(approxVec(center, QVector3D(5.f, 30.f, 0.f)),
           "single-batch midpoint must be ((min+max)/2) on each axis");
}

void GizmoStateWiringTests::testMultiBatchFindsSelected()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(0, 0.f, 0.f, 0.f, 2.f, 2.f, 2.f));    // midpoint (1,1,1)
  batches.append(makeBatch(2, 10.f, 20.f, 30.f, 20.f, 40.f, 50.f)); // midpoint (15,30,40)
  batches.append(makeBatch(5, -1.f, -1.f, -1.f, 1.f, 1.f, 1.f));  // midpoint (0,0,0)
  QVector3D center = GizmoCenter::fromSelectedBatch(2, batches);
  QVERIFY2(approxVec(center, QVector3D(15.f, 30.f, 40.f)),
           "multi-batch: must pick the midpoint of the matching sourceObjectIndex only");
}

void GizmoStateWiringTests::testBatchBoundsWithNegativeRanges()
{
  QList<PrepareSceneData::ModelBatch> batches;
  // Bounds entirely in negative space: X[-20,-10] Y[-40,-20] Z[-5,5] -> midpoint (-15,-30,0).
  batches.append(makeBatch(3, -20.f, -40.f, -5.f, -10.f, -20.f, 5.f));
  QVector3D center = GizmoCenter::fromSelectedBatch(3, batches);
  QVERIFY2(approxVec(center, QVector3D(-15.f, -30.f, 0.f)),
           "negative-range bounds must still produce the correct midpoint");
}

QTEST_MAIN(GizmoStateWiringTests)
#include "GizmoStateWiringTests.moc"

