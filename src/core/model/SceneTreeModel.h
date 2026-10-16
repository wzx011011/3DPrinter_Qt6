#pragma once

#include <QAbstractItemModel>
#include <QVector>
#include <QHash>

class EditorViewModel;
class ProjectService;

// ── X6 (2026-10-15): object-list panel scene-tree bridge ────────────────────
// QAbstractItemModel mirror of the upstream Plater object list's data model
// (third_party/OrcaSlicer/src/slic3r/GUI/ObjectDataViewModel.hpp:25-36):
//   root → plates (itPlate) → objects (itObject)
//        → object children: volumes (itVolume) + instance root (itInstanceRoot,
//          display name "Instances") → instances (itInstance, "Instance N").
// Settings / layer / info subtrees are deliberately not built -- the parity
// doc (docs/ui-reference/object-list-parity.md §7) registers them as carried
// by the existing dialog/canvas paths, not as missing behavior.
//
// Data source: the ProjectService owned by the injected EditorViewModel
// (SOURCE object indices -- never the organize-mode filtered list; every
// existing EditorViewModel object* accessor maps a filtered index first, so
// routing tree rows through them would double-map). Mutations never live
// here: setData(EditRole) and the toggles route back through the
// EditorViewModel undo-wired methods (RenameCommand / PrintableCommand /
// MoveObjectCommand -- the upstream take_snapshot equivalents).
//
// Refresh routing rides the S1 semantic clusters (EditorViewModel.h:2052):
//   sceneTopologyChanged / plateChanged / currentPlateIndexChanged → rebuild
//   (beginResetModel -- node set or plate membership changed);
//   selectionChanged / stateChanged → dataChanged over the existing rows
//   (selection, printable, names; stateChanged is the carrier for undo/redo
//   because UndoRedoManager relays its stateChanged into the VM umbrella).
// transformChanged is ignored: per-drag-frame transforms never alter tree
// content, so the panel stays quiet during gizmo drags.
class SceneTreeModel final : public QAbstractItemModel
{
  Q_OBJECT
public:
  // Values mirror the upstream ItemType numbering (ObjectDataViewModel.hpp:
  // 25-36) so a parity diff stays a 1:1 mapping.
  enum NodeType
  {
    NodePlate = 1,         // itPlate
    NodeObject = 2,        // itObject
    NodeVolume = 4,        // itVolume
    NodeInstanceRoot = 8,  // itInstanceRoot ("Instances" container)
    NodeInstance = 16      // itInstance ("Instance N")
  };
  Q_ENUM(NodeType)

  // QML delegate roles. DisplayRole/EditRole carry the row name (EditRole
  // renames plate/object rows through the VM undo path).
  enum Roles
  {
    NodeTypeRole = Qt::UserRole + 1,
    PrintableRole,       // object: printable flag (== visible, R-P0.5); instance: per-instance
    InstanceCountRole,   // object rows
    VolumeCountRole,     // object rows
    SelectedRole,        // object rows: in the VM selection set
    IsCurrentPlateRole,  // plate rows
    VolumeTypeLabelRole, // volume rows ("Part"/"Modifier"/"Negative"/...)
    ExtruderIdRole,      // object/volume rows (-1 = inherit/n/a)
    SourceObjectIndexRole,
    VolumeIndexRole,
    InstanceIndexRole,
    PlateIndexRole,
    CanRenameRole
  };
  Q_ENUM(Roles)

  explicit SceneTreeModel(EditorViewModel *viewModel, ProjectService *service,
                          QObject *parent = nullptr);
  ~SceneTreeModel() override;

  // QAbstractItemModel -------------------------------------------------------
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  // Upstream renders Name + per-state icon columns inside one tree column
  // view (BitmapTextRenderer composes the Name cell, ObjectDataViewModel.cpp
  // :551-576); the QML delegate does the same composition, so one model column.
  int columnCount(const QModelIndex &parent = QModelIndex()) const override
  {
    Q_UNUSED(parent)
    return 1;
  }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  /// EditRole on a plate/object row routes to EditorViewModel::renamePlate /
  /// renameObject (both undo-wired). Returns false for rows upstream does not
  /// make editable (instance root/instance) or rejected input (empty name,
  /// upstream-illegal filename characters).
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;

  /// Upstream-canonical row identity lookups for the QML delegate: resolve a
  /// tree row to (source object index, volume index, instance index, plate
  /// index). -1 where not applicable. The delegate uses these to route
  /// selection / context-menu / printable requests back to the
  /// EditorViewModel by SOURCE index.
  Q_INVOKABLE int sourceObjectIndexAt(const QModelIndex &index) const;
  Q_INVOKABLE int volumeIndexAt(const QModelIndex &index) const;
  Q_INVOKABLE int instanceIndexAt(const QModelIndex &index) const;
  Q_INVOKABLE int plateIndexAt(const QModelIndex &index) const;
  Q_INVOKABLE int nodeTypeAt(const QModelIndex &index) const;

private:
  struct Node
  {
    NodeType type = NodePlate;
    Node *parent = nullptr;
    int row = 0; // index within parent's children
    // Upstream node identity fields (ObjectDataViewModel.hpp:88-103):
    int plateIndex = -1;         // itPlate: plate index; others: owning plate
    int sourceObjectIndex = -1;  // itObject; itInstance rows: owning object
    int volumeIndex = -1;        // itVolume
    int instanceIndex = -1;      // itInstance
    QVector<Node *> children;
  };

  void rebuild();
  void refreshRows();
  Node *nodeFor(const QModelIndex &index) const;
  QModelIndex indexFor(Node *node) const;
  Node *makeNode(NodeType type, Node *parent, int row);
  void buildObjectChildren(Node *objectNode, int sourceObjectIndex, int plateIndex);
  void clearTree();

  EditorViewModel *m_viewModel = nullptr;
  ProjectService *m_service = nullptr;
  Node *m_root = nullptr;
  QVector<Node *> m_nodes; // owning pool, mirrors upstream node heap ownership
};
