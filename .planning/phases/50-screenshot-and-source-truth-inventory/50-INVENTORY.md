# Phase 50 Inventory Contract — Screenshot & Source-Truth Sign-Off

**Frozen:** 2026-07-01

This is the frozen Phase 50 snapshot of `docs/v3.6-ui-inventory.md`. The
canonical doc is the live copy mutated by Phases 51–57 (statuses update as work
lands); this file is the verified, frozen snapshot at Phase 50 close, embedding
the same greppable region tables plus an explicit Verification & Sign-Off
section and a full INV-01..05 traceability table. Region IDs are immutable.

> Snapshot. Region IDs are immutable. Statuses mutate in the canonical doc
> (`docs/v3.6-ui-inventory.md`) as Phases 51–57 land; this frozen copy reflects
> the Phase 50 close state.

## 1. Inventory excerpts

Excerpts of the four region tables, the §6 modify-vs-replace summary, and the
§7 aggregate cleanup checklist from `docs/v3.6-ui-inventory.md`. The 9-column
header rows and the cleanup-tag lines are byte-identical to the canonical doc so
the same greps pass against this file.

### 1.1 Prepare page (准备页.png)

| region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |
|---|---|---|---|---|---|---|---|---|
| PREP-TOP | Top shell / menu bar (顶部菜单栏) | file menu, save/undo/redo, page tabs (准备/预览), more-menu, window controls | src/qml_gui/main.qml + src/qml_gui/components/Toolbar.qml | Plater.cpp MainFrame::* + GUI_App.cpp | Hybrid | manual-visual | modify | n/a |
| PREP-SIDEBAR | Left preset/settings sidebar (左侧预设栏) | Printer preset combo, Filament slots, Process preset combo, expand/gear buttons | src/qml_gui/panels/LeftSidebar.qml + FilamentPanel.qml + FilamentSlot.qml | Plater.cpp Sidebar::* + PresetComboBoxes.cpp | Hybrid | manual-visual | modify | n/a |
| PREP-OBJLIST | Object list panel (对象列表) | imported-object rows, add/select/delete/rename context actions | src/qml_gui/panels/ObjectList.qml | GUI_ObjectList.cpp + GUI_ObjectSettings.cpp | Hybrid | manual-uat-checklist | modify | n/a |
| PREP-VIEWPORT | 3D viewport / bed (3D 视口/热床) | bed grid, imported model, camera scene, plate | src/qml_gui/Renderer/GLViewport + PrepareSceneData + GLViewportRenderer | GLCanvas3D.cpp + Plater.cpp PartPlate::* | Real | manual-visual | modify | n/a |
| PREP-VTOOLBAR | Vertical GL tool buttons (竖排工具栏) | move/rotate/scale/cut/place/support/seam/paint gizmo entry icons | src/qml_gui/components/GLToolbars.qml + EditorViewModel gizmo state | GLCanvas3D.cpp GLToolbar + Gizmos/GLGizmoBase.cpp + Gizmos/GLGizmoMove3D.cpp | Hybrid | manual-visual | modify | n/a |
| PREP-GIZMOFLOAT | Gizmo floating control panel (Gizmo 浮动面板) | conditional gizmo input window (cut connectors, seam/support brush size, text emboss) | inline Rectangle panels in src/qml_gui/pages/PreparePage.qml | Gizmos/GLGizmoAdvancedCut.cpp on_render_input_window + Gizmos/GLGizmoEmboss.cpp + Gizmos/GLGizmoFdmSupports.cpp | Placeholder | manual-visual | modify | n/a |
| PREP-PLATEBAR | Plate / multi-plate strip (平板栏) | plate thumbnail strip, plate selector, add/select plate | plate bar in src/qml_gui/pages/PreparePage.qml + PartPlateList model | Plater.cpp PartPlateList::* + GLCanvas3D.cpp | Hybrid | manual-visual | modify | n/a |
| PREP-SLICESTATUS | Slice progress / status (切片状态) | slice progress bar, export/slice status text, status bar footer | src/qml_gui/panels/SliceProgress.qml + src/qml_gui/components/StatusBar.qml | Plater.cpp BackgroundSlicingProcess::* + SliceProgressNotification | Real | build-only | modify | n/a |
| PREP-VIEWOPTS | Arrange settings popup / view-orientation controls (排列/视角控制) | camera preset buttons (top/front/right/iso), arrange popup, view toolbar | CxPopup arrangeSettingsPopup + view buttons in src/qml_gui/pages/PreparePage.qml | GLCanvas3D.cpp view toolbar + Plater.cpp ArrangeSettings::* | Hybrid | manual-visual | modify | n/a |

<!-- INV-02 coverage: Plater.* GLCanvas3D.* GUI_ObjectList.* GUI_ObjectSettings.* Gizmos/* -->

### 1.2 Preview page (预览页.png)

| region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |
|---|---|---|---|---|---|---|---|---|
| PREV-TOP | Preview top shell / header bar (预览顶栏) | "预览模式" label, view-mode combo, camera preset buttons (顶/前/右/等轴), time/progress chip, layer/move summary chip | src/qml_gui/pages/PreviewPage.qml header Rectangle | GUI_Preview.cpp Preview::* + GCodeViewer.cpp | Hybrid | manual-visual | modify | n/a |
| PREV-LEFT | Left state/slider panel (左侧状态/层滑块面板) | vertical layer slider + plate thumbnail/summary placeholder | src/qml_gui/pages/PreviewPage.qml left Rectangle + src/qml_gui/components/LayerSlider.qml | GUI_Preview.cpp IMSlider + Preview left bar | Placeholder | manual-visual | modify | n/a |
| PREV-VIEWPORT | G-code viewport (G-code 视口) | center GL canvas, layer/move range binding, tool marker, color mode | src/qml_gui/Renderer/GLViewport (CanvasPreview) + src/qml_gui/Renderer/GCodeRenderer + PreviewViewModel | GCodeViewer.cpp + GLCanvas3D.cpp + libslic3r/GCode/GCodeProcessor.cpp | Real | manual-visual | modify | n/a |
| PREV-VSLIDER | Vertical layer slider (竖向层滑块) | layer range thumb, min/max layer handles, current-layer indicator | src/qml_gui/components/LayerSlider.qml | GUI_Preview.cpp IMSlider layer side + GCodeViewer.cpp | Real | manual-visual | modify | n/a |
| PREV-MSLIDER | Bottom horizontal move slider (底部移动滑块) | move-range thumb, play/pause, step, current-move indicator | src/qml_gui/components/MoveSlider.qml | GUI_Preview.cpp IMSlider move side + GCodeViewer.cpp | Real | manual-visual | modify | n/a |
| PREV-RIGHT | Right legend + statistics panel (右侧图例/统计) | stats panel, collapsible legend, right panel container | src/qml_gui/pages/PreviewPage.qml right Rectangle | GCodeViewer.cpp Legend::* + GCodeViewer.cpp statistics | Hybrid | manual-visual | modify | n/a |
| PREV-LEGEND | Color legend (图例) | collapsible color/role legend rows, legend toggle | src/qml_gui/components/Legend.qml | GCodeViewer.cpp Legend::* (m_legend_enabled) | Real | manual-visual | modify | n/a |
| PREV-STATS | Statistics panel (统计面板) | print time, filament usage, layer count stats | src/qml_gui/components/StatsPanel.qml | GCodeViewer.cpp statistics + libslic3r/GCode/GCodeProcessor.cpp | Real | manual-visual | modify | n/a |
| PREV-TOOLTIP | Tool position tooltip (工具位置提示) | bottom-left floating tool X/Y/Z tooltip | src/qml_gui/components/ToolPositionTooltip.qml | GCodeViewer.cpp Marker::render | Real | manual-visual | modify | n/a |

<!-- INV-03 coverage: GUI_Preview.* GCodeViewer.* GLCanvas3D.* libslic3r/GCode/* -->

### 1.3 Printer settings (打印机参数设置页.png)

| region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |
|---|---|---|---|---|---|---|---|---|
| SETPRINT-SHELL | Dialog shell / window chrome (对话框外壳) | dialog title bar, close button, overall window frame (dark theme) | Missing (no independent printer dialog Qt target; current surface is embedded src/qml_gui/pages/SettingsPage.qml) | Tab.cpp Tab::* dialog frame + GUI dialog chrome | Missing | upstream-parity-audit | missing | n/a (no prior Qt target) |
| SETPRINT-PRESETBAR | Preset selector bar (顶部预设栏) | current preset combo, dirty/save indicator, save/save-as/reset actions, compatibility flag | src/qml_gui/pages/SettingsPage.qml preset RowLayout + src/qml_gui/dialogs/SavePresetDialog.qml | PresetComboBoxes.cpp + Tab.cpp preset line + Preset.cpp | Hybrid | manual-visual | modify | n/a |
| SETPRINT-TABS | Top category tabs (顶部类别标签) | row of tabs (Machine / Extruder printer-scope tabs) | src/qml_gui/pages/SettingsPage.qml tierBa (Print/Filament/Printer) | Tab.cpp m_tabpanel page switching | Hybrid | manual-visual | modify | n/a |
| SETPRINT-GROUPNAV | Left option-group navigation (左侧选项组导航) | left column option groups (General, Build Volume, Capabilities, Custom G-code, etc.) | src/qml_gui/components/ParamsPage.qml category list | Tab.cpp OptionsGroup + Page side nav | Placeholder | manual-visual | modify | n/a |
| SETPRINT-OPTIONS | Main option editing area (主参数编辑区) | center scroll area: checkboxes, number fields, combos, spin boxes with units (mm, %, °C, s) | src/qml_gui/components/ParamsPage.qml + src/qml_gui/Models/ConfigOptionModel.h | ConfigManipulation.cpp + libslic3r/PrintConfig.cpp option keys | Hybrid | manual-uat-checklist | modify | n/a |
| SETPRINT-SEARCH | Search field + basic/advanced toggle (搜索/进阶切换) | search box + Advanced toggle above options | src/qml_gui/components/SearchDialog.qml | Tab.cpp search + m_mode (Basic/Advanced) | Placeholder | manual-visual | modify | n/a |
| SETPRINT-FOOTER | Footer action bar (底部操作栏) | bottom Save / Discard / Cancel buttons | src/qml_gui/pages/SettingsPage.qml footer actions + src/qml_gui/dialogs/UnsavedChangesDialog.qml | Tab.cpp dialog footer + UnsavedChangesDialog.cpp | Placeholder | manual-visual | modify | n/a |
| SETPRINT-DIRTY | Dirty/compat/warning indicators (脏状态/兼容指示) | inline modified-option markers, inheritance/source indicators, compatibility badges | src/qml_gui/pages/SettingsPage.qml isPresetDirty label | Tab.cpp dirty markers + Preset.cpp compatibility | Hybrid | manual-visual | modify | n/a |

<!-- INV-04 coverage: Tab.* PresetComboBoxes.* ConfigManipulation.* UnsavedChangesDialog.* CreatePresetsDialog.* PrintConfig.* Preset.* PresetBundle.* -->

### 1.4 Material settings (材料参数设置页.png)

| region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |
|---|---|---|---|---|---|---|---|---|
| SETMAT-SHELL | Dialog shell (对话框外壳) | dialog title bar, close button, window frame (dark theme) | Missing (no independent material dialog Qt target; current surface is embedded src/qml_gui/pages/SettingsPage.qml) | Tab.cpp Tab::* dialog frame + GUI dialog chrome | Missing | upstream-parity-audit | missing | n/a (no prior Qt target) |
| SETMAT-PRESETBAR | Preset selector bar (顶部预设栏) | material preset combo, dirty/save indicator, save/save-as/reset actions | src/qml_gui/pages/SettingsPage.qml preset RowLayout + src/qml_gui/dialogs/SavePresetDialog.qml | PresetComboBoxes.cpp (Filament) + PresetBundle.cpp | Hybrid | manual-visual | modify | n/a |
| SETMAT-TABS | Top category tabs (顶部类别标签) | row of tabs: Filament / Temperature / Cooling / Retraction / Advanced / G-code | src/qml_gui/pages/SettingsPage.qml tierBa (filamentCategories) | Tab.cpp m_tabpanel page switching | Hybrid | manual-visual | modify | n/a |
| SETMAT-GROUPNAV | Left option-group navigation (左侧选项组导航) | left column option groups per visible tab | src/qml_gui/components/ParamsPage.qml category list | Tab.cpp OptionsGroup + Page side nav | Placeholder | manual-visual | modify | n/a |
| SETMAT-OPTIONS | Main option editing area (主参数编辑区) | center area: temperature fields (°C), retraction (mm), flow (%) typed controls | src/qml_gui/components/ParamsPage.qml + src/qml_gui/Models/ConfigOptionModel.h | ConfigManipulation.cpp + libslic3r/PrintConfig.cpp filament keys | Hybrid | manual-uat-checklist | modify | n/a |
| SETMAT-SEARCH | Search + advanced toggle (搜索/进阶切换) | search box + Advanced toggle | src/qml_gui/components/SearchDialog.qml | Tab.cpp search + m_mode (Basic/Advanced) | Placeholder | manual-visual | modify | n/a |
| SETMAT-FOOTER | Footer action bar (底部操作栏) | bottom Save / Discard / Cancel buttons | src/qml_gui/pages/SettingsPage.qml footer actions + src/qml_gui/dialogs/UnsavedChangesDialog.qml | Tab.cpp dialog footer + UnsavedChangesDialog.cpp | Placeholder | manual-visual | modify | n/a |
| SETMAT-DIRTY | Dirty/compat/warning indicators (脏状态/兼容指示) | inline modified-option markers, inheritance/source indicators, compatibility badges | src/qml_gui/pages/SettingsPage.qml isPresetDirty label | Tab.cpp dirty markers + Preset.cpp compatibility | Hybrid | manual-visual | modify | n/a |

<!-- INV-04 coverage: Tab.* PresetComboBoxes.* ConfigManipulation.* UnsavedChangesDialog.* CreatePresetsDialog.* PrintConfig.* Preset.* PresetBundle.* -->

### 1.5 Modify-vs-replace summary

One row per region_id across all four screenshots. Decision rules (research §7):

- `modify` — the existing Qt file is structurally aligned to upstream and only
  needs behavior/parity completion. `cleanup` = `n/a`.
- `replace` — the existing Qt file is materially off-design or too simplified
  per the REQUIREMENTS Scope Contract; the old files/routes/resources/
  registrations/imports/tests must be removed (listed in §7).
- `missing` — paired with status `Missing`; no Qt target exists yet, so there
  is nothing to remove. `cleanup` = `n/a (no prior Qt target)`.

Pre-flagged candidates confirmed: SettingsPage/ConfigPage embedding = `replace`
(SETTINGS-01 rejects off-design Project/Settings embedding); Printer/Material
independent dialogs = `missing`; Top shell (BBLTopbar/main.qml) = `modify`;
Prepare left sidebar = `modify`; Preview page = `modify`.

| region_id | modify_or_replace | rationale |
|---|---|---|
| PREP-TOP | modify | Shell chrome (main.qml + Toolbar.qml) is structurally present; top toolbar acceptance already covered. Parity completion only. |
| PREP-SIDEBAR | modify | LeftSidebar.qml has Printer/Filament/Objects collapsible sections aligned to upstream Sidebar; parity work, not replacement. |
| PREP-OBJLIST | modify | ObjectList.qml maps to upstream GUI_ObjectList; structurally present, parity completion. |
| PREP-VIEWPORT | modify | GLViewport + PrepareSceneData + GLViewportRenderer render the bed/model; Real, parity completion. |
| PREP-VTOOLBAR | modify | GLToolbars.qml + EditorViewModel gizmo state align to upstream GLToolbar/Gizmos; parity completion. |
| PREP-GIZMOFLOAT | modify | Gizmo floating panels exist inline in PreparePage.qml; behavior wiring (Placeholder) to complete, structure kept. |
| PREP-PLATEBAR | modify | Plate bar + PartPlateList model exist; parity completion. |
| PREP-SLICESTATUS | modify | SliceProgress.qml + StatusBar.qml are Real; parity completion. |
| PREP-VIEWOPTS | modify | Arrange popup + view buttons exist; parity completion. |
| PREV-TOP | modify | PreviewPage.qml header matches screenshot layout; parity completion. |
| PREV-LEFT | modify | Left panel + LayerSlider exist; plate-thumbnail placeholder to complete within existing structure. |
| PREV-VIEWPORT | modify | GLViewport/GCodeRenderer are Real; parity completion. |
| PREV-VSLIDER | modify | LayerSlider.qml is Real; parity completion. |
| PREV-MSLIDER | modify | MoveSlider.qml is Real; parity completion. |
| PREV-RIGHT | modify | Right panel container matches screenshot; parity completion. |
| PREV-LEGEND | modify | Legend.qml is Real; parity completion. |
| PREV-STATS | modify | StatsPanel.qml is Real; parity completion. |
| PREV-TOOLTIP | modify | ToolPositionTooltip.qml is Real; parity completion. |
| SETPRINT-SHELL | missing | No independent printer dialog Qt target exists; current surface is the off-design embedded SettingsPage/ConfigPage (replaced, see replace rows). Build new dialog. |
| SETPRINT-PRESETBAR | replace | Preset bar lives in the embedded SettingsPage.qml, which is materially off-design (SETTINGS-01 wants independent dialogs); old embedding removed, rebuilt in the new dialog. |
| SETPRINT-TABS | replace | Tier tab bar in embedded SettingsPage.qml is off-design; replaced within the new printer dialog. |
| SETPRINT-GROUPNAV | replace | Group navigation in embedded ParamsPage.qml is off-design; replaced within the new printer dialog. |
| SETPRINT-OPTIONS | replace | Option editing in embedded ParamsPage.qml + ConfigOptionModel is off-design; replaced within the new printer dialog. |
| SETPRINT-SEARCH | replace | Search in embedded SearchDialog.qml is off-design; replaced within the new printer dialog. |
| SETPRINT-FOOTER | replace | Footer in embedded SettingsPage.qml/UnsavedChangesDialog is off-design; replaced within the new printer dialog. |
| SETPRINT-DIRTY | replace | Dirty markers in embedded SettingsPage.qml are off-design; replaced within the new printer dialog. |
| SETMAT-SHELL | missing | No independent material dialog Qt target exists; build new dialog. |
| SETMAT-PRESETBAR | replace | Preset bar in embedded SettingsPage.qml off-design; replaced within the new material dialog. |
| SETMAT-TABS | replace | Tab bar in embedded SettingsPage.qml off-design; replaced within the new material dialog. |
| SETMAT-GROUPNAV | replace | Group navigation in embedded ParamsPage.qml off-design; replaced within the new material dialog. |
| SETMAT-OPTIONS | replace | Option editing in embedded ParamsPage.qml off-design; replaced within the new material dialog. |
| SETMAT-SEARCH | replace | Search in embedded SearchDialog.qml off-design; replaced within the new material dialog. |
| SETMAT-FOOTER | replace | Footer in embedded SettingsPage.qml off-design; replaced within the new material dialog. |
| SETMAT-DIRTY | replace | Dirty markers in embedded SettingsPage.qml off-design; replaced within the new material dialog. |

Aggregate counts:

- # modify: 18
- # replace: 14
- # missing-target: 2

### 1.6 Aggregate cleanup checklist

Concatenated, deduplicated cleanup items across ALL `replace` decisions (§6),
grouped by removal category, one item per line, machine-grep format with
verbatim paths. Only genuine removal targets for the Settings replacement are
listed (the off-design embedded SettingsPage/ConfigPage surface and its
routes/resources/imports, plus the stale doc path rewrites). Every `file:`
path below exists on disk today (verified at authoring time).

```text
file:    src/qml_gui/pages/SettingsPage.qml
file:    src/qml_gui/pages/ConfigPage.qml
file:    src/qml_gui/components/ParamsPage.qml
file:    src/qml_gui/components/SearchDialog.qml
qrc:     pages/ConfigPage.qml
qrc:     pages/SettingsPage.qml
qrc:     components/ParamsPage.qml
route:   BackendContext::canLeaveSettingsPage()
route:   BackendContext::requestConfigPageExitIfNeeded()
route:   ConfigViewModel::requestLeaveSettingsPage()
import:  components/ParamsPage.qml
import:  components/SearchDialog.qml
doc:     docs/源码真值功能矩阵.md  (rewrite third_party/CrealityPrint/ paths to third_party/OrcaSlicer/)
doc:     docs/源码真值基线.md  (rewrite third_party/CrealityPrint/ paths to third_party/OrcaSlicer/)
```
