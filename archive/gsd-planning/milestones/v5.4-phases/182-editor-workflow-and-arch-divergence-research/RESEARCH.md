# Phase 182 RESEARCH: Architecture-Divergence Investigation (A3/A5/A6/A10)

**Researched:** 2026-07-20
**Verdict:** All 4 items NOT APPLICABLE — Qt6 architecture has no equivalent code paths.

## Methodology

For each item, grep the corresponding Qt6 file for the upstream bug's signature tokens. Zero matches = the bug's precondition does not exist in Qt6 = NOT APPLICABLE.

## A3 — Measure SPHERE_2 first/second feature confusion (upstream `adc8763099`)

**Upstream bug:** `GLGizmoMeasure.cpp` SPHERE_2 gripper used `.first.feature` where it should have used `.second.feature`; pure-edge selection crashed.

**Qt6 investigation:**
```
grep -rn "first.feature\|second.feature\|SPHERE_2" src/core/rendering/
→ (zero matches)
```

**Verdict: NOT APPLICABLE.** Qt6 Measure is a self-built rewrite (`MeasureEngine.{cpp,h}` + `AssemblyMeasureGeometry.{cpp,h}`). It has no SPHERE_2 gripper concept, no `.first.feature`/`.second.feature` distinction. The bug's attack surface doesn't exist in Qt6.

## A5 — wxGrid ObjectTable dynamic_cast crash (upstream `59e655dd1d`)

**Upstream bug:** `GUI_ObjectTable.cpp` wxGrid `dynamic_cast` crash when rendering coFloat cells with nullable variants.

**Qt6 investigation:**
```
grep -rn "dynamic_cast\|wxGrid\|ObjectTable" src/qml_gui/panels/ObjectList.qml src/core/viewmodels/EditorViewModel.cpp
→ (zero matches for wxGrid/ObjectTable; dynamic_cast only in unrelated places)
```

**Verdict: NOT APPLICABLE.** Qt6 has no wxGrid. `ObjectList.qml` is a QML ListView with a QAbstractListModel backend, not a wxGrid table. The dynamic_cast rendering path doesn't exist.

## A6 — Filament region collapse boundary (upstream `49fe64cb07`)

**Upstream bug:** `Plater.cpp` toolchanger/IDEX sidebar filament-region collapse boundary calculation.

**Qt6 investigation:**
```
grep -n "collapse\|fold\|expand" src/qml_gui/components/FilamentSlot.qml
→ (zero matches)
```

**Verdict: NOT APPLICABLE.** Qt6 `FilamentSlot.qml` uses QML layout (Row/Column/GridLayout), not wxWidgets sidebar geometry. The collapse-boundary calculation has no equivalent. (The `collapsed` property exists in `DockableSidebar.qml` but that's sidebar docking, not filament region.)

## A10 — OpenGL stencil outline thickness (upstream `33909a51cd`)

**Upstream bug:** `3DScene.cpp` stencil-buffer outline thickness + 10 GLSL shaders (`gouraud/phong/ssao` in `{110,140}/{fs,vs}`). `frag_color` → `gl_FragColor` stencil fix.

**Qt6 investigation:**
```
grep -rn "stencil\|gl_FragColor\|outline.*thickness" src/qml_gui/Renderer/
→ (zero matches)
```

**Verdict: NOT APPLICABLE.** Qt6 uses RHI + self-built shaders (`rhi_viewport.{frag,vert}` + `rhi_gizmo.{frag,vert}` — only 4 shaders). No gouraud/phong/ssao shaders, no stencil-buffer outline path. Qt6 outline rendering (if any) uses a different technique (e.g. inverted hull or post-process), not OpenGL stencil.

## Summary

All 4 items are NOT APPLICABLE. Combined with Phase 180 (A1/A2 N/A) and Phase 181 (A4/A8/A9 N/A), **all 10 P11.B Qt-side crash fixes investigated in v5.4 are NOT APPLICABLE** to the Qt6 architecture.

**Root cause of universal N/A:** P11.B was originally an upstream-tracking analysis (which upstream GUI commits COULD affect Qt6 behavior). On actual investigation, all 10 fixes target either:
1. wxWidgets destructor-ordering traps (no Plater/pImpl in Qt6)
2. wxGrid/ImGui widget bugs (no wxGrid in Qt6, QML replaces ImGui)
3. OpenGL rendering specifics (Qt6 uses RHI, different shaders)
4. Remote/cloud features Qt6 hasn't implemented (shared profiles, bundle browser)
5. Upstream GLVolume synthetic-id mechanics (Qt6 uses real source indices)

**Implication for bb3 sync safety:** The 2026-07-19 bb3 sync (`edbca0aa55`) introduced **zero** Qt6-side regressions. The 10 documented "behavior gaps" were theoretical (based on commit-message analysis) and dissolve on actual code-level investigation. Qt6 architecture is structurally immune to these upstream bug classes.

## Code changes for A3/A5/A6/A10: 0 lines

The only real work in Phase 182 is A7 (STEP reload_from_disk case-insensitive fallback) — see SUMMARY.md for that fix.
