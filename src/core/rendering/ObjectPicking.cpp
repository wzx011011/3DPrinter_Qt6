#include "ObjectPicking.h"

#include <cfloat>
#include <cmath>
#include <limits>

namespace
{
bool isFiniteVec(const QVector3D &v)
{
  return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
}

bool isFiniteBounds(const PrepareSceneData::ModelBounds &bounds)
{
  return std::isfinite(bounds.minX) && std::isfinite(bounds.minY) && std::isfinite(bounds.minZ)
      && std::isfinite(bounds.maxX) && std::isfinite(bounds.maxY) && std::isfinite(bounds.maxZ)
      && bounds.minX <= bounds.maxX && bounds.minY <= bounds.maxY && bounds.minZ <= bounds.maxZ;
}

bool rayAABB(const QVector3D &origin,
             const QVector3D &direction,
             const PrepareSceneData::ModelBounds &bounds,
             float &t)
{
  if (!isFiniteVec(origin) || !isFiniteVec(direction) || !isFiniteBounds(bounds))
    return false;

  float tmin = -FLT_MAX;
  float tmax = FLT_MAX;
  const float ov[3] = {origin.x(), origin.y(), origin.z()};
  const float dv[3] = {direction.x(), direction.y(), direction.z()};
  const float bmin[3] = {bounds.minX, bounds.minY, bounds.minZ};
  const float bmax[3] = {bounds.maxX, bounds.maxY, bounds.maxZ};

  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(dv[axis]) < 1e-8f) {
      if (ov[axis] < bmin[axis] || ov[axis] > bmax[axis])
        return false;
      continue;
    }

    float t1 = (bmin[axis] - ov[axis]) / dv[axis];
    float t2 = (bmax[axis] - ov[axis]) / dv[axis];
    if (t1 > t2)
      std::swap(t1, t2);
    tmin = std::max(tmin, t1);
    tmax = std::min(tmax, t2);
    if (tmin > tmax)
      return false;
  }

  if (tmax < 0.0f)
    return false;
  t = std::max(tmin, 0.0f);
  return true;
}

bool rayTriangleMoller(const QVector3D &origin,
                       const QVector3D &direction,
                       const PrepareSceneData::ModelVertex &a,
                       const PrepareSceneData::ModelVertex &b,
                       const PrepareSceneData::ModelVertex &c,
                       float &t)
{
  const QVector3D v0(a.x, a.y, a.z);
  const QVector3D v1(b.x, b.y, b.z);
  const QVector3D v2(c.x, c.y, c.z);
  if (!isFiniteVec(v0) || !isFiniteVec(v1) || !isFiniteVec(v2))
    return false;

  const QVector3D e0 = v1 - v0;
  const QVector3D e1 = v2 - v0;
  const QVector3D h = QVector3D::crossProduct(direction, e1);
  const float det = QVector3D::dotProduct(e0, h);
  if (det > -1e-6f && det < 1e-6f)
    return false;

  const float invDet = 1.0f / det;
  const QVector3D s = origin - v0;
  const float u = invDet * QVector3D::dotProduct(s, h);
  if (u < 0.0f || u > 1.0f)
    return false;

  const QVector3D q = QVector3D::crossProduct(s, e0);
  const float v = invDet * QVector3D::dotProduct(direction, q);
  if (v < 0.0f || u + v > 1.0f)
    return false;

  const float hitT = invDet * QVector3D::dotProduct(e1, q);
  if (hitT < 1e-4f || !std::isfinite(hitT))
    return false;

  t = hitT;
  return true;
}

bool validBatchSpan(const PrepareSceneData::ModelBatch &batch, int vertexCount)
{
  return batch.sourceObjectIndex >= 0
      && batch.volumeIndex >= 0
      && batch.instanceIndex >= 0
      && batch.firstVertex >= 0
      && batch.vertexCount >= 3
      && (batch.vertexCount % 3) == 0
      && batch.firstVertex <= vertexCount
      && batch.vertexCount <= vertexCount - batch.firstVertex;
}
}

ObjectPicking::Hit ObjectPicking::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  if (!isFiniteVec(rayOrigin) || !isFiniteVec(rayDirection) || rayDirection.lengthSquared() <= 1e-12f)
    return {};

  const QVector3D direction = rayDirection.normalized();
  float bestT = std::numeric_limits<float>::max();
  Hit bestHit;

  for (const PrepareSceneData::ModelBatch &batch : batches) {
    if (!validBatchSpan(batch, vertices.size()))
      continue;

    float tBox = 0.0f;
    if (!rayAABB(rayOrigin, direction, batch.bounds, tBox) || tBox >= bestT)
      continue;

    const int end = batch.firstVertex + batch.vertexCount;
    for (int i = batch.firstVertex; i + 2 < end; i += 3) {
      float tTriangle = 0.0f;
      if (!rayTriangleMoller(rayOrigin, direction,
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
