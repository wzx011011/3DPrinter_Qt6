#include "ConfigOptionModel.h"

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Config.hpp>
#endif

ConfigOptionModel::ConfigOptionModel(QObject *parent)
    : QAbstractListModel(parent)
{
  // ── 质量 / 层 ───────────────────────────────────────────────
  m_options = {
      {"layer_height", tr("层高"), "double", 0.20, 0.05, 0.35, 0.01, {}, tr("质量")},
      {"initial_layer_print_height", tr("首层层高"), "double", 0.20, 0.10, 0.40, 0.01, {}, tr("质量")},
      {"line_width", tr("线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("质量")},
      // ── 壁 ──────────────────────────────────────────────────
      {"wall_loops", tr("壁线圈数"), "int", 3, 1, 8, 1, {}, tr("质量")},
      {"top_shell_layers", tr("顶层层数"), "int", 4, 2, 10, 1, {}, tr("质量")},
      {"bottom_shell_layers", tr("底层层数"), "int", 4, 2, 10, 1, {}, tr("质量")},
      // ── 填充 ─────────────────────────────────────────────────
      {"sparse_infill_density", tr("填充密度 (%)"), "int", 15, 0, 100, 5, {}, tr("填充")},
      {"sparse_infill_pattern", tr("填充图案"), "enum", 0, 0, 0, 1, {tr("网格"), tr("直线"), tr("三角"), tr("蜂巢"), tr("陀螺")}, tr("填充")},
      {"infill_wall_overlap", tr("填充重叠 (%)"), "int", 10, 0, 30, 1, {}, tr("填充")},
      // ── 速度 ─────────────────────────────────────────────────
      {"outer_wall_speed", tr("外壁速度 (mm/s)"), "int", 150, 20, 400, 10, {}, tr("速度")},
      {"travel_speed", tr("空走速度 (mm/s)"), "int", 400, 100, 800, 10, {}, tr("速度")},
      {"initial_layer_speed", tr("首层速度 (mm/s)"), "int", 50, 10, 80, 5, {}, tr("速度")},
      // ── 温度 ─────────────────────────────────────────────────
      {"nozzle_temp", tr("喷嘴温度 (°C)"), "int", 220, 150, 300, 5, {}, tr("温度")},
      {"bed_temp", tr("热床温度 (°C)"), "int", 65, 0, 120, 5, {}, tr("温度")},
      {"chamber_temperature", tr("腔体温度 (°C)"), "int", 0, 0, 60, 5, {}, tr("温度")},
      {"preheat_time", tr("预热时间 (s)"), "int", 90, 30, 300, 10, {}, tr("温度"), true},
      // ── 支撑 ─────────────────────────────────────────────────
      {"enable_support", tr("启用支撑"), "bool", false, 0, 1, 1, {}, tr("支撑")},
      {"support_density", tr("支撑密度 (%)"), "int", 20, 10, 50, 5, {}, tr("支撑")},
      {"support_type", tr("支撑类型"), "enum", 0, 0, 0, 1, {tr("普通"), tr("树状")}, tr("支撑")},
      // ── 底座 ─────────────────────────────────────────────────
      {"brim_enable", tr("启用 Brim"), "bool", false, 0, 1, 1, {}, tr("底座")},
      {"brim_width", tr("Brim 宽度 (mm)"), "double", 5.0, 2.0, 20.0, 0.5, {}, tr("底座")},
      {"skirt_loops", tr("裙边线数"), "int", 1, 0, 5, 1, {}, tr("底座")},
      // ── 冷却 ─────────────────────────────────────────────────
      {"fan_speed", tr("风扇速度 (%)"), "int", 100, 0, 100, 5, {}, tr("冷却")},
      {"fan_initial_delay", tr("风扇延迟 (层)"), "int", 3, 0, 10, 1, {}, tr("冷却")},
      // ── 回退 ─────────────────────────────────────────────────
      {"retract_enable", tr("启用回退"), "bool", true, 0, 1, 1, {}, tr("回退")},
      {"retract_dist", tr("回退距离 (mm)"), "double", 1.0, 0.5, 8.0, 0.1, {}, tr("回退")},
      {"retract_speed", tr("回退速度 (mm/s)"), "int", 45, 20, 80, 5, {}, tr("回退")},
      // ── 其他 ─────────────────────────────────────────────────
      {"z_seam_type", tr("Z 缝策略"), "enum", 0, 0, 0, 1, {tr("最锐角"), tr("最短"), tr("随机"), tr("指定")}, tr("其他")},
      {"enable_coasting", tr("启用 Coasting"), "bool", false, 0, 1, 1, {}, tr("其他")},
  };

  for (const auto &option : m_options)
  {
    if (option.readonly)
      m_baseReadonlyKeys.insert(option.key);
  }
}

int ConfigOptionModel::rowCount(const QModelIndex &parent) const
{
  return parent.isValid() ? 0 : m_options.size();
}

QVariant ConfigOptionModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() >= m_options.size())
    return {};
  const auto &o = m_options[index.row()];
  switch (role)
  {
  case KeyRole:
    return o.key;
  case LabelRole:
    return o.label;
  case TypeRole:
    return o.type;
  case ValueRole:
    return o.value;
  case MinRole:
    return o.min;
  case MaxRole:
    return o.max;
  case StepRole:
    return o.step;
  case EnumLabelsRole:
    return o.enumLabels;
  case CategoryRole:
    return o.category;
  case ReadonlyRole:
    return o.readonly;
  default:
    return {};
  }
}

QHash<int, QByteArray> ConfigOptionModel::roleNames() const
{
  return {
      {KeyRole, "optKey"},
      {LabelRole, "optLabel"},
      {TypeRole, "optType"},
      {ValueRole, "optValue"},
      {MinRole, "optMin"},
      {MaxRole, "optMax"},
      {StepRole, "optStep"},
      {EnumLabelsRole, "optEnumLabels"},
      {CategoryRole, "optCategory"},
      {ReadonlyRole, "optReadonly"},
  };
}

// ── Individual accessors (safe for QML, no QVariantList) ─────────────────────
QString ConfigOptionModel::optKey(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].key : QString{}; }
QString ConfigOptionModel::optLabel(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].label : QString{}; }
QString ConfigOptionModel::optType(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].type : QString{}; }
QVariant ConfigOptionModel::optValue(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].value : QVariant{}; }
double ConfigOptionModel::optMin(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].min : 0.0; }
double ConfigOptionModel::optMax(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].max : 0.0; }
double ConfigOptionModel::optStep(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].step : 1.0; }
QString ConfigOptionModel::optCategory(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].category : QString{}; }
bool ConfigOptionModel::optReadonly(int i) const { return (i >= 0 && i < m_options.size()) && m_options[i].readonly; }

int ConfigOptionModel::optEnumCount(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].enumLabels.size() : 0;
}
QString ConfigOptionModel::optEnumLabel(int i, int j) const
{
  if (i < 0 || i >= m_options.size())
    return {};
  const auto &e = m_options[i].enumLabels;
  return (j >= 0 && j < e.size()) ? e[j] : QString{};
}

void ConfigOptionModel::setValue(int row, const QVariant &value)
{
  if (row < 0 || row >= m_options.size())
    return;
  m_options[row].value = value;
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {ValueRole});
  emit optionValueChanged(m_options[row].key, value);
}

int ConfigOptionModel::indexOfKey(const QString &key) const
{
  return findIndex(key);
}

int ConfigOptionModel::countForCategory(const QString &category) const
{
  int c = 0;
  for (const auto &o : m_options)
    if (o.category == category)
      ++c;
  return c;
}

QVariantList ConfigOptionModel::indicesForCategory(const QString &category) const
{
  QVariantList result;
  for (int i = 0; i < m_options.size(); ++i)
    if (m_options[i].category == category)
      result.append(i);
  return result;
}

QHash<QString, QVariant> ConfigOptionModel::valuesByKey() const
{
  QHash<QString, QVariant> result;
  result.reserve(m_options.size());
  for (const auto &option : m_options)
    result.insert(option.key, option.value);
  return result;
}

void ConfigOptionModel::applyValues(const QHash<QString, QVariant> &values)
{
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const auto it = values.constFind(m_options[i].key);
    if (it == values.cend() || m_options[i].value == it.value())
      continue;
    m_options[i].value = it.value();
    changed = true;
  }

  if (changed)
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole});
}

void ConfigOptionModel::setReadonlyKeys(const QSet<QString> &keys)
{
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const bool readonly = m_baseReadonlyKeys.contains(m_options[i].key) || keys.contains(m_options[i].key);
    if (m_options[i].readonly == readonly)
      continue;
    m_options[i].readonly = readonly;
    changed = true;
  }

  if (changed)
    emit dataChanged(index(0), index(m_options.size() - 1), {ReadonlyRole});
}

int ConfigOptionModel::findIndex(const QString &key) const
{
  for (int i = 0; i < m_options.size(); ++i)
  {
    if (m_options[i].key == key)
      return i;
  }
  return -1;
}

#ifdef HAS_LIBSLIC3R

namespace
{
  // Map upstream ConfigOptionType to Qt6 type string
  QString mapType(Slic3r::ConfigOptionType t)
  {
    switch (t)
    {
    case Slic3r::coFloat:
    case Slic3r::coFloatOrPercent:
    case Slic3r::coFloats:
    case Slic3r::coPercents:
      return QStringLiteral("double");
    case Slic3r::coInt:
    case Slic3r::coInts:
      return QStringLiteral("int");
    case Slic3r::coBool:
    case Slic3r::coBools:
      return QStringLiteral("bool");
    case Slic3r::coEnum:
    case Slic3r::coEnums:
      return QStringLiteral("enum");
    case Slic3r::coString:
    case Slic3r::coStrings:
      return QStringLiteral("string");
    default:
      return QStringLiteral("double");
    }
  }

  // Map upstream category to Qt6 display category
  QString mapCategory(const std::string &upstreamCategory)
  {
    const QString cat = QString::fromUtf8(upstreamCategory.c_str());
    if (cat.contains("Layers", Qt::CaseInsensitive) || cat.contains("Perimeters", Qt::CaseInsensitive))
      return QObject::tr("质量");
    if (cat.contains("Infill", Qt::CaseInsensitive))
      return QObject::tr("填充");
    if (cat.contains("Speed", Qt::CaseInsensitive))
      return QObject::tr("速度");
    if (cat.contains("Temperature", Qt::CaseInsensitive) || cat.contains("Cooling", Qt::CaseInsensitive))
      return QObject::tr("温度");
    if (cat.contains("Support", Qt::CaseInsensitive))
      return QObject::tr("支撑");
    if (cat.contains("Skirt", Qt::CaseInsensitive) || cat.contains("Brim", Qt::CaseInsensitive) || cat.contains("Raft", Qt::CaseInsensitive))
      return QObject::tr("底座");
    if (cat.contains("Fan", Qt::CaseInsensitive))
      return QObject::tr("冷却");
    if (cat.contains("Retract", Qt::CaseInsensitive))
      return QObject::tr("回退");
    if (cat.contains("Extruder", Qt::CaseInsensitive) || cat.contains("Extrusion", Qt::CaseInsensitive))
      return QObject::tr("挤出机");
    if (cat.contains("Adhesion", Qt::CaseInsensitive))
      return QObject::tr("底座");
    if (cat.contains("Output", Qt::CaseInsensitive) || cat.contains("G-code", Qt::CaseInsensitive))
      return QObject::tr("输出");
    if (cat.contains("Ironing", Qt::CaseInsensitive))
      return QObject::tr("质量");
    if (cat.contains("Seam", Qt::CaseInsensitive))
      return QObject::tr("质量");
    return QObject::tr("其他");
  }

  // Extract default value from ConfigOptionDef
  QVariant extractDefault(const Slic3r::ConfigOptionDef *def)
  {
    if (!def || !def->default_value)
      return {};
    switch (def->type)
    {
    case Slic3r::coFloat:
      return def->get_default_value<Slic3r::ConfigOptionFloat>()->value;
    case Slic3r::coInt:
      return def->get_default_value<Slic3r::ConfigOptionInt>()->value;
    case Slic3r::coBool:
      return def->get_default_value<Slic3r::ConfigOptionBool>()->value;
    case Slic3r::coString:
      return QString::fromStdString(def->get_default_value<Slic3r::ConfigOptionString>()->value);
    case Slic3r::coEnum:
    case Slic3r::coEnums:
    {
      const int intVal = def->get_default_value<Slic3r::ConfigOptionEnumGeneric>()->value;
      // Try to find the enum label for the default value
      const auto &enumValues = def->enum_values;
      const auto &enumLabels = def->enum_labels;
      if (intVal >= 0 && intVal < static_cast<int>(enumValues.size()) && intVal < static_cast<int>(enumLabels.size()))
        return QString::fromUtf8(enumLabels[intVal].c_str());
      return intVal;
    }
    case Slic3r::coFloatOrPercent:
      return def->get_default_value<Slic3r::ConfigOptionFloatOrPercent>()->value;
    case Slic3r::coPercent:
      return def->get_default_value<Slic3r::ConfigOptionPercent>()->value;
    default:
      return {};
    }
  }
}

// Keys we want to expose in the UI (print-related, user-facing)
static const char *kDesiredKeys[] = {
  // Layer
  "layer_height", "initial_layer_print_height", "line_width",
  // Shell
  "wall_loops", "top_shell_layers", "bottom_shell_layers",
  "wall_infill_order", "infill_wall_overlap",
  // Infill
  "sparse_infill_density", "sparse_infill_pattern",
  "top_surface_pattern", "bottom_surface_pattern",
  "sparse_infill_filament", "wall_filament",
  // Speed
  "outer_wall_speed", "inner_wall_speed", "travel_speed", "initial_layer_speed",
  "sparse_infill_speed", "top_surface_speed", "support_interface_speed",
  "support_speed", "small_perimeter_speed", "bridge_speed",
  // Temperature
  "nozzle_temp", "bed_temp", "chamber_temperature",
  "nozzle_temperature_range", "bed_temperature_range",
  // Support
  "enable_support", "support_density", "support_type",
  "support_on_build_plate_only", "support_interface_top_layers",
  "support_interface_bottom_layers", "support_interface_spacing",
  // Brim / Skirt
  "brim_enable", "brim_width", "skirt_loops", "skirt_distance",
  // Cooling / Fan
  "fan_cooling_layer_time", "default_fan_speed",
  "min_fan_speed", "max_fan_speed",
  "overhang_fan_speed", "slow_down_layer_time",
  "additional_cooling_fan_speed",
  // Retraction
  "retract_length", "retract_speed", "retract_before_wipe",
  "retraction_speed", "deretraction_speed", "retract_restart_extra",
  "wipe_distance",
  // Seam
  "z_seam_type", "z_seam_position", "z_seam_corner",
  // Quality
  "reduce_crossing_wall", "detect_overhang_wall",
  "resolve_multi_overlaps", "max_travel_detour_distance",
  // Adhesion
  "adhesion_type", "raft_layers", "raft_interface_layers",
  // Output
  "gcode_comments", "gcode_precision_xy", "gcode_precision_z",
  "max_print_speed", "max_travel_speed",
  // Extruder
  "nozzle_diameter", "extruder_offset", "printer_model",
  // Ironing
  "ironing_type", "ironing_speed", "ironing_flow",
  nullptr
};

void ConfigOptionModel::loadFromUpstreamSchema()
{
  beginResetModel();
  m_options.clear();
  m_baseReadonlyKeys.clear();

  const auto &def = Slic3r::print_config_def;

  for (int ki = 0; kDesiredKeys[ki] != nullptr; ++ki)
  {
    const auto *opt = def.get(kDesiredKeys[ki]);
    if (!opt)
      continue;

    ConfigOption entry;
    entry.key = QString::fromUtf8(opt->opt_key.c_str());
    entry.label = QString::fromUtf8(opt->label.c_str());
    entry.type = mapType(opt->type);
    entry.category = mapCategory(opt->category);
    entry.readonly = opt->readonly;
    entry.value = extractDefault(opt);

    // Range
    entry.min = static_cast<double>(opt->min);
    entry.max = static_cast<double>(opt->max);
    if (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent)
      entry.max = opt->max_literal;

    // Step
    entry.step = (opt->type == Slic3r::coFloat || opt->type == Slic3r::coFloatOrPercent) ? 0.01 : 1.0;

    // Enum labels
    for (size_t i = 0; i < opt->enum_labels.size() && i < opt->enum_values.size(); ++i)
      entry.enumLabels.append(QString::fromUtf8(opt->enum_labels[i].c_str()));

    m_options.append(entry);

    if (entry.readonly)
      m_baseReadonlyKeys.insert(entry.key);
  }

  endResetModel();
}

#endif // HAS_LIBSLIC3R
