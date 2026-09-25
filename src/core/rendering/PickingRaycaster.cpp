#include "PickingRaycaster.h"

#include "PickingPrimitives.h"

#include <QVector3D>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <limits>

namespace
{
constexpr int kMaxLeafSize = 8;
constexpr int kMaxDepth = 64;
constexpr int kBinCount = 8;

// PERF (pick_ray): finite-check-free twins of the PickingPrimitives math for
// the raycaster's own traversal. Preconditions are established by callers:
// pick() validates the ray once at entry, and build() admits only triangles
// with all-finite vertices, so every node bound and every stored vertex is
// finite by construction. The arithmetic is statement-for-statement the same
// as PickingPrimitives::rayAABB / rayTriangleMoller, so results are
// bit-identical on those inputs (parity with ObjectPicking::pick preserved).
bool rayAABBFast(const QVector3D &origin,
                 const QVector3D &direction,
                 float bminX, float bminY, float bminZ,
                 float bmaxX, float bmaxY, float bmaxZ,
                 float &t)
{
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

bool rayTriangleFast(const QVector3D &origin,
                     const QVector3D &direction,
                     const PrepareSceneData::ModelVertex &a,
                     const PrepareSceneData::ModelVertex &b,
                     const PrepareSceneData::ModelVertex &c,
                     float &t)
{
  const float e0x = b.x - a.x;
  const float e0y = b.y - a.y;
  const float e0z = b.z - a.z;
  const float e1x = c.x - a.x;
  const float e1y = c.y - a.y;
  const float e1z = c.z - a.z;

  const float dx = direction.x();
  const float dy = direction.y();
  const float dz = direction.z();
  // h = cross(direction, e1) -- component order matches QVector3D::crossProduct.
  const float hx = dy * e1z - dz * e1y;
  const float hy = dz * e1x - dx * e1z;
  const float hz = dx * e1y - dy * e1x;

  const float det = e0x * hx + e0y * hy + e0z * hz;
  if (det > -1e-6f && det < 1e-6f)
    return false;

  const float invDet = 1.0f / det;
  const float sx = origin.x() - a.x;
  const float sy = origin.y() - a.y;
  const float sz = origin.z() - a.z;

  const float u = invDet * (sx * hx + sy * hy + sz * hz);
  if (u < 0.0f || u > 1.0f)
    return false;

  // q = cross(s, e0).
  const float qx = sy * e0z - sz * e0y;
  const float qy = sz * e0x - sx * e0z;
  const float qz = sx * e0y - sy * e0x;

  const float v = invDet * (dx * qx + dy * qy + dz * qz);
  if (v < 0.0f || u + v > 1.0f)
    return false;

  const float hitT = invDet * (e1x * qx + e1y * qy + e1z * qz);
  if (hitT < 1e-4f || !std::isfinite(hitT))
    return false;

  t = hitT;
  return true;
}

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
  m_leafTriangleBatch.clear();
  m_leafTriangleFirstVertex.clear();
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

  // PERF (pick_ray): flatten the double indirection the leaf tests used to
  // pay (leaf slot -> m_triangleOrder ordinal -> per-ordinal arrays). After
  // the build partitions, the ordinal for leaf slot k is scattered, so the
  // ordinal-indexed arrays were touched in near-random order on every ray.
  // One O(n) permutation gives the leaf loop two dense, slot-ordered arrays.
  m_leafTriangleBatch.resize(size_t(m_triangleCount));
  m_leafTriangleFirstVertex.resize(size_t(m_triangleCount));
  for (int k = 0; k < m_triangleCount; ++k) {
    const int ordinal = m_triangleOrder[k];
    m_leafTriangleBatch[size_t(k)] = m_triangleBatch[size_t(ordinal)];
    m_leafTriangleFirstVertex[size_t(k)] = m_triangleFirstVertex[size_t(ordinal)];
  }
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

  // PERF (pick_ray): every node's AABB is tested exactly once. The previous
  // traversal re-tested each child at pop time even though it had already
  // been tested (and bestT-culled) when pushed, doubling the AABB work per
  // internal node. The pop-time re-check only ever skipped subtrees whose
  // entry distance already exceeded the then-current bestT -- they hold no
  // closer triangle -- so dropping it changes the visited set, never the
  // nearest hit. Bounds are finite by construction (build() admits only
  // finite triangles) and the ray was validated above, so the unchecked
  // fast twin is exact here.
  const Node &root = m_nodes.front();
  float tRoot = 0.0f;
  if (!rayAABBFast(rayOrigin, direction, root.minX, root.minY, root.minZ,
                   root.maxX, root.maxY, root.maxZ, tRoot))
    return {};

  // Near-child-first ordered traversal; the kMaxDepth cap bounds the stack
  // occupancy to kMaxDepth + 1 entries.
  std::array<int, kMaxDepth + 2> stack;
  int stackTop = 0;
  stack[stackTop++] = 0;

  // Hoisted base pointers: the lists are fixed for the duration of the pick
  // (same lists the tree was built from) and the leaf loop is the hot path.
  const PrepareSceneData::ModelVertex *const vertexBase = vertices.constData();
  const PrepareSceneData::ModelBatch *const batchBase = batches.constData();

  while (stackTop > 0) {
    const Node &node = m_nodes[stack[--stackTop]];

    if (node.triangleCount > 0) {
      const int end = node.firstChildOrTriangle + node.triangleCount;
      for (int k = node.firstChildOrTriangle; k < end; ++k) {
        // Leaf slots address the pre-permuted slot-ordered arrays directly.
        const int vertexIndex = m_leafTriangleFirstVertex[size_t(k)];
        float tTriangle = 0.0f;
        if (!rayTriangleFast(rayOrigin, direction,
                             vertexBase[vertexIndex],
                             vertexBase[vertexIndex + 1],
                             vertexBase[vertexIndex + 2],
                             tTriangle))
          continue;
        if (tTriangle < bestT) {
          const PrepareSceneData::ModelBatch &batch =
              batchBase[m_leafTriangleBatch[size_t(k)]];
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
    const Node &leftNode = m_nodes[left];
    const Node &rightNode = m_nodes[right];
    float tLeft = 0.0f;
    float tRight = 0.0f;
    const bool hitLeft = rayAABBFast(rayOrigin, direction,
                                     leftNode.minX, leftNode.minY, leftNode.minZ,
                                     leftNode.maxX, leftNode.maxY, leftNode.maxZ,
                                     tLeft)
        && tLeft <= bestT;
    const bool hitRight = rayAABBFast(rayOrigin, direction,
                                      rightNode.minX, rightNode.minY, rightNode.minZ,
                                      rightNode.maxX, rightNode.maxY, rightNode.maxZ,
                                      tRight)
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
