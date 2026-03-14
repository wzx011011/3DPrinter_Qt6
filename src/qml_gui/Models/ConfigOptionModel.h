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
  QStringList enumLabels{};
  QString category{}; // "质量" | "填充" | "速度" | "温度" | "支撑" | "底座" | "冷却" | "回退" | "其他"
  QString group{};   // 对齐上游 ConfigOptionsGroup — 子分组标题
  QString page{};     // 对齐上游 Tab Page — 顶层页面 (Quality/Strength/Speed/Support/Other) (empty = auto-detect)
  bool readonly = false;
  QString tooltip{}; // 帮助文案（对齐上游 ConfigOptionDef::tooltip）
  int mode = 2;       // 0=Simple only, 1=Advanced only, 2=Both (对齐上游 ConfigOptionMode)

  // Constructor for 10-field aggregate init (key, label, type, value, min, max, step, enumLabels, category, group)
  ConfigOption(QString k, QString l, QString t, QVariant v, double mn, double mx, double s,
                QStringList el, QString c, QString g)
    : key(std::move(k)), label(std::move(l)), type(std::move(t)), value(std::move(v)),
      min(mn), max(mx), step(s), enumLabels(std::move(el)),
      category(std::move(c)), group(std::move(g)) {}

  // Constructor with 11 fields (adds readonly)
  ConfigOption(QString k, QString l, QString t, QVariant v, double mn, double mx, double s,
                QStringList el, QString c, QString g, bool ro)
    : key(std::move(k)), label(std::move(l)), type(std::move(t)), value(std::move(v)),
      min(mn), max(mx), step(s), enumLabels(std::move(el)),
      category(std::move(c)), group(std::move(g)), readonly(ro) {}

  // Constructor with 12 fields (adds mode)
  ConfigOption(QString k, QString l, QString t, QVariant v, double mn, double mx, double s,
                QStringList el, QString c, QString g, bool ro, int m)
    : key(std::move(k)), label(std::move(l)), type(std::move(t)), value(std::move(v)),
      min(mn), max(mx), step(s), enumLabels(std::move(el)),
      category(std::move(c)), group(std::move(g)), readonly(ro), mode(m) {}

  ConfigOption() = default;
};

class ConfigOptionModel final : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int dataVersion READ dataVersion NOTIFY dataVersionChanged)

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
    GroupRole,
    PageRole,
    ReadonlyRole,
    DirtyRole,
    TooltipRole,
    ModeRole
  };
  Q_ENUM(Roles)

  explicit ConfigOptionModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = {}) const override;
  int dataVersion() const { return m_dataVersion; }
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
  Q_INVOKABLE QString optGroup(int i) const;
  /// Returns the upstream Tab page for this option (对齐上游 Tab::Page)
  Q_INVOKABLE QString optPage(int i) const;
  Q_INVOKABLE bool optReadonly(int i) const;
  Q_INVOKABLE int optEnumCount(int i) const;
  Q_INVOKABLE QString optEnumLabel(int i, int j) const;
  /// Returns all enum labels for option i as QStringList (replaces QML inline enumOptionLabels)
  Q_INVOKABLE QStringList optEnumLabelsList(int i) const;
  /// Returns the display unit string based on option key (replaces QML inline regex unit detection)
  Q_INVOKABLE QString optUnit(int i) const;
  /// 返回选项的帮助文案
  Q_INVOKABLE QString optTooltip(int i) const;
  /// 返回选项的显示模式 (对齐上游 ConfigOptionMode: 0=Simple, 1=Advanced, 2=Both)
  Q_INVOKABLE int optMode(int i) const;
  /// 返回选项是否被修改（与默认值不同）
  Q_INVOKABLE bool optIsDirty(int i) const;
  /// 重置单个选项到默认值
  Q_INVOKABLE void resetOption(int row);
  /// 返回脏选项数量
  Q_INVOKABLE int dirtyCount() const;

  Q_INVOKABLE void setValue(int row, const QVariant &value);
  Q_INVOKABLE int indexOfKey(const QString &key) const;

  // Safe count (avoids QVariantList exposure to QML V4 engine)
  Q_INVOKABLE int countForCategory(const QString &category) const;

  // Filter helpers (returns list of indices matching category)
  // NOTE: avoid calling from QML — QVariantList crashes Qt 6.10 V4
  Q_INVOKABLE QVariantList indicesForCategory(const QString &category) const;
  /// Returns list of unique page names (对齐上游 Tab::Page)
  Q_INVOKABLE QStringList pageNames() const;
  /// Returns indices filtered by page
  Q_INVOKABLE QList<int> filterIndicesByPage(const QList<int> &indices, const QString &page) const;

  QHash<QString, QVariant> valuesByKey() const;
  QHash<QString, QVariant> defaultValuesByKey() const;
  void resetToDefaults();
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
  void dataVersionChanged();

private:
  int findIndex(const QString &key) const;
  QList<ConfigOption> m_options;
  QSet<QString> m_baseReadonlyKeys;
  QHash<QString, QVariant> m_defaultValues;
  QSet<QString> m_dirtyKeys; ///< 被修改过的选项 key
  int m_dataVersion = 0;
};
