#include "ConfigViewModel.h"

#include "core/services/PresetServiceMock.h"
#include "qml_gui/Models/ConfigOptionModel.h"
#include "qml_gui/Models/PresetListModel.h"

ConfigViewModel::ConfigViewModel(PresetServiceMock *presetService, QObject *parent)
    : QObject(parent), presetService_(presetService)
{
  printOptions_ = new ConfigOptionModel(this);
  presetList_ = new PresetListModel(this);
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
