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

  /// 获取指定预设的值映射（不存在则返回空）
  QHash<QString, QVariant> presetValues(const QString &presetName) const;
  /// 保存当前值到指定预设
  void savePresetValues(const QString &presetName, const QHash<QString, QVariant> &values);
  /// 检查指定预设是否存在
  bool hasPreset(const QString &presetName) const;

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
  /// 查找第一个兼容的耗材预设（对齐上游 PresetBundle::update_compatible 自动匹配）
  QString findCompatibleFilament(const QString &printerName) const;

private:
  /// 预设值存储（预设名 → key-value 映射）
  QMap<QString, QHash<QString, QVariant>> m_presetStore;
  /// 内置预设名集合（不可删除）
  QSet<QString> m_builtinPresetNames;
  /// 按类别存储的预设名列表（category → names）
  QMap<int, QStringList> m_categoryPresets;

  /// 初始化内置默认预设值（对齐上游 PresetBundle 默认值）
  void initBuiltinDefaults();
};
