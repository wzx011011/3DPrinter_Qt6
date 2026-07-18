#!/usr/bin/env python
# Phase 166 follow-up: SavePresetDialog + AccessCodeInputDialog EN→ZH sweep.
TRANSLATIONS = [
    ("Save Preset", "另存为预设"),
    ("Preset type:", "预设类型："),
    ("Preset name:", "预设名称："),
    ("Save", "保存"),
    ("Cancel", "取消"),
    ("Print", "打印"),
    ("Filament", "耗材"),
    ("Printer", "打印机"),
    ("Enter the access code", "请输入访问码"),
    ("Access code", "访问码"),
    ("OK", "确定"),
    ("Confirm", "确认"),
    ("Submit", "提交"),
    ("Apply", "应用"),
    ("Close", "关闭"),
]

import os
target_files = [
    "src/qml_gui/dialogs/SavePresetDialog.qml",
    "src/qml_gui/dialogs/AccessCodeInputDialog.qml",
]

total = 0
for tf in target_files:
    if not os.path.exists(tf): continue
    with open(tf, encoding="utf-8") as f: src = f.read()
    original = src
    for en, zh in TRANSLATIONS:
        if en == zh: continue
        pattern = 'qsTr("' + en + '")'
        repl = 'qsTr("' + zh + '")'
        cnt = src.count(pattern)
        if cnt > 0:
            src = src.replace(pattern, repl)
            total += cnt
    if src != original:
        with open(tf, "w", encoding="utf-8") as f: f.write(src)
        print(f"  updated {tf}")
print(f"Total EN→ZH replacements: {total}")
