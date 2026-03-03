#include "ConfigOptionModel.h"

ConfigOptionModel::ConfigOptionModel(QObject *parent)
    : QAbstractListModel(parent)
{
  // ── 质量 / 层 ───────────────────────────────────────────────
  m_options = {
      {"layer_height", tr("层高"), "double", 0.20, 0.05, 0.35, 0.01, {}, tr("质量")},
      {"first_layer_height", tr("首层层高"), "double", 0.20, 0.10, 0.40, 0.01, {}, tr("质量")},
      {"line_width", tr("线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("质量")},
      // ── 壁 ──────────────────────────────────────────────────
      {"wall_count", tr("壁线圈数"), "int", 3, 1, 8, 1, {}, tr("质量")},
      {"top_layers", tr("顶层层数"), "int", 4, 2, 10, 1, {}, tr("质量")},
      {"bottom_layers", tr("底层层数"), "int", 4, 2, 10, 1, {}, tr("质量")},
      // ── 填充 ─────────────────────────────────────────────────
      {"infill_density", tr("填充密度 (%)"), "int", 15, 0, 100, 5, {}, tr("填充")},
      {"infill_pattern", tr("填充图案"), "enum", 0, 0, 0, 1, {tr("网格"), tr("直线"), tr("三角"), tr("蜂巢"), tr("陀螺")}, tr("填充")},
      {"infill_overlap", tr("填充重叠 (%)"), "double", 10, 0, 30, 0.5, {}, tr("填充")},
      // ── 速度 ─────────────────────────────────────────────────
      {"print_speed", tr("打印速度 (mm/s)"), "int", 300, 20, 600, 10, {}, tr("速度")},
      {"travel_speed", tr("空走速度 (mm/s)"), "int", 400, 100, 800, 10, {}, tr("速度")},
      {"first_layer_speed", tr("首层速度 (mm/s)"), "int", 50, 10, 80, 5, {}, tr("速度")},
      {"outer_wall_speed", tr("外壁速度 (mm/s)"), "int", 150, 20, 400, 10, {}, tr("速度")},
      // ── 温度 ─────────────────────────────────────────────────
      {"nozzle_temp", tr("喷嘴温度 (°C)"), "int", 220, 150, 300, 5, {}, tr("温度")},
      {"bed_temp", tr("热床温度 (°C)"), "int", 65, 0, 120, 5, {}, tr("温度")},
      {"chamber_temp", tr("腔体温度 (°C)"), "int", 0, 0, 60, 5, {}, tr("温度")},
      {"preheat_time", tr("预热时间 (s)"), "int", 90, 30, 300, 10, {}, tr("温度"), true},
      // ── 支撑 ─────────────────────────────────────────────────
      {"support_enable", tr("启用支撑"), "bool", false, 0, 1, 1, {}, tr("支撑")},
      {"support_density", tr("支撑密度 (%)"), "int", 20, 10, 50, 5, {}, tr("支撑")},
      {"support_type", tr("支撑类型"), "enum", 0, 0, 0, 1, {tr("普通"), tr("树状")}, tr("支撑")},
      // ── 底座 ─────────────────────────────────────────────────
      {"brim_enable", tr("启用 Brim"), "bool", false, 0, 1, 1, {}, tr("底座")},
      {"brim_width", tr("Brim 宽度 (mm)"), "double", 5.0, 2.0, 20.0, 0.5, {}, tr("底座")},
      {"skirt_line_count", tr("裙边线数"), "int", 1, 0, 5, 1, {}, tr("底座")},
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
