#pragma once

#include <QHash>
#include <QSortFilterProxyModel>
#include <QString>

class ConfigOptionModel;

// Client-side projection over ConfigOptionModel for the params UIs
// (LeftSidebar pages, SettingsDialog lists). Replaces the QML-side
// filterOptionIndices()+filterIndicesByPage() index-array orchestration:
// views bind to a declaratively instantiated proxy and QSortFilterProxyModel
// feeds mapped rows + dataChanged through to the delegates.
//
// Filtering reuses the row-wise ConfigOptionModel predicates (matchesFilter /
// matchesPage / matchesGroup) so the proxy can never drift from the legacy VM
// filter semantics. All properties use equality-guarded setters — a value
// that does not change never invalidates the mapping (the ≈130ms visibility-
// flip budget in LeftSidebar relies on tab toggles touching no proxy input).
//
// Presentation role: OptPrevGroupRole ("optPrevGroup", Qt::UserRole + 0x100)
// exposes the GroupRole of the previous *accepted* row so delegates can decide
// group headers without indexing neighbors (index-1 lookups break under a
// proxy because proxy rows are a filtered subset). UserRole+0x100 is far above
// the source model's largest role (UserRole+22) — a collision would shadow the
// source role (e.g. SourceRowRole's optIdx write path).
// NOTE: not final — qmlRegisterType wraps instances in QQmlElement<T>, which
// derives from T.
class ConfigOptionFilterProxy : public QSortFilterProxyModel
{
  Q_OBJECT
  // Upstream Tab page ("Quality"/"Strength"/...); empty = no page filter.
  Q_PROPERTY(QString page READ page WRITE setPage NOTIFY pageChanged)
  // Search needle; empty = match everything.
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
  // advancedMode=false rejects optMode >= 1 rows (upstream comSimple view).
  Q_PROPERTY(bool advancedMode READ advancedMode WRITE setAdvancedMode NOTIFY advancedModeChanged)
  // Option group ("Layer height"/"Line width"/...); empty = no group filter.
  Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY groupChanged)
  // When true, rows sort by the upstream process manifest order
  // (ConfigOptionModel::processOptionRank) instead of source row order.
  Q_PROPERTY(bool upstreamProcessOrder READ upstreamProcessOrder WRITE setUpstreamProcessOrder NOTIFY upstreamProcessOrderChanged)
  // Accepted row count for QML empty-state checks (SettingsDialog group
  // visibility). Intentionally a plain int — no QVariantList crosses to QML.
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  explicit ConfigOptionFilterProxy(QObject *parent = nullptr);

  QString page() const { return m_page; }
  void setPage(const QString &page);
  QString searchText() const { return m_searchText; }
  void setSearchText(const QString &searchText);
  bool advancedMode() const { return m_advancedMode; }
  void setAdvancedMode(bool advancedMode);
  QString group() const { return m_group; }
  void setGroup(const QString &group);
  bool upstreamProcessOrder() const { return m_upstreamProcessOrder; }
  void setUpstreamProcessOrder(bool upstreamProcessOrder);
  int count() const { return rowCount(); }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void pageChanged();
  void searchTextChanged();
  void advancedModeChanged();
  void groupChanged();
  void upstreamProcessOrderChanged();
  void countChanged();

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
  bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
  ConfigOptionModel *optionModel() const;
  void refreshSorting();

  QString m_page;
  QString m_searchText;
  bool m_advancedMode = false;
  QString m_group;
  bool m_upstreamProcessOrder = false;
};
