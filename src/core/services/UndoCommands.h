#pragma once

#include <QUndoCommand>
#include <QByteArray>
#include <QVector3D>
#include <QSet>
#include <QString>

class ProjectServiceMock;
class EditorViewModel;

// ── TransformCommand ────────────────────────────────────────────────────────
/// Records before/after position/rotation/scale for one object.
/// On undo, restores the old transform. On redo, applies the new transform.
class TransformCommand : public QUndoCommand
{
public:
  /// Stores the old transform in constructor; call setNewTransform() before push().
  TransformCommand(int objectIndex,
                   const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                   ProjectServiceMock *service,
                   QUndoCommand *parent = nullptr);

  void setNewTransform(const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale);

  void undo() override;
  void redo() override;
  int id() const override { return 1; }
  bool mergeWith(const QUndoCommand *other) override;

private:
  int m_objectIndex;
  QVector3D m_oldPos, m_oldRot, m_oldScale;
  QVector3D m_newPos, m_newRot, m_newScale;
  ProjectServiceMock *m_service;
};

// ── AssembleTransformCommand ───────────────────────────────────────────────
// ASM-01 (Phase 138): like TransformCommand but targets ModelInstance::
// m_assemble_transformation (upstream Model.hpp:1253-1298) via the service's
// setAssembleOffset/Rotation/Scale accessors. Used by the Move/Rotate/Scale
// gizmos when the active canvas is CanvasAssembleView so that assembly-canvas
// edits round-trip through undo/redo without disturbing the Prepare transform.
// Distinct id (7) so mergeWith never crosses the two transform kinds.
class AssembleTransformCommand : public QUndoCommand
{
public:
  /// Stores the old assemble transform in constructor; call setNewTransform() before push().
  AssembleTransformCommand(int objectIndex,
                           const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                           ProjectServiceMock *service,
                           QUndoCommand *parent = nullptr);

  void setNewTransform(const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale);

  void undo() override;
  void redo() override;
  int id() const override { return 7; }
  bool mergeWith(const QUndoCommand *other) override;

private:
  int m_objectIndex;
  QVector3D m_oldPos, m_oldRot, m_oldScale;
  QVector3D m_newPos, m_newRot, m_newScale;
  ProjectServiceMock *m_service;
};

// ── MultiTransformCommand ───────────────────────────────────────────────────
/// Records before/after transforms for multiple objects (e.g. uniform scale).
class MultiTransformCommand : public QUndoCommand
{
public:
  MultiTransformCommand(ProjectServiceMock *service,
                        QUndoCommand *parent = nullptr);

  /// Add an object transform pair. Must be called before push().
  void addTransform(int objectIndex,
                    const QVector3D &oldPos, const QVector3D &oldRot, const QVector3D &oldScale,
                    const QVector3D &newPos, const QVector3D &newRot, const QVector3D &newScale);

  void undo() override;
  void redo() override;

private:
  struct Entry {
    int index;
    QVector3D oldPos, oldRot, oldScale;
    QVector3D newPos, newRot, newScale;
  };
  QList<Entry> m_entries;
  ProjectServiceMock *m_service;
};

// ── DeleteObjectsCommand ────────────────────────────────────────────────────
/// Records object data before deletion. On undo, re-inserts objects.
/// On redo, re-deletes.
// v5.16 (UNDO-01): upstream takes whole-model snapshots (Plater::_take_snapshot)
// so undo restores the deleted object with its full mesh/volumes. This command
// now captures a single-object 3MF snapshot (captureFullObjectSnapshot) and
/// restores via restoreFullObjectSnapshot; the old name-only addObject path
/// stays as the mock-mode fallback (empty snapshot).
class DeleteObjectsCommand : public QUndoCommand
{
public:
  /// Takes ownership of deletion logic. The objects at indicesToDelete are
  /// captured before the actual deletion happens.
  DeleteObjectsCommand(const QList<int> &indicesToDelete,
                       ProjectServiceMock *service,
                       EditorViewModel *viewModel,
                       QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  struct ObjectSnapshot {
    QString name;
    QVector3D pos, rot, scale;
    bool printable;
    bool visible;
    int volumeCount;
    int plateIndex;
    int originalIndex = -1;   ///< index at capture time (insert target for undo)
    QByteArray full3mf;       ///< v5.16 UNDO-01: single-object 3MF snapshot
    int restoredIndex = -1;   ///< index returned by restore (redo delete target)
  };
  QList<ObjectSnapshot> m_snapshots;
  /// QUndoStack::push() invokes redo() once while the objects are already
  /// deleted — skip that first call (otherwise a duplicate name still present
  /// in the scene would be mis-deleted by the name fallback).
  bool m_firstRedoDone = false;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── AddObjectCommand ────────────────────────────────────────────────────────
/// Records the index/name of a newly added object. On undo, removes it.
/// On redo, re-adds it.
// v5.16 (UNDO-01/UNDO-05): the object already exists when the command is
// constructed (paste pushes after addObject), so the constructor captures a
// full 3MF snapshot + transform; redo restores the mesh-bearing object instead
// of re-adding an empty one. undo deletes by index, falling back to a
// name lookup when indices drifted (name collision risk is low: the object
// was just added).
class AddObjectCommand : public QUndoCommand
{
public:
  /// objectIndex is the index returned by ProjectServiceMock::addObject() /
  /// restoreFullObjectSnapshot(). Must be constructed while the object still
  /// exists so the snapshot can be captured.
  AddObjectCommand(int objectIndex, const QString &objectName,
                   ProjectServiceMock *service,
                   QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  QString m_objectName;
  ProjectServiceMock *m_service;
  QByteArray m_full3mf;          ///< v5.16: single-object 3MF snapshot for redo
  QVector3D m_pos, m_rot, m_scale;
  bool m_printable = true;
  bool m_visible = true;
  int m_plateIndex = -1;
  /// QUndoStack::push() invokes redo() once on push; the object already exists
  /// at that point, so the first redo is a no-op (Qt skip-first pattern).
  bool m_firstRedoDone = false;
};

// ── SelectionCommand ────────────────────────────────────────────────────────
/// Records before/after selection state. On undo/redo, restores selection.
class SelectionCommand : public QUndoCommand
{
public:
  SelectionCommand(const QSet<int> &oldSelection, int oldPrimary,
                   const QSet<int> &newSelection, int newPrimary,
                   EditorViewModel *viewModel,
                   QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  QSet<int> m_oldSelection, m_newSelection;
  int m_oldPrimary, m_newPrimary;
  EditorViewModel *m_viewModel;
};

// ── RenameCommand ───────────────────────────────────────────────────────────
/// Records object index, old name, new name. On undo/redo, swaps.
class RenameCommand : public QUndoCommand
{
public:
  RenameCommand(int objectIndex, const QString &oldName, const QString &newName,
                ProjectServiceMock *service,
                QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  QString m_oldName;
  QString m_newName;
  ProjectServiceMock *m_service;
};

// ── MoveObjectCommand ───────────────────────────────────────────────────────
/// Records fromIndex/toIndex for object reordering. On undo/redo, swaps back.
class MoveObjectCommand : public QUndoCommand
{
public:
  MoveObjectCommand(int fromIndex, int toIndex,
                    ProjectServiceMock *service,
                    QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_fromIndex;
  int m_toIndex;
  ProjectServiceMock *m_service;
};

// ── CloneCommand ────────────────────────────────────────────────────────────
/// Records the cloned object index. On undo, deletes the clone.
/// On redo, re-creates the clone from the source.
class CloneCommand : public QUndoCommand
{
public:
  CloneCommand(int sourceIndex, int clonedIndex,
               ProjectServiceMock *service,
               QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_sourceIndex;
  int m_clonedIndex;
  ProjectServiceMock *m_service;
};

// ── VolumeDeleteCommand ─────────────────────────────────────────────────────
/// Records volume data before deletion. On undo, restores. On redo, re-deletes.
// v5.16 (UNDO-02): captures the volume mesh + type/transform/extruder via
// captureVolumeMeshSnapshot so undo rebuilds the real volume (the old
// addVolume fallback created a mesh-less placeholder). Must be constructed
// BEFORE the volume is deleted.
class VolumeDeleteCommand : public QUndoCommand
{
public:
  VolumeDeleteCommand(int objectIndex, int volumeIndex,
                      ProjectServiceMock *service,
                      EditorViewModel *viewModel = nullptr,
                      QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  int m_volumeIndex;
  QString m_volumeName;
  int m_volumeType;
  int m_extruderId;
  QByteArray m_volumeMesh;  ///< v5.16 UNDO-02: serialized its+type+transform+extruder
  /// QUndoStack::push() invokes redo() once on push; the volume is already
  /// deleted at that point, so the first redo is a no-op (Qt skip-first
  /// pattern) — otherwise the push would delete the WRONG successor volume.
  bool m_firstRedoDone = false;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── BooleanCommand ──────────────────────────────────────────────────────────
/// Records source mesh before boolean operation and tool object snapshot.
/// On undo: restores the source mesh and re-inserts the deleted tool object.
/// On redo: re-executes the boolean operation.
class BooleanCommand : public QUndoCommand
{
public:
  /// Captures source mesh snapshot and tool object snapshot before the
  /// actual boolean operation is performed.
  /// \a adjustedSrcIndex is the source index AFTER the tool is deleted
  /// (may differ from original if toolIndex < srcIndex).
  BooleanCommand(int srcObjectIndex, int toolObjectIndex, int operation,
                 ProjectServiceMock *service, EditorViewModel *viewModel,
                 QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_srcObjectIndex;   ///< original source index before any deletion
  int m_toolObjectIndex;  ///< original tool index before deletion
  int m_operation;        ///< 0=union, 1=diff, 2=intersect
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
  // Tool object snapshot (for re-insertion on undo)
  struct ObjectSnapshot {
    QString name;
    QVector3D pos, rot, scale;
    bool printable;
    bool visible;
    int volumeCount;
    int plateIndex;
  };
  ObjectSnapshot m_toolSnapshot;
  // Source mesh snapshot stored as QByteArray (serialized indexed_triangle_set)
  QByteArray m_srcMeshSnapshot;
};

// ── DrillCommand ────────────────────────────────────────────────────────────
/// Records object mesh before drilling. On undo, restores the original mesh.
/// On redo, re-executes the drill operation.
class DrillCommand : public QUndoCommand
{
public:
  /// Captures the object mesh snapshot before drilling.
  DrillCommand(int objectIndex, float radius, float depth,
               int shape, int direction, bool oneLayerOnly,
               ProjectServiceMock *service, EditorViewModel *viewModel,
               QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  float m_radius;
  float m_depth;
  int m_shape;
  int m_direction;
  bool m_oneLayerOnly;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
  QByteArray m_meshSnapshot;
};

// ── CutCommand ────────────────────────────────────────────────────────────
/// Records source mesh before cut and object count. On undo, restores source mesh
/// and removes cut-result objects. On redo, re-executes the cut.
class CutCommand : public QUndoCommand
{
public:
  /// Captures source mesh snapshot and object count before the cut.
  CutCommand(int srcObjectIndex, int axis, double position, int keepMode,
             ProjectServiceMock *service, EditorViewModel *viewModel,
             QUndoCommand *parent = nullptr);

  /// Call after cut succeeds to record the new object index and its name.
  void setResult(int newObjectIndex, const QString &newObjectName);

  void undo() override;
  void redo() override;

private:
  int m_srcObjectIndex;
  int m_axis;
  double m_position;
  int m_keepMode;
  int m_objectCountBefore;    ///< total object count before cut
  QByteArray m_srcMeshSnapshot;
  // Cut result object snapshot (for re-insertion on redo)
  QString m_newObjectName;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── SimplifyCommand ───────────────────────────────────────────────────────
/// Records object mesh before simplification. On undo, restores the original mesh.
/// On redo, re-executes the simplify operation.
class SimplifyCommand : public QUndoCommand
{
public:
  /// Captures the object mesh snapshot before simplification.
  SimplifyCommand(int objectIndex, int wantedCount, float maxError,
                  ProjectServiceMock *service, EditorViewModel *viewModel,
                  QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  int m_wantedCount;
  float m_maxError;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
  QByteArray m_meshSnapshot;
};

// ── AddVolumeCommand ──────────────────────────────────────────────────────
/// Records object state before adding a text/SVG/emboss volume. On undo, removes
/// the added volume. On redo, re-executes the add operation.

// ── UpdateVolumeMeshCommand ────────────────────────────────────────────────
// Phase 240 (GIZ-06): in-place text-volume re-generation undo (upstream
// GLGizmoEmboss takes an UndoRedo snapshot per emboss edit). Captures the
// volume mesh snapshot BEFORE the update; undo restores it, redo re-applies
// the AFTER snapshot (captured post-update via setAfterSnapshot).
class UpdateVolumeMeshCommand : public QUndoCommand
{
public:
  UpdateVolumeMeshCommand(int objectIndex, int volumeIndex,
                          const QString &volumeName,
                          ProjectServiceMock *service,
                          EditorViewModel *viewModel,
                          QUndoCommand *parent = nullptr);

  /// Call AFTER the update succeeded with the post-update snapshot.
  void setAfterSnapshot(const QByteArray &after);
  void setVolumeType(int type);

  void undo() override;
  void redo() override;
  int id() const override { return 12; }

private:
  int m_objectIndex;
  int m_volumeIndex;
  QString m_volumeName;
  int m_volumeType = 0;
  QByteArray m_before;
  QByteArray m_after;
  bool m_firstRedoDone = false;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

class AddVolumeCommand : public QUndoCommand
{
public:
  /// operationType: 0=text, 1=svg, 2=emboss
  AddVolumeCommand(int objectIndex, int operationType, const QString &param,
                   ProjectServiceMock *service, EditorViewModel *viewModel,
                   QUndoCommand *parent = nullptr);

  /// Call after add succeeds to record the volume count before add (for undo).
  void setVolumeCountBefore(int count);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  int m_operationType;  ///< 0=text, 1=svg, 2=emboss
  QString m_param;       ///< text string, svg file path, or emboss text
  int m_volumeCountBefore;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── PlateCommand ────────────────────────────────────────────────────────────
// v5.16 (UNDO-03): plate operations enter the undo stack. Upstream truth:
// every plate op takes a whole-model snapshot (PartPlate.cpp:7060
// "add partplate", :13993 "lock partplate", :14033 "move plate",
// :14074 "delete partplate"). Qt6 equivalent under the per-command
// architecture: capture the full plate-list state (plate fields + instance
// membership + current index; DeletePlate additionally embeds per-object 3MF
// snapshots via capturePlateListSnapshot(deep)) BEFORE the operation, capture
// the state again AFTER it, and swap the two snapshots on undo/redo.
class PlateCommand : public QUndoCommand
{
public:
  enum Action {
    AddPlate = 0,
    DeletePlate,
    MovePlate,
    ClonePlate,
    LockPlate,
    SetPrintable
  };

  /// Captures the BEFORE snapshot (deep object blobs for DeletePlate — its
  /// members must be restorable with mesh fidelity). Construct BEFORE the
  /// plate operation runs.
  PlateCommand(Action action, ProjectServiceMock *service,
               EditorViewModel *viewModel, QUndoCommand *parent = nullptr);

  /// Captures the AFTER snapshot. Call after the operation succeeded and
  /// before push() (two-phase construction, same as TransformCommand).
  void setAfterState();

  void undo() override;
  void redo() override;

private:
  Action m_action;
  QByteArray m_before;
  QByteArray m_after;
  /// QUndoStack::push() invokes redo() once while the operation's result is
  /// already applied — skip that first call (Qt skip-first pattern, same as
  /// DeleteObjectsCommand / VolumeDeleteCommand).
  bool m_firstRedoDone = false;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── PaintCommand ────────────────────────────────────────────────────────────
// v5.16 (UNDO-04): paint strokes enter the undo stack. Upstream truth: the
// painter gizmos commit each stroke through Plater::_take_snapshot(
// GizmoAction) (GLGizmoPainterBase). Qt6 equivalent: every paintAtFacet call
// pushes a command holding the FacetsAnnotation serialization (kind-specific
// supported/seam/mmu_segmentation facets) before and after the stroke;
// consecutive commands on the same (object, volume, kind) merge via
// mergeWith so one drag = one undo step (TransformCommand coalescing
// pattern). undo/redo also re-sync the PaintEngine's cached TriangleSelector
// so the overlay keeps rendering the restored state.
class PaintCommand : public QUndoCommand
{
public:
  /// `before` is captured before the stroke modifies the FacetsAnnotation;
  /// call setNewResult() with the post-stroke capture before push().
  PaintCommand(int objectIndex, int volumeIndex, int kind,
               const QByteArray &before, ProjectServiceMock *service,
               EditorViewModel *viewModel, QUndoCommand *parent = nullptr);

  void setNewResult(const QByteArray &after);

  void undo() override;
  void redo() override;
  int id() const override { return 8; }
  bool mergeWith(const QUndoCommand *other) override;

private:
  int m_objectIndex;
  int m_volumeIndex;
  int m_kind;  ///< 0=Support, 1=Seam, 2=Mmu (PaintKind)
  QByteArray m_before;
  QByteArray m_after;
  /// QUndoStack::push() invokes redo() once while the paint is already
  /// applied — skip that first call (Qt skip-first pattern).
  bool m_firstRedoDone = false;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};
