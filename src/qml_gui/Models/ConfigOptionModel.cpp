#include "ConfigOptionModel.h"

#include <QByteArray>
#include <QDebug>
#include <QVector>
#include <initializer_list>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Config.hpp>
#include <libslic3r/Preset.hpp>
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

  assignProcessHierarchyMetadata();
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
    {QStringLiteral("挤出机"), QStringLiteral("Other")},
    {QStringLiteral("输出"), QStringLiteral("Other")},
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
  case NullableRole:
    return o.nullable;
  case IsVectorRole:
    return o.isVector;
  case SidetextRole:
    return o.sidetext;
  default:
    return {};
  }
}

bool ConfigOptionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid() || index.row() >= m_options.size())
    return false;
  auto &o = m_options[index.row()];
  switch (role)
  {
  case PageRole:
    o.page = value.toString();
    emit dataChanged(index, index, {PageRole});
    return true;
  case GroupRole:
    o.group = value.toString();
    emit dataChanged(index, index, {GroupRole});
    return true;
  default:
    return false;
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
      {NullableRole, "optNullable"},
      {IsVectorRole, "optIsVector"},
      {SidetextRole, "optSidetext"},
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
  // Prefer upstream sidetext (对齐上游 ConfigOptionDef::sidetext)
  if (!m_options[i].sidetext.isEmpty())
    return m_options[i].sidetext;
  // Fallback: key-based heuristic
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

bool ConfigOptionModel::optNullable(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].nullable : false;
}

bool ConfigOptionModel::optIsVector(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].isVector : false;
}

QString ConfigOptionModel::optSidetext(int i) const
{
  return (i >= 0 && i < m_options.size()) ? m_options[i].sidetext : QString{};
}

QStringList ConfigOptionModel::groupNames() const
{
  QSet<QString> groups;
  for (const auto &o : m_options)
    if (!o.group.isEmpty())
      groups.insert(o.group);
  QStringList result = groups.values();
  result.sort();
  return result;
}

int ConfigOptionModel::countForGroup(const QString &group) const
{
  int count = 0;
  for (const auto &o : m_options) {
    if (o.group == group)
      ++count;
  }
  return count;
}

int ConfigOptionModel::dirtyCountForGroup(const QString &group) const
{
  int count = 0;
  for (const auto &key : m_dirtyKeys) {
    int idx = findIndex(key);
    if (idx >= 0 && m_options[idx].group == group)
      ++count;
  }
  return count;
}

void ConfigOptionModel::resetOption(int row)
{
  if (row < 0 || row >= m_options.size())
    return;
  const auto &key = m_options[row].key;
  const QVariant targetValue = m_referenceValues.contains(key)
      ? m_referenceValues.value(key)
      : m_defaultValues.value(key);
  if (!targetValue.isValid())
    return;
  m_options[row].value = targetValue;
  m_dirtyKeys.remove(key);
  const QModelIndex idx = index(row);
  emit dataChanged(idx, idx, {ValueRole, DirtyRole});
  emit optionValueChanged(key, targetValue);
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
  const QVariant referenceValue = m_referenceValues.contains(key)
      ? m_referenceValues.value(key)
      : m_defaultValues.value(key);
  const bool wasDirty = m_dirtyKeys.contains(key);
  const bool nowDirty = (referenceValue != value);
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
  // Collect every page actually present on a loaded option. A page is the
  // explicit per-option page if assigned (e.g. via assignPageGroupForTier from
  // upstream Tab.cpp), otherwise the legacy category-derived fallback.
  QSet<QString> pages;
  pages.reserve(12);
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &pg = m_options[i].page.isEmpty() ? pageForCategory(m_options[i].category) : m_options[i].page;
    if (!pg.isEmpty())
      pages.insert(pg);
  }

  // Comprehensive cross-tier ordering (Print + Filament + Printer pages from
  // upstream Tab.cpp). Known pages emit in this stable order; any page not in
  // the list (future tiers / upstream additions) appends sorted so nothing is
  // silently dropped.
  const QStringList knownOrder = {
    // Print tier (TabPrint::build)
    QStringLiteral("Quality"), QStringLiteral("Strength"), QStringLiteral("Speed"),
    QStringLiteral("Temperature"), QStringLiteral("Support"), QStringLiteral("Base"),
    QStringLiteral("Cooling"), QStringLiteral("Retraction"), QStringLiteral("Other"),
    // Filament tier (TabFilament::build)
    QStringLiteral("Filament"), QStringLiteral("Advanced"), QStringLiteral("Multimaterial"),
    QStringLiteral("Dependencies"), QStringLiteral("Notes"),
    // Printer tier (TabPrinter::build_fff)
    QStringLiteral("Basic information"), QStringLiteral("Machine G-code"),
    QStringLiteral("Motion ability")
  };

  QStringList result;
  result.reserve(pages.size());
  for (const QString &p : knownOrder)
    if (pages.contains(p))
      result.append(p);

  // Append any remaining present pages not covered by knownOrder (sorted for
  // deterministic output) so no page is hidden by the ordering table.
  QStringList remainder;
  for (const QString &p : pages)
    if (!result.contains(p))
      remainder.append(p);
  remainder.sort();
  result.append(remainder);
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

QList<int> ConfigOptionModel::filterIndicesByGroup(const QList<int> &indices, const QString &group) const
{
  QList<int> result;
  result.reserve(indices.size());
  for (int idx : indices)
  {
    if (idx < 0 || idx >= m_options.size())
      continue;
    if (m_options[idx].group == group)
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
    const QVariant referenceValue = m_referenceValues.contains(m_options[i].key)
        ? m_referenceValues.value(m_options[i].key)
        : m_defaultValues.value(m_options[i].key);
    const bool nowDirty = (referenceValue != it.value());
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

void ConfigOptionModel::setReferenceValues(const QHash<QString, QVariant> &values)
{
  m_referenceValues = values;
  if (m_options.isEmpty())
    return;

  bool changed = false;
  for (int i = 0; i < m_options.size(); ++i)
  {
    const QString &key = m_options[i].key;
    const QVariant referenceValue = m_referenceValues.contains(key)
        ? m_referenceValues.value(key)
        : m_defaultValues.value(key);
    const bool wasDirty = m_dirtyKeys.contains(key);
    const bool nowDirty = (referenceValue != m_options[i].value);
    if (nowDirty)
      m_dirtyKeys.insert(key);
    else
      m_dirtyKeys.remove(key);
    changed = changed || (wasDirty != nowDirty);
  }

  if (changed)
  {
    emit dataChanged(index(0), index(m_options.size() - 1), {DirtyRole});
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

namespace
{
#ifdef HAS_LIBSLIC3R
  // Map upstream ConfigOptionType to Qt6 type string
  QString mapType(Slic3r::ConfigOptionType t)
  {
    switch (t)
    {
    case Slic3r::coPercent:
    case Slic3r::coPercents:
      return QStringLiteral("percent");
    case Slic3r::coFloat:
    case Slic3r::coFloatOrPercent:
    case Slic3r::coFloats:
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
    if (cat.contains("Acceleration", Qt::CaseInsensitive) || cat.contains("Jerk", Qt::CaseInsensitive))
      return QObject::tr("加速度");
    if (cat.contains("Temperature", Qt::CaseInsensitive))
      return QObject::tr("温度");
    if (cat.contains("Support", Qt::CaseInsensitive))
      return QObject::tr("支撑");
    if (cat.contains("Skirt", Qt::CaseInsensitive) || cat.contains("Brim", Qt::CaseInsensitive) || cat.contains("Raft", Qt::CaseInsensitive))
      return QObject::tr("底座");
    if (cat.contains("Cooling", Qt::CaseInsensitive) || cat.contains("Fan", Qt::CaseInsensitive))
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
    if (cat.contains("Prime", Qt::CaseInsensitive) || cat.contains("Wipe", Qt::CaseInsensitive))
      return QObject::tr("底座");
    // Machine-specific categories (对齐上游 TabPrinter)
    if (cat.contains("Printable", Qt::CaseInsensitive) || cat.contains("Bed", Qt::CaseInsensitive))
      return QObject::tr("打印空间");
    if (cat.contains("Motion", Qt::CaseInsensitive))
      return QObject::tr("运动能力");
    if (cat.contains("Multi", Qt::CaseInsensitive))
      return QObject::tr("多材料");
    // Filament-specific categories (对齐上游 TabFilament)
    if (cat.contains("Filament", Qt::CaseInsensitive))
      return QObject::tr("耗材");
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
    // Vector types (per-extruder): bb3 upstream stores speed/accel/jerk/temp as
    // coFloats/coInts/coBools/coFloatsOrPercents/coPercents. For the Qt6 single-value
    // editor we surface the first element (extruder 0) as the default value — this
    // is what single-extruder printers need, and matches the pre-bb3 OWzx behavior
    // when these fields were rolled back to single-value types. Multi-extruder
    // per-extruder editing is a follow-up UI surface (see v5.4 Phase 183 PLAN.md).
    case Slic3r::coFloats:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionFloats>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coInts:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionInts>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coBools:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionBools>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coFloatsOrPercents:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionFloatsOrPercents>();
      return v->values.empty() ? QVariant() : QVariant::fromValue(v->values[0]);
    }
    case Slic3r::coPercents:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionPercents>();
      return v->values.empty() ? QVariant() : QVariant(v->values[0]);
    }
    case Slic3r::coStrings:
    {
      const auto *v = def->get_default_value<Slic3r::ConfigOptionStrings>();
      return v->values.empty() ? QVariant() : QString::fromStdString(v->values[0]);
    }
    default:
      return {};
    }
  }

  // ── Static page-to-group mapping tables ────────────────────────────
  // Upstream Tab.cpp:2311-2788 (TabPrint::build), 3892-4216 (TabFilament::build),
  // 4438-4900 (TabPrinter::build_fff). Each entry maps option_key -> (page, group).
  // These are derived from upstream new_optgroup / append_single_option_line calls.
  // Options not in the table will appear under an "Other" bucket in the dialog.

  // Upstream: Tab.cpp:4438-4900 (TabPrinter::build_fff)
  static const QHash<QString, QPair<QString, QString>> &kPrinterPageGroupMap()
  {
    static const QHash<QString, QPair<QString, QString>> m = {
      // Page: "Basic information" - Groups: Printable space, Advanced, Cooling Fan, Extruder Clearance, Adaptive bed mesh, Accessory
      {QStringLiteral("printable_area"),        {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("printable_height"),      {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("bed_exclude_area"),       {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("z_offset"),               {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("preferred_orientation"),  {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("nozzle_volume"),          {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("best_object_pos"),        {QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("support_multi_bed_types"),{QStringLiteral("Basic information"), QStringLiteral("Printable space")}},
      {QStringLiteral("printer_structure"),      {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("gcode_flavor"),           {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("use_relative_e_distances"),{QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("use_firmware_retraction"),{QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("machine_load_filament_time"),{QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("machine_unload_filament_time"),{QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("nozzle_type"),            {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("nozzle_hrc"),             {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("disable_m73"),            {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("thumbnails"),             {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("time_cost"),              {QStringLiteral("Basic information"), QStringLiteral("Advanced")}},
      {QStringLiteral("auxiliary_fan"),           {QStringLiteral("Basic information"), QStringLiteral("Cooling Fan")}},
      {QStringLiteral("support_chamber_temp_control"),{QStringLiteral("Basic information"), QStringLiteral("Cooling Fan")}},
      {QStringLiteral("support_air_filtration"),{QStringLiteral("Basic information"), QStringLiteral("Cooling Fan")}},
      // Page: "Machine G-code" - Groups: File header G-code, Machine start/end G-code, etc.
      {QStringLiteral("machine_start_gcode"),     {QStringLiteral("Machine G-code"), QStringLiteral("Machine start G-code")}},
      {QStringLiteral("machine_end_gcode"),      {QStringLiteral("Machine G-code"), QStringLiteral("Machine end G-code")}},
      {QStringLiteral("before_layer_change_gcode"),{QStringLiteral("Machine G-code"), QStringLiteral("Before layer change G-code")}},
      {QStringLiteral("layer_change_gcode"),     {QStringLiteral("Machine G-code"), QStringLiteral("Layer change G-code")}},
      {QStringLiteral("time_lapse_gcode"),       {QStringLiteral("Machine G-code"), QStringLiteral("Timelapse G-code")}},
      {QStringLiteral("change_filament_gcode"),  {QStringLiteral("Machine G-code"), QStringLiteral("Change filament G-code")}},
      {QStringLiteral("change_extrusion_role_gcode"),{QStringLiteral("Machine G-code"), QStringLiteral("Change extrusion role G-code")}},
      {QStringLiteral("machine_pause_gcode"),    {QStringLiteral("Machine G-code"), QStringLiteral("Pause G-code")}},
      {QStringLiteral("template_custom_gcode"),  {QStringLiteral("Machine G-code"), QStringLiteral("Template Custom G-code")}},
      {QStringLiteral("printing_by_object_gcode"),{QStringLiteral("Machine G-code"), QStringLiteral("Printing by object G-code")}},
      // Page: "Notes" - Groups: Notes
      {QStringLiteral("printer_notes"),          {QStringLiteral("Notes"), QStringLiteral("Notes")}},
      // Page: "Motion ability" - Groups: Advanced, Resonance Compensation, Speed limitation, Acceleration limitation, Jerk limitation
      {QStringLiteral("emit_machine_limits_to_gcode"),{QStringLiteral("Motion ability"), QStringLiteral("Advanced")}},
      {QStringLiteral("machine_max_speed_x"),    {QStringLiteral("Motion ability"), QStringLiteral("Speed limitation")}},
      {QStringLiteral("machine_max_speed_y"),    {QStringLiteral("Motion ability"), QStringLiteral("Speed limitation")}},
      {QStringLiteral("machine_max_speed_z"),    {QStringLiteral("Motion ability"), QStringLiteral("Speed limitation")}},
      {QStringLiteral("machine_max_speed_e"),    {QStringLiteral("Motion ability"), QStringLiteral("Speed limitation")}},
      {QStringLiteral("machine_max_acceleration_x"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_y"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_z"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_e"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_extruding"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_retracting"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_acceleration_travel"),{QStringLiteral("Motion ability"), QStringLiteral("Acceleration limitation")}},
      {QStringLiteral("machine_max_jerk_x"),     {QStringLiteral("Motion ability"), QStringLiteral("Jerk limitation")}},
      {QStringLiteral("machine_max_jerk_y"),     {QStringLiteral("Motion ability"), QStringLiteral("Jerk limitation")}},
      {QStringLiteral("machine_max_jerk_z"),     {QStringLiteral("Motion ability"), QStringLiteral("Jerk limitation")}},
      {QStringLiteral("machine_max_jerk_e"),     {QStringLiteral("Motion ability"), QStringLiteral("Jerk limitation")}},
      // Extruder pages - Groups: Size, Retraction
      {QStringLiteral("nozzle_diameter"),         {QStringLiteral("Motion ability"), QStringLiteral("Size")}},
      {QStringLiteral("min_layer_height"),       {QStringLiteral("Motion ability"), QStringLiteral("Size")}},
      {QStringLiteral("max_layer_height"),       {QStringLiteral("Motion ability"), QStringLiteral("Size")}},
      {QStringLiteral("extruder_offset"),        {QStringLiteral("Motion ability"), QStringLiteral("Size")}},
      {QStringLiteral("retraction_length"),      {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retract_restart_extra"),  {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("z_hop"),                  {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("z_hop_types"),            {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retraction_speed"),       {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("deretraction_speed"),     {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retraction_minimum_travel"),{QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retract_when_changing_layer"),{QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("wipe"),                   {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("wipe_distance"),          {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retract_before_wipe"),     {QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retract_length_toolchange"),{QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("retract_restart_extra_toolchange"),{QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      {QStringLiteral("long_retractions_when_cut"),{QStringLiteral("Motion ability"), QStringLiteral("Retraction")}},
      // Extruder Clearance (for multi-extruder printers)
      // These aren't actual option keys but are relevant groups
    };
    return m;
  }

  // Upstream: Tab.cpp:3892-4216 (TabFilament::build)
  static const QHash<QString, QPair<QString, QString>> &kFilamentPageGroupMap()
  {
    static const QHash<QString, QPair<QString, QString>> m = {
      // Page: "Filament" - Groups: Basic information, Flow ratio and Pressure Advance, etc.
      {QStringLiteral("filament_type"),           {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_vendor"),         {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_colour"),         {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_density"),        {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_cost"),           {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_spool_weight"),   {QStringLiteral("Filament"), QStringLiteral("Basic information")}},
      {QStringLiteral("filament_flow_ratio"),     {QStringLiteral("Filament"), QStringLiteral("Flow ratio and Pressure Advance")}},
      {QStringLiteral("filament_max_volumetric_speed"),{QStringLiteral("Filament"), QStringLiteral("Volumetric speed limitation")}},
      {QStringLiteral("filament_min_speed"),      {QStringLiteral("Filament"), QStringLiteral("Volumetric speed limitation")}},
      // Temperature groups
      {QStringLiteral("nozzle_temperature"),      {QStringLiteral("Filament"), QStringLiteral("Print temperature")}},
      {QStringLiteral("nozzle_temperature_initial_layer"),{QStringLiteral("Filament"), QStringLiteral("Print temperature")}},
      {QStringLiteral("nozzle_temperature_range"),{QStringLiteral("Filament"), QStringLiteral("Print temperature")}},
      {QStringLiteral("hot_plate_temp"),          {QStringLiteral("Filament"), QStringLiteral("Bed temperature")}},
      {QStringLiteral("hot_plate_temp_initial_layer"),{QStringLiteral("Filament"), QStringLiteral("Bed temperature")}},
      {QStringLiteral("cool_plate_temp"),         {QStringLiteral("Filament"), QStringLiteral("Bed temperature")}},
      {QStringLiteral("chamber_temperature"),     {QStringLiteral("Filament"), QStringLiteral("Print chamber temperature")}},
      // Page: "Cooling"
      {QStringLiteral("fan_cooling_layer_time"),   {QStringLiteral("Cooling"), QStringLiteral("Cooling for specific layer")}},
      {QStringLiteral("slow_down_min_speed"),     {QStringLiteral("Cooling"), QStringLiteral("Cooling for specific layer")}},
      {QStringLiteral("fan_min_speed"),           {QStringLiteral("Cooling"), QStringLiteral("Part cooling fan")}},
      {QStringLiteral("fan_max_speed"),           {QStringLiteral("Cooling"), QStringLiteral("Part cooling fan")}},
      {QStringLiteral("default_fan_speed"),      {QStringLiteral("Cooling"), QStringLiteral("Part cooling fan")}},
      {QStringLiteral("overhang_fan_speed"),     {QStringLiteral("Cooling"), QStringLiteral("Part cooling fan")}},
      {QStringLiteral("close_fan_the_first_x_layers"),{QStringLiteral("Cooling"), QStringLiteral("Part cooling fan")}},
      // Retraction
      {QStringLiteral("filament_retraction_length"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retraction_speed"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_deretraction_speed"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_z_hop"),          {QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retract_restart_extra"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retract_length_toolchange"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retract_restart_extra_toolchange"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retract_lift_above"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      {QStringLiteral("filament_retract_lift_below"),{QStringLiteral("Filament"), QStringLiteral("Retraction")}},
      // Page: "Advanced"
      {QStringLiteral("filament_start_gcode"),    {QStringLiteral("Advanced"), QStringLiteral("Filament start G-code")}},
      {QStringLiteral("filament_end_gcode"),      {QStringLiteral("Advanced"), QStringLiteral("Filament end G-code")}},
      {QStringLiteral("filament_change_gcode"),   {QStringLiteral("Advanced"), QStringLiteral("Change extrusion role G-code")}},
      // Page: "Notes"
      {QStringLiteral("filament_notes"),          {QStringLiteral("Notes"), QStringLiteral("Notes")}},
    };
    return m;
  }

#endif // HAS_LIBSLIC3R

  struct ProcessGroupDefinition
  {
    QString name;
    QStringList optionKeys;
  };

  struct ProcessPageDefinition
  {
    QString name;
    QList<ProcessGroupDefinition> groups;
  };

  static QStringList processKeys(std::initializer_list<const char *> keys)
  {
    QStringList result;
    result.reserve(static_cast<qsizetype>(keys.size()));
    for (const char *key : keys)
      result.append(QString::fromLatin1(key));
    return result;
  }

  // Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376.
  static const QList<ProcessPageDefinition> &processPageDefinitions()
  {
    static const QList<ProcessPageDefinition> definitions = {
        {QStringLiteral("Quality"), {
            {QStringLiteral("Layer height"), processKeys({"layer_height", "initial_layer_print_height"})},
            {QStringLiteral("Line width"), processKeys({"line_width", "initial_layer_line_width", "outer_wall_line_width", "inner_wall_line_width", "top_surface_line_width", "sparse_infill_line_width", "internal_solid_infill_line_width", "support_line_width"})},
            {QStringLiteral("Seam"), processKeys({"seam_position", "staggered_inner_seams", "seam_gap", "seam_slope_type", "seam_slope_conditional", "scarf_angle_threshold", "scarf_overhang_threshold", "scarf_joint_speed", "seam_slope_start_height", "seam_slope_entire_loop", "seam_slope_min_length", "seam_slope_steps", "scarf_joint_flow_ratio", "seam_slope_inner_walls", "role_based_wipe_speed", "wipe_speed", "wipe_on_loops", "wipe_before_external_loop"})},
            {QStringLiteral("Precision"), processKeys({"slice_closing_radius", "resolution", "enable_arc_fitting", "xy_hole_compensation", "xy_contour_compensation", "elefant_foot_compensation", "elefant_foot_compensation_layers", "precise_outer_wall", "precise_z_height", "hole_to_polyhole", "hole_to_polyhole_threshold", "hole_to_polyhole_twisted"})},
            {QStringLiteral("Ironing"), processKeys({"ironing_type", "ironing_pattern", "ironing_speed", "ironing_flow", "ironing_spacing", "ironing_angle"})},
            {QStringLiteral("Wall generator"), processKeys({"wall_generator", "wall_transition_angle", "wall_transition_filter_deviation", "wall_transition_length", "wall_distribution_count", "initial_layer_min_bead_width", "min_bead_width", "min_feature_size", "min_length_factor"})},
            {QStringLiteral("Walls and surfaces"), processKeys({"wall_sequence", "is_infill_first", "wall_direction", "print_flow_ratio", "top_solid_infill_flow_ratio", "bottom_solid_infill_flow_ratio", "only_one_wall_top", "min_width_top_surface", "only_one_wall_first_layer", "reduce_crossing_wall", "max_travel_detour_distance", "small_area_infill_flow_compensation", "small_area_infill_flow_compensation_model"})},
            {QStringLiteral("Bridging"), processKeys({"bridge_flow", "internal_bridge_flow", "bridge_density", "thick_bridges", "thick_internal_bridges", "dont_filter_internal_bridges", "counterbore_hole_bridging"})},
            {QStringLiteral("Overhangs"), processKeys({"detect_overhang_wall", "make_overhang_printable", "make_overhang_printable_angle", "make_overhang_printable_hole_size", "extra_perimeters_on_overhangs", "overhang_reverse", "overhang_reverse_internal_only", "overhang_reverse_threshold"})},
        }},
        {QStringLiteral("Strength"), {
            {QStringLiteral("Walls"), processKeys({"wall_loops", "alternate_extra_wall", "detect_thin_wall"})},
            {QStringLiteral("Top/bottom shells"), processKeys({"top_shell_layers", "top_shell_thickness", "top_surface_pattern", "bottom_shell_layers", "bottom_shell_thickness", "bottom_surface_pattern", "top_bottom_infill_wall_overlap"})},
            {QStringLiteral("Infill"), processKeys({"sparse_infill_density", "sparse_infill_pattern", "infill_anchor_max", "infill_anchor", "internal_solid_infill_pattern", "gap_fill_target", "filter_out_gap_fill", "infill_wall_overlap"})},
            {QStringLiteral("Advanced"), processKeys({"infill_direction", "solid_infill_direction", "rotate_solid_infill_direction", "bridge_angle", "minimum_sparse_infill_area", "infill_combination", "infill_combination_max_layer_height", "detect_narrow_internal_solid_infill", "ensure_vertical_shell_thickness"})},
        }},
        {QStringLiteral("Speed"), {
            {QStringLiteral("Initial layer speed"), processKeys({"initial_layer_speed", "initial_layer_infill_speed", "initial_layer_travel_speed", "slow_down_layers"})},
            {QStringLiteral("Other layers speed"), processKeys({"outer_wall_speed", "inner_wall_speed", "small_perimeter_speed", "small_perimeter_threshold", "sparse_infill_speed", "internal_solid_infill_speed", "top_surface_speed", "gap_infill_speed", "support_speed", "support_interface_speed"})},
            {QStringLiteral("Overhang speed"), processKeys({"enable_overhang_speed", "slowdown_for_curled_perimeters", "overhang_1_4_speed", "overhang_2_4_speed", "overhang_3_4_speed", "overhang_4_4_speed", "bridge_speed", "internal_bridge_speed"})},
            {QStringLiteral("Travel speed"), processKeys({"travel_speed"})},
            {QStringLiteral("Acceleration"), processKeys({"default_acceleration", "outer_wall_acceleration", "inner_wall_acceleration", "bridge_acceleration", "sparse_infill_acceleration", "internal_solid_infill_acceleration", "initial_layer_acceleration", "top_surface_acceleration", "travel_acceleration", "accel_to_decel_enable", "accel_to_decel_factor"})},
            {QStringLiteral("Jerk(XY)"), processKeys({"default_jerk", "outer_wall_jerk", "inner_wall_jerk", "infill_jerk", "top_surface_jerk", "initial_layer_jerk", "travel_jerk"})},
            {QStringLiteral("Advanced"), processKeys({"max_volumetric_extrusion_rate_slope", "max_volumetric_extrusion_rate_slope_segment_length", "extrusion_rate_smoothing_external_perimeter_only"})},
        }},
        {QStringLiteral("Support"), {
            {QStringLiteral("Support"), processKeys({"enable_support", "support_type", "support_style", "support_threshold_angle", "raft_first_layer_density", "raft_first_layer_expansion", "support_on_build_plate_only", "support_critical_regions_only", "support_remove_small_overhang"})},
            {QStringLiteral("Raft"), processKeys({"raft_layers", "raft_contact_distance"})},
            {QStringLiteral("Support filament"), processKeys({"support_filament", "support_interface_filament", "support_interface_not_for_body"})},
            {QStringLiteral("Advanced"), processKeys({"support_top_z_distance", "support_bottom_z_distance", "support_base_pattern", "support_base_pattern_spacing", "support_angle", "support_interface_top_layers", "support_interface_bottom_layers", "support_interface_pattern", "support_interface_spacing", "support_bottom_interface_spacing", "support_expansion", "support_object_xy_distance", "bridge_no_support", "max_bridge_length", "independent_support_layer_height"})},
            {QStringLiteral("Tree supports"), processKeys({"tree_support_tip_diameter", "tree_support_branch_distance", "tree_support_branch_distance_organic", "tree_support_top_rate", "tree_support_branch_diameter", "tree_support_branch_diameter_organic", "tree_support_branch_diameter_angle", "tree_support_branch_angle", "tree_support_branch_angle_organic", "tree_support_angle_slow", "tree_support_branch_diameter_double_wall", "tree_support_wall_count", "tree_support_adaptive_layer_height", "tree_support_auto_brim", "tree_support_brim_width"})},
        }},
        {QStringLiteral("Multimaterial"), {
            {QStringLiteral("Prime tower"), processKeys({"enable_prime_tower", "prime_tower_width", "prime_volume", "prime_tower_brim_width", "wipe_tower_rotation_angle", "wipe_tower_bridging", "wipe_tower_cone_angle", "wipe_tower_extra_spacing", "wipe_tower_extra_flow", "wipe_tower_max_purge_speed", "wipe_tower_no_sparse_layers", "single_extruder_multi_material_priming"})},
            {QStringLiteral("Filament for Features"), processKeys({"wall_filament", "sparse_infill_filament", "solid_infill_filament", "wipe_tower_filament"})},
            {QStringLiteral("Ooze prevention"), processKeys({"ooze_prevention", "standby_temperature_delta", "preheat_time", "preheat_steps"})},
            {QStringLiteral("Flush options"), processKeys({"flush_into_infill", "flush_into_objects", "flush_into_support"})},
            {QStringLiteral("Advanced"), processKeys({"interlocking_beam", "mmu_segmented_region_max_width", "mmu_segmented_region_interlocking_depth", "interlocking_beam_width", "interlocking_orientation", "interlocking_beam_layer_count", "interlocking_depth", "interlocking_boundary_avoidance"})},
        }},
        {QStringLiteral("Others"), {
            {QStringLiteral("Skirt"), processKeys({"skirt_loops", "skirt_type", "min_skirt_length", "skirt_distance", "skirt_start_angle", "skirt_height", "skirt_speed", "draft_shield"})},
            {QStringLiteral("Brim"), processKeys({"brim_type", "brim_width", "brim_object_gap", "brim_ears_max_angle", "brim_ears_detection_length"})},
            {QStringLiteral("Special mode"), processKeys({"slicing_mode", "print_sequence", "print_order", "spiral_mode", "spiral_mode_smooth", "spiral_mode_max_xy_smoothing", "timelapse_type", "fuzzy_skin", "fuzzy_skin_point_distance", "fuzzy_skin_thickness", "fuzzy_skin_first_layer"})},
            {QStringLiteral("G-code output"), processKeys({"reduce_infill_retraction", "gcode_add_line_number", "gcode_comments", "gcode_label_objects", "exclude_object", "filename_format"})},
            {QStringLiteral("Post-processing Scripts"), processKeys({"post_process"})},
            {QStringLiteral("Notes"), processKeys({"notes"})},
        }},
    };
    return definitions;
  }

  static const ProcessGroupDefinition *processGroupDefinition(const QString &page, const QString &group)
  {
    for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
      if (pageDefinition.name != page)
        continue;
      for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
        if (groupDefinition.name == group)
          return &groupDefinition;
      }
      return nullptr;
    }
    return nullptr;
  }

  static const QHash<QString, QPair<QString, QString>> &processPageGroupLookup()
  {
    static const QHash<QString, QPair<QString, QString>> lookup = [] {
      QHash<QString, QPair<QString, QString>> result;
      for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
        for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
          for (const QString &key : groupDefinition.optionKeys) {
            Q_ASSERT(!result.contains(key));
            result.insert(key, qMakePair(pageDefinition.name, groupDefinition.name));
          }
        }
      }
      return result;
    }();
    return lookup;
  }

  static QStringList processManifestKeys()
  {
    QStringList result;
    const auto &lookup = processPageGroupLookup();
    result.reserve(lookup.size());
    for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
      for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups) {
        for (const QString &key : groupDefinition.optionKeys)
          result.append(key);
      }
    }
    return result;
  }

#ifdef HAS_LIBSLIC3R
  // Preset::print_options() includes a small set of source-backed preset fields
  // that TabPrint intentionally does not render as editable rows.
  static const QSet<QString> &processNonUiPresetKeys()
  {
    static const QSet<QString> keys = {
        QStringLiteral("travel_speed_z"),
        QStringLiteral("enforce_support_layers"),
        QStringLiteral("raft_expansion"),
        QStringLiteral("support_interface_loop_pattern"),
        QStringLiteral("interface_shells"),
        QStringLiteral("compatible_printers"),
        QStringLiteral("compatible_printers_condition"),
        QStringLiteral("inherits"),
        QStringLiteral("overhang_speed_classic"),
        QStringLiteral("wiping_volumes_extruders"),
    };
    return keys;
  }

  static QStringList auditProcessManifest()
  {
    const auto &lookup = processPageGroupLookup();
    const auto &nonUiKeys = processNonUiPresetKeys();
    QSet<QString> upstreamKeys;
    QStringList errors;

    for (const std::string &rawKey : Slic3r::Preset::print_options()) {
      const QString key = QString::fromStdString(rawKey);
      upstreamKeys.insert(key);
      if (!lookup.contains(key) && !nonUiKeys.contains(key))
        errors.append(QStringLiteral("Unmapped Process preset key: %1").arg(key));
    }

    for (const QString &key : processManifestKeys()) {
      if (!upstreamKeys.contains(key))
        errors.append(QStringLiteral("Process manifest key is not a print preset option: %1").arg(key));
      if (!Slic3r::print_config_def.has(key.toStdString()))
        errors.append(QStringLiteral("Process manifest key is missing from print_config_def: %1").arg(key));
    }

    for (const QString &key : nonUiKeys) {
      if (!upstreamKeys.contains(key))
        errors.append(QStringLiteral("Stale non-UI Process preset key: %1").arg(key));
    }

    errors.sort(Qt::CaseSensitive);
    return errors;
  }

static const char *kMachineKeys[] = {
  // Page: 基础信息 — 打印空间
  "printable_area", "bed_exclude_area", "printable_height",
  "support_multi_bed_types", "nozzle_volume", "best_object_pos",
  "z_offset", "preferred_orientation",
  // Page: 基础信息 — 高级
  "printer_structure", "gcode_flavor", "use_relative_e_distances",
  "use_firmware_retraction", "machine_load_filament_time",
  "machine_unload_filament_time", "nozzle_type", "nozzle_hrc",
  "disable_m73", "thumbnails", "time_cost",
  "auxiliary_fan", "support_chamber_temp_control",
  "support_air_filtration",
  // Page: 打印机G-code
  "machine_start_gcode", "machine_end_gcode",
  "before_layer_change_gcode", "layer_change_gcode",
  "time_lapse_gcode", "change_filament_gcode",
  "change_extrusion_role_gcode", "machine_pause_gcode",
  "template_custom_gcode", "printing_by_object_gcode",
  // Page: 运动能力
  "emit_machine_limits_to_gcode",
  "machine_max_speed_x", "machine_max_speed_y",
  "machine_max_speed_z", "machine_max_speed_e",
  "machine_max_acceleration_x", "machine_max_acceleration_y",
  "machine_max_acceleration_z", "machine_max_acceleration_e",
  "machine_max_acceleration_extruding",
  "machine_max_acceleration_retracting",
  "machine_max_acceleration_travel",
  "machine_max_jerk_x", "machine_max_jerk_y",
  "machine_max_jerk_z", "machine_max_jerk_e",
  // Page: 挤出机
  "nozzle_diameter", "min_layer_height", "max_layer_height",
  "extruder_offset", "retraction_length", "retract_restart_extra",
  "z_hop", "z_hop_types", "retraction_speed", "deretraction_speed",
  "retraction_minimum_travel", "retract_when_changing_layer",
  "wipe", "wipe_distance", "retract_before_wipe",
  "retract_length_toolchange", "retract_restart_extra_toolchange",
  "long_retractions_when_cut",
  // Page: 多材料
  "single_extruder_multi_material", "extruders_count",
  "manual_filament_change",
  "cooling_tube_retraction", "cooling_tube_length",
  "parking_pos_retraction", "extra_loading_move",
  "enable_filament_ramming",
  // Page: 注释
  "printer_notes",
  nullptr
};

// Filament config keys (对齐上游 TabFilament)
static const char *kFilamentKeys[] = {
  // Basic
  "filament_type", "filament_vendor", "filament_colour",
  "filament_density", "filament_cost", "filament_spool_weight",
  // Temperature
  "nozzle_temperature", "nozzle_temperature_initial_layer",
  "hot_plate_temp", "hot_plate_temp_initial_layer",
  "chamber_temperature", "cool_plate_temp",
  "nozzle_temperature_range",
  // Cooling
  "fan_cooling_layer_time", "slow_down_min_speed",
  "fan_min_speed", "fan_max_speed", "default_fan_speed",
  "overhang_fan_speed", "close_fan_the_first_x_layers",
  // Speed / Quality
  "filament_max_volumetric_speed", "filament_flow_ratio",
  "filament_min_speed", "filament_retract_lift_above",
  "filament_retract_lift_below",
  // Retraction
  "filament_retraction_length", "filament_retraction_speed",
  "filament_deretraction_speed", "filament_z_hop",
  "filament_retract_restart_extra",
  "filament_retract_length_toolchange",
  "filament_retract_restart_extra_toolchange",
  // G-code
  "filament_start_gcode", "filament_end_gcode",
  "filament_change_gcode",
  // Notes
  "filament_notes",
  nullptr
};

#endif // HAS_LIBSLIC3R

} // anonymous namespace

// Assign page/group from the tier-specific mapping table.
// Called AFTER loadSchemaFromKeys populates m_options but BEFORE endResetModel().
static void assignPageGroupForTier(ConfigOptionModel *model, const QHash<QString, QPair<QString, QString>> &pageGroupMap)
{
  for (int i = 0; i < model->rowCount(); ++i)
  {
    const QString key = model->optKey(i);
    const auto it = pageGroupMap.constFind(key);
    if (it != pageGroupMap.cend())
    {
      QModelIndex idx = model->index(i, 0);
      model->setData(idx, it.value().first, ConfigOptionModel::PageRole);
      model->setData(idx, it.value().second, ConfigOptionModel::GroupRole);
    }
  }
}

QStringList ConfigOptionModel::processPageNames() const
{
  QStringList result;
  result.reserve(processPageDefinitions().size());
  for (const ProcessPageDefinition &definition : processPageDefinitions())
    result.append(definition.name);
  return result;
}

QStringList ConfigOptionModel::processGroupsForPage(const QString &page) const
{
  QStringList result;
  for (const ProcessPageDefinition &pageDefinition : processPageDefinitions()) {
    if (pageDefinition.name != page)
      continue;
    result.reserve(pageDefinition.groups.size());
    for (const ProcessGroupDefinition &groupDefinition : pageDefinition.groups)
      result.append(groupDefinition.name);
    return result;
  }
  return result;
}

QList<int> ConfigOptionModel::orderedProcessIndicesForGroup(const QList<int> &candidateIndices,
                                                            const QString &page,
                                                            const QString &group) const
{
  const ProcessGroupDefinition *definition = processGroupDefinition(page, group);
  if (!definition)
    return {};

  QHash<QString, int> candidateByKey;
  candidateByKey.reserve(candidateIndices.size());
  for (int index : candidateIndices) {
    if (index < 0 || index >= m_options.size())
      continue;
    const ConfigOption &option = m_options[index];
    if (option.page == page && option.group == group)
      candidateByKey.insert(option.key, index);
  }

  QList<int> result;
  result.reserve(definition->optionKeys.size());
  for (const QString &key : definition->optionKeys) {
    const auto candidate = candidateByKey.constFind(key);
    if (candidate != candidateByKey.cend())
      result.append(candidate.value());
  }
  return result;
}

QStringList ConfigOptionModel::processMappingErrors() const
{
#ifdef HAS_LIBSLIC3R
  return auditProcessManifest();
#else
  return {};
#endif
}

void ConfigOptionModel::assignProcessHierarchyMetadata()
{
  const auto &lookup = processPageGroupLookup();
  for (ConfigOption &option : m_options) {
    const auto location = lookup.constFind(option.key);
    if (location == lookup.cend())
      continue;
    option.page = location.value().first;
    option.group = location.value().second;
  }
}

#ifdef HAS_LIBSLIC3R

void ConfigOptionModel::loadSchemaFromKeys(const char *const keys[])
{
  beginResetModel();
  m_options.clear();
  m_baseReadonlyKeys.clear();
  m_defaultValues.clear();
  m_referenceValues.clear();
  m_dirtyKeys.clear();

  const auto &def = Slic3r::print_config_def;

  for (int ki = 0; keys[ki] != nullptr; ++ki)
  {
    const auto *opt = def.get(keys[ki]);
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

    // Tooltip from upstream (对齐上游 ConfigOptionDef::tooltip)
    if (!opt->tooltip.empty())
      entry.tooltip = QString::fromUtf8(opt->tooltip.c_str());

    // Sidetext (unit label) from upstream (对齐上游 ConfigOptionDef::sidetext)
    if (!opt->sidetext.empty())
      entry.sidetext = QString::fromUtf8(opt->sidetext.c_str());

    // Mode from upstream (对齐上游 ConfigOptionMode: 0=Simple, 1=Advanced, 2=Develop)
    entry.mode = static_cast<int>(opt->mode);

    // Nullable: accepts nil = inherit from parent preset (upstream ConfigOptionDef::nullable)
    entry.nullable = opt->nullable;

    // IsVector: multi-value per-extruder (upstream coVectorType = 0x4000 bit)
    entry.isVector = ((opt->type & Slic3r::coVectorType) != 0);

    m_options.append(entry);
    m_defaultValues.insert(entry.key, entry.value);

    if (entry.readonly)
      m_baseReadonlyKeys.insert(entry.key);
  }

  endResetModel();
  emit countChanged();
}

void ConfigOptionModel::loadFromUpstreamSchema()
{
  const QStringList mappingErrors = processMappingErrors();
  if (!mappingErrors.isEmpty()) {
    qCritical().noquote() << "Process settings hierarchy audit failed:" << mappingErrors.join(QStringLiteral("; "));
    static const char *const kNoKeys[] = {nullptr};
    loadSchemaFromKeys(kNoKeys);
    return;
  }

  const QStringList manifestKeys = processManifestKeys();
  QVector<QByteArray> encodedKeys;
  encodedKeys.reserve(manifestKeys.size());
  QVector<const char *> keyPointers;
  keyPointers.reserve(manifestKeys.size() + 1);
  for (const QString &key : manifestKeys) {
    encodedKeys.append(key.toUtf8());
    keyPointers.append(encodedKeys.constLast().constData());
  }
  keyPointers.append(nullptr);
  loadSchemaFromKeys(keyPointers.constData());
  assignProcessHierarchyMetadata();
}

void ConfigOptionModel::loadMachineSchema()
{
  loadSchemaFromKeys(kMachineKeys);
  // Upstream: Tab.cpp:4438-4900 (TabPrinter::build_fff) - assign page/group metadata
  assignPageGroupForTier(this, kPrinterPageGroupMap());
}

void ConfigOptionModel::loadFilamentSchema()
{
  loadSchemaFromKeys(kFilamentKeys);
  // Upstream: Tab.cpp:3892-4216 (TabFilament::build) - assign page/group metadata
  assignPageGroupForTier(this, kFilamentPageGroupMap());
}

#endif // HAS_LIBSLIC3R
