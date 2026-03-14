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
      // ── 质量 / 层 ───────────────────────────────────────────────
      {"layer_height", tr("层高"), "double", 0.20, 0.05, 0.35, 0.01, {}, tr("质量"), tr("层高与线宽")},
      {"initial_layer_print_height", tr("首层层高"), "double", 0.20, 0.10, 0.40, 0.01, {}, tr("质量"), tr("层高与线宽")},
      {"line_width", tr("线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("质量"), tr("层高与线宽")},
      {"initial_layer_line_width", tr("首层线宽"), "double", 0.42, 0.20, 1.00, 0.01, {}, tr("质量"), tr("层高与线宽")},
      // ── 壁 ──────────────────────────────────────────────────
      {"wall_loops", tr("壁线圈数"), "int", 3, 1, 8, 1, {}, tr("质量"), tr("壁与壳")},
      {"top_shell_layers", tr("顶层层数"), "int", 4, 2, 10, 1, {}, tr("质量"), tr("壁与壳")},
      {"bottom_shell_layers", tr("底层层数"), "int", 4, 2, 10, 1, {}, tr("质量"), tr("壁与壳")},
      {"wall_infill_order", tr("壁/填充顺序"), "enum", 0, 0, 0, 1, {tr("内壁→填充→外壁"), tr("外壁→填充→内壁")}, tr("质量"), tr("壁与壳")},
      {"infill_wall_overlap", tr("填充重叠 (%)"), "int", 10, 0, 30, 1, {}, tr("质量"), tr("壁与壳")},
      {"top_bottom_infill_wall_overlap", tr("顶底填充重叠 (%)"), "int", 15, 0, 40, 1, {}, tr("质量"), tr("壁与壳")},
      {"outer_wall_line_width", tr("外壁线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("质量"), tr("壁与壳")},
      {"inner_wall_line_width", tr("内壁线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("质量"), tr("壁与壳")},
      {"wall_sequence", tr("壁打印顺序"), "enum", 0, 0, 0, 1, {tr("内壁优先"), tr("外壁优先"), tr("交替")}, tr("质量"), tr("壁与壳")},
      {"reduce_crossing_wall", tr("减少穿越壁"), "bool", true, 0, 1, 1, {}, tr("质量"), tr("壁与壳")},
      {"only_one_wall_top", tr("仅单壁顶层"), "bool", false, 0, 1, 1, {}, tr("质量"), tr("壁与壳")},
      {"precise_outer_wall", tr("精确外壁"), "bool", true, 0, 1, 1, {}, tr("质量"), tr("壁与壳")},
      {"ironing_type", tr("熨烫类型"), "enum", 0, 0, 0, 1, {tr("无"), tr("顶面"), tr("全部")}, tr("质量"), tr("表面质量")},
      {"ironing_speed", tr("熨烫速度 (%)"), "int", 30, 10, 100, 5, {}, tr("质量"), tr("表面质量")},
      {"detect_overhang_wall", tr("检测悬空壁"), "bool", true, 0, 1, 1, {}, tr("质量"), tr("表面质量")},
      // ── 填充 ─────────────────────────────────────────────────
      {"sparse_infill_density", tr("填充密度 (%)"), "int", 15, 0, 100, 5, {}, tr("填充"), tr("填充设置")},
      {"sparse_infill_pattern", tr("填充图案"), "enum", 0, 0, 0, 1, {tr("网格"), tr("直线"), tr("三角"), tr("蜂巢"), tr("陀螺")}, tr("填充"), tr("填充设置")},
      {"infill_direction", tr("填充角度 (°)"), "double", 45.0, 0.0, 90.0, 1.0, {}, tr("填充"), tr("填充设置")},
      // ── 速度 ─────────────────────────────────────────────────
      {"outer_wall_speed", tr("外壁速度 (mm/s)"), "int", 150, 20, 400, 10, {}, tr("速度"), tr("壁速度")},
      {"inner_wall_speed", tr("内壁速度 (mm/s)"), "int", 200, 20, 400, 10, {}, tr("速度"), tr("壁速度")},
      {"sparse_infill_speed", tr("填充速度 (mm/s)"), "int", 200, 20, 400, 10, {}, tr("速度"), tr("填充速度")},
      {"top_surface_speed", tr("顶面速度 (mm/s)"), "int", 150, 20, 300, 10, {}, tr("速度"), tr("填充速度")},
      {"support_speed", tr("支撑速度 (mm/s)"), "int", 100, 20, 300, 10, {}, tr("速度"), tr("支撑速度")},
      {"travel_speed", tr("空走速度 (mm/s)"), "int", 400, 100, 800, 10, {}, tr("速度"), tr("空走速度")},
      {"initial_layer_speed", tr("首层速度 (mm/s)"), "int", 50, 10, 80, 5, {}, tr("速度"), tr("首层速度")},
      {"bridge_speed", tr("桥接速度 (mm/s)"), "int", 80, 20, 200, 10, {}, tr("速度"), tr("特殊速度")},
      {"internal_bridge_speed", tr("内部桥接速度 (mm/s)"), "int", 100, 20, 300, 10, {}, tr("速度"), tr("特殊速度")},
      {"initial_layer_infill_speed", tr("首层填充速度 (mm/s)"), "int", 40, 10, 80, 5, {}, tr("速度"), tr("首层速度")},
      // ── 加速度 ──────────────────────────────────────────────
      {"outer_wall_acceleration", tr("外壁加速度 (mm/s²)"), "int", 3000, 500, 20000, 100, {}, tr("加速度"), tr("壁加速度")},
      {"inner_wall_acceleration", tr("内壁加速度 (mm/s²)"), "int", 4000, 500, 20000, 100, {}, tr("加速度"), tr("壁加速度")},
      {"travel_acceleration", tr("空走加速度 (mm/s²)"), "int", 10000, 1000, 30000, 500, {}, tr("加速度"), tr("空走加速度")},
      {"default_acceleration", tr("默认加速度 (mm/s²)"), "int", 4000, 500, 20000, 100, {}, tr("加速度"), tr("默认加速度")},
      // ── 温度 ─────────────────────────────────────────────────
      {"nozzle_temp", tr("喷嘴温度 (°C)"), "int", 220, 150, 300, 5, {}, tr("温度"), tr("喷嘴温度")},
      {"bed_temp", tr("热床温度 (°C)"), "int", 65, 0, 120, 5, {}, tr("温度"), tr("热床温度")},
      {"chamber_temperature", tr("腔体温度 (°C)"), "int", 0, 0, 60, 5, {}, tr("温度"), tr("腔体温度")},
      {"nozzle_temperature_initial_layer", tr("首层喷嘴温度 (°C)"), "int", 220, 150, 300, 5, {}, tr("温度"), tr("首层温度")},
      {"preheat_time", tr("预热时间 (s)"), "int", 90, 30, 300, 10, {}, tr("温度"), tr("预热"), true},
      // ── 支撑 ─────────────────────────────────────────────────
      {"enable_support", tr("启用支撑"), "bool", false, 0, 1, 1, {}, tr("支撑"), tr("基础支撑")},
      {"support_density", tr("支撑密度 (%)"), "int", 20, 10, 50, 5, {}, tr("支撑"), tr("基础支撑")},
      {"support_type", tr("支撑类型"), "enum", 0, 0, 0, 1, {tr("普通"), tr("树状"), tr("可溶性")}, tr("支撑"), tr("基础支撑")},
      {"support_on_build_plate_only", tr("仅底板支撑"), "bool", true, 0, 1, 1, {}, tr("支撑"), tr("基础支撑")},
      {"support_interface_top_layers", tr("支撑顶面层数"), "int", 2, 0, 10, 1, {}, tr("支撑"), tr("支撑界面")},
      {"support_interface_bottom_layers", tr("支撑底面层数"), "int", 0, 0, 10, 1, {}, tr("支撑"), tr("支撑界面")},
      {"support_angle", tr("支撑角度 (°)"), "int", 45, 20, 90, 1, {}, tr("支撑"), tr("支撑参数")},
      {"support_object_xy_distance", tr("支撑XY间距 (mm)"), "double", 0.35, 0.10, 1.00, 0.05, {}, tr("支撑"), tr("支撑参数")},
      {"support_line_width", tr("支撑线宽"), "double", 0.40, 0.20, 0.80, 0.01, {}, tr("支撑"), tr("支撑参数")},
      {"support_interface_spacing", tr("支撑界面间距"), "double", 0.20, 0.10, 0.50, 0.01, {}, tr("支撑"), tr("支撑界面")},
      {"support_expansion", tr("支撑膨胀 (mm)"), "double", 1.00, 0.00, 5.00, 0.10, {}, tr("支撑"), tr("支撑参数")},
      // ── 底座 ─────────────────────────────────────────────────
      {"brim_enable", tr("启用 Brim"), "bool", false, 0, 1, 1, {}, tr("底座"), tr("Brim")},
      {"brim_width", tr("Brim 宽度 (mm)"), "double", 5.0, 2.0, 20.0, 0.5, {}, tr("底座"), tr("Brim")},
      {"brim_type", tr("Brim 类型"), "enum", 0, 0, 0, 1, {tr("自动"), tr("外圈"), tr("内圈"), tr("全圈")}, tr("底座"), tr("Brim")},
      {"skirt_loops", tr("裙边线数"), "int", 1, 0, 5, 1, {}, tr("底座"), tr("裙边")},
      {"skirt_distance", tr("裙边距离 (mm)"), "double", 3.0, 1.0, 10.0, 0.5, {}, tr("底座"), tr("裙边")},
      {"skirt_height", tr("裙边高度 (层)"), "int", 1, 1, 10, 1, {}, tr("底座"), tr("裙边")},
      {"adhesion_type", tr("附着力类型"), "enum", 0, 0, 0, 1, {tr("无"), tr("裙边"), tr("Brim"), tr("底筏")}, tr("底座"), tr("附着力")},
      {"raft_layers", tr("底筏层数"), "int", 3, 1, 10, 1, {}, tr("底座"), tr("底筏")},
      // ── 冷却 ─────────────────────────────────────────────────
      {"fan_speed", tr("风扇速度 (%)"), "int", 100, 0, 100, 5, {}, tr("冷却"), tr("风扇设置")},
      {"fan_initial_delay", tr("风扇延迟 (层)"), "int", 3, 0, 10, 1, {}, tr("冷却"), tr("风扇设置")},
      {"min_fan_speed", tr("最小风扇 (%)"), "int", 20, 0, 100, 5, {}, tr("冷却"), tr("风扇设置")},
      {"overhang_fan_speed", tr("悬空风扇 (%)"), "int", 80, 0, 100, 5, {}, tr("冷却"), tr("风扇设置")},
      {"slow_down_layer_time", tr("减速层时间 (s)"), "int", 8, 0, 30, 1, {}, tr("冷却"), tr("降温策略")},
      {"close_fan_the_first_x_layers", tr("关闭风扇层数"), "int", 1, 0, 10, 1, {}, tr("冷却"), tr("降温策略")},
      {"slow_down_for_layer_cooling", tr("逐层降温"), "bool", true, 0, 1, 1, {}, tr("冷却"), tr("降温策略")},
      // ── 回退 ─────────────────────────────────────────────────
      {"retract_enable", tr("启用回退"), "bool", true, 0, 1, 1, {}, tr("回退"), tr("回退设置")},
      {"retract_dist", tr("回退距离 (mm)"), "double", 0.8, 0.0, 10.0, 0.1, {}, tr("回退"), tr("回退设置")},
      {"retract_speed", tr("回退速度 (mm/s)"), "int", 45, 20, 150, 5, {}, tr("回退"), tr("回退设置")},
      {"deretraction_speed", tr("回填速度 (mm/s)"), "int", 20, 10, 100, 5, {}, tr("回退"), tr("回退设置")},
      {"retract_length_toolchange", tr("换料回退 (mm)"), "double", 16.0, 0.0, 40.0, 0.5, {}, tr("回退"), tr("换料回退")},
      {"z_hop", tr("Z 抬升 (mm)"), "double", 0.40, 0.00, 2.00, 0.05, {}, tr("回退"), tr("回退设置")},
      {"retract_when_changing_layer", tr("换层回退"), "bool", true, 0, 1, 1, {}, tr("回退"), tr("回退设置")},
      {"wipe_distance", tr("擦拭距离 (mm)"), "double", 0.20, 0.00, 2.00, 0.05, {}, tr("回退"), tr("擦拭")},
      // ── 其他 ─────────────────────────────────────────────────
      {"z_seam_type", tr("Z 缝策略"), "enum", 0, 0, 0, 1, {tr("最锐角"), tr("最短"), tr("随机"), tr("指定")}, tr("其他"), tr("Z 缝")},
      {"z_seam_corner", tr("Z 缝角落"), "enum", 0, 0, 0, 1, {tr("自动"), tr("内角"), tr("外角"), tr("任意")}, tr("其他"), tr("Z 缝")},
      {"enable_coasting", tr("启用 Coasting"), "bool", false, 0, 1, 1, {}, tr("其他"), tr("高级")},
      {"max_print_speed", tr("最大打印速度 (mm/s)"), "int", 300, 20, 500, 10, {}, tr("其他"), tr("高级")},
  };

  for (const auto &option : m_options)
  {
    if (option.readonly)
      m_baseReadonlyKeys.insert(option.key);
  }

  // Snapshot default values for resetToDefaults()
  for (const auto &option : m_options)
    m_defaultValues.insert(option.key, option.value);

  // Mock tooltip descriptions (对齐上游 ConfigOptionDef::tooltip)
  static const QHash<QString, const char *> kTooltips = {
    {"layer_height", QT_TRANSLATE_NOOP("ConfigOption", "每层的打印高度。较小的值提高表面质量但增加打印时间。")},
    {"initial_layer_print_height", QT_TRANSLATE_NOOP("ConfigOption", "首层的打印高度，通常比标准层高更高以确保附着力。")},
    {"line_width", QT_TRANSLATE_NOOP("ConfigOption", "挤出线的宽度。默认自动匹配喷嘴直径。")},
    {"wall_loops", QT_TRANSLATE_NOOP("ConfigOption", "模型外壁的打印圈数。更多圈数增加壁厚和强度。")},
    {"top_shell_layers", QT_TRANSLATE_NOOP("ConfigOption", "模型顶面的实心层数。")},
    {"bottom_shell_layers", QT_TRANSLATE_NOOP("ConfigOption", "模型底面的实心层数。")},
    {"sparse_infill_density", QT_TRANSLATE_NOOP("ConfigOption", "模型内部填充的密度百分比。0% = 空心，100% = 实心。")},
    {"sparse_infill_pattern", QT_TRANSLATE_NOOP("ConfigOption", "内部填充使用的几何图案。不同图案影响强度、速度和耗材用量。")},
    {"outer_wall_speed", QT_TRANSLATE_NOOP("ConfigOption", "外壁打印速度。较低速度获得更好的表面质量。")},
    {"inner_wall_speed", QT_TRANSLATE_NOOP("ConfigOption", "内壁打印速度。可以比外壁更快。")},
    {"sparse_infill_speed", QT_TRANSLATE_NOOP("ConfigOption", "内部填充打印速度。")},
    {"top_surface_speed", QT_TRANSLATE_NOOP("ConfigOption", "顶面打印速度。较低速度获得更平滑的顶面。")},
    {"support_speed", QT_TRANSLATE_NOOP("ConfigOption", "支撑结构的打印速度。")},
    {"travel_speed", QT_TRANSLATE_NOOP("ConfigOption", "空走（非打印）移动速度。")},
    {"initial_layer_speed", QT_TRANSLATE_NOOP("ConfigOption", "首层打印速度。较低速度确保良好的热床附着力。")},
    {"nozzle_temp", QT_TRANSLATE_NOOP("ConfigOption", "喷嘴（热端）温度。")},
    {"bed_temp", QT_TRANSLATE_NOOP("ConfigOption", "热床温度。确保首层良好附着。")},
    {"enable_support", QT_TRANSLATE_NOOP("ConfigOption", "启用支撑结构以支撑悬空部分。")},
    {"support_density", QT_TRANSLATE_NOOP("ConfigOption", "支撑结构的密度百分比。")},
    {"support_type", QT_TRANSLATE_NOOP("ConfigOption", "支撑类型：普通/树状/可溶性。树状支撑更省材料。")},
    {"brim_enable", QT_TRANSLATE_NOOP("ConfigOption", "在模型底部边缘添加 Brim 以增加附着力。")},
    {"brim_width", QT_TRANSLATE_NOOP("ConfigOption", "Brim 的宽度。")},
    {"fan_speed", QT_TRANSLATE_NOOP("ConfigOption", "打印冷却风扇速度百分比。")},
    {"retract_enable", QT_TRANSLATE_NOOP("ConfigOption", "启用回退（retraction）以减少拉丝。")},
    {"retract_dist", QT_TRANSLATE_NOOP("ConfigOption", "回退距离（毫米）。")},
    {"retract_speed", QT_TRANSLATE_NOOP("ConfigOption", "回退速度（毫米/秒）。")},
    {"z_seam_type", QT_TRANSLATE_NOOP("ConfigOption", "Z 缝位置策略。控制层之间的可见接缝放在哪里。")},
    {"max_print_speed", QT_TRANSLATE_NOOP("ConfigOption", "所有打印移动的最大速度限制。")},
    {"ironing_type", QT_TRANSLATE_NOOP("ConfigOption", "熨烫类型：在打印后用热喷嘴熨平表面。")},
    {"adhesion_type", QT_TRANSLATE_NOOP("ConfigOption", "附着力类型：裙边/Brim/底筏。不同的方式确保模型附着在热床上。")},
  };
  for (auto &option : m_options) {
    const auto it = kTooltips.constFind(option.key);
    if (it != kTooltips.cend())
      option.tooltip = tr(it.value());
  }
}

// Category-to-Page mapping (对齐上游 Tab::Page hierarchy)
static QString pageForCategory(const QString &cat)
{
  static const QHash<QString, QString> map = {
    // 中文 category → English page
    {QStringLiteral("质量"), QStringLiteral("Quality")},
    {QStringLiteral("填充"), QStringLiteral("Strength")},
    {QStringLiteral("速度"), QStringLiteral("Speed")},
    {QStringLiteral("加速度"), QStringLiteral("Speed")},
    {QStringLiteral("温度"), QStringLiteral("Temperature")},
    {QStringLiteral("支撑"), QStringLiteral("Support")},
    {QStringLiteral("底座"), QStringLiteral("Base")},
    {QStringLiteral("冷却"), QStringLiteral("Cooling")},
    {QStringLiteral("回退"), QStringLiteral("Retraction")},
    {QStringLiteral("其他"), QStringLiteral("Other")},
    // English category (for upstream schema mode)
    {QStringLiteral("Quality"), QStringLiteral("Quality")},
    {QStringLiteral("Infill"), QStringLiteral("Strength")},
    {QStringLiteral("Speed"), QStringLiteral("Speed")},
    {QStringLiteral("Acceleration"), QStringLiteral("Speed")},
    {QStringLiteral("Temperature"), QStringLiteral("Temperature")},
    {QStringLiteral("Support"), QStringLiteral("Support")},
    {QStringLiteral("Base"), QStringLiteral("Base")},
    {QStringLiteral("Cooling"), QStringLiteral("Cooling")},
    {QStringLiteral("Retraction"), QStringLiteral("Retraction")},
    {QStringLiteral("Other"), QStringLiteral("Other")},
    {QStringLiteral("Seam"), QStringLiteral("Quality")},
    {QStringLiteral("Walls"), QStringLiteral("Quality")},
  };
  return map.value(cat, QStringLiteral("Other"));
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
  case GroupRole:
    return o.group;
  case PageRole:
    return o.page.isEmpty() ? pageForCategory(o.category) : o.page;
  case ReadonlyRole:
    return o.readonly;
  case DirtyRole:
    return m_dirtyKeys.contains(o.key);
  case TooltipRole:
    return o.tooltip;
  case ModeRole:
    return o.mode;
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
      {GroupRole, "optGroup"},
      {PageRole, "optPage"},
      {ReadonlyRole, "optReadonly"},
      {DirtyRole, "optDirty"},
      {TooltipRole, "optTooltip"},
      {ModeRole, "optMode"},
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
QString ConfigOptionModel::optGroup(int i) const { return (i >= 0 && i < m_options.size()) ? m_options[i].group : QString{}; }

QString ConfigOptionModel::optPage(int i) const
{
  if (i >= 0 && i < m_options.size())
    return m_options[i].page.isEmpty() ? pageForCategory(m_options[i].category) : m_options[i].page;
  return QStringLiteral("Other");
}
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

QStringList ConfigOptionModel::optEnumLabelsList(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].enumLabels : QStringList{};
}

QString ConfigOptionModel::optUnit(int i) const
{
  if (i < 0 || i >= m_options.size())
    return {};
  const QString &key = m_options[i].key;
  if (key.contains("speed") || key.contains("feed"))
    return QStringLiteral("mm/s");
  if (key.contains("temp") || key.contains("preheat"))
    return QStringLiteral("°C");
  if (key.contains("density") || key.contains("fan") || key.contains("flow") || key.contains("overlap") || key.contains("ironing"))
    return QStringLiteral("%");
  if (key.contains("angle"))
    return QStringLiteral("°");
  if (key.contains("time") || key.contains("delay"))
    return QStringLiteral("s");
  if (key.contains("distance"))
    return QStringLiteral("mm");
  // Countable items (loops, layers, count) have no unit
  if (key.contains("loops") || key.contains("count"))
    return {};
  return QStringLiteral("mm");
}

QString ConfigOptionModel::optTooltip(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].tooltip : QString{};
}

int ConfigOptionModel::optMode(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].mode : 2; // 2=Both default
}

bool ConfigOptionModel::optIsDirty(int i) const
{
  if (i < 0 || i >= m_options.size())
    return false;
  return m_dirtyKeys.contains(m_options[i].key);
}

void ConfigOptionModel::resetOption(int row)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto &key = m_options[row].key;
  const auto it = m_defaultValues.constFind(key);
  if (it == m_defaultValues.cend())
    return;
  m_options[row].value = it.value();
  m_dirtyKeys.remove(key);
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  emit optionValueChanged(key, it.value());
  ++m_dataVersion;
  emit dataVersionChanged();
}

int ConfigOptionModel::dirtyCount() const
{
  return m_dirtyKeys.size();
}

void ConfigOptionModel::setValue(int row, const QVariant &value)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto &key = m_options[row].key;
  m_options[row].value = value;
  // 更新脏状态：与默认值比较
  const bool wasDirty = m_dirtyKeys.contains(key);
  const bool nowDirty = (m_defaultValues.value(key) != value);
  if (nowDirty)
    m_dirtyKeys.insert(key);
  else
    m_dirtyKeys.remove(key);
  const QModelIndex idx = index(row);
  const QList<int> roles = {ValueRole};
  if (wasDirty != nowDirty)
    emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  else
    emit dataChanged(idx, idx, roles);
  emit optionValueChanged(key, value);
  ++m_dataVersion;
  emit dataVersionChanged();
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

QStringList ConfigOptionModel::pageNames() const
{
  QSet<QString> pages;
  pages.reserve(8);
  const QStringList pageOrder = {
    QStringLiteral("Quality"), QStringLiteral("Strength"), QStringLiteral("Speed"),
    QStringLiteral("Temperature"), QStringLiteral("Support"), QStringLiteral("Base"),
    QStringLiteral("Cooling"), QStringLiteral("Retraction"), QStringLiteral("Other")
  };
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &pg = m_options[i].page.isEmpty() ? pageForCategory(m_options[i].category) : m_options[i].page;
    if (!pg.isEmpty())
      pages.insert(pg);
  }
  QStringList result;
  result.reserve(pages.size());
  for (const QString &p : pageOrder)
    if (pages.contains(p))
      result.append(p);
  return result;
}

QList<int> ConfigOptionModel::filterIndicesByPage(const QList<int> &indices, const QString &page) const
{
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (idx < 0 || idx >= m_options.size())
      continue;
    const QString &pg = m_options[idx].page.isEmpty() ? pageForCategory(m_options[idx].category) : m_options[idx].page;
    if (pg == page)
      result.append(idx);
  }
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

QHash<QString, QVariant> ConfigOptionModel::defaultValuesByKey() const
{
  return m_defaultValues;
}

void ConfigOptionModel::resetToDefaults()
{
  if (m_defaultValues.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const auto it = m_defaultValues.constFind(m_options[i].key);
    if (it != m_defaultValues.cend() && m_options[i].value != it.value())
    {
      m_options[i].value = it.value();
      changed = true;
    }
  }

  if (changed)
  {
    m_dirtyKeys.clear();
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole, DirtyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
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
    // 更新脏状态
    const bool nowDirty = (m_defaultValues.value(m_options[i].key) != it.value());
    if (nowDirty)
      m_dirtyKeys.insert(m_options[i].key);
    else
      m_dirtyKeys.remove(m_options[i].key);
    changed = true;
  }

  if (changed)
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {ValueRole, DirtyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
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
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {ReadonlyRole});
    ++m_dataVersion;
    emit dataVersionChanged();
  }
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
  "initial_layer_line_width",
  // Shell
  "wall_loops", "top_shell_layers", "bottom_shell_layers",
  "wall_infill_order", "infill_wall_overlap",
  "top_bottom_infill_wall_overlap",
  "outer_wall_line_width", "inner_wall_line_width",
  "wall_sequence", "ensure_vertical_shell_thickness",
  "extra_perimeters_on_overhangs",
  // Infill
  "sparse_infill_density", "sparse_infill_pattern",
  "top_surface_pattern", "bottom_surface_pattern",
  "sparse_infill_filament", "wall_filament",
  "infill_direction", "solid_infill_direction",
  "infill_anchor", "infill_anchor_max",
  // Speed
  "outer_wall_speed", "inner_wall_speed", "travel_speed", "initial_layer_speed",
  "sparse_infill_speed", "top_surface_speed", "support_interface_speed",
  "support_speed", "small_perimeter_speed", "bridge_speed",
  "internal_bridge_speed", "initial_layer_infill_speed",
  "gap_infill_speed", "internal_solid_infill_speed",
  // Acceleration
  "outer_wall_acceleration", "inner_wall_acceleration", "travel_acceleration",
  "top_surface_acceleration", "bridge_acceleration",
  "sparse_infill_acceleration", "default_acceleration",
  "initial_layer_acceleration",
  // Jerk
  "default_jerk", "outer_wall_jerk", "inner_wall_jerk",
  "infill_jerk", "travel_jerk", "initial_layer_jerk",
  // Temperature
  "nozzle_temp", "bed_temp", "chamber_temperature",
  "nozzle_temperature_range", "bed_temperature_range",
  "nozzle_temperature_initial_layer",
  "cool_plate_temp", "eng_plate_temp", "hot_plate_temp",
  // Support
  "enable_support", "support_density", "support_type",
  "support_on_build_plate_only", "support_interface_top_layers",
  "support_interface_bottom_layers", "support_interface_spacing",
  "support_object_xy_distance", "support_angle",
  "support_remove_small_overhang",
  "support_top_z_distance", "support_bottom_z_distance",
  "support_speed", "support_line_width",
  "support_interface_speed", "support_filament",
  "support_interface_filament",
  "support_base_pattern", "support_expansion",
  // Brim / Skirt
  "brim_enable", "brim_width", "brim_type", "brim_object_gap",
  "skirt_loops", "skirt_distance", "skirt_height", "skirt_speed",
  // Cooling / Fan
  "fan_cooling_layer_time", "default_fan_speed",
  "min_fan_speed", "max_fan_speed",
  "overhang_fan_speed", "slow_down_layer_time",
  "additional_cooling_fan_speed",
  "close_fan_the_first_x_layers",
  "overhang_fan_threshold", "overhang_bridge_fan",
  "slow_down_for_layer_cooling",
  // Retraction
  "retract_length", "retract_speed", "retract_before_wipe",
  "retraction_speed", "deretraction_speed", "retract_restart_extra",
  "wipe_distance", "wipe_speed",
  "retraction_minimum_travel", "retract_when_changing_layer",
  "z_hop", "reduce_infill_retraction",
  "retract_length_toolchange",
  // Seam
  "z_seam_type", "z_seam_position", "z_seam_corner",
  // Quality
  "reduce_crossing_wall", "detect_overhang_wall",
  "resolve_multi_overlaps", "max_travel_detour_distance",
  "only_one_wall_top", "only_one_wall_first_layer",
  "precise_outer_wall",
  // Adhesion
  "adhesion_type", "raft_layers", "raft_interface_layers",
  "raft_contact_distance", "raft_expansion",
  "draft_shield",
  // Output
  "gcode_comments", "gcode_precision_xy", "gcode_precision_z",
  "max_print_speed", "max_travel_speed",
  "gcode_flavor", "filename_format",
  "machine_start_gcode", "machine_end_gcode",
  "layer_change_gcode", "before_layer_change_gcode",
  // Extruder
  "nozzle_diameter", "extruder_offset", "printer_model",
  // Ironing
  "ironing_type", "ironing_speed", "ironing_flow",
  "only_top_surface",
  // Output advanced
  "time_lapse_gcode", "spaghetti_detector",
  "scan_first_layer", "gcode_label_objects",
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
  emit countChanged();
}

#endif // HAS_LIBSLIC3R
