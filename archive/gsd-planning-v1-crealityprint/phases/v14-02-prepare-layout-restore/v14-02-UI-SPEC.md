---
phase: v14-01
slug: prepare-layout-restore
status: draft
shadcn_initialized: false
preset: none
created: 2026-06-03
---

# Phase v14-01 -- UI Design Contract: Prepare Page Layout Restoration

> Source-truth migration: the upstream CrealityPrint (C++/wxWidgets/ImGui) defines the authoritative layout.
> This contract specifies every layout change needed to align the Qt6/QML PreparePage with upstream.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none (Qt6/QML, not React) |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 "Basic" style + custom Cx* controls |
| Icon library | Inline SVG assets in `src/qml_gui/assets/icons/` |
| Font | System default (Segoe UI on Windows) |
| Theme | Singleton `Theme.qml` with dark-mode tokens |

---

## Spacing Scale

Existing tokens from `Theme.qml` (multiples of 4):

| Token | Value | Usage |
|-------|-------|-------|
| spacingXS | 4px | Icon gaps, inline padding |
| spacingSM | 6px | Compact element spacing |
| spacingMD | 8px | Default element spacing |
| spacingLG | 12px | Section padding |
| spacingXL | 16px | Layout gaps |
| spacingXXL | 24px | Major section breaks |

Exceptions: Theme.fontSizeSM=11px (odd), Theme.fontSizeMD=12px (not multiple of 4). These are existing and not changed in this phase.

---

## Typography

Existing tokens from `Theme.qml`:

| Role | Size | Weight | Line Height | Usage |
|------|------|--------|-------------|-------|
| Body/Label | 12px (fontSizeMD) | 400 normal | default (~1.5) | Panel content, labels |
| Small | 11px (fontSizeSM) | 400 normal | default | Secondary info, badges |
| Section | 14px (fontSizeLG) | 600 bold | ~1.2 | Section headers |
| Title | 16px (fontSizeXL) | 700 bold | ~1.2 | Panel titles, dialog headers |
| Tiny | 10px (fontSizeXS) | 400 normal | default | Tertiary info, status text |

---

## Color

Existing tokens from `Theme.qml`:

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | #0d0f12 (bgBase) | Main background, viewport area |
| Secondary (30%) | #131720-#1a1e28 (bgSurface, bgPanel) | Panels, cards, sidebar |
| Accent (10%) | #18c75e (accent) | Selected tab, active state, slice button, key actions |
| Destructive | #e04040 (statusError) | Delete confirmations, error states |

Accent reserved for: selected tab indicator, active gizmo button, Slice/Print button, current plate highlight, progress bar fill, selected objects indicator.

---

## 1. Overview

This spec covers the layout restoration of the Prepare Page to match the upstream CrealityPrint v7.0.1. The upstream uses:
- **BBLTopbar** (`BBLTopbar.cpp`): custom title bar with tab navigation and Slice/Print action buttons on the RIGHT side of the tab bar
- **Plater** (`Plater.cpp`): main workspace with sidebar, canvas toolbars
- **GLCanvas3D** (`GLCanvas3D.cpp`): in-canvas overlays for main toolbar, process bar, collapse bar, plate bar
- **ParamsPanel** (`ParamsPanel.hpp`): right-side parameter panel with Printer/Filament/Process sections
- **SiderBar** (`SiderBar.h`): sidebar with object list + filament section

Source-truth principle: upstream file locations and line references are given for each section.

---

## 2. TopBar Layout: Slice/Print Button Positioning

**Upstream Reference:**
- `MainFrame.cpp:1104-1106` -- `create_side_tools()` creates Slice/Print buttons and passes them as `side_tools` to `Notebook` constructor
- `MainFrame.cpp:1787-1817` -- `create_side_tools()` builds: [slice_option_btn] [slice_btn] --- gap 15px --- [print_option_btn] [print_btn] --- gap 19px
- `MainFrame.hpp:427-430` -- `m_slice_btn`, `m_slice_option_btn`, `m_print_btn`, `m_print_option_btn`
- These buttons appear in the TOP TAB BAR, on the RIGHT side of the page tabs

**Current Qt6 State:**
- `main.qml:3716-3749` -- Bottom-right floating `CxPillAction` row: "打印配置", "导出 G-code", "切片全部平板", "发送打印"
- No Slice/Print buttons in the title bar

**Target State:**
Move Slice and Print action buttons from the bottom-right floating row to the TITLE BAR, positioned to the RIGHT of the page tab buttons. The layout in the title bar RowLayout becomes:

```
[Logo] [File/Edit/View/Help/Settings menus] [action-tools: save/undo/redo] [page tabs] [SLICE btn] [PRINT btn] [project title] [...] [notifications] [window controls]
```

**Exact specification:**
- Slice button: `SideButton` style, text "Slice plate" (configurable to "Slice all" via dropdown), color #009688 (teal) per upstream `MainFrame.cpp:2274-2277`
- Print button: `SideButton` style, text "Print plate" (configurable via dropdown), same teal color
- Each button has a small dropdown chevron for mode selection (slice plate / slice all; print plate / print all / export gcode / send)
- Buttons positioned immediately after the page tabs, before the project title field
- Gap between Slice and Print: 15px (FromDIP(15))
- After Print button: 19px gap, then project title field

**Implementation Notes:**
- Add `CxPillAction` or new `CxSideButton` components in `main.qml` titleBar RowLayout, between the tabs group and the project title
- Connect to `backend.editorViewModel.requestSliceAll()` / `requestSlicePlate()` / print actions
- Remove the floating bottom-right action row from PreparePage.qml (lines 3716-3749) -- the "打印配置", "切片全部平板", "发送打印" buttons move to title bar

**Priority: P0** -- This is a primary action placement mismatch

---

## 3. Left Panel: Restructure to Match Upstream ImGui Overlay

**Upstream Reference:**
- `GLCanvas3D.cpp` -- Left panel is a GL ImGui overlay with: printer preset combo, bed type combo, collapsible object tree
- Plates are NOT in the left panel -- they are at the BOTTOM as a separate bar
- The left panel is approximately 200-250px wide, semi-transparent, overlaid on the viewport

**Current Qt6 State:**
- `PreparePage.qml:3154-3519` -- Left panel is 296px wide, contains:
  1. Printer combo
  2. Bed type combo
  3. Bed settings button
  4. "全部" / "对象" toggle
  5. Plate card list with thumbnails (this is upstream's bottom plate bar content)
  6. Object list
  7. Drag-drop target for model files

**Target State:**
Remove plate cards from the left panel. The left panel should contain ONLY:
1. Printer preset combo (smaller, ~200px width)
2. Bed type combo
3. Object tree (collapsible sections matching upstream's ImGui tree)
4. Drag-drop target

Reduce panel width from 296px to approximately 220px to match upstream's narrower overlay.

**Implementation Notes:**
- Remove `Repeater` for plate cards (lines 3257-3519) from left panel
- Move plate cards to new bottom plate bar (see Section 6)
- Adjust `leftPanel.width` from 296 to 220
- Keep printer combo, bed type combo, object list
- The "全部" / "对象" toggle can remain for object list filtering

**Priority: P0** -- Plates are misplaced; left panel is too wide

---

## 4. Main Toolbar: Reposition from Floating Center to Viewport-Top-Left

**Upstream Reference:**
- `GLCanvas3D.cpp` -- Main toolbar is a horizontal GL toolbar rendered at the TOP-LEFT of the 3D viewport canvas
- It contains: Add, Delete, DeleteAll, Copy, Paste, Clone, More(+arrange, orient, split, mirror), Undo, Redo

**Current Qt6 State:**
- `PreparePage.qml:1555-1908` -- `topTools` CxPanel, anchored `top: parent.top, horizontalCenter: parent.horizontalCenter, topMargin: 14`
- Horizontally centered floating bar above the viewport

**Target State:**
Reposition toolbar from horizontal-center-float to TOP-LEFT of the viewport:
- Change anchors from `horizontalCenter: parent.horizontalCenter` to `left: parent.left, top: parent.top`
- `leftMargin: 14` (keep same top margin)
- The toolbar remains horizontal (matching upstream)
- Remove the "sidebar toggle" and "settings" buttons from this toolbar -- those belong in the collapse bar (Section 11)

**Implementation Notes:**
- In `PreparePage.qml`, change `topTools` anchors:
  ```
  anchors.top: parent.top
  anchors.left: parent.left
  anchors.topMargin: 14
  anchors.leftMargin: 14
  ```
- Remove the last two button groups (sidebar toggle + settings) from toolbarRow
- Keep: Import, Add Plate, Undo, Redo, Delete, DeleteAll, Copy, Paste, Clone, Gizmo buttons, Arrange, Orient, Split, Mirror, Fit view, Left panel toggle

**Priority: P1** -- Position differs from upstream but functionality is preserved

---

## 5. Process Bar: Change from Horizontal to Vertical, Reposition to Right Edge

**Upstream Reference:**
- `Plater.cpp:9913-10008` (`init_process_toolbar`):
  - Layout type: **Vertical** (`ProcessBar::GLToolbar::Layout::Vertical`)
  - Horizontal orientation: **Center** (between viewport and sidebar)
  - Vertical orientation: **Center** (vertically centered)
  - Items: separator, then vertical list of category icons (Frequent, Quality, Strength, Speed, Support, Infill, ...)
  - Icon size: 30px
  - Gap size: 2px, separator size: 5px
  - Border: 0px
  - Rendered as GL overlay on the right edge of the viewport, between viewport and sidebar

**Current Qt6 State:**
- `PreparePage.qml:1910-1972` -- `processBar` CxPanel, horizontal layout
  - Anchored `top: topTools.bottom, horizontalCenter: parent.horizontalCenter`
  - Horizontal Row of category pills: "全部", "质量", "速度", "支撑", "温度", "填充", "底座", "其他"

**Target State:**
Change from horizontal floating bar to **vertical bar on the right edge** of the viewport:
- Position: `anchors.right: sidebar.left` (or `anchors.right: parent.right` if sidebar collapsed), vertically centered
- Orientation: **Column** (vertical) instead of **Row** (horizontal)
- Width: ~46px (icon width + padding), height: auto based on item count
- Each item: icon + optional label, stacked vertically
- Gap between items: 2px
- Categories: Frequent, Quality, Strength, Speed, Support, Infill, ... (matching upstream order)

**Implementation Notes:**
- Replace `Row { id: procRow }` with `Column { id: procCol }`
- Change Repeater delegate from horizontal pill to vertical icon button
- Reposition: `anchors.right: sidebar.left; anchors.rightMargin: 0; anchors.verticalCenter: parent.verticalCenter`
- Each category becomes a small icon button (~30x30 matching upstream icon size)
- Clicking a category scrolls the right sidebar PrintSettings to that section

**Priority: P0** -- Direction and position are both wrong vs upstream

---

## 6. Plate Selector: Move from Left Panel to Bottom Bar

**Upstream Reference:**
- `PartPlate.cpp` / `GLCanvas3D.cpp` -- Plate selector is a GL-rendered bottom bar inside the canvas
- Shows plate thumbnails horizontally, with slice status indicators
- Positioned at the BOTTOM of the viewport, full-width or near-full-width

**Current Qt6 State:**
- `PreparePage.qml:3257-3519` -- Plate cards are embedded in the left panel as a vertical list with thumbnails

**Target State:**
Create a **horizontal plate bar** at the BOTTOM of the viewport:
- Position: `anchors.bottom: parent.bottom`, `anchors.left: leftPanel.right`, `anchors.right: sidebar.left`
- Height: ~80px
- Contains: horizontal scrollable row of plate thumbnail cards
- Each card: ~64x64 thumbnail + plate name + slice status badge + lock icon
- Selected plate highlighted with accent border
- Right-click context menu preserved (plateContextMenu)
- Drag-drop target for moving objects between plates

**Implementation Notes:**
- Create new `Rectangle { id: plateBar }` at the bottom of the PreparePage viewport area
- Move the plate card Repeater from leftPanel to plateBar
- Change layout from vertical ColumnLayout to horizontal Row (with `ListView` orientation `Qt.Horizontal`)
- Add `ScrollView` for horizontal scrolling when plates exceed visible width
- Add plate bar toggle button (+ icon) at the end for adding new plates
- The left panel's "平板" section header and "全部"/"对象" toggle move to object list only

**Priority: P0** -- Complete position mismatch

---

## 7. Right Sidebar: Restructure from 3-Tab to Single-Scroll

**Upstream Reference:**
- `ParamsPanel.hpp:134-364` -- `ParamsPanel` has Printer/Filament/Process sections
- `Plater.hpp:123-231` -- `Sidebar` class manages the scrolled panel with:
  - Printer preset section (combo)
  - Filament section (color pickers, preset combos, AMS sync, auto-mapping, flushing volumes)
  - Process/Print settings section (parameter tree)
- All sections in a **single scrollable panel**, not tabbed
- `m_scrolled_sizer` holds all sections vertically

**Current Qt6 State:**
- `Sidebar.qml:1-200` -- 3-tab layout: "对象" (Object) / "打印" (Print) / "切片" (Slice)
- Tab switcher at top, `StackLayout` below
- Width: 328px expanded, 32px collapsed
- Collapse button at top-left edge

**Target State:**
Replace 3-tab layout with **single-scroll layout** containing these sections in order:
1. **Object List** (collapsible) -- transform panel + object tree
2. **Printer Preset** section -- combo box with preset selection
3. **Material/Filament** section -- filament color pickers, preset combos, add/delete filament (see Section 8)
4. **Process Settings** section -- parameter categories (quality, speed, support, etc.)

The section headers are clickable to collapse/expand each section.

**Implementation Notes:**
- Replace `TabBar` + `StackLayout` with a single `ScrollView` containing a `ColumnLayout`
- Each section wrapped in a collapsible group (click header to toggle)
- Width remains 328px expanded
- The process bar (Section 5) category clicks scroll to the corresponding section
- Remove tab bar UI entirely

**Priority: P1** -- Organization differs from upstream but tabs provide similar function

---

## 8. Material Panel: Create Standalone Filament Management Section

**Upstream Reference:**
- `Plater.hpp:156-176` -- `Sidebar` methods: `on_filaments_change()`, `add_filament()`, `delete_filament()`, `change_filament()`, `add_custom_filament()`, `show_flushDialog()`, `show_auto_mapping()`, `update_filament_panel()`
- `Plater.cpp` -- Filament section includes:
  - Filament color picker per extruder slot
  - Filament preset combo per slot
  - Add/delete filament buttons
  - AMS sync button
  - Auto-mapping button
  - Flushing volumes dialog button
  - Bed type combo

**Current Qt6 State:**
- No independent material/filament section exists
- Filament-related settings are partially merged into the Print tab
- No color picker UI for filaments
- No flushing volumes UI
- No AMS sync UI

**Target State:**
Add a **Material/Filament** section in the right sidebar (between Object List and Process Settings):
- Section header: "耗材" / "Filament"
- For each extruder slot:
  - Color circle (clickable to open color picker)
  - Filament preset combo (vendor/type selection)
  - Delete button
- Add filament button (+)
- AMS sync button (if device connected)
- Auto-mapping button
- Flushing volumes button (opens dialog)
- Bed type combo at top of section

**Implementation Notes:**
- Create new `FilamentPanel.qml` component in `src/qml_gui/panels/`
- Bind to `EditorViewModel` filament-related Q_INVOKABLE methods
- Color picker: use existing color dialog or inline grid of preset colors
- This is a NEW panel -- does not exist yet in Qt6 codebase
- May require new ViewModel properties for filament management

**Priority: P0** -- Entire section missing from Qt6

---

## 9. View Presets: Decision

**Upstream Reference:**
- Upstream has NO vertical view preset bar on the right side of the viewport
- View presets are accessible via keyboard shortcuts and the View menu only

**Current Qt6 State:**
- `PreparePage.qml:3528-3582` -- 46px wide vertical bar with 5 buttons: "俯"(Top), "前"(Front), "右"(Right), "轴"(Iso), "适"(Fit)
- Positioned at right edge, below top toolbar, right of viewport

**Target State:**
**Keep** the view presets bar. Rationale:
- Useful UX addition that does not conflict with upstream behavior
- Does not obscure any upstream UI element
- Provides quick access to camera presets that upstream hides in menus/shortcuts
- Small footprint (46px x 186px)

**Implementation Notes:**
- No changes needed -- keep as-is
- Consider reducing visual prominence (lower opacity when not hovered) to match upstream's minimal overlay aesthetic

**Priority: P2** -- Qt6-only addition, no upstream conflict, useful UX

---

## 10. Bottom Actions: Decision

**Upstream Reference:**
- Upstream has NO bottom action bar in the Prepare page
- Slice/Print buttons are in the TOP TAB BAR (see Section 2)
- All actions are accessible from top bar or GL overlay toolbars

**Current Qt6 State:**
- `PreparePage.qml:3716-3749` -- Bottom-right floating row with:
  - "打印配置" (Print Settings) -- navigates to Print tab in sidebar
  - "导出 G-code" -- opens export dialog
  - "切片全部平板" -- slices all plates
  - "发送打印" (Send Print) -- opens print dialog
- `PreparePage.qml:3655-3713` -- Bottom-left status bar with object count and slice status

**Target State:**
- **Remove** the bottom-right floating action row (打印配置, 导出 G-code, 切片全部平板, 发送打印)
- Move Slice/Print to title bar (Section 2)
- **Keep** the bottom-left status bar (object count, slice status) -- this aligns with upstream status information display
- "导出 G-code" becomes an option in the Print dropdown in the title bar
- "打印配置" becomes a click on the Process section header in the sidebar

**Implementation Notes:**
- Remove `Row { anchors.bottom ... anchors.right: sidebar.left }` (lines 3716-3749) from PreparePage.qml
- Keep status bar `Row { anchors.bottom ... anchors.left: leftPanel.right }` (lines 3655-3713)
- Status bar position adjusts to be `anchors.left: plateBar.left` (new plate bar) instead of `leftPanel.right`

**Priority: P0** -- Actions must move to title bar per upstream

---

## 11. Sidebar Collapse: Verify Alignment

**Upstream Reference:**
- `Plater.cpp:9889-9910` (`init_collapse_toolbar`):
  - Collapse bar is a **vertical** GL toolbar on the RIGHT edge
  - `Layout::Vertical`, `HO_Right`, `VO_Top`
  - Single button that toggles sidebar collapse
  - Tooltip updates to show current state
- `Plater.hpp:198-199` -- `is_collapsed()`, `collapse(bool)`

**Current Qt6 State:**
- `Sidebar.qml:37-65` -- Toggle button at top-left edge of sidebar
  - 28x28px circle button with arrow (left/right)
  - `z: 10` to float above content
  - Position: `anchors.left: parent.left, leftMargin: -10` (overlaps edge)
- `Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }`

**Target State:**
Move collapse button to match upstream's right-edge vertical bar position:
- Position: `anchors.right: parent.left` (on the left edge of the sidebar, between viewport and sidebar)
- Vertically centered or top-aligned
- Single icon button (no text), toggle behavior
- Animation: 200ms width transition (already correct)

**Implementation Notes:**
- Change toggle button position from top-left to right edge of sidebar's left border
- Or: create a separate thin vertical strip between viewport and sidebar (like upstream)
- The collapse animation and width values (328px expanded, 32px collapsed) are correct
- The upstream collapse bar renders in GL, so exact pixel match is not required -- functionally aligned

**Priority: P1** -- Functionally aligned, position refinement needed

---

## 12. Interaction Patterns

### Drag-Drop (Model Import)
**Upstream:** Files dropped onto the canvas are imported via `Plater::load_files()`
**Qt6:** `PreparePage.qml:1476-1507` -- `DropArea` on viewport with URI-list keys. Already aligned.
**Priority: P2** -- Already working

### Context Menu (Right-Click Objects)
**Upstream:** `Plater::PopupMenu()` / `create_object_menu()` -- Copy, Paste, Delete, Clone, Mirror, Arrange, Split, etc.
**Qt6:** `PreparePage.qml:146-255` -- `objectContextMenu` with matching items. Already aligned.
**Priority: P2** -- Already working

### Keyboard Shortcuts
**Upstream:** `GLCanvas3D::on_char()` / `on_key()` -- W/E/R for gizmos, Delete, Ctrl+Z/Y, Ctrl+A, etc.
**Qt6:** `PreparePage.qml:49-143` -- `Keys.onPressed` handler with matching bindings. Also `main.qml` Shortcuts. Already aligned.
**Priority: P2** -- Already working

### Plate Drag-Drop (Object to Plate)
**Upstream:** Objects can be dragged from the object list to different plates
**Qt6:** Plate cards in left panel have `DropArea` for receiving dragged objects. This should be preserved when plates move to bottom bar.
**Priority: P1** -- Must preserve when plates move to bottom bar

---

## 13. Animation/Transitions

### Sidebar Collapse/Expand
**Upstream:** Smooth GL-rendered slide animation
**Qt6:** `Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }` -- Already correct.
**No changes needed.**

### Panel Show/Hide
**Upstream:** Panels appear/disappear instantly (no animation on ImGui panels)
**Qt6:** Left panel visibility toggled via `leftPanelVisible` boolean -- instant show/hide. Already correct.

### Gizmo Panels
**Current:** Cut/Flatten/Measure/Paint panels appear with `visible` toggle -- instant. This matches upstream.
**No changes needed.**

### Plate Bar Items
**New:** When plate bar is created (Section 6), add subtle entrance animation for new plate cards:
- `NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }`
- `NumberAnimation { property: "scale"; from: 0.9; to: 1.0; duration: 150 }`

**Priority: P2** -- Polish, not structural

---

## 14. Responsive/Scaling

### DPI Handling
**Upstream:** Uses `FromDIP()` for all sizing, which accounts for Windows DPI scaling
**Qt6:** Uses `backend.uiScale` with a `Scale` transform on the shell rectangle (`main.qml:546-551`):
```qml
transform: Scale {
    xScale: backend.uiScale
    yScale: backend.uiScale
    origin.x: 0
    origin.y: 0
}
```
All sizes are in logical pixels, scaled by the transform. This is equivalent to upstream's `FromDIP()`.

**Minimum sizes:**
- Window: 1100x700 (already declared in `main.qml:19`)
- Left panel: 220px (reduced from 296)
- Right sidebar: 328px expanded, 32px collapsed (unchanged)
- Top bar: 52px (unchanged)
- Bottom plate bar: ~80px height

**Priority: P2** -- Existing scaling is adequate

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Primary CTA (Slice) | "切片" (with dropdown: "切片当前板" / "切片全部") |
| Primary CTA (Print) | "打印" (with dropdown: "打印当前板" / "打印全部" / "导出G-code" / "发送") |
| Empty plate state | "拖拽模型到此处或点击导入" |
| Empty object list | "暂无对象 — 导入 STL/3MF/OBJ 文件开始" |
| Error state | "操作失败: {error message} — 请检查模型或参数后重试" |
| Delete confirmation | "删除对象": "确定要删除选中的 {N} 个对象？此操作不可撤销。" |
| Delete plate confirmation | "删除平板": "确定要删除平板 {name}？其上的对象将被移除。" |

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | N/A | not applicable (Qt6/QML project) |
| Third-party | N/A | not applicable |

---

## Implementation Priority Summary

| # | Section | Priority | Effort | Description |
|---|---------|----------|--------|-------------|
| 2 | TopBar Slice/Print | P0 | M | Move Slice/Print buttons from bottom to title bar |
| 3 | Left Panel restructure | P0 | S | Remove plate cards, narrow to 220px |
| 5 | Process Bar vertical | P0 | M | Horizontal to vertical, right edge |
| 6 | Plate Selector bottom bar | P0 | M | New horizontal plate bar at viewport bottom |
| 8 | Material/Filament panel | P0 | L | New filament management section in sidebar |
| 10 | Bottom Actions removal | P0 | S | Remove floating action row |
| 4 | Toolbar reposition | P1 | S | Center to top-left anchor |
| 7 | Sidebar single-scroll | P1 | M | 3-tab to single scroll with collapsible sections |
| 11 | Sidebar collapse position | P1 | S | Move collapse button to edge |
| 9 | View Presets | P2 | -- | Keep as-is |
| 12 | Interaction patterns | P2 | -- | Already aligned |
| 13 | Animations | P2 | S | Add plate card entrance animation |
| 14 | Responsive/scaling | P2 | -- | Already handled |

---

## Checker Sign-Off

- [ ] Dimension 1 Copywriting: PASS
- [ ] Dimension 2 Visuals: PASS
- [ ] Dimension 3 Color: PASS
- [ ] Dimension 4 Typography: PASS
- [ ] Dimension 5 Spacing: PASS
- [ ] Dimension 6 Registry Safety: PASS

**Approval:** pending
