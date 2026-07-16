#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
translate_en_ts.py
==================
Fill all `type="unfinished"` translations in i18n/en.ts with real English.

Approach: regex in-place replacement on the .ts text (NOT xml.etree, which
reorders attributes and breaks the Qt Linguist format).

- 1218 unique unfinished sources: 975 CJK (manual English mapping) +
  243 non-CJK (already English; translation == source).
- The dictionary keys are the EXACT escaped <source> text as it appears in the
  .ts file (e.g. "&gt;" stays "&gt;"). Translations are XML-escaped before
  being written.
- Output format: <translation>English</translation> (no type attribute,
  matching the existing finished entries that use no-space form).
- CRLF line endings are preserved by reading/writing in binary-ish text mode
  (we never touch the newline bytes; regex operates on the decoded string but
  Python preserves '\r\n' as-is inside the string).

Re-runnable: safe to run multiple times (idempotent on already-finished entries
because they no longer carry type="unfinished").
"""

import os
import re
import sys
import xml.sax.saxutils as saxutils

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
EN_TS = os.path.join(ROOT, 'i18n', 'en.ts')


# ---------------------------------------------------------------------------
# CJK source -> English translation.
# Keys use the EXACT escaped form found in the .ts file (e.g. "&gt;").
# Translations are plain English; the runner XML-escapes them before writing.
# ---------------------------------------------------------------------------
CJK_TRANSLATIONS = {
    # --- AMS / slot mapping ---
    "槽位 %1": "Slot %1",
    "(温度覆盖 %1°C)": "(Temperature override %1°C)",
    "添加映射": "Add Mapping",
    "槽位耗材余量": "Slot filament remaining",
    "关闭": "Close",

    # --- About dialog ---
    "关于 OWzx": "About OWzx",
    "版本 2.4.0-dev  (Qt6 QML)": "Version 2.4.0-dev  (Qt6 QML)",
    "本软件基于 Qt 6 框架构建，遵循 GNU LGPL v3 协议。使用本软件即代表您同意相关使用条款。":
        "This software is built on the Qt 6 framework and licensed under the GNU LGPL v3. "
        "By using this software you agree to the relevant terms of use.",

    # --- Access code / connection ---
    "连接到 %1": "Connect to %1",
    "连接 Bambu 打印机": "Connect Bambu Printer",
    "输入局域网访问码以建立 MQTT 连接": "Enter the LAN access code to establish an MQTT connection",
    "打印机 IP 地址": "Printer IP address",
    "局域网访问码": "LAN Access Code",
    "在打印机屏幕：设置 &gt; 网络 &gt; 局域网访问码":
        "On the printer screen: Settings > Network > LAN Access Code",
    "MQTT 端口": "MQTT Port",
    "高级": "Advanced",
    "取消": "Cancel",

    # --- Assembly / measure ---
    "装配测量": "Assembly Measure",
    "距离": "Distance",
    "关闭测量": "Close Measure",
    "请将爆炸比例重置为 1.00 后再使用测量":
        "Please reset the explosion scale to 1.00 before using measure",
    "爆炸比例": "Explosion Scale",
    "装配体信息": "Assembly Info",
    "对象: %1": "Object: %1",

    # --- Top-level menus / navigation ---
    "新建项目": "New Project",
    "打开项目...": "Open Project...",
    "保存项目": "Save Project",
    "校准": "Calibration",
    "准备": "Prepare",
    "预览": "Preview",
    "设备": "Device",
    "项目": "Project",
    "切片": "Slice",
    "打印": "Print",
    "通知中心": "Notification Center",
    "最小化": "Minimize",
    "还原": "Restore",
    "最大化": "Maximize",
    "切片单盘": "Slice Single Plate",
    "导出G-code文件": "Export G-code File",
    "切换到装配视图": "Switch to Assembly View",
    "最近文件": "Recent Files",
    "清空最近文件": "Clear Recent Files",
    "项目另存为...": "Save Project As...",

    # --- Import/export file filters ---
    "3MF 文件 (*.3mf)": "3MF Files (*.3mf)",
    "STL 文件 (*.stl)": "STL Files (*.stl)",
    "OBJ 文件 (*.obj)": "OBJ Files (*.obj)",
    "STEP 文件 (*.step *.stp)": "STEP Files (*.step *.stp)",
    "AMF 文件 (*.amf)": "AMF Files (*.amf)",
    "导出": "Export",
    "剪切": "Cut",
    "删除选中": "Delete Selected",
    "取消选择": "Deselect",
    "反向选择": "Invert Selection",
    "视图": "View",
    "显示/隐藏 Gizmo": "Show/Hide Gizmo",
    "重置视图": "Reset View",
    "显示层": "Show Layer",
    "隐藏层": "Hide Layer",

    # --- Toast / status messages ---
    "导入失败": "Import failed",
    "切片失败": "Slicing failed",
    # Garbled (mojibake) source string present in upstream; original was likely "导出失败"
    "瀵煎嚭澶辫触": "Export failed",
    "项目已保存": "Project saved",
    "项目已保存到: %1": "Project saved to: %1",
    "切片中": "Slicing",
    "正在切片... %1%": "Slicing... %1%",
    "切片完成": "Slicing complete",
    "切片已完成，可以预览或导出 G-code": "Slicing complete. You can preview or export the G-code.",
    "已导出到: %1": "Exported to: %1",
    "导出完成": "Export complete",
    "导出中": "Exporting",
    "正在导出 G-code...": "Exporting G-code...",
    "工作区警告": "Workspace Warning",
    "工作区错误": "Workspace Error",
    "验证错误": "Validation Error",
    "验证警告": "Validation Warning",
    "排列中": "Arranging",
    "正在自动排列... %1%": "Auto arranging... %1%",

    # --- Tips / educational hints ---
    "层高越小打印越精细，但耗时越长。常用范围: 0.1mm - 0.3mm。":
        "A smaller layer height gives finer prints but takes longer. Common range: 0.1mm - 0.3mm.",
    "填充密度影响模型强度和重量。20% 适合大多数场景，100% 为实心。":
        "Infill density affects model strength and weight. 20% suits most cases; 100% is solid.",
    "悬空角度超过 45° 的部分需要支撑。合理使用支撑可以提升打印质量。":
        "Overhangs steeper than 45° need support. Proper use of support improves print quality.",
    "打印速度越快效率越高，但可能影响表面质量。建议先慢后快测试。":
        "Faster print speed improves throughput but may affect surface quality. Test slow-to-fast first.",
    "Brim（裙边）可以增加模型与热床的附着力，防止翘边。":
        "Brim increases adhesion between the model and the print bed to prevent warping.",
    "你知道吗": "Did you know",
    "未命名": "Untitled",

    # --- Bed shape dialog ---
    "热床形状设置": "Print Bed Shape Settings",
    "热床形状": "Print Bed Shape",
    "矩形": "Rectangular",
    "圆形": "Circular",
    "自定义": "Custom",
    "尺寸": "Dimensions",
    "直径": "Diameter",
    "原点偏移": "Origin Offset",
    "从 STL 导入": "Import from STL",

    # --- Calibration dialog ---
    "校准历史记录": "Calibration History",
    "暂无校准历史记录": "No calibration history yet",
    "完成校准后会在此显示记录": "Records will be shown here after calibration completes",
    "K值: %1": "K value: %1",
    "喷嘴: %1mm": "Nozzle: %1mm",
    "共 %1 条记录": "%1 records in total",
    "清空": "Clear",
    "硬件校准选项": "Hardware Calibration Options",
    "微型激光雷达校准": "LiDAR Calibration",
    "热床调平": "Bed Leveling",
    "振动补偿": "Vibration Compensation",
    "电机降噪": "Motor Noise Reduction",
    "可选": "Optional",
    "编辑扫描范围（起始 / 结束 / 步长），覆盖默认值":
        "Edit scan range (start / end / step), overriding defaults",
    "历史记录": "History",
    "K 值": "K Value",
    "N 值": "N Value",
    "校准结果": "Calibration Result",
    "尚未获得校准结果，请先完成校准步骤":
        "No calibration result yet. Please complete the calibration steps first.",
    "保存到预设": "Save to Preset",
    "保存到历史": "Save to History",
    "耗材预设": "Filament Preset",
    "点击下方按钮切换预设": "Click the button below to switch preset",

    # --- Process option tooltips ---
    "每层的打印高度。较小的值提高表面质量但增加打印时间。":
        "Print height of each layer. Smaller values improve surface quality but increase print time.",
    "首层的打印高度，通常比标准层高更高以确保附着力。":
        "Print height of the first layer, usually higher than standard layer height to ensure adhesion.",
    "挤出线的宽度。默认自动匹配喷嘴直径。":
        "Width of the extruded line. Defaults to match the nozzle diameter automatically.",
    "模型外壁的打印圈数。更多圈数增加壁厚和强度。":
        "Number of wall loops printed. More loops increase wall thickness and strength.",
    "模型顶面的实心层数。": "Number of solid layers on the top of the model.",
    "模型底面的实心层数。": "Number of solid layers on the bottom of the model.",
    "模型内部填充的密度百分比。0% = 空心，100% = 实心。":
        "Infill density percentage inside the model. 0% = hollow, 100% = solid.",
    "内部填充使用的几何图案。不同图案影响强度、速度和耗材用量。":
        "Geometric pattern used for infill. Different patterns affect strength, speed and filament usage.",
    "外壁打印速度。较低速度获得更好的表面质量。":
        "Outer wall print speed. Lower speed yields better surface quality.",
    "内壁打印速度。可以比外壁更快。":
        "Inner wall print speed. Can be faster than the outer wall.",
    "内部填充打印速度。": "Infill print speed.",
    "顶面打印速度。较低速度获得更平滑的顶面。":
        "Top surface print speed. Lower speed yields a smoother top surface.",
    "支撑结构的打印速度。": "Print speed of the support structure.",
    "空走（非打印）移动速度。": "Travel (non-printing) move speed.",
    "首层打印速度。较低速度确保良好的热床附着力。":
        "First layer print speed. Lower speed ensures good bed adhesion.",
    "喷嘴（热端）温度。": "Nozzle (hotend) temperature.",
    "热床温度。确保首层良好附着。":
        "Print bed temperature. Ensure good first-layer adhesion.",
    "启用支撑结构以支撑悬空部分。": "Enable support structures to prop up overhangs.",
    "支撑结构的密度百分比。": "Density percentage of the support structure.",
    "支撑类型：普通/树状/可溶性。树状支撑更省材料。":
        "Support type: normal/tree/soluble. Tree supports save material.",
    "在模型底部边缘添加 Brim 以增加附着力。":
        "Add a brim at the bottom edge of the model to improve adhesion.",
    "Brim 的宽度。": "Width of the brim.",
    "打印冷却风扇速度百分比。": "Print cooling fan speed percentage.",
    "启用回退（retraction）以减少拉丝。":
        "Enable retraction to reduce stringing.",
    "回退距离（毫米）。": "Retraction distance (millimeters).",
    "回退速度（毫米/秒）。": "Retraction speed (millimeters/second).",
    "Z 缝位置策略。控制层之间的可见接缝放在哪里。":
        "Z seam position strategy. Controls where the visible seam between layers is placed.",
    "所有打印移动的最大速度限制。":
        "Maximum speed limit for all printing moves.",
    "熨烫类型：在打印后用热喷嘴熨平表面。":
        "Ironing type: iron the surface with the hot nozzle after printing.",
    "附着力类型：裙边/Brim/底筏。不同的方式确保模型附着在热床上。":
        "Adhesion type: skirt/brim/raft. Different methods ensure the model adheres to the print bed.",

    # --- Quality / shell group labels ---
    "层高与线宽": "Layer Height and Line Width",
    "首层线宽": "First Layer Line Width",
    "壁与壳": "Walls and Shell",
    "壁/填充顺序": "Wall/Infill Order",
    "内壁→填充→外壁": "Inner wall -> Infill -> Outer wall",
    "外壁→填充→内壁": "Outer wall -> Infill -> Inner wall",
    "顶底填充重叠 (%)": "Top/Bottom Infill Overlap (%)",
    "外壁线宽": "Outer Wall Line Width",
    "内壁线宽": "Inner Wall Line Width",
    "壁打印顺序": "Wall Print Order",
    "内壁优先": "Inner Wall First",
    "外壁优先": "Outer Wall First",
    "交替": "Alternate",
    "减少穿越壁": "Reduce Crossing Walls",
    "仅单壁顶层": "Single Wall Top Layer",
    "精确外壁": "Precise Outer Wall",
    "熨烫类型": "Ironing Type",
    "顶面": "Top Surface",
    "全部": "All",
    "表面质量": "Surface Quality",
    "熨烫速度 (%)": "Ironing Speed (%)",
    "检测悬空壁": "Detect Overhang Walls",
    "填充设置": "Infill Settings",
    "填充角度 (°)": "Infill Angle (°)",

    # --- Speed group labels ---
    "壁速度": "Wall Speed",
    "内壁速度 (mm/s)": "Inner Wall Speed (mm/s)",
    "填充速度 (mm/s)": "Infill Speed (mm/s)",
    "填充速度": "Infill Speed",
    "顶面速度 (mm/s)": "Top Surface Speed (mm/s)",
    "支撑速度 (mm/s)": "Support Speed (mm/s)",
    "支撑速度": "Support Speed",
    "空走速度": "Travel Speed",
    "首层速度": "First Layer Speed",
    "桥接速度 (mm/s)": "Bridging Speed (mm/s)",
    "特殊速度": "Special Speed",
    "内部桥接速度 (mm/s)": "Internal Bridging Speed (mm/s)",
    "首层填充速度 (mm/s)": "First Layer Infill Speed (mm/s)",

    # --- Acceleration ---
    "外壁加速度 (mm/s²)": "Outer Wall Acceleration (mm/s²)",
    "加速度": "Acceleration",
    "壁加速度": "Wall Acceleration",
    "内壁加速度 (mm/s²)": "Inner Wall Acceleration (mm/s²)",
    "空走加速度 (mm/s²)": "Travel Acceleration (mm/s²)",
    "空走加速度": "Travel Acceleration",
    "默认加速度 (mm/s²)": "Default Acceleration (mm/s²)",
    "默认加速度": "Default Acceleration",

    # --- Temperature ---
    "喷嘴温度": "Nozzle Temperature",
    "热床温度": "Bed Temperature",
    "腔体温度": "Chamber Temperature",
    "首层喷嘴温度 (°C)": "First Layer Nozzle Temperature (°C)",
    "首层温度": "First Layer Temperature",
    "预热": "Preheat",

    # --- Support group ---
    "基础支撑": "Basic Support",
    "可溶性": "Soluble",
    "仅底板支撑": "Support on Build Plate Only",
    "支撑顶面层数": "Support Top Layers",
    "支撑界面": "Support Interface",
    "支撑底面层数": "Support Bottom Layers",
    "支撑角度 (°)": "Support Angle (°)",
    "支撑参数": "Support Parameters",
    "支撑XY间距 (mm)": "Support XY Spacing (mm)",
    "支撑线宽": "Support Line Width",
    "支撑界面间距": "Support Interface Spacing",
    "支撑膨胀 (mm)": "Support Expansion (mm)",

    # --- Adhesion ---
    "外圈": "Outer Brim",
    "内圈": "Inner Brim",
    "全圈": "Full Brim",
    "裙边": "Skirt",
    "裙边距离 (mm)": "Skirt Distance (mm)",
    "裙边高度 (层)": "Skirt Height (layers)",
    "附着力类型": "Adhesion Type",
    "底筏": "Raft",
    "附着力": "Adhesion",
    "底筏层数": "Raft Layers",

    # --- Cooling ---
    "风扇设置": "Fan Settings",
    "最小风扇 (%)": "Min Fan (%)",
    "悬空风扇 (%)": "Overhang Fan (%)",
    "减速层时间 (s)": "Slow Down Layer Time (s)",
    "降温策略": "Cooling Strategy",
    "关闭风扇层数": "Fan Off Layers",
    "逐层降温": "Layer-by-Layer Cooling",

    # --- Retraction group ---
    "回退设置": "Retraction Settings",
    "回填速度 (mm/s)": "Extra Length on Restart (mm/s)",
    "换料回退 (mm)": "Filament Change Retraction (mm)",
    "换料回退": "Filament Change Retraction",
    "Z 抬升 (mm)": "Z Lift (mm)",
    "换层回退": "Layer Change Retraction",
    "擦拭距离 (mm)": "Wipe Distance (mm)",
    "擦拭": "Wipe",

    # --- Z seam ---
    "Z 缝": "Z Seam",
    "Z 缝角落": "Z Seam Corner",
    "内角": "Inner Corner",
    "外角": "Outer Corner",
    "任意": "Any",
    "最大打印速度 (mm/s)": "Max Print Speed (mm/s)",

    # --- Setup wizard ---
    "首次配置向导": "First-Time Setup Wizard",
    "第 %1/%2 步": "Step %1/%2",
    "欢迎使用 3D 打印切片软件": "Welcome to the 3D Print Slicing Software",
    "我们将引导您完成基本配置，选择您的打印机和耗材，以获得最佳切片体验。":
        "We will guide you through the basic configuration: choose your printer and filament "
        "for the best slicing experience.",
    "选择打印机": "Select Printer",
    "请选择您使用的打印机型号：": "Please select your printer model:",
    "热床类型：": "Bed Type:",
    "光滑 PEI 板": "Smooth PEI Plate",
    "普通 PEI 板": "Standard PEI Plate",
    "PC 热床": "PC Bed",
    "EP 热床": "EP Bed",
    "喷嘴直径:": "Nozzle Diameter:",
    "选择耗材": "Select Filament",
    "请选择您常用的耗材类型：": "Please select your commonly used filament type:",
    "喷嘴温度:": "Nozzle Temperature:",
    "热床温度:": "Bed Temperature:",
    "PLA 是最常用的耗材，易于打印，适合初学者。建议打印时开启风扇冷却。":
        "PLA is the most common filament, easy to print and suitable for beginners. "
        "Enable fan cooling when printing.",
    "ABS 强度高，耐热性好，适合工程零件。打印时建议关闭风扇，防止翘边。":
        "ABS has high strength and good heat resistance, suitable for engineering parts. "
        "Disable the fan when printing to prevent warping.",
    "PETG 兼具强度和韧性，透明度可选，适合功能性零件。":
        "PETG combines strength and toughness, with optional transparency, "
        "suitable for functional parts.",
    "TPU 是柔性耗材，适合打印弹性零件。建议降低打印速度以获得更好质量。":
        "TPU is a flexible filament suitable for printing elastic parts. "
        "Reduce print speed for better quality.",
    "ASA 具有优异的户外耐候性，适合户外使用的零件。打印时建议关闭风扇。":
        "ASA has excellent outdoor weather resistance, suitable for outdoor parts. "
        "Disable the fan when printing.",
    "设置完成!": "Setup complete!",
    "您的基本配置已保存，可以开始使用了。":
        "Your basic configuration has been saved and is ready to use.",
    "打印机:": "Printer:",
    "热床:": "Bed:",
    "耗材:": "Filament:",
    "上一步": "Previous",
    "开始设置": "Start Setup",
    "完成": "Done",
    "下一步": "Next",

    # --- G-code placeholder dialog ---
    "内置占位符（双击项添加到 G-code）：":
        "Built-in placeholders (double-click an item to add it to the G-code):",
    "搜索 G-code 占位符...": "Search G-code placeholders...",
    "（双击添加）": "(double-click to add)",
    "选择占位符": "Select Placeholder",
    "将选中的占位符添加到 G-code": "Add the selected placeholder to the G-code",
    "在此编辑自定义 G-code...": "Edit custom G-code here...",

    # --- G-code placeholders (status panel) ---
    "切片状态": "Slice Status",
    "当前层号 (从 1 开始)": "Current Layer Number (from 1)",
    "当前层 Z 高度 (mm)": "Current Layer Z Height (mm)",
    "模型最大 Z 高度 (mm)": "Model Max Z Height (mm)",
    "当前打印 Z 高度 (mm)": "Current Print Z Height (mm)",
    "当前挤出机 ID": "Current Extruder ID",
    "当前挤出机温度": "Current Extruder Temperature",
    "打印统计": "Print Statistics",
    "耗材类型": "Filament Type",
    "已用耗材长度 (mm)": "Filament Length Used (mm)",
    "已用耗材重量 (g)": "Filament Weight Used (g)",
    "回抽长度 (mm)": "Retraction Length (mm)",
    "预估打印时间": "Estimated Print Time",
    "总层数": "Total Layers",
    "对象信息": "Object Info",
    "当前对象名称": "Current Object Name",
    "当前对象 ID": "Current Object ID",
    "总对象数": "Total Objects",
    "总实例数": "Total Instances",
    "是否启用擦料塔": "Enable Wipe Tower",
    "首层层高 (mm)": "First Layer Height (mm)",
    "层高 (mm)": "Layer Height (mm)",
    "最大打印高度 (mm)": "Max Print Height (mm)",
    "当前对象高度 (mm)": "Current Object Height (mm)",
    "首层打印高度 (mm)": "First Layer Print Height (mm)",
    "温度": "Temperature",
    "首层喷嘴温度": "First Layer Nozzle Temperature",
    "首层热床温度": "First Layer Bed Temperature",
    "封闭室温度": "Enclosure Temperature",
    "时间戳": "Timestamp",
    "当前时间戳 (YYYYMMDD-HHmmss)": "Current Timestamp (YYYYMMDD-HHmmss)",
    "当前日期 (YYYYMMDD)": "Current Date (YYYYMMDD)",
    "当前时间 (HHmmss)": "Current Time (HHmmss)",
    "年份 (YYYY)": "Year (YYYY)",
    "月份 (MM)": "Month (MM)",
    "日期 (DD)": "Day (DD)",
    "小时 (HH)": "Hour (HH)",
    "分钟 (mm)": "Minute (mm)",
    "秒 (ss)": "Second (ss)",

    # --- Lite mode / preview settings ---
    "预设参数": "Preset Parameters",
    "填充模式": "Infill Pattern",
    "填充密度 (%)": "Infill Density (%)",
    "墙体层数": "Wall Loops",
    "顶部层数": "Top Layers",
    "底部层数": "Bottom Layers",
    "是否生成支撑": "Generate Support",
    "打印速度 (mm/s)": "Print Speed (mm/s)",
    "空走速度 (mm/s)": "Travel Speed (mm/s)",
    "裙边宽度 (mm)": "Skirt Width (mm)",
    "喷嘴直径 (mm)": "Nozzle Diameter (mm)",
    "预览模式设置": "Preview Mode Settings",
    "精简预览模式": "Lite Preview Mode",
    "启用精简模式可隐藏内部填充结构，仅显示关键工具路径，显著提升预览响应速度。建议在内存较低的设备上启用。":
        "Enable lite mode to hide the internal infill structure and show only key toolpaths, "
        "significantly improving preview responsiveness. Recommended on low-memory devices.",
    "模式对比": "Mode Comparison",
    "功能": "Feature",
    "完整模式": "Full Mode",
    "精简模式": "Lite Mode",
    "外壳轮廓": "Shell Outline",
    "内部填充": "Internal Infill",
    "支撑结构": "Support Structure",
    "工具路径": "Toolpath",
    "移动路径": "Travel Path",
    "启用精简预览模式": "Enable Lite Preview Mode",
    "可在设置中随时切换": "Can be switched at any time in Settings",
    "文档": "Documentation",
    "不再提示": "Don't show again",
    "确认": "OK",

    # --- Preset bundle export ---
    "导出预设包": "Export Preset Bundle",
    "将当前所有自定义预设导出为可分享的预设包文件。":
        "Export all current custom presets as a shareable preset bundle file.",
    "格式: .zip（含 print/filament/printer 自定义预设）":
        "Format: .zip (containing print/filament/printer custom presets)",
    "选择路径...": "Select Path...",
    "预设包 (*.json)": "Preset Bundle (*.json)",

    # --- Firmware upgrade ---
    "固件升级": "Firmware Upgrade",
    "打印机型号": "Printer Model",
    "序列号": "Serial Number",
    "当前版本": "Current Version",
    "新版本可用: %1": "New version available: %1",
    "最新版本": "Latest Version",
    "更新日志": "Changelog",
    "v%1 更新内容:\n\n1. 优化切片算法，提升打印质量\n2. 修复 AMS 多耗材切换偶尔失败的问题\n3. 新增 timelapse 视频录制优化\n4. 改善 Wi-Fi 连接稳定性\n5. 修复部分情况下热床温度显示异常":
        "v%1 release notes:\n\n"
        "1. Optimized slicing algorithm to improve print quality\n"
        "2. Fixed occasional AMS multi-filament switch failures\n"
        "3. Added timelapse recording optimization\n"
        "4. Improved Wi-Fi connection stability\n"
        "5. Fixed abnormal bed temperature display in some cases",
    "正在升级...": "Upgrading...",
    "升级过程中请勿断开电源或网络连接":
        "Do not disconnect power or network during the upgrade",
    "升级成功！打印机将自动重启。":
        "Upgrade successful! The printer will restart automatically.",
    "升级失败，请检查网络连接后重试。":
        "Upgrade failed. Please check the network connection and retry.",
    "开始升级": "Start Upgrade",
    "重试": "Retry",

    # --- Cloud / account ---
    "%1 台设备": "%1 devices",
    "无绑定设备": "No bound devices",
    "同步中...": "Syncing...",
    "云端设备": "Cloud Devices",
    "解绑": "Unbind",
    "绑定设备": "Bind Device",
    "登录 OWzx 账号": "Sign in to OWzx Account",
    "用户名": "Username",
    "输入用户名": "Enter username",
    "密码": "Password",
    "输入密码": "Enter password",
    "登录": "Sign In",
    "设备名称": "Device Name",
    "例如：K1 Max": "e.g. K1 Max",
    "PIN 码": "PIN Code",
    "设备屏幕上显示的 PIN 码": "PIN code shown on the device screen",
    "绑定": "Bind",
    "每日提示": "Tip of the Day",
    "切片前确保模型已平放在热床上。使用 W/E/R 切换移动/旋转/缩放工具。":
        "Before slicing, make sure the model is flat on the print bed. "
        "Use W/E/R to switch the move/rotate/scale tools.",

    # --- Settings sidebar / printer panel ---
    "版本 2.4.0-dev  |  Qt 6.10  |  OWzx": "Version 2.4.0-dev  |  Qt 6.10  |  OWzx",
    "打印机": "Printer",
    "打印机设置": "Printer Settings",
    "预设已修改（未保存）": "Preset modified (unsaved)",
    "喷嘴": "Nozzle",
    "热床": "Print Bed",
    "编辑打印机预设": "Edit Printer Preset",
    "打印机连接": "Printer Connection",
    "耗材设置": "Filament Settings",
    "编辑耗材预设": "Edit Filament Preset",
    "工艺设置": "Process Settings",
    "全局": "Global",
    "盘": "Plate",
    "编辑工艺预设": "Edit Process Preset",
    "搜索设置...": "Search settings...",
    "质量": "Quality",
    "强度": "Strength",
    "支撑": "Support",
    "其他": "Other",
    "纹理 PEI": "Textured PEI",
    "耗材": "Filament",
    "对象": "Object",
    "图例": "Legend",
    "暂无图例数据": "No legend data",
    "+ 添加": "+ Add",
    "搜索设备...": "Search devices...",
    "打印中": "Printing",
    "空闲": "Idle",
    "离线": "Offline",
    "连接中": "Connecting",
    "未找到匹配的设备": "No matching devices found",
    "网络已连接": "Network connected",
    "网络未连接": "Network not connected",
    "未检测到打印机": "No printer detected",
    "请添加打印机或确保打印机已连接到同一局域网":
        "Please add a printer or make sure the printer is connected to the same LAN",
    "🔍 扫描设备": "🔍 Scan Devices",
    "正在连接...": "Connecting...",
    "正在与打印机建立连接，请稍候":
        "Establishing a connection with the printer, please wait",
    "连接已断开": "Disconnected",
    "打印机连接已断开，请重试或检查网络":
        "Printer connection lost. Please retry or check the network",
    "↻ 重新连接": "↻ Reconnect",
    "未选择设备": "No device selected",
    "在线": "Online",
    "状态": "Status",
    "SD 卡": "SD Card",
    "视频": "Video",
    "请从左侧选择一台设备": "Please select a device from the left",
    "喷头温度": "Tool Temperature",
    "IP 地址": "IP Address",
    "↻ 刷新状态": "↻ Refresh Status",
    "当前任务": "Current Task",
    "未知任务": "Unknown Task",
    "舱室灯": "Chamber Light",
    "工作灯": "Work Light",
    "录像": "Recording",
    "延时摄影": "Timelapse",
    "加热中...": "Heating...",
    "首层打印中": "Printing First Layer",
    "冷却中...": "Cooling...",
    "剩余 %1": "Remaining %1",
    "层 %1": "Layer %1",
    "断开": "Disconnect",
    "暂停": "Pause",
    "继续": "Resume",
    "停止": "Stop",
    "连接方式": "Connection Method",
    "局域网直连": "LAN Direct Connection",
    "信号强度": "Signal Strength",
    "信号优秀": "Excellent Signal",
    "信号良好": "Good Signal",
    "信号较弱": "Weak Signal",
    "无信号": "No Signal",
    "固件版本: 1.0.0": "Firmware Version: 1.0.0",
    "空": "Empty",
    "当前活动": "Current Activity",
    "槽": "Slot",
    "未选择": "Not Selected",
    "SD 卡文件管理": "SD Card File Manager",
    "个文件": "files",
    "已用": "Used",
    "文件名": "File Name",
    "大小": "Size",
    "日期": "Date",
    "摄像头未连接": "Camera not connected",
    "已连接，等待视频流": "Connected, waiting for video stream",
    "视频流传输中，等待首帧...": "Streaming video, waiting for first frame...",
    "连接失败": "Connection failed",
    "停止录像": "Stop Recording",
    "停止延时": "Stop Timelapse",
    "切换": "Toggle",
    "分辨率:": "Resolution:",
    "设备健康监控 (HMS)": "Device Health Monitoring (HMS)",
    "%1 条未读": "%1 unread",
    "暂无告警": "No alerts",
    "严重": "Critical",
    "一般": "General",
    "信息": "Info",
    "喷头": "Tool Head",
    "设备运行正常，暂无告警": "Device is running normally. No alerts.",
    "全部标为已读": "Mark all as read",
    "播放": "Play",
    "网络测试": "Network Test",
    "测试与打印机的网络连接性：": "Test network connectivity with the printer:",
    "局域网发现 (SSDP)": "LAN Discovery (SSDP)",
    "待实现": "Not yet implemented",
    "MQTT 连接": "MQTT Connection",
    "云端连通性": "Cloud Connectivity",
    "DNS 解析": "DNS Resolution",
    "开始测试": "Start Test",
    "暂无通知记录": "No notification records",

    # --- Object list panel ---
    "当前盘": "Current Plate",
    "按盘": "By Plate",
    "按模块": "By Module",
    "已选 %1 个部件": "%1 parts selected",
    "已选 %1 项": "%1 items selected",
    "参数": "Parameters",
    "禁打": "Do Not Print",
    "查看所在平板": "View Plate",
    "设为不参与打印": "Set as Non-printing",
    "设为可打印": "Set as Printable",
    "重命名": "Rename",
    "添加部件": "Add Part",
    "添加负体积": "Add Negative Volume",
    "添加修改器": "Add Modifier",
    "添加支撑屏蔽": "Add Support Blocker",
    "添加支撑增强": "Add Support Enforcer",
    "文字浮雕": "Text Emboss",
    "导入 SVG 浮雕...": "Import SVG Emboss...",
    "从文件导入部件...": "Import Part from File...",
    "添加原始体": "Add Primitive",
    "立方体": "Cube",
    "球体": "Sphere",
    "圆柱体": "Cylinder",
    "圆环": "Torus",
    "转换为": "Convert To",
    "部件": "Part",
    "负体积": "Negative Volume",
    "修改器": "Modifier",
    "支撑屏蔽": "Support Blocker",
    "支撑增强": "Support Enforcer",
    "在参数表中编辑": "Edit in Settings",
    "居中到热床": "Center to Bed",
    "拆分为对象": "Split into Objects",
    "修复网格": "Repair Mesh",
    "导出为 STL...": "Export as STL...",
    "拆分为独立对象": "Split into Separate Objects",
    "设为独立对象": "Set as Separate Object",
    "删除已选对象": "Delete Selected Objects",
    "删除对象": "Delete Object",
    "%1 个对象": "%1 objects",
    "0 个对象": "0 objects",
    "不参与打印": "Not Printed",
    "编辑文字": "Edit Text",
    "编辑 SVG": "Edit SVG",
    "删除已选部件": "Delete Selected Parts",
    "删除部件": "Delete Part",
    "简化模型": "Simplify Model",
    "沿 X 轴": "Along X Axis",
    "沿 Y 轴": "Along Y Axis",
    "沿 Z 轴": "Along Z Axis",
    "拆分": "Split",
    "拆分为部件": "Split into Parts",
    "替换为 STL...": "Replace with STL...",
    "屏蔽": "Blocker",
    "增强": "Enforcer",
    "场景中无对象\n请从顶部菜单导入模型":
        "No objects in the scene\nPlease import a model from the top menu",
    "选择模型文件导入为部件": "Select a model file to import as a part",
    "选择 SVG 文件": "Select SVG File",
    "添加文字浮雕": "Add Text Emboss",
    "输入浮雕文字": "Enter emboss text",
    "请输入文字...": "Please enter text...",
    "选择 STL 文件替换部件": "Select an STL file to replace the part",

    # --- Plugin manager ---
    "插件管理": "Plugin Manager",
    "网络通信插件": "Network Communication Plugin",
    "已安装": "Installed",
    "高级支撑生成器": "Advanced Support Generator",
    "基于树形结构的智能支撑生成": "Smart tree-based support generation",
    "可下载": "Downloadable",
    "基于 AI 模型的切片参数自动优化":
        "Automatic slicing parameter optimization based on an AI model",
    "安装": "Install",
    "卸载": "Uninstall",
    "从插件市场浏览更多插件": "Browse more plugins from the marketplace",
    "通用": "General",
    "开发者": "Developer",

    # --- Preferences ---
    "启动时显示主页": "Show home page on startup",
    "默认页面": "Default Page",
    "主页": "Home",
    "公制 (mm)": "Metric (mm)",
    "英制 (inch)": "Imperial (inch)",
    "用户角色": "User Role",
    "基础": "Basic",
    "专业": "Professional",
    "自动保存": "Auto Save",
    "每": "Every",
    "分钟": "minute(s)",
    "启动时检查更新": "Check for updates on startup",
    "减少动画效果": "Reduce animations",
    "通知设置": "Notification Settings",
    "配置通知的显示方式和自动消失行为。通知将在切片完成、导出等操作时弹出。":
        "Configure how notifications are displayed and auto-dismissed. "
        "Notifications appear on slicing completion, export, etc.",
    "启用通知": "Enable Notifications",
    "显示提示": "Show Tips",
    "显示进度通知": "Show Progress Notifications",
    "关闭后将不再显示切片进度弹窗，切片完成后仍会通知。":
        "Once closed, the slicing progress popup will no longer be shown, "
        "but you will still be notified when slicing completes.",
    "自动消失时间": "Auto-dismiss Time",
    "区域设置": "Region Settings",
    "跟随系统": "Follow System",
    "中国": "China",
    "美国": "United States",
    "欧洲": "Europe",
    "日本": "Japan",

    # --- Shortcuts page ---
    "快捷键绑定": "Shortcut Bindings",
    "以下为当前版本支持的快捷键列表。部分快捷键仅在特定页面生效。":
        "The following is the list of shortcuts supported in the current version. "
        "Some shortcuts only take effect on specific pages.",
    "页面": "Page",
    "打开项目": "Open Project",
    "另存为": "Save As",
    "克隆选中": "Clone Selected",
    "搜索设置": "Search Settings",
    "移动模式": "Move Mode",
    "旋转模式": "Rotate Mode",
    "缩放模式": "Scale Mode",
    "适应视图": "Fit View",
    "俯视": "Top View",
    "准备/预览": "Prepare/Preview",
    "前视": "Front View",
    "右视": "Right View",
    "等轴视": "Isometric View",
    "播放/暂停": "Play/Pause",
    "跳转前100步": "Jump Back 100 Steps",
    "跳转后100步": "Jump Forward 100 Steps",
    "跳到开头": "Jump to Start",
    "跳到结尾": "Jump to End",

    # --- Default printer settings ---
    "默认打印机设置": "Default Printer Settings",
    "配置打印机默认参数。此处设置将作为新项目的初始值。":
        "Configure the default printer parameters. These settings are used as the initial values for new projects.",
    "默认喷嘴直径": "Default Nozzle Diameter",
    "默认热床形状": "Default Bed Shape",
    "切片完成后自动上传": "Auto-upload after slicing",
    "启用后，切片完成后将自动上传 G-code 到连接的打印机（需先在设备页面连接打印机）。":
        "When enabled, the G-code will be automatically uploaded to the connected printer after slicing "
        "(connect the printer on the Device page first).",
    "软件更新": "Software Update",
    "当前版本：2.4.0-dev (Qt6 Edition)": "Current version: 2.4.0-dev (Qt6 Edition)",
    "上游基线：OrcaSlicer main branch": "Upstream baseline: OrcaSlicer main branch",
    "自动检查更新": "Check for Updates Automatically",
    "更新通道": "Update Channel",
    "稳定版": "Stable",
    "测试版": "Beta",
    "开发版": "Development",
    "检查更新": "Check for Updates",
    "当前为 Mock 模式，更新检查功能需要连接更新服务器后启用。":
        "Currently in Mock mode. Update checking requires a connection to the update server.",
    "自动备份项目到云端": "Auto-backup projects to cloud",
    "启用后，项目文件将自动备份到您的云端账户。需要先登录云端账号。":
        "When enabled, project files will be automatically backed up to your cloud account. "
        "Sign in to your cloud account first.",
    "低细节模式": "Low-detail Mode",
    "启用后，3D 视口将降低渲染细节以提升性能。适合模型较多或硬件性能不足时使用。":
        "When enabled, the 3D viewport reduces rendering detail to boost performance. "
        "Suitable when there are many models or hardware performance is limited.",
    "撤销栈上限": "Undo Stack Limit",
    "设置撤销/重做的历史记录上限。值越大可回退的操作越多，但占用更多内存。":
        "Set the history limit for undo/redo. A larger value allows more operations to be undone, "
        "but uses more memory.",
    "开发者选项": "Developer Options",
    "这些选项面向开发者调试使用，普通用户无需更改。":
        "These options are for developer debugging. General users do not need to change them.",
    "开发者模式": "Developer Mode",
    "调试覆盖层": "Debug Overlay",
    "日志级别": "Log Level",
    "详细 G-code": "Detailed G-code",
    "OpenGL 调试上下文": "OpenGL Debug Context",
    "最大日志大小 (MB)": "Max Log Size (MB)",
    "日志文件达到指定大小后将自动轮转。增大此值可保留更多历史日志，但占用更多磁盘空间。":
        "Log files rotate automatically when they reach the specified size. Increasing this value "
        "retains more log history but uses more disk space.",

    # --- Prepare page primitives / context menu ---
    "添加模型...": "Add Model...",
    "添加图元": "Add Primitive",
    "圆锥体": "Cone",
    "截锥体": "Frustum",
    "圆环体": "Torus",
    "圆盘": "Disk",
    "隐藏标签": "Hide Labels",
    "显示标签": "Show Labels",
    "复制选中": "Copy Selected",
    "铺满热床": "Fill Bed",
    "导出为 STL": "Export as STL",
    "拆分对象": "Split Object",
    "沿 X 轴镜像": "Mirror Along X Axis",
    "沿 Y 轴镜像": "Mirror Along Y Axis",
    "沿 Z 轴镜像": "Mirror Along Z Axis",
    "显示/隐藏": "Show/Hide",
    "修复模型": "Repair Model",
    "网格布尔运算": "Mesh Boolean",
    "编辑参数表": "Edit Settings Table",
    "编辑工艺设置": "Edit Process Settings",
    "更换耗材": "Change Filament",
    "组合": "Group",
    "重命名对象": "Rename Object",
    "输入新名称:": "Enter new name:",
    "对象名称": "Object Name",
    "选择全部对象": "Select All Objects",
    "清空平板": "Clear Plate",
    "排列对象": "Arrange Objects",
    "平板设置": "Plate Settings",
    "解锁平板": "Unlock Plate",
    "锁定平板": "Lock Plate",
    "设为不打印": "Set as Non-printing",
    "设置打印状态失败": "Failed to set print status",
    "操作失败": "Operation failed",
    "克隆平板": "Clone Plate",
    "克隆平板失败：可能已达到最大平板数（36）":
        "Clone plate failed: maximum plate count (36) may have been reached",
    "克隆失败": "Clone failed",
    "左移平板": "Move Plate Left",
    "移动平板失败": "Failed to move plate",
    "右移平板": "Move Plate Right",
    "全部重新加载": "Reload All",
    "删除平板": "Delete Plate",
    "重命名平板": "Rename Plate",
    "平板名称": "Plate Name",
    "热床类型": "Bed Type",
    "跟随全局": "Follow Global",
    "光滑 PEI": "Smooth PEI",
    "高温 PEI": "High-temp PEI",
    "环氧树脂板": "Epoxy Board",
    "打印顺序": "Print Order",
    "按层打印": "Print by Layer",
    "按对象打印": "Print by Object",
    "螺旋花瓶": "Spiral Vase",
    "开启": "On",
    "首层耗材顺序": "First Layer Filament Order",
    "模式": "Mode",
    "挤出机顺序（拖拽调整）": "Extruder Order (drag to adjust)",
    "其他层耗材顺序": "Other Layer Filament Order",
    "层范围序列（从第 2 层起，自动排序）":
        "Layer Range Sequence (auto-sorted from layer 2 onward)",
    "起始层": "Start Layer",
    "结束层": "End Layer",
    "+ 添加层范围": "+ Add Layer Range",
    "排列设置": "Arrange Settings",
    "对象间距": "Object Spacing",
    "mm (0=自动)": "mm (0=auto)",
    "自动旋转": "Auto Rotate",
    "对齐 Y 轴": "Align Y Axis",
    "允许多耗材": "Allow Multi-filament",
    "避免校准区域": "Avoid Calibration Area",
    "重置默认": "Reset to Default",

    # --- File filters / model menu ---
    "打开模型文件": "Open Model File",
    "所有文件 (*)": "All Files (*)",
    "导出 G-code": "Export G-code",
    "G-code 文件 (*.gcode)": "G-code Files (*.gcode)",
    "替换为 STL": "Replace with STL",
    "松开以导入模型": "Release to import model",
    "缝线绘制": "Seam Painting",
    "SLA 空洞标记": "SLA Hole Marking",
    "MMU 分段": "MMU Segmentation",
    "钻孔": "Hole",
    "高级切割": "Advanced Cut",
    "面检测": "Face Detection",
    "文字工具": "Text Tool",
    "SVG 导入": "SVG Import",
    "SLA 支撑": "SLA Support",
    "已强制: %1": "Enforced: %1",
    "已阻止: %1": "Blocked: %1",
    "强制": "Enforce",
    "阻止": "Block",
    "光标:": "Cursor:",

    # --- Measure gizmo ---
    "点测量": "Point Measure",
    "特征测量": "Feature Measure",
    "体积: ": "Volume: ",
    "角度: ": "Angle: ",
    "垂直距离: ": "Perpendicular Distance: ",
    "直线距离: ": "Direct Distance: ",
    "点击网格面拾取特征 (点/边/圆/平面)":
        "Click a mesh face to pick a feature (point/edge/circle/plane)",
    "点测量模式 — 显示选中对象尺寸":
        "Point measure mode - show the dimensions of the selected object",
    "悬停特征: 无": "Hover feature: None",
    "悬停特征: 点": "Hover feature: Point",
    "悬停特征: 边": "Hover feature: Edge",
    "悬停特征: 圆": "Hover feature: Circle",
    "悬停特征: 平面": "Hover feature: Plane",
    "悬停特征: 未知": "Hover feature: Unknown",
    "Shift = 点测量 (对齐上游 GLGizmoMeasure)":
        "Shift = Point Measure (aligned with upstream GLGizmoMeasure)",
    "平放至面": "Flatten to Face",
    "候选面: ": "Candidate faces: ",
    "将选中对象最大面朝下平放":
        "Place the selected object's largest face down",

    # --- Cut gizmo ---
    "切割对象": "Cut Object",
    "X 轴": "X Axis",
    "Y 轴": "Y Axis",
    "Z 轴": "Z Axis",
    "平面切割": "Planar Cut",
    "舌槽模式": "Tongue and Groove Mode",
    "类型:": "Type:",
    "样式:": "Style:",
    "形状:": "Shape:",
    "尺寸:": "Size:",
    "深度:": "Depth:",
    "位置:": "Position:",
    "全部保留": "Keep All",
    "保留上半": "Keep Upper",
    "保留下半": "Keep Lower",
    "翻转": "Flip",
    "居中": "Center",
    "执行切割": "Perform Cut",
    "强制缝线": "Enforce Seam",
    "阻止缝线": "Block Seam",
    "半径:": "Radius:",
    "启用空洞化:": "Enable Hollowing:",
    "钻孔半径:": "Hole Radius:",
    "钻孔高度:": "Hole Height:",
    "偏移:": "Offset:",
    "删除选中 (%1)": "Delete Selected (%1)",
    "模型简化": "Simplify Model",
    "当前面数:": "Current Faces:",
    "目标面数:": "Target Faces:",
    "最大误差:": "Max Error:",
    "执行简化": "Perform Simplification",
    "当前耗材: %1": "Current Filament: %1",
    "清除分段": "Clear Segmentation",
    "形状": "Shape",
    "支撑绘制": "Support Painting",
    "三角形": "Triangle",
    "方形": "Square",
    "方向": "Direction",
    "法线": "Normal",
    "平行平台": "Parallel to Plate",
    "垂直屏幕": "Perpendicular to Screen",
    "执行钻孔": "Perform Hole",
    "文本": "Text",
    "执行浮雕": "Perform Emboss",
    "运算类型": "Operation Type",
    "并集 (Union)": "Union",
    "差集 (Difference)": "Difference",
    "交集 (Intersection)": "Intersection",
    "需选中 2 个以上对象": "Two or more objects must be selected",
    "执行运算": "Perform Operation",
    "切割轴": "Cut Axis",
    "保留两侧": "Keep Both Sides",
    "仅上半部": "Upper Half Only",
    "角度阈值": "Angle Threshold",
    "检测与 Z 轴平行的平面": "Detect Planes Parallel to Z Axis",
    "执行检测": "Perform Detection",
    "文本内容": "Text Content",
    "字号": "Font Size",
    "添加文字": "Add Text",
    "文件路径": "File Path",
    "选择 SVG 文件...": "Select SVG File...",
    "导入 SVG": "Import SVG",
    "点击模型表面添加支撑点": "Click the model surface to add support points",
    "右键删除单个支撑": "Right-click to delete a single support",
    "（需 SLA 切片配置）": "(requires SLA slice config)",

    # --- Object count / preview ---
    "%1 对象": "%1 objects",
    "顶": "Top",
    "前": "Front",
    "右": "Right",
    "等轴": "Isometric",
    "时间": "Time",
    "请先切片或载入 G-code": "Please slice or load G-code first",
    "分析": "Analyze",
    "行 %1 / %2": "Line %1 / %2",
    "行 -- / --": "Line -- / --",
    "发送打印": "Send to Print",

    # --- Print host dialog ---
    "打印主机设置": "Print Host Settings",
    "预设名称": "Preset Name",
    "主机类型": "Host Type",
    "主机地址": "Host Address",
    "认证方式": "Authentication",
    "用户名/密码": "Username/Password",
    "无认证": "No Authentication",
    "测试连接": "Test Connection",
    "需要有效的网络连接": "A valid network connection is required",

    # --- Project export/import ---
    "3MF 项目 (*.3mf)": "3MF Project (*.3mf)",
    "模型 (*.stl *.3mf *.obj *.amf *.step *.stp)":
        "Models (*.stl *.3mf *.obj *.amf *.step *.stp)",
    "导出模型": "Export Model",
    "填充": "Infill",
    "速度": "Speed",
    "底座": "Adhesion",
    "冷却": "Cooling",
    "回退": "Retraction",
    "输出": "Output",
    "打印空间": "Print Volume",
    "运动能力": "Motion Capability",
    "多材料": "Multi-material",
    "选择目标打印机发送 G-code：": "Select a target printer to send the G-code:",
    "● 在线": "● Online",
    "● 离线": "● Offline",
    "IP: 未知": "IP: Unknown",
    "（未选择）": "(not selected)",
    "材料设置": "Material Settings",
    "基础信息": "Basic Info",
    "打印机G-code": "Printer G-code",
    "移动能力": "Mobility",
    "注释": "Comment",
    "耗材丝": "Filament",
    "参数覆盖": "Parameter Override",
    "依赖": "Dependency",
    "底板": "Build Plate",
    "预设已修改": "Preset Modified",
    "预设不兼容": "Preset Incompatible",
    "保存": "Save",
    "高级模式": "Advanced Mode",

    # --- Slice result / stats ---
    "所有平板已切片完成": "All plates sliced",
    "平板切片状态": "Plate Slice Status",
    "切片结果": "Slice Result",
    "模型尺寸": "Model Size",
    "预估时间:": "Estimated Time:",
    "平均打印速度": "Average Print Speed",
    "当前平板": "Current Plate",
    "切片层数": "Slice Layers",
    " 层": " layers",
    "耗材重量": "Filament Weight",
    "耗材用量": "Filament Usage",
    "预估成本": "Estimated Cost",
    "耗材用量明细": "Filament Usage Details",
    "输出文件": "Output File",
    "全部切片": "Slice All",
    "速度与加速度限制": "Speed and Acceleration Limits",
    "重量": "Weight",
    "范围最小": "Min Range",
    "范围最大": "Max Range",
    "速度限制": "Speed Limit",
    "加速度限制": "Acceleration Limit",
    "统计": "Statistics",
    "标准": "Standard",
    "静音": "Silent",
    "显示空驶": "Show Travel",
    "显示热床": "Show Bed",
    "显示工具位置": "Show Tool Position",
    "总时间": "Total Time",
    "层数": "Layer Count",
    "挤出移动": "Extrusion Moves",
    "空驶移动": "Travel Moves",
    "耗材长度": "Filament Length",
    "平均速度": "Average Speed",
    "工具切换": "Tool Changes",
    "预计成本": "Estimated Cost",
    "按类型耗时": "Time by Type",
    "层时间分布": "Layer Time Distribution",
    "最短": "Shortest",
    "平均": "Average",
    "最长": "Longest",
    "挤出机 %1": "Extruder %1",
    "对象: ": "Object: ",
    "速度 ": "Speed ",
    "层 ": "Layer ",
    "风扇 ": "Fan ",
    "温度 ": "Temperature ",
    "线宽 ": "Line Width ",
    "加速度 ": "Acceleration ",
    "层耗时 ": "Layer Time ",
    "挤出机 ": "Extruder ",

    # --- Device troubleshooting ---
    "设备排错": "Device Troubleshooting",
    "设备连接问题排查（按顺序检查）：":
        "Device connection troubleshooting (check in order):",
    "检查设备电源": "Check device power",
    "确保打印机已开机且启动完成":
        "Make sure the printer is powered on and has finished booting",
    "检查网络连接": "Check network connection",
    "确保打印机和电脑在同一局域网":
        "Make sure the printer and computer are on the same LAN",
    "检查 IP 地址": "Check IP address",
    "在打印机设置中查看 IP，确认可 ping 通":
        "View the IP in printer settings and confirm it responds to ping",
    "检查访问码": "Check access code",
    "Bambu 打印机需在设置中启用局域网访问码":
        "Bambu printers require the LAN access code to be enabled in settings",
    "防火墙设置": "Firewall Settings",
    "确保防火墙未阻止 MQTT(8883)/lan 通信":
        "Make sure the firewall does not block MQTT(8883)/LAN communication",
    "固件版本": "Firmware Version",
    "确保打印机固件为最新版本": "Make sure the printer firmware is up to date",

    # --- Unsaved changes / preset modified ---
    "未保存的修改": "Unsaved Changes",
    "当前预设已修改但未保存。切换前请选择如何处理这些修改：":
        "The current preset has been modified but not saved. Choose how to handle these changes before switching:",
    "已修改 %1 个参数": "%1 parameters modified",
    "丢弃修改": "Discard Changes",
    "保存为预设...": "Save as Preset...",
    "线型可见性": "Line Type Visibility",

    # --- Wipe tower dialog ---
    "擦料塔设置": "Wipe Tower Settings",
    "耗材1": "Filament 1",
    "耗材2": "Filament 2",
    "耗材3": "Filament 3",
    "耗材4": "Filament 4",
    "简单": "Simple",
    "撞击设置": "Bump Settings",
    "线宽倍率": "Line Width Ratio",
    "步进倍率": "Step Ratio",
    "擦洗设置": "Scrub Settings",
    "耗材%1": "Filament %1",
    "擦洗倍率": "Scrub Ratio",
    "最小体积": "Min Volume",
    "计算": "Calculate",

    # --- Project save / new ---
    "项目文件 (*.3mf *.cxprj *.json)": "Project Files (*.3mf *.cxprj *.json)",
    "项目另存为": "Save Project As",
    "项目文件 (*.3mf *.cxprj)": "Project Files (*.3mf *.cxprj)",
    "项目元数据 (*.json)": "Project Metadata (*.json)",
    "切片当前平板": "Slice Current Plate",
    "切片全部平板": "Slice All Plates",
    "快捷键一览": "Shortcut Overview",
    "导出全部平板 G-code": "Export All Plate G-code",
    "剪切选中": "Cut Selected",
    "克隆选中 (备)": "Clone Selected (Alt)",
    "播放/暂停预览动画": "Play/Pause Preview Animation",
    "测量工具": "Measure Tool",
    "导入模型": "Import Model",
    "预览步进 ±100": "Preview Step ±100",
    "预览跳到头/尾": "Preview Jump to Start/End",
    "层范围 ±1 层": "Layer Range ±1 Layer",
    "层范围 ±10 层": "Layer Range ±10 Layers",
    "预设视角 顶/右/等轴": "Preset View Top/Right/Isometric",
    "预设视角 前视": "Preset View Front",
    "基于 OrcaSlicer 开源版本": "Based on the open-source OrcaSlicer",
    "Qt 6.10 + QML 重写迁移版": "Qt 6.10 + QML rewrite migration edition",
    "上游基线: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557":
        "Upstream baseline: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557",
    "将创建新项目，当前未保存的更改将丢失。\n是否继续？":
        "A new project will be created and current unsaved changes will be lost.\nContinue?",
    "编辑自定义 G-code (%1)": "Edit Custom G-code (%1)",
}


# ---------------------------------------------------------------------------
# Non-CJK sources: translation == source (already English). Built at runtime
# from the authoritative list extracted from en.ts so it never drifts.
# ---------------------------------------------------------------------------
def has_cjk(s):
    return any('\u4e00' <= ch <= '\u9fff' or '\u3400' <= ch <= '\u4dbf' for ch in s)


def build_noncjk_keys_from_file(content):
    """Extract the set of non-CJK unfinished source strings directly from .ts text."""
    msg_pat = re.compile(r'<message>(.*?)</message>', re.DOTALL)
    src_pat = re.compile(r'<source>(.*?)</source>', re.DOTALL)
    trans_pat = re.compile(r'<translation([^>]*)>(.*?)</translation>', re.DOTALL)
    keys = set()
    for m in msg_pat.finditer(content):
        block = m.group(1)
        sm = src_pat.search(block)
        tm = trans_pat.search(block)
        if not sm or not tm:
            continue
        if 'type="unfinished"' in tm.group(1):
            src = sm.group(1)
            if not has_cjk(src):
                keys.add(src)
    return keys


def main():
    if not os.path.isfile(EN_TS):
        print('ERROR: %s not found' % EN_TS, file=sys.stderr)
        return 2

    # Read with utf-8; newline='' keeps CRLF bytes intact in the string.
    with open(EN_TS, 'r', encoding='utf-8', newline='') as f:
        content = f.read()

    unfinished_before = content.count('type="unfinished"')
    print('Unfinished before:', unfinished_before)

    # Build full translation map: manual CJK + non-CJK (source==translation).
    translations = dict(CJK_TRANSLATIONS)
    noncjk_keys = build_noncjk_keys_from_file(content)
    for k in noncjk_keys:
        if k not in translations:        # never override a manual entry
            translations[k] = k
    print('Dictionary size:', len(translations), '(CJK:', len(CJK_TRANSLATIONS),
          'non-CJK:', len(noncjk_keys), ')')

    # Regex: match <source>...</source> ... <translation type="unfinished">...</translation>
    # STRICTLY within the same <message> block. Both the source capture AND the gap forbid
    # `<source>`, `</source>` and `</message>`, so the match cannot backtrack across message
    # boundaries (each message has exactly one <source> + one <translation>).
    pattern = re.compile(
        r'(<source>((?:(?!<source>|</message>).)*?)</source>'
        r'(?:(?!<source>|</source>|</message>).)*?<translation\s+)'
        r'type="unfinished"[^>]*>([^<]*)</translation>',
        re.DOTALL,
    )

    residuals = []   # sources we could not translate
    translated_count = 0

    def lookup(source):
        """Look up a translation, tolerating CRLF vs LF differences for multi-line sources."""
        eng = translations.get(source)
        if eng is not None:
            return eng
        if '\r' in source:
            eng = translations.get(source.replace('\r\n', '\n').replace('\r', '\n'))
        return eng

    def repl(m):
        nonlocal translated_count
        prefix = m.group(1)          # '<source>...</source> ... <translation '
        source = m.group(2)          # raw (escaped) source text
        eng = lookup(source)
        if eng is None:
            residuals.append(source)
            return m.group(0)        # leave untouched
        translated_count += 1
        # XML-escape the translation text (it is plain English; source is already
        # escaped in the file). Escape & < > only.
        eng_escaped = saxutils.escape(eng)
        return '%s>%s</translation>' % (prefix, eng_escaped)

    new_content = pattern.sub(repl, content)

    unfinished_after = new_content.count('type="unfinished"')
    print('Translated entries:', translated_count)
    print('Unfinished after:', unfinished_after)
    print('Residual sources:', len(residuals))
    if residuals:
        print('--- Residual unique sources ---')
        seen = set()
        for r in residuals:
            if r in seen:
                continue
            seen.add(r)
            print(repr(r))

    # Write back, preserving the original newline bytes (newline='' keeps \r\n).
    with open(EN_TS, 'w', encoding='utf-8', newline='') as f:
        f.write(new_content)
    print('Wrote', EN_TS)

    # Validate XML.
    try:
        import xml.etree.ElementTree as ET
        ET.parse(EN_TS)
        print('XML validation: OK')
    except Exception as e:
        print('XML validation FAILED:', e, file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
