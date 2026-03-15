#include "ConfigViewModel.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "qml_gui/Models/ConfigOptionModel.h"
#include "qml_gui/Models/PresetListModel.h"

ConfigViewModel::ConfigViewModel(PresetServiceMock *presetService, ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), presetService_(presetService), projectService_(projectService)
{
  printOptions_ = new ConfigOptionModel(this);
#ifdef HAS_LIBSLIC3R
  printOptions_->loadFromUpstreamSchema();
#endif
  presetList_ = new PresetListModel(this);

  scopedWritableKeys_ = {
      // Layer
      QStringLiteral("layer_height"),
      QStringLiteral("initial_layer_print_height"),
      QStringLiteral("line_width"),
      QStringLiteral("initial_layer_line_width"),
      // Shell
      QStringLiteral("wall_loops"),
      QStringLiteral("top_shell_layers"),
      QStringLiteral("bottom_shell_layers"),
      QStringLiteral("wall_infill_order"),
      QStringLiteral("infill_wall_overlap"),
      QStringLiteral("top_bottom_infill_wall_overlap"),
      QStringLiteral("outer_wall_line_width"),
      QStringLiteral("inner_wall_line_width"),
      QStringLiteral("wall_sequence"),
      // Infill
      QStringLiteral("sparse_infill_density"),
      QStringLiteral("sparse_infill_pattern"),
      QStringLiteral("infill_direction"),
      // Speed
      QStringLiteral("outer_wall_speed"),
      QStringLiteral("inner_wall_speed"),
      QStringLiteral("sparse_infill_speed"),
      QStringLiteral("top_surface_speed"),
      QStringLiteral("support_speed"),
      QStringLiteral("travel_speed"),
      QStringLiteral("initial_layer_speed"),
      QStringLiteral("bridge_speed"),
      QStringLiteral("internal_bridge_speed"),
      QStringLiteral("initial_layer_infill_speed"),
      QStringLiteral("gap_infill_speed"),
      // Acceleration
      QStringLiteral("outer_wall_acceleration"),
      QStringLiteral("inner_wall_acceleration"),
      QStringLiteral("travel_acceleration"),
      QStringLiteral("default_acceleration"),
      // Temperature
      QStringLiteral("nozzle_temp"),
      QStringLiteral("bed_temp"),
      QStringLiteral("chamber_temperature"),
      QStringLiteral("nozzle_temperature_initial_layer"),
      // Support
      QStringLiteral("enable_support"),
      QStringLiteral("support_type"),
      QStringLiteral("support_density"),
      QStringLiteral("support_on_build_plate_only"),
      QStringLiteral("support_interface_top_layers"),
      QStringLiteral("support_interface_bottom_layers"),
      QStringLiteral("support_speed"),
      QStringLiteral("support_angle"),
      // Adhesion
      QStringLiteral("brim_enable"),
      QStringLiteral("brim_width"),
      QStringLiteral("brim_type"),
      QStringLiteral("skirt_loops"),
      QStringLiteral("skirt_distance"),
      QStringLiteral("adhesion_type"),
      // Retraction
      QStringLiteral("retract_length"),
      QStringLiteral("retract_speed"),
      QStringLiteral("deretraction_speed"),
      QStringLiteral("retract_length_toolchange"),
      QStringLiteral("z_hop"),
      // Cooling
      QStringLiteral("fan_speed"),
      QStringLiteral("min_fan_speed"),
      QStringLiteral("overhang_fan_speed"),
      QStringLiteral("slow_down_layer_time"),
      QStringLiteral("close_fan_the_first_x_layers"),
      // Ironing
      QStringLiteral("ironing_type"),
      QStringLiteral("ironing_speed"),
      // Quality
      QStringLiteral("max_print_speed"),
      QStringLiteral("reduce_crossing_wall"),
      QStringLiteral("only_one_wall_top"),
      QStringLiteral("precise_outer_wall")};

  connect(printOptions_, &ConfigOptionModel::optionValueChanged, this, &ConfigViewModel::handleOptionValueChanged);
  loadDefault();
}

QObject *ConfigViewModel::printOptions() const { return printOptions_; }
QObject *ConfigViewModel::presetList() const { return presetList_; }

QStringList ConfigViewModel::presetNames() const
{
  return presetService_ ? presetService_->presetNames() : QStringList{};
}

QString ConfigViewModel::currentPreset() const
{
  return currentPreset_;
}

double ConfigViewModel::layerHeight() const
{
  return layerHeight_;
}

void ConfigViewModel::loadDefault()
{
  if (presetService_) {
    currentPreset_ = presetService_->defaultPreset();
    currentPrinterPreset_ = presetService_->defaultPresetForCategory(0);
    currentFilamentPreset_ = presetService_->defaultPresetForCategory(1);
    currentPrintPreset_ = presetService_->defaultPresetForCategory(2);
    layerHeight_ = presetService_->defaultLayerHeight();
  }
  printSpeed_ = 300;
  supportEnabled_ = false;
  infillDensity_ = 15;
  nozzleTemp_ = 220;
  bedTemp_ = 65;
  wallCount_ = 3;
  topLayers_ = 4;
  bottomLayers_ = 4;
  enableBrim_ = false;

  // Reset to global scope
  settingsScope_ = QStringLiteral("global");
  settingsTargetObjectIndex_ = -1;
  settingsTargetVolumeIndex_ = -1;
  settingsTargetPlateIndex_ = -1;

  // Reset model values to original defaults, then snapshot global values
  if (printOptions_)
    printOptions_->resetToDefaults();
  globalOptionValues_ = printOptions_ ? printOptions_->valuesByKey() : QHash<QString, QVariant>{};
  savedPresetValues_ = globalOptionValues_; // 初始化预设快照
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::setCurrentPreset(const QString &presetName)
{
  currentPreset_ = presetName;

  // 加载预设值（如果有保存的值）
  if (presetService_ && presetService_->hasPreset(presetName))
  {
    auto presetVals = presetService_->presetValues(presetName);
    if (!presetVals.isEmpty())
    {
      // 切换到全局作用域
      settingsScope_ = QStringLiteral("global");
      settingsTargetObjectIndex_ = -1;
      settingsTargetVolumeIndex_ = -1;

      if (printOptions_)
        printOptions_->applyValues(presetVals);
      globalOptionValues_ = presetVals;
      applyScopeValues();
    }
  }

  // 保存当前预设值的快照（用于脏检测）
  savedPresetValues_ = globalOptionValues_;
  emit stateChanged();
}

void ConfigViewModel::saveCurrentPreset()
{
  if (!presetService_ || currentPreset_.isEmpty())
    return;

  presetService_->savePresetValues(currentPreset_, globalOptionValues_);
  // 更新快照
  savedPresetValues_ = globalOptionValues_;
  emit stateChanged();
}

bool ConfigViewModel::isPresetDirty() const
{
  if (savedPresetValues_.isEmpty())
    return !globalOptionValues_.isEmpty();
  return savedPresetValues_ != globalOptionValues_;
}

// ── 3-tier preset inheritance (对齐上游 PresetBundle) ─────────

QStringList ConfigViewModel::printerPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(0) : QStringList{};
}

QStringList ConfigViewModel::filamentPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(1) : QStringList{};
}

QStringList ConfigViewModel::printPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(2) : QStringList{};
}

void ConfigViewModel::setCurrentPrinterPreset(const QString &name)
{
  currentPrinterPreset_ = name;
  mergePresetHierarchy();
  autoMatchFilament();  // 切换打印机时自动匹配兼容耗材
  emit stateChanged();
}

void ConfigViewModel::setCurrentFilamentPreset(const QString &name)
{
  currentFilamentPreset_ = name;
  mergePresetHierarchy();
  emit stateChanged();
}

void ConfigViewModel::setCurrentPrintPreset(const QString &name)
{
  currentPrintPreset_ = name;
  currentPreset_ = name; // legacy compat
  mergePresetHierarchy();
  emit stateChanged();
}

void ConfigViewModel::mergePresetHierarchy()
{
  if (!printOptions_)
    return;

  // Start with model defaults
  printOptions_->resetToDefaults();
  QHash<QString, QVariant> merged = printOptions_->valuesByKey();

  // 初始化所有 key 的来源为 "default"
  valueSources_.clear();
  for (auto it = merged.begin(); it != merged.end(); ++it)
    valueSources_[it.key()] = QStringLiteral("default");

  if (!presetService_)
    return;

  // Tier 1: Printer preset (base layer)
  {
    const auto vals = presetService_->presetValues(currentPrinterPreset_);
    for (auto it = vals.begin(); it != vals.end(); ++it) {
      merged[it.key()] = it.value();
      valueSources_[it.key()] = QStringLiteral("printer");
    }
  }

  // Tier 2: Filament preset (overrides printer where keys overlap)
  {
    const auto vals = presetService_->presetValues(currentFilamentPreset_);
    for (auto it = vals.begin(); it != vals.end(); ++it) {
      merged[it.key()] = it.value();
      valueSources_[it.key()] = QStringLiteral("filament");
    }
  }

  // Tier 3: Print preset (overrides both where keys overlap)
  {
    const auto vals = presetService_->presetValues(currentPrintPreset_);
    for (auto it = vals.begin(); it != vals.end(); ++it) {
      merged[it.key()] = it.value();
      valueSources_[it.key()] = QStringLiteral("print");
    }
  }

  globalOptionValues_ = merged;
  savedPresetValues_ = merged;
  applyScopeValues();
}

bool ConfigViewModel::createCustomPreset(int category, const QString &name)
{
  if (!presetService_)
    return false;

  // 使用当前全局值作为新预设的值
  return presetService_->createCustomPreset(category, name, globalOptionValues_);
}

bool ConfigViewModel::deletePreset(int category, const QString &name)
{
  if (!presetService_)
    return false;

  // 不允许删除当前正在使用的预设
  if (name == currentPrinterPreset_ || name == currentFilamentPreset_ || name == currentPrintPreset_)
    return false;

  bool ok = presetService_->deletePreset(name);
  if (ok)
  {
    // 如果删除的是当前类别中正在使用的预设，切换回默认
    if (name == currentPrintPreset_)
      setCurrentPrintPreset(presetService_->defaultPresetForCategory(0));
    if (name == currentFilamentPreset_)
      setCurrentFilamentPreset(presetService_->defaultPresetForCategory(1));
    if (name == currentPrinterPreset_)
      setCurrentPrinterPreset(presetService_->defaultPresetForCategory(2));
    emit stateChanged();
  }
  return ok;
}

bool ConfigViewModel::renamePreset(int category, const QString &oldName, const QString &newName)
{
  if (!presetService_ || newName.trimmed().isEmpty())
    return false;

  bool ok = presetService_->renamePreset(oldName, newName.trimmed());
  if (ok)
  {
    // 更新当前活跃预设名引用
    if (oldName == currentPrintPreset_)
      currentPrintPreset_ = newName.trimmed();
    if (oldName == currentFilamentPreset_)
      currentFilamentPreset_ = newName.trimmed();
    if (oldName == currentPrinterPreset_)
      currentPrinterPreset_ = newName.trimmed();
    emit stateChanged();
  }
  return ok;
}

bool ConfigViewModel::canDeletePreset(const QString &name) const
{
  return presetService_ && !presetService_->isBuiltinPreset(name);
}

void ConfigViewModel::autoMatchFilament()
{
  // 对齐上游 PresetBundle::update_compatible
  // 切换打印机后，自动选择兼容的耗材预设（基于 nozzle diameter / max_temp 匹配）
  if (!presetService_)
    return;

  const QString compatible = presetService_->findCompatibleFilament(currentPrinterPreset_);
  if (!compatible.isEmpty() && compatible != currentFilamentPreset_)
  {
    currentFilamentPreset_ = compatible;
    mergePresetHierarchy();
  }
  emit stateChanged();
}

bool ConfigViewModel::isCurrentFilamentCompatible() const
{
  if (!presetService_)
    return true;
  return presetService_->isFilamentCompatibleWithPrinter(currentFilamentPreset_, currentPrinterPreset_);
}

bool ConfigViewModel::isFilamentCompatible(const QString &filamentName) const
{
  if (!presetService_)
    return true;
  return presetService_->isFilamentCompatibleWithPrinter(filamentName, currentPrinterPreset_);
}

void ConfigViewModel::setLayerHeight(double v)
{
  layerHeight_ = v;
  emit stateChanged();
}
void ConfigViewModel::setPrintSpeed(int v)
{
  printSpeed_ = v;
  emit stateChanged();
}
void ConfigViewModel::setSupportEnabled(bool v)
{
  supportEnabled_ = v;
  emit stateChanged();
}
void ConfigViewModel::setInfillDensity(int v)
{
  infillDensity_ = v;
  emit stateChanged();
}
void ConfigViewModel::setNozzleTemp(int v)
{
  nozzleTemp_ = v;
  emit stateChanged();
}
void ConfigViewModel::setBedTemp(int v)
{
  bedTemp_ = v;
  emit stateChanged();
}
void ConfigViewModel::setWallCount(int v)
{
  wallCount_ = v;
  emit stateChanged();
}
void ConfigViewModel::setEnableBrim(bool v)
{
  enableBrim_ = v;
  emit stateChanged();
}

void ConfigViewModel::activateGlobalScope()
{
  settingsScope_ = QStringLiteral("global");
  settingsTargetObjectIndex_ = -1;
  settingsTargetVolumeIndex_ = -1;
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::activateObjectScope(const QString &targetType, const QString &targetName, int objectIndex, int volumeIndex)
{
  settingsTargetType_ = targetType;
  settingsTargetName_ = targetName;
  settingsTargetObjectIndex_ = objectIndex;
  settingsTargetVolumeIndex_ = volumeIndex;
  settingsTargetPlateIndex_ = -1;
  // Distinguish volume scope from object scope (对齐上游 Tab scope semantics)
  if (targetName.isEmpty())
    settingsScope_ = QStringLiteral("global");
  else if (volumeIndex >= 0)
    settingsScope_ = QStringLiteral("volume");
  else
    settingsScope_ = QStringLiteral("object");
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::activatePlateScope(int plateIndex)
{
  settingsScope_ = QStringLiteral("plate");
  settingsTargetPlateIndex_ = plateIndex;
  settingsTargetObjectIndex_ = -1;
  settingsTargetVolumeIndex_ = -1;
  settingsTargetType_ = QStringLiteral("plate");
  settingsTargetName_ = tr("平板 %1").arg(plateIndex + 1);
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::applyScopeValues()
{
  if (!printOptions_)
    return;

  applyingScopeValues_ = true;
  printOptions_->applyValues(buildScopeValues());
  printOptions_->setReadonlyKeys(readonlyKeysForCurrentScope());
  applyingScopeValues_ = false;
}

void ConfigViewModel::handleOptionValueChanged(const QString &key, const QVariant &value)
{
  if (applyingScopeValues_)
    return;

  if (settingsScope_ == QStringLiteral("global") || (settingsScope_ != QStringLiteral("plate") && settingsTargetObjectIndex_ < 0))
  {
    globalOptionValues_[key] = value;
    return;
  }

  // Plate scope: route to plate-scoped storage
  if (settingsScope_ == QStringLiteral("plate") && settingsTargetPlateIndex_ >= 0)
  {
    if (!projectService_ || !projectService_->setPlateScopedOptionValue(settingsTargetPlateIndex_, key, value))
      return;
    applyScopeValues();
    return;
  }

  // Object/volume scope
  if (!scopedWritableKeys_.contains(key))
    return;

  if (!projectService_ || !projectService_->setScopedOptionValue(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, key, value))
    return;

  applyScopeValues();
}

QVariant ConfigViewModel::scopedValueForKey(const QString &key, const QVariant &fallback) const
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return fallback;

  return projectService_->scopedOptionValue(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, key, fallback);
}

QHash<QString, QVariant> ConfigViewModel::buildScopeValues() const
{
  QHash<QString, QVariant> values = globalOptionValues_;

  if (settingsScope_ == QStringLiteral("plate") && settingsTargetPlateIndex_ >= 0)
  {
    // Apply plate-level overrides on top of global values
    if (projectService_)
    {
      for (auto it = values.begin(); it != values.end(); ++it)
      {
        const QVariant plateVal = projectService_->plateScopedOptionValue(settingsTargetPlateIndex_, it.key());
        if (plateVal.isValid())
          it.value() = plateVal;
      }
    }
    return values;
  }

  if ((settingsScope_ == QStringLiteral("object") || settingsScope_ == QStringLiteral("volume")) && settingsTargetObjectIndex_ >= 0)
  {
    // Apply object/volume-level overrides on top of global values
    for (auto it = values.begin(); it != values.end(); ++it)
      it.value() = scopedValueForKey(it.key(), it.value());
    return values;
  }

  return values;
}

QSet<QString> ConfigViewModel::readonlyKeysForCurrentScope() const
{
  if (settingsScope_ == QStringLiteral("volume"))
  {
    // Volume scope: same writable keys as object scope
    QSet<QString> readonly;
    for (auto it = globalOptionValues_.cbegin(); it != globalOptionValues_.cend(); ++it)
    {
      if (!scopedWritableKeys_.contains(it.key()))
        readonly.insert(it.key());
    }
    return readonly;
  }

  if (settingsScope_ == QStringLiteral("object"))
  {
    QSet<QString> readonly;
    for (auto it = globalOptionValues_.cbegin(); it != globalOptionValues_.cend(); ++it)
    {
      if (!scopedWritableKeys_.contains(it.key()))
        readonly.insert(it.key());
    }
    return readonly;
  }

  // Global and plate scope: all keys writable
  return {};
}

// ── Fuzzy matching helper (对齐上游 OptionsSearcher::search / fts_fuzzy_match) ──
namespace {

// Subsequence matching with scoring (simplified Möller–Trumbore-style scan)
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
    prevRow = std::move(curRow);
  }

  int bestScore = prevRow[n].first;
  int score = qMax(0, bestScore);
  outScore = score;
  return score >= 60; // minimum threshold (对齐上游 minimum score)
}

} // anonymous namespace

QList<int> ConfigViewModel::filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode) const
{
  if (!printOptions_)
    return {};

  const int n = printOptions_->rowCount();
  QList<int> result;
  result.reserve(n);

  const bool matchAll = category.isEmpty() || category == QStringLiteral("全部") || category == tr("全部");
  const QString needle = searchText.toLower();
  const bool useFuzzy = needle.length() >= 2; // 启用 fuzzy matching（对齐上游 fts_fuzzy_match）

  for (int i = 0; i < n; ++i)
  {
    if (!matchAll && printOptions_->optCategory(i) != category)
      continue;

    if (!needle.isEmpty())
    {
      bool matched = false;
      if (useFuzzy)
      {
        int score = 0;
        matched = fuzzyMatch(needle, printOptions_->optLabel(i).toLower(), score);
        if (!matched)
          matched = fuzzyMatch(needle, printOptions_->optKey(i).toLower(), score);
      }
      else
      {
        matched = printOptions_->optLabel(i).toLower().contains(needle) ||
                printOptions_->optKey(i).toLower().contains(needle);
      }
      if (!matched)
        continue;
    }

    // Mode filter (对齐上游 ConfigOptionMode: 0=Simple, 1=Advanced, 2=Both)
    const int optMode = printOptions_->optMode(i);
    if (optMode == 1 && !advancedMode)
      continue; // Advanced-only options hidden in simple mode
    if (optMode == 0 && advancedMode)
      continue; // Simple-only options hidden in advanced mode (rare)

    result.append(i);
  }
  return result;
}

Q_INVOKABLE QList<int> ConfigViewModel::moveListItem(int fromRow, int toRow) const
{
  if (!printOptions_)
    return {};

  if (fromRow == toRow || fromRow < 0 || fromRow >= printOptions_->rowCount() || toRow < 0 || toRow >= printOptions_->rowCount())
    return {};

  // Return the swapped row indices for the QML side to handle the visual reorder
  return {fromRow, toRow};
}

QList<int> ConfigViewModel::filterIndicesByPage(const QList<int> &indices, const QString &page) const
{
  if (!printOptions_ || page.isEmpty())
    return indices;
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (printOptions_->optPage(idx) == page)
      result.append(idx);
  }
  return result;
}

QList<int> ConfigViewModel::filterIndicesByCategory(const QList<int> &indices, const QString &category) const
{
  if (!printOptions_ || category.isEmpty())
    return indices;
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (printOptions_->optCategory(idx) == category)
      result.append(idx);
  }
  return result;
}

QString ConfigViewModel::materialPresetName(int localIndex) const
{
  if (!presetList_ || localIndex < 0)
    return {};
  // Use "耗材" to match PresetListModel's category (original QML used "耗材丝" which was a bug)
  const int globalIdx = presetList_->globalIndex(tr("耗材"), localIndex);
  return globalIdx >= 0 ? presetList_->presetName(globalIdx) : QString{};
}

// ── Layer range support (对齐上游 ModelObject::layer_config_ranges) ─────────────────

int ConfigViewModel::layerRangeCount() const
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return 0;
  return projectService_->objectLayerRanges(settingsTargetObjectIndex_).size();
}

double ConfigViewModel::layerRangeMinZ(int rangeIndex) const
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return 0.0;
  const auto ranges = projectService_->objectLayerRanges(settingsTargetObjectIndex_);
  return (rangeIndex >= 0 && rangeIndex < ranges.size()) ? ranges[rangeIndex].minZ : 0.0;
}

double ConfigViewModel::layerRangeMaxZ(int rangeIndex) const
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return 0.0;
  const auto ranges = projectService_->objectLayerRanges(settingsTargetObjectIndex_);
  return (rangeIndex >= 0 && rangeIndex < ranges.size()) ? ranges[rangeIndex].maxZ : 0.0;
}

bool ConfigViewModel::addLayerRange(double minZ, double maxZ)
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return false;
  if (projectService_->addObjectLayerRange(settingsTargetObjectIndex_, minZ, maxZ))
  {
    emit stateChanged();
    return true;
  }
  return false;
}

bool ConfigViewModel::removeLayerRange(int rangeIndex)
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return false;
  if (projectService_->removeObjectLayerRange(settingsTargetObjectIndex_, rangeIndex))
  {
    emit stateChanged();
    return true;
  }
  return false;
}

bool ConfigViewModel::setLayerRangeValue(int rangeIndex, const QString &key, const QVariant &value)
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return false;
  return projectService_->setLayerRangeValue(settingsTargetObjectIndex_, rangeIndex, key, value);
}

QVariant ConfigViewModel::layerRangeValue(int rangeIndex, const QString &key, const QVariant &fallback) const
{
  if (!projectService_ || settingsTargetObjectIndex_ < 0)
    return fallback;
  return projectService_->layerRangeValue(settingsTargetObjectIndex_, rangeIndex, key, fallback);
}

// ── Enhanced search (对齐上游 OptionsSearcher + fts_fuzzy_match) ─────────────

namespace {

/// 对齐上游 fts_fuzzy_match：轻量级模糊匹配算法
/// 返回匹配分数（0=无匹配，正值越高匹配度越好），同时输出是否全部字符连续
/// 算法：贪心字符匹配 + 滑动窗口连续匹配 + 前缀/边界奖励
int fuzzyMatch(const QString &pattern, const QString &text, bool *outAllConsecutive = nullptr)
{
  if (pattern.isEmpty() || text.isEmpty())
    return 0;

  const QChar *pat = pattern.unicode();
  const QChar *str = text.unicode();
  const int patLen = pattern.length();
  const int strLen = text.length();

  // 贪心匹配：允许字符间跳过（gap），统计匹配得分
  int score = 0;
  int patIdx = 0;
  int consecutiveCount = 0;
  int lastMatchPos = -2;
  bool allConsecutive = true;

  for (int i = 0; i < strLen && patIdx < patLen; ++i) {
    if (str[i].toLower() == pat[patIdx].toLower()) {
      // 连续匹配奖励（对齐上游 sequential bonus）
      if (lastMatchPos == i - 1) {
        consecutiveCount++;
        score += 15 + consecutiveCount * 2;
      } else {
        consecutiveCount = 1;
        // 非连续匹配时重置 allConsecutive 标志
        if (lastMatchPos >= 0)
          allConsecutive = false;
        score += 10;
      }
      // 前缀匹配额外奖励
      if (patIdx == 0)
        score += 5;
      // 单词边界匹配奖励（首字母或前一个字符是分隔符）
      if (i == 0 || (str[i - 1] == '_' || str[i - 1] == ' ' || str[i - 1] == '-'))
        score += 10;
      // 全部字符大写匹配时额外奖励（缩写匹配）
      if (pat[patIdx].isUpper())
        score += 5;

      lastMatchPos = i;
      patIdx++;
    }
  }

  if (patIdx < patLen) {
    // 模式未完全匹配
    if (outAllConsecutive) *outAllConsecutive = false;
    return 0;
  }

  // 惩罚跳过的字符数量（对齐上游 gap penalty）
  int gaps = strLen - (lastMatchPos - patIdx + 1 + (patLen - consecutiveCount));
  score -= gaps;

  if (outAllConsecutive)
    *outAllConsecutive = allConsecutive;

  return score;
}

/// 在多个字段中搜索，返回最佳分数和匹配的字段索引
/// fields: 要搜索的文本字段列表
/// 返回最佳模糊匹配分数
int bestFuzzyScore(const QString &needle, const QStringList &fields)
{
  int best = 0;
  for (const auto &field : fields) {
    // 子串完全包含给最高分
    if (field.toLower().contains(needle))
      return 1000 + needle.length() * 10;

    // 模糊匹配
    bool dummy;
    int s = fuzzyMatch(needle, field, &dummy);
    if (s > best)
      best = s;
  }
  return best;
}

} // anonymous namespace

QList<int> ConfigViewModel::searchOptions(const QString &query) const
{
  if (!printOptions_ || query.isEmpty())
    return {};

  const QString needle = query.toLower().trimmed();
  // 对齐上游 OptionsSearcher：score > 阈值才返回
  static constexpr int MIN_SCORE = 10;

  struct ScoredIndex { int index; int score; };
  QList<ScoredIndex> scored;

  for (int i = 0; i < printOptions_->rowCount(); ++i)
  {
    QStringList fields = {
      printOptions_->optKey(i),
      printOptions_->optLabel(i),
      printOptions_->optCategory(i),
      printOptions_->optGroup(i)
    };

    int score = bestFuzzyScore(needle, fields);
    if (score >= MIN_SCORE) {
      scored.append({i, score});
    }
  }

  // 按分数降序排序（对齐上游：高分优先，同分按字母序）
  QObject *opts = printOptions_;
  std::sort(scored.begin(), scored.end(), [opts](const ScoredIndex &a, const ScoredIndex &b) {
    if (a.score != b.score)
      return a.score > b.score;
    auto *optModel = qobject_cast<ConfigOptionModel*>(opts);
    if (optModel)
      return optModel->optKey(a.index) < optModel->optKey(b.index);
    return a.index < b.index;
  });

  QList<int> result;
  result.reserve(scored.size());
  for (const auto &s : scored)
    result.append(s.index);

  m_lastSearchResults_ = result;
  return result;
}

QString ConfigViewModel::valueSourceForKey(const QString &key) const
{
  return valueSources_.value(key, QStringLiteral("default"));
}

QString ConfigViewModel::valueChainForKey(const QString &key) const
{
  // 返回 JSON: {"default":v,"printer":v,"filament":v,"print":v}
  // 对齐上游 PresetBundle value_at_level 可视化
  if (!printOptions_ || !presetService_)
    return QStringLiteral("{\"default\":\"\"}");

  QVariant defVal = printOptions_->defaultValuesByKey().value(key);
  QVariant printerVal = presetService_->presetValue(currentPrinterPreset_, key);
  QVariant filamentVal = presetService_->presetValue(currentFilamentPreset_, key);
  QVariant printVal = presetService_->presetValue(currentPrintPreset_, key);
  QVariant currentVal = globalOptionValues_.value(key, defVal);

  // 构建最终 JSON
  QString result = "{";
  result += "\"default\":\"" + defVal.toString() + "\"";
  result += ",\"printer\":\"" + (printerVal.isValid() ? printerVal.toString() : "-") + "\"";
  result += ",\"filament\":\"" + (filamentVal.isValid() ? filamentVal.toString() : "-") + "\"";
  result += ",\"print\":\"" + (printVal.isValid() ? printVal.toString() : "-") + "\"";
  result += ",\"current\":\"" + currentVal.toString() + "\"";
  result += "}";
  return result;
}

bool ConfigViewModel::resetOptionToLevel(const QString &key, int level)
{
  // level: 0=default, 1=print, 2=filament, 3=printer
  // 对齐上游 Tab reset_to_level
  if (!printOptions_ || !presetService_)
    return false;

  QVariant targetVal;
  switch (level) {
  case 0: targetVal = printOptions_->defaultValuesByKey().value(key); break;
  case 1: targetVal = presetService_->presetValue(currentPrintPreset_, key); break;
  case 2: targetVal = presetService_->presetValue(currentFilamentPreset_, key); break;
  case 3: targetVal = presetService_->presetValue(currentPrinterPreset_, key); break;
  default: return false;
  }

  if (!targetVal.isValid())
    return false;

  globalOptionValues_[key] = targetVal;
  savedPresetValues_[key] = targetVal;
  applyScopeValues();

  // 更新来源层级
  switch (level) {
  case 0: valueSources_[key] = QStringLiteral("default"); break;
  case 1: valueSources_[key] = QStringLiteral("print"); break;
  case 2: valueSources_[key] = QStringLiteral("filament"); break;
  case 3: valueSources_[key] = QStringLiteral("printer"); break;
  }

  emit stateChanged();
  return true;
}

QString ConfigViewModel::searchResultSource(int searchIndex) const
{
  if (searchIndex < 0 || searchIndex >= m_lastSearchResults_.size() || !printOptions_)
    return QStringLiteral("default");
  const int idx = m_lastSearchResults_[searchIndex];
  return valueSources_.value(printOptions_->optKey(idx), QStringLiteral("default"));
}

QString ConfigViewModel::searchResultPath(int searchIndex) const
{
  if (searchIndex < 0 || searchIndex >= m_lastSearchResults_.size() || !printOptions_)
    return {};
  const int idx = m_lastSearchResults_[searchIndex];
  return printOptions_->optPage(idx) + QStringLiteral(" / ") +
         printOptions_->optCategory(idx) + QStringLiteral(" / ") +
         printOptions_->optGroup(idx);
}

QString ConfigViewModel::searchResultGroup(int searchIndex) const
{
  if (searchIndex < 0 || searchIndex >= m_lastSearchResults_.size() || !printOptions_)
    return {};
  return printOptions_->optGroup(m_lastSearchResults_[searchIndex]);
}

QString ConfigViewModel::searchResultCategory(int searchIndex) const
{
  if (searchIndex < 0 || searchIndex >= m_lastSearchResults_.size() || !printOptions_)
    return {};
  return printOptions_->optCategory(m_lastSearchResults_[searchIndex]);
}

QString ConfigViewModel::searchResultPage(int searchIndex) const
{
  if (searchIndex < 0 || searchIndex >= m_lastSearchResults_.size() || !printOptions_)
    return {};
  return printOptions_->optPage(m_lastSearchResults_[searchIndex]);
}

// ── Scope difference (对齐上游 Tab::is_modified_value per-scope diff) ──

QString ConfigViewModel::scopeDiffSummary(const QString &key) const
{
  // Returns a JSON string: {"global":v,"object":v_or_null,"volume":v_or_null,"plate":v_or_null}
  // null indicates no override at that scope
  auto globalVal = globalOptionValues_.value(key);
  if (!projectService_) return QStringLiteral("{}");

  QJsonObject obj;
  obj[QStringLiteral("global")] = QJsonValue::fromVariant(globalVal);

  if (settingsTargetObjectIndex_ >= 0) {
    auto objVal = projectService_->scopedOptionValue(settingsTargetObjectIndex_, -1, key, {});
    if (objVal.isValid())
      obj[QStringLiteral("object")] = QJsonValue::fromVariant(objVal);
    else
      obj[QStringLiteral("object")] = QJsonValue::Null;
  }
  if (settingsTargetObjectIndex_ >= 0 && settingsTargetVolumeIndex_ >= 0) {
    auto volVal = projectService_->scopedOptionValue(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, key, {});
    if (volVal.isValid())
      obj[QStringLiteral("volume")] = QJsonValue::fromVariant(volVal);
    else
      obj[QStringLiteral("volume")] = QJsonValue::Null;
  }
  if (settingsTargetPlateIndex_ >= 0) {
    auto plateVal = projectService_->plateScopedOptionValue(settingsTargetPlateIndex_, key, {});
    if (plateVal.isValid())
      obj[QStringLiteral("plate")] = QJsonValue::fromVariant(plateVal);
    else
      obj[QStringLiteral("plate")] = QJsonValue::Null;
  }
  return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

int ConfigViewModel::scopeOverrideCount() const
{
  if (!projectService_) return 0;
  int count = 0;
  if (settingsScope_ == "object" && settingsTargetObjectIndex_ >= 0)
    count = projectService_->scopedOverrideCount(settingsTargetObjectIndex_, -1);
  else if (settingsScope_ == "volume" && settingsTargetObjectIndex_ >= 0 && settingsTargetVolumeIndex_ >= 0)
    count = projectService_->scopedOverrideCount(settingsTargetObjectIndex_, settingsTargetVolumeIndex_);
  else if (settingsScope_ == "plate" && settingsTargetPlateIndex_ >= 0)
    count = projectService_->plateScopedOverrideCount(settingsTargetPlateIndex_);
  return count;
}

QString ConfigViewModel::scopeOverriddenKey(int index) const
{
  if (!projectService_ || index < 0) return {};
  if (settingsScope_ == "object" && settingsTargetObjectIndex_ >= 0)
    return projectService_->scopedOverriddenKey(settingsTargetObjectIndex_, -1, index);
  if (settingsScope_ == "volume" && settingsTargetObjectIndex_ >= 0 && settingsTargetVolumeIndex_ >= 0)
    return projectService_->scopedOverriddenKey(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, index);
  if (settingsScope_ == "plate" && settingsTargetPlateIndex_ >= 0)
    return projectService_->plateScopedOverriddenKey(settingsTargetPlateIndex_, index);
  return {};
}

bool ConfigViewModel::resetScopeOverride(const QString &key)
{
  if (!projectService_) return false;
  bool ok = false;
  if (settingsScope_ == "object" && settingsTargetObjectIndex_ >= 0)
    ok = projectService_->resetScopedOptionValue(settingsTargetObjectIndex_, -1, key);
  else if (settingsScope_ == "volume" && settingsTargetObjectIndex_ >= 0 && settingsTargetVolumeIndex_ >= 0)
    ok = projectService_->resetScopedOptionValue(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, key);
  else if (settingsScope_ == "plate" && settingsTargetPlateIndex_ >= 0)
    ok = projectService_->resetPlateScopedOptionValue(settingsTargetPlateIndex_, key);
  if (ok) emit stateChanged();
  return ok;
}
