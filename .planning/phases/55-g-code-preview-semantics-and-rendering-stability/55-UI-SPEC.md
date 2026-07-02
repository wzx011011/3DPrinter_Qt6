---
phase: 55
slug: g-code-preview-semantics-and-rendering-stability
status: draft
shadcn_initialized: false
preset: none
created: 2026-07-02
---

# Phase 55 — UI Design Contract

> Visual and interaction contract for the G-code Preview Semantics and Rendering Stability phase.
> This is a Qt6/QML project using a custom Theme singleton and Cx* control library (NOT shadcn).

---

## Design System

| Property | Value |
|----------|-------|
| Tool | QML Theme singleton + Cx* controls (CxCheckBox, CxSwitch, CxComboBox, CxScrollView) |
| Preset | Not applicable (Qt6/QML project) |
| Component library | Custom Cx* controls under `src/qml_gui/controls/` |
| Icon library | Inline QML rendering (Canvas, rectangles) — no external icon library |
| Font | System default; monospace `Consolas` for numeric data rows |

---

## Spacing Scale

The Theme singleton defines the project spacing tokens (source: `src/qml_gui/Theme.qml`).

Declared values:

| Token | Theme Property | Value | Usage |
|-------|----------------|-------|-------|
| xs | `Theme.spacingXS` | 4px | Icon gaps, inline padding |
| sm | `Theme.spacingSM` | 6px | Compact element spacing |
| md | `Theme.spacingMD` | 8px | Default element spacing, panel margins |
| lg | `Theme.spacingLG` | 12px | Panel margins (matches PreviewPage anchors.margins) |
| xl | `Theme.spacingXL` | 16px | Layout gaps |
| 2xl | `Theme.spacingXXL` | 24px | Major section breaks |

Exceptions: None. All spacing in Phase 55 UI additions must use Theme spacing tokens.

---

## Typography

The Theme singleton defines the project font-size tokens.

| Role | Theme Property | Size | Weight | Line Height |
|------|----------------|------|--------|-------------|
| Body | `Theme.fontSizeSM` | 11px | Regular (400) | 1.5 |
| Label | `Theme.fontSizeMD` | 12px | Regular (400) | 1.5 |
| Heading | `Theme.fontSizeLG` | 14px | Bold (700) | 1.2 |
| Numeric data | `Theme.fontSizeXS` | 10px | Regular (400) | 1.5 |

Font weights: 2 declared (400 Regular, 700 Bold). No intermediate weights.

Monospace: `font.family: "Consolas"` for all numeric data, G-code text, gradient legend labels, and stat values. Source: existing StatsPanel.qml, Legend.qml, PreviewPage.qml G-code text window.

---

## Color

| Role | Theme Property | Value | Usage |
|------|----------------|-------|-------|
| Dominant (60%) | `Theme.bgBase` / `#0d0f12` | Page background, empty-state backdrop |
| Secondary (30%) | `Theme.bgPanel` / `#161a23` + `#24272e` | Right panel surface, card/section backgrounds (StatsPanel, Legend, G-code text, VisibilityFilter) |
| Accent (10%) | `Theme.accent` / `#18c75e` | Active/selected state indicators, checkbox accent, current-line highlight |
| Destructive | `Theme.statusError` / `#e04040` | Error states only (not used in this phase) |

Additional semantic colors used in Preview panels (source: existing PreviewPage.qml, StatsPanel.qml, Legend.qml):

| Token | Value | Usage |
|-------|-------|-------|
| `Theme.textPrimary` | `#e8edf6` | Primary labels, stat values, heading text |
| `Theme.textSecondary` | `#a0abbe` | Secondary labels, toggle labels |
| `Theme.textTertiary` | `#7f90a6` | Disabled/muted text, empty-state text |
| `Theme.borderSubtle` | `#333b4e` | Panel/card borders |
| `Theme.bgHover` | `#2e3444` | Checkbox/row hover feedback |
| `Theme.bgElevated` | `#2a3140` | Button/row normal background |

Accent reserved for:
- Active CxSwitch track (`Theme.switchTrackOn`)
- Current G-code line highlight (`#3a2515` background with `#ff9f40` text — existing pattern in PreviewPage.qml G-code text window)
- Active/selected panel border accent
- CxCheckBox checked state

Upstream extrusion-role feature-type colors (for FeatureType/Line Type view-mode legend). Source: `GCodeViewer.cpp:1518-1524` and upstream default palette. The legend uses discrete role colors, not gradient, in FeatureType mode. Per-role colors are baked into `StoredSegment.baseR/baseG/baseB` at recolor time and consumed by the renderer as-is.

---

## Component Inventory

### New Components (Phase 55 additions)

| Component | File | Purpose |
|-----------|------|---------|
| VisibilityFilter | `src/qml_gui/components/VisibilityFilter.qml` | Collapsible per-role line-type visibility checkbox group in the right panel |

### Modified Components

| Component | File | Modification |
|-----------|------|-------------|
| PreviewPage.qml | `src/qml_gui/pages/PreviewPage.qml` | Insert VisibilityFilter into right panel ScrollView, between StatsPanel and Legend |
| PreviewViewModel | `src/core/viewmodels/PreviewViewModel.h/.cpp` | Add per-role visibility toggle properties, visibility-mask state |

### Existing Components Consumed (no modification)

| Component | File | Role in Phase 55 |
|-----------|------|-----------------|
| CxCheckBox | `src/qml_gui/controls/CxCheckBox.qml` | Per-role visibility checkboxes in VisibilityFilter |
| CxSwitch | `src/qml_gui/controls/CxSwitch.qml` | Existing Stealth/Normal toggle (unchanged) |
| CxComboBox | `src/qml_gui/controls/CxComboBox.qml` | Existing view-mode selector (unchanged) |
| CxScrollView | `src/qml_gui/controls/CxScrollView.qml` | Right panel scroll container |
| StatsPanel | `src/qml_gui/components/StatsPanel.qml` | Existing — Stealth toggle, display toggles, stat rows (unchanged) |
| Legend | `src/qml_gui/components/Legend.qml` | Existing — gradient/discrete legend (unchanged) |
| Theme | `src/qml_gui/Theme.qml` | Singleton — all tokens consumed as-is (no modification) |

---

## VisibilityFilter Component Specification

### Purpose (GCODE-02)

A collapsible group of per-role checkboxes placed inside the right panel's ScrollView, between StatsPanel and Legend. Mirrors upstream `GCodeViewer::render_legend()` FeatureType section where each extrusion role gets a colored rectangle + label + visibility checkbox. Source: upstream `GCodeViewer.cpp:3873-3912`.

### Location

Inside `PreviewPage.qml` right panel ScrollView ColumnLayout, between `Components.StatsPanel` and `Components.Legend` (lines 364-373 of current PreviewPage.qml).

### Behavior

1. **Visibility scope**: The VisibilityFilter group is always visible in the right panel regardless of current view-mode. Checking/unchecking a role toggles render-side draw filtering only (the renderer skips segments matching that role). It does NOT repack `gcodePreviewData`. Source: CONTEXT.md decision "render-side filtering".

2. **Collapsible**: Has a section header "可见线条类型" (qsTr: "Visible Line Types") with collapse/expand toggle matching the existing SidePanelHeader pattern used by the left panel ("盘与层") and right panel ("分析"). Default: expanded.

3. **Role list**: The checkbox list contains the following roles, ordered to match upstream `EGCodeExtrusionRole` enumeration order. Each row has a 10x10 colored square (matching the role's FeatureType color), a label, and a CxCheckBox.

   | # | Role (English label) | Role (Chinese label via qsTr) | Default visible |
   |---|---|---|---|
   | 1 | Inner wall | 内壁 | Yes |
   | 2 | Outer wall | 外壁 | Yes |
   | 3 | Overhang wall | 悬垂壁 | Yes |
   | 4 | Sparse infill | 稀疏填充 | Yes |
   | 5 | Internal solid infill | 内部实心填充 | Yes |
   | 6 | Top surface | 顶面 | Yes |
   | 7 | Ironing | 烫平 | Yes |
   | 8 | Bridge | 桥接 | Yes |
   | 9 | Gap infill | 间隙填充 | Yes |
   | 10 | Skirt | 裙边 | Yes |
   | 11 | Support | 支撑 | Yes |
   | 12 | Support interface | 支撑界面 | Yes |
   | 13 | Prime tower | 打塔 | Yes |
   | 14 | Bottom surface | 底面 | Yes |
   | 15 | Internal bridge | 内部桥接 | Yes |
   | 16 | Brim | 边缘 | Yes |
   | 17 | Support transition | 支撑过渡 | Yes |
   | 18 | Mixed | 混合 | Yes |

   Notes:
   - "Custom" role (index 14 upstream) is excluded from the visibility filter list since it has no meaningful upstream display name for user toggling.
   - Labels use `qsTr()` for i18n. English labels match upstream `to_string(EGCodeExtrusionRole)` exactly. Chinese labels are new translations for this component.
   - Default visibility: all extrusion roles visible. This is distinct from travel/wipe default-hidden (see existing `showTravelMoves` toggle in StatsPanel). Source: upstream extrusion_roles_visibility defaults all true.

4. **Row layout**: Each row is a `RowLayout` containing:
   - `Rectangle` (10x10, radius 2) colored with the role's FeatureType color
   - `Label` with role name (font: `Theme.fontSizeSM`, color: `Theme.textPrimary`)
   - `Item { Layout.fillWidth: true }` spacer
   - `CxCheckBox` bound to `previewVm.isRoleVisible(index)`

   Row height: 22px (matching existing G-code text row height in PreviewPage.qml).
   Row spacing: 4px between rows.

5. **Scroll behavior**: The entire right panel ScrollView already wraps StatsPanel + Legend + G-code text. VisibilityFilter is added to the same ColumnLayout inside the ScrollView, so it scrolls with the rest.

### ViewModel API Additions (PreviewViewModel)

The following properties/methods are added to `PreviewViewModel` to support the VisibilityFilter:

```cpp
// Per-role visibility toggle — 18 roles matching EGCodeExtrusionRole (excluding None and Custom)
Q_PROPERTY(QVariantList roleVisibilities READ roleVisibilities NOTIFY stateChanged)
Q_INVOKABLE bool isRoleVisible(int roleIndex) const;
Q_INVOKABLE void toggleRoleVisibility(int roleIndex);
```

- `roleVisibilities()` returns a `QVariantList` of objects `{ label, color, visible }` for binding to the VisibilityFilter Repeater model.
- `toggleRoleVisibility(int)` flips the visibility boolean, sets a dirty flag for render-side filtering, emits `stateChanged()`.
- The visibility mask is a `std::array<bool, 20>` (matching upstream `EGCodeExtrusionRole::COUNT = 20`) stored in PreviewViewModel.
- The visibility mask is passed to the renderer via the existing `gcodePreviewData` path or a new property (implementation detail at planner discretion). The renderer reads the mask to skip drawing segments whose role is not visible, WITHOUT repacking data.

---

## Normal/Stealth Mode (existing — no new UI)

The existing `StatsPanel.qml` already has a Normal/Stealth toggle via CxSwitch (lines 37-49 of StatsPanel.qml). This is the upstream `PrintEstimatedStatistics::ETimeMode::Normal` / `Stealth` mode toggle. Source: CONTEXT.md decision "Keep the existing stealthMode property; expose a two-segment Normal/Stealth estimate control near the view-mode combo."

No new UI is needed for this. The existing implementation is correct.

---

## Color Mode / View Mode (existing — audit only)

The existing CxComboBox in PreviewPage.qml header (line 90-95) binds to `viewModes` and `viewModeIndex`. The current 13 view modes are:

| # | Current Name | Upstream Equivalent | Gap? |
|---|---|---|---|
| 0 | Line Type | `FeatureType` | No |
| 1 | Layer Height | `Height` | No |
| 2 | Line Width | `Width` | No |
| 3 | Tool | `Tool` | No |
| 4 | Speed | `Speed` | No |
| 5 | Fan Speed | `FanSpeed` | No |
| 6 | Temperature | `Temperature` | No |
| 7 | Filament | `VolumetricFlowRate` | No |
| 8 | Filament ID | `ActualVolumetricFlowRate` | No |
| 9 | Flow | `ColorPrint` | **Mismatch — upstream has ColorPrint, not "Flow"** |
| 10 | Layer Time | `LayerTimeLinear` | No |
| 11 | Layer Time (log) | `LayerTimeLogarithmic` | No |
| 12 | Acceleration | `Acceleration` | No |

Missing from current list vs upstream:
- `Summary` — upstream has this as a non-color view mode (no legend, just statistics). May need to be added if upstream includes it in the combo.
- `Jerk` — upstream has this gradient mode.
- `PressureAdvance` — ORCA-specific addition upstream.
- `ActualSpeed` — upstream separate from `Speed`.

**Action for planner**: Audit and align the view-mode list against upstream `GCodeViewer::update_by_mode()` (lines 1077-1103). Add missing modes. Rename "Flow" to "Color Print" to match upstream. This is a ViewModel string-list change only, not a UI layout change.

---

## Legend Coherence (GCODE-03)

### Existing behavior
- `Legend.qml` shows a gradient bar when `legendType === 1` (continuous color mode like Speed, Temperature), or discrete colored items when `legendType === 0` or `2`.
- Gradient min/max labels use `Theme.fontSizeXS` (10px) and `Consolas` font.

### Requirements for Phase 55
- **Legend scope under slider filtering is global**: min/max/colors reflect the entire slice, not the filtered layer/move range. Legend must NOT recompute on every slider drag. Source: CONTEXT.md decision.
- **Gradient legend min/max recomputed only on recolor (mode change)**: When the user switches view mode, `recolorAndPackSegments()` runs, which also recomputes the legend gradient bounds. Slider drag only updates `currentLayerMax` / `currentMove`, which triggers draw-range updates in the renderer but NOT legend recomputation.
- **Per-move current value**: The tool-position tooltip (existing `ToolPositionTooltip.qml`) shows the per-move value for the current cursor position. This is already implemented and unchanged.

No new UI components needed for legend coherence. The guarantee is enforced in ViewModel logic (planner scope).

---

## Empty State Copy

| State | Location | Copy |
|-------|----------|------|
| No G-code loaded | Center viewport overlay (existing) | "请先切片或载入 G-code" (existing — unchanged) |
| No legend data | Legend.qml (existing) | "暂无图例数据" (existing — unchanged) |

No new empty states are introduced in this phase. The VisibilityFilter has no empty state — it always shows the fixed role list regardless of whether preview data exists.

---

## Error State Copy

| State | Copy | Resolution |
|-------|------|-----------|
| Slice failed | Routed via BackendContext notification system (existing) | "切片失败" — existing notification toast pattern |
| Preview data empty after slice | (GCODE-01 test verifies this never happens on normal local workflow) | N/A — prevented by GCODE-01 RED test |

No new error-copy patterns are needed in this phase. Rendering stability is enforced via source-audit tests (GCODE-04, GCODE-05), not via new UI error states.

---

## Destructive Actions

This phase introduces no destructive actions (no delete, reset, or overwrite operations in the Preview G-code semantics scope).

---

## Copywriting Contract

| Element | Copy (Chinese) | Copy (English source) |
|---------|---------------|-----------------------|
| VisibilityFilter section header | "可见线条类型" | "Visible Line Types" (noun phrase — describes the element type) |
| Inner wall | "内壁" | "Inner wall" (upstream `to_string(EGCodeExtrusionRole::Perimeter)`) |
| Outer wall | "外壁" | "Outer wall" (upstream `ExternalPerimeter`) |
| Overhang wall | "悬垂壁" | "Overhang wall" (upstream `OverhangPerimeter`) |
| Sparse infill | "稀疏填充" | "Sparse infill" (upstream `InternalInfill`) |
| Internal solid infill | "内部实心填充" | "Internal solid infill" (upstream `SolidInfill`) |
| Top surface | "顶面" | "Top surface" (upstream `TopSolidInfill`) |
| Ironing | "烫平" | "Ironing" (upstream `Ironing`) |
| Bridge | "桥接" | "Bridge" (upstream `BridgeInfill`) |
| Gap infill | "间隙填充" | "Gap infill" (upstream `GapFill`) |
| Skirt | "裙边" | "Skirt" (upstream `Skirt`) |
| Support | "支撑" | "Support" (upstream `SupportMaterial`) |
| Support interface | "支撑界面" | "Support interface" (upstream `SupportMaterialInterface`) |
| Prime tower | "打塔" | "Prime tower" (upstream `WipeTower`) |
| Bottom surface | "底面" | "Bottom surface" (upstream `BottomSurface`) |
| Internal bridge | "内部桥接" | "Internal bridge" (upstream `InternalBridgeInfill`) |
| Brim | "边缘" | "Brim" (upstream `Brim`) |
| Support transition | "支撑过渡" | "Support transition" (upstream `SupportTransition`) |
| Mixed | "混合" | "Mixed" (upstream `Mixed`) |

All user-visible strings MUST use `qsTr()`. The English column is the upstream source-of-truth key.

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| N/A — Qt6/QML project | N/A | N/A (no shadcn/third-party registry) |
| Cx* controls (project-local) | CxCheckBox | Existing controls — no new registry imports needed |

No third-party registries or external component libraries are introduced in this phase. All components are either existing Cx* controls or new QML components built from primitives (Rectangle, RowLayout, Label).

---

## Interaction States

### VisibilityFilter checkbox toggle
- **Normal**: CxCheckBox unchecked, role label in `Theme.textPrimary`
- **Hover**: Row background becomes `Theme.bgHover` (subtle feedback)
- **Active/Toggled**: CxCheckBox checked, draw filtering updates immediately (render-side only, no repack)

### No new page-level interactions
- Camera drag, layer slider drag, move slider drag: existing behavior, enforced stable by GCODE-05 regression tests.
- Page switch (Prepare <-> Preview): existing behavior, enforced stable by GCODE-05.
- Reslice: existing behavior, enforced by GCODE-01 no-placeholder test.

---

## Rendering Stability Contract (GCODE-04, GCODE-05)

This is not a UI visual spec but a behavioral contract the executor must enforce:

1. **No `SoftwareViewport` reference in PreviewPage.qml**: Source-audit test asserts `PreviewPage.qml` never contains the string `SoftwareViewport`. Already guarded by existing `QmlUiAuditTests.cpp`.

2. **RhiViewport as default**: `main_qml.cpp` registers `RhiViewport` as `GLViewport` type. SoftwareViewport is only an init-time fallback. Startup-policy test asserts this.

3. **Payload-survives-interaction invariant**: Camera drag, layer drag, move drag, page switch, and toggle operations must call `update()` only and never mutate `m_previewData` / `gcodePreviewData_`. Source-audit test extending `QmlUiAuditTests.cpp`.

4. **Reslice invalidation**: After a settings change that invalidates slice, old `gcodePreviewData` is cleared and rebuilt. Assert payload bytes change and layer/move counts recompute.

5. **Export-stability**: Exporting while Preview is visible leaves `gcodePreviewData` intact. Focused test.

---

## Checker Sign-Off

- [ ] Dimension 1 Copywriting: PASS — All labels defined with Chinese qsTr keys mapped to upstream English source-of-truth. Section headers are noun phrases (not bare adjectives).
- [ ] Dimension 2 Visuals: PASS — VisibilityFilter layout matches upstream right-panel legend structure. No new visual elements outside the right panel.
- [ ] Dimension 3 Color: PASS — 60/30/10 split maintained. Accent reserved for active states only. Role colors from upstream FeatureType palette.
- [ ] Dimension 4 Typography: PASS — 4 sizes declared (10/11/12/14), 2 weights (400/700). Monospace Consolas for numeric data.
- [ ] Dimension 5 Spacing: PASS — All spacing uses Theme tokens (4/6/8/12/16/24). No ad-hoc spacing values.
- [ ] Dimension 6 Registry Safety: PASS — No third-party registries. All controls are project-local Cx* or new QML from primitives.

**Approval:** pending
