// GizmoStateWiringTests - verifies the Phase 67 gizmoCenter computation
// (GWIRE-02): the static helper RhiViewportRenderer::computeGizmoCenterFromBatches
// must return the union AABB midpoint of all batches matching the selected
// sourceObjectIndex, or origin when there is no usable selection.
//
// P15.11 (MULTICENTER): the multi-selection overload fromSelectedIndices()
// must union the AABBs of ALL listed source objects (upstream
// Selection::get_bounding_box().center()), not just the primary one.
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
  // P15.11 (MULTICENTER): union AABB center across multiple source objects.
  void testMultiObjectUnionAcrossSourceObjects();
  void testMultiObjectListOrderAndStaleEntries();
  void testMultiObjectEmptyListReturnsOrigin();
  void testSingleObjectOverloadDelegatesToIndices();
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
  batches.append(makeBatch(0, 0.f, 0.f, 0.f, 2.f, 2.f, 2.f));
  // The selected source object spans disjoint render batches. The union is
  // X[-10,20] Y[-20,40] Z[-30,50] -> center (5,10,10).
  batches.append(makeBatch(2, 10.f, 20.f, 30.f, 20.f, 40.f, 50.f));
  batches.append(makeBatch(2, -10.f, -20.f, -30.f, -2.f, 0.f, 5.f));
  batches.append(makeBatch(5, -1.f, -1.f, -1.f, 1.f, 1.f, 1.f));
  QVector3D center = GizmoCenter::fromSelectedBatch(2, batches);
  QVERIFY2(approxVec(center, QVector3D(5.f, 10.f, 10.f)),
           "multi-batch selection must use the union AABB center");
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

// P15.11 (MULTICENTER): two disjoint source objects selected together. The
// gizmo pivot must be the midpoint of the UNION of both objects' bounds
// (upstream Selection::get_bounding_box().center()), not either object alone.
void GizmoStateWiringTests::testMultiObjectUnionAcrossSourceObjects()
{
  QList<PrepareSceneData::ModelBatch> batches;
  // Object 1: X[0,10] Y[0,10] Z[0,10] (two render batches, same object).
  batches.append(makeBatch(1, 0.f, 0.f, 0.f, 5.f, 10.f, 10.f));
  batches.append(makeBatch(1, 5.f, 0.f, 0.f, 10.f, 10.f, 10.f));
  // Object 4: X[20,30] Y[-10,0] Z[5,15].
  batches.append(makeBatch(4, 20.f, -10.f, 5.f, 30.f, 0.f, 15.f));
  // Object 2 stays unselected: X[-50,-40] must NOT shift the union.
  batches.append(makeBatch(2, -50.f, -50.f, -50.f, -40.f, -40.f, -40.f));

  const QList<int> selected = {1, 4};
  // Union of objects 1+4: X[0,30] Y[-10,10] Z[0,15] -> center (15, 0, 7.5).
  QVector3D center = GizmoCenter::fromSelectedIndices(selected, batches);
  QVERIFY2(approxVec(center, QVector3D(15.f, 0.f, 7.5f)),
           "multi-object selection must use the union AABB center of ALL selected objects");
}

// P15.11 (MULTICENTER): order of the index list must not matter, and a listed
// index with no batches (stale selection) contributes nothing to the union.
void GizmoStateWiringTests::testMultiObjectListOrderAndStaleEntries()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(1, 0.f, 0.f, 0.f, 10.f, 10.f, 10.f));
  batches.append(makeBatch(4, 20.f, -10.f, 5.f, 30.f, 0.f, 15.f));

  const QList<int> reversed = {4, 1};
  QVector3D center = GizmoCenter::fromSelectedIndices(reversed, batches);
  QVERIFY2(approxVec(center, QVector3D(15.f, 0.f, 7.5f)),
           "union center must be independent of the selection list order");

  const QList<int> withStale = {99, 1, 4};
  center = GizmoCenter::fromSelectedIndices(withStale, batches);
  QVERIFY2(approxVec(center, QVector3D(15.f, 0.f, 7.5f)),
           "stale indices without batches must not affect the union center");
}

void GizmoStateWiringTests::testMultiObjectEmptyListReturnsOrigin()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(0, 0, 0, 0, 10, 20, 30));
  QVector3D center = GizmoCenter::fromSelectedIndices(QList<int>(), batches);
  QVERIFY2(approxVec(center, QVector3D(0, 0, 0)),
           "empty multi-selection list must return origin (marker/highlight hidden)");
}

void GizmoStateWiringTests::testSingleObjectOverloadDelegatesToIndices()
{
  QList<PrepareSceneData::ModelBatch> batches;
  batches.append(makeBatch(1, 0.f, 0.f, 0.f, 10.f, 10.f, 10.f));
  batches.append(makeBatch(4, 20.f, -10.f, 5.f, 30.f, 0.f, 15.f));
  // The kept single-index overload must behave exactly like a one-entry list.
  QVector3D viaBatch = GizmoCenter::fromSelectedBatch(4, batches);
  QVector3D viaIndices = GizmoCenter::fromSelectedIndices(QList<int>{4}, batches);
  QVERIFY2(approxVec(viaBatch, viaIndices),
           "fromSelectedBatch must delegate to fromSelectedIndices");
  QVERIFY2(approxVec(viaBatch, QVector3D(25.f, -5.f, 10.f)),
           "single-object union center must be unchanged after the P15.11 refactor");
}

QTEST_MAIN(GizmoStateWiringTests)
#include "GizmoStateWiringTests.moc"

