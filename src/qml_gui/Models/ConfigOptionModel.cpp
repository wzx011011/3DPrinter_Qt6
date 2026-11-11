#include "ConfigOptionModel.h"

#include "core/services/PluginOverrides.h"
#include "core/util/PrintableArea.h"
#include "qml_gui/Models/OptionLabelTable.h"

#include <QByteArray>
#include <algorithm>
#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QPair>
#include <QVector>
#include <climits>
#include <initializer_list>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Config.hpp>
#include <libslic3r/Preset.hpp>
#endif

namespace
{
  // Defined with the line table below (upstream multi-option lines).
  static const QHash<QString, QString> &lineSubLabels();
} // namespace

// Defined with the zh enum-label table below.
static QString zhEnumLabel(const QString &label);

ConfigOptionModel::ConfigOptionModel(QObject *parent)
    : QAbstractListModel(parent)
{
  // Q11-②: no constructor seed. The hand-written ~90-option table plus the
  // mock tooltip map used to be built here, but every HAS_LIBSLIC3R build
  // (the only real configuration; BUILD_LIBSLIC3R=OFF is an explicit stub)
  // wipes it immediately — ConfigViewModel loads the three tiers straight
  // from the upstream schema (loadFromUpstreamSchema/loadMachineSchema/
  // loadFilamentSchema -> loadSchemaFromKeys clears m_options and rebuilds
  // readonly/default snapshots). Seeding here was a second source of truth
  // that could never survive the first reset; the schema is now the single
  // source. Row metadata (page/group/readonly/defaults) is assigned by the
  // loaders after the rebuild, exactly as before.
}


// Category-to-Page mapping (对齐上游 Tab::Page hierarchy)
static QString pageForCategory(const QString &cat)
{
  static const QHash<QString, QString> map = {
    // 中文 category → English page
    {QStringLiteral("质量"), QStringLiteral("Quality")},
    {QStringLiteral("填充"), QStringLiteral("Strength")},
    {QStringLiteral("速度"), QStringLiteral("Speed")},
    {QStringLiteral("加速度"), QStringLiteral("Speed")},
    {QStringLiteral("温度"), QStringLiteral("Temperature")},
    {QStringLiteral("支撑"), QStringLiteral("Support")},
    {QStringLiteral("底座"), QStringLiteral("Base")},
    {QStringLiteral("冷却"), QStringLiteral("Cooling")},
    {QStringLiteral("回退"), QStringLiteral("Retraction")},
    {QStringLiteral("挤出机"), QStringLiteral("Other")},
    {QStringLiteral("输出"), QStringLiteral("Other")},
    {QStringLiteral("其他"), QStringLiteral("Other")},
    // English category (for upstream schema mode)
    {QStringLiteral("Quality"), QStringLiteral("Quality")},
    {QStringLiteral("Infill"), QStringLiteral("Strength")},
    {QStringLiteral("Speed"), QStringLiteral("Speed")},
    {QStringLiteral("Acceleration"), QStringLiteral("Speed")},
    {QStringLiteral("Temperature"), QStringLiteral("Temperature")},
    {QStringLiteral("Support"), QStringLiteral("Support")},
    {QStringLiteral("Base"), QStringLiteral("Base")},
    {QStringLiteral("Cooling"), QStringLiteral("Cooling")},
    {QStringLiteral("Retraction"), QStringLiteral("Retraction")},
    {QStringLiteral("Other"), QStringLiteral("Other")},
    {QStringLiteral("Seam"), QStringLiteral("Quality")},
    {QStringLiteral("Walls"), QStringLiteral("Quality")},
  };
  return map.value(cat, QStringLiteral("Other"));
}

int ConfigOptionModel::rowCount(const QModelIndex &parent) const
{
  return parent.isValid() ? 0 : m_options.size();
}

QVariant ConfigOptionModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() >= m_options.size())
    return {};
  const auto &o = m_options[index.row()];
  switch (role)
  {
  case KeyRole:
    return o.key;
  case LabelRole:
    return o.label;
  case TypeRole:
    return o.type;
  case ValueRole:
    if (o.serialized)
    {
      QString text;
      const QVariantList values = o.value.toList();
      if (!values.isEmpty() && !values.first().toString().isEmpty())
        for (const QVariant &value : values)
          text += value.toString() + QLatin1Char(';');
      return text;
    }
    return editValue(index.row());
  case MinRole:
    return o.min;
  case MaxRole:
    return o.max;
  case StepRole:
    return o.step;
  case EnumLabelsRole:
    return o.enumLabels;
  case CategoryRole:
    return o.category;
  case GroupRole:
    return o.group;
  case PageRole:
    return o.page.isEmpty() ? pageForCategory(o.category) : o.page;
  case ReadonlyRole:
    return o.readonly;
  case DirtyRole:
    return m_dirtyKeys.contains(o.key);
  case TooltipRole:
    return o.tooltip;
  case ModeRole:
    return o.mode;
  case NullableRole:
    return o.nullable;
  case IsVectorRole:
    return o.isVector;
  case SidetextRole:
    return o.sidetext;
  case DisplayLabelRole:
    // zh_CN row label table (former OptionLabels.js ROW_LABELS); keys outside
    // the table fall back to the model label.
    return OptionLabelTable::rowLabels().value(o.key, o.label);
  case UnitRole:
    return optUnit(index.row());
  case ValueSourceRole:
    return m_valueSourceByKey.value(o.key, QStringLiteral("default"));
  case SourceRowRole:
    return index.row();
  case EnabledRole:
    return !m_disabledKeys.contains(o.key) && !dependencyConditionOff(o.key);
  case LineVisibleRole:
    return !m_hiddenKeys.contains(o.key);
  case MultilineRole:
    return o.multiline;
  case LineHeadRole:
    return m_lineHeadByKey.value(o.key);
  case LineLabelRole:
    return m_lineLabelByHead.value(o.key);
  case OverrideRole:
  {
    const auto it = m_overrideStates.constFind(o.key);
    if (it == m_overrideStates.cend())
      return 0;
    return it->value(QStringLiteral("checked")).toBool() ? 1 : 2;
  }
  case OverrideEnabledRole:
    return m_overrideStates.value(o.key).value(QStringLiteral("checkEnabled"), true);
  case InheritedValueRole:
    return m_overrideStates.value(o.key).value(QStringLiteral("inheritedValue"));
  case SubLabelRole:
    return lineSubLabels().value(o.key, OptionLabelTable::rowLabels().value(o.key, o.label));
  default:
    return {};
  }
}

bool ConfigOptionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid() || index.row() >= m_options.size())
    return false;
  auto &o = m_options[index.row()];
  switch (role)
  {
  case PageRole:
    o.page = value.toString();
    emit dataChanged(index, index, {PageRole});
    return true;
  case GroupRole:
    o.group = value.toString();
    emit dataChanged(index, index, {GroupRole});
    return true;
  case ValueRole:
    if (m_options[index.row()].serialized && value.typeId() == QMetaType::QString)
    {
      // Split on ";" (one trailing ";" dropped); an empty text is no scripts.
      QString text = value.toString();
      if (text.endsWith(QLatin1Char(';')))
        text.chop(1);
      QVariantList values;
      if (!text.isEmpty())
        for (const QString &part : text.split(QLatin1Char(';')))
          values.append(part);
      writeValue(index.row(), values);
      return true;
    }
    writeValue(index.row(), value);
    return true;
  default:
    return false;
  }
}

QHash<int, QByteArray> ConfigOptionModel::roleNames() const
{
  return {
      {KeyRole, "optKey"},
      {LabelRole, "optLabel"},
      {TypeRole, "optType"},
      {ValueRole, "optValue"},
      {MinRole, "optMin"},
      {MaxRole, "optMax"},
      {StepRole, "optStep"},
      {EnumLabelsRole, "optEnumLabels"},
      {CategoryRole, "optCategory"},
      {GroupRole, "optGroup"},
      {PageRole, "optPage"},
      {ReadonlyRole, "optReadonly"},
      {DirtyRole, "optDirty"},
      {TooltipRole, "optTooltip"},
      {ModeRole, "optMode"},
      {NullableRole, "optNullable"},
      {IsVectorRole, "optIsVector"},
      {SidetextRole, "optSidetext"},
      {DisplayLabelRole, "displayLabel"},
      {UnitRole, "optUnit"},
      {ValueSourceRole, "valueSource"},
      {SourceRowRole, "optIdx"},
      {EnabledRole, "optEnabled"},
      {LineVisibleRole, "optLineVisible"},
      {MultilineRole, "optMultiline"},
      {LineHeadRole, "optLineHead"},
      {LineLabelRole, "optLineLabel"},
      {SubLabelRole, "optSubLabel"},
      {OverrideRole, "optOverride"},
      {OverrideEnabledRole, "optOverrideEnabled"},
      {InheritedValueRole, "optInheritedValue"},
  };
}

// ── Individual accessors (safe for QML, no QVariantList) ─────────────────────
QString ConfigOptionModel::optKey(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].key : QString{}; }
QString ConfigOptionModel::optLabel(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].label : QString{}; }
QString ConfigOptionModel::optType(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].type : QString{}; }
QVariant ConfigOptionModel::optValue(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].value : QVariant{}; }
double ConfigOptionModel::optMin(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].min : 0.0; }
double ConfigOptionModel::optMax(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].max : 0.0; }
double ConfigOptionModel::optStep(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].step : 1.0; }
QString ConfigOptionModel::optCategory(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].category : QString{}; }
QString ConfigOptionModel::optGroup(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].group : QString{}; }

QString ConfigOptionModel::optPage(int i) const
{
  if (i >= 0 && i < m_options.size())
    return m_options[i].page.isEmpty() ? pageForCategory(m_options[i].category) : m_options[i].page;
  return QStringLiteral("Other");
}
bool ConfigOptionModel::optReadonly(int i) const { return (i >= 0 && i < m_options.size()) && m_options[i].readonly; }

int ConfigOptionModel::optEnumCount(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].enumLabels.size() : 0;
}
QString ConfigOptionModel::optEnumLabel(int i, int j) const
{
  if (i < 0 || i >= m_options.size())
    return {};
  const auto &e = m_options[i].enumLabels;
  return (j >= 0 && j < e.size()) ? e[j] : QString{};
}

QStringList ConfigOptionModel::optEnumLabelsList(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].enumLabels : QStringList{};
}

QString ConfigOptionModel::optUnit(int i) const
{
  if (i < 0 || i >= m_options.size())
    return {};
  // Prefer upstream sidetext (对齐上游 ConfigOptionDef::sidetext)
  if (!m_options[i].sidetext.isEmpty())
    return m_options[i].sidetext;
  // Fallback: key-based heuristic
  const QString &key = m_options[i].key;
  if (key.contains("speed") || key.contains("feed"))
    return QStringLiteral("mm/s");
  if (key.contains("temp") || key.contains("preheat"))
    return QStringLiteral("°C");
  if (key.contains("density") || key.contains("fan") || key.contains("flow") || key.contains("overlap") || key.contains("ironing"))
    return QStringLiteral("%");
  if (key.contains("angle"))
    return QStringLiteral("°");
  if (key.contains("time") || key.contains("delay"))
    return QStringLiteral("s");
  if (key.contains("distance"))
    return QStringLiteral("mm");
  // Countable items (loops, layers, count) have no unit
  if (key.contains("loops") || key.contains("count"))
    return {};
  return QStringLiteral("mm");
}

QString ConfigOptionModel::optTooltip(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].tooltip : QString{};
}

int ConfigOptionModel::optMode(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].mode : 2; // 2=Both default
}

bool ConfigOptionModel::optIsDirty(int i) const
{
  if (i < 0 || i >= m_options.size())
    return false;
  return m_dirtyKeys.contains(m_options[i].key);
}

bool ConfigOptionModel::optNullable(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].nullable : false;
}

bool ConfigOptionModel::optIsVector(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].isVector : false;
}

QString ConfigOptionModel::optSidetext(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].sidetext : QString{};
}

QStringList ConfigOptionModel::groupNames() const
{
  QSet<QString> groups;
  for (const auto &o : m_options)
    if (!o.group.isEmpty())
      groups.insert(o.group);
  QStringList result = groups.values();
  result.sort();
  return result;
}

int ConfigOptionModel::countForGroup(const QString &group) const
{
  int count = 0;
  for (const auto &o : m_options) {
    if (o.group == group)
      ++count;
  }
  return count;
}

int ConfigOptionModel::dirtyCountForGroup(const QString &group) const
{
  int count = 0;
  for (const auto &key : m_dirtyKeys) {
    int idx = findIndex(key);
    if (idx >= 0 && m_options[idx].group == group)
      ++count;
  }
  return count;
}

void ConfigOptionModel::resetOption(int row)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto &key = m_options[row].key;
  const QVariant targetValue = m_referenceValues.contains(key)
      ? m_referenceValues.value(key)
      : m_defaultValues.value(key);
  if (!targetValue.isValid())
    return;
  m_options[row].value = targetValue;
  m_dirtyKeys.remove(key);
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  emit optionValueChanged(key, targetValue);
  ++m_dataVersion;
  emit dataVersionChanged();
}

int ConfigOptionModel::dirtyCount() const
{
  return m_dirtyKeys.size();
}

void ConfigOptionModel::writeValue(int row, const QVariant &value)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto &key = m_options[row].key;
  if (isExtruderRow(row))
  {
    // Only the active extruder's entry changes; the others stay.
    QVariant element = value;
    if (value.typeId() == QMetaType::QVariantList || value.typeId() == QMetaType::QStringList)
    {
      const QVariantList edited = value.toList();
      element = edited.isEmpty() ? QVariant() : edited.first();
    }
    QVariantList list = m_options[row].value.toList();
    while (list.size() <= m_activeExtruder)
      list.append(list.isEmpty() ? element : list.last());
    list[m_activeExtruder] = element;
    m_options[row].value = list;
  }
  else
  {
    m_options[row].value = value;
  }
  const QVariant &storedValue = m_options[row].value;
  // 更新脏状态：与默认值比较
  const QVariant referenceValue = m_referenceValues.contains(key)
      ? m_referenceValues.value(key)
      : m_defaultValues.value(key);
  const bool nowDirty = (referenceValue != storedValue);
  if (nowDirty)
    m_dirtyKeys.insert(key);
  else
    m_dirtyKeys.remove(key);
  // 恒发双角色（ValueRole + DirtyRole）：较原先的按翻转分支是良性超集，
  // 保证经代理转发的 delegate 两种角色都会重估。
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  emit optionValueChanged(key, storedValue);
  ++m_dataVersion;
  emit dataVersionChanged();
}

void ConfigOptionModel::setValue(int row, const QVariant &value)
{
  // Thin wrapper over the setData(ValueRole) write path so QML invokers
  // (OptionRow) and C++ callers keep the same behavior.
  setData(index(row), value, ValueRole);
}

int ConfigOptionModel::indexOfKey(const QString &key) const
{
  return findIndex(key);
}

int ConfigOptionModel::countForCategory(const QString &category) const
{
  int c = 0;
  for (const auto &o : m_options)
    if (o.category == category)
      ++c;
  return c;
}

QVariantList ConfigOptionModel::indicesForCategory(const QString &category) const
{
  QVariantList result;
  for (int i = 0; i < m_options.size(); ++i)
    if (m_options[i].category == category)
      result.append(i);
  return result;
}

QStringList ConfigOptionModel::pageNames() const
{
  // Collect every page actually present on a loaded option. A page is the
  // explicit per-option page if assigned (e.g. via assignPageGroupForTier from
  // upstream Tab.cpp), otherwise the legacy category-derived fallback.
  QSet<QString> pages;
  pages.reserve(12);
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &pg = m_options[i].page.isEmpty() ? pageForCategory(m_options[i].category) : m_options[i].page;
    if (!pg.isEmpty())
      pages.insert(pg);
  }

  // Comprehensive cross-tier ordering (Print + Filament + Printer pages from
  // upstream Tab.cpp). Known pages emit in this stable order; any page not in
  // the list (future tiers / upstream additions) appends sorted so nothing is
  // silently dropped.
  const QStringList knownOrder = {
    // Print tier (TabPrint::build at the 8b93cc5df lock: Quality /
    // Strength / Speed / Support / Multimaterial / Others)
    QStringLiteral("Quality"), QStringLiteral("Strength"), QStringLiteral("Speed"),
    QStringLiteral("Support"), QStringLiteral("Multimaterial"), QStringLiteral("Others"),
    // Filament tier (TabFilament::build)
    QStringLiteral("Filament"), QStringLiteral("Advanced"), QStringLiteral("Multimaterial"),
    QStringLiteral("Dependencies"), QStringLiteral("Notes"),
    // Printer tier (TabPrinter::build_fff)
    QStringLiteral("Basic information"), QStringLiteral("Machine G-code"),
    QStringLiteral("Motion ability")
  };

  QStringList result;
  result.reserve(pages.size());
  for (const QString &p : knownOrder)
    if (pages.contains(p))
      result.append(p);

  // Append any remaining present pages not covered by knownOrder (sorted for
  // deterministic output) so no page is hidden by the ordering table.
  QStringList remainder;
  for (const QString &p : pages)
    if (!result.contains(p))
      remainder.append(p);
  remainder.sort();
  result.append(remainder);
  return result;
}

QList<int> ConfigOptionModel::filterIndicesByPage(const QList<int> &indices, const QString &page) const
{
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (idx < 0 || idx >= m_options.size())
      continue;
    const QString &pg = m_options[idx].page.isEmpty() ? pageForCategory(m_options[idx].category) : m_options[idx].page;
    if (pg == page)
      result.append(idx);
  }
  return result;
}

QList<int> ConfigOptionModel::filterIndicesByGroup(const QList<int> &indices, const QString &group) const
{
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (idx < 0 || idx >= m_options.size())
      continue;
    if (m_options[idx].group == group)
      result.append(idx);
  }
  return result;
}

// Fuzzy matching helper based on upstream OptionsSearcher / fts_fuzzy_match.
// Migrated verbatim from the ConfigViewModel.cpp anonymous namespace so the
// row-wise matchesFilter() below and ConfigOptionFilterProxy share the exact
// scoring rule the legacy VM filter used.
namespace {

// Subsequence matching with scoring.
static bool fuzzyMatch(const QString &pattern, const QString &text, int &outScore)
{
  const int m = pattern.size(), n = text.size();
  if (m == 0 || n == 0) { outScore = 0; return false; }

  // Quick reject: if all pattern chars must appear in text
  QVector<bool> charUsed(m, false);
  for (int pi = 0; pi < m; ++pi)
  {
    QChar pc = pattern[pi].toLower();
    bool found = false;
    for (int ti = 0; ti < n && !found; ++ti)
    {
      if (text[ti].toLower() == pc) found = true;
    }
    if (!found) { outScore = 0; return false; }
  }

  // Dynamic programming: dp[i][j] = best score matching pattern[0..i-1] against text[0..j-1]
  // dp[i][j].first = score, .second = position of last match in text
  QVector<QPair<int,int>> prevRow(n + 1, {0, -1}), curRow(n + 1, {0, -1});

  for (int pi = 1; pi <= m; ++pi)
  {
    curRow.fill({0, -1});
    const QChar pc = pattern[pi - 1].toLower();

    for (int ti = 1; ti <= n; ++ti)
    {
      // Match at current position
      if (text[ti - 1].toLower() == pc)
      {
        int prevScore = prevRow[ti - 1].first;
        int prevPos = prevRow[ti - 1].second;
        int bonus = 15; // sequential bonus (对齐上游 sequential bonus)
        if (ti > 1 && prevPos == ti - 2) bonus = 25; // consecutive chars
        int score = prevScore + bonus;
        if (score > curRow[ti].first)
        {
          curRow[ti] = {score, ti - 1};
        }
      }

      // Skip (gap penalty = -1)
      {
        int score = prevRow[ti].first - 1;
        if (score > curRow[ti].first)
        {
          curRow[ti] = {score, prevRow[ti].second};
        }
      }
    }
    // Copy (not move): a moved-from curRow would be empty, so the next
    // iteration's curRow.fill() would be a no-op and the curRow[ti] accesses
    // below would go out of bounds. Keep curRow sized n+1 every iteration.
    prevRow = curRow;
  }

  int bestScore = prevRow[n].first;
  int score = qMax(0, bestScore);
  outScore = score;
  return score >= 60; // minimum threshold (对齐上游 minimum score)
}

} // anonymous namespace

bool ConfigOptionModel::matchesFilter(int row, const QString &needle, bool advancedMode) const
{
  return matchesFilter(row, needle, advancedMode ? 2 : 0);
}

bool ConfigOptionModel::matchesFilter(int row, const QString &needle, int viewMode) const
{
  if (row < 0 || row >= m_options.size())
    return false;
  const ConfigOption &o = m_options[row];

  // Same semantics as the search arm of ConfigViewModel::filterOptionIndices:
  // needles >= 2 chars use the fuzzy score (>= 60), shorter needles substring;
  // fields are optLabel + optKey, both lowered.
  // A non-head line member is rendered inside its head row.
  const QString lineHead = m_lineHeadByKey.value(o.key);
  if (!lineHead.isEmpty() && lineHead != o.key)
    return false;
  const QStringList lineMembers = lineHead.isEmpty() ? QStringList{o.key} : m_lineMembersByHead.value(lineHead);

  const QString loweredNeedle = needle.toLower();
  if (!loweredNeedle.isEmpty())
  {
    const bool useFuzzy = loweredNeedle.length() >= 2;
    bool matched = false;
    // A line matches when any of its options does (upstream search lands on
    // the line that holds the option).
    for (const QString &memberKey : lineMembers)
    {
      const int memberRow = memberKey == o.key ? row : findIndex(memberKey);
      if (memberRow < 0)
        continue;
      const ConfigOption &m = m_options[memberRow];
      if (useFuzzy)
      {
        int score = 0;
        matched = fuzzyMatch(loweredNeedle, m.label.toLower(), score);
        if (!matched)
          matched = fuzzyMatch(loweredNeedle, m.key.toLower(), score);
      }
      else
      {
        matched = m.label.toLower().contains(loweredNeedle) ||
                  m.key.toLower().contains(loweredNeedle);
      }
      if (matched)
        break;
    }
    if (!matched)
      return false;
  }

  // Mode filter (upstream ConfigOptionMode: 0=comSimple, 1=comAdvanced,
  // 2=comExpert, 3=comDevelop). A line shows when its mode <= the view mode
  // (OptionsGroup::is_visible), so each level is a superset of the one below:
  // Simple mode (advancedMode=false) shows only comSimple options.
  if (o.mode > viewMode)
    return false;

  // toggle_line(false): the dependency rules hid the whole line. Upstream
  // get_line(key) returns the Line holding the option, so hiding any member
  // hides the line.
  for (const QString &memberKey : lineMembers)
    if (m_hiddenKeys.contains(memberKey))
      return false;

  return true;
}

bool ConfigOptionModel::matchesPage(int row, const QString &page) const
{
  if (row < 0 || row >= m_options.size())
    return false;
  if (page.isEmpty())
    return true;
  // Orca objects-mode pages (Tab.cpp TabPrintModel::build / TabPrintPlate):
  // the Frequent page is a fixed four-line whitelist and the Plate page the
  // plate_keys whitelist — both cut across the category-derived pages, so
  // they resolve by key set before the per-option page lookup.
  static const QSet<QString> kFrequentKeys = {
      QStringLiteral("layer_height"), QStringLiteral("sparse_infill_density"),
      QStringLiteral("wall_loops"), QStringLiteral("enable_support")};
  // Tab.cpp:94 plate_keys.
  static const QSet<QString> kPlateKeys = {
      QStringLiteral("curr_bed_type"), QStringLiteral("skirt_start_angle"),
      QStringLiteral("print_sequence"), QStringLiteral("spiral_mode"),
      QStringLiteral("first_layer_sequence_choice"),
      QStringLiteral("other_layers_sequence_choice")};
  const ConfigOption &o = m_options[row];
  if (page == QLatin1String("Frequent"))
    return kFrequentKeys.contains(o.key);
  if (page == QLatin1String("Plate"))
    return kPlateKeys.contains(o.key);
  // Same page resolution as filterIndicesByPage: explicit per-option page,
  // otherwise the legacy category-derived fallback.
  const QString resolvedPage = o.page.isEmpty() ? pageForCategory(o.category) : o.page;
  return resolvedPage == page;
}

bool ConfigOptionModel::matchesGroup(int row, const QString &group) const
{
  if (row < 0 || row >= m_options.size())
    return false;
  if (group.isEmpty())
    return true;
  return m_options[row].group == group;
}

void ConfigOptionModel::setValueSources(const QHash<QString, QString> &sources)
{
  // Mirror of ConfigViewModel::valueSources_. Diff per row and only emit for
  // rows whose source actually changed (preset switches rarely change the
  // per-key source, so this usually stays silent).
  const QHash<QString, QString> previous = m_valueSourceByKey;
  m_valueSourceByKey = sources;

  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    if (sources.value(key, QStringLiteral("default")) == previous.value(key, QStringLiteral("default")))
      continue;
    emit dataChanged(index(i), index(i), {ValueSourceRole});
  }
}

QHash<QString, QVariant> ConfigOptionModel::valuesByKey() const
{
  QHash<QString, QVariant> result;
  result.reserve(m_options.size());
  for (const auto &option : m_options)
    result.insert(option.key, option.value);
  return result;
}

QHash<QString, QVariant> ConfigOptionModel::defaultValuesByKey() const
{
  return m_defaultValues;
}

void ConfigOptionModel::resetToDefaults()
{
  if (m_defaultValues.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const auto it = m_defaultValues.constFind(m_options[i].key);
    if (it != m_defaultValues.cend() && m_options[i].value != it.value())
    {
      m_options[i].value = it.value();
      changed = true;
    }
  }

  if (changed)
  {
    m_dirtyKeys.clear();
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole, DirtyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
}

namespace
{
QStringList stringEntries(const QVariant &value)
{
  QStringList names;
  const QVariantList list = value.typeId() == QMetaType::QVariantList || value.typeId() == QMetaType::QStringList
                                ? value.toList()
                                : (value.toString().isEmpty() ? QVariantList{} : QVariantList{value});
  for (const QVariant &entry : list)
    if (!entry.toString().isEmpty())
      names.append(entry.toString());
  return names;
}

QString dependencyConditionKey(const QString &listKey)
{
  return listKey + QStringLiteral("_condition");
}
} // namespace

void ConfigOptionModel::applyValues(const QHash<QString, QVariant> &values)
{
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    const QVariant referenceValue = m_referenceValues.contains(key)
        ? m_referenceValues.value(key)
        : m_defaultValues.value(key);
    // The applied values are the whole state: a key they leave out is not
    // set, so it shows its reference / default value instead of keeping a
    // stale edit (a reverted or switched preset without that key).
    const auto it = values.constFind(key);
    const QVariant value = it != values.cend() ? it.value() : referenceValue;
    if (m_options[i].value == value)
      continue;
    m_options[i].value = value;
    // 更新脏状态
    const bool nowDirty = (referenceValue != value);
    if (nowDirty)
      m_dirtyKeys.insert(key);
    else
      m_dirtyKeys.remove(key);
    changed = true;
  }

  if (changed)
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole, DirtyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
  refreshExtruderCount();
  // A stored list means "All" is off; the toggled state only matters while
  // the list is empty (Tab::reload_compatible_prints_widget).
  for (auto it = m_dependencyAll.begin(); it != m_dependencyAll.end();)
  {
    const int row = findIndex(it.key());
    it = row >= 0 && !stringEntries(m_options[row].value).isEmpty() ? m_dependencyAll.erase(it) : std::next(it);
  }
  for (const char *conditionKey : {"compatible_printers_condition", "compatible_prints_condition"})
  {
    const int row = findIndex(QString::fromLatin1(conditionKey));
    if (row >= 0)
      emit dataChanged(index(row), index(row), {EnabledRole});
  }
}

void ConfigOptionModel::setReferenceValues(const QHash<QString, QVariant> &values)
{
  m_referenceValues = values;
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    const QVariant referenceValue = m_referenceValues.contains(key)
        ? m_referenceValues.value(key)
        : m_defaultValues.value(key);
    const bool wasDirty = m_dirtyKeys.contains(key);
    const bool nowDirty = (referenceValue != m_options[i].value);
    if (nowDirty)
      m_dirtyKeys.insert(key);
    else
      m_dirtyKeys.remove(key);
    changed = changed || (wasDirty != nowDirty);
  }

  if (changed)
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {DirtyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
}

void ConfigOptionModel::setReadonlyKeys(const QSet<QString> &keys)
{
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const bool readonly = m_baseReadonlyKeys.contains(m_options[i].key) || keys.contains(m_options[i].key);
    if (m_options[i].readonly == readonly)
      continue;
    m_options[i].readonly = readonly;
    changed = true;
  }

  if (changed)
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {ReadonlyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
}

int ConfigOptionModel::findIndex(const QString &key) const
{
  for (int i = 0; i < m_options.size(); ++i)
  {
    if (m_options[i].key == key)
      return i;
  }
  return -1;
}

namespace
{
  QString formatCoord(double v)
  {
    return QString::number(v, 'g', 10);
  }

  QString pointString(const QPointF &p)
  {
    return formatCoord(p.x()) + QLatin1Char('x') + formatCoord(p.y());
  }
} // namespace

QString ConfigOptionModel::groupIconName(const QString &group) const
{
  // Upstream new_optgroup(title, icon) pairs across TabPrint / TabFilament /
  // TabPrinter (Tab.cpp, pin 0a3724ed2f). Icons live in assets/icons; the
  // "note" / "fuzzy_skin" / "advanced" glyphs map onto their param_* copies.
  static const QHash<QString, QString> icons = {
      {QStringLiteral("Acceleration"), QStringLiteral("param_acceleration")},
      {QStringLiteral("Acceleration limitation"), QStringLiteral("param_acceleration")},
      {QStringLiteral("Accessory"), QStringLiteral("param_accessory")},
      {QStringLiteral("Adaptive bed mesh"), QStringLiteral("param_adaptive_mesh")},
      {QStringLiteral("Advanced"), QStringLiteral("param_advanced")},
      {QStringLiteral("Auxiliary part cooling fan"), QStringLiteral("param_cooling_aux_fan")},
      {QStringLiteral("Basic information"), QStringLiteral("param_information")},
      {QStringLiteral("Bed temperature"), QStringLiteral("param_bed_temp")},
      {QStringLiteral("Before layer change G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Bridging"), QStringLiteral("param_bridge")},
      {QStringLiteral("Brim"), QStringLiteral("param_adhension")},
      {QStringLiteral("Change extrusion role G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Change filament G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Clumping Detection G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Compatible printers"), QStringLiteral("param_dependencies_printers")},
      {QStringLiteral("Compatible process profiles"), QStringLiteral("param_dependencies_presets")},
      {QStringLiteral("Cooling Fan"), QStringLiteral("param_cooling_fan")},
      {QStringLiteral("Cooling for specific layer"), QStringLiteral("param_cooling_specific_layer")},
      {QStringLiteral("Exhaust fan"), QStringLiteral("param_cooling_exhaust")},
      {QStringLiteral("Extruder Clearance"), QStringLiteral("param_extruder_clearance")},
      {QStringLiteral("Filament end G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Filament for Features"), QStringLiteral("param_filament_for_features")},
      {QStringLiteral("Filament for Supports"), QStringLiteral("param_support_filament")},
      {QStringLiteral("Filament start G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("File header G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("First layer speed"), QStringLiteral("param_speed_first")},
      {QStringLiteral("Flow ratio and Pressure Advance"), QStringLiteral("param_flow_ratio_and_pressure_advance")},
      {QStringLiteral("Flush options"), QStringLiteral("param_flush")},
      {QStringLiteral("Fuzzy skin"), QStringLiteral("param_fuzzy_skin")},
      {QStringLiteral("G-code output"), QStringLiteral("param_gcode")},
      {QStringLiteral("Infill"), QStringLiteral("param_infill")},
      {QStringLiteral("Ironing"), QStringLiteral("param_ironing")},
      {QStringLiteral("Jerk limitation"), QStringLiteral("param_jerk")},
      {QStringLiteral("Jerk(XY)"), QStringLiteral("param_jerk")},
      {QStringLiteral("Junction Deviation"), QStringLiteral("param_junction_deviation")},
      {QStringLiteral("Layer change G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Layer height"), QStringLiteral("param_layer_height")},
      {QStringLiteral("Layer height limits"), QStringLiteral("param_layer_height")},
      {QStringLiteral("Line width"), QStringLiteral("param_line_width")},
      {QStringLiteral("Machine end G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Machine start G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Notes"), QStringLiteral("param_note")},
      {QStringLiteral("Ooze prevention"), QStringLiteral("param_ooze_prevention")},
      {QStringLiteral("Other layers speed"), QStringLiteral("param_speed")},
      {QStringLiteral("Overhang speed"), QStringLiteral("param_overhang_speed")},
      {QStringLiteral("Overhangs"), QStringLiteral("param_overhang")},
      {QStringLiteral("Part cooling fan"), QStringLiteral("param_cooling_part_fan")},
      {QStringLiteral("Pause G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Plugin Configuration"), QStringLiteral("param_gcode")},
      {QStringLiteral("Position"), QStringLiteral("param_position")},
      {QStringLiteral("Post-processing Scripts"), QStringLiteral("param_gcode")},
      {QStringLiteral("Precision"), QStringLiteral("param_precision")},
      {QStringLiteral("Prime tower"), QStringLiteral("param_tower")},
      {QStringLiteral("Print chamber temperature"), QStringLiteral("param_chamber_temp")},
      {QStringLiteral("Print temperature"), QStringLiteral("param_extruder_temp")},
      {QStringLiteral("Printable space"), QStringLiteral("param_printable_space")},
      {QStringLiteral("Printing by object G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Profile dependencies"), QStringLiteral("param_dependencies_presets")},
      {QStringLiteral("Raft"), QStringLiteral("param_raft")},
      {QStringLiteral("Resonance Compensation"), QStringLiteral("param_resonance_avoidance")},
      {QStringLiteral("Retraction"), QStringLiteral("param_retraction")},
      {QStringLiteral("Retraction when switching material"), QStringLiteral("param_retraction_material_change")},
      {QStringLiteral("Seam"), QStringLiteral("param_seam")},
      {QStringLiteral("Single extruder multi-material parameters"), QStringLiteral("param_settings")},
      {QStringLiteral("Single extruder multi-material setup"), QStringLiteral("param_multi_material")},
      {QStringLiteral("Skirt"), QStringLiteral("param_skirt")},
      {QStringLiteral("Slicing Pipeline Plugin"), QStringLiteral("param_gcode")},
      {QStringLiteral("Special mode"), QStringLiteral("param_special")},
      {QStringLiteral("Speed limitation"), QStringLiteral("param_speed")},
      {QStringLiteral("Support"), QStringLiteral("param_support")},
      {QStringLiteral("Support ironing"), QStringLiteral("param_ironing")},
      {QStringLiteral("Template Custom G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Timelapse G-code"), QStringLiteral("param_gcode")},
      {QStringLiteral("Tool change parameters with multi extruder MM printers"), QStringLiteral("param_toolchange_multi_extruder")},
      {QStringLiteral("Tool change parameters with single extruder MM printers"), QStringLiteral("param_toolchange")},
      {QStringLiteral("Top/bottom shells"), QStringLiteral("param_shell")},
      {QStringLiteral("Travel speed"), QStringLiteral("param_travel_speed")},
      {QStringLiteral("Tree supports"), QStringLiteral("param_support_tree")},
      {QStringLiteral("Volumetric speed limitation"), QStringLiteral("param_volumetric_speed")},
      {QStringLiteral("Wall generator"), QStringLiteral("param_wall_generator")},
      {QStringLiteral("Walls"), QStringLiteral("param_wall")},
      {QStringLiteral("Walls and surfaces"), QStringLiteral("param_wall_surface")},
      {QStringLiteral("Wipe tower"), QStringLiteral("param_tower")},
      {QStringLiteral("Wipe tower parameters"), QStringLiteral("param_tower")},
      {QStringLiteral("Z contouring"), QStringLiteral("param_z_contouring")},
      {QStringLiteral("Z-Hop"), QStringLiteral("param_extruder_lift_enforcement")},
  };
  return icons.value(group);
}

int ConfigOptionModel::pluginOverrideCount(const QVariant &raw) const
{
  return PluginOverrides::count(raw.toString());
}

QVariantList ConfigOptionModel::pluginOverrideEntries(const QVariant &raw) const
{
  PluginOverrides::Document document;
  QString error;
  QVariantList out;
  if (!PluginOverrides::parse(raw.toString(), document, error))
    return out;
  for (const QJsonObject &entry : document.entries)
  {
    QVariantMap row;
    row.insert(QStringLiteral("pluginKey"), PluginOverrides::pluginKey(entry));
    row.insert(QStringLiteral("capability"), PluginOverrides::capability(entry));
    row.insert(QStringLiteral("capabilityType"), PluginOverrides::capabilityType(entry));
    row.insert(QStringLiteral("configText"),
               QString::fromUtf8(QJsonDocument(PluginOverrides::capConfig(entry)).toJson(QJsonDocument::Indented)));
    out.append(row);
  }
  return out;
}

QString ConfigOptionModel::pluginOverridesError(const QVariant &raw) const
{
  PluginOverrides::Document document;
  QString error;
  PluginOverrides::parse(raw.toString(), document, error);
  return error;
}

QVariantMap ConfigOptionModel::pluginOverrideSetConfig(const QString &raw, int index, const QString &configText) const
{
  QVariantMap result;
  PluginOverrides::Document document;
  QString error;
  bool ok = PluginOverrides::parse(raw, document, error) &&
            PluginOverrides::setCapConfig(document, index, configText, error);
  result.insert(QStringLiteral("ok"), ok);
  result.insert(QStringLiteral("text"), ok ? PluginOverrides::serialize(document) : raw);
  result.insert(QStringLiteral("error"), error);
  return result;
}

QString ConfigOptionModel::pluginOverrideRemove(const QString &raw, int index) const
{
  PluginOverrides::Document document;
  QString error;
  if (!PluginOverrides::parse(raw, document, error) || !PluginOverrides::remove(document, index))
    return raw;
  return PluginOverrides::serialize(document);
}

bool ConfigOptionModel::pluginOverridesEqual(const QString &a, const QString &b) const
{
  PluginOverrides::Document da, db;
  QString ea, eb;
  if (!PluginOverrides::parse(a, da, ea) || !PluginOverrides::parse(b, db, eb))
    return a == b;
  return PluginOverrides::serialize(da) == PluginOverrides::serialize(db);
}

QString ConfigOptionModel::pluginConfigTitle() const
{
  return m_hierarchyTier == 1 ? QStringLiteral("材料插件")
       : m_hierarchyTier == 2 ? QStringLiteral("打印机插件")
                              : QStringLiteral("工艺插件");
}

QString ConfigOptionModel::colourOf(const QVariant &value) const
{
  QVariant v = value;
  if (v.typeId() == QMetaType::QVariantList || v.typeId() == QMetaType::QStringList)
  {
    const QVariantList list = v.toList();
    v = list.isEmpty() ? QVariant() : list.first();
  }
  const QString text = v.toString().trimmed();
  return QColor::isValidColorName(text) ? text : QString();
}

void ConfigOptionModel::setColour(int row, const QString &colour)
{
  if (row < 0 || row >= m_options.size())
    return;
  const QVariant current = editValue(row);
  if (current.typeId() == QMetaType::QVariantList || current.typeId() == QMetaType::QStringList)
  {
    QVariantList list = current.toList();
    if (list.isEmpty())
      list.append(colour);
    else
      list[0] = colour;
    writeValue(row, list);
    return;
  }
  writeValue(row, colour);
}

bool ConfigOptionModel::dependencyAll(int row) const
{
  if (row < 0 || row >= m_options.size())
    return true;
  const ConfigOption &o = m_options[row];
  const auto it = m_dependencyAll.constFind(o.key);
  return it != m_dependencyAll.cend() ? it.value() : stringEntries(o.value).isEmpty();
}

void ConfigOptionModel::setDependencyAll(int row, bool all)
{
  if (row < 0 || row >= m_options.size())
    return;
  const QString key = m_options[row].key;
  m_dependencyAll.insert(key, all);
  const QString conditionKey = dependencyConditionKey(key);
  const int conditionRow = findIndex(conditionKey);
  if (conditionRow >= 0)
    emit dataChanged(index(conditionRow), index(conditionRow), {EnabledRole});
  if (all && !stringEntries(m_options[row].value).isEmpty())
    writeValue(row, QVariantList{});
  else
    emit dataChanged(index(row), index(row), {ValueRole});
  ++m_dataVersion;
  emit dataVersionChanged();
}

bool ConfigOptionModel::dependencyConditionOff(const QString &conditionKey) const
{
  if (!conditionKey.endsWith(QStringLiteral("_condition")))
    return false;
  const int listRow = findIndex(conditionKey.chopped(int(QStringLiteral("_condition").size())));
  return listRow >= 0 && m_options[listRow].type == QStringLiteral("compatible_list") && !dependencyAll(listRow);
}

QStringList ConfigOptionModel::dependencyCandidates(int row) const
{
  if (row < 0 || row >= m_options.size())
    return {};
  return m_dependencyCandidates.value(m_options[row].key);
}

QStringList ConfigOptionModel::dependencySelection(int row) const
{
  if (row < 0 || row >= m_options.size())
    return {};
  return stringEntries(m_options[row].value);
}

void ConfigOptionModel::setDependencySelection(int row, const QStringList &names)
{
  if (row < 0 || row >= m_options.size())
    return;
  const QStringList candidates = dependencyCandidates(row);
  QVariantList value;
  // Leave the list empty when every entry is checked (= "All").
  if (names.size() != candidates.size())
    for (const QString &candidate : candidates)
      if (names.contains(candidate))
        value.append(candidate);
  if (value.isEmpty())
    setDependencyAll(row, true);
  if (stringEntries(m_options[row].value) != stringEntries(value))
    writeValue(row, value);
}

QString ConfigOptionModel::dependencyDialogTitle(int row) const
{
  const bool printers = row >= 0 && row < m_options.size() && m_options[row].key == QStringLiteral("compatible_printers");
  return printers ? QObject::tr("兼容打印机") : QObject::tr("兼容的切片配置");
}

QString ConfigOptionModel::dependencyDialogLabel(int row) const
{
  const bool printers = row >= 0 && row < m_options.size() && m_options[row].key == QStringLiteral("compatible_printers");
  return printers ? QObject::tr("选择打印机") : QObject::tr("选择配置文件");
}

void ConfigOptionModel::setDependencyCandidates(const QString &key, const QStringList &names)
{
  m_dependencyCandidates.insert(key, names);
}

QString ConfigOptionModel::openEnumText(int row, const QVariant &value) const
{
  if (row < 0 || row >= m_options.size())
    return {};
  QVariant v = value;
  if (v.typeId() == QMetaType::QVariantList || v.typeId() == QMetaType::QStringList)
  {
    const QVariantList list = v.toList();
    v = list.isEmpty() ? QVariant() : list.first();
  }
  if (!v.isValid())
    return {};
  const ConfigOption &o = m_options[row];
  QString text = v.toString();
  if (v.typeId() == QMetaType::Double)
    text = QString::number(v.toDouble(), 'g', 12);
  const int index = o.enumValues.indexOf(text);
  return index >= 0 && index < o.enumLabels.size() ? o.enumLabels.at(index) : text;
}

bool ConfigOptionModel::setOpenEnumText(int row, const QString &rawText)
{
  if (row < 0 || row >= m_options.size())
    return false;
  const ConfigOption &o = m_options[row];
  const QString text = rawText.trimmed();
  QString valueText = text;
  const int labelIndex = o.enumLabels.indexOf(text);
  if (labelIndex >= 0 && labelIndex < o.enumValues.size())
    valueText = o.enumValues.at(labelIndex);

  QVariant parsed;
  bool ok = true;
  if (o.openBase == QStringLiteral("int"))
    parsed = valueText.toInt(&ok);
  else if (o.openBase == QStringLiteral("float"))
    parsed = valueText.toDouble(&ok);
  else if (o.openBase == QStringLiteral("floatOrPercent"))
  {
    // ConfigOptionFloatOrPercent::deserialize: "N" or "N%".
    const bool percent = valueText.endsWith(QLatin1Char('%'));
    valueText.chop(percent ? 1 : 0);
    const double number = valueText.trimmed().toDouble(&ok);
    parsed = percent ? QVariant(QString::number(number, 'g', 12) + QLatin1Char('%')) : QVariant(number);
  }
  else
    parsed = valueText;
  if (!ok || (o.openBase == QStringLiteral("string") && valueText.isEmpty()))
    return false;

  const QVariant current = editValue(row);
  if (o.isVector || current.typeId() == QMetaType::QVariantList || current.typeId() == QMetaType::QStringList)
  {
    QVariantList list = current.toList();
    if (list.isEmpty())
      list.append(parsed);
    else
      list[0] = parsed;
    writeValue(row, list);
    return true;
  }
  writeValue(row, parsed);
  return true;
}

int ConfigOptionModel::enumIndexOf(int row, const QVariant &value) const
{
  if (row < 0 || row >= m_options.size())
    return 0;
  const ConfigOption &o = m_options[row];
  QVariant v = value;
  if (v.typeId() == QMetaType::QVariantList || v.typeId() == QMetaType::QStringList)
  {
    const QVariantList list = v.toList();
    v = list.isEmpty() ? QVariant() : list.first();
  }
  if (v.typeId() == QMetaType::QString)
  {
    const QString text = v.toString();
    int index = o.enumValues.indexOf(text);
    if (index < 0)
      index = o.enumLabels.indexOf(text);
    if (index < 0)
      index = o.enumLabels.indexOf(zhEnumLabel(text)); // English def label (extractDefault)
    if (index >= 0)
      return index;
    bool isNumber = false;
    const int numeric = text.toInt(&isNumber);
    if (isNumber && numeric >= 0 && numeric < o.enumLabels.size())
      return numeric;
    return 0;
  }
  bool ok = false;
  const int numeric = v.toInt(&ok);
  return ok && numeric >= 0 && numeric < o.enumLabels.size() ? numeric : 0;
}

void ConfigOptionModel::setEnumIndex(int row, int index)
{
  if (row < 0 || row >= m_options.size())
    return;
  const ConfigOption &o = m_options[row];
  if (index >= 0 && index < o.enumValues.size())
    writeValue(row, o.enumValues.at(index));
  else
    writeValue(row, index);
}

QList<QPointF> ConfigOptionModel::parsePoints(const QVariant &value)
{
  // Shared preset-point normalization (owzx::flatPointCoords).
  const QList<float> coords = owzx::flatPointCoords(value);
  QList<QPointF> points;
  for (qsizetype i = 0; i + 1 < coords.size(); i += 2)
    points.append(QPointF(coords[i], coords[i + 1]));
  return points;
}

QString ConfigOptionModel::pointsText(const QVariant &value) const
{
  QStringList out;
  for (const QPointF &p : parsePoints(value))
    out.append(QString::number(static_cast<int>(p.x())) + QLatin1Char('x') +
               QString::number(static_cast<int>(p.y())));
  return out.join(QStringLiteral(", "));
}

bool ConfigOptionModel::setPointsText(int row, const QString &text)
{
  if (row < 0 || row >= m_options.size())
    return false;
  QVariantList list;
  for (const QString &raw : text.split(QLatin1Char(','), Qt::SkipEmptyParts))
  {
    const QList<QPointF> one = parsePoints(raw.trimmed());
    if (one.size() != 1 || !raw.contains(QLatin1Char('x'), Qt::CaseInsensitive))
      return false;
    list.append(pointString(one.first()));
  }
  if (list.isEmpty())
    return false;
  writeValue(row, list);
  return true;
}

double ConfigOptionModel::pointCoord(const QVariant &value, int axis) const
{
  const QList<QPointF> points = parsePoints(value);
  if (points.isEmpty())
    return 0.0;
  return axis == 0 ? points.first().x() : points.first().y();
}

void ConfigOptionModel::setPointCoord(int row, int axis, double coord)
{
  if (row < 0 || row >= m_options.size())
    return;
  const QVariant current = editValue(row);
  QList<QPointF> points = parsePoints(current);
  if (points.isEmpty())
    points.append(QPointF());
  if (axis == 0)
    points.first().setX(coord);
  else
    points.first().setY(coord);
  if (current.typeId() == QMetaType::QVariantList)
  {
    QVariantList list;
    for (const QPointF &p : points)
      list.append(pointString(p));
    writeValue(row, list);
  }
  else
  {
    writeValue(row, pointString(points.first()));
  }
}

namespace
{
#ifdef HAS_LIBSLIC3R
  // Map upstream ConfigOptionType to Qt6 type string
  QString mapType(Slic3r::ConfigOptionType t)
  {
    switch (t)
    {
    case Slic3r::coPercent:
    case Slic3r::coPercents:
      return QStringLiteral("percent");
    case Slic3r::coFloat:
    case Slic3r::coFloatOrPercent:
    case Slic3r::coFloats:
      return QStringLiteral("double");
    case Slic3r::coInt:
    case Slic3r::coInts:
      return QStringLiteral("int");
    case Slic3r::coBool:
    case Slic3r::coBools:
      return QStringLiteral("bool");
    case Slic3r::coEnum:
    case Slic3r::coEnums:
      return QStringLiteral("enum");
    case Slic3r::coString:
    case Slic3r::coStrings:
      return QStringLiteral("string");
    case Slic3r::coPoint:
      return QStringLiteral("point");
    case Slic3r::coPoints:
      return QStringLiteral("points");
    default:
      return QStringLiteral("double");
    }
  }

  // Map upstream category to Qt6 display category
  QString mapCategory(const std::string &upstreamCategory)
  {
    const QString cat = QString::fromUtf8(upstreamCategory.c_str());
    if (cat.contains("Layers", Qt::CaseInsensitive) || cat.contains("Perimeters", Qt::CaseInsensitive))
      return QObject::tr("质量");
    if (cat.contains("Infill", Qt::CaseInsensitive))
      return QObject::tr("填充");
    if (cat.contains("Speed", Qt::CaseInsensitive))
      return QObject::tr("速度");
    if (cat.contains("Acceleration", Qt::CaseInsensitive) || cat.contains("Jerk", Qt::CaseInsensitive))
      return QObject::tr("加速度");
    if (cat.contains("Temperature", Qt::CaseInsensitive))
      return QObject::tr("温度");
    if (cat.contains("Support", Qt::CaseInsensitive))
      return QObject::tr("支撑");
    if (cat.contains("Skirt", Qt::CaseInsensitive) || cat.contains("Brim", Qt::CaseInsensitive) || cat.contains("Raft", Qt::CaseInsensitive))
      return QObject::tr("底座");
    if (cat.contains("Cooling", Qt::CaseInsensitive) || cat.contains("Fan", Qt::CaseInsensitive))
      return QObject::tr("冷却");
    if (cat.contains("Retract", Qt::CaseInsensitive))
      return QObject::tr("回退");
    if (cat.contains("Extruder", Qt::CaseInsensitive) || cat.contains("Extrusion", Qt::CaseInsensitive))
      return QObject::tr("挤出机");
    if (cat.contains("Adhesion", Qt::CaseInsensitive))
      return QObject::tr("底座");
    if (cat.contains("Output", Qt::CaseInsensitive) || cat.contains("G-code", Qt::CaseInsensitive))
      return QObject::tr("输出");
    if (cat.contains("Ironing", Qt::CaseInsensitive))
      return QObject::tr("质量");
    if (cat.contains("Seam", Qt::CaseInsensitive))
      return QObject::tr("质量");
    if (cat.contains("Prime", Qt::CaseInsensitive) || cat.contains("Wipe", Qt::CaseInsensitive))
      return QObject::tr("底座");
    // Machine-specific categories (对齐上游 TabPrinter)
    if (cat.contains("Printable", Qt::CaseInsensitive) || cat.contains("Bed", Qt::CaseInsensitive))
      return QObject::tr("打印空间");
    if (cat.contains("Motion", Qt::CaseInsensitive))
      return QObject::tr("运动能力");
    if (cat.contains("Multi", Qt::CaseInsensitive))
      return QObject::tr("多材料");
    // Filament-specific categories (对齐上游 TabFilament)
    if (cat.contains("Filament", Qt::CaseInsensitive))
      return QObject::tr("耗材");
    return QObject::tr("其他");
  }

  // Extract default value from ConfigOptionDef
  QVariant extractDefault(const Slic3r::ConfigOptionDef *def)
  {
    if (!def || !def->default_value)
      return {};
    switch (def->type)
    {
    case Slic3r::coFloat:
      return def->get_default_value<Slic3r::ConfigOptionFloat>()->value;
    case Slic3r::coInt:
      return def->get_default_value<Slic3r::ConfigOptionInt>()->value;
    case Slic3r::coBool:
      return def->get_default_value<Slic3r::ConfigOptionBool>()->value;
    case Slic3r::coString:
      return QString::fromStdString(def->get_default_value<Slic3r::ConfigOptionString>()->value);
    case Slic3r::coEnum:
    case Slic3r::coEnums:
    {
      const int intVal = def->get_default_value<Slic3r::ConfigOptionEnumGeneric>()->value;
      // Try to find the enum label for the default value
      const auto &enumValues = def->enum_values;
      const auto &enumLabels = def->enum_labels;
      if (intVal >= 0 && intVal < static_cast<int>(enumValues.size()) && intVal < static_cast<int>(enumLabels.size()))
        return QString::fromUtf8(enumLabels[intVal].c_str());
      return intVal;
    }
    case Slic3r::coFloatOrPercent:
      return def->get_default_value<Slic3r::ConfigOptionFloatOrPercent>()->value;
    case Slic3r::coPercent:
      return def->get_default_value<Slic3r::ConfigOptionPercent>()->value;
    // Vector types (per-extruder): bb3 upstream stores speed/accel/jerk/temp as
    // coFloats/coInts/coBools/coFloatsOrPercents/coPercents. For the Qt6 single-value
    // editor we surface the first element (extruder 0) as the default value — this
    // is what single-extruder printers need, and matches the pre-bb3 OWzx behavior
    // when these fields were rolled back to single-value types. Multi-extruder
    // per-extruder editing is a follow-up UI surface (see v5.4 Phase 183 PLAN.md).
    case Slic3r::coFloats:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionFloats>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coInts:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionInts>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coBools:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionBools>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coFloatsOrPercents:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionFloatsOrPercents>();
      return v->values.empty() ? QVariant() : QVariant::fromValue(v->values[0]);
    }
    case Slic3r::coPercents:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionPercents>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coStrings:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionStrings>();
      return v->values.empty() ? QVariant() : QString::fromStdString(v->values[0]);
    }
    case Slic3r::coPoints:
    {
      // Bed polygons (printable_area / bed_exclude_area): serialize as
      // "x1,y1,x2,y2,..." — the format EditorViewModel's bed-sync and
      // ProjectService::arrangeObjects printableArea parsing expect.
      const auto *v = def->get_default_value<Slic3r::ConfigOptionPoints>();
      QString serialized;
      for (const auto &pt : v->values)
      {
        if (!serialized.isEmpty())
          serialized += QLatin1Char(',');
        serialized += QString::number(pt.x(), 'f', 1) + QLatin1Char(',') +
                      QString::number(pt.y(), 'f', 1);
      }
      return serialized;
    }
    case Slic3r::coPoint:
    {
      // Same "XxY" text a preset JSON stores for a scalar point
      // (e.g. best_object_pos "0.5x0.5").
      const auto *v = def->get_default_value<Slic3r::ConfigOptionPoint>();
      return QString::number(v->value.x()) + QLatin1Char('x') + QString::number(v->value.y());
    }
    default:
      return {};
    }
  }


#endif // HAS_LIBSLIC3R

  struct ProcessGroupDefinition
  {
    QString name;
    QStringList optionKeys;
  };

  struct ProcessPageDefinition
  {
    QString name;
    QList<ProcessGroupDefinition> groups;
  };

  // Upstream multi-option lines (Tab.cpp, pin 0a3724ed2f): `Line line =
  // {label, tooltip}; line.append_option(...)`. label is the line's zh text
  // (OrcaSlicer_zh_CN.po); keys in append_option order, the first is the
  // head. The Motion-ability Normal/Silent columns are not lines here
  // (single machine-limit column).
  struct OptionLineDefinition
  {
    QString label;
    QStringList keys;
  };

  static const QList<OptionLineDefinition> &lineDefinitions(int tier)
  {
    // TabPrint::build (Tab.cpp:2866-2877)
    static const QList<OptionLineDefinition> print = {
        {QStringLiteral("悬垂速度"), {QStringLiteral("overhang_1_4_speed"), QStringLiteral("overhang_2_4_speed"),
                                    QStringLiteral("overhang_3_4_speed"), QStringLiteral("overhang_4_4_speed")}},
        {QStringLiteral("桥接"), {QStringLiteral("bridge_speed"), QStringLiteral("internal_bridge_speed")}},
    };
    // TabFilament::build (Tab.cpp:4309-4523)
    static const QList<OptionLineDefinition> filament = {
        {QStringLiteral("建议喷嘴温度"), {QStringLiteral("nozzle_temperature_range_low"), QStringLiteral("nozzle_temperature_range_high")}},
        {QStringLiteral("机箱温度"), {QStringLiteral("chamber_temperature"), QStringLiteral("chamber_minimal_temperature")}},
        {QStringLiteral("喷嘴"), {QStringLiteral("nozzle_temperature_initial_layer"), QStringLiteral("nozzle_temperature")}},
        {QStringLiteral("低温打印板（超强粘附）"), {QStringLiteral("supertack_plate_temp_initial_layer"), QStringLiteral("supertack_plate_temp")}},
        {QStringLiteral("低温打印热床"), {QStringLiteral("cool_plate_temp_initial_layer"), QStringLiteral("cool_plate_temp")}},
        {QStringLiteral("纹理的低温打印床"), {QStringLiteral("textured_cool_plate_temp_initial_layer"), QStringLiteral("textured_cool_plate_temp")}},
        {QStringLiteral("工程材料热床"), {QStringLiteral("eng_plate_temp_initial_layer"), QStringLiteral("eng_plate_temp")}},
        {QStringLiteral("光滑PEI板/高温板"), {QStringLiteral("hot_plate_temp_initial_layer"), QStringLiteral("hot_plate_temp")}},
        {QStringLiteral("纹理PEI热床"), {QStringLiteral("textured_plate_temp_initial_layer"), QStringLiteral("textured_plate_temp")}},
        {QStringLiteral("最小风扇速度阈值"), {QStringLiteral("fan_min_speed"), QStringLiteral("fan_cooling_layer_time")}},
        {QStringLiteral("最大风扇速度阈值"), {QStringLiteral("fan_max_speed"), QStringLiteral("slow_down_layer_time")}},
        {QStringLiteral("打印中"), {QStringLiteral("activate_air_filtration_during_print"), QStringLiteral("during_print_exhaust_fan_speed")}},
        {QStringLiteral("完成打印"), {QStringLiteral("activate_air_filtration_on_completion"), QStringLiteral("complete_print_exhaust_fan_speed")}},
    };
    // TabPrinter::build_fff / build_kinematics_page (Tab.cpp:5113, 5427-5446)
    static const QList<OptionLineDefinition> printer = {
        {QStringLiteral("风扇响应时间"), {QStringLiteral("fan_speedup_time"), QStringLiteral("fan_speedup_overhangs")}},
        {QStringLiteral("共振规避速度"), {QStringLiteral("min_resonance_avoidance_speed"), QStringLiteral("max_resonance_avoidance_speed")}},
        {QStringLiteral("频率"), {QStringLiteral("input_shaping_freq_x"), QStringLiteral("input_shaping_freq_y")}},
        {QStringLiteral("阻尼"), {QStringLiteral("input_shaping_damp_x"), QStringLiteral("input_shaping_damp_y")}},
    };
    return tier == 1 ? filament : tier == 2 ? printer : print;
  }

  // Per-option labels inside a line: ConfigOptionDef::label through the zh
  // po (chamber Target/Minimal are the label overrides Tab.cpp:4357-4362
  // sets; the air-filtration checkboxes carry no label).
  static const QHash<QString, QString> &lineSubLabels()
  {
    static const QHash<QString, QString> labels = {
        {QStringLiteral("overhang_1_4_speed"), QStringLiteral("10%")},
        {QStringLiteral("overhang_2_4_speed"), QStringLiteral("25%")},
        {QStringLiteral("overhang_3_4_speed"), QStringLiteral("50%")},
        {QStringLiteral("overhang_4_4_speed"), QStringLiteral("75%")},
        {QStringLiteral("bridge_speed"), QStringLiteral("外部")},
        {QStringLiteral("internal_bridge_speed"), QStringLiteral("内部")},
        {QStringLiteral("nozzle_temperature_range_low"), QStringLiteral("最小")},
        {QStringLiteral("nozzle_temperature_range_high"), QStringLiteral("最大")},
        {QStringLiteral("chamber_temperature"), QStringLiteral("目标")},
        {QStringLiteral("chamber_minimal_temperature"), QStringLiteral("最低")},
        {QStringLiteral("nozzle_temperature_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("nozzle_temperature"), QStringLiteral("其它层")},
        {QStringLiteral("supertack_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("supertack_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("cool_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("cool_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("textured_cool_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("textured_cool_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("eng_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("eng_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("hot_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("hot_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("textured_plate_temp_initial_layer"), QStringLiteral("首层")},
        {QStringLiteral("textured_plate_temp"), QStringLiteral("其它层")},
        {QStringLiteral("fan_min_speed"), QStringLiteral("风扇速度")},
        {QStringLiteral("fan_cooling_layer_time"), QStringLiteral("层时间")},
        {QStringLiteral("fan_max_speed"), QStringLiteral("风扇速度")},
        {QStringLiteral("slow_down_layer_time"), QStringLiteral("层时间")},
        {QStringLiteral("activate_air_filtration_during_print"), QString()},
        {QStringLiteral("during_print_exhaust_fan_speed"), QStringLiteral("风扇速度")},
        {QStringLiteral("activate_air_filtration_on_completion"), QString()},
        {QStringLiteral("complete_print_exhaust_fan_speed"), QStringLiteral("风扇速度")},
        {QStringLiteral("fan_speedup_time"), QString()},
        {QStringLiteral("fan_speedup_overhangs"), QStringLiteral("仅悬垂")},
        {QStringLiteral("min_resonance_avoidance_speed"), QStringLiteral("最小")},
        {QStringLiteral("max_resonance_avoidance_speed"), QStringLiteral("最大")},
        {QStringLiteral("input_shaping_freq_x"), QStringLiteral("X")},
        {QStringLiteral("input_shaping_freq_y"), QStringLiteral("Y")},
        {QStringLiteral("input_shaping_damp_x"), QStringLiteral("X")},
        {QStringLiteral("input_shaping_damp_y"), QStringLiteral("Y")},
    };
    return labels;
  }

  static QStringList processKeys(std::initializer_list<const char *> keys)
  {
    QStringList result;
    result.reserve(static_cast<qsizetype>(keys.size()));
    for (const char *key : keys)
      result.append(QString::fromLatin1(key));
    return result;
  }

  // Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2647-3147
  // (TabPrint::build at the 8b93cc5df source-truth lock).
  static const QList<ProcessPageDefinition> &processPageDefinitions()
  {
    static const QList<ProcessPageDefinition> definitions = {
        {QStringLiteral("Quality"), {
            {QStringLiteral("Layer height"), processKeys({"layer_height", "initial_layer_print_height", "enable_mixed_color_sublayer"})},
            {QStringLiteral("Line width"), processKeys({"line_width", "initial_layer_line_width", "outer_wall_line_width", "inner_wall_line_width", "top_surface_line_width", "sparse_infill_line_width", "internal_solid_infill_line_width", "support_line_width", "bridge_line_width"})},
            {QStringLiteral("Seam"), processKeys({"seam_position", "staggered_inner_seams", "seam_gap", "seam_slope_type", "seam_slope_conditional", "scarf_angle_threshold", "scarf_overhang_threshold", "scarf_joint_speed", "seam_slope_start_height", "seam_slope_entire_loop", "seam_slope_min_length", "seam_slope_steps", "scarf_joint_flow_ratio", "seam_slope_inner_walls", "role_based_wipe_speed", "wipe_speed", "wipe_on_loops", "wipe_before_external_loop"})},
            {QStringLiteral("Precision"), processKeys({"slice_closing_radius", "resolution", "enable_arc_fitting", "xy_hole_compensation", "xy_contour_compensation", "elefant_foot_compensation", "elefant_foot_layers_density", "elefant_foot_compensation_layers", "precise_outer_wall", "precise_z_height", "hole_to_polyhole", "hole_to_polyhole_threshold", "hole_to_polyhole_twisted", "hole_to_polyhole_max_edges"})},
            {QStringLiteral("Ironing"), processKeys({"ironing_type", "ironing_pattern", "ironing_flow", "ironing_spacing", "ironing_inset", "ironing_angle", "ironing_angle_fixed"})},
            {QStringLiteral("Z contouring"), processKeys({"zaa_enabled", "zaa_minimize_perimeter_height", "zaa_min_z", "zaa_dont_alternate_fill_direction"})},
            {QStringLiteral("Wall generator"), processKeys({"wall_generator", "wall_transition_angle", "wall_transition_filter_deviation", "wall_transition_length", "wall_distribution_count", "initial_layer_min_bead_width", "min_bead_width", "min_feature_size", "min_length_factor", "wall_maximum_resolution", "wall_maximum_deviation"})},
            {QStringLiteral("Walls and surfaces"), processKeys({"wall_sequence", "is_infill_first", "wall_direction", "print_flow_ratio", "top_solid_infill_flow_ratio", "bottom_solid_infill_flow_ratio", "set_other_flow_ratios", "first_layer_flow_ratio", "outer_wall_flow_ratio", "inner_wall_flow_ratio", "overhang_flow_ratio", "sparse_infill_flow_ratio", "internal_solid_infill_flow_ratio", "gap_fill_flow_ratio", "support_flow_ratio", "support_interface_flow_ratio", "only_one_wall_first_layer", "only_one_wall_top", "min_width_top_surface", "reduce_crossing_wall", "max_travel_detour_distance", "small_area_infill_flow_compensation", "small_area_infill_flow_compensation_model"})},
            {QStringLiteral("Bridging"), processKeys({"bridge_flow", "internal_bridge_flow", "bridge_density", "internal_bridge_density", "thick_bridges", "thick_internal_bridges", "enable_extra_bridge_layer", "dont_filter_internal_bridges", "counterbore_hole_bridging"})},
            {QStringLiteral("Overhangs"), processKeys({"detect_overhang_wall", "make_overhang_printable", "make_overhang_printable_angle", "make_overhang_printable_hole_size", "extra_perimeters_on_overhangs", "overhang_reverse", "overhang_reverse_internal_only", "overhang_reverse_threshold"})},
        }},
        {QStringLiteral("Strength"), {
            {QStringLiteral("Walls"), processKeys({"wall_loops", "alternate_extra_wall", "unsupported_wall_last", "detect_thin_wall"})},
            {QStringLiteral("Top/bottom shells"), processKeys({"top_shell_layers", "top_shell_thickness", "top_surface_density", "top_surface_pattern", "top_surface_fill_order", "top_layer_direction", "top_surface_expansion", "top_surface_expansion_margin", "top_surface_expansion_direction", "bottom_shell_layers", "bottom_shell_thickness", "bottom_surface_density", "bottom_surface_pattern", "bottom_surface_fill_order", "bottom_layer_direction", "center_of_surface_pattern", "top_bottom_infill_wall_overlap"})},
            {QStringLiteral("Infill"), processKeys({"adaptive_infill_gradient", "adaptive_infill_interior_density", "adaptive_infill_mode", "infill_complete_top", "sparse_infill_density", "fill_multiline", "sparse_infill_pattern", "gyroid_optimized", "sparse_infill_smooth_factor", "infill_direction", "sparse_infill_rotate_template", "skin_infill_density", "skeleton_infill_density", "infill_lock_depth", "skin_infill_depth", "skin_infill_line_width", "skeleton_infill_line_width", "symmetric_infill_y_axis", "infill_shift_step", "lateral_lattice_angle_1", "lateral_lattice_angle_2", "infill_overhang_angle", "lightning_overhang_angle", "lightning_prune_angle", "lightning_straightening_angle", "infill_anchor_max", "infill_anchor", "internal_solid_infill_pattern", "solid_infill_direction", "solid_infill_rotate_template", "gap_fill_target", "filter_out_gap_fill", "separated_infills", "infill_wall_overlap"})},
            {QStringLiteral("Advanced"), processKeys({"align_infill_direction_to_model", "extra_solid_infills", "bridge_angle", "internal_bridge_angle", "relative_bridge_angle", "minimum_sparse_infill_area", "infill_combination", "infill_combination_max_layer_height", "detect_narrow_internal_solid_infill", "ensure_vertical_shell_thickness"})},
        }},
        {QStringLiteral("Speed"), {
            {QStringLiteral("First layer speed"), processKeys({"initial_layer_speed", "initial_layer_infill_speed", "initial_layer_travel_speed", "slow_down_layers"})},
            {QStringLiteral("Other layers speed"), processKeys({"outer_wall_speed", "inner_wall_speed", "small_perimeter_speed", "small_perimeter_threshold", "sparse_infill_speed", "internal_solid_infill_speed", "top_surface_speed", "gap_infill_speed", "ironing_speed", "support_speed", "support_interface_speed", "small_support_perimeter_speed", "small_support_perimeter_threshold"})},
            {QStringLiteral("Overhang speed"), processKeys({"enable_overhang_speed", "slowdown_for_curled_perimeters", "overhang_1_4_speed", "overhang_2_4_speed", "overhang_3_4_speed", "overhang_4_4_speed", "bridge_speed", "internal_bridge_speed"})},
            {QStringLiteral("Travel speed"), processKeys({"travel_speed"})},
            {QStringLiteral("Acceleration"), processKeys({"default_acceleration", "outer_wall_acceleration", "inner_wall_acceleration", "bridge_acceleration", "sparse_infill_acceleration", "internal_solid_infill_acceleration", "initial_layer_acceleration", "initial_layer_travel_acceleration", "top_surface_acceleration", "travel_acceleration", "accel_to_decel_enable", "accel_to_decel_factor"})},
            {QStringLiteral("Junction Deviation"), processKeys({"default_junction_deviation"})},
            {QStringLiteral("Jerk(XY)"), processKeys({"default_jerk", "outer_wall_jerk", "inner_wall_jerk", "infill_jerk", "top_surface_jerk", "initial_layer_jerk", "initial_layer_travel_jerk", "travel_jerk"})},
            {QStringLiteral("Advanced"), processKeys({"max_volumetric_extrusion_rate_slope", "max_volumetric_extrusion_rate_slope_segment_length", "extrusion_rate_smoothing_external_perimeter_only"})},
        }},
        {QStringLiteral("Support"), {
            {QStringLiteral("Support"), processKeys({"enable_support", "support_type", "support_style", "support_threshold_angle", "support_threshold_overlap", "raft_first_layer_density", "raft_first_layer_expansion", "support_on_build_plate_only", "support_critical_regions_only", "support_remove_small_overhang"})},
            {QStringLiteral("Raft"), processKeys({"raft_layers", "raft_contact_distance"})},
            {QStringLiteral("Filament for Supports"), processKeys({"support_filament", "support_interface_filament", "support_interface_not_for_body"})},
            {QStringLiteral("Support ironing"), processKeys({"support_ironing", "support_ironing_pattern", "support_ironing_flow", "support_ironing_spacing"})},
            {QStringLiteral("Advanced"), processKeys({"support_top_z_distance", "support_bottom_z_distance", "tree_support_wall_count", "support_base_pattern", "support_base_pattern_spacing", "support_angle", "support_interface_top_layers", "support_interface_bottom_layers", "support_interface_pattern", "support_interface_spacing", "support_bottom_interface_spacing", "support_expansion", "support_object_xy_distance", "support_object_first_layer_gap", "bridge_no_support", "max_bridge_length", "independent_support_layer_height"})},
            {QStringLiteral("Tree supports"), processKeys({"tree_support_tip_diameter", "tree_support_branch_distance", "tree_support_branch_distance_organic", "tree_support_top_rate", "tree_support_branch_diameter", "tree_support_branch_diameter_organic", "tree_support_branch_diameter_angle", "tree_support_branch_angle", "tree_support_branch_angle_organic", "tree_support_angle_slow", "tree_support_auto_brim", "tree_support_brim_width"})},
        }},
        {QStringLiteral("Multimaterial"), {
            {QStringLiteral("Prime tower"), processKeys({"enable_prime_tower", "prime_tower_skip_points", "enable_tower_interface_features", "enable_tower_interface_cooldown_during_tower", "prime_tower_enable_framework", "prime_tower_width", "prime_volume", "prime_tower_brim_width", "prime_tower_infill_gap", "wipe_tower_rotation_angle", "wipe_tower_bridging", "wipe_tower_extra_spacing", "wipe_tower_extra_flow", "wipe_tower_max_purge_speed", "wipe_tower_wall_type", "wipe_tower_cone_angle", "wipe_tower_extra_rib_length", "wipe_tower_rib_width", "wipe_tower_fillet_wall", "wipe_tower_no_sparse_layers", "single_extruder_multi_material_priming"})},
            {QStringLiteral("Filament for Features"), processKeys({"outer_wall_filament_id", "inner_wall_filament_id", "sparse_infill_filament_id", "internal_solid_filament_id", "top_surface_filament_id", "bottom_surface_filament_id", "wipe_tower_filament"})},
            {QStringLiteral("Ooze prevention"), processKeys({"ooze_prevention", "standby_temperature_delta", "preheat_time", "preheat_steps"})},
            {QStringLiteral("Flush options"), processKeys({"flush_into_infill", "flush_into_objects", "flush_into_support"})},
            {QStringLiteral("Advanced"), processKeys({"wipe_tower_sparse_layers_combination", "interlocking_beam", "toolchange_ordering", "toolchange_cyclic_first_layer", "toolchange_cyclic_order", "bottom_color_penetration_layers", "top_color_penetration_layers", "interface_shells", "mmu_segmented_region_max_width", "mmu_segmented_region_interlocking_depth", "interlocking_beam_width", "interlocking_orientation", "interlocking_beam_layer_count", "interlocking_depth", "interlocking_boundary_avoidance"})},
        }},
        {QStringLiteral("Others"), {
            {QStringLiteral("Skirt"), processKeys({"skirt_loops", "skirt_type", "min_skirt_length", "skirt_distance", "skirt_start_angle", "skirt_speed", "skirt_height", "draft_shield", "single_loop_draft_shield"})},
            {QStringLiteral("Brim"), processKeys({"brim_type", "brim_width", "extra_brim_width", "leading_brim_length", "brim_object_gap", "brim_flow_ratio", "brim_use_efc_outline", "combine_brims", "brim_ears_max_angle", "brim_ears_detection_length", "brim_ears_outer_only"})},
            {QStringLiteral("Special mode"), processKeys({"slicing_mode", "print_sequence", "print_order", "spiral_mode", "spiral_mode_smooth", "spiral_mode_max_xy_smoothing", "spiral_starting_flow_ratio", "spiral_finishing_flow_ratio", "timelapse_type", "enable_wrapping_detection", "wipe_inward", "wipe_inward_distance"})},
            {QStringLiteral("Fuzzy skin"), processKeys({"fuzzy_skin", "fuzzy_skin_mode", "fuzzy_skin_noise_type", "fuzzy_skin_point_distance", "fuzzy_skin_thickness", "fuzzy_skin_scale", "fuzzy_skin_octaves", "fuzzy_skin_persistence", "fuzzy_skin_ripples_per_layer", "fuzzy_skin_ripple_offset", "fuzzy_skin_layers_between_ripple_offset", "fuzzy_skin_first_layer"})},
            {QStringLiteral("G-code output"), processKeys({"reduce_infill_retraction", "gcode_add_line_number", "gcode_comments", "gcode_label_objects", "exclude_object", "exclude_object_tolerance", "high_precision_gcode", "filename_format"})},
            {QStringLiteral("Change extrusion role G-code"), processKeys({"process_change_extrusion_role_gcode"})},
            {QStringLiteral("Post-processing Scripts"), processKeys({"post_process"})},
            // Upstream 0a3724ed2f: shared "plugin_config_overrides" was removed in
            // favor of per-tab keys (PrintConfig.cpp:1093-1095 migration note);
            // TabPrint::build hosts print_plugin_config_overrides (Tab.cpp:3126).
            {QStringLiteral("Plugin Configuration"), processKeys({"print_plugin_config_overrides"})},
            {QStringLiteral("Notes"), processKeys({"notes"})},
        }},
    };
    return definitions;
  }

  // Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp TabFilament::build + add_filament_overrides_page (pin 0a3724ed2f); generated by build/tmp_edits/ui_cmp/gen_tiers.py
  static const QList<ProcessPageDefinition> &filamentPageDefinitions()
  {
    static const QList<ProcessPageDefinition> definitions = {
        {QStringLiteral("Filament"), {
            {QStringLiteral("Basic information"), processKeys({"filament_type", "filament_vendor", "filament_soluble", "filament_is_support", "filament_change_length", "default_filament_colour", "filament_diameter", "filament_adhesiveness_category", "filament_density", "filament_shrink", "filament_shrinkage_compensation_z", "filament_cost", "temperature_vitrification", "idle_temperature", "nozzle_temperature_range_low", "nozzle_temperature_range_high"})},
            {QStringLiteral("Flow ratio and Pressure Advance"), processKeys({"pellet_flow_coefficient", "filament_flow_ratio", "enable_pressure_advance", "pressure_advance", "adaptive_pressure_advance", "adaptive_pressure_advance_overhangs", "adaptive_pressure_advance_bridges", "adaptive_pressure_advance_model"})},
            {QStringLiteral("Print chamber temperature"), processKeys({"activate_chamber_temp_control", "chamber_temperature", "chamber_minimal_temperature"})},
            {QStringLiteral("Print temperature"), processKeys({"nozzle_temperature_initial_layer", "nozzle_temperature"})},
            {QStringLiteral("Bed temperature"), processKeys({"supertack_plate_temp_initial_layer", "supertack_plate_temp", "cool_plate_temp_initial_layer", "cool_plate_temp", "textured_cool_plate_temp_initial_layer", "textured_cool_plate_temp", "eng_plate_temp_initial_layer", "eng_plate_temp", "hot_plate_temp_initial_layer", "hot_plate_temp", "textured_plate_temp_initial_layer", "textured_plate_temp"})},
            {QStringLiteral("Volumetric speed limitation"), processKeys({"filament_adaptive_volumetric_speed", "filament_max_volumetric_speed"})},
        }},
        {QStringLiteral("Cooling"), {
            {QStringLiteral("Cooling for specific layer"), processKeys({"close_fan_the_first_x_layers", "initial_layer_fan_speed", "full_fan_speed_layer"})},
            {QStringLiteral("Part cooling fan"), processKeys({"fan_min_speed", "fan_cooling_layer_time", "fan_max_speed", "slow_down_layer_time", "reduce_fan_stop_start_freq", "slow_down_for_layer_cooling", "dont_slow_down_outer_wall", "slow_down_min_speed", "enable_overhang_bridge_fan", "overhang_fan_threshold", "overhang_fan_speed", "internal_bridge_fan_speed", "support_material_interface_fan_speed", "ironing_fan_speed"})},
            {QStringLiteral("Auxiliary part cooling fan"), processKeys({"additional_cooling_fan_speed"})},
            {QStringLiteral("Exhaust fan"), processKeys({"activate_air_filtration", "activate_air_filtration_during_print", "during_print_exhaust_fan_speed", "activate_air_filtration_on_completion", "complete_print_exhaust_fan_speed"})},
        }},
        {QStringLiteral("Setting Overrides"), {
            {QStringLiteral("Retraction"), processKeys({"filament_retraction_length", "filament_z_hop", "filament_z_hop_types", "filament_retract_lift_above", "filament_retract_lift_below", "filament_retract_lift_enforce", "filament_retraction_speed", "filament_deretraction_speed", "filament_retract_restart_extra", "filament_retraction_minimum_travel", "filament_retract_when_changing_layer", "filament_wipe", "filament_wipe_distance", "filament_retract_before_wipe", "filament_retract_after_wipe", "filament_long_retractions_when_cut", "filament_retraction_distances_when_cut"})},
            {QStringLiteral("Retraction when switching material"), processKeys({"filament_retract_length_toolchange", "filament_retract_restart_extra_toolchange"})},
            {QStringLiteral("Ironing"), processKeys({"filament_ironing_flow", "filament_ironing_spacing", "filament_ironing_inset", "filament_ironing_speed"})},
        }},
        {QStringLiteral("Advanced"), {
            {QStringLiteral("Filament start G-code"), processKeys({"filament_start_gcode"})},
            {QStringLiteral("Change extrusion role G-code"), processKeys({"filament_change_extrusion_role_gcode"})},
            {QStringLiteral("Filament end G-code"), processKeys({"filament_end_gcode"})},
            {QStringLiteral("Plugin Configuration"), processKeys({"filament_plugin_config_overrides"})},
        }},
        {QStringLiteral("Multimaterial"), {
            {QStringLiteral("Wipe tower parameters"), processKeys({"filament_minimal_purge_on_wipe_tower", "filament_tower_interface_pre_extrusion_dist", "filament_tower_interface_pre_extrusion_length", "filament_tower_ironing_area", "filament_tower_interface_purge_volume", "filament_tower_interface_print_temp"})},
            {QStringLiteral("Multi Filament"), processKeys({"long_retractions_when_ec", "retraction_distances_when_ec"})},
            {QStringLiteral("Tool change parameters with single extruder MM printers"), processKeys({"filament_loading_speed_start", "filament_loading_speed", "filament_unloading_speed_start", "filament_unloading_speed", "filament_toolchange_delay", "filament_cooling_moves", "filament_cooling_initial_speed", "filament_cooling_final_speed", "filament_stamping_loading_speed", "filament_stamping_distance", "filament_ramming_parameters"})},
            {QStringLiteral("Tool change parameters with multi extruder MM printers"), processKeys({"filament_multitool_ramming", "filament_multitool_ramming_volume", "filament_multitool_ramming_flow"})},
        }},
        {QStringLiteral("Dependencies"), {
            {QStringLiteral("Compatible printers"), processKeys({"compatible_printers", "compatible_printers_condition"})},
            {QStringLiteral("Compatible process profiles"), processKeys({"compatible_prints", "compatible_prints_condition"})},
        }},
        {QStringLiteral("Notes"), {
            {QStringLiteral("Notes"), processKeys({"filament_notes"})},
        }},
    };
    return definitions;
  }
  // Source: Tab.cpp TabPrinter::build_fff + build_kinematics_page + build_unregular_pages (pin 0a3724ed2f; Marlin/Klipper page order, single extruder page).
  // build_unregular_pages inserts Motion ability at index 2, then Multimaterial at
  // end()-2 and the extruder pages after it: Basic, G-code, Multimaterial, Extruder,
  // Motion ability, Notes.
  static const QList<ProcessPageDefinition> &printerPageDefinitions()
  {
    static const QList<ProcessPageDefinition> definitions = {
        {QStringLiteral("Basic information"), {
            {QStringLiteral("Printable space"), processKeys({"printable_area", "parallel_printheads_count", "bed_exclude_area", "printable_height", "support_multi_bed_types", "best_object_pos", "z_offset", "preferred_orientation"})},
            {QStringLiteral("Advanced"), processKeys({"printer_structure", "gcode_flavor", "gcode_skip_config_block", "pellet_modded_printer", "bbl_use_printhost", "use_3mf", "scan_first_layer", "enable_power_loss_recovery", "disable_m73", "thumbnails", "use_relative_e_distances", "use_firmware_retraction", "time_cost"})},
            {QStringLiteral("Plugin Configuration"), processKeys({"printer_plugin_config_overrides"})},
            {QStringLiteral("Cooling Fan"), processKeys({"fan_speedup_time", "fan_speedup_overhangs", "fan_kickstart", "part_cooling_fan_min_pwm"})},
            {QStringLiteral("Extruder Clearance"), processKeys({"extruder_clearance_radius", "extruder_clearance_height_to_rod", "extruder_clearance_height_to_lid"})},
            {QStringLiteral("Adaptive bed mesh"), processKeys({"bed_mesh_min", "bed_mesh_max", "bed_mesh_probe_distance", "adaptive_bed_mesh_margin"})},
            {QStringLiteral("Accessory"), processKeys({"nozzle_type", "nozzle_hrc", "auxiliary_fan", "fan_direction", "support_chamber_temp_control", "support_air_filtration", "cooling_filter_enabled"})},
        }},
        {QStringLiteral("Machine G-code"), {
            {QStringLiteral("File header G-code"), processKeys({"file_start_gcode"})},
            {QStringLiteral("Machine start G-code"), processKeys({"machine_start_gcode"})},
            {QStringLiteral("Machine end G-code"), processKeys({"machine_end_gcode"})},
            {QStringLiteral("Printing by object G-code"), processKeys({"printing_by_object_gcode"})},
            {QStringLiteral("Before layer change G-code"), processKeys({"before_layer_change_gcode"})},
            {QStringLiteral("Layer change G-code"), processKeys({"layer_change_gcode"})},
            {QStringLiteral("Timelapse G-code"), processKeys({"time_lapse_gcode"})},
            {QStringLiteral("Clumping Detection G-code"), processKeys({"wrapping_detection_gcode"})},
            {QStringLiteral("Change filament G-code"), processKeys({"change_filament_gcode"})},
            {QStringLiteral("Change extrusion role G-code"), processKeys({"change_extrusion_role_gcode"})},
            {QStringLiteral("Pause G-code"), processKeys({"machine_pause_gcode"})},
            {QStringLiteral("Template Custom G-code"), processKeys({"template_custom_gcode"})},
        }},
        {QStringLiteral("Multimaterial"), {
            {QStringLiteral("Single extruder multi-material setup"), processKeys({"single_extruder_multi_material", "manual_filament_change", "bed_temperature_formula"})},
            {QStringLiteral("Wipe tower"), processKeys({"wipe_tower_type", "purge_in_prime_tower", "enable_filament_ramming", "tool_change_on_wipe_tower", "wait_for_temp_on_wipe_tower"})},
            {QStringLiteral("Single extruder multi-material parameters"), processKeys({"cooling_tube_retraction", "cooling_tube_length", "parking_pos_retraction", "extra_loading_move", "high_current_on_filament_swap"})},
            {QStringLiteral("Advanced"), processKeys({"machine_load_filament_time", "machine_unload_filament_time", "machine_tool_change_time"})},
        }},
        {QStringLiteral("Extruder"), {
            {QStringLiteral("Basic information"), processKeys({"nozzle_diameter", "nozzle_volume", "extruder_printable_height", "extruder_printable_area"})},
            {QStringLiteral("Layer height limits"), processKeys({"min_layer_height", "max_layer_height"})},
            {QStringLiteral("Position"), processKeys({"extruder_offset"})},
            {QStringLiteral("Retraction"), processKeys({"retraction_length", "retract_restart_extra", "retraction_speed", "deretraction_speed", "retraction_minimum_travel", "retract_when_changing_layer", "wipe", "wipe_distance", "retract_before_wipe", "retract_after_wipe"})},
            {QStringLiteral("Z-Hop"), processKeys({"retract_lift_enforce", "z_hop_types", "z_hop", "travel_slope", "retract_lift_above", "retract_lift_below"})},
            {QStringLiteral("Retraction when switching material"), processKeys({"retract_length_toolchange", "retract_restart_extra_toolchange", "long_retractions_when_cut", "retraction_distances_when_cut"})},
        }},
        {QStringLiteral("Motion ability"), {
            {QStringLiteral("Advanced"), processKeys({"emit_machine_limits_to_gcode"})},
            {QStringLiteral("Resonance Compensation"), processKeys({"resonance_avoidance", "min_resonance_avoidance_speed", "max_resonance_avoidance_speed", "input_shaping_emit", "input_shaping_type", "input_shaping_freq_x", "input_shaping_freq_y", "input_shaping_damp_x", "input_shaping_damp_y"})},
            {QStringLiteral("Speed limitation"), processKeys({"machine_max_speed_x", "machine_max_speed_y", "machine_max_speed_z", "machine_max_speed_e"})},
            {QStringLiteral("Acceleration limitation"), processKeys({"machine_max_acceleration_x", "machine_max_acceleration_y", "machine_max_acceleration_z", "machine_max_acceleration_e", "machine_max_acceleration_extruding", "machine_max_acceleration_retracting", "machine_max_acceleration_travel"})},
            {QStringLiteral("Jerk limitation"), processKeys({"machine_max_junction_deviation", "machine_max_jerk_x", "machine_max_jerk_y", "machine_max_jerk_z", "machine_max_jerk_e"})},
        }},
        {QStringLiteral("Notes"), {
            {QStringLiteral("Notes"), processKeys({"printer_notes"})},
        }}
    };
    return definitions;
  }
  // Hierarchy per settings tier: 0 = Print (process), 1 = Filament, 2 = Printer.
  static const QList<ProcessPageDefinition> &hierarchyDefinitions(int tier)
  {
    if (tier == 1)
      return filamentPageDefinitions();
    if (tier == 2)
      return printerPageDefinitions();
    return processPageDefinitions();
  }

  static const ProcessGroupDefinition *processGroupDefinition(const QList<ProcessPageDefinition> &definitions,
                                                              const QString &page, const QString &group)
  {
    for (const ProcessPageDefinition &pageDefinition : definitions) {
      if (pageDefinition.name != page)
        continue;
      for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
        if (groupDefinition.name == group)
          return &groupDefinition;
      }
      return nullptr;
    }
    return nullptr;
  }

  static const QHash<QString, QPair<QString, QString>> &processPageGroupLookup()
  {
    static const QHash<QString, QPair<QString, QString>> lookup = [] {
      QHash<QString, QPair<QString, QString>> result;
      for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
        for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
          for (const QString &key : groupDefinition.optionKeys) {
            Q_ASSERT(!result.contains(key));
            result.insert(key, qMakePair(pageDefinition.name, groupDefinition.name));
          }
        }
      }
      return result;
    }();
    return lookup;
  }

  static QStringList processManifestKeys()
  {
    QStringList result;
    const auto &lookup = processPageGroupLookup();
    result.reserve(lookup.size());
    for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
      for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
        for (const QString &key : groupDefinition.optionKeys)
          result.append(key);
      }
    }
    return result;
  }

#ifdef HAS_LIBSLIC3R
  // Preset::print_options() includes a small set of source-backed preset fields
  // that TabPrint intentionally does not render as editable rows.
  static const QSet<QString> &processNonUiPresetKeys()
  {
    static const QSet<QString> keys = {
        QStringLiteral("travel_speed_z"),
        QStringLiteral("enforce_support_layers"),
        QStringLiteral("raft_expansion"),
        QStringLiteral("support_interface_loop_pattern"),
        QStringLiteral("interface_shells"),
        // R6: ironing_expansion is a print_options() field upstream keeps out
        // of TabPrint::build as well (Tab.cpp:2706 commented, "not used yet").
        // It used to sit in the Z contouring whitelist, showing what upstream
        // deliberately hides.
        QStringLiteral("ironing_expansion"),
        QStringLiteral("compatible_printers"),
        QStringLiteral("compatible_printers_condition"),
        QStringLiteral("inherits"),
        // overhang_speed_classic was removed from print_options() upstream.
        QStringLiteral("wiping_volumes_extruders"),
        // print_options() entries with no Qt editor row. At pin 0a3724ed2f
        // upstream TabPrint::build does show slicing_pipeline_plugin (plugin
        // picker over installed plugins); OWzx has no plugin runtime yet, so
        // it stays off the page (tracked gap). filename_format, notes,
        // post_process and process_change_extrusion_role_gcode are on the
        // Others page.
        QStringLiteral("prime_tower_flat_ironing"),
        QStringLiteral("print_extruder_id"),
        QStringLiteral("print_extruder_variant"),
        QStringLiteral("plugins"),
        QStringLiteral("slicing_pipeline_plugin"),
        QStringLiteral("calib_flowrate_topinfill_special_order"),
        // Orca 2.5 additions: belt-printer and IMEX machine keys without a
        // Qt editor row (belt/imex hardware OWzx does not target yet).
        QStringLiteral("belt_purge_tower_object"),
        QStringLiteral("belt_purge_tower_width"),
        QStringLiteral("imex_head_filament_map"),
        QStringLiteral("imex_parallel_mode"),
    };
    return keys;
  }

  static QStringList auditProcessManifest()
  {
    const auto &lookup = processPageGroupLookup();
    const auto &nonUiKeys = processNonUiPresetKeys();
    QSet<QString> upstreamKeys;
    QStringList errors;

    for (const std::string &rawKey : Slic3r::Preset::print_options()) {
      const QString key = QString::fromStdString(rawKey);
      upstreamKeys.insert(key);
      if (!lookup.contains(key) && !nonUiKeys.contains(key))
        errors.append(QStringLiteral("Unmapped Process preset key: %1").arg(key));
    }

    for (const QString &key : processManifestKeys()) {
      if (!upstreamKeys.contains(key))
        errors.append(QStringLiteral("Process manifest key is not a print preset option: %1").arg(key));
      if (!Slic3r::print_config_def.has(key.toStdString()))
        errors.append(QStringLiteral("Process manifest key is missing from print_config_def: %1").arg(key));
    }

    for (const QString &key : nonUiKeys) {
      if (!upstreamKeys.contains(key))
        errors.append(QStringLiteral("Stale non-UI Process preset key: %1").arg(key));
    }

    errors.sort(Qt::CaseSensitive);
    return errors;
  }



#endif // HAS_LIBSLIC3R

} // anonymous namespace

// Assign page/group from the tier-specific mapping table.
// Called AFTER loadSchemaFromKeys populates m_options but BEFORE endResetModel().

// zh_CN group labels for the printer/filament tier maps above (2026-10-26
// i18n batch). Source: upstream OrcaSlicer_zh_CN.po, same policy as
// OptionLabelTable — static data, never tr(). Process-tier groups arrive
// in Chinese from the preset schema and pass through unchanged.
// zh_CN enum-value labels (2026-10-26 i18n completion): upstream
// PrintConfig enum_labels L(...) strings through OrcaSlicer_zh_CN.po,
// same static-data policy as OptionLabelTable / zhGroupLabel. Unmapped
// labels (proper nouns: Arachne, CoreXY, ...) pass through.
static QString zhEnumLabel(const QString &label)
{
  static const QHash<QString, QString> map = {
      {QStringLiteral("0 (Simple connect)"), QStringLiteral("0（简单连接）")},
      {QStringLiteral("0 (no open anchors)"), QStringLiteral("0 (无锚线)")},
      {QStringLiteral("1000 (unlimited)"), QStringLiteral("1000（无限制）")},
      {QStringLiteral("3D Honeycomb"), QStringLiteral("3D 蜂窝")},
      {QStringLiteral("API key"), QStringLiteral("API秘钥")},
      {QStringLiteral("Adaptive Cubic"), QStringLiteral("自适应立方体")},
      {QStringLiteral("Aligned"), QStringLiteral("对齐")},
      {QStringLiteral("Aligned Rectilinear"), QStringLiteral("直线排列")},
      {QStringLiteral("Aligned back"), QStringLiteral("背部对齐")},
      {QStringLiteral("All"), QStringLiteral("所有")},
      {QStringLiteral("All Surfaces"), QStringLiteral("所有表面")},
      {QStringLiteral("All solid layers"), QStringLiteral("所有实心层")},
      {QStringLiteral("All top surfaces"), QStringLiteral("顶面")},
      {QStringLiteral("All walls"), QStringLiteral("所有墙")},
      {QStringLiteral("Apply to all"), QStringLiteral("全部应用")},
      {QStringLiteral("Archimedean Chords"), QStringLiteral("阿基米德和弦")},
      {QStringLiteral("As object list"), QStringLiteral("按对象列表中的顺序")},
      {QStringLiteral("Auto"), QStringLiteral("自动")},
      {QStringLiteral("Auto For Flush"), QStringLiteral("自动冲洗")},
      {QStringLiteral("Auto For Match"), QStringLiteral("自动匹配")},
      {QStringLiteral("Back"), QStringLiteral("返回")},
      {QStringLiteral("Best of all (shortest path)"), QStringLiteral("全部比较（最短路径）")},
      {QStringLiteral("Billow"), QStringLiteral("云状噪波")},
      {QStringLiteral("Both"), QStringLiteral("两者")},
      {QStringLiteral("Bottom Only"), QStringLiteral("仅底面")},
      {QStringLiteral("Bowden"), QStringLiteral("远程挤出机")},
      {QStringLiteral("Brass"), QStringLiteral("黄铜")},
      {QStringLiteral("By First filament"), QStringLiteral("根据初始打印耗材")},
      {QStringLiteral("By Highest Temp"), QStringLiteral("根据最高打印温度")},
      {QStringLiteral("By layer"), QStringLiteral("逐层")},
      {QStringLiteral("By object"), QStringLiteral("逐件")},
      {QStringLiteral("Classic"), QStringLiteral("经典")},
      {QStringLiteral("Clockwise"), QStringLiteral("顺时针")},
      {QStringLiteral("Close holes"), QStringLiteral("闭孔")},
      {QStringLiteral("Combined"), QStringLiteral("合并")},
      {QStringLiteral("Concentric"), QStringLiteral("同心")},
      {QStringLiteral("Cone"), QStringLiteral("锥体")},
      {QStringLiteral("Contour"), QStringLiteral("轮廓")},
      {QStringLiteral("Contour and hole"), QStringLiteral("轮廓和孔")},
      {QStringLiteral("Cool Plate (SuperTack)"), QStringLiteral("低温打印板（超强粘附）")},
      {QStringLiteral("Counter clockwise"), QStringLiteral("逆时针")},
      {QStringLiteral("Critical Only"), QStringLiteral("仅关键区域")},
      {QStringLiteral("Cross Hatch"), QStringLiteral("交叉层叠")},
      {QStringLiteral("Cross Zag"), QStringLiteral("交叉之字")},
      {QStringLiteral("Cubic"), QStringLiteral("立方体")},
      {QStringLiteral("Customize"), QStringLiteral("自定义")},
      {QStringLiteral("Cyclic"), QStringLiteral("循环")},
      {QStringLiteral("Default"), QStringLiteral("默认")},
      {QStringLiteral("Default (Grid/Organic)"), QStringLiteral("默认 (网格/有机)")},
      {QStringLiteral("Delta"), QStringLiteral("Delta（三角）")},
      {QStringLiteral("Direct Drive"), QStringLiteral("直驱（近程）")},
      {QStringLiteral("Disable"), QStringLiteral("禁用")},
      {QStringLiteral("Disabled"), QStringLiteral("禁用")},
      {QStringLiteral("Displacement"), QStringLiteral("位移")},
      {QStringLiteral("Each Assembly"), QStringLiteral("每个装配体")},
      {QStringLiteral("Each Model"), QStringLiteral("每个模型")},
      {QStringLiteral("Each Surface"), QStringLiteral("每个表面")},
      {QStringLiteral("Enable"), QStringLiteral("开启")},
      {QStringLiteral("Enabled"), QStringLiteral("启用")},
      {QStringLiteral("Engineering Plate"), QStringLiteral("工程材料热床")},
      {QStringLiteral("Even-odd"), QStringLiteral("奇偶")},
      {QStringLiteral("Everywhere"), QStringLiteral("所有地方")},
      {QStringLiteral("External bridge only"), QStringLiteral("仅外部桥接")},
      {QStringLiteral("Extrusion"), QStringLiteral("挤出")},
      {QStringLiteral("Fast"), QStringLiteral("快速")},
      {QStringLiteral("Filter"), QStringLiteral("过滤")},
      {QStringLiteral("Grid"), QStringLiteral("网格")},
      {QStringLiteral("Gyroid"), QStringLiteral("螺旋体")},
      {QStringLiteral("HTTP digest"), QStringLiteral("HTTP摘要")},
      {QStringLiteral("Hardened steel"), QStringLiteral("硬化钢")},
      {QStringLiteral("High Flow"), QStringLiteral("高流量")},
      {QStringLiteral("Hilbert Curve"), QStringLiteral("希尔伯特曲线")},
      {QStringLiteral("Hole"), QStringLiteral("孔")},
      {QStringLiteral("Hollow"), QStringLiteral("空心")},
      {QStringLiteral("Honeycomb"), QStringLiteral("蜂窝")},
      {QStringLiteral("Hybrid"), QStringLiteral("混合")},
      {QStringLiteral("Inner brim only"), QStringLiteral("仅内侧")},
      {QStringLiteral("Inner/Outer"), QStringLiteral("内墙/外墙")},
      {QStringLiteral("Inner/Outer/Inner"), QStringLiteral("内墙/外墙/内墙")},
      {QStringLiteral("Internal bridge only"), QStringLiteral("仅内部桥接")},
      {QStringLiteral("Inward"), QStringLiteral("向内")},
      {QStringLiteral("Inward and Outward"), QStringLiteral("向内和向外")},
      {QStringLiteral("Klipper"), QStringLiteral("Klipper固件")},
      {QStringLiteral("Lateral Honeycomb"), QStringLiteral("侧向蜂窝")},
      {QStringLiteral("Lateral Lattice"), QStringLiteral("侧向向晶格")},
      {QStringLiteral("Left"), QStringLiteral("左")},
      {QStringLiteral("Lightning"), QStringLiteral("闪电")},
      {QStringLiteral("Limited filtering"), QStringLiteral("有限保留")},
      {QStringLiteral("Line"), QStringLiteral("线")},
      {QStringLiteral("Locked Zag"), QStringLiteral("限定之字")},
      {QStringLiteral("Manual"), QStringLiteral("手动")},
      {QStringLiteral("Moderate"), QStringLiteral("适量")},
      {QStringLiteral("Monotonic"), QStringLiteral("单调")},
      {QStringLiteral("Monotonic line"), QStringLiteral("单调线")},
      {QStringLiteral("Mouse ear"), QStringLiteral("圆盘")},
      {QStringLiteral("Nearest"), QStringLiteral("最近")},
      {QStringLiteral("No filtering"), QStringLiteral("保留全部")},
      {QStringLiteral("No ironing"), QStringLiteral("不熨烫")},
      {QStringLiteral("No-brim"), QStringLiteral("无brim")},
      {QStringLiteral("None"), QStringLiteral("无")},
      {QStringLiteral("Normal"), QStringLiteral("普通")},
      {QStringLiteral("Normal (auto)"), QStringLiteral("普通(自动)")},
      {QStringLiteral("Normal (manual)"), QStringLiteral("普通(手动)")},
      {QStringLiteral("Nowhere"), QStringLiteral("不填充")},
      {QStringLiteral("Nozzle Manual"), QStringLiteral("喷嘴手册")},
      {QStringLiteral("Octagram Spiral"), QStringLiteral("八角螺旋")},
      {QStringLiteral("Organic"), QStringLiteral("有机树")},
      {QStringLiteral("Outer and inner brim"), QStringLiteral("内侧和外侧")},
      {QStringLiteral("Outer brim only"), QStringLiteral("仅外侧")},
      {QStringLiteral("Outer/Inner"), QStringLiteral("外墙/内墙")},
      {QStringLiteral("Outward"), QStringLiteral("向外")},
      {QStringLiteral("Painted"), QStringLiteral("绘制")},
      {QStringLiteral("Painted only"), QStringLiteral("仅涂漆")},
      {QStringLiteral("Partially bridged"), QStringLiteral("部分搭桥")},
      {QStringLiteral("Per object"), QStringLiteral("按对象")},
      {QStringLiteral("Perlin"), QStringLiteral("柏林噪波")},
      {QStringLiteral("Printer configuration"), QStringLiteral("打印机配置")},
      {QStringLiteral("Quarter Cubic"), QStringLiteral("四分之一立方体")},
      {QStringLiteral("Random"), QStringLiteral("随机")},
      {QStringLiteral("Rectangle"), QStringLiteral("矩形")},
      {QStringLiteral("Rectilinear"), QStringLiteral("直线")},
      {QStringLiteral("Rectilinear Interlaced"), QStringLiteral("交叠的直线")},
      {QStringLiteral("Rectilinear grid"), QStringLiteral("直线网格")},
      {QStringLiteral("Regular"), QStringLiteral("常规")},
      {QStringLiteral("Rib"), QStringLiteral("肋条")},
      {QStringLiteral("Ridged Multifractal"), QStringLiteral("脊状多重分形")},
      {QStringLiteral("Right"), QStringLiteral("右")},
      {QStringLiteral("Ripple"), QStringLiteral("波纹")},
      {QStringLiteral("Sacrificial layer"), QStringLiteral("牺牲层")},
      {QStringLiteral("Same as top"), QStringLiteral("和顶部相同")},
      {QStringLiteral("Saving"), QStringLiteral("节省")},
      {QStringLiteral("Slope"), QStringLiteral("梯形")},
      {QStringLiteral("Smooth"), QStringLiteral("平滑模式")},
      {QStringLiteral("Smooth Cool Plate"), QStringLiteral("光滑的低温打印床")},
      {QStringLiteral("Smooth High Temp Plate"), QStringLiteral("光滑高温打印热床")},
      {QStringLiteral("Snake"), QStringLiteral("蛇形")},
      {QStringLiteral("Snug"), QStringLiteral("紧贴")},
      {QStringLiteral("Spiral"), QStringLiteral("螺旋")},
      {QStringLiteral("Stainless steel"), QStringLiteral("不锈钢")},
      {QStringLiteral("Standard"), QStringLiteral("标准")},
      {QStringLiteral("Support Cubic"), QStringLiteral("支撑立方体")},
      {QStringLiteral("TPMS-D"), QStringLiteral("TPMS-D结构")},
      {QStringLiteral("TPMS-FK"), QStringLiteral("TPMS-FK结构")},
      {QStringLiteral("TPU High Flow"), QStringLiteral("TPU 高流量")},
      {QStringLiteral("Textured Cool Plate"), QStringLiteral("纹理的低温打印床")},
      {QStringLiteral("Textured PEI Plate"), QStringLiteral("纹理PEI热床")},
      {QStringLiteral("Top Only"), QStringLiteral("仅顶面")},
      {QStringLiteral("Top and Bottom"), QStringLiteral("顶面和地面")},
      {QStringLiteral("Top and bottom surfaces"), QStringLiteral("仅顶层和底层")},
      {QStringLiteral("Topmost surface only"), QStringLiteral("最顶面")},
      {QStringLiteral("Traditional"), QStringLiteral("传统模式")},
      {QStringLiteral("Tree (auto)"), QStringLiteral("树状(自动)")},
      {QStringLiteral("Tree (manual)"), QStringLiteral("树状(手动)")},
      {QStringLiteral("Tree Hybrid"), QStringLiteral("混合树")},
      {QStringLiteral("Tree Slim"), QStringLiteral("苗条树")},
      {QStringLiteral("Tree Strong"), QStringLiteral("粗壮树")},
      {QStringLiteral("Tri-hexagon"), QStringLiteral("内六边形")},
      {QStringLiteral("Triangles"), QStringLiteral("三角形")},
      {QStringLiteral("Tungsten carbide"), QStringLiteral("碳化钨")},
      {QStringLiteral("Type 1"), QStringLiteral("类型 1")},
      {QStringLiteral("Type 2"), QStringLiteral("类型 2")},
      {QStringLiteral("Undefined"), QStringLiteral("未定义")},
      {QStringLiteral("Voronoi"), QStringLiteral("维诺图")},
      {QStringLiteral("Zig Zag"), QStringLiteral("之字形")},
      {QStringLiteral("default"), QStringLiteral("默认")},
  };
  return map.value(label, label);
}

static QString zhGroupLabel(const QString &group)
{
  static const QHash<QString, QString> map = {
      // Upstream tab hierarchy pages/groups (zh_CN strings from
      // OrcaSlicer localization/i18n/zh_CN/OrcaSlicer_zh_CN.po).
      // Print tab pages/groups (also the unsaved-changes list headers).
      {QStringLiteral("Quality"), QStringLiteral("质量")},
      {QStringLiteral("Strength"), QStringLiteral("强度")},
      {QStringLiteral("Speed"), QStringLiteral("速度")},
      {QStringLiteral("Support"), QStringLiteral("支撑")},
      {QStringLiteral("Others"), QStringLiteral("其他")},
      {QStringLiteral("Layer height"), QStringLiteral("层高")},
      {QStringLiteral("Line width"), QStringLiteral("线宽")},
      {QStringLiteral("Seam"), QStringLiteral("接缝")},
      {QStringLiteral("Precision"), QStringLiteral("精度")},
      {QStringLiteral("Wall generator"), QStringLiteral("墙生成器")},
      {QStringLiteral("Walls and surfaces"), QStringLiteral("墙壁和表面")},
      {QStringLiteral("Bridging"), QStringLiteral("搭桥")},
      {QStringLiteral("Overhangs"), QStringLiteral("悬垂")},
      {QStringLiteral("Walls"), QStringLiteral("墙")},
      {QStringLiteral("Top/bottom shells"), QStringLiteral("顶部/底部外壳")},
      {QStringLiteral("Infill"), QStringLiteral("填充")},
      {QStringLiteral("Initial layer speed"), QStringLiteral("首层速度")},
      {QStringLiteral("Other layers speed"), QStringLiteral("其他层速度")},
      {QStringLiteral("Overhang speed"), QStringLiteral("悬垂速度")},
      {QStringLiteral("Travel speed"), QStringLiteral("空驶速度")},
      {QStringLiteral("Acceleration"), QStringLiteral("加速度")},
      {QStringLiteral("Jerk(XY)"), QStringLiteral("抖动（XY轴）")},
      {QStringLiteral("Raft"), QStringLiteral("筏层")},
      {QStringLiteral("Support filament"), QStringLiteral("支撑耗材")},
      {QStringLiteral("Tree supports"), QStringLiteral("树状支撑")},
      {QStringLiteral("Prime tower"), QStringLiteral("擦拭塔")},
      {QStringLiteral("Filament for Features"), QStringLiteral("特征用耗材")},
      {QStringLiteral("Ooze prevention"), QStringLiteral("Ooze 预防")},
      {QStringLiteral("Flush options"), QStringLiteral("换料冲刷选项")},
      {QStringLiteral("Skirt"), QStringLiteral("裙边")},
      {QStringLiteral("Brim"), QStringLiteral("Brim")},
      {QStringLiteral("Special mode"), QStringLiteral("特殊模式")},
      {QStringLiteral("G-code output"), QStringLiteral("G-code 输出")},
      {QStringLiteral("Post-processing Scripts"), QStringLiteral("后处理脚本")},
      {QStringLiteral("Accessory"), QStringLiteral("配件")},
      {QStringLiteral("Adaptive bed mesh"), QStringLiteral("自适应网床")},
      {QStringLiteral("Clumping Detection G-code"), QStringLiteral("结块检测 G-code")},
      {QStringLiteral("Compatible printers"), QStringLiteral("兼容打印机")},
      {QStringLiteral("Compatible process profiles"), QStringLiteral("兼容的切片配置")},
      {QStringLiteral("Cooling"), QStringLiteral("冷却")},
      {QStringLiteral("Dependencies"), QStringLiteral("依赖")},
      {QStringLiteral("Exhaust fan"), QStringLiteral("排气风扇")},
      {QStringLiteral("Extruder"), QStringLiteral("挤出机")},
      {QStringLiteral("Extruder Clearance"), QStringLiteral("挤出机避让空间")},
      {QStringLiteral("Filament"), QStringLiteral("耗材丝")},
      {QStringLiteral("File header G-code"), QStringLiteral("文件头 G-code")},
      {QStringLiteral("Layer height limits"), QStringLiteral("层高限制")},
      {QStringLiteral("Machine G-code"), QStringLiteral("打印机G-code")},
      {QStringLiteral("Motion ability"), QStringLiteral("移动能力")},
      {QStringLiteral("Multi Filament"), QStringLiteral("复丝")},
      {QStringLiteral("Multimaterial"), QStringLiteral("材料")},
      {QStringLiteral("Plugin Configuration"), QStringLiteral("插件配置")},
      {QStringLiteral("Position"), QStringLiteral("位置")},
      {QStringLiteral("Resonance Compensation"), QStringLiteral("共振补偿")},
      {QStringLiteral("Retraction when switching material"), QStringLiteral("切换材料时的回抽量")},
      {QStringLiteral("Setting Overrides"), QStringLiteral("参数覆盖")},
      {QStringLiteral("Single extruder multi-material parameters"), QStringLiteral("单挤出机多材料参数")},
      {QStringLiteral("Single extruder multi-material setup"), QStringLiteral("设置单挤出机多材料")},
      {QStringLiteral("Tool change parameters with multi extruder MM printers"), QStringLiteral("多挤出机多材料打印机的换色参数")},
      {QStringLiteral("Tool change parameters with single extruder MM printers"), QStringLiteral("单挤出机多材料打印机的换色参数")},
      {QStringLiteral("Wipe tower"), QStringLiteral("擦拭塔")},
      {QStringLiteral("Wipe tower parameters"), QStringLiteral("擦拭塔参数")},
      {QStringLiteral("Z-Hop"), QStringLiteral("Z轴抬升")},
      {QStringLiteral("Acceleration limitation"), QStringLiteral("加速度限制")},
      {QStringLiteral("Advanced"), QStringLiteral("高级")},
      {QStringLiteral("Basic information"), QStringLiteral("基础信息")},
      {QStringLiteral("Bed temperature"), QStringLiteral("热床温度")},
      {QStringLiteral("Before layer change G-code"), QStringLiteral("换层前G-code")},
      {QStringLiteral("Change extrusion role G-code"), QStringLiteral("挤出类型更换G-code")},
      {QStringLiteral("Change filament G-code"), QStringLiteral("耗材丝更换G-code")},
      {QStringLiteral("Cooling Fan"), QStringLiteral("冷却风扇")},
      {QStringLiteral("Cooling for specific layer"), QStringLiteral("特定层冷却")},
      {QStringLiteral("Filament end G-code"), QStringLiteral("耗材丝结束G-code")},
      {QStringLiteral("Filament start G-code"), QStringLiteral("耗材丝起始G-code")},
      {QStringLiteral("Flow ratio and Pressure Advance"), QStringLiteral("流量和压力提前")},
      {QStringLiteral("Jerk limitation"), QStringLiteral("抖动限制")},
      {QStringLiteral("Layer change G-code"), QStringLiteral("换层G-code")},
      {QStringLiteral("Machine end G-code"), QStringLiteral("打印机结束G-code")},
      {QStringLiteral("Machine start G-code"), QStringLiteral("打印机起始G-code")},
      {QStringLiteral("Notes"), QStringLiteral("注释")},
      {QStringLiteral("Part cooling fan"), QStringLiteral("部件冷却风扇")},
      {QStringLiteral("Pause G-code"), QStringLiteral("暂停 G-code")},
      {QStringLiteral("Print chamber temperature"), QStringLiteral("打印仓温度")},
      {QStringLiteral("Print temperature"), QStringLiteral("打印温度")},
      {QStringLiteral("Printable space"), QStringLiteral("可打印区域")},
      {QStringLiteral("Printing by object G-code"), QStringLiteral("逐件打印G-code")},
      {QStringLiteral("Retraction"), QStringLiteral("回抽")},
      {QStringLiteral("Size"), QStringLiteral("大小")},
      {QStringLiteral("Speed limitation"), QStringLiteral("速度限制")},
      {QStringLiteral("Template Custom G-code"), QStringLiteral("模板自定义G-code")},
      {QStringLiteral("Timelapse G-code"), QStringLiteral("延时摄影G-code")},
      {QStringLiteral("Volumetric speed limitation"), QStringLiteral("体积流量限制")},
      // F7 groups
      {QStringLiteral("Ironing"), QStringLiteral("匀光")},
      {QStringLiteral("Air filtration"), QStringLiteral("空气过滤")},
      {QStringLiteral("Toolchange and flush"), QStringLiteral("换料与冲洗")},
      {QStringLiteral("Auxiliary part cooling fan"), QStringLiteral("辅助冷却风扇")},
  };
  return map.value(group, group);
}

QString ConfigOptionModel::hierarchyLabel(const QString &name) const
{
  const int extruder = extruderOfPage(name);
  if (extruder >= 0 && name != QStringLiteral("Extruder"))
    return zhGroupLabel(QStringLiteral("Extruder")) + QStringLiteral(" %1").arg(extruder + 1);
  return zhGroupLabel(name);
}

QStringList ConfigOptionModel::processPageNames() const
{
  QStringList result;
  const QList<ProcessPageDefinition> &definitions = hierarchyDefinitions(m_hierarchyTier);
  result.reserve(definitions.size());
  for (const ProcessPageDefinition &definition : definitions)
  {
    if (m_hiddenPages.contains(definition.name))
      continue;
    // One Extruder page per extruder (Tab.cpp:5639-5645).
    if (m_hierarchyTier == 2 && definition.name == QStringLiteral("Extruder") && m_extruderCount > 1)
    {
      for (int extruder = 1; extruder <= m_extruderCount; ++extruder)
        result.append(QStringLiteral("Extruder %1").arg(extruder));
      continue;
    }
    result.append(definition.name);
  }
  return result;
}

QString ConfigOptionModel::basePageName(const QString &page) const
{
  return extruderOfPage(page) >= 0 ? QStringLiteral("Extruder") : page;
}

int ConfigOptionModel::extruderOfPage(const QString &page) const
{
  if (page == QStringLiteral("Extruder"))
    return 0;
  static const QString prefix = QStringLiteral("Extruder ");
  if (!page.startsWith(prefix))
    return -1;
  bool ok = false;
  const int number = page.mid(prefix.size()).toInt(&ok);
  return ok && number >= 1 ? number - 1 : -1;
}

void ConfigOptionModel::setActiveExtruder(int extruder)
{
  extruder = std::clamp(extruder, 0, std::max(0, m_extruderCount - 1));
  if (extruder == m_activeExtruder)
    return;
  m_activeExtruder = extruder;
  emit activeExtruderChanged();
  if (!m_options.isEmpty())
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole});
  ++m_dataVersion;
  emit dataVersionChanged();
}

bool ConfigOptionModel::isExtruderRow(int row) const
{
  return m_hierarchyTier == 2 && row >= 0 && row < m_options.size() && m_options[row].isVector &&
         m_options[row].page == QStringLiteral("Extruder");
}

QVariant ConfigOptionModel::editValue(int row) const
{
  if (row < 0 || row >= m_options.size())
    return {};
  const QVariant &value = m_options[row].value;
  if (!isExtruderRow(row) || (value.typeId() != QMetaType::QVariantList && value.typeId() != QMetaType::QStringList))
    return value;
  const QVariantList list = value.toList();
  if (list.isEmpty())
    return value;
  return QVariantList{list.value(std::min<qsizetype>(m_activeExtruder, list.size() - 1))};
}

void ConfigOptionModel::refreshExtruderCount()
{
  int count = 1;
  if (m_hierarchyTier == 2)
  {
    const int row = findIndex(QStringLiteral("nozzle_diameter"));
    if (row >= 0)
    {
      const QVariant &value = m_options[row].value;
      if (value.typeId() == QMetaType::QVariantList || value.typeId() == QMetaType::QStringList)
        count = std::max<int>(1, int(value.toList().size()));
    }
  }
  if (count == m_extruderCount)
    return;
  m_extruderCount = count;
  emit extruderCountChanged();
  if (m_activeExtruder >= m_extruderCount)
    setActiveExtruder(m_extruderCount - 1);
}

QStringList ConfigOptionModel::hiddenPages() const
{
  QStringList pages(m_hiddenPages.cbegin(), m_hiddenPages.cend());
  pages.sort();
  return pages;
}

void ConfigOptionModel::setHiddenPages(const QSet<QString> &pages)
{
  if (pages == m_hiddenPages)
    return;
  m_hiddenPages = pages;
  emit hiddenPagesChanged();
}

QStringList ConfigOptionModel::processGroupsForPage(const QString &page) const
{
  QStringList result;
  for (const ProcessPageDefinition &pageDefinition : hierarchyDefinitions(m_hierarchyTier)) {
    if (pageDefinition.name != page)
      continue;
    result.reserve(pageDefinition.groups.size());
    for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups)
      result.append(groupDefinition.name);
    return result;
  }
  return result;
}

QList<int> ConfigOptionModel::orderedProcessIndicesForGroup(const QList<int> &candidateIndices,
                                                            const QString &page,
                                                            const QString &group) const
{
  const ProcessGroupDefinition *definition = processGroupDefinition(hierarchyDefinitions(m_hierarchyTier), page, group);
  if (!definition)
    return {};

  QHash<QString, int> candidateByKey;
  candidateByKey.reserve(candidateIndices.size());
  for (int index : candidateIndices) {
    if (index < 0 || index >= m_options.size())
      continue;
    const ConfigOption &option = m_options[index];
    if (option.page == page && option.group == group)
      candidateByKey.insert(option.key, index);
  }

  QList<int> result;
  result.reserve(definition->optionKeys.size());
  for (const QString &key : definition->optionKeys) {
    const auto candidate = candidateByKey.constFind(key);
    if (candidate != candidateByKey.cend())
      result.append(candidate.value());
  }
  return result;
}

int ConfigOptionModel::processProjectedRowCount(const QString &page, const QString &needle, bool advancedMode) const
{
  return processProjectedRowCountForMode(page, needle, advancedMode ? 2 : 0);
}

int ConfigOptionModel::processProjectedRowCountForMode(const QString &page, const QString &needle, int viewMode) const
{
  // Count of rows surviving the same projection the per-group proxies apply:
  // search filter -> page filter -> the (page, group) process definition must
  // exist and contain the option key. Equivalent to aggregating the
  // orderedProcessIndicesForGroup(...) non-empty check over all groups of the
  // page, so an empty count means no group row would be visible.
  int count = 0;
  for (int i = 0; i < m_options.size(); ++i)
  {
    if (!matchesFilter(i, needle, viewMode))
      continue;
    if (!matchesPage(i, page))
      continue;
    const ProcessGroupDefinition *definition = processGroupDefinition(hierarchyDefinitions(m_hierarchyTier), page, m_options[i].group);
    if (!definition || !definition->optionKeys.contains(m_options[i].key))
      continue;
    ++count;
  }
  return count;
}

int ConfigOptionModel::processOptionRank(const QString &page, const QString &group, int row) const
{
  // Upstream manifest order (processGroupDefinition ->optionKeys position);
  // rows outside the manifest sort last.
  if (row < 0 || row >= m_options.size())
    return INT_MAX;
  const ProcessGroupDefinition *definition = processGroupDefinition(hierarchyDefinitions(m_hierarchyTier), page, group);
  if (!definition)
    return INT_MAX;
  const int rank = definition->optionKeys.indexOf(m_options[row].key);
  return rank >= 0 ? rank : INT_MAX;
}

QStringList ConfigOptionModel::processMappingErrors() const
{
#ifdef HAS_LIBSLIC3R
  return auditProcessManifest();
#else
  return {};
#endif
}

bool ConfigOptionModel::matchesLineMember(int row, const QString &headKey, int viewMode) const
{
  if (row < 0 || row >= m_options.size() || headKey.isEmpty())
    return false;
  const ConfigOption &o = m_options[row];
  return m_lineHeadByKey.value(o.key) == headKey && o.mode <= viewMode;
}

int ConfigOptionModel::lineMemberRank(int row) const
{
  if (row < 0 || row >= m_options.size())
    return INT_MAX;
  const QString &key = m_options[row].key;
  const int rank = m_lineMembersByHead.value(m_lineHeadByKey.value(key)).indexOf(key);
  return rank >= 0 ? rank : INT_MAX;
}

void ConfigOptionModel::assignProcessHierarchyMetadata()
{
  m_lineHeadByKey.clear();
  m_lineMembersByHead.clear();
  m_lineLabelByHead.clear();
  for (const OptionLineDefinition &line : lineDefinitions(m_hierarchyTier))
  {
    const QString head = line.keys.first();
    if (findIndex(head) < 0)
      continue;
    QStringList members;
    for (const QString &key : line.keys)
      if (findIndex(key) >= 0)
      {
        m_lineHeadByKey.insert(key, head);
        members.append(key);
      }
    m_lineMembersByHead.insert(head, members);
    m_lineLabelByHead.insert(head, line.label);
  }

  QHash<QString, QPair<QString, QString>> lookup;
  for (const ProcessPageDefinition &pageDefinition : hierarchyDefinitions(m_hierarchyTier))
    for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups)
      for (const QString &key : groupDefinition.optionKeys)
        lookup.insert(key, qMakePair(pageDefinition.name, groupDefinition.name));
  for (ConfigOption &option : m_options) {
    const auto location = lookup.constFind(option.key);
    if (location == lookup.cend())
      continue;
    option.page = location.value().first;
    option.group = location.value().second;
  }
}

#ifdef HAS_LIBSLIC3R

void ConfigOptionModel::loadSchemaFromKeys(const char *const keys[])
{
  beginResetModel();
  m_options.clear();
  m_baseReadonlyKeys.clear();
  m_defaultValues.clear();
  m_referenceValues.clear();
  m_dirtyKeys.clear();

  const auto &def = Slic3r::print_config_def;

  for (int ki = 0; keys[ki] != nullptr; ++ki)
  {
    const auto *opt = def.get(keys[ki]);
    if (!opt)
      continue;

    ConfigOption entry;
    entry.key = QString::fromUtf8(opt->opt_key.c_str());
    entry.label = QString::fromUtf8(opt->label.c_str());
    entry.type = mapType(opt->type);
    // Upstream OptionsGroup builds a PointCtrl (X/Y pair, first element for
    // per-extruder vectors) for every coPoints option except the polygon
    // keys, which Field.cpp edits as one "XxY, XxY, ..." text line
    // (OptionsGroup.cpp:1174-1183); printable_area is the Set... button.
    // Upstream GUIType::plugin_config: PluginConfigField, a "配置 (N)"
    // button opening PluginsConfigDialog (Field.cpp PluginConfigField).
    if (opt->gui_type == Slic3r::ConfigOptionDef::GUIType::plugin_config)
      entry.type = QStringLiteral("plugin_config");
    // Upstream GUIType::color: ColourPicker button.
    if (opt->gui_type == Slic3r::ConfigOptionDef::GUIType::color)
      entry.type = QStringLiteral("color");
    if (opt->opt_key == "compatible_printers" || opt->opt_key == "compatible_prints")
      entry.type = QStringLiteral("compatible_list");
    entry.serialized = opt->gui_flags == "serialized" && opt->type == Slic3r::coStrings;
    // Upstream open enums: an editable Choice whose list only suggests
    // values (Field.cpp Choice, wxCB_DROPDOWN). Options whose list is filled
    // at runtime (filament ids, print host ports) have no enum_values in the
    // def and keep their plain editor.
    using GUIType = Slic3r::ConfigOptionDef::GUIType;
    if ((opt->gui_type == GUIType::i_enum_open || opt->gui_type == GUIType::f_enum_open ||
         opt->gui_type == GUIType::select_open) && !opt->enum_values.empty())
    {
      const Slic3r::ConfigOptionType base = Slic3r::ConfigOptionType(opt->type & ~Slic3r::coVectorType);
      entry.openBase = base == Slic3r::coInt ? QStringLiteral("int")
                       : base == Slic3r::coFloatOrPercent ? QStringLiteral("floatOrPercent")
                       : base == Slic3r::coFloat || base == Slic3r::coPercent ? QStringLiteral("float")
                                                                             : QStringLiteral("string");
      entry.type = QStringLiteral("enum_open");
    }
    if (opt->type == Slic3r::coPoints
        && opt->opt_key != "printable_area" && opt->opt_key != "bed_exclude_area"
        && opt->opt_key != "wrapping_exclude_area")
      entry.type = QStringLiteral("point");
    entry.category = mapCategory(opt->category);
    entry.readonly = opt->readonly;
    entry.value = extractDefault(opt);

    // Range
    entry.min = static_cast<double>(opt->min);
    entry.max = static_cast<double>(opt->max);
    if (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent)
      entry.max = opt->max_literal;

    // Step
    entry.step = (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent) ? 0.01 : 1.0;

    // Enum labels — translated at the model boundary (zhEnumLabel).
    for (size_t i = 0; i < opt->enum_labels.size() && i < opt->enum_values.size(); ++i)
    {
      entry.enumLabels.append(zhEnumLabel(QString::fromUtf8(opt->enum_labels[i].c_str())));
      entry.enumValues.append(QString::fromUtf8(opt->enum_values[i].c_str()));
    }
    // Open enums may list values only (filament_type): the value is the label.
    if (!entry.openBase.isEmpty() && opt->enum_labels.empty())
      for (const std::string &value : opt->enum_values)
      {
        entry.enumLabels.append(QString::fromUtf8(value.c_str()));
        entry.enumValues.append(QString::fromUtf8(value.c_str()));
      }

    // Tooltip from upstream (对齐上游 ConfigOptionDef::tooltip)
    if (!opt->tooltip.empty())
      entry.tooltip = QString::fromUtf8(opt->tooltip.c_str());

    // Sidetext (unit label) from upstream (对齐上游 ConfigOptionDef::sidetext)
    if (!opt->sidetext.empty())
      entry.sidetext = QString::fromUtf8(opt->sidetext.c_str());

    // Mode from upstream (对齐上游 ConfigOptionMode: 0=Simple, 1=Advanced, 2=Expert, 3=Develop)
    entry.mode = static_cast<int>(opt->mode);

    // Nullable: accepts nil = inherit from parent preset (upstream ConfigOptionDef::nullable)
    entry.nullable = opt->nullable;

    // IsVector: multi-value per-extruder (upstream coVectorType = 0x4000 bit)
    entry.isVector = ((opt->type & Slic3r::coVectorType) != 0);

    // Field.cpp TextCtrl: wxTE_MULTILINE only when the def asks for it;
    // every other string is a single-line field.
    entry.multiline = opt->multiline;

    m_options.append(entry);
    m_defaultValues.insert(entry.key, entry.value);

    if (entry.readonly)
      m_baseReadonlyKeys.insert(entry.key);
  }

  endResetModel();
  emit countChanged();
}

void ConfigOptionModel::loadFromUpstreamSchema()
{
  const QStringList mappingErrors = processMappingErrors();
  if (!mappingErrors.isEmpty()) {
    qCritical().noquote() << "Process settings hierarchy audit failed:" << mappingErrors.join(QStringLiteral("; "));
    static const char *const kNoKeys[] = {nullptr};
    loadSchemaFromKeys(kNoKeys);
    return;
  }

  const QStringList manifestKeys = processManifestKeys();
  QVector<QByteArray> encodedKeys;
  encodedKeys.reserve(manifestKeys.size());
  QVector<const char *> keyPointers;
  keyPointers.reserve(manifestKeys.size() + 1);
  for (const QString &key : manifestKeys) {
    encodedKeys.append(key.toUtf8());
    keyPointers.append(encodedKeys.constLast().constData());
  }
  keyPointers.append(nullptr);
  loadSchemaFromKeys(keyPointers.constData());
  assignProcessHierarchyMetadata();
}

void ConfigOptionModel::loadTierSchema(int tier)
{
  // Rows follow the upstream tab hierarchy (page -> group -> option order) of
  // the tier; the same grouped projection the Print tier uses renders them.
  m_hierarchyTier = tier;
  QVector<QByteArray> encodedKeys;
  for (const ProcessPageDefinition &pageDefinition : hierarchyDefinitions(tier))
    for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups)
      for (const QString &key : groupDefinition.optionKeys)
        encodedKeys.append(key.toUtf8());
  QVector<const char *> keyPointers;
  keyPointers.reserve(encodedKeys.size() + 1);
  for (const QByteArray &key : encodedKeys)
    keyPointers.append(key.constData());
  keyPointers.append(nullptr);
  loadSchemaFromKeys(keyPointers.constData());
  assignProcessHierarchyMetadata();
}

void ConfigOptionModel::loadMachineSchema()
{
  loadTierSchema(2);
}

void ConfigOptionModel::loadFilamentSchema()
{
  loadTierSchema(1);
}

#endif // HAS_LIBSLIC3R

void ConfigOptionModel::setToggleState(const QSet<QString> &hiddenKeys, const QSet<QString> &disabledKeys)
{
  if (hiddenKeys == m_hiddenKeys && disabledKeys == m_disabledKeys)
    return;
  const bool visibilityChanged = hiddenKeys != m_hiddenKeys;
  const QSet<QString> oldHidden = m_hiddenKeys;
  const QSet<QString> oldDisabled = m_disabledKeys;
  m_hiddenKeys = hiddenKeys;
  m_disabledKeys = disabledKeys;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    if (oldHidden.contains(key) != m_hiddenKeys.contains(key) ||
        oldDisabled.contains(key) != m_disabledKeys.contains(key))
      emit dataChanged(index(i), index(i), {EnabledRole, LineVisibleRole});
  }
  if (visibilityChanged)
    emit toggleVisibilityChanged();
}

void ConfigOptionModel::setOverrideStates(const QHash<QString, QVariantMap> &states)
{
  if (states == m_overrideStates)
    return;
  const QHash<QString, QVariantMap> old = m_overrideStates;
  m_overrideStates = states;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    if (old.value(key) != m_overrideStates.value(key))
      emit dataChanged(index(i), index(i), {OverrideRole, OverrideEnabledRole, InheritedValueRole});
  }
}

void ConfigOptionModel::setOverrideChecked(int row, bool checked)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto it = m_overrideStates.constFind(m_options[row].key);
  if (it == m_overrideStates.cend())
    return;
  if (!checked)
  {
    // Upstream set_at_to_nil: the vector entry becomes nil.
    writeValue(row, QVariantList{QStringLiteral("nil")});
    return;
  }
  // set_last_meaningful_value: start from the inherited value.
  const QVariant inheritedValue = it->value(QStringLiteral("inheritedValue"));
  writeValue(row, inheritedValue.typeId() == QMetaType::QVariantList ? inheritedValue : QVariant(QVariantList{inheritedValue}));
}
