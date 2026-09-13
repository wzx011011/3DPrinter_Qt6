#include "ObjectPicking.h"

#include "PickingPrimitives.h"

#include <limits>

// Brute-force reference sweep (batch AABB prefilter -> per-triangle
// Moller-Trumbore). The raycasters in RhiViewport/SoftwareViewport go through
// PickingRaycaster (BVH, see PrepareSceneData::pickingRaycaster); this path
// stays as the shared nearest-hit reference that tests and PerfBench assert
// parity against.

ObjectPicking::Hit ObjectPicking::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  if (!PickingPrimitives::isFiniteVec(rayOrigin) || !PickingPrimitives::isFiniteVec(rayDirection)
      || rayDirection.lengthSquared() <= 1e-12f)
    return {};

  const QVector3D direction = rayDirection.normalized();
  float bestT = std::numeric_limits<float>::max();
  Hit bestHit;

  for (const PrepareSceneData::ModelBatch &batch : batches) {
    if (!PickingPrimitives::validBatchSpan(batch, vertices.size()))
      continue;

    float tBox = 0.0f;
    if (!PickingPrimitives::rayAABB(rayOrigin, direction, batch.bounds, tBox) || tBox >= bestT)
      continue;

    const int end = batch.firstVertex + batch.vertexCount;
    for (int i = batch.firstVertex; i + 2 < end; i += 3) {
      float tTriangle = 0.0f;
      if (!PickingPrimitives::rayTriangleMoller(rayOrigin, direction,
                                                vertices.at(i),
                                                vertices.at(i + 1),
                                                vertices.at(i + 2),
                                                tTriangle))
        continue;

      if (tTriangle < bestT) {
        bestT = tTriangle;
        bestHit.sourceObjectIndex = batch.sourceObjectIndex;
        bestHit.volumeIndex = batch.volumeIndex;
        bestHit.instanceIndex = batch.instanceIndex;
        bestHit.distance = tTriangle;
        bestHit.position = rayOrigin + direction * tTriangle;
      }
    }
  }

  return bestHit;
}

int ObjectPicking::pickSourceObject(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  return pick(rayOrigin, rayDirection, vertices, batches).sourceObjectIndex;
}
