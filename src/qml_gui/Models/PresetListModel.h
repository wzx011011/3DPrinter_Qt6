#pragma once

#include <QAbstractListModel>
#include <QStringList>

// G2 — PresetListModel: Print / Filament / Printer three-category preset list
struct PresetEntry
{
  QString name;
  QString category; // "打印质量" | "耗材" | "打印机"
  bool isDefault = false;
};

class PresetListModel final : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    CategoryRole,
    IsDefaultRole
  };
  Q_ENUM(Roles)

  explicit PresetListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Safe individual accessors
  Q_INVOKABLE QString presetName(int i) const;
  Q_INVOKABLE QString presetCategory(int i) const;
  Q_INVOKABLE bool presetIsDefault(int i) const;

  // Filtered name list per category (returns QVariantList of names — acceptable here,
  // consumer uses index-based accessor to fetch individual names)
  Q_INVOKABLE int countByCategory(const QString &category) const;
  Q_INVOKABLE int globalIndex(const QString &category, int localIndex) const;

signals:
  void countChanged();

private:
  QList<PresetEntry> m_presets;
};
