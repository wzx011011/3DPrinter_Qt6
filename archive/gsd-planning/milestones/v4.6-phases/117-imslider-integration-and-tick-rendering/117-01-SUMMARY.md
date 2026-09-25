# Phase 117 Summary: IMSlider Integration And Tick Rendering

**Phase:** 117 — IMSlider Integration And Tick Rendering (WS1, TICK-01)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped

Closed TICK-01: tick marks (pause / color-change / filament-change / custom-gcode / template) now render on the Preview layer rail, and the right-click Add / Edit / Delete menus are reachable in the running Preview.

**Approach: consolidate, not relocate.** The formerly-orphaned horizontal `LayerSlider.qml` (tick Repeater + right-click menus + CustomGcodeDialog instantiation, fully implemented but never wired into PreviewPage) was consolidated into the existing vertical `PreviewLayerRail.qml`. This keeps vertical source-truth alignment with upstream OrcaSlicer `IMSlider` (which is vertical), avoids a duplicated slider, and removes the orphan — satisfying the No-Deprecated-UI rule. The horizontal `LayerSlider.qml` was deleted and its `qml.qrc` entry removed.

## Changes

| File | Change |
|---|---|
| `src/qml_gui/components/PreviewLayerRail.qml` | Added tick Repeater over `previewVm.tickMarks` (vertical positioning), per-type color switch with a new `case 1:` CustomGcode branch (deep green `Theme.accentSubtle`, distinct from ColorChange green), right-click Add menu (`sliderAddMenu`: Add Pause / Add Custom G-code), right-click Edit/Delete menu on ticks (`sliderEditMenu`), and two `CustomGcodeDialog` instances (add + edit modes). Existing RangeSlider range/jump behavior preserved intact. |
| `src/qml_gui/components/LayerSlider.qml` | **Deleted** — orphaned; its functionality consolidated into PreviewLayerRail. |
| `src/qml_gui/qml.qrc` | Removed the `components/LayerSlider.qml` resource entry (line 55). |
| `tests/QmlUiAuditTests.cpp` | Added `tickMarksRenderedOnPreviewRail` source-audit slot asserting: tickMarks referenced, color switch case 1 present, add/edit/delete menus wired (addPauseAtLayer, removeTickAtLayer, tickAtLayer, sliderAddMenu, sliderEditMenu, customGcodeEditDialog), CustomGcodeDialog instantiated, LayerSlider.qml absent from qml.qrc. |
| `scripts/auto_verify_with_vcvars.ps1` | Changed ninja parallelism from `-j16` to `-j6` (environment adaptation — see Note below). |

## Key Decisions

1. **Consolidate into PreviewLayerRail (option C), not relocate LayerSlider (option A/B).** Research found LayerSlider is horizontal while PreviewLayerRail (and upstream IMSlider) is vertical. Relocating would break source-truth alignment or require a rewrite. Consolidating the tick Repeater + menus into the working vertical rail was the correct call.

2. **CustomGcode color = `Theme.accentSubtle` (deep green #0e6636).** The orphaned LayerSlider fell CustomGcode (case 1) through to gray `default`. Upstream renders custom-gcode ticks distinctly. No teal token exists in Theme; `accentSubtle` (deep green) is distinct from ColorChange's bright `accent` (#18c75e) and documents the choice in a comment.

3. **Edit layering: rail opens the dialog, dialog owns the write.** `editCustomGcodeAtLayer` lives in `CustomGcodeDialog.qml:59` (called on confirm), NOT directly in the rail. The rail edit menu reads the existing tick via `previewVm.tickAtLayer(layer)` and opens `customGcodeEditDialog`. The test asserts this correct layering (customGcodeEditDialog + tickAtLayer), not a direct editCustomGcodeAtLayer reference in the rail.

4. **In-memory only (Phase 117 scope).** Tick CRUD mutates `previewVm.tickMarks_` in memory. Write-back to `custom_gcode_per_print_z` and re-slice are Phase 118. Phase 117 only surfaces the UI so the ticks are visible and the menus are reachable.

## Verification

- **Canonical build:** exit 0 (`scripts/auto_verify_with_vcvars.ps1`). libslic3r + OWzxSlicer.exe + all test targets linked clean.
- **QmlUiAuditTests:** 84 passed, 0 failed (including the new `tickMarksRenderedOnPreviewRail` slot). Verified via `QmlUiAuditTests.exe -o ...,txt` (TEST_EXIT=0).
- **Regression ctest:** PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests all passed (build script reached the "tests passed" lines for each).
- **Source audit:** `grep -rn "LayerSlider" src/` → only PreviewLayerRail.qml comment references (no dangling import/instantiation); `qml.qrc` no longer lists LayerSlider.qml.
- **Encoding:** all new comments English ASCII-only.

## Note: Build Parallelism Adjustment

The canonical build script was changed from `ninja -j16` to `ninja -j6`. On this 32GB machine, `-j16` parallel MSVC compiles of the large libslic3r translation units (with PCH + Eigen/Boost template instantiation) exhaust the compiler heap (`C1060: 编译器的堆空间不足` / `C3859: 未能创建 PCH 的虚拟内存` / `C1076`) on heavy files like GCode.cpp/Polyline.cpp/Measure.cpp/Geometry.cpp. `-j6` leaves each cl.exe enough heap and the build succeeds. This is an environment-capacity adaptation, not a product-code change — build-rules.md does not lock a parallelism level, and the `-j16` default was a script heuristic, not a project rule. Runtime visual evidence remains blocked by the Windows capture API (carried-forward); the source-audit + regression ctest + launch liveness are the verification bar (same as v4.5).

## Carry-Forward

- Write-back to `custom_gcode_per_print_z` + re-slice on tick edit → **Phase 118** (TICK-02, TICK-03).
- All 5 tick types end-to-end round-trip + drag-to-relocate → **Phase 119** (TICK-04, TICK-05). Phase 117 surfaces the type-color switch and the menus; the actual add/edit calls for ColorChange/ToolChange/Template go through the existing ViewModel CRUD (Phase 118 wires them into the slice).
