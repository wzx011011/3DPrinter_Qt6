#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVariant>

// G1 — ConfigOptionModel: QAbstractListModel exposing ~30 print parameters.
// Driven by QJsonArray-like structs; no dependency on DynamicPrintConfig.
struct ConfigOption
{
  QString key;
  QString label;
  QString type; // "double" | "int" | "bool" | "enum"
  QVariant value;
  double min = 0;
  double max = 0;
  double step = 1;
  QStringList enumLabels;
  QString category; // "质量" | "填充" | "速度" | "温度" | "支撑" | "底座" | "冷却" | "回退" | "其他"
  bool readonly = false;
};

class ConfigOptionModel final : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Roles
  {
    KeyRole = Qt::UserRole + 1,
    LabelRole,
    TypeRole,
    ValueRole,
    MinRole,
    MaxRole,
    StepRole,
    EnumLabelsRole,
    CategoryRole,
    ReadonlyRole
  };
  Q_ENUM(Roles)

  explicit ConfigOptionModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Safe individual accessors to avoid QVariantList exposure issues
  Q_INVOKABLE QString optKey(int i) const;
  Q_INVOKABLE QString optLabel(int i) const;
  Q_INVOKABLE QString optType(int i) const;
  Q_INVOKABLE QVariant optValue(int i) const;
  Q_INVOKABLE double optMin(int i) const;
  Q_INVOKABLE double optMax(int i) const;
  Q_INVOKABLE double optStep(int i) const;
  Q_INVOKABLE QString optCategory(int i) const;
  Q_INVOKABLE bool optReadonly(int i) const;
  Q_INVOKABLE int optEnumCount(int i) const;
  Q_INVOKABLE QString optEnumLabel(int i, int j) const;

  Q_INVOKABLE void setValue(int row, const QVariant &value);
  Q_INVOKABLE int indexOfKey(const QString &key) const;

  // Safe count (avoids QVariantList exposure to QML V4 engine)
  Q_INVOKABLE int countForCategory(const QString &category) const;

  // Filter helpers (returns list of indices matching category)
  // NOTE: avoid calling from QML — QVariantList crashes Qt 6.10 V4
  Q_INVOKABLE QVariantList indicesForCategory(const QString &category) const;

  QHash<QString, QVariant> valuesByKey() const;
  void applyValues(const QHash<QString, QVariant> &values);
  void setReadonlyKeys(const QSet<QString> &keys);

#ifdef HAS_LIBSLIC3R
  // Populate model from upstream libslic3r print_config_def.
  // Replaces the hardcoded option list with schema-driven options.
  void loadFromUpstreamSchema();
#endif

signals:
  void countChanged();
  void optionValueChanged(const QString &key, const QVariant &value);

private:
  int findIndex(const QString &key) const;
  QList<ConfigOption> m_options;
  QSet<QString> m_baseReadonlyKeys;
};
