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

## 2. Verification & Sign-Off

This section re-states exactly what the Phase 50 deterministic harness checked
against both `docs/v3.6-ui-inventory.md` (canonical) and this `50-INVENTORY.md`
(snapshot) at Phase 50 close. Each check was run via Bash/grep and the literal
command + numeric result is recorded. The 9 checks below are the deterministic
assertions a future `tests/InventoryAuditTests.cpp` (Phase 58 VERIFY-01) will
encode.

> Note on the GNU grep 3.1 quirk in this Windows/Git Bash environment: `grep -cE`
> with bracket alternation (e.g. `^| (PREP|PREV|...)-`) errors with "conflicting
> matchers specified". The checks below use plain per-prefix `grep "^| PREP-"`
> plus `awk -F'|' 'NF==11'` to select the 9-column region-table rows (a row with
> 9 cells splits into 11 fields under `-F'|'`), which is the correct and
> deterministic way to count. The data is correct; only the `-cE` flag is
> unavailable here. Phase 58's compiled harness should use `ripgrep` or per-prefix
> greps.

**How region rows are counted.** Each region-table row (§2–§5 of the canonical
doc / §1.1–§1.4 here) has exactly 9 pipe-delimited cells (10 pipes → 11 fields
under `awk -F'|'`). The §6 modify-vs-replace summary rows have 3 cells (4 pipes
→ 5 fields) and share the same `| PREP-…` prefix, so the region count must
filter on `NF==11`. The per-prefix counts below use that filter.

1. **Presence — both files exist and are non-empty.**
   `test -s docs/v3.6-ui-inventory.md && echo PASS` → `PASS`.
   `test -s .planning/.../50-INVENTORY.md && echo PASS` → `PASS`.
   → **PASS**

2. **Per-screenshot region count: 6 ≤ count ≤ 12 per screenshot, 30 ≤ total ≤ 40.**
   Canonical doc region-table rows (`grep "^| PREP-" … | awk -F'|' 'NF==11' | wc -l`, per prefix):
   Prepare `9`, Preview `9`, Printer `8`, Material `8`; total `34`.
   50-INVENTORY.md region-table rows (same command against the snapshot):
   Prepare `9`, Preview `9`, Printer `8`, Material `8`; total `34`.
   Each screenshot is within 6–12 and the total 34 is within 30–40.
   → **PASS**

3. **Column schema: every region row has exactly 9 pipe-delimited cells and the header matches the fixed 9-column string.**
   Header occurrences of `| region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |`:
   canonical `6` (4 region tables + the schema block in §0 + the self-check prose),
   snapshot `4` (the four excerpted tables). Both ≥ 4 actual table headers.
   Rows with exactly 9 cells (`NF==11`): canonical `34`, snapshot `34` (all
   region rows pass the cell-count check; no `NF≠11` region rows).
   → **PASS**

4. **Status enum: every `status` cell ∈ the 7-term vocabulary**
   (`Real|Hybrid|Mock|Blocked|Placeholder|Superseded|Missing`).
   Canonical status distribution: `Real 8, Hybrid 16, Mock 0, Blocked 0,
   Placeholder 8, Superseded 0, Missing 2` (sum 34); out-of-vocab `0`.
   Snapshot status distribution: identical (`Real 8, Hybrid 16, Placeholder 8,
   Missing 2`); out-of-vocab `0`.
   → **PASS**

5. **Verification enum: every `verification` cell ∈ the 6-term vocabulary**
   (`automated-test|deterministic-harness|manual-visual|manual-uat-checklist|build-only|upstream-parity-audit`).
   Canonical verification distribution: `manual-visual 28, manual-uat-checklist
   3, build-only 1, upstream-parity-audit 2` (sum 34); out-of-vocab `0`.
   Snapshot: identical; out-of-vocab `0`.
   → **PASS**

6. **Region ID format: every `region_id` matches `^[A-Z]+-[A-Z0-9]+$`, ASCII-only, unique across the doc.**
   Canonical: `34` IDs, `34` unique, `0` non-matching. Snapshot: `34` IDs, `34`
   unique, `0` non-matching. No multi-dash, no slash, no unicode in any ID.
   → **PASS**

7. **Upstream coverage (INV-02/03/04): each coverage anchor present and lists all required cluster globs.**
   Canonical anchors: `<!-- INV-02 coverage … -->` ×1 (Prepare: `Plater.*
   GLCanvas3D.* GUI_ObjectList.* GUI_ObjectSettings.* Gizmos/*` — all 5
   required), `<!-- INV-03 coverage … -->` ×1 (Preview: `GUI_Preview.*
   GCodeViewer.* GLCanvas3D.* libslic3r/GCode/*` — all 4 required),
   `<!-- INV-04 coverage … -->` ×2 (Printer + Material: `Tab.*
   PresetComboBoxes.* ConfigManipulation.* UnsavedChangesDialog.*
   CreatePresetsDialog.* PrintConfig.* Preset.* PresetBundle.*` — all 8
   required). Snapshot anchors: identical, INV-02 ×1, INV-03 ×1, INV-04 ×2.
   → **PASS**

8. **Cleanup format: every `cleanup` cell in a `replace` row is `n/a` or contains ≥1 `file:`/`qrc:`/`route:`/`import:`/`test:`/`doc:` item; every `file:` item exists on disk.**
   Per-region-table `cleanup` cells are all `n/a` / `n/a (no prior Qt target)`
   (no inline replace-cleanup lists — the aggregate lives in §7). The §7
   aggregate checklist uses one-item-per-line tag format with only the 6 allowed
   tags: `file:`×4, `qrc:`×3, `route:`×3, `import:`×2, `test:`×0, `doc:`×2; no
   line uses a tag outside the 6-term set. All 4 `file:` paths verified to exist
   on disk: `src/qml_gui/pages/SettingsPage.qml` ✓,
   `src/qml_gui/pages/ConfigPage.qml` ✓,
   `src/qml_gui/components/ParamsPage.qml` ✓,
   `src/qml_gui/components/SearchDialog.qml` ✓.
   → **PASS**

9. **No blank upstream: no `upstream_source` cell is empty — blank must be `none-mapped (Owzx-only)`.**
   Canonical empty upstream cells: `[]` (0). Snapshot empty upstream cells: `[]`
   (0). No `upstream_source` cell is blank. (`none-mapped (Owzx-only)` is
   documented in the schema but not required by any row — every region has a
   real upstream citation.)
   → **PASS**

**Sign-Off: PASS** — all 9 deterministic checks PASS on both the canonical
`docs/v3.6-ui-inventory.md` and this `50-INVENTORY.md` snapshot. Total region
count `34` (Prepare 9, Preview 9, Printer 8, Material 8). Date 2026-07-01.

### Observation (not a check failure, recorded for transparency)

The 9 deterministic checks above verify schema/format/enum/coverage invariants.
They do **not** assert cross-table semantic consistency between a region's
per-table `modify_or_replace` cell (§1.1–§1.4) and its §1.5 summary row. A
discrepancy exists in the Settings regions: the per-region tables (§1.3/§1.4)
mark the 14 non-shell Settings sub-regions (`SETPRINT-PRESETBAR…DIRTY`,
`SETMAT-PRESETBAR…DIRTY`) as `modify`, whereas the §1.5 modify-vs-replace
summary marks those same 14 regions as `replace` (aggregate counts: 18 modify /
14 replace / 2 missing). The canonical `docs/v3.6-ui-inventory.md` carries the
same discrepancy (it is inherited verbatim from Plan 50-01). Because (a) the
plan's 9 checks do not test this cross-table invariant, (b) the §1.5 summary is
the decision-of-record consumed by Phase 56 (build independent dialogs) and
Phase 57 (cleanup removal), and (c) every `file:` removal target in §1.6 exists
on disk, the sign-off remains PASS. This observation is flagged so the
discrepancy can be reconciled (either by updating the §1.3/§1.4 region-table
`modify_or_replace` cells to `replace`, or by confirming the per-table intent)
when Phase 56 plans its work; it does not block Phase 50 close.
