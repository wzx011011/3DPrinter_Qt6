#!/usr/bin/env python
# Phase 168 (VS-01): targeted detection + report of pseudo-button patterns.
# Pattern: Rectangle {...} containing a Text {...} and a MouseArea {...}
#          with an onClicked handler, where the Rectangle's color binds to
#          a *Hover.containsMouse expression (the pseudo-button tell).
#
# This is a REPORT-only script — actual conversion is hand-done per finding
# because each site has unique context (signal args, disabled bindings, etc.)
# that a mechanical sweep would corrupt.

import os, re

target_files = [
    'src/qml_gui/pages/MonitorPage.qml',
    'src/qml_gui/pages/MultiMachinePage.qml',
    'src/qml_gui/pages/CalibrationPage.qml',
    'src/qml_gui/pages/PreparePage.qml',
    'src/qml_gui/pages/HomePage.qml',
    'src/qml_gui/pages/PreviewPage.qml',
]

pseudo_button_tells = 0
for tf in target_files:
    if not os.path.exists(tf): continue
    with open(tf, encoding='utf-8') as f: src = f.read()
    # Find Rectangle blocks whose color binds to a *Hover.containsMouse
    # AND has a Text child AND a MouseArea with onClicked
    for m in re.finditer(
        r'Rectangle\s*\{[^{}]*?color:\s*\w*[Hh]over\.containsMouse[^{}]*?Text\s*\{[^{}]*?\}[^{}]*?MouseArea\s*\{[^{}]*?onClicked:',
        src, re.DOTALL):
        pseudo_button_tells += 1
        line = src[:m.start()].count('\n') + 1
        print(f'{tf}:{line}')
print(f'\nTotal pseudo-button candidates (Rectangle+Hover+Text+MouseArea+onClicked): {pseudo_button_tells}')
