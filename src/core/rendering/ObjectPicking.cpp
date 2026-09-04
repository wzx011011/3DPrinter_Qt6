#include "ObjectPicking.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

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

bool rayAABBFloats(const QVector3D &origin,
                   const QVector3D &direction,
                   float bminX, float bminY, float bminZ,
                   float bmaxX, float bmaxY, float bmaxZ,
                   float &t)
{
  if (!isFiniteVec(origin) || !isFiniteVec(direction))
    return false;

  float tmin = -FLT_MAX;
  float tmax = FLT_MAX;
  const float ov[3] = {origin.x(), origin.y(), origin.z()};
  const float dv[3] = {direction.x(), direction.y(), direction.z()};
  const float bmin[3] = {bminX, bminY, bminZ};
  const float bmax[3] = {bmaxX, bmaxY, bmaxZ};

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

// BVH leaf threshold: median-split recursion stops once a subtree holds at
// most this many triangles (upstream AABBMesh uses an analogous leaf size).
constexpr int kBvhLeafTriangles = 8;
}

ObjectPicking::Bvh ObjectPicking::Bvh::build(
    const QList<PrepareSceneData::ModelVertex> &vertices,
    int firstVertex, int vertexCount)
{
  Bvh bvh;
  if (firstVertex < 0 || vertexCount < 3 || (vertexCount % 3) != 0
      || firstVertex + vertexCount > vertices.size())
    return bvh;

  const int triangleCount = vertexCount / 3;
  bvh.m_firstVertex = firstVertex;
  bvh.m_triangles.resize(triangleCount);
  for (int t = 0; t < triangleCount; ++t)
    bvh.m_triangles[t] = t;

  // Per-triangle centroid + bounds, filled while growing the root box.
  struct TriBox
  {
    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
  };
  std::vector<TriBox> triBoxes;
  triBoxes.resize(size_t(triangleCount));

  float rootMinX = FLT_MAX, rootMinY = FLT_MAX, rootMinZ = FLT_MAX;
  float rootMaxX = -FLT_MAX, rootMaxY = -FLT_MAX, rootMaxZ = -FLT_MAX;
  for (int t = 0; t < triangleCount; ++t) {
    const PrepareSceneData::ModelVertex &a = vertices.at(firstVertex + t * 3 + 0);
    const PrepareSceneData::ModelVertex &b = vertices.at(firstVertex + t * 3 + 1);
    const PrepareSceneData::ModelVertex &c = vertices.at(firstVertex + t * 3 + 2);
    TriBox &box = triBoxes[size_t(t)];
    box.minX = std::min(a.x, std::min(b.x, c.x));
    box.minY = std::min(a.y, std::min(b.y, c.y));
    box.minZ = std::min(a.z, std::min(b.z, c.z));
    box.maxX = std::max(a.x, std::max(b.x, c.x));
    box.maxY = std::max(a.y, std::max(b.y, c.y));
    box.maxZ = std::max(a.z, std::max(b.z, c.z));
    box.cx = (box.minX + box.maxX) * 0.5f;
    box.cy = (box.minY + box.maxY) * 0.5f;
    box.cz = (box.minZ + box.maxZ) * 0.5f;
    rootMinX = std::min(rootMinX, box.minX);
    rootMinY = std::min(rootMinY, box.minY);
    rootMinZ = std::min(rootMinZ, box.minZ);
    rootMaxX = std::max(rootMaxX, box.maxX);
    rootMaxY = std::max(rootMaxY, box.maxY);
    rootMaxZ = std::max(rootMaxZ, box.maxZ);
  }

  // Iterative median-split build (explicit stack; recursion depth would be
  // fine too but the stack form keeps the hot build cache-friendly).
  struct Task
  {
    qint32 nodeIndex;
    int triStart;
    int triCount;
  };
  bvh.m_nodes.reserve(size_t(triangleCount / (kBvhLeafTriangles / 2) + 8));

  Node root;
  root.minX = rootMinX; root.minY = rootMinY; root.minZ = rootMinZ;
  root.maxX = rootMaxX; root.maxY = rootMaxY; root.maxZ = rootMaxZ;
  bvh.m_nodes.append(root);
  std::vector<Task> stack;
  stack.push_back(Task{0, 0, triangleCount});

  while (!stack.empty()) {
    const Task task = stack.back();
    stack.pop_back();
    Node &node = bvh.m_nodes[int(task.nodeIndex)];

    if (task.triCount <= kBvhLeafTriangles) {
      node.left = -1;
      node.right = -1;
      node.triStart = task.triStart;
      node.triCount = task.triCount;
      continue;
    }

    // Longest axis of the node box.
    const float ex = node.maxX - node.minX;
    const float ey = node.maxY - node.minY;
    const float ez = node.maxZ - node.minZ;
    const int axis = (ex >= ey && ex >= ez) ? 0 : ((ey >= ez) ? 1 : 2);

    // nth_element by centroid on the chosen axis (median split).
    const int mid = task.triStart + task.triCount / 2;
    std::nth_element(
        bvh.m_triangles.begin() + task.triStart,
        bvh.m_triangles.begin() + mid,
        bvh.m_triangles.begin() + task.triStart + task.triCount,
        [axis, &triBoxes](qint32 lhs, qint32 rhs) {
          const TriBox &l = triBoxes[size_t(lhs)];
          const TriBox &r = triBoxes[size_t(rhs)];
          switch (axis) {
          case 0: return l.cx < r.cx;
          case 1: return l.cy < r.cy;
          default: return l.cz < r.cz;
          }
        });

    float lMinX = FLT_MAX, lMinY = FLT_MAX, lMinZ = FLT_MAX;
    float lMaxX = -FLT_MAX, lMaxY = -FLT_MAX, lMaxZ = -FLT_MAX;
    float rMinX = FLT_MAX, rMinY = FLT_MAX, rMinZ = FLT_MAX;
    float rMaxX = -FLT_MAX, rMaxY = -FLT_MAX, rMaxZ = -FLT_MAX;
    for (int i = task.triStart; i < mid; ++i) {
      const TriBox &box = triBoxes[size_t(bvh.m_triangles[i])];
      lMinX = std::min(lMinX, box.minX); lMaxX = std::max(lMaxX, box.maxX);
      lMinY = std::min(lMinY, box.minY); lMaxY = std::max(lMaxY, box.maxY);
      lMinZ = std::min(lMinZ, box.minZ); lMaxZ = std::max(lMaxZ, box.maxZ);
    }
    for (int i = mid; i < task.triStart + task.triCount; ++i) {
      const TriBox &box = triBoxes[size_t(bvh.m_triangles[i])];
      rMinX = std::min(rMinX, box.minX); rMaxX = std::max(rMaxX, box.maxX);
      rMinY = std::min(rMinY, box.minY); rMaxY = std::max(rMaxY, box.maxY);
      rMinZ = std::min(rMinZ, box.minZ); rMaxZ = std::max(rMaxZ, box.maxZ);
    }

    Node leftNode;
    leftNode.minX = lMinX; leftNode.minY = lMinY; leftNode.minZ = lMinZ;
    leftNode.maxX = lMaxX; leftNode.maxY = lMaxY; leftNode.maxZ = lMaxZ;
    Node rightNode;
    rightNode.minX = rMinX; rightNode.minY = rMinY; rightNode.minZ = rMinZ;
    rightNode.maxX = rMaxX; rightNode.maxY = rMaxY; rightNode.maxZ = rMaxZ;

    // NOTE: appending may reallocate, so index bookkeeping only (no Node&).
    const qint32 leftIndex = qint32(bvh.m_nodes.size());
    bvh.m_nodes.append(leftNode);
    const qint32 rightIndex = qint32(bvh.m_nodes.size());
    bvh.m_nodes.append(rightNode);
    bvh.m_nodes[int(task.nodeIndex)].left = leftIndex;
    bvh.m_nodes[int(task.nodeIndex)].right = rightIndex;

    stack.push_back(Task{leftIndex, task.triStart, mid - task.triStart});
    stack.push_back(Task{rightIndex, mid, task.triStart + task.triCount - mid});
  }

  return bvh;
}

bool ObjectPicking::Bvh::raycast(
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QVector3D &origin, const QVector3D &direction,
    float maxDistance, QVector3D *hitPosition, float *hitDistance) const
{
  if (isEmpty() || vertices.size() < m_firstVertex + m_triangles.size() * 3)
    return false;

  float best = maxDistance;
  bool anyHit = false;

  // Explicit near-child-first traversal stack; node AABB hits farther than
  // the current best are pruned wholesale.
  qint32 stack[64];
  int stackSize = 0;
  stack[stackSize++] = 0;

  while (stackSize > 0) {
    const qint32 nodeIndex = stack[--stackSize];
    const Node &node = m_nodes.at(int(nodeIndex));

    float tNode = 0.0f;
    if (!rayAABBFloats(origin, direction,
                       node.minX, node.minY, node.minZ,
                       node.maxX, node.maxY, node.maxZ, tNode)
        || tNode >= best)
      continue;

    if (node.left < 0) {
      const int triEnd = node.triStart + node.triCount;
      for (int i = node.triStart; i < triEnd; ++i) {
        const int v0 = m_firstVertex + int(m_triangles.at(i)) * 3;
        float tTriangle = 0.0f;
        if (!rayTriangleMoller(origin, direction,
                               vertices.at(v0),
                               vertices.at(v0 + 1),
                               vertices.at(v0 + 2),
                               tTriangle))
          continue;
        if (tTriangle < best) {
          best = tTriangle;
          anyHit = true;
        }
      }
      continue;
    }

    // Push the FAR child first so the NEAR child is processed first.
    float tLeft = 0.0f;
    float tRight = 0.0f;
    const Node &leftNode = m_nodes.at(int(node.left));
    const Node &rightNode = m_nodes.at(int(node.right));
    const bool leftHit = rayAABBFloats(origin, direction,
                                       leftNode.minX, leftNode.minY, leftNode.minZ,
                                       leftNode.maxX, leftNode.maxY, leftNode.maxZ,
                                       tLeft);
    const bool rightHit = rayAABBFloats(origin, direction,
                                        rightNode.minX, rightNode.minY, rightNode.minZ,
                                        rightNode.maxX, rightNode.maxY, rightNode.maxZ,
                                        tRight);
    if (leftHit && rightHit && tRight < tLeft) {
      if (stackSize < 64) stack[stackSize++] = node.left;
      if (stackSize < 64) stack[stackSize++] = node.right;
    } else {
      if (rightHit && stackSize < 64) stack[stackSize++] = node.right;
      if (leftHit && stackSize < 64) stack[stackSize++] = node.left;
    }
  }

  if (!anyHit)
    return false;
  if (hitDistance != nullptr)
    *hitDistance = best;
  if (hitPosition != nullptr)
    *hitPosition = origin + direction * best;
  return true;
}

ObjectPicking::Hit ObjectPicking::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches,
    const QVector<Bvh> &batchBvhs)
{
  if (!isFiniteVec(rayOrigin) || !isFiniteVec(rayDirection) || rayDirection.lengthSquared() <= 1e-12f)
    return {};

  const QVector3D direction = rayDirection.normalized();
  float bestT = std::numeric_limits<float>::max();
  Hit bestHit;

  for (int batchIndex = 0; batchIndex < batches.size(); ++batchIndex) {
    const PrepareSceneData::ModelBatch &batch = batches.at(batchIndex);
    if (!validBatchSpan(batch, vertices.size()))
      continue;

    float tBox = 0.0f;
    if (!rayAABB(rayOrigin, direction, batch.bounds, tBox) || tBox >= bestT)
      continue;

    const Bvh *bvh = (batchIndex < batchBvhs.size() && !batchBvhs.at(batchIndex).isEmpty())
                         ? &batchBvhs.at(batchIndex)
                         : nullptr;

    if (bvh != nullptr) {
      QVector3D hitPosition;
      float hitDistance = 0.0f;
      if (bvh->raycast(vertices, rayOrigin, direction, bestT,
                       &hitPosition, &hitDistance)
          && hitDistance < bestT) {
        bestT = hitDistance;
        bestHit.sourceObjectIndex = batch.sourceObjectIndex;
        bestHit.volumeIndex = batch.volumeIndex;
        bestHit.instanceIndex = batch.instanceIndex;
        bestHit.distance = hitDistance;
        bestHit.position = hitPosition;
      }
      continue;
    }

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

ObjectPicking::Hit ObjectPicking::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  return pick(rayOrigin, rayDirection, vertices, batches, QVector<Bvh>());
}

int ObjectPicking::pickSourceObject(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches)
{
  return pick(rayOrigin, rayDirection, vertices, batches).sourceObjectIndex;
}

int ObjectPicking::pickSourceObject(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches,
    const QVector<Bvh> &batchBvhs)
{
  return pick(rayOrigin, rayDirection, vertices, batches, batchBvhs).sourceObjectIndex;
}
