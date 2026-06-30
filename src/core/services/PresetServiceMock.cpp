#include "PresetServiceMock.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QSettings>
#include <cmath>

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
  // Locate vendor profile directory (对齐上游 PresetBundle data_dir / resource_dir)
  // Vendor index is at resources/profiles/Creality.json, sub-directories at resources/profiles/Creality/
  QString vendorDir;
  const QStringList searchPaths = {
      // Source tree: index at profiles/Creality.json, subdirs at profiles/Creality/
      QDir::currentPath() + QStringLiteral("/third_party/OrcaSlicer/resources/profiles"),
      // Installed resource path
      QCoreApplication::applicationDirPath() + QStringLiteral("/resources/profiles"),
  };

  QString profilesDir;
  for (const auto &path : searchPaths)
  {
    if (QFileInfo::exists(path + QStringLiteral("/Creality.json")))
    {
      profilesDir = path;
      break;
    }
  }

  if (profilesDir.isEmpty())
    return false;

  // vendorDir points to the Creality/ subdirectory for machine/filament/process sub_path resolution
  vendorDir = profilesDir + QStringLiteral("/Creality");
  const QString indexFile = profilesDir + QStringLiteral("/Creality.json");
  QFile f(indexFile);
  if (!f.open(QIODevice::ReadOnly))
    return false;

  QJsonParseError err;
  const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &err).object();
  if (err.error != QJsonParseError::NoError)
    return false;
  const QString vendorName = root.value(QStringLiteral("name")).toString(QStringLiteral("Creality"));

  // Parse vendor index to get sub-file lists
  struct SubFileEntry
  {
    QString name;
    QString subPath;
  };
  QList<SubFileEntry> machineEntries, processEntries, filamentEntries;

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

  machineEntries = parseSubFileList(root, QStringLiteral("machine_list"));
  processEntries = parseSubFileList(root, QStringLiteral("process_list"));
  filamentEntries = parseSubFileList(root, QStringLiteral("filament_list"));

  // Resolved config cache (preset_name → merged key-values)
  QMap<QString, QHash<QString, QVariant>> resolvedConfigs;
  QMap<QString, QString> inheritMap;

  // Load machine (printer) presets
  for (const auto &entry : machineEntries)
  {
    const QString filePath = vendorDir + QStringLiteral("/") + entry.subPath;
    const QHash<QString, QVariant> resolved = resolveInheritance(entry.name, filePath, resolvedConfigs, inheritMap);
    if (resolved.isEmpty())
      continue;

    // Only store presets with instantiation=true (real printer configs, not base templates)
    const QString instantiation = resolved.value(QStringLiteral("__instantiation__")).toString();
    if (instantiation == QStringLiteral("false"))
    {
      resolvedConfigs[entry.name] = resolved;
      continue;
    }

    // Remove internal metadata keys
    QHash<QString, QVariant> cleanValues = resolved;
    cleanValues.remove(QStringLiteral("__instantiation__"));
    cleanValues.remove(QStringLiteral("__inherits__"));
    cleanValues.remove(QStringLiteral("__type__"));
    cleanValues.remove(QStringLiteral("__from__"));
    cleanValues.remove(QStringLiteral("__name__"));

    m_presetStore[entry.name] = cleanValues;
    registerPresetMetadata(entry.name, PrinterCat, true, true, vendorName);
    if (!inheritMap.value(entry.name).isEmpty())
      m_presetInherits[entry.name] = inheritMap[entry.name];
  }

  // Load filament presets
  for (const auto &entry : filamentEntries)
  {
    const QString filePath = vendorDir + QStringLiteral("/") + entry.subPath;
    const QHash<QString, QVariant> resolved = resolveInheritance(entry.name, filePath, resolvedConfigs, inheritMap);
    if (resolved.isEmpty())
      continue;

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
    registerPresetMetadata(entry.name, FilamentCat, true, true, vendorName);
    if (!inheritMap.value(entry.name).isEmpty())
      m_presetInherits[entry.name] = inheritMap[entry.name];
  }

  // Load process (print) presets
  for (const auto &entry : processEntries)
  {
    const QString filePath = vendorDir + QStringLiteral("/") + entry.subPath;
    const QHash<QString, QVariant> resolved = resolveInheritance(entry.name, filePath, resolvedConfigs, inheritMap);
    if (resolved.isEmpty())
      continue;

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
    registerPresetMetadata(entry.name, PrintCat, true, true, vendorName);
    if (!inheritMap.value(entry.name).isEmpty())
      m_presetInherits[entry.name] = inheritMap[entry.name];
  }

  return !m_presetStore.isEmpty();
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

bool PresetServiceMock::createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values)
{
  const QString trimmedName = name.trimmed();
  if (!isValidCategory(category) || trimmedName.isEmpty() || m_presetStore.contains(trimmedName))
    return false;

  m_presetStore[trimmedName] = values;
  registerPresetMetadata(trimmedName, category, false, false);
  setSelectedPresetForCategory(category, trimmedName);
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
  if (isValidCategory(category) && m_selectedPresets.value(category) == oldName)
    setSelectedPresetForCategory(category, trimmedNewName);
  return true;
}

QString PresetServiceMock::presetInherits(const QString &presetName) const
{
  return m_presetInherits.value(presetName);
}
