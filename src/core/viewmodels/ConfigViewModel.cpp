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
  machineOptions_ = new ConfigOptionModel(this);
  filamentOptions_ = new ConfigOptionModel(this);
#ifdef HAS_LIBSLIC3R
  printOptions_->loadFromUpstreamSchema();
  machineOptions_->loadMachineSchema();
  filamentOptions_->loadFilamentSchema();
#endif
  presetList_ = new PresetListModel(this);
  presetList_->refreshFromService(presetService_);

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
  connect(machineOptions_, &ConfigOptionModel::optionValueChanged, this, &ConfigViewModel::handleOptionValueChanged);
  connect(filamentOptions_, &ConfigOptionModel::optionValueChanged, this, &ConfigViewModel::handleOptionValueChanged);
  loadDefault();
}

QObject *ConfigViewModel::printOptions() const { return printOptions_; }
QObject *ConfigViewModel::machineOptions() const { return machineOptions_; }
QObject *ConfigViewModel::filamentOptions() const { return filamentOptions_; }
QObject *ConfigViewModel::presetList() const { return presetList_; }

QString ConfigViewModel::normalizedTier(const QString &tier) const
{
  // Accept both new tier strings ("printer"/"filament"/"print") and legacy aliases
  // ("machine"/"process") so existing callers don't break.
  if (tier == QStringLiteral("printer") || tier == QStringLiteral("machine"))
    return QStringLiteral("printer");
  if (tier == QStringLiteral("filament"))
    return QStringLiteral("filament");
  if (tier == QStringLiteral("print") || tier == QStringLiteral("process"))
    return QStringLiteral("print");
  return QStringLiteral("print");
}

ConfigOptionModel *ConfigViewModel::optionModelForTier(const QString &tier) const
{
  const QString normalized = normalizedTier(tier);
  if (normalized == QStringLiteral("printer"))
    return machineOptions_;
  if (normalized == QStringLiteral("filament"))
    return filamentOptions_;
  return printOptions_;
}

QHash<QString, QVariant> ConfigViewModel::editableValuesForTier(const QString &tier) const
{
  const QString normalized = normalizedTier(tier);
  if (normalized == QStringLiteral("printer"))
    return printerPresetValues_;
  if (normalized == QStringLiteral("filament"))
    return filamentPresetValues_;
  return printPresetValues_;
}

QHash<QString, QVariant> ConfigViewModel::referenceValuesForTier(const QString &tier) const
{
  const QString normalized = normalizedTier(tier);
  if (normalized == QStringLiteral("printer"))
    return persistedEffectiveValuesForTier(QStringLiteral("printer"));
  if (normalized == QStringLiteral("filament"))
    return persistedEffectiveValuesForTier(QStringLiteral("filament"));
  return persistedEffectiveValuesForTier(QStringLiteral("print"));
}

QHash<QString, QVariant> ConfigViewModel::selectedPresetValuesForTier(const QString &tier) const
{
  if (!presetService_)
    return {};

  const QString normalized = normalizedTier(tier);
  if (normalized == QStringLiteral("printer"))
    return presetService_->presetValues(currentPrinterPreset_);
  if (normalized == QStringLiteral("filament"))
    return presetService_->presetValues(currentFilamentPreset_);
  return presetService_->presetValues(currentPrintPreset_);
}

QHash<QString, QVariant> ConfigViewModel::persistedEffectiveValuesForTier(const QString &tier) const
{
  const QString normalized = normalizedTier(tier);
  QHash<QString, QVariant> values;

  if (printOptions_)
    values = printOptions_->defaultValuesByKey();

  if (normalized == QStringLiteral("printer"))
  {
    const auto printerValues = selectedPresetValuesForTier(QStringLiteral("printer"));
    for (auto it = printerValues.cbegin(); it != printerValues.cend(); ++it)
      values.insert(it.key(), it.value());
    return values;
  }

  values = persistedEffectiveValuesForTier(QStringLiteral("printer"));
  if (normalized == QStringLiteral("filament"))
  {
    const auto filamentValues = selectedPresetValuesForTier(QStringLiteral("filament"));
    for (auto it = filamentValues.cbegin(); it != filamentValues.cend(); ++it)
      values.insert(it.key(), it.value());
    return values;
  }

  values = persistedEffectiveValuesForTier(QStringLiteral("filament"));
  const auto printValues = selectedPresetValuesForTier(QStringLiteral("print"));
  for (auto it = printValues.cbegin(); it != printValues.cend(); ++it)
    values.insert(it.key(), it.value());
  return values;
}

QHash<QString, QVariant> ConfigViewModel::effectivePresetValuesForTier(const QString &tier) const
{
  const QString normalized = normalizedTier(tier);
  QHash<QString, QVariant> values;

  if (printOptions_)
    values = printOptions_->defaultValuesByKey();

  if (normalized == QStringLiteral("printer"))
  {
    for (auto it = printerPresetValues_.cbegin(); it != printerPresetValues_.cend(); ++it)
      values.insert(it.key(), it.value());
    return values;
  }

  values = effectivePresetValuesForTier(QStringLiteral("printer"));
  if (normalized == QStringLiteral("filament"))
  {
    for (auto it = filamentPresetValues_.cbegin(); it != filamentPresetValues_.cend(); ++it)
      values.insert(it.key(), it.value());
    return values;
  }

  values = effectivePresetValuesForTier(QStringLiteral("filament"));
  for (auto it = printPresetValues_.cbegin(); it != printPresetValues_.cend(); ++it)
    values.insert(it.key(), it.value());
  return values;
}

void ConfigViewModel::setCurrentPresetTierValue(const QString &tier, const QString &presetName)
{
  const QString normalized = normalizedTier(tier);
  if (normalized == QStringLiteral("printer"))
  {
    if (presetService_ && !presetService_->setSelectedPresetForCategory(PresetServiceMock::PrinterCat, presetName))
      return;
    currentPrinterPreset_ = presetName;
    if (presetService_) {
      if (!presetService_->isPresetCompatibleWithPrinter(PresetServiceMock::FilamentCat,
                                                         currentFilamentPreset_,
                                                         currentPrinterPreset_)) {
        const QString compatibleFilament =
            presetService_->findCompatiblePresetForCategory(PresetServiceMock::FilamentCat,
                                                            currentPrinterPreset_);
        if (!compatibleFilament.isEmpty()) {
          currentFilamentPreset_ = compatibleFilament;
          presetService_->setSelectedPresetForCategory(PresetServiceMock::FilamentCat, compatibleFilament);
          filamentPresetValues_ = presetService_->presetValues(currentFilamentPreset_);
        }
      }

      if (!presetService_->isPresetCompatibleWithPrinter(PresetServiceMock::PrintCat,
                                                         currentPrintPreset_,
                                                         currentPrinterPreset_)) {
        const QString compatiblePrint =
            presetService_->findCompatiblePresetForCategory(PresetServiceMock::PrintCat,
                                                            currentPrinterPreset_);
        QString nextPrint = compatiblePrint;
        if (nextPrint.isEmpty())
          nextPrint = presetService_->defaultPresetForCategory(PresetServiceMock::PrintCat);
        if (!nextPrint.isEmpty() && nextPrint != currentPrintPreset_) {
          currentPrintPreset_ = nextPrint;
          currentPreset_ = nextPrint;
          presetService_->setSelectedPresetForCategory(PresetServiceMock::PrintCat, nextPrint);
          printPresetValues_ = presetService_->presetValues(currentPrintPreset_);
        }
      }
    }
    printerPresetValues_ = presetService_ ? presetService_->presetValues(currentPrinterPreset_)
                                          : QHash<QString, QVariant>{};
    return;
  }

  if (normalized == QStringLiteral("filament"))
  {
    if (presetService_ && !presetService_->setSelectedPresetForCategory(PresetServiceMock::FilamentCat, presetName))
      return;
    currentFilamentPreset_ = presetName;
    filamentPresetValues_ = presetService_ ? presetService_->presetValues(currentFilamentPreset_)
                                           : QHash<QString, QVariant>{};
    return;
  }

  if (presetService_ && !presetService_->setSelectedPresetForCategory(PresetServiceMock::PrintCat, presetName))
    return;
  currentPrintPreset_ = presetName;
  currentPreset_ = presetName;
  printPresetValues_ = presetService_ ? presetService_->presetValues(currentPrintPreset_)
                                      : QHash<QString, QVariant>{};
}

void ConfigViewModel::updateMergedPresetValues()
{
  globalOptionValues_ = effectivePresetValuesForTier(QStringLiteral("print"));

  valueSources_.clear();
  const auto defaults = printOptions_ ? printOptions_->defaultValuesByKey() : QHash<QString, QVariant>{};
  for (auto it = globalOptionValues_.cbegin(); it != globalOptionValues_.cend(); ++it)
    valueSources_.insert(it.key(), QStringLiteral("default"));

  for (auto it = printerPresetValues_.cbegin(); it != printerPresetValues_.cend(); ++it)
    valueSources_.insert(it.key(), QStringLiteral("printer"));
  for (auto it = filamentPresetValues_.cbegin(); it != filamentPresetValues_.cend(); ++it)
    valueSources_.insert(it.key(), QStringLiteral("filament"));
  for (auto it = printPresetValues_.cbegin(); it != printPresetValues_.cend(); ++it)
    valueSources_.insert(it.key(), QStringLiteral("print"));

  for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
  {
    if (!globalOptionValues_.contains(it.key()))
      globalOptionValues_.insert(it.key(), it.value());
    if (!valueSources_.contains(it.key()))
      valueSources_.insert(it.key(), QStringLiteral("default"));
  }
}

void ConfigViewModel::refreshOptionModelReferences()
{
  if (printOptions_) {
    printOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("print")));
    printOptions_->applyValues(buildScopeValues());
  }
  if (filamentOptions_) {
    filamentOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("filament")));
    filamentOptions_->applyValues(buildScopeValues());
  }
  if (machineOptions_) {
    machineOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("printer")));
    machineOptions_->applyValues(buildScopeValues());
  }
}

bool ConfigViewModel::queuePendingAction(const QString &action, const QString &target)
{
  if (!isPresetDirty()) {
    pendingUnsavedAction_.clear();
    pendingUnsavedTarget_.clear();
    return true;
  }

  pendingUnsavedAction_ = action;
  pendingUnsavedTarget_ = target;
  emit stateChanged();
  emit pendingUnsavedChangesRequested();
  return false;
}

void ConfigViewModel::clearPendingAction()
{
  if (pendingUnsavedAction_.isEmpty() && pendingUnsavedTarget_.isEmpty())
    return;
  pendingUnsavedAction_.clear();
  pendingUnsavedTarget_.clear();
  emit stateChanged();
  emit pendingActionCleared();
}

bool ConfigViewModel::applyPendingAction()
{
  if (pendingUnsavedAction_.isEmpty())
    return true;

  const QString action = pendingUnsavedAction_;
  const QString target = pendingUnsavedTarget_;
  pendingUnsavedAction_.clear();
  pendingUnsavedTarget_.clear();
  emit stateChanged();
  emit pendingActionApplied(action, target);

  if (action == QStringLiteral("switch-print-preset")) {
    setCurrentPrintPreset(target);
    return true;
  }
  if (action == QStringLiteral("switch-filament-preset")) {
    setCurrentFilamentPreset(target);
    return true;
  }
  if (action == QStringLiteral("switch-printer-preset")) {
    setCurrentPrinterPreset(target);
    return true;
  }
  if (action == QStringLiteral("scope-global")) {
    activateGlobalScope();
    return true;
  }
  if (action == QStringLiteral("scope-plate")) {
    activatePlateScope(target.toInt());
    return true;
  }
  if (action.startsWith(QStringLiteral("scope-object:"))) {
    const QStringList parts = action.split(QLatin1Char(':'));
    if (parts.size() == 5)
      activateObjectScope(parts[1], parts[2], parts[3].toInt(), parts[4].toInt());
    return true;
  }
  return true;
}

void ConfigViewModel::setActivePresetTier(const QString &tier)
{
  const QString normalized = normalizedTier(tier);
  if (activePresetTier_ == normalized)
    return;
  activePresetTier_ = normalized;
  refreshOptionModelReferences();
  emit stateChanged();
}

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
    currentPrinterPreset_ = presetService_->selectedPresetForCategory(PresetServiceMock::PrinterCat);
    currentFilamentPreset_ = presetService_->selectedPresetForCategory(PresetServiceMock::FilamentCat);
    currentPrintPreset_ = presetService_->selectedPresetForCategory(PresetServiceMock::PrintCat);
    currentPreset_ = currentPrintPreset_;
    layerHeight_ = presetService_->defaultLayerHeight();
    printerPresetValues_ = presetService_->presetValues(currentPrinterPreset_);
    filamentPresetValues_ = presetService_->presetValues(currentFilamentPreset_);
    printPresetValues_ = presetService_->presetValues(currentPrintPreset_);
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
  if (presetService_) {
    mergePresetHierarchy();
    emit stateChanged();
    return;
  }
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::setCurrentPreset(const QString &presetName)
{
  if (!presetService_)
  {
    currentPreset_ = presetName;
    emit stateChanged();
    emit sliceAffectingConfigChanged();
    return;
  }

  if (presetService_->presetCategory(presetName) != PresetServiceMock::PrintCat)
    return;
  if (!presetService_->setSelectedPresetForCategory(PresetServiceMock::PrintCat, presetName))
    return;

  setCurrentPresetTierValue(QStringLiteral("print"), presetName);
  settingsScope_ = QStringLiteral("global");
  settingsTargetObjectIndex_ = -1;
  settingsTargetVolumeIndex_ = -1;
  mergePresetHierarchy();
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}

// v2.4 IO: preset bundle import/export.
bool ConfigViewModel::exportBundle(const QString &filePath) const
{
    if (!presetService_) return false;
    return presetService_->exportBundle(filePath);
}

bool ConfigViewModel::importBundle(const QString &filePath)
{
    if (!presetService_) return false;
    const bool ok = presetService_->importBundle(filePath);
    if (ok) {
      if (presetList_)
        presetList_->refreshFromService(presetService_);
      emit stateChanged();
    }
    return ok;
}

void ConfigViewModel::saveCurrentPreset()
{
  if (!presetService_)
    return;

  const QString tier = normalizedTier(activePresetTier_);
  QString targetPreset;
  if (tier == QStringLiteral("printer")) {
    targetPreset = currentPrinterPreset_;
  } else if (tier == QStringLiteral("filament")) {
    targetPreset = currentFilamentPreset_;
  } else {
    targetPreset = currentPrintPreset_.isEmpty() ? currentPreset_ : currentPrintPreset_;
  }

  if (targetPreset.isEmpty())
    return;

  const QHash<QString, QVariant> tierValues = editableValuesForTier(tier);

  if (!presetService_->savePresetValues(targetPreset, tierValues))
    return;

  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = tierValues;
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = tierValues;
  else
    printPresetValues_ = tierValues;
  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
}

bool ConfigViewModel::isPresetDirty() const
{
  const QString tier = normalizedTier(activePresetTier_);
  return effectivePresetValuesForTier(tier) != persistedEffectiveValuesForTier(tier);
}

// 鈹€鈹€ 3-tier preset inheritance (瀵归綈涓婃父 PresetBundle) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

QStringList ConfigViewModel::printerPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(PresetServiceMock::PrinterCat) : QStringList{};
}

QStringList ConfigViewModel::filamentPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(PresetServiceMock::FilamentCat) : QStringList{};
}

QStringList ConfigViewModel::printPresetNames() const
{
  return presetService_ ? presetService_->presetNamesForCategory(PresetServiceMock::PrintCat) : QStringList{};
}

QStringList ConfigViewModel::compatibleFilamentPresetNames() const
{
  if (!presetService_)
    return {};
  QStringList names =
      presetService_->compatiblePresetNamesForCategory(PresetServiceMock::FilamentCat, currentPrinterPreset_);
  if (!currentFilamentPreset_.isEmpty() && !names.contains(currentFilamentPreset_))
    names.prepend(currentFilamentPreset_);
  return names;
}

QStringList ConfigViewModel::compatiblePrintPresetNames() const
{
  if (!presetService_)
    return {};
  QStringList names =
      presetService_->compatiblePresetNamesForCategory(PresetServiceMock::PrintCat, currentPrinterPreset_);
  if (!currentPrintPreset_.isEmpty() && !names.contains(currentPrintPreset_))
    names.prepend(currentPrintPreset_);
  return names;
}

bool ConfigViewModel::currentPresetCombinationValid() const
{
  return !presetService_ ||
      presetService_->isCurrentSelectionCompatible(currentPrinterPreset_, currentFilamentPreset_, currentPrintPreset_);
}

QString ConfigViewModel::currentPresetCompatibilityMessage() const
{
  return presetService_ ?
      presetService_->currentSelectionCompatibilityMessage(currentPrinterPreset_, currentFilamentPreset_, currentPrintPreset_) :
      QString();
}

void ConfigViewModel::setCurrentPrinterPreset(const QString &name)
{
  const QString beforePrinter = currentPrinterPreset_;
  const QString beforeFilament = currentFilamentPreset_;
  const QString beforePrint = currentPrintPreset_;
  setCurrentPresetTierValue(QStringLiteral("printer"), name);
  mergePresetHierarchy();
  emit stateChanged();
  if (beforePrinter != currentPrinterPreset_
      || beforeFilament != currentFilamentPreset_
      || beforePrint != currentPrintPreset_)
    emit sliceAffectingConfigChanged();
}

void ConfigViewModel::setCurrentFilamentPreset(const QString &name)
{
  const QString before = currentFilamentPreset_;
  setCurrentPresetTierValue(QStringLiteral("filament"), name);
  mergePresetHierarchy();
  emit stateChanged();
  if (before != currentFilamentPreset_)
    emit sliceAffectingConfigChanged();
}

void ConfigViewModel::setCurrentPrintPreset(const QString &name)
{
  const QString before = currentPrintPreset_;
  setCurrentPresetTierValue(QStringLiteral("print"), name);
  mergePresetHierarchy();
  emit stateChanged();
  if (before != currentPrintPreset_)
    emit sliceAffectingConfigChanged();
}

void ConfigViewModel::mergePresetHierarchy()
{
  updateMergedPresetValues();
  applyScopeValues();
}

bool ConfigViewModel::createCustomPreset(int category, const QString &name)
{
  if (!presetService_)
    return false;

  QString tier;
  if (category == PresetServiceMock::PrinterCat)
    tier = QStringLiteral("printer");
  else if (category == PresetServiceMock::FilamentCat)
    tier = QStringLiteral("filament");
  else if (category == PresetServiceMock::PrintCat)
    tier = QStringLiteral("print");
  else
    return false;

  const QHash<QString, QVariant> tierValues = editableValuesForTier(tier);

  const QString trimmedName = name.trimmed();
  if (!presetService_->createCustomPreset(category, trimmedName, tierValues))
    return false;

  setCurrentPresetTierValue(tier, trimmedName);

  if (presetList_)
    presetList_->refreshFromService(presetService_);
  mergePresetHierarchy();
  emit stateChanged();
  if (hasPendingUnsavedChanges())
    return applyPendingAction();
  return true;
}

bool ConfigViewModel::deletePreset(int category, const QString &name)
{
  if (!presetService_)
    return false;
  if (presetService_->presetCategory(name) != category)
    return false;

  // 涓嶅厑璁稿垹闄ゅ綋鍓嶆鍦ㄤ娇鐢ㄧ殑棰勮
  if (name == currentPrinterPreset_ || name == currentFilamentPreset_ || name == currentPrintPreset_)
    return false;

  bool ok = presetService_->deletePreset(name);
  if (ok)
  {
    // 濡傛灉鍒犻櫎鐨勬槸褰撳墠绫诲埆涓鍦ㄤ娇鐢ㄧ殑棰勮锛屽垏鎹㈠洖榛樿
    if (name == currentPrintPreset_)
      setCurrentPrintPreset(presetService_->defaultPresetForCategory(PresetServiceMock::PrintCat));
    if (name == currentFilamentPreset_)
      setCurrentFilamentPreset(presetService_->defaultPresetForCategory(PresetServiceMock::FilamentCat));
    if (name == currentPrinterPreset_)
      setCurrentPrinterPreset(presetService_->defaultPresetForCategory(PresetServiceMock::PrinterCat));
    if (presetList_)
      presetList_->refreshFromService(presetService_);
    emit stateChanged();
  }
  return ok;
}

bool ConfigViewModel::renamePreset(int category, const QString &oldName, const QString &newName)
{
  if (!presetService_ || newName.trimmed().isEmpty())
    return false;
  if (presetService_->presetCategory(oldName) != category)
    return false;

  bool ok = presetService_->renamePreset(oldName, newName.trimmed());
  if (ok)
  {
    // 鏇存柊褰撳墠娲昏穬棰勮鍚嶅紩鐢?
    if (oldName == currentPrintPreset_)
      currentPrintPreset_ = newName.trimmed();
    if (oldName == currentPreset_)
      currentPreset_ = newName.trimmed();
    if (oldName == currentFilamentPreset_)
      currentFilamentPreset_ = newName.trimmed();
    if (oldName == currentPrinterPreset_)
      currentPrinterPreset_ = newName.trimmed();
    if (presetList_)
      presetList_->refreshFromService(presetService_);
    emit stateChanged();
  }
  return ok;
}

bool ConfigViewModel::canDeletePreset(const QString &name) const
{
  return presetService_ && presetService_->isUserPreset(name);
}

QStringList ConfigViewModel::comparePresets(const QString &presetA, const QString &presetB) const
{
  QStringList diffs;
  if (!presetService_)
    return diffs;

  auto valsA = presetService_->presetValues(presetA);
  auto valsB = presetService_->presetValues(presetB);

  QSet<QString> allKeys;
  for (auto it = valsA.constBegin(); it != valsA.constEnd(); ++it) allKeys.insert(it.key());
  for (auto it = valsB.constBegin(); it != valsB.constEnd(); ++it) allKeys.insert(it.key());

  for (const auto &key : allKeys) {
    QVariant valA = valsA.value(key);
    QVariant valB = valsB.value(key);
    if (valA != valB)
      diffs.append(QStringLiteral("%1: %2 鈫?%3").arg(key, valA.toString(), valB.toString()));
  }

  return diffs;
}

void ConfigViewModel::autoMatchFilament()
{
  // 瀵归綈涓婃父 PresetBundle::update_compatible
  // 鍒囨崲鎵撳嵃鏈哄悗锛岃嚜鍔ㄩ€夋嫨鍏煎鐨勮€楁潗棰勮锛堝熀浜?nozzle diameter / max_temp 鍖归厤锛?
  if (!presetService_)
    return;

  const QString compatible = presetService_->findCompatibleFilament(currentPrinterPreset_);
  if (!compatible.isEmpty() && compatible != currentFilamentPreset_)
  {
    currentFilamentPreset_ = compatible;
    presetService_->setSelectedPresetForCategory(PresetServiceMock::FilamentCat, compatible);
    mergePresetHierarchy();
  }
  emit stateChanged();
}

bool ConfigViewModel::isCurrentFilamentCompatible() const
{
  if (!presetService_)
    return true;
  return presetService_->isPresetCompatibleWithPrinter(PresetServiceMock::FilamentCat,
                                                       currentFilamentPreset_,
                                                       currentPrinterPreset_);
}

bool ConfigViewModel::isFilamentCompatible(const QString &filamentName) const
{
  if (!presetService_)
    return true;
  return presetService_->isPresetCompatibleWithPrinter(PresetServiceMock::FilamentCat,
                                                       filamentName,
                                                       currentPrinterPreset_);
}

bool ConfigViewModel::canUseCurrentPresetCombination() const
{
  return currentPresetCombinationValid();
}

QString ConfigViewModel::presetActionBlocker(int category, const QString &presetName, const QString &action) const
{
  return presetService_ ? presetService_->presetActionBlocker(category, presetName, action) : QString();
}

void ConfigViewModel::setLayerHeight(double v)
{
  if (qFuzzyCompare(layerHeight_, v))
    return;
  layerHeight_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setPrintSpeed(int v)
{
  if (printSpeed_ == v)
    return;
  printSpeed_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setSupportEnabled(bool v)
{
  if (supportEnabled_ == v)
    return;
  supportEnabled_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setInfillDensity(int v)
{
  if (infillDensity_ == v)
    return;
  infillDensity_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setNozzleTemp(int v)
{
  if (nozzleTemp_ == v)
    return;
  nozzleTemp_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setBedTemp(int v)
{
  if (bedTemp_ == v)
    return;
  bedTemp_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setWallCount(int v)
{
  if (wallCount_ == v)
    return;
  wallCount_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
}
void ConfigViewModel::setEnableBrim(bool v)
{
  if (enableBrim_ == v)
    return;
  enableBrim_ = v;
  emit stateChanged();
  emit sliceAffectingConfigChanged();
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
  // Distinguish volume scope from object scope (瀵归綈涓婃父 Tab scope semantics)
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
  settingsTargetName_ = tr("骞虫澘 %1").arg(plateIndex + 1);
  applyScopeValues();
  emit stateChanged();
}

bool ConfigViewModel::requestCurrentPrinterPreset(const QString &name)
{
  if (queuePendingAction(QStringLiteral("switch-printer-preset"), name))
  {
    setCurrentPrinterPreset(name);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestCurrentFilamentPreset(const QString &name)
{
  if (queuePendingAction(QStringLiteral("switch-filament-preset"), name))
  {
    setCurrentFilamentPreset(name);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestCurrentPrintPreset(const QString &name)
{
  if (queuePendingAction(QStringLiteral("switch-print-preset"), name))
  {
    setCurrentPrintPreset(name);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestGlobalScope()
{
  if (queuePendingAction(QStringLiteral("scope-global"), QString()))
  {
    activateGlobalScope();
    return true;
  }
  return false;
}

bool ConfigViewModel::requestObjectScope(const QString &targetType, const QString &targetName, int objectIndex, int volumeIndex)
{
  const QString action = QStringLiteral("scope-object:%1:%2:%3:%4")
      .arg(targetType, targetName)
      .arg(objectIndex)
      .arg(volumeIndex);
  if (queuePendingAction(action, QString()))
  {
    activateObjectScope(targetType, targetName, objectIndex, volumeIndex);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestPlateScope(int plateIndex)
{
  if (queuePendingAction(QStringLiteral("scope-plate"), QString::number(plateIndex)))
  {
    activatePlateScope(plateIndex);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestSavePendingChanges()
{
  const QString tier = normalizedTier(activePresetTier_);
  const QString currentPresetName =
      tier == QStringLiteral("printer") ? currentPrinterPreset_ :
      tier == QStringLiteral("filament") ? currentFilamentPreset_ :
      currentPrintPreset_;
  if (presetService_ && presetService_->isReadOnlyPreset(currentPresetName)) {
    emit saveAsRequired();
    return false;
  }

  saveCurrentPreset();
  return applyPendingAction();
}

bool ConfigViewModel::requestDiscardPendingChanges()
{
  const bool wasDirty = isPresetDirty();
  const QString tier = normalizedTier(activePresetTier_);
  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = selectedPresetValuesForTier(tier);
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = selectedPresetValuesForTier(tier);
  else
    printPresetValues_ = selectedPresetValuesForTier(tier);

  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
  if (wasDirty)
    emit sliceAffectingConfigChanged();
  return applyPendingAction();
}

bool ConfigViewModel::requestCancelPendingChanges()
{
  clearPendingAction();
  return true;
}

void ConfigViewModel::applyScopeValues()
{
  if (!printOptions_)
    return;

  applyingScopeValues_ = true;
  const auto values = buildScopeValues();
  printOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("print")));
  printOptions_->applyValues(values);
  if (machineOptions_) {
    machineOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("printer")));
    machineOptions_->applyValues(machineOptions_->valuesByKey().isEmpty() ? values : effectivePresetValuesForTier(QStringLiteral("printer")));
  }
  if (filamentOptions_) {
    filamentOptions_->setReferenceValues(referenceValuesForTier(QStringLiteral("filament")));
    filamentOptions_->applyValues(filamentOptions_->valuesByKey().isEmpty() ? values : effectivePresetValuesForTier(QStringLiteral("filament")));
  }
  printOptions_->setReadonlyKeys(readonlyKeysForCurrentScope());
  applyingScopeValues_ = false;
}

void ConfigViewModel::handleOptionValueChanged(const QString &key, const QVariant &value)
{
  if (applyingScopeValues_)
    return;

  if (settingsScope_ == QStringLiteral("global") || (settingsScope_ != QStringLiteral("plate") && settingsTargetObjectIndex_ < 0))
  {
    QString tier = activePresetTier_.isEmpty() ? QStringLiteral("print") : activePresetTier_;
    if (sender() == machineOptions_)
      tier = QStringLiteral("printer");
    else if (sender() == filamentOptions_)
      tier = QStringLiteral("filament");
    else if (sender() == printOptions_)
      tier = QStringLiteral("print");
    if (tier == QStringLiteral("printer"))
      printerPresetValues_.insert(key, value);
    else if (tier == QStringLiteral("filament"))
      filamentPresetValues_.insert(key, value);
    else
      printPresetValues_.insert(key, value);
    updateMergedPresetValues();
    applyScopeValues();
    emit stateChanged();
    emit sliceAffectingConfigChanged();
    return;
  }

  // Plate scope: route to plate-scoped storage
  if (settingsScope_ == QStringLiteral("plate") && settingsTargetPlateIndex_ >= 0)
  {
    if (!projectService_ || !projectService_->setPlateScopedOptionValue(settingsTargetPlateIndex_, key, value))
      return;
    applyScopeValues();
    emit stateChanged();
    emit sliceAffectingConfigChanged();
    return;
  }

  // Object/volume scope
  if (!scopedWritableKeys_.contains(key))
    return;

  if (!projectService_ || !projectService_->setScopedOptionValue(settingsTargetObjectIndex_, settingsTargetVolumeIndex_, key, value))
    return;

  applyScopeValues();
  emit stateChanged();
  emit sliceAffectingConfigChanged();
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

// Fuzzy matching helper based on upstream OptionsSearcher / fts_fuzzy_match.
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
        int bonus = 15; // sequential bonus (瀵归綈涓婃父 sequential bonus)
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
  return score >= 60; // minimum threshold (瀵归綈涓婃父 minimum score)
}

} // anonymous namespace

QList<int> ConfigViewModel::filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode) const
{
  // Dispatch to the correct option model via the category/tier parameter.
  // Accepts new tier strings ("printer"/"filament"/"print") and legacy
  // aliases ("machine"/"process") for backward compatibility.
  ConfigOptionModel *model = optionModelForTier(category);
  if (!model)
    return {};

  const int n = model->rowCount();
  QList<int> result;
  result.reserve(n);

  const bool matchAll = category.isEmpty() || category == QStringLiteral("all");
  const QString needle = searchText.toLower();
  const bool useFuzzy = needle.length() >= 2;

  for (int i = 0; i < n; ++i)
  {
    if (!needle.isEmpty())
    {
      bool matched = false;
      if (useFuzzy)
      {
        int score = 0;
        matched = fuzzyMatch(needle, model->optLabel(i).toLower(), score);
        if (!matched)
          matched = fuzzyMatch(needle, model->optKey(i).toLower(), score);
      }
      else
      {
        matched = model->optLabel(i).toLower().contains(needle) ||
              model->optKey(i).toLower().contains(needle);
      }
      if (!matched)
        continue;
    }

    // Mode filter (upstream ConfigOptionMode: 0=comSimple, 1=comAdvanced, 2=comDevelop).
    // Simple mode (advancedMode=false) shows only comSimple options; advanced mode
    // (advancedMode=true) is a SUPERSET and shows comSimple + comAdvanced + comDevelop.
    // Never exclude a simple-mode option from advanced mode.
    const int optMode = model->optMode(i);
    if (optMode >= 1 && !advancedMode)
      continue; // Advanced/Develop-only options hidden in simple mode

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
  // Use "鑰楁潗" to match PresetListModel's category (original QML used "鑰楁潗涓? which was a bug)
  const int globalIdx = presetList_->globalIndex(tr("鑰楁潗"), localIndex);
  return globalIdx >= 0 ? presetList_->presetName(globalIdx) : QString{};
}

// 鈹€鈹€ Layer range support (瀵归綈涓婃父 ModelObject::layer_config_ranges) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

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

// 鈹€鈹€ Enhanced search (瀵归綈涓婃父 OptionsSearcher + fts_fuzzy_match) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

namespace {

/// 瀵归綈涓婃父 fts_fuzzy_match锛氳交閲忕骇妯＄硦鍖归厤绠楁硶
/// 杩斿洖鍖归厤鍒嗘暟锛?=鏃犲尮閰嶏紝姝ｅ€艰秺楂樺尮閰嶅害瓒婂ソ锛夛紝鍚屾椂杈撳嚭鏄惁鍏ㄩ儴瀛楃杩炵画
/// 绠楁硶锛氳椽蹇冨瓧绗﹀尮閰?+ 婊戝姩绐楀彛杩炵画鍖归厤 + 鍓嶇紑/杈圭晫濂栧姳
int fuzzyMatch(const QString &pattern, const QString &text, bool *outAllConsecutive = nullptr)
{
  if (pattern.isEmpty() || text.isEmpty())
    return 0;

  const QChar *pat = pattern.unicode();
  const QChar *str = text.unicode();
  const int patLen = pattern.length();
  const int strLen = text.length();

  // 璐績鍖归厤锛氬厑璁稿瓧绗﹂棿璺宠繃锛坓ap锛夛紝缁熻鍖归厤寰楀垎
  int score = 0;
  int patIdx = 0;
  int consecutiveCount = 0;
  int lastMatchPos = -2;
  bool allConsecutive = true;

  for (int i = 0; i < strLen && patIdx < patLen; ++i) {
    if (str[i].toLower() == pat[patIdx].toLower()) {
      // 杩炵画鍖归厤濂栧姳锛堝榻愪笂娓?sequential bonus锛?
      if (lastMatchPos == i - 1) {
        consecutiveCount++;
        score += 15 + consecutiveCount * 2;
      } else {
        consecutiveCount = 1;
        // 闈炶繛缁尮閰嶆椂閲嶇疆 allConsecutive 鏍囧織
        if (lastMatchPos >= 0)
          allConsecutive = false;
        score += 10;
      }
      // 鍓嶇紑鍖归厤棰濆濂栧姳
      if (patIdx == 0)
        score += 5;
      // 鍗曡瘝杈圭晫鍖归厤濂栧姳锛堥瀛楁瘝鎴栧墠涓€涓瓧绗︽槸鍒嗛殧绗︼級
      if (i == 0 || (str[i - 1] == '_' || str[i - 1] == ' ' || str[i - 1] == '-'))
        score += 10;
      // 鍏ㄩ儴瀛楃澶у啓鍖归厤鏃堕澶栧鍔憋紙缂╁啓鍖归厤锛?
      if (pat[patIdx].isUpper())
        score += 5;

      lastMatchPos = i;
      patIdx++;
    }
  }

  if (patIdx < patLen) {
    // 妯″紡鏈畬鍏ㄥ尮閰?
    if (outAllConsecutive) *outAllConsecutive = false;
    return 0;
  }

  // 鎯╃綒璺宠繃鐨勫瓧绗︽暟閲忥紙瀵归綈涓婃父 gap penalty锛?
  int gaps = strLen - (lastMatchPos - patIdx + 1 + (patLen - consecutiveCount));
  score -= gaps;

  if (outAllConsecutive)
    *outAllConsecutive = allConsecutive;

  return score;
}

/// Returns the best fuzzy score across searchable fields.
int bestFuzzyScore(const QString &needle, const QStringList &fields)
{
  int best = 0;
  for (const auto &field : fields) {
    // Direct substring match is strongest.
    if (field.toLower().contains(needle))
      return 1000 + needle.length() * 10;

    // Fuzzy subsequence match.
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
  // 瀵归綈涓婃父 OptionsSearcher锛歴core > 闃堝€兼墠杩斿洖
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

  // 鎸夊垎鏁伴檷搴忔帓搴忥紙瀵归綈涓婃父锛氶珮鍒嗕紭鍏堬紝鍚屽垎鎸夊瓧姣嶅簭锛?
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
  // Return a JSON value chain for default/printer/filament/print levels.
  // Mirrors upstream PresetBundle value-at-level diagnostics.
  if (!printOptions_ || !presetService_)
    return QStringLiteral("{\"default\":\"\"}");

  QVariant defVal = printOptions_->defaultValuesByKey().value(key);
  QVariant printerVal = printerPresetValues_.value(key, presetService_->presetValue(currentPrinterPreset_, key));
  QVariant filamentVal = filamentPresetValues_.value(key, presetService_->presetValue(currentFilamentPreset_, key));
  QVariant printVal = printPresetValues_.value(key, presetService_->presetValue(currentPrintPreset_, key));
  QVariant currentVal = globalOptionValues_.value(key, defVal);

  // 鏋勫缓鏈€缁?JSON
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
  // 瀵归綈涓婃父 Tab reset_to_level
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

  const QString tier = normalizedTier(activePresetTier_);
  if (tier == QStringLiteral("printer"))
    printerPresetValues_.insert(key, targetVal);
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_.insert(key, targetVal);
  else
    printPresetValues_.insert(key, targetVal);
  updateMergedPresetValues();
  applyScopeValues();

  // 鏇存柊鏉ユ簮灞傜骇
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

// 鈹€鈹€ Scope difference (瀵归綈涓婃父 Tab::is_modified_value per-scope diff) 鈹€鈹€

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
  if (ok) {
    applyScopeValues();
    emit stateChanged();
  }
  return ok;
}

void ConfigViewModel::resetAllScopeOverrides()
{
  if (!projectService_) return;
  int count = scopeOverrideCount();
  // Collect keys first since resetting changes the count
  QStringList keys;
  for (int i = 0; i < count; ++i)
    keys.append(scopeOverriddenKey(i));
  for (const auto &key : keys)
    resetScopeOverride(key);
}

// 鈹€鈹€ Global modified options (瀵归綈涓婃父 Tab::modified_options) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€

int ConfigViewModel::globalModifiedCount() const
{
  const QString tier = normalizedTier(activePresetTier_);
  const auto effective = effectivePresetValuesForTier(tier);
  const auto reference = referenceValuesForTier(tier);
  int count = 0;
  for (auto it = effective.cbegin(); it != effective.cend(); ++it)
  {
    if (reference.value(it.key()) != it.value())
      ++count;
  }
  return count;
}

QHash<QString, QVariant> ConfigViewModel::mergedConfigValues() const
{
  return globalOptionValues_;
}

void ConfigViewModel::applyProjectConfig(const QHash<QString, QVariant> &config)
{
  if (config.isEmpty()) return;

  // Try to match printer/filament/print presets from loaded config
  // Upstream maps: printer_preset_id 鈫?PresetBundle printer, etc.
  const auto printerIt = config.find(QStringLiteral("printer_preset_id"));
  if (printerIt != config.end() && !printerIt.value().toString().isEmpty())
  {
    const QString name = printerIt.value().toString();
    if (presetService_ && presetService_->hasPreset(name))
      setCurrentPrinterPreset(name);
  }

  const auto filamentIt = config.find(QStringLiteral("filament_preset_id"));
  if (filamentIt != config.end() && !filamentIt.value().toString().isEmpty())
  {
    const QString name = filamentIt.value().toString();
    if (presetService_ && presetService_->hasPreset(name))
      setCurrentFilamentPreset(name);
  }

  const auto printIt = config.find(QStringLiteral("print_preset_id"));
  if (printIt != config.end() && !printIt.value().toString().isEmpty())
  {
    const QString name = printIt.value().toString();
    if (presetService_ && presetService_->hasPreset(name))
      setCurrentPrintPreset(name);
  }

  // Apply remaining config keys into the editable print-tier state.
  for (auto it = config.constBegin(); it != config.constEnd(); ++it)
  {
    const QString &key = it.key();
    // Skip meta keys that aren't real config options
    if (key == QStringLiteral("printer_preset_id") ||
        key == QStringLiteral("filament_preset_id") ||
        key == QStringLiteral("print_preset_id") ||
        key == QStringLiteral("print_sequence") ||
        key == QStringLiteral("total_filament_names"))
      continue;
    printPresetValues_[key] = it.value();
  }

  updateMergedPresetValues();
  applyScopeValues();

  // Sync individual Q_PROPERTY values from merged config
  if (globalOptionValues_.contains(QStringLiteral("layer_height")))
    layerHeight_ = globalOptionValues_.value(QStringLiteral("layer_height")).toDouble();
  if (globalOptionValues_.contains(QStringLiteral("speed_print")))
    printSpeed_ = globalOptionValues_.value(QStringLiteral("speed_print")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("support_material")))
    supportEnabled_ = globalOptionValues_.value(QStringLiteral("support_material")).toBool();
  if (globalOptionValues_.contains(QStringLiteral("infill_density")))
    infillDensity_ = globalOptionValues_.value(QStringLiteral("infill_density")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("temperature")))
    nozzleTemp_ = globalOptionValues_.value(QStringLiteral("temperature")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("bed_temperature")))
    bedTemp_ = globalOptionValues_.value(QStringLiteral("bed_temperature")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("wall_filament")))
    wallCount_ = globalOptionValues_.value(QStringLiteral("wall_filament")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("top_solid_layers")))
    topLayers_ = globalOptionValues_.value(QStringLiteral("top_solid_layers")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("bottom_solid_layers")))
    bottomLayers_ = globalOptionValues_.value(QStringLiteral("bottom_solid_layers")).toInt();
  if (globalOptionValues_.contains(QStringLiteral("brim_width")))
    enableBrim_ = globalOptionValues_.value(QStringLiteral("brim_width")).toDouble() > 0;

  emit stateChanged();
}

QString ConfigViewModel::globalModifiedKey(int index) const
{
  if (index < 0)
    return {};
  const QString tier = normalizedTier(activePresetTier_);
  const auto effective = effectivePresetValuesForTier(tier);
  const auto reference = referenceValuesForTier(tier);
  int pos = 0;
  for (auto it = effective.cbegin(); it != effective.cend(); ++it)
  {
    if (reference.value(it.key()) != it.value())
    {
      if (pos == index)
        return it.key();
      ++pos;
    }
  }
  return {};
}

QString ConfigViewModel::globalModifiedCurrentValue(const QString &key) const
{
  return effectivePresetValuesForTier(activePresetTier_).value(key).toString();
}

QString ConfigViewModel::globalModifiedDefaultValue(const QString &key) const
{
  return referenceValuesForTier(activePresetTier_).value(key).toString();
}

bool ConfigViewModel::resetGlobalOption(const QString &key)
{
  const QString tier = normalizedTier(activePresetTier_);
  const QVariant before = effectivePresetValuesForTier(tier).value(key);
  const auto presetValues = selectedPresetValuesForTier(tier);
  const auto fallbackValues = referenceValuesForTier(tier);
  if (!presetValues.contains(key) && !fallbackValues.contains(key))
    return false;
  const QVariant target = presetValues.contains(key) ? presetValues.value(key) : fallbackValues.value(key);
  if (tier == QStringLiteral("printer"))
    printerPresetValues_.insert(key, target);
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_.insert(key, target);
  else
    printPresetValues_.insert(key, target);
  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
  if (before != target)
    emit sliceAffectingConfigChanged();
  return true;
}

void ConfigViewModel::resetAllGlobalOptions()
{
  const QString tier = normalizedTier(activePresetTier_);
  const auto before = effectivePresetValuesForTier(tier);
  const auto presetValues = selectedPresetValuesForTier(tier);
  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = presetValues;
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = presetValues;
  else
    printPresetValues_ = presetValues;
  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
  if (before != effectivePresetValuesForTier(tier))
    emit sliceAffectingConfigChanged();
}

// SETTINGS-05 reset-group: reset all options in a named group to reference values.
// Per upstream Tab.cpp reset_group behavior.
void ConfigViewModel::resetGroup(const QString &tier, const QString &groupName)
{
  ConfigOptionModel *model = optionModelForTier(tier);
  if (!model)
    return;

  bool anyReset = false;
  for (int i = 0; i < model->rowCount(); ++i)
  {
    if (model->optGroup(i) == groupName)
    {
      model->resetOption(i);
      anyReset = true;
    }
  }
  if (anyReset)
  {
    emit stateChanged();
    emit sliceAffectingConfigChanged();
  }
}

// Per-option nullable flag proxy — delegates to optionModelForTier.
bool ConfigViewModel::optNullable(const QString &tier, int index) const
{
  ConfigOptionModel *model = optionModelForTier(tier);
  return model ? model->optNullable(index) : false;
}

// Per-option isVector flag proxy — delegates to optionModelForTier.
bool ConfigViewModel::optIsVector(const QString &tier, int index) const
{
  ConfigOptionModel *model = optionModelForTier(tier);
  return model ? model->optIsVector(index) : false;
}

// Per-option sidetext proxy — delegates to optionModelForTier.
QString ConfigViewModel::optSidetext(const QString &tier, int index) const
{
  ConfigOptionModel *model = optionModelForTier(tier);
  return model ? model->optSidetext(index) : QString();
}

// Group names for a given tier — delegates to optionModel->groupNames().
QStringList ConfigViewModel::groupNames(const QString &tier) const
{
  ConfigOptionModel *model = optionModelForTier(tier);
  return model ? model->groupNames() : QStringList();
}

// Per-group dirty count — delegates to optionModel->dirtyCountForGroup().
int ConfigViewModel::dirtyCountForGroup(const QString &tier, const QString &groupName) const
{
  ConfigOptionModel *model = optionModelForTier(tier);
  return model ? model->dirtyCountForGroup(groupName) : 0;
}
