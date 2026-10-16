// SceneTreeModel.cpp — X6 object-list panel scene-tree bridge (see the header
// for the upstream mirror contract and refresh routing).

#include "core/model/SceneTreeModel.h"

#include "core/services/ProjectService.h"
#include "core/viewmodels/EditorViewModel.h"

#include <QMetaEnum>

namespace
{
// Upstream per-instance row naming (ObjectDataViewModel.cpp:121-124):
// "Instance %d" (1-based).
QString instanceRowName(int instanceIndex)
{
  return QStringLiteral("Instance %1").arg(instanceIndex + 1);
}

// Upstream instance-root row naming (ObjectDataViewModel.cpp:117-119):
// dev-localized "Instances".
constexpr QLatin1String kInstanceRootName("Instances");
} // namespace

SceneTreeModel::SceneTreeModel(EditorViewModel *viewModel, ProjectService *service,
                               QObject *parent)
    : QAbstractItemModel(parent),
      m_viewModel(viewModel),
      m_service(service)
{
  m_root = new Node();
  m_root->type = NodePlate;
  m_root->row = 0;

  if (m_viewModel)
  {
    // S1 clusters (EditorViewModel.h:2052-2095). Topology/plate edits reshape
    // the node tree; selection/state edits only re-read the cells. The
    // UndoRedoManager relays its stateChanged into the VM umbrella
    // (EditorViewModel.cpp setUndoRedoManager), so undo/redo of a rename /
    // printable / move reaches refreshRows() with no extra plumbing.
    connect(m_viewModel, &EditorViewModel::sceneTopologyChanged,
            this, &SceneTreeModel::rebuild);
    connect(m_viewModel, &EditorViewModel::plateChanged,
            this, &SceneTreeModel::rebuild);
    connect(m_viewModel, &EditorViewModel::currentPlateIndexChanged,
            this, &SceneTreeModel::rebuild);
    connect(m_viewModel, &EditorViewModel::selectionChanged,
            this, &SceneTreeModel::refreshRows);
    connect(m_viewModel, &EditorViewModel::stateChanged,
            this, &SceneTreeModel::refreshRows);
  }
  rebuild();
}

SceneTreeModel::~SceneTreeModel()
{
  clearTree();
  delete m_root;
}

void SceneTreeModel::clearTree()
{
  // Ownership: every non-root node lives in m_nodes (makeNode appends
  // unconditionally), so deleting the pool frees the whole child tree. The
  // root itself is NOT in the pool and outlives rebuilds -- only its child
  // list needs clearing. Do NOT touch the freed children here: resetting
  // their parent pointers after qDeleteAll is a use-after-free WRITE that
  // corrupts the heap (this exact loop crashed ViewModelSmokeTests under
  // c0000374 once rebuild ran often enough; 2026-10-16).
  qDeleteAll(m_nodes);
  m_nodes.clear();
  m_root->children.clear();
}

// ── tree construction ────────────────────────────────────────────────────────

SceneTreeModel::Node *SceneTreeModel::makeNode(NodeType type, Node *parent, int row)
{
  Node *node = new Node();
  node->type = type;
  node->parent = parent;
  node->row = row;
  m_nodes.append(node);
  parent->children.append(node);
  return node;
}

void SceneTreeModel::buildObjectChildren(Node *objectNode, int sourceObjectIndex,
                                         int plateIndex)
{
  if (!m_service)
    return;

  objectNode->sourceObjectIndex = sourceObjectIndex;
  objectNode->plateIndex = plateIndex;

  // Volume children (itVolume). Volumes render read-only in this first panel
  // increment: the ProjectService has no moveVolume/renameVolume surface yet
  // (parity doc §7), so the rows display name + type label only.
  const int volumeCount = m_service->objectVolumeCount(sourceObjectIndex);
  for (int v = 0; v < volumeCount; ++v)
  {
    Node *volumeNode = makeNode(NodeVolume, objectNode, objectNode->children.size());
    volumeNode->sourceObjectIndex = sourceObjectIndex;
    volumeNode->volumeIndex = v;
    volumeNode->plateIndex = plateIndex;
  }

  // Instance root + per-instance rows (itInstanceRoot/itInstance), upstream
  // AddInstanceChild shape (ObjectDataViewModel.cpp:779-830): one container
  // row, then one "Instance N" row per instance.
  const int instanceCount = m_service->objectInstanceCount(sourceObjectIndex);
  if (instanceCount > 0)
  {
    Node *instanceRoot = makeNode(NodeInstanceRoot, objectNode,
                                  objectNode->children.size());
    instanceRoot->sourceObjectIndex = sourceObjectIndex;
    instanceRoot->plateIndex = plateIndex;
    for (int i = 0; i < instanceCount; ++i)
    {
      Node *instanceNode = makeNode(NodeInstance, instanceRoot,
                                    instanceRoot->children.size());
      instanceNode->sourceObjectIndex = sourceObjectIndex;
      instanceNode->instanceIndex = i;
      instanceNode->plateIndex = plateIndex;
    }
  }
}

void SceneTreeModel::rebuild()
{
  beginResetModel();
  clearTree();

  if (m_service)
  {
    const int plateCount = m_service->plateCount();
    for (int p = 0; p < plateCount; ++p)
    {
      Node *plateNode = makeNode(NodePlate, m_root, m_root->children.size());
      plateNode->plateIndex = p;
      const QList<int> objectIndices = m_service->plateObjectIndices(p);
      for (int sourceObjectIndex : objectIndices)
      {
        Node *objectNode = makeNode(NodeObject, plateNode, plateNode->children.size());
        buildObjectChildren(objectNode, sourceObjectIndex, p);
      }
    }
  }

  endResetModel();
}

void SceneTreeModel::refreshRows()
{
  // Cell-level refresh over the existing tree (no restructure). Cheap reads:
  // name from the service's parallel QStringList, printable flags from the
  // bool lists, selection from the VM set. Emits dataChanged per row so the
  // delegate bindings (selected / printable / name) re-evaluate without a
  // reset (expansion state and current index survive).
  for (Node *plateNode : std::as_const(m_root->children))
  {
    const QModelIndex plateIdx = indexFor(plateNode);
    if (!plateIdx.isValid())
      continue;
    Q_EMIT dataChanged(plateIdx, plateIdx);
    for (Node *objectNode : std::as_const(plateNode->children))
    {
      const QModelIndex objectIdx = indexFor(objectNode);
      if (!objectIdx.isValid())
        continue;
      Q_EMIT dataChanged(objectIdx, objectIdx);
      for (Node *child : std::as_const(objectNode->children))
      {
        if (child->type == NodeInstanceRoot)
        {
          const QModelIndex rootIdx = indexFor(child);
          if (rootIdx.isValid())
            Q_EMIT dataChanged(rootIdx, rootIdx);
          for (Node *instance : std::as_const(child->children))
          {
            const QModelIndex instIdx = indexFor(instance);
            if (instIdx.isValid())
              Q_EMIT dataChanged(instIdx, instIdx);
          }
        }
        else if (child->type == NodeVolume)
        {
          const QModelIndex volIdx = indexFor(child);
          if (volIdx.isValid())
            Q_EMIT dataChanged(volIdx, volIdx);
        }
      }
    }
  }
}

// ── QAbstractItemModel -------------------------------------------------------

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex &parent) const
{
  if (row < 0 || column != 0)
    return {};
  Node *parentNode = parent.isValid() ? nodeFor(parent) : m_root;
  if (!parentNode || row >= parentNode->children.size())
    return {};
  return createIndex(row, column, parentNode->children[row]);
}

QModelIndex SceneTreeModel::parent(const QModelIndex &child) const
{
  if (!child.isValid())
    return {};
  Node *node = nodeFor(child);
  if (!node || !node->parent || node->parent == m_root)
    return {};
  return createIndex(node->parent->row, 0, node->parent);
}

int SceneTreeModel::rowCount(const QModelIndex &parent) const
{
  if (parent.column() > 0)
    return 0;
  Node *parentNode = parent.isValid() ? nodeFor(parent) : m_root;
  return parentNode ? parentNode->children.size() : 0;
}

SceneTreeModel::Node *SceneTreeModel::nodeFor(const QModelIndex &index) const
{
  if (!index.isValid() || index.column() != 0)
    return nullptr;
  return static_cast<Node *>(index.internalPointer());
}

QModelIndex SceneTreeModel::indexFor(Node *node) const
{
  if (!node || node == m_root)
    return {};
  return createIndex(node->row, 0, node);
}

QVariant SceneTreeModel::data(const QModelIndex &index, int role) const
{
  Node *node = nodeFor(index);
  if (!node || !m_service)
    return {};

  switch (role)
  {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch (node->type)
    {
    case NodePlate:
      return m_viewModel ? m_viewModel->plateName(node->plateIndex) : QString();
    case NodeObject:
      return m_service->objectNames().value(node->sourceObjectIndex);
    case NodeVolume:
      return m_service->objectVolumeName(node->sourceObjectIndex, node->volumeIndex);
    case NodeInstanceRoot:
      return QString(kInstanceRootName);
    case NodeInstance:
      return instanceRowName(node->instanceIndex);
    }
    break;
  case Qt::ToolTipRole:
    if (node->type == NodeObject)
      return m_service->objectNames().value(node->sourceObjectIndex);
    break;
  default:
    break;
  }

  switch (role)
  {
  case NodeTypeRole:
    return int(node->type);
  case PrintableRole:
    // Real booleans for every row (never invalid QVariant): the QML
    // delegate binds this role as a required bool property on ALL rows, and
    // Qt 6.12 warns on undefined-to-bool assignment for inapplicable rows.
    switch (node->type)
    {
    case NodeObject:
      // R-P0.5: the object-level eye/printable column is ONE flag
      // (ProjectService::setObjectVisible routes into setObjectPrintable).
      return m_service->objectPrintable(node->sourceObjectIndex);
    case NodeInstance:
      return m_service->instancePrintable(node->sourceObjectIndex,
                                          node->instanceIndex);
    default:
      return false;
    }
  case InstanceCountRole:
    return node->type == NodeObject
               ? m_service->objectInstanceCount(node->sourceObjectIndex)
               : 0;
  case VolumeCountRole:
    return node->type == NodeObject
               ? m_service->objectVolumeCount(node->sourceObjectIndex)
               : 0;
  case SelectedRole:
    return node->type == NodeObject && m_viewModel
               ? m_viewModel->objectSelectedBySource(node->sourceObjectIndex)
               : false;
  case IsCurrentPlateRole:
    // Real boolean on every row (see PrintableRole note above).
    return node->type == NodePlate
               && node->plateIndex == m_service->currentPlateIndex();
  case VolumeTypeLabelRole:
    return node->type == NodeVolume
               ? m_service->objectVolumeTypeLabel(node->sourceObjectIndex,
                                                  node->volumeIndex)
               : QString();
  case ExtruderIdRole:
    switch (node->type)
    {
    case NodeObject:
      // Upstream initial_extruder reads the item config "extruder" key
      // (GUI_Factories.cpp:1915-1922); -1 marks "not configured / inherit".
      return m_service->scopedOptionValue(node->sourceObjectIndex, -1,
                                          QStringLiteral("extruder"),
                                          QVariant(-1)).toInt();
    case NodeVolume:
      return m_service->volumeExtruderId(node->sourceObjectIndex, node->volumeIndex);
    default:
      return -1;
    }
  case SourceObjectIndexRole:
    return node->sourceObjectIndex;
  case VolumeIndexRole:
    return node->volumeIndex;
  case InstanceIndexRole:
    return node->instanceIndex;
  case PlateIndexRole:
    return node->plateIndex;
  case CanRenameRole:
    // Upstream editable renderer gate (GUI_ObjectList.cpp:418-421):
    // itVolume | itObject | itPlate. The volume write surface does not exist
    // on ProjectService yet (parity §7), so volumes stay read-only here.
    return node->type == NodePlate || node->type == NodeObject;
  default:
    return {};
  }
}

bool SceneTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  Node *node = nodeFor(index);
  if (!node || role != Qt::EditRole || !m_viewModel)
    return false;
  if (node->type != NodePlate && node->type != NodeObject)
    return false; // upstream only edits name cells of plate/object/volume rows;
                  // volume rename has no service surface yet (parity §7)

  const QString newName = value.toString();
  if (newName.isEmpty())
    return false; // upstream rename_item: empty input cancels (:6089-6090)

  if (node->type == NodePlate)
    return m_viewModel->renamePlate(node->plateIndex, newName);
  return m_viewModel->renameObject(node->sourceObjectIndex, newName);
}

Qt::ItemFlags SceneTreeModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags base = QAbstractItemModel::flags(index);
  if (!index.isValid())
    return base;
  if (Node *node = nodeFor(index); node && (node->type == NodePlate || node->type == NodeObject))
    return base | Qt::ItemIsEditable;
  return base;
}

QHash<int, QByteArray> SceneTreeModel::roleNames() const
{
  // Qt::DisplayRole/Qt::EditRole are mapped explicitly: the default
  // QAbstractItemModel::roleNames() mapping is fully replaced by this
  // override, and the QML TreeView delegate reads the row name through the
  // "display" required property (and setData's EditRole round-trip in tests).
  static const QHash<int, QByteArray> kRoles = {
      {Qt::DisplayRole, "display"},
      {Qt::EditRole, "edit"},
      {NodeTypeRole, "nodeType"},
      {PrintableRole, "printable"},
      {InstanceCountRole, "instanceCount"},
      {VolumeCountRole, "volumeCount"},
      // SelectedRole maps to "objectSelected" -- NOT "selected":
      // QQuickTreeView injects a view-driven
      // `required property bool selected` (selection-model backed) that
      // would shadow any same-named model role in delegates.
      {SelectedRole, "objectSelected"},
      {IsCurrentPlateRole, "isCurrentPlate"},
      {VolumeTypeLabelRole, "volumeTypeLabel"},
      {ExtruderIdRole, "extruderId"},
      {SourceObjectIndexRole, "sourceObjectIndex"},
      {VolumeIndexRole, "volumeIndex"},
      {InstanceIndexRole, "instanceIndex"},
      {PlateIndexRole, "plateIndex"},
      {CanRenameRole, "canRename"},
  };
  return kRoles;
}

int SceneTreeModel::sourceObjectIndexAt(const QModelIndex &index) const
{
  Node *node = nodeFor(index);
  return node ? node->sourceObjectIndex : -1;
}

int SceneTreeModel::volumeIndexAt(const QModelIndex &index) const
{
  Node *node = nodeFor(index);
  return node ? node->volumeIndex : -1;
}

int SceneTreeModel::instanceIndexAt(const QModelIndex &index) const
{
  Node *node = nodeFor(index);
  return node ? node->instanceIndex : -1;
}

int SceneTreeModel::plateIndexAt(const QModelIndex &index) const
{
  Node *node = nodeFor(index);
  return node ? node->plateIndex : -1;
}

int SceneTreeModel::nodeTypeAt(const QModelIndex &index) const
{
  Node *node = nodeFor(index);
  return node ? int(node->type) : -1;
}
