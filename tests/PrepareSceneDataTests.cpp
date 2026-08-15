#include <QtTest>

#include "qml_gui/Renderer/PrepareSceneData.h"

namespace
{
  void appendInt32(QByteArray &bytes, qint32 value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  void appendFloat(QByteArray &bytes, float value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  void appendTriangle(QByteArray &bytes,
                      int objectId,
                      const QList<float> &vertices)
  {
    appendInt32(bytes, objectId);
    appendInt32(bytes, 1);
    for (float value : vertices)
      appendFloat(bytes, value);
  }

  QByteArray packedMeshWithBatches(const QList<int> &objectIds,
                                   const QList<QList<float>> &triangles)
  {
    QByteArray bytes;
    appendInt32(bytes, objectIds.size());
    for (int i = 0; i < objectIds.size(); ++i)
      appendTriangle(bytes, objectIds.at(i), triangles.at(i));

    appendFloat(bytes, -10.0f);
    appendFloat(bytes, -20.0f);
    appendFloat(bytes, -30.0f);
    appendFloat(bytes, 40.0f);
    appendFloat(bytes, 50.0f);
    appendFloat(bytes, 60.0f);
    return bytes;
  }
}

class PrepareSceneDataTests final : public QObject
{
  Q_OBJECT

private slots:
  void bedGeometryUsesDimensionsAndGridIntervals();
  // v5.16: upstream PartPlateList multi-plate grid + select/unselect colors.
  void bedGridRendersEveryPlateWithSelectionColors();
  void dirtyFlagsAreConsumedOnlyOnRequest();
  void activePlateContextDoesNotLeakInactiveObjects();
  void plateContextDirtyFlagsOnlyChangeOnPlateDifferences();
  void invalidBedDimensionsDoNotGenerateUnboundedBuffers();
  void modelBatchesParsePackedMeshWithSourceIndices();
  void modelBatchesRejectMalformedPayloads();
  void modelBatchesRejectMisalignedIdentityMetadata();
  void currentPlateFootprintClassifiesRectangleAndCircle();
  void activePlateFilteringKeepsOnlyCurrentPlateSources();
  void emptyActivePlateDoesNotFallbackToAllModelBatches();
  void selectionAndHoverDoNotDirtyModelGeometry();
};

void PrepareSceneDataTests::bedGeometryUsesDimensionsAndGridIntervals()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);

  QVERIFY(scene.bedFillVertices().size() >= 6);
  QVERIFY(scene.bedLineVertices().size() > 0);
  QCOMPARE(scene.fineGridSpacingMm(), 10.0f);
  QCOMPARE(scene.coarseGridSpacingMm(), 50.0f);
  QVERIFY(scene.bedLineVertices().size() > scene.bedFillVertices().size());
}

void PrepareSceneDataTests::dirtyFlagsAreConsumedOnlyOnRequest()
{
  PrepareSceneData scene;
  const quint32 initialFlags = scene.peekDirtyFlags();
  QVERIFY((initialFlags & PrepareSceneData::DirtyBed) != 0);
  QVERIFY((initialFlags & PrepareSceneData::DirtyGpu) != 0);
  QCOMPARE(scene.peekDirtyFlags(), initialFlags);
  QCOMPARE(scene.takeDirtyFlags(), initialFlags);
  QCOMPARE(scene.peekDirtyFlags(), quint32(PrepareSceneData::DirtyNone));

  scene.setBed(200.0f, 210.0f, 5.0f, 6.0f, 0, 200.0f);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyBed) != 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyGpu) != 0);
  scene.takeDirtyFlags();
  scene.setBed(200.0f, 210.0f, 5.0f, 6.0f, 0, 200.0f);
  QCOMPARE(scene.peekDirtyFlags(), quint32(PrepareSceneData::DirtyNone));
}

void PrepareSceneDataTests::activePlateContextDoesNotLeakInactiveObjects()
{
  PrepareSceneData scene;
  scene.takeDirtyFlags();

  scene.setPlateContext(1, 3, QList<int>{2, 5});
  QCOMPARE(scene.currentPlateIndex(), 1);
  QCOMPARE(scene.plateCount(), 3);
  QCOMPARE(scene.activeObjectIndices(), (QList<int>{2, 5}));
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyPlate) != 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyGpu) != 0);

  scene.takeDirtyFlags();
  scene.setPlateContext(1, 3, QList<int>{2, 5});
  QCOMPARE(scene.peekDirtyFlags(), quint32(PrepareSceneData::DirtyNone));

  scene.setPlateContext(4, 3, QList<int>{0, 1, 2, 3});
  QCOMPARE(scene.currentPlateIndex(), -1);
  QCOMPARE(scene.plateCount(), 3);
  QVERIFY(scene.activeObjectIndices().isEmpty());
}

void PrepareSceneDataTests::plateContextDirtyFlagsOnlyChangeOnPlateDifferences()
{
  PrepareSceneData scene;
  scene.takeDirtyFlags();

  scene.setPlateContext(0, 2, QList<int>{1});
  quint32 dirtyFlags = scene.peekDirtyFlags();
  QVERIFY((dirtyFlags & PrepareSceneData::DirtyPlate) != 0);
  QVERIFY((dirtyFlags & PrepareSceneData::DirtyGpu) != 0);

  scene.takeDirtyFlags();
  scene.setPlateContext(0, 2, QList<int>{1});
  QCOMPARE(scene.peekDirtyFlags(), quint32(PrepareSceneData::DirtyNone));

  scene.setPlateContext(1, 2, QList<int>{3, 4});
  dirtyFlags = scene.peekDirtyFlags();
  QVERIFY((dirtyFlags & PrepareSceneData::DirtyPlate) != 0);
  QVERIFY((dirtyFlags & PrepareSceneData::DirtyGpu) != 0);
  QCOMPARE(scene.currentPlateIndex(), 1);
  QCOMPARE(scene.activeObjectIndices(), (QList<int>{3, 4}));
}

void PrepareSceneDataTests::invalidBedDimensionsDoNotGenerateUnboundedBuffers()
{
  PrepareSceneData scene;
  scene.setBed(1000000.0f, -5.0f, 0.0f, 0.0f, 0, 1000000.0f);

  QVERIFY(scene.bedFillVertices().size() <= 6);
  QVERIFY(scene.bedLineVertices().size() < 2000);
  QVERIFY(scene.bedWidth() <= 2000.0f);
  QVERIFY(scene.bedDepth() <= 2000.0f);
}

void PrepareSceneDataTests::modelBatchesParsePackedMeshWithSourceIndices()
{
  PrepareSceneData scene;
  scene.clearDirtyFlags();

  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{101, 202},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 20.0f, 0.0f},
          QList<float>{30.0f, 0.0f, 0.0f, 40.0f, 0.0f, 0.0f, 30.0f, 20.0f, 10.0f}});

  scene.setModelMeshData(mesh, QList<int>{4, 7}, QList<int>{4, 7});

  QCOMPARE(scene.modelVertices().size(), 6);
  QCOMPARE(scene.modelBatches().size(), 2);
  QCOMPARE(scene.modelBatches().at(0).renderObjectId, 101);
  QCOMPARE(scene.modelBatches().at(0).sourceObjectIndex, 4);
  QCOMPARE(scene.modelBatches().at(0).firstVertex, 0);
  QCOMPARE(scene.modelBatches().at(0).vertexCount, 3);
  QCOMPARE(scene.modelBatches().at(1).renderObjectId, 202);
  QCOMPARE(scene.modelBatches().at(1).sourceObjectIndex, 7);
  QCOMPARE(scene.modelBatches().at(1).firstVertex, 3);
  QCOMPARE(scene.modelBatches().at(1).vertexCount, 3);
  QVERIFY(scene.hasModelBounds());
  QCOMPARE(scene.modelBounds().minX, 0.0f);
  QCOMPARE(scene.modelBounds().minY, 0.0f);
  QCOMPARE(scene.modelBounds().minZ, 0.0f);
  QCOMPARE(scene.modelBounds().maxX, 40.0f);
  QCOMPARE(scene.modelBounds().maxY, 20.0f);
  QCOMPARE(scene.modelBounds().maxZ, 10.0f);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) != 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyGpu) != 0);
}

void PrepareSceneDataTests::modelBatchesRejectMalformedPayloads()
{
  PrepareSceneData scene;
  scene.clearDirtyFlags();

  QByteArray truncated;
  appendInt32(truncated, 1);
  appendInt32(truncated, 123);
  appendInt32(truncated, 2);
  appendFloat(truncated, 1.0f);

  scene.setModelMeshData(truncated, QList<int>{0}, QList<int>{0});
  QVERIFY(scene.modelVertices().isEmpty());
  QVERIFY(scene.modelBatches().isEmpty());
  QVERIFY(!scene.hasModelBounds());
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) != 0);

  scene.clearDirtyFlags();
  const QByteArray valid = packedMeshWithBatches(
      QList<int>{11},
      QList<QList<float>>{QList<float>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}});
  scene.setModelMeshData(valid, QList<int>{0, 1}, QList<int>{0});
  QVERIFY(scene.modelVertices().isEmpty());
  QVERIFY(scene.modelBatches().isEmpty());
  QVERIFY(!scene.hasModelBounds());
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) != 0);
}

void PrepareSceneDataTests::modelBatchesRejectMisalignedIdentityMetadata()
{
  PrepareSceneData scene;
  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{10},
      QList<QList<float>>{QList<float>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}});
  scene.setModelMeshData(mesh, QList<int>{1}, QList<int>{0, 1},
                         QList<int>{0}, QList<int>{1});
  QVERIFY(scene.modelBatches().isEmpty());
  QVERIFY(scene.modelVertices().isEmpty());
}

void PrepareSceneDataTests::currentPlateFootprintClassifiesRectangleAndCircle()
{
  PrepareSceneData scene;
  scene.setPlateContext(0, 1, QList<int>{});
  scene.setBed(200.0f, 100.0f, 10.0f, 20.0f, 0, 100.0f);
  QVERIFY(scene.containsCurrentPlatePoint(10.0f, 20.0f));
  QVERIFY(scene.containsCurrentPlatePoint(210.0f, 120.0f));
  QVERIFY(!scene.containsCurrentPlatePoint(211.0f, 120.0f));

  scene.setBed(100.0f, 100.0f, 0.0f, 0.0f, 1, 100.0f);
  QVERIFY(scene.containsCurrentPlatePoint(50.0f, 50.0f));
  QVERIFY(!scene.containsCurrentPlatePoint(0.0f, 0.0f));
  scene.setPlateContext(2, 1, QList<int>{});
  QVERIFY(!scene.containsCurrentPlatePoint(50.0f, 50.0f));
}

void PrepareSceneDataTests::activePlateFilteringKeepsOnlyCurrentPlateSources()
{
  PrepareSceneData scene;
  scene.clearDirtyFlags();

  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{10, 20, 30},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
          QList<float>{5.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 5.0f, 1.0f, 0.0f},
          QList<float>{9.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 9.0f, 1.0f, 0.0f}});

  scene.setModelMeshData(mesh, QList<int>{0, 1, 2}, QList<int>{1});

  QCOMPARE(scene.modelBatches().size(), 1);
  QCOMPARE(scene.modelVertices().size(), 3);
  QCOMPARE(scene.modelBatches().first().renderObjectId, 20);
  QCOMPARE(scene.modelBatches().first().sourceObjectIndex, 1);
  QVERIFY(scene.hasModelBounds());
  QCOMPARE(scene.modelBounds().minX, 5.0f);
  QCOMPARE(scene.modelBounds().maxX, 6.0f);
}

void PrepareSceneDataTests::emptyActivePlateDoesNotFallbackToAllModelBatches()
{
  PrepareSceneData scene;
  scene.clearDirtyFlags();

  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{10, 20},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
          QList<float>{5.0f, 0.0f, 0.0f, 6.0f, 0.0f, 0.0f, 5.0f, 1.0f, 0.0f}});

  scene.setModelMeshData(mesh, QList<int>{0, 1}, QList<int>{});

  QVERIFY(scene.modelVertices().isEmpty());
  QVERIFY(scene.modelBatches().isEmpty());
  QVERIFY(!scene.hasModelBounds());
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) != 0);
}

void PrepareSceneDataTests::selectionAndHoverDoNotDirtyModelGeometry()
{
  PrepareSceneData scene;
  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{10},
      QList<QList<float>>{QList<float>{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}});
  scene.setModelMeshData(mesh, QList<int>{5}, QList<int>{5});
  scene.clearDirtyFlags();

  scene.setSelectedSourceObjectIndex(5);
  QCOMPARE(scene.selectedSourceObjectIndex(), 5);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtySelection) != 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) == 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyGpu) == 0);

  scene.takeDirtyFlags();
  scene.setHoveredSourceObjectIndex(5);
  QCOMPARE(scene.hoveredSourceObjectIndex(), 5);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtySelection) != 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyMesh) == 0);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyGpu) == 0);

  scene.takeDirtyFlags();
  scene.setHoveredSourceObjectIndex(5);
  QCOMPARE(scene.peekDirtyFlags(), quint32(PrepareSceneData::DirtyNone));
}



// v5.16: upstream renders every plate in a ceil(sqrt(n))-column grid with
// stride = bed size * 1.2 (PartPlate.hpp:38, PartPlate.cpp:53). The
// selected plate uses SELECT_COLOR (0.2666/0.2784/0.2784), the others
// UNSELECT_DARK_COLOR (0.384/0.384/0.412) — PartPlate.cpp:77-79.
void PrepareSceneDataTests::bedGridRendersEveryPlateWithSelectionColors()
{
  QCOMPARE(PrepareSceneData::computePlateColumns(1), 1);
  QCOMPARE(PrepareSceneData::computePlateColumns(4), 2);
  QCOMPARE(PrepareSceneData::computePlateColumns(5), 3);
  QCOMPARE(PrepareSceneData::computePlateColumns(36), 6);

  PrepareSceneData scene;
  scene.setBed(200.0f, 200.0f, 0.0f, 0.0f, 0, 200.0f);
  scene.setShowBed(true);
  scene.setPlateContext(1, 2, QList<int>{0});

  // Two plates -> two 6-vertex fills.
  QCOMPARE(scene.bedFillVertices().size(), 12);

  bool hasSelected = false;
  bool hasUnselected = false;
  for (const auto &v : scene.bedFillVertices()) {
    if (v.r == 0.2666f && v.g == 0.2784f && v.b == 0.2784f)
      hasSelected = true;
    if (v.r == 0.384f && v.g == 0.384f && v.b == 0.412f)
      hasUnselected = true;
  }
  QVERIFY2(hasSelected, "selected plate fill must use upstream SELECT_COLOR");
  QVERIFY2(hasUnselected, "unselected plate fill must use upstream UNSELECT_DARK_COLOR");

  // Second plate sits one stride (bed width * 1.2) to the right.
  const float strideX = 200.0f * 1.2f;
  bool foundAtStride = false;
  for (const auto &v : scene.bedFillVertices()) {
    if (v.x >= strideX - 0.5f && v.x <= strideX + 200.5f)
      foundAtStride = true;
  }
  QVERIFY2(foundAtStride, "plate 2 must be laid out one stride right of plate 1");
}

QTEST_MAIN(PrepareSceneDataTests)
#include "PrepareSceneDataTests.moc"
