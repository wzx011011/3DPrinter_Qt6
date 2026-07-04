#pragma once

#include <QList>
#include <QVector3D>

#include "qml_gui/Renderer/PrepareSceneData.h"

// Pure CPU object picking helper for the default RHI path.
// Uses scene-space PrepareSceneData vertices: ray -> AABB prefilter ->
// Moller-Trumbore triangle intersection -> nearest source-object hit.
class ObjectPicking final
{
public:
  static int pickSourceObject(
      const QVector3D &rayOrigin,
      const QVector3D &rayDirection,
      const QList<PrepareSceneData::ModelVertex> &vertices,
      const QList<PrepareSceneData::ModelBatch> &batches);

private:
  ObjectPicking() = delete;
};
