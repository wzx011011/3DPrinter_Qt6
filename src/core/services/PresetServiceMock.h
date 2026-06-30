#pragma once

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QMap>
#include <QSet>

class PresetServiceMock final : public QObject
{
  Q_OBJECT

public:
  explicit PresetServiceMock(QObject *parent = nullptr);

  /// Preset categories (对齐上游 PresetBundle: print/filament/printer)
  enum Category { PrintCat = 0, FilamentCat = 1, PrinterCat = 2 };

  Q_INVOKABLE QStringList presetNames() const; // legacy flat list
  Q_INVOKABLE QString defaultPreset() const;
  Q_INVOKABLE double defaultLayerHeight() const;

  /// Category-aware preset lists
  Q_INVOKABLE QStringList presetNamesForCategory(int category) const;
  Q_INVOKABLE QString defaultPresetForCategory(int category) const;
  Q_INVOKABLE int presetCategory(const QString &presetName) const;
  Q_INVOKABLE bool isReadOnlyPreset(const QString &presetName) const;
  Q_INVOKABLE bool isUserPreset(const QString &presetName) const;
  Q_INVOKABLE int presetValueCount(const QString &presetName) const;
  Q_INVOKABLE QString presetVendor(const QString &presetName) const;
  Q_INVOKABLE QString presetSettingId(const QString &presetName) const;
  Q_INVOKABLE bool setSelectedPresetForCategory(int category, const QString &presetName);
  Q_INVOKABLE QString selectedPresetForCategory(int category) const;

  /// 获取指定预设的值映射（不存在则返回空）
  QHash<QString, QVariant> presetValues(const QString &presetName) const;
  /// 获取指定预设中单个 key 的值（不存在返回无效 QVariant）
  QVariant presetValue(const QString &presetName, const QString &key) const;
  /// 保存当前值到指定预设
  bool savePresetValues(const QString &presetName, const QHash<QString, QVariant> &values);
  /// 检查指定预设是否存在
  bool hasPreset(const QString &presetName) const;
  // v2.4 IO-04/05: 预设包导入导出（JSON 格式，简化版）
  Q_INVOKABLE bool exportBundle(const QString &filePath) const;
  Q_INVOKABLE bool importBundle(const QString &filePath);

  /// 创建自定义预设（对齐上游 PresetBundle::save_current_preset）
  bool createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values);
  /// 删除自定义预设（内置预设不可删除）
  bool deletePreset(const QString &presetName);
  /// 重命名自定义预设（对齐上游 PresetBundle 重命名，内置预设不可重命名）
  bool renamePreset(const QString &oldName, const QString &newName);
  /// 检查指定预设是否为内置预设（内置不可删除）
  bool isBuiltinPreset(const QString &presetName) const;
  /// 检查耗材预设是否与打印机预设兼容（对齐上游 PresetBundle::update_compatible）
  bool isFilamentCompatibleWithPrinter(const QString &filamentName, const QString &printerName) const;
  Q_INVOKABLE QStringList compatiblePresetNamesForCategory(int category, const QString &printerName) const;
  Q_INVOKABLE bool isPresetCompatibleWithPrinter(int category, const QString &presetName, const QString &printerName) const;
  Q_INVOKABLE QString presetCompatibilityMessage(int category, const QString &presetName, const QString &printerName) const;
  Q_INVOKABLE bool isCurrentSelectionCompatible(const QString &printerName, const QString &filamentName, const QString &printName) const;
  Q_INVOKABLE QString currentSelectionCompatibilityMessage(const QString &printerName, const QString &filamentName, const QString &printName) const;
  Q_INVOKABLE QString presetActionBlocker(int category, const QString &presetName, const QString &action) const;
  QString findCompatiblePresetForCategory(int category, const QString &printerName) const;
  /// 查找第一个兼容的耗材预设（对齐上游 PresetBundle::update_compatible 自动匹配）
  QString findCompatibleFilament(const QString &printerName) const;

  /// 获取预设继承的父预设名（对齐上游 Preset::inherits）
  QString presetInherits(const QString &presetName) const;

private:
  struct PresetMetadata
  {
    int category = -1;
    bool builtin = false;
    bool readOnly = false;
    QString vendor;
    QString settingId;
  };

  /// 预设值存储（预设名 → key-value 映射）
  QMap<QString, QHash<QString, QVariant>> m_presetStore;
  /// 内置预设名集合（不可删除）
  QSet<QString> m_builtinPresetNames;
  /// 按类别存储的预设名列表（category → names）
  QMap<int, QStringList> m_categoryPresets;
  /// 预设继承关系（预设名 → 父预设名）
  QMap<QString, QString> m_presetInherits;
  QMap<QString, PresetMetadata> m_presetMetadata;
  QMap<int, QString> m_selectedPresets;

  /// 初始化内置默认预设值（对齐上游 PresetBundle 默认值）
  void initBuiltinDefaults();
  bool isValidCategory(int category) const;
  void registerPresetMetadata(const QString &name, int category, bool builtin, bool readOnly,
                              const QString &vendor = QString(), const QString &settingId = QString());
  void loadSelectedPresets();
  void updateSelectedPresetFallback(int category);
  static QString selectionSettingsKey(int category);
  static QString bundleCategoryName(int category);

#ifdef HAS_LIBSLIC3R
  /// 从上游 vendor JSON 预设文件加载真实预设（对齐上游 PresetBundle::load_vendor_configs_from_json）
  bool loadVendorPresets();
  /// 从上游 print_config_def schema 提取所有默认值到 __upstream_defaults__
  void loadUpstreamSchemaDefaults();
  /// 加载单个预设 JSON 文件并解析继承链
  QHash<QString, QVariant> loadPresetJson(const QString &filePath, int category);
  /// 递归解析继承链，返回合并后的完整配置
  QHash<QString, QVariant> resolveInheritance(const QString &presetName, const QString &filePath,
                                               QMap<QString, QHash<QString, QVariant>> &resolvedConfigs,
                                               QMap<QString, QString> &inheritMap);
#endif
};
