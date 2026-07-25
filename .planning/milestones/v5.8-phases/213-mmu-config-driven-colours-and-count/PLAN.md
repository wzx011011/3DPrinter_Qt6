# Phase 213 Plan: MMU Config-Driven Colours and Count

**Requirement:** MMU-01..03
**Goal:** Drive the MMU segmentation gizmo's extruder colours and count from
the active print config (对齐 upstream `m_extruders_colors` / extruders
count) instead of hard-coded values, and show an honest single-material
notice.
**Status:** Complete · **Commit:** `11a6afe`

## Files

- Modify: `src/core/services/ProjectServiceMock.h`, `.cpp` (add
  `plateFilamentColours`, `filamentCount`)
- Modify: `src/core/viewmodels/EditorViewModel.h`, `.cpp` (add
  `mmuExtruderColors` Q_PROPERTY; rebind `mmuExtruderCount`)
- Modify: `src/qml_gui/pages/PreparePage.qml` (swatch colour binding +
  single-material notice)

## Steps

- [x] `ProjectServiceMock::plateFilamentColours()` reads the `filament_colour`
  coStrings option from the current plate's DynamicPrintConfig (mirrors
  upstream `Plater::get_extruders_colors`); fallback `#F2754E` pre-load.
- [x] `ProjectServiceMock::filamentCount()` returns the `filament_colour`
  vector size; fallback 1.
- [x] `EditorViewModel::mmuExtruderColors` Q_PROPERTY forwards to
  `plateFilamentColours`; `mmuExtruderCount` rebinds to `filamentCount`
  (was hard-coded 4).
- [x] PreparePage.qml swatch `color:` binds `mmuExtruderColors[index]` with
  the hard-coded palette kept only as a pre-load fallback.
- [x] When `mmuExtruderCount <= 1`, the swatch Row hides and a
  "需要多耗材打印机配置" notice shows (对齐 upstream MM-03 `is_mm_painted`).

## Verification

- owzx_app_core builds clean.
- `multiPlateFullStateRoundTrip` PASS (3 passed, 0 failed).
- `mmuSegmentationPaintFeedsSlice` PASS (3 passed, 0 failed) — no regression
  in the existing MMU paint path.
