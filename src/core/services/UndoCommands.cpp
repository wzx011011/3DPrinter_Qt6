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

// ── AssembleTransformCommand (ASM-01 Phase 138) ────────────────────────────
// Mirrors TransformCommand but writes the assemble transform (ModelInstance::
// m_assemble_transformation, Model.hpp:1253-1298) via setAssembleOffset/Rotation/
// Scale so assembly-canvas edits round-trip through undo/redo without disturbing
// the Prepare transform. Distinct id (7) so mergeWith never crosses the two kinds.

AssembleTransformCommand::AssembleTransformCommand(int objectIndex,
                                                   const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                                                   ProjectServiceMock *service,
                                                   QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Assemble Transform")), m_objectIndex(objectIndex),
      m_oldPos(oldPos), m_oldRot(oldRot), m_oldScale(oldScale),
      m_service(service)
{
  Q_UNUSED(parent)
}

void AssembleTransformCommand::setNewTransform(const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale)
{
  m_newPos = newPos;
  m_newRot = newRot;
  m_newScale = newScale;
}

void AssembleTransformCommand::undo()
{
  if (m_service && m_objectIndex >= 0)
  {
    m_service->setAssembleOffset(m_objectIndex, m_oldPos.x(), m_oldPos.y(), m_oldPos.z());
    m_service->setAssembleRotation(m_objectIndex, m_oldRot.x(), m_oldRot.y(), m_oldRot.z());
    m_service->setAssembleScale(m_objectIndex, m_oldScale.x(), m_oldScale.y(), m_oldScale.z());
  }
}

void AssembleTransformCommand::redo()
{
  if (m_service && m_objectIndex >= 0)
  {
    m_service->setAssembleOffset(m_objectIndex, m_newPos.x(), m_newPos.y(), m_newPos.z());
    m_service->setAssembleRotation(m_objectIndex, m_newRot.x(), m_newRot.y(), m_newRot.z());
    m_service->setAssembleScale(m_objectIndex, m_newScale.x(), m_newScale.y(), m_newScale.z());
  }
}

bool AssembleTransformCommand::mergeWith(const QUndoCommand *other)
{
  if (other->id() != id())
    return false;
  const auto *otherCmd = static_cast<const AssembleTransformCommand *>(other);
  if (otherCmd->m_objectIndex != m_objectIndex)
    return false;
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
    snap.originalIndex = idx;
    // v5.16 (UNDO-01): full-object 3MF snapshot (mesh + volumes + config) —
    // mirrors upstream Plater::_take_snapshot whole-object fidelity. Empty in
    // mock mode / on capture failure; undo falls back to the name-only path.
    snap.full3mf = service->captureFullObjectSnapshot(idx);
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
    int newIdx = -1;
    if (!snap.full3mf.isEmpty())
    {
      // v5.16 (UNDO-01): restore mesh + volumes + name/printable/visible/plate,
      // then re-apply the instance transform (restore leaves transforms alone).
      newIdx = m_service->restoreFullObjectSnapshot(snap.full3mf, snap.originalIndex,
                                                    snap.name, snap.printable,
                                                    snap.visible, snap.plateIndex);
      if (newIdx >= 0)
      {
        m_service->setObjectPosition(newIdx, snap.pos.x(), snap.pos.y(), snap.pos.z());
        m_service->setObjectRotation(newIdx, snap.rot.x(), snap.rot.y(), snap.rot.z());
        m_service->setObjectScale(newIdx, snap.scale.x(), snap.scale.y(), snap.scale.z());
      }
    }
    if (newIdx < 0)
    {
      // Fallback (mock mode / snapshot failure): old name-only restore.
      newIdx = m_service->addObject(snap.name);
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
    m_snapshots[i].restoredIndex = newIdx;
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
  // QUndoStack::push() invokes redo() once right after the deletion — skip it
  // (Qt skip-first pattern; the old name-matching no-op could mis-delete a
  // same-named survivor on push).
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }

  // v5.16 (UNDO-01): prefer the restore indices captured by undo (identity,
  // immune to duplicate-name mismatches); name matching stays as the fallback
  // for snapshots restored through the mock path (restoredIndex < 0).
  QList<int> deleteIndices;
  bool haveRestoreIndices = true;
  for (const auto &snap : m_snapshots)
  {
    int idx = snap.restoredIndex;
    if (idx < 0)
    {
      haveRestoreIndices = false;
      break;
    }
    deleteIndices.append(idx);
  }
  if (!haveRestoreIndices)
  {
    deleteIndices.clear();
    const QStringList currentNames = m_service->objectNames();
    for (const auto &snap : m_snapshots)
    {
      // Find the first object matching this name
      for (int i = 0; i < currentNames.size(); ++i)
      {
        if (currentNames[i] == snap.name)
        {
          deleteIndices.append(i);
          break;
        }
      }
    }
  }

  // Delete highest index first so lower indices stay stable mid-loop.
  std::sort(deleteIndices.begin(), deleteIndices.end(), std::greater<int>());
  deleteIndices.erase(std::unique(deleteIndices.begin(), deleteIndices.end()),
                      deleteIndices.end());
  for (int idx : deleteIndices)
  {
    if (idx >= 0 && idx < m_service->objectNames().size())
      m_service->deleteObject(idx);
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
  // v5.16 (UNDO-01/UNDO-05): the object exists at push time (paste pushes
  // after addObject), so capture the full state here — redo restores the
  // mesh-bearing object instead of re-adding an empty one. The capture may
  // legitimately come back empty (mock mode / IO failure): redo then falls
  // back to the old name-only path.
  if (m_service && m_objectIndex >= 0 && m_objectIndex < m_service->objectNames().size())
  {
    m_full3mf = m_service->captureFullObjectSnapshot(m_objectIndex);
    m_pos = m_service->objectPosition(m_objectIndex);
    m_rot = m_service->objectRotation(m_objectIndex);
    m_scale = m_service->objectScale(m_objectIndex);
    m_printable = m_service->objectPrintable(m_objectIndex);
    m_visible = m_service->objectVisible(m_objectIndex);
    m_plateIndex = m_service->plateIndexForObject(m_objectIndex);
  }
}

void AddObjectCommand::undo()
{
  if (!m_service || m_objectIndex < 0)
    return;
  // Delete the added object. The recorded index may have drifted if other
  // commands shifted the list; prefer the index when the name still matches
  // at that slot, else locate by name (name-collision risk is low here: the
  // object was just added — upstream undo of an add simply removes it).
  const QStringList currentNames = m_service->objectNames();
  int idx = -1;
  if (m_objectIndex < currentNames.size() && currentNames.value(m_objectIndex) == m_objectName)
    idx = m_objectIndex;
  else
    idx = currentNames.indexOf(m_objectName);
  if (idx >= 0)
    m_service->deleteObject(idx);
}

void AddObjectCommand::redo()
{
  if (!m_service)
    return;
  // QUndoStack::push() invokes redo() once while the object already exists —
  // skip that first call (Qt skip-first pattern).
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }
  if (!m_full3mf.isEmpty())
  {
    // v5.16 (UNDO-01/UNDO-05): restore the full mesh + volumes + state at the
    // tail (matches the original addObject append position).
    const int newIdx = m_service->restoreFullObjectSnapshot(
        m_full3mf, -1, m_objectName, m_printable, m_visible, m_plateIndex);
    if (newIdx >= 0)
    {
      m_service->setObjectPosition(newIdx, m_pos.x(), m_pos.y(), m_pos.z());
      m_service->setObjectRotation(newIdx, m_rot.x(), m_rot.y(), m_rot.z());
      m_service->setObjectScale(newIdx, m_scale.x(), m_scale.y(), m_scale.z());
    }
  }
  else
  {
    int newIdx = m_service->addObject(m_objectName);
    Q_UNUSED(newIdx)
  }
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
                                         EditorViewModel *viewModel,
                                         QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Delete Volume")), m_objectIndex(objectIndex),
      m_volumeIndex(volumeIndex), m_service(service), m_viewModel(viewModel)
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
    // v5.16 (UNDO-02): serialize the real mesh + type/transform/extruder so
    // undo rebuilds the true volume (addVolume fallback inherits the first
    // model_part mesh, which is wrong for a deleted part with own geometry).
    m_volumeMesh = m_service->captureVolumeMeshSnapshot(m_objectIndex, m_volumeIndex);
  }
}

void VolumeDeleteCommand::undo()
{
  if (!m_service)
    return;
  if (!m_volumeMesh.isEmpty())
  {
    // v5.16 (UNDO-02): rebuild the exact volume (its + type + transform +
    // extruder) at its original slot.
    m_service->restoreVolumeSnapshot(m_objectIndex, m_volumeIndex, m_volumeMesh,
                                     m_volumeName, m_volumeType);
  }
  else
  {
    // Fallback (mock mode / capture failure): re-create a placeholder volume.
    m_service->addVolume(m_objectIndex, m_volumeType);
    // Set the extruder if it was non-default
    if (m_extruderId >= 0)
      m_service->setVolumeExtruderId(m_objectIndex, m_volumeIndex, m_extruderId);
  }
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void VolumeDeleteCommand::redo()
{
  if (!m_service)
    return;
  // QUndoStack::push() invokes redo() once after the volume was already
  // deleted — skip that first call, otherwise it would delete the successor
  // volume that shifted into m_volumeIndex.
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }
  m_service->deleteObjectVolume(m_objectIndex, m_volumeIndex);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
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

// ── UpdateVolumeMeshCommand ────────────────────────────────────────────────
// Phase 240 (GIZ-06): in-place text-volume re-generation undo. The BEFORE
// snapshot is captured in the constructor (pre-update state); setAfterSnapshot
// records the post-update mesh so redo reproduces the regenerated volume.
UpdateVolumeMeshCommand::UpdateVolumeMeshCommand(int objectIndex, int volumeIndex,
                                                 const QString &volumeName,
                                                 ProjectServiceMock *service,
                                                 EditorViewModel *viewModel,
                                                 QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Update Text Volume")),
      m_objectIndex(objectIndex), m_volumeIndex(volumeIndex),
      m_volumeName(volumeName), m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  if (m_service)
    m_before = m_service->captureVolumeMeshSnapshot(objectIndex, volumeIndex);
}

void UpdateVolumeMeshCommand::setAfterSnapshot(const QByteArray &after)
{
  m_after = after;
}

void UpdateVolumeMeshCommand::setVolumeType(int type)
{
  m_volumeType = type;
}

void UpdateVolumeMeshCommand::undo()
{
  if (!m_service || m_before.isEmpty())
    return;
  m_service->restoreVolumeSnapshot(m_objectIndex, m_volumeIndex, m_before,
                                   m_volumeName, m_volumeType);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void UpdateVolumeMeshCommand::redo()
{
  if (!m_service || m_after.isEmpty())
    return;
  // QUndoStack::push() invokes redo() once while the update is already
  // applied -- skip that first call (same pattern as PaintCommand).
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }
  m_service->restoreVolumeSnapshot(m_objectIndex, m_volumeIndex, m_after,
                                   m_volumeName, m_volumeType);
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

// ── PlateCommand (v5.16 UNDO-03) ──────────────────────────────────────────
// Upstream truth: plate ops take whole-model snapshots (PartPlate.cpp:7060
// "add partplate", :13993 "lock partplate", :14033 "move plate",
// :14074 "delete partplate"). This command swaps before/after plate-list
// snapshots captured around the operation, so every plate action (add /
// delete / move / clone / lock / printable) round-trips through the stack.

PlateCommand::PlateCommand(Action action, ProjectServiceMock *service,
                           EditorViewModel *viewModel, QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Plate Operation")), m_action(action),
      m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
  // DeletePlate embeds per-object 3MF snapshots for the plate's members so
  // undo can restore them with full mesh fidelity if they die with the plate.
  if (m_service)
    m_before = m_service->capturePlateListSnapshot(action == DeletePlate);
  switch (action)
  {
  case AddPlate:     setText(QObject::tr("Add Plate")); break;
  case DeletePlate:  setText(QObject::tr("Delete Plate")); break;
  case MovePlate:    setText(QObject::tr("Move Plate")); break;
  case ClonePlate:   setText(QObject::tr("Clone Plate")); break;
  case LockPlate:    setText(QObject::tr("Lock Plate")); break;
  case SetPrintable: setText(QObject::tr("Plate Printable")); break;
  }
}

void PlateCommand::setAfterState()
{
  if (m_service)
    m_after = m_service->capturePlateListSnapshot(false);
}

void PlateCommand::undo()
{
  if (!m_service || m_before.isEmpty())
    return;
  m_service->restorePlateListSnapshot(m_before);
  // Paint state follows the objects (a restored/removed object changes the
  // cached selectors), and the scene/object lists must be rebuilt.
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

void PlateCommand::redo()
{
  if (!m_service)
    return;
  // QUndoStack::push() invokes redo() once right after the operation already
  // applied its effect — skip that first call (Qt skip-first pattern).
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }
  if (m_after.isEmpty())
    return;
  m_service->restorePlateListSnapshot(m_after);
  if (m_viewModel)
    QMetaObject::invokeMethod(m_viewModel, "rebuildAndNotify", Qt::QueuedConnection);
}

// ── PaintCommand (v5.16 UNDO-04) ──────────────────────────────────────────
// Upstream truth: GLGizmoPainterBase commits strokes via
// Plater::_take_snapshot(GizmoAction). Each paintAtFacet call pushes one
// command; mergeWith coalesces consecutive strokes on the same
// (object, volume, kind) so one drag collapses into one undo step.

PaintCommand::PaintCommand(int objectIndex, int volumeIndex, int kind,
                           const QByteArray &before, ProjectServiceMock *service,
                           EditorViewModel *viewModel, QUndoCommand *parent)
    : QUndoCommand(QObject::tr("Paint")), m_objectIndex(objectIndex),
      m_volumeIndex(volumeIndex), m_kind(kind), m_before(before),
      m_service(service), m_viewModel(viewModel)
{
  Q_UNUSED(parent)
}

void PaintCommand::setNewResult(const QByteArray &after)
{
  m_after = after;
}

void PaintCommand::undo()
{
  if (!m_service)
    return;
  m_service->restorePaintSnapshot(m_objectIndex, m_volumeIndex, m_kind, m_before);
  // Direct call (SelectionCommand's restoreSelection pattern): the selector
  // must mirror the restored annotation before the next paintAt reads it.
  if (m_viewModel)
    m_viewModel->resyncPaintSelector(m_objectIndex, m_volumeIndex, m_kind);
}

void PaintCommand::redo()
{
  if (!m_service)
    return;
  // QUndoStack::push() invokes redo() once while the paint is already
  // applied — skip that first call (Qt skip-first pattern).
  if (!m_firstRedoDone)
  {
    m_firstRedoDone = true;
    return;
  }
  m_service->restorePaintSnapshot(m_objectIndex, m_volumeIndex, m_kind, m_after);
  if (m_viewModel)
    m_viewModel->resyncPaintSelector(m_objectIndex, m_volumeIndex, m_kind);
}

bool PaintCommand::mergeWith(const QUndoCommand *other)
{
  if (other->id() != id())
    return false;
  const auto *otherCmd = static_cast<const PaintCommand *>(other);
  if (otherCmd->m_objectIndex != m_objectIndex || otherCmd->m_volumeIndex != m_volumeIndex
      || otherCmd->m_kind != m_kind)
    return false;
  // Same stroke target: keep this command's BEFORE, take the newest AFTER
  // (TransformCommand merge semantics — the pair spans the whole drag).
  m_after = otherCmd->m_after;
  return true;
}
