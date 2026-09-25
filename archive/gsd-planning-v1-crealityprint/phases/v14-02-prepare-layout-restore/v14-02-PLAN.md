# PLAN: v14-02 Prepare Page Layout Restoration

## Goal

Align PreparePage layout with upstream CrealityPrint v7.0.1. After this phase, the spatial layout of panels, toolbars, and action buttons matches upstream.

**Prerequisite:** v14-01 Component System Hardening must be complete.

## Execution Order

```
Wave 1 (P0 structural) → Wave 2 (P1 positioning) → Wave 3 (P2 polish) → Verify
```

---

## Wave 1: P0 Structural Changes

### Task 1.1: Move Slice/Print buttons to title bar

**Files:** `src/qml_gui/main.qml`

Upstream ref: `MainFrame.cpp:1787-1817` — Slice/Print are side_tools in the tab bar.

Changes in `main.qml` titleBar RowLayout:
1. After the page tab buttons (`Row { id: tabRow }`), add Slice/Print action group:
   ```
   Row {
       spacing: Theme.spacingSM
       // Slice button with dropdown
       CxPillAction {
           text: qsTr("Slice plate")
           style: CxPillAction.Style.Primary  // teal/accent color
           onClicked: backend.editorViewModel.requestSlice()
           // Dropdown: "Slice plate" / "Slice all plates"
       }
       // Print button with dropdown
       CxPillAction {
           text: qsTr("Print")
           style: CxPillAction.Style.Secondary
           // Dropdown: "Print plate" / "Export G-code" / "Send"
       }
   }
   ```
2. Connect to existing `requestSlice()`, `requestSliceAll()`, export/print actions
3. Add dropdown menus using `CxMenu` + `CxMenuItem`

**Then** remove the bottom-right floating action row from `PreparePage.qml` (lines 3716-3749):
- Delete `Row { id: actionRow }` with 打印配置/导出G-code/切片全部平板/发送打印 buttons

**Verify:** Slice/Print visible in title bar, no bottom action row

### Task 1.2: Create bottom plate bar, remove plate cards from left panel

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GLCanvas3D.cpp:11030` — plate thumbnails at viewport bottom.

Step 1 — Create plate bar at viewport bottom:
```
Rectangle {
    id: plateBar
    anchors.bottom: parent.bottom
    anchors.left: leftPanel.right
    anchors.right: sidebar.left
    anchors.bottomMargin: Theme.spacingMD
    height: 80
    color: "transparent"

    ListView {
        orientation: ListView.Horizontal
        layoutDirection: Qt.LeftToRight
        spacing: Theme.spacingSM
        // Reuse plate card delegate, now horizontal
        model: editorVm.plateCount
        delegate: plateCardDelegate
    }
}
```

Step 2 — Move the plate card Repeater from `leftPanel` (lines 3257-3519) to `plateBar`
Step 3 — Adjust card layout from vertical to horizontal thumbnail
Step 4 — Add horizontal scroll via `CxScrollView`
Step 5 — Preserve right-click context menu (`plateContextMenu`)
Step 6 — Preserve drag-drop for cross-plate object moves

**Verify:** Plates appear at bottom of viewport, not in left panel

### Task 1.3: Narrow left panel, remove plate section

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: Left ImGui overlay ~200-250px wide.

Changes:
1. `leftPanel.width: 296` → `leftPanel.width: 220`
2. Remove `CxSectionHeader "Plates"` and plate card Repeater (now in plate bar)
3. Keep: Printer preset combo, bed type combo, object list
4. Keep "全部"/"对象" toggle for object list filtering

**Verify:** Left panel is 220px, shows only printer + object list

### Task 1.4: Convert process bar from horizontal to vertical

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `Plater.cpp:9913-10008` — vertical GL toolbar, right edge, icon size 30px.

Changes:
1. Reposition `processBar`:
   - From: `anchors.top: topTools.bottom, horizontalCenter: parent.horizontalCenter`
   - To: `anchors.right: sidebar.left, verticalCenter: parent.verticalCenter`
2. Change layout from `Row` to `Column`
3. Change delegate from horizontal pill to vertical icon button (~30x30)
4. Categories: Quality, Strength, Speed, Support, Infill, Others (match upstream order)
5. Width: 46px, height: auto based on item count
6. Gap: 2px between items
7. Keep click-to-scroll behavior targeting PrintSettings categories

**Verify:** Process bar is vertical, right side of viewport, between viewport and sidebar

### Task 1.5: Add FilamentPanel to sidebar

**Files:**
- NEW: `src/qml_gui/panels/FilamentPanel.qml`
- MODIFY: `src/qml_gui/panels/Sidebar.qml`

Upstream ref: `Plater.hpp:156-176` — filament management section.

Create `FilamentPanel.qml`:
1. Section header: "耗材" / qsTr("Filament")
2. Per-extruder slot row:
   - Color circle (clickable → color picker popup)
   - Filament preset combo (`CxComboBox`)
   - Delete button (`CxIconButton`)
3. Bottom row:
   - Add filament button (+)
   - Bed type combo
4. Action buttons (if device connected):
   - AMS sync
   - Auto-mapping
   - Flushing volumes dialog button

Bind to ViewModel properties for filament data (may need new Q_PROPERTY on EditorViewModel or ConfigViewModel).

**Verify:** Filament section visible in sidebar, shows per-extruder color + preset

---

## Wave 2: P1 Positioning Changes

### Task 2.1: Reposition main toolbar to viewport top-left

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GLCanvas3D.cpp:10922-10923` — toolbar starts at left panel right edge.

Changes in `topTools`:
1. Change anchors:
   - From: `anchors.horizontalCenter: parent.horizontalCenter`
   - To: `anchors.left: leftPanel.right, anchors.leftMargin: Theme.spacingSM`
2. Keep `anchors.top: parent.top, anchors.topMargin: 14`
3. Keep toolbar as horizontal Row
4. Remove sidebar toggle + settings buttons from toolbar (those belong to collapse bar / sidebar)

**Verify:** Toolbar anchored to top-left of viewport area (right of left panel)

### Task 2.2: Restructure sidebar from 3-tab to single-scroll

**Files:** `src/qml_gui/panels/Sidebar.qml`

Upstream ref: `Plater.cpp:791` — `m_scrolled_sizer = new wxBoxSizer(wxVERTICAL)`.

Changes:
1. Remove `TabBar` + `StackLayout`
2. Replace with `CxScrollView` containing `ColumnLayout`:
   ```
   CxScrollView {
       ColumnLayout {
           // Section 1: Object List (collapsible)
           CollapsibleSection { title: qsTr("Objects"); content: ObjectList { } }

           // Section 2: Printer Preset (collapsible)
           CollapsibleSection { title: qsTr("Printer"); content: PrinterSection { } }

           // Section 3: Filament/Material (collapsible)
           CollapsibleSection { title: qsTr("Filament"); content: FilamentPanel { } }

           // Section 4: Process Settings (collapsible)
           CollapsibleSection { title: qsTr("Process"); content: PrintSettings { } }

           // Section 5: Slice Progress (collapsible, visible after slice)
           CollapsibleSection { title: qsTr("Slice"); visible: editorVm.slicing; content: SliceProgress { } }
       }
   }
   ```
3. Section headers clickable to collapse/expand
4. Process bar category clicks scroll to corresponding section

Note: Use existing `CollapsibleSection.qml` from `src/qml_gui/components/`.

**Verify:** Sidebar is single-scroll, no tabs, sections collapse/expand

### Task 2.3: Move collapse button to sidebar left edge

**Files:** `src/qml_gui/panels/Sidebar.qml`

Upstream ref: `Plater.cpp:9889-9910` — vertical GL bar at right edge.

Changes:
1. Move toggle button from top-left to center-left of sidebar edge:
   - `anchors.verticalCenter: parent.verticalCenter`
   - `anchors.left: parent.left; anchors.leftMargin: -14` (half overlapping)
2. Keep animation: `Behavior on implicitWidth { NumberAnimation { duration: 200 } }`

**Verify:** Collapse button centered on sidebar left edge

---

## Wave 3: P2 Polish

### Task 3.1: View presets bar — lower visual prominence

**Files:** `src/qml_gui/pages/PreparePage.qml`

No structural change. Add subtle opacity reduction:
```
viewPresets.opacity: viewPresets.hovered ? 1.0 : 0.6
Behavior on opacity { NumberAnimation { duration: 150 } }
```

### Task 3.2: Plate bar entrance animation

**Files:** `src/qml_gui/pages/PreparePage.qml`

Add to plate card delegate:
```
Component.onCompleted: { opacity = 0; scale = 0.9 }
NumberAnimation on opacity { from: 0; to: 1; duration: 150 }
NumberAnimation on scale { from: 0.9; to: 1.0; duration: 150 }
```

### Task 3.3: Verify interaction patterns preserved

- [ ] Drag-drop model import still works (DropArea on viewport)
- [ ] Object context menu works (right-click on viewport)
- [ ] Keyboard shortcuts work (W/E/R, Delete, Ctrl+Z/Y)
- [ ] Plate drag-drop for object-to-plate moves works in bottom bar
- [ ] All gizmo info panels still position correctly

---

## Final Verification

### Layout match checklist (compare with upstream screenshots):

| Area | Must Match |
|------|-----------|
| Title bar: Logo → Menus → Actions → Tabs → **Slice** → **Print** → Title → Notifications → Controls | ✓ |
| Left panel: 220px, printer preset + object tree only | ✓ |
| Bottom plate bar: horizontal thumbnails, slice status | ✓ |
| Process bar: vertical, right edge, 46px wide | ✓ |
| Right sidebar: single-scroll, collapsible sections | ✓ |
| Sidebar: collapse button on left edge | ✓ |
| Bottom-left: status bar only (object count, slice status) | ✓ |
| No bottom-right action row | ✓ |

### Build

```bash
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

### Functional smoke test

1. Open app → Prepare page loads correctly
2. Slice button in title bar → click → slice starts
3. Print button in title bar → click → print dialog opens
4. Plates appear at bottom bar → click to switch
5. Right-click plate → context menu works
6. Drag object to different plate → works
7. Process bar vertical → click Quality → sidebar scrolls to quality section
8. Sidebar collapse/expand → works with animation
9. All gizmo panels (cut, measure, etc.) → still position correctly
10. No QML warnings in console

---

## Task Summary

| Wave | Tasks | New Files | Files Modified | Est. LOC Changed |
|------|-------|-----------|---------------|------------------|
| 1 (P0) | 5 | 1 (FilamentPanel.qml) | 2 | ~400 |
| 2 (P1) | 3 | 0 | 2 | ~300 |
| 3 (P2) | 3 | 0 | 1 | ~30 |
| **Total** | **11** | **1** | **3** | **~730** |
