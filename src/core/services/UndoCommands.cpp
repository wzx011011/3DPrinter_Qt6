#include "UndoCommands.h"
#include "ProjectServiceMock.h"
#include "core/viewmodels/EditorViewModel.h"

#include <QDebug>

// ── TransformCommand ────────────────────────────────────────────────────────

TransformCommand::TransformCommand(int objectIndex,
                                   const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                                   ProjectServiceMock *service,
                                   QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Transform")), m_objectIndex(objectIndex),
      m_oldPos(oldPos), m_oldRot(oldRot), m_oldScale(oldScale),
      m_service(service)
{
  Q_UNUSED(parent)
}

void TransformCommand::setNewTransform(const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale)
{
  m_newPos = newPos;
  m_newRot = newRot;
  m_newScale = newScale;
}

void TransformCommand::undo()
{
  if (m_service && m_objectIndex >= 0)
  {
    m_service->setObjectPosition(m_objectIndex, m_oldPos.x(), m_oldPos.y(), m_oldPos.z());
    m_service->setObjectRotation(m_objectIndex, m_oldRot.x(), m_oldRot.y(), m_oldRot.z());
    m_service->setObjectScale(m_objectIndex, m_oldScale.x(), m_oldScale.y(), m_oldScale.z());
  }
}

void TransformCommand::redo()
{
  if (m_service && m_objectIndex >= 0)
  {
    m_service->setObjectPosition(m_objectIndex, m_newPos.x(), m_newPos.y(), m_newPos.z());
    m_service->setObjectRotation(m_objectIndex, m_newRot.x(), m_newRot.y(), m_newRot.z());
    m_service->setObjectScale(m_objectIndex, m_newScale.x(), m_newScale.y(), m_newScale.z());
  }
}

bool TransformCommand::mergeWith(const QUndoCommand *other)
{
  if (other->id() != id())
    return false;
  const auto *otherCmd = static_cast<const TransformCommand *>(other);
  if (otherCmd->m_objectIndex != m_objectIndex)
    return false;
  // Merge: take the new values from the other command
  m_newPos = otherCmd->m_newPos;
  m_newRot = otherCmd->m_newRot;
  m_newScale = otherCmd->m_newScale;
  return true;
}

// ── MultiTransformCommand ───────────────────────────────────────────────────

MultiTransformCommand::MultiTransformCommand(ProjectServiceMock *service,
                                             QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Transform")), m_service(service)
{
  Q_UNUSED(parent)
}

void MultiTransformCommand::addTransform(int objectIndex,
                                         const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                                         const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale)
{
  m_entries.append({objectIndex, oldPos, oldRot, oldScale, newPos, newRot, newScale});
}

void MultiTransformCommand::undo()
{
  if (!m_service)
    return;
  for (const auto &e : m_entries)
  {
    m_service->setObjectPosition(e.index, e.oldPos.x(), e.oldPos.y(), e.oldPos.z());
    m_service->setObjectRotation(e.index, e.oldRot.x(), e.oldRot.y(), e.oldRot.z());
    m_service->setObjectScale(e.index, e.oldScale.x(), e.oldScale.y(), e.oldScale.z());
  }
}

void MultiTransformCommand::redo()
{
  if (!m_service)
    return;
  for (const auto &e : m_entries)
  {
    m_service->setObjectPosition(e.index, e.newPos.x(), e.newPos.y(), e.newPos.z());
    m_service->setObjectRotation(e.index, e.newRot.x(), e.newRot.y(), e.newRot.z());
    m_service->setObjectScale(e.index, e.newScale.x(), e.newScale.y(), e.newScale.z());
  }
}

// ── DeleteObjectsCommand ────────────────────────────────────────────────────

DeleteObjectsCommand::DeleteObjectsCommand(const QList<int> &indicesToDelete,
                                           ProjectServiceMock *service,
                                           EditorViewModel *viewModel,
                                           QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Delete Objects")), m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // Capture snapshots in reverse order (largest index first) for stable undo
  QList<int> sorted = indicesToDelete;
  std::sort(sorted.begin(), sorted.end(), std::greater<int>());
  for (int idx : sorted)
  {
    if (idx < 0 || !service)
      continue;
    ObjectSnapshot snap;
    snap.name = service->objectNames().value(idx);
    snap.pos = service->objectPosition(idx);
    snap.rot = service->objectRotation(idx);
    snap.scale = service->objectScale(idx);
    snap.printable = service->objectPrintable(idx);
    snap.visible = service->objectVisible(idx);
    snap.volumeCount = service->objectVolumeCount(idx);
    snap.plateIndex = service->plateIndexForObject(idx);
    m_snapshots.append(snap);
  }
}

void DeleteObjectsCommand::undo()
{
  if (!m_service)
    return;

  // Re-insert objects. The snapshots are in reverse-index order,
  // so we re-insert from back to front (smallest index first)
  // to maintain correct ordering.
  for (int i = m_snapshots.size() - 1; i >= 0; --i)
  {
    const auto &snap = m_snapshots[i];
    int newIdx = m_service->addObject(snap.name);
    if (newIdx < 0)
      continue;
    m_service->setObjectPosition(newIdx, snap.pos.x(), snap.pos.y(), snap.pos.z());
    m_service->setObjectRotation(newIdx, snap.rot.x(), snap.rot.y(), snap.rot.z());
    m_service->setObjectScale(newIdx, snap.scale.x(), snap.scale.y(), snap.scale.z());
    m_service->setObjectPrintable(newIdx, snap.printable);
    m_service->setObjectVisible(newIdx, snap.visible);
    if (snap.plateIndex >= 0)
      m_service->setObjectPlateForIndex(newIdx, snap.plateIndex);
  }

  // Trigger view model rebuild
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void DeleteObjectsCommand::redo()
{
  // The actual deletion was already performed before the command was pushed.
  // This is a no-op for the initial redo. For subsequent redos after undo,
  // we need to delete the re-inserted objects.
  if (!m_service)
    return;

  // Find objects by name that match our snapshots and delete them
  const QStringList currentNames = m_service->objectNames();
  for (const auto &snap : m_snapshots)
  {
    // Find the first object matching this name
    for (int i = 0; i < currentNames.size(); ++i)
    {
      if (currentNames[i] == snap.name)
      {
        m_service->deleteObject(i);
        break;
      }
    }
  }

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── AddObjectCommand ────────────────────────────────────────────────────────

AddObjectCommand::AddObjectCommand(int objectIndex, const QString &objectName,
                                   ProjectServiceMock *service,
                                   QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Add Object")), m_objectIndex(objectIndex),
      m_objectName(objectName), m_service(service)
{
  Q_UNUSED(parent)
}

void AddObjectCommand::undo()
{
  if (!m_service || m_objectIndex < 0)
    return;
  // Delete the added object by index
  m_service->deleteObject(m_objectIndex);
}

void AddObjectCommand::redo()
{
  // The object was already added before push. Re-create on redo-after-undo.
  if (!m_service)
    return;
  int newIdx = m_service->addObject(m_objectName);
  Q_UNUSED(newIdx)
}

// ── SelectionCommand ────────────────────────────────────────────────────────

SelectionCommand::SelectionCommand(const QSet<int> &oldSelection, int oldPrimary,
                                   const QSet<int> &newSelection, int newPrimary,
                                   EditorViewModel *viewModel,
                                   QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Select")), m_oldSelection(oldSelection), m_oldPrimary(oldPrimary),
      m_newSelection(newSelection), m_newPrimary(newPrimary), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
}

void SelectionCommand::undo()
{
  if (m_viewModel)
    m_viewModel->restoreSelection(m_oldSelection, m_oldPrimary);
}

void SelectionCommand::redo()
{
  if (m_viewModel)
    m_viewModel->restoreSelection(m_newSelection, m_newPrimary);
}

// ── RenameCommand ───────────────────────────────────────────────────────────

RenameCommand::RenameCommand(int objectIndex, const QString &oldName, const QString &newName,
                             ProjectServiceMock *service,
                             QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Rename")), m_objectIndex(objectIndex),
      m_oldName(oldName), m_newName(newName), m_service(service)
{
  Q_UNUSED(parent)
}

void RenameCommand::undo()
{
  if (m_service)
    m_service->renameObject(m_objectIndex, m_oldName);
}

void RenameCommand::redo()
{
  if (m_service)
    m_service->renameObject(m_objectIndex, m_newName);
}

// ── MoveObjectCommand ───────────────────────────────────────────────────────

MoveObjectCommand::MoveObjectCommand(int fromIndex, int toIndex,
                                     ProjectServiceMock *service,
                                     QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Move Object")), m_fromIndex(fromIndex),
      m_toIndex(toIndex), m_service(service)
{
  Q_UNUSED(parent)
}

void MoveObjectCommand::undo()
{
  if (m_service)
    m_service->moveObject(m_toIndex, m_fromIndex);
}

void MoveObjectCommand::redo()
{
  if (m_service)
    m_service->moveObject(m_fromIndex, m_toIndex);
}

// ── CloneCommand ────────────────────────────────────────────────────────────

CloneCommand::CloneCommand(int sourceIndex, int clonedIndex,
                           ProjectServiceMock *service,
                           QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Clone")), m_sourceIndex(sourceIndex),
      m_clonedIndex(clonedIndex), m_service(service)
{
  Q_UNUSED(parent)
}

void CloneCommand::undo()
{
  if (m_service && m_clonedIndex >= 0)
    m_service->deleteObject(m_clonedIndex);
}

void CloneCommand::redo()
{
  if (m_service && m_sourceIndex >= 0)
    m_service->duplicateObject(m_sourceIndex);
}

// ── VolumeDeleteCommand ─────────────────────────────────────────────────────

VolumeDeleteCommand::VolumeDeleteCommand(int objectIndex, int volumeIndex,
                                         ProjectServiceMock *service,
                                         QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Delete Volume")), m_objectIndex(objectIndex),
      m_volumeIndex(volumeIndex), m_service(service)
{
  Q_UNUSED(parent)
  // Capture volume data before deletion
  if (m_service)
  {
    m_volumeName = m_service->objectVolumeName(m_objectIndex, m_volumeIndex);
    // Map type label back to int type
    QString typeLabel = m_service->objectVolumeTypeLabel(m_objectIndex, m_volumeIndex);
    if (typeLabel == QLatin1String("Modifier"))
      m_volumeType = 2;
    else if (typeLabel == QLatin1String("Support Blocker"))
      m_volumeType = 3;
    else if (typeLabel == QLatin1String("Support Enforcer"))
      m_volumeType = 4;
    else
      m_volumeType = 0;
    m_extruderId = m_service->volumeExtruderId(m_objectIndex, m_volumeIndex);
  }
}

void VolumeDeleteCommand::undo()
{
  if (!m_service)
    return;
  // Re-create the deleted volume
  m_service->addVolume(m_objectIndex, m_volumeType);
  // Set the extruder if it was non-default
  if (m_extruderId >= 0)
    m_service->setVolumeExtruderId(m_objectIndex, m_volumeIndex, m_extruderId);
}

void VolumeDeleteCommand::redo()
{
  if (!m_service)
    return;
  m_service->deleteObjectVolume(m_objectIndex, m_volumeIndex);
}

// ── BooleanCommand ──────────────────────────────────────────────────────────

BooleanCommand::BooleanCommand(int srcObjectIndex, int toolObjectIndex, int operation,
                               ProjectServiceMock *service, EditorViewModel *viewModel,
                               QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Boolean Operation")),
      m_srcObjectIndex(srcObjectIndex), m_toolObjectIndex(toolObjectIndex),
      m_operation(operation), m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)

  // Capture source mesh snapshot BEFORE the boolean operation modifies it
  m_srcMeshSnapshot = m_service ? m_service->captureObjectMeshSnapshot(m_srcObjectIndex) : QByteArray();

  // Capture tool object snapshot (same pattern as DeleteObjectsCommand)
  if (m_service)
  {
    m_toolSnapshot.name = service->objectNames().value(m_toolObjectIndex);
    m_toolSnapshot.pos = service->objectPosition(m_toolObjectIndex);
    m_toolSnapshot.rot = service->objectRotation(m_toolObjectIndex);
    m_toolSnapshot.scale = service->objectScale(m_toolObjectIndex);
    m_toolSnapshot.printable = service->objectPrintable(m_toolObjectIndex);
    m_toolSnapshot.visible = service->objectVisible(m_toolObjectIndex);
    m_toolSnapshot.volumeCount = service->objectVolumeCount(m_toolObjectIndex);
    m_toolSnapshot.plateIndex = service->plateIndexForObject(m_toolObjectIndex);
  }
}

void BooleanCommand::undo()
{
  if (!m_service)
    return;

  // Restore source object mesh to its pre-boolean state
  if (!m_srcMeshSnapshot.isEmpty())
    m_service->restoreObjectMeshSnapshot(m_srcObjectIndex, m_srcMeshSnapshot);

  // Re-insert the tool object at its original position
  int newIdx = m_service->addObject(m_toolSnapshot.name);
  if (newIdx >= 0)
  {
    m_service->setObjectPosition(newIdx, m_toolSnapshot.pos.x(), m_toolSnapshot.pos.y(), m_toolSnapshot.pos.z());
    m_service->setObjectRotation(newIdx, m_toolSnapshot.rot.x(), m_toolSnapshot.rot.y(), m_toolSnapshot.rot.z());
    m_service->setObjectScale(newIdx, m_toolSnapshot.scale.x(), m_toolSnapshot.scale.y(), m_toolSnapshot.scale.z());
    m_service->setObjectPrintable(newIdx, m_toolSnapshot.printable);
    m_service->setObjectVisible(newIdx, m_toolSnapshot.visible);
    if (m_toolSnapshot.plateIndex >= 0)
      m_service->setObjectPlateForIndex(newIdx, m_toolSnapshot.plateIndex);
  }

  // Trigger view model rebuild
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void BooleanCommand::redo()
{
  // The initial redo is a no-op (operation was already performed before push).
  // For subsequent redos after undo, re-execute the boolean operation.
  if (!m_service)
    return;

  // Find source and tool objects by name to handle potential index shifts after undo re-inserted the tool
  const QStringList currentNames = m_service->objectNames();

  // Find the source object (its name should still end with the suffix from meshBoolean,
  // but we match by checking the original position -- use name prefix matching)
  int srcIdx = -1;
  int toolIdx = -1;
  for (int i = 0; i < currentNames.size(); ++i)
  {
    if (toolIdx < 0 && currentNames[i] == m_toolSnapshot.name)
      toolIdx = i;
  }
  // Source object: its name was modified by meshBoolean with a suffix (_union/_diff/_inter).
  // We need to find it. After undo, the source name still has the suffix.
  // Use the index -- after undo, the source should be at m_srcObjectIndex
  // (since tool was re-inserted at a different position, indices may shift)
  if (m_srcObjectIndex >= 0 && m_srcObjectIndex < currentNames.size())
    srcIdx = m_srcObjectIndex;

  if (srcIdx < 0 || toolIdx < 0)
    return;

  m_service->meshBoolean(srcIdx, toolIdx, m_operation);

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── DrillCommand ────────────────────────────────────────────────────────────

DrillCommand::DrillCommand(int objectIndex, float radius, float depth,
                           int shape, int direction, bool oneLayerOnly,
                           ProjectServiceMock *service, EditorViewModel *viewModel,
                           QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Drill")),
      m_objectIndex(objectIndex), m_radius(radius), m_depth(depth),
      m_shape(shape), m_direction(direction), m_oneLayerOnly(oneLayerOnly),
      m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // Capture mesh snapshot BEFORE drilling
  m_meshSnapshot = m_service ? m_service->captureObjectMeshSnapshot(m_objectIndex) : QByteArray();
}

void DrillCommand::undo()
{
  if (!m_service || m_meshSnapshot.isEmpty())
    return;
  m_service->restoreObjectMeshSnapshot(m_objectIndex, m_meshSnapshot);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void DrillCommand::redo()
{
  // The initial redo is a no-op (drill was already performed before push).
  // For subsequent redos after undo, re-execute the drill operation.
  if (!m_service)
    return;
  m_service->drillObject(m_objectIndex, m_radius, m_depth, m_shape, m_direction, m_oneLayerOnly);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── CutCommand ────────────────────────────────────────────────────────────

CutCommand::CutCommand(int srcObjectIndex, int axis, double position, int keepMode,
                       ProjectServiceMock *service, EditorViewModel *viewModel,
                       QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Cut")),
      m_srcObjectIndex(srcObjectIndex), m_axis(axis), m_position(position),
      m_keepMode(keepMode), m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // Capture source mesh snapshot BEFORE the cut operation
  m_srcMeshSnapshot = m_service ? m_service->captureObjectMeshSnapshot(m_srcObjectIndex) : QByteArray();
  // Record total object count before cut
  m_objectCountBefore = m_service ? m_service->objectNames().size() : 0;
}

void CutCommand::setResult(int newObjectIndex, const QString &newObjectName)
{
  m_newObjectName = newObjectName;
  Q_UNUSED(newObjectIndex)
}

void CutCommand::undo()
{
  if (!m_service)
    return;

  // Restore source object mesh to its pre-cut state
  if (!m_srcMeshSnapshot.isEmpty())
    m_service->restoreObjectMeshSnapshot(m_srcObjectIndex, m_srcMeshSnapshot);

  // Remove any objects added by the cut (objects beyond m_objectCountBefore)
  const QStringList currentNames = m_service->objectNames();
  while (currentNames.size() > m_objectCountBefore)
  {
    // Remove the last added object (cut result)
    m_service->deleteObject(currentNames.size() - 1);
  }

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void CutCommand::redo()
{
  // The initial redo is a no-op (cut was already performed before push).
  // For subsequent redos after undo, re-execute the cut operation.
  if (!m_service)
    return;

  m_service->cutObject(m_srcObjectIndex, m_axis, m_position, m_keepMode);
  m_service->syncTransformsFromModel();

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── SimplifyCommand ───────────────────────────────────────────────────────

SimplifyCommand::SimplifyCommand(int objectIndex, int wantedCount, float maxError,
                                 ProjectServiceMock *service, EditorViewModel *viewModel,
                                 QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Simplify")),
      m_objectIndex(objectIndex), m_wantedCount(wantedCount), m_maxError(maxError),
      m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // Capture mesh snapshot BEFORE simplification
  m_meshSnapshot = m_service ? m_service->captureObjectMeshSnapshot(m_objectIndex) : QByteArray();
}

void SimplifyCommand::undo()
{
  if (!m_service || m_meshSnapshot.isEmpty())
    return;
  m_service->restoreObjectMeshSnapshot(m_objectIndex, m_meshSnapshot);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void SimplifyCommand::redo()
{
  // The initial redo is a no-op (simplify was already performed before push).
  // For subsequent redos after undo, re-execute the simplify operation.
  if (!m_service)
    return;
  m_service->simplifyObject(m_objectIndex, m_wantedCount, m_maxError);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── AddVolumeCommand ──────────────────────────────────────────────────────

AddVolumeCommand::AddVolumeCommand(int objectIndex, int operationType, const QString &param,
                                   ProjectServiceMock *service, EditorViewModel *viewModel,
                                   QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Add Volume")),
      m_objectIndex(objectIndex), m_operationType(operationType), m_param(param),
      m_volumeCountBefore(0), m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // Capture volume count before the add operation
  m_volumeCountBefore = m_service ? m_service->objectVolumeCount(m_objectIndex) : 0;
}

void AddVolumeCommand::setVolumeCountBefore(int count)
{
  m_volumeCountBefore = count;
}

void AddVolumeCommand::undo()
{
  if (!m_service)
    return;

  // Remove the last added volume (the one we added in this command)
  // We look for volumes beyond m_volumeCountBefore
  int currentCount = m_service->objectVolumeCount(m_objectIndex);
  while (currentCount > m_volumeCountBefore)
  {
    m_service->deleteObjectVolume(m_objectIndex, currentCount - 1);
    --currentCount;
  }

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void AddVolumeCommand::redo()
{
  // The initial redo is a no-op (volume was already added before push).
  // For subsequent redos after undo, re-execute the add operation.
  if (!m_service)
    return;

  switch (m_operationType)
  {
  case 0: // text
    m_service->addTextVolume(m_objectIndex, m_param);
    break;
  case 1: // svg
    m_service->addSvgVolume(m_objectIndex, m_param);
    break;
  case 2: // emboss (same as text for volume add)
    m_service->addTextVolume(m_objectIndex, m_param);
    break;
  }

  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}
