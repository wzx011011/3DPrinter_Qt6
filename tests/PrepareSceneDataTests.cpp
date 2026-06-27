#include <QtTest>

#include "qml_gui/Renderer/PrepareSceneData.h"

class PrepareSceneDataTests final : public QObject
{
  Q_OBJECT

private slots:
  void bedGeometryUsesDimensionsAndGridIntervals();
  void dirtyFlagsAreConsumedOnlyOnRequest();
  void activePlateContextDoesNotLeakInactiveObjects();
  void plateContextDirtyFlagsOnlyChangeOnPlateDifferences();
  void invalidBedDimensionsDoNotGenerateUnboundedBuffers();
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

QTEST_MAIN(PrepareSceneDataTests)
#include "PrepareSceneDataTests.moc"
