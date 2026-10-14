// ObjectPickingTests - pure CPU tests for precise RHI object picking.
//
// Phase 72 (GPICK-01). These tests pin the source-truth picking behavior used
// by the GL path: screen ray -> ray-AABB prefilter -> Moller-Trumbore
// ray-triangle intersection -> nearest source-object hit.
//
// AUTOMOC caveat (single-file QtTest with cpp-internal Q_OBJECT, see
// tests/GizmoMathTests.cpp:1-12): after editing private slots here, re-run
// cmake configure (the canonical verify script does this automatically).

#include <QtTest>
#include <QByteArray>
#include <QList>
#include <QMatrix4x4>
#include <QSize>
#include <QVector3D>

#include "core/rendering/GizmoMath.h"
#include "core/rendering/ObjectPicking.h"
#include "core/rendering/PickingRaycaster.h"
#include "qml_gui/Renderer/PrepareSceneData.h"

#include <algorithm>

#include <array>
#include <cmath>

namespace
{
  void appendInt32(QByteArray &bytes, qint32 value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  // Projects a world point to widget pixel coordinates through the same
  // proj*view chain GizmoMath::computeRay inverts, so tests can aim the pick
  // cursor at an exact world-space body point (X3 grabber-picking tests).
  QPointF worldToScreen(const QVector3D &world, const QMatrix4x4 &proj,
                        const QMatrix4x4 &view, const QSize &viewSize)
  {
    const QVector4D clip = proj * view * QVector4D(world, 1.0f);
    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();
    return QPointF(float((ndcX + 1.0) * 0.5 * viewSize.width()),
                   float((1.0 - ndcY) * 0.5 * viewSize.height()));
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

  QByteArray packedMeshWithTriangles(const QList<int> &objectIds,
                                     const QList<QList<float>> &triangles)
  {
    QByteArray bytes;
    appendInt32(bytes, objectIds.size());
    for (int i = 0; i < objectIds.size(); ++i)
      appendTriangle(bytes, objectIds.at(i), triangles.at(i));

    appendFloat(bytes, -100.0f);
    appendFloat(bytes, -100.0f);
    appendFloat(bytes, -100.0f);
    appendFloat(bytes, 100.0f);
    appendFloat(bytes, 100.0f);
    appendFloat(bytes, 100.0f);
    return bytes;
  }

  // PrepareSceneData is intentionally non-copyable (it carries the full
  // expanded vertex lists), so scene-building helpers fill a caller-owned
  // scene instead of returning one.
  void fillTriangleScene(PrepareSceneData &scene,
                         const QList<int> &sourceObjectIndices,
                         const QList<QList<float>> &triangles)
  {
    QList<int> objectIds;
    objectIds.reserve(sourceObjectIndices.size());
    for (int i = 0; i < sourceObjectIndices.size(); ++i)
      objectIds.append(100 + i);

    scene.clearDirtyFlags();
    scene.setPlateContext(0, 1, sourceObjectIndices);
    scene.setModelMeshData(packedMeshWithTriangles(objectIds, triangles),
                           sourceObjectIndices,
                           sourceObjectIndices);
  }

  PrepareSceneData::ModelBounds boundsFor(const QList<PrepareSceneData::ModelVertex> &vertices,
                                          int first,
                                          int count)
  {
    PrepareSceneData::ModelBounds bounds;
    if (first < 0 || count <= 0 || first + count > vertices.size())
      return bounds;

    const auto &v0 = vertices.at(first);
    bounds = {v0.x, v0.y, v0.z, v0.x, v0.y, v0.z};
    for (int i = first + 1; i < first + count; ++i) {
      const auto &v = vertices.at(i);
      bounds.minX = std::min(bounds.minX, v.x);
      bounds.minY = std::min(bounds.minY, v.y);
      bounds.minZ = std::min(bounds.minZ, v.z);
      bounds.maxX = std::max(bounds.maxX, v.x);
      bounds.maxY = std::max(bounds.maxY, v.y);
      bounds.maxZ = std::max(bounds.maxZ, v.z);
    }
    return bounds;
  }

  // PICK-BVH: full nearest-hit parity between the BVH raycaster and the
  // brute-force sweep for one ray.
  void assertRaycasterParity(const PrepareSceneData &scene,
                             const QVector3D &origin,
                             const QVector3D &direction)
  {
    PickingRaycaster raycaster;
    raycaster.build(scene.modelVertices(), scene.modelBatches());
    const ObjectPicking::Hit brute = ObjectPicking::pick(
        origin, direction, scene.modelVertices(), scene.modelBatches());
    const ObjectPicking::Hit fast = raycaster.pick(
        origin, direction, scene.modelVertices(), scene.modelBatches());

    QCOMPARE(fast.isValid(), brute.isValid());
    QCOMPARE(fast.sourceObjectIndex, brute.sourceObjectIndex);
    QCOMPARE(fast.volumeIndex, brute.volumeIndex);
    QCOMPARE(fast.instanceIndex, brute.instanceIndex);
    if (!brute.isValid())
      return;
    const float tolerance = 1e-4f * std::max(1.0f, brute.distance);
    QVERIFY(std::abs(fast.distance - brute.distance) <= tolerance);
    QVERIFY((fast.position - brute.position).length() <= 1e-3f);
  }

  // Deterministic LCG scene: `batchCount` overlapping boxes of ~`trisPerBox`
  // big triangles each, scene bbox roughly [0,10] x [0,10] x [0,4.5].
  void fillRandomBoxesScene(PrepareSceneData &scene, int batchCount,
                            int trisPerBox)
  {
    quint32 seed = 0x12345678u;
    const auto nextFloat = [&seed](float lo, float hi) {
      seed = seed * 1664525u + 1013904223u;
      return lo + float((seed >> 8) & 0xFFFFu) / 65535.0f * (hi - lo);
    };

    QByteArray mesh;
    QList<int> objectIds;
    appendInt32(mesh, batchCount);
    for (int b = 0; b < batchCount; ++b) {
      objectIds.append(900 + b);
      const float ox = nextFloat(0.0f, 6.0f);
      const float oy = nextFloat(0.0f, 6.0f);
      const float oz = nextFloat(0.0f, 3.0f);
      appendInt32(mesh, 900 + b);
      appendInt32(mesh, trisPerBox);
      for (int t = 0; t < trisPerBox; ++t) {
        for (int v = 0; v < 3; ++v) {
          appendFloat(mesh, ox + nextFloat(0.0f, 4.0f));
          appendFloat(mesh, oy + nextFloat(0.0f, 4.0f));
          appendFloat(mesh, oz + nextFloat(0.0f, 1.5f));
        }
      }
    }
    for (float value : {-20.0f, -20.0f, -20.0f, 40.0f, 40.0f, 40.0f})
      appendFloat(mesh, value);

    scene.clearDirtyFlags();
    scene.setPlateContext(0, 1, QList<int>{});
    scene.setModelMeshData(mesh, objectIds, objectIds);
  }

  struct LookDownZ
  {
    QSize viewSize{800, 600};
    float eyeZ = 5.0f;
    float aspect = 800.0f / 600.0f;

    QMatrix4x4 projMatrix() const
    {
      QMatrix4x4 p;
      p.perspective(45.0f, aspect, 0.1f, 100.0f);
      return p;
    }

    QMatrix4x4 viewMatrix() const
    {
      QMatrix4x4 v;
      v.translate(0.0f, 0.0f, -eyeZ);
      return v;
    }

    float sxCenter() const { return float(viewSize.width()) * 0.5f; }
    float syCenter() const { return float(viewSize.height()) * 0.5f; }
  };
}

class ObjectPickingTests final : public QObject
{
  Q_OBJECT

private slots:
  void aabbHitTriangleMissDoesNotPickObject();
  void nearestTriangleHitWinsAcrossBatches();
  void nearestTriangleHitCarriesVolumeAndInstanceIdentity();
  void invalidAndDegenerateBatchesAreIgnored();
  void screenRayUsesGizmoMathAndSceneVertices();
  void raycasterMatchesBruteForceOnCraftedScenes();
  void raycasterMatchesBruteForceOnRandomScene();
  void raycasterNeverHitsInvalidBatchTriangles();
  void sceneRaycasterRebuildsAfterMeshChange();
  // X3: gizmo grabber bodies are picked exactly (ray vs drawn triangles,
  // closest hit wins, no world-unit proximity threshold).
  void gizmoMovePickHitsDrawnConeBody();
  void gizmoMovePickMissesRadialGapThatOldThresholdAccepted();
  void gizmoScalePickHitsBoxHandleAndMissesRadialGap();
  void gizmoRotatePickHitsRingTubeAndMissesBeyondTube();
};

void ObjectPickingTests::aabbHitTriangleMissDoesNotPickObject()
{
  PrepareSceneData scene;
  fillTriangleScene(scene,
      QList<int>{7},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 0.0f,
                       10.0f, 0.0f, 0.0f,
                       0.0f, 10.0f, 0.0f}});

  const int hit = ObjectPicking::pickSourceObject(
      QVector3D(9.0f, 9.0f, 5.0f),
      QVector3D(0.0f, 0.0f, -1.0f),
      scene.modelVertices(),
      scene.modelBatches());

  QCOMPARE(hit, -1);
}

void ObjectPickingTests::nearestTriangleHitWinsAcrossBatches()
{
  PrepareSceneData scene;
  fillTriangleScene(scene,
      QList<int>{4, 7},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 2.0f,
                       1.0f, 0.0f, 2.0f,
                       0.0f, 1.0f, 2.0f},
          QList<float>{0.0f, 0.0f, 5.0f,
                       1.0f, 0.0f, 5.0f,
                       0.0f, 1.0f, 5.0f}});

  const int hit = ObjectPicking::pickSourceObject(
      QVector3D(0.25f, 0.25f, 10.0f),
      QVector3D(0.0f, 0.0f, -1.0f),
      scene.modelVertices(),
      scene.modelBatches());

  QCOMPARE(hit, 7);
}

void ObjectPickingTests::nearestTriangleHitCarriesVolumeAndInstanceIdentity()
{
  const QByteArray mesh = packedMeshWithTriangles(
      QList<int>{100, 200},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 2.0f,
                       1.0f, 0.0f, 2.0f,
                       0.0f, 1.0f, 2.0f},
          QList<float>{0.0f, 0.0f, 5.0f,
                       1.0f, 0.0f, 5.0f,
                       0.0f, 1.0f, 5.0f}});
  PrepareSceneData scene;
  scene.setPlateContext(0, 1, QList<int>{4, 7});
  scene.setModelMeshData(mesh, QList<int>{4, 7}, QList<int>{2, 6},
                         QList<int>{1, 3}, QList<int>{4, 7});

  const ObjectPicking::Hit hit = ObjectPicking::pick(
      QVector3D(0.25f, 0.25f, 10.0f), QVector3D(0.0f, 0.0f, -1.0f),
      scene.modelVertices(), scene.modelBatches());
  QVERIFY(hit.isValid());
  QCOMPARE(hit.sourceObjectIndex, 7);
  QCOMPARE(hit.volumeIndex, 6);
  QCOMPARE(hit.instanceIndex, 3);
}

void ObjectPickingTests::invalidAndDegenerateBatchesAreIgnored()
{
  QList<PrepareSceneData::ModelVertex> vertices;
  vertices << PrepareSceneData::ModelVertex{0.0f, 0.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{1.0f, 0.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{0.0f, 1.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{0.0f, 0.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{1.0f, 0.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

  QList<PrepareSceneData::ModelBatch> batches;
  batches << PrepareSceneData::ModelBatch{101, -1, -1, -1, 0, 3, boundsFor(vertices, 0, 3)}
          << PrepareSceneData::ModelBatch{102, 5, 0, 0, 3, 2, boundsFor(vertices, 3, 2)}
          << PrepareSceneData::ModelBatch{103, 6, 0, 0, 5, 3, boundsFor(vertices, 5, 3)};

  const int hit = ObjectPicking::pickSourceObject(
      QVector3D(0.25f, 0.25f, 10.0f),
      QVector3D(0.0f, 0.0f, -1.0f),
      vertices,
      batches);

  QCOMPARE(hit, -1);
}

void ObjectPickingTests::screenRayUsesGizmoMathAndSceneVertices()
{
  const LookDownZ camera;
  const auto [origin, direction] = GizmoMath::computeRay(
      camera.sxCenter(),
      camera.syCenter(),
      camera.viewSize,
      camera.projMatrix(),
      camera.viewMatrix());

  PrepareSceneData scene;
  fillTriangleScene(scene,
      QList<int>{42},
      QList<QList<float>>{
          QList<float>{-1.0f, -1.0f, 0.0f,
                       1.0f, -1.0f, 0.0f,
                       0.0f, 1.0f, 0.0f}});

  const int hit = ObjectPicking::pickSourceObject(
      origin,
      direction,
      scene.modelVertices(),
      scene.modelBatches());

  QCOMPARE(hit, 42);
}

void ObjectPickingTests::raycasterMatchesBruteForceOnCraftedScenes()
{
  // Two overlapping batches, nearest-hit across batch order.
  PrepareSceneData twoBatch;
  fillTriangleScene(twoBatch,
      QList<int>{4, 7},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 2.0f},
          QList<float>{0.0f, 0.0f, 5.0f, 1.0f, 0.0f, 5.0f, 0.0f, 1.0f, 5.0f}});

  // One batch whose triangle spans a wide area (exercises leaf splitting).
  PrepareSceneData wide;
  fillTriangleScene(wide,
      QList<int>{42},
      QList<QList<float>>{
          QList<float>{-30.0f, -30.0f, 0.0f, 30.0f, -30.0f, 0.0f,
                       30.0f, 30.0f, 0.0f}});

  for (float x = -2.0f; x <= 2.0f; x += 0.5f) {
    for (float y = -2.0f; y <= 2.0f; y += 0.5f) {
      assertRaycasterParity(twoBatch,
                            QVector3D(x, y, 10.0f),
                            QVector3D(0.0f, 0.0f, -1.0f));
      assertRaycasterParity(wide,
                            QVector3D(x, y, 10.0f),
                            QVector3D(0.0f, 0.0f, -1.0f));
    }
  }
  // Oblique rays against the wide triangle.
  assertRaycasterParity(wide,
                        QVector3D(-40.0f, 5.0f, 20.0f),
                        QVector3D(1.0f, -0.1f, -1.0f));
  assertRaycasterParity(wide,
                        QVector3D(40.0f, -5.0f, 20.0f),
                        QVector3D(-1.0f, 0.1f, -1.0f));
}

void ObjectPickingTests::raycasterMatchesBruteForceOnRandomScene()
{
  PrepareSceneData scene;
  fillRandomBoxesScene(scene, 5, 60);

  PickingRaycaster raycaster;
  raycaster.build(scene.modelVertices(), scene.modelBatches());
  QVERIFY(raycaster.isValid());
  QCOMPARE(raycaster.triangleCount(), 5 * 60);
  QVERIFY(raycaster.nodeCount() > 0);

  quint32 raySeed = 0xDEADBEEFu;
  for (int i = 0; i < 24; ++i) {
    raySeed = raySeed * 1664525u + 1013904223u;
    const float ox = float((raySeed >> 8) & 0xFFu) / 255.0f * 24.0f - 12.0f;
    raySeed = raySeed * 1664525u + 1013904223u;
    const float oy = float((raySeed >> 8) & 0xFFu) / 255.0f * 24.0f - 12.0f;
    raySeed = raySeed * 1664525u + 1013904223u;
    const float oz = float((raySeed >> 8) & 0xFFu) / 255.0f * 12.0f + 8.0f;
    assertRaycasterParity(scene,
                          QVector3D(ox, oy, oz),
                          QVector3D(0.05f, 0.0f, -1.0f));
  }
}

void ObjectPickingTests::raycasterNeverHitsInvalidBatchTriangles()
{
  // Same situation as invalidAndDegenerateBatchesAreIgnored: the batch
  // carries a geometrically hittable triangle but an invalid identity, so it
  // must never enter the BVH.
  QList<PrepareSceneData::ModelVertex> vertices;
  vertices << PrepareSceneData::ModelVertex{0.0f, 0.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{1.0f, 0.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f}
           << PrepareSceneData::ModelVertex{0.0f, 1.0f, 3.0f, 1.0f, 1.0f, 1.0f, 1.0f};

  QList<PrepareSceneData::ModelBatch> batches;
  batches << PrepareSceneData::ModelBatch{101, -1, -1, -1, 0, 3,
                                          boundsFor(vertices, 0, 3)};

  PickingRaycaster raycaster;
  raycaster.build(vertices, batches);
  QVERIFY(!raycaster.isValid());

  const ObjectPicking::Hit hit = raycaster.pick(
      QVector3D(0.25f, 0.25f, 10.0f), QVector3D(0.0f, 0.0f, -1.0f),
      vertices, batches);
  QVERIFY(!hit.isValid());
}

void ObjectPickingTests::sceneRaycasterRebuildsAfterMeshChange()
{
  // Empty scene: valid empty raycaster, no hit, no crash.
  PrepareSceneData scene;
  PickingRaycaster *raycaster = scene.pickingRaycaster();
  QVERIFY(raycaster != nullptr);
  QVERIFY(!raycaster->isValid());
  QVERIFY(!scene
               .pickingRaycaster()
               ->pick(QVector3D(0.0f, 0.0f, 5.0f),
                      QVector3D(0.0f, 0.0f, -1.0f),
                      scene.modelVertices(), scene.modelBatches())
               .isValid());

  // First mesh: one triangle of object 11.
  const QList<int> firstObjects{11};
  const QList<QList<float>> firstTris{
      QList<float>{0.0f, 0.0f, 2.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 2.0f}};
  scene.clearDirtyFlags();
  scene.setPlateContext(0, 1, firstObjects);
  scene.setModelMeshData(packedMeshWithTriangles(QList<int>{100}, firstTris),
                         firstObjects, firstObjects);
  QCOMPARE(scene.pickingRaycaster()->triangleCount(), 1);
  QCOMPARE(scene.pickingRaycaster()
               ->pick(QVector3D(0.25f, 0.25f, 10.0f),
                      QVector3D(0.0f, 0.0f, -1.0f),
                      scene.modelVertices(), scene.modelBatches())
               .sourceObjectIndex,
           11);

  // Replacement mesh (same scene object): the cached tree must reflect the
  // new geometry, not the first mesh.
  const QList<int> secondObjects{13};
  const QList<QList<float>> secondTris{
      QList<float>{5.0f, 5.0f, 1.0f, 6.0f, 5.0f, 1.0f, 5.0f, 6.0f, 1.0f}};
  scene.clearDirtyFlags();
  scene.setPlateContext(0, 1, secondObjects);
  scene.setModelMeshData(packedMeshWithTriangles(QList<int>{200}, secondTris),
                         secondObjects, secondObjects);
  const ObjectPicking::Hit moved = scene.pickingRaycaster()->pick(
      QVector3D(0.25f, 0.25f, 10.0f), QVector3D(0.0f, 0.0f, -1.0f),
      scene.modelVertices(), scene.modelBatches());
  QVERIFY(!moved.isValid());
  QCOMPARE(scene.pickingRaycaster()->triangleCount(), 1);
  QCOMPARE(scene.pickingRaycaster()
               ->pick(QVector3D(5.5f, 5.5f, 10.0f),
                      QVector3D(0.0f, 0.0f, -1.0f),
                      scene.modelVertices(), scene.modelBatches())
               .sourceObjectIndex,
           13);
}

// ===========================================================================
// X3: gizmo grabber precise picking (exact body hit, no proximity threshold)
//
// Camera rig: eye (0,0,40) looking at the origin, so gizmoWorldScale =
// max(40*0.15, 5) = 6.0 and every drawn body is comfortably inside the
// 45-degree frustum (half-extent at z=0 is tan(22.5)*40 = 16.57). Body
// geometry in world units at scale 6.0 (gizmo-local * 6.0):
//   move cone   axial [4.68, 6.0], base radius 0.33 -> apex 0
//   scale box   center (4.68,0,0), half extents 0.24
//   rings       center-circle radius 4.2, tube radius 0.21
// Old heuristic thresholds for comparison: move 6*0.08 = 0.48, rotate/scale
// 6*0.10 = 0.60 world units - the negative cases below sit inside those old
// thresholds but outside the drawn bodies, pinning that the threshold
// heuristic is gone.
// ===========================================================================

void ObjectPickingTests::gizmoMovePickHitsDrawnConeBody()
{
  LookDownZ cam;
  cam.eyeZ = 40.0f;
  const QMatrix4x4 proj = cam.projMatrix();
  const QMatrix4x4 view = cam.viewMatrix();
  const QVector3D gizmoCenter{0, 0, 0};

  // Halfway up the X cone, 0.05 from the axis: strictly inside the body
  // (cone radius there is ~0.165).
  const QPointF cursor = worldToScreen(QVector3D(5.34f, 0.05f, 0.0f),
                                       proj, view, cam.viewSize);
  const int ax = GizmoMath::pickMoveAxis(float(cursor.x()), float(cursor.y()),
                                         cam.viewSize, proj, view,
                                         gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                         /*hasSelection=*/true);
  QCOMPARE(ax, 1);
}

void ObjectPickingTests::gizmoMovePickMissesRadialGapThatOldThresholdAccepted()
{
  LookDownZ cam;
  cam.eyeZ = 40.0f;
  const QMatrix4x4 proj = cam.projMatrix();
  const QMatrix4x4 view = cam.viewMatrix();
  const QVector3D gizmoCenter{0, 0, 0};

  // Same axial position but 0.3 off the X axis: outside the drawn cone
  // (~0.165 there) yet inside the old 0.48 world-unit threshold, which used
  // to report a hit. The ray must now miss everything -> 0.
  const QPointF cursor = worldToScreen(QVector3D(5.34f, 0.30f, 0.0f),
                                       proj, view, cam.viewSize);
  const int ax = GizmoMath::pickMoveAxis(float(cursor.x()), float(cursor.y()),
                                         cam.viewSize, proj, view,
                                         gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                         /*hasSelection=*/true);
  QCOMPARE(ax, 0);
}

void ObjectPickingTests::gizmoScalePickHitsBoxHandleAndMissesRadialGap()
{
  LookDownZ cam;
  cam.eyeZ = 40.0f;
  const QMatrix4x4 proj = cam.projMatrix();
  const QMatrix4x4 view = cam.viewMatrix();
  const QVector3D gizmoCenter{0, 0, 0};

  // Inside the X box handle (|y|,|z| <= 0.24): hit -> axis 1.
  QPointF cursor = worldToScreen(QVector3D(4.68f, 0.10f, 0.10f),
                                 proj, view, cam.viewSize);
  int ax = GizmoMath::pickScaleAxis(float(cursor.x()), float(cursor.y()),
                                    cam.viewSize, proj, view,
                                    gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                    /*hasSelection=*/true);
  QCOMPARE(ax, 1);

  // 0.4 off the axis at the box's axial position: outside the 0.24 half
  // extent but inside the old 0.60 threshold -> must miss now.
  cursor = worldToScreen(QVector3D(4.68f, 0.40f, 0.0f), proj, view,
                         cam.viewSize);
  ax = GizmoMath::pickScaleAxis(float(cursor.x()), float(cursor.y()),
                                cam.viewSize, proj, view,
                                gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                /*hasSelection=*/true);
  QCOMPARE(ax, 0);
}

void ObjectPickingTests::gizmoRotatePickHitsRingTubeAndMissesBeyondTube()
{
  LookDownZ cam;
  cam.eyeZ = 40.0f;
  const QMatrix4x4 proj = cam.projMatrix();
  const QMatrix4x4 view = cam.viewMatrix();
  const QVector3D gizmoCenter{0, 0, 0};

  // On the Z ring's center circle at 45 degrees: inside the tube (radius
  // 0.21) and clear of the X/Y rings (1.23 from their circles) -> axis 3.
  QPointF cursor = worldToScreen(QVector3D(2.97f, 2.97f, 0.0f),
                                 proj, view, cam.viewSize);
  int ax = GizmoMath::pickRotateAxis(float(cursor.x()), float(cursor.y()),
                                     cam.viewSize, proj, view,
                                     gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                     /*hasSelection=*/true);
  QCOMPARE(ax, 3);

  // 0.35 beyond the center circle (radially outward at 45 degrees): outside
  // the 0.21 tube but inside the old 0.60 threshold -> must miss now.
  cursor = worldToScreen(QVector3D(3.2244f, 3.2244f, 0.0f), proj, view,
                         cam.viewSize);
  ax = GizmoMath::pickRotateAxis(float(cursor.x()), float(cursor.y()),
                                 cam.viewSize, proj, view,
                                 gizmoCenter, QVector3D(0.0f, 0.0f, cam.eyeZ),
                                 /*hasSelection=*/true);
  QCOMPARE(ax, 0);
}

QTEST_MAIN(ObjectPickingTests)
#include "ObjectPickingTests.moc"
