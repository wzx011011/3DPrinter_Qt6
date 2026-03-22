#pragma once

#include <QUndoCommand>
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
  };
  QList<ObjectSnapshot> m_snapshots;
  ProjectServiceMock *m_service;
  EditorViewModel *m_viewModel;
};

// ── AddObjectCommand ────────────────────────────────────────────────────────
/// Records the index/name of a newly added object. On undo, removes it.
/// On redo, re-adds it.
class AddObjectCommand : public QUndoCommand
{
public:
  /// objectIndex is the index returned by ProjectServiceMock::addObject().
  AddObjectCommand(int objectIndex, const QString &objectName,
                   ProjectServiceMock *service,
                   QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  QString m_objectName;
  ProjectServiceMock *m_service;
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
class VolumeDeleteCommand : public QUndoCommand
{
public:
  VolumeDeleteCommand(int objectIndex, int volumeIndex,
                      ProjectServiceMock *service,
                      QUndoCommand *parent = nullptr);

  void undo() override;
  void redo() override;

private:
  int m_objectIndex;
  int m_volumeIndex;
  QString m_volumeName;
  int m_volumeType;
  int m_extruderId;
  ProjectServiceMock *m_service;
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
