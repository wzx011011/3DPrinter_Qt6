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
  // P15.1/15.2 (COLOR): upstream volume coloring lock.
  void modelColorsFollowUpstreamVolumeSemantics();
  void modelBatchesRejectMalformedPayloads();
  void modelBatchesRejectMisalignedIdentityMetadata();
  void currentPlateFootprintClassifiesRectangleAndCircle();
  void activePlateFilteringKeepsOnlyCurrentPlateSources();
  void emptyActivePlateDoesNotFallbackToAllModelBatches();
  void selectionAndHoverDoNotDirtyModelGeometry();
  // v5.16 (EXCLAREA): upstream render_exclude_area fill per plate.
  void bedExcludeAreasFillPerPlateWithUpstreamColors();
  // v5.16 (HTLIMIT): upstream calc_height_limit rings + verticals.
  void heightLimitVerticesFollowUpstreamPalette();
  // v5.16 (BEDBOTTOM): below-horizon grid in LINE_BOTTOM_COLOR.
  void bedBottomLineGridUsesUpstreamBottomColor();
  // P15.8 (BOLDGRID): every-5th grid line quads (PartPlate.cpp:909-911).
  void bolderGridQuadsFollowEveryFifthLine();
  // P15.6 (CIRCLEBED): circular plate fan fill + circle-clipped grid.
  void roundBedBuildsFanFillWithClippedGrid();
  // P15.3 (OUTOFBED): print volume uniforms + error-background state
  // (gouraud.fs:154-171 data, GLCanvas3D.cpp:7069-7097).
  void printVolumeUniformAndErrorBackgroundState();
  // P15.5 (SINK): bed-plane contour bands of sinking batches
  // (3DScene.cpp:108-161).
  void sinkingContoursSliceSinkingBatchesAtBedPlane();
};

// v5.16 (EXCLAREA): one rectangle polygon must fan-fill on every plate with
// the upstream selected/unselected grays, offset by each plate's grid stride
// (upstream set_shape adds the plate position to every exclude point).
void PrepareSceneDataTests::bedExcludeAreasFillPerPlateWithUpstreamColors()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  scene.setPlateContext(0, 2, {});
  scene.takeDirtyFlags();
  const qsizetype baseFillCount = scene.bedFillVertices().size();

  // Flat point stream (upstream coPoints): one rectangle = 8 coords.
  QVariantList excludeFlat{10.0, 10.0, 50.0, 10.0, 50.0, 40.0, 10.0, 40.0};
  scene.setBedExcludeAreas(excludeFlat);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyBed) != 0);
  QCOMPARE(scene.bedExcludeAreas().size(), 8);

  // Fan triangulation of a 4-point rectangle = 2 triangles = 6 vertices per
  // plate; two plates -> +12.
  QCOMPARE(scene.bedFillVertices().size(), baseFillCount + 12);

  const float strideX = 220.0f * 1.2f;
  auto findVertex = [&scene](float x, float y, float r, float g, float b) {
    for (const auto &v : scene.bedFillVertices()) {
      if (qFuzzyCompare(v.x, x) && qFuzzyCompare(v.y, y)
          && qFuzzyCompare(v.r, r) && qFuzzyCompare(v.g, g)
          && qFuzzyCompare(v.b, b))
        return true;
    }
    return false;
  };
  // Plate 0 is selected: upstream render_exclude_area select gray.
  QVERIFY(findVertex(10.0f, 10.0f, 0.765f, 0.7686f, 0.7686f));
  // Plate 1 sits at stride X and uses the unselect gray.
  QVERIFY(findVertex(10.0f + strideX, 10.0f, 0.9f, 0.9f, 0.9f));

  // Malformed payloads are ignored: odd coordinate count and an incomplete
  // rectangle group (< 8 coords) never reach the fill buffer.
  scene.setBedExcludeAreas(QVariantList{1.0, 2.0, 3.0, 4.0, 5.0});
  QCOMPARE(scene.bedFillVertices().size(), baseFillCount);
}

// v5.16 (HTLIMIT): active limits draw ground->rod verticals + rod ring in the
// BOTTOM color and rod->lid verticals + lid ring in the TOP color per plate;
// inactive or zero-height requests stay empty.
void PrepareSceneDataTests::heightLimitVerticesFollowUpstreamPalette()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  scene.takeDirtyFlags();

  scene.setHeightLimit(true, 140.0f, 250.0f);
  QVERIFY((scene.peekDirtyFlags() & PrepareSceneData::DirtyBed) != 0);
  // Per plate: 4 corner verticals (ground->rod) + rod ring (4 edges) +
  // 4 corner verticals (rod->lid) + lid ring = 8+8+8+8 line vertices.
  QCOMPARE(scene.bedLimitVertices().size(), qsizetype(32));

  auto hasLineAtHeight = [&scene](float height, float r, float g, float b) {
    for (const auto &v : scene.bedLimitVertices()) {
      if (qFuzzyCompare(v.y, height) && qFuzzyCompare(v.r, r)
          && qFuzzyCompare(v.g, g) && qFuzzyCompare(v.b, b))
        return true;
    }
    return false;
  };
  QVERIFY(hasLineAtHeight(140.0f, 0.4f, 0.4f, 1.0f));
  QVERIFY(hasLineAtHeight(250.0f, 0.6f, 0.6f, 1.0f));

  // Inactive gate clears the geometry (upstream HEIGHT_LIMIT_NONE).
  scene.takeDirtyFlags();
  scene.setHeightLimit(false, 140.0f, 250.0f);
  QVERIFY(scene.bedLimitVertices().isEmpty());
}

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

  // P15.8 (BOLDGRID): the every-5th-line bolder quads share the fill buffer,
  // so the fill bound accounts for them (2000mm clamped bed -> ~200 fine
  // lines of which ~40 are bold quads).
  QVERIFY(scene.bedFillVertices().size() <= 1000);
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

// v5.16 (BEDBOTTOM): upstream PartPlate::render keeps ONLY render_grid alive
// when viewed from below, in LINE_BOTTOM_COLOR {0.8, 0.8, 0.8, 0.4}
// (PartPlate.cpp:85, render_grid(true)). The bottom line buffer must hold
// the same fine grid as the top buffer but exclude the border and origin
// axes (both live in render_background upstream, gated by `if (!bottom)`).
// P15.8 (BOLDGRID): the every-5th-line bolder set renders from below too --
// each bold line contributes two parallel 1px twin lines in the same bottom
// color (upstream render_grid draws m_gridlines_bolder in both passes).
void PrepareSceneDataTests::bedBottomLineGridUsesUpstreamBottomColor()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  scene.setPlateContext(0, 1, {});

  const auto &bottom = scene.bedBottomLineVertices();
  QVERIFY2(bottom.size() > 0, "below-horizon grid must exist");

  // 220mm bed, 10mm spacing: 21 vertical + 21 horizontal lines; 4 of each
  // are bold (distance 50/100/150/200 from the plate edge). Non-bold lines
  // emit one plain line (2 verts), bold ones two twin lines (4 verts).
  // x: 17*2 + 4*4 = 50, y: 21*2 + 4*4 = 58 -> 108 vertices.
  QCOMPARE(bottom.size(), qsizetype(108));

  for (const auto &v : bottom) {
    QVERIFY2(qFuzzyCompare(v.r, 0.8f) && qFuzzyCompare(v.g, 0.8f)
                 && qFuzzyCompare(v.b, 0.8f) && qFuzzyCompare(v.a, 0.4f),
             "every below-horizon grid vertex must use LINE_BOTTOM_COLOR");
  }

  // Border (0.95 alpha) never leaks into the bottom set; with the invented
  // green origin cross removed by P15.7 there is no axes color either.
  for (const auto &v : bottom) {
    QVERIFY2(!qFuzzyCompare(v.a, 0.95f), "plate border must stay top-view only");
  }
}


// P15.1/15.2 (COLOR): locks the upstream model coloring semantics --
// color_from_model_volume (3DScene.cpp:306-334) for the special volume
// types, update_colors_by_extruder (:1184-1235) filament colors for parts
// with the extruder index clamped to 0, UNPRINTABLE_COLOR, and
// adjust_color_for_rendering (black lift / fully-transparent fix).
void PrepareSceneDataTests::modelColorsFollowUpstreamVolumeSemantics()
{
  const float kTri[9] = {0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 10.0f, 0.0f};
  const QList<float> triangle{kTri, kTri + 9};
  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{1, 2, 3, 4, 5, 6, 7},
      QList<QList<float>>{triangle, triangle, triangle, triangle,
                          triangle, triangle, triangle});
  // Batches: 0=part extruder2, 1=part extruder out-of-range (clamp to 1),
  // 2=negative, 3=modifier, 4=blocker, 5=enforcer, 6=unprintable part.
  const QList<int> types{0, 0, 1, 2, 3, 4, 0};
  const QList<int> extruders{2, 99, 1, 1, 1, 1, 1};
  const QList<int> printable{1, 1, 1, 1, 1, 1, 0};
  // Filament colors: extruder1 = near-black (0.05), extruder2 = red.
  const QList<QVector4D> colors{
      QVector4D(0.05f, 0.05f, 0.05f, 1.0f),
      QVector4D(1.0f, 0.0f, 0.0f, 1.0f),
  };

  PrepareSceneData scene;
  scene.setModelMeshData(mesh, QList<int>{0, 1, 2, 3, 4, 5, 6},
                         QList<int>{0, 1, 2, 3, 4, 5, 6},
                         QList<int>{0, 1, 2, 3, 4, 5, 6},
                         QList<int>{0, 1, 2, 3, 4, 5, 6},
                         types, extruders, printable, colors);

  const auto &batches = scene.modelBatches();
  QCOMPARE(batches.size(), 7);
  // Part with extruder 2 -> filament color 2 (red), opaque.
  QCOMPARE(batches.at(0).translucent, false);
  QCOMPARE(batches.at(0).firstVertex, 0);
  QCOMPARE(scene.modelVertices().at(0).r, 1.0f);
  QCOMPARE(scene.modelVertices().at(0).g, 0.0f);
  QCOMPARE(scene.modelVertices().at(0).b, 0.0f);
  QCOMPARE(scene.modelVertices().at(0).a, 1.0f);
  // Out-of-range extruder clamps to color 0; near-black lifts to 0.2
  // (adjust_color_for_rendering).
  QCOMPARE(scene.modelVertices().at(3).r, 0.2f);
  QCOMPARE(scene.modelVertices().at(3).g, 0.2f);
  QCOMPARE(scene.modelVertices().at(3).b, 0.2f);
  // Negative volume: (0.3,0.3,0.3,0.4), translucent pass.
  QCOMPARE(batches.at(2).translucent, true);
  QCOMPARE(scene.modelVertices().at(6).r, 0.3f);
  QCOMPARE(scene.modelVertices().at(6).a, 0.4f);
  // Modifier: (1,1,0,0.6).
  QCOMPARE(batches.at(3).translucent, true);
  QCOMPARE(scene.modelVertices().at(9).g, 1.0f);
  QCOMPARE(scene.modelVertices().at(9).b, 0.0f);
  QCOMPARE(scene.modelVertices().at(9).a, 0.6f);
  // Blocker: (1,0.3,0.3,0.4). Enforcer: (0.3,0.3,1,0.4).
  QCOMPARE(scene.modelVertices().at(12).r, 1.0f);
  QCOMPARE(scene.modelVertices().at(12).a, 0.4f);
  QCOMPARE(scene.modelVertices().at(15).b, 1.0f);
  QCOMPARE(scene.modelVertices().at(15).a, 0.4f);
  // Unprintable part: (0,0,0,0.5) -> adjust lifts to 0.2 gray @0.5.
  QCOMPARE(batches.at(6).translucent, true);
  QCOMPARE(scene.modelVertices().at(18).r, 0.2f);
  QCOMPARE(scene.modelVertices().at(18).g, 0.2f);
  QCOMPARE(scene.modelVertices().at(18).a, 0.5f);

  // Without render channels (the picking scene form) the neutral fallback
  // keeps every batch opaque.
  PrepareSceneData neutral;
  neutral.setModelMeshData(mesh, QList<int>{0, 1, 2, 3, 4, 5, 6},
                           QList<int>{0, 1, 2, 3, 4, 5, 6},
                           QList<int>{0, 1, 2, 3, 4, 5, 6},
                           QList<int>{0, 1, 2, 3, 4, 5, 6});
  QCOMPARE(neutral.modelVertices().at(0).r, 0.8f);
  QCOMPARE(neutral.modelVertices().at(0).a, 1.0f);
  QCOMPARE(neutral.modelBatches().at(0).translucent, false);
}

// P15.8 (BOLDGRID): upstream calc_gridlines bolds every 5th grid line
// (PartPlate.cpp:483/496) and render_grid draws the bolder set a second time
// at 2px (PartPlate.cpp:909-911). The QRhi transport stores the bolder lines
// as flat quads in the plate line color; X bold lines leave the fine set
// (exclusive) while Y bold lines stay in it too (PartPlate.cpp:478-500).
void PrepareSceneDataTests::bolderGridQuadsFollowEveryFifthLine()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  scene.setPlateContext(0, 1, {});

  const auto &bold = scene.bedBoldLineVertices();
  // 4 bold x-lines (x = 50/100/150/200) + 4 bold y-lines, 6 quad verts each.
  QCOMPARE(bold.size(), qsizetype(8 * 6));

  // Every bold vertex uses the selected-plate line color and is opaque.
  for (const auto &v : bold) {
    QVERIFY2(qFuzzyCompare(v.r, 0.5294f) && qFuzzyCompare(v.g, 0.5451f)
                 && qFuzzyCompare(v.b, 0.5333f) && qFuzzyCompare(v.a, 1.0f),
             "bolder grid must use LINE_TOP_SEL_COLOR");
  }

  // Bold x quad sits around x=50 spanning the plate depth.
  bool foundBoldX50 = false;
  for (const auto &v : bold) {
    if (v.x > 49.0f && v.x < 51.0f && v.y >= -0.5f && v.y <= 220.5f)
      foundBoldX50 = true;
  }
  QVERIFY2(foundBoldX50, "bolder x quad expected at x = 50 (5th grid line)");

  // X bold lines are exclusive to the bolder set: no fine line at x = 50,
  // while the fine x = 10 line stays; Y bold lines remain in the fine set
  // too (upstream y-loop pushes every line into axes_lines).
  auto fineHasX = [&scene](float x) {
    for (const auto &v : scene.bedLineVertices()) {
      if (qFuzzyCompare(v.x, x))
        return true;
    }
    return false;
  };
  auto fineHasY = [&scene](float y) {
    for (const auto &v : scene.bedLineVertices()) {
      if (qFuzzyCompare(v.y, y))
        return true;
    }
    return false;
  };
  QVERIFY2(!fineHasX(50.0f), "x bold line must not duplicate in the fine set");
  QVERIFY2(fineHasX(10.0f), "x = 10 stays a fine line");
  QVERIFY2(fineHasY(50.0f), "y bold line stays in the fine set (inclusive y rule)");
}

// P15.6 (CIRCLEBED): circular plates fan-fill the boundary polygon and clip
// the straight grid lines to the circle (upstream init_model_from_poly +
// calc_gridlines intersection_pl, PartPlate.cpp:504-515).
void PrepareSceneDataTests::roundBedBuildsFanFillWithClippedGrid()
{
  PrepareSceneData scene;
  scene.setBed(200.0f, 200.0f, 0.0f, 0.0f, 1, 200.0f);
  scene.setPlateContext(0, 1, {});

  const auto &fill = scene.bedFillVertices();
  // 48-segment boundary polygon -> 48 fan triangles = 144 fill vertices.
  QCOMPARE(fill.size(), qsizetype(48 * 3));

  const float cx = 100.0f;
  const float cz = 100.0f;
  const float radius = 100.0f;
  bool hasCenterVertex = false;
  for (const auto &v : fill) {
    const float dx = v.x - cx;
    const float dz = v.y - cz;
    QVERIFY2(dx * dx + dz * dz <= radius * radius + 0.5f,
             "circular fill fan must stay inside the bed circle");
    if (qFuzzyCompare(v.x, cx) && qFuzzyCompare(v.y, cz))
      hasCenterVertex = true;
  }
  QVERIFY2(hasCenterVertex, "fan fill must originate at the circle center");

  // Selected-plate fill color on the fan.
  QVERIFY2(fill.at(0).r == 0.2666f && fill.at(0).g == 0.2784f,
           "circle fill must use the selected plate color");

  // Grid chords exist and stay inside the circle; the bolder set is present
  // (every 5th line) and the boundary chords close the contour.
  QVERIFY2(scene.bedLineVertices().size() > 0, "circle border/grid lines must exist");
  QVERIFY2(scene.bedBoldLineVertices().size() > 0, "circle bolder grid must exist");
  for (const auto &v : scene.bedLineVertices()) {
    const float dx = v.x - cx;
    const float dz = v.y - cz;
    QVERIFY2(dx * dx + dz * dz <= (radius + 1.0f) * (radius + 1.0f),
             "circle grid chords must be clipped to the bed circle");
  }

  // The print volume follows the circle branch (P15.3 shader data).
  QCOMPARE(scene.printVolume().type, 1);
  QVERIFY(qFuzzyCompare(scene.printVolume().xyData.x(), cx));
  QVERIFY(qFuzzyCompare(scene.printVolume().xyData.y(), cz));
  QVERIFY(qFuzzyCompare(scene.printVolume().xyData.z(), radius + 1e-4f));
}

// P15.3 (OUTOFBED): the print volume uniforms mirror GLCanvas3D.cpp:7177-7205
// (plate bounds inflated by BuildVolume::SceneEpsilon) and the error
// background state mirrors check_outside_state/_is_any_volume_outside
// (3DScene.cpp:1043-1171, GLCanvas3D.cpp:7069-7097).
void PrepareSceneDataTests::printVolumeUniformAndErrorBackgroundState()
{
  PrepareSceneData scene;
  scene.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  scene.setPlateContext(0, 1, {});

  const auto &pv = scene.printVolume();
  QCOMPARE(pv.type, 0);
  // Rectangle inflated by SceneEpsilon = 1e-4 (GLCanvas3D.cpp:7180).
  QVERIFY(qFuzzyCompare(pv.xyData.x(), -1e-4f));
  QVERIFY(qFuzzyCompare(pv.xyData.y(), -1e-4f));
  QVERIFY(qFuzzyCompare(pv.xyData.z(), 220.0f + 1e-4f));
  QVERIFY(qFuzzyCompare(pv.xyData.w(), 220.0f + 1e-4f));
  QCOMPARE(pv.zMin, 0.0f);
  // Printable height is not yet plumbed from the viewmodel: unbounded top.
  QVERIFY(pv.zMax > 1e30f);

  auto triangleAt = [](float x0, float z0, float x1, float z1) {
    return QList<float>{x0, 0.0f, z0, x1, 0.0f, z0, x0, 10.0f, z1};
  };

  // Fully inside object -> no error background.
  QByteArray inside = packedMeshWithBatches(
      QList<int>{1}, QList<QList<float>>{triangleAt(10.0f, 10.0f, 30.0f, 40.0f)});
  scene.setModelMeshData(inside, QList<int>{0}, QList<int>{0},
                         QList<int>{0}, QList<int>{0});
  QVERIFY2(!scene.anyVolumeOutside(), "inside object must not turn the background red");

  // Object crossing the +X plate edge -> error background.
  QByteArray outside = packedMeshWithBatches(
      QList<int>{1}, QList<QList<float>>{triangleAt(210.0f, 10.0f, 230.0f, 40.0f)});
  scene.setModelMeshData(outside, QList<int>{0}, QList<int>{0},
                         QList<int>{0}, QList<int>{0});
  QVERIFY2(scene.anyVolumeOutside(), "object over the plate edge must turn the background red");

  // Object fully below the bed plane -> Below state counts as outside
  // (volume_state_bbox: max z <= -SceneEpsilon, BuildVolume.cpp:308-320).
  QByteArray below;
  {
    appendInt32(below, 1);
    appendInt32(below, 1);
    appendInt32(below, 1);
    const QList<float> sunk{-10.0f, -5.0f, 10.0f, 10.0f, -5.0f, 10.0f,
                            -10.0f, -1.0f, 20.0f};
    for (float value : sunk)
      appendFloat(below, value);
    appendFloat(below, -10.0f);
    appendFloat(below, -20.0f);
    appendFloat(below, -30.0f);
    appendFloat(below, 40.0f);
    appendFloat(below, 50.0f);
    appendFloat(below, 60.0f);
  }
  scene.setModelMeshData(below, QList<int>{0}, QList<int>{0},
                         QList<int>{0}, QList<int>{0});
  QVERIFY2(scene.anyVolumeOutside(), "object fully below the bed must turn the background red");

  // Modifier batches are excluded from the outside check (upstream
  // shader_outside_printer_detection_enabled = is_model_part,
  // 3DScene.cpp:728): a negative volume over the edge keeps the plate calm.
  PrepareSceneData modifiers;
  modifiers.setBed(220.0f, 220.0f, 0.0f, 0.0f, 0, 220.0f);
  modifiers.setPlateContext(0, 1, {});
  modifiers.setModelMeshData(outside, QList<int>{0}, QList<int>{0},
                             QList<int>{0}, QList<int>{0},
                             QList<int>{1} /* MODEL_NEGTIVE */,
                             QList<int>{1}, QList<int>{1},
                             QList<QVector4D>{QVector4D(1.0f, 0.0f, 0.0f, 1.0f)});
  QVERIFY2(!modifiers.anyVolumeOutside(),
           "negative volumes must not participate in the outside check");
}

// P15.5 (SINK): sinking batches (bbox dips under the bed without being fully
// below it, 3DScene.cpp:614-620) get a white bed-plane contour band lifted by
// 0.015 (3DScene.cpp:150); batches above the bed get none.
void PrepareSceneDataTests::sinkingContoursSliceSinkingBatchesAtBedPlane()
{
  PrepareSceneData scene;
  // Batch 0: triangle spanning y = -10..+10 (sinking). Batch 1: fully above.
  const QList<float> sinkingTri{-10.0f, -10.0f, 0.0f, 10.0f, -10.0f, 0.0f,
                                0.0f, 10.0f, 0.0f};
  const QList<float> aboveTri{-10.0f, 5.0f, 0.0f, 10.0f, 5.0f, 0.0f,
                              0.0f, 15.0f, 0.0f};
  const QByteArray mesh = packedMeshWithBatches(
      QList<int>{1, 2}, QList<QList<float>>{sinkingTri, aboveTri});
  scene.setModelMeshData(mesh, QList<int>{0, 1}, QList<int>{0, 1},
                         QList<int>{0, 1}, QList<int>{0, 1});

  const auto &ranges = scene.sinkingContourRanges();
  QCOMPARE(ranges.size(), 1);
  QCOMPARE(ranges.at(0).sourceObjectIndex, 0);
  QCOMPARE(ranges.at(0).firstVertex, 0);

  const auto &verts = scene.sinkingContourVertices();
  QVERIFY2(verts.size() > 0, "sinking batch must produce contour band vertices");
  QCOMPARE(ranges.at(0).vertexCount, int(verts.size()));
  // The crossing triangle yields one segment band = 6 vertices.
  QCOMPARE(verts.size(), qsizetype(6));
  for (const auto &v : verts) {
    // White band lifted slightly above the bed plane (3DScene.cpp:140/:150).
    QVERIFY(qFuzzyCompare(v.y, 0.015f));
    QVERIFY(v.r == 1.0f && v.g == 1.0f && v.b == 1.0f && v.a == 1.0f);
    // Band stays around the slice segment between x = -5 and x = 5 and
    // within one half width (0.25mm) of the z = 0 contour line.
    QVERIFY(v.x >= -5.6f && v.x <= 5.6f);
    QVERIFY(v.z >= -0.26f && v.z <= 0.26f);
  }

  // A mesh change that lifts the object clears the bands.
  scene.setModelMeshData(packedMeshWithBatches(
                             QList<int>{1, 2},
                             QList<QList<float>>{aboveTri, aboveTri}),
                         QList<int>{0, 1}, QList<int>{0, 1},
                         QList<int>{0, 1}, QList<int>{0, 1});
  QVERIFY(scene.sinkingContourVertices().isEmpty());
  QVERIFY(scene.sinkingContourRanges().isEmpty());
}

QTEST_MAIN(PrepareSceneDataTests)
#include "PrepareSceneDataTests.moc"
