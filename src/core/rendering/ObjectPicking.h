#pragma once

#include <QList>
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

  static Hit pick(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches);

  static int pickSourceObject(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches);

private:
  ObjectPicking() = delete;
};
