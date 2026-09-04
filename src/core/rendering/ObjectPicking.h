#pragma once

#include <QList>
#include <QVector>
#include <QVector3D>

#include "qml_gui/Renderer/PrepareSceneData.h"

// Pure CPU object picking helper for the default RHI path.
// Uses scene-space PrepareSceneData vertices: ray -> AABB prefilter ->
// Moller-Trumbore triangle intersection -> nearest source-object hit.
//
// Phase 113 (MEASURE-02) two-stage pick (pitfall 7): this is STAGE 1 -- the
// coarse ray->AABB prefilter that narrows the scene to candidate source
// objects. STAGE 2 (per-triangle ITS raycast over the candidate volumes only)
// lives in SceneRaycaster::hitTest (core/rendering/SceneRaycaster.h). The
// picking bridge hands the stage-1 survivor(s) to SceneRaycaster as
// SceneRaycasterCandidate entries; SceneRaycaster never loops the whole
// scene, which is the upstream GLGizmoMeasure perf bug pitfall 7 calls out.
class ObjectPicking final
{
public:
  struct Hit
  {
    int sourceObjectIndex = -1;
    int volumeIndex = -1;
    int instanceIndex = -1;
    QVector3D position;
    float distance = -1.0f;

    bool isValid() const
    {
      return sourceObjectIndex >= 0 && volumeIndex >= 0 && instanceIndex >= 0
          && distance >= 0.0f;
    }
  };

  // Wave 2 (perf): flat AABB BVH over one batch's triangle soup. Upstream
  // picking is backed by AABBMesh's BVH (MeshUtils.hpp:159-167,
  // SceneRaycaster.cpp:99-170); the Qt port's hover/press pick previously
  // brute-forced every triangle per mouse move (measured 33 ms/ray at 2M
  // triangles, docs/perf-baseline.md). Build once per mesh change, raycast
  // per event: same Moller-Trumbore math, ordered BVH traversal instead of a
  // linear sweep.
  class Bvh
  {
  public:
    // vertices[firstVertex, firstVertex+vertexCount) is a triangle soup
    // (vertexCount % 3 == 0). Returns an empty Bvh for invalid ranges.
    static Bvh build(const QList<PrepareSceneData::ModelVertex> &vertices,
                     int firstVertex, int vertexCount);

    bool isEmpty() const { return m_nodes.isEmpty(); }

    // Nearest hit along the (normalized) ray closer than maxDistance.
    // Updates hitPosition/hitDistance; returns false when nothing hit.
    bool raycast(const QList<PrepareSceneData::ModelVertex> &vertices,
                 const QVector3D &origin, const QVector3D &direction,
                 float maxDistance,
                 QVector3D *hitPosition, float *hitDistance) const;

  private:
    struct Node
    {
      float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
      float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
      qint32 left = -1;      // < 0 => leaf (triStart/triCount valid)
      qint32 right = -1;
      qint32 triStart = 0;   // leaf: range into m_triangles
      qint32 triCount = 0;
    };

    QVector<Node> m_nodes;
    QVector<qint32> m_triangles; // local triangle index (0-based in range)
    int m_firstVertex = 0;
  };

  static Hit pick(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches);

  // Accelerated variant: batchBvhs[i] accelerates batches[i] (empty Bvh =>
  // that batch falls back to the brute-force sweep). Identical hit
  // semantics to the 4-argument pick.
  static Hit pick(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches,
      const QVector<Bvh> &batchBvhs);

  static int pickSourceObject(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches);

  static int pickSourceObject(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches,
      const QVector<Bvh> &batchBvhs);

private:
  ObjectPicking() = delete;
};
