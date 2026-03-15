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
  /// 当前预设是否已被修改（与保存时不同）
  Q_PROPERTY(bool isPresetDirty READ isPresetDirty NOTIFY stateChanged)
  Q_PROPERTY(QString settingsScope READ settingsScope NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetType READ settingsTargetType NOTIFY stateChanged)
  Q_PROPERTY(QString settingsTargetName READ settingsTargetName NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetObjectIndex READ settingsTargetObjectIndex NOTIFY stateChanged)
  Q_PROPERTY(int settingsTargetVolumeIndex READ settingsTargetVolumeIndex NOTIFY stateChanged)
  /// 3-tier preset inheritance (对齐上游 PresetBundle)
  Q_PROPERTY(QStringList printerPresetNames READ printerPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList filamentPresetNames READ filamentPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QStringList printPresetNames READ printPresetNames NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrinterPreset READ currentPrinterPreset NOTIFY stateChanged)
  Q_PROPERTY(QString currentFilamentPreset READ currentFilamentPreset NOTIFY stateChanged)
  Q_PROPERTY(QString currentPrintPreset READ currentPrintPreset NOTIFY stateChanged)

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

  // 3-tier preset accessors
  QStringList printerPresetNames() const;
  QStringList filamentPresetNames() const;
  QStringList printPresetNames() const;
  QString currentPrinterPreset() const { return currentPrinterPreset_; }
  QString currentFilamentPreset() const { return currentFilamentPreset_; }
  QString currentPrintPreset() const { return currentPrintPreset_; }

  Q_INVOKABLE void loadDefault();
  Q_INVOKABLE void setCurrentPreset(const QString &presetName);
  /// 3-tier preset setters (对齐上游 PresetBundle tab selection)
  Q_INVOKABLE void setCurrentPrinterPreset(const QString &name);
  Q_INVOKABLE void setCurrentFilamentPreset(const QString &name);
  Q_INVOKABLE void setCurrentPrintPreset(const QString &name);
  /// 保存当前值为预设（对齐上游 PresetBundle::save）
  Q_INVOKABLE void saveCurrentPreset();
  /// 当前预设是否被修改
  bool isPresetDirty() const;
  /// 创建自定义预设（对齐上游 PresetBundle::save_current_preset）
  Q_INVOKABLE bool createCustomPreset(int category, const QString &name);
  /// 删除预设（对齐上游 PresetBundle::delete_preset）
  Q_INVOKABLE bool deletePreset(int category, const QString &name);
  /// 重命名自定义预设（对齐上游 PresetBundle 重命名）
  Q_INVOKABLE bool renamePreset(int category, const QString &oldName, const QString &newName);
  /// 检查预设是否可删除（内置预设不可删除）
  Q_INVOKABLE bool canDeletePreset(const QString &name) const;
  /// 切换打印机时自动匹配兼容耗材（对齐上游 PresetBundle::update_compatible）
  Q_INVOKABLE void autoMatchFilament();
  /// 检查当前耗材是否与当前打印机兼容（对齐上游 PresetBundle::is_compatible）
  Q_INVOKABLE bool isCurrentFilamentCompatible() const;
  /// 检查指定耗材是否与当前打印机兼容（用于灰化不兼容耗材列表项）
  Q_INVOKABLE bool isFilamentCompatible(const QString &filamentName) const;
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
  /// 激活平板级参数作用域（对齐上游 PartPlate config override）
  Q_INVOKABLE void activatePlateScope(int plateIndex);

  /// 按分类和搜索文本过滤配置选项（替代 QML 内联 rebuildFilter 逻辑）
  Q_INVOKABLE QList<int> filterOptionIndices(const QString &category, const QString &searchText, bool advancedMode = false) const;
  /// 拖拽排序（对齐上游 DragCanvas 拖拽排序交互）
  Q_INVOKABLE QList<int> moveListItem(int fromRow, int toRow) const;
  /// 增强搜索（对齐上游 OptionsSearcher）：搜索 key/label/category/group，返回匹配的索引列表
  Q_INVOKABLE QList<int> searchOptions(const QString &query) const;
  /// 获取选项的值来源层级（对齐上游 PresetBundle 继承链）
  /// 返回 "printer"/"filament"/"print"/"default" 字符串
  Q_INVOKABLE QString valueSourceForKey(const QString &key) const;
  /// 获取选项在继承链各级的值（对齐上游 PresetBundle value_at_level）
  /// 返回 JSON 字符串: {"default":v,"printer":v,"filament":v,"print":v}
  Q_INVOKABLE QString valueChainForKey(const QString &key) const;
  /// 重置选项到指定层级（对齐上游 Tab reset_to_level）
  /// level: 0=default, 1=print, 2=filament, 3=printer
  Q_INVOKABLE bool resetOptionToLevel(const QString &key, int level);
  /// 当前搜索结果的值来源（按索引查询）
  Q_INVOKABLE QString searchResultSource(int searchIndex) const;
  /// 当前搜索结果的分类路径（按索引查询）
  Q_INVOKABLE QString searchResultPath(int searchIndex) const;
  /// 当前搜索结果的 Group（按索引查询，对齐上游 OptionsSearcher group 字段）
  Q_INVOKABLE QString searchResultGroup(int searchIndex) const;
  /// 当前搜索结果的 Category（按索引查询）
  Q_INVOKABLE QString searchResultCategory(int searchIndex) const;
  /// 当前搜索结果的 Page（按索引查询，对齐上游 Tab::Page）
  Q_INVOKABLE QString searchResultPage(int searchIndex) const;
  /// 从已有索引列表中按分类过滤（替代 QML inline groupedIndices）
  Q_INVOKABLE QList<int> filterIndicesByCategory(const QList<int> &indices, const QString &category) const;
  /// 从已有索引列表中按页面过滤（对齐上游 Tab::Page 分组）
  Q_INVOKABLE QList<int> filterIndicesByPage(const QList<int> &indices, const QString &page) const;
  /// 获取耗材丝预设名称（替代 QML inline materialPresetAt，修正分类名"耗材"匹配）
  Q_INVOKABLE QString materialPresetName(int localIndex) const;

  /// ── Layer range support (对齐上游 ModelObject::layer_config_ranges) ──
  /// 获取当前作用域对象的 layer range 数量
  Q_INVOKABLE int layerRangeCount() const;
  /// 获取指定 layer range 的 min Z
  Q_INVOKABLE double layerRangeMinZ(int rangeIndex) const;
  /// 获取指定 layer range 的 max Z
  Q_INVOKABLE double layerRangeMaxZ(int rangeIndex) const;
  /// 添加新 layer range
  Q_INVOKABLE bool addLayerRange(double minZ, double maxZ);
  /// 删除指定 layer range
  Q_INVOKABLE bool removeLayerRange(int rangeIndex);
  /// 设置指定 layer range 的配置值
  Q_INVOKABLE bool setLayerRangeValue(int rangeIndex, const QString &key, const QVariant &value);
  /// 获取指定 layer range 的配置值
  Q_INVOKABLE QVariant layerRangeValue(int rangeIndex, const QString &key, const QVariant &fallback = QVariant()) const;

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
  /// Merge 3-tier presets into globalOptionValues_ (printer → filament → print)
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
  QString settingsScope_ = QStringLiteral("global");
  QString settingsTargetType_;
  QString settingsTargetName_;
  int settingsTargetObjectIndex_ = -1;
  int settingsTargetVolumeIndex_ = -1;
  int settingsTargetPlateIndex_ = -1;
  bool applyingScopeValues_ = false;
  QHash<QString, QVariant> globalOptionValues_;
  QHash<QString, QVariant> savedPresetValues_; ///< 切换预设时保存的快照，用于脏检测
  QSet<QString> scopedWritableKeys_;

  // 3-tier preset tracking
  QString currentPrinterPreset_;
  QString currentFilamentPreset_;
  QString currentPrintPreset_;
  /// 追踪每个 key 的值来源层级（"printer"/"filament"/"print"/"default"）
  QHash<QString, QString> valueSources_;
  /// 缓存最后一次搜索结果（供 searchResultSource/searchResultPath 使用）
  mutable QList<int> m_lastSearchResults_;
};
