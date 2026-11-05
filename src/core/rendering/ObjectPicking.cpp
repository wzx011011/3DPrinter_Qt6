#include "ObjectPicking.h"

#include "PickingPrimitives.h"

#include <limits>

// Brute-force reference sweep (batch AABB prefilter -> per-triangle
// Moller-Trumbore). The raycasters in RhiViewport/SoftwareViewport go through
// PickingRaycaster (BVH, see PrepareSceneData::pickingRaycaster); this path
// stays as the shared nearest-hit reference that tests and PerfBench assert
// parity against.
//
// FIX-5M: `indices` carries the shared triangle-corner stream for indexed
// (blob v3) scenes; empty selects the legacy soup walk (consecutive vertex
// runs), keeping the two pick paths format-for-format in parity.

namespace {

template <typename TriangleAt>
ObjectPicking::Hit pickImpl(const QVector3D &rayOrigin,
                            const QVector3D &rayDirection,
                            const QList<PrepareSceneData::ModelVertex> &vertices,
                            const QList<PrepareSceneData::ModelBatch> &batches,
                            const TriangleAt &triangleAt)
{
  if (!PickingPrimitives::isFiniteVec(rayOrigin) || !PickingPrimitives::isFiniteVec(rayDirection)
      || rayDirection.lengthSquared() <= 1e-12f)
    return {};

  const QVector3D direction = rayDirection.normalized();
  float bestT = std::numeric_limits<float>::max();
  ObjectPicking::Hit bestHit;

  for (const PrepareSceneData::ModelBatch &batch : batches) {
    float tBox = 0.0f;
    if (!PickingPrimitives::rayAABB(rayOrigin, direction, batch.bounds, tBox) || tBox >= bestT)
      continue;

    triangleAt(batch, [&](const PrepareSceneData::ModelVertex &a,
                          const PrepareSceneData::ModelVertex &b,
                          const PrepareSceneData::ModelVertex &c) {
      float tTriangle = 0.0f;
      if (PickingPrimitives::rayTriangleMoller(rayOrigin, direction, a, b, c, tTriangle)
          && tTriangle < bestT) {
        bestT = tTriangle;
        bestHit.sourceObjectIndex = batch.sourceObjectIndex;
        bestHit.volumeIndex = batch.volumeIndex;
        bestHit.instanceIndex = batch.instanceIndex;
        bestHit.distance = tTriangle;
        bestHit.position = rayOrigin + direction * tTriangle;
      }
    });
  }

  return bestHit;
}

}  // namespace

ObjectPicking::Hit ObjectPicking::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<quint32> &indices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  const bool indexed = !indices.isEmpty();
  return pickImpl(rayOrigin, rayDirection, vertices, batches,
                  [indexed, &vertices, &indices](const PrepareSceneData::ModelBatch &batch,
                                                 auto &&emitTriangle) {
                    if (indexed
                        ? !PickingPrimitives::validIndexedBatchSpan(batch, indices.size())
                        : !PickingPrimitives::validBatchSpan(batch, vertices.size()))
                      return;
                    if (indexed) {
                      const int indexEnd = batch.firstIndex + batch.indexCount;
                      if (batch.firstIndex < 0 || indexEnd > indices.size())
                        return;
                      for (int t = batch.firstIndex; t + 2 < indexEnd; t += 3) {
                        const quint32 ia = indices.at(t);
                        const quint32 ib = indices.at(t + 1);
                        const quint32 ic = indices.at(t + 2);
                        if (int(ia) >= vertices.size() || int(ib) >= vertices.size()
                            || int(ic) >= vertices.size())
                          continue;
                        emitTriangle(vertices.at(int(ia)), vertices.at(int(ib)),
                                     vertices.at(int(ic)));
                      }
                      return;
                    }
                    if (!PickingPrimitives::validBatchSpan(batch, vertices.size()))
                      return;
                    const int end = batch.firstVertex + batch.vertexCount;
                    for (int i = batch.firstVertex; i + 2 < end; i += 3)
                      emitTriangle(vertices.at(i), vertices.at(i + 1), vertices.at(i + 2));
                  });
}

int ObjectPicking::pickSourceObject(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<quint32> &indices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  return pick(rayOrigin, rayDirection, vertices, indices, batches).sourceObjectIndex;
}
