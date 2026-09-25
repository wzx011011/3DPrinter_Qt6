// OptionRow.qml - compact typed-option renderer for settings dialogs.
//
// Presentation only: all durable option semantics stay in ConfigOptionModel
// and ConfigViewModel. Edits continue to route through optionModel.setValue().

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root

    required property var optionModel
    required property int optIdx
    required property int rowIndex

    property string searchText: ""
    property bool showGroupHeader: false
    property string oGroup: ""
    property string valueSource: ""
    property bool compact: false
    property int compactLabelWidth: 112
    property int compactFieldWidth: 72
    property int compactEnumWidth: 112

    function displayValueSource(sourceKey) {
        if (sourceKey === "default") return qsTr("默认")
        if (sourceKey === "print") return qsTr("工艺")
        if (sourceKey === "filament") return qsTr("耗材")
        if (sourceKey === "printer") return qsTr("打印机")
        return sourceKey
    }

    // zh_CN row labels. Source of truth: upstream libslic3r PrintConfig
    // labels rendered through the upstream zh_CN locale
    // (OrcaSlicer_zh_CN.po), keyed by option key; keys outside the map
    // fall back to the model label.
    function displayOptionLabel(key, label) {
        var labels = {
            "accel_to_decel_enable": "启用制动速度",
            "accel_to_decel_factor": "制动速度",
            "align_infill_direction_to_model": "将方向对齐到模型",
            "alternate_extra_wall": "交替添加额外内墙",
            "bottom_layer_direction": "底层方向",
            "bottom_shell_layers": "底部壳体层数",
            "bottom_shell_thickness": "底部壳体厚度",
            "bottom_solid_infill_flow_ratio": "底部表面流量比例",
            "bottom_surface_density": "底面密度",
            "bottom_surface_filament_id": "底面",
            "bottom_surface_fill_order": "底面填充顺序",
            "bottom_surface_pattern": "底面图案",
            "bridge_acceleration": "桥接",
            "bridge_angle": "外部桥接填充方向",
            "bridge_density": "外部桥接密度",
            "bridge_flow": "桥接流量",
            "bridge_line_width": "桥接",
            "bridge_no_support": "不支撑桥接",
            "bridge_speed": "外部",
            "brim_ears_detection_length": "圆盘检测半径",
            "brim_ears_max_angle": "圆盘最大角度",
            "brim_ears_outer_only": "仅外轮廓生成圆盘",
            "brim_flow_ratio": "Brim流量比",
            "brim_object_gap": "Brim与模型的间隙",
            "brim_type": "Brim类型",
            "brim_use_efc_outline": "Brim遵循补偿轮廓",
            "brim_width": "Brim宽度",
            "center_of_surface_pattern": "表面图案居中于",
            "combine_brims": "合并Brim",
            "counterbore_hole_bridging": "沉孔搭桥",
            "default_acceleration": "普通打印",
            "default_jerk": "默认",
            "default_junction_deviation": "结点偏差",
            "detect_narrow_internal_solid_infill": "识别狭窄的内部实心填充",
            "detect_overhang_wall": "识别悬垂外墙",
            "detect_thin_wall": "检查薄壁",
            "dont_filter_internal_bridges": "过滤掉小的内部桥接",
            "draft_shield": "风挡",
            "elefant_foot_compensation": "象脚补偿",
            "elefant_foot_compensation_layers": "象脚补偿层数",
            "elefant_foot_layers_density": "象脚补偿层密度",
            "enable_arc_fitting": "圆弧拟合",
            "enable_extra_bridge_layer": "额外桥层（测试版）",
            "enable_mixed_color_sublayer": "混色子层",
            "enable_overhang_speed": "悬垂降速",
            "enable_prime_tower": "开启",
            "enable_support": "开启支撑",
            "enable_tower_interface_cooldown_during_tower": "擦拭塔期间从接触层加速状态冷却",
            "enable_tower_interface_features": "启用擦拭塔接触层功能",
            "enable_wrapping_detection": "启用结块检测",
            "enforce_support_layers": "前几层强制支撑",
            "ensure_vertical_shell_thickness": "确保垂直外壳厚度",
            "exclude_object": "对象排除",
            "extra_perimeters_on_overhangs": "悬垂上的额外周长",
            "extra_solid_infills": "插入实心层",
            "extrusion_rate_smoothing_external_perimeter_only": "仅应用于外部特征",
            "fill_multiline": "填充多线",
            "filter_out_gap_fill": "忽略微小间隙",
            "first_layer_flow_ratio": "第一层流量比",
            "flush_into_infill": "冲刷到对象的填充",
            "flush_into_objects": "冲刷到这个对象",
            "flush_into_support": "冲刷到对象的支撑",
            "fuzzy_skin": "绒毛表面",
            "fuzzy_skin_first_layer": "绒毛表面应用至首层",
            "fuzzy_skin_layers_between_ripple_offset": "波纹偏移之间的层数",
            "fuzzy_skin_mode": "绒毛表面生成器模式",
            "fuzzy_skin_noise_type": "绒毛噪波类型",
            "fuzzy_skin_octaves": "绒毛噪波倍频",
            "fuzzy_skin_persistence": "绒毛噪波持续性",
            "fuzzy_skin_point_distance": "绒毛表面点间距",
            "fuzzy_skin_ripple_offset": "波纹偏移",
            "fuzzy_skin_ripples_per_layer": "每层的波纹数量",
            "fuzzy_skin_scale": "绒毛表面特征尺寸",
            "fuzzy_skin_thickness": "绒毛表面厚度",
            "gap_fill_flow_ratio": "间隙填充流量比",
            "gap_fill_target": "启用间隙填充",
            "gap_infill_speed": "填缝",
            "gcode_add_line_number": "标注行号",
            "gcode_comments": "注释G-code",
            "gcode_label_objects": "标注模型",
            "gyroid_optimized": "Z 轴屈曲偏置优化（实验性）",
            "hole_to_polyhole": "将圆孔转换为多边型孔",
            "hole_to_polyhole_max_edges": "多边形孔最大边数",
            "hole_to_polyhole_threshold": "多边型孔检测边缘",
            "hole_to_polyhole_twisted": "扭曲多边型孔",
            "independent_support_layer_height": "支撑独立层高",
            "infill_anchor": "稀疏填充锚线长度",
            "infill_anchor_max": "填充锚线的最大长度",
            "infill_combination": "合并填充",
            "infill_combination_max_layer_height": "填充组合 - 最大层高",
            "infill_direction": "稀疏填充方向",
            "infill_jerk": "填充",
            "infill_lock_depth": "填充锁定深度",
            "infill_overhang_angle": "填充悬垂角度",
            "infill_shift_step": "填充偏移步长",
            "infill_wall_overlap": "填充/墙 重叠",
            "initial_layer_acceleration": "首层",
            "initial_layer_infill_speed": "首层填充",
            "initial_layer_jerk": "首层",
            "initial_layer_line_width": "首层",
            "initial_layer_min_bead_width": "首层最小墙宽度",
            "initial_layer_print_height": "首层层高",
            "initial_layer_speed": "首层",
            "initial_layer_travel_acceleration": "首层空驶",
            "initial_layer_travel_jerk": "首层空驶",
            "initial_layer_travel_speed": "首层空驶速度",
            "inner_wall_acceleration": "内墙",
            "inner_wall_filament_id": "内墙",
            "inner_wall_flow_ratio": "内壁流量比",
            "inner_wall_jerk": "内墙",
            "inner_wall_line_width": "内墙",
            "inner_wall_speed": "内墙",
            "interface_shells": "接触面外壳",
            "interlocking_beam": "启用互锁梁",
            "interlocking_beam_layer_count": "互锁梁层数",
            "interlocking_beam_width": "互锁梁宽度",
            "interlocking_boundary_avoidance": "互锁与边界的留白量",
            "interlocking_depth": "互锁深度",
            "interlocking_orientation": "互锁方向",
            "internal_bridge_angle": "内部桥接填充方向",
            "internal_bridge_density": "内部桥接密度",
            "internal_bridge_flow": "内部搭桥流量比例",
            "internal_bridge_speed": "内部",
            "internal_solid_filament_id": "内部实心填充",
            "internal_solid_infill_acceleration": "内部实心填充",
            "internal_solid_infill_flow_ratio": "内部固体填充流动比",
            "internal_solid_infill_line_width": "内部实心填充",
            "internal_solid_infill_pattern": "内部实心填充图案",
            "internal_solid_infill_speed": "内部实心填充",
            "ironing_angle": "熨烫角度偏移",
            "ironing_angle_fixed": "固定角度熨烫",
            "ironing_expansion": "熨烫扩展",
            "ironing_flow": "熨烫流量",
            "ironing_inset": "熨烫内缩",
            "ironing_pattern": "熨烫图案",
            "ironing_spacing": "熨烫间距",
            "ironing_speed": "熨烫速度",
            "ironing_type": "熨烫类型",
            "is_infill_first": "首先打印填充",
            "lateral_lattice_angle_1": "侧向晶格角度1",
            "lateral_lattice_angle_2": "侧向晶格角度2",
            "layer_height": "层高",
            "lightning_overhang_angle": "闪电悬垂角度",
            "lightning_prune_angle": "修剪角度",
            "lightning_straightening_angle": "拉直角度",
            "line_width": "默认",
            "make_overhang_printable": "悬垂可打印化",
            "make_overhang_printable_angle": "悬垂可打印化的最大角度",
            "make_overhang_printable_hole_size": "最大孔洞面积",
            "max_bridge_length": "最大桥接长度",
            "max_travel_detour_distance": "避免跨越外墙-最大绕行长度",
            "max_volumetric_extrusion_rate_slope": "平滑挤出率",
            "max_volumetric_extrusion_rate_slope_segment_length": "平滑段长度",
            "min_bead_width": "最窄墙宽度",
            "min_feature_size": "最小特征尺寸",
            "min_length_factor": "最短墙长度",
            "min_skirt_length": "裙边最小挤出长度",
            "min_width_top_surface": "单层墙阈值",
            "minimum_sparse_infill_area": "稀疏填充最小阈值",
            "mmu_segmented_region_interlocking_depth": "分割区域的交错深度",
            "mmu_segmented_region_max_width": "分段区域的最大宽度",
            "only_one_wall_first_layer": "首层仅单层墙",
            "only_one_wall_top": "顶面单层墙",
            "ooze_prevention": "开启",
            "outer_wall_acceleration": "外墙",
            "outer_wall_filament_id": "外墙",
            "outer_wall_flow_ratio": "外壁流量比",
            "outer_wall_jerk": "外墙",
            "outer_wall_line_width": "外墙",
            "outer_wall_speed": "外墙",
            "overhang_1_4_speed": "10%",
            "overhang_2_4_speed": "25%",
            "overhang_3_4_speed": "50%",
            "overhang_4_4_speed": "75%",
            "overhang_flow_ratio": "悬垂流量比",
            "overhang_reverse": "反转偶数层悬垂方向",
            "overhang_reverse_internal_only": "仅反转内部墙壁",
            "overhang_reverse_threshold": "反转阈值",
            "precise_outer_wall": "精准外墙尺寸",
            "precise_z_height": "精准 Z 高度",
            "preheat_steps": "预热步骤",
            "preheat_time": "预热时间",
            "prime_tower_brim_width": "Brim宽度",
            "prime_tower_enable_framework": "内部加强筋",
            "prime_tower_infill_gap": "填补空白",
            "prime_tower_skip_points": "跳过点",
            "prime_tower_width": "宽度",
            "prime_volume": "清理量",
            "print_flow_ratio": "流量比例",
            "print_order": "层内打印顺序",
            "print_plugin_config_overrides": "能力",
            "print_sequence": "打印顺序",
            "raft_contact_distance": "筏层Z间距",
            "raft_first_layer_density": "首层密度",
            "raft_first_layer_expansion": "首层扩展",
            "raft_layers": "筏层",
            "reduce_crossing_wall": "避免跨越外墙",
            "reduce_infill_retraction": "减小填充回抽",
            "relative_bridge_angle": "相对桥接角度",
            "resolution": "分辨率",
            "role_based_wipe_speed": "自动擦拭速度",
            "scarf_angle_threshold": "角度阈值",
            "scarf_joint_flow_ratio": "斜拼接缝流量",
            "scarf_joint_speed": "斜拼接缝速度",
            "scarf_overhang_threshold": "悬垂阈值",
            "seam_gap": "接缝间隔",
            "seam_position": "接缝位置",
            "seam_slope_conditional": "选择性应用斜拼接缝",
            "seam_slope_entire_loop": "围绕整个围墙",
            "seam_slope_inner_walls": "应用斜拼于内墙",
            "seam_slope_min_length": "斜拼接缝长度",
            "seam_slope_start_height": "斜拼接缝起始高度",
            "seam_slope_steps": "斜拼段数",
            "seam_slope_type": "斜拼接缝（试验）",
            "separated_infills": "分离式填充",
            "set_other_flow_ratios": "设置其他流量比",
            "single_extruder_multi_material_priming": "所有挤出机画线",
            "single_loop_draft_shield": "首层后单圈",
            "skeleton_infill_density": "骨架填充密度",
            "skeleton_infill_line_width": "骨架线宽",
            "skin_infill_density": "外壳填充密度",
            "skin_infill_depth": "外壳填充深度",
            "skin_infill_line_width": "外壳线宽",
            "skirt_distance": "裙边距离",
            "skirt_height": "裙边高度",
            "skirt_loops": "裙边圈数",
            "skirt_speed": "裙边速度",
            "skirt_start_angle": "裙边起始点",
            "skirt_type": "裙边类型",
            "slice_closing_radius": "切片间隙闭合半径",
            "slicing_mode": "切片模式",
            "slow_down_layers": "慢速层数量",
            "slowdown_for_curled_perimeters": "翘边降速",
            "small_area_infill_flow_compensation": "小区域填充流量补偿（试验）",
            "small_area_infill_flow_compensation_model": "流量补偿模型",
            "small_perimeter_speed": "微小部位",
            "small_perimeter_threshold": "微小部位周长阈值",
            "small_support_perimeter_speed": "支撑微小部位",
            "small_support_perimeter_threshold": "支撑微小部位阈值",
            "solid_infill_direction": "实心填充方向",
            "solid_infill_rotate_template": "实心填充旋转模板",
            "sparse_infill_acceleration": "稀疏填充",
            "sparse_infill_density": "稀疏填充密度",
            "sparse_infill_filament_id": "填充",
            "sparse_infill_flow_ratio": "稀疏填充流量比",
            "sparse_infill_line_width": "稀疏填充",
            "sparse_infill_pattern": "稀疏填充图案",
            "sparse_infill_rotate_template": "稀疏填充旋转模板",
            "sparse_infill_smooth_factor": "稀疏填充平滑系数",
            "sparse_infill_speed": "稀疏填充",
            "spiral_finishing_flow_ratio": "螺旋结束流量比",
            "spiral_mode": "旋转花瓶",
            "spiral_mode_max_xy_smoothing": "最大XY平滑阈值",
            "spiral_mode_smooth": "光滑螺旋模式",
            "spiral_starting_flow_ratio": "螺旋开始流量比",
            "staggered_inner_seams": "交错的内墙接缝",
            "standby_temperature_delta": "软化温度",
            "support_angle": "图案角度",
            "support_base_pattern": "支撑主体图案",
            "support_base_pattern_spacing": "主体图案线距",
            "support_bottom_interface_spacing": "底部接触面线距",
            "support_bottom_z_distance": "底部Z距离",
            "support_critical_regions_only": "仅支撑关键区域",
            "support_expansion": "普通支撑拓展",
            "support_filament": "支撑/筏层主体",
            "support_flow_ratio": "支撑流量比",
            "support_interface_bottom_layers": "底部接触面层数",
            "support_interface_filament": "支撑/筏层界面",
            "support_interface_flow_ratio": "支撑面流量比例",
            "support_interface_loop_pattern": "接触面采用圈形走线。",
            "support_interface_not_for_body": "界面材料不用于主体",
            "support_interface_pattern": "支撑面图案",
            "support_interface_spacing": "顶部接触面线距",
            "support_interface_speed": "支撑面",
            "support_interface_top_layers": "顶部接触面层数",
            "support_ironing": "支撑界面熨烫",
            "support_ironing_flow": "支撑熨烫流量",
            "support_ironing_pattern": "支撑熨烫图案",
            "support_ironing_spacing": "支撑熨烫线间距",
            "support_line_width": "支撑",
            "support_object_first_layer_gap": "支撑/对象首层间距",
            "support_object_xy_distance": "支撑/模型xy间距",
            "support_on_build_plate_only": "仅在打印板生成",
            "support_remove_small_overhang": "忽略微小悬垂",
            "support_speed": "支撑",
            "support_style": "样式",
            "support_threshold_angle": "阈值角度",
            "support_threshold_overlap": "阈值支撑比例",
            "support_top_z_distance": "顶部Z距离",
            "support_type": "类型",
            "symmetric_infill_y_axis": "对称填充Y轴",
            "thick_bridges": "外部搭桥用厚桥",
            "thick_internal_bridges": "内部搭桥用厚桥",
            "timelapse_type": "延时摄影",
            "toolchange_ordering": "换料顺序",
            "top_bottom_infill_wall_overlap": "顶/底部实心填充/墙重叠率",
            "top_layer_direction": "顶层方向",
            "top_shell_layers": "顶部壳体层数",
            "top_shell_thickness": "顶部壳体厚度",
            "top_solid_infill_flow_ratio": "顶部表面流量比例",
            "top_surface_acceleration": "顶面",
            "top_surface_density": "顶面密度",
            "top_surface_expansion": "顶面扩展",
            "top_surface_expansion_direction": "顶面扩展方向",
            "top_surface_expansion_margin": "顶面扩展墙边距",
            "top_surface_filament_id": "顶面",
            "top_surface_fill_order": "顶面填充顺序",
            "top_surface_jerk": "顶面",
            "top_surface_line_width": "顶面",
            "top_surface_pattern": "顶面图案",
            "top_surface_speed": "顶面",
            "travel_acceleration": "空驶",
            "travel_jerk": "空驶",
            "travel_speed": "空驶",
            "tree_support_angle_slow": "首选分支角度",
            "tree_support_auto_brim": "自动裙边宽度",
            "tree_support_branch_angle": "树状支撑分支角度",
            "tree_support_branch_angle_organic": "树状支撑分支角度",
            "tree_support_branch_diameter": "树状支撑分支直径",
            "tree_support_branch_diameter_angle": "分支直径的角度",
            "tree_support_branch_diameter_organic": "树状支撑分支直径",
            "tree_support_branch_distance": "树状支撑分支距离",
            "tree_support_branch_distance_organic": "树状支撑分支距离",
            "tree_support_brim_width": "树状支撑裙边宽度",
            "tree_support_tip_diameter": "尖端直径",
            "tree_support_top_rate": "分支密度",
            "tree_support_wall_count": "支撑外墙层数",
            "wall_direction": "围墙打印方向",
            "wall_distribution_count": "墙分布计数",
            "wall_generator": "墙生成器",
            "wall_loops": "墙层数",
            "wall_maximum_deviation": "最大墙体偏差",
            "wall_maximum_resolution": "最大墙体分辨率",
            "wall_sequence": "墙顺序",
            "wall_transition_angle": "墙过渡阈值角度",
            "wall_transition_filter_deviation": "墙过渡过滤间距",
            "wall_transition_length": "墙过渡长度",
            "wipe_before_external_loop": "额外的外墙打印前擦拭",
            "wipe_on_loops": "闭环擦拭",
            "wipe_speed": "擦拭速度",
            "wipe_tower_bridging": "最大桥接距离",
            "wipe_tower_cone_angle": "稳定锥体顶角",
            "wipe_tower_extra_flow": "额外冲刷量",
            "wipe_tower_extra_rib_length": "额外加强筋长度",
            "wipe_tower_extra_spacing": "擦拭塔冲刷线间距",
            "wipe_tower_filament": "擦拭塔",
            "wipe_tower_fillet_wall": "墙加圆角",
            "wipe_tower_max_purge_speed": "擦拭塔最大打印速度",
            "wipe_tower_no_sparse_layers": "无稀疏层 （实验功能）",
            "wipe_tower_rib_width": "加强筋宽度",
            "wipe_tower_rotation_angle": "擦拭塔旋转角度",
            "wipe_tower_wall_type": "墙类型",
            "xy_contour_compensation": "X-Y 外轮廓尺寸补偿",
            "xy_hole_compensation": "X-Y 孔洞尺寸补偿",
            "zaa_dont_alternate_fill_direction": "不交替填充方向",
            "zaa_enabled": "启用 Z 层抗锯齿",
            "zaa_min_z": "最小 Z 高度",
            "zaa_minimize_perimeter_height": "最小化墙高角度"
        }
        return labels[key] || label
    }

    // zh_CN group titles for the upstream Tab.cpp option groups
    // (new_optgroup titles, OrcaSlicer_zh_CN.po translations).
    function displayGroupLabel(group) {
        var groups = {
            "Layer height": qsTr("层高"),
            "Line width": qsTr("线宽"),
            "Seam": qsTr("接缝"),
            "Precision": qsTr("精度"),
            "Ironing": qsTr("熨烫"),
            "Z contouring": qsTr("Z 层抗锯齿"),
            "Wall generator": qsTr("墙生成器"),
            "Walls and surfaces": qsTr("墙壁和表面"),
            "Bridging": qsTr("搭桥"),
            "Overhangs": qsTr("悬垂"),
            "Walls": qsTr("墙"),
            "Top/bottom shells": qsTr("顶部/底部外壳"),
            "Infill": qsTr("填充"),
            "Advanced": qsTr("高级"),
            "First layer speed": qsTr("首层速度"),
            "Other layers speed": qsTr("其他层速度"),
            "Overhang speed": qsTr("悬垂速度"),
            "Travel speed": qsTr("空驶速度"),
            "Acceleration": qsTr("加速度"),
            "Junction Deviation": qsTr("结点偏差"),
            "Jerk(XY)": qsTr("抖动（XY轴）"),
            "Support": qsTr("支撑"),
            "Raft": qsTr("筏层"),
            "Filament for Supports": qsTr("支撑耗材"),
            "Support ironing": qsTr("支撑熨烫"),
            "Tree supports": qsTr("树状支撑"),
            "Prime tower": qsTr("擦拭塔"),
            "Filament for Features": qsTr("特征用耗材"),
            "Ooze prevention": qsTr("Ooze 预防"),
            "Flush options": qsTr("换料冲刷选项"),
            "Skirt": qsTr("裙边"),
            "Brim": qsTr("Brim"),
            "Special mode": qsTr("特殊模式"),
            "Fuzzy skin": qsTr("绒毛表面"),
            "G-code output": qsTr("G-code 输出"),
            "Plugin Configuration": qsTr("插件配置")
        }
        return groups[group] || group
    }

    // Group icons matching the upstream new_optgroup icon names
    // (Tab.cpp:2638-3125). Static decoration only.
    function groupIconSource(group) {
        var icons = {
            "Layer height": "qrc:/qml/assets/icons/param_layer_height.svg",
            "Line width": "qrc:/qml/assets/icons/param_line_width.svg",
            "Seam": "qrc:/qml/assets/icons/param_seam.svg",
            "Precision": "qrc:/qml/assets/icons/param_precision.svg",
            "Ironing": "qrc:/qml/assets/icons/param_ironing.svg",
            "Z contouring": "qrc:/qml/assets/icons/param_z_contouring.svg",
            "Wall generator": "qrc:/qml/assets/icons/param_wall_generator.svg",
            "Walls and surfaces": "qrc:/qml/assets/icons/param_wall_surface.svg",
            "Bridging": "qrc:/qml/assets/icons/param_bridge.svg",
            "Overhangs": "qrc:/qml/assets/icons/param_overhang.svg",
            "Walls": "qrc:/qml/assets/icons/param_wall.svg",
            "Top/bottom shells": "qrc:/qml/assets/icons/param_shell.svg",
            "Infill": "qrc:/qml/assets/icons/param_infill.svg",
            "Advanced": "qrc:/qml/assets/icons/param_advanced.svg",
            "First layer speed": "qrc:/qml/assets/icons/param_speed_first.svg",
            "Other layers speed": "qrc:/qml/assets/icons/param_speed.svg",
            "Overhang speed": "qrc:/qml/assets/icons/param_overhang_speed.svg",
            "Travel speed": "qrc:/qml/assets/icons/param_travel_speed.svg",
            "Acceleration": "qrc:/qml/assets/icons/param_acceleration.svg",
            "Junction Deviation": "qrc:/qml/assets/icons/param_junction_deviation.svg",
            "Jerk(XY)": "qrc:/qml/assets/icons/param_jerk.svg",
            "Support": "qrc:/qml/assets/icons/param_support.svg",
            "Raft": "qrc:/qml/assets/icons/param_raft.svg",
            "Filament for Supports": "qrc:/qml/assets/icons/param_support_filament.svg",
            "Support ironing": "qrc:/qml/assets/icons/param_ironing.svg",
            "Tree supports": "qrc:/qml/assets/icons/param_support_tree.svg",
            "Prime tower": "qrc:/qml/assets/icons/param_tower.svg",
            "Filament for Features": "qrc:/qml/assets/icons/param_filament_for_features.svg",
            "Ooze prevention": "qrc:/qml/assets/icons/param_ooze_prevention.svg",
            "Flush options": "qrc:/qml/assets/icons/param_flush.svg",
            "Skirt": "qrc:/qml/assets/icons/param_skirt.svg",
            "Brim": "qrc:/qml/assets/icons/param_adhension.svg",
            "Special mode": "qrc:/qml/assets/icons/param_special.svg",
            // Upstream fuzzy_skin group icon (Tab.cpp:3072) has no
            // counterpart in the project icon set (fuzzy_skin.svg is absent
            // from disk and from qml.qrc); param_rectilinear is the closest
            // registered line-texture glyph so the header stays filled.
            "Fuzzy skin": "qrc:/qml/assets/icons/param_rectilinear.svg",
            "G-code output": "qrc:/qml/assets/icons/param_gcode.svg",
            "Plugin Configuration": "qrc:/qml/assets/icons/param_gcode.svg"
        }
        return icons[group] || ""
    }

    function normalizedNumber(value, fallbackValue) {
        if (typeof value === "number")
            return value
        var parsed = parseFloat(value)
        return isNaN(parsed) ? fallbackValue : parsed
    }

    function formattedNumber(value) {
        var numberValue = root.normalizedNumber(value, root.oMin)
        if (root.oType === "int" || root.oType === "percent")
            return Math.round(numberValue).toString()
        var rounded = Math.round(numberValue * 1000) / 1000
        return rounded.toFixed(rounded % 1 === 0 ? 0 : 2)
    }

    function clampNumber(value) {
        var numberValue = root.normalizedNumber(value, root.oMin)
        if (numberValue < root.oMin) numberValue = root.oMin
        if (numberValue > root.oMax) numberValue = root.oMax
        return root.oType === "int" || root.oType === "percent"
            ? Math.round(numberValue)
            : numberValue
    }

    function setNumericValue(value) {
        if (!root.optionModel || root.oRO)
            return
        root.optionModel.setValue(root.optIdx, root.clampNumber(value))
    }

    readonly property string oType: root.optionModel ? root.optionModel.optType(optIdx) : ""
    readonly property string oKey: root.optionModel ? root.optionModel.optKey(optIdx) : ""
    readonly property string oLabel: root.optionModel ? root.displayOptionLabel(root.oKey, root.optionModel.optLabel(optIdx)) : ""
    readonly property var oVal: root.optionModel ? root.optionModel.optValue(optIdx) : 0
    readonly property double oMin: root.optionModel ? root.optionModel.optMin(optIdx) : 0
    readonly property double oMax: root.optionModel ? root.optionModel.optMax(optIdx) : 1
    readonly property double oStep: root.optionModel ? root.optionModel.optStep(optIdx) : 1
    readonly property bool oRO: root.optionModel ? root.optionModel.optReadonly(optIdx) : false
    readonly property bool oDirty: root.optionModel ? root.optionModel.optIsDirty(optIdx) : false
    readonly property string oTip: root.optionModel ? root.optionModel.optTooltip(optIdx) : ""
    readonly property string oUnit: root.optionModel ? root.optionModel.optUnit(optIdx) : ""
    readonly property string oSidetext: root.optionModel ? root.optionModel.optSidetext(root.optIdx) : ""
    readonly property string displayUnit: root.oSidetext !== "" ? root.oSidetext : root.oUnit
    readonly property bool oNullable: root.optionModel ? root.optionModel.optNullable(optIdx) : false
    readonly property bool oIsVector: root.optionModel ? root.optionModel.optIsVector(optIdx) : false
    readonly property var oEnumLabels: {
        if (!root.optionModel || root.oType !== "enum") return []
        return root.optionModel.optEnumLabelsList(root.optIdx)
    }

    readonly property bool isNumeric: root.oType === "int" || root.oType === "double" || root.oType === "percent"
    // optMin/optMax are ConfigOption schema bounds, not a two-value option.
    // Keep range-like keys identifiable so their permitted interval can be
    // shown without presenting the bounds as independently editable values.
    readonly property bool isRangeLike:
        root.isNumeric && (root.oKey.indexOf("_range") >= 0
            || root.oKey.indexOf("_min") >= 0
            || root.oKey.indexOf("_max") >= 0
            || root.oKey === "fan_min_speed"
            || root.oKey === "fan_max_speed"
            || root.oKey === "nozzle_temperature_range"
            || root.oLabel.toLowerCase().indexOf("range") >= 0)
    readonly property bool isColorLike:
        root.oKey.toLowerCase().indexOf("colour") >= 0
        || root.oKey.toLowerCase().indexOf("color") >= 0
    // Phase 236 (DLG-01): keys whose value is edited in a dedicated dialog
    // rather than the inline field — G-code fields (machine_start_gcode,
    // machine_end_gcode, ...) open EditGCodeDialog; the bed geometry keys
    // (printable_area / bed_shape) open BedShapeDialog. Mirrors upstream
    // ConfigOptionsGroup button pickers for these option types.
    readonly property bool isGcodeOption:
        root.oType === "string" && root.oKey.length > 6
        && root.oKey.slice(-6) === "_gcode"
    readonly property bool isBedShapeOption:
        root.oKey === "printable_area" || root.oKey === "bed_shape"
    readonly property bool hasTrailingDialogAction: root.isGcodeOption || root.isBedShapeOption
    readonly property bool hasBounds: root.isNumeric && root.oMax > root.oMin

    readonly property int headerHeight: root.showGroupHeader ? (root.compact ? 28 : 32) : 0
    readonly property int rowHeight:
        root.compact ? (root.oType === "string" ? 48 : 34)
        : root.oType === "string" ? 70
        : 44
    readonly property int contentHeight: root.rowHeight
    readonly property int totalHeight: root.headerHeight + root.rowHeight
    readonly property int controlColumnWidth:
        root.compact
        ? Math.max(root.compactEnumWidth, root.compactFieldWidth + 76)
        : 230
    // BUILDGATE restore (2026-09-24): the fixed state-indicator lane is back
    // (settingsOptionRowsRestorePhase86ControlContract locks its ids; the
    // hover tooltip below keeps summarizing the same metadata).
    readonly property int metadataLaneWidth: root.compact ? 88 : 112
    // Row metadata (value source / RO / inherit / vector / bounds) that used
    // to render as an inline badge lane now folds into the hover tooltip
    // (ref layout: label + single field only).
    readonly property string metadataSummary: {
        var parts = []
        if (root.valueSource !== "")
            parts.push(root.displayValueSource(root.valueSource))
        if (root.oRO)
            parts.push("RO")
        if (root.oNullable)
            parts.push(qsTr("继承"))
        if (root.oIsVector)
            parts.push(qsTr("多值"))
        if (root.hasBounds)
            parts.push(qsTr("范围") + " " + root.formattedNumber(root.oMin)
                       + " - " + root.formattedNumber(root.oMax))
        return parts.join(" · ")
    }
    readonly property string rowTooltip:
        root.oTip !== "" && root.metadataSummary !== ""
        ? root.oTip + "\n" + root.metadataSummary
        : (root.metadataSummary !== "" ? root.metadataSummary : root.oTip)

    ToolTip.visible: root.rowTooltip !== "" && tipMA.containsMouse
    ToolTip.text: root.rowTooltip
    ToolTip.delay: 500

    Rectangle {
        id: sectionHeader
        visible: root.showGroupHeader
        anchors.top: parent.top
        width: parent.width
        height: root.headerHeight
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.compact ? 6 : 14
            anchors.rightMargin: root.compact ? 10 : 18
            spacing: 5  // upstream StaticLine: icon + 5px + title + line

            // Upstream OptionsGroup renders its title through StaticLine with
            // an 18px group icon (StaticLine.cpp:37,93-115). Still a static
            // title -- the icon is decoration with no click affordance.
            Image {
                visible: root.groupIconSource(root.oGroup) !== ""
                source: root.groupIconSource(root.oGroup)
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                fillMode: Image.PreserveAspectFit
            }

            Text {
                text: root.displayGroupLabel(root.oGroup)
                color: "#f0f0f0"
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                elide: Text.ElideRight
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                id: sectionDivider
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#4c4c55"
            }
        }
    }

    Rectangle {
        id: paramRow
        y: root.headerHeight
        width: parent.width
        height: root.rowHeight
        // Ref/upstream rows share the panel background -- no zebra striping.
        color: "transparent"
        opacity: root.oRO ? 0.72 : 1.0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: root.compact ? 10 : 20
            anchors.rightMargin: root.compact ? 10 : Theme.fontSizeLG
            spacing: root.compact ? 8 : 12

            Item {
                Layout.preferredWidth: root.compact ? root.compactLabelWidth : 180
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 6

                    Rectangle {
                        id: dirtyBadge
                        visible: root.oDirty
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        radius: 3
                        color: Theme.statusWarning
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.oLabel
                        color: root.oRO ? Theme.textDisabled
                              : root.oDirty ? Theme.statusWarning
                              : root.compact ? Theme.textPrimary
                              : Theme.textSecondary
                        font.pixelSize: root.compact ? Theme.fontSizeLG : Theme.fontSizeMD
                        font.bold: root.oDirty || root.searchText !== ""
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // BUILDGATE restore (2026-09-24): the Phase 86 fixed badge lane,
            // verbatim from the last committed revision — the audit
            // (settingsOptionRowsRestorePhase86ControlContract) still locks
            // these ids even though the summary also folds into the tooltip.
            RowLayout {
                id: metadataLane
                Layout.preferredWidth: root.metadataLaneWidth
                Layout.minimumWidth: root.metadataLaneWidth
                Layout.maximumWidth: root.metadataLaneWidth
                Layout.fillHeight: true
                spacing: 4

                CxBadge {
                    id: sourceBadge
                    visible: root.valueSource !== ""
                    label: root.displayValueSource(root.valueSource)
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: readOnlyBadge
                    visible: root.oRO
                    label: "RO"
                    colorToken: Theme.textDisabled
                    fillToken: Theme.bgPanel
                }

                CxBadge {
                    id: nullableBadge
                    visible: root.oNullable
                    label: "Inh"
                    colorToken: Theme.statusInfo
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: vectorBadge
                    visible: root.oIsVector
                    label: "E"
                    colorToken: Theme.accent
                    fillToken: Theme.bgInset
                }

                CxBadge {
                    id: boundsBadge
                    visible: root.hasBounds
                    label: "rng"
                    colorToken: Theme.textTertiary
                    fillToken: Theme.bgInset
                }
            }

            Item {
                id: controlCell
                Layout.fillWidth: true
                Layout.fillHeight: true

                CxCheckBox {
                    visible: root.oType === "bool"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: ""
                    checked: root.oVal === true || root.oVal === "true"
                    enabled: !root.oRO
                    onToggled: root.optionModel.setValue(root.optIdx, checked)
                }

                RowLayout {
                    id: numericCluster
                    visible: root.isNumeric
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    spacing: 6

                    CxSpinBox {
                        visible: root.oType === "int" || root.oType === "percent"
                        Layout.preferredWidth: root.compact ? root.compactFieldWidth : 90
                        Layout.preferredHeight: root.compact ? 24 : Theme.controlHeightSM
                        value: root.clampNumber(root.oVal)
                        from: Math.round(root.oMin)
                        to: Math.round(root.oMax)
                        stepSize: Math.max(1, Math.round(root.oStep))
                        suffix: root.displayUnit
                        enabled: !root.oRO
                        editable: true
                        onValueModified: root.optionModel.setValue(root.optIdx, value)
                    }

                    CxNumericEdit {
                        visible: root.oType === "double"
                        Layout.preferredWidth: root.compact ? root.compactFieldWidth : 120
                        Layout.preferredHeight: root.compact ? 24 : Theme.controlHeightSM
                        decimals: root.oType === "int" || root.oType === "percent" ? 0 : 3
                        text: root.formattedNumber(root.oVal)
                        // Unit lives inside the field, right-aligned
                        // (upstream Field.cpp:923 combine_side_text).
                        suffix: root.displayUnit
                        enabled: !root.oRO
                        onCommit: (valueText) => root.setNumericValue(valueText)
                    }
                }

                RowLayout {
                    id: rangeCluster
                    visible: root.isRangeLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: numericCluster.left
                    anchors.rightMargin: 8
                    spacing: 4

                    Text {
                        id: rangeMinLabel
                        text: qsTr("Min")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        id: rangeMinEditor
                        text: root.formattedNumber(root.oMin)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        visible: root.displayUnit !== ""
                        text: root.displayUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        id: rangeMaxLabel
                        text: qsTr("Max")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        id: rangeMaxEditor
                        text: root.formattedNumber(root.oMax)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        visible: root.displayUnit !== ""
                        text: root.displayUnit
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                CxComboBox {
                    visible: root.oType === "enum"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: root.compact ? root.compactEnumWidth : 160
                    enabled: !root.oRO
                    model: root.oEnumLabels
                    currentIndex: typeof root.oVal === "number" ? root.oVal : 0
                    onActivated: (i) => root.optionModel.setValue(root.optIdx, i)
                }

                RowLayout {
                    visible: root.oType === "string" && root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    width: Math.min(parent.width, root.controlColumnWidth)
                    spacing: 6

                    Rectangle {
                        id: colorSwatchButton
                        Layout.preferredWidth: root.compact ? 24 : 28
                        Layout.preferredHeight: root.compact ? 24 : 28
                        color: "transparent"

                        Rectangle {
                            id: colorSwatch
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            radius: 2
                            color: (typeof root.oVal === "string" && root.oVal.length > 0)
                                   ? root.oVal : Theme.accent
                            border.color: Theme.borderDefault
                            border.width: 1
                        }

                        HoverHandler { id: colorSwatchHover }
                        ToolTip.visible: colorSwatchHover.hovered
                        ToolTip.text: qsTr("Edit the color value in the field")
                        ToolTip.delay: 500
                    }

                    CxTextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.compact ? 24 : Theme.controlHeightSM
                        text: typeof root.oVal === "string" ? root.oVal : ""
                        enabled: !root.oRO
                        onEditingFinished: root.optionModel.setValue(root.optIdx, text)
                    }
                }

                CxTextArea {
                    visible: root.oType === "string" && !root.isColorLike
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.rightMargin: root.hasTrailingDialogAction ? 34 : 0
                    height: root.compact ? 42 : 60
                    text: typeof root.oVal === "string" ? root.oVal : (root.oVal ? root.oVal.toString() : "")
                    font.pixelSize: Theme.fontSizeSM
                    readOnly: root.oRO
                    wrapMode: TextArea.Wrap
                    onTextChanged: {
                        if (root.optionModel && activeFocus)
                            root.optionModel.setValue(root.optIdx, text)
                    }
                }

                // Phase 236 (DLG-01): whole-option editor affordance. The
                // "edit" button next to G-code / bed-shape rows requests the
                // dedicated dialog from BackendContext, which routes the
                // current key + value through showEditGCodeDialogRequested /
                // showBedShapeDialogRequested (value forwarded so the dialog
                // opens with the preset's current text).
                Row {
                    visible: root.hasTrailingDialogAction
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    spacing: 4
                    z: 2

                    CxIconButton {
                        buttonSize: 24
                        iconSize: 13
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: root.isGcodeOption
                            ? "qrc:/qml/assets/icons/clipboard.svg"
                            : "qrc:/qml/assets/icons/settings.svg"
                        toolTipText: root.isGcodeOption
                            ? qsTr("编辑 G-code…")
                            : qsTr("编辑热床形状…")
                        enabled: true
                        onClicked: {
                            if (typeof backend === "undefined" || !backend)
                                return
                            if (root.isGcodeOption)
                                backend.showEditGCodeDialog(
                                    root.oKey,
                                    typeof root.oVal === "string" ? root.oVal
                                        : (root.oVal ? root.oVal.toString() : ""))
                            else
                                backend.showBedShapeDialog()
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: tipMA
        anchors.fill: paramRow
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        z: -1
    }
}
