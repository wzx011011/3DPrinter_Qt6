#include "EditorViewModel.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/UndoCommands.h"
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

  // 检查视口告警（对齐上游 Plater::check_outside_state / EWarning）
  checkViewportWarnings();
}

// ── Object manipulation (对齐上游 ObjectManipulation) ──────────────────────

static int primarySelectedSourceIndex(const EditorViewModel *vm)
{
  // Use single object selection for manipulation; fall back to primary
  if (!vm)
    return -1;
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
      if (tri.state != Crality3D::SupportPaintState::None) ++total;
  return total;
}

void EditorViewModel::setTriangleSupportState(int objectIndex, int triangleIndex, int paintState)
{
  // 查找或创建 ObjectPaintData 条目
  for (auto &obj : m_paintData) {
    if (obj.objectIndex == objectIndex) {
      obj.setTriangleState(triangleIndex,
                           static_cast<Crality3D::SupportPaintState>(paintState));
      emit paintDataChanged();
      return;
    }
  }
  // 新对象条目
  Crality3D::ObjectPaintData newObj;
  newObj.objectIndex = objectIndex;
  newObj.setTriangleState(triangleIndex,
                           static_cast<Crality3D::SupportPaintState>(paintState));
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
  if (idx < 0 || !projectService_)
    return {};

  const QString name = objectName(idx);
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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
  const int idx = selectedObjectIndex();
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

int EditorViewModel::mapFilteredToSourceIndex(int filteredIndex) const
{
  const QList<int> indices = visibleObjectIndices();
  if (filteredIndex < 0 || filteredIndex >= indices.size())
    return -1;
  return indices[filteredIndex];
}

// ── Undo/Redo integration (对齐上游 UndoRedo 框架) ──────────────────────────

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
  emit stateChanged();
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
    } });
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

int EditorViewModel::volumeExtruderId(int objectIndex, int volumeIndex) const
{
  if (!projectService_)
    return -1;
  return projectService_->volumeExtruderId(objectIndex, volumeIndex);
}

bool EditorViewModel::setVolumeExtruderId(int objectIndex, int volumeIndex, int extruderId)
{
  if (!projectService_)
  {
    statusText_ = QStringLiteral("无法设置耗材：服务未就绪");
    emit stateChanged();
    return false;
  }

  if (!projectService_->setVolumeExtruderId(objectIndex, volumeIndex, extruderId))
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
  bool ok = projectService_->addVolumeFromFile(objectIndex, filePath, volumeType);
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
  bool ok = projectService_->addPrimitive(objectIndex, primitiveType);
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
  bool ok = projectService_->addTextVolume(objectIndex, text);
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
  bool ok = projectService_->addSvgVolume(objectIndex, svgFilePath);
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

  // Create undo command BEFORE deleting (captures snapshots)
  DeleteObjectsCommand *deleteCmd = nullptr;
  if (m_undoManager)
    deleteCmd = new DeleteObjectsCommand({sourceIndex}, projectService_, this);

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
    const int sourceIndex = mapFilteredToSourceIndex(selectedObjectIndex());
    if (sourceIndex >= 0)
      sourceIndices.append(sourceIndex);
  }

  if (sourceIndices.isEmpty())
    return;

  std::sort(sourceIndices.begin(), sourceIndices.end(), std::greater<int>());
  sourceIndices.erase(std::unique(sourceIndices.begin(), sourceIndices.end()), sourceIndices.end());

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
  emit stateChanged();
}

void EditorViewModel::mirrorSelectedObjects(int axis)
{
  if (!projectService_ || m_selectedSourceIndices.isEmpty())
    return;

#ifdef HAS_LIBSLIC3R
  // Sync real model mirror state
  for (int srcIdx : m_selectedSourceIndices)
    projectService_->mirrorObject(srcIdx, axis);
  projectService_->syncTransformsFromModel();
#endif

  // GLViewportRenderer handles the visual mirror via InputEvent
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
      "#2a1c3c", "#1c2a3c", "#3c2a1a", "#1a3c2c", "#2c3a1c"};
  const int count = static_cast<int>(std::size(kColors));
  return QString::fromUtf8(kColors[plateIndex % count]);
}

bool EditorViewModel::renameObject(int index, const QString &newName)
{
  if (!projectService_ || index < 0)
    return false;

  const QString oldName = projectService_->objectNames().value(index);
  if (oldName == newName)
    return true;

  projectService_->renameObject(index, newName);

  if (m_undoManager)
    m_undoManager->push(new RenameCommand(index, oldName, newName, projectService_));

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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
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
  if (ok)
    emit stateChanged();
  return ok;
}

bool EditorViewModel::removePlateOtherLayersSeqEntry(int plateIndex, int entryIndex)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->removePlateOtherLayersSeqEntry(plateIndex, entryIndex);
  if (ok)
    emit stateChanged();
  return ok;
}

bool EditorViewModel::setPlateOtherLayersSeqRange(int plateIndex, int entryIndex, int beginLayer, int endLayer)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateOtherLayersSeqRange(plateIndex, entryIndex, beginLayer, endLayer);
  if (ok)
    emit stateChanged();
  return ok;
}

bool EditorViewModel::setPlateOtherLayersSeqOrder(int plateIndex, int entryIndex, const QVariantList &order)
{
  if (!projectService_)
    return false;
  bool ok = projectService_->setPlateOtherLayersSeqOrder(plateIndex, entryIndex, order);
  if (ok)
    emit stateChanged();
  return ok;
}

int EditorViewModel::plateExtruderCount(int plateIndex) const
{
  return projectService_ ? projectService_->plateExtruderCount(plateIndex) : 1;
}

QString EditorViewModel::generatePlateThumbnail(int plateIndex, int size)
{
  return projectService_ ? projectService_->generatePlateThumbnail(plateIndex, size) : QString();
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
  emit stateChanged();
}

void EditorViewModel::exportSelectedAsStl()
{
  // 对齐上游 Plater::priv::on_export_stl
  // Mock 模式：仅通知用户（实际 STL 导出需要 libslic3r mesh 序列化）
  if (!projectService_)
    return;
  const int sel = selectedObjectIndex();
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
  if (sliceService_)
    sliceService_->cancelSlice();
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
  if (sliceService_)
    sliceService_->startSlicePlate(plate);
}

bool EditorViewModel::requestExportGCode(const QString &targetPath)
{
  if (!sliceService_)
    return false;
  QUrl url(targetPath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : targetPath;
  return sliceService_->exportGCodeToPath(localPath);
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
  if (projectService_->arrangeObjects(spacing, m_arrangeRotation, m_arrangeAlignY))
  {
    // Real arrange succeeded — sync transforms from model to mock arrays
    projectService_->syncTransformsFromModel();
    emit stateChanged();
  }
  // If real arrange not available (no HAS_LIBSLIC3R), the GLViewport mock
  // arrange will still work via the existing ArrangeSelected InputEvent path
}

void EditorViewModel::switchToPreview()
{
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
  emit bedShapeChanged();
}

float EditorViewModel::bedDepth() const { return m_bedDepth; }
void EditorViewModel::setBedDepth(float v)
{
  v = qBound(10.0f, v, 2000.0f);
  if (qFuzzyCompare(m_bedDepth, v)) return;
  m_bedDepth = v;
  QSettings s; s.setValue(QStringLiteral("bed/depth"), v);
  emit bedShapeChanged();
}

float EditorViewModel::bedMaxHeight() const { return m_bedMaxHeight; }
void EditorViewModel::setBedMaxHeight(float v)
{
  v = qBound(1.0f, v, 5000.0f);
  if (qFuzzyCompare(m_bedMaxHeight, v)) return;
  m_bedMaxHeight = v;
  QSettings s; s.setValue(QStringLiteral("bed/maxHeight"), v);
  emit bedShapeChanged();
}

float EditorViewModel::bedOriginX() const { return m_bedOriginX; }
void EditorViewModel::setBedOriginX(float v)
{
  if (qFuzzyCompare(m_bedOriginX, v)) return;
  m_bedOriginX = v;
  QSettings s; s.setValue(QStringLiteral("bed/originX"), v);
  emit bedShapeChanged();
}

float EditorViewModel::bedOriginY() const { return m_bedOriginY; }
void EditorViewModel::setBedOriginY(float v)
{
  if (qFuzzyCompare(m_bedOriginY, v)) return;
  m_bedOriginY = v;
  QSettings s; s.setValue(QStringLiteral("bed/originY"), v);
  emit bedShapeChanged();
}

int EditorViewModel::bedShapeType() const { return m_bedShapeType; }
void EditorViewModel::setBedShapeType(int v)
{
  v = qBound(0, v, 2);
  if (m_bedShapeType == v) return;
  m_bedShapeType = v;
  QSettings s; s.setValue(QStringLiteral("bed/shapeType"), v);
  emit bedShapeChanged();
}

float EditorViewModel::bedDiameter() const { return m_bedDiameter; }
void EditorViewModel::setBedDiameter(float v)
{
  v = qBound(1.0f, v, 2000.0f);
  if (qFuzzyCompare(m_bedDiameter, v)) return;
  m_bedDiameter = v;
  QSettings s; s.setValue(QStringLiteral("bed/diameter"), v);
  emit bedShapeChanged();
}
