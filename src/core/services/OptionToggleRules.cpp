#include "core/services/OptionToggleRules.h"

#include <QStringList>

#include <cmath>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#endif

namespace OptionToggleRules
{
#ifdef HAS_LIBSLIC3R
namespace
{
using namespace Slic3r;

// Typed view over one value lookup, mirroring the DynamicPrintConfig
// accessors the upstream rules use (opt_bool / opt_int / opt_float /
// opt_enum / opt_string). Values arrive in the mixed shapes the Qt side
// stores: preset-JSON strings ("1", "15%", "rectilinear"), QML writes (bool,
// enum index), and vectors as lists (variant / extruder 0 is read, like the
// single-extruder upstream path). Missing keys fall back to the
// ConfigOptionDef default.
class Cfg
{
public:
  explicit Cfg(Lookup lookup) : m_lookup(std::move(lookup)) {}

  QVariant raw(const char *key) const
  {
    QVariant v = m_lookup ? m_lookup(QString::fromLatin1(key)) : QVariant();
    if (v.typeId() == QMetaType::QVariantList || v.typeId() == QMetaType::QStringList)
    {
      const QVariantList list = v.toList();
      v = list.isEmpty() ? QVariant() : list.first();
    }
    if (!v.isValid() || (v.typeId() == QMetaType::QString && v.toString().isEmpty() && !isString(key)))
      v = defaultValue(key);
    return v;
  }

  bool b(const char *key) const
  {
    const QVariant v = raw(key);
    if (v.typeId() == QMetaType::Bool)
      return v.toBool();
    if (v.typeId() == QMetaType::QString)
    {
      const QString s = v.toString().trimmed().toLower();
      return s == QLatin1String("1") || s == QLatin1String("true");
    }
    return v.toDouble() != 0.0;
  }

  double f(const char *key) const
  {
    const QVariant v = raw(key);
    if (v.typeId() == QMetaType::Bool)
      return v.toBool() ? 1.0 : 0.0;
    if (v.typeId() == QMetaType::QString)
    {
      QString s = v.toString().trimmed();
      if (s.endsWith(QLatin1Char('%')))
        s.chop(1);
      bool ok = false;
      const double d = s.toDouble(&ok);
      return ok ? d : 0.0; // "nil" and other non-numbers read as 0
    }
    return v.toDouble();
  }

  int i(const char *key) const { return static_cast<int>(std::lround(f(key))); }

  QString s(const char *key) const { return raw(key).toString(); }

  // Enum value (the C++ enum's int) through the option's keys map.
  int e(const char *key) const
  {
    const ConfigOptionDef *def = print_config_def.get(key);
    if (!def || !def->enum_keys_map)
      return 0;
    const QVariant v = raw(key);
    const auto byName = [&](const std::string &name, int &out) {
      const auto it = def->enum_keys_map->find(name);
      if (it == def->enum_keys_map->end())
        return false;
      out = it->second;
      return true;
    };
    int out = 0;
    if (v.typeId() == QMetaType::QString)
    {
      const std::string name = v.toString().toStdString();
      if (byName(name, out))
        return out;
      for (size_t idx = 0; idx < def->enum_labels.size() && idx < def->enum_values.size(); ++idx)
        if (def->enum_labels[idx] == name && byName(def->enum_values[idx], out))
          return out;
      bool isNumber = false;
      const int index = v.toString().toInt(&isNumber);
      if (isNumber && index >= 0 && index < static_cast<int>(def->enum_values.size()) &&
          byName(def->enum_values[index], out))
        return out;
    }
    else if (v.isValid())
    {
      // QML combo writes the row index into enum_values (ConfigOptionModel
      // builds enumLabels in enum_values order).
      const int index = v.toInt();
      if (index >= 0 && index < static_cast<int>(def->enum_values.size()) &&
          byName(def->enum_values[index], out))
        return out;
    }
    return defaultEnum(def);
  }

  template <class T> T en(const char *key) const { return static_cast<T>(e(key)); }

private:
  // Vector enums (e.g. z_hop_types, coEnums) keep their default in a
  // ConfigOptionEnumsGeneric, where getInt() throws.
  static int defaultEnum(const ConfigOptionDef *def)
  {
    if (!def->default_value)
      return 0;
    if (def->type == coEnums)
    {
      const auto *v = dynamic_cast<const ConfigOptionInts *>(def->default_value.get());
      return v && !v->values.empty() ? v->values.front() : 0;
    }
    try
    {
      return def->default_value->getInt();
    }
    catch (const std::exception &)
    {
      return 0;
    }
  }

  static bool isString(const char *key)
  {
    const ConfigOptionDef *def = print_config_def.get(key);
    return def && (def->type == coString || def->type == coStrings);
  }

  static QVariant defaultValue(const char *key)
  {
    const ConfigOptionDef *def = print_config_def.get(key);
    if (!def || !def->default_value)
      return {};
    // Enums: no value -> Cfg::e falls back to default_value->getInt().
    if (def->type == coEnum || def->type == coEnums)
      return {};
    QString text = QString::fromStdString(def->default_value->serialize());
    if (def->type & coVectorType)
      text = text.section(QLatin1Char(','), 0, 0);
    if (def->type == coString || def->type == coStrings)
    {
      if (text.size() >= 2 && text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')))
        text = text.mid(1, text.size() - 2);
    }
    return text;
  }

  Lookup m_lookup;
};

class Toggler
{
public:
  explicit Toggler(Result &result) : m_result(result) {}

  // Later calls on the same key override earlier ones, like the upstream
  // Field::toggle / Line::toggle_visible assignments.
  void field(const char *key, bool on)
  {
    const QString k = QString::fromLatin1(key);
    if (on)
      m_result.disabled.remove(k);
    else
      m_result.disabled.insert(k);
  }

  void line(const char *key, bool on)
  {
    const QString k = QString::fromLatin1(key);
    if (on)
      m_result.hidden.remove(k);
    else
      m_result.hidden.insert(k);
  }

private:
  Result &m_result;
};
} // namespace

Result printToggles(const Context &ctx)
{
  Result result;
  Toggler t(result);
  const Cfg config(ctx.print);
  const Cfg printer(ctx.printer);
  const bool is_BBL_Printer = ctx.isBblPrinter;

  const GCodeFlavor gcflavor = printer.en<GCodeFlavor>("gcode_flavor");
  const bool gcf_is_marlin_firmware = gcflavor == GCodeFlavor::gcfMarlinFirmware;
  const bool gcf_is_klipper = gcflavor == GCodeFlavor::gcfKlipper;

  const bool have_volumetric_extrusion_rate_slope = config.f("max_volumetric_extrusion_rate_slope") > 0;
  t.field("enable_arc_fitting", !have_volumetric_extrusion_rate_slope);
  t.line("max_volumetric_extrusion_rate_slope_segment_length", have_volumetric_extrusion_rate_slope);
  t.line("extrusion_rate_smoothing_external_perimeter_only", have_volumetric_extrusion_rate_slope);

  const bool have_perimeters = config.i("wall_loops") > 0;
  for (auto el : {"extra_perimeters_on_overhangs", "ensure_vertical_shell_thickness", "detect_thin_wall", "detect_overhang_wall",
                  "seam_position", "staggered_inner_seams", "wall_sequence", "outer_wall_line_width"})
    t.field(el, have_perimeters);
  for (auto el : {"inner_wall_speed", "outer_wall_speed", "small_perimeter_speed", "small_perimeter_threshold"})
    t.field(el, have_perimeters);

  const bool have_infill = config.f("sparse_infill_density") > 0;
  for (auto el : {"sparse_infill_pattern", "infill_combination", "fill_multiline", "infill_direction",
                  "minimum_sparse_infill_area", "sparse_infill_filament_id", "infill_shift_step",
                  "sparse_infill_rotate_template", "symmetric_infill_y_axis"})
    t.line(el, have_infill);

  const InfillPattern pattern = config.en<InfillPattern>("sparse_infill_pattern");

  const bool have_infill_anchor = have_infill && pattern != ipConcentric && pattern != ipSpiralInset;
  t.line("infill_anchor", have_infill_anchor);
  t.line("infill_anchor_max", have_infill_anchor);

  const bool have_combined_infill = config.b("infill_combination") && have_infill;
  t.line("infill_combination_max_layer_height", have_combined_infill);

  const bool have_multiline_infill_pattern =
      pattern == ipGyroid || pattern == ipGrid || pattern == ipRectilinear || pattern == ipTpmsD || pattern == ipTpmsFK ||
      pattern == ipCrossHatch || pattern == ipHoneycomb || pattern == ipLateralLattice || pattern == ipLateralHoneycomb ||
      pattern == ipConcentric || pattern == ipCubic || pattern == ipStars || pattern == ipAlignedRectilinear ||
      pattern == ipLightning || pattern == ip3DHoneycomb || pattern == ipAdaptiveCubic || pattern == ipSupportCubic ||
      pattern == ipTriangles || pattern == ipQuarterCubic || pattern == ipArchimedeanChords ||
      pattern == ipHilbertCurve || pattern == ipOctagramSpiral;

  t.line("gyroid_optimized", have_infill && pattern == ipGyroid);

  if (have_infill)
  {
    t.field("fill_multiline", have_multiline_infill_pattern);
    const bool infill_anchor = pattern != ipLine;
    t.field("infill_anchor_max", infill_anchor);
    const bool has_infill_anchors = infill_anchor && config.f("infill_anchor_max") > 0;
    t.field("infill_anchor", has_infill_anchors);
  }

  const bool is_cross_zag = pattern == ipCrossZag;
  const bool is_locked_zig = pattern == ipLockedZag;
  t.line("infill_shift_step", is_cross_zag || is_locked_zig);
  for (auto el : {"skeleton_infill_density", "skin_infill_density", "infill_lock_depth", "skin_infill_depth",
                  "skin_infill_line_width", "skeleton_infill_line_width"})
    t.line(el, is_locked_zig);

  const bool is_zig_zag = pattern == ipZigZag;
  t.line("symmetric_infill_y_axis", is_zig_zag || is_cross_zag || is_locked_zig);

  const bool has_spiral_vase = config.b("spiral_mode");
  t.line("spiral_mode_smooth", has_spiral_vase);
  t.line("spiral_mode_max_xy_smoothing", has_spiral_vase && config.b("spiral_mode_smooth"));
  t.line("spiral_starting_flow_ratio", has_spiral_vase);
  t.line("spiral_finishing_flow_ratio", has_spiral_vase);
  const bool has_top_shell_layers =
      config.i("top_shell_layers") > 0 || (has_spiral_vase && config.i("bottom_shell_layers") > 1);
  const bool has_top_shell = has_top_shell_layers && config.f("top_surface_density") > 0;
  const bool has_bottom_shell = config.i("bottom_shell_layers") > 0;
  const bool has_solid_infill = has_top_shell_layers || has_bottom_shell;
  t.line("sparse_infill_smooth_factor", is_smoothable_infill_pattern(pattern, config.i("fill_multiline")));
  t.field("top_surface_pattern", has_top_shell);
  t.field("bottom_surface_pattern", has_bottom_shell);
  t.field("top_surface_density", has_top_shell_layers);
  t.field("bottom_surface_density", has_bottom_shell);
  t.field("top_layer_direction", has_top_shell);
  t.field("bottom_layer_direction", has_bottom_shell);

  t.line("top_surface_expansion", has_top_shell);
  t.line("top_surface_expansion_margin", has_top_shell);
  const bool has_top_surface_expansion = config.f("top_surface_expansion") > 0;
  t.field("top_surface_expansion_margin", has_top_surface_expansion);
  t.line("top_surface_expansion_direction", has_top_shell);
  t.field("top_surface_expansion_direction", has_top_surface_expansion);

  const auto is_centered_pattern = [](InfillPattern p) { return p == ipArchimedeanChords || p == ipOctagramSpiral; };
  const InfillPattern top_pattern = config.en<InfillPattern>("top_surface_pattern");
  const InfillPattern bottom_pattern = config.en<InfillPattern>("bottom_surface_pattern");
  const bool has_centered_surface =
      (has_top_shell && is_centered_pattern(top_pattern)) || (has_bottom_shell && is_centered_pattern(bottom_pattern));
  t.line("center_of_surface_pattern", has_centered_surface);

  const bool is_internal_infill_separable = is_separable_infill_pattern(pattern) ||
                                            !config.s("sparse_infill_rotate_template").isEmpty() ||
                                            !config.s("solid_infill_rotate_template").isEmpty();
  t.line("separated_infills", is_internal_infill_separable);

  const auto is_centered_fill = [](InfillPattern p) {
    return p == ipConcentric || p == ipSpiralInset || p == ipArchimedeanChords || p == ipOctagramSpiral;
  };
  t.line("top_surface_fill_order", has_top_shell && is_centered_fill(top_pattern));
  t.line("bottom_surface_fill_order", has_bottom_shell && is_centered_fill(bottom_pattern));

  for (auto el : {"infill_direction", "sparse_infill_line_width", "gap_fill_target", "filter_out_gap_fill", "infill_wall_overlap",
                  "bridge_angle", "internal_bridge_angle", "relative_bridge_angle", "solid_infill_direction",
                  "solid_infill_rotate_template", "internal_solid_infill_pattern", "internal_solid_filament_id",
                  "top_surface_filament_id", "bottom_surface_filament_id"})
    t.field(el, have_infill || has_solid_infill);
  for (auto el : {"sparse_infill_speed", "bridge_speed", "internal_bridge_speed"})
    t.field(el, have_infill || has_solid_infill);

  t.field("top_shell_thickness", !has_spiral_vase && has_top_shell_layers);
  t.field("bottom_shell_thickness", !has_spiral_vase && has_bottom_shell);

  t.field("gap_infill_speed", have_perimeters);

  t.field("top_surface_line_width", has_top_shell);
  t.field("top_surface_speed", has_top_shell);

  const bool have_default_acceleration = config.f("default_acceleration") > 0;
  for (auto el : {"outer_wall_acceleration", "inner_wall_acceleration", "initial_layer_acceleration",
                  "initial_layer_travel_acceleration", "top_surface_acceleration", "travel_acceleration",
                  "bridge_acceleration", "sparse_infill_acceleration", "internal_solid_infill_acceleration"})
    t.field(el, have_default_acceleration);

  const bool machine_supports_junction_deviation =
      gcf_is_marlin_firmware && printer.f("machine_max_junction_deviation") > 0.0;
  t.line("default_junction_deviation", gcf_is_marlin_firmware);
  if (machine_supports_junction_deviation)
  {
    t.field("default_junction_deviation", true);
    t.field("default_jerk", false);
    for (auto el : {"outer_wall_jerk", "inner_wall_jerk", "initial_layer_jerk", "initial_layer_travel_jerk",
                    "top_surface_jerk", "travel_jerk", "infill_jerk"})
      t.line(el, false);
  }
  else
  {
    t.field("default_junction_deviation", false);
    t.field("default_jerk", true);
    const bool have_default_jerk = config.f("default_jerk") > 0;
    for (auto el : {"outer_wall_jerk", "inner_wall_jerk", "initial_layer_jerk", "initial_layer_travel_jerk",
                    "top_surface_jerk", "travel_jerk", "infill_jerk"})
    {
      t.line(el, true);
      t.field(el, have_default_jerk);
    }
  }

  const bool have_skirt = config.i("skirt_loops") > 0;
  t.field("skirt_height", have_skirt && config.en<DraftShield>("draft_shield") != dsEnabled);
  t.line("single_loop_draft_shield", have_skirt);
  for (auto el : {"skirt_type", "min_skirt_length", "skirt_distance", "skirt_start_angle", "skirt_speed", "draft_shield"})
    t.field(el, have_skirt);

  const BrimType brim_type = config.en<BrimType>("brim_type");
  const bool have_brim = brim_type != btNoBrim;
  t.field("brim_object_gap", have_brim);
  t.field("brim_use_efc_outline", have_brim);
  t.field("combine_brims", have_brim);
  const bool have_brim_width = brim_type != btNoBrim && brim_type != btAutoBrim && brim_type != btPainted;
  t.field("brim_width", have_brim_width);
  t.field("brim_flow_ratio", have_brim);
  t.field("outer_wall_filament_id", have_perimeters || have_brim);
  t.field("inner_wall_filament_id", have_perimeters || have_brim);

  const bool have_auto_brim_ear = brim_type == btEar;
  const bool have_painted_brim_ear = brim_type == btPainted;
  const double brim_width = config.f("brim_width");
  t.field("brim_ears_max_angle", brim_width > 0.0);
  t.field("brim_ears_detection_length", brim_width > 0.0);
  t.field("brim_ears_outer_only", have_painted_brim_ear || brim_width > 0.0);
  t.line("brim_ears_max_angle", have_auto_brim_ear);
  t.line("brim_ears_detection_length", have_auto_brim_ear);
  t.line("brim_ears_outer_only", have_auto_brim_ear || have_painted_brim_ear);

  t.line("elefant_foot_compensation_layers",
         config.f("elefant_foot_compensation") > 0 || config.f("elefant_foot_layers_density") / 100.0 < 1.0);

  const bool have_raft = config.i("raft_layers") > 0;
  const bool have_support_material = config.b("enable_support") || have_raft;

  const SupportType support_type = config.en<SupportType>("support_type");
  const bool have_support_interface =
      config.i("support_interface_top_layers") > 0 || config.i("support_interface_bottom_layers") > 0;
  const bool have_support_soluble = have_support_material && config.f("support_top_z_distance") == 0;
  const SupportMaterialStyle support_style = config.en<SupportMaterialStyle>("support_style");
  for (auto el : {"support_style", "support_base_pattern", "support_base_pattern_spacing", "support_expansion", "support_angle",
                  "support_interface_pattern", "support_interface_top_layers", "support_interface_bottom_layers",
                  "bridge_no_support", "max_bridge_length", "support_top_z_distance", "support_bottom_z_distance",
                  "support_type", "support_on_build_plate_only", "support_critical_regions_only",
                  "support_interface_not_for_body", "support_object_xy_distance", "support_object_first_layer_gap",
                  "independent_support_layer_height"})
    t.field(el, have_support_material);
  t.field("support_threshold_angle", have_support_material && is_auto(support_type));
  t.field("support_threshold_overlap",
          config.i("support_threshold_angle") == 0 && have_support_material && is_auto(support_type));

  const bool support_is_tree = config.b("enable_support") && is_tree(support_type);
  const bool support_is_organic = support_is_tree && (support_style == smsTreeOrganic || support_style == smsDefault);
  const bool support_is_normal_tree = support_is_tree && !support_is_organic;

  t.line("support_threshold_overlap", !support_is_tree);
  for (auto el : {"tree_support_branch_angle", "tree_support_branch_distance", "tree_support_branch_diameter",
                  "tree_support_auto_brim", "tree_support_brim_width"})
    t.line(el, support_is_normal_tree);
  for (auto el : {"tree_support_branch_angle_organic", "tree_support_branch_distance_organic",
                  "tree_support_branch_diameter_organic", "tree_support_angle_slow", "tree_support_tip_diameter",
                  "tree_support_top_rate", "tree_support_branch_diameter_angle"})
    t.line(el, support_is_organic);
  t.line("independent_support_layer_height", have_support_material && !support_is_organic);

  t.field("tree_support_brim_width", support_is_tree && !config.b("tree_support_auto_brim"));
  t.line("max_bridge_length", support_is_tree);
  t.line("bridge_no_support", !support_is_tree);
  t.line("support_critical_regions_only", is_auto(support_type) && support_is_tree);

  for (auto el : {"support_interface_filament", "support_interface_loop_pattern", "support_bottom_interface_spacing"})
    t.field(el, have_support_material && have_support_interface);

  const bool can_ironing_support = have_raft || (have_support_material && config.i("support_interface_top_layers") > 0);
  t.field("support_ironing", can_ironing_support);
  const bool has_support_ironing = can_ironing_support && config.b("support_ironing");
  for (auto el : {"support_ironing_pattern", "support_ironing_flow", "support_ironing_spacing"})
    t.line(el, has_support_ironing);
  t.field("support_interface_spacing", have_support_material && have_support_interface && !has_support_ironing);

  for (auto el : {"small_support_perimeter_speed", "small_support_perimeter_threshold"})
    t.field(el, config.b("enable_support"));

  t.field("inner_wall_line_width", have_perimeters || have_skirt || have_brim);
  t.field("support_filament", have_support_material || have_skirt);

  t.line("raft_contact_distance", have_raft && !have_support_soluble);

  t.field("raft_first_layer_density", have_support_material);
  t.field("raft_first_layer_expansion",
          have_support_material && ((!support_is_normal_tree || support_style == smsTreeHybrid) || have_raft));

  const bool has_ironing = config.en<IroningType>("ironing_type") != IroningType::NoIroning;
  for (auto el : {"ironing_pattern", "ironing_flow", "ironing_spacing", "ironing_angle", "ironing_inset", "ironing_angle_fixed"})
    t.line(el, has_ironing);
  const bool has_rectilinear_ironing = config.en<InfillPattern>("ironing_pattern") == ipRectilinear;
  for (auto el : {"ironing_angle", "ironing_angle_fixed"})
    t.field(el, has_ironing && has_rectilinear_ironing);

  t.line("ironing_speed", has_ironing || has_support_ironing);

  const bool has_zaa = config.b("zaa_enabled");
  for (auto el : {"zaa_minimize_perimeter_height", "zaa_min_z", "zaa_dont_alternate_fill_direction", "ironing_expansion"})
    t.line(el, has_zaa);

  const bool have_sequential_printing = config.en<PrintSequence>("print_sequence") == PrintSequence::ByObject;
  t.field("print_order", !have_sequential_printing);

  t.field("single_extruder_multi_material", !is_BBL_Printer);

  const bool bSEMM = printer.b("single_extruder_multi_material");
  const bool supports_wipe_tower_2 = !is_BBL_Printer && printer.en<WipeTowerType>("wipe_tower_type") == WipeTowerType::Type2;

  t.field("ooze_prevention", !bSEMM);
  const bool have_ooze_prevention = config.b("ooze_prevention");
  t.line("standby_temperature_delta", have_ooze_prevention);
  t.line("preheat_time", have_ooze_prevention);
  t.line("preheat_steps", have_ooze_prevention && config.i("preheat_steps") > 0);

  const bool have_prime_tower = config.b("enable_prime_tower");
  for (auto el : {"prime_tower_width", "prime_tower_brim_width", "prime_tower_skip_points", "wipe_tower_wall_type",
                  "prime_tower_infill_gap", "prime_tower_enable_framework", "enable_tower_interface_features"})
    t.line(el, have_prime_tower);

  t.line("enable_tower_interface_cooldown_during_tower", have_prime_tower && config.b("enable_tower_interface_features"));

  const bool purge_in_primetower = printer.b("purge_in_prime_tower");

  for (auto el : {"wipe_tower_rotation_angle", "wipe_tower_cone_angle", "wipe_tower_extra_spacing", "wipe_tower_max_purge_speed",
                  "wipe_tower_bridging", "wipe_tower_extra_flow", "wipe_tower_no_sparse_layers"})
    t.line(el, have_prime_tower && supports_wipe_tower_2);

  const WipeTowerWallType wipe_tower_wall_type = config.en<WipeTowerWallType>("wipe_tower_wall_type");
  const bool have_rib_wall = wipe_tower_wall_type == WipeTowerWallType::wtwRib && have_prime_tower;
  t.line("wipe_tower_cone_angle",
         have_prime_tower && supports_wipe_tower_2 && wipe_tower_wall_type == WipeTowerWallType::wtwCone);
  t.line("wipe_tower_extra_rib_length", have_rib_wall);
  t.line("wipe_tower_rib_width", have_rib_wall);
  t.line("wipe_tower_fillet_wall", have_rib_wall);
  t.field("prime_tower_width", have_prime_tower && !have_rib_wall);

  t.line("single_extruder_multi_material_priming", !bSEMM && have_prime_tower && supports_wipe_tower_2);

  t.line("prime_volume", have_prime_tower && (!purge_in_primetower || !bSEMM));

  for (auto el : {"flush_into_infill", "flush_into_support", "flush_into_objects"})
    t.field(el, have_prime_tower);

  t.line("max_travel_detour_distance", config.b("reduce_crossing_wall"));

  const bool has_set_other_flow_ratios = config.b("set_other_flow_ratios");
  for (auto el : {"first_layer_flow_ratio", "outer_wall_flow_ratio", "inner_wall_flow_ratio", "overhang_flow_ratio",
                  "sparse_infill_flow_ratio", "internal_solid_infill_flow_ratio", "gap_fill_flow_ratio",
                  "support_flow_ratio", "support_interface_flow_ratio"})
    t.line(el, has_set_other_flow_ratios);

  const bool has_overhang_speed = config.b("enable_overhang_speed");
  for (auto el : {"overhang_1_4_speed", "overhang_2_4_speed", "overhang_3_4_speed", "overhang_4_4_speed"})
    t.line(el, has_overhang_speed);

  t.line("slowdown_for_curled_perimeters", has_overhang_speed);

  t.line("flush_into_objects", !ctx.isGlobalConfig);

  t.line("support_interface_not_for_body", config.i("support_interface_filament") && !config.i("support_filament"));

  const bool has_fuzzy_skin = config.en<FuzzySkinType>("fuzzy_skin") != FuzzySkinType::Disabled_fuzzy;
  for (auto el : {"fuzzy_skin_mode", "fuzzy_skin_noise_type", "fuzzy_skin_point_distance", "fuzzy_skin_thickness",
                  "fuzzy_skin_first_layer"})
    t.line(el, has_fuzzy_skin);

  const NoiseType fuzzy_skin_noise_type = config.en<NoiseType>("fuzzy_skin_noise_type");
  const bool is_ripple = fuzzy_skin_noise_type == NoiseType::Ripple;
  t.line("fuzzy_skin_scale", fuzzy_skin_noise_type != NoiseType::Classic && has_fuzzy_skin && !is_ripple);
  t.line("fuzzy_skin_octaves", fuzzy_skin_noise_type != NoiseType::Classic && fuzzy_skin_noise_type != NoiseType::Voronoi &&
                                   has_fuzzy_skin && !is_ripple);
  t.line("fuzzy_skin_persistence",
         (fuzzy_skin_noise_type == NoiseType::Perlin || fuzzy_skin_noise_type == NoiseType::Billow) && has_fuzzy_skin && !is_ripple);
  t.line("fuzzy_skin_ripples_per_layer", is_ripple && has_fuzzy_skin);
  t.line("fuzzy_skin_ripple_offset", is_ripple && has_fuzzy_skin);
  t.line("fuzzy_skin_layers_between_ripple_offset", is_ripple && has_fuzzy_skin);

  const bool have_arachne = config.en<PerimeterGeneratorType>("wall_generator") == PerimeterGeneratorType::Arachne;
  for (auto el : {"wall_transition_length", "wall_transition_filter_deviation", "wall_transition_angle", "min_feature_size",
                  "min_length_factor", "min_bead_width", "wall_distribution_count", "initial_layer_min_bead_width",
                  "wall_maximum_resolution", "wall_maximum_deviation"})
    t.line(el, have_arachne);
  t.field("detect_thin_wall", !have_arachne);

  t.field("wipe_speed", !config.b("role_based_wipe_speed"));

  for (auto el : {"accel_to_decel_enable", "accel_to_decel_factor"})
    t.line(el, gcf_is_klipper);
  if (gcf_is_klipper)
    t.field("accel_to_decel_factor", config.b("accel_to_decel_enable"));

  const bool have_make_overhang_printable = config.b("make_overhang_printable");
  t.line("make_overhang_printable_angle", have_make_overhang_printable);
  t.line("make_overhang_printable_hole_size", have_make_overhang_printable);

  t.line("only_one_wall_first_layer", has_bottom_shell);
  t.line("only_one_wall_top", has_top_shell_layers);
  t.line("min_width_top_surface", (has_top_shell_layers && config.b("only_one_wall_top")) ||
                                      (config.f("min_length_factor") > 0.5 && have_arachne));

  for (auto el : {"hole_to_polyhole_threshold", "hole_to_polyhole_twisted", "hole_to_polyhole_max_edges"})
    t.line(el, config.b("hole_to_polyhole"));

  const bool has_detect_overhang_wall = config.b("detect_overhang_wall");
  const bool has_overhang_reverse = config.b("overhang_reverse");
  const bool allow_overhang_reverse = !has_spiral_vase;
  t.line("overhang_reverse", allow_overhang_reverse);
  t.line("overhang_reverse_internal_only", allow_overhang_reverse && has_overhang_reverse);
  const bool has_overhang_reverse_internal_only = config.b("overhang_reverse_internal_only");
  t.line("overhang_reverse_threshold", has_detect_overhang_wall && allow_overhang_reverse && has_overhang_reverse &&
                                           !has_overhang_reverse_internal_only);
  t.line("timelapse_type", is_BBL_Printer);

  t.line("small_area_infill_flow_compensation_model", config.b("small_area_infill_flow_compensation"));

  t.field("seam_slope_type", !has_spiral_vase);
  const bool has_seam_slope = !has_spiral_vase && config.en<SeamScarfType>("seam_slope_type") != SeamScarfType::None;
  t.line("seam_slope_conditional", has_seam_slope);
  t.line("seam_slope_start_height", has_seam_slope);
  t.line("seam_slope_entire_loop", has_seam_slope);
  t.line("seam_slope_min_length", has_seam_slope);
  t.line("seam_slope_steps", has_seam_slope);
  t.line("seam_slope_inner_walls", has_seam_slope);
  t.line("scarf_joint_speed", has_seam_slope);
  t.line("scarf_joint_flow_ratio", has_seam_slope);
  t.field("seam_slope_min_length", !config.b("seam_slope_entire_loop"));
  t.line("scarf_angle_threshold", has_seam_slope && config.b("seam_slope_conditional"));
  t.line("scarf_overhang_threshold", has_seam_slope && config.b("seam_slope_conditional"));

  const bool use_beam_interlocking = config.b("interlocking_beam");
  t.line("mmu_segmented_region_interlocking_depth", !use_beam_interlocking);
  t.line("interlocking_beam_width", use_beam_interlocking);
  t.line("interlocking_orientation", use_beam_interlocking);
  t.line("interlocking_beam_layer_count", use_beam_interlocking);
  t.line("interlocking_depth", use_beam_interlocking);
  t.line("interlocking_boundary_avoidance", use_beam_interlocking);

  for (auto el : {"lateral_lattice_angle_1", "lateral_lattice_angle_2"})
    t.line(el, pattern == ipLateralLattice);

  for (auto el : {"lightning_overhang_angle", "lightning_prune_angle", "lightning_straightening_angle"})
    t.line(el, pattern == ipLightning);

  const bool fill_adaptive = pattern == ipAdaptiveCubic || pattern == ipSupportCubic;
  t.field("sparse_infill_rotate_template", !fill_adaptive);
  t.field("infill_direction", config.s("sparse_infill_rotate_template").isEmpty() && !fill_adaptive);
  t.field("solid_infill_direction", config.s("solid_infill_rotate_template").isEmpty());

  t.line("infill_overhang_angle", pattern == ipLateralHoneycomb);

  // DevPrinterConfigUtil::support_wrapping_detection reads BBL device
  // profiles; no non-BBL printer supports it. BBL device capability tables
  // are out of scope (tracker 5.2), so the line stays hidden here.
  t.line("enable_wrapping_detection", false);

  return result;
}

Result printerToggles(const Context &ctx)
{
  Result result;
  Toggler t(result);
  const Cfg config(ctx.printer);
  const bool is_BBL_printer = ctx.isBblPrinter;

  // Basic information
  for (auto el : {"scan_first_layer", "bbl_calib_mark_logo", "bbl_use_printhost"})
    t.line(el, is_BBL_printer);
  for (auto el : {"use_firmware_retraction", "use_relative_e_distances", "support_multi_bed_types", "pellet_modded_printer",
                  "bed_mesh_max", "bed_mesh_min", "bed_mesh_probe_distance", "adaptive_bed_mesh_margin", "thumbnails"})
    t.line(el, !is_BBL_printer);

  const GCodeFlavor gcf = config.en<GCodeFlavor>("gcode_flavor");
  t.line("enable_power_loss_recovery", is_BBL_printer || gcf == gcfMarlinFirmware);
  t.line("parallel_printheads_count", config.b("support_parallel_printheads"));
  t.line("fan_direction", config.b("auxiliary_fan"));
  t.line("support_air_filtration", !config.b("support_cooling_filter"));
  t.line("cooling_filter_enabled", config.b("support_cooling_filter"));

  // Machine G-code: see printToggles for wrapping detection.
  t.line("wrapping_detection_gcode", false);

  // Multimaterial
  {
    const bool supports_wipe_tower_2 = !is_BBL_printer && config.en<WipeTowerType>("wipe_tower_type") == WipeTowerType::Type2;
    t.line("wipe_tower_type", !is_BBL_printer);
    for (auto el : {"enable_filament_ramming", "cooling_tube_retraction", "cooling_tube_length", "parking_pos_retraction",
                    "extra_loading_move", "high_current_on_filament_swap"})
      t.field(el, supports_wipe_tower_2);

    const bool bSEMM = config.b("single_extruder_multi_material");
    t.field("extruders_count", !bSEMM);
    t.field("manual_filament_change", bSEMM);
    t.field("purge_in_prime_tower", bSEMM && supports_wipe_tower_2);

    const QVariant nozzles = ctx.printer ? ctx.printer(QStringLiteral("nozzle_diameter")) : QVariant();
    const int extruders_count = nozzles.typeId() == QMetaType::QVariantList ? qMax(1, int(nozzles.toList().size())) : 1;
    t.field("tool_change_on_wipe_tower", !bSEMM && supports_wipe_tower_2 && extruders_count > 1);
    t.field("wait_for_temp_on_wipe_tower", !bSEMM && supports_wipe_tower_2 && extruders_count > 1);
  }

  // Extruder (single extruder page, index 0)
  {
    const bool have_retract_length = config.f("retraction_length") > 0;
    // extruder_printable_area / _height: disabled, shown only on 2-extruder
    // printers (get_printer_extruder_count() == 2).
    const QVariant nozzles = ctx.printer ? ctx.printer(QStringLiteral("nozzle_diameter")) : QVariant();
    const bool two_extruders = nozzles.typeId() == QMetaType::QVariantList && nozzles.toList().size() == 2;
    t.field("extruder_printable_area", false);
    t.line("extruder_printable_area", two_extruders);
    t.field("extruder_printable_height", false);
    t.line("extruder_printable_height", two_extruders);

    const bool use_firmware_retraction = config.b("use_firmware_retraction");
    t.field("retract_length", !use_firmware_retraction);
    t.field("retraction_minimum_travel", have_retract_length || use_firmware_retraction);

    const bool retraction = have_retract_length || use_firmware_retraction;
    for (auto el : {"z_hop", "retract_when_changing_layer"})
      t.field(el, retraction);
    for (auto el : {"retract_lift_above", "retract_lift_below", "retract_lift_enforce"})
      t.field(el, retraction && config.f("z_hop") > 0);
    for (auto el : {"retraction_speed", "deretraction_speed", "retract_before_wipe", "retract_after_wipe", "retract_length",
                    "retract_restart_extra", "wipe_distance"})
      t.field(el, retraction && !use_firmware_retraction);

    const bool wipe = retraction && config.b("wipe");
    const double retract_before_wipe = config.f("retract_before_wipe");
    const double retract_after_wipe = config.f("retract_after_wipe");
    t.field("retract_before_wipe", wipe && std::abs(retract_after_wipe - 100.) > 1e-6);
    t.field("retract_after_wipe", wipe && std::abs(retract_before_wipe - 100.) > 1e-6);
    t.field("wipe_distance", wipe);

    const bool have_multiple_extruders = true; // upstream: hard-wired true
    t.field("retract_length_toolchange", have_multiple_extruders);
    const bool toolchange_retraction = config.f("retract_length_toolchange") > 0;
    t.field("retract_restart_extra_toolchange", have_multiple_extruders && toolchange_retraction);

    t.field("long_retractions_when_cut", !use_firmware_retraction && config.i("enable_long_retraction_when_cut"));
    t.line("retraction_distances_when_cut", config.b("long_retractions_when_cut"));

    t.field("travel_slope", config.en<ZHopType>("z_hop_types") != ZHopType::zhtNormal);
  }

  // Motion ability
  {
    const bool gcf_is_marlin_legacy = gcf == gcfMarlinLegacy;
    const bool gcf_is_marlin_firmware = gcf == gcfMarlinFirmware;
    const bool gcf_is_klipper = gcf == gcfKlipper;
    const bool gcf_is_reprap_firmware = gcf == gcfRepRapFirmware;

    t.field("machine_max_acceleration_travel", !gcf_is_marlin_legacy && !gcf_is_klipper);
    t.line("machine_max_acceleration_travel", !gcf_is_marlin_legacy && !gcf_is_klipper);
    t.field("machine_max_junction_deviation", gcf_is_marlin_firmware);
    t.line("machine_max_junction_deviation", gcf_is_marlin_firmware);

    bool enable_jerk = !gcf_is_marlin_firmware;
    if (gcf_is_marlin_firmware)
    {
      enable_jerk = true;
      const QVariant jd = ctx.printer ? ctx.printer(QStringLiteral("machine_max_junction_deviation")) : QVariant();
      const QVariantList values = jd.typeId() == QMetaType::QVariantList ? jd.toList() : QVariantList{jd};
      for (const QVariant &v : values)
        if (v.isValid() && v.toDouble() != 0.0)
          enable_jerk = false;
    }
    for (auto el : {"machine_max_jerk_x", "machine_max_jerk_y", "machine_max_jerk_z", "machine_max_jerk_e"})
      t.field(el, enable_jerk);

    t.field("emit_machine_limits_to_gcode", gcf_is_marlin_legacy || gcf_is_marlin_firmware || gcf_is_reprap_firmware);

    const bool resonance_avoidance = config.b("resonance_avoidance");
    t.field("min_resonance_avoidance_speed", resonance_avoidance);
    t.field("max_resonance_avoidance_speed", resonance_avoidance);

    const bool input_shaping_compatible = gcf_is_marlin_firmware || gcf_is_reprap_firmware;
    for (auto el : {"input_shaping_emit", "input_shaping_type", "input_shaping_freq_x", "input_shaping_freq_y",
                    "input_shaping_damp_x", "input_shaping_damp_y"})
      t.line(el, input_shaping_compatible);
    if (input_shaping_compatible)
    {
      const bool emit_limits = config.b("emit_machine_limits_to_gcode");
      t.field("input_shaping_emit", emit_limits);
      const bool input_shaping_emit = emit_limits && config.b("input_shaping_emit");
      t.field("input_shaping_type", input_shaping_emit);
      t.field("input_shaping_freq_x", input_shaping_emit);
      t.field("input_shaping_freq_y", input_shaping_emit && !gcf_is_reprap_firmware);
      t.field("input_shaping_damp_x", input_shaping_emit);
      t.field("input_shaping_damp_y", input_shaping_emit && !gcf_is_reprap_firmware);
    }
  }

  return result;
}

Result filamentToggles(const Context &ctx)
{
  Result result;
  Toggler t(result);
  const Cfg config(ctx.filament);
  const Cfg printer_cfg(ctx.printer);
  const bool is_BBL_printer = ctx.isBblPrinter;

  // Cooling
  {
    const bool has_enable_overhang_bridge_fan = config.b("enable_overhang_bridge_fan");
    for (auto el : {"overhang_fan_speed", "overhang_fan_threshold", "internal_bridge_fan_speed"})
      t.field(el, has_enable_overhang_bridge_fan);
    t.field("dont_slow_down_outer_wall", config.b("slow_down_for_layer_cooling"));
    t.line("initial_layer_fan_speed", config.i("close_fan_the_first_x_layers") <= 0);
    t.line("additional_cooling_fan_speed", printer_cfg.b("auxiliary_fan"));

    const bool support_air_filtration = printer_cfg.b("support_air_filtration");
    for (auto el : {"activate_air_filtration", "during_print_exhaust_fan_speed", "complete_print_exhaust_fan_speed"})
      t.line(el, support_air_filtration);
    if (support_air_filtration)
    {
      const bool activate_air_filtration = config.b("activate_air_filtration");
      t.field("activate_air_filtration_during_print", activate_air_filtration);
      t.field("during_print_exhaust_fan_speed", activate_air_filtration && config.b("activate_air_filtration_during_print"));
      t.field("activate_air_filtration_on_completion", activate_air_filtration);
      t.field("complete_print_exhaust_fan_speed", activate_air_filtration && config.b("activate_air_filtration_on_completion"));
    }
  }

  // Filament
  {
    const bool pa = config.b("enable_pressure_advance");
    t.field("pressure_advance", pa);

    QString bed_temp_1st_layer_key;
    if (ctx.project)
    {
      const QVariant bed = ctx.project(QStringLiteral("curr_bed_type"));
      if (bed.isValid() && !bed.toString().isEmpty())
      {
        const Cfg project(ctx.project);
        bed_temp_1st_layer_key = QString::fromStdString(get_bed_temp_1st_layer_key(project.en<BedType>("curr_bed_type")));
      }
    }
    const QStringList bed_temp_keys = {QStringLiteral("supertack_plate_temp_initial_layer"),
                                       QStringLiteral("cool_plate_temp_initial_layer"),
                                       QStringLiteral("textured_cool_plate_temp_initial_layer"),
                                       QStringLiteral("eng_plate_temp_initial_layer"),
                                       QStringLiteral("textured_plate_temp_initial_layer"),
                                       QStringLiteral("hot_plate_temp_initial_layer")};
    const bool support_multi_bed_types = !bed_temp_keys.contains(bed_temp_1st_layer_key) || is_BBL_printer ||
                                         printer_cfg.b("support_multi_bed_types");
    for (const QString &key : bed_temp_keys)
    {
      const QByteArray k = key.toLatin1();
      t.line(k.constData(), support_multi_bed_types || bed_temp_1st_layer_key == key);
    }

    t.field("adaptive_pressure_advance", pa);
    t.field("adaptive_pressure_advance_overhangs", pa);
    const bool has_adaptive_pa = config.b("adaptive_pressure_advance");
    t.line("adaptive_pressure_advance_overhangs", has_adaptive_pa && pa);
    t.line("adaptive_pressure_advance_model", has_adaptive_pa && pa);
    t.line("adaptive_pressure_advance_bridges", has_adaptive_pa && pa);

    const bool is_pellet_printer = printer_cfg.b("pellet_modded_printer");
    t.line("pellet_flow_coefficient", is_pellet_printer);
    t.line("filament_diameter", !is_pellet_printer);

    t.line("activate_chamber_temp_control", printer_cfg.b("support_chamber_temp_control"));

    t.field("filament_adaptive_volumetric_speed", config.s("volumetric_speed_coefficients") != QLatin1String("0 0 0 0 0 0"));
  }

  // Multimaterial
  {
    for (auto el : {"filament_minimal_purge_on_wipe_tower", "filament_loading_speed_start", "filament_loading_speed",
                    "filament_unloading_speed_start", "filament_unloading_speed", "filament_toolchange_delay",
                    "filament_cooling_moves", "filament_cooling_initial_speed", "filament_cooling_final_speed"})
      t.field(el, !is_BBL_printer);

    const bool multitool_ramming = config.b("filament_multitool_ramming");
    t.field("filament_multitool_ramming_volume", multitool_ramming);
    t.field("filament_multitool_ramming_flow", multitool_ramming);

    const QVariant nozzles = ctx.printer ? ctx.printer(QStringLiteral("nozzle_diameter")) : QVariant();
    const bool is_BBL_multi_extruder =
        is_BBL_printer && nozzles.typeId() == QMetaType::QVariantList && nozzles.toList().size() > 1;
    t.line("long_retractions_when_ec", is_BBL_multi_extruder);
    t.line("retraction_distances_when_ec", is_BBL_multi_extruder && config.b("long_retractions_when_ec"));
  }

  return result;
}

#else // HAS_LIBSLIC3R

Result printToggles(const Context &) { return {}; }
Result printerToggles(const Context &) { return {}; }
Result filamentToggles(const Context &) { return {}; }

#endif // HAS_LIBSLIC3R
} // namespace OptionToggleRules
