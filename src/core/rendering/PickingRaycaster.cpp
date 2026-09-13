#include "PickingRaycaster.h"

#include "PickingPrimitives.h"

#include <QVector3D>

#include <algorithm>
#include <array>
#include <cfloat>
#include <limits>

namespace
{
constexpr int kMaxLeafSize = 8;
constexpr int kMaxDepth = 64;
constexpr int kBinCount = 8;

struct Bounds
{
  float minX = 0.0f;
  float minY = 0.0f;
  float minZ = 0.0f;
  float maxX = -1.0f;
  float maxY = -1.0f;
  float maxZ = -1.0f;

  bool isValid() const { return minX <= maxX && minY <= maxY && minZ <= maxZ; }

  void extend(float x, float y, float z)
  {
    if (!isValid()) {
      minX = maxX = x;
      minY = maxY = y;
      minZ = maxZ = z;
      return;
    }
    minX = std::min(minX, x);
    minY = std::min(minY, y);
    minZ = std::min(minZ, z);
    maxX = std::max(maxX, x);
    maxY = std::max(maxY, y);
    maxZ = std::max(maxZ, z);
  }

  void extend(const Bounds &other)
  {
    if (!other.isValid())
      return;
    extend(other.minX, other.minY, other.minZ);
    extend(other.maxX, other.maxY, other.maxZ);
  }

  // Surface area (half-perimeter form is enough for SAH comparisons).
  float area() const
  {
    if (!isValid())
      return 0.0f;
    const float dx = maxX - minX;
    const float dy = maxY - minY;
    const float dz = maxZ - minZ;
    return dx * dy + dy * dz + dx * dz;
  }
};

struct SahBin
{
  Bounds bounds;
  int count = 0;
};
}

// Transient per-build state; nodes and the permuted triangle order live in
// the raycaster, everything else is scratch.
struct PickingRaycaster::BuildScratch
{
  std::vector<Bounds> triangleBounds;
  std::vector<std::array<float, 3>> centroids;
  std::vector<int> scratchOrder;
};

int PickingRaycaster::buildRange(BuildScratch &scratch, int first, int count, int depth)
{
  Bounds nodeBounds;
  std::array<float, 3> centroidMin{{FLT_MAX, FLT_MAX, FLT_MAX}};
  std::array<float, 3> centroidMax{{-FLT_MAX, -FLT_MAX, -FLT_MAX}};
  for (int k = first; k < first + count; ++k) {
    const int ordinal = m_triangleOrder[k];
    nodeBounds.extend(scratch.triangleBounds[ordinal]);
    for (int axis = 0; axis < 3; ++axis) {
      centroidMin[axis] = std::min(centroidMin[axis], scratch.centroids[ordinal][axis]);
      centroidMax[axis] = std::max(centroidMax[axis], scratch.centroids[ordinal][axis]);
    }
  }

  const int nodeIndex = int(m_nodes.size());
  Node node;
  node.minX = nodeBounds.minX;
  node.minY = nodeBounds.minY;
  node.minZ = nodeBounds.minZ;
  node.maxX = nodeBounds.maxX;
  node.maxY = nodeBounds.maxY;
  node.maxZ = nodeBounds.maxZ;
  node.firstChildOrTriangle = first;
  node.secondChildOrZero = 0;
  node.triangleCount = count;
  m_nodes.push_back(node);

  if (count <= kMaxLeafSize || depth >= kMaxDepth)
    return nodeIndex;

  int bestAxis = -1;
  int bestSplit = -1;
  float bestCost = FLT_MAX;
  for (int axis = 0; axis < 3; ++axis) {
    const float extent = centroidMax[axis] - centroidMin[axis];
    if (!(extent > 0.0f))
      continue;
    const float scale = float(kBinCount) / extent;

    std::array<SahBin, kBinCount> bins;
    for (SahBin &bin : bins)
      bin = SahBin{};
    for (int k = first; k < first + count; ++k) {
      const int ordinal = m_triangleOrder[k];
      int binIndex = int((scratch.centroids[ordinal][axis] - centroidMin[axis]) * scale);
      binIndex = std::min(binIndex, kBinCount - 1);
      bins[binIndex].count += 1;
      bins[binIndex].bounds.extend(scratch.triangleBounds[ordinal]);
    }

    // Right-to-left cumulative bounds, then a left-to-right cost sweep.
    std::array<Bounds, kBinCount> rightBounds;
    std::array<int, kBinCount> rightCount;
    Bounds carry;
    int carryCount = 0;
    for (int k = kBinCount - 1; k >= 0; --k) {
      carry.extend(bins[k].bounds);
      carryCount += bins[k].count;
      rightBounds[k] = carry;
      rightCount[k] = carryCount;
    }

    Bounds leftBounds;
    int leftCountAcc = 0;
    for (int k = 0; k < kBinCount - 1; ++k) {
      if (bins[k].count == 0)
        continue;
      leftBounds.extend(bins[k].bounds);
      leftCountAcc += bins[k].count;
      if (rightCount[k + 1] == 0)
        continue;
      const float cost = leftBounds.area() * float(leftCountAcc)
          + rightBounds[k + 1].area() * float(rightCount[k + 1]);
      if (cost < bestCost) {
        bestCost = cost;
        bestAxis = axis;
        bestSplit = k;
      }
    }
  }

  int leftCount = 0;
  if (bestAxis >= 0 && bestCost < float(count)) {
    const float extent = centroidMax[bestAxis] - centroidMin[bestAxis];
    const float plane = centroidMin[bestAxis]
        + float(bestSplit + 1) * extent / float(kBinCount);
    const auto firstIt = m_triangleOrder.begin() + first;
    const auto lastIt = m_triangleOrder.begin() + first + count;
    const auto mid = std::stable_partition(firstIt, lastIt,
                                           [&](int ordinal) {
                                             return scratch.centroids[ordinal][bestAxis] < plane;
                                           });
    leftCount = int(mid - firstIt);
  }

  // Degenerate partition (rounding or tied centroids): median split keeps
  // the build terminating and the tree balanced.
  if (leftCount == 0 || leftCount == count) {
    const int axis = bestAxis >= 0 ? bestAxis : 0;
    const auto firstIt = m_triangleOrder.begin() + first;
    const auto lastIt = m_triangleOrder.begin() + first + count;
    const auto mid = firstIt + count / 2;
    std::nth_element(firstIt, mid, lastIt, [&](int lhs, int rhs) {
      return scratch.centroids[lhs][axis] < scratch.centroids[rhs][axis];
    });
    leftCount = count / 2;
  }

  const int leftChild = buildRange(scratch, first, leftCount, depth + 1);
  const int rightChild = buildRange(scratch, first + leftCount, count - leftCount, depth + 1);
  m_nodes[nodeIndex].firstChildOrTriangle = leftChild;
  m_nodes[nodeIndex].secondChildOrZero = rightChild;
  m_nodes[nodeIndex].triangleCount = 0;
  return nodeIndex;
}

void PickingRaycaster::build(const QList<PrepareSceneData::ModelVertex> &vertices,
                             const QList<PrepareSceneData::ModelBatch> &batches)
{
  m_nodes.clear();
  m_triangleOrder.clear();
  m_triangleBatch.clear();
  m_triangleFirstVertex.clear();
  m_triangleCount = 0;

  const int vertexCount = vertices.size();
  BuildScratch scratch;
  scratch.triangleBounds.reserve(size_t(batches.size()) * 4);
  scratch.centroids.reserve(size_t(batches.size()) * 4);

  // Same batch validity predicate as the brute-force sweep: triangles of
  // invalid batches must never produce a hit, so they never enter the tree.
  for (int batchIndex = 0; batchIndex < batches.size(); ++batchIndex) {
    const PrepareSceneData::ModelBatch &batch = batches.at(batchIndex);
    if (!PickingPrimitives::validBatchSpan(batch, vertexCount))
      continue;

    const int end = batch.firstVertex + batch.vertexCount;
    for (int i = batch.firstVertex; i + 2 < end; i += 3) {
      const PrepareSceneData::ModelVertex &a = vertices.at(i);
      const PrepareSceneData::ModelVertex &b = vertices.at(i + 1);
      const PrepareSceneData::ModelVertex &c = vertices.at(i + 2);
      if (!PickingPrimitives::isFiniteVec(QVector3D(a.x, a.y, a.z))
          || !PickingPrimitives::isFiniteVec(QVector3D(b.x, b.y, b.z))
          || !PickingPrimitives::isFiniteVec(QVector3D(c.x, c.y, c.z)))
        continue;

      Bounds bounds;
      bounds.extend(a.x, a.y, a.z);
      bounds.extend(b.x, b.y, b.z);
      bounds.extend(c.x, c.y, c.z);

      const int ordinal = int(scratch.centroids.size());
      scratch.centroids.push_back({(a.x + b.x + c.x) / 3.0f,
                                   (a.y + b.y + c.y) / 3.0f,
                                   (a.z + b.z + c.z) / 3.0f});
      scratch.triangleBounds.push_back(bounds);
      m_triangleBatch.push_back(batchIndex);
      m_triangleFirstVertex.push_back(i);
      m_triangleOrder.push_back(ordinal);
    }
  }

  m_triangleCount = int(scratch.centroids.size());
  if (m_triangleCount == 0)
    return;

  m_nodes.reserve(2 * size_t(m_triangleCount / kMaxLeafSize + 1));
  scratch.scratchOrder.reserve(size_t(m_triangleCount));
  buildRange(scratch, 0, m_triangleCount, 0);
}

ObjectPicking::Hit PickingRaycaster::pick(
    const QVector3D &rayOrigin,
    const QVector3D &rayDirection,
    const QList<PrepareSceneData::ModelVertex> &vertices,
    const QList<PrepareSceneData::ModelBatch> &batches) const
{
  // Input handling mirrors ObjectPicking::pick exactly.
  if (!isValid())
    return {};
  if (!PickingPrimitives::isFiniteVec(rayOrigin) || !PickingPrimitives::isFiniteVec(rayDirection)
      || rayDirection.lengthSquared() <= 1e-12f)
    return {};

  const QVector3D direction = rayDirection.normalized();
  float bestT = std::numeric_limits<float>::max();
  ObjectPicking::Hit bestHit;

  // Near-child-first ordered traversal; the kMaxDepth cap bounds the stack
  // occupancy to kMaxDepth + 1 entries.
  std::array<int, kMaxDepth + 2> stack;
  int stackTop = 0;
  stack[stackTop++] = 0;
  while (stackTop > 0) {
    const Node &node = m_nodes[stack[--stackTop]];

    float tEnter = 0.0f;
    if (!PickingPrimitives::rayAABB(rayOrigin, direction, node.bounds(), tEnter)
        || tEnter > bestT)
      continue;

    if (node.triangleCount > 0) {
      const int end = node.firstChildOrTriangle + node.triangleCount;
      for (int k = node.firstChildOrTriangle; k < end; ++k) {
        // Leaf slots index into the permuted order; the ordinal addresses the
        // per-triangle batch/vertex arrays.
        const int ordinal = m_triangleOrder[k];
        const int vertexIndex = m_triangleFirstVertex[ordinal];
        float tTriangle = 0.0f;
        if (!PickingPrimitives::rayTriangleMoller(rayOrigin, direction,
                                                  vertices.at(vertexIndex),
                                                  vertices.at(vertexIndex + 1),
                                                  vertices.at(vertexIndex + 2),
                                                  tTriangle))
          continue;
        if (tTriangle < bestT) {
          const PrepareSceneData::ModelBatch &batch =
              batches.at(m_triangleBatch[ordinal]);
          bestT = tTriangle;
          bestHit.sourceObjectIndex = batch.sourceObjectIndex;
          bestHit.volumeIndex = batch.volumeIndex;
          bestHit.instanceIndex = batch.instanceIndex;
          bestHit.distance = tTriangle;
          bestHit.position = rayOrigin + direction * tTriangle;
        }
      }
      continue;
    }

    const int left = node.firstChildOrTriangle;
    const int right = node.secondChildOrZero;
    float tLeft = 0.0f;
    float tRight = 0.0f;
    const bool hitLeft = PickingPrimitives::rayAABB(rayOrigin, direction,
                                                    m_nodes[left].bounds(), tLeft)
        && tLeft <= bestT;
    const bool hitRight = PickingPrimitives::rayAABB(rayOrigin, direction,
                                                     m_nodes[right].bounds(), tRight)
        && tRight <= bestT;
    if (!hitLeft && !hitRight)
      continue;
    // Push the farther child first so the nearer one is tested first.
    if (hitLeft && hitRight && tLeft > tRight) {
      stack[stackTop++] = left;
      stack[stackTop++] = right;
    } else if (hitLeft && hitRight) {
      stack[stackTop++] = right;
      stack[stackTop++] = left;
    } else {
      stack[stackTop++] = hitLeft ? left : right;
    }
  }

  return bestHit;
}
