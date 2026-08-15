#include "PresetServiceMock.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <algorithm> // std::sort for comparePresets deterministic key ordering
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <cmath>
#include "core/FlushVolCalculator.h"  // v5.12: flush matrix calc

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#endif

namespace
{
QStringList variantStringList(const QVariant &value)
{
  QStringList result;
  if (!value.isValid())
    return result;

  if (value.typeId() == QMetaType::Type::QStringList)
  {
    result = value.toStringList();
  }
  else if (value.typeId() == QMetaType::Type::QVariantList)
  {
    const QVariantList list = value.toList();
    for (const QVariant &item : list)
    {
      const QString text = item.toString().trimmed();
      if (!text.isEmpty())
        result.append(text);
    }
  }
  else
  {
    const QString text = value.toString().trimmed();
    if (!text.isEmpty())
      result.append(text);
  }
  return result;
}

double scalarOrFirstDouble(const QVariant &value, double fallback)
{
  if (!value.isValid())
    return fallback;
  if (value.typeId() == QMetaType::Type::QVariantList)
  {
    const QVariantList list = value.toList();
    return list.isEmpty() ? fallback : list.first().toDouble();
  }
  if (value.typeId() == QMetaType::Type::QStringList)
  {
    const QStringList list = value.toStringList();
    return list.isEmpty() ? fallback : list.first().toDouble();
  }
  return value.toDouble();
}

bool isDestructivePresetAction(const QString &action)
{
  const QString normalized = action.trimmed().toLower();
  return normalized == QStringLiteral("delete") ||
         normalized == QStringLiteral("rename") ||
         normalized == QStringLiteral("save") ||
         normalized == QStringLiteral("overwrite");
}

bool hasCompatiblePrinterConstraint(const QHash<QString, QVariant> &values)
{
  return !variantStringList(values.value(QStringLiteral("compatible_printers"))).isEmpty();
}

bool explicitlyMatchesPrinter(const QHash<QString, QVariant> &values, const QString &printerName, const QString &parentPrinter)
{
  const QStringList compatiblePrinters = variantStringList(values.value(QStringLiteral("compatible_printers")));
  return compatiblePrinters.contains(printerName) ||
         (!parentPrinter.isEmpty() && compatiblePrinters.contains(parentPrinter));
}

// v5.16 (PSET2-01): header keys of the upstream user-preset JSON shape —
// ConfigBase::save_to_json writes version/name/from/is_custom (Config.cpp:1390-1433)
// and Preset::save adds type/inherits (Preset.cpp:498-536). They are metadata,
// never config values.
bool isUserPresetHeaderKey(const QString &key)
{
  return key == QStringLiteral("version") || key == QStringLiteral("name") ||
         key == QStringLiteral("from") || key == QStringLiteral("is_custom") ||
         key == QStringLiteral("type") || key == QStringLiteral("inherits") ||
         key == QStringLiteral("setting_id") || key == QStringLiteral("vendor") ||
         key == QStringLiteral("readonly");
}

// v5.16 (PSET2-01): JSON value -> QVariant with the same integral coercion
// resolveInheritance applies to string numbers (int when the value has no
// fractional part), so disk round-trips keep the in-memory QVariant types
// stable.
QVariant variantFromJsonValue(const QJsonValue &value)
{
  if (value.isBool())
    return value.toBool();
  if (value.isDouble())
  {
    const double d = value.toDouble();
    if (d == std::floor(d) && std::abs(d) < 2147483648.0)
      return static_cast<int>(d);
    return d;
  }
  if (value.isString())
    return value.toString();
  if (value.isArray())
  {
    QVariantList list;
    const QJsonArray arr = value.toArray();
    for (const QJsonValue &item : arr)
      list.append(variantFromJsonValue(item));
    return list;
  }
  return value.toVariant();
}
}

PresetServiceMock::PresetServiceMock(QObject *parent)
    : QObject(parent)
{
#ifdef HAS_LIBSLIC3R
  loadUpstreamSchemaDefaults();
  if (!loadVendorPresets())
    initBuiltinDefaults();
#else
  initBuiltinDefaults();
#endif
  // v5.16 (PSET2-01): load persisted user presets ahead of the selection
  // restore so a saved selection referencing a user preset resolves.
  loadUserPresets();
  loadSelectedPresets();
}

bool PresetServiceMock::isValidCategory(int category) const
{
  return category == PrintCat || category == FilamentCat || category == PrinterCat;
}

void PresetServiceMock::registerPresetMetadata(const QString &name, int category, bool builtin,
                                               bool readOnly, const QString &vendor,
                                               const QString &settingId)
{
  if (name.trimmed().isEmpty() || !isValidCategory(category))
    return;

  PresetMetadata metadata;
  metadata.category = category;
  metadata.builtin = builtin;
  metadata.readOnly = readOnly || builtin;
  metadata.vendor = vendor;
  metadata.settingId = settingId;
  m_presetMetadata[name] = metadata;

  if (builtin)
    m_builtinPresetNames.insert(name);
  else
    m_builtinPresetNames.remove(name);

  QStringList &names = m_categoryPresets[category];
  if (!names.contains(name))
    names.append(name);
}

QString PresetServiceMock::selectionSettingsKey(int category)
{
  switch (category)
  {
  case PrintCat: return QStringLiteral("presets/selectedPrint");
  case FilamentCat: return QStringLiteral("presets/selectedFilament");
  case PrinterCat: return QStringLiteral("presets/selectedPrinter");
  default: return {};
  }
}

QString PresetServiceMock::bundleCategoryName(int category)
{
  switch (category)
  {
  case PrintCat: return QStringLiteral("print");
  case FilamentCat: return QStringLiteral("filament");
  case PrinterCat: return QStringLiteral("printer");
  default: return {};
  }
}

void PresetServiceMock::loadSelectedPresets()
{
  QSettings settings;
  for (int category : {PrintCat, FilamentCat, PrinterCat})
  {
    const QString savedName = settings.value(selectionSettingsKey(category)).toString();
    if (!savedName.isEmpty() && presetCategory(savedName) == category)
      m_selectedPresets[category] = savedName;
    else
      updateSelectedPresetFallback(category);
  }
}

void PresetServiceMock::updateSelectedPresetFallback(int category)
{
  if (!isValidCategory(category))
    return;

  const QStringList names = m_categoryPresets.value(category);
  if (names.isEmpty())
  {
    m_selectedPresets.remove(category);
    return;
  }

  const QString current = m_selectedPresets.value(category);
  if (!names.contains(current))
    m_selectedPresets[category] = names.first();
}

// ── v5.16 (PSET2-01): user preset disk persistence ───────────────────────
// Upstream truth: Preset::save (Preset.cpp:498-536) writes each user preset
// as JSON via ConfigBase::save_to_json (Config.cpp:1390-1433) with a
// version/name/from/is_custom header (+ type/inherits), under
// data_dir/user/<category>; PresetBundle::load_user_presets
// (PresetBundle.cpp:565-602) reloads that tree at startup.

QString PresetServiceMock::userPresetDirResolved() const
{
  if (!m_userPresetDir.isEmpty())
    return m_userPresetDir;
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + QStringLiteral("/user/presets");
}

QString PresetServiceMock::userPresetDir() const
{
  return userPresetDirResolved();
}

void PresetServiceMock::setUserPresetDir(const QString &dir)
{
  if (m_userPresetDir == dir)
    return;
  m_userPresetDir = dir;
  // Re-scan: presets already in memory keep their state; the new location
  // only contributes presets not seen yet (createCustomPreset-style guard).
  loadUserPresets();
}

QString PresetServiceMock::userPresetCategoryDir(int category)
{
  switch (category)
  {
  case PrinterCat: return QStringLiteral("printer");
  case FilamentCat: return QStringLiteral("filament");
  case PrintCat: return QStringLiteral("process");
  default: return {};
  }
}

QString PresetServiceMock::safePresetFileName(const QString &name)
{
  // Upstream names preset files after the preset and escapes path-hostile
  // characters; mirror that for Windows-invalid chars.
  QString safe = name;
  const QString invalid = QStringLiteral("<>:\"/\\|?*");
  for (QChar &c : safe)
  {
    if (invalid.contains(c) || c.category() == QChar::Other_Control)
      c = QLatin1Char('_');
  }
  return safe;
}

QString PresetServiceMock::presetJsonFilePath(const QString &baseDir, int category, const QString &name)
{
  return baseDir + QStringLiteral("/") + userPresetCategoryDir(category) + QStringLiteral("/")
             + safePresetFileName(name) + QStringLiteral(".json");
}

bool PresetServiceMock::writePresetJsonFile(const QString &baseDir, int category, const QString &name,
                                            const QHash<QString, QVariant> &values, const QString &inherits)
{
  const QString dir = baseDir + QStringLiteral("/") + userPresetCategoryDir(category);
  if (!QDir().mkpath(dir))
  {
    qWarning("[Preset] writePresetJsonFile: cannot create directory %s", dir.toUtf8().constData());
    return false;
  }

  QJsonObject root;
  root[QStringLiteral("version")] = QStringLiteral("1.0.0.0");
  root[QStringLiteral("name")] = name;
  root[QStringLiteral("from")] = QStringLiteral("User");
  root[QStringLiteral("is_custom")] = QStringLiteral("1");
  // Upstream type names: "machine" for printer presets, "filament", "process".
  switch (category)
  {
  case PrinterCat: root[QStringLiteral("type")] = QStringLiteral("machine"); break;
  case FilamentCat: root[QStringLiteral("type")] = QStringLiteral("filament"); break;
  case PrintCat: root[QStringLiteral("type")] = QStringLiteral("process"); break;
  default: return false;
  }
  if (!inherits.isEmpty())
    root[QStringLiteral("inherits")] = inherits;
  for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    root[it.key()] = QJsonValue::fromVariant(it.value());

  QFile f(presetJsonFilePath(baseDir, category, name));
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    qWarning("[Preset] writePresetJsonFile: cannot write %s", f.fileName().toUtf8().constData());
    return false;
  }
  f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  f.close();
  return true;
}

bool PresetServiceMock::writeUserPresetFile(int category, const QString &name,
                                            const QHash<QString, QVariant> &values) const
{
  if (!isValidCategory(category))
    return false;
  return writePresetJsonFile(userPresetDirResolved(), category, name, values,
                             m_presetInherits.value(name));
}

bool PresetServiceMock::removeUserPresetFile(int category, const QString &name) const
{
  if (!isValidCategory(category))
    return false;
  const QString path = presetJsonFilePath(userPresetDirResolved(), category, name);
  return !QFile::exists(path) || QFile::remove(path);
}

bool PresetServiceMock::loadUserPresetJson(const QString &filePath, int category)
{
  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly))
    return false;

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject())
    return false;

  const QJsonObject root = doc.object();
  const QString name = root.value(QStringLiteral("name")).toString().trimmed();
  if (name.isEmpty() || m_presetStore.contains(name))
    return false;

  QHash<QString, QVariant> values;
  for (auto it = root.constBegin(); it != root.constEnd(); ++it)
  {
    if (isUserPresetHeaderKey(it.key()))
      continue;
    values.insert(it.key(), variantFromJsonValue(it.value()));
  }

  m_presetStore[name] = values;
  registerPresetMetadata(name, category, false, false);
  const QString inherits = root.value(QStringLiteral("inherits")).toString();
  if (!inherits.isEmpty())
    m_presetInherits[name] = inherits;
  return true;
}

void PresetServiceMock::loadUserPresets()
{
  const QString base = userPresetDirResolved();
  for (int category : {PrintCat, FilamentCat, PrinterCat})
  {
    QDir dir(base + QStringLiteral("/") + userPresetCategoryDir(category));
    if (!dir.exists())
      continue;
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QString &fileName : files)
      loadUserPresetJson(dir.absoluteFilePath(fileName), category);
  }

  // User presets list ahead of the system presets (upstream combo layout:
  // Project/User before System, PresetComboBoxes.cpp:1289-1317).
  for (auto it = m_categoryPresets.begin(); it != m_categoryPresets.end(); ++it)
  {
    QStringList userNames, systemNames;
    for (const QString &name : it.value())
    {
      if (m_presetMetadata.value(name).builtin)
        systemNames.append(name);
      else
        userNames.append(name);
    }
    if (!userNames.isEmpty())
      it.value() = userNames + systemNames;
  }
}

void PresetServiceMock::initBuiltinDefaults()
{
  // --- Printer presets (对齐上游 printer preset) ---
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("bed_temp")] = 65;
    vals[QStringLiteral("chamber_temperature")] = 0;
    vals[QStringLiteral("max_print_speed")] = 300;
    vals[QStringLiteral("retract_length")] = 0.8;
    vals[QStringLiteral("retract_speed")] = 30;
    vals[QStringLiteral("retract_length_toolchange")] = 12.0;
    vals[QStringLiteral("deretraction_speed")] = 30;
    vals[QStringLiteral("z_hop")] = 0.4;
    vals[QStringLiteral("fan_speed")] = 100;
    vals[QStringLiteral("min_fan_speed")] = 20;
    vals[QStringLiteral("nozzle_diameter")] = 0.4;
    vals[QStringLiteral("max_nozzle_temp")] = 300;
    vals[QStringLiteral("machine_max_speed")] = 600;
    const QString name = QStringLiteral("Creality K1C 0.4");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, PrinterCat, true, true, QStringLiteral("OWzx Builtin"));
  }
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("bed_temp")] = 60;
    vals[QStringLiteral("chamber_temperature")] = 0;
    vals[QStringLiteral("max_print_speed")] = 250;
    vals[QStringLiteral("retract_length")] = 0.6;
    vals[QStringLiteral("retract_speed")] = 25;
    vals[QStringLiteral("retract_length_toolchange")] = 10.0;
    vals[QStringLiteral("deretraction_speed")] = 25;
    vals[QStringLiteral("z_hop")] = 0.4;
    vals[QStringLiteral("fan_speed")] = 100;
    vals[QStringLiteral("min_fan_speed")] = 20;
    vals[QStringLiteral("nozzle_diameter")] = 0.4;
    vals[QStringLiteral("max_nozzle_temp")] = 260;
    vals[QStringLiteral("machine_max_speed")] = 500;
    const QString name = QStringLiteral("Creality Ender-3 S1");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, PrinterCat, true, true, QStringLiteral("OWzx Builtin"));
  }

  // --- Filament presets (对齐上游 filament preset) ---
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("nozzle_temp")] = 220;
    vals[QStringLiteral("nozzle_temperature_initial_layer")] = 220;
    vals[QStringLiteral("bed_temp")] = 65;
    vals[QStringLiteral("fan_speed")] = 100;
    vals[QStringLiteral("min_fan_speed")] = 20;
    vals[QStringLiteral("overhang_fan_speed")] = 80;
    vals[QStringLiteral("close_fan_the_first_x_layers")] = 1;
    vals[QStringLiteral("slow_down_layer_time")] = 8;
    vals[QStringLiteral("nozzle_temp_range_min")] = 190;
    vals[QStringLiteral("nozzle_temp_range_max")] = 230;
    vals[QStringLiteral("compatible_nozzle_min")] = 0.2;
    vals[QStringLiteral("compatible_nozzle_max")] = 0.8;
    const QString name = QStringLiteral("Creality Generic PLA");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, FilamentCat, true, true, QStringLiteral("OWzx Builtin"));
  }
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("nozzle_temp")] = 240;
    vals[QStringLiteral("nozzle_temperature_initial_layer")] = 240;
    vals[QStringLiteral("bed_temp")] = 80;
    vals[QStringLiteral("fan_speed")] = 30;
    vals[QStringLiteral("min_fan_speed")] = 10;
    vals[QStringLiteral("overhang_fan_speed")] = 30;
    vals[QStringLiteral("close_fan_the_first_x_layers")] = 3;
    vals[QStringLiteral("slow_down_layer_time")] = 15;
    vals[QStringLiteral("nozzle_temp_range_min")] = 230;
    vals[QStringLiteral("nozzle_temp_range_max")] = 270;
    vals[QStringLiteral("compatible_nozzle_min")] = 0.2;
    vals[QStringLiteral("compatible_nozzle_max")] = 0.8;
    const QString name = QStringLiteral("Creality Generic ABS");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, FilamentCat, true, true, QStringLiteral("OWzx Builtin"));
  }
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("nozzle_temp")] = 200;
    vals[QStringLiteral("nozzle_temperature_initial_layer")] = 200;
    vals[QStringLiteral("bed_temp")] = 55;
    vals[QStringLiteral("fan_speed")] = 100;
    vals[QStringLiteral("min_fan_speed")] = 30;
    vals[QStringLiteral("overhang_fan_speed")] = 100;
    vals[QStringLiteral("close_fan_the_first_x_layers")] = 1;
    vals[QStringLiteral("slow_down_layer_time")] = 8;
    vals[QStringLiteral("nozzle_temp_range_min")] = 190;
    vals[QStringLiteral("nozzle_temp_range_max")] = 250;
    vals[QStringLiteral("compatible_nozzle_min")] = 0.2;
    vals[QStringLiteral("compatible_nozzle_max")] = 0.8;
    const QString name = QStringLiteral("Creality Generic PETG");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, FilamentCat, true, true, QStringLiteral("OWzx Builtin"));
  }

  // --- Print presets (对齐上游 print preset) ---
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("layer_height")] = 0.20;
    vals[QStringLiteral("initial_layer_print_height")] = 0.20;
    vals[QStringLiteral("wall_loops")] = 3;
    vals[QStringLiteral("top_shell_layers")] = 4;
    vals[QStringLiteral("bottom_shell_layers")] = 4;
    vals[QStringLiteral("sparse_infill_density")] = 15;
    vals[QStringLiteral("sparse_infill_pattern")] = 0;
    vals[QStringLiteral("enable_support")] = false;
    vals[QStringLiteral("support_type")] = 0;
    vals[QStringLiteral("support_density")] = 15;
    vals[QStringLiteral("outer_wall_speed")] = 120;
    vals[QStringLiteral("inner_wall_speed")] = 200;
    vals[QStringLiteral("sparse_infill_speed")] = 200;
    vals[QStringLiteral("top_surface_speed")] = 100;
    vals[QStringLiteral("travel_speed")] = 300;
    vals[QStringLiteral("brim_enable")] = false;
    vals[QStringLiteral("brim_type")] = 0;
    const QString name = QStringLiteral("0.20mm Standard");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, PrintCat, true, true, QStringLiteral("OWzx Builtin"));
  }
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("layer_height")] = 0.16;
    vals[QStringLiteral("initial_layer_print_height")] = 0.16;
    vals[QStringLiteral("wall_loops")] = 3;
    vals[QStringLiteral("top_shell_layers")] = 5;
    vals[QStringLiteral("bottom_shell_layers")] = 5;
    vals[QStringLiteral("sparse_infill_density")] = 15;
    vals[QStringLiteral("sparse_infill_pattern")] = 0;
    vals[QStringLiteral("enable_support")] = false;
    vals[QStringLiteral("support_type")] = 0;
    vals[QStringLiteral("support_density")] = 15;
    vals[QStringLiteral("outer_wall_speed")] = 80;
    vals[QStringLiteral("inner_wall_speed")] = 150;
    vals[QStringLiteral("sparse_infill_speed")] = 150;
    vals[QStringLiteral("top_surface_speed")] = 60;
    vals[QStringLiteral("travel_speed")] = 250;
    vals[QStringLiteral("brim_enable")] = false;
    vals[QStringLiteral("brim_type")] = 0;
    const QString name = QStringLiteral("0.16mm Fine");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, PrintCat, true, true, QStringLiteral("OWzx Builtin"));
  }
  {
    QHash<QString, QVariant> vals;
    vals[QStringLiteral("layer_height")] = 0.28;
    vals[QStringLiteral("initial_layer_print_height")] = 0.28;
    vals[QStringLiteral("wall_loops")] = 2;
    vals[QStringLiteral("top_shell_layers")] = 3;
    vals[QStringLiteral("bottom_shell_layers")] = 3;
    vals[QStringLiteral("sparse_infill_density")] = 10;
    vals[QStringLiteral("sparse_infill_pattern")] = 0;
    vals[QStringLiteral("enable_support")] = false;
    vals[QStringLiteral("support_type")] = 0;
    vals[QStringLiteral("support_density")] = 15;
    vals[QStringLiteral("outer_wall_speed")] = 150;
    vals[QStringLiteral("inner_wall_speed")] = 250;
    vals[QStringLiteral("sparse_infill_speed")] = 250;
    vals[QStringLiteral("top_surface_speed")] = 120;
    vals[QStringLiteral("travel_speed")] = 350;
    vals[QStringLiteral("brim_enable")] = false;
    vals[QStringLiteral("brim_type")] = 0;
    const QString name = QStringLiteral("0.28mm Draft");
    m_presetStore[name] = vals;
    registerPresetMetadata(name, PrintCat, true, true, QStringLiteral("OWzx Builtin"));
  }
}

#ifdef HAS_LIBSLIC3R

bool PresetServiceMock::loadVendorPresets()
{
  // Phase 216 (WIZ-03): load the default vendor (Creality) on startup for
  // single-vendor parity. Additional vendors are loaded on demand via
  // loadVendor() from the ConfigWizard. The profiles dir is shared.
  const QString profilesDir = resolveProfilesDir();
  if (profilesDir.isEmpty())
    return false;
  return loadSingleVendor(profilesDir, QStringLiteral("Creality")) > 0;
}

QString PresetServiceMock::resolveProfilesDir() const
{
  // Locate vendor profile directory (对齐上游 PresetBundle data_dir / resource_dir).
  // Three probes so the tree resolves regardless of the process working
  // directory (double-clicking the exe in build/ leaves cwd = build/, where
  // the cwd-relative probe used to fail and silently disabled vendor
  // presets — no printer bed texture / bed model at runtime).
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList searchPaths = {
      QDir::currentPath() + QStringLiteral("/third_party/OrcaSlicer/resources/profiles"),
      appDir + QStringLiteral("/resources/profiles"),
      // Dev layout: <repo>/build/OWzxSlicer.exe -> <repo>/third_party/...
      appDir + QStringLiteral("/../third_party/OrcaSlicer/resources/profiles"),
  };
  for (const auto &path : searchPaths)
  {
    if (QFileInfo::exists(path + QStringLiteral("/Creality.json")))
      return QDir::cleanPath(path);
  }
  return {};
}

int PresetServiceMock::loadSingleVendor(const QString &profilesDir, const QString &vendorFileName)
{
  // Phase 216 (WIZ-03): vendor-agnostic single-vendor loader. Reads
  // `<profilesDir>/<vendorFileName>.json` + parses `<profilesDir>/<vendorName>/`
  // subdirectory machine/filament/process sub-files. Mirrors upstream
  // ConfigWizard BundleMap::load's per-vendor path. Returns the number of
  // presets registered (0 = failure/empty). Idempotent via m_loadedVendors.
  const QString vendorKey = vendorFileName.trimmed();
  if (m_loadedVendors.contains(vendorKey))
    return 0; // already loaded -- skip re-parse

  const QString indexFile = profilesDir + QStringLiteral("/") + vendorKey + QStringLiteral(".json");
  QFile f(indexFile);
  if (!f.open(QIODevice::ReadOnly))
    return 0;

  QJsonParseError err;
  const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &err).object();
  if (err.error != QJsonParseError::NoError)
    return 0;
  const QString vendorName = root.value(QStringLiteral("name")).toString(vendorKey);

  // vendorDir points to the vendor's subdirectory for sub_path resolution.
  // The subdir is named after the JSON FILE (e.g. BBL.json -> BBL/), NOT the
  // JSON "name" field (BBL.json has name "Bambulab" but the dir is BBL/).
  // Using vendorKey (filename) matches the on-disk layout for all 46 vendors.
  const QString vendorDir = profilesDir + QStringLiteral("/") + vendorKey;

  // Parse vendor index to get sub-file lists
  struct SubFileEntry
  {
    QString name;
    QString subPath;
  };
  auto parseSubFileList = [](const QJsonObject &root, const QString &key) -> QList<SubFileEntry> {
    QList<SubFileEntry> result;
    const QJsonArray arr = root.value(key).toArray();
    for (const QJsonValue &val : arr)
    {
      const QJsonObject obj = val.toObject();
      SubFileEntry entry;
      entry.name = obj.value(QStringLiteral("name")).toString();
      entry.subPath = obj.value(QStringLiteral("sub_path")).toString();
      if (!entry.name.isEmpty() && !entry.subPath.isEmpty())
        result.append(entry);
    }
    return result;
  };

  const QList<SubFileEntry> machineEntries = parseSubFileList(root, QStringLiteral("machine_list"));
  const QList<SubFileEntry> processEntries = parseSubFileList(root, QStringLiteral("process_list"));
  const QList<SubFileEntry> filamentEntries = parseSubFileList(root, QStringLiteral("filament_list"));

  // Resolved config cache (preset_name → merged key-values). Per-call so
  // cross-vendor inheritance collisions don't bleed (each vendor's inheritance
  // chain is self-contained within its own subdirectory).
  QMap<QString, QHash<QString, QVariant>> resolvedConfigs;
  QMap<QString, QString> inheritMap;

  int registered = 0;

  // Reusable register lambda: resolve + clean metadata + store + register.
  auto registerCategory = [&](const QList<SubFileEntry> &entries, int category) {
    for (const auto &entry : entries)
    {
      const QString filePath = vendorDir + QStringLiteral("/") + entry.subPath;
      const QHash<QString, QVariant> resolved = resolveInheritance(entry.name, filePath, resolvedConfigs, inheritMap);
      if (resolved.isEmpty())
        continue;

      // Only store presets with instantiation=true (real configs, not base templates)
      const QString instantiation = resolved.value(QStringLiteral("__instantiation__")).toString();
      if (instantiation == QStringLiteral("false"))
      {
        resolvedConfigs[entry.name] = resolved;
        continue;
      }

      QHash<QString, QVariant> cleanValues = resolved;
      cleanValues.remove(QStringLiteral("__instantiation__"));
      cleanValues.remove(QStringLiteral("__inherits__"));
      cleanValues.remove(QStringLiteral("__type__"));
      cleanValues.remove(QStringLiteral("__from__"));
      cleanValues.remove(QStringLiteral("__name__"));

      m_presetStore[entry.name] = cleanValues;
      registerPresetMetadata(entry.name, category, true, true, vendorName);
      // v5.15 (BEDTEX): remember the vendor directory so relative asset keys
      // like bed_texture can be resolved to absolute file paths (mirrors
      // upstream PresetUtils::system_printer_bed_texture's profiles-dir join).
      if (category == PrinterCat)
        m_presetVendorDir[entry.name] = vendorDir;
      if (!inheritMap.value(entry.name).isEmpty())
        m_presetInherits[entry.name] = inheritMap[entry.name];
      ++registered;
    }
  };

  registerCategory(machineEntries, PrinterCat);
  registerCategory(filamentEntries, FilamentCat);
  registerCategory(processEntries, PrintCat);

  if (registered > 0)
    m_loadedVendors.append(vendorKey);
  return registered;
}

QStringList PresetServiceMock::availableVendorNames() const
{
  // Scan the profiles dir for *.json vendor index files. Returns vendor file
  // names (without .json) for the ConfigWizard vendor picker. Does NOT load
  // any presets -- purely a filename scan (对齐 upstream BundleMap::load
  // enumerating vendor JSONs without materialising them).
  const QString profilesDir = resolveProfilesDir();
  if (profilesDir.isEmpty())
    return {};
  QDir dir(profilesDir);
  QStringList names;
  const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
  for (const QString &fn : files)
  {
    if (fn.compare(QStringLiteral("Private.json"), Qt::CaseInsensitive) == 0)
      continue; // upstream internal metadata, not a vendor index
    names.append(fn.chopped(5)); // strip ".json"
  }
  std::sort(names.begin(), names.end());
  return names;
}

bool PresetServiceMock::loadVendor(const QString &vendorName)
{
  // Phase 216 (WIZ-03): on-demand vendor load from the ConfigWizard. Idempotent
  // -- returns true when the vendor is already loaded or loads now.
  const QString key = vendorName.trimmed();
  if (m_loadedVendors.contains(key))
    return true; // already loaded
  const QString profilesDir = resolveProfilesDir();
  if (profilesDir.isEmpty())
    return false;
  return loadSingleVendor(profilesDir, key) > 0;
}

QString PresetServiceMock::selectedVendor() const
{
  // AppConfig-lite (对齐 upstream AppConfig wizard/selected_vendor).
  QSettings settings;
  return settings.value(QStringLiteral("wizard/selectedVendor")).toString();
}

void PresetServiceMock::setSelectedVendor(const QString &vendor)
{
  QSettings settings;
  settings.setValue(QStringLiteral("wizard/selectedVendor"), vendor);
}

QString PresetServiceMock::selectedPrinterModel() const
{
  QSettings settings;
  return settings.value(QStringLiteral("wizard/selectedPrinterModel")).toString();
}

QString PresetServiceMock::bedTextureFileForPreset(const QString &presetName) const
{
  // v5.15 (BEDTEX): mirrors upstream Plater::set_bed_shape ->
  // PresetUtils::system_printer_bed_texture (Preset.cpp:3561-3571). The
  // machine preset links its machine_model JSON via the printer_model key;
  // that model JSON carries the bed_texture asset filename, resolved against
  // the vendor profile directory. Only .png/.svg files that exist are
  // returned (upstream update_logo_texture_filename validation).
  const QString vendorDir = m_presetVendorDir.value(presetName);
  const QHash<QString, QVariant> resolved = m_presetStore.value(presetName);
  QString printerModel = resolved.value(QStringLiteral("printer_model")).toString();
  if (vendorDir.isEmpty() || printerModel.isEmpty()) {
    // Fallback for presets without a machine_model link: a direct relative
    // bed_texture value in the preset itself.
    const QString direct = resolved.value(QStringLiteral("bed_texture")).toString();
    if (direct.isEmpty())
      return {};
    return validatedTexturePath(vendorDir.isEmpty() ? QString() : vendorDir, direct);
  }

  const QString modelFile = vendorDir + QStringLiteral("/machine/") + printerModel
      + QStringLiteral(".json");
  QFile f(modelFile);
  if (!f.open(QIODevice::ReadOnly))
    return {};
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject())
    return {};
  const QString bedTexture = doc.object().value(QStringLiteral("bed_texture")).toString();
  if (bedTexture.isEmpty())
    return {};
  return validatedTexturePath(vendorDir, bedTexture);
}

QString PresetServiceMock::bedModelFileForPreset(const QString &presetName) const
{
  // v5.16 (BEDMODEL): mirrors the bed_texture resolution but for the
  // machine_model JSON's bed_model STL (upstream Bed3D::detect_type ->
  // PresetUtils::system_printer_bed_model path join).
  const QString vendorDir = m_presetVendorDir.value(presetName);
  const QHash<QString, QVariant> resolved = m_presetStore.value(presetName);
  QString printerModel = resolved.value(QStringLiteral("printer_model")).toString();
  if (vendorDir.isEmpty() || printerModel.isEmpty())
    return {};
  const QString modelFile = vendorDir + QStringLiteral("/machine/") + printerModel
      + QStringLiteral(".json");
  QFile f(modelFile);
  if (!f.open(QIODevice::ReadOnly))
    return {};
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject())
    return {};
  const QString bedModel = doc.object().value(QStringLiteral("bed_model")).toString();
  if (bedModel.isEmpty() || !bedModel.endsWith(QStringLiteral(".stl"), Qt::CaseInsensitive))
    return {};
  const QString path = vendorDir + QStringLiteral("/") + bedModel;
  return QFileInfo::exists(path) ? path : QString();
}

bool PresetServiceMock::isBblVendorPreset(const QString &presetName) const
{
  // Upstream Tab.cpp:1817 gate: bed-type textures render only for the BBL
  // vendor; every other vendor uses the printer's own bed_texture image.
  const QString vendorDir = m_presetVendorDir.value(presetName);
  if (vendorDir.isEmpty())
    return false;
  const QString key = QFileInfo(vendorDir).fileName();
  return key.compare(QStringLiteral("BBL"), Qt::CaseInsensitive) == 0;
}

bool PresetServiceMock::presetHasCaliLines(const QString &presetName) const
{
  // Upstream Preset::has_cali_lines (Preset.cpp:754-761): only specific BBL
  // machine model ids carry the calibration-line bed overlay.
  const QHash<QString, QVariant> resolved = m_presetStore.value(presetName);
  const QString printerModel = resolved.value(QStringLiteral("printer_model")).toString();
  return printerModel == QStringLiteral("BL-P001")
      || printerModel == QStringLiteral("BL-P002")
      || printerModel == QStringLiteral("C13");
}

QString PresetServiceMock::resourcesImagesDir() const
{
  // The shared image assets (bbl_bed_*.svg / bbl_cali_lines.svg) live in the
  // upstream resources tree next to the profiles dir.
  const QString profiles = resolveProfilesDir();
  if (profiles.isEmpty())
    return {};
  return QFileInfo(profiles).absolutePath() + QStringLiteral("/images");
}

QString PresetServiceMock::validatedTexturePath(const QString &vendorDir,
                                                const QString &bedTexture) const
{
  if (vendorDir.isEmpty())
    return {};
  if (!bedTexture.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)
      && !bedTexture.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive))
    return {};
  const QString path = vendorDir + QStringLiteral("/") + bedTexture;
  return QFileInfo::exists(path) ? path : QString();
}

void PresetServiceMock::setSelectedPrinterModel(const QString &model)
{
  QSettings settings;
  settings.setValue(QStringLiteral("wizard/selectedPrinterModel"), model);
}

void PresetServiceMock::loadUpstreamSchemaDefaults()
{
  const auto &def = Slic3r::print_config_def;
  QHash<QString, QVariant> upstreamDefaults;
  for (const auto &optPair : def.options)
  {
    const auto &optDef = optPair.second;
    if (!optDef.default_value)
      continue;
    const QString key = QString::fromUtf8(optPair.first.c_str());
    if (upstreamDefaults.contains(key))
      continue;
    switch (optDef.type)
    {
    case Slic3r::coFloat:
    case Slic3r::coFloatOrPercent:
      upstreamDefaults[key] = static_cast<const Slic3r::ConfigOptionFloat *>(optDef.default_value.get())->value;
      break;
    case Slic3r::coInt:
      upstreamDefaults[key] = static_cast<const Slic3r::ConfigOptionInt *>(optDef.default_value.get())->value;
      break;
    case Slic3r::coBool:
      upstreamDefaults[key] = static_cast<const Slic3r::ConfigOptionBool *>(optDef.default_value.get())->value != 0;
      break;
    case Slic3r::coString:
      upstreamDefaults[key] = QString::fromStdString(static_cast<const Slic3r::ConfigOptionString *>(optDef.default_value.get())->value);
      break;
    case Slic3r::coFloats:
    {
      const auto *v = static_cast<const Slic3r::ConfigOptionFloats *>(optDef.default_value.get());
      if (!v->values.empty())
      {
        QVariantList list;
        for (double val : v->values) list << val;
        upstreamDefaults[key] = list;
      }
      break;
    }
    case Slic3r::coEnum:
    {
      const auto *enumMap = optDef.enum_keys_map;
      if (enumMap)
      {
        int enumValue = static_cast<const Slic3r::ConfigOptionEnumGeneric *>(optDef.default_value.get())->value;
        for (const auto &kv : *enumMap)
        {
          if (kv.second == enumValue)
          {
            upstreamDefaults[key] = QString::fromStdString(kv.first);
            break;
          }
        }
      }
      break;
    }
    case Slic3r::coPoints:
    {
      const auto *v = static_cast<const Slic3r::ConfigOptionPoints *>(optDef.default_value.get());
      if (!v->values.empty())
      {
        QStringList parts;
        for (const auto &p : v->values)
          parts << QStringLiteral("%1x%2").arg(p.x()).arg(p.y());
        upstreamDefaults[key] = parts.join(",");
      }
      break;
    }
    case Slic3r::coInts:
    {
      const auto *v = static_cast<const Slic3r::ConfigOptionInts *>(optDef.default_value.get());
      if (!v->values.empty())
      {
        QVariantList list;
        for (int val : v->values) list << val;
        upstreamDefaults[key] = list;
      }
      break;
    }
    default:
      break;
    }
  }
  m_presetStore[QStringLiteral("__upstream_defaults__")] = upstreamDefaults;
}

QHash<QString, QVariant> PresetServiceMock::resolveInheritance(
    const QString &presetName, const QString &filePath,
    QMap<QString, QHash<QString, QVariant>> &resolvedConfigs,
    QMap<QString, QString> &inheritMap)
{
  // Check cache first
  if (resolvedConfigs.contains(presetName))
    return resolvedConfigs[presetName];

  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly))
    return {};

  QJsonParseError err;
  const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &err).object();
  if (err.error != QJsonParseError::NoError)
    return {};

  // Get inherits name
  const QString inherits = root.value(QStringLiteral("inherits")).toString();
  const QString type = root.value(QStringLiteral("type")).toString();
  const QString name = root.value(QStringLiteral("name")).toString();
  const QString instantiation = root.value(QStringLiteral("instantiation")).toString();

  // Start with parent config (if inherits is specified)
  QHash<QString, QVariant> result;
  if (!inherits.isEmpty())
  {
    inheritMap[presetName] = inherits;

    // Try to resolve parent from cache or load it
    if (resolvedConfigs.contains(inherits))
    {
      result = resolvedConfigs[inherits];
    }
    else
    {
      // Try to find and load the parent preset file.
      // Parent files are in the same subdirectory (e.g., machine/fdm_creality_common.json)
      const QFileInfo fi(filePath);
      const QDir fileDir = fi.absoluteDir();
      const QString parentFileName = inherits + QStringLiteral(".json");

      // Same directory as current file
      const QString candidate = fileDir.filePath(parentFileName);
      if (QFileInfo::exists(candidate))
        result = resolveInheritance(inherits, candidate, resolvedConfigs, inheritMap);
    }
  }

  // Apply current preset values on top of parent (child overrides parent)
  for (auto it = root.begin(); it != root.end(); ++it)
  {
    const QString key = it.key();
    // Skip metadata keys (but keep compatible_printers/compatible_prints for compatibility filtering)
    if (key == QStringLiteral("type") || key == QStringLiteral("name") ||
        key == QStringLiteral("from") || key == QStringLiteral("inherits") ||
        key == QStringLiteral("instantiation") || key == QStringLiteral("setting_id"))
      continue;

    const QJsonValue val = it.value();
    if (val.isString())
    {
      const QString str = val.toString();
      // Try to parse as number
      bool ok = false;
      const double dval = str.toDouble(&ok);
      if (ok && !str.isEmpty())
      {
        // Check if it's actually an integer
        if (str.indexOf('.') < 0 && str.indexOf('%') < 0)
          result[key] = static_cast<int>(qRound(dval));
        else
          result[key] = dval;
      }
      else
      {
        result[key] = str;
      }
    }
    else if (val.isBool())
    {
      result[key] = val.toBool();
    }
    else if (val.isDouble())
    {
      result[key] = val.toDouble();
    }
    else if (val.isArray())
    {
      // Store arrays as QVariantList
      QVariantList list;
      const QJsonArray arr = val.toArray();
      for (const QJsonValue &av : arr)
        list.append(av.toVariant());
      result[key] = list;
    }
  }

  // Store metadata
  result[QStringLiteral("__type__")] = type;
  result[QStringLiteral("__name__")] = name;
  result[QStringLiteral("__instantiation__")] = instantiation;
  result[QStringLiteral("__inherits__")] = inherits;

  resolvedConfigs[presetName] = result;
  return result;
}

#endif // HAS_LIBSLIC3R

QStringList PresetServiceMock::presetNames() const
{
  // Legacy: return print category presets
  return m_categoryPresets.value(PrintCat);
}

QString PresetServiceMock::defaultPreset() const
{
  return defaultPresetForCategory(PrintCat);
}

double PresetServiceMock::defaultLayerHeight() const
{
  return 0.2;
}

QStringList PresetServiceMock::presetNamesForCategory(int category) const
{
  return isValidCategory(category) ? m_categoryPresets.value(category) : QStringList{};
}

QString PresetServiceMock::defaultPresetForCategory(int category) const
{
  if (!isValidCategory(category))
    return {};
  const auto names = m_categoryPresets.value(category);
  if (names.isEmpty())
    return {};
  // Return first preset in the category (typically the default)
  return names.first();
}

int PresetServiceMock::presetCategory(const QString &presetName) const
{
  const auto it = m_presetMetadata.constFind(presetName);
  if (it != m_presetMetadata.constEnd())
    return it->category;

  for (auto catIt = m_categoryPresets.constBegin(); catIt != m_categoryPresets.constEnd(); ++catIt)
  {
    if (catIt.value().contains(presetName))
      return catIt.key();
  }
  return -1;
}

bool PresetServiceMock::isReadOnlyPreset(const QString &presetName) const
{
  const auto it = m_presetMetadata.constFind(presetName);
  if (it != m_presetMetadata.constEnd())
    return it->readOnly || it->builtin;
  return m_builtinPresetNames.contains(presetName);
}

bool PresetServiceMock::isUserPreset(const QString &presetName) const
{
  const auto it = m_presetMetadata.constFind(presetName);
  return it != m_presetMetadata.constEnd() && !it->builtin && !it->readOnly;
}

int PresetServiceMock::presetValueCount(const QString &presetName) const
{
  return m_presetStore.value(presetName).size();
}

QString PresetServiceMock::presetVendor(const QString &presetName) const
{
  const auto it = m_presetMetadata.constFind(presetName);
  return it == m_presetMetadata.constEnd() ? QString() : it->vendor;
}

QString PresetServiceMock::presetSettingId(const QString &presetName) const
{
  const auto it = m_presetMetadata.constFind(presetName);
  return it == m_presetMetadata.constEnd() ? QString() : it->settingId;
}

bool PresetServiceMock::setSelectedPresetForCategory(int category, const QString &presetName)
{
  if (!isValidCategory(category) || presetCategory(presetName) != category)
    return false;

  m_selectedPresets[category] = presetName;
  QSettings settings;
  settings.setValue(selectionSettingsKey(category), presetName);
  settings.sync();
  return true;
}

QString PresetServiceMock::selectedPresetForCategory(int category) const
{
  if (!isValidCategory(category))
    return {};

  const QString selected = m_selectedPresets.value(category);
  if (!selected.isEmpty() && presetCategory(selected) == category)
    return selected;
  return defaultPresetForCategory(category);
}

// Phase 199 (WIZ-01): vendor/model enumeration for the ConfigWizard.
// All helpers walk m_presetMetadata / m_categoryPresets, which only contain
// real presets. The "__upstream_defaults__" sink lives solely in
// m_presetStore and is never registered via registerPresetMetadata, so it is
// naturally excluded from every enumeration below.
QStringList PresetServiceMock::vendors() const
{
  QStringList result;
  for (auto it = m_presetMetadata.constBegin(); it != m_presetMetadata.constEnd(); ++it)
  {
    const QString vendor = it.value().vendor;
    if (!vendor.isEmpty() && !result.contains(vendor))
      result.append(vendor);
  }
  // Deterministic ordering for stable QML display.
  std::sort(result.begin(), result.end());
  return result;
}

QStringList PresetServiceMock::printerModelsForVendor(const QString &vendor) const
{
  QStringList result;
  const QString normalized = vendor.trimmed();
  const QStringList names = m_categoryPresets.value(PrinterCat);
  for (const QString &name : names)
  {
    const auto it = m_presetMetadata.constFind(name);
    if (it == m_presetMetadata.constEnd())
      continue;
    if (it.value().vendor == normalized)
      result.append(name);
  }
  return result;
}

QStringList PresetServiceMock::materialsForVendor(const QString &vendor) const
{
  QStringList result;
  const QString normalized = vendor.trimmed();
  const QStringList names = m_categoryPresets.value(FilamentCat);
  for (const QString &name : names)
  {
    const auto it = m_presetMetadata.constFind(name);
    if (it == m_presetMetadata.constEnd())
      continue;
    if (it.value().vendor == normalized)
      result.append(name);
  }
  return result;
}

QStringList PresetServiceMock::activeFilamentColours() const
{
  // Phase 222 (FIL-COLOUR): enumerate all loaded filament presets and read
  // each one's default_filament_colour (对齐 upstream Preset filament_colour /
  // default_filament_colour). This is the source the MMU gizmo's swatch and
  // painted-face colours should bind to so they stay consistent. Presets
  // without an explicit colour fall back to the PrintConfig default #F2754E.
  QStringList colours;
  const QStringList names = m_categoryPresets.value(FilamentCat);
  colours.reserve(names.size());
  for (const QString &name : names) {
    const auto it = m_presetStore.constFind(name);
    if (it == m_presetStore.constEnd()) {
      colours.append(QStringLiteral("#F2754E"));
      continue;
    }
    // default_filament_colour is coStrings (per-extruder); take the first
    // entry, matching how upstream displays the swatch.
    const QVariant v = it.value().value(QStringLiteral("default_filament_colour"));
    if (v.userType() == QMetaType::QStringList) {
      const QStringList sl = v.toStringList();
      colours.append(!sl.isEmpty() ? sl.first() : QStringLiteral("#F2754E"));
    } else if (v.userType() == QMetaType::QString) {
      colours.append(v.toString());
    } else {
      // Also try filament_colour (some presets use it directly).
      const QVariant v2 = it.value().value(QStringLiteral("filament_colour"));
      if (v2.userType() == QMetaType::QStringList) {
        const QStringList sl = v2.toStringList();
        colours.append(!sl.isEmpty() ? sl.first() : QStringLiteral("#F2754E"));
      } else if (v2.userType() == QMetaType::QString) {
        colours.append(v2.toString());
      } else {
        colours.append(QStringLiteral("#F2754E"));
      }
    }
  }
  return colours;
}

QVariantList PresetServiceMock::calculateFlushMatrix() const
{
  // v5.12 gap-closure: compute the N×N flush matrix from filament colours
  // (对齐 upstream WipeTowerDialog calc_flushing_volumes). Uses the
  // FlushVolCalculator colour-distance formula on the active filament colours.
  const QStringList colours = activeFilamentColours();
  const int n = colours.size();
  QVariantList matrix;
  if (n == 0)
    return matrix;
  matrix.reserve(n * n);
  const OWzx::FlushVolCalculator calc(/*min*/ 0, /*max*/ 800, /*multiplier*/ 1.0f);
  // Parse hex colours (#RRGGBB) to RGB bytes.
  auto parseHex = [](const QString &hex) -> std::tuple<int, int, int> {
    QString h = hex.trimmed();
    if (h.startsWith(QLatin1Char('#')))
      h = h.mid(1);
    bool ok = false;
    const unsigned int val = h.toUInt(&ok, 16);
    if (!ok || h.length() < 6)
      return {255, 255, 255}; // white fallback
    return {int((val >> 16) & 0xFF), int((val >> 8) & 0xFF), int(val & 0xFF)};
  };
  for (int i = 0; i < n; ++i) {
    const auto [sr, sg, sb] = parseHex(colours[i]);
    for (int j = 0; j < n; ++j) {
      if (i == j) { matrix.append(0); continue; }
      const auto [dr, dg, db] = parseHex(colours[j]);
      matrix.append(calc.calcFlushVol(255, sr, sg, sb, 255, dr, dg, db));
    }
  }
  return matrix;
}

QStringList PresetServiceMock::materialsForVendorAndPrinter(const QString &vendor, const QString &printerModel) const
{
  // Phase 224 (WIZ-04): filament presets of the given vendor that are
  // compatible with the given printer model (对齐上游 ConfigWizard
  // PageMaterials::update_lists printer-dimension filter). Reuses the parsed
  // compatible_printers field + isPresetCompatibleWithPrinter. When
  // printerModel is empty, falls back to the vendor-only filter.
  if (printerModel.isEmpty())
    return materialsForVendor(vendor);
  QStringList result;
  const QString normalizedVendor = vendor.trimmed();
  const QStringList names = m_categoryPresets.value(FilamentCat);
  for (const QString &name : names) {
    const auto it = m_presetMetadata.constFind(name);
    if (it == m_presetMetadata.constEnd())
      continue;
    if (it.value().vendor != normalizedVendor)
      continue;
    // A preset is compatible if it has no compatible_printers restriction
    // (universal) OR explicitly lists this printer model.
    if (isPresetCompatibleWithPrinter(FilamentCat, name, printerModel))
      result.append(name);
  }
  return result;
}

QStringList PresetServiceMock::defaultBedTypes() const
{
  // Four standard bed surfaces, matching the prior mock wizard. The upstream
  // vendor bundle does not yet carry per-model bed-surface metadata, so this
  // list is the single source of truth until that data gap is closed (see
  // PLAN.md). Names stay Chinese to preserve wizard UI parity.
  return {
      QStringLiteral("光滑 PEI 板"),
      QStringLiteral("普通 PEI 板"),
      QStringLiteral("PC 热床"),
      QStringLiteral("EP 热床"),
  };
}

QStringList PresetServiceMock::bedTypesForPrinterModel(const QString &model) const
{
  // The machineEntries payload has no bed-surface field today, so we always
  // fall back to the canonical 4-surface list. The `model` parameter is
  // accepted for API symmetry with the upstream ConfigWizard and to keep the
  // QML call site forward-compatible once per-model bed data is wired in.
  // We do validate that the model exists so callers can detect typos via the
  // returned (non-empty) list semantics matching the prior wizard.
  Q_UNUSED(model);
  return defaultBedTypes();
}

QHash<QString, QVariant> PresetServiceMock::presetValues(const QString &presetName) const
{
  return m_presetStore.value(presetName);
}

QVariant PresetServiceMock::presetValue(const QString &presetName, const QString &key) const
{
  const auto &vals = m_presetStore.value(presetName);
  return vals.value(key);
}

bool PresetServiceMock::savePresetValues(const QString &presetName, const QHash<QString, QVariant> &values)
{
  if (!m_presetStore.contains(presetName) || isReadOnlyPreset(presetName))
    return false;

  m_presetStore[presetName] = values;
  // v5.16 (PSET2-01): persist the saved preset to the user tree (upstream
  // Preset::save on every dirty-save).
  writeUserPresetFile(presetCategory(presetName), presetName, values);
  return true;
}

bool PresetServiceMock::hasPreset(const QString &presetName) const
{
  return m_presetStore.contains(presetName);
}

// v2.4 IO-04: 导出预设包（JSON 格式，简化版）
bool PresetServiceMock::exportBundle(const QString &filePath) const
{
  QJsonObject root;
  QJsonArray presets;
  int exported = 0;

  for (auto it = m_presetStore.constBegin(); it != m_presetStore.constEnd(); ++it)
  {
    const QString &name = it.key();
    const auto metaIt = m_presetMetadata.constFind(name);
    if (metaIt == m_presetMetadata.constEnd() || metaIt->builtin)
      continue;

    QJsonObject presetObj;
    presetObj[QStringLiteral("name")] = name;
    presetObj[QStringLiteral("category")] = metaIt->category;
    presetObj[QStringLiteral("categoryName")] = bundleCategoryName(metaIt->category);
    presetObj[QStringLiteral("readOnly")] = metaIt->readOnly;
    if (!metaIt->vendor.isEmpty())
      presetObj[QStringLiteral("vendor")] = metaIt->vendor;
    if (!metaIt->settingId.isEmpty())
      presetObj[QStringLiteral("settingId")] = metaIt->settingId;
    const QString inherits = m_presetInherits.value(name);
    if (!inherits.isEmpty())
      presetObj[QStringLiteral("inherits")] = inherits;

    QJsonObject values;
    for (auto vit = it.value().constBegin(); vit != it.value().constEnd(); ++vit)
      values[vit.key()] = QJsonValue::fromVariant(vit.value());
    presetObj[QStringLiteral("values")] = values;
    presets.append(presetObj);
    ++exported;
  }

  root[QStringLiteral("kind")] = QStringLiteral("owzx-preset-bundle");
  root[QStringLiteral("version")] = QStringLiteral("1.0");
  root[QStringLiteral("exported")] = QDateTime::currentDateTime().toString(Qt::ISODate);
  root[QStringLiteral("presets")] = presets;

  QFile f(filePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    qWarning("[Preset] exportBundle: cannot open %s", filePath.toUtf8().constData());
    return false;
  }
  f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  f.close();
  qDebug("[Preset] exported %d user presets to: %s", exported, filePath.toUtf8().constData());
  return true;
}

// v2.4 IO-05: 导入预设包（JSON 格式）
bool PresetServiceMock::importBundle(const QString &filePath)
{
  QFile f(filePath);
  if (!f.open(QIODevice::ReadOnly))
  {
    qWarning("[Preset] importBundle: cannot open %s", filePath.toUtf8().constData());
    return false;
  }

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  f.close();
  if (err.error != QJsonParseError::NoError || !doc.isObject())
  {
    qWarning("[Preset] importBundle: invalid JSON: %s", err.errorString().toUtf8().constData());
    return false;
  }

  const QJsonObject root = doc.object();
  if (root.value(QStringLiteral("kind")).toString() != QStringLiteral("owzx-preset-bundle") ||
      root.value(QStringLiteral("version")).toString() != QStringLiteral("1.0"))
  {
    qWarning("[Preset] importBundle: unsupported bundle kind or version");
    return false;
  }

  const QJsonValue presetsValue = root.value(QStringLiteral("presets"));
  if (!presetsValue.isArray())
  {
    qWarning("[Preset] importBundle: presets array missing");
    return false;
  }

  struct ImportPreset
  {
    QString name;
    int category = -1;
    bool readOnly = false;
    QString vendor;
    QString settingId;
    QString inherits;
    QHash<QString, QVariant> values;
  };

  QList<ImportPreset> pending;
  QSet<QString> bundleNames;
  const QJsonArray presets = presetsValue.toArray();
  for (const QJsonValue &presetValue : presets)
  {
    if (!presetValue.isObject())
    {
      qWarning("[Preset] importBundle: preset entry must be an object");
      return false;
    }

    const QJsonObject presetObj = presetValue.toObject();
    ImportPreset item;
    item.name = presetObj.value(QStringLiteral("name")).toString().trimmed();
    item.category = presetObj.value(QStringLiteral("category")).toInt(-1);
    item.readOnly = false;
    item.vendor = presetObj.value(QStringLiteral("vendor")).toString();
    item.settingId = presetObj.value(QStringLiteral("settingId")).toString();
    item.inherits = presetObj.value(QStringLiteral("inherits")).toString();

    if (item.name.isEmpty() || !isValidCategory(item.category))
    {
      qWarning("[Preset] importBundle: invalid preset name or category");
      return false;
    }
    if (m_presetStore.contains(item.name) || bundleNames.contains(item.name))
    {
      qWarning("[Preset] importBundle: duplicate preset %s", item.name.toUtf8().constData());
      return false;
    }
    if (!presetObj.value(QStringLiteral("values")).isObject())
    {
      qWarning("[Preset] importBundle: values object missing for %s", item.name.toUtf8().constData());
      return false;
    }

    const QJsonObject valuesObj = presetObj.value(QStringLiteral("values")).toObject();
    for (auto vit = valuesObj.constBegin(); vit != valuesObj.constEnd(); ++vit)
      item.values[vit.key()] = vit.value().toVariant();

    bundleNames.insert(item.name);
    pending.append(item);
  }

  for (const ImportPreset &item : pending)
  {
    m_presetStore[item.name] = item.values;
    registerPresetMetadata(item.name, item.category, false, item.readOnly, item.vendor, item.settingId);
    if (!item.inherits.isEmpty())
      m_presetInherits[item.name] = item.inherits;
    // v5.16 (PSET2-04): imported user presets persist to disk like any
    // user-created preset (upstream import lands in the user preset tree).
    writeUserPresetFile(item.category, item.name, item.values);
  }

  qDebug("[Preset] imported %d presets from: %s", int(pending.size()), filePath.toUtf8().constData());
  return true;
}

bool PresetServiceMock::isBuiltinPreset(const QString &presetName) const
{
  return m_builtinPresetNames.contains(presetName);
}

bool PresetServiceMock::isFilamentCompatibleWithPrinter(const QString &filamentName, const QString &printerName) const
{
  return isPresetCompatibleWithPrinter(FilamentCat, filamentName, printerName);
}

QString PresetServiceMock::findCompatibleFilament(const QString &printerName) const
{
  return findCompatiblePresetForCategory(FilamentCat, printerName);
}

QStringList PresetServiceMock::compatiblePresetNamesForCategory(int category, const QString &printerName) const
{
  if (!isValidCategory(category))
    return {};

  QStringList result;
  const QStringList names = m_categoryPresets.value(category);
  for (const QString &name : names)
  {
    if (isPresetCompatibleWithPrinter(category, name, printerName))
      result.append(name);
  }
  return result;
}

bool PresetServiceMock::isPresetCompatibleWithPrinter(int category, const QString &presetName, const QString &printerName) const
{
  return presetCompatibilityMessage(category, presetName, printerName).isEmpty();
}

QString PresetServiceMock::presetCompatibilityMessage(int category, const QString &presetName, const QString &printerName) const
{
  if (!isValidCategory(category))
    return tr("Unknown preset category.");

  const int actualCategory = presetCategory(presetName);
  if (actualCategory < 0 || !m_presetStore.contains(presetName))
    return tr("Preset \"%1\" does not exist.").arg(presetName);
  if (actualCategory != category)
    return tr("Preset \"%1\" belongs to another category.").arg(presetName);

  const int printerCategory = presetCategory(printerName);
  if (printerCategory < 0 || !m_presetStore.contains(printerName))
    return tr("Printer preset \"%1\" does not exist.").arg(printerName);
  if (printerCategory != PrinterCat)
    return tr("\"%1\" is not a printer preset.").arg(printerName);

  if (category == PrinterCat)
    return presetName == printerName ? QString() :
        tr("Printer preset \"%1\" is not the active printer.").arg(presetName);

  const QHash<QString, QVariant> &values = m_presetStore[presetName];
  const QHash<QString, QVariant> &printerValues = m_presetStore[printerName];

  const QString condition = values.value(QStringLiteral("compatible_printers_condition")).toString().trimmed();
  if (!condition.isEmpty())
    return tr("Preset \"%1\" uses unsupported printer compatibility expression.").arg(presetName);

  const QStringList compatiblePrinters = variantStringList(values.value(QStringLiteral("compatible_printers")));
  if (!compatiblePrinters.isEmpty())
  {
    const QString parentPrinter = m_presetInherits.value(printerName);
    if (!compatiblePrinters.contains(printerName) &&
        (parentPrinter.isEmpty() || !compatiblePrinters.contains(parentPrinter)))
    {
      return tr("Preset \"%1\" is not compatible with printer \"%2\".").arg(presetName, printerName);
    }
  }

  if (category == FilamentCat)
  {
    const double nozzleMin = values.value(QStringLiteral("compatible_nozzle_min"), 0.15).toDouble();
    const double nozzleMax = values.value(QStringLiteral("compatible_nozzle_max"), 1.0).toDouble();
    const double printerNozzle = scalarOrFirstDouble(printerValues.value(QStringLiteral("nozzle_diameter")), 0.4);
    if (printerNozzle < nozzleMin || printerNozzle > nozzleMax)
      return tr("Filament \"%1\" requires nozzle %2-%3 mm; active printer uses %4 mm.")
          .arg(presetName)
          .arg(nozzleMin)
          .arg(nozzleMax)
          .arg(printerNozzle);

    const double filamentMaxTemp = values.value(QStringLiteral("nozzle_temp_range_max"), 300).toDouble();
    const double printerMaxTemp = printerValues.value(QStringLiteral("max_nozzle_temp"), 300).toDouble();
    if (filamentMaxTemp > printerMaxTemp)
      return tr("Filament \"%1\" requires nozzle temperature up to %2 C; active printer max is %3 C.")
          .arg(presetName)
          .arg(filamentMaxTemp)
          .arg(printerMaxTemp);
  }

  return {};
}

bool PresetServiceMock::isCurrentSelectionCompatible(const QString &printerName, const QString &filamentName, const QString &printName) const
{
  return currentSelectionCompatibilityMessage(printerName, filamentName, printName).isEmpty();
}

QString PresetServiceMock::currentSelectionCompatibilityMessage(const QString &printerName, const QString &filamentName, const QString &printName) const
{
  const QString printerMessage = presetCompatibilityMessage(PrinterCat, printerName, printerName);
  if (!printerMessage.isEmpty())
    return printerMessage;

  const QString printMessage = presetCompatibilityMessage(PrintCat, printName, printerName);
  if (!printMessage.isEmpty())
    return printMessage;

  const QString filamentMessage = presetCompatibilityMessage(FilamentCat, filamentName, printerName);
  if (!filamentMessage.isEmpty())
    return filamentMessage;

  return {};
}

QString PresetServiceMock::presetActionBlocker(int category, const QString &presetName, const QString &action) const
{
  if (!isValidCategory(category))
    return tr("Unknown preset category.");
  if (!m_presetStore.contains(presetName))
    return tr("Preset \"%1\" does not exist.").arg(presetName);
  if (presetCategory(presetName) != category)
    return tr("Preset \"%1\" belongs to another category.").arg(presetName);
  if (isDestructivePresetAction(action) && isReadOnlyPreset(presetName))
    return tr("Preset \"%1\" is built-in or read-only. Use Save As to create an editable copy.").arg(presetName);
  return {};
}

QString PresetServiceMock::findCompatiblePresetForCategory(int category, const QString &printerName) const
{
  if (!isValidCategory(category))
    return {};

  const QString parentPrinter = m_presetInherits.value(printerName);
  QString firstGeneric;
  const QStringList names = m_categoryPresets.value(category);
  for (const QString &name : names)
  {
    if (!isPresetCompatibleWithPrinter(category, name, printerName))
      continue;

    const QHash<QString, QVariant> values = m_presetStore.value(name);
    if (hasCompatiblePrinterConstraint(values) &&
        explicitlyMatchesPrinter(values, printerName, parentPrinter))
      return name;

    if (firstGeneric.isEmpty())
      firstGeneric = name;
  }
  return firstGeneric;
}

// v5.16 (PSET2-04): upstream-compatible bundle export. Each user preset is
// written as one upstream-shaped JSON file (Config.cpp:1390-1433 save_to_json
// header + Preset::save type/inherits, Preset.cpp:498-536) under
// <dirPath>/{printer,filament,process}/, plus an index.json manifest.
// Upstream ExportPresetBundleDialog packs the per-preset JSON tree into a
// .zip; zip packaging is deferred here — the directory tree + manifest is
// the interop unit (importBundleIni reads either).
int PresetServiceMock::exportBundleIni(const QString &dirPath) const
{
  QDir dir(dirPath);
  if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
  {
    qWarning("[Preset] exportBundleIni: cannot create directory %s", dirPath.toUtf8().constData());
    return -1;
  }

  QJsonArray manifestPresets;
  int exported = 0;
  for (auto it = m_presetStore.constBegin(); it != m_presetStore.constEnd(); ++it)
  {
    const QString &name = it.key();
    const auto metaIt = m_presetMetadata.constFind(name);
    if (metaIt == m_presetMetadata.constEnd() || metaIt->builtin)
      continue;

    const QString categoryDir = userPresetCategoryDir(metaIt->category);
    if (!writePresetJsonFile(dirPath, metaIt->category, name, it.value(),
                             m_presetInherits.value(name)))
    {
      qWarning("[Preset] exportBundleIni: cannot write preset %s", name.toUtf8().constData());
      continue;
    }

    QJsonObject entry;
    entry[QStringLiteral("name")] = name;
    entry[QStringLiteral("category")] = metaIt->category;
    entry[QStringLiteral("categoryName")] = bundleCategoryName(metaIt->category);
    entry[QStringLiteral("file")] = categoryDir + QStringLiteral("/")
                                        + safePresetFileName(name) + QStringLiteral(".json");
    manifestPresets.append(entry);
    ++exported;
  }

  // Manifest: upstream bundles carry a machine index; ours lists the exported
  // preset files so imports stay name/category-exact without path guessing.
  QJsonObject manifest;
  manifest[QStringLiteral("kind")] = QStringLiteral("owzx-preset-bundle-dir");
  manifest[QStringLiteral("version")] = QStringLiteral("1.0");
  manifest[QStringLiteral("exported")] = QDateTime::currentDateTime().toString(Qt::ISODate);
  manifest[QStringLiteral("presets")] = manifestPresets;
  QFile manifestFile(dir.absoluteFilePath(QStringLiteral("index.json")));
  if (manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    manifestFile.close();
  }

  qDebug("[Preset] exportBundleIni: exported %d user presets to %s", exported, dirPath.toUtf8().constData());
  return exported;
}

// v5.16 (PSET2-04): upstream-compatible bundle import. Reads the export
// tree: index.json manifest first, then per-category subdirs of
// upstream-shaped JSON, then legacy flat `.ini` files (category header with
// the corrected enum mapping — the previous code swapped printer/print).
int PresetServiceMock::importBundleIni(const QString &dirPath)
{
  QDir dir(dirPath);
  if (!dir.exists())
  {
    qWarning("[Preset] importBundleIni: directory does not exist %s", dirPath.toUtf8().constData());
    return -1;
  }

  struct PendingImport
  {
    QString name;
    int category = -1;
    QString inherits;
    QString vendor;
    QString settingId;
    QHash<QString, QVariant> values;
  };
  QList<PendingImport> pending;

  // Category directory names -> enum. "machine" is upstream's name for
  // printer presets (PRESET_PRINTER_NAME); "print" is accepted as a legacy
  // alias of "process".
  auto categoryFromDirName = [](const QString &dirName) -> int {
    if (dirName == QLatin1String("printer") || dirName == QLatin1String("machine"))
      return PrinterCat;
    if (dirName == QLatin1String("filament"))
      return FilamentCat;
    if (dirName == QLatin1String("process") || dirName == QLatin1String("print"))
      return PrintCat;
    return -1;
  };
  auto categoryFromTypeName = [](const QString &typeName) -> int {
    if (typeName == QLatin1String("machine") || typeName == QLatin1String("printer"))
      return PrinterCat;
    if (typeName == QLatin1String("filament"))
      return FilamentCat;
    if (typeName == QLatin1String("process") || typeName == QLatin1String("print"))
      return PrintCat;
    return -1;
  };
  auto ingestJsonFile = [&](const QString &filePath, int categoryHint) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
      return;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
      return;
    const QJsonObject root = doc.object();
    PendingImport item;
    item.name = root.value(QStringLiteral("name")).toString().trimmed();
    item.inherits = root.value(QStringLiteral("inherits")).toString();
    int category = categoryHint >= 0 ? categoryHint
                                     : categoryFromTypeName(root.value(QStringLiteral("type")).toString());
    if (item.name.isEmpty() || !isValidCategory(category))
      return;
    item.category = category;
    for (auto it = root.constBegin(); it != root.constEnd(); ++it)
    {
      if (isUserPresetHeaderKey(it.key()))
        continue;
      item.values.insert(it.key(), variantFromJsonValue(it.value()));
    }
    pending.append(item);
  };

  // 1) Manifest-driven import (exact name/category/file triples).
  QFile manifestFile(dir.absoluteFilePath(QStringLiteral("index.json")));
  if (manifestFile.open(QIODevice::ReadOnly))
  {
    const QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
    manifestFile.close();
    if (doc.isObject() && doc.object().value(QStringLiteral("kind")).toString()
                              == QStringLiteral("owzx-preset-bundle-dir"))
    {
      const QJsonArray presets = doc.object().value(QStringLiteral("presets")).toArray();
      for (const QJsonValue &v : presets)
      {
        const QJsonObject entry = v.toObject();
        const int category = entry.value(QStringLiteral("category")).toInt(-1);
        const QString file = entry.value(QStringLiteral("file")).toString();
        if (!isValidCategory(category) || file.isEmpty())
          continue;
        ingestJsonFile(dir.absoluteFilePath(file), category);
      }
    }
  }

  // 2) Category-subdir scan (upstream user preset layout, no manifest).
  const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QString &subDir : subDirs)
  {
    const int category = categoryFromDirName(subDir);
    if (!isValidCategory(category))
      continue;
    const QDir catDir(dir.absoluteFilePath(subDir));
    const QStringList files = catDir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString &fileName : files)
      ingestJsonFile(catDir.absoluteFilePath(fileName), category);
  }

  // 3) Loose upstream JSON at the root (category from the "type" header).
  {
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString &fileName : files)
    {
      if (fileName == QLatin1String("index.json"))
        continue;
      ingestJsonFile(dir.absoluteFilePath(fileName), -1);
    }
  }

  int imported = 0;

  // 4) Legacy flat `.ini` files (the Phase 147 format, kept readable).
  const QStringList iniFiles = dir.entryList({QStringLiteral("*.ini")}, QDir::Files);
  for (const QString &fileName : iniFiles)
  {
    QFile f(dir.absoluteFilePath(fileName));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
      continue;

    QString name;
    int category = PrintCat; // default to process
    QString vendor, settingId, inherits;
    bool readOnly = false;
    QHash<QString, QVariant> values;

    QTextStream in(&f);
    bool inHeader = false;
    for (QString line = in.readLine(); !line.isNull(); line = in.readLine())
    {
      QString trimmed = line.trimmed();
      if (trimmed.isEmpty())
        continue;
      if (trimmed.startsWith(QStringLiteral("[")))
      {
        inHeader = (trimmed == QStringLiteral("[preset]"));
        continue;
      }
      const int eq = trimmed.indexOf(QLatin1Char('='));
      if (eq < 0)
        continue;
      const QString key = trimmed.left(eq).trimmed();
      const QString val = trimmed.mid(eq + 1).trimmed();
      if (inHeader)
      {
        if (key == QLatin1String("name")) name = val;
        else if (key == QLatin1String("category"))
        {
          // v5.16 (PSET2-04) fix: enum is Print=0/Filament=1/Printer=2
          // (PresetServiceMock.h). The previous mapping swapped
          // printer<->print so imported printers landed as process presets.
          if (val == QLatin1String("printer")) category = PrinterCat;
          else if (val == QLatin1String("filament")) category = FilamentCat;
          else if (val == QLatin1String("print")) category = PrintCat;
        }
        else if (key == QLatin1String("vendor")) vendor = val;
        else if (key == QLatin1String("setting_id")) settingId = val;
        else if (key == QLatin1String("inherits")) inherits = val;
        else if (key == QLatin1String("readonly")) readOnly = (val == QLatin1String("1"));
      }
      else
      {
        // Value section: keep as string; the existing JSON path stores
        // everything as QVariants and stringifies on demand, so this matches.
        values.insert(key, val);
      }
    }
    f.close();

    if (name.isEmpty())
      continue;
    PendingImport item;
    item.name = name;
    item.category = category;
    item.inherits = inherits;
    item.vendor = vendor;
    item.settingId = settingId;
    item.values = values;
    pending.append(item);
  }

  for (const PendingImport &item : pending)
  {
    if (item.name.isEmpty() || !isValidCategory(item.category))
      continue;
    if (createCustomPreset(item.category, item.name, item.values, item.inherits))
    {
      // Patch metadata that createCustomPreset doesn't take.
      auto metaIt = m_presetMetadata.find(item.name);
      if (metaIt != m_presetMetadata.end())
      {
        if (!item.vendor.isEmpty()) metaIt->vendor = item.vendor;
        if (!item.settingId.isEmpty()) metaIt->settingId = item.settingId;
      }
      ++imported;
    }
  }
  qDebug("[Preset] importBundleIni: imported %d presets from %s", imported, dirPath.toUtf8().constData());
  return imported;
}

// Phase 149 (PSET-05): compare two presets key-by-key. Returns a QVariantList
// of {key, valueA, valueB} maps for differing keys. Mirrors upstream
// UnsavedChangesDialog diff-view mode (single-direction A vs B; 3-way is a UI
// layer concern built on top of this primitive).
QVariantList PresetServiceMock::comparePresets(const QString &presetA, const QString &presetB) const
{
  QVariantList result;
  const auto itA = m_presetStore.constFind(presetA);
  const auto itB = m_presetStore.constFind(presetB);
  if (itA == m_presetStore.constEnd() || itB == m_presetStore.constEnd())
  {
    qWarning("[Preset] comparePresets: missing preset A=%s B=%s",
             presetA.toUtf8().constData(), presetB.toUtf8().constData());
    return result;
  }
  const QHash<QString, QVariant> &a = itA.value();
  const QHash<QString, QVariant> &b = itB.value();

  // Collect the union of keys (sorted for deterministic display).
  QList<QString> keys;
  keys.reserve(a.size() + b.size());
  for (auto k : a.keys()) keys << k;
  for (auto k : b.keys())
  {
    if (!a.contains(k))
      keys << k;
  }
  std::sort(keys.begin(), keys.end());

  static const QString sMissing = QStringLiteral("<missing>");
  for (const QString &k : keys)
  {
    const auto va = a.constFind(k);
    const auto vb = b.constFind(k);
    const bool hasA = (va != a.constEnd());
    const bool hasB = (vb != b.constEnd());
    QString sa = hasA ? va->toString() : sMissing;
    QString sb = hasB ? vb->toString() : sMissing;
    if (sa != sb)
    {
      QVariantMap entry;
      entry.insert(QStringLiteral("key"), k);
      entry.insert(QStringLiteral("valueA"), sa);
      entry.insert(QStringLiteral("valueB"), sb);
      entry.insert(QStringLiteral("status"),
                   !hasA ? QStringLiteral("added")
                   : !hasB ? QStringLiteral("removed")
                           : QStringLiteral("changed"));
      result.append(entry);
    }
  }
  return result;
}

bool PresetServiceMock::createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values)
{
  return createCustomPreset(category, name, values, QString());
}

bool PresetServiceMock::createCustomPreset(int category, const QString &name,
                                           const QHash<QString, QVariant> &values, const QString &inherits)
{
  const QString trimmedName = name.trimmed();
  if (!isValidCategory(category) || trimmedName.isEmpty() || m_presetStore.contains(trimmedName))
    return false;

  // v5.16 (PSET2-02): resolve the inheritance chain — the parent's stored
  // values already carry its full resolved chain (resolveInheritance output
  // captured at vendor-load time), so overlaying `values` on top reproduces
  // upstream Preset inheritance semantics at creation.
  const QString parent = inherits.trimmed();
  if (!parent.isEmpty() && !m_presetStore.contains(parent))
    return false;

  QHash<QString, QVariant> resolved;
  if (!parent.isEmpty())
    resolved = m_presetStore.value(parent);
  for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    resolved.insert(it.key(), it.value());

  m_presetStore[trimmedName] = resolved;
  registerPresetMetadata(trimmedName, category, false, false);
  if (!parent.isEmpty())
    m_presetInherits[trimmedName] = parent;
  // v5.16 (PSET2-01): persist to the user preset tree (upstream
  // PresetBundle::save_current_preset -> Preset::save).
  writeUserPresetFile(category, trimmedName, resolved);
  setSelectedPresetForCategory(category, trimmedName);
  return true;
}

bool PresetServiceMock::mergePresetValues(const QString &presetName, const QHash<QString, QVariant> &values)
{
  // v5.16 (PSET2-03): Transfer primitive — selected keys land on the target
  // user preset (and disk) while every other key is kept; the source preset
  // is never written (upstream UnsavedChangesDialog Action::Transfer).
  if (!m_presetStore.contains(presetName) || isReadOnlyPreset(presetName) || values.isEmpty())
    return false;

  QHash<QString, QVariant> &target = m_presetStore[presetName];
  for (auto it = values.constBegin(); it != values.constEnd(); ++it)
    target.insert(it.key(), it.value());
  writeUserPresetFile(presetCategory(presetName), presetName, target);
  return true;
}

bool PresetServiceMock::deletePreset(const QString &presetName)
{
  if (!m_presetStore.contains(presetName) || isReadOnlyPreset(presetName))
    return false;

  const int category = presetCategory(presetName);
  m_presetStore.remove(presetName);
  m_presetMetadata.remove(presetName);
  m_builtinPresetNames.remove(presetName);
  m_presetInherits.remove(presetName);
  // 从所有分类中移除
  for (auto it = m_categoryPresets.begin(); it != m_categoryPresets.end(); ++it)
    it->removeAll(presetName);
  // v5.16 (PSET2-01): delete the persisted user preset file (upstream
  // PresetCollection erases the user JSON + .info).
  removeUserPresetFile(category, presetName);
  updateSelectedPresetFallback(category);
  if (isValidCategory(category))
  {
    QSettings settings;
    settings.setValue(selectionSettingsKey(category), m_selectedPresets.value(category));
    settings.sync();
  }
  return true;
}

bool PresetServiceMock::renamePreset(const QString &oldName, const QString &newName)
{
  // 内置预设和不存在预设不可重命名
  if (!m_presetStore.contains(oldName) || isReadOnlyPreset(oldName))
    return false;
  const QString trimmedNewName = newName.trimmed();
  if (trimmedNewName.isEmpty() || m_presetStore.contains(trimmedNewName))
    return false;

  const int category = presetCategory(oldName);
  // 移动预设值
  m_presetStore.insert(trimmedNewName, m_presetStore.take(oldName));
  if (m_presetMetadata.contains(oldName))
    m_presetMetadata.insert(trimmedNewName, m_presetMetadata.take(oldName));
  if (m_presetInherits.contains(oldName))
    m_presetInherits.insert(trimmedNewName, m_presetInherits.take(oldName));
  // 更新分类列表中的名称
  for (auto it = m_categoryPresets.begin(); it != m_categoryPresets.end(); ++it)
  {
    for (int i = 0; i < it->size(); ++i)
    {
      if (it->at(i) == oldName)
        (*it)[i] = trimmedNewName;
    }
  }
  // v5.16 (PSET2-01): rename the persisted file (old JSON removed, new JSON
  // written; upstream keeps the file name in sync with the preset name).
  if (isValidCategory(category))
  {
    removeUserPresetFile(category, oldName);
    writeUserPresetFile(category, trimmedNewName, m_presetStore.value(trimmedNewName));
  }
  if (isValidCategory(category) && m_selectedPresets.value(category) == oldName)
    setSelectedPresetForCategory(category, trimmedNewName);
  return true;
}

QString PresetServiceMock::presetInherits(const QString &presetName) const
{
  return m_presetInherits.value(presetName);
}
