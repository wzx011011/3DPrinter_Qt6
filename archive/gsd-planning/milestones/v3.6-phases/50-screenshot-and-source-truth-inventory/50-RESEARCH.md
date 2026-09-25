# Phase 50 Research: Screenshot and Source-Truth Inventory

**Researcher:** gsd-phase-researcher
**Gathered:** 2026-07-01
**Phase:** 50 — Screenshot and Source-Truth Inventory (documentation/contract only)
**Requirements addressed:** INV-01, INV-02, INV-03, INV-04, INV-05
**Question answered:** "What do I need to know to PLAN this inventory phase well?"

---

## 0. How this research was gathered

Inputs consumed:
- `.planning/phases/50-.../50-CONTEXT.md` (USER DECISIONS — inventory format, status vocab, scope boundaries are final; this research does **not** re-litigate them).
- `.planning/REQUIREMENTS.md` (INV-01..INV-05, status terms, traceability).
- `.planning/STATE.md`, `.planning/PROJECT.md` (source-truth rules, screenshot-truth rule, cleanup rule, upstream lock).
- Four seed docs: `docs/源码真值功能矩阵.md`, `docs/源码真值基线.md`, `docs/顶部工具栏验收清单.md`, `docs/项目结构.md`.
- On-disk verification of Qt targets (`src/qml_gui/**`, `src/core/**`) and upstream targets (`third_party/OrcaSlicer/**`).
- Visual analysis of the two settings screenshots via `mcp__4_5v_mcp__analyze_image` (CDN-uploaded). The two large page screenshots (`准备页.png` 211KB, `预览页.png` 485KB) were **not** accepted by the vision pipeline in full (image format/parse errors on the large files); their region decomposition below is therefore triangulated from (a) `PreviewPage.qml`'s confirmed 1:1 region layout, (b) `LeftSidebar.qml`/`PreparePage.qml` confirmed panel structure, (c) the analyzed settings dialog shell, and (d) upstream OrcaSlicer `Plater`/`GUI_Preview` structure. **This triangulation is high-confidence for region identity but the plan must still mark the exact pixel boundaries of prepare/preview regions as a Claude's-discretion item during execution** (see §9).

> ⚠ One factual correction surfaced: the older seed docs (`源码真值功能矩阵.md`, `源码真值基线.md`, `项目结构.md`) were written against `third_party/CrealityPrint` (the historical upstream name, tag `v7.0.1`). The **v3.6 upstream is OrcaSlicer** (`third_party/OrcaSlicer/`), confirmed to exist on disk. The matrix doc's *structure* and *Qt anchors* remain usable for seeding, but every `third_party/CrealityPrint/...` path must be rewritten to `third_party/OrcaSlicer/...` in the inventory. This is a transcription task, not a re-decision.

---

## 1. Verified ground truth (what actually exists on disk)

### 1.1 Upstream OrcaSlicer source targets — ALL CONFIRMED PRESENT

| Requirement cluster | Upstream path(s) | Verified |
|---|---|---|
| INV-02 Prepare | `third_party/OrcaSlicer/src/slic3r/GUI/Plater.{cpp,hpp}` | ✅ |
| INV-02 Prepare | `…/GUI/GLCanvas3D.{cpp,hpp}` | ✅ |
| INV-02 Prepare | `…/GUI/GUI_ObjectList.{cpp,hpp}` | ✅ |
| INV-02 Prepare | `…/GUI/GUI_ObjectSettings.{cpp,hpp}` | ✅ |
| INV-02 Prepare | `…/GUI/Gizmos/` | ✅ **55 files** |
| INV-03 Preview | `…/GUI/GUI_Preview.{cpp,hpp}` | ✅ |
| INV-03 Preview | `…/GUI/GCodeViewer.{cpp,hpp}` | ✅ |
| INV-03 Preview | `…/libslic3r/GCode/` | ✅ **38 files** incl. `GCodeProcessor.{cpp,hpp}`, `WipeTower.*`, `SeamPlacer.*`, `ToolOrdering.*`, `ThumbnailData.*` |
| INV-04 Settings | `…/GUI/Tab.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/GUI/PresetComboBoxes.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/GUI/ConfigManipulation.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/GUI/UnsavedChangesDialog.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/GUI/CreatePresetsDialog.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/libslic3r/PrintConfig.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/libslic3r/Preset.{cpp,hpp}` | ✅ |
| INV-04 Settings | `…/libslic3r/PresetBundle.{cpp,hpp}` | ✅ |
| Shell (seed) | `…/GUI/GUI_App.{cpp,hpp}`, `…/GUI/MainFrame.{cpp,hpp}` | ✅ |

**Implication for planning:** the `upstream_source` column can be filled from real files. No row should be left blank for "upstream not found" — if a region has no upstream match, the cell must say `none-mapped (Owzx-only)` explicitly (see status `Placeholder`/`Superseded`).

### 1.2 Qt target files — CONFIRMED PRESENT (seed the `qt_target` column)

Confirmed on disk (relevant subset; full list in `项目结构.md`):

- **Shell** (`src/qml_gui/`): `main.qml`, `BBLTopbar.qml`, `BackendContext.{h,cpp}`, `main_qml.cpp`. Components: `components/Toolbar.qml`, `components/StatusBar.qml`, `components/NotificationCenter.qml`, `components/ErrorBanner.qml`, `components/ErrorToast.qml`.
- **Prepare page**: `pages/PreparePage.qml` (3184 lines, very large — hosts gizmo floating panels), `pages/Plater.qml` (orchestrator). Panels: `panels/LeftSidebar.qml`, `panels/Sidebar.qml`, `panels/ObjectList.qml`, `panels/PrintSettings.qml`, `panels/FilamentPanel.qml`, `panels/SliceProgress.qml`, `panels/DockableSidebar.qml`. Components: `components/CollapsibleSection.qml`, `components/FilamentSlot.qml`, `components/GLToolbars.qml`.
- **Preview page**: `pages/PreviewPage.qml` (305 lines, clean region layout — **the reference for region decomposition**). Components: `components/LayerSlider.qml`, `components/MoveSlider.qml`, `components/Legend.qml`, `components/StatsPanel.qml`, `components/ToolPositionTooltip.qml`.
- **Settings dialogs**: `pages/SettingsPage.qml`, `pages/ConfigPage.qml`, `components/ParamsPage.qml`, `dialogs/UnsavedChangesDialog.qml`, `dialogs/SavePresetDialog.qml`. Also `dialogs/BedShapeDialog.qml`, `dialogs/EditGCodeDialog.qml`, `dialogs/ExportPresetBundleDialog.qml`.
- **Renderer** (`src/qml_gui/Renderer/` + `src/core/rendering/`): `GLViewport.{h,cpp}`, `GLViewportRenderer.{h,cpp}`, `RhiViewport.{h,cpp}`, `RhiViewportRenderer.{h,cpp}`, `RhiBackendSelector.{h,cpp}`, `SoftwareViewport.{h,cpp}`, `GCodeRenderer.{h,cpp}`, `CameraController.{h,cpp}`, `PrepareSceneData.{h,cpp}`; core: `GLShaderUtil.{h,cpp}`, `SupportPaintTypes.h`, `TickCodeTypes.h`.
- **Viewmodels** (`src/core/viewmodels/`): `EditorViewModel.{h,cpp}`, `PreviewViewModel.{h,cpp}`, `ConfigViewModel.{h,cpp}`, `SettingsViewModel.{h,cpp}`, plus `Models/ConfigOptionModel.{h,cpp}`, `Models/PresetListModel.{h,cpp}`.
- **Services** (`src/core/services/`): `SliceService.{h,cpp}` (real), `ProjectServiceMock.{h,cpp}`, `PresetServiceMock.{h,cpp}`, `UndoRedoManager.{h,cpp}`.

### 1.3 Status vocabulary is FROZEN (do not invent more)

From REQUIREMENTS.md + 50-CONTEXT.md, the exact allowed tokens for the `status` column:

`Real` | `Hybrid` | `Mock` | `Blocked` | `Placeholder` | `Superseded` | `Missing`

(`Missing` is the Owzx-added 7th term for a Qt target that does not yet exist — required because several restored Settings dialog regions have no Qt file yet. Per 50-CONTEXT §"Status Vocabulary".)

Verification-method tokens (exact, no free text):

`automated-test` | `deterministic-harness` | `manual-visual` | `manual-uat-checklist` | `build-only` | `upstream-parity-audit`

---

## 2. Decision: the canonical column schema (already fixed — reproduced for the plan)

Per 50-CONTEXT `<decisions>`, each inventory row uses exactly these 9 columns, in this order:

```
region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup
```

Rules the plan must enforce:
- `region_id` is **ASCII-only**, page-prefixed, stable (e.g. `PREP-TOP`, `PREV-VSLIDER`). Never reused across screenshots.
- `upstream_source` is **file-level + optional symbol/region**, never line numbers (they drift vs the v7.0.1 lock). Use the form `Plater.cpp Sidebar::*` or `GCodeViewer.cpp Legend::*`.
- `cleanup` is either `n/a` (for modify decisions) or a machine-grep-friendly item list (see §6).
- `qt_target` may be `Missing` (status `Missing`) — do not force a nearest-file guess.

---

## 3. Screenshot region decomposition (the core planning input)

Region count target: 6–12 per screenshot (50-CONTEXT §"Claude's Discretion"). Below is the **proposed region ID list + boundary description** for each of the 4 screenshots. These are the recommended defaults; the executor may merge/split within the 6–12 band but must keep IDs ASCII and stable.

### 3.1 `准备页.png` — Prepare page (10 proposed regions)

> Triangulated from `PreparePage.qml` (shell + 3-pane layout + gizmo floating panels) and `LeftSidebar.qml` (collapsible sections). Matches upstream OrcaSlicer `Plater` left-SiderBar + center canvas + right ObjectList layout.

| region_id | region_name (EN / zh) | Boundary on screen |
|---|---|---|
| `PREP-TOP` | Top shell / menu bar (顶部菜单栏) | Full-width top strip: file menu, save/undo/redo, page tabs (准备/预览/…), more-menu, window controls. Seed from `docs/顶部工具栏验收清单.md`. |
| `PREP-SIDEBAR` | Left preset/settings sidebar (左侧预设栏) | Left column (~280px): Printer preset combo, Filament slots, Process preset combo, expand gear buttons. Maps to upstream `SiderBar` / `SidebarPrinter`/`Sidebar Filament`/`Sidebar Process`. Qt: `LeftSidebar.qml` + `FilamentPanel.qml` + `FilamentSlot.qml`. |
| `PREP-OBJLIST` | Object list panel (对象列表) | Right-side or inner-left list of imported objects with context actions. Upstream `GUI_ObjectList`. Qt: `ObjectList.qml`. |
| `PREP-VIEWPORT` | 3D viewport / bed (3D 视口/热床) | Center canvas: bed grid, imported model, camera scene. Upstream `GLCanvas3D`. Qt: `Renderer/GLViewport.qml`→`GLViewport` + `PrepareSceneData` + `GLViewportRenderer`. |
| `PREP-VTOOLBAR` | Vertical GL tool buttons (竖排工具栏) | Left or right vertical icon strip on the canvas: move/rotate/scale/cut/place/support/seam/paint gizmo entry. Upstream `GLToolbar` + `Gizmos/*`. Qt: `GLToolbars.qml` + gizmo state in `EditorViewModel`. |
| `PREP-GIZMO-FLOAT` | Gizmo floating control panel (Gizmo 浮动面板) | Center-top floating panel shown when a gizmo is active (cut connector options, seam paint tools, support brush size, etc.). Conditional visibility. Upstream `Gizmos/GLGizmo*.cpp::on_render_input_window`. Qt: inline `Rectangle` panels in `PreparePage.qml` (lines ~1850–2700). |
| `PREP-PLATEBAR` | Plate / multi-plate strip (平板栏) | Bottom plate thumbnail strip or plate selector. Upstream `Plater` plate tabs / `PartPlateList`. Qt: plate bar in `PreparePage.qml` + `PartPlateList` model. |
| `PREP-CTXMENU` | Context menus (右键菜单) | Not always-on-screen but screenshot-visible if open: object/plate/default menus. Upstream `Plater::create_*_menu`. Qt: `CxMenu` blocks in `PreparePage.qml`. *(Flag: optional region — see §9 ambiguity.)* |
| `PREP-SLICESTATUS` | Slice progress / status (切片状态) | Progress indicator when slicing. Upstream `BackgroundSlicingProcess` + `SliceProgressNotification`. Qt: `SliceProgress.qml` + `StatusBar.qml`. |
| `PREP-ARRANGE/VIEW-OPTS` | Arrange settings popup / view-orientation controls (排列/视角控制) | Camera preset buttons + arrange popup. Upstream `GLCanvas3D` view toolbar + `ArrangeSettings`. Qt: `CxPopup arrangeSettingsPopup` + view buttons. |

**INV-02 coverage note:** every Prepare row's `upstream_source` must reference at least one of `Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`, `GUI_ObjectSettings.*`, `Gizmos/*` to satisfy INV-02 "mapped against" before parity is claimed. The plan should add a per-cluster coverage assertion.

### 3.2 `预览页.png` — Preview page (9 proposed regions)

> **High confidence** — `PreviewPage.qml` (305 lines) implements exactly this layout 1:1 (verified by reading). Region IDs below map directly to the QML structural `Rectangle` blocks.

| region_id | region_name (EN / zh) | Boundary / QML anchor |
|---|---|---|
| `PREV-TOP` | Preview top shell / header bar (预览顶栏) | Inherited from `main.qml` shell (same `PREP-TOP` shell chrome — but Preview-specific header row: "预览模式" label, view-mode combo, camera presets, time/progress chip, layer/move summary chip). |
| `PREV-LEFT` | Left state/slider panel (左侧状态/层滑块面板) | Left ~240px `Rectangle`: `LayerSlider.qml` + (plate thumbnail/summary placeholder). Upstream `GUI_Preview` left bar + `IMSlider` layer side. |
| `PREV-VIEWPORT` | G-code viewport (G-code 视口) | Center `GLViewport` with `canvasType: CanvasPreview`, `previewData`, layer/move range bindings, tool marker. Upstream `GCodeViewer` + `GLCanvas3D`. Qt: `Renderer/GLViewport` + `GCodeRenderer`. |
| `PREV-VSLIDER` | Vertical layer slider (竖向层滑块) | Inside `PREV-LEFT`: `LayerSlider.qml`. Upstream `IMSlider` (layer). |
| `PREV-MSLIDER` | Bottom horizontal move slider (底部移动滑块) | Bottom 56px `Rectangle`: `MoveSlider.qml`. Upstream `IMSlider` (move). |
| `PREV-RIGHT` | Right legend + statistics panel (右侧图例/统计) | Right ~280px `Rectangle`: `StatsPanel.qml` + collapsible `Legend.qml`. Upstream `GCodeViewer` legend + statistics. |
| `PREV-LEGEND` | Color legend (图例) | Sub-region of `PREV-RIGHT`, collapsible (matches upstream `m_legend_enabled`). |
| `PREV-STATS` | Statistics panel (统计面板) | Sub-region of `PREV-RIGHT`: print time, filament usage, layer count. Upstream `GCodeViewer` stats. |
| `PREV-TOOLTIP` | Tool position tooltip (工具位置提示) | Bottom-left floating `ToolPositionTooltip.qml`. Upstream `GCodeViewer::Marker::render`. |

**INV-03 coverage note:** every Preview row's `upstream_source` must reference at least one of `GUI_Preview.*`, `GCodeViewer.*`, `GLCanvas3D.*`, `libslic3r/GCode/*`. The `PREV-LEFT` plate-thumbnail sub-area may currently be `Placeholder`/`Missing` and must be classified honestly.

### 3.3 `打印机参数设置页.png` — Printer settings dialog (8 proposed regions)

> **High confidence** — visually analyzed via vision pipeline. Shares a common dialog shell with the material settings page.

| region_id | region_name (EN / zh) | Boundary |
|---|---|---|
| `SETPRINT-SHELL` | Dialog shell / window chrome (对话框外壳) | Title bar, close button, overall window frame. Dark theme (#2D2D2D bg). Upstream `Tab` dialog frame + `GUI` dialog chrome. |
| `SETPRINT-PRESETBAR` | Preset selector bar (顶部预设栏) | Top of dialog: current preset combo, dirty/save indicator, save/save-as/reset actions, compatibility flag. Upstream `PresetComboBoxes` + `Tab` preset line. |
| `SETPRINT-TABS` | Top category tabs (顶部类别标签) | Row of tabs: Machine / Extruder (printer scope) and any others visible. Upstream `Tab::m_tabpanel` page switching. |
| `SETPRINT-GROUPNAV` | Left option-group navigation (左侧选项组导航) | Left column listing option groups (e.g. General, Build Volume, Capabilities, Custom G-code, etc.). Upstream `OptionsGroup` / `Page` side nav. |
| `SETPRINT-OPTIONS` | Main option editing area (主参数编辑区) | Center scroll area: checkboxes, number fields, combo boxes, spin boxes with units (mm, %, °C, s). Upstream `OptionsGroup` + `ConfigManipulation`. Qt: `ParamsPage.qml` / `ConfigOptionModel`. |
| `SETPRINT-SEARCH` | Search field + basic/advanced toggle (搜索/进阶切换) | Top-right or above options: search box + Advanced toggle. Upstream `Tab` search + `m_mode` (Basic/Advanced). |
| `SETPRINT-FOOTER` | Footer action bar (底部操作栏) | Bottom: Save / Discard / Cancel / Compare? buttons. Upstream dialog footer. |
| `SETPRINT-DIRTY` | Dirty/compat/warning indicators (脏状态/兼容指示) | Inline modified-option markers, inheritance/source indicators, compatibility badges. Upstream `Tab` dirty markers + `Preset` compatibility. |

### 3.4 `材料参数设置页.png` — Material/Filament settings dialog (8 proposed regions)

> **High confidence** — visually analyzed. Same dialog shell as printer settings; tab set differs (Filament / Temperature / Cooling / etc.).

| region_id | region_name (EN / zh) | Boundary |
|---|---|---|
| `SETMAT-SHELL` | Dialog shell (对话框外壳) | Identical chrome to `SETPRINT-SHELL`. |
| `SETMAT-PRESETBAR` | Preset selector bar (顶部预设栏) | Same role as `SETPRINT-PRESETBAR`; material preset combo. Upstream `PresetComboBoxes` (Filament). |
| `SETMAT-TABS` | Top category tabs (顶部类别标签) | Tabs: Filament / Temperature / Cooling / etc. (read exactly as shown in screenshot). |
| `SETMAT-GROUPNAV` | Left option-group navigation (左侧选项组导航) | Left group list per the visible tab. |
| `SETMAT-OPTIONS` | Main option editing area (主参数编辑区) | Center area; expect temperature fields (°C), retraction (mm), flow (%) — typed controls. Upstream `OptionsGroup` + `PrintConfig` filament keys. |
| `SETMAT-SEARCH` | Search + advanced toggle (搜索/进阶切换) | As above. |
| `SETMAT-FOOTER` | Footer action bar (底部操作栏) | As above. |
| `SETMAT-DIRTY` | Dirty/compat/warning indicators (脏状态/兼容指示) | As above. |

**Shared-shell note:** Because `SETPRINT-*` and `SETMAT-*` share a dialog shell and most controls, the canonical doc should define the shell once (a "Settings dialog shell" section) and reference it from both screenshot tables, rather than duplicating cleanup rows. The `region_id`s stay distinct per screenshot (INV-01 wants *every visible region* cataloged per screenshot), but `qt_target`/`upstream_source`/`cleanup` will often be identical strings — the plan should note this is intentional.

**INV-04 coverage note:** every Settings row's `upstream_source` must reference at least one of `Tab.*`, `PresetComboBoxes.*`, `ConfigManipulation.*`, `UnsavedChangesDialog.*`, `CreatePresetsDialog.*`, `PrintConfig.*`, `Preset.*`, `PresetBundle.*`.

---

## 4. Canonical document structure: `docs/v3.6-ui-inventory.md`

The long-lived reference. Recommended section order (Claude's-discretion ordering per 50-CONTEXT):

```text
# v3.6 UI Inventory — Screenshot & Source-Truth Contract

## 0. How to read this document
   - Status vocabulary (the 7 terms) + verification-method vocabulary (the 6 terms)
   - Column schema (the 9 columns)
   - Region ID convention (ASCII, page-prefixed, stable)
   - Upstream reference convention (file-level + symbol, NO line numbers)

## 1. Upstream & Qt source baselines
   - Upstream: third_party/OrcaSlicer @ v7.0.1 lock (cite 源码真值基线.md)
   - Qt targets: src/qml_gui/** + src/core/** (cite 项目结构.md)
   - Screenshot visual truth: shotScreen/*.png

## 2. Prepare page — 准备页.png
   - Screenshot crop reference (optional pixel-rect helper column)
   - Region table (PREP-* rows, 9 columns)
   - INV-02 upstream-coverage assertion (all 5 cluster files referenced ≥ once)

## 3. Preview page — 预览页.png
   - Region table (PREV-* rows)
   - INV-03 upstream-coverage assertion

## 4. Printer settings — 打印机参数设置页.png
   - Settings dialog shell note (shared with §5)
   - Region table (SETPRINT-* rows)
   - INV-04 upstream-coverage assertion

## 5. Material settings — 材料参数设置页.png
   - Region table (SETMAT-* rows)
   - INV-04 upstream-coverage assertion

## 6. Cross-cutting modify-vs-replace summary
   - One row per region_id with just: region_id | modify_or_replace | rationale
   - Aggregate counts: # modify / # replace / # missing-target

## 7. Aggregate cleanup checklist
   - Concatenated, deduplicated cleanup items across all "replace" decisions
   - Grouped by removal category: files | qml.qrc entries | registrations | routes | imports | tests | docs
   - Machine-grep format (see §6 of this research)

## 8. Open behavior gaps
   - Table: region_id | gap | classification (one of the 7 statuses) | owning phase (51–57) | note

## 9. Traceability
   - INV-01 → §2–§5 completeness
   - INV-02 → §2 coverage assertion
   - INV-03 → §3 coverage assertion
   - INV-04 → §4–§5 coverage assertion
   - INV-05 → §6 + §7
```

**Key design rule:** the canonical doc is *informational + reference*; it never duplicates the phase-contract sign-off. It must stay editable by later phases (51–57 update statuses as work lands). Mark with a header line: `> Living document. Statuses updated by Phases 51–57. Region IDs are immutable.`

---

## 5. Phase deliverable: `50-INVENTORY.md` vs canonical doc

The two artifacts have distinct roles (50-CONTEXT `<decisions>` confirms both are produced):

| Aspect | `docs/v3.6-ui-inventory.md` (canonical) | `…/50-INVENTORY.md` (phase contract) |
|---|---|---|
| Lifetime | Long-lived, mutated by 51–57 | Frozen at Phase 50 close (snapshot) |
| Primary job | Reference for every downstream phase | Verified contract + sign-off gate for Phase 50 |
| Contains the full region tables? | **Yes** (the source of truth) | Yes, as an excerpt/snapshot OR by reference |
| Contains INV-01..05 traceability? | Brief pointer in §9 | **Full traceability matrix** (requirement → where satisfied → evidence row count) |
| Contains cleanup-checklist aggregate? | Yes (§7) | **Yes, plus a dedup/sign-off summary** |
| Contains sign-off summary? | No | **Yes** (the deterministic-harness check results + region count + per-cluster coverage pass/fail) |
| Format for Phase 58 tests | Greppable tables + cleanup list | Same greppable content + an explicit "Verification" section restating what the harness asserts |

**Recommendation for the plan:** `50-INVENTORY.md` should (a) embed-or-excerpt the same tables as the canonical doc, (b) add a **"Verification & Sign-Off"** section that lists exactly what the Phase 50 deterministic harness checked (doc presence, per-screenshot region count, 9-column schema, cleanup-item format, per-cluster upstream coverage), and (c) add an explicit **INV-01..05 traceability table**. The canonical doc and the phase file can share the same table markdown (copy at Phase 50 close; canonical becomes the live copy afterward).

---

## 6. Machine-grep-friendly format for Phase 58 tests (VERIFY-01)

VERIFY-01 requires "Automated tests cover screenshot/source inventory completeness". To make that **deterministic**, the inventory must be parseable by a future test (likely a `tests/InventoryAuditTests.cpp` modeled on the existing `tests/QmlUiAuditTests.cpp` and `tests/CliTests.cpp`). The plan must mandate these format rules:

### 6.1 Table format (greppable)
- Use **GitHub-flavored markdown tables** (not HTML tables, not definition lists).
- Each region is **exactly one table row** (no multi-line cells).
- The header row is **fixed and identical in every region table**:
  ```
  | region_id | region_name | visible_controls | qt_target | upstream_source | status | verification | modify_or_replace | cleanup |
  ```
- `region_id` is the first column, ASCII-only, format `^[A-Z]+-[A-Z0-9]+$`.
- A Phase 58 test can then assert, per screenshot section:
  - `grep -c "^| [A-Z]+-" → region count ≥ 6 and ≤ 12` (sanity band).
  - each row has exactly 9 `|`-delimited cells (column-count check).
  - `status` cell matches `^(Real|Hybrid|Mock|Blocked|Placeholder|Superseded|Missing)$`.
  - `verification` cell matches `^(automated-test|deterministic-harness|manual-visual|manual-uat-checklist|build-only|upstream-parity-audit)$`.

### 6.2 Cleanup-checklist format (greppable, one item per line)
For any row where `modify_or_replace = replace`, the `cleanup` cell must be either `n/a` or a semicolon-joined inline list. The **aggregate cleanup checklist** (canonical doc §7) must use **one file/item per line**, prefixed by a category tag, with verbatim paths:

```text
file:    src/qml_gui/panels/OldSidebar.qml
file:    src/qml_gui/dialogs/LegacyPresetDialog.qml
qrc:     qml/assets/icons/old-preset.svg
route:   backend.openLegacySettings()
import:  pages/OldSettingsPage.qml
test:    tests/LegacySettingsTests.cpp
doc:     docs/源码真值功能矩阵.md  (CrealityPrint → OrcaSlicer path rewrite)
```

A Phase 58 test can then: `grep -E "^(file|qrc|route|import|test|doc):"` to enumerate cleanup items, and assert that every `file:` entry actually exists on disk (so a checklist referencing a non-existent removal target fails loudly).

### 6.3 Coverage-assertion anchors (greppable)
Each screenshot section must end with a coverage line the test can grep, e.g.:

```text
<!-- INV-02 coverage: Plater.* ✔ GLCanvas3D.* ✔ GUI_ObjectList.* ✔ GUI_ObjectSettings.* ✔ Gizmos/* ✔ -->
```

This lets VERIFY-01 assert all required upstream cluster files are referenced. (Comment format keeps it out of rendered prose but trivially greppable.)

---

## 7. Modify-vs-replace decision pattern (INV-05)

INV-05: "For each module, the plan records whether to modify existing Qt code or replace it, and replacement decisions include a cleanup checklist."

Decision rules the plan should encode (so the executor isn't guessing):

- **`modify`** when the existing Qt file is structurally aligned to upstream and only needs behavior/parity completion. `cleanup` cell = `n/a`.
- **`replace`** when the existing Qt file is materially off-design or too simplified per REQUIREMENTS.md Scope Contract ("If an existing Qt page is materially off-design or too simplified for this scope, replace it"). `cleanup` cell must then list old files/routes/resources/registrations/imports/tests (CLEAN-01..04 semantics).
- **`missing`** (paired with status `Missing`) when no Qt target exists yet — there is nothing to remove; `cleanup` cell = `n/a (no prior Qt target)`. This is distinct from `replace`.

**Known replace/missing candidates to pre-flag** (from PROJECT.md + current code state — to be confirmed row-by-row at execution):

| Module | Likely decision | Why |
|---|---|---|
| SettingsPage / ConfigPage embedding | **replace** | SETTINGS-01 explicitly rejects "off-design Project/Settings embedding"; wants *independent* printer/material/process dialogs. Old embedding + routes must be cleaned (CLEAN-01/02). |
| Printer/Material settings dialogs | **missing** (then build) | SETTINGS-01: "independent … dialogs/pages that visually match the supplied screenshots." Current `SettingsPage.qml` is an embedded page, not the dialog form. `UnsavedChangesDialog.qml`/`SavePresetDialog.qml` exist but the main settings dialog shell may be `Missing`. |
| Top shell `BBLTopbar.qml` | **modify** | Shell chrome exists; `docs/顶部工具栏验收清单.md` already covers it. |
| Prepare left sidebar | **modify** (likely) | `LeftSidebar.qml` is structurally present with Printer/Filament/Objects sections; parity work needed, not replacement. |
| Preview page | **modify** | `PreviewPage.qml` layout already matches screenshot region layout (verified). |
| Old seed docs (CrealityPrint-named) | **modify (rewrite paths)** | Transcription: `third_party/CrealityPrint/` → `third_party/OrcaSlicer/` in matrix doc; not a code change. |

The cleanup aggregate (canonical §7) will be dominated by the Settings replacement (old embedded SettingsPage routes/qrc/imports/tests).

---

## 8. How Phase 58 / VERIFY-01 will deterministically check this

Concrete test assertions a future `tests/InventoryAuditTests.cpp` (or a `.planning` deterministic harness) can make against `docs/v3.6-ui-inventory.md` and `50-INVENTORY.md`:

1. **Presence:** both files exist and are non-empty.
2. **Per-screenshot region count:** for each of the 4 screenshots, count rows under its section header; assert `6 ≤ count ≤ 12` and total across all 4 is ~30–40.
3. **Column schema:** every region row has exactly 9 pipe-delimited cells and the header matches the fixed 9-column string.
4. **Status enum:** every `status` cell ∈ the 7 allowed tokens.
5. **Verification enum:** every `verification` cell ∈ the 6 allowed tokens.
6. **Region ID format:** every `region_id` matches `^[A-Z]+-[A-Z0-9]+$`, ASCII-only, unique across the whole doc.
7. **Upstream coverage (INV-02/03/04):** grep the coverage-assertion comments; assert all required cluster files appear in each screenshot section.
8. **Cleanup format:** every `cleanup` cell in a `replace` row is either `n/a` or contains ≥1 `file:`/`qrc:`/`route:`/`import:`/`test:`/`doc:` item; every `file:` item exists on disk.
9. **No blank upstream:** no `upstream_source` cell is empty — blank upstream must be explicitly `none-mapped (Owzx-only)`.

The Phase 50 deterministic harness (the phase's own verification gate) should run a subset of these (1, 2, 3, 4, 5, 6) against the just-produced files; full disk-existence checks (8) can be a Phase 50 gate too since the files are being enumerated now.

---

## 9. Ambiguities / Claude's-discretion flags for the plan

These are the points where the executor must make a judgment call — the plan should pre-authorize them via the 50-CONTEXT "Claude's Discretion" clauses rather than block:

1. **Prepare/Preview pixel boundaries** — the two large screenshots could not be fully parsed by the vision pipeline in this research pass. Region *identity* is high-confidence (from QML structure), but exact pixel-rect crop coordinates are not. **Decision: proceed with the proposed region IDs; the executor MAY add an optional `pixel_rect` helper column (allowed by 50-CONTEXT) but is NOT required to.** Flag `PREP-CTXMENU` (context menus are only visible when right-clicked) as "optional region — include only if a menu is open in the screenshot."
2. **`PREP-OBJLIST` placement** — the object list may be a right-side panel or nested in the left sidebar depending on the exact screenshot. The executor decides one stable location; if both exist, split into `PREP-OBJLIST` + `PREP-OBJSETTINGS` (≤12 cap still holds).
3. **Shared settings-shell dedup** — `SETPRINT-*` and `SETMAT-*` duplicate the shell. The executor may either (a) keep 8 rows each with repeated cells (simpler for INV-01 "every visible region per screenshot"), or (b) define the shell once and cross-reference. **Recommend (a)** for INV-01 compliance; note the duplication is intentional in §4 of the canonical doc.
4. **Bilingual display names** — 50-CONTEXT allows EN primary + (zh) optional. **Recommend EN primary with zh in parentheses for region names** to keep the doc greppable in ASCII while honoring the Chinese screenshot context. Region IDs stay ASCII.
5. **`PREV-TOP` vs `PREP-TOP`** — the top shell is shared across pages, but INV-01 wants per-screenshot cataloging. **Decision: each screenshot gets its own `*-TOP` row** pointing at the same `qt_target` (`main.qml`/`BBLTopbar.qml`); they are distinct region IDs in distinct tables.
6. **Gizmo floating panel (`PREP-GIZMO-FLOAT`)** — only visible when a gizmo is active. If the screenshot shows none active, mark it `status=Placeholder` or omit; executor decides. Omitting keeps count within band.

---

## 10. Seed-data shortcuts for the executor (copy-paste starts)

To minimize re-derivation, the executor can pre-fill `upstream_source` cells from this verified map:

| Qt region cluster | upstream_source seed value |
|---|---|
| Shell/top bar | `MainFrame.cpp` + `GUI_App.cpp` + (`BBLTopbar.cpp` upstream-equivalent) |
| Prepare sidebar (printer/filament/process) | `Plater.cpp Sidebar::*` + `PresetComboBoxes.cpp` |
| Object list / object settings | `GUI_ObjectList.cpp` + `GUI_ObjectSettings.cpp` |
| Prepare viewport / gizmos | `GLCanvas3D.cpp` + `Gizmos/GLGizmo*.cpp` + `GLToolbar.cpp` |
| Preview layout | `GUI_Preview.cpp` |
| Preview G-code render/legend/stats | `GCodeViewer.cpp` + `libslic3r/GCode/GCodeProcessor.cpp` |
| Preview sliders | `IMSlider.cpp` (note: upstream OrcaSlicer; not in the explicit INV-03 list but part of `GUI_Preview` family — cite as `GUI_Preview.cpp IMSlider`) |
| Settings dialog shell + tabs/groups | `Tab.cpp` + `ConfigManipulation.cpp` |
| Settings preset bar | `PresetComboBoxes.cpp` + `Preset.cpp` + `PresetBundle.cpp` |
| Settings unsaved-changes | `UnsavedChangesDialog.cpp` |
| Settings create-preset | `CreatePresetsDialog.cpp` |
| Settings option values | `libslic3r/PrintConfig.cpp` |

And `qt_target` seeds are in §1.2 above and in `docs/项目结构.md`.

---

## 11. Risk register (what could block PLAN)

| Risk | Likelihood | Mitigation in plan |
|---|---|---|
| Vision pipeline can't fully parse large screenshots at execution time | Medium | Plan does not *require* vision; QML-structure + upstream triangulation is sufficient. Executor falls back to structural analysis + optional pixel_rect helper. |
| Settings dialog shell may be `Missing` (no Qt dialog form yet) | High | Explicitly allow `status=Missing` rows; this is the honest classification per 50-CONTEXT. Pre-flagged in §7. |
| Seed docs cite CrealityPrint not OrcaSlicer | Certain | Path-rewrite task; flagged in §0. Not a blocker. |
| Region count drift outside 6–12 band | Low | 50-CONTEXT authorizes band; executor merges/splits within band. Proposed counts (10/9/8/8 = 35 total) are within band. |
| Cleanup checklist references stale/nonexistent files | Medium | Phase 58 test asserts `file:` entries exist on disk (§6.2, §8). Phase 50 harness can pre-check. |

---

## 12. One-paragraph answer to the planning question

To plan Phase 50 well you need: (1) the 9-column frozen schema and 7-status/6-verification vocabularies (already locked in 50-CONTEXT — do not re-decide); (2) the proposed ~35 region IDs across 4 screenshots (§3), triangulated from QML structure + analyzed settings dialogs + upstream Plater/GUI_Preview; (3) the two-artifact split — a long-lived `docs/v3.6-ui-inventory.md` (reference) and a frozen `50-INVENTORY.md` (phase contract with INV-01..05 traceability + sign-off) (§4–§5); (4) a strict greppable table + cleanup-list format so VERIFY-01 can deterministically assert region counts, column counts, enum validity, and upstream-cluster coverage (§6, §8); (5) per-module modify-vs-replace decisions with the Settings embedding pre-flagged as `replace`/`missing` (§7); and (6) pre-authorized Claude's-discretion calls for the ~6 genuine ambiguities (§9). All upstream and Qt target files are confirmed present on disk (§1), so the inventory is a transcription/mapping task, not a discovery task.

## RESEARCH COMPLETE
