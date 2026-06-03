#include "PresetServiceMock.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#endif

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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[PrinterCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[PrinterCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[FilamentCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[FilamentCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[FilamentCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[PrintCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[PrintCat].append(name);
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
    m_builtinPresetNames.insert(name);
    m_categoryPresets[PrintCat].append(name);
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
      QDir::currentPath() + QStringLiteral("/third_party/CrealityPrint/resources/profiles"),
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
    m_builtinPresetNames.insert(entry.name);
    m_categoryPresets[PrinterCat].append(entry.name);
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
    m_builtinPresetNames.insert(entry.name);
    m_categoryPresets[FilamentCat].append(entry.name);
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
    m_builtinPresetNames.insert(entry.name);
    m_categoryPresets[PrintCat].append(entry.name);
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
  return m_categoryPresets.value(category);
}

QString PresetServiceMock::defaultPresetForCategory(int category) const
{
  const auto names = m_categoryPresets.value(category);
  if (names.isEmpty())
    return {};
  // Return first preset in the category (typically the default)
  return names.first();
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

void PresetServiceMock::savePresetValues(const QString &presetName, const QHash<QString, QVariant> &values)
{
  m_presetStore[presetName] = values;
}

bool PresetServiceMock::hasPreset(const QString &presetName) const
{
  return m_presetStore.contains(presetName);
}

bool PresetServiceMock::isBuiltinPreset(const QString &presetName) const
{
  return m_builtinPresetNames.contains(presetName);
}

bool PresetServiceMock::isFilamentCompatibleWithPrinter(const QString &filamentName, const QString &printerName) const
{
  const auto filIt = m_presetStore.find(filamentName);
  const auto prnIt = m_presetStore.find(printerName);
  if (filIt == m_presetStore.end() || prnIt == m_presetStore.end())
    return true; // unknown presets → assume compatible

  const auto &filVals = filIt.value();
  const auto &prnVals = prnIt.value();

  // Primary check: compatible_printers array (对齐上游 Preset::compatible_printers)
  const QVariant compatVar = filVals.value(QStringLiteral("compatible_printers"));
  if (compatVar.isValid())
  {
    QStringList compatList;
    if (compatVar.typeId() == QMetaType::Type::QVariantList)
    {
      const QVariantList vl = compatVar.toList();
      for (const auto &v : vl)
        compatList.append(v.toString());
    }
    else if (compatVar.typeId() == QMetaType::Type::QStringList)
    {
      compatList = compatVar.toStringList();
    }
    else if (compatVar.typeId() == QMetaType::Type::QString)
    {
      // Single string: treat as one-element list
      const QString s = compatVar.toString();
      if (!s.isEmpty())
        compatList.append(s);
    }
    if (!compatList.isEmpty())
    {
      // If compatible_printers is specified, printer must be in the list
      bool found = false;
      for (const auto &cp : compatList)
      {
        if (cp == printerName)
        {
          found = true;
          break;
        }
      }
      if (!found)
        return false;
    }
    // Empty list → fall through to nozzle/temp range checks
  }

  // Check nozzle diameter compatibility (对齐上游 nozzle_diameter matching)
  const double filNozzleMin = filVals.value(QStringLiteral("compatible_nozzle_min"), 0.15).toDouble();
  const double filNozzleMax = filVals.value(QStringLiteral("compatible_nozzle_max"), 1.0).toDouble();
  const double prnNozzle = prnVals.value(QStringLiteral("nozzle_diameter"), 0.4).toDouble();

  if (prnNozzle < filNozzleMin || prnNozzle > filNozzleMax)
    return false;

  // Check max nozzle temperature compatibility (对齐上游 max_temp matching)
  const double filTempMax = filVals.value(QStringLiteral("nozzle_temp_range_max"), 300).toDouble();
  const double prnMaxTemp = prnVals.value(QStringLiteral("max_nozzle_temp"), 300).toDouble();

  if (filTempMax > prnMaxTemp)
    return false;

  return true;
}

QString PresetServiceMock::findCompatibleFilament(const QString &printerName) const
{
  const auto prnIt = m_presetStore.find(printerName);
  if (prnIt == m_presetStore.end())
    return {};

  const auto filList = m_categoryPresets.value(FilamentCat);
  // Prefer the first compatible filament in the list
  for (const auto &filName : filList)
  {
    if (isFilamentCompatibleWithPrinter(filName, printerName))
      return filName;
  }
  return filList.isEmpty() ? QString() : filList.first();
}

bool PresetServiceMock::createCustomPreset(int category, const QString &name, const QHash<QString, QVariant> &values)
{
  if (name.isEmpty() || m_presetStore.contains(name))
    return false;

  m_presetStore[name] = values;
  m_categoryPresets[category].append(name);
  return true;
}

bool PresetServiceMock::deletePreset(const QString &presetName)
{
  if (m_builtinPresetNames.contains(presetName) || !m_presetStore.contains(presetName))
    return false;

  m_presetStore.remove(presetName);
  // 从所有分类中移除
  for (auto it = m_categoryPresets.begin(); it != m_categoryPresets.end(); ++it)
    it->removeAll(presetName);
  return true;
}

bool PresetServiceMock::renamePreset(const QString &oldName, const QString &newName)
{
  // 内置预设和不存在预设不可重命名
  if (m_builtinPresetNames.contains(oldName) || !m_presetStore.contains(oldName))
    return false;
  if (newName.trimmed().isEmpty() || m_presetStore.contains(newName))
    return false;

  // 移动预设值
  m_presetStore.insert(newName, m_presetStore.take(oldName));
  // 更新分类列表中的名称
  for (auto it = m_categoryPresets.begin(); it != m_categoryPresets.end(); ++it)
  {
    for (int i = 0; i < it->size(); ++i)
    {
      if (it->at(i) == oldName)
        (*it)[i] = newName;
    }
  }
  return true;
}

QString PresetServiceMock::presetInherits(const QString &presetName) const
{
  return m_presetInherits.value(presetName);
}
