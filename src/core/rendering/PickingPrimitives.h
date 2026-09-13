#pragma once

#include <QVector3D>

#include "qml_gui/Renderer/PrepareSceneData.h"

#include <cfloat>
#include <cmath>

// Shared CPU picking math for ObjectPicking (brute-force sweep) and
// PickingRaycaster (BVH). One definition so both paths test rays and
// triangles with identical epsilon semantics -- tests and PerfBench assert
// nearest-hit parity between the two paths.
namespace PickingPrimitives
{
inline bool isFiniteVec(const QVector3D &v)
{
  return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
}

inline bool isFiniteBounds(const PrepareSceneData::ModelBounds &bounds)
{
  return std::isfinite(bounds.minX) && std::isfinite(bounds.minY) && std::isfinite(bounds.minZ)
      && std::isfinite(bounds.maxX) && std::isfinite(bounds.maxY) && std::isfinite(bounds.maxZ)
      && bounds.minX <= bounds.maxX && bounds.minY <= bounds.maxY && bounds.minZ <= bounds.maxZ;
}

inline bool rayAABB(const QVector3D &origin,
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

inline bool rayTriangleMoller(const QVector3D &origin,
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

inline bool validBatchSpan(const PrepareSceneData::ModelBatch &batch, int vertexCount)
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
