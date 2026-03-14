#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include <QFileInfo>
#include <QUrl>
#include <QVector4D>
#include <algorithm>
#include <QSet>
#include <QTimer>
#include <cstring> // memcpy
#include <cmath>

void EditorViewModel::refreshMeshCacheAndFitHint()
{
  m_cachedMeshData = projectService_->meshData();
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
}

// ── Object manipulation (对齐上游 ObjectManipulation) ──────────────────────

static int primarySelectedSourceIndex(const EditorViewModel *vm)
{
  // Use single object selection for manipulation; fall back to primary
  if (!vm) return -1;
  if (vm->selectedObjectCount() == 1)
    return vm->selectedObjectIndex();
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

void EditorViewModel::setObjectPosX(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectPosition(idx, v, objectPosY(), objectPosZ()); }
void EditorViewModel::setObjectPosY(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectPosition(idx, objectPosX(), v, objectPosZ()); }
void EditorViewModel::setObjectPosZ(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectPosition(idx, objectPosX(), objectPosY(), v); }
void EditorViewModel::setObjectRotX(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectRotation(idx, v, objectRotY(), objectRotZ()); }
void EditorViewModel::setObjectRotY(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectRotation(idx, objectRotX(), v, objectRotZ()); }
void EditorViewModel::setObjectRotZ(float v) { const int idx = primarySelectedSourceIndex(this); if (idx >= 0) projectService_->setObjectRotation(idx, objectRotX(), objectRotY(), v); }
void EditorViewModel::setObjectScaleX(float v) {
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0) return;
  if (m_uniformScale) projectService_->setObjectScaleUniform(idx, v);
  else projectService_->setObjectScale(idx, v, objectScaleY(), objectScaleZ());
}
void EditorViewModel::setObjectScaleY(float v) {
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0) return;
  if (m_uniformScale) projectService_->setObjectScaleUniform(idx, v);
  else projectService_->setObjectScale(idx, objectScaleX(), v, objectScaleZ());
}
void EditorViewModel::setObjectScaleZ(float v) {
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0) return;
  if (m_uniformScale) projectService_->setObjectScaleUniform(idx, v);
  else projectService_->setObjectScale(idx, objectScaleX(), objectScaleY(), v);
}
void EditorViewModel::setUniformScale(bool v) { m_uniformScale = v; emit stateChanged(); }

void EditorViewModel::resetObjectTransform()
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_) return;
  projectService_->setObjectPosition(idx, 0, 0, 0);
  projectService_->setObjectRotation(idx, 0, 0, 0);
  projectService_->setObjectScaleUniform(idx, 1);
}

void EditorViewModel::applyScaleFactor(float factor)
{
  const int idx = primarySelectedSourceIndex(this);
  if (idx < 0 || !projectService_) return;
  const QVector3D cur = projectService_->objectScale(idx);
  projectService_->setObjectScale(idx, cur.x() * factor, cur.y() * factor, cur.z() * factor);
}

// --- Flatten / Cut (对齐上游 GLGizmoFlatten / GLGizmoCut) ---

void EditorViewModel::setCutAxis(int axis) { m_cutAxis = axis; emit stateChanged(); }
void EditorViewModel::setCutPosition(double pos) { m_cutPosition = pos; emit stateChanged(); }
void EditorViewModel::setCutKeepMode(int mode) { m_cutKeepMode = mode; emit stateChanged(); }

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
  if (idx < 0 || !projectService_) return;
  const QVector3D pos = projectService_->objectPosition(idx);
  m_cutPosition = (m_cutAxis == 0) ? pos.x() : (m_cutAxis == 1) ? pos.y() : pos.z();
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

  QTimer::singleShot(400, this, [this]() {
    // Mock: 重置旋转到使 Z 面朝下
    const int idx = *m_selectedSourceIndices.constBegin();
    if (projectService_)
      projectService_->setObjectRotation(idx, 0, 0, 0);
    statusText_ = tr("已平放至最大面（Mock，6 个候选面）");
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  });
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

#ifdef HAS_LIBSLIC3R
  // TODO: 调用 ModelObject::cut() 执行真实网格切割
  // 1. 构建切割平面 (axis + position)
  // 2. ModelObject::cut(cut_plane, keep_upper, keep_lower)
  // 3. 替换原对象为切割后的对象
  // 4. 设置 undo/redo 快照 "Cut"
#endif

  // Mock mode: 将对象沿指定轴切割为上下两部分
  const int newIdx = projectService_->addObject(origName + tr("_upper"));
  if (newIdx >= 0)
  {
    // Mock: 缩放上半部分和下半部分
    projectService_->setObjectPosition(newIdx,
      projectService_->objectPosition(srcIdx).x() + 2,
      projectService_->objectPosition(srcIdx).y(),
      projectService_->objectPosition(srcIdx).z() + (axis == 2 ? 5 : 0));

    if (m_cutKeepMode != 2)  // 保留上半
    {
      m_selectedSourceIndices.clear();
      m_selectedSourceIndices.insert(srcIdx);
      m_selectedSourceIndices.insert(newIdx);
    }
    else  // 仅保留下半
    {
      m_selectedSourceIndices.clear();
      m_selectedSourceIndices.insert(srcIdx);
    }
    statusText_ = tr("已切割为 2 个部分（Mock）");
  }
  else
  {
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

int EditorViewModel::mapFilteredToSourceIndex(int filteredIndex) const
{
  const QList<int> indices = visibleObjectIndices();
  if (filteredIndex < 0 || filteredIndex >= indices.size())
    return -1;
  return indices[filteredIndex];
}

EditorViewModel::EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent)
    : QObject(parent), projectService_(projectService), sliceService_(sliceService)
{
  connect(projectService_, &ProjectServiceMock::projectChanged, this, [this]()
          {
    if (!sliceService_ || !sliceService_->slicing())
      m_sliceResultPlateIndex = -1;
    // Invalidate slice results for the affected plate
    m_slicedPlateIndices.remove(projectService_ ? projectService_->currentPlateIndex() : -1);
        statusText_ = QStringLiteral("已更新项目对象");
        emit stateChanged(); });

  connect(projectService_, &ProjectServiceMock::plateSelectionChanged, this, [this]()
          {
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
    if (m_sliceResultPlateIndex >= 0)
      m_slicedPlateIndices.insert(m_sliceResultPlateIndex);
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
    }
  });
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
    }
    emit stateChanged(); });
}

QString EditorViewModel::projectName() const { return projectService_ ? projectService_->projectName() : QString(); }
int EditorViewModel::modelCount() const { return projectService_ ? projectService_->modelCount() : 0; }
int EditorViewModel::plateCount() const { return projectService_ ? projectService_->plateCount() : 0; }
int EditorViewModel::currentPlateIndex() const { return projectService_ ? projectService_->currentPlateIndex() : 0; }
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

  const int sourceIndex = selectedObjectIndex();
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

  if (!projectService_ || !projectService_->setObjectPrintable(sourceIndex, printable))
  {
    statusText_ = projectService_ ? projectService_->lastError() : QStringLiteral("服务不可用");
    emit stateChanged();
    return;
  }

  m_objects[sourceIndex].printable = printable;
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

  if (projectService_->deleteObject(sourceIndex))
  {
    m_collapsedObjectSourceIndices.clear();
    rebuildObjectEntriesFromService();
    m_selectedSourceIndices.clear();
    ensureValidObjectSelection(true);
    refreshMeshCacheAndFitHint();
    statusText_ = QStringLiteral("已删除对象，剩余 %1 个模型").arg(projectService_->modelCount());
    emit stateChanged();
  }
  else
  {
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
    const int sourceIndex = mapFilteredToSourceIndex(selectedObjectIndex());
    if (sourceIndex >= 0)
      sourceIndices.append(sourceIndex);
  }

  if (sourceIndices.isEmpty())
    return;

  std::sort(sourceIndices.begin(), sourceIndices.end(), std::greater<int>());
  sourceIndices.erase(std::unique(sourceIndices.begin(), sourceIndices.end()), sourceIndices.end());

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
  if (m_selectedSourceIndices.isEmpty())
    return;

  for (int sourceIndex : m_selectedSourceIndices)
  {
    if (sourceIndex >= 0 && sourceIndex < m_objects.size())
    {
      if (!projectService_ || !projectService_->setObjectPrintable(sourceIndex, printable))
      {
        statusText_ = projectService_ ? projectService_->lastError() : QStringLiteral("服务不可用");
        emit stateChanged();
        return;
      }
      m_objects[sourceIndex].printable = printable;
    }
  }

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
  if (!projectService_)
    return;

  // 收集所有选中的源索引（按从大到小排列，避免插入导致索引偏移问题）
  QList<int> selected = m_selectedSourceIndices.values();
  std::sort(selected.begin(), selected.end(), std::greater<int>());

  if (selected.isEmpty())
    return;

  // 逐个复制（每次调用后元数据会更新，但源索引在首次插入后仍然有效
  // 因为 duplicateObject 在 sourceIndex+1 插入，不影响 sourceIndex 本身）
  bool anySuccess = false;
  for (int srcIdx : selected)
  {
    const int newIdx = projectService_->duplicateObject(srcIdx);
    if (newIdx >= 0)
      anySuccess = true;
  }

  if (anySuccess)
  {
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  }
}

bool EditorViewModel::hasClipboardContent() const
{
  return !m_clipboard.isEmpty();
}

void EditorViewModel::copySelectedObjects()
{
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
  for (const auto &entry : m_clipboard)
  {
    const int newIdx = projectService_->addObject(entry.name);
    if (newIdx >= 0)
      m_selectedSourceIndices.insert(newIdx);
  }
  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
  emit stateChanged();
}

void EditorViewModel::cutSelectedObjects()
{
  copySelectedObjects();
  deleteSelectedObjects();
}

void EditorViewModel::toggleSelectedObjectsVisibility()
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
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
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
    return;

  // 对齐上游 Plater::orient() + OrientJob
  // Mock 模式：对每个选中对象设置朝向标记（上游会在切片时应用最优旋转）
  // 真实模式：OrientJob 会调用 orientation::orient() 计算最优旋转
  statusText_ = tr("正在计算最优朝向...");
  emit stateChanged();

#ifdef HAS_LIBSLIC3R
  // TODO: 创建 OrientJob 并提交到线程池
  // wxGetApp().plater()->orient() equivalent
#endif

  // Mock mode: signal completion after brief delay
  QTimer::singleShot(300, this, [this]() {
    statusText_ = tr("自动朝向完成（Mock）");
    rebuildObjectEntriesFromService();
    refreshMeshCacheAndFitHint();
    emit stateChanged();
  });
}

void EditorViewModel::splitSelectedObject()
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
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
  // TODO: 调用 ModelObject::split() 拆分网格
  // 1. current_model_object->split(&new_objects)
  // 2. 如果只有一个结果，提示"无法拆分"
  // 3. 移除原对象，加载新对象
  // 4. 设置 undo/redo 快照 "Split to Objects"
#endif

  // Mock mode: 复制选中对象为 2 个子对象模拟拆分
  const QString origName = (srcIdx >= 0 && srcIdx < m_objects.size())
                               ? m_objects[srcIdx].name
                               : tr("对象");
  const int newIdx = projectService_->addObject(origName + tr("_part1"));
  if (newIdx >= 0)
  {
    m_selectedSourceIndices.clear();
    m_selectedSourceIndices.insert(srcIdx);
    m_selectedSourceIndices.insert(newIdx);
    statusText_ = tr("已拆分为 2 个部件（Mock）");
  }
  else
  {
    statusText_ = tr("拆分失败");
  }

  rebuildObjectEntriesFromService();
  refreshMeshCacheAndFitHint();
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
  if (!projectService_) return {};
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
  if (!projectService_)
    return false;
  if (projectService_->addPlate())
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
    return true;
  }
  return false;
}

bool EditorViewModel::deletePlate(int plateIndex)
{
  if (!projectService_)
    return false;
  if (projectService_->deletePlate(plateIndex))
  {
    rebuildObjectEntriesFromService();
    emit stateChanged();
    return true;
  }
  return false;
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
  emit stateChanged();
}

bool EditorViewModel::isPlateSliced(int plateIndex) const
{
  return m_slicedPlateIndices.contains(plateIndex);
}

bool EditorViewModel::moveSelectedObjectToPlate(int targetPlateIndex)
{
  // 对齐上游 Plater::priv::on_arrange 跨平板拖拽
  if (!projectService_)
    return false;

  const int srcPlate = currentPlateIndex();
  if (srcPlate < 0 || targetPlateIndex < 0 || srcPlate == targetPlateIndex)
    return false;

  const int selObj = selectedObjectIndex();
  if (selObj < 0)
    return false;

  // Mock 模式：更新对象的平板归属
  projectService_->setObjectPlateForIndex(selObj, targetPlateIndex);
  emit stateChanged();
  return true;
}

QString EditorViewModel::plateThumbnailColor(int plateIndex) const
{
  // Mock 模式：基于平板索引生成独特颜色（对齐上游 PartPlate thumbnail 视觉区分）
  static const char *kColors[] = {
    "#1a3a5c", "#2a3a2c", "#3a2a3c", "#1c3c3a", "#3c3a1a",
    "#2a1c3c", "#1c2a3c", "#3c2a1a", "#1a3c2c", "#2c3a1c"
  };
  const int count = static_cast<int>(std::size(kColors));
  return QString::fromUtf8(kColors[plateIndex % count]);
}

bool EditorViewModel::renameObject(int index, const QString &newName)
{
  if (!projectService_ || index < 0)
    return false;
  projectService_->renameObject(index, newName);
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
  if (!projectService_) return false;
  bool ok = projectService_->setPlateBedType(plateIndex, bedType);
  if (ok) emit stateChanged();
  return ok;
}

int EditorViewModel::platePrintSequence(int plateIndex) const
{
  return projectService_ ? projectService_->platePrintSequence(plateIndex) : 0;
}

bool EditorViewModel::setPlatePrintSequence(int plateIndex, int seq)
{
  if (!projectService_) return false;
  bool ok = projectService_->setPlatePrintSequence(plateIndex, seq);
  if (ok) emit stateChanged();
  return ok;
}

int EditorViewModel::plateSpiralMode(int plateIndex) const
{
  return projectService_ ? projectService_->plateSpiralMode(plateIndex) : 0;
}

bool EditorViewModel::setPlateSpiralMode(int plateIndex, int mode)
{
  if (!projectService_) return false;
  bool ok = projectService_->setPlateSpiralMode(plateIndex, mode);
  if (ok) emit stateChanged();
  return ok;
}

void EditorViewModel::centerSelectedObjects()
{
  // 对齐上游 Plater::priv::on_center / ModelObject::center_instances
  // Mock 模式：重置选中对象位置到原点
  if (!projectService_)
    return;
  const int count = objectCount();
  for (int i = 0; i < count; ++i) {
    if (isObjectSelected(i)) {
      projectService_->setObjectPosition(i, 0, 0, 0);
    }
  }
  emit stateChanged();
}

void EditorViewModel::fillBedWithCopies()
{
  // 对齐上游 Plater::priv::on_fill_bed
  // Mock 模式：生成 9 个副本（3x3 网格）
  if (!projectService_)
    return;
  const int sel = selectedObjectIndex();
  if (sel < 0)
    return;
  const QString name = projectService_->objectNames().value(sel, "Object");
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      if (r == 0 && c == 0)
        continue;
      const int idx = projectService_->addObject(name + "_" + QString::number(r * 3 + c + 1));
      if (idx >= 0)
        projectService_->setObjectPosition(idx, (c - 1) * 60.0f, (r - 1) * 60.0f, 0);
    }
  }
  emit stateChanged();
}

void EditorViewModel::exportSelectedAsStl()
{
  // 对齐上游 Plater::priv::on_export_stl
  // Mock 模式：仅通知用户（实际 STL 导出需要 libslic3r mesh 序列化）
  if (!projectService_)
    return;
  const int sel = selectedObjectIndex();
  if (sel >= 0) {
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

int EditorViewModel::extruderCount() const
{
  // Mock mode: return 1 extruder unless slice result has filament data
  return hasSliceResult() ? 1 : 0;
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
  return sliceService_ && !sliceService_->outputPath().isEmpty() && !isSlicing() && projectService_ && m_sliceResultPlateIndex == projectService_->currentPlateIndex();
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

QString EditorViewModel::sliceActionLabel() const
{
  return QStringLiteral("▶ 开始切片");
}

QString EditorViewModel::sliceActionHint() const
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

  return QStringLiteral("当前平板已满足基础切片条件");
}

// ---------- actions ----------
void EditorViewModel::requestSlice()
{
  if (!canRequestSlice())
  {
    statusText_ = sliceActionHint();
    emit stateChanged();
    return;
  }

  m_sliceEstimatedTime.clear();
  m_sliceResultPlateIndex = -1;
  sliceService_->startSlice(projectService_->projectName());
}

void EditorViewModel::cancelSlice()
{
  if (sliceService_) sliceService_->cancelSlice();
}

bool EditorViewModel::loadFile(const QString &filePath)
{
  // Accept both local paths and file:// URLs (from QML FileDialog)
  QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  m_sliceEstimatedTime.clear();
  m_sliceResultPlateIndex = -1;
  const bool started = projectService_->loadFile(localPath);
  if (started)
  {
    statusText_ = QStringLiteral("正在加载...");
    m_collapsedGroupKeys.clear();
    m_collapsedObjectSourceIndices.clear();
    m_selectedVolumeObjectSourceIndex = -1;
    m_selectedVolumeIndices.clear();
    m_selectedVolumeIndex = -1;
    m_fitHint = QVector4D();
    m_cachedMeshData.clear();
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
  m_slicingAll = false;
  m_sliceAllQueue.clear();
  m_fitHint = QVector4D();
  m_cachedMeshData.clear();
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
  m_slicedPlateIndices.clear();
  rebuildObjectEntriesFromService();
  statusText_ = QStringLiteral("项目已加载");
  emit stateChanged();
}

void EditorViewModel::requestSliceAll()
{
  if (!projectService_ || projectService_->plateCount() <= 0)
    return;

  m_sliceAllQueue.clear();
  for (int i = 0; i < projectService_->plateCount(); ++i)
    m_sliceAllQueue.append(i);
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
  if (sliceService_) sliceService_->startSlicePlate(plate);
}

bool EditorViewModel::requestExportGCode(const QString &targetPath)
{
  if (!sliceService_) return false;
  QUrl url(targetPath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : targetPath;
  return sliceService_->exportGCodeToPath(localPath);
}

void EditorViewModel::switchToPreview()
{
  emit previewRequested();
}
