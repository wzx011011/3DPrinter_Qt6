# PLAN: v15-01 Context Menu Migration

## Goal

Align all right-click context menus with upstream CrealityPrint v7.0.1 `GUI_Factories.cpp`. After this phase, every upstream context menu scenario has a corresponding Qt6 menu with matching items, dispatch logic, and behavior.

**Upstream source truth:** `third_party/CrealityPrint/src/slic3r/GUI/GUI_Factories.cpp` (lines 1477-2410+), `GUI_ObjectList.cpp` (lines 1360-1539), `IMSlider.cpp` (lines 1817-1916), `TickCode.hpp` (tick data model), `FilamentPanel.cpp:2725-2827` (material context menu)

**Prerequisite:** v14-02 Prepare Page Layout Restoration must be complete.

## Execution Order

```
Wave 0 (prerequisites: C++ infrastructure)
  → Wave 1 (P0 dispatch + default menu)
  → Wave 2 (P0 volume/part menus)
  → Wave 3 (P1 object menu gaps + multi-selection + plate)
  → Wave 4 (P2 slider G-code menus)
  → Wave 5 (P2 filament panel right-click)
  → Wave 6 (P2 auxiliary list component + right-click)
  → Verify
```

---

## Previously Blocked — Now Included in This Plan

The original plan had 4 items marked as "blocked." Analysis showed:

| Item | Blocker | Resolution |
|------|---------|-----------|
| Filament panel right-click | Panel didn't exist | **False** — `FilamentPanel.qml` exists (143 lines, in Sidebar.qml). Added as Wave 5. |
| Slider G-code menus | No TickCode infrastructure in PreviewViewModel | Added TickCode model as Wave 0.1 prerequisite. Unblocked. |
| Auxiliary list right-click | No auxiliary list component | Build the component as part of Wave 6. Unblocked. |
| Assemble view menus | No assemble view at all | Genuinely requires full `CanvasAssembleView` implementation — separate phase. See Deferred section. |

---

## Wave 0: Prerequisites — C++ Infrastructure

These tasks add the backend methods and data models that multiple later waves depend on.

### Task 0.1: TickCode model + PreviewViewModel methods

**Files:** `src/core/viewmodels/PreviewViewModel.h/.cpp` (new file: `src/core/rendering/TickCodeTypes.h`)

Upstream ref: `TickCode.hpp` — tick data struct, `IMSlider.cpp` — tick management.

**0.1a: Create TickCodeTypes.h**

Plain data types for tick marks (aligned with upstream `TickCode.hpp`):

```cpp
namespace Crality3D {
enum class TickType : int { ColorChange = 0, PausePrint = 1, ToolChange = 2, Custom = 3, Template = 4 };
struct TickCode {
    int tick;           // layer number
    TickType type;
    int extruder;       // for ToolChange
    QString color;      // for ColorChange
    QString extra;      // gcode content for Custom/Template
};
}
```

**0.1b: Add to PreviewViewModel**

```
Q_PROPERTY:
- QVariantList tickMarks READ tickMarks NOTIFY tickMarksChanged
- int tickMarkCount READ tickMarkCount NOTIFY tickMarksChanged

Q_INVOKABLE:
- void addPauseAtLayer(int layer)
- void addCustomGcodeAtLayer(int layer, QString gcode)
- void removeTickAtLayer(int layer)
- void editCustomGcodeAtLayer(int layer, QString newGcode)
- void addFilamentChangeAtLayer(int layer, int extruderId)
- QVariantMap tickAtLayer(int layer) const
- void clearAllTicks()

Internal:
- QList<Crality3D::TickCode> tickMarks_ member
- emit tickMarksChanged() after mutations
```

Note: Initially the ticks are in-memory only. Integration with actual G-code export (via `GCodeProcessor::apply_custom_gcode_per_print_z()`) can be wired later. The context menu UI and tick model are unblocked.

**Verify:** Unit-testable — add/remove ticks, verify tickMarks property updates.

### Task 0.2: EditorViewModel missing action methods

**Files:** `src/core/viewmodels/EditorViewModel.h/.cpp`, `src/core/services/ProjectServiceMock.h/.cpp`

Add all Q_INVOKABLE methods referenced by later waves but not yet implemented:

```
EditorViewModel:
- bool fixMeshSelected()         → calls projectSvc fixMesh
- bool simplifyMeshSelected()    → stub that returns false (needs simplify dialog later)
- bool meshBooleanSelected()     → stub that returns false (needs boolean dialog later)
- bool changeVolumeType(int objIdx, int volIdx, int newType) → calls projectSvc
- bool reloadSelectedFromDisk()  → calls projectSvc reload
- bool replaceWithStl(QString path) → calls projectSvc replaceVolume
- bool reloadAllOnPlate()        → iterates objects on plate, reload each
- bool assembleSelectedObjects() → merges selected into one multi-part object
- bool instanceToObject(int instIdx) → duplicates instance as independent object
- int getSelectedVolumeType()    → returns VolumeType enum for selected volume
- void addPrimitive(int type)    → creates cube/sphere/cylinder geometry

ProjectServiceMock:
- bool fixMesh(int objIdx)           → #ifdef HAS_LIBSLIC3R: call admesh repair
- bool changeVolumeType(int obj, int vol, int newType)
- bool reloadFromDisk(int objIdx)    → re-reads source file path
- bool replaceVolume(int obj, int vol, QString stlPath)
- bool assembleObjects(QList<int> objIndices) → merges into multi-part
- bool duplicateInstanceAsObject(int objIdx, int instIdx)
- QObject* addPrimitiveToPlate(int type) → creates primitive mesh

EditorViewModel properties:
- bool showLabels / setShowLabels → forwarded to GLViewport
```

Each method follows the `#ifdef HAS_LIBSLIC3R` pattern — real impl when available, stub returning false otherwise. Menu items gate on `enabled: editorVm.fixMeshSupported` etc.

**Verify:** Build passes. Methods callable from QML (even if stubs return false for now).

---

## Wave 1: P0 — Dispatch Refactor + Default (Empty Canvas) Menu

### Task 1.1: Refactor context menu dispatch in PreparePage

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `Plater.cpp:9512-9574` — `on_right_click()` dispatches to 8 different menus based on selection state.

Currently `objectContextMenu` is shown on every right-click regardless of selection. Refactor to dispatch like upstream:

1. On right-click in GLViewport, determine context:
   - **No selection + empty space** → `defaultContextMenu` (new)
   - **Single object selected** → `objectContextMenu` (existing, enhanced)
   - **Multiple objects selected** → `multiContextMenu` (new)
2. Add a JS function `determineContextMenu(mouseX, mouseY)` that checks `editorVm.selectedObjectIndex` and selection state
3. Gate visibility of menu items via `enabled` and `visible` bindings based on selection state

**Verify:** Right-click on empty canvas shows default menu, right-click on object shows object menu, right-click with multi-selection shows multi-selection menu

### Task 1.2: Implement Default Menu (empty canvas right-click)

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GUI_Factories.cpp:1486-1526` — `create_default_menu()`

Create `defaultContextMenu` CxMenu:

```
Menu items:
- 添加模型... (Add Models) → openModelDialog.open()
- 添加测试体 (Add Primitive) submenu:
  - 立方体 (Cube)        → editorVm.addPrimitive(0)
  - 球体 (Sphere)        → editorVm.addPrimitive(1)
  - 圆柱体 (Cylinder)    → editorVm.addPrimitive(2)
  - 圆锥体 (Cone)        → editorVm.addPrimitive(3)
  - 截锥体 (Truncated Cone) → editorVm.addPrimitive(4)
  - 圆环体 (Torus)       → editorVm.addPrimitive(5)
  - 盘状体 (Disc)        → editorVm.addPrimitive(6)
  - 文字 (Text)          → opens addTextDialog
  - SVG                  → opens svgFileDialog
- ─────────
- 显示标签 (Show Labels, checkable) → editorVm.showLabels toggle
```

**Verify:** Right-click empty canvas → see default menu → Add Models opens file dialog, Add Primitive adds cube to scene

---

## Wave 2: P0 — Volume/Part Context Menu

### Task 2.1: Expand Volume Menu in ObjectList

**Files:** `src/qml_gui/panels/ObjectList.qml`

Upstream ref: `GUI_Factories.cpp:1703-1735` — `create_bbl_part_menu()`

Current `volumeMenu` has only 2 items (Edit in Settings, Delete). Expand to match upstream:

```
Menu items:
- 克隆 (Clone) → editorVm.duplicateSelectedObjects()
- ─────────
- 删除 (Delete) → editorVm.deleteSelection()
- 复制 (Copy) → editorVm.copySelectedObjects()
- 粘贴 (Paste) → editorVm.pasteObjects()
- ─────────
- 修复网格 (Fix Model) → editorVm.fixMeshSelected()
- 简化模型 (Simplify Model) → editorVm.simplifyMeshSelected()
- 居中到热床 (Center) → editorVm.centerSelectedObjects()
- 镜像 (Mirror) submenu: X/Y/Z
- 拆分 (Split) submenu: To objects / To parts
- ─────────
- 在参数表中编辑 (Edit in Parameter Table) → editorVm.requestSelectionSettings()
- 转换类型 (Change Type) submenu:
  - 部件 (Part)
  - 负体积 (Negative Volume)
  - 修改器 (Modifier)
  - 支撑屏蔽 (Support Blocker)
  - 支撑增强 (Support Enforcer)
- ─────────
- 从磁盘重新加载 (Reload from disk) → editorVm.reloadSelectedFromDisk()
- 替换为 STL (Replace with STL) → open replace dialog
```

**Verify:** Right-click on a volume in ObjectList → see full menu with Clone/Fix/Simplify/Change Type/etc

### Task 2.2: Add Text/SVG volume menu distinction

**Files:** `src/qml_gui/panels/ObjectList.qml`

Upstream ref: `GUI_Factories.cpp:1675-1700` — separate `text_part_menu()` and `svg_part_menu()`

When the selected volume is a TextVolume or SvgVolume type, prepend the volume menu with:
- **Text:** "编辑文字 (Edit Text)" as first item → opens text emboss editor
- **SVG:** "编辑 SVG (Edit SVG)" as first item → opens SVG import dialog

Gate with `visible: editorVm.getSelectedVolumeType() === VolumeType.TextEmboss` etc.

**Verify:** Right-click text volume → first item is "Edit Text"; right-click SVG volume → first item is "Edit SVG"

---

## Wave 3: P1 — Object Menu Gaps + Multi-Selection + Plate

### Task 3.1: Fill missing Object Context Menu items

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GUI_Factories.cpp:1573-1622` — `create_extra_object_menu()`

Add these missing items to `objectContextMenu`:

```
New items (insert in order):
- 修复模型 (Fix Model) → editorVm.fixMeshSelected()
- 简化模型 (Simplify Model) → editorVm.simplifyMeshSelected()
- 布尔运算 (Mesh Boolean) → editorVm.meshBooleanSelected()
- ─────────
- 在参数表中编辑 (Edit in Parameter Table) → editorVm.requestSelectionSettings()
- 编辑工艺设置 (Edit Process Settings) → backend.openSettings()
- ─────────
- 从磁盘重新加载 (Reload from disk) → editorVm.reloadSelectedFromDisk()
- 替换为 STL (Replace with STL) → open replace dialog
- 更换耗材 (Change Filament) submenu → per-extruder items + Default
```

**Verify:** Right-click object → see Fix/Simplify/Boolean/Reload/Replace/Change Filament

### Task 3.2: Create Multi-Selection Context Menu

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GUI_Factories.cpp:2004-2072` — `multi_selection_menu()`

Create `multiContextMenu` CxMenu, shown when multiple objects are selected:

```
Menu items:
- 合并装配 (Assemble) → editorVm.assembleSelectedObjects()
- 克隆 (Clone) → editorVm.duplicateSelectedObjects()
- ─────────
- 居中到热床 (Center) → editorVm.centerSelectedObjects()
- 修复网格 (Fix Model) → editorVm.fixMeshSelected()
- 删除 (Delete) → editorVm.deleteSelectedObjects()
- 复制 (Copy) → editorVm.copySelectedObjects()
- 粘贴 (Paste) → editorVm.pasteObjects()
- ─────────
- 设为可打印/不打印 (Printable toggle) → editorVm.setObjectPrintable()
- 编辑工艺设置 (Edit Process Settings) → backend.openSettings()
- ─────────
- 更换耗材 (Change Filament) submenu
- ─────────
- 导出为 STL (Export as one STL) → editorVm.exportSelectedAsStl()
```

**Verify:** Select 2+ objects, right-click → see multi-selection menu with Assemble/Fix/Change Filament

### Task 3.3: Fill Plate Context Menu gaps

**Files:** `src/qml_gui/pages/PreparePage.qml`

Upstream ref: `GUI_Factories.cpp:1798-1896` — `create_plate_menu()`

Add missing items to `plateContextMenu`:

```
New items:
- 从磁盘全部重新加载 (Reload All) → editorVm.reloadAllOnPlate()
- 粘贴 (Paste) → editorVm.pasteObjects()
- 添加模型... (Add Models) → openModelDialog.open()
- 添加测试体 (Add Primitive) submenu (same as default menu)
```

**Verify:** Right-click plate → see Reload All/Paste/Add Models

---

## Wave 4: P2 — Slider G-code Menus

### Task 4.1: Expand LayerSlider context menus

**Files:** `src/qml_gui/components/LayerSlider.qml`, `src/qml_gui/components/CustomGcodeDialog.qml` (new)

Upstream ref: `IMSlider.cpp:1817-1916` — `render_add_menu()` and `render_edit_menu()`

**4.1a: Add Menu** (right-click on slider groove in empty area):

```
Menu items:
- 添加暂停 (Add Pause) → previewVm.addPauseAtLayer(currentLayer)
- 添加自定义 G-code (Add Custom G-code) → open customGcodeDialog
- 跳转到层... (Jump to Layer) → existing
```

**4.1b: Edit Menu** (right-click on existing tick mark):

```
Depends on tick type:
- PausePrint tick → "删除暂停 (Delete Pause)"
- Custom/Template tick → "编辑自定义 G-code" + "删除自定义 G-code"
- ToolChange tick → "更换耗材" submenu + "删除耗材切换"
```

**4.1c: Create CustomGcodeDialog.qml** — simple dialog with a TextArea for G-code input, OK/Cancel buttons.

**4.1d: Render tick marks on slider** — add colored markers on the slider track at tick positions. Use a Repeater bound to `previewVm.tickMarks`. Color-code by type (orange=Pause, blue=Filament, green=Custom).

**Verify:** In Preview, right-click slider → see Add Pause / Add Custom G-code. After adding, tick mark appears, right-click tick → see Delete/Edit options.

### Task 4.2: Add Instance to Object menu

**Files:** `src/qml_gui/panels/ObjectList.qml`

Upstream ref: `GUI_Factories.cpp:1929` — `append_menu_item_instance_to_object()`

When the selected row is an instance, add to rowMenu:

```
- 实例转为对象 (Instance to Object) → editorVm.instanceToObject(instIdx)
```

**Verify:** Right-click on instance row → see "Instance to Object" → clicking creates independent object

---

## Wave 5: P2 — Filament Panel Right-Click Menu

### Task 5.1: Add context menu to FilamentPanel

**Files:** `src/qml_gui/panels/FilamentPanel.qml`, `src/core/viewmodels/EditorViewModel.h/.cpp`

Upstream ref: `FilamentPanel.cpp:2725-2827` — `MaterialContextMenu`

FilamentPanel.qml already exists (143 lines, embedded in Sidebar.qml). Add right-click on each filament slot:

```
Menu items:
- 编辑 (Edit) → configVm.setCurrentFilamentPreset(presetName)
- 删除 (Delete) → editorVm.deleteFilamentSlot(index)
- 合并到 (Merge with) submenu → lists all other filament slots
  - 耗材 N (Filament N) → editorVm.mergeFilamentSlots(fromIndex, toIndex)
```

Gate "Delete" on `editorVm.extruderCount > 1` (can't delete last slot).
Gate "Merge with" on `editorVm.extruderCount > 1`.

Add to EditorViewModel:
- `Q_INVOKABLE bool deleteFilamentSlot(int index)` → removes filament from config
- `Q_INVOKABLE bool mergeFilamentSlots(int from, int to)` → merges presets

Add MouseArea with `acceptedButtons: Qt.RightButton` to each filament slot delegate.

**Verify:** Right-click filament slot → see Edit/Delete/Merge with. Delete removes slot. Merge combines two slots.

---

## Wave 6: P2 — Auxiliary List Component + Right-Click Menu

### Task 6.1: Create AuxiliaryListPanel component

**Files:** `src/qml_gui/panels/AuxiliaryListPanel.qml` (new), `src/core/services/AuxiliaryService.h/.cpp` (new)

Upstream ref: `GUI_AuxiliaryList.cpp`, `AuxiliaryDialog.cpp`

Create a minimal auxiliary file manager panel:

**AuxiliaryListPanel.qml:**
- TreeView-like list with folder/file nodes
- Toolbar: Import File, New Folder, Delete buttons
- Delegate: icon (folder/file) + name + right-click trigger
- Double-click file → `Qt.openUrlExternally(filePath)`

**AuxiliaryService (new QObject service):**
- `Q_PROPERTY QVariantList auxiliaryFiles READ auxiliaryFiles NOTIFY filesChanged`
- `Q_INVOKABLE void importFile(QString path, QString folderPath)`
- `Q_INVOKABLE void createFolder(QString name, QString parentPath)`
- `Q_INVOKABLE void deleteItem(QString path)`
- `Q_INVOKABLE void renameItem(QString path, QString newName)`
- `Q_INVOKABLE void openFile(QString path)` → `QDesktopServices::openUrl()`
- Internal: manages files under model auxiliary temp path

Register in BackendContext, expose to QML.

### Task 6.2: Add Auxiliary List context menu

**Files:** `src/qml_gui/panels/AuxiliaryListPanel.qml`

Upstream ref: `GUI_AuxiliaryList.cpp:194-241`

```
Context menu varies by target:
- Empty space → "新建文件夹 (New Folder)" → auxSvc.createFolder()
- Folder node → "导入文件 (Import File)" + "删除 (Delete)"
- File node   → "打开 (Open)" + "删除 (Delete)" + "重命名 (Rename)"
```

### Task 6.3: Wire AuxiliaryListPanel into PreparePage

**Files:** `src/qml_gui/pages/PreparePage.qml` or `src/qml_gui/panels/Sidebar.qml`

Add a collapsible section in the sidebar (below FilamentPanel or as a dialog accessible from toolbar).

**Verify:** Open auxiliary panel → import file → right-click file → see Open/Delete/Rename. Right-click empty → see New Folder.

---

## Deferred to Dedicated Phase (NOT blockers)

These items require major infrastructure that is a separate feature, not a context menu task:

### Assemble View + 3 Context Menus
- **What:** Full `CanvasAssembleView` mode with section view slider, explosion ratio, separate selection/transform logic, dedicated gizmo data pool.
- **Menus:** `assemble_object_menu`, `assemble_part_menu`, `assemble_multi_selection_menu` (hide/show, delete, change filament).
- **Why separate:** Requires new GLCanvas3D canvas type, ImGui-to-QML migration of section/explosion controls, assemble transformation system in Selection.cpp. Estimated 2-3 weeks standalone.
- **Not a blocker because:** Normal view context menus do not depend on assemble view. The "Assemble" action in multi-selection menu (merging objects) works in normal view.

### SLA Object Menu
- **What:** `create_sla_object_menu()` with SLA-specific items (scale to build volume, auto orientation).
- **Why separate:** Project is FDM-only for now. SLA support requires upstream SLA print pipeline integration.

---

## Verification Checklist

After all waves complete:

- [ ] Build passes: `scripts/auto_verify_with_vcvars.ps1`
- [ ] 0 new QML warnings
- [ ] Right-click empty canvas → Default menu with Add Models + 9 Primitives
- [ ] Right-click single object → Full object menu (Fix/Simplify/Boolean/Reload/Replace/Change Filament)
- [ ] Right-click multiple objects → Multi-selection menu (Assemble/Fix/Change Filament)
- [ ] Right-click volume in ObjectList → Full volume menu (Clone/Fix/Simplify/Change Type/Reload/Replace)
- [ ] Right-click text volume → "Edit Text" as first item
- [ ] Right-click SVG volume → "Edit SVG" as first item
- [ ] Right-click plate → Full plate menu (Reload All/Paste/Add Models)
- [ ] Right-click instance → "Instance to Object" item
- [ ] Right-click preview slider → Add Pause / Add Custom G-code
- [ ] Right-click slider tick mark → Delete/Edit options by tick type
- [ ] Right-click filament slot → Edit/Delete/Merge with menu
- [ ] Auxiliary panel → Import files, right-click file → Open/Delete/Rename
