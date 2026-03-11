#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QStringList>

class PresetServiceMock;
class ProjectServiceMock;
class ConfigOptionModel;
class PresetListModel;

class ConfigViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY stateChanged)
  Q_PROPERTY(QString currentPreset READ currentPreset NOTIFY stateChanged)
  Q_PROPERTY(double layerHeight READ layerHeight NOTIFY stateChanged)
  // Extended print settings
  Q_PROPERTY(int printSpeed READ printSpeed NOTIFY stateChanged)
  Q_PROPERTY(bool supportEnabled READ supportEnabled NOTIFY stateChanged)
  Q_PROPERTY(int infillDensity READ infillDensity NOTIFY stateChanged)
  Q_PROPERTY(int nozzleTemp READ nozzleTemp NOTIFY stateChanged)
  Q_PROPERTY(int bedTemp READ bedTemp NOTIFY stateChanged)
  Q_PROPERTY(int wallCount READ wallCount NOTIFY stateChanged)
  Q_PROPERTY(int topLayers READ topLayers NOTIFY stateChanged)
  Q_PROPERTY(int bottomLayers READ bottomLayers NOTIFY stateChanged)
  Q_PROPERTY(bool enableBrim READ enableBrim NOTIFY stateChanged)
  // G3 — Dynamic models
  Q_PROPERTY(QObject *printOptions READ printOptions CONSTANT)
  Q_PROPERTY(QObject *presetList READ presetList CONSTANT)
  Q_PROPERTY(QString settingsScope READ settingsScope NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)

public:
  explicit ConfigViewModel(PresetServiceMock *presetService, ProjectServiceMock *projectService, QObject *parent = nullptr);

  QStringList presetNames() const;
  QString currentPreset() const;
  double layerHeight() const;

  int printSpeed() const { return printSpeed_; }
  bool supportEnabled() const { return supportEnabled_; }
  int infillDensity() const { return infillDensity_; }
  int nozzleTemp() const { return nozzleTemp_; }
  int bedTemp() const { return bedTemp_; }
  int wallCount() const { return wallCount_; }
  int topLayers() const { return topLayers_; }
  int bottomLayers() const { return bottomLayers_; }
  bool enableBrim() const { return enableBrim_; }
  QString settingsScope() const { return settingsScope_; }
  QString settingsTargetType() const { return settingsTargetType_; }
  QString settingsTargetName() const { return settingsTargetName_; }
  int settingsTargetObjectIndex() const { return settingsTargetObjectIndex_; }
  int settingsTargetVolumeIndex() const { return settingsTargetVolumeIndex_; }

  QObject *printOptions() const;
  QObject *presetList() const;

  Q_INVOKABLE void loadDefault();
  Q_INVOKABLE void setCurrentPreset(const QString &presetName);
  Q_INVOKABLE void setLayerHeight(double v);
  Q_INVOKABLE void setPrintSpeed(int v);
  Q_INVOKABLE void setSupportEnabled(bool v);
  Q_INVOKABLE void setInfillDensity(int v);
  Q_INVOKABLE void setNozzleTemp(int v);
  Q_INVOKABLE void setBedTemp(int v);
  Q_INVOKABLE void setWallCount(int v);
  Q_INVOKABLE void setEnableBrim(bool v);
  Q_INVOKABLE void activateGlobalScope();
  Q_INVOKABLE void activateObjectScope(const QString &targetType, const QString &targetName, int objectIndex = -1, int volumeIndex = -1);

signals:
  void stateChanged();

private:
  PresetServiceMock *presetService_ = nullptr;
  ProjectServiceMock *projectService_ = nullptr;
  ConfigOptionModel *printOptions_ = nullptr;
  PresetListModel *presetList_ = nullptr;

  void applyScopeValues();
  void handleOptionValueChanged(const QString &key, const QVariant &value);
  QVariant scopedValueForKey(const QString &key, const QVariant &fallback) const;
  QHash<QString, QVariant> buildScopeValues() const;
  QSet<QString> readonlyKeysForCurrentScope() const;

  QString currentPreset_;
  double layerHeight_ = 0.2;
  int printSpeed_ = 300;
  bool supportEnabled_ = false;
  int infillDensity_ = 15;
  int nozzleTemp_ = 220;
  int bedTemp_ = 65;
  int wallCount_ = 3;
  int topLayers_ = 4;
  int bottomLayers_ = 4;
  bool enableBrim_ = false;
  QString settingsScope_ = QStringLiteral("global");
  QString settingsTargetType_;
  QString settingsTargetName_;
  int settingsTargetObjectIndex_ = -1;
  int settingsTargetVolumeIndex_ = -1;
  bool applyingScopeValues_ = false;
  QHash<QString, QVariant> globalOptionValues_;
  QSet<QString> scopedWritableKeys_;
};
