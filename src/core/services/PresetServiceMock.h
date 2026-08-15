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

  /// Phase 199 (WIZ-01): vendor/model enumeration for the ConfigWizard.
  /// These reuse the already-parsed m_presetMetadata / m_categoryPresets
  /// (no re-parse of vendor JSON). All enumerations exclude the internal
  /// "__upstream_defaults__" sink (which lives only in m_presetStore, never
  /// in the metadata/category maps, so it is naturally absent here).
  /// Mirrors the upstream ConfigWizard vendor/printer/filament picker model.
  /// Return distinct vendor names across all categories. With only the
  /// built-in bundle this yields "OWzx Builtin"; once vendor JSON (e.g.
  /// Creality) is loaded it yields "Creality".
  Q_INVOKABLE QStringList vendors() const;
  /// Printer (machine) preset names whose vendor matches `vendor`.
  Q_INVOKABLE QStringList printerModelsForVendor(const QString &vendor) const;
  /// Filament preset names whose vendor matches `vendor`.
  Q_INVOKABLE QStringList materialsForVendor(const QString &vendor) const;
  /// Supported bed-surface types for a printer model. The upstream bundle
  /// does not yet expose per-model bed-surface metadata in machineEntries,
  /// so this returns a 4-entry default list (mirrors the prior mock). Data
  /// gap documented in PLAN.md. Falls back to the default list when the
  /// model is unknown.
  Q_INVOKABLE QStringList bedTypesForPrinterModel(const QString &model) const;
  /// v5.15 (BEDTEX): absolute path of the printer preset's bed texture image
  /// (.png/.svg from the linked machine_model JSON, resolved against the
  /// vendor profile dir). Empty when the preset has no usable texture.
  /// Mirrors upstream PresetUtils::system_printer_bed_texture.
  Q_INVOKABLE QString bedTextureFileForPreset(const QString &presetName) const;
  /// v5.16 (BEDMODEL): absolute path of the preset's bed_model STL (the 3D
  /// printer frame, upstream Bed3D::render_model). Empty when unavailable.
  Q_INVOKABLE QString bedModelFileForPreset(const QString &presetName) const;
  /// v5.16 (BEDTYPE-TEX): upstream enables the bed-type texture system only
  /// for BBL-vendor printers (Tab.cpp on_presets_changed is_bbl_vendor).
  Q_INVOKABLE bool isBblVendorPreset(const QString &presetName) const;
  /// v5.16 (CALI): upstream render_cali gate — BBL printer model ids
  /// BL-P001/BL-P002/C13 (Preset::has_cali_lines, Preset.cpp:754).
  Q_INVOKABLE bool presetHasCaliLines(const QString &presetName) const;
  /// v5.16: upstream resources/images dir (bed-type/cali SVG assets).
  Q_INVOKABLE QString resourcesImagesDir() const;
  /// Default bed-surface list (4 entries) shared by the wizard when a model
  /// has no per-model override. Exposed for QML/tests.
  Q_INVOKABLE QStringList defaultBedTypes() const;

  /// 获取指定预设的值映射（不存在则返回空）
  QHash<QString, QVariant> presetValues(const QString &presetName) const;
  /// 获取指定预设中单个 key 的值（不存在返回无效 QVariant）
  /// Q_INVOKABLE so the ConfigWizard can read per-preset temperatures.
  Q_INVOKABLE QVariant presetValue(const QString &presetName, const QString &key) const;
  /// 保存当前值到指定预设
  bool savePresetValues(const QString &presetName, const QHash<QString, QVariant> &values);
  /// 检查指定预设是否存在
  bool hasPreset(const QString &presetName) const;
  // v2.4 IO-04/05: 预设包导入导出（JSON 格式，简化版）
  Q_INVOKABLE bool exportBundle(const QString &filePath) const;
  Q_INVOKABLE bool importBundle(const QString &filePath);
  /// Phase 147 (PSET-01) / v5.16 (PSET2-04): upstream-compatible bundle
  /// export. Writes one JSON file per user preset under
  /// `<dirPath>/{printer,filament,process}/` in the upstream user-preset
  /// shape (Config.cpp:1390-1433 save_to_json + Preset::save
  /// type/inherits, Preset.cpp:498-536), plus an `index.json` manifest.
  /// Upstream packs this per-preset JSON tree into a .zip
  /// (ExportPresetBundleDialog); zip packaging is deferred — the directory
  /// tree + manifest is the interop unit. Returns the count of exported
  /// presets (-1 = directory creation failure).
  Q_INVOKABLE int exportBundleIni(const QString &dirPath) const;
  /// Phase 147 (PSET-01) / v5.16 (PSET2-04): upstream-compatible bundle
  /// import. Reads the export tree (index.json or per-category subdirs of
  /// upstream-shaped JSON) plus legacy flat `.ini` files. Returns the count
  /// of imported presets (-1 = directory access failure).
  Q_INVOKABLE int importBundleIni(const QString &dirPath);
  /// Phase 149 (PSET-05): compare two presets key-by-key. Returns a QVariantList
  /// of {key, valueA, valueB} maps for every key where the two presets differ
  /// (added/removed/changed). Keys present in only one preset get a missing
  /// marker ("<missing>") on the other side. Mirrors the upstream
  /// UnsavedChangesDialog diff-view mode (single-direction diff A vs B; the
  /// 3-way variant is a UI-layer concern on top of this primitive).
  Q_INVOKABLE QVariantList comparePresets(const QString &presetA, const QString &presetB) const;

  /// 创建自定义预设（对齐上游 PresetBundle::save_current_preset）
  bool createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values);
  /// v5.16 (PSET2-02): create with an explicit parent preset. The stored
  /// values start from the parent's resolved chain (resolveInheritance output
  /// captured at vendor-load time) overlaid with `values`, and the preset
  /// records `inherits` (upstream Preset::inherits). Fails when the parent
  /// does not exist.
  bool createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values,
                          const QString &inherits);
  /// v5.16 (PSET2-03): merge `values` into an existing user preset without
  /// replacing its other keys and persist to disk. This is the Transfer
  /// primitive (upstream UnsavedChangesDialog Action::Transfer,
  /// UnsavedChangesDialog.cpp:1087/2380): selected keys move onto the target
  /// preset; the source preset is never saved. Read-only targets reject.
  bool mergePresetValues(const QString &presetName, const QHash<QString, QVariant> &values);
  /// 删除自定义预设（内置预设不可删除）
  bool deletePreset(const QString &presetName);
  /// 重命名自定义预设（对齐上游 PresetBundle 重命名，内置预设不可重命名）
  bool renamePreset(const QString &oldName, const QString &newName);
  /// 检查指定预设是否为内置预设（内置不可删除）
  bool isBuiltinPreset(const QString &presetName) const;
  /// 检查耗材预设是否与打印机预设兼容（对齐上游 PresetBundle::update_compatible）
  bool isFilamentCompatibleWithPrinter(const QString &filamentName, const QString &printerName) const;
  /// Phase 222 (FIL-COLOUR): the colour hex strings of all loaded filament
  /// presets (default_filament_colour), in enumeration order. Drives the MMU
  /// gizmo's extruder colour source so the swatches and painted faces match
  /// the configured filament colours. Falls back to the PrintConfig default
  /// (#F2754E) for presets without an explicit colour.
  Q_INVOKABLE QStringList activeFilamentColours() const;
  /// Phase 224 (WIZ-04): filament presets of the given vendor that are
  /// compatible with the given printer model (对齐上游 PageMaterials 3-way
  /// filter's printer dimension). Reuses the parsed compatible_printers field.
  Q_INVOKABLE QStringList materialsForVendorAndPrinter(const QString &vendor, const QString &printerModel) const;
  Q_INVOKABLE QStringList compatiblePresetNamesForCategory(int category, const QString &printerName) const;
  /// v5.12 gap-closure: compute the N×N flush matrix from filament colours
  /// (对齐上游 WipeTowerDialog calc_flushing_volumes → FlushVolCalculator).
  /// Returns a flat QVariantList of N*N ints (row-major); flush[i*N+j] = flush
  /// volume from extruder i to extruder j. Uses the loaded filament colours.
  /// Phase 236 (DLG-02): a previously saved flush_volumes_matrix (see
  /// saveFlushVolumes) takes precedence over the colour-derived calculation,
  /// making the WipeTowerDialog OK button a real round trip.
  Q_INVOKABLE QVariantList calculateFlushMatrix() const;
  /// Phase 236 (DLG-02): persist the WipeTowerDialog flush matrix under the
  /// upstream key "flush_volumes_matrix" (PrintConfig.cpp:5049, coFloats,
  /// row-major flat N*N). Upstream stores it on the project config after the
  /// dialog closes (Plater.cpp:2125); the preset store's filament presets are
  /// the OWzx equivalent sink, so every filament preset receives the same
  /// matrix (the matrix is global to the multi-material setup, not per
  /// filament). User presets are re-persisted to the user tree.
  Q_INVOKABLE bool saveFlushVolumes(const QVariantList &rows);
  /// C++ overload (tests / non-QML callers): rows[i][j] = flush volume from
  /// extruder i to extruder j. Non-square input is rejected.
  bool saveFlushVolumes(const QList<QList<double>> &rows);
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

  /// v5.16 (PSET2-01): user preset persistence root. Defaults to
  /// QStandardPaths AppDataLocation + "/user/presets" (mirrors upstream
  /// data_dir/user/<category>, PresetBundle.cpp:565-602). create/rename/
  /// delete/save all touch this tree and the constructor loads it. Tests
  /// redirect to a throwaway directory via setUserPresetDir (changing it
  /// re-scans the new location; names already loaded are kept).
  Q_INVOKABLE QString userPresetDir() const;
  Q_INVOKABLE void setUserPresetDir(const QString &dir);

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
  /// 已加载的厂商文件名集合（loadSingleVendor 去重，避免重复解析）
  QStringList m_loadedVendors;
  /// v5.15 (BEDTEX): printer preset name -> vendor profile dir (absolute),
  /// recorded by loadSingleVendor for bed-texture asset resolution.
  QMap<QString, QString> m_presetVendorDir;
  QString validatedTexturePath(const QString &vendorDir, const QString &bedTexture) const;

  /// 初始化内置默认预设值（对齐上游 PresetBundle 默认值）
  void initBuiltinDefaults();
  bool isValidCategory(int category) const;
  void registerPresetMetadata(const QString &name, int category, bool builtin, bool readOnly,
                              const QString &vendor = QString(), const QString &settingId = QString());
  void loadSelectedPresets();
  void updateSelectedPresetFallback(int category);
  static QString selectionSettingsKey(int category);
  static QString bundleCategoryName(int category);

  // ── v5.16 (PSET2-01): user preset disk persistence ───────────────────
  /// Effective user preset root (injection dir when set, AppData default).
  QString userPresetDirResolved() const;
  /// Directory name per category under the user preset root
  /// ("printer"/"filament"/"process").
  static QString userPresetCategoryDir(int category);
  /// Filesystem-safe preset file name (upstream replaces path-hostile chars).
  static QString safePresetFileName(const QString &name);
  /// <baseDir>/<category>/<name>.json
  static QString presetJsonFilePath(const QString &baseDir, int category, const QString &name);
  /// Write one preset as upstream-shaped user JSON (Config.cpp:1390-1433
  /// save_to_json header + Preset::save type/inherits, Preset.cpp:498-536).
  static bool writePresetJsonFile(const QString &baseDir, int category, const QString &name,
                                  const QHash<QString, QVariant> &values, const QString &inherits);
  bool writeUserPresetFile(int category, const QString &name, const QHash<QString, QVariant> &values) const;
  bool removeUserPresetFile(int category, const QString &name) const;
  /// Scan <userDir>/{printer,filament,process}/*.json and register each as a
  /// user preset ahead of the system presets (upstream load_user_presets).
  void loadUserPresets();
  /// Parse one upstream-shaped user preset JSON file. Returns false when the
  /// file is not valid JSON or carries no usable name.
  bool loadUserPresetJson(const QString &filePath, int category);
  QString m_userPresetDir;
  /// Resolve the vendor profiles directory (source tree or installed).
  /// Returns empty when not found. Shared by loadVendorPresets /
  /// loadSingleVendor / availableVendorNames.
  QString resolveProfilesDir() const;

#ifdef HAS_LIBSLIC3R
  /// 从上游 vendor JSON 预设文件加载真实预设（对齐上游 PresetBundle::load_vendor_configs_from_json）
  bool loadVendorPresets();
  /// 加载单个厂商的全部预设（machine/filament/process）。厂商无关：读
  /// `<profilesDir>/<vendorFileName>.json` + 解析 `<profilesDir>/<vendorName>/`
  /// 子目录。对齐上游 ConfigWizard BundleMap::load 的单厂商路径。返回加载的
  /// 预设数（0 表示失败或空）。已加载的厂商会被跳过（m_loadedVendors 去重）。
  int loadSingleVendor(const QString &profilesDir, const QString &vendorFileName);
#endif
public:
  /// 列出 profiles 目录下所有可用厂商 JSON 文件名（不含路径和 .json 后缀）。
  /// 不加载任何预设，只扫描文件名供 ConfigWizard 厂商选择器使用。
  Q_INVOKABLE QStringList availableVendorNames() const;
  /// 按需加载一个厂商的预设（对齐上游 ConfigWizard 选厂商时加载该厂商
  /// bundle）。幂等：已加载的厂商返回 true 不重复加载。返回 false 表示
  /// profiles 目录未找到或解析失败。
  Q_INVOKABLE bool loadVendor(const QString &vendorName);
  /// AppConfig-lite: 用户在 ConfigWizard 选的厂商（QSettings wizard/selectedVendor）
  Q_INVOKABLE QString selectedVendor() const;
  Q_INVOKABLE void setSelectedVendor(const QString &vendor);
  /// AppConfig-lite: 用户在 ConfigWizard 选的打印机型号（QSettings wizard/selectedPrinterModel）
  Q_INVOKABLE QString selectedPrinterModel() const;
  Q_INVOKABLE void setSelectedPrinterModel(const QString &model);
private:
#ifdef HAS_LIBSLIC3R
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
