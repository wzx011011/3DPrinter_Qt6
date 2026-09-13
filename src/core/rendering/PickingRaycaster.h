#pragma once

#include <QList>
#include <QVector3D>

#include "core/rendering/ObjectPicking.h"

#include <vector>

// BVH-accelerated scene picking -- the Qt6 counterpart of the upstream
// per-volume GUI::MeshRaycaster AABB tree (MeshUtils.hpp:159): the tree is
// built once per scene revision and each ray test descends O(log n) nodes
// instead of sweeping every triangle (ObjectPicking::pick stays the
// brute-force reference path).
//
// The tree is a binned-SAH BVH over per-triangle AABBs of all pickable
// batches (same validBatchSpan predicate as the brute-force sweep). Leaf
// tests read vertices from the caller's list, so the raycaster stores only
// indices and node bounds; pick() reproduces ObjectPicking::pick nearest-hit
// semantics exactly (parity asserted by ObjectPickingTests and PerfBench).
class PickingRaycaster final
{
public:
  // Rebuilds the tree over every pickable triangle of `batches`. Triangles
  // with non-finite coordinates are excluded (they can never produce a hit
  // in the brute-force sweep either).
  void build(const QList<PrepareSceneData::ModelVertex> &vertices,
             const QList<PrepareSceneData::ModelBatch> &batches);

  // Nearest-hit semantics identical to ObjectPicking::pick. `vertices` and
  // `batches` must be the same lists the tree was built from.
  ObjectPicking::Hit pick(const QVector3D &rayOrigin,
                          const QVector3D &rayDirection,
                          const QList<PrepareSceneData::ModelVertex> &vertices,
                          const QList<PrepareSceneData::ModelBatch> &batches) const;

  bool isValid() const { return m_triangleCount > 0 && !m_nodes.empty(); }
  int triangleCount() const { return m_triangleCount; }
  int nodeCount() const { return int(m_nodes.size()); }

private:
  struct Node
  {
    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;
    // Leaf (triangleCount > 0): first slot in m_triangleOrder.
    // Internal (triangleCount == 0): left child node index.
    int firstChildOrTriangle = 0;
    // Internal only: right child node index (subtrees are not contiguous,
    // so the right root must be stored explicitly).
    int secondChildOrZero = 0;
    int triangleCount = 0;

    PrepareSceneData::ModelBounds bounds() const
    {
      return {minX, minY, minZ, maxX, maxY, maxZ};
    }
  };

  // Transient build state (declared here so buildRange can be a member;
  // defined in the .cpp).
  struct BuildScratch;
  // Recursive binned-SAH builder over m_triangleOrder[first, first+count).
  // Returns the new subtree root's node index.
  int buildRange(BuildScratch &scratch, int first, int count, int depth);

  std::vector<Node> m_nodes;
  // Triangle ordinals permuted by the build partitions; leaf slots index in.
  std::vector<int> m_triangleOrder;
  // Per ordinal: owning batch index into the batches list, and the first of
  // the triangle's 3 consecutive vertex indices.
  std::vector<int> m_triangleBatch;
  std::vector<int> m_triangleFirstVertex;
  int m_triangleCount = 0;
};
