#include "GizmoCenter.h"

namespace GizmoCenter
{
QVector3D fromSelectedBatch(int selectedSourceObjectIndex,
                            const QList<PrepareSceneData::ModelBatch> &batches)
{
  if (selectedSourceObjectIndex < 0)
    return {}; // no selection - gizmo sits at origin
  for (const auto &b : batches)
  {
    if (b.sourceObjectIndex == selectedSourceObjectIndex)
    {
      // Midpoint of the world-space AABB. Matches GL path's m_gizmoCenter
      // derivation (object bbox center, not bed origin).
      return QVector3D(
          (b.bounds.minX + b.bounds.maxX) * 0.5f,
          (b.bounds.minY + b.bounds.maxY) * 0.5f,
          (b.bounds.minZ + b.bounds.maxZ) * 0.5f);
    }
  }
  return {}; // selected index not in current batches (stale selection)
}
} // namespace GizmoCenter
