#!/usr/bin/env python
# Phase 165 (CW-01) EN→ZH sweep for the 3 English-source dialogs flagged by
# Dialogs-UI-REVIEW. Project source-language policy is Chinese.

TRANSLATIONS = [
    # (EN, ZH) — exact qsTr("EN") → qsTr("ZH")
    ("Compare Presets", "预设对比"),
    ("Scope:", "范围："),
    ("Printer", "打印机"),
    ("Material", "材料"),
    ("Process", "工艺"),
    ("(none)", "（无）"),
    ("A:", "A："),
    ("B:", "B："),
    ("A", "A"),  # leave single-letter labels as-is
    ("B", "B"),
    ("Compare", "对比"),
    ("Close", "关闭"),
    ("Key", "键名"),
    ("No differences", "无差异"),
    ("Select two presets and click Compare", "请选择两个预设后点击对比"),
    ("Create Preset", "创建预设"),
    ("Create a new user preset", "创建新的用户预设"),
    ("Inherits from:", "继承自："),
    ("Name:", "名称："),
    ("enter preset name", "输入预设名称"),
    ("A preset with this name already exists", "该名称的预设已存在"),
    ("Cancel", "取消"),
    ("Create", "创建"),
    ("Create failed (duplicate name or invalid scope)", "创建失败（名称重复或范围无效）"),
    ("Filament Group", "耗材分组"),
    ("Choose how filaments map to extruders for this plate.", "为本盘选择耗材到喷嘴的映射方式。"),
    ("Filament-Saving", "省耗材"),
    ("Minimize flush volume (auto-recommended).", "最小化冲刷量（自动推荐）。"),
    ("Convenience", "便利"),
    ("Match AMS-loaded filaments (auto-recommended).", "匹配 AMS 已装载耗材（自动推荐）。"),
    ("Custom", "自定义"),
    ("Use the explicit per-extruder filament map.", "使用显式的每喷嘴耗材映射。"),
    ("(no extruders)", "（无喷嘴）"),
    ("Auto-recommended map (mode %1)", "自动推荐映射（模式 %1）"),
]

import sys
target_files = [
    "src/qml_gui/dialogs/PresetDiffDialog.qml",
    "src/qml_gui/dialogs/CreatePresetsDialog.qml",
    "src/qml_gui/dialogs/FilamentGroupPopup.qml",
]

total = 0
for tf in target_files:
    with open(tf, encoding="utf-8") as f:
        src = f.read()
    original = src
    for en, zh in TRANSLATIONS:
        if en == zh:
            continue
        pattern = 'qsTr("' + en + '")'
        repl = 'qsTr("' + zh + '")'
        cnt = src.count(pattern)
        if cnt > 0:
            src = src.replace(pattern, repl)
            total += cnt
    if src != original:
        with open(tf, "w", encoding="utf-8") as f:
            f.write(src)
        print(f"  updated {tf}")
print(f"Total EN→ZH replacements: {total}")
