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
#include "qml_gui/Renderer/PrepareSceneData.h"

#include <algorithm>

namespace
{
  void appendInt32(QByteArray &bytes, qint32 value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  void appendUint32(QByteArray &bytes, quint32 value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  void appendFloat(QByteArray &bytes, float value)
  {
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }

  // Wave 2b (perf): TLV v2 packet — "OMD2", geometry/batch counts, one
  // indexed geometry per batch (one triangle each, identity matrix),
  // identity + matrix + render-channel fields inlined in the batch section,
  // 24-byte bbox trailer (docs/perf-wave2b-design.md).
  QByteArray packedMeshWithTriangles(const QList<int> &sourceObjectIndices,
                                     const QList<QList<float>> &triangles,
                                     const QList<int> &volumeIndices = {},
                                     const QList<int> &instanceIndices = {})
  {
    const int count = sourceObjectIndices.size();
    QByteArray bytes;
    bytes.append(QStringLiteral("OMD2").toUtf8());
    appendInt32(bytes, count); // geometryCount
    appendInt32(bytes, count); // batchCount
    for (int i = 0; i < count; ++i)
    {
      appendInt32(bytes, i); // geomId == geometry index
      appendInt32(bytes, 3); // vertexCount
      appendInt32(bytes, 3); // indexCount
      for (float value : triangles.at(i))
        appendFloat(bytes, value);
      appendUint32(bytes, 0);
      appendUint32(bytes, 1);
      appendUint32(bytes, 2);
      for (int k = 0; k < 9; ++k)
        appendFloat(bytes, 0.0f); // normals (unused by the soup path)
    }
    for (int i = 0; i < count; ++i)
    {
      appendInt32(bytes, 100 + i); // renderObjectId
      appendInt32(bytes, sourceObjectIndices.at(i));
      appendInt32(bytes, volumeIndices.isEmpty() ? 0 : volumeIndices.at(i));
      appendInt32(bytes, instanceIndices.isEmpty() ? 0 : instanceIndices.at(i));
      appendInt32(bytes, i); // geomId
      const float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
      for (float value : identity)
        appendFloat(bytes, value);
      appendInt32(bytes, 0); // volumeType: model part
      appendInt32(bytes, 1); // extruderId
      appendInt32(bytes, 1); // printable
    }
    appendFloat(bytes, -100.0f);
    appendFloat(bytes, -100.0f);
    appendFloat(bytes, -100.0f);
    appendFloat(bytes, 100.0f);
    appendFloat(bytes, 100.0f);
    appendFloat(bytes, 100.0f);
    return bytes;
  }

  PrepareSceneData sceneFromTriangles(const QList<int> &sourceObjectIndices,
                                      const QList<QList<float>> &triangles)
  {
    PrepareSceneData scene;
    scene.clearDirtyFlags();
    scene.setPlateContext(0, 1, sourceObjectIndices);
    scene.setModelMeshData(packedMeshWithTriangles(sourceObjectIndices, triangles),
                           sourceObjectIndices,
                           sourceObjectIndices);
    return scene;
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
  // Wave 2 (perf): the per-batch AABB BVH acceleration must be
  // indistinguishable from the brute-force sweep (same nearest hit).
  void bvhMatchesBruteForceOnRandomTriangles();
};

void ObjectPickingTests::aabbHitTriangleMissDoesNotPickObject()
{
  const PrepareSceneData scene = sceneFromTriangles(
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
  const PrepareSceneData scene = sceneFromTriangles(
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
      QList<int>{4, 7},
      QList<QList<float>>{
          QList<float>{0.0f, 0.0f, 2.0f,
                       1.0f, 0.0f, 2.0f,
                       0.0f, 1.0f, 2.0f},
          QList<float>{0.0f, 0.0f, 5.0f,
                       1.0f, 0.0f, 5.0f,
                       0.0f, 1.0f, 5.0f}},
      QList<int>{2, 6},
      QList<int>{1, 3});
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

  const PrepareSceneData scene = sceneFromTriangles(
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

void ObjectPickingTests::bvhMatchesBruteForceOnRandomTriangles()
{
  // Deterministic pseudo-random triangle cloud in [-50,50]^3.
  quint32 rng = 20260903u;
  const auto nextRandom = [&rng]() {
    rng = rng * 1664525u + 1013904223u;
    return float(double(rng >> 8) / double(1u << 24)) - 0.5f; // [-0.5, 0.5)
  };

  const int kTriangles = 600;
  QList<QList<float>> triangles;
  triangles.reserve(kTriangles);
  for (int t = 0; t < kTriangles; ++t) {
    QList<float> tri;
    for (int v = 0; v < 3; ++v)
      for (int c = 0; c < 3; ++c)
        tri.append(nextRandom() * 100.0f);
    triangles.append(tri);
  }

  const PrepareSceneData scene = sceneFromTriangles(
      QList<int>{11, 11, 12},
      triangles);

  QVector<ObjectPicking::Bvh> bvhs;
  bvhs.resize(scene.modelBatches().size());
  for (int i = 0; i < scene.modelBatches().size(); ++i)
    bvhs[i] = ObjectPicking::Bvh::build(
        scene.modelVertices(), scene.modelBatches().at(i).firstVertex,
        scene.modelBatches().at(i).vertexCount);

  int bothHit = 0;
  const int kRays = 400;
  for (int r = 0; r < kRays; ++r) {
    const QVector3D origin(nextRandom() * 200.0f, nextRandom() * 200.0f,
                           nextRandom() * 200.0f);
    QVector3D direction(nextRandom(), nextRandom(), nextRandom());
    if (direction.lengthSquared() < 1e-4f)
      continue;
    direction.normalize();

    const ObjectPicking::Hit brute = ObjectPicking::pick(
        origin, direction, scene.modelVertices(), scene.modelBatches());
    const ObjectPicking::Hit accel = ObjectPicking::pick(
        origin, direction, scene.modelVertices(), scene.modelBatches(), bvhs);

    QCOMPARE(accel.sourceObjectIndex, brute.sourceObjectIndex);
    QCOMPARE(accel.volumeIndex, brute.volumeIndex);
    QCOMPARE(accel.instanceIndex, brute.instanceIndex);
    if (brute.isValid()) {
      ++bothHit;
      QCOMPARE(accel.isValid(), true);
      QVERIFY(qAbs(accel.distance - brute.distance) < 1e-3f);
      QVERIFY((accel.position - brute.position).length() < 1e-2f);
    } else {
      QCOMPARE(accel.isValid(), false);
    }
  }
  QVERIFY2(bothHit > 0,
           "random-ray fan must produce at least one hit to exercise the comparison");
}

QTEST_MAIN(ObjectPickingTests)
#include "ObjectPickingTests.moc"
