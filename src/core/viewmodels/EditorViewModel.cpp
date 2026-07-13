#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/UndoCommands.h"
#include "core/model/PartPlateList.h"
#include "core/rendering/AssemblyMeasureGeometry.h"
#include "core/viewmodels/ConfigViewModel.h"
#ifdef HAS_LIBSLIC3R
// Phase 114 (MEASURE-03): MeasureEngine instantiates Measure::Measuring per
// volume from ProjectServiceMock::volumeMeshIts (Phase 112 ITS accessor) and
// scrubs the SurfaceFeature back-pointers at the boundary (pitfall 6). The
// SceneRaycaster (Phase 113) produces the hit that feeds getFeature.
#include "core/rendering/MeasureEngine.h"
// Phase 115 (MEASURE-04): SceneRaycaster runs the two-stage pick stage-2
// (per-triangle ITS raycast over the stage-1 candidate volumes only --
// pitfall 7 mitigation). The candidate list handed to hitTest is the single
// stage-1 survivor (RhiViewport::pickSourceObjectAt), so the per-frame cost
// is O(hit volume), NOT O(all volumes) (the upstream perf bug).
#include "core/rendering/SceneRaycaster.h"
#include <libslic3r/Geometry.hpp>  // Geometry::assemble_transform (canonical TRS)
#include <libslic3r/Point.hpp>     // Vec3d / Transform3d for world-transform rebuild
#endif
#include <QFileInfo>
#include <QUrl>
#include <QVector4D>
#include <QSettings>
#include <algorithm>
#include <QSet>
#include <QTimer>
#include <cstring> // memcpy
#include <cmath>
#include <QDebug>
#include <QMap>

namespace
{
constexpr float kRadiansToDegrees = 180.0f / float(M_PI);
constexpr bool kViewportTrianglePickingAvailable = false;
constexpr bool kCgalMeshBooleanAvailable = false;

// Phase 93 (ASMROUTE-02): parse the cached mesh blob into a map of
// sourceObjectIndex -> unioned AABB, the per-object bounds source Phase 92
// encapsulated inline in selectedVolumeBoundsForAssemblyMeasure(). Shared by
// refreshAssembleViewDataPool() (feeds the pool's ModelObjectsInfo) and
// selectedVolumeBoundsForAssemblyMeasure() (reads the two selected bounds) so
// the bounds source is single-sourced. Returns an empty map on a malformed
// blob (same early-return contract as the Phase 92 inline parse). The blob
// format mirrors PrepareSceneData::setModelMeshData: [objectCount int32] then
// per-batch [renderObjectId int32][triangleCount int32][verts...], followed by
// a 6-float global bbox trailer (skipped here).
QMap<int, PrepareSceneData::ModelBounds>
parseAllSourceObjectBoundsFromMeshBlob(const QByteArray &blob,
                                       const QList<int> &batchSourceObjectIndices)
{
  QMap<int, PrepareSceneData::ModelBounds> result;
  constexpr qsizetype kFloatBytes = qsizetype(sizeof(float));
  constexpr qsizetype kIntBytes = qsizetype(sizeof(qint32));
  constexpr qsizetype kVertexBytes = 3 * kFloatBytes;

  qsizetype offset = 0;
  qint32 objectCount = 0;
  if (blob.size() < qsizetype(sizeof(qint32)))
    return result;
  memcpy(&objectCount, blob.constData() + offset, kIntBytes);
  offset += kIntBytes;
  if (objectCount < 0 || objectCount > batchSourceObjectIndices.size())
    return result;

  for (qint32 i = 0; i < objectCount; ++i)
  {
    if (offset + 2 * kIntBytes > blob.size())
      return {};
    qint32 renderObjectId = 0;
    qint32 triangleCount = 0;
    memcpy(&renderObjectId, blob.constData() + offset, kIntBytes);
    offset += kIntBytes;
    memcpy(&triangleCount, blob.constData() + offset, kIntBytes);
    offset += kIntBytes;
    if (triangleCount < 0)
      return result;
    (void)renderObjectId; // not needed for bounds derivation

    const qsizetype vertexCount = qsizetype(triangleCount) * 3;
    const qsizetype payloadBytes = vertexCount * kVertexBytes;
    if (payloadBytes < 0 || blob.size() - offset < payloadBytes)
      return result;

    const int sourceIndex = batchSourceObjectIndices.value(i, -1);
    if (sourceIndex >= 0)
    {
      PrepareSceneData::ModelBounds batchBounds{};
      bool first = true;
      for (qsizetype v = 0; v < vertexCount; ++v)
      {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        memcpy(&x, blob.constData() + offset + v * kVertexBytes + 0 * kFloatBytes, kFloatBytes);
        memcpy(&y, blob.constData() + offset + v * kVertexBytes + 1 * kFloatBytes, kFloatBytes);
        memcpy(&z, blob.constData() + offset + v * kVertexBytes + 2 * kFloatBytes, kFloatBytes);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
        {
          first = false;
          break;
        }
        if (first)
        {
          batchBounds = PrepareSceneData::ModelBounds{x, y, z, x, y, z};
          first = false;
        }
        else
        {
          batchBounds.minX = std::min(batchBounds.minX, x);
          batchBounds.minY = std::min(batchBounds.minY, y);
          batchBounds.minZ = std::min(batchBounds.minZ, z);
          batchBounds.maxX = std::max(batchBounds.maxX, x);
          batchBounds.maxY = std::max(batchBounds.maxY, y);
          batchBounds.maxZ = std::max(batchBounds.maxZ, z);
        }
      }
      if (!first)
      {
        // Union this batch into the source object's running bounds (a source
        // object may have multiple volume batches — mirrors the renderer's
        // uploadHighlightBuffer union).
        auto it = result.find(sourceIndex);
        if (it == result.end())
          result.insert(sourceIndex, batchBounds);
        else
        {
          it.value().minX = std::min(it.value().minX, batchBounds.minX);
          it.value().minY = std::min(it.value().minY, batchBounds.minY);
          it.value().minZ = std::min(it.value().minZ, batchBounds.minZ);
          it.value().maxX = std::max(it.value().maxX, batchBounds.maxX);
          it.value().maxY = std::max(it.value().maxY, batchBounds.maxY);
          it.value().maxZ = std::max(it.value().maxZ, batchBounds.maxZ);
        }
      }
    }
    offset += payloadBytes;
    if (offset > blob.size())
      return result;
  }
  return result;
}
} // namespace

void EditorViewModel::invalidateSliceResultsForCurrentPlate()
{
  const int plateIdx = projectService_ ? projectService_->currentPlateIndex() : -1;
  invalidateSliceResultsForPlate(plateIdx);
}

void EditorViewModel::invalidateSliceResultsForPlate(int plateIndex)
{
  if (plateIndex < 0)
    return;

  const bool hadKnownResult = m_slicedPlateIndices.remove(plateIndex)
      || (sliceService_ && sliceService_->hasPlateResult(plateIndex))
      || m_sliceResultPlateIndex == plateIndex;
  if (hadKnownResult)
    m_stalePlateIndices.insert(plateIndex);

  if (sliceService_)
    sliceService_->removePlateResult(plateIndex);

  if (m_sliceResultPlateIndex == plateIndex)
  {
    m_sliceEstimatedTime.clear();
    m_sliceResultPlateIndex = -1;
  }
}

void EditorViewModel::invalidateAllSliceResults()
{
  QSet<int> knownPlates = m_slicedPlateIndices;
  if (sliceService_ && sliceService_->resultPlateIndex() >= 0)
    knownPlates.insert(sliceService_->resultPlateIndex());
  if (m_sliceResultPlateIndex >= 0)
    knownPlates.insert(m_sliceResultPlateIndex);

  for (int plateIndex : knownPlates)
  {
    if (plateIndex >= 0)
      m_stalePlateIndices.insert(plateIndex);
  }

  m_slicedPlateIndices.clear();
  m_sliceEstimatedTime.clear();
  m_sliceResultPlateIndex = -1;
  if (sliceService_)
    sliceService_->clearResults();
}

void EditorViewModel::refreshMeshCacheAndFitHint()
{
  m_cachedMeshData = projectService_->meshData();
  m_cachedMeshBatchSourceObjectIndices = projectService_->meshBatchSourceObjectIndices();
  m_fitHint = QVector4D();

  constexpr int kBboxBytes = 6 * int(sizeof(float));
  if (m_cachedMeshData.size() < kBboxBytes)
    return;

  float bbox[6];
  memcpy(bbox, m_cachedMeshData.constData() + m_cachedMeshData.size() - kBboxBytes, kBboxBytes);
  const float cx = (bbox[0] + bbox[3]) * 0.5f;
  const float cy = (bbox[1] + bbox[4]) * 0.5f;
  const float cz = (bbox[2] + bbox[5]) * 0.5f;
  const float dx = bbox[3] - bbox[0];
  const float dy = bbox[4] - bbox[1];
  const float dz = bbox[5] - bbox[2];
  const float radius = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;
  m_fitHint = QVector4D(cx, cy, cz, std::max(radius, 10.f));

  // 更新测量尺寸（边界框 X/Y/Z + 体积）
  m_measureDimensions = QVector4D(dx, dy, dz, dx * dy * dz);

  // 检查视口告警（对齐上游 Plater::check_outside_state / EWarning）
  checkViewportWarnings();

  // Phase 93 (ASMROUTE-02): when on AssembleView, refresh the data pool so the
  // cached per-object info tracks the freshly-loaded mesh. Gated to
  // m_activeCanvasType == 2 so Prepare/Preview never populate the pool
  // (mirrors upstream GLGizmosManager.cpp:427-431 running the pool update in
  // the AssembleView-only gizmo update).
  if (m_activeCanvasType == 2)
  {
    refreshAssembleViewDataPool();
    m_assembleViewDataPool.update(AssembleViewDataID::ModelObjectsInfo);
  }
}

// ── Object manipulation (对齐上游 ObjectManipulation) ──────────────────────

static int primarySelectedSourceIndex(const EditorViewModel *vm)
{
  // Use single object selection for manipulation; fall back to primary
  if (!vm)
    return -1;
  if (vm->selectedObjectCount() == 1)
    return vm->selectedSourceObjectIndex();
  return -1; // Multi-select: no numeric editing
}

float EditorViewModel::objectPosX() const { return projectService_ ? projectService_->objectPosition(primarySelectedSourceIndex(this)).x() : 0; }
float EditorViewModel::objectPosY() const { return projectService_ ? projectService_->objectPosition(primarySelectedSourceIndex(this)).y() : 0; }
float EditorViewModel::objectPosZ() const { return projectService_ ? projectService_->objectPosition(primarySelectedSourceIndex(this)).z() : 0; }
float EditorViewModel::objectRotX() const { return projectService_ ? projectService_->objectRotation(primarySelectedSourceIndex(this)).x() : 0; }
float EditorViewModel::objectRotY() const { return projectService_ ? projectService_->objectRotation(primarySelectedSourceIndex(this)).y() : 0; }
float EditorViewModel::objectRotZ() const { return projectService_ ? projectService_->objectRotation(primarySelectedSourceIndex(this)).z() : 0; }
float EditorViewModel::objectScaleX() const { return projectService_ ? projectService_->objectScale(primarySelectedSourceIndex(this)).x() : 1; }
float EditorViewModel::objectScaleY() const { return projectService_ ? projectService_->objectScale(primarySelectedSourceIndex(this)).y() : 1; }
float EditorViewModel::objectScaleZ() const { return projectService_ ? projectService_->objectScale(primarySelectedSourceIndex(this)).z() : 1; }
bool EditorViewModel::uniformScale() const { return m_uniformScale; }
bool EditorViewModel::hasObjectManipSelection() const { return selectedObjectCount() == 1 && !hasSelectedVolume(); }

void EditorViewModel::setObjectPosX(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldPos = projectService_->objectPosition(idx);
  if (qFuzzyCompare(oldPos.x(), v))
    return;
  projectService_->setObjectPosition(idx, v, oldPos.y(), oldPos.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, oldPos, projectService_->objectRotation(idx),
                                     projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(QVector3D(v, oldPos.y(), oldPos.z()),
                         projectService_->objectRotation(idx),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectPosY(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldPos = projectService_->objectPosition(idx);
  if (qFuzzyCompare(oldPos.y(), v))
    return;
  projectService_->setObjectPosition(idx, oldPos.x(), v, oldPos.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, oldPos, projectService_->objectRotation(idx),
                                     projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(QVector3D(oldPos.x(), v, oldPos.z()),
                         projectService_->objectRotation(idx),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectPosZ(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldPos = projectService_->objectPosition(idx);
  if (qFuzzyCompare(oldPos.z(), v))
    return;
  projectService_->setObjectPosition(idx, oldPos.x(), oldPos.y(), v);
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, oldPos, projectService_->objectRotation(idx),
                                     projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(QVector3D(oldPos.x(), oldPos.y(), v),
                         projectService_->objectRotation(idx),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}

void EditorViewModel::beginGizmoMoveDrag()
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
  {
    m_gizmoMoveDragActive = false;
    m_gizmoMoveDragSourceIndex = -1;
    m_gizmoMoveDragStartPos = {};
    return;
  }

  m_gizmoMoveDragActive = true;
  m_gizmoMoveDragSourceIndex = idx;
  m_gizmoMoveDragStartPos = projectService_->objectPosition(idx);
}

void EditorViewModel::applyGizmoMoveDelta(float dx, float dy, float dz)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;

  const QVector3D delta(dx, dy, dz);
  if (delta.lengthSquared() <= 1e-12f)
    return;

  if (m_gizmoMoveDragActive && idx != m_gizmoMoveDragSourceIndex)
    return;

  const QVector3D oldPos = projectService_->objectPosition(idx);
  const QVector3D newPos = oldPos + delta;
  projectService_->setObjectPosition(idx, newPos.x(), newPos.y(), newPos.z());
  invalidateSliceResultsForCurrentPlate();

  if (!m_gizmoMoveDragActive && m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, oldPos, projectService_->objectRotation(idx),
                                     projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(newPos,
                         projectService_->objectRotation(idx),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }

  emit stateChanged();
}

void EditorViewModel::endGizmoMoveDrag()
{
  if (!m_gizmoMoveDragActive)
    return;

  const int idx = m_gizmoMoveDragSourceIndex;
  m_gizmoMoveDragActive = false;
  m_gizmoMoveDragSourceIndex = -1;

  if (idx < 0 || !projectService_)
    return;

  const QVector3D finalPos = projectService_->objectPosition(idx);
  const QVector3D delta = finalPos - m_gizmoMoveDragStartPos;
  if (delta.lengthSquared() <= 1e-12f)
  {
    m_gizmoMoveDragStartPos = {};
    return;
  }

  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, m_gizmoMoveDragStartPos,
                                     projectService_->objectRotation(idx),
                                     projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(finalPos,
                         projectService_->objectRotation(idx),
                         projectService_->objectScale(idx));
    if (m_undoManager->isInMacro()) {
      m_undoManager->push(cmd);
    } else {
      m_undoManager->beginMacro(tr("Move object"));
      m_undoManager->push(cmd);
      m_undoManager->endMacro();
    }
  }

  m_gizmoMoveDragStartPos = {};
  emit stateChanged();
}

void EditorViewModel::beginGizmoRotateDrag()
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
  {
    m_gizmoRotateDragActive = false;
    m_gizmoRotateDragSourceIndex = -1;
    m_gizmoRotateDragStartRot = {};
    return;
  }

  m_gizmoRotateDragActive = true;
  m_gizmoRotateDragSourceIndex = idx;
  m_gizmoRotateDragStartRot = projectService_->objectRotation(idx);
}

void EditorViewModel::applyGizmoRotateDelta(int axis, float radians)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_ || axis < 1 || axis > 3)
    return;

  if (!std::isfinite(radians))
    return;

  if (qFuzzyIsNull(radians))
    return;

  if (m_gizmoRotateDragActive && idx != m_gizmoRotateDragSourceIndex)
    return;

  const QVector3D oldRot = projectService_->objectRotation(idx);
  QVector3D newRot = oldRot;
  const float degrees = radians * kRadiansToDegrees;
  if (axis == 1)
    newRot.setX(newRot.x() + degrees);
  else if (axis == 2)
    newRot.setY(newRot.y() + degrees);
  else
    newRot.setZ(newRot.z() + degrees);

  projectService_->setObjectRotation(idx, newRot.x(), newRot.y(), newRot.z());
  invalidateSliceResultsForCurrentPlate();

  if (!m_gizmoRotateDragActive && m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     oldRot, projectService_->objectScale(idx),
                                     projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         newRot,
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }

  emit stateChanged();
}

void EditorViewModel::endGizmoRotateDrag()
{
  if (!m_gizmoRotateDragActive)
    return;

  const int idx = m_gizmoRotateDragSourceIndex;
  m_gizmoRotateDragActive = false;
  m_gizmoRotateDragSourceIndex = -1;

  if (idx < 0 || !projectService_)
    return;

  const QVector3D finalRot = projectService_->objectRotation(idx);
  if ((finalRot - m_gizmoRotateDragStartRot).lengthSquared() <= 1e-12f)
  {
    m_gizmoRotateDragStartRot = {};
    return;
  }

  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     m_gizmoRotateDragStartRot,
                                     projectService_->objectScale(idx),
                                     projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         finalRot,
                         projectService_->objectScale(idx));
    if (m_undoManager->isInMacro()) {
      m_undoManager->push(cmd);
    } else {
      m_undoManager->beginMacro(tr("Rotate object"));
      m_undoManager->push(cmd);
      m_undoManager->endMacro();
    }
  }

  m_gizmoRotateDragStartRot = {};
  emit stateChanged();
}

void EditorViewModel::beginGizmoScaleDrag()
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
  {
    m_gizmoScaleDragActive = false;
    m_gizmoScaleDragSourceIndex = -1;
    m_gizmoScaleDragStartScale = {};
    return;
  }

  m_gizmoScaleDragActive = true;
  m_gizmoScaleDragSourceIndex = idx;
  m_gizmoScaleDragStartScale = projectService_->objectScale(idx);
}

void EditorViewModel::applyGizmoScaleFactor(int axis, float factor)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_ || axis < 1 || axis > 3)
    return;

  if (!std::isfinite(factor))
    return;

  const float clampedFactor = std::max(factor, 0.01f);
  if (qFuzzyCompare(clampedFactor, 1.0f))
    return;

  if (m_gizmoScaleDragActive && idx != m_gizmoScaleDragSourceIndex)
    return;

  const QVector3D oldScale = projectService_->objectScale(idx);
  QVector3D newScale = oldScale;
  if (axis == 1)
    newScale.setX(newScale.x() * clampedFactor);
  else if (axis == 2)
    newScale.setY(newScale.y() * clampedFactor);
  else
    newScale.setZ(newScale.z() * clampedFactor);

  projectService_->setObjectScale(idx, newScale.x(), newScale.y(), newScale.z());
  invalidateSliceResultsForCurrentPlate();

  if (!m_gizmoScaleDragActive && m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx), oldScale,
                                     projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx),
                         newScale);
    m_undoManager->push(cmd);
  }

  emit stateChanged();
}

void EditorViewModel::endGizmoScaleDrag()
{
  if (!m_gizmoScaleDragActive)
    return;

  const int idx = m_gizmoScaleDragSourceIndex;
  m_gizmoScaleDragActive = false;
  m_gizmoScaleDragSourceIndex = -1;

  if (idx < 0 || !projectService_)
    return;

  const QVector3D finalScale = projectService_->objectScale(idx);
  if ((finalScale - m_gizmoScaleDragStartScale).lengthSquared() <= 1e-12f)
  {
    m_gizmoScaleDragStartScale = {};
    return;
  }

  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx),
                                     m_gizmoScaleDragStartScale,
                                     projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx),
                         finalScale);
    if (m_undoManager->isInMacro()) {
      m_undoManager->push(cmd);
    } else {
      m_undoManager->beginMacro(tr("Scale object"));
      m_undoManager->push(cmd);
      m_undoManager->endMacro();
    }
  }

  m_gizmoScaleDragStartScale = {};
  emit stateChanged();
}

void EditorViewModel::setObjectRotX(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldRot = projectService_->objectRotation(idx);
  if (qFuzzyCompare(oldRot.x(), v))
    return;
  projectService_->setObjectRotation(idx, v, oldRot.y(), oldRot.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     oldRot, projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         QVector3D(v, oldRot.y(), oldRot.z()),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectRotY(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldRot = projectService_->objectRotation(idx);
  if (qFuzzyCompare(oldRot.y(), v))
    return;
  projectService_->setObjectRotation(idx, oldRot.x(), v, oldRot.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     oldRot, projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         QVector3D(oldRot.x(), v, oldRot.z()),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectRotZ(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldRot = projectService_->objectRotation(idx);
  if (qFuzzyCompare(oldRot.z(), v))
    return;
  projectService_->setObjectRotation(idx, oldRot.x(), oldRot.y(), v);
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     oldRot, projectService_->objectScale(idx), projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         QVector3D(oldRot.x(), oldRot.y(), v),
                         projectService_->objectScale(idx));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectScaleX(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldScale = projectService_->objectScale(idx);
  if (qFuzzyCompare(oldScale.x(), v))
    return;
  if (m_uniformScale)
    projectService_->setObjectScaleUniform(idx, v);
  else
    projectService_->setObjectScale(idx, v, oldScale.y(), oldScale.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx), oldScale, projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx),
                         m_uniformScale ? QVector3D(v, v, v) : QVector3D(v, oldScale.y(), oldScale.z()));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectScaleY(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldScale = projectService_->objectScale(idx);
  if (qFuzzyCompare(oldScale.y(), v))
    return;
  if (m_uniformScale)
    projectService_->setObjectScaleUniform(idx, v);
  else
    projectService_->setObjectScale(idx, oldScale.x(), v, oldScale.z());
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx), oldScale, projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx),
                         m_uniformScale ? QVector3D(v, v, v) : QVector3D(oldScale.x(), v, oldScale.z()));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setObjectScaleZ(float v)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldScale = projectService_->objectScale(idx);
  if (qFuzzyCompare(oldScale.z(), v))
    return;
  if (m_uniformScale)
    projectService_->setObjectScaleUniform(idx, v);
  else
    projectService_->setObjectScale(idx, oldScale.x(), oldScale.y(), v);
  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx), oldScale, projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx),
                         m_uniformScale ? QVector3D(v, v, v) : QVector3D(oldScale.x(), oldScale.y(), v));
    m_undoManager->push(cmd);
  }
}
void EditorViewModel::setUniformScale(bool v)
{
  m_uniformScale = v;
  emit stateChanged();
}

void EditorViewModel::resetObjectTransform()
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;

  const QVector3D oldPos = projectService_->objectPosition(idx);
  const QVector3D oldRot = projectService_->objectRotation(idx);
  const QVector3D oldScale = projectService_->objectScale(idx);

  projectService_->setObjectPosition(idx, 0, 0, 0);
  projectService_->setObjectRotation(idx, 0, 0, 0);
  projectService_->setObjectScaleUniform(idx, 1);

  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, oldPos, oldRot, oldScale, projectService_);
    cmd->setNewTransform(QVector3D(0, 0, 0), QVector3D(0, 0, 0), QVector3D(1, 1, 1));
    m_undoManager->push(cmd);
  }
}

void EditorViewModel::applyScaleFactor(float factor)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D oldScale = projectService_->objectScale(idx);
  const QVector3D newScale(oldScale.x() * factor, oldScale.y() * factor, oldScale.z() * factor);
  projectService_->setObjectScale(idx, newScale.x(), newScale.y(), newScale.z());

  if (m_undoManager)
  {
    auto *cmd = new TransformCommand(idx, projectService_->objectPosition(idx),
                                     projectService_->objectRotation(idx), oldScale, projectService_);
    cmd->setNewTransform(projectService_->objectPosition(idx),
                         projectService_->objectRotation(idx), newScale);
    m_undoManager->push(cmd);
  }
}

// --- Flatten / Cut (对齐上游 GLGizmoFlatten / GLGizmoCut) ---

void EditorViewModel::setCutAxis(int axis)
{
  m_cutAxis = axis;
  emit stateChanged();
}
void EditorViewModel::setCutPosition(double pos)
{
  m_cutPosition = pos;
  emit stateChanged();
}
void EditorViewModel::setCutKeepMode(int mode)
{
  m_cutKeepMode = mode;
  emit stateChanged();
}

// Cut connector settings (对齐上游 GLGizmoCut connector)
int EditorViewModel::cutMode() const { return m_cutMode; }
void EditorViewModel::setCutMode(int mode)
{
  if (m_cutMode != mode)
  {
    m_cutMode = mode;
    emit stateChanged();
  }
}
int EditorViewModel::connectorType() const { return m_connectorType; }
void EditorViewModel::setConnectorType(int v)
{
  if (m_connectorType == v)
    return;

  m_connectorType = v;

  // 对齐上游 GLGizmoCut: Dowel 固定 Prism，Snap 固定 Circle。
  if (m_connectorType == 1)
    m_connectorStyle = 0;
  else if (m_connectorType == 2)
    m_connectorShape = 3;

  emit stateChanged();
}
int EditorViewModel::connectorStyle() const { return m_connectorStyle; }
void EditorViewModel::setConnectorStyle(int v)
{
  if (m_connectorType != 0)
    v = 0;
  if (m_connectorStyle != v)
  {
    m_connectorStyle = v;
    emit stateChanged();
  }
}
int EditorViewModel::connectorShape() const { return m_connectorShape; }
void EditorViewModel::setConnectorShape(int v)
{
  if (m_connectorType == 2)
    v = 3;
  if (m_connectorShape != v)
  {
    m_connectorShape = v;
    emit stateChanged();
  }
}
float EditorViewModel::connectorSize() const { return m_connectorSize; }
void EditorViewModel::setConnectorSize(float v)
{
  if (!qFuzzyCompare(m_connectorSize, v))
  {
    m_connectorSize = v;
    emit stateChanged();
  }
}
float EditorViewModel::connectorDepth() const { return m_connectorDepth; }
void EditorViewModel::setConnectorDepth(float v)
{
  if (!qFuzzyCompare(m_connectorDepth, v))
  {
    m_connectorDepth = v;
    emit stateChanged();
  }
}

// Measure selection mode (对齐上游 GLGizmoMeasure)
int EditorViewModel::measureSelectionMode() const { return m_measureSelectionMode; }
void EditorViewModel::setMeasureSelectionMode(int mode)
{
  if (m_measureSelectionMode != mode)
  {
    m_measureSelectionMode = mode;
    emit stateChanged();
  }
}

// Support painting (对齐上游 GLGizmoFdmSupports)
int EditorViewModel::supportPaintTool() const { return m_supportPaintTool; }
void EditorViewModel::setSupportPaintTool(int tool)
{
  if (m_supportPaintTool != tool)
  {
    m_supportPaintTool = tool;
    emit stateChanged();
  }
}

int EditorViewModel::supportPaintCursorType() const { return m_supportPaintCursorType; }
void EditorViewModel::setSupportPaintCursorType(int type)
{
  if (m_supportPaintCursorType != type)
  {
    m_supportPaintCursorType = type;
    emit stateChanged();
  }
}

int EditorViewModel::supportPaintToolType() const { return m_supportPaintToolType; }
void EditorViewModel::setSupportPaintToolType(int type)
{
  if (m_supportPaintToolType != type)
  {
    m_supportPaintToolType = type;
    emit stateChanged();
  }
}

float EditorViewModel::supportPaintCursorRadius() const { return m_supportPaintCursorRadius; }
void EditorViewModel::setSupportPaintCursorRadius(float radius)
{
  if (!qFuzzyCompare(m_supportPaintCursorRadius, radius))
  {
    m_supportPaintCursorRadius = radius;
    emit stateChanged();
  }
}

float EditorViewModel::supportPaintAngleThreshold() const { return m_supportPaintAngleThreshold; }
void EditorViewModel::setSupportPaintAngleThreshold(float angle)
{
  if (!qFuzzyCompare(m_supportPaintAngleThreshold, angle))
  {
    m_supportPaintAngleThreshold = angle;
    emit stateChanged();
  }
}

float EditorViewModel::supportPaintSmartFillAngle() const { return m_supportPaintSmartFillAngle; }
void EditorViewModel::setSupportPaintSmartFillAngle(float angle)
{
  if (!qFuzzyCompare(m_supportPaintSmartFillAngle, angle))
  {
    m_supportPaintSmartFillAngle = angle;
    emit stateChanged();
  }
}

float EditorViewModel::supportPaintGapArea() const { return m_supportPaintGapArea; }
void EditorViewModel::setSupportPaintGapArea(float area)
{
  if (!qFuzzyCompare(m_supportPaintGapArea, area))
  {
    m_supportPaintGapArea = area;
    emit stateChanged();
  }
}

bool EditorViewModel::supportPaintOnOverhangsOnly() const { return m_supportPaintOnOverhangsOnly; }
void EditorViewModel::setSupportPaintOnOverhangsOnly(bool on)
{
  if (m_supportPaintOnOverhangsOnly != on)
  {
    m_supportPaintOnOverhangsOnly = on;
    emit stateChanged();
  }
}

bool EditorViewModel::supportEnable() const { return m_supportEnable; }
void EditorViewModel::setSupportEnable(bool enable)
{
  if (m_supportEnable != enable)
  {
    m_supportEnable = enable;
    emit stateChanged();
  }
}

int EditorViewModel::supportType() const { return m_supportType; }
void EditorViewModel::setSupportType(int type)
{
  if (m_supportType != type)
  {
    m_supportType = type;
    emit stateChanged();
  }
}

void EditorViewModel::setSupportPaintToolFromQml(int tool)
{
  // Map QML tool index to internal tool state
  // 0 = Enforcer, 1 = Blocker, 2 = Erase (None)
  int internalTool = (tool == 0) ? 1 : ((tool == 1) ? 2 : 0);
  setSupportPaintTool(internalTool);
}

void EditorViewModel::clearSupportPaintOnSelection()
{
  // TODO: Implement actual clear logic when mesh selection is available
  // For now, just emit state changed signal
  emit stateChanged();
}

// ── Paint data management (对齐上游 TriangleSelector) ──────────────────────

int EditorViewModel::enforcedSupportCount() const
{
  int total = 0;
  for (const auto &obj : m_paintData)
    total += obj.enforcedCount();
  return total;
}

int EditorViewModel::blockedSupportCount() const
{
  int total = 0;
  for (const auto &obj : m_paintData)
    total += obj.blockedCount();
  return total;
}

int EditorViewModel::totalPaintedTriangleCount() const
{
  int total = 0;
  for (const auto &obj : m_paintData)
    for (const auto &tri : obj.triangles)
      if (tri.state != OWzx::SupportPaintState::None) ++total;
  return total;
}

void EditorViewModel::setTriangleSupportState(int objectIndex, int triangleIndex, int paintState)
{
  // 查找或创建 ObjectPaintData 条目
  for (auto &obj : m_paintData) {
    if (obj.objectIndex == objectIndex) {
      obj.setTriangleState(triangleIndex,
                           static_cast<OWzx::SupportPaintState>(paintState));
      emit paintDataChanged();
      return;
    }
  }
  // 新对象条目
  OWzx::ObjectPaintData newObj;
  newObj.objectIndex = objectIndex;
  newObj.setTriangleState(triangleIndex,
                           static_cast<OWzx::SupportPaintState>(paintState));
  m_paintData.push_back(newObj);
  emit paintDataChanged();
}

void EditorViewModel::clearAllPaintData()
{
  for (auto &obj : m_paintData)
    obj.clearAll();
  emit paintDataChanged();
}

// ── Seam painting (对齐上游 GLGizmoSeam) ──────────────────────────────────

int EditorViewModel::seamPaintTool() const { return m_seamPaintTool; }
void EditorViewModel::setSeamPaintTool(int tool)
{
  if (m_seamPaintTool != tool) { m_seamPaintTool = tool; emit stateChanged(); }
}
float EditorViewModel::seamPaintCursorRadius() const { return m_seamPaintCursorRadius; }
void EditorViewModel::setSeamPaintCursorRadius(float radius)
{
  if (!qFuzzyCompare(m_seamPaintCursorRadius, radius)) { m_seamPaintCursorRadius = radius; emit stateChanged(); }
}
bool EditorViewModel::seamPaintOnOverhangsOnly() const { return m_seamPaintOnOverhangsOnly; }
void EditorViewModel::setSeamPaintOnOverhangsOnly(bool on)
{
  if (m_seamPaintOnOverhangsOnly != on) { m_seamPaintOnOverhangsOnly = on; emit stateChanged(); }
}
void EditorViewModel::clearSeamPaintOnSelection()
{
  // TODO: Implement actual clear logic when mesh selection is available
  emit stateChanged();
}

// ── Hollow gizmo (对齐上游 GLGizmoHollow) ─────────────────────────────────

bool EditorViewModel::hollowEnabled() const { return m_hollowEnabled; }
void EditorViewModel::setHollowEnabled(bool on)
{
  if (m_hollowEnabled != on) { m_hollowEnabled = on; emit stateChanged(); }
}
float EditorViewModel::hollowHoleRadius() const { return m_hollowHoleRadius; }
void EditorViewModel::setHollowHoleRadius(float r)
{
  if (!qFuzzyCompare(m_hollowHoleRadius, r)) { m_hollowHoleRadius = r; emit stateChanged(); }
}
float EditorViewModel::hollowHoleHeight() const { return m_hollowHoleHeight; }
void EditorViewModel::setHollowHoleHeight(float h)
{
  if (!qFuzzyCompare(m_hollowHoleHeight, h)) { m_hollowHoleHeight = h; emit stateChanged(); }
}
float EditorViewModel::hollowOffset() const { return m_hollowOffset; }
void EditorViewModel::setHollowOffset(float v)
{
  if (!qFuzzyCompare(m_hollowOffset, v)) { m_hollowOffset = v; emit stateChanged(); }
}
float EditorViewModel::hollowQuality() const { return m_hollowQuality; }
void EditorViewModel::setHollowQuality(float v)
{
  if (!qFuzzyCompare(m_hollowQuality, v)) { m_hollowQuality = v; emit stateChanged(); }
}
float EditorViewModel::hollowClosingDistance() const { return m_hollowClosingDistance; }
void EditorViewModel::setHollowClosingDistance(float v)
{
  if (!qFuzzyCompare(m_hollowClosingDistance, v)) { m_hollowClosingDistance = v; emit stateChanged(); }
}
int EditorViewModel::hollowSelectedHoleCount() const { return m_hollowSelectedHoleCount; }
void EditorViewModel::deleteSelectedHollowPoints()
{
  // TODO: Implement actual deletion when hollow point selection is available
  m_hollowSelectedHoleCount = 0;
  emit stateChanged();
}

// ── Simplify gizmo (对齐上游 GLGizmoSimplify) ──

int EditorViewModel::simplifyWantedCount() const { return m_simplifyWantedCount; }
void EditorViewModel::setSimplifyWantedCount(int count) { m_simplifyWantedCount = count; emit stateChanged(); }
float EditorViewModel::simplifyMaxError() const { return m_simplifyMaxError; }
void EditorViewModel::setSimplifyMaxError(float error) { m_simplifyMaxError = error; emit stateChanged(); }

bool EditorViewModel::simplifySelected()
{
  if (!projectService_)
    return false;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0)
    return false;

  // Capture undo snapshot before simplification
  auto *cmd = new SimplifyCommand(idx, m_simplifyWantedCount, m_simplifyMaxError, projectService_, this);

  bool ok = projectService_->simplifyObject(idx, m_simplifyWantedCount, m_simplifyMaxError);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    refreshMeshCacheAndFitHint();
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  else
  {
    delete cmd;
  }
  return ok;
}

int EditorViewModel::selectedObjectTriangleCount() const
{
  if (!projectService_)
    return 0;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0)
    return 0;
#ifdef HAS_LIBSLIC3R
  return projectService_->objectTriangleCount(idx);
#else
  return 0;
#endif
}

// ── Object info (对齐上游 Plater::show_object_info) ──

QString EditorViewModel::selectedObjectInfoText() const
{
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || !projectService_)
    return {};

  const QString name = projectService_->objectNames().value(idx);
  const QVector4D dims = m_measureDimensions;

  if (dims.x() <= 0)
    return name;

  // 对齐上游格式: "Object name" + "Size: X x Y x Z mm" + "Volume: V mm3" + "Triangles: N"
  QString info = QStringLiteral("%1 | %2 x %3 x %4 mm")
      .arg(name)
      .arg(dims.x(), 0, 'f', 2)
      .arg(dims.y(), 0, 'f', 2)
      .arg(dims.z(), 0, 'f', 2);

  const int triangles = selectedObjectTriangles();
  if (triangles > 0)
    info += QStringLiteral(" | %1 triangles").arg(triangles);

#ifdef HAS_LIBSLIC3R
  const float vol = projectService_->objectVolume(idx);
  if (vol > 0.f)
    info += QStringLiteral(" | %1 mm\u00B3").arg(QLocale().toString(vol, 'f', 1));
#else
  if (dims.w() > 0)
    info += QStringLiteral(" | %1 mm\u00B3").arg(QLocale().toString(dims.w(), 'f', 1));
#endif

  return info;
}

int EditorViewModel::selectedObjectTriangles() const
{
  return selectedObjectTriangleCount();
}

int EditorViewModel::selectedObjectOpenEdges() const
{
#ifdef HAS_LIBSLIC3R
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || !projectService_)
    return 0;
  return projectService_->objectOpenEdges(idx);
#else
  return 0;
#endif
}

int EditorViewModel::selectedObjectRepairedErrors() const
{
#ifdef HAS_LIBSLIC3R
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || !projectService_)
    return 0;
  return projectService_->objectRepairedErrors(idx);
#else
  return 0;
#endif
}

bool EditorViewModel::selectedObjectIsManifold() const
{
  return selectedObjectOpenEdges() == 0;
}

// ── MMU segmentation gizmo (对齐上游 GLGizmoMmuSegmentation) ──

int EditorViewModel::mmuSelectedExtruder() const { return m_mmuSelectedExtruder; }
void EditorViewModel::setMmuSelectedExtruder(int idx) { m_mmuSelectedExtruder = idx; emit stateChanged(); }
int EditorViewModel::mmuExtruderCount() const { return m_mmuExtruderCount; }

bool EditorViewModel::clearMmuSegmentation()
{
  // TODO: Clear per-triangle MMU facet data when TriangleSelector is available
  emit stateChanged();
  return false;
}

// ── Drill gizmo (对齐上游 GLGizmoDrill) ──

float EditorViewModel::drillRadius() const { return m_drillRadius; }
void EditorViewModel::setDrillRadius(float r) { m_drillRadius = r; emit stateChanged(); }
float EditorViewModel::drillDepth() const { return m_drillDepth; }
void EditorViewModel::setDrillDepth(float d) { m_drillDepth = d; emit stateChanged(); }
int EditorViewModel::drillShape() const { return m_drillShape; }
void EditorViewModel::setDrillShape(int s) { m_drillShape = s; emit stateChanged(); }
int EditorViewModel::drillDirection() const { return m_drillDirection; }
void EditorViewModel::setDrillDirection(int d) { m_drillDirection = d; emit stateChanged(); }
bool EditorViewModel::drillOneLayerOnly() const { return m_drillOneLayerOnly; }
void EditorViewModel::setDrillOneLayerOnly(bool v) { m_drillOneLayerOnly = v; emit stateChanged(); }
bool EditorViewModel::drillSelected()
{
  if (!projectService_)
    return false;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0)
    return false;

  // Capture undo snapshot before drilling
  auto *cmd = new DrillCommand(idx, m_drillRadius, m_drillDepth, m_drillShape, m_drillDirection, m_drillOneLayerOnly, projectService_, this);

  // Delegate to real drillObject API (对齐上游 GLGizmoDrill → sla::hollow_mesh_and_drill)
  bool ok = projectService_->drillObject(idx, m_drillRadius, m_drillDepth, m_drillShape, m_drillDirection, m_drillOneLayerOnly);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    refreshMeshCacheAndFitHint();
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  else
  {
    delete cmd;
  }
  return ok;
}

// ── Emboss gizmo (对齐上游 GLGizmoEmboss) ──

QString EditorViewModel::embossText() const { return m_embossText; }
void EditorViewModel::setEmbossText(const QString &t) { m_embossText = t; emit stateChanged(); }
float EditorViewModel::embossHeight() const { return m_embossHeight; }
void EditorViewModel::setEmbossHeight(float h) { m_embossHeight = h; emit stateChanged(); }
float EditorViewModel::embossDepth() const { return m_embossDepth; }
void EditorViewModel::setEmbossDepth(float d) { m_embossDepth = d; emit stateChanged(); }
bool EditorViewModel::embossSelected()
{
  if (!projectService_)
    return false;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || m_embossText.isEmpty())
    return false;

  // Capture undo command before adding emboss volume
  auto *cmd = new AddVolumeCommand(idx, 2 /* emboss */, m_embossText, projectService_, this);

  // Delegate to the real addTextVolume API (对齐上游 GLGizmoEmboss → Emboss::text2shapes)
  bool ok = projectService_->addTextVolume(idx, m_embossText);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  }
  else
  {
    delete cmd;
  }
  return ok;
}

// ── MeshBoolean gizmo (对齐上游 GLGizmoMeshBoolean) ──

int EditorViewModel::booleanOperation() const { return m_booleanOperation; }
void EditorViewModel::setBooleanOperation(int op) { m_booleanOperation = op; emit stateChanged(); }
bool EditorViewModel::booleanExecute()
{
  if (!projectService_)
    return false;

  // Need exactly 2 selected objects: primary = source (A), secondary = tool (B)
  // (对齐上游: select source volume, then tool volume)
  if (m_selectedSourceIndices.size() != 2)
  {
    statusText_ = QStringLiteral("需选中 2 个对象");
    emit stateChanged();
    return false;
  }

  // Primary selected = source, other = tool
  int srcIndex = m_primarySelectedSourceIndex;
  int toolIndex = -1;
  for (int idx : m_selectedSourceIndices)
  {
    if (idx != srcIndex)
    {
      toolIndex = idx;
      break;
    }
  }
  if (srcIndex < 0 || toolIndex < 0)
    return false;

  // Capture undo snapshot before boolean operation
  auto *cmd = new BooleanCommand(srcIndex, toolIndex, m_booleanOperation, projectService_, this);

  // Delegate to real MeshBoolean API (对齐上游 Slic3r::MeshBoolean::cgal)
  bool ok = projectService_->meshBoolean(srcIndex, toolIndex, m_booleanOperation);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    // Source object was updated and tool was deleted
    // Update srcIndex if toolIndex < srcIndex (tool removed before source, shifting source index down)
    if (toolIndex < srcIndex)
      --srcIndex;
    m_selectedSourceIndices = {srcIndex};
    m_primarySelectedSourceIndex = srcIndex;
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    statusText_ = QStringLiteral("布尔运算完成");
    emit stateChanged();
  }
  else
  {
    delete cmd;
    statusText_ = QStringLiteral("布尔运算失败");
    emit stateChanged();
  }
  return ok;
}

// ── AdvancedCut gizmo (对齐上游 GLGizmoAdvancedCut) ──

int EditorViewModel::advCutAxis() const { return m_advCutAxis; }
void EditorViewModel::setAdvCutAxis(int a) { m_advCutAxis = a; emit stateChanged(); }
float EditorViewModel::advCutPosition() const { return m_advCutPosition; }
void EditorViewModel::setAdvCutPosition(float p) { m_advCutPosition = p; emit stateChanged(); }
bool EditorViewModel::advCutKeepBoth() const { return m_advCutKeepBoth; }
void EditorViewModel::setAdvCutKeepBoth(bool v) { m_advCutKeepBoth = v; emit stateChanged(); }
bool EditorViewModel::advCutConnectors() const { return m_advCutConnectors; }
void EditorViewModel::setAdvCutConnectors(bool v) { m_advCutConnectors = v; emit stateChanged(); }
bool EditorViewModel::advCutSelected()
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
    return false;

  if (m_selectedSourceIndices.size() != 1)
  {
    statusText_ = tr("切割操作仅支持选中单个对象");
    emit stateChanged();
    return false;
  }

  const int srcIdx = *m_selectedSourceIndices.constBegin();
  const int keepMode = m_advCutKeepBoth ? 0 : 1; // 0=all, 1=upper only

  statusText_ = tr("正在执行切割...");
  emit stateChanged();

#ifdef HAS_LIBSLIC3R
  int cutNewIdx = -1;

  if (m_cutMode == 1)
  {
    // Tongue and Groove mode (对齐上游 CutMode::cutTongueAndGroove)
    // Map connector parameters to groove struct
    // groove.depth = connectorSize (mm), groove.width = connectorSize * 4 (upstream default ratio)
    // groove.flaps_angle = PI/3 (upstream default), groove.angle = 0 (upstream default)
    const float grooveDepth = m_connectorSize;                           // depth in mm
    const float grooveWidth = m_connectorSize * 4.0f;                   // width = 4x depth (upstream default)
    const float grooveFlapsAngle = float(M_PI) / 3.0f;                 // 60 degrees (upstream default)
    const float grooveAngle = 0.0f;                                     // 0 degrees (upstream default)

    cutNewIdx = projectService_->cutObjectWithGroove(
        srcIdx, m_advCutAxis, m_advCutPosition, keepMode,
        grooveDepth, grooveWidth, grooveFlapsAngle, grooveAngle);
  }
  else
  {
    // Planar cut mode (对齐上游 CutMode::cutPlanar)
    cutNewIdx = projectService_->cutObject(srcIdx, m_advCutAxis, m_advCutPosition, keepMode);
  }

  if (cutNewIdx >= 0)
  {
    projectService_->syncTransformsFromModel();
    m_selectedSourceIndices.clear();
    if (keepMode == 0) // Keep both: select both parts
    {
      m_selectedSourceIndices.insert(srcIdx);
      if (cutNewIdx != srcIdx)
        m_selectedSourceIndices.insert(cutNewIdx);
    }
    else
    {
      m_selectedSourceIndices.insert(srcIdx);
    }
    statusText_ = tr("已切割对象");
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
    return true;
  }
#endif

  statusText_ = tr("切割失败");
  emit stateChanged();
  return false;
}

// ── FaceDetector gizmo (对齐上游 GLGizmoFaceDetector) ──

float EditorViewModel::faceDetectorAngle() const { return m_faceDetectorAngle; }
void EditorViewModel::setFaceDetectorAngle(float a) { m_faceDetectorAngle = a; emit stateChanged(); }
bool EditorViewModel::detectFlatFaces() { qWarning() << "detectFlatFaces: mock"; return false; }

// ── Text gizmo (对齐上游 GLGizmoText) ──

QString EditorViewModel::textContent() const { return m_textContent; }
void EditorViewModel::setTextContent(const QString &t) { m_textContent = t; emit stateChanged(); }
float EditorViewModel::textSize() const { return m_textSize; }
void EditorViewModel::setTextSize(float s) { m_textSize = s; emit stateChanged(); }
bool EditorViewModel::addTextObject()
{
  if (!projectService_)
    return false;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || m_textContent.isEmpty())
    return false;

  // Capture undo command before adding text volume
  auto *cmd = new AddVolumeCommand(idx, 0 /* text */, m_textContent, projectService_, this);

  // Delegate to the real addTextVolume API (对齐上游 GLGizmoText → Emboss::text2shapes)
  bool ok = projectService_->addTextVolume(idx, m_textContent);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  }
  else
  {
    delete cmd;
  }
  return ok;
}

// ── SVG gizmo (对齐上游 GLGizmoSVG) ──

QString EditorViewModel::svgFilePath() const { return m_svgFilePath; }
void EditorViewModel::setSvgFilePath(const QString &p) { m_svgFilePath = p; emit stateChanged(); }
float EditorViewModel::svgScale() const { return m_svgScale; }
void EditorViewModel::setSvgScale(float s) { m_svgScale = s; emit stateChanged(); }
bool EditorViewModel::importSVG()
{
  if (!projectService_)
    return false;
  const int idx = selectedSourceObjectIndex();
  if (idx < 0 || m_svgFilePath.isEmpty())
    return false;

  // Capture undo command before adding SVG volume
  auto *cmd = new AddVolumeCommand(idx, 1 /* svg */, m_svgFilePath, projectService_, this);

  // Delegate to the real addSvgVolume API (对齐上游 GLGizmoSVG → Model::read_from_file)
  bool ok = projectService_->addSvgVolume(idx, m_svgFilePath);
  if (ok)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  }
  else
  {
    delete cmd;
  }
  return ok;
}

void EditorViewModel::flipCutPlane()
{
  // 翻转切割位置到对称侧 (对齐上游 GLGizmoCut::flip_cut_plane)
  m_cutPosition = -m_cutPosition;
  emit stateChanged();
}

void EditorViewModel::centerCutPlane()
{
  // 将切割位置居中到选中对象中心 (对齐上游 cut plane reset)
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_)
    return;
  const QVector3D pos = projectService_->objectPosition(idx);
  m_cutPosition = (m_cutAxis == 0) ? pos.x() : (m_cutAxis == 1) ? pos.y()
                                                                : pos.z();
  emit stateChanged();
}

void EditorViewModel::flattenSelected()
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
    return;

  // 对齐上游 GLGizmoFlatten::set_flattening_data + on_mouse
  // Flatten 将选中对象旋转使最大面朝下 (Z 平放)
  // Mock 模式：模拟凸包面计算，重置旋转使最大面法线朝 -Z
  statusText_ = tr("正在计算最佳平放面...");
  emit stateChanged();

#ifdef HAS_LIBSLIC3R
  // TODO: 调用 orientation::orient() 或凸包面选择逻辑
  // 1. 计算凸包面及其法线
  // 2. 选择面积最大的面
  // 3. 计算将该面法线旋转至 -Z 的四元数
  // 4. 应用旋转到 ModelObject
#endif

  // Mock mode: 模拟凸包面检测，设置 6 个候选面
  m_flattenFaceCount = 6;

  QTimer::singleShot(400, this, [this]()
                     {
    // Mock: 重置旋转到使 Z 面朝下
    const int idx = *m_selectedSourceIndices.constBegin();
    if (projectService_)
      projectService_->setObjectRotation(idx, 0, 0, 0);
    statusText_ = tr("已平放至最大面（Mock，6 个候选面）");
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged(); });
}

void EditorViewModel::cutSelected(int axis, double position)
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
    return;

  // 对齐上游 GLGizmoCut::perform_cut()
  // 仅支持单对象切割
  if (m_selectedSourceIndices.size() != 1)
  {
    statusText_ = tr("切割操作仅支持选中单个对象");
    emit stateChanged();
    return;
  }

  const int srcIdx = *m_selectedSourceIndices.constBegin();
  const QString origName = (srcIdx >= 0 && srcIdx < m_objects.size())
                               ? m_objects[srcIdx].name
                               : tr("对象");
  statusText_ = tr("正在切割对象...");
  emit stateChanged();

  // Capture undo command before cut (captures source mesh snapshot and object count)
  auto *cmd = new CutCommand(srcIdx, axis, position, m_cutKeepMode, projectService_, this);

#ifdef HAS_LIBSLIC3R
  // 真实网格切割（对齐上游 GLGizmoCut::perform_cut → cut_mesh）
  {
    const int cutNewIdx = projectService_->cutObject(srcIdx, axis, position, m_cutKeepMode);
    if (cutNewIdx >= 0)
    {
      // Record result for redo
      const QStringList namesAfter = projectService_->objectNames();
      cmd->setResult(cutNewIdx, namesAfter.value(cutNewIdx));

      if (m_undoManager)
        m_undoManager->push(cmd);

      projectService_->syncTransformsFromModel();
      m_selectedSourceIndices.clear();
      if (m_cutKeepMode != 2) // Keep upper: select both parts
      {
        m_selectedSourceIndices.insert(srcIdx);
        m_selectedSourceIndices.insert(cutNewIdx);
      }
      else // Keep lower only
      {
        m_selectedSourceIndices.insert(srcIdx);
      }
      statusText_ = tr("已切割对象");
      rebuildObjectEntriesFromService();
      refreshMeshCacheAndFitHint();
      emit stateChanged();
      return;
    }
  }
#endif

  // Mock mode: 将对象沿指定轴切割为上下两部分
  const int newIdx = projectService_->addObject(origName + tr("_upper"));
  if (newIdx >= 0)
  {
    if (m_undoManager)
      m_undoManager->push(cmd);

    // Mock: 缩放上半部分和下半部分
    projectService_->setObjectPosition(newIdx,
                                       projectService_->objectPosition(srcIdx).x() + 2,
                                       projectService_->objectPosition(srcIdx).y(),
                                       projectService_->objectPosition(srcIdx).z() + (axis == 2 ? 5 : 0));

    if (m_cutKeepMode != 2) // 保留上半
    {
      m_selectedSourceIndices.clear();
      m_selectedSourceIndices.insert(srcIdx);
      m_selectedSourceIndices.insert(newIdx);
    }
    else // 仅保留下半
    {
      m_selectedSourceIndices.clear();
      m_selectedSourceIndices.insert(srcIdx);
    }
    statusText_ = tr("已切割为 2 个部分（Mock）");
  }
  else
  {
    delete cmd;
    statusText_ = tr("切割失败");
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  emit stateChanged();
}

void EditorViewModel::rebuildObjectEntriesFromService()
{
  m_objects.clear();
  if (!projectService_)
    return;

  const QStringList names = projectService_->objectNames();
  for (int i = 0; i < names.size(); ++i)
    m_objects.append({names[i], projectService_->objectPrintable(i)});
}

void EditorViewModel::ensureValidObjectSelection(bool preferFirstVisible)
{
  const QList<int> visibleIndices = visibleObjectIndices();
  QSet<int> visibleSet;
  visibleSet.reserve(visibleIndices.size());
  for (int sourceIndex : visibleIndices)
    visibleSet.insert(sourceIndex);

  QSet<int> filteredSelection;
  filteredSelection.reserve(m_selectedSourceIndices.size());
  for (int sourceIndex : m_selectedSourceIndices)
  {
    if (visibleSet.contains(sourceIndex))
      filteredSelection.insert(sourceIndex);
  }
  m_selectedSourceIndices = filteredSelection;

  if (visibleIndices.isEmpty())
  {
    m_selectedSourceIndices.clear();
    m_primarySelectedSourceIndex = -1;
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
    return;
  }

  if (m_selectedSourceIndices.isEmpty())
  {
    if (preferFirstVisible)
    {
      m_primarySelectedSourceIndex = visibleIndices.front();
      m_selectedSourceIndices.insert(m_primarySelectedSourceIndex);
    }
    else
    {
      m_primarySelectedSourceIndex = -1;
    }
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
    return;
  }

  if (m_selectedSourceIndices.contains(m_primarySelectedSourceIndex))
  {
    if (!m_selectedSourceIndices.contains(m_selectedVolumeObjectSourceIndex))
    {
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndices.clear();
      m_selectedVolumeIndex = -1;
    }
    return;
  }

  for (int sourceIndex : visibleIndices)
  {
    if (m_selectedSourceIndices.contains(sourceIndex))
    {
      m_primarySelectedSourceIndex = sourceIndex;
      if (!m_selectedSourceIndices.contains(m_selectedVolumeObjectSourceIndex))
      {
        m_selectedVolumeObjectSourceIndex = -1;
        m_selectedVolumeIndices.clear();
        m_selectedVolumeIndex = -1;
      }
      return;
    }
  }

  m_primarySelectedSourceIndex = preferFirstVisible ? visibleIndices.front() : -1;
  if (preferFirstVisible && m_primarySelectedSourceIndex >= 0)
    m_selectedSourceIndices.insert(m_primarySelectedSourceIndex);
  if (!m_selectedSourceIndices.contains(m_selectedVolumeObjectSourceIndex))
  {
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
  }
}

QList<int> EditorViewModel::baseVisibleObjectIndices() const
{
  if (!projectService_)
    return {};

  if (m_showAllObjects)
  {
    QList<int> all;
    all.reserve(m_objects.size());
    for (int i = 0; i < m_objects.size(); ++i)
      all.append(i);
    return all;
  }

  const QList<int> plateIndices = projectService_->currentPlateObjectIndices();
  if (!plateIndices.isEmpty())
    return plateIndices;

  QList<int> all;
  all.reserve(m_objects.size());
  for (int i = 0; i < m_objects.size(); ++i)
    all.append(i);
  return all;
}

QList<int> EditorViewModel::visibleObjectIndices() const
{
  const QList<int> baseIndices = baseVisibleObjectIndices();
  if (baseIndices.isEmpty() || m_collapsedGroupKeys.isEmpty())
    return baseIndices;

  QList<int> filtered;
  filtered.reserve(baseIndices.size());
  QSet<QString> emittedCollapsedGroups;
  for (int sourceIndex : baseIndices)
  {
    const QString groupKey = sourceObjectGroupKey(sourceIndex);
    if (!m_collapsedGroupKeys.contains(groupKey))
    {
      filtered.append(sourceIndex);
      continue;
    }

    if (!emittedCollapsedGroups.contains(groupKey))
    {
      filtered.append(sourceIndex);
      emittedCollapsedGroups.insert(groupKey);
    }
  }
  return filtered;
}

bool EditorViewModel::currentPlateHasPrintableObjects() const
{
  if (!projectService_)
    return false;

  const QList<int> objectIndices = projectService_->currentPlateObjectIndices();
  for (int objectIndex : objectIndices)
  {
    if (projectService_->objectPrintable(objectIndex))
      return true;
  }

  return false;
}

bool EditorViewModel::hasValidSliceResultForPlate(int plateIndex) const
{
  if (!sliceService_ || !projectService_ || plateIndex < 0 || sliceService_->slicing())
    return false;
  return m_slicedPlateIndices.contains(plateIndex)
      && sliceService_->hasPlateResult(plateIndex)
      && !m_stalePlateIndices.contains(plateIndex);
}

int EditorViewModel::mapFilteredToSourceIndex(int filteredIndex) const
{
  const QList<int> indices = visibleObjectIndices();
  if (filteredIndex < 0 || filteredIndex >= indices.size())
    return -1;
  return indices[filteredIndex];
}

// ── Undo/Redo integration (对齐上游 UndoRedo 框架) ──────────────────────────

void EditorViewModel::setConfigViewModel(ConfigViewModel *vm)
{
  configViewModel_ = vm;
}

void EditorViewModel::setUndoRedoManager(UndoRedoManager *manager)
{
  if (m_undoManager == manager)
    return;
  // Disconnect from old manager
  if (m_undoManager)
    disconnect(m_undoManager, nullptr, this, nullptr);
  m_undoManager = manager;
  // Forward stateChanged from UndoRedoManager so QML properties update
  if (m_undoManager)
    connect(m_undoManager, &UndoRedoManager::stateChanged, this, &EditorViewModel::stateChanged);
}

void EditorViewModel::setActiveCanvasType(int type)
{
  // Phase 90 (ASMROUTE-01): active canvas type set by BackendContext on
  // view-mode change. Mirrors upstream canvas-type routing
  // (GLCanvas3D.hpp:509-513; Plater.cpp:7322,11601,11635). The
  // availableGizmoMask() AssembleView branch reads m_activeCanvasType.
  if (m_activeCanvasType == type)
    return;
  m_activeCanvasType = type;
  // Phase 93 (ASMROUTE-02): refresh the AssembleView data pool ONLY on the
  // AssembleView canvas. Mirrors upstream GLGizmosManager.cpp:427-431 where
  // update_data() runs the pool update with the ModelObjectsInfo bitmask on
  // AssembleView and with None otherwise (releasing all resources).
  // Prepare/Preview never populate or read the pool (isolation constraint).
  if (m_activeCanvasType == 2)
  {
    refreshAssembleViewDataPool();
    m_assembleViewDataPool.update(AssembleViewDataID::ModelObjectsInfo);
  }
  else
  {
    m_assembleViewDataPool.update(AssembleViewDataID::None);
  }
  emit stateChanged();
}

void EditorViewModel::setExplosionRatio(float ratio)
{
  // Phase 91 (ASMEXPLODE-01): mirror upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596, default 1.0). NOTIFY stateChanged drives the QML
  // slider readout AND the RhiViewport explosionRatio re-render (the renderer
  // copies m_explosionRatio in synchronize() and re-uploads with the new
  // per-volume offset on every change).
  if (qFuzzyCompare(m_explosionRatio, ratio) || !std::isfinite(ratio))
    return;
  m_explosionRatio = ratio;
  emit stateChanged();
}

void EditorViewModel::resetExplosionRatio()
{
  // Phase 91 (ASMEXPLODE-01): mirror upstream reset_explosion_ratio()
  // (GLCanvas3D.hpp:770-771). No-op when already at default 1.0.
  if (qFuzzyCompare(m_explosionRatio, 1.0f))
    return;
  m_explosionRatio = 1.0f;
  emit stateChanged();
}

bool EditorViewModel::isAssemblyMeasureActivable() const
{
  // Phase 92 (ASMMEASURE-01): Assembly measurement gizmo activability. Mirrors
  // upstream GLGizmoAssembly::on_is_activable()
  // (third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.cpp:53-68):
  //   abs(get_explosion_ratio() - 1.0f) < 1e-2 && selection.volumes_count() >= 2
  // The gizmo is only meaningful on AssembleView (m_activeCanvasType == 2).
  // m_selectedSourceIndices is the per-object selection set; the renderer
  // treats each source object's first volume batch as the measured AABB.
  return m_activeCanvasType == 2
      && std::abs(m_explosionRatio - 1.0f) < 1e-2f
      && m_selectedSourceIndices.size() >= 2;
}

QList<PrepareSceneData::ModelBounds>
EditorViewModel::selectedVolumeBoundsForAssemblyMeasure() const
{
  // Phase 92 (ASMMEASURE-02) + Phase 93 (ASMROUTE-02): per-volume AABBs for the
  // first two selected source indices. Phase 93 routes the bounds through the
  // cached AssembleViewDataPool ModelObjectsInfo resource when on AssembleView
  // + the pool is valid (the bounds source is unchanged — same mesh blob — now
  // formalized as the cached pool resource). When not on AssembleView the pool
  // is released and this returns empty (Phase 92 behavior) — Prepare/Preview
  // never call it.
  const QList<int> selected = assemblyMeasureSelectedSourceIndices();
  if (selected.size() < 2)
    return {};

  // Phase 93 (ASMROUTE-02): prefer the cached pool resource when valid. The
  // bounds values are byte-identical to the inline parse (same source) — this
  // is a pure refactor that formalizes the per-object data the view needs.
  if (const AssembleViewModelObjectsInfo *info = m_assembleViewDataPool.model_objects_info())
  {
    const QList<AssembleViewObjectInfo> &objects = info->objects();
    const int targetA = selected.at(0);
    const int targetB = selected.at(1);
    const AssembleViewObjectInfo *a = nullptr;
    const AssembleViewObjectInfo *b = nullptr;
    for (const AssembleViewObjectInfo &o : objects)
    {
      if (o.sourceObjectIndex == targetA) a = &o;
      else if (o.sourceObjectIndex == targetB) b = &o;
    }
    if (a && b)
      return {a->bounds, b->bounds};
    return {};
  }

  // Fallback: inline parse (kept for the rare case where the pool is not yet
  // populated on AssembleView, e.g. before the first refresh). Shares the same
  // parseAllSourceObjectBoundsFromMeshBlob helper as refreshAssembleViewDataPool.
  const QMap<int, PrepareSceneData::ModelBounds> all =
      parseAllSourceObjectBoundsFromMeshBlob(m_cachedMeshData,
                                             m_cachedMeshBatchSourceObjectIndices);
  if (all.isEmpty())
    return {};
  const int targetA = selected.at(0);
  const int targetB = selected.at(1);
  auto itA = all.constFind(targetA);
  auto itB = all.constFind(targetB);
  if (itA == all.constEnd() || itB == all.constEnd())
    return {};
  return {itA.value(), itB.value()};
}

void EditorViewModel::refreshAssembleViewDataPool()
{
  // Phase 93 (ASMROUTE-02): populate the pool's ModelObjectsInfo resource from
  // the same bounds source Phase 92 used (the cached mesh blob + the
  // per-batch source-object index map). One AssembleViewObjectInfo entry per
  // source object (bounds unioned across its volume batches). Called only when
  // m_activeCanvasType == 2 (setActiveCanvasType / refreshMeshCacheAndFitHint
  // guard the entry points) — Prepare/Preview never reach here.
  const QMap<int, PrepareSceneData::ModelBounds> all =
      parseAllSourceObjectBoundsFromMeshBlob(m_cachedMeshData,
                                             m_cachedMeshBatchSourceObjectIndices);
  QList<AssembleViewObjectInfo> objects;
  objects.reserve(all.size());
  for (auto it = all.constBegin(); it != all.constEnd(); ++it)
  {
    AssembleViewObjectInfo info;
    info.sourceObjectIndex = it.key();
    info.bounds = it.value();
    objects.append(info);
  }
  // The minimal-port seam: pre-fill the resource before the pool's update()
  // marks it valid (AssembleViewModelObjectsInfo::on_update is a no-op).
  m_assembleViewDataPool.model_objects_info_for_refresh()->setObjects(std::move(objects));
}

int EditorViewModel::assembleViewDataPoolObjectCountForTest() const
{
  // Phase 93 (ASMROUTE-02): test accessor. Returns 0 when the pool's
  // ModelObjectsInfo is not valid — which is itself the isolation assertion
  // (Prepare/Preview leave the pool released).
  if (const AssembleViewModelObjectsInfo *info = m_assembleViewDataPool.model_objects_info())
    return info->objects().size();
  return 0;
}

bool EditorViewModel::activateAssemblyMeasureGizmo()
{
  // Phase 92 (ASMMEASURE-01): mirror upstream GLGizmoAssembly activation
  // (Ctrl+Y, ONLY_ASSEMBLY mode; GLGizmoAssembly.cpp:45-51). Guarded by the
  // activability rule so a stale Ctrl+Y (e.g. while exploded or <2 volumes)
  // is a no-op. Returns success so QML can branch on the result.
  if (!isAssemblyMeasureActivable())
    return false;
  if (m_assemblyMeasureGizmoActive)
    return true;
  m_assemblyMeasureGizmoActive = true;
  emit stateChanged();
  return true;
}

void EditorViewModel::deactivateAssemblyMeasureGizmo()
{
  // Phase 92 (ASMMEASURE-01): mirror upstream GLGizmoBase deactivation.
  if (!m_assemblyMeasureGizmoActive)
    return;
  m_assemblyMeasureGizmoActive = false;
  emit stateChanged();
}

QList<int> EditorViewModel::assemblyMeasureSelectedSourceIndices() const
{
  // Phase 92 (ASMMEASURE-02): the first two selected source indices, fed to the
  // RhiViewport overlay (assemblyMeasureSelectedA/B) so the renderer knows which
  // two volumes to annotate. Order follows the QSet iteration order; the
  // measurement is symmetric so order does not affect the distance/angle.
  QList<int> result;
  result.reserve(2);
  for (int idx : m_selectedSourceIndices)
  {
    if (result.size() >= 2)
      break;
    result.append(idx);
  }
  return result;
}

QString EditorViewModel::assemblyMeasureDistanceText() const
{
  // Phase 92 (ASMMEASURE-02): the center-to-center distance between the first
  // two selected volumes, formatted to upstream precision (3 decimals + mm,
  // GLGizmoMeasure.cpp:24,1250). Documented simplification: AABB-center
  // distance, not picked-feature distance (full feature-picking is future).
  if (!m_assemblyMeasureGizmoActive || m_selectedSourceIndices.size() < 2)
    return {};
  const QList<PrepareSceneData::ModelBounds> bounds = selectedVolumeBoundsForAssemblyMeasure();
  if (bounds.size() < 2)
    return {};
  const AssemblyMeasureResult r = AssemblyMeasureGeometry::measure(bounds.at(0), bounds.at(1));
  if (!r.valid)
    return {};
  return AssemblyMeasureGeometry::formatDistance(r.distance);
}

QString EditorViewModel::assemblyMeasureAngleText() const
{
  // Phase 92 (ASMMEASURE-02): the angle between the two selected volumes'
  // longest-AABB-axis directions, formatted as degrees (3 decimals + degree
  // glyph, GLGizmoMeasure.cpp:1558). Documented simplification: longest-axis
  // direction stands in for the upstream edge/plane feature direction.
  if (!m_assemblyMeasureGizmoActive || m_selectedSourceIndices.size() < 2)
    return {};
  const QList<PrepareSceneData::ModelBounds> bounds = selectedVolumeBoundsForAssemblyMeasure();
  if (bounds.size() < 2)
    return {};
  const AssemblyMeasureResult r = AssemblyMeasureGeometry::measure(bounds.at(0), bounds.at(1));
  if (!r.valid)
    return {};
  return AssemblyMeasureGeometry::formatAngle(r.angleDeg);
}

QVector3D EditorViewModel::assemblyMeasureDistanceXyz() const
{
  // Phase 92 (ASMMEASURE-02): the per-axis XYZ delta between the two selected
  // volumes' AABB centers (mm). Distance-XYZ editing (moving volumes) is a
  // deferred future enhancement (mutates the model).
  if (!m_assemblyMeasureGizmoActive || m_selectedSourceIndices.size() < 2)
    return {};
  const QList<PrepareSceneData::ModelBounds> bounds = selectedVolumeBoundsForAssemblyMeasure();
  if (bounds.size() < 2)
    return {};
  const AssemblyMeasureResult r = AssemblyMeasureGeometry::measure(bounds.at(0), bounds.at(1));
  if (!r.valid)
    return {};
  return r.distanceXyz;
}

QString EditorViewModel::assemblyMeasurePlaneText() const
{
  // Phase 92 (ASMMEASURE-02): plane-selection indicator mirroring 装配页_测量.png
  // ("选中 N 平面"). Phase 92 approximates "plane" by the count of selected
  // volumes (each volume contributes one longest-AABB-axis plane); full
  // per-triangle plane picking is deferred (needs ITS + raycaster).
  if (!m_assemblyMeasureGizmoActive || m_selectedSourceIndices.isEmpty())
    return {};
  // "选中 N 平面" — emit as \uXXXX escapes (ASCII-only source per AGENTS.md).
  return QString::fromUtf8(u8"\u9009\u4e2d ") + QString::number(m_selectedSourceIndices.size())
       + QString::fromUtf8(u8" \u5e73\u9762");
}

// Phase 114 (MEASURE-03): real feature-picking measurement readout text
// getters (ME-04). Each formats to upstream precision (3 decimals + unit/
// glyph, mirroring GLGizmoMeasure.cpp:24 format_double + the
// ClipboardAngle/ClipboardDistance* rows at GLGizmoMeasure.cpp:1996,2009,
// 2022,2031). Empty when no valid readout is cached (the valid gate is
// measureReadoutValid()).
QString EditorViewModel::measureAngleText() const
{
  if (!m_measureReadout.valid || !m_measureReadout.hasAngle)
    return {};
  // Upstream format_double(rad2deg(angle)) + degree glyph
  // (GLGizmoMeasure.cpp:1996).
  return QString::number(m_measureReadout.angleDeg, 'f', 3) +
         QString::fromUtf8(u8"\u00b0");
}

QString EditorViewModel::measurePerpendicularDistanceText() const
{
  if (!m_measureReadout.valid || !m_measureReadout.hasPerpendicularDistance)
    return {};
  // Upstream "Perpendicular distance" row (distance_infinite,
  // GLGizmoMeasure.cpp:2009): value + " mm".
  return QString::number(m_measureReadout.perpendicularDistance, 'f', 3) +
         QStringLiteral(" mm");
}

QString EditorViewModel::measureDirectDistanceText() const
{
  if (!m_measureReadout.valid || !m_measureReadout.hasDirectDistance)
    return {};
  // Upstream "Direct distance" row (distance_strict,
  // GLGizmoMeasure.cpp:2022): value + " mm".
  return QString::number(m_measureReadout.directDistance, 'f', 3) +
         QStringLiteral(" mm");
}

QString EditorViewModel::measureDistanceXyzText() const
{
  if (!m_measureReadout.valid || !m_measureReadout.hasDistanceXyz)
    return {};
  // Upstream "Distance XYZ" row (GLGizmoMeasure.cpp:2031 format_vec3):
  // "(x, y, z)". Skip when the vector is ~zero (mirrors the upstream
  // norm()>EPSILON gate at GLGizmoMeasure.cpp:2029).
  const QVector3D &v = m_measureReadout.distanceXyz;
  if (v.lengthSquared() < 1e-12f)
    return {};
  return QStringLiteral("(%1, %2, %3)")
      .arg(v.x(), 0, 'f', 3)
      .arg(v.y(), 0, 'f', 3)
      .arg(v.z(), 0, 'f', 3);
}

int EditorViewModel::measureEngineCachedCount() const
{
#ifdef HAS_LIBSLIC3R
  return m_measureEngine ? int(m_measureEngine->cachedMeasuringCount()) : 0;
#else
  return 0;
#endif
}

void EditorViewModel::clearMeasureReadout()
{
  // Phase 114 (MEASURE-03): reset the cached readout to the invalid defaults
  // + drop the "from" feature so the next hit starts a fresh two-click
  // measure. Always emits measureReadoutChanged so the QML bindings refresh
  // (mirrors the WTREAD-02 / FMAP-01 always-emit pattern).
  m_measureReadout = MeasureReadout{};
#ifdef HAS_LIBSLIC3R
  m_measureFromFeatureValid = false;
  m_measureFromObjectIndex = -1;
  m_measureFromVolumeIndex = -1;
  m_measureFromFacetIdx = -1;
  m_measureFromWorldPoint = {};
  m_measureFromVolumeTranslation = {};
  m_measureFromVolumeRotationRad = {};
  m_measureFromVolumeScale = QVector3D(1.0f, 1.0f, 1.0f);
#endif
  // Phase 115 (MEASURE-04): clear the hovered-feature highlight so no stale
  // overlay marker lingers on cursor-leave / gizmo-deactivate.
  m_measureHoverFeatureKind = 0;
  m_measureHoverWorldPosition = {};
  emit measureReadoutChanged();
}

void EditorViewModel::invalidateMeasureEngine()
{
  // Phase 114 (MEASURE-03): drop the per-volume Measuring cache on mesh
  // change (load/cut/boolean/simplify/drill). Mirrors the upstream rebuild-
  // on-mesh-change signal (GLGizmoMeasure.cpp m_curr_measuring rebuild,
  // pitfall 6). Without this, a stale Measuring serves features from the
  // OLD mesh. No-op when HAS_LIBSLIC3R is off or the engine was never built.
#ifdef HAS_LIBSLIC3R
  if (m_measureEngine)
    m_measureEngine->invalidate();
  // Phase 115 (MEASURE-04): drop the stage-2 raycaster cache too -- a stale
  // MeshRaycaster BVH would serve hits from the OLD mesh. Same model-change
  // signal as MeasureEngine (pitfall 6 rebuild contract).
  if (m_sceneRaycaster)
    m_sceneRaycaster->invalidate();
#endif
}

#ifdef HAS_LIBSLIC3R
namespace {
// Rebuild a Slic3r::Transform3d from the QML-facing translation/rotation/
// scale decomposition. Eigen types cannot be Q_PROPERTY members or cross
// QML, so the renderer decomposes the candidate worldTransform it already
// holds and the C++ side rebuilds it here. Uses the canonical
// Geometry::assemble_transform (the SAME helper ModelInstance::get_matrix
// uses, Geometry.hpp:354) so the composition matches the rest of the
// codebase byte-for-byte: T = translation * RotZ * RotY * RotX * Scale.
Slic3r::Transform3d rebuildWorldTransform(QVector3D translation,
                                          QVector3D rotationRad,
                                          QVector3D scale)
{
  return Slic3r::Geometry::assemble_transform(
      Slic3r::Vec3d(double(translation.x()), double(translation.y()),
                    double(translation.z())),
      Slic3r::Vec3d(double(rotationRad.x()), double(rotationRad.y()),
                    double(rotationRad.z())),
      Slic3r::Vec3d(double(scale.x()), double(scale.y()),
                    double(scale.z())));
}
} // namespace

bool EditorViewModel::computeMeasureReadoutFromHit(int objectIndex,
                                                   int volumeIndex,
                                                   int facetIdx,
                                                   QVector3D worldPoint,
                                                   QVector3D volumeTranslation,
                                                   QVector3D volumeRotationRad,
                                                   QVector3D volumeScale,
                                                   bool onlySelectPlane)
{
  // Phase 114 (MEASURE-03): drive a measurement readout from a Phase 113
  // SceneRaycaster hit. This is the production wiring of MeasureEngine to
  // the QML layer: the renderer (RhiViewportRenderer) holds the
  // SceneRaycasterHit + the candidate worldTransform; it decomposes the
  // transform and calls this Q_INVOKABLE. The engine resolves the picked
  // feature (Measure::Measuring on the per-volume ITS, pitfall-6 scrubbed)
  // and, when a "from" feature is already cached, computes the readout
  // between them (Measure::get_measurement). Mirrors the upstream two-click
  // measure flow (GLGizmoMeasure.cpp m_selected_features first/second).
  //
  // ProjectServiceMock is required for the ITS source. Without it (or
  // without HAS_LIBSLIC3R), the readout stays invalid.
  if (!projectService_)
    return false;

  // Lazy-construct the MeasureEngine on first use (ME-01). The ITS source
  // lambda captures projectService_ (non-owning -- EditorViewModel does not
  // own the service). The same accessor feeds the Phase 113 SceneRaycaster
  // in production, keeping per-volume ITS identity consistent.
  if (!m_measureEngine) {
    m_measureEngine = std::make_unique<OWzx::MeasureEngine>(
        [svc = projectService_](int objIdx, int volIdx)
            -> std::shared_ptr<const indexed_triangle_set> {
          return svc ? svc->volumeMeshIts(objIdx, volIdx) : nullptr;
        });
  }

  const Slic3r::Vec3d worldPt(double(worldPoint.x()), double(worldPoint.y()),
                              double(worldPoint.z()));
  const Slic3r::Transform3d worldTran =
      rebuildWorldTransform(volumeTranslation, volumeRotationRad, volumeScale);

  // Resolve the feature at this hit (ME-02). The returned QtFeature is
  // pure POD (pitfall 6 scrubbed) -- safe to hold across calls.
  OWzx::QtFeature feature = m_measureEngine->getFeature(
      objectIndex, volumeIndex, static_cast<std::size_t>(facetIdx), worldPt,
      worldTran, onlySelectPlane);
  if (!feature.valid)
    return false; // ray missed / facet out of range / no mesh

  // Phase 115 (MEASURE-04): update the hover highlight from the resolved
  // feature so the overlay reflects the picked feature type + position
  // (MS-03 visual feedback). This runs on every mouse-move hit (the
  // hover path calls this with the stage-2 hit).
  m_measureHoverFeatureKind = static_cast<int>(feature.kind);
  m_measureHoverWorldPosition = feature.pt1;

  if (!m_measureFromFeatureValid) {
    // First click: stash as the "from" feature, no readout yet.
    m_measureFromFeatureValid = true;
    m_measureFromObjectIndex = objectIndex;
    m_measureFromVolumeIndex = volumeIndex;
    m_measureFromFacetIdx = facetIdx;
    m_measureFromWorldPoint = worldPoint;
    m_measureFromVolumeTranslation = volumeTranslation;
    m_measureFromVolumeRotationRad = volumeRotationRad;
    m_measureFromVolumeScale = volumeScale;
    m_measureReadout = MeasureReadout{};
    emit measureReadoutChanged();
    return false;
  }

  // Second click: re-resolve the "from" feature from the stashed hit fields
  // (we stash the hit, not the feature, so the engine can rebuild the
  // SurfaceFeature on this side of the boundary without holding a libslic3r
  // object). Then measure feature-vs-fromFeature.
  const Slic3r::Vec3d fromWorldPt(
      double(m_measureFromWorldPoint.x()), double(m_measureFromWorldPoint.y()),
      double(m_measureFromWorldPoint.z()));
  const Slic3r::Transform3d fromWorldTran = rebuildWorldTransform(
      m_measureFromVolumeTranslation, m_measureFromVolumeRotationRad,
      m_measureFromVolumeScale);
  OWzx::QtFeature fromFeature = m_measureEngine->getFeature(
      m_measureFromObjectIndex, m_measureFromVolumeIndex,
      static_cast<std::size_t>(m_measureFromFacetIdx), fromWorldPt,
      fromWorldTran, onlySelectPlane);
  if (!fromFeature.valid) {
    // The "from" feature is no longer resolvable (mesh changed without an
    // invalidate). Fall back: this hit becomes the new "from" feature.
    m_measureFromFeatureValid = true;
    m_measureFromObjectIndex = objectIndex;
    m_measureFromVolumeIndex = volumeIndex;
    m_measureFromFacetIdx = facetIdx;
    m_measureFromWorldPoint = worldPoint;
    m_measureFromVolumeTranslation = volumeTranslation;
    m_measureFromVolumeRotationRad = volumeRotationRad;
    m_measureFromVolumeScale = volumeScale;
    m_measureReadout = MeasureReadout{};
    emit measureReadoutChanged();
    return false;
  }

  // Compute the readout in WORLD space. Both features are resolved with
  // their own worldTransform; the measurement uses the SECOND hit's
  // worldTransform as the reference frame (mirrors upstream world_tran on
  // the active feature). The QtMeasurement is POD (pitfall 6 scrubbed).
  const OWzx::QtMeasurement m =
      m_measureEngine->measureFeatures(fromFeature, feature, worldTran);

  MeasureReadout out;
  out.valid = m.valid;
  out.hasAngle = m.hasAngle;
  out.angleDeg = m.hasAngle ? float(m.angleRad * kRadiansToDegrees) : 0.0f;
  out.hasPerpendicularDistance = m.hasPerpendicularDistance;
  out.perpendicularDistance = m.perpendicularDistance;
  out.hasDirectDistance = m.hasDirectDistance;
  out.directDistance = m.directDistance;
  out.hasDistanceXyz = m.hasDistanceXyz;
  out.distanceXyz = m.distanceXyz;
  m_measureReadout = out;
  emit measureReadoutChanged();
  return out.valid;
}
#else
bool EditorViewModel::computeMeasureReadoutFromHit(int, int, int, QVector3D,
                                                   QVector3D, QVector3D,
                                                   QVector3D, bool)
{
  // HAS_LIBSLIC3R off: Measure::Measuring + the ITS accessor do not exist.
  // Return false so the QML caller sees no readout (mirrors the
  // kViewportTrianglePickingAvailable=false stub pattern).
  return false;
}
#endif

#ifdef HAS_LIBSLIC3R
bool EditorViewModel::pickMeasureFeatureAt(QVector3D rayOrigin,
                                           QVector3D rayDirection,
                                           int pickedSourceIndex,
                                           bool shiftHeld)
{
  // Phase 115 (MEASURE-04): the snap UX entry point. Mirrors the upstream
  // GLGizmoMeasure mouse-move/click flow:
  //   1. Stage-1 (RhiViewport::pickSourceObjectAt) already narrowed the scene
  //      to pickedSourceIndex (cheap ray->AABB prefilter).
  //   2. Stage-2 (HERE -- SceneRaycaster::hitTest) runs the per-triangle ITS
  //      raycast over the candidate volume(s) only (pitfall 7 mitigation: NO
  //      whole-scene loop). Returns the closest world-space hit.
  //   3. The hit feeds computeMeasureReadoutFromHit (Phase 114 MeasureEngine::
  //      getFeature) which resolves the SurfaceFeature (Point/Edge/Circle/
  //      Plane) and drives the measure* readouts.
  //   4. shiftHeld implements the Shift toggle (GLGizmoMeasure.cpp:409-442):
  //      true forces EMode::PointSelection -- the raw world hit point is used
  //      as a Point feature (no feature snapping); false keeps the default
  //      FeatureSelection (snap to nearest edge/circle/plane).
  //
  // ProjectServiceMock is required for the ITS source. Without it (or without
  // HAS_LIBSLIC3R), no pick happens.
  if (!projectService_ || pickedSourceIndex < 0)
  {
    m_measureHoverFeatureKind = 0;
    m_measureHoverWorldPosition = {};
    emit measureReadoutChanged();
    return false;
  }

  // Lazy-construct the SceneRaycaster from the SAME ITS source MeasureEngine
  // uses (ProjectServiceMock::volumeMeshIts, Phase 112). Reusing one accessor
  // keeps per-volume ITS identity consistent across the raycaster + engine.
  if (!m_sceneRaycaster) {
    m_sceneRaycaster = std::make_unique<OWzx::SceneRaycaster>(
        [svc = projectService_](int objIdx, int volIdx)
            -> std::shared_ptr<const indexed_triangle_set> {
          return svc ? svc->volumeMeshIts(objIdx, volIdx) : nullptr;
        });
  }

  // Reconstruct the candidate volume's world transform from the object's
  // translation/rotation/scale (Eigen types cannot cross QML). The candidate
  // list is the SINGLE stage-1 survivor -- SceneRaycaster never loops the
  // whole scene. Volume 0 is the primary mesh volume per ProjectServiceMock
  // convention (objectVolumeCount >= 1 for any loaded object).
  const QVector3D translation = projectService_->objectPosition(pickedSourceIndex);
  const QVector3D rotationDeg = projectService_->objectRotation(pickedSourceIndex);
  const QVector3D scale = projectService_->objectScale(pickedSourceIndex);
  const QVector3D rotationRad(float(rotationDeg.x() * float(M_PI) / 180.0f),
                              float(rotationDeg.y() * float(M_PI) / 180.0f),
                              float(rotationDeg.z() * float(M_PI) / 180.0f));
  const Slic3r::Transform3d worldTransform =
      rebuildWorldTransform(translation, rotationRad, scale);

  // Enumerate the candidate volume(s) for the picked object. Most objects
  // have exactly one mesh volume; multi-volume objects add their mesh
  // volumes here. The loop is bounded by objectVolumeCount (typically 1).
  std::vector<OWzx::SceneRaycasterCandidate> candidates;
  const int volumeCount = projectService_->objectVolumeCount(pickedSourceIndex);
  candidates.reserve(std::max(1, volumeCount));
  for (int v = 0; v < std::max(1, volumeCount); ++v) {
    OWzx::SceneRaycasterCandidate cand;
    cand.objectIndex = pickedSourceIndex;
    cand.volumeIndex = v;
    cand.worldTransform = worldTransform;
    candidates.push_back(cand);
  }

  const Slic3r::Vec3d origin(double(rayOrigin.x()), double(rayOrigin.y()),
                             double(rayOrigin.z()));
  const Slic3r::Vec3d dir(double(rayDirection.x()), double(rayDirection.y()),
                          double(rayDirection.z()));
  const OWzx::SceneRaycasterHit hit =
      m_sceneRaycaster->hitTest(origin, dir, candidates);
  if (!hit.hit) {
    // Ray missed every candidate volume: clear the hover highlight so no
    // stale marker follows the cursor off the mesh.
    m_measureHoverFeatureKind = 0;
    m_measureHoverWorldPosition = {};
    emit measureReadoutChanged();
    return false;
  }

  // Drive the existing Phase 114 readout path with the resolved world-space
  // hit. computeMeasureReadoutFromHit stashes the first hit as the "from"
  // feature and measures the second against it (mirrors GLGizmoMeasure
  // two-click flow). It also updates m_measureHoverFeatureKind/Position from
  // the resolved feature (MS-03 visual feedback), so the default
  // FeatureSelection path needs no extra work here.
  const QVector3D worldHit(float(hit.worldPosition.x()),
                           float(hit.worldPosition.y()),
                           float(hit.worldPosition.z()));
  const bool onlySelectPlane = false;  // full feature sniff (Point/Edge/Circle/Plane)
  computeMeasureReadoutFromHit(hit.objectIndex, hit.volumeIndex,
                               int(hit.facetIdx), worldHit,
                               translation, rotationRad, scale,
                               onlySelectPlane);
  if (shiftHeld) {
    // Upstream EMode::PointSelection (GLGizmoMeasure.cpp:413): the raw hit
    // point is the feature, NOT the snapped edge/circle/plane. Override the
    // hover highlight (which computeMeasureReadoutFromHit set from the
    // resolved feature) to a Point at the raw world hit position. The readout
    // math (measureFeatures) still works because Measure::get_measurement
    // accepts any feature pair -- the resolved feature feeds the A->B measure
    // correctly regardless of the highlight override.
    m_measureHoverFeatureKind = static_cast<int>(OWzx::FeatureKind::Point);
    m_measureHoverWorldPosition = worldHit;
    emit measureReadoutChanged();
  }
  return true;
}
#else
bool EditorViewModel::pickMeasureFeatureAt(QVector3D, QVector3D, int, bool)
{
  // HAS_LIBSLIC3R off: SceneRaycaster + MeasureEngine do not exist. No pick.
  return false;
}
#endif

// Phase 114 (MEASURE-03): out-of-line destructor so the unique_ptr<MeasureEngine>
// member (forward-declared in the header) destroys where MeasureEngine is
// complete. Empty body -- unique_ptr + the Qt parent-child ownership handle
// the cleanup. Defined here (not = default in the header) so the compiler
// does NOT instantiate the deleter at the header's point of incomplete type.
EditorViewModel::~EditorViewModel() = default;

void EditorViewModel::undo()
{
  if (m_undoManager)
    m_undoManager->undo();
}

void EditorViewModel::redo()
{
  if (m_undoManager)
    m_undoManager->redo();
}

void EditorViewModel::clearUndoStack()
{
  if (m_undoManager)
    m_undoManager->clear();
}

bool EditorViewModel::canUndo() const
{
  return m_undoManager ? m_undoManager->canUndo() : false;
}

bool EditorViewModel::canRedo() const
{
  return m_undoManager ? m_undoManager->canRedo() : false;
}

void EditorViewModel::restoreSelection(const QSet<int> &sourceIndices, int primaryIndex)
{
  m_selectedSourceIndices = sourceIndices;
  m_primarySelectedSourceIndex = primaryIndex;
  // Clear volume selection when restoring object-level selection
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  ensureValidObjectSelection(false);
  emit stateChanged();
}

void EditorViewModel::rebuildAndNotify()
{
  rebuildObjectEntriesFromService();
  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  invalidateSliceResultsForCurrentPlate();
  emit stateChanged();
}

EditorViewModel::EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent)
    : QObject(parent), projectService_(projectService), sliceService_(sliceService)
{
  connect(projectService_, &ProjectServiceMock::projectChanged, this, [this]()
          {
        statusText_ = QStringLiteral("已更新项目对象");
        emit stateChanged(); });

  connect(projectService_, &ProjectServiceMock::plateSelectionChanged, this, [this]()
          {
        if (sliceService_ && projectService_)
          sliceService_->activatePlateResult(projectService_->currentPlateIndex());
        ensureValidObjectSelection(true);
        emit stateChanged(); });

  connect(sliceService_, &SliceService::slicingChanged, this, [this]()
          {
    if (sliceService_->slicing())
    {
      m_sliceEstimatedTime.clear();
      m_sliceResultPlateIndex = -1;
    }
        statusText_ = sliceService_->slicing() ? QStringLiteral("切片中...") : QStringLiteral("切片完成");
        emit stateChanged(); });

  connect(sliceService_, &SliceService::progressChanged, this, &EditorViewModel::stateChanged);
  connect(sliceService_, &SliceService::progressUpdated, this, [this](int percent, const QString &label)
          {
        statusText_ = QStringLiteral("%1... %2%").arg(label).arg(percent);
        emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFailed, this, [this](const QString &message)
          {
    m_sliceEstimatedTime.clear();
      m_sliceResultPlateIndex = -1;
      m_sliceAllQueue.clear();
      m_slicingAll = false;
        statusText_ = message;
        emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFinished, this, [this](const QString &estimatedTime)
          {
    m_sliceEstimatedTime = estimatedTime;
      m_sliceResultPlateIndex = sliceService_ ? sliceService_->resultPlateIndex() : -1;
    // Track per-plate slice state
    if (m_sliceResultPlateIndex >= 0) {
      m_slicedPlateIndices.insert(m_sliceResultPlateIndex);
      m_stalePlateIndices.remove(m_sliceResultPlateIndex);
      if (projectService_ && projectService_->currentPlateIndex() == m_sliceResultPlateIndex)
        sliceService_->activatePlateResult(m_sliceResultPlateIndex);
    }
    if (m_slicingAll && !m_sliceAllQueue.isEmpty())
    {
      statusText_ = QStringLiteral("切片完成，继续下一平板...");
      emit stateChanged();
      continueSliceAllQueue();
    }
    else if (m_slicingAll)
    {
      m_slicingAll = false;
      statusText_ = QStringLiteral("全部平板切片完成");
      emit stateChanged();
    }
    else
    {
      statusText_ = QStringLiteral("切片完成");
      emit stateChanged();
    } });
  // Phase 100 (WTREAD-01): deliver the captured-by-value wipe-tower geometry
  // (from Print::wipe_tower_data(), read in the SliceService worker) to the
  // six wipeTower* Q_PROPERTYs above. The slot applies the has_wipe_tower()
  // gate (WTREAD-02): single-material slices keep showWipeTower=false and do
  // not push placeholder dims as "real" geometry.
  connect(sliceService_, &SliceService::wipeTowerGeometryReady,
          this, &EditorViewModel::onWipeTowerGeometryReady);
  // Phase 108 (FMAP-01): deliver the captured-by-value filament-map auto-
  // recommendation (from Print::get_filament_maps(), read in the SliceService
  // worker) to the three auto* Q_PROPERTYs above. The slot applies the valid
  // gate (mirrors WTREAD-02): when result.valid is false, hasAutoFilamentMap
  // stays false and no stale map leaks to the Phase 110 UI.
  connect(sliceService_, &SliceService::filamentMapReady,
          this, &EditorViewModel::onFilamentMapReady);
  connect(projectService_, &ProjectServiceMock::loadProgressUpdated, this, [this](int progress, const QString &stageText)
          {
    if (projectService_->loading())
      statusText_ = QStringLiteral("%1... %2%").arg(stageText).arg(progress);
    emit stateChanged(); });
  connect(projectService_, &ProjectServiceMock::loadingChanged, this, [this]()
          {
    if (!projectService_->loading())
      emit stateChanged(); });
  connect(projectService_, &ProjectServiceMock::loadFinished, this, [this](bool success, const QString &message)
          {
    if (success)
    {
    m_collapsedGroupKeys.clear();
    m_collapsedObjectSourceIndices.clear();
      rebuildObjectEntriesFromService();
      ensureValidObjectSelection(true);
      statusText_ = QStringLiteral("已加载 %1 个模型，%2 个平板").arg(m_objects.size()).arg(projectService_->plateCount());
      refreshMeshCacheAndFitHint();
    }
    else
    {
      statusText_ = message;
      m_selectedSourceIndices.clear();
      m_primarySelectedSourceIndex = -1;
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndices.clear();
      m_selectedVolumeIndex = -1;
      m_fitHint = QVector4D();
      m_cachedMeshData.clear();
      m_cachedMeshBatchSourceObjectIndices.clear();
    }
    emit stateChanged(); });

  // Restore bed shape from QSettings (对齐上游 bed_shape persistence)
  {
    QSettings s;
    m_bedWidth = s.value(QStringLiteral("bed/width"), 220.0f).toFloat();
    m_bedDepth = s.value(QStringLiteral("bed/depth"), 220.0f).toFloat();
    m_bedMaxHeight = s.value(QStringLiteral("bed/maxHeight"), 300.0f).toFloat();
    m_bedOriginX = s.value(QStringLiteral("bed/originX"), 0.0f).toFloat();
    m_bedOriginY = s.value(QStringLiteral("bed/originY"), 0.0f).toFloat();
    m_bedShapeType = s.value(QStringLiteral("bed/shapeType"), 0).toInt();
    m_bedDiameter = s.value(QStringLiteral("bed/diameter"), 220.0f).toFloat();
  }
}

QString EditorViewModel::projectName() const { return projectService_ ? projectService_->projectName() : QString(); }
int EditorViewModel::modelCount() const { return projectService_ ? projectService_->modelCount() : 0; }
int EditorViewModel::plateCount() const { return projectService_ ? projectService_->plateCount() : 0; }
int EditorViewModel::maxPlateCount() const { return OWzx::kMaxPlateCount; }
int EditorViewModel::currentPlateIndex() const { return projectService_ ? projectService_->currentPlateIndex() : 0; }
QVariantList EditorViewModel::activePlateObjectIndices() const
{
  QVariantList indices;
  if (!projectService_)
    return indices;

  const QList<int> plateIndices = projectService_->currentPlateObjectIndices();
  indices.reserve(plateIndices.size());
  for (int objectIndex : plateIndices)
    indices.append(objectIndex);
  return indices;
}

QVariantList EditorViewModel::meshBatchSourceObjectIndices() const
{
  QVariantList indices;
  indices.reserve(m_cachedMeshBatchSourceObjectIndices.size());
  for (int objectIndex : m_cachedMeshBatchSourceObjectIndices)
    indices.append(objectIndex);
  return indices;
}
QString EditorViewModel::statusText() const { return statusText_; }
int EditorViewModel::loadProgress() const { return projectService_ ? projectService_->loadProgress() : 0; }
bool EditorViewModel::loading() const { return projectService_ ? projectService_->loading() : false; }
bool EditorViewModel::showAllObjects() const { return m_showAllObjects; }
int EditorViewModel::objectOrganizeMode() const { return m_objectOrganizeMode; }
bool EditorViewModel::hasSelection() const
{
  return !m_selectedSourceIndices.isEmpty() || m_selectedVolumeObjectSourceIndex >= 0;
}

bool EditorViewModel::hasSelectedVolume() const
{
  return m_selectedVolumeObjectSourceIndex >= 0 && !m_selectedVolumeIndices.isEmpty();
}

int EditorViewModel::selectedVolumeCount() const
{
  return m_selectedVolumeIndices.size();
}

bool EditorViewModel::canOpenSelectionSettings() const
{
  if (m_selectedVolumeObjectSourceIndex >= 0)
    return m_selectedVolumeIndices.size() == 1;

  return m_selectedSourceIndices.size() == 1;
}

bool EditorViewModel::canRenameSelectedObject() const
{
  return projectService_ && !hasSelectedVolume() && m_selectedSourceIndices.size() == 1;
}

bool EditorViewModel::canDuplicateSelectedObjects() const
{
  return projectService_ && !hasSelectedVolume() && !m_selectedSourceIndices.isEmpty();
}

bool EditorViewModel::canDeleteSelection() const
{
  return projectService_ && hasSelection();
}

bool EditorViewModel::canSetSelectionPrintable() const
{
  return projectService_ && !hasSelectedVolume() && !m_selectedSourceIndices.isEmpty();
}

bool EditorViewModel::canTransformSelection() const
{
  return projectService_ && !hasSelectedVolume() && !m_selectedSourceIndices.isEmpty();
}

bool EditorViewModel::canArrangeObjects() const
{
  return projectService_ && projectService_->modelCount() > 0;
}

int EditorViewModel::availableGizmoMask() const
{
  // Phase 90 (ASMROUTE-01): AssembleView-aware gizmo routing. When the active
  // canvas is CanvasAssembleView (m_activeCanvasType == 2), do not advertise
  // the Prepare gizmos (move/rotate/scale/cut/etc.) that have no AssembleView
  // implementation yet. Phase 92 (ASMMEASURE-01) advertises ONLY the Assembly
  // measurement gizmo (GizmoAssemblyMeasure = 19), and only when activable:
  // explosion ratio ~= 1.0 AND >=2 volumes selected. Mirrors upstream
  // GLGizmoAssembly::on_is_activable (GLGizmoAssembly.cpp:53-68) and the
  // gizmos manager + snapshot selection routing on CanvasAssembleView
  // (Plater.cpp:11601,11635). Prepare (CanvasView3D) path below is unchanged.
  if (m_activeCanvasType == 2)
  {
    if (isAssemblyMeasureActivable())
      return (1 << 19);  // GizmoAssemblyMeasure bit only.
    return 0;
  }

  int mask = 0;
  for (int mode = 0; mode <= 18; ++mode)
  {
    if (canActivateGizmo(mode))
      mask |= (1 << mode);
  }
  return mask;
}

QString EditorViewModel::settingsTargetType() const
{
  if (!canOpenSelectionSettings())
    return {};

  return m_selectedVolumeObjectSourceIndex >= 0 ? QStringLiteral("volume")
                                                : QStringLiteral("object");
}

QString EditorViewModel::settingsTargetName() const
{
  if (!projectService_ || !canOpenSelectionSettings())
    return {};

  if (m_selectedVolumeObjectSourceIndex >= 0)
  {
    const int volumeIndex = m_selectedVolumeIndex >= 0 ? m_selectedVolumeIndex : *m_selectedVolumeIndices.begin();
    return projectService_->objectVolumeName(m_selectedVolumeObjectSourceIndex, volumeIndex);
  }

  if (m_primarySelectedSourceIndex >= 0 && m_primarySelectedSourceIndex < m_objects.size())
    return m_objects[m_primarySelectedSourceIndex].name;

  return {};
}

int EditorViewModel::settingsTargetObjectIndex() const
{
  if (!canOpenSelectionSettings())
    return -1;

  return m_selectedVolumeObjectSourceIndex >= 0 ? m_selectedVolumeObjectSourceIndex
                                                : m_primarySelectedSourceIndex;
}

int EditorViewModel::settingsTargetVolumeIndex() const
{
  if (!canOpenSelectionSettings() || m_selectedVolumeObjectSourceIndex < 0)
    return -1;

  return m_selectedVolumeIndex >= 0 ? m_selectedVolumeIndex
                                    : *m_selectedVolumeIndices.begin();
}

QByteArray EditorViewModel::meshData() const { return m_cachedMeshData; }

// ---------- object list ----------
int EditorViewModel::objectCount() const { return visibleObjectIndices().size(); }
int EditorViewModel::selectedObjectIndex() const
{
  const QList<int> visibleIndices = visibleObjectIndices();
  for (int i = 0; i < visibleIndices.size(); ++i)
  {
    if (visibleIndices[i] == m_primarySelectedSourceIndex)
      return i;
  }
  return -1;
}

int EditorViewModel::selectedSourceObjectIndex() const
{
  return m_primarySelectedSourceIndex;
}

int EditorViewModel::selectedObjectCount() const { return m_selectedSourceIndices.size(); }

QString EditorViewModel::objectName(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  return (sourceIndex >= 0 && sourceIndex < m_objects.size()) ? m_objects[sourceIndex].name : QString{};
}

QString EditorViewModel::objectModuleName(int i) const
{
  if (!projectService_)
    return {};

  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceIndex >= 0 ? projectService_->objectModuleName(sourceIndex) : QString{};
}

QString EditorViewModel::sourceObjectGroupLabel(int sourceIndex) const
{
  if (sourceIndex < 0)
    return {};

  if (m_objectOrganizeMode == 1)
  {
    const QString moduleName = projectService_ ? projectService_->objectModuleName(sourceIndex) : QString{};
    return moduleName.isEmpty() ? QStringLiteral("默认模块") : moduleName;
  }

  return plateName(projectService_ ? projectService_->plateIndexForObject(sourceIndex) : -1);
}

QString EditorViewModel::sourceObjectGroupKey(int sourceIndex) const
{
  return QStringLiteral("%1:%2").arg(m_objectOrganizeMode).arg(sourceObjectGroupLabel(sourceIndex));
}

QString EditorViewModel::objectGroupLabel(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceObjectGroupLabel(sourceIndex);
}

int EditorViewModel::objectGroupCount(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return 0;

  const QString groupLabel = sourceObjectGroupLabel(sourceIndex);
  if (groupLabel.isEmpty())
    return 0;

  int count = 0;
  const QList<int> visibleIndices = baseVisibleObjectIndices();
  for (int candidateIndex : visibleIndices)
  {
    if (sourceObjectGroupLabel(candidateIndex) == groupLabel)
      ++count;
  }
  return count;
}

bool EditorViewModel::objectGroupExpanded(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return true;

  return !m_collapsedGroupKeys.contains(sourceObjectGroupKey(sourceIndex));
}

void EditorViewModel::toggleObjectGroupExpanded(int i)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

  const QString groupKey = sourceObjectGroupKey(sourceIndex);
  if (groupKey.isEmpty())
    return;

  if (m_collapsedGroupKeys.contains(groupKey))
    m_collapsedGroupKeys.remove(groupKey);
  else
    m_collapsedGroupKeys.insert(groupKey);

  ensureValidObjectSelection(true);
  emit stateChanged();
}

bool EditorViewModel::objectExpanded(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return false;

  return !m_collapsedObjectSourceIndices.contains(sourceIndex);
}

void EditorViewModel::toggleObjectExpanded(int i)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

  if (!projectService_ || projectService_->objectVolumeCount(sourceIndex) <= 0)
    return;

  if (m_collapsedObjectSourceIndices.contains(sourceIndex))
    m_collapsedObjectSourceIndices.remove(sourceIndex);
  else
    m_collapsedObjectSourceIndices.insert(sourceIndex);

  emit stateChanged();
}

int EditorViewModel::objectVolumeCount(int i) const
{
  if (!projectService_)
    return 0;

  const int sourceIndex = mapFilteredToSourceIndex(i);
  return (sourceIndex >= 0 && projectService_) ? projectService_->objectVolumeCount(sourceIndex) : 0;
}

int EditorViewModel::objectInstanceCount(int i) const
{
  if (!projectService_)
    return 0;

  const int sourceIndex = mapFilteredToSourceIndex(i);
  return (sourceIndex >= 0 && projectService_) ? projectService_->objectInstanceCount(sourceIndex) : 0;
}

QString EditorViewModel::objectVolumeName(int i, int volumeIndex) const
{
  if (!projectService_)
    return {};

  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceIndex >= 0 ? projectService_->objectVolumeName(sourceIndex, volumeIndex) : QString{};
}

QString EditorViewModel::objectVolumeTypeLabel(int i, int volumeIndex) const
{
  if (!projectService_)
    return {};

  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceIndex >= 0 ? projectService_->objectVolumeTypeLabel(sourceIndex, volumeIndex) : QString{};
}

bool EditorViewModel::isVolumeSelected(int i, int volumeIndex) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceIndex >= 0 && sourceIndex == m_selectedVolumeObjectSourceIndex && m_selectedVolumeIndices.contains(volumeIndex);
}

void EditorViewModel::selectVolume(int i, int volumeIndex)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;
  if (volumeIndex < 0 || !projectService_ || projectService_->objectVolumeCount(sourceIndex) <= volumeIndex)
    return;

  m_selectedSourceIndices.clear();
  m_selectedSourceIndices.insert(sourceIndex);
  m_primarySelectedSourceIndex = sourceIndex;
  m_selectedVolumeObjectSourceIndex = sourceIndex;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndices.insert(volumeIndex);
  m_selectedVolumeIndex = volumeIndex;
  emit stateChanged();
}

void EditorViewModel::toggleVolumeSelection(int i, int volumeIndex)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;
  if (volumeIndex < 0 || !projectService_ || projectService_->objectVolumeCount(sourceIndex) <= volumeIndex)
    return;

  if (m_selectedVolumeObjectSourceIndex >= 0 && m_selectedVolumeObjectSourceIndex != sourceIndex)
  {
    selectVolume(i, volumeIndex);
    return;
  }

  if (m_selectedVolumeObjectSourceIndex < 0)
  {
    selectVolume(i, volumeIndex);
    return;
  }

  m_selectedSourceIndices.clear();
  m_selectedSourceIndices.insert(sourceIndex);
  m_primarySelectedSourceIndex = sourceIndex;

  if (m_selectedVolumeIndices.contains(volumeIndex))
  {
    m_selectedVolumeIndices.remove(volumeIndex);
    if (m_selectedVolumeIndex == volumeIndex)
      m_selectedVolumeIndex = m_selectedVolumeIndices.isEmpty() ? -1 : *m_selectedVolumeIndices.begin();

    if (m_selectedVolumeIndices.isEmpty())
      m_selectedVolumeObjectSourceIndex = -1;
  }
  else
  {
    m_selectedVolumeIndices.insert(volumeIndex);
    m_selectedVolumeObjectSourceIndex = sourceIndex;
    m_selectedVolumeIndex = volumeIndex;
  }

  emit stateChanged();
}

bool EditorViewModel::deleteSelectedVolumesBySource()
{
  if (!projectService_ || m_selectedVolumeObjectSourceIndex < 0 || m_selectedVolumeIndices.isEmpty())
    return false;

  QList<int> volumeIndices = m_selectedVolumeIndices.values();
  std::sort(volumeIndices.begin(), volumeIndices.end(), std::greater<int>());
  volumeIndices.erase(std::unique(volumeIndices.begin(), volumeIndices.end()), volumeIndices.end());

  int deletedCount = 0;
  for (int volumeIndex : volumeIndices)
  {
    if (!projectService_->deleteObjectVolume(m_selectedVolumeObjectSourceIndex, volumeIndex))
    {
      statusText_ = projectService_->lastError();
      emit stateChanged();
      return deletedCount > 0;
    }
    ++deletedCount;
  }

  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  rebuildObjectEntriesFromService();
  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  statusText_ = deletedCount == 1 ? QStringLiteral("已删除部件")
                                  : QStringLiteral("已删除 %1 个部件").arg(deletedCount);
  emit stateChanged();
  return deletedCount > 0;
}

void EditorViewModel::deleteVolume(int i, int volumeIndex)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
  {
    statusText_ = QStringLiteral("删除失败：对象索引无效");
    emit stateChanged();
    return;
  }

  if (!projectService_->deleteObjectVolume(sourceIndex, volumeIndex))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return;
  }

  if (m_selectedVolumeObjectSourceIndex == sourceIndex && m_selectedVolumeIndex == volumeIndex)
  {
    m_selectedVolumeIndices.remove(volumeIndex);
    m_selectedVolumeIndex = m_selectedVolumeIndices.isEmpty() ? -1 : *m_selectedVolumeIndices.begin();
    if (m_selectedVolumeIndices.isEmpty())
      m_selectedVolumeObjectSourceIndex = -1;
  }
  else if (m_selectedVolumeObjectSourceIndex == sourceIndex)
  {
    QSet<int> nextSelection;
    for (int selectedIndex : m_selectedVolumeIndices)
    {
      if (selectedIndex == volumeIndex)
        continue;
      nextSelection.insert(selectedIndex > volumeIndex ? selectedIndex - 1 : selectedIndex);
    }
    m_selectedVolumeIndices = nextSelection;
    if (m_selectedVolumeIndex > volumeIndex)
      --m_selectedVolumeIndex;
    if (m_selectedVolumeIndices.isEmpty())
    {
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndex = -1;
    }
  }

  rebuildObjectEntriesFromService();
  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  statusText_ = QStringLiteral("已删除部件");
  emit stateChanged();
}

bool EditorViewModel::addVolumeToObject(int volumeType)
{
  // Require exactly one object selected (对齐上游约束：单对象选中时才可添加部件)
  if (!projectService_ || selectedObjectCount() != 1 || hasSelectedVolume())
  {
    statusText_ = QStringLiteral("请先选中一个对象");
    emit stateChanged();
    return false;
  }

  const int sourceIndex = selectedSourceObjectIndex();
  if (sourceIndex < 0)
    return false;

  if (!projectService_->addVolume(sourceIndex, volumeType))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  // Expand the object to show the new volume
  m_collapsedObjectSourceIndices.remove(sourceIndex);
  // Auto-select the newly added volume (last one)
  int volCount = projectService_->objectVolumeCount(sourceIndex);
  if (volCount > 0)
  {
    m_selectedVolumeObjectSourceIndex = sourceIndex;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndices.insert(volCount - 1);
    m_selectedVolumeIndex = volCount - 1;
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  statusText_ = QStringLiteral("已添加部件");
  emit stateChanged();
  return true;
}

bool EditorViewModel::changeVolumeType(int newVolumeType)
{
  // Require exactly one volume selected (对齐上游约束：单部件选中时才可转换类型)
  if (!projectService_ || !hasSelectedVolume())
  {
    statusText_ = QStringLiteral("请先选中一个部件");
    emit stateChanged();
    return false;
  }

  const int sourceIndex = m_selectedVolumeObjectSourceIndex;
  if (sourceIndex < 0)
    return false;

  if (!projectService_->changeVolumeType(sourceIndex, m_selectedVolumeIndex, newVolumeType))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  statusText_ = QStringLiteral("已转换部件类型");
  emit stateChanged();
  return true;
}

int EditorViewModel::volumeExtruderId(int objectIndex, int volumeIndex) const
{
  if (!projectService_)
    return -1;
  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  return sourceIndex >= 0 ? projectService_->volumeExtruderId(sourceIndex, volumeIndex) : -1;
}

bool EditorViewModel::setVolumeExtruderId(int objectIndex, int volumeIndex, int extruderId)
{
  if (!projectService_)
  {
    statusText_ = QStringLiteral("无法设置耗材：服务未就绪");
    emit stateChanged();
    return false;
  }

  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  if (sourceIndex < 0)
  {
    statusText_ = QStringLiteral("无法设置耗材：对象索引无效");
    emit stateChanged();
    return false;
  }

  if (!projectService_->setVolumeExtruderId(sourceIndex, volumeIndex, extruderId))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  statusText_ = QStringLiteral("已设置部件耗材");
  emit stateChanged();
  return true;
}

bool EditorViewModel::addVolumeFromFile(int objectIndex, const QString &filePath, int volumeType)
{
  if (!projectService_)
    return false;
  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  if (sourceIndex < 0)
    return false;
  bool ok = projectService_->addVolumeFromFile(sourceIndex, filePath, volumeType);
  if (ok)
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::addPrimitive(int objectIndex, int primitiveType)
{
  if (!projectService_)
    return false;
  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  if (sourceIndex < 0)
    return false;
  bool ok = projectService_->addPrimitive(sourceIndex, primitiveType);
  if (ok)
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::addTextVolume(int objectIndex, const QString &text)
{
  if (!projectService_)
    return false;
  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  if (sourceIndex < 0)
    return false;
  bool ok = projectService_->addTextVolume(sourceIndex, text);
  if (ok)
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::addSvgVolume(int objectIndex, const QString &svgFilePath)
{
  if (!projectService_)
    return false;
  const int sourceIndex = mapFilteredToSourceIndex(objectIndex);
  if (sourceIndex < 0)
    return false;
  bool ok = projectService_->addSvgVolume(sourceIndex, svgFilePath);
  if (ok)
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::isObjectSelected(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  return sourceIndex >= 0 && m_selectedSourceIndices.contains(sourceIndex);
}

bool EditorViewModel::objectPrintable(int i) const
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  return (sourceIndex >= 0 && sourceIndex < m_objects.size()) && m_objects[sourceIndex].printable;
}

void EditorViewModel::setObjectPrintable(int i, bool printable)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0 || sourceIndex >= m_objects.size())
    return;

  const int plateIndex = projectService_ ? projectService_->plateIndexForObject(sourceIndex) : -1;
  if (!projectService_ || !projectService_->setObjectPrintable(sourceIndex, printable))
  {
    statusText_ = projectService_ ? projectService_->lastError() : QStringLiteral("服务不可用");
    emit stateChanged();
    return;
  }

  m_objects[sourceIndex].printable = printable;
  invalidateSliceResultsForPlate(plateIndex);
  statusText_ = printable ? QStringLiteral("对象已设为可打印") : QStringLiteral("对象已设为不参与打印");
  emit stateChanged();
}

void EditorViewModel::deleteObject(int i)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
  {
    statusText_ = QStringLiteral("删除失败：对象索引无效");
    emit stateChanged();
    return;
  }

  // Create undo command BEFORE deleting (captures snapshots)
  DeleteObjectsCommand *deleteCmd = nullptr;
  if (m_undoManager)
    deleteCmd = new DeleteObjectsCommand({sourceIndex}, projectService_, this);

  const int plateIndex = projectService_ ? projectService_->plateIndexForObject(sourceIndex) : -1;
  if (projectService_->deleteObject(sourceIndex))
  {
    m_collapsedObjectSourceIndices.clear();
    rebuildObjectEntriesFromService();
    m_selectedSourceIndices.clear();
    ensureValidObjectSelection(true);
    refreshMeshCacheAndFitHint();
    statusText_ = QStringLiteral("已删除对象，剩余 %1 个模型").arg(projectService_->modelCount());

    if (m_undoManager && deleteCmd)
      m_undoManager->push(deleteCmd);

    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  else
  {
    delete deleteCmd;
    statusText_ = projectService_->lastError();
    emit stateChanged();
  }
}

void EditorViewModel::deleteSelectedObjects()
{
  if (!projectService_)
    return;

  QList<int> sourceIndices = m_selectedSourceIndices.values();
  if (sourceIndices.isEmpty())
  {
    const int sourceIndex = selectedSourceObjectIndex();
    if (sourceIndex >= 0)
      sourceIndices.append(sourceIndex);
  }

  if (sourceIndices.isEmpty())
    return;

  std::sort(sourceIndices.begin(), sourceIndices.end(), std::greater<int>());
  sourceIndices.erase(std::unique(sourceIndices.begin(), sourceIndices.end()), sourceIndices.end());
  QSet<int> affectedPlates;
  for (int sourceIndex : sourceIndices)
  {
    const int plateIndex = projectService_->plateIndexForObject(sourceIndex);
    if (plateIndex >= 0)
      affectedPlates.insert(plateIndex);
  }

  // Create undo command BEFORE deleting (captures snapshots)
  DeleteObjectsCommand *deleteCmd = nullptr;
  if (m_undoManager)
    deleteCmd = new DeleteObjectsCommand(sourceIndices, projectService_, this);

  int deletedCount = 0;
  for (int sourceIndex : sourceIndices)
  {
    if (!projectService_->deleteObject(sourceIndex))
    {
      statusText_ = projectService_->lastError();
      rebuildObjectEntriesFromService();
      ensureValidObjectSelection(true);
      refreshMeshCacheAndFitHint();
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndices.clear();
      m_selectedVolumeIndex = -1;
      delete deleteCmd;
      emit stateChanged();
      return;
    }
    ++deletedCount;
  }

  m_collapsedObjectSourceIndices.clear();
  rebuildObjectEntriesFromService();
  m_selectedSourceIndices.clear();
  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  statusText_ = QStringLiteral("已删除 %1 个对象，剩余 %2 个模型").arg(deletedCount).arg(projectService_->modelCount());

  if (m_undoManager && deleteCmd)
    m_undoManager->push(deleteCmd);

  for (int plateIndex : affectedPlates)
    invalidateSliceResultsForPlate(plateIndex);
  emit stateChanged();
}

void EditorViewModel::selectObject(int i)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

  if (m_selectedSourceIndices.size() == 1 && m_selectedSourceIndices.contains(sourceIndex) && m_primarySelectedSourceIndex == sourceIndex)
    return;

  m_selectedSourceIndices.clear();
  m_selectedSourceIndices.insert(sourceIndex);
  m_primarySelectedSourceIndex = sourceIndex;
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  emit stateChanged();
}

bool EditorViewModel::selectSourceObject(int sourceIndex)
{
  if (!projectService_ || sourceIndex < 0 || sourceIndex >= m_objects.size())
    return false;

  // Phase 90 (ASMROUTE-01): selection on the AssembleView canvas routes through
  // the same shared ProjectServiceMock model (90-CONTEXT.md decision 2 — no
  // separate scene copy). This mirrors upstream Plater.cpp:11601 (snapshot
  // selection routing) and Plater.cpp:11635 (gizmos manager selection on
  // CanvasAssembleView), where the selection state is shared and not
  // duplicated per canvas. Prepare (CanvasView3D) path below is unchanged.
  // (No explicit branch needed here for Phase 90; the comment documents that
  // the shared selection state is the AssembleView routing surface.)

  const QList<int> activePlateObjects = projectService_->currentPlateObjectIndices();
  if (!activePlateObjects.contains(sourceIndex)) {
    if (m_primarySelectedSourceIndex >= 0) {
      m_selectedSourceIndices.clear();
      m_primarySelectedSourceIndex = -1;
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndices.clear();
      m_selectedVolumeIndex = -1;
      emit stateChanged();
    }
    return false;
  }

  if (m_selectedSourceIndices.size() == 1
      && m_selectedSourceIndices.contains(sourceIndex)
      && m_primarySelectedSourceIndex == sourceIndex)
    return true;

  m_selectedSourceIndices.clear();
  m_selectedSourceIndices.insert(sourceIndex);
  m_primarySelectedSourceIndex = sourceIndex;
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  emit stateChanged();
  return true;
}

void EditorViewModel::toggleObjectSelection(int i)
{
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

  if (m_selectedSourceIndices.contains(sourceIndex))
  {
    m_selectedSourceIndices.remove(sourceIndex);
    if (m_primarySelectedSourceIndex == sourceIndex)
      m_primarySelectedSourceIndex = -1;
    if (m_selectedVolumeObjectSourceIndex == sourceIndex)
    {
      m_selectedVolumeObjectSourceIndex = -1;
      m_selectedVolumeIndices.clear();
      m_selectedVolumeIndex = -1;
    }
    ensureValidObjectSelection(false);
  }
  else
  {
    m_selectedSourceIndices.insert(sourceIndex);
    m_primarySelectedSourceIndex = sourceIndex;
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
  }

  emit stateChanged();
}

void EditorViewModel::clearObjectSelection()
{
  if (m_selectedSourceIndices.isEmpty() && m_primarySelectedSourceIndex < 0)
    return;

  m_selectedSourceIndices.clear();
  m_primarySelectedSourceIndex = -1;
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  emit stateChanged();
}

void EditorViewModel::selectAllVisibleObjects()
{
  const QList<int> visibleIndices = visibleObjectIndices();
  if (visibleIndices.isEmpty())
    return;

  m_selectedSourceIndices.clear();
  for (int sourceIndex : visibleIndices)
    m_selectedSourceIndices.insert(sourceIndex);
  m_primarySelectedSourceIndex = visibleIndices.front();
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  emit stateChanged();
}

void EditorViewModel::setSelectedObjectsPrintable(bool printable)
{
  if (!canSetSelectionPrintable())
    return;

  QSet<int> affectedPlates;
  for (int sourceIndex : m_selectedSourceIndices)
  {
    if (sourceIndex >= 0 && sourceIndex < m_objects.size())
    {
      const int plateIndex = projectService_ ? projectService_->plateIndexForObject(sourceIndex) : -1;
      if (!projectService_ || !projectService_->setObjectPrintable(sourceIndex, printable))
      {
        statusText_ = projectService_ ? projectService_->lastError() : QStringLiteral("服务不可用");
        emit stateChanged();
        return;
      }
      m_objects[sourceIndex].printable = printable;
      if (plateIndex >= 0)
        affectedPlates.insert(plateIndex);
    }
  }

  for (int plateIndex : affectedPlates)
    invalidateSliceResultsForPlate(plateIndex);
  statusText_ = printable ? QStringLiteral("已将所选对象设为可打印") : QStringLiteral("已将所选对象设为不参与打印");
  emit stateChanged();
}

void EditorViewModel::deleteSelection()
{
  if (hasSelectedVolume())
  {
    deleteSelectedVolumesBySource();
    return;
  }

  if (selectedObjectCount() > 1)
  {
    deleteSelectedObjects();
    return;
  }

  const int index = selectedObjectIndex();
  if (index >= 0)
    deleteObject(index);
}

void EditorViewModel::duplicateSelectedObjects()
{
  if (!canDuplicateSelectedObjects())
    return;

  // 收集所有选中的源索引（按从大到小排列，避免插入导致索引偏移问题）
  QList<int> selected = m_selectedSourceIndices.values();
  std::sort(selected.begin(), selected.end(), std::greater<int>());

  if (selected.isEmpty())
    return;

  // Track clones for undo commands
  QList<QPair<int, int>> clonePairs; // (sourceIndex, clonedIndex)

  // 逐个复制
  bool anySuccess = false;
  for (int srcIdx : selected)
  {
    const int newIdx = projectService_->duplicateObject(srcIdx);
    if (newIdx >= 0)
    {
      anySuccess = true;
      clonePairs.append({srcIdx, newIdx});
    }
  }

  if (anySuccess)
  {
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    invalidateSliceResultsForCurrentPlate();

    // Push undo commands (in reverse order so undo restores correctly)
    if (m_undoManager)
    {
      if (clonePairs.size() == 1)
      {
        m_undoManager->push(new CloneCommand(clonePairs[0].first, clonePairs[0].second, projectService_));
      }
      else if (clonePairs.size() > 1)
      {
        m_undoManager->beginMacro(QObject::tr("Clone %1 Objects").arg(clonePairs.size()));
        for (int i = clonePairs.size() - 1; i >= 0; --i)
          m_undoManager->push(new CloneCommand(clonePairs[i].first, clonePairs[i].second, projectService_));
        m_undoManager->endMacro();
      }
    }

    emit stateChanged();
  }
}

bool EditorViewModel::hasClipboardContent() const
{
  return !m_clipboard.isEmpty();
}

void EditorViewModel::copySelectedObjects()
{
  if (!canDuplicateSelectedObjects())
    return;

  // 复制选中对象元数据到内部剪贴板（对齐上游 Selection::copy_to_clipboard）
  m_clipboard.clear();
  QList<int> sorted = m_selectedSourceIndices.values();
  std::sort(sorted.begin(), sorted.end());
  for (int srcIdx : sorted)
  {
    if (srcIdx >= 0 && srcIdx < m_objects.size())
      m_clipboard.append(m_objects[srcIdx]);
  }
  emit stateChanged();
}

void EditorViewModel::pasteObjects()
{
  if (!projectService_ || m_clipboard.isEmpty())
    return;
  // 粘贴：对每个剪贴板条目添加新对象（对齐上游 Selection::paste_objects_from_clipboard）
  m_selectedSourceIndices.clear();
  QList<QPair<int, QString>> addedPairs; // (newIndex, name)

  for (const auto &entry : m_clipboard)
  {
    const int newIdx = projectService_->addObject(entry.name);
    if (newIdx >= 0)
    {
      m_selectedSourceIndices.insert(newIdx);
      addedPairs.append({newIdx, entry.name});
    }
  }

  // Push undo commands
  if (m_undoManager && !addedPairs.isEmpty())
  {
    if (addedPairs.size() == 1)
    {
      m_undoManager->push(new AddObjectCommand(addedPairs[0].first, addedPairs[0].second, projectService_));
    }
    else
    {
      m_undoManager->beginMacro(QObject::tr("Paste %1 Objects").arg(addedPairs.size()));
      for (const auto &pair : addedPairs)
        m_undoManager->push(new AddObjectCommand(pair.first, pair.second, projectService_));
      m_undoManager->endMacro();
    }
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  invalidateSliceResultsForCurrentPlate();
  emit stateChanged();
}

void EditorViewModel::mirrorSelectedObjects(int axis)
{
  if (!canTransformSelection())
    return;

#ifdef HAS_LIBSLIC3R
  // Sync real model mirror state
  for (int srcIdx : m_selectedSourceIndices)
    projectService_->mirrorObject(srcIdx, axis);
  projectService_->syncTransformsFromModel();
#endif

  // The active viewport handles the visual mirror through its bound state.
  invalidateSliceResultsForCurrentPlate();
  emit stateChanged();
}

void EditorViewModel::cutSelectedObjects()
{
  if (!canDuplicateSelectedObjects())
    return;

  copySelectedObjects();
  deleteSelectedObjects();
}

void EditorViewModel::toggleSelectedObjectsVisibility()
{
  if (!canTransformSelection())
    return;

  for (int srcIdx : m_selectedSourceIndices)
  {
    bool current = projectService_->objectVisible(srcIdx);
    projectService_->setObjectVisible(srcIdx, !current);
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  emit stateChanged();
}

void EditorViewModel::autoOrientSelected()
{
  if (!canTransformSelection())
    return;

  statusText_ = tr("正在计算最优朝向...");
  emit stateChanged();

#ifdef HAS_LIBSLIC3R
  // 对齐上游 Plater::orient() + OrientJob → Slic3r::orientation::orient(ModelObject*)
  auto indices = m_selectedSourceIndices.values();
  bool anyOriented = false;

  for (int idx : indices)
  {
    if (projectService_->orientObject(idx))
      anyOriented = true;
  }

  if (anyOriented)
    projectService_->syncTransformsFromModel();

  statusText_ = anyOriented ? tr("自动朝向完成") : tr("自动朝向完成（无变更）");
  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  emit stateChanged();

#else
  // Mock mode: signal completion after brief delay
  QTimer::singleShot(300, this, [this]()
  {
    statusText_ = tr("自动朝向完成（Mock）");
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  });
#endif
}

void EditorViewModel::splitSelectedObject()
{
  if (!canRenameSelectedObject())
    return;

  // 对齐上游 Plater::priv::split_object()
  // 仅支持单个选中对象拆分
  if (m_selectedSourceIndices.size() != 1)
  {
    statusText_ = tr("拆分操作仅支持选中单个对象");
    emit stateChanged();
    return;
  }

  const int srcIdx = *m_selectedSourceIndices.constBegin();
  statusText_ = tr("正在拆分对象...");
  emit stateChanged();

#ifdef HAS_LIBSLIC3R
  // 对齐上游 Plater::priv::split_object() → ModelObject::split()
  QList<int> newIndices = projectService_->splitObject(srcIdx);
  if (!newIndices.isEmpty())
  {
    m_selectedSourceIndices.clear();
    for (int newIdx : newIndices)
      m_selectedSourceIndices.insert(newIdx);
    statusText_ = tr("已拆分为 %1 个部分").arg(newIndices.size());
  }
  else
  {
    statusText_ = tr("无法拆分（对象已是最简形式）");
  }
#else
  // Mock 模式：基于 AABB 中点切面做模拟拆分
  const QString origName = (srcIdx >= 0 && srcIdx < m_objects.size())
                               ? m_objects[srcIdx].name
                               : tr("对象");

  const QVector4D dims = m_measureDimensions;

  const int idx1 = projectService_->addObject(origName + tr("_upper"));
  const int idx2 = projectService_->addObject(origName + tr("_lower"));

  if (idx1 >= 0 && idx2 >= 0) {
    m_selectedSourceIndices.clear();
    m_selectedSourceIndices.insert(srcIdx);
    m_selectedSourceIndices.insert(idx1);
    m_selectedSourceIndices.insert(idx2);
    statusText_ = tr("已沿 Y 轴拆分为 2 个部件");
  } else {
    statusText_ = tr("拆分失败");
  }
#endif

  rebuildObjectEntriesFromService();
  invalidateSliceResultsForCurrentPlate();
  refreshMeshCacheAndFitHint();
  emit stateChanged();
}

bool EditorViewModel::fixMeshSelected()
{
  if (!canTransformSelection())
    return false;

  bool anyFixed = false;
  for (int srcIdx : m_selectedSourceIndices)
  {
    if (projectService_->fixMesh(srcIdx))
      anyFixed = true;
  }

  if (anyFixed)
  {
    refreshMeshCacheAndFitHint();
    invalidateSliceResultsForCurrentPlate();
    statusText_ = tr("网格修复完成");
  }
  else
  {
    statusText_ = tr("网格修复失败");
  }
  emit stateChanged();
  return anyFixed;
}

bool EditorViewModel::simplifyMeshSelected()
{
  // Stub — needs simplify dialog (GLGizmoSimplify) integration
  qWarning("[EditorViewModel] simplifyMeshSelected: not yet implemented (needs simplify dialog)");
  return false;
}

bool EditorViewModel::meshBooleanSelected()
{
  // Stub — needs boolean dialog (GLGizmoMeshBoolean) integration
  qWarning("[EditorViewModel] meshBooleanSelected: not yet implemented (needs boolean dialog)");
  return false;
}

bool EditorViewModel::changeVolumeTypeByIndex(int objIdx, int volIdx, int newType)
{
  if (!projectService_)
    return false;
  if (objIdx < 0 || volIdx < 0)
    return false;

  if (!projectService_->changeVolumeType(objIdx, volIdx, newType))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  statusText_ = tr("已转换部件类型");
  emit stateChanged();
  return true;
}

bool EditorViewModel::reloadSelectedFromDisk()
{
  if (!canTransformSelection())
    return false;

  bool anyReloaded = false;
  for (int srcIdx : m_selectedSourceIndices)
  {
    if (projectService_->reloadFromDisk(srcIdx))
      anyReloaded = true;
  }

  if (anyReloaded)
  {
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    invalidateSliceResultsForCurrentPlate();
    statusText_ = tr("已从磁盘重新加载");
  }
  else
  {
    statusText_ = tr("重新加载失败");
  }
  emit stateChanged();
  return anyReloaded;
}

bool EditorViewModel::replaceWithStl(const QString &path)
{
  if (!projectService_ || !hasSelectedVolume())
    return false;

  const int srcIdx = m_selectedVolumeObjectSourceIndex;
  if (srcIdx < 0)
    return false;

  if (!projectService_->replaceVolume(srcIdx, m_selectedVolumeIndex, path))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  invalidateSliceResultsForCurrentPlate();
  statusText_ = tr("已替换部件网格");
  emit stateChanged();
  return true;
}

bool EditorViewModel::reloadAllOnPlate()
{
  if (!projectService_)
    return false;

  QList<int> plateObjs = projectService_->currentPlateObjectIndices();
  bool anyReloaded = false;
  for (int idx : plateObjs)
  {
    if (projectService_->reloadFromDisk(idx))
      anyReloaded = true;
  }

  if (anyReloaded)
  {
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    invalidateSliceResultsForCurrentPlate();
    statusText_ = tr("已重新加载当前平板所有对象");
  }
  else
  {
    statusText_ = tr("重新加载失败");
  }
  emit stateChanged();
  return anyReloaded;
}

bool EditorViewModel::assembleSelectedObjects()
{
  if (!projectService_ || m_selectedSourceIndices.size() < 2)
  {
    statusText_ = tr("合并需要至少选中两个对象");
    emit stateChanged();
    return false;
  }

  QList<int> indices = m_selectedSourceIndices.values();
  if (!projectService_->assembleObjects(indices))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  m_selectedSourceIndices.clear();
  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  invalidateSliceResultsForCurrentPlate();
  statusText_ = tr("已合并选中对象");
  emit stateChanged();
  return true;
}

bool EditorViewModel::instanceToObject(int instIdx)
{
  if (!projectService_)
    return false;

  const int sourceIndex = selectedSourceObjectIndex();
  if (sourceIndex < 0)
    return false;

  if (!projectService_->duplicateInstanceAsObject(sourceIndex, instIdx))
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  statusText_ = tr("已将实例转换为独立对象");
  emit stateChanged();
  return true;
}

int EditorViewModel::getSelectedVolumeType() const
{
  if (!projectService_ || m_selectedVolumeObjectSourceIndex < 0)
    return 0;
  return projectService_->objectVolumeType(m_selectedVolumeObjectSourceIndex, m_selectedVolumeIndex);
}

bool EditorViewModel::addPrimitiveToPlate(int type)
{
  if (!projectService_)
    return false;

  int newIdx = projectService_->addPrimitiveToPlate(type);
  if (newIdx < 0)
  {
    statusText_ = projectService_->lastError();
    emit stateChanged();
    return false;
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  statusText_ = tr("已添加原始体");
  emit stateChanged();
  return true;
}

bool EditorViewModel::deleteFilamentSlot(int index)
{
  // Stub — needs filament management integration
  qWarning("[EditorViewModel] deleteFilamentSlot(%d): not yet implemented (needs filament management)", index);
  Q_UNUSED(index);
  return false;
}

bool EditorViewModel::mergeFilamentSlots(int from, int to)
{
  // Stub — needs filament management integration
  qWarning("[EditorViewModel] mergeFilamentSlots(%d, %d): not yet implemented (needs filament management)", from, to);
  Q_UNUSED(from);
  Q_UNUSED(to);
  return false;
}

bool EditorViewModel::showLabels() const { return m_showLabels; }

void EditorViewModel::setShowLabels(bool v)
{
  if (m_showLabels == v) return;
  m_showLabels = v;
  emit stateChanged();
}

void EditorViewModel::fixMeshForObject(int i)
{
  if (!projectService_)
    return;
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

#ifdef HAS_LIBSLIC3R
  // Align upstream MeshRepairDialog / fix_mesh via its_quadric_edge_collapse + repair
  statusText_ = tr("正在修复网格...");
  emit stateChanged();
  projectService_->fixMeshForObject(sourceIndex);
  refreshMeshCacheAndFitHint();
  invalidateSliceResultsForCurrentPlate();
  statusText_ = tr("网格修复完成");
#else
  statusText_ = tr("网格修复需要 libslic3r 支持");
#endif
  emit stateChanged();
}

void EditorViewModel::exportObjectAsStl(int i)
{
  if (!projectService_)
    return;
  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return;

  statusText_ = tr("STL 导出功能准备中...");
  emit stateChanged();
  // Actual file dialog + export is deferred to a dedicated export service
  // For now, record the intent
  statusText_ = tr("请使用主菜单 文件 > 导出 STL 功能");
  emit stateChanged();
}

void EditorViewModel::requestSelectionSettings()
{
  if (!canOpenSelectionSettings())
    return;

  emit selectionSettingsRequested();
}

QString EditorViewModel::plateName(int i) const
{
  if (!projectService_)
    return {};
  const QStringList names = projectService_->plateNames();
  return (i >= 0 && i < names.size()) ? names[i] : QString{};
}

int EditorViewModel::plateObjectCount(int i) const
{
  return projectService_ ? projectService_->plateObjectCount(i) : 0;
}

int EditorViewModel::objectPlateIndex(int i) const
{
  if (!projectService_)
    return -1;

  const int sourceIndex = mapFilteredToSourceIndex(i);
  if (sourceIndex < 0)
    return -1;

  return projectService_->plateIndexForObject(sourceIndex);
}

QString EditorViewModel::objectPlateName(int i) const
{
  if (!projectService_)
    return QString{};

  return plateName(objectPlateIndex(i));
}

bool EditorViewModel::setCurrentPlateIndex(int i)
{
  const bool ok = projectService_->setCurrentPlateIndex(i);
  if (!ok)
    return false;

  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  emit stateChanged();
  return true;
}

void EditorViewModel::setShowAllObjects(bool showAll)
{
  if (m_showAllObjects == showAll)
    return;
  m_showAllObjects = showAll;

  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  emit stateChanged();
}

void EditorViewModel::setObjectOrganizeMode(int mode)
{
  const int normalizedMode = mode == 1 ? 1 : 0;
  if (m_objectOrganizeMode == normalizedMode)
    return;

  m_objectOrganizeMode = normalizedMode;
  ensureValidObjectSelection(true);
  emit stateChanged();
}

bool EditorViewModel::addPlate()
{
  if (!canAddPlate())
    return false;
  if (projectService_->addPlate())
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
    return true;
  }
  return false;
}

bool EditorViewModel::canAddPlate() const
{
  return projectService_ && plateCount() < maxPlateCount();
}

bool EditorViewModel::deletePlate(int plateIndex)
{
  if (!canDeletePlate(plateIndex))
    return false;
  if (projectService_->deletePlate(plateIndex))
  {
    invalidateAllSliceResults();
    rebuildObjectEntriesFromService();
    emit stateChanged();
    return true;
  }
  return false;
}

bool EditorViewModel::canDeletePlate(int plateIndex) const
{
  return projectService_ && plateIndex >= 0 && plateIndex < plateCount() && plateCount() > 1;
}

void EditorViewModel::selectAllOnPlate(int plateIndex)
{
  if (!projectService_)
    return;
  const QList<int> objs = projectService_->plateObjectIndices(plateIndex);
  m_selectedSourceIndices.clear();
  m_primarySelectedSourceIndex = -1;
  for (int objIdx : objs)
    m_selectedSourceIndices.insert(objIdx);
  if (!objs.isEmpty())
    m_primarySelectedSourceIndex = objs.first();
  emit stateChanged();
}

void EditorViewModel::removeAllOnPlate(int plateIndex)
{
  if (!projectService_)
    return;
  if (projectService_->removeAllOnPlate(plateIndex))
  {
    invalidateSliceResultsForPlate(plateIndex);
    m_selectedSourceIndices.clear();
    m_primarySelectedSourceIndex = -1;
    rebuildObjectEntriesFromService();
    emit stateChanged();
  }
}

bool EditorViewModel::renamePlate(int plateIndex, const QString &newName)
{
  if (!projectService_)
    return false;
  return projectService_->renamePlate(plateIndex, newName);
}

bool EditorViewModel::isPlateLocked(int plateIndex) const
{
  if (!projectService_)
    return false;
  return projectService_->isPlateLocked(plateIndex);
}

void EditorViewModel::togglePlateLocked(int plateIndex)
{
  if (!projectService_)
    return;
  const bool current = projectService_->isPlateLocked(plateIndex);
  projectService_->setPlateLocked(plateIndex, !current);
  invalidateSliceResultsForPlate(plateIndex);
  emit stateChanged();
}

// ── v3.0 Phase 17: plate lifecycle completion (PLATE-03/04/05) ──

bool EditorViewModel::clonePlate(int sourceIndex)
{
  if (!projectService_)
    return false;
  const bool ok = projectService_->clonePlate(sourceIndex);
  if (ok) {
    invalidateAllSliceResults();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::movePlate(int oldIndex, int newIndex)
{
  if (!projectService_)
    return false;
  const bool ok = projectService_->movePlate(oldIndex, newIndex);
  if (ok) {
    invalidateAllSliceResults();
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::setPlatePrintable(int plateIndex, bool printable)
{
  if (!projectService_)
    return false;
  const bool ok = projectService_->setPlatePrintable(plateIndex, printable);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::isPlatePrintable(int plateIndex) const
{
  return projectService_ ? projectService_->isPlatePrintable(plateIndex) : false;
}

// Phase 110 (FMAP-03): mode-only write path for the FilamentGroupPopup.
// Delegates to ProjectServiceMock::setPlateFilamentMapMode; the clamp at
// PartPlate::setFilamentMapMode(int) (R-02 / FP-04) guards the int boundary.
bool EditorViewModel::setPlateFilamentMapMode(int plateIndex, int mode)
{
  if (!projectService_)
    return false;
  const bool ok = projectService_->setPlateFilamentMapMode(plateIndex, mode);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

int EditorViewModel::plateFilamentMapMode(int plateIndex) const
{
  return projectService_ ? projectService_->plateFilamentMapMode(plateIndex)
                         : int(OWzx::FilamentMapMode::fmmAutoForFlush);
}

bool EditorViewModel::isPlateSliced(int plateIndex) const
{
  return m_slicedPlateIndices.contains(plateIndex);
}

bool EditorViewModel::moveSelectedObjectToPlate(int targetPlateIndex)
{
  if (!canMoveSelectionToPlate(targetPlateIndex))
    return false;

  QSet<int> sourcePlates;
  QList<int> selected = m_selectedSourceIndices.values();
  std::sort(selected.begin(), selected.end());
  for (int sourceIndex : selected)
  {
    const int sourcePlate = projectService_->plateIndexForObject(sourceIndex);
    if (sourcePlate >= 0)
      sourcePlates.insert(sourcePlate);
    projectService_->setObjectPlateForIndex(sourceIndex, targetPlateIndex);
  }

  for (int sourcePlate : sourcePlates)
    invalidateSliceResultsForPlate(sourcePlate);
  invalidateSliceResultsForPlate(targetPlateIndex);
  rebuildObjectEntriesFromService();
  ensureValidObjectSelection(true);
  refreshMeshCacheAndFitHint();
  emit stateChanged();
  return true;
}

bool EditorViewModel::canMoveSelectionToPlate(int targetPlateIndex) const
{
  if (!projectService_ || targetPlateIndex < 0 || targetPlateIndex >= plateCount())
    return false;
  if (hasSelectedVolume() || m_selectedSourceIndices.isEmpty())
    return false;
  for (int sourceIndex : m_selectedSourceIndices)
  {
    if (sourceIndex < 0 || sourceIndex >= m_objects.size())
      return false;
    if (projectService_->plateIndexForObject(sourceIndex) != targetPlateIndex)
      return true;
  }
  return false;
}

QString EditorViewModel::plateThumbnailColor(int plateIndex) const
{
  // Mock 模式：基于平板索引生成独特颜色（对齐上游 PartPlate thumbnail 视觉区分）
  static const char *kColors[] = {
      "#1a3a5c", "#2a3a2c", "#3a2a3c", "#1c3c3a", "#3c3a1a",
      "#2a1c3c", "#1c2a3c", "#3c2a1a", "#1a3c2c", "#2c3a1c"};
  const int count = static_cast<int>(std::size(kColors));
  return QString::fromUtf8(kColors[plateIndex % count]);
}

bool EditorViewModel::renameObject(int index, const QString &newName)
{
  if (!projectService_ || index < 0)
    return false;

  const int sourceIndex = mapFilteredToSourceIndex(index);
  if (sourceIndex < 0)
    return false;

  const QString oldName = projectService_->objectNames().value(sourceIndex);
  if (oldName == newName)
    return true;

  projectService_->renameObject(sourceIndex, newName);

  if (m_undoManager)
    m_undoManager->push(new RenameCommand(sourceIndex, oldName, newName, projectService_));

  emit stateChanged();
  return true;
}

bool EditorViewModel::moveObject(int fromIndex, int toIndex)
{
  if (!projectService_ || fromIndex < 0 || toIndex < 0)
    return false;
  // Map filtered indices to source indices
  const int srcFrom = mapFilteredToSourceIndex(fromIndex);
  const int srcTo = mapFilteredToSourceIndex(toIndex);
  if (srcFrom < 0 || srcTo < 0)
    return false;
  return projectService_->moveObject(srcFrom, srcTo);
}

int EditorViewModel::plateBedType(int plateIndex) const
{
  return projectService_ ? projectService_->plateBedType(plateIndex) : 0;
}

bool EditorViewModel::setPlateBedType(int plateIndex, int bedType)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateBedType(plateIndex, bedType);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

int EditorViewModel::platePrintSequence(int plateIndex) const
{
  return projectService_ ? projectService_->platePrintSequence(plateIndex) : 0;
}

bool EditorViewModel::setPlatePrintSequence(int plateIndex, int seq)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlatePrintSequence(plateIndex, seq);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

int EditorViewModel::plateSpiralMode(int plateIndex) const
{
  return projectService_ ? projectService_->plateSpiralMode(plateIndex) : 0;
}

bool EditorViewModel::setPlateSpiralMode(int plateIndex, int mode)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateSpiralMode(plateIndex, mode);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

// ── 首层耗材顺序（对齐上游 first_layer_print_sequence）──

int EditorViewModel::plateFirstLayerSeqChoice(int plateIndex) const
{
  return projectService_ ? projectService_->plateFirstLayerSeqChoice(plateIndex) : 0;
}

bool EditorViewModel::setPlateFirstLayerSeqChoice(int plateIndex, int choice)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateFirstLayerSeqChoice(plateIndex, choice);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

QVariantList EditorViewModel::plateFirstLayerSeqOrder(int plateIndex) const
{
  return projectService_ ? projectService_->plateFirstLayerSeqOrder(plateIndex) : QVariantList();
}

bool EditorViewModel::setPlateFirstLayerSeqOrder(int plateIndex, const QVariantList &order)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateFirstLayerSeqOrder(plateIndex, order);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

// ── 其他层耗材顺序（对齐上游 other_layers_print_sequence）──

int EditorViewModel::plateOtherLayersSeqChoice(int plateIndex) const
{
  return projectService_ ? projectService_->plateOtherLayersSeqChoice(plateIndex) : 0;
}

bool EditorViewModel::setPlateOtherLayersSeqChoice(int plateIndex, int choice)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateOtherLayersSeqChoice(plateIndex, choice);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

int EditorViewModel::plateOtherLayersSeqCount(int plateIndex) const
{
  return projectService_ ? projectService_->plateOtherLayersSeqCount(plateIndex) : 0;
}

int EditorViewModel::plateOtherLayersSeqBegin(int plateIndex, int entryIndex) const
{
  return projectService_ ? projectService_->plateOtherLayersSeqBegin(plateIndex, entryIndex) : 2;
}

int EditorViewModel::plateOtherLayersSeqEnd(int plateIndex, int entryIndex) const
{
  return projectService_ ? projectService_->plateOtherLayersSeqEnd(plateIndex, entryIndex) : 100;
}

QVariantList EditorViewModel::plateOtherLayersSeqOrder(int plateIndex, int entryIndex) const
{
  return projectService_ ? projectService_->plateOtherLayersSeqOrder(plateIndex, entryIndex) : QVariantList();
}

bool EditorViewModel::addPlateOtherLayersSeqEntry(int plateIndex, int beginLayer, int endLayer)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->addPlateOtherLayersSeqEntry(plateIndex, beginLayer, endLayer);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->removePlateOtherLayersSeqEntry(plateIndex, entryIndex);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateOtherLayersSeqRange(plateIndex, entryIndex, beginLayer, endLayer);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

bool EditorViewModel::setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateOtherLayersSeqOrder(plateIndex, entryIndex, order);
  if (ok) {
    invalidateSliceResultsForPlate(plateIndex);
    emit stateChanged();
  }
  return ok;
}

int EditorViewModel::plateExtruderCount(int plateIndex) const
{
  return projectService_ ? projectService_->plateExtruderCount(plateIndex) : 1;
}

QString EditorViewModel::plateThumbnailBase64(int plateIndex) const
{
  return projectService_ ? projectService_->plateThumbnailBase64(plateIndex) : QString();
}

void EditorViewModel::centerSelectedObjects()
{
  // 对齐上游 Plater::priv::on_center / ModelObject::center_instances
  // Mock 模式：重置选中对象位置到原点
  if (!projectService_)
    return;
  const int count = objectCount();
  for (int i = 0; i < count; ++i)
  {
    if (isObjectSelected(i))
    {
      const int sourceIndex = mapFilteredToSourceIndex(i);
      if (sourceIndex >= 0)
        projectService_->setObjectPosition(sourceIndex, 0, 0, 0);
    }
  }
  invalidateSliceResultsForCurrentPlate();
  emit stateChanged();
}

void EditorViewModel::fillBedWithCopies()
{
  // 对齐上游 Plater::priv::on_fill_bed
  // Mock 模式：生成 9 个副本（3x3 网格）
  if (!projectService_)
    return;
  const int sel = selectedSourceObjectIndex();
  if (sel < 0)
    return;
  const QString name = projectService_->objectNames().value(sel, "Object");
  for (int r = 0; r < 3; ++r)
  {
    for (int c = 0; c < 3; ++c)
    {
      if (r == 0 && c == 0)
        continue;
      const int idx = projectService_->addObject(name + "_" + QString::number(r * 3 + c + 1));
      if (idx >= 0)
        projectService_->setObjectPosition(idx, (c - 1) * 60.0f, (r - 1) * 60.0f, 0);
    }
  }
  invalidateSliceResultsForCurrentPlate();
  emit stateChanged();
}

void EditorViewModel::exportSelectedAsStl()
{
  // 对齐上游 Plater::priv::on_export_stl
  // Mock 模式：仅通知用户（实际 STL 导出需要 libslic3r mesh 序列化）
  if (!projectService_)
    return;
  const int sel = selectedSourceObjectIndex();
  if (sel >= 0)
  {
    const QString name = projectService_->objectNames().value(sel, "object");
    emit stateChanged();
    // 通知通过 BackendContext 显示
    emit stateChanged(); // 触发 UI 更新
    // TODO: 实际模式下调用 libslic3r IO::STL::write_ascii 或 store_stl
  }
}

// ---------- slice bridge ----------
int EditorViewModel::sliceProgress() const
{
  if (!sliceService_)
    return 0;
  if (sliceService_->slicing())
    return sliceService_->progress();

  return hasSliceResult() ? sliceService_->progress() : 0;
}

bool EditorViewModel::isSlicing() const { return sliceService_ && sliceService_->slicing(); }

QString EditorViewModel::sliceStatusLabel() const
{
  if (!sliceService_)
    return QStringLiteral("等待切片");
  if (sliceService_->slicing())
    return sliceService_->statusLabel();
  if (hasSliceResult())
    return sliceService_->statusLabel().isEmpty() ? QStringLiteral("切片完成") : sliceService_->statusLabel();

  return QStringLiteral("等待切片");
}

QString EditorViewModel::sliceEstimatedTime() const
{
  return hasSliceResult() ? m_sliceEstimatedTime : QString{};
}

QString EditorViewModel::sliceOutputPath() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->outputPath() : QString{};
}

QString EditorViewModel::sliceResultWeight() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->resultWeightLabel() : QString{};
}

QString EditorViewModel::sliceResultPlateLabel() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->resultPlateLabel() : QString{};
}

QString EditorViewModel::sliceResultFilament() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->resultFilamentLabel() : QString{};
}

QString EditorViewModel::sliceResultCost() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->resultCostLabel() : QString{};
}

int EditorViewModel::sliceResultLayerCount() const
{
  return hasSliceResult() && sliceService_ ? sliceService_->resultLayerCount() : 0;
}

QString EditorViewModel::modelSizeText() const
{
  if (m_measureDimensions.x() > 0)
    return QStringLiteral("%1 x %2 x %3 mm")
        .arg(m_measureDimensions.x(), 0, 'f', 1)
        .arg(m_measureDimensions.y(), 0, 'f', 1)
        .arg(m_measureDimensions.z(), 0, 'f', 1);
  return {};
}

QString EditorViewModel::avgPrintSpeed() const
{
  // 对齐上游 PrintEstimatedStatistics::modes[].time / filament_length
  if (!hasSliceResult())
    return {};
  // 简化计算：基于总时长和耗材长度估算平均速度
  const double totalSecs = m_sliceEstimatedTime.toDouble();
  const double totalMm = sliceService_ ? sliceService_->resultTotalFilamentMm() : 0.0;
  if (totalSecs <= 0 || totalMm <= 0)
    return {};
  // mm/min
  const double mmPerMin = (totalMm / 1000.0) / (totalSecs / 60.0);
  return QStringLiteral("%1 mm/s").arg(totalMm / totalSecs, 0, 'f', 1);
}

// ── 预估打印时间（对齐上游 PrintEstimatedStatistics::total_time）
QString EditorViewModel::estimatedPrintTime() const
{
  return sliceService_ ? sliceService_->estimatedTimeLabel() : QStringLiteral("--");
}

QString EditorViewModel::estimatePrintTimeForObject(int objectIndex) const
{
  Q_UNUSED(objectIndex);
  return estimatedPrintTime();
}

// ── 视口告警系统（对齐上游 EWarning / Plater::_set_warning_notification）──

int EditorViewModel::viewportWarning() const { return m_viewportWarning; }

QString EditorViewModel::viewportWarningMessage() const { return m_viewportWarningMessage; }

bool EditorViewModel::hasViewportWarning() const { return m_viewportWarning > 0; }

bool EditorViewModel::allPlatesSliced() const
{
  if (plateCount() <= 0)
    return false;
  for (int i = 0; i < plateCount(); ++i)
    if (!isPlateSliced(i))
      return false;
  return true;
}

void EditorViewModel::checkViewportWarnings()
{
  // 对齐上游 Plater::check_outside_state()
  // Mock 模式：基于对象位置检查是否超出热床范围
  // 假设热床尺寸为 220x220mm（K1C 默认），高度限制 250mm
  constexpr float bedW = 220.0f, bedD = 220.0f, maxH = 250.0f;
  constexpr float margin = 5.0f; // 超出容差

  bool outsideBed = false;
  bool beyondHeight = false;
  int outsideCount = 0;

  if (!projectService_)
    return;

  for (int i = 0; i < projectService_->modelCount(); ++i)
  {
    QVector3D pos = projectService_->objectPosition(i);
    QVector3D scl = projectService_->objectScale(i);
    // 简化：用缩放值作为对象尺寸估算（Mock 模式无真实 mesh 尺寸）
    const float halfW = scl.x() * 5.0f; // 估算半径
    const float halfD = scl.z() * 5.0f;

    // 检查 X/Y 范围（对象中心在热床范围内）
    const bool xOutside = (pos.x() - halfW < -margin) || (pos.x() + halfW > bedW + margin);
    const bool yOutside = (pos.y() - halfD < -margin) || (pos.y() + halfD > bedD + margin);

    if (xOutside || yOutside)
    {
      outsideCount++;
      outsideBed = true;
    }

    // 检查 Z 高度（超过打印体积高度）
    if (pos.z() > maxH + margin)
    {
      beyondHeight = true;
    }
  }

  if (outsideCount > 0)
  {
    m_viewportWarning = 2; // ObjectClashed (对齐上游)
    m_viewportWarningMessage = tr("%1 个对象超出热床范围。请将对象移至热床范围内。").arg(outsideCount);
  }
  else if (beyondHeight)
  {
    m_viewportWarning = 1; // ObjectOutside
    m_viewportWarningMessage = tr("对象超过最大打印高度 %.0fmm。").arg(maxH);
  }
  else
  {
    m_viewportWarning = 0;
    m_viewportWarningMessage.clear();
  }
}

int EditorViewModel::extruderCount() const
{
  // Mock mode: return 1 extruder unless slice result has filament data
  return hasSliceResult() ? 1 : 0;
}

QVariantList EditorViewModel::stalePlateIndices() const
{
  // Phase 52 PREPSB-05: surface the stale-plate set to QML so Preview/Export
  // can show a "stale -- reslice" indicator and disable export of stale results.
  // m_stalePlateIndices is maintained by invalidateSliceResultsForPlate /
  // invalidateAllSliceResults and cleared on a successful reslice.
  QVariantList list;
  for (int plateIndex : m_stalePlateIndices)
    list.append(plateIndex);
  return list;
}

bool EditorViewModel::hasStaleSliceResults() const
{
  // True when any previously-sliced plate result no longer matches current
  // inputs (a preset/scope/option changed after slicing). Kept SEPARATE from
  // canSlice: canSlice = "is there something to slice"; staleness = "is the
  // existing result out of date".
  return !m_stalePlateIndices.isEmpty();
}

QString EditorViewModel::extruderUsedLength(int extruderId) const
{
  if (!hasSliceResult() || extruderId != 0)
    return QString();
  return sliceService_ ? sliceService_->resultFilamentLabel() : QString();
}

QString EditorViewModel::extruderUsedWeight(int extruderId) const
{
  if (!hasSliceResult() || extruderId != 0)
    return QString();
  return sliceService_ ? sliceService_->resultWeightLabel() : QString();
}

bool EditorViewModel::hasSliceResult() const
{
  return projectService_ && hasValidSliceResultForPlate(projectService_->currentPlateIndex());
}

bool EditorViewModel::canRequestSlice() const
{
  if (!projectService_ || !sliceService_)
    return false;
  if (projectService_->loading() || sliceService_->slicing())
    return false;
  if (projectService_->sourceFilePath().isEmpty())
    return false;
  if (hasSliceResult())
    return false;

  return currentPlateHasPrintableObjects();
}

bool EditorViewModel::canPreview() const
{
  return hasSliceResult();
}

bool EditorViewModel::canExportGCode() const
{
  return hasSliceResult()
      && sliceService_
      && !sliceService_->outputPath().isEmpty()
      && sliceService_->sliceState() != SliceService::State::Exporting
      && !sliceService_->slicing();
}

QString EditorViewModel::sliceReadinessReason() const
{
  if (!projectService_)
    return {};
  if (sliceService_ && sliceService_->slicing())
    return QStringLiteral("当前切片任务进行中");
  if (projectService_->loading())
    return QStringLiteral("模型导入完成后可开始切片");
  if (projectService_->sourceFilePath().isEmpty())
    return QStringLiteral("请先导入模型文件");
  if (hasSliceResult())
    return QStringLiteral("当前平板已有有效切片结果；修改对象或参数后将重新启用切片");
  if (!currentPlateHasPrintableObjects())
    return QStringLiteral("当前平板没有可切片对象");
  if (plateSliceResultStatus(projectService_->currentPlateIndex()) == SliceResultStale)
    return QStringLiteral("当前平板切片结果已过期，请重新切片");
  return QStringLiteral("当前平板已满足基础切片条件");
}

QString EditorViewModel::previewActionHint() const
{
  if (!projectService_)
    return {};
  if (canPreview())
    return QStringLiteral("可预览当前平板切片结果");
  if (sliceService_ && sliceService_->slicing())
    return QStringLiteral("当前切片任务进行中");
  if (projectService_->loading())
    return QStringLiteral("模型导入完成后可预览");
  if (projectService_->sourceFilePath().isEmpty())
    return QStringLiteral("请先导入模型文件");
  if (!currentPlateHasPrintableObjects())
    return QStringLiteral("当前平板没有可预览的可打印对象");
  if (plateSliceResultStatus(projectService_->currentPlateIndex()) == SliceResultStale)
    return QStringLiteral("当前平板切片结果已过期，请重新切片");
  return QStringLiteral("当前平板尚未切片");
}

QString EditorViewModel::exportActionHint() const
{
  if (!projectService_)
    return {};
  if (canExportGCode())
    return QStringLiteral("可导出当前平板 G-code");
  if (sliceService_ && sliceService_->slicing())
    return QStringLiteral("当前切片任务进行中");
  if (projectService_->loading())
    return QStringLiteral("模型导入完成后可导出");
  if (projectService_->sourceFilePath().isEmpty())
    return QStringLiteral("请先导入模型文件");
  if (!currentPlateHasPrintableObjects())
    return QStringLiteral("当前平板没有可导出的可打印对象");
  if (plateSliceResultStatus(projectService_->currentPlateIndex()) == SliceResultStale)
    return QStringLiteral("当前平板切片结果已过期，请重新切片");
  if (hasSliceResult() && (!sliceService_ || sliceService_->outputPath().isEmpty()))
    return QStringLiteral("没有可导出的 G-code 输出路径");
  return QStringLiteral("当前平板尚未切片");
}

QString EditorViewModel::sliceActionLabel() const
{
  return QStringLiteral("▶ 开始切片");
}

QString EditorViewModel::sliceActionHint() const
{
  return sliceReadinessReason();
}

int EditorViewModel::plateSliceResultStatus(int plateIndex) const
{
  if (hasValidSliceResultForPlate(plateIndex))
    return SliceResultValid;
  if (m_stalePlateIndices.contains(plateIndex))
    return SliceResultStale;
  return SliceResultMissing;
}

// ---------- actions ----------
// v2.4 IO: 项目保存/导出转发到 ProjectService
bool EditorViewModel::saveProjectAs(const QString &filePath) const
{
    if (!projectService_) return false;
    return projectService_->saveProjectAs(filePath);
}

bool EditorViewModel::exportModel(const QString &filePath, const QString &format) const
{
    if (!projectService_) return false;
    return projectService_->exportModel(filePath, format);
}

void EditorViewModel::requestSlice()
{
  if (!canRequestSlice())
  {
    statusText_ = sliceActionHint();
    emit stateChanged();
    return;
  }

  // Inject merged preset config into SliceService before slicing
  // (对齐上游 PresetBundle::full_fff_config → BackgroundSlicingProcess)
  if (configViewModel_ && sliceService_)
  {
    sliceService_->setMergedPresetConfig(configViewModel_->mergedConfigValues());
    // v2.7 P0: ensure bed_shape is set (mirror CLI). If the merged preset
    // does not carry a bed, default to 220x220 so slicing has a valid bed.
    // setBedShape uses set_key_value(ConfigOptionPoints) directly, bypassing
    // the unreliable printable_area/set_deserialize_strict path.
    sliceService_->setBedShape({QPointF(0,0), QPointF(220,0), QPointF(220,220), QPointF(0,220)});
  }

  m_sliceEstimatedTime.clear();
  m_sliceResultPlateIndex = -1;
  sliceService_->startSlice(projectService_->projectName());
}

void EditorViewModel::cancelSlice()
{
  if (sliceService_)
    sliceService_->cancelSlice();
}

QString EditorViewModel::defaultExportGCodeFileName(int plateIndex) const
{
  return sliceService_ ? sliceService_->defaultExportGCodeFileName(plateIndex) : QStringLiteral("output.gcode");
}

bool EditorViewModel::loadFile(const QString &filePath)
{
  // Accept both local paths and file:// URLs (from QML FileDialog)
  QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  const bool started = projectService_->loadFile(localPath);
  if (started)
  {
    if (sliceService_)
      sliceService_->clearResults();
    m_sliceEstimatedTime.clear();
    m_sliceResultPlateIndex = -1;
    m_slicedPlateIndices.clear();
    m_stalePlateIndices.clear();
    statusText_ = QStringLiteral("正在加载...");
    m_collapsedGroupKeys.clear();
    m_collapsedObjectSourceIndices.clear();
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
    m_fitHint = QVector4D();
    m_cachedMeshData.clear();
    m_cachedMeshBatchSourceObjectIndices.clear();
  }
  else
  {
    statusText_ = projectService_->lastError();
  }
  emit stateChanged();
  return started;
}

void EditorViewModel::clearWorkspace()
{
  if (projectService_)
    projectService_->clearProject();
  if (sliceService_)
    sliceService_->clearResults();

  // 新建/清空工作区时清除撤销栈（对齐上游 UndoRedo::reset）
  clearUndoStack();

  m_objects.clear();
  m_selectedSourceIndices.clear();
  m_collapsedGroupKeys.clear();
  m_collapsedObjectSourceIndices.clear();
  m_primarySelectedSourceIndex = -1;
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  m_sliceEstimatedTime.clear();
  m_sliceResultPlateIndex = -1;
  m_slicedPlateIndices.clear();
  m_stalePlateIndices.clear();
  m_slicingAll = false;
  m_sliceAllQueue.clear();
  m_fitHint = QVector4D();
  m_cachedMeshData.clear();
  m_cachedMeshBatchSourceObjectIndices.clear();
  m_sliceAllQueue.clear();
  statusText_ = QStringLiteral("已新建项目");
  emit stateChanged();
}

void EditorViewModel::refreshAfterLoad()
{
  m_collapsedGroupKeys.clear();
  m_collapsedObjectSourceIndices.clear();
  m_selectedVolumeObjectSourceIndex = -1;
  m_selectedVolumeIndices.clear();
  m_selectedVolumeIndex = -1;
  m_fitHint = QVector4D();
  m_cachedMeshData.clear();
  m_cachedMeshBatchSourceObjectIndices.clear();
  m_slicedPlateIndices.clear();
  m_stalePlateIndices.clear();
  if (sliceService_)
    sliceService_->clearResults();
  rebuildObjectEntriesFromService();
  statusText_ = QStringLiteral("项目已加载");
  emit stateChanged();
}

void EditorViewModel::requestSliceAll()
{
  if (!projectService_ || projectService_->plateCount() <= 0)
    return;

  m_sliceAllQueue.clear();
  for (int i = 0; i < projectService_->plateCount(); ++i) {
    // D-08 (Phase 17): exclude locked AND non-printable plates from slice-all.
    if (!isPlateLocked(i) && projectService_->isPlatePrintable(i))
      m_sliceAllQueue.append(i);
  }
  if (m_sliceAllQueue.isEmpty())
    return;
  m_slicingAll = true;
  continueSliceAllQueue();
}

void EditorViewModel::continueSliceAllQueue()
{
  if (m_sliceAllQueue.isEmpty())
  {
    m_slicingAll = false;
    statusText_ = tr("全部平板切片完成");
    emit stateChanged();
    return;
  }
  const int plate = m_sliceAllQueue.takeFirst();
  statusText_ = tr("正在切片平板 %1/%2...").arg(plate + 1).arg(plate + m_sliceAllQueue.size() + 1);
  emit stateChanged();
  if (sliceService_)
    sliceService_->startSlicePlate(plate);
}

bool EditorViewModel::requestExportGCode(const QString &targetPath)
{
  if (!sliceService_)
    return false;
  if (!canExportGCode())
  {
    statusText_ = exportActionHint();
    emit stateChanged();
    return false;
  }
  QUrl url(targetPath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : targetPath;
  const bool ok = sliceService_->exportGCodeToPath(localPath);
  statusText_ = sliceService_->statusLabel();
  emit stateChanged();
  return ok;
}

bool EditorViewModel::requestExportAllGCode(const QString &directoryPath, const QString &baseName)
{
  if (!sliceService_)
    return false;
  if (sliceService_->slicing())
  {
    statusText_ = exportActionHint();
    emit stateChanged();
    return false;
  }

  QUrl url(directoryPath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : directoryPath;
  const bool ok = sliceService_->exportAllPlateGCodeToDirectory(localPath, baseName);
  statusText_ = sliceService_->statusLabel();
  emit stateChanged();
  return ok;
}

// --- Arrange settings (对齐上游 ArrangeSettings) ---
float EditorViewModel::arrangeDistance() const { return m_arrangeDistance; }
void EditorViewModel::setArrangeDistance(float v)
{
  if (!qFuzzyCompare(m_arrangeDistance, v))
  {
    m_arrangeDistance = v;
    emit stateChanged();
  }
}
bool EditorViewModel::arrangeRotation() const { return m_arrangeRotation; }
void EditorViewModel::setArrangeRotation(bool v)
{
  if (m_arrangeRotation != v)
  {
    m_arrangeRotation = v;
    emit stateChanged();
  }
}
bool EditorViewModel::arrangeAlignY() const { return m_arrangeAlignY; }
void EditorViewModel::setArrangeAlignY(bool v)
{
  if (m_arrangeAlignY != v)
  {
    m_arrangeAlignY = v;
    emit stateChanged();
  }
}
bool EditorViewModel::arrangeMultiMaterial() const { return m_arrangeMultiMaterial; }
void EditorViewModel::setArrangeMultiMaterial(bool v)
{
  if (m_arrangeMultiMaterial != v)
  {
    m_arrangeMultiMaterial = v;
    emit stateChanged();
  }
}
bool EditorViewModel::arrangeAvoidCalibration() const { return m_arrangeAvoidCalibration; }
void EditorViewModel::setArrangeAvoidCalibration(bool v)
{
  if (m_arrangeAvoidCalibration != v)
  {
    m_arrangeAvoidCalibration = v;
    emit stateChanged();
  }
}
void EditorViewModel::resetArrangeSettings()
{
  m_arrangeDistance = 0.f;
  m_arrangeRotation = false;
  m_arrangeAlignY = false;
  m_arrangeMultiMaterial = true;
  m_arrangeAvoidCalibration = true;
  emit stateChanged();
}

void EditorViewModel::arrangeAllObjects()
{
  if (!projectService_)
    return;
  // Use upstream arrange_objects if available, fall back to 6mm default spacing
  const float spacing = (m_arrangeDistance > 0.f) ? m_arrangeDistance : 6.0f;
  // Get real bed shape from printer preset config (对齐上游 ArrangeJob get_bed_shape)
  QString printableArea;
  if (configViewModel_)
  {
    const auto merged = configViewModel_->mergedConfigValues();
    printableArea = merged.value(QStringLiteral("printable_area")).toString();
  }
  if (projectService_->arrangeObjects(spacing, m_arrangeRotation, m_arrangeAlignY, printableArea))
  {
    // Real arrange succeeded — sync transforms from model to mock arrays
    projectService_->syncTransformsFromModel();
    invalidateAllSliceResults();
    emit stateChanged();
  }
  // If real arrange is not available (no HAS_LIBSLIC3R), the viewport fallback
  // arrange path remains a no-op rather than mutating model state in QML.
}

bool EditorViewModel::canActivateGizmo(int gizmoMode) const
{
  if (!projectService_)
    return false;

  const bool hasSingleObject = !hasSelectedVolume() && m_selectedSourceIndices.size() == 1;

  switch (gizmoMode)
  {
  case 0: // Move
  case 1: // Rotate
  case 2: // Scale
  case 4: // Flatten
  case 5: // Cut
  case 9: // Simplify
  case 14: // Advanced Cut
    return hasSingleObject;
  case 3: // Measure
    return projectService_->modelCount() > 0;
  case 6: // Support paint
  case 7: // Seam paint
    return kViewportTrianglePickingAvailable && hasSingleObject;
  case 11: // Drill
    return kCgalMeshBooleanAvailable && hasSingleObject;
  case 12: // Emboss
  case 16: // Text
  case 17: // SVG
    return hasSingleObject;
  case 13: // Mesh Boolean
    return kCgalMeshBooleanAvailable && !hasSelectedVolume() && m_selectedSourceIndices.size() == 2;
  case 8: // Hollow
  case 10: // MMU segmentation
  case 15: // Face detector
  case 18: // SLA supports
    return false;
  default:
    return false;
  }
}

QString EditorViewModel::gizmoStatusText(int gizmoMode) const
{
  if (!projectService_)
    return QStringLiteral("Backend unavailable");

  if (canActivateGizmo(gizmoMode))
    return QStringLiteral("Ready");

  switch (gizmoMode)
  {
  case 0:
  case 1:
  case 2:
  case 4:
  case 5:
  case 9:
  case 12:
  case 14:
  case 16:
  case 17:
    return QStringLiteral("Requires one selected object");
  case 6:
  case 7:
    return QStringLiteral("Blocked: viewport triangle picking unavailable");
  case 11:
    return QStringLiteral("Blocked: CGAL MeshBoolean unavailable");
  case 3:
    return QStringLiteral("Requires a loaded model");
  case 13:
    return QStringLiteral("Blocked: CGAL MeshBoolean unavailable");
  case 8:
  case 18:
    return QStringLiteral("Blocked: OpenVDB unavailable");
  case 10:
  case 15:
    return QStringLiteral("Not yet backed by a real Qt workflow");
  default:
    return QStringLiteral("Unsupported tool");
  }
}

void EditorViewModel::switchToPreview()
{
  if (!canPreview())
  {
    statusText_ = previewActionHint();
    emit stateChanged();
    return;
  }
  emit previewRequested();
}

// ── Bed shape (对齐上游 BedShapeDialog / bed_shape config) ──────────────────

float EditorViewModel::bedWidth() const { return m_bedWidth; }
void EditorViewModel::setBedWidth(float v)
{
  v = qBound(10.0f, v, 2000.0f);
  if (qFuzzyCompare(m_bedWidth, v)) return;
  m_bedWidth = v;
  QSettings s; s.setValue(QStringLiteral("bed/width"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

float EditorViewModel::bedDepth() const { return m_bedDepth; }
void EditorViewModel::setBedDepth(float v)
{
  v = qBound(10.0f, v, 2000.0f);
  if (qFuzzyCompare(m_bedDepth, v)) return;
  m_bedDepth = v;
  QSettings s; s.setValue(QStringLiteral("bed/depth"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

float EditorViewModel::bedMaxHeight() const { return m_bedMaxHeight; }
void EditorViewModel::setBedMaxHeight(float v)
{
  v = qBound(1.0f, v, 5000.0f);
  if (qFuzzyCompare(m_bedMaxHeight, v)) return;
  m_bedMaxHeight = v;
  QSettings s; s.setValue(QStringLiteral("bed/maxHeight"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

float EditorViewModel::bedOriginX() const { return m_bedOriginX; }
void EditorViewModel::setBedOriginX(float v)
{
  if (qFuzzyCompare(m_bedOriginX, v)) return;
  m_bedOriginX = v;
  QSettings s; s.setValue(QStringLiteral("bed/originX"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

float EditorViewModel::bedOriginY() const { return m_bedOriginY; }
void EditorViewModel::setBedOriginY(float v)
{
  if (qFuzzyCompare(m_bedOriginY, v)) return;
  m_bedOriginY = v;
  QSettings s; s.setValue(QStringLiteral("bed/originY"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

int EditorViewModel::bedShapeType() const { return m_bedShapeType; }
void EditorViewModel::setBedShapeType(int v)
{
  v = qBound(0, v, 2);
  if (m_bedShapeType == v) return;
  m_bedShapeType = v;
  QSettings s; s.setValue(QStringLiteral("bed/shapeType"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

float EditorViewModel::bedDiameter() const { return m_bedDiameter; }
void EditorViewModel::setBedDiameter(float v)
{
  v = qBound(1.0f, v, 2000.0f);
  if (qFuzzyCompare(m_bedDiameter, v)) return;
  m_bedDiameter = v;
  QSettings s; s.setValue(QStringLiteral("bed/diameter"), v);
  invalidateAllSliceResults();
  emit bedShapeChanged();
  emit stateChanged();
}

// ── Phase 100 (WTREAD-01): wipe-tower geometry readback from SliceService ──
//
// onWipeTowerGeometryReady applies the has_wipe_tower() gate (WTREAD-02): when
// geometry.valid is false (single-material / enable_prime_tower off), the
// renderer must show no wipe-tower — m_showWipeTower is forced to false and
// the dim members are left untouched so no placeholder 10/10/50/100/25 box is
// pushed as "real" geometry. When geometry.valid is true, the real dims from
// Print::wipe_tower_data() (captured by value in the SliceService worker,
// Frozen Decision 1) overwrite the placeholder defaults.
void EditorViewModel::onWipeTowerGeometryReady(const WipeTowerGeometry &geometry)
{
  if (geometry.valid) {
    m_wipeTowerWidth = geometry.width;
    m_wipeTowerDepth = geometry.depth;
    m_wipeTowerHeight = geometry.height;
    m_wipeTowerX = geometry.x;
    m_wipeTowerZ = geometry.z;
    m_showWipeTower = true;
    // Phase 109 (WTMESH-01/02): mirror the Option B real-mesh readback. The
    // worker populates hasRealMesh + meshVertices ONLY when the engine
    // produced wipe_tower_mesh_data (multi-material post-slice); otherwise
    // hasRealMesh stays false and the renderer takes the Option A fallback.
    // The mesh vertices are flattened XYZ triples (libslic3r world frame);
    // the getter converts to QVariantList for the QML binding.
    m_wipeTowerHasRealMesh = geometry.hasRealMesh;
    m_wipeTowerMeshVertices = geometry.meshVertices;
  } else {
    // WTREAD-02 gate: do NOT overwrite width/depth/height/x/z with placeholder
    // values. They are irrelevant while show=false; leaving them at their
    // prior values guarantees no placeholder box leaks as "real" geometry.
    m_showWipeTower = false;
    // Phase 109 (WTMESH-02): force Option A on the invalid path so a stale
    // real-mesh from a prior multi-material slice cannot leak through a
    // single-material re-slice. The renderer's hasRealMesh gate is only
    // meaningful while showWipeTower is true, but clearing it here keeps the
    // invariant tight: invalid readback => pure Option A state.
    m_wipeTowerHasRealMesh = false;
    m_wipeTowerMeshVertices.clear();
  }
  emit wipeTowerGeometryChanged();
}

bool EditorViewModel::showWipeTower() const { return m_showWipeTower; }
float EditorViewModel::wipeTowerWidth() const { return m_wipeTowerWidth; }
float EditorViewModel::wipeTowerDepth() const { return m_wipeTowerDepth; }
float EditorViewModel::wipeTowerHeight() const { return m_wipeTowerHeight; }
float EditorViewModel::wipeTowerX() const { return m_wipeTowerX; }
float EditorViewModel::wipeTowerZ() const { return m_wipeTowerZ; }
bool EditorViewModel::wipeTowerHasRealMesh() const { return m_wipeTowerHasRealMesh; }
QVariantList EditorViewModel::wipeTowerMeshVertices() const
{
  QVariantList out;
  out.reserve(int(m_wipeTowerMeshVertices.size()));
  for (float v : m_wipeTowerMeshVertices)
    out.append(v);
  return out;
}

// ── Phase 108 (FMAP-01): filament-map auto-recommendation readback ──
//
// onFilamentMapReady applies the valid gate (mirrors the WTREAD-02 gate on
// onWipeTowerGeometryReady): when result.valid is false (user picked Manual,
// so the engine computed no auto-map at Print.cpp:2484-2491 -- the
// mode < fmmManual branch did not fire), m_hasAutoFilamentMap is forced to
// false and the maps/mode members are left untouched so no stale map leaks to
// the Phase 110 UI. When result.valid is true, the real auto recommendation
// from Print::get_filament_maps() (captured by value in the SliceService
// worker, Frozen Decision 1) overwrites the defaults.
void EditorViewModel::onFilamentMapReady(const FilamentMapResult &result)
{
  if (result.valid) {
    m_autoFilamentMapMode = static_cast<int>(result.mode);
    QVariantList maps;
    maps.reserve(static_cast<int>(result.maps.size()));
    for (const int id : result.maps)
      maps.append(id);
    m_autoFilamentMaps = std::move(maps);
    m_hasAutoFilamentMap = true;
  } else {
    // FMAP-01 gate: do NOT overwrite maps/mode with placeholder values. They
    // are irrelevant while hasAuto=false; leaving them at their prior values
    // guarantees no stale map leaks as a "fresh" recommendation.
    m_hasAutoFilamentMap = false;
  }
  emit filamentMapChanged();
}

bool EditorViewModel::hasAutoFilamentMap() const { return m_hasAutoFilamentMap; }
int EditorViewModel::autoFilamentMapMode() const { return m_autoFilamentMapMode; }
QVariantList EditorViewModel::autoFilamentMaps() const { return m_autoFilamentMaps; }
