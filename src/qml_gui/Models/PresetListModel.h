#pragma once

#include <QAbstractListModel>
#include <QStringList>

// G2 — PresetListModel: Print / Filament / Printer three-category preset list
struct PresetEntry
{
  QString name;
  QString category; // "打印质量" | "耗材" | "打印机"
  bool isDefault = false;
  /// v5.16 (PSET2-05): combo section ("用户预设"/"系统预设", upstream
  /// PresetComboBoxes.cpp:1281-1317 User/System separators).
  QString section;
  /// v5.16 (PSET2-05): incompatible with the active printer (drives the
  /// in-list graying, upstream LABEL_ITEM_DISABLED).
  bool incompatible = false;
};

class PresetServiceMock;

class PresetListModel final : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    CategoryRole,
    IsDefaultRole,
    SectionRole,
    IncompatibleRole
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
  Q_INVOKABLE QString presetSection(int i) const;
  Q_INVOKABLE bool presetIncompatible(int i) const;

  // Filtered name list per category (returns QVariantList of names — acceptable here,
  // consumer uses index-based accessor to fetch individual names)
  Q_INVOKABLE int countByCategory(const QString &category) const;
  Q_INVOKABLE int globalIndex(const QString &category, int localIndex) const;

  /// Refresh from PresetServiceMock (populates from loaded vendor presets)
  void refreshFromService(PresetServiceMock *service);
  /// v5.16 (PSET2-05): refresh with an active printer — entries of the
  /// filament/print categories get `incompatible` computed against it
  /// (upstream PresetBundle::update_compatible). An empty printerName skips
  /// the compatibility pass.
  void refreshFromService(PresetServiceMock *service, const QString &printerName);

signals:
  void countChanged();

private:
  QList<PresetEntry> m_presets;
};
