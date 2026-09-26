#include "qml_gui/Models/ConfigOptionFilterProxy.h"

#include "qml_gui/Models/ConfigOptionModel.h"

namespace
{
// Presentation role owned by the proxy. Qt::UserRole + 0x100 keeps it far
// above the source model's largest role (UserRole+22) — a collision would
// shadow a source role (e.g. SourceRowRole's optIdx write path) because
// data() intercepts owned roles before forwarding to the base class.
constexpr int OptPrevGroupRole = Qt::UserRole + 0x100;
} // namespace

ConfigOptionFilterProxy::ConfigOptionFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
  // Property changes re-filter/re-sort only through the setters' explicit
  // invalidate()/sort() calls. In particular, value dataChanged from the
  // source model must never trigger re-filtering or re-ordering — it only
  // forwards through the mapping so visible delegates refresh in place.
  setDynamicSortFilter(false);

  // countChanged sources: any proxy mapping change that can alter rowCount().
  // modelReset is redundant in this codebase (source models reset only during
  // construction, before QML creates the proxy) but kept for robustness.
  connect(this, &QAbstractItemModel::rowsInserted, this, &ConfigOptionFilterProxy::countChanged);
  connect(this, &QAbstractItemModel::rowsRemoved, this, &ConfigOptionFilterProxy::countChanged);
  connect(this, &QAbstractItemModel::layoutChanged, this, &ConfigOptionFilterProxy::countChanged);
  connect(this, &QAbstractItemModel::modelReset, this, &ConfigOptionFilterProxy::countChanged);
}

void ConfigOptionFilterProxy::setPage(const QString &page)
{
  if (m_page == page)
    return;
  m_page = page;
  emit pageChanged();
  invalidate();
  refreshSorting();
}

void ConfigOptionFilterProxy::setSearchText(const QString &searchText)
{
  if (m_searchText == searchText)
    return;
  m_searchText = searchText;
  emit searchTextChanged();
  invalidate();
  refreshSorting();
}

void ConfigOptionFilterProxy::setAdvancedMode(bool advancedMode)
{
  if (m_advancedMode == advancedMode)
    return;
  m_advancedMode = advancedMode;
  emit advancedModeChanged();
  invalidate();
  refreshSorting();
}

void ConfigOptionFilterProxy::setGroup(const QString &group)
{
  if (m_group == group)
    return;
  m_group = group;
  emit groupChanged();
  invalidate();
  refreshSorting();
}

void ConfigOptionFilterProxy::setUpstreamProcessOrder(bool upstreamProcessOrder)
{
  if (m_upstreamProcessOrder == upstreamProcessOrder)
    return;
  m_upstreamProcessOrder = upstreamProcessOrder;
  emit upstreamProcessOrderChanged();
  refreshSorting();
}

QVariant ConfigOptionFilterProxy::data(const QModelIndex &index, int role) const
{
  // Intercept the proxy-owned presentation role first; everything else
  // (including the source roles forwarded by QSortFilterProxyModel) goes to
  // the base class unchanged.
  if (role == OptPrevGroupRole)
  {
    if (!index.isValid() || index.row() <= 0 || !sourceModel())
      return QString();
    const QModelIndex prevSource = mapToSource(this->index(index.row() - 1, 0));
    return sourceModel()->data(prevSource, ConfigOptionModel::GroupRole);
  }
  return QSortFilterProxyModel::data(index, role);
}

QHash<int, QByteArray> ConfigOptionFilterProxy::roleNames() const
{
  // QAbstractProxyModel forwards the source roleNames() in Qt 6; add the one
  // proxy-owned presentation role on top.
  QHash<int, QByteArray> roles = QSortFilterProxyModel::roleNames();
  roles.insert(OptPrevGroupRole, QByteArrayLiteral("optPrevGroup"));
  return roles;
}

bool ConfigOptionFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
  if (sourceParent.isValid())
    return false;
  // Non-ConfigOptionModel sources pass through untouched (defensive).
  ConfigOptionModel *model = optionModel();
  if (!model)
    return true;
  return model->matchesFilter(sourceRow, m_searchText, m_advancedMode)
      && model->matchesPage(sourceRow, m_page)
      && model->matchesGroup(sourceRow, m_group);
}

bool ConfigOptionFilterProxy::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
  // Upstream process manifest order. QSortFilterProxyModel reads sort-role
  // data from the *source* model, which does not know the proxy's page/group,
  // so the rank is computed here instead of through a sort role.
  if (m_upstreamProcessOrder)
  {
    if (ConfigOptionModel *model = optionModel())
    {
      const int leftRank = model->processOptionRank(m_page, m_group, sourceLeft.row());
      const int rightRank = model->processOptionRank(m_page, m_group, sourceRight.row());
      if (leftRank != rightRank)
        return leftRank < rightRank;
      // Deterministic tiebreak for keys absent from the manifest.
      return sourceLeft.row() < sourceRight.row();
    }
  }
  return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

ConfigOptionModel *ConfigOptionFilterProxy::optionModel() const
{
  return qobject_cast<ConfigOptionModel *>(sourceModel());
}

void ConfigOptionFilterProxy::refreshSorting()
{
  if (m_upstreamProcessOrder && optionModel())
    sort(0, Qt::AscendingOrder);
  else
    sort(-1); // source row order
}
