#include "PresetServiceMock.h"

PresetServiceMock::PresetServiceMock(QObject *parent)
    : QObject(parent)
{
  initBuiltinDefaults();
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

QStringList PresetServiceMock::presetNames() const
{
  return {
      QStringLiteral("0.20mm Standard @Creality K1C"),
      QStringLiteral("0.16mm Fine"),
      QStringLiteral("0.12mm Ultra")};
}

QString PresetServiceMock::defaultPreset() const
{
  return QStringLiteral("0.20mm Standard @Creality K1C");
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
  switch (category)
  {
  case PrintCat: return QStringLiteral("0.20mm Standard");
  case FilamentCat: return QStringLiteral("Creality Generic PLA");
  case PrinterCat: return QStringLiteral("Creality K1C 0.4");
  default: return {};
  }
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
