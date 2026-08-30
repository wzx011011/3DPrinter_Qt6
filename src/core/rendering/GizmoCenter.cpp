#include "GizmoCenter.h"

#include <algorithm>

namespace GizmoCenter
{
QVector3D fromSelectedBatch(int selectedSourceObjectIndex,
                            const QList<PrepareSceneData::ModelBatch> &batches)
{
  if (selectedSourceObjectIndex < 0)
    return {}; // no selection - gizmo sits at origin

  PrepareSceneData::ModelBounds bounds;
  bool found = false;
  for (const auto &b : batches)
  {
    if (b.sourceObjectIndex != selectedSourceObjectIndex)
      continue;

    if (!found)
    {
      bounds = b.bounds;
      found = true;
      continue;
    }

    // Selection::get_bounding_box() merges all selected volume bounds before
    // taking the center, so do the same for every render batch of this object.
    bounds.minX = std::min(bounds.minX, b.bounds.minX);
    bounds.minY = std::min(bounds.minY, b.bounds.minY);
    bounds.minZ = std::min(bounds.minZ, b.bounds.minZ);
    bounds.maxX = std::max(bounds.maxX, b.bounds.maxX);
    bounds.maxY = std::max(bounds.maxY, b.bounds.maxY);
    bounds.maxZ = std::max(bounds.maxZ, b.bounds.maxZ);
  }

  if (!found)
    return {}; // selected index not in current batches (stale selection)

  // Midpoint of the unioned world-space AABB, matching Selection::get_bounding_box().center().
  return QVector3D(
      (bounds.minX + bounds.maxX) * 0.5f,
      (bounds.minY + bounds.maxY) * 0.5f,
      (bounds.minZ + bounds.maxZ) * 0.5f);
}
} // namespace GizmoCenter
