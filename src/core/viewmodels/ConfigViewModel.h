#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QList>
#include <QVariant>

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
  Q_PROPERTY(int printSpeed READ printSpeed NOTIFY stateChanged)
  Q_PROPERTY(bool supportEnabled READ supportEnabled NOTIFY stateChanged)
  Q_PROPERTY(int infillDensity READ infillDensity NOTIFY stateChanged)
  Q_PROPERTY(int nozzleTemp READ nozzleTemp NOTIFY stateChanged)
  Q_PROPERTY(int bedTemp READ bedTemp NOTIFY stateChanged)
  Q_PROPERTY(int wallCount READ wallCount NOTIFY stateChanged)
  Q_PROPERTY(int topLayers READ topLayers NOTIFY stateChanged)
  Q_PROPERTY(int bottomLayers READ bottomLayers NOTIFY stateChanged)
  Q_PROPERTY(bool enableBrim READ enableBrim NOTIFY stateChanged)
  Q_PROPERTY(QObject *printOptions READ printOptions CONSTANT)
  Q_PROPERTY(QObject *machineOptions READ machineOptions CONSTANT)
  Q_PROPERTY(QObject *filamentOptions READ filamentOptions CONSTANT)
  Q_PROPERTY(QObject *presetList READ presetList CONSTANT)
  Q_PROPERTY(bool isPresetDirty READ isPresetDirty NOTIFY stateChanged)
  Q_PROPERTY(QString settingsScope READ settingsScope NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)
  Q_PROPERTY(QString activePresetTier READ activePresetTier NOTIFY stateChanged)
  Q_PROPERTY(QStringList printerPresetNames READ printerPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList filamentPresetNames READ filamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList printPresetNames READ printPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList compatibleFilamentPresetNames READ compatibleFilamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList compatiblePrintPresetNames READ compatiblePrintPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrinterPreset READ currentPrinterPreset NOTIFY stateChanged)
  Q_PROPERTY(QString currentFilamentPreset READ currentFilamentPreset NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrintPreset READ currentPrintPreset NOTIFY stateChanged)
  Q_PROPERTY(bool currentPresetCombinationValid READ currentPresetCombinationValid NOTIFY stateChanged)
  Q_PROPERTY(QString currentPresetCompatibilityMessage READ currentPresetCompatibilityMessage NOTIFY stateChanged)
  Q_PROPERTY(QString pendingUnsavedAction READ pendingUnsavedAction NOTIFY stateChanged)
  Q_PROPERTY(QString pendingUnsavedTarget READ pendingUnsavedTarget NOTIFY stateChanged)
  Q_PROPERTY(bool hasPendingUnsavedChanges READ hasPendingUnsavedChanges NOTIFY stateChanged)
  Q_PROPERTY(int globalModifiedCount READ globalModifiedCount NOTIFY stateChanged)

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
  QString activePresetTier() const { return activePresetTier_; }

  QObject *printOptions() const;
  QObject *machineOptions() const;
  QObject *filamentOptions() const;
  QObject *presetList() const;

  QStringList printerPresetNames() const;
  QStringList filamentPresetNames() const;
  QStringList printPresetNames() const;
  QStringList compatibleFilamentPresetNames() const;
  QStringList compatiblePrintPresetNames() const;
  QString currentPrinterPreset() const { return currentPrinterPreset_; }
  QString currentFilamentPreset() const { return currentFilamentPreset_; }
  QString currentPrintPreset() const { return currentPrintPreset_; }
  bool currentPresetCombinationValid() const;
  QString currentPresetCompatibilityMessage() const;
  QString pendingUnsavedAction() const { return pendingUnsavedAction_; }
  QString pendingUnsavedTarget() const { return pendingUnsavedTarget_; }
  bool hasPendingUnsavedChanges() const { return !pendingUnsavedAction_.isEmpty(); }

  Q_INVOKABLE void loadDefault();
  Q_INVOKABLE void setCurrentPreset(const QString &presetName);
  Q_INVOKABLE void setCurrentPrinterPreset(const QString &name);
  Q_INVOKABLE void setCurrentFilamentPreset(const QString &name);
  Q_INVOKABLE void setCurrentPrintPreset(const QString &name);
  Q_INVOKABLE bool requestCurrentPrinterPreset(const QString &name);
  Q_INVOKABLE bool requestCurrentFilamentPreset(const QString &name);
  Q_INVOKABLE bool requestCurrentPrintPreset(const QString &name);
  Q_INVOKABLE void saveCurrentPreset();
  Q_INVOKABLE bool exportBundle(const QString &filePath) const;
  Q_INVOKABLE bool importBundle(const QString &filePath);
  bool isPresetDirty() const;
  int globalModifiedCount() const;
  Q_INVOKABLE bool createCustomPreset(int category, const QString &name);
  Q_INVOKABLE bool deletePreset(int category, const QString &name);
  Q_INVOKABLE bool renamePreset(int category, const QString &oldName, const QString &newName);
  Q_INVOKABLE bool canDeletePreset(const QString &name) const;
  Q_INVOKABLE QStringList comparePresets(const QString &presetA, const QString &presetB) const;
  Q_INVOKABLE void autoMatchFilament();
  Q_INVOKABLE bool isCurrentFilamentCompatible() const;
  Q_INVOKABLE bool isFilamentCompatible(const QString &filamentName) const;
  Q_INVOKABLE bool canUseCurrentPresetCombination() const;
  Q_INVOKABLE QString presetActionBlocker(int category, const QString &presetName, const QString &action) const;
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
  Q_INVOKABLE void activatePlateScope(int plateIndex);
  Q_INVOKABLE bool requestGlobalScope();
  Q_INVOKABLE bool requestObjectScope(const QString &targetType, const QString &targetName, int objectIndex = -1, int volumeIndex = -1);
  Q_INVOKABLE bool requestPlateScope(int plateIndex);
  Q_INVOKABLE void setActivePresetTier(const QString &tier);

  // Per-group reset (SETTINGS-05: per upstream Tab.cpp reset_group)
  Q_INVOKABLE void resetGroup(const QString &tier, const QString &groupName);
  // Per-option nullable flag proxy (delegates to optionModelForTier)
  Q_INVOKABLE bool optNullable(const QString &tier, int index) const;
  // Per-option isVector flag proxy
  Q_INVOKABLE bool optIsVector(const QString &tier, int index) const;
  // Per-option sidetext proxy
  Q_INVOKABLE QString optSidetext(const QString &tier, int index) const;
  // Group names for a given tier (delegates to optionModel->groupNames())
  Q_INVOKABLE QStringList groupNames(const QString &tier) const;
  // Per-group dirty count
  Q_INVOKABLE int dirtyCountForGroup(const QString &tier, const QString &groupName) const;

  Q_INVOKABLE bool requestSavePendingChanges();
  Q_INVOKABLE bool requestDiscardPendingChanges();
  Q_INVOKABLE bool requestCancelPendingChanges();

  Q_INVOKABLE QList<int> filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode = false) const;
  Q_INVOKABLE QList<int> moveListItem(int fromRow, int toRow) const;
  Q_INVOKABLE QList<int> searchOptions(const QString &query) const;
  Q_INVOKABLE QString valueSourceForKey(const QString &key) const;
  Q_INVOKABLE QString valueChainForKey(const QString &key) const;
  Q_INVOKABLE bool resetOptionToLevel(const QString &key, int level);
  Q_INVOKABLE QString searchResultSource(int searchIndex) const;
  Q_INVOKABLE QString searchResultPath(int searchIndex) const;
  Q_INVOKABLE QString searchResultGroup(int searchIndex) const;
  Q_INVOKABLE QString searchResultCategory(int searchIndex) const;
  Q_INVOKABLE QString searchResultPage(int searchIndex) const;
  Q_INVOKABLE QList<int> filterIndicesByCategory(const QList<int> &indices, const QString &category) const;
  Q_INVOKABLE QList<int> filterIndicesByPage(const QList<int> &indices, const QString &page) const;
  Q_INVOKABLE QString scopeDiffSummary(const QString &key) const;

  Q_INVOKABLE int scopeOverrideCount() const;
  Q_INVOKABLE QString scopeOverriddenKey(int index) const;
  Q_INVOKABLE bool resetScopeOverride(const QString &key);
  Q_INVOKABLE void resetAllScopeOverrides();

  Q_INVOKABLE QHash<QString, QVariant> mergedConfigValues() const;
  Q_INVOKABLE void applyProjectConfig(const QHash<QString, QVariant> &config);

  Q_INVOKABLE QString globalModifiedKey(int index) const;
  Q_INVOKABLE QString globalModifiedCurrentValue(const QString &key) const;
  Q_INVOKABLE QString globalModifiedDefaultValue(const QString &key) const;
  Q_INVOKABLE bool resetGlobalOption(const QString &key);
  Q_INVOKABLE void resetAllGlobalOptions();
  Q_INVOKABLE QString materialPresetName(int localIndex) const;

  Q_INVOKABLE int layerRangeCount() const;
  Q_INVOKABLE double layerRangeMinZ(int rangeIndex) const;
  Q_INVOKABLE double layerRangeMaxZ(int rangeIndex) const;
  Q_INVOKABLE bool addLayerRange(double minZ, double maxZ);
  Q_INVOKABLE bool removeLayerRange(int rangeIndex);
  Q_INVOKABLE bool setLayerRangeValue(int rangeIndex, const QString &key, const QVariant &value);
  Q_INVOKABLE QVariant layerRangeValue(int rangeIndex, const QString &key, const QVariant &fallback = QVariant()) const;

signals:
  void stateChanged();
  void sliceAffectingConfigChanged();
  void pendingUnsavedChangesRequested();
  void pendingActionApplied(const QString &action, const QString &target);
  void pendingActionCleared();
  void saveAsRequired();

private:
  PresetServiceMock *presetService_ = nullptr;
  ProjectServiceMock *projectService_ = nullptr;
  ConfigOptionModel *printOptions_ = nullptr;
  ConfigOptionModel *machineOptions_ = nullptr;
  ConfigOptionModel *filamentOptions_ = nullptr;
  PresetListModel *presetList_ = nullptr;

  void applyScopeValues();
  void handleOptionValueChanged(const QString &key, const QVariant &value);
  QVariant scopedValueForKey(const QString &key, const QVariant &fallback) const;
  QHash<QString, QVariant> buildScopeValues() const;
  QSet<QString> readonlyKeysForCurrentScope() const;
  QHash<QString, QVariant> effectivePresetValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> editableValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> referenceValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> selectedPresetValuesForTier(const QString &tier) const;
  QHash<QString, QVariant> persistedEffectiveValuesForTier(const QString &tier) const;
  ConfigOptionModel *optionModelForTier(const QString &tier) const;
  QString normalizedTier(const QString &tier) const;
  void refreshOptionModelReferences();
  void updateMergedPresetValues();
  bool queuePendingAction(const QString &action, const QString &target);
  void clearPendingAction();
  bool applyPendingAction();
  void setCurrentPresetTierValue(const QString &tier, const QString &presetName);
  void mergePresetHierarchy();

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
  QString activePresetTier_ = QStringLiteral("print");
  QString settingsScope_ = QStringLiteral("global");
  QString settingsTargetType_;
  QString settingsTargetName_;
  int settingsTargetObjectIndex_ = -1;
  int settingsTargetVolumeIndex_ = -1;
  int settingsTargetPlateIndex_ = -1;
  bool applyingScopeValues_ = false;
  QHash<QString, QVariant> globalOptionValues_;
  QSet<QString> scopedWritableKeys_;

  QString currentPrinterPreset_;
  QString currentFilamentPreset_;
  QString currentPrintPreset_;
  QHash<QString, QVariant> printerPresetValues_;
  QHash<QString, QVariant> filamentPresetValues_;
  QHash<QString, QVariant> printPresetValues_;
  QHash<QString, QString> valueSources_;
  QString pendingUnsavedAction_;
  QString pendingUnsavedTarget_;
  mutable QList<int> m_lastSearchResults_;
};
