#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QPointF>
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
  QStringList enumValues{}; // upstream ConfigOptionDef::enum_values (preset keys), same order as enumLabels
  QString category{}; // "质量" | "填充" | "速度" | "温度" | "支撑" | "底座" | "冷却" | "回退" | "其他"
  QString group{};   // 对齐上游 ConfigOptionsGroup — 子分组标题
  QString page{};     // 对齐上游 Tab Page — 顶层页面 (Quality/Strength/Speed/Support/Other) (empty = auto-detect)
  bool readonly = false;
  QString tooltip{};  // 帮助文案（对齐上游 ConfigOptionDef::tooltip）
  QString sidetext{}; // 输入框右侧单位文本（对齐上游 ConfigOptionDef::sidetext）
  int mode = 2;       // 0=comSimple, 1=comAdvanced, 2=comExpert, 3=comDevelop (对齐上游 ConfigOptionMode)
  bool nullable = false;  // accepts nil value = inherit from parent preset (upstream ConfigOptionDef::nullable)
  bool multiline = false; // upstream ConfigOptionDef::multiline (multi-line text editor)
  /// Open enum (GUIType::i_enum_open / f_enum_open / select_open): type is
  /// "enum_open", enumValues are suggestions and any value of this base
  /// type ("int" | "float" | "floatOrPercent" | "string") is accepted.
  QString openBase{};
  /// gui_flags "serialized" (post_process): a string list edited as one
  /// ";"-joined text (OptionsGroup::config_value / GUI.cpp change_opt_value).
  bool serialized = false;
  bool isVector = false;  // multi-value per-extruder (upstream coFloats/coInts/coBools/coEnums/coPercents/coStrings where type has coVectorType=0x4000 bit set)

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
  /// Tab pages removed by the current config (processPageNames skips them).
  Q_PROPERTY(QStringList hiddenPages READ hiddenPages NOTIFY hiddenPagesChanged)
  /// Printer tier: extruder count (nozzle_diameter entries). With more than
  /// one, the Extruder page becomes "Extruder 1".."Extruder N" (Tab.cpp:5640).
  Q_PROPERTY(int extruderCount READ extruderCount NOTIFY extruderCountChanged)
  /// Extruder whose values the Extruder page rows show and edit.
  Q_PROPERTY(int activeExtruder READ activeExtruder WRITE setActiveExtruder NOTIFY activeExtruderChanged)

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
    ModeRole,
    NullableRole,
    IsVectorRole,
    SidetextRole,
    // Extended presentation roles (UserRole+19..+22). Consumed directly by
    // delegates through the ConfigOptionFilterProxy (which forwards unknown
    // roles to the source model).
    DisplayLabelRole,   // zh_CN row label: OptionLabelTable::rowLabels(), falls back to label
    UnitRole,           // optUnit(i) — sidetext-first + key-heuristic unit string
    ValueSourceRole,    // "printer" | "filament" | "print" | "default" mirror (see setValueSources)
    SourceRowRole,      // source row index — survives proxy re-mapping (optIdx write path)
    // Upstream dependent-option toggles (ConfigManipulation::toggle_field /
    // toggle_line, Tab::toggle_options), computed in core (OptionToggleRules)
    // and pushed through setToggleState().
    EnabledRole,        // optEnabled: field editable (toggle_field)
    LineVisibleRole,    // optLineVisible: line shown (toggle_line)
    MultilineRole,      // optMultiline: ConfigOptionDef::multiline string editor
    // Upstream multi-option Line (Tab.cpp `Line line = {...};
    // line.append_option(...)`): the first member is the line head and
    // renders the whole line; the other members only appear inside it.
    LineHeadRole,       // optLineHead: head key of the line this option belongs to ("" = single-option line)
    LineLabelRole,      // optLineLabel: zh line label (head rows only)
    SubLabelRole,       // optSubLabel: zh per-option label inside a line (ConfigOptionDef::label)
    // Filament "Setting Overrides" near-label checkbox (OptionToggleRules).
    OverrideRole,       // optOverride: 0 = no checkbox, 1 = override on, 2 = off (value inherited)
    OverrideEnabledRole,// optOverrideEnabled: checkbox enabled
    InheritedValueRole  // optInheritedValue: printer / process value shown while off
  };
  Q_ENUM(Roles)

  explicit ConfigOptionModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = {}) const override;
  int dataVersion() const { return m_dataVersion; }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
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
  /// 返回选项的显示模式 (对齐上游 ConfigOptionMode: 0=Simple, 1=Advanced, 2=Expert, 3=Develop)
  Q_INVOKABLE int optMode(int i) const;
  /// 返回选项是否被修改（与默认值不同）
  Q_INVOKABLE bool optIsDirty(int i) const;
  /// Returns whether this option accepts nil value = inherit from parent preset
  Q_INVOKABLE bool optNullable(int i) const;
  /// Returns whether this option is multi-value per-extruder (vector type)
  Q_INVOKABLE bool optIsVector(int i) const;
  /// Returns the sidetext (unit label) for this option from upstream metadata
  Q_INVOKABLE QString optSidetext(int i) const;
  /// Returns unique sorted group names currently present in the model
  Q_INVOKABLE QStringList groupNames() const;
  /// Returns the count of options whose option group matches the given group
  Q_INVOKABLE int countForGroup(const QString &group) const;
  /// Returns the count of dirty keys whose option group matches the given group
  Q_INVOKABLE int dirtyCountForGroup(const QString &group) const;
  /// 重置单个选项到默认值
  Q_INVOKABLE void resetOption(int row);
  /// 返回脏选项数量
  Q_INVOKABLE int dirtyCount() const;

  Q_INVOKABLE void setValue(int row, const QVariant &value);

  /// Upstream optgroup icon name for a group title ("" when none).
  Q_INVOKABLE QString groupIconName(const QString &group) const;

  /// Combo index for an enum value in any stored form: preset key
  /// ("klipper"), upstream label, or a row index written by older edits.
  Q_INVOKABLE int enumIndexOf(int row, const QVariant &value) const;
  /// Writes the enum_values key for a combo index -- the form preset JSON
  /// and set_deserialize use (an int index never deserializes into an enum).
  Q_INVOKABLE void setEnumIndex(int row, int index);

  // ── Plugin capability overrides (upstream GUIType::plugin_config) ──────
  // Wrappers over core PluginOverrides for PluginsConfigDialog.qml; the
  // dialog edits a working copy of the option's JSON text.
  Q_INVOKABLE int pluginOverrideCount(const QVariant &raw) const;
  /// [{pluginKey, capability, capabilityType, configText}] for each entry.
  Q_INVOKABLE QVariantList pluginOverrideEntries(const QVariant &raw) const;
  Q_INVOKABLE QString pluginOverridesError(const QVariant &raw) const;
  /// {ok, text, error}: replaces entry `index`'s cap_config.
  Q_INVOKABLE QVariantMap pluginOverrideSetConfig(const QString &raw, int index, const QString &configText) const;
  /// Text without entry `index` ("" once no override remains).
  Q_INVOKABLE QString pluginOverrideRemove(const QString &raw, int index) const;
  /// Semantic equality: whitespace / formatting differences do not dirty the preset.
  Q_INVOKABLE bool pluginOverridesEqual(const QString &a, const QString &b) const;
  /// Upstream preset_type_title: "工艺插件" / "材料插件" / "打印机插件".
  Q_INVOKABLE QString pluginConfigTitle() const;

  // ── Colour options (upstream GUIType::color / ColourPicker) ───────────
  /// "#RRGGBB" of the (first) colour, "" when undefined.
  Q_INVOKABLE QString colourOf(const QVariant &value) const;
  /// Writes the colour ("" = undefined); vector options keep their other
  /// elements (per-extruder / per-filament colours).
  Q_INVOKABLE void setColour(int row, const QString &colour);
  /// Open enum text (Field.cpp Choice::set_value): the label of a matching
  /// enum value, otherwise the value itself (first element for vectors).
  Q_INVOKABLE QString openEnumText(int row, const QVariant &value) const;
  /// Choice::get_value: a label picks its enum value, other text is parsed
  /// as the base type; returns false (nothing written) for invalid text.
  Q_INVOKABLE bool setOpenEnumText(int row, const QString &text);

  /// Filament Dependencies (Tab::compatible_widget_create): "All" checkbox
  /// + "Set ..." multi-choice over the printer / process presets.
  /// compatible_printers / compatible_prints rows have type "compatible_list".
  Q_INVOKABLE bool dependencyAll(int row) const;
  /// Ticking "All" clears the list; either way the matching *_condition
  /// field follows the checkbox (on_toggle: get_field(key_condition)->toggle).
  Q_INVOKABLE void setDependencyAll(int row, bool all);
  Q_INVOKABLE QStringList dependencyCandidates(int row) const;
  Q_INVOKABLE QStringList dependencySelection(int row) const;
  /// MultiChoiceDialog OK: every candidate checked stores the empty list
  /// ("All"), otherwise the checked names.
  Q_INVOKABLE void setDependencySelection(int row, const QStringList &names);
  Q_INVOKABLE QString dependencyDialogTitle(int row) const;
  Q_INVOKABLE QString dependencyDialogLabel(int row) const;
  /// Non-default presets of the depending type (ConfigViewModel feeds them).
  void setDependencyCandidates(const QString &key, const QStringList &names);

  // ── Point options (upstream coPoint / coPoints) ─────────────────────────
  // Values arrive either as preset-JSON lists of "XxY" strings or as the
  // legacy "x1,y1,x2,y2" serialization; both parse here.
  static QList<QPointF> parsePoints(const QVariant &value);
  /// "XxY, XxY, ..." text with integer coordinates (upstream
  /// get_thumbnails_string, Field.cpp:83-91).
  Q_INVOKABLE QString pointsText(const QVariant &value) const;
  /// Parses "XxY, XxY, ..." and writes it back as a list of "XxY" strings;
  /// returns false (no write) on malformed input, like Field.cpp:705-719.
  Q_INVOKABLE bool setPointsText(int row, const QString &text);
  /// First point's X (axis 0) or Y (axis 1).
  Q_INVOKABLE double pointCoord(const QVariant &value, int axis) const;
  /// Writes one coordinate of the first point (PointCtrl X/Y edit). Vector
  /// options keep their other elements.
  Q_INVOKABLE void setPointCoord(int row, int axis, double coord);
  Q_INVOKABLE int indexOfKey(const QString &key) const;

  // Safe count (avoids QVariantList exposure to QML V4 engine)
  Q_INVOKABLE int countForCategory(const QString &category) const;

  // Filter helpers (returns list of indices matching category)
  // NOTE: avoid calling from QML — QVariantList crashes Qt 6.10 V4
  Q_INVOKABLE QVariantList indicesForCategory(const QString &category) const;
  /// Returns list of unique page names (对齐上游 Tab::Page)
  Q_INVOKABLE QStringList pageNames() const;
  /// Localised page/group label for the active tier's hierarchy.
  Q_INVOKABLE QString hierarchyLabel(const QString &name) const;
  /// Page names of the active tier's upstream hierarchy (Print, Filament or
  /// Printer tab), in upstream order.
  Q_INVOKABLE QStringList processPageNames() const;
  QStringList hiddenPages() const;
  void setHiddenPages(const QSet<QString> &pages);
  int extruderCount() const { return m_extruderCount; }
  int activeExtruder() const { return m_activeExtruder; }
  void setActiveExtruder(int extruder);
  /// "Extruder 2" -> "Extruder"; other pages unchanged.
  Q_INVOKABLE QString basePageName(const QString &page) const;
  /// "Extruder N" -> N-1, "Extruder" -> 0, other pages -> -1.
  Q_INVOKABLE int extruderOfPage(const QString &page) const;
  Q_INVOKABLE QStringList processGroupsForPage(const QString &page) const;
  Q_INVOKABLE QStringList processMappingErrors() const;
  Q_INVOKABLE QList<int> orderedProcessIndicesForGroup(const QList<int> &candidateIndices,
                                                        const QString &page,
                                                        const QString &group) const;
  /// Returns indices filtered by page
  Q_INVOKABLE QList<int> filterIndicesByPage(const QList<int> &indices, const QString &page) const;
  /// Returns indices filtered by option group
  Q_INVOKABLE QList<int> filterIndicesByGroup(const QList<int> &indices, const QString &group) const;

  // ── Row-wise filter predicates (shared with ConfigOptionFilterProxy) ────
  // Same semantics as the matching arms of ConfigViewModel::filterOptionIndices
  // / filterIndicesByPage / filterIndicesByGroup so the QML-side proxy and the
  // legacy VM helpers cannot drift apart.
  /// Search match: empty needle always matches; needles >= 2 chars use the
  /// upstream-style fuzzy score (>= 60) over optLabel+optKey, shorter needles
  /// use substring. optMode >= 1 rows are rejected when advancedMode is false.
  bool matchesFilter(int row, const QString &needle, bool advancedMode) const;
  /// Same, against an upstream view mode: a row shows when its
  /// ConfigOptionDef::mode <= viewMode (0 Simple, 1 Advanced, 2 Expert,
  /// 3 Develop; OptionsGroup::is_visible) and toggle_line has not hidden it.
  /// The bool overload maps advanced to Expert and simple to Simple.
  bool matchesFilter(int row, const QString &needle, int viewMode) const;

  /// Replaces the toggle_line / toggle_field result set. Emits dataChanged
  /// (EnabledRole / LineVisibleRole) for affected rows and
  /// toggleVisibilityChanged when the hidden set changed.
  void setToggleState(const QSet<QString> &hiddenKeys, const QSet<QString> &disabledKeys);
  bool isLineHidden(const QString &key) const { return m_hiddenKeys.contains(key); }

  /// Line membership (see LineHeadRole). A member that is not the head never
  /// passes matchesFilter: the head row renders it.
  QString lineHeadForKey(const QString &key) const { return m_lineHeadByKey.value(key); }
  /// Rows of one line in upstream append_option order, filtered by the view
  /// mode (a member above the mode is hidden inside the line).
  bool matchesLineMember(int row, const QString &headKey, int viewMode) const;
  int lineMemberRank(int row) const;
  Q_INVOKABLE QStringList lineMemberKeys(const QString &headKey) const { return m_lineMembersByHead.value(headKey); }
  bool isFieldDisabled(const QString &key) const { return m_disabledKeys.contains(key); }

  /// Override rows: key -> {checked, checkEnabled, inheritedValue}.
  void setOverrideStates(const QHash<QString, QVariantMap> &states);
  /// Upstream override checkbox: on = the field starts from the inherited
  /// value, off = the filament value becomes nil (inherit again).
  Q_INVOKABLE void setOverrideChecked(int row, bool checked);
  /// Page match: empty page always matches; option page falls back to
  /// pageForCategory when unset (same as filterIndicesByPage).
  bool matchesPage(int row, const QString &page) const;
  /// Group match: empty group always matches (same as filterIndicesByGroup).
  bool matchesGroup(int row, const QString &group) const;
  /// Number of rows that would survive the process-page projection:
  /// matchesFilter && matchesPage(page) && (page,group) has a process
  /// definition containing the option key. Equivalent to the per-group
  /// orderedProcessIndicesForGroup non-empty check aggregated over groups.
  Q_INVOKABLE int processProjectedRowCount(const QString &page, const QString &needle, bool advancedMode) const;
  Q_INVOKABLE int processProjectedRowCountForMode(const QString &page, const QString &needle, int viewMode) const;
  /// Position of the option key inside processGroupDefinition(page,group)
  /// ->optionKeys (upstream manifest order); INT_MAX when absent.
  int processOptionRank(const QString &page, const QString &group, int row) const;

  /// Mirror of ConfigViewModel::valueSources_ ("printer"/"filament"/"print"/
  /// "default" per key). Stores the map, diffs per row and emits dataChanged
  /// with ValueSourceRole only for rows whose source actually changed.
  void setValueSources(const QHash<QString, QString> &sources);

  QHash<QString, QVariant> valuesByKey() const;
  QHash<QString, QVariant> defaultValuesByKey() const;
  void resetToDefaults();
  void applyValues(const QHash<QString, QVariant> &values);
  void setReferenceValues(const QHash<QString, QVariant> &values);
  void setReadonlyKeys(const QSet<QString> &keys);

#ifdef HAS_LIBSLIC3R
  // Populate model from upstream libslic3r print_config_def.
  // Replaces the hardcoded option list with schema-driven options.
  void loadFromUpstreamSchema();
  // Load machine-specific config keys (printer hardware params).
  void loadMachineSchema();
  // Load filament-specific config keys (material params).
  void loadFilamentSchema();
  // Load a tier (0 Print, 1 Filament, 2 Printer) from its upstream hierarchy.
  void loadTierSchema(int tier);
  // Generic schema loader driven by a key array.
  void loadSchemaFromKeys(const char *const keys[]);
#endif

signals:
  void countChanged();
  void optionValueChanged(const QString &key, const QVariant &value);
  void dataVersionChanged();
  void toggleVisibilityChanged();
  void hiddenPagesChanged();
  void extruderCountChanged();
  void activeExtruderChanged();

private:
  int findIndex(const QString &key) const;
  void assignProcessHierarchyMetadata();
  /// Real write path behind setData(ValueRole) and the setValue(row,value)
  /// Q_INVOKABLE wrapper: value write → dirty bookkeeping → dataChanged →
  /// optionValueChanged → dataVersion tick.
  void writeValue(int row, const QVariant &value);
  QList<ConfigOption> m_options;
  QSet<QString> m_baseReadonlyKeys;
  QHash<QString, QVariant> m_defaultValues;
  QHash<QString, QVariant> m_referenceValues;
  QSet<QString> m_dirtyKeys; ///< 被修改过的选项 key
  QSet<QString> m_hiddenKeys;   ///< toggle_line(false) keys
  QSet<QString> m_hiddenPages;  ///< pages removed by the config
  QHash<QString, QStringList> m_dependencyCandidates;
  QHash<QString, bool> m_dependencyAll;   ///< "All" checkbox once toggled
  /// *_condition rows are editable only while "All" is ticked.
  bool dependencyConditionOff(const QString &conditionKey) const;
  int m_extruderCount = 1;
  int m_activeExtruder = 0;
  /// Printer Extruder page vector rows edit one element (append_single_
  /// option_line(key, extruder_idx)).
  bool isExtruderRow(int row) const;
  /// The row value as edited: a one-element list holding the active
  /// extruder's entry for extruder rows, the stored value otherwise.
  QVariant editValue(int row) const;
  void refreshExtruderCount();
  QHash<QString, QString> m_lineHeadByKey;          ///< member key -> head key
  QHash<QString, QStringList> m_lineMembersByHead;  ///< head key -> members (head first)
  QHash<QString, QString> m_lineLabelByHead;        ///< head key -> zh line label
  QSet<QString> m_disabledKeys; ///< toggle_field(false) keys
  QHash<QString, QVariantMap> m_overrideStates; ///< Setting Overrides rows
  QHash<QString, QString> m_valueSourceByKey; ///< valueSource mirror (see setValueSources)
  int m_dataVersion = 0;
  int m_hierarchyTier = 0; ///< 0 Print, 1 Filament, 2 Printer (hierarchyDefinitions)
};
