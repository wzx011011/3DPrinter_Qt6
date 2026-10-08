#include "ConfigViewModel.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSettings>

#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "qml_gui/Models/ConfigOptionModel.h"
#include "qml_gui/Models/PresetListModel.h"

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Preset.hpp>
#endif

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
  refreshPresetListModel();

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
  // Self-connect BEFORE loadDefault(): loadDefault() emits stateChanged on
  // both of its branches, which recomputes filamentSlotCompatibility_ so the
  // property is non-empty from construction on. The explicit call at the end
  // of the constructor is the second leg of the guarantee — either path
  // alone suffices; the O(n) diff guard makes the duplicate harmless.
  connect(this, &ConfigViewModel::stateChanged, this, &ConfigViewModel::refreshFilamentSlotCompatibility);
  loadDefault();
  refreshFilamentSlotCompatibility();
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

  pushValueSourcesToModels();
}

void ConfigViewModel::pushValueSourcesToModels()
{
  // Mirror valueSources_ into the three option models so delegates can read
  // the per-key source as the valueSource role (replaces the per-row
  // valueSourceForKey Q_INVOKABLE reachback). The model-side diff keeps this
  // silent when nothing changed.
  if (printOptions_)
    printOptions_->setValueSources(valueSources_);
  if (machineOptions_)
    machineOptions_->setValueSources(valueSources_);
  if (filamentOptions_)
    filamentOptions_->setValueSources(valueSources_);
}

void ConfigViewModel::refreshFilamentSlotCompatibility()
{
  // Length = 1 + filamentSlotPresets_.size(); item i mirrors
  // isFilamentCompatibleForSlot(i) exactly — including the fallback semantics
  // of filamentPresetForSlot (slot 0 and out-of-range slots resolve to
  // currentFilamentPreset_).
  QList<int> next;
  next.reserve(filamentSlotPresets_.size() + 1);
  for (int slot = 0; slot <= filamentSlotPresets_.size(); ++slot)
    next.append(isFilamentCompatible(filamentPresetForSlot(slot)) ? 1 : 0);

  if (next == filamentSlotCompatibility_)
    return;
  filamentSlotCompatibility_ = next;
  emit filamentSlotCompatibilityChanged();
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
  // v5.16 (PSET2-03/PSET2-06): per-slot filament switch actions queued by
  // requestFilamentPresetForSlot ("switch-filament-preset-<n>"). These
  // previously fell through the exact-match branches above and the switch
  // was silently dropped after the dirty guard resolved.
  if (action.startsWith(QStringLiteral("switch-filament-preset-"))) {
    const int slot = action.mid(QStringLiteral("switch-filament-preset-").size()).toInt();
    if (slot > 0)
      requestFilamentPresetForSlot(slot, target);
    else
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
      refreshPresetListModel();
      emit stateChanged();
    }
    return ok;
}

// v5.16 (PSET2-04): per-preset upstream-shape JSON bundle proxies.
int ConfigViewModel::exportBundleIni(const QString &dirPath) const
{
    return presetService_ ? presetService_->exportBundleIni(dirPath) : -1;
}

int ConfigViewModel::exportBundleIni(const QString &dirPath, const QStringList &presetNames) const
{
    return presetService_ ? presetService_->exportBundleIni(dirPath, presetNames) : -1;
}

int ConfigViewModel::importBundleIni(const QString &dirPath)
{
    if (!presetService_) return -1;
    const int imported = presetService_->importBundleIni(dirPath);
    if (imported > 0) {
      refreshPresetListModel();
      emit stateChanged();
    }
    return imported;
}

void ConfigViewModel::refreshPresetListModel()
{
  // v5.16 (PSET2-05): pass the active printer so the model can compute the
  // per-preset compatibility used for the in-list graying.
  if (presetList_ && presetService_)
    presetList_->refreshFromService(presetService_, currentPrinterPreset_);
}

bool ConfigViewModel::saveCurrentPreset()
{
  if (!presetService_)
    return false;

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
    return false;

  const QHash<QString, QVariant> tierValues = editableValuesForTier(tier);

  if (!presetService_->savePresetValues(targetPreset, tierValues))
    return false;

  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = tierValues;
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = tierValues;
  else
    printPresetValues_ = tierValues;
  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
  return true;
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

QStringList ConfigViewModel::userPresetNamesForCategory(int category) const
{
  return presetService_ ? presetService_->userPresetNamesForCategory(category) : QStringList{};
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

// ── v5.16 (PSET2-05): sectioned/grayed combo display lists ────────────────
// Upstream truth: PresetComboBoxes.cpp:1281-1317 — non-system (user) presets
// and system presets render under "User presets" / "System presets"
// separators, and incompatible entries are disabled with
// LABEL_ITEM_DISABLED. CxComboBox renders "— … —" entries as non-selectable
// section headers and " (不兼容)"-suffixed entries as disabled, so the plain
// preset lists above stay machine-readable and the decorated ones
// presentation-only.

namespace {
const QString kIncompatibleSuffix = QStringLiteral(" (不兼容)");

QString decoratedSectionHeader(const QString &label)
{
  return QStringLiteral("— ") + label + QStringLiteral(" —");
}
} // namespace

QString ConfigViewModel::plainPresetName(const QString &displayName) const
{
  if (displayName.endsWith(kIncompatibleSuffix))
    return displayName.left(displayName.size() - kIncompatibleSuffix.size());
  return displayName;
}

QStringList ConfigViewModel::decoratedPrinterPresetNames() const
{
  return decoratedPresetNamesForCategory(PresetServiceMock::PrinterCat);
}

QStringList ConfigViewModel::decoratedFilamentPresetNames() const
{
  return decoratedPresetNamesForCategory(PresetServiceMock::FilamentCat);
}

QStringList ConfigViewModel::decoratedPrintPresetNames() const
{
  return decoratedPresetNamesForCategory(PresetServiceMock::PrintCat);
}

QStringList ConfigViewModel::decoratedPresetNamesForCategory(int category) const
{
  if (!presetService_ || category < PresetServiceMock::PrintCat ||
      category > PresetServiceMock::PrinterCat)
    return {};

  const QStringList names = presetService_->presetNamesForCategory(category);
  QString current;
  if (category == PresetServiceMock::PrinterCat)
    current = currentPrinterPreset_;
  else if (category == PresetServiceMock::FilamentCat)
    current = currentFilamentPreset_;
  else
    current = currentPrintPreset_;

  // Compatibility graying applies to filament/process presets against the
  // active printer (upstream update_compatible); printer combos never gray.
  const bool compatibilityRelevant = category != PresetServiceMock::PrinterCat;

  QStringList userEntries, systemEntries;
  for (const QString &name : names)
  {
    QString display = name;
    if (compatibilityRelevant && name != current &&
        !presetService_->isPresetCompatibleWithPrinter(category, name, currentPrinterPreset_))
    {
      // The current selection stays undecorated so combo lookup keeps
      // matching (upstream validate_selection keeps it selectable).
      display += kIncompatibleSuffix;
    }
    if (presetService_->isUserPreset(name))
      userEntries.append(display);
    else
      systemEntries.append(display);
  }

  QStringList result;
  if (!userEntries.isEmpty())
  {
    result.append(decoratedSectionHeader(tr("用户预设")));
    result.append(userEntries);
  }
  if (!systemEntries.isEmpty())
  {
    result.append(decoratedSectionHeader(tr("系统预设")));
    result.append(systemEntries);
  }
  return result;
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

QString ConfigViewModel::bedTextureFile() const
{
  // v5.15 (BEDTEX): PresetServiceMock resolves the machine_model JSON's
  // bed_texture against the vendor profile dir (upstream
  // PresetUtils::system_printer_bed_texture mapping).
  if (presetService_ == nullptr || currentPrinterPreset_.isEmpty())
    return {};
  return presetService_->bedTextureFileForPreset(currentPrinterPreset_);
}

QString ConfigViewModel::bedModelFile() const
{
  if (presetService_ == nullptr || currentPrinterPreset_.isEmpty())
    return {};
  return presetService_->bedModelFileForPreset(currentPrinterPreset_);
}

bool ConfigViewModel::bedTypeTexturesActive() const
{
  if (presetService_ == nullptr || currentPrinterPreset_.isEmpty())
    return false;
  return presetService_->isBblVendorPreset(currentPrinterPreset_);
}

bool ConfigViewModel::bedCaliLinesActive() const
{
  if (presetService_ == nullptr || currentPrinterPreset_.isEmpty())
    return false;
  return presetService_->isBblVendorPreset(currentPrinterPreset_)
      && presetService_->presetHasCaliLines(currentPrinterPreset_);
}

QString ConfigViewModel::bedTypeImagesDir() const
{
  return presetService_ ? presetService_->resourcesImagesDir() : QString();
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
  return createCustomPreset(category, name, QString());
}

// ── R-P1.J layering governance: wizard / wipe-tower proxies (pass-throughs)
QStringList ConfigViewModel::wizardVendors() const
{
  return presetService_ ? presetService_->vendors() : QStringList();
}

QStringList ConfigViewModel::wizardAvailableVendorNames() const
{
  return presetService_ ? presetService_->availableVendorNames() : QStringList();
}

QString ConfigViewModel::wizardSelectedVendor() const
{
  return presetService_ ? presetService_->selectedVendor() : QString();
}

void ConfigViewModel::wizardLoadVendor(const QString &vendor)
{
  if (presetService_)
    presetService_->loadVendor(vendor);
}

QStringList ConfigViewModel::wizardPrinterModelsForVendor(const QString &vendor) const
{
  return presetService_ ? presetService_->printerModelsForVendor(vendor) : QStringList();
}

QStringList ConfigViewModel::wizardMaterialsForVendorAndPrinter(const QString &vendor, const QString &printerModel) const
{
  return presetService_
      ? presetService_->materialsForVendorAndPrinter(vendor, printerModel)
      : QStringList();
}

QStringList ConfigViewModel::wizardDefaultBedTypes() const
{
  return presetService_ ? presetService_->defaultBedTypes() : QStringList();
}

void ConfigViewModel::wizardSetSelectedVendor(const QString &vendor)
{
  if (presetService_)
    presetService_->setSelectedVendor(vendor);
}

void ConfigViewModel::wizardSetSelectedPrinterModel(const QString &model)
{
  if (presetService_)
    presetService_->setSelectedPrinterModel(model);
}

QVariantList ConfigViewModel::wizardPrinterPickerGroups(const QString &vendor) const
{
  // U-WIZ: group the vendor's flat printer preset names into PrinterPicker
  // cells (upstream ConfigWizard.cpp:198-373 walks VendorProfile::PrinterModel
  // and emits one grid cell per model with a checkbox per nozzle variant).
  // Vendor-profile preset names end with "<variant> nozzle"; built-in
  // fallback names do not, so they become single-variant cells whose label
  // comes from the preset's nozzle_diameter (default 0.4, upstream
  // PageDiameters fallback ConfigWizard.cpp:1328-1334).
  QVariantList groups;
  static const QRegularExpression nozzleTail(
      QStringLiteral("^(.*)\\s+([0-9.]+)\\s+nozzle$"));
  QMap<QString, QStringList> byModel;
  QStringList modelOrder;
  const QStringList names = wizardPrinterModelsForVendor(vendor);
  for (const QString &name : names)
  {
    const auto match = nozzleTail.match(name);
    QString model;
    if (match.hasMatch())
      model = match.captured(1);
    else
      model = name;
    if (!byModel.contains(model))
      modelOrder.append(model);
    byModel[model].append(name);
  }
  for (const QString &model : modelOrder)
  {
    QVariantMap cell;
    cell.insert(QStringLiteral("model"), model);
    cell.insert(QStringLiteral("variants"), byModel.value(model));
    groups.append(cell);
  }
  return groups;
}

QStringList ConfigViewModel::wizardGcodeFlavorLabels() const
{
  // U-WIZ: upstream print_config_def "gcode_flavor" enum_labels
  // (PrintConfig.cpp:4289-4310). Order matches enum_values marlin / klipper /
  // reprapfirmware / repetier / marlin2; the upstream default is index 0
  // (gcfMarlinLegacy, PrintConfig.cpp:4321).
  return {
      QStringLiteral("Marlin(legacy)"),
      QStringLiteral("Klipper"),
      QStringLiteral("RepRapFirmware"),
      QStringLiteral("Repetier"),
      QStringLiteral("Marlin 2"),
  };
}

bool ConfigViewModel::wizardPrinterVariantEnabled(const QString &vendor, const QString &model, const QString &variant) const
{
  // U-WIZ: AppConfig-lite mirror of upstream AppConfig::get_variant
  // (ConfigWizard.cpp:567-569 reads it to seed the checkbox states).
  QSettings settings;
  return settings.value(QStringLiteral("wizard/variant/%1/%2/%3").arg(vendor, model, variant), false).toBool();
}

void ConfigViewModel::wizardSetPrinterVariantEnabled(const QString &vendor, const QString &model, const QString &variant, bool enabled)
{
  // U-WIZ: AppConfig-lite mirror of upstream AppConfig::set_variant.
  QSettings settings;
  settings.setValue(QStringLiteral("wizard/variant/%1/%2/%3").arg(vendor, model, variant), enabled);
}

QStringList ConfigViewModel::wizardFilamentTypes() const
{
  // U-WIZ: PageMaterials Type column. The Qt6 preset store carries no
  // per-preset type metadata, so the type token is parsed from the preset
  // name (upstream derives the same grouping from filament preset aliases).
  static const char *tokens[] = {
      "PLA", "ABS", "ASA", "PETG", "TPU", "TPE", "PVA", "HIPS",
      "PA", "PC", "PP", "PET", "PEEK", "PEI", "POM", "PVB",
  };
  QStringList result;
  if (!presetService_)
    return result;
  const QStringList loadedVendors = presetService_->vendors();
  for (const QString &vendor : loadedVendors)
  {
    const QStringList materials = presetService_->materialsForVendor(vendor);
    for (const QString &name : materials)
    {
      for (const char *token : tokens)
      {
        if (name.contains(QLatin1String(token), Qt::CaseInsensitive)
            && !result.contains(QLatin1String(token)))
        {
          result.append(QLatin1String(token));
          break;
        }
      }
    }
  }
  return result;
}

QStringList ConfigViewModel::wizardCompatiblePrinters(const QString &vendor, const QString &filamentName) const
{
  // U-WIZ: feeds the compatible-printers preview window (upstream
  // PageMaterials::set_compatible_printers_html_window,
  // ConfigWizard.cpp:758/884). A printer is listed when the filament preset
  // has no compatible_printers restriction (universal) or names it.
  QStringList result;
  if (!presetService_)
    return result;
  const QStringList printers = presetService_->printerModelsForVendor(vendor);
  for (const QString &printer : printers)
  {
    if (presetService_->isPresetCompatibleWithPrinter(PresetServiceMock::FilamentCat, filamentName, printer))
      result.append(printer);
  }
  return result;
}

bool ConfigViewModel::wizardCreateCustomPrinterPreset(const QString &name, const QVariantMap &values)
{
  // U-WIZ: the custom-printer path of the wizard (upstream PageCustom +
  // PageFirmware/PageBedShape/PageDiameters write into custom_config and
  // apply_config persists it, ConfigWizard.cpp:1365-1392/2773-2785). The
  // Qt6 sink is a USER printer preset created from the collected values.
  lastPresetError_.clear();
  if (!presetService_)
  {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }
  QHash<QString, QVariant> hash;
  for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    hash.insert(it.key(), it.value());
  const bool ok = presetService_->createCustomPreset(PresetServiceMock::PrinterCat, name, hash);
  if (!ok)
  {
    lastPresetError_ = tr("Failed to create custom printer preset \"%1\".").arg(name);
  }
  else
  {
    // G-13: rebuild the preset list models so the new preset is selectable.
    refreshPresetLists();
  }
  emit stateChanged();
  return ok;
}

QVariant ConfigViewModel::wizardPresetValue(const QString &presetName, const QString &key) const
{
  return presetService_ ? presetService_->presetValue(presetName, key) : QVariant();
}

QVariantList ConfigViewModel::wizardFlushMatrix() const
{
  return presetService_ ? presetService_->calculateFlushMatrix() : QVariantList();
}

bool ConfigViewModel::wizardSaveFlushVolumes(const QVariantList &rows)
{
  return presetService_ ? presetService_->saveFlushVolumes(rows) : false;
}

QStringList ConfigViewModel::wizardFilamentColours() const
{
  return presetService_ ? presetService_->activeFilamentColours() : QStringList();
}

double ConfigViewModel::flushMultiplier() const
{
  QSettings settings;
  return settings.value(QStringLiteral("flush/multiplier"), 1.0).toDouble();
}

void ConfigViewModel::setFlushMultiplier(double multiplier)
{
  // Valid range [0, 3] (g_min/g_max_flush_multiplier,
  // WipeTowerDialog.cpp:199-200; the web layer clamps the same window on
  // input, WipingDialog.html:599-609).
  multiplier = std::clamp(multiplier, 0.0, 3.0);
  QSettings settings;
  if (qFuzzyCompare(settings.value(QStringLiteral("flush/multiplier"), 1.0).toDouble(),
                    multiplier))
    return;
  settings.setValue(QStringLiteral("flush/multiplier"), multiplier);
}

bool ConfigViewModel::createCustomPreset(int category, const QString &name, const QString &inherits)
{
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }

  QString tier;
  if (category == PresetServiceMock::PrinterCat)
    tier = QStringLiteral("printer");
  else if (category == PresetServiceMock::FilamentCat)
    tier = QStringLiteral("filament");
  else if (category == PresetServiceMock::PrintCat)
    tier = QStringLiteral("print");
  else {
    lastPresetError_ = tr("Unsupported preset category.");
    emit stateChanged();
    return false;
  }

  // v5.16 (PSET2-02): with an explicit parent, seed the new preset from the
  // parent's resolved chain (CreatePresetsDialog "inherits from"); otherwise
  // fall back to the current tier edits (upstream save_current_preset).
  const QString parent = inherits.trimmed();
  QHash<QString, QVariant> tierValues;
  if (!parent.isEmpty())
  {
    if (!presetService_->hasPreset(parent)) {
      lastPresetError_ = tr("The selected parent preset no longer exists.");
      emit stateChanged();
      return false;
    }
    if (presetService_->presetCategory(parent) != category) {
      lastPresetError_ = tr("The selected parent belongs to another preset category.");
      emit stateChanged();
      return false;
    }
    tierValues = presetService_->presetValues(parent);
  }
  else
  {
    tierValues = editableValuesForTier(tier);
  }

  const QString trimmedName = name.trimmed();
  if (trimmedName.isEmpty()) {
    lastPresetError_ = tr("Preset name cannot be empty.");
    emit stateChanged();
    return false;
  }
  if (presetService_->hasPreset(trimmedName)) {
    lastPresetError_ = tr("A preset with this name already exists.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->createCustomPreset(category, trimmedName, tierValues, parent)) {
    lastPresetError_ = tr("Failed to save preset '%1' to disk.").arg(trimmedName);
    emit stateChanged();
    return false;
  }

  setCurrentPresetTierValue(tier, trimmedName);

  refreshPresetListModel();
  mergePresetHierarchy();
  emit stateChanged();
  if (hasPendingUnsavedChanges())
    return applyPendingAction();
  return true;
}

bool ConfigViewModel::presetExists(const QString &name) const
{
  return presetService_ ? presetService_->hasPreset(name.trimmed()) : false;
}

bool ConfigViewModel::createPresetFromBase(int category, const QString &name,
                                           const QString &baseName,
                                           const QVariantMap &overrides, bool overwrite)
{
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }

  if (!presetService_->createPresetFromBase(category, name, baseName, overrides, overwrite)) {
    const QString trimmedName = name.trimmed();
    if (!overwrite && presetService_->hasPreset(trimmedName))
      lastPresetError_ = tr("A preset with this name already exists.");
    else
      lastPresetError_ = tr("Failed to save preset '%1' to disk.").arg(trimmedName);
    emit stateChanged();
    return false;
  }

  refreshPresetListModel();
  mergePresetHierarchy();
  emit stateChanged();
  return true;
}

QStringList ConfigViewModel::compatiblePresetsForPrinter(int category, const QString &printerName) const
{
  return presetService_
      ? presetService_->compatiblePresetNamesForCategory(category, printerName)
      : QStringList{};
}

void ConfigViewModel::refreshPresetLists()
{
  // G-13: re-read the service store (e.g. after in-project embedded presets
  // from a loaded 3MF were adopted) and surface the change.
  refreshPresetListModel();
  mergePresetHierarchy();
  emit stateChanged();
}

QString ConfigViewModel::presetInheritsParent(const QString &name) const
{
  // G-13: parent preset name for the Detach flow (empty = standalone).
  return presetService_ ? presetService_->presetInherits(name.trimmed()) : QString();
}

// ── SavePresetDialog support (upstream SavePresetDialog.cpp) ──────────────

QString ConfigViewModel::currentPresetNameForCategory(int category) const
{
  if (category == PresetServiceMock::PrinterCat)
    return currentPrinterPreset_;
  if (category == PresetServiceMock::FilamentCat)
    return currentFilamentPreset_;
  if (category == PresetServiceMock::PrintCat)
    return currentPrintPreset_.isEmpty() ? currentPreset_ : currentPrintPreset_;
  return QString();
}

void ConfigViewModel::setSaveToProjectSelection(bool saveToProject)
{
  // Upstream radio binding (SavePresetDialog.cpp:163-165): selecting a radio
  // writes Item::m_save_to_project.
  if (m_saveToProject == saveToProject)
    return;
  m_saveToProject = saveToProject;
  emit stateChanged();
}

bool ConfigViewModel::editedPresetIsProjectEmbedded(int category) const
{
  // Upstream Item ctor (SavePresetDialog.cpp:167-168): the initial radio
  // selection comes from get_edited_preset().is_project_embedded.
  return presetIsProjectEmbedded(currentPresetNameForCategory(category));
}

bool ConfigViewModel::presetIsProjectEmbedded(const QString &name) const
{
  return m_projectEmbeddedPresetNames.contains(name.trimmed());
}

void ConfigViewModel::markPresetProjectEmbedded(const QString &name)
{
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty())
    return;
  m_projectEmbeddedPresetNames.insert(trimmed);
  emit stateChanged();
}

QString ConfigViewModel::suggestedSavePresetName(int category, const QString &suffix) const
{
  // Upstream Item ctor (SavePresetDialog.cpp:39): is_default -> "Untitled";
  // is_system -> "<name> - <suffix>"; bundle alias -> alias; else the plain
  // name. This backend has no default-only preset object and no alias
  // metadata, so the first and third branches never apply here.
  QString presetName = currentPresetNameForCategory(category);
  if (presetName.isEmpty())
    return QString();
  if (presetService_ && presetService_->isBuiltinPreset(presetName)) {
    const QString effectiveSuffix = suffix.isEmpty() ? QStringLiteral("Copy") : suffix;
    presetName = presetName + QStringLiteral(" - ") + effectiveSuffix;
  }
  // cpp:41-45: a trailing ".ini" (any case) is stripped.
  if (presetName.endsWith(QStringLiteral(".ini"), Qt::CaseInsensitive))
    presetName.chop(4);
  return presetName;
}

QString ConfigViewModel::presetAliasFor(const QString &name) const
{
  // Upstream cpp:234-237 rejects a name equal to a preset alias
  // (PresetCollection::get_preset_name_by_alias). The Qt6 preset backend
  // carries no alias metadata yet, so this always returns empty and the
  // check stays dormant until the backend exposes aliases.
  Q_UNUSED(name);
  return QString();
}

int ConfigViewModel::savePresetNameErrorCode(int category, const QString &name) const
{
  // Upstream Item::update() (SavePresetDialog.cpp:182-237), same check
  // order. Upstream validates the raw input -- leading/trailing spaces have
  // their own messages, so no trimming here.
  const QString illegalChars = QStringLiteral("<>[]:/\\|?*\"");  // cpp:182
  for (const QChar &ch : illegalChars) {
    if (name.contains(ch))
      return SavePresetNameIllegalChars;
  }
  if (name.contains(QStringLiteral(" (modified)")))  // Preset.cpp:439 g_suffix_modified
    return SavePresetNameIllegalSuffix;
  if (name == QStringLiteral("Default Setting")
      || name == QStringLiteral("Default Filament")  // PresetBundle.cpp:115 ORCA_DEFAULT_FILAMENT_PLACEHOLDER
      || name == QStringLiteral("Default Printer"))
    return SavePresetNameReserved;
  if (presetService_) {
    // cpp:204-208: presets whose can_overwrite() is false (system/builtin)
    // must not be overwritten.
    const QString trimmed = name.trimmed();
    if (presetService_->presetNamesForCategory(category).contains(trimmed)
        && !presetService_->isUserPreset(trimmed))
      return SavePresetNameSystemOverwrite;
  }
  if (name.isEmpty())
    return SavePresetNameEmpty;
  if (name.startsWith(QLatin1Char(' ')))
    return SavePresetNameLeadingSpace;
  if (name.endsWith(QLatin1Char(' ')))
    return SavePresetNameTrailingSpace;
  if (!presetAliasFor(name).isEmpty())
    return SavePresetNameAliasConflict;
  return SavePresetNameValid;
}

int ConfigViewModel::savePresetNameWarningCode(int category, const QString &name) const
{
  // Upstream cpp:210-217: an existing preset other than the current
  // selection produces the overwrite warning; its compatibility decides the
  // wording and the save stays allowed (Warning, not NoValid).
  if (!presetService_)
    return SavePresetNameNoWarning;
  const QString trimmed = name.trimmed();
  if (trimmed.isEmpty() || !presetService_->presetNamesForCategory(category).contains(trimmed))
    return SavePresetNameNoWarning;
  if (trimmed == currentPresetNameForCategory(category))
    return SavePresetNameNoWarning;
  return presetService_->isPresetCompatibleWithPrinter(category, trimmed, currentPrinterPreset_)
             ? SavePresetNameExists
             : SavePresetNameExistsIncompatible;
}

QString ConfigViewModel::savePresetParentName(int category) const
{
  // Upstream cpp:114-117: a system preset is its own parent; a user preset
  // uses its inherits() link. Empty = "Unique preset".
  const QString name = currentPresetNameForCategory(category);
  if (name.isEmpty())
    return QString();
  if (presetService_ && presetService_->isBuiltinPreset(name))
    return name;
  return presetInheritsParent(name);
}

bool ConfigViewModel::physicalPrinterHasSelection() const
{
  return !m_physicalPrinterName.isEmpty();
}

QString ConfigViewModel::physicalPrinterSelectedName() const
{
  return m_physicalPrinterName;
}

QString ConfigViewModel::physicalPrinterSelectedPresetName() const
{
  return m_physicalPrinterPresetName;
}

void ConfigViewModel::setPhysicalPrinterSelection(const QString &printerName, const QString &presetName)
{
  if (m_physicalPrinterName == printerName && m_physicalPrinterPresetName == presetName)
    return;
  m_physicalPrinterName = printerName;
  m_physicalPrinterPresetName = presetName;
  emit stateChanged();
}

void ConfigViewModel::applyPhysicalPrinterAction(int action, const QString &presetName)
{
  // Upstream update_physical_printers (SavePresetDialog.cpp:472-493). The
  // Qt6 backend has no physical-printer store yet, so the selected printer's
  // preset association is kept consistent on this viewmodel; a Switch
  // unselects the printer, Change/Add re-select it with the new preset.
  if (action == PhysicalPrinterUndefAction || !physicalPrinterHasSelection())
    return;
  if (action == PhysicalPrinterSwitch) {
    m_physicalPrinterName.clear();
    m_physicalPrinterPresetName.clear();
  } else {
    m_physicalPrinterPresetName = presetName;
  }
  emit stateChanged();
}

bool ConfigViewModel::overwriteUserPreset(int category, const QString &name)
{
  // G-01: replace an existing USER preset with the current tier edits
  // (upstream SavePresetDialog replace path). Mirrors createCustomPreset's
  // validation/bookkeeping minus the duplicate rejection; the current
  // selection is intentionally NOT switched -- replacing preset Y while
  // editing preset X keeps X selected (upstream behaviour).
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }

  QString tier;
  if (category == PresetServiceMock::PrinterCat)
    tier = QStringLiteral("printer");
  else if (category == PresetServiceMock::FilamentCat)
    tier = QStringLiteral("filament");
  else if (category == PresetServiceMock::PrintCat)
    tier = QStringLiteral("print");
  else {
    lastPresetError_ = tr("Unsupported preset category.");
    emit stateChanged();
    return false;
  }

  const QString trimmedName = name.trimmed();
  if (trimmedName.isEmpty()) {
    lastPresetError_ = tr("Preset name cannot be empty.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->isUserPreset(trimmedName)) {
    lastPresetError_ = tr("This preset is built-in or read-only.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->overwriteUserPreset(category, trimmedName, editableValuesForTier(tier))) {
    lastPresetError_ = tr("Failed to save preset '%1' to disk.").arg(trimmedName);
    emit stateChanged();
    return false;
  }

  refreshPresetListModel();
  mergePresetHierarchy();
  emit stateChanged();
  if (hasPendingUnsavedChanges())
    return applyPendingAction();
  return true;
}

bool ConfigViewModel::detachPresetFromParent(int category, const QString &name)
{
  // G-13: Detach (upstream "detach from system preset") -- flatten an
  // inherited USER preset with the current tier edits and cut the inherits
  // link. Mirrors overwriteUserPreset's bookkeeping; the selection is
  // intentionally unchanged.
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }

  QString tier;
  if (category == PresetServiceMock::PrinterCat)
    tier = QStringLiteral("printer");
  else if (category == PresetServiceMock::FilamentCat)
    tier = QStringLiteral("filament");
  else if (category == PresetServiceMock::PrintCat)
    tier = QStringLiteral("print");
  else {
    lastPresetError_ = tr("Unsupported preset category.");
    emit stateChanged();
    return false;
  }

  const QString trimmedName = name.trimmed();
  if (trimmedName.isEmpty()) {
    lastPresetError_ = tr("Preset name cannot be empty.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->isUserPreset(trimmedName)) {
    lastPresetError_ = tr("This preset is built-in or read-only.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->detachPresetFromParent(category, trimmedName,
                                              editableValuesForTier(tier))) {
    lastPresetError_ = tr("Failed to save preset '%1' to disk.").arg(trimmedName);
    emit stateChanged();
    return false;
  }

  refreshPresetListModel();
  mergePresetHierarchy();
  emit stateChanged();
  return true;
}

bool ConfigViewModel::deletePreset(int category, const QString &name)
{
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }
  if (presetService_->presetCategory(name) != category) {
    lastPresetError_ = tr("Preset does not belong to the selected category.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->isUserPreset(name)) {
    lastPresetError_ = tr("This preset is built-in or read-only.");
    emit stateChanged();
    return false;
  }
  if (presetService_->hasPresetChildren(name)) {
    lastPresetError_ = tr("Presets inherited by other presets cannot be deleted.");
    emit stateChanged();
    return false;
  }

  // v5.16 (PSET2-07): the previous blanket "in use" early return made the
  // default-switch branches below unreachable dead code. Upstream allows
  // deleting the selected user preset and falls the selection back to the
  // category default (PresetCollection::delete_preset); the UI warns first
  // via isPresetInUse.
  bool ok = presetService_->deletePreset(name);
  if (ok)
  {
    // If the deleted preset was the tier's active selection, fall back to
    // the category default.
    if (name == currentPrintPreset_)
      setCurrentPrintPreset(presetService_->defaultPresetForCategory(PresetServiceMock::PrintCat));
    if (name == currentFilamentPreset_)
      setCurrentFilamentPreset(presetService_->defaultPresetForCategory(PresetServiceMock::FilamentCat));
    if (name == currentPrinterPreset_)
      setCurrentPrinterPreset(presetService_->defaultPresetForCategory(PresetServiceMock::PrinterCat));
    // Per-extruder slots referencing the deleted preset fall back to the
    // (already repaired) global selection.
    bool slotsChanged = false;
    for (QString &slot : filamentSlotPresets_)
    {
      if (slot == name)
      {
        slot = currentFilamentPreset_;
        slotsChanged = true;
      }
    }
    if (slotsChanged)
      emit sliceAffectingConfigChanged();
    refreshPresetListModel();
    emit stateChanged();
  }
  return ok;
}

bool ConfigViewModel::isPresetInUse(const QString &name) const
{
  // v5.16 (PSET2-07): drives the delete confirmation copy — a preset counts
  // as in use when it is any tier's current selection or a per-extruder
  // slot's preset.
  return name == currentPrinterPreset_ || name == currentFilamentPreset_ ||
         name == currentPrintPreset_ || name == currentPreset_ ||
         filamentSlotPresets_.contains(name);
}

bool ConfigViewModel::renamePreset(int category, const QString &oldName, const QString &newName)
{
  lastPresetError_.clear();
  if (!presetService_) {
    lastPresetError_ = tr("Preset service is unavailable.");
    emit stateChanged();
    return false;
  }
  if (newName.trimmed().isEmpty()) {
    lastPresetError_ = tr("Preset name cannot be empty.");
    emit stateChanged();
    return false;
  }
  if (presetService_->presetCategory(oldName) != category) {
    lastPresetError_ = tr("Preset does not belong to the selected category.");
    emit stateChanged();
    return false;
  }
  if (!presetService_->isUserPreset(oldName)) {
    lastPresetError_ = tr("This preset is built-in or read-only.");
    emit stateChanged();
    return false;
  }
  if (presetService_->hasPresetChildren(oldName)) {
    lastPresetError_ = tr("A preset inherited by other presets cannot be renamed.");
    emit stateChanged();
    return false;
  }

  bool ok = presetService_->renamePreset(oldName, newName.trimmed());
  if (!ok) {
    lastPresetError_ = tr("A preset with this name already exists or could not be saved.");
    emit stateChanged();
  }
  if (ok)
  {
    // Keep the current tier preset names in sync with the rename.
    if (oldName == currentPrintPreset_)
      currentPrintPreset_ = newName.trimmed();
    if (oldName == currentPreset_)
      currentPreset_ = newName.trimmed();
    if (oldName == currentFilamentPreset_)
      currentFilamentPreset_ = newName.trimmed();
    if (oldName == currentPrinterPreset_)
      currentPrinterPreset_ = newName.trimmed();
    // v5.16 (PSET2-07): keep per-extruder slot selections pointing at the
    // renamed preset (upstream rename keeps filament_presets in sync).
    for (QString &slot : filamentSlotPresets_)
    {
      if (slot == oldName)
        slot = newName.trimmed();
    }
    refreshPresetListModel();
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
      diffs.append(QStringLiteral("%1: %2 鈭?%3").arg(key, valA.toString(), valB.toString()));
  }

  return diffs;
}

namespace {
// PresetDiffDialog short display string — upstream DiffViewCtrl::get_short_string
// (UnsavedChangesDialog.cpp:660-675): empty and color-like ("#...") strings
// pass through untouched; anything else is cut at 30 chars or at the first
// newline (whichever is shorter) with a trailing ASCII "...".
QString diffShortString(const QString &fullString)
{
  static const int kMaxLen = 30;
  static const QString kDots = QStringLiteral("...");
  if (fullString.isEmpty() || fullString.startsWith(QLatin1Char('#')) ||
      (!fullString.contains(QLatin1Char('\n')) && fullString.length() < kMaxLen))
    return fullString;
  int cut = kMaxLen;
  const int newlinePos = fullString.indexOf(QLatin1Char('\n'));
  if (newlinePos != -1 && newlinePos < kMaxLen)
    cut = newlinePos;
  return fullString.left(cut) + kDots;
}

// PresetDiffDialog tree metadata for one option key (the Qt6 counterpart of
// the upstream searcher lookup in DiffPresetDialog::update_tree,
// UnsavedChangesDialog.cpp:2126-2138: category_local/group_local/label_local).
// Resolved from the option model that owns the compared preset category.
// Keys outside the model fall back to a generic bucket so every dirty option
// stays visible: the Qt6 schemas only cover a subset of the preset store, and
// the upstream searcher-less skip would hide most real diffs.
struct DiffOptionMeta
{
  QString category;
  QString group;
  QString label;
};

DiffOptionMeta diffOptionMeta(ConfigOptionModel *model, const QString &key)
{
  DiffOptionMeta meta;
  meta.category = QObject::tr("其他");
  meta.group = QObject::tr("参数");
  meta.label = key;

  if (!model)
    return meta;
  const int index = model->indexOfKey(key);
  if (index < 0)
    return meta;

  QString category = model->optCategory(index);
  if (category.isEmpty())
    category = model->optPage(index);
  if (!category.isEmpty())
    meta.category = category;

  const QString group = model->optGroup(index);
  if (!group.isEmpty())
    meta.group = group;

  const QVariant displayLabel = model->index(index, 0).data(ConfigOptionModel::DisplayLabelRole);
  if (displayLabel.isValid() && !displayLabel.toString().isEmpty())
    meta.label = displayLabel.toString();
  return meta;
}
} // namespace

// Phase 154 (CLOS-01): structured diff variant — proxies to
// PresetServiceMock::comparePresets (the Phase 149 primitive) and enriches
// each {key, valueA, valueB, status} row with the presentation data the
// PresetDiffDialog tree groups by (upstream DiffViewCtrl::Append input):
//   category/group/label   — searcher-equivalent metadata (diffOptionMeta)
//   valueA/valueB          — 30-char short display strings (diffShortString)
//   fullValueA/fullValueB  — untruncated values for FullCompareDialog
//   isLong                 — truncated/multiline row opens the full-compare
//                            sub dialog (upstream ItemData::is_long)
// The legacy QStringList overload above stays for older callers; this variant
// is the one consumed by PresetDiffDialog.
QVariantList ConfigViewModel::comparePresetsDetailed(const QString &presetA, const QString &presetB) const
{
  if (!presetService_)
    return {};

  // Metadata source follows the compared preset category: a preset row always
  // holds two presets of the same category (printer/filament/print).
  ConfigOptionModel *metaModel = printOptions_;
  switch (presetService_->presetCategory(presetA))
  {
  case PresetServiceMock::PrinterCat:
    metaModel = machineOptions_;
    break;
  case PresetServiceMock::FilamentCat:
    metaModel = filamentOptions_;
    break;
  default:
    break;
  }

  QVariantList enriched;
  const QVariantList rows = presetService_->comparePresets(presetA, presetB);
  enriched.reserve(rows.size());
  for (const QVariant &entry : rows)
  {
    QVariantMap row = entry.toMap();
    const QString key = row.value(QStringLiteral("key")).toString();
    const QString fullA = row.value(QStringLiteral("valueA")).toString();
    const QString fullB = row.value(QStringLiteral("valueB")).toString();
    const QString shortA = diffShortString(fullA);
    const QString shortB = diffShortString(fullB);

    row.insert(QStringLiteral("valueA"), shortA);
    row.insert(QStringLiteral("valueB"), shortB);
    row.insert(QStringLiteral("fullValueA"), fullA);
    row.insert(QStringLiteral("fullValueB"), fullB);
    row.insert(QStringLiteral("isLong"), shortA != fullA || shortB != fullB);

    const DiffOptionMeta meta = diffOptionMeta(metaModel, key);
    row.insert(QStringLiteral("category"), meta.category);
    row.insert(QStringLiteral("group"), meta.group);
    row.insert(QStringLiteral("label"), meta.label);
    enriched.append(row);
  }
  return enriched;
}

// PresetDiffDialog transfer (upstream Tab::transfer_options via
// UnsavedChangesDialog Action::Transfer): copy the selected option values
// from the left preset onto the right preset. The source preset is never
// written; read-only targets are refused by mergePresetValues.
int ConfigViewModel::transferPresetValues(const QString &leftPreset, const QString &rightPreset,
                                          const QStringList &keys)
{
  if (!presetService_ || leftPreset.isEmpty() || rightPreset.isEmpty() || keys.isEmpty())
    return 0;

  QHash<QString, QVariant> values;
  for (const QString &key : keys)
  {
    const QVariant value = presetService_->presetValue(leftPreset, key);
    if (!value.isValid())
      continue;
    values.insert(key, value);
  }
  if (values.isEmpty())
    return 0;
  if (!presetService_->mergePresetValues(rightPreset, values))
    return 0;

  // Upstream transfer_options applies into the live tab when the target is
  // the currently edited preset (Tab.cpp:7331-7337 select_preset +
  // load_current_preset); refresh the merged option values so the settings
  // pages reflect the transfer immediately.
  if (rightPreset == currentPrinterPreset_ || rightPreset == currentFilamentPreset_ ||
      rightPreset == currentPrintPreset_)
  {
    updateMergedPresetValues();
    applyScopeValues();
    emit stateChanged();
  }
  return values.size();
}

// True when the preset is read-only (system/builtin); PresetDiffDialog
// disables Transfer into such targets.
bool ConfigViewModel::presetIsReadOnly(const QString &presetName) const
{
  return presetService_ && presetService_->isReadOnlyPreset(presetName);
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
  // v5.16 (PSET2-05): accept both plain and decorated (combo display) names.
  const QString plain = plainPresetName(name);
  if (presetService_ && presetService_->presetCategory(plain) != PresetServiceMock::PrinterCat) {
    lastPresetError_ = tr("The selected printer preset is unavailable.");
    emit stateChanged();
    return false;
  }
  if (queuePendingAction(QStringLiteral("switch-printer-preset"), plain))
  {
    setCurrentPrinterPreset(plain);
    return true;
  }
  return false;
}

bool ConfigViewModel::requestCurrentFilamentPreset(const QString &name)
{
  const QString plain = plainPresetName(name);
  if (presetService_ && presetService_->presetCategory(plain) != PresetServiceMock::FilamentCat) {
    lastPresetError_ = tr("The selected filament preset is unavailable.");
    emit stateChanged();
    return false;
  }
  if (queuePendingAction(QStringLiteral("switch-filament-preset"), plain))
  {
    setCurrentFilamentPreset(plain);
    return true;
  }
  return false;
}

QString ConfigViewModel::filamentPresetForSlot(int slot) const
{
  // Slot 0 is the global selection; slots 1..N index filamentSlotPresets_.
  if (slot <= 0)
    return currentFilamentPreset_;
  if (slot - 1 < filamentSlotPresets_.size())
    return filamentSlotPresets_.at(slot - 1);
  return currentFilamentPreset_;
}

bool ConfigViewModel::requestFilamentPresetForSlot(int slot, const QString &name)
{
  const QString plain = plainPresetName(name);
  if (slot <= 0)
    return requestCurrentFilamentPreset(plain);
  if (queuePendingAction(QStringLiteral("switch-filament-preset-%1").arg(slot), plain))
  {
    while (filamentSlotPresets_.size() < slot)
      filamentSlotPresets_.append(currentFilamentPreset_);
    if (filamentSlotPresets_.at(slot - 1) == plain)
      return true;
    filamentSlotPresets_[slot - 1] = plain;
    emit stateChanged();
    // A per-slot preset change alters the effective multi-material config.
    emit sliceAffectingConfigChanged();
    return true;
  }
  return false;
}

bool ConfigViewModel::isFilamentCompatibleForSlot(int slot) const
{
  return isFilamentCompatible(filamentPresetForSlot(slot));
}

void ConfigViewModel::setExtruderCount(int count)
{
  // v5.16 (PSET2-06): keep the per-slot vector sized to the extruder count
  // (upstream PresetBundle::update_multi_material_filament_presets resizes
  // filament_presets). Slots beyond the count drop; new slots inherit the
  // global selection.
  // NOTE: the local must NOT be named "slots" — it is a Qt keyword macro
  // and would expand to nothing, breaking the declaration (C2513).
  const int slotCount = qMax(0, count - 1);
  if (filamentSlotPresets_.size() == slotCount)
    return;
  while (filamentSlotPresets_.size() > slotCount)
    filamentSlotPresets_.removeLast();
  while (filamentSlotPresets_.size() < slotCount)
    filamentSlotPresets_.append(currentFilamentPreset_);
  emit stateChanged();
}

QVariantMap ConfigViewModel::projectPresetConfigOverlay() const
{
  // v5.16 (PSET2-06): preset selection state persisted with the project —
  // the tier preset ids plus the filament_presets slot vector (";"
  // separated, slot 0 mirrors the global selection; upstream stores the
  // vector in the 3MF config).
  QVariantMap overlay;
  overlay.insert(QStringLiteral("printer_preset_id"), currentPrinterPreset_);
  overlay.insert(QStringLiteral("filament_preset_id"), currentFilamentPreset_);
  overlay.insert(QStringLiteral("print_preset_id"), currentPrintPreset_);
  // NOTE: named slotList — "slots" is a Qt keyword macro (same as above).
  QStringList slotList;
  slotList.reserve(filamentSlotPresets_.size() + 1);
  slotList.append(currentFilamentPreset_);
  slotList.append(filamentSlotPresets_);
  overlay.insert(QStringLiteral("filament_presets"), slotList.join(QLatin1Char(';')));
  return overlay;
}

bool ConfigViewModel::requestCurrentPrintPreset(const QString &name)
{
  const QString plain = plainPresetName(name);
  if (presetService_ && presetService_->presetCategory(plain) != PresetServiceMock::PrintCat) {
    lastPresetError_ = tr("The selected print preset is unavailable.");
    emit stateChanged();
    return false;
  }
  if (queuePendingAction(QStringLiteral("switch-print-preset"), plain))
  {
    setCurrentPrintPreset(plain);
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

  if (!saveCurrentPreset())
    return false;
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

bool ConfigViewModel::beginUnsavedDialog()
{
  // v5.16 (PSET2-03): re-entry gate — the first SettingsDialog listener to
  // call this owns the modal; the other two shared-configVm instances get
  // false and must not open theirs.
  if (m_unsavedDialogActive)
    return false;
  m_unsavedDialogActive = true;
  emit stateChanged();
  return true;
}

void ConfigViewModel::endUnsavedDialog()
{
  if (!m_unsavedDialogActive)
    return;
  m_unsavedDialogActive = false;
  emit stateChanged();
}

bool ConfigViewModel::transferPendingChanges(const QStringList &keys)
{
  // v5.16 (PSET2-03): upstream Transfer (UnsavedChangesDialog.cpp:1087,
  // 2380) — move the selected modified keys onto the pending target preset,
  // leave the source preset unsaved, then proceed with the pending switch.
  if (!presetService_ || pendingUnsavedAction_.isEmpty() ||
      !pendingUnsavedAction_.startsWith(QStringLiteral("switch-")))
    return false;

  const QString target = pendingUnsavedTarget_;
  if (target.isEmpty() || !presetService_->hasPreset(target) ||
      presetService_->isReadOnlyPreset(target))
    return false;

  const QString tier = normalizedTier(activePresetTier_);
  const QHash<QString, QVariant> effective = effectivePresetValuesForTier(tier);
  QHash<QString, QVariant> selected;
  for (const QString &key : keys)
  {
    const auto it = effective.constFind(key);
    if (it != effective.constEnd())
      selected.insert(key, it.value());
  }
  if (!presetService_->mergePresetValues(target, selected))
    return false;

  // The source preset stays unsaved: revert the tier's edited values so the
  // dirty state clears (upstream Transfer moves values, source reverts).
  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = selectedPresetValuesForTier(tier);
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = selectedPresetValuesForTier(tier);
  else
    printPresetValues_ = selectedPresetValuesForTier(tier);
  updateMergedPresetValues();
  applyScopeValues();

  const bool applied = applyPendingAction();
  refreshPresetListModel();
  emit stateChanged();
  emit sliceAffectingConfigChanged();
  return applied;
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

// Fuzzy matching helper based on upstream OptionsSearcher / fts_fuzzy_match:
// the scoring rule itself now lives in ConfigOptionModel.cpp (anonymous
// namespace, shared with matchesFilter()) so the legacy VM filter and the
// QML-side ConfigOptionFilterProxy cannot drift apart.
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

  const QString needle = searchText.toLower();

  for (int i = 0; i < n; ++i)
  {
    // Search + mode filter (upstream ConfigOptionMode: 0=comSimple, 1=comAdvanced, 2=comDevelop).
    // Simple mode (advancedMode=false) shows only comSimple options; advanced mode
    // (advancedMode=true) is a SUPERSET and shows comSimple + comAdvanced + comDevelop.
    // Never exclude a simple-mode option from advanced mode.
    if (!model->matchesFilter(i, needle, advancedMode))
      continue;

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
  // v5.16 (CIRC-06): match PresetListModel's literal category tr("耗材").
  // The previous tr("鑰楁潗") was GBK mojibake and never matched, so every
  // filament slot displayed as unselected. (Per-slot selection itself is
  // reworked in Phase 232 / CIRC-04.)
  const int globalIdx = presetList_->globalIndex(QStringLiteral("耗材"), localIndex);
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

bool ConfigViewModel::setValue(const QString &key, const QVariant &value)
{
  // Phase 236 (DLG-02): single-key write entry for QML surfaces that edit a
  // whole option outside OptionRow (EditGCodeDialog's editor saves its text
  // back onto machine_start_gcode / machine_end_gcode / ...). Find the option
  // model that owns the key (machine first: gcode keys live there), then run
  // the normal row-based edit so tier routing, dirty tracking and scope
  // handling behave exactly like an inline edit.
  ConfigOptionModel *const models[] = {machineOptions_, filamentOptions_, printOptions_};
  for (ConfigOptionModel *model : models) {
    if (!model)
      continue;
    const int row = model->indexOfKey(key);
    if (row >= 0) {
      model->setValue(row, value);
      return true;
    }
  }
  return false;
}

QVariantList ConfigViewModel::editGcodeParamGroups(const QString &customGcodeKey) const
{
  // EditGCodeDialog placeholder tree. 1:1 with upstream
  // EditGCodeDialog::init_params_list (EditGCodeDialog.cpp:153-243): the
  // [Global] Slicing State group with its Read Only / Read Write lock
  // subgroups (:161-175), Slicing State (:176-182), Print Statistics
  // (:187-192), Objects Info (:195-200), Dimensions (:203-208), Temperatures
  // (:211-216), Timestamps (:219-224), the per-key "Specific for <key>" group
  // (:228-236) and the Presets group with its three preset subgroups
  // (:238, add_presets_placeholders :301-312). All content comes from the
  // real libslic3r ConfigDefs, exactly like upstream -- no hardcoded key
  // lists on the QML side.
  QVariantList groups;
#ifndef HAS_LIBSLIC3R
  Q_UNUSED(customGcodeKey);
  return groups;
#else
  using namespace Slic3r;

  // Function-local statics for the placeholder defs upstream keeps as dialog
  // members (EditGCodeDialog.hpp:36-45 + custom_gcode_specific_config_def).
  // Lazy construction sidesteps the static-init-order hazards that keep
  // libslic3r objects out of the global constructor phase.
  static const ReadOnlySlicingStatesConfigDef cgpRoSlicingStates;
  static const ReadWriteSlicingStatesConfigDef cgpRwSlicingStates;
  static const OtherSlicingStatesConfigDef cgpOtherSlicingStates;
  static const PrintStatisticsConfigDef cgpPrintStatistics;
  static const ObjectsInfoConfigDef cgpObjectsInfo;
  static const DimensionsConfigDef cgpDimensions;
  static const TemperaturesConfigDef cgpTemperatures;
  static const TimestampsConfigDef cgpTimestamps;
  static const OtherPresetsConfigDef cgpOtherPresets;
  static const CustomGcodeSpecificConfigDef cgpSpecific;

  // Type string for the selection label panel (upstream
  // EditGCodeDialog.cpp:372-387: float/integer/string/percent/"float or
  // percent"/point/bool/enum, vectors get a "[]" suffix).
  auto typeString = [](const ConfigOptionDef &def) {
    const ConfigOptionType scalarType =
        def.is_scalar() ? def.type : static_cast<ConfigOptionType>(def.type - coVectorType);
    QString typeStr =
        scalarType == coNone           ? QStringLiteral("none") :
        scalarType == coFloat          ? QStringLiteral("float") :
        scalarType == coInt            ? QStringLiteral("integer") :
        scalarType == coString         ? QStringLiteral("string") :
        scalarType == coPercent        ? QStringLiteral("percent") :
        scalarType == coFloatOrPercent ? QStringLiteral("float or percent") :
        scalarType == coPoint          ? QStringLiteral("point") :
        scalarType == coBool           ? QStringLiteral("bool") :
        scalarType == coEnum           ? QStringLiteral("enum") : QStringLiteral("undef");
    if (!def.is_scalar())
      typeStr += QStringLiteral("[]");
    return typeStr;
  };

  // Param node payload. Upstream AppendParam (EditGCodeDialog.cpp:481-492)
  // shows the bare key (vector keys rendered with a "[]" suffix); the def
  // metadata here feeds the selection label / description panel in QML.
  auto appendParam = [&typeString](QVariantList &out, const ConfigOptionDef *def,
                                   const std::string &optKey) {
    if (!def || def->type == coNone)
      return;
    QVariantMap node;
    node.insert(QStringLiteral("key"), QString::fromStdString(optKey));
    // kind: 0=Scalar, 1=Vector (upstream get_type, EditGCodeDialog.cpp:148-151;
    // FilamentVector is never constructed in that dialog).
    node.insert(QStringLiteral("kind"), def->is_scalar() ? 0 : 1);
    node.insert(QStringLiteral("typeStr"), typeString(*def));
    node.insert(QStringLiteral("label"), QString::fromStdString(def->label));
    node.insert(QStringLiteral("fullLabel"), QString::fromStdString(def->full_label));
    node.insert(QStringLiteral("tooltip"), QString::fromStdString(def->tooltip));
    out.append(node);
  };

  auto appendDefParams = [&appendParam](QVariantList &out, const ConfigDef &def) {
    for (const auto &entry : def.options)
      appendParam(out, &entry.second, entry.first);
  };

  // -- [Global] Slicing State + Read Only / Read Write lock subgroups
  //    (EditGCodeDialog.cpp:161-175) --
  {
    QVariantMap globalGroup;
    globalGroup.insert(QStringLiteral("id"), QStringLiteral("global_slicing_state"));
    QVariantList subgroups;
    if (!cgpRoSlicingStates.options.empty()) {
      QVariantMap ro;
      ro.insert(QStringLiteral("id"), QStringLiteral("read_only"));
      QVariantList roParams;
      appendDefParams(roParams, cgpRoSlicingStates);
      ro.insert(QStringLiteral("params"), roParams);
      subgroups.append(ro);
    }
    if (!cgpRwSlicingStates.options.empty()) {
      QVariantMap rw;
      rw.insert(QStringLiteral("id"), QStringLiteral("read_write"));
      QVariantList rwParams;
      appendDefParams(rwParams, cgpRwSlicingStates);
      rw.insert(QStringLiteral("params"), rwParams);
      subgroups.append(rw);
    }
    globalGroup.insert(QStringLiteral("subgroups"), subgroups);
    globalGroup.insert(QStringLiteral("params"), QVariantList());
    groups.append(globalGroup);
  }

  // -- Slicing State / Print Statistics / Objects Info / Dimensions /
  //    Temperatures / Timestamps (EditGCodeDialog.cpp:176-224) --
  const std::pair<const char *, const ConfigDef *> simpleGroups[] = {
      {"slicing_state", &cgpOtherSlicingStates}, {"print_statistics", &cgpPrintStatistics},
      {"objects_info", &cgpObjectsInfo},         {"dimensions", &cgpDimensions},
      {"temperatures", &cgpTemperatures},        {"timestamps", &cgpTimestamps},
  };
  for (const auto &entry : simpleGroups) {
    if (entry.second->options.empty())
      continue;
    QVariantMap group;
    group.insert(QStringLiteral("id"), QString::fromLatin1(entry.first));
    QVariantList params;
    appendDefParams(params, *entry.second);
    group.insert(QStringLiteral("params"), params);
    group.insert(QStringLiteral("subgroups"), QVariantList());
    groups.append(group);
  }

  // -- Specific for <key> (EditGCodeDialog.cpp:228-236): the placeholder list
  //    from custom_gcode_specific_placeholders(), each key resolved through
  //    CustomGcodeSpecificConfigDef and coNone defs skipped (:231). --
  {
    const auto &placeholders = custom_gcode_specific_placeholders();
    const auto it = placeholders.find(customGcodeKey.toStdString());
    if (it != placeholders.end() && !it->second.empty()) {
      QVariantList params;
      for (const auto &optKey : it->second)
        appendParam(params, cgpSpecific.get(optKey), optKey);
      if (!params.isEmpty()) {
        QVariantMap group;
        group.insert(QStringLiteral("id"), QStringLiteral("specific"));
        group.insert(QStringLiteral("params"), params);
        group.insert(QStringLiteral("subgroups"), QVariantList());
        groups.append(group);
      }
    }
  }

  // -- Presets (EditGCodeDialog.cpp:238 + add_presets_placeholders :301-312):
  //    three subgroups fed by the FFF preset option key sets, every key
  //    resolved through the full print_config_def (upstream full_config.optptr
  //    gate, :281). Keys are sorted alphabetically to mirror the upstream
  //    per-page std::map / std::set iteration order (init_from_tab :262-292);
  //    the per-tab-page breakdown is not reproduced here, matching the
  //    verified gap scope (three subgroups only). --
  {
    QVariantMap presetsGroup;
    presetsGroup.insert(QStringLiteral("id"), QStringLiteral("presets"));
    auto presetSubgroup = [&appendParam](const char *id,
                                         const std::vector<std::string> &keys) {
      QVariantMap sub;
      sub.insert(QStringLiteral("id"), QString::fromLatin1(id));
      std::vector<std::string> sorted(keys.cbegin(), keys.cend());
      std::sort(sorted.begin(), sorted.end());
      QVariantList params;
      for (const auto &optKey : sorted)
        appendParam(params, print_config_def.get(optKey), optKey);
      sub.insert(QStringLiteral("params"), params);
      return sub;
    };
    // Qt6 app is FFF-only; upstream picks the option sets by
    // plater()->printer_technology() (EditGCodeDialog.cpp:253-255).
    QVariantList subgroups;
    subgroups.append(presetSubgroup("print_settings", Preset::print_options()));
    subgroups.append(presetSubgroup("filament_settings", Preset::filament_options()));
    subgroups.append(presetSubgroup("printer_settings", Preset::printer_options()));
    presetsGroup.insert(QStringLiteral("subgroups"), subgroups);
    // Other preset-related params appended directly under Presets, after the
    // subgroups (EditGCodeDialog.cpp:239-242).
    QVariantList presetParams;
    appendDefParams(presetParams, cgpOtherPresets);
    presetsGroup.insert(QStringLiteral("params"), presetParams);
    groups.append(presetsGroup);
  }

  return groups;
#endif
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
  pushValueSourcesToModels();

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

  // v5.16 (PSET2-06): restore the per-slot filament preset vector. The key
  // carries every slot including 0 (the global selection — upstream
  // filament_presets semantics); the first entry wins over
  // filament_preset_id when both are present.
  const auto slotsIt = config.find(QStringLiteral("filament_presets"));
  if (slotsIt != config.end())
  {
    const QStringList slotNames = slotsIt.value().toString().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (!slotNames.isEmpty())
    {
      const QString globalName = plainPresetName(slotNames.first());
      if (!globalName.isEmpty() && presetService_ && presetService_->hasPreset(globalName))
        setCurrentFilamentPreset(globalName);
      QStringList rest;
      rest.reserve(slotNames.size() - 1);
      for (int i = 1; i < slotNames.size(); ++i)
        rest.append(plainPresetName(slotNames.at(i)));
      filamentSlotPresets_ = rest;
    }
  }

  // Apply remaining config keys into the editable print-tier state.
  for (auto it = config.constBegin(); it != config.constEnd(); ++it)
  {
    const QString &key = it.key();
    // Skip meta keys that aren't real config options
    if (key == QStringLiteral("printer_preset_id") ||
        key == QStringLiteral("filament_preset_id") ||
        key == QStringLiteral("print_preset_id") ||
        key == QStringLiteral("filament_presets") ||
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

// Upstream UnsavedChangesDialog renders a category -> group -> option tree
// (update_list, UnsavedChangesDialog.cpp:1329-1460) built from the
// PresetItem metadata (category_name / group_name / option_name,
// :1396-1448). Resolve the same three fields from the active tier's option
// model so the QML diff guard can rebuild the hierarchy. Keys that have no
// option-model entry (e.g. raw keys applied from a loaded project) resolve
// to empty metadata; globalModifiedLabel falls back to the raw key so the
// row never renders blank.
QString ConfigViewModel::globalModifiedCategory(const QString &key) const
{
  ConfigOptionModel *model = optionModelForTier(normalizedTier(activePresetTier_));
  if (!model)
    return {};
  const int idx = model->indexOfKey(key);
  return idx < 0 ? QString() : model->optCategory(idx);
}

QString ConfigViewModel::globalModifiedGroup(const QString &key) const
{
  ConfigOptionModel *model = optionModelForTier(normalizedTier(activePresetTier_));
  if (!model)
    return {};
  const int idx = model->indexOfKey(key);
  return idx < 0 ? QString() : model->optGroup(idx);
}

QString ConfigViewModel::globalModifiedLabel(const QString &key) const
{
  ConfigOptionModel *model = optionModelForTier(normalizedTier(activePresetTier_));
  if (!model)
    return key;
  const int idx = model->indexOfKey(key);
  return idx < 0 ? key : model->optLabel(idx);
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

bool ConfigViewModel::restoreAllSystemValues()
{
  if (!presetService_)
    return false;
  const QString tier = normalizedTier(activePresetTier_);
  QString presetName;
  if (tier == QStringLiteral("printer"))
    presetName = currentPrinterPreset_;
  else if (tier == QStringLiteral("filament"))
    presetName = currentFilamentPreset_;
  else
    presetName = currentPrintPreset_;
  if (presetName.isEmpty())
    return false;

  const auto before = effectivePresetValuesForTier(tier);
  const auto systemValues = presetService_->presetSystemValues(presetName);
  if (systemValues.isEmpty())
    return false;
  if (tier == QStringLiteral("printer"))
    printerPresetValues_ = systemValues;
  else if (tier == QStringLiteral("filament"))
    filamentPresetValues_ = systemValues;
  else
    printPresetValues_ = systemValues;
  updateMergedPresetValues();
  applyScopeValues();
  emit stateChanged();
  if (before != effectivePresetValuesForTier(tier))
    emit sliceAffectingConfigChanged();
  return true;
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
