#include "ConfigViewModel.h"

#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "qml_gui/Models/ConfigOptionModel.h"
#include "qml_gui/Models/PresetListModel.h"

ConfigViewModel::ConfigViewModel(PresetServiceMock *presetService, ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), presetService_(presetService), projectService_(projectService)
{
  printOptions_ = new ConfigOptionModel(this);
  presetList_ = new PresetListModel(this);

  scopedWritableKeys_ = {
      QStringLiteral("layer_height"),
      QStringLiteral("initial_layer_print_height"),
      QStringLiteral("line_width"),
      QStringLiteral("wall_loops"),
      QStringLiteral("top_shell_layers"),
      QStringLiteral("bottom_shell_layers"),
      QStringLiteral("sparse_infill_density"),
      QStringLiteral("sparse_infill_pattern"),
      QStringLiteral("infill_wall_overlap"),
      QStringLiteral("travel_speed"),
      QStringLiteral("initial_layer_speed"),
      QStringLiteral("outer_wall_speed"),
      QStringLiteral("nozzle_temp"),
      QStringLiteral("bed_temp"),
      QStringLiteral("chamber_temperature"),
      QStringLiteral("enable_support"),
      QStringLiteral("support_type"),
      QStringLiteral("brim_enable"),
      QStringLiteral("brim_width"),
      QStringLiteral("skirt_loops")};

  connect(printOptions_, &ConfigOptionModel::optionValueChanged, this, &ConfigViewModel::handleOptionValueChanged);
  loadDefault();
}

QObject *ConfigViewModel::printOptions() const { return printOptions_; }
QObject *ConfigViewModel::presetList() const { return presetList_; }

QStringList ConfigViewModel::presetNames() const
{
  return presetService_->presetNames();
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
  currentPreset_ = presetService_->defaultPreset();
  layerHeight_ = presetService_->defaultLayerHeight();
  printSpeed_ = 300;
  supportEnabled_ = false;
  infillDensity_ = 15;
  nozzleTemp_ = 220;
  bedTemp_ = 65;
  wallCount_ = 3;
  topLayers_ = 4;
  bottomLayers_ = 4;
  enableBrim_ = false;
  globalOptionValues_ = printOptions_->valuesByKey();
  applyScopeValues();
  emit stateChanged();
}

void ConfigViewModel::setCurrentPreset(const QString &presetName)
{
  currentPreset_ = presetName;
  emit stateChanged();
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
  settingsScope_ = targetName.isEmpty() ? QStringLiteral("global") : QStringLiteral("object");
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

  if (settingsScope_ == QStringLiteral("global") || settingsTargetObjectIndex_ < 0)
  {
    globalOptionValues_[key] = value;
    return;
  }

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
  if (settingsScope_ != QStringLiteral("object") || settingsTargetObjectIndex_ < 0)
    return values;

  for (auto it = values.begin(); it != values.end(); ++it)
    it.value() = scopedValueForKey(it.key(), it.value());
  return values;
}

QSet<QString> ConfigViewModel::readonlyKeysForCurrentScope() const
{
  if (settingsScope_ != QStringLiteral("object"))
    return {};

  QSet<QString> readonly;
  for (auto it = globalOptionValues_.cbegin(); it != globalOptionValues_.cend(); ++it)
  {
    if (!scopedWritableKeys_.contains(it.key()))
      readonly.insert(it.key());
  }
  return readonly;
}
