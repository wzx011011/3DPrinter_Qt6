# Phase 198 Plan: ObjectList Tree Deepening (Auxiliary file-tree panel)

**Requirement:** INDEX.md:58 "Auxiliary file-tree panel"
**Goal:** Deepen the existing `src/qml_gui/panels/ObjectList.qml` object tree to
better match upstream OrcaSlicer `GUI_ObjectList` density and hierarchy, without
rebuilding the whole panel or reviving the removed `AuxiliaryPage`. This is an
incremental enhancement of ObjectList.qml, not a new page.

## Interpretation of the INDEX.md item

`INDEX.md:58` "Auxiliary file-tree panel" was undefined. The most reasonable
reading is a deepening of the existing object→volume two-level tree in
`ObjectList.qml` toward upstream `GUI_ObjectList` (object→volume→layer,
right-click menu parity, visual density). We explicitly do **not** revive the
deleted `AuxiliaryPage` (that belongs to the removed-device scope).

## Background / source truth

Upstream tree node types live in
`D:/work/OrcaSlicer/src/slic3r/GUI/ObjectDataViewModel.hpp:25-36`:

```
itObject, itVolume, itInstanceRoot, itInstance, itSettings,
itLayerRoot, itLayer, itInfo
```

So the full upstream hierarchy under one object is roughly:
object → [Instances, Layers (LayerRoot/Layer), Settings, Volumes]. The right-click
menus are built in `D:/work/OrcaSlicer/src/slic3r/GUI/GUI_Factories.cpp`:
`create_object_menu` (1273), `create_common_object_menu` (1253: Rename, Delete,
Reload, Export STL, Fix, Mirror), `create_extra_object_menu` (1297: Fill bed,
Clone, Fix, Simplify, Merge-to-single-part, Center, Drop, Split, Mirror, Delete,
Add volumes, Printable, per-object process, Add settings, Layers), `create_part_menu`
(1372: Rename, Delete, Reload, Export, Fix, Mirror, Merge, Split, Change type,
per-object settings), `append_menu_item_per_object_settings` and the per-object
layer editor in `GUI_ObjectLayers.cpp`.

## Current Qt6 state (before Phase 198)

- `ObjectList.qml` already implements object→volume two levels, inline rename,
  drag-reorder, add-volume submenu (part/negative/modifier/support-blocker/
  support-enforcer, primitives, text/SVG emboss, file import), change-type
  submenu, copy/cut/clone, center, split, fix-mesh, export STL, instance-to-
  object, delete. Volume-level menu has edit text/SVG, clone, delete, copy/paste,
  fix, simplify, center, mirror, split, per-object settings, change type, reload
  from disk, replace with STL.
- `EditorViewModel` already exposes Phase 175 layer-range proxies:
  `objectLayerRangeCount`, `layerRangeMinZ/MaxZ`, `layerRangeValue`,
  `addObjectLayerRange`, `removeObjectLayerRange`, `setLayerRangeValue`.
- `ObjectLayersDialog.qml` (Phase 175) exists and is wired in `PreparePage.qml`
  but was **only** reachable from PreparePage's own context menu
  (PreparePage.qml:480-483 `层高范围...`), NOT from the ObjectList panel's
  object right-click menu. So the layer editor was effectively disconnected from
  the object tree.
- `SelectionSettingsDialog` (Phase 174) is wired via the
  `selectionSettingsRequested` signal; ObjectList already calls
  `requestSelectionSettings()` via its "在参数表中编辑" item.
- `selectedVolumeIndex` was a private member (`m_selectedVolumeIndex`) used by
  `settingsTargetVolumeIndex` but NOT exposed as a Q_PROPERTY. PreparePage.qml:61
  bound `editorVm.selectedVolumeIndex`, which resolved to `undefined` in QML.

## Changes

### C++ (`src/core/viewmodels/EditorViewModel.{h,cpp}`)

- Add `Q_PROPERTY(int selectedVolumeIndex READ selectedVolumeIndex NOTIFY
  stateChanged)` + getter `int selectedVolumeIndex() const` returning the
  existing `m_selectedVolumeIndex`. Fixes the latent PreparePage.qml:61 binding.
- Add `Q_INVOKABLE void requestObjectLayerRanges()` + signal
  `objectLayerRangeRequested()`, mirroring `requestSelectionSettings`/`
  selectionSettingsRequested`. Guards on exactly one source object selected
  (the dialog keys off `selectedObjectIndex`).

### QML — `src/qml_gui/pages/PreparePage.qml`

- Add `onObjectLayerRangeRequested` handler to the existing `Connections` block
  that opens `objectLayersDialog`. The dialog already binds
  `objectIndex: editorVm.selectedObjectIndex`, so it refreshes on open.

### QML — `src/qml_gui/panels/ObjectList.qml`

Object right-click menu additions:
- New "层高范围..." item (label includes count when > 0) right after "在参数表
  中编辑". Calls `requestObjectLayerRanges()`. This connects the previously
  PreparePage-only layer editor into the object tree (对齐上游 object_menu
  "Layers" + `GUI_ObjectLayers`).

Row delegate additions:
- `readonly property int layerRangeCount` bound to `objectLayerRangeCount`.
- New "层高 ×N" accent badge in the object status-row (alongside plate/module
  pills). Clicking the badge selects the object and opens the layer editor.
- New "N 部件" tertiary hint showing volume count for multipart objects.

Visual density (对齐上游 wxDataViewTreeCtrl geometry):
- Object expand/collapse glyph replaced: `+`/`-` text → a rotated disclosure
  triangle drawn via `Canvas`, with a 100 ms rotation animation and hover-driven
  color. The disclosure hit area grew from 14 to 16 px and now spans the full
  row height for an easier click target.
- Print-state toggle dot now has a `ToolTip` ("参与打印（点击禁用）" /
  "不参与打印（点击启用）") and `hoverEnabled`.

All QML colors use Theme tokens (no hard-coded colors introduced; existing
hard-coded `#ffaaaa` and `font.pixelSize: 8` literals left as-is to avoid scope
creep).

## Layer-level integration status

Layer ranges are now **reachable** from the object tree (menu item + badge →
existing ObjectLayersDialog), and the count is surfaced read-only in the row.
However the layer ranges are NOT yet rendered as an actual third nesting level
inside the tree (an expandable "Layers" node under each object with one child
row per range). Reasons for deferral:

- The current delegate computes row height from a fixed `volumeCount` child
  Repeater. Adding a parallel layer-range Repeater would require restructuring
  the delegate's height binding and the volume/layer coexistence layout, which
  is high-risk for the existing object/volume CRUD.
- Upstream models layers via a dedicated `itLayerRoot`/`itLayer` node type in
  `ObjectDataViewModel`; the Qt6 port has no equivalent tree-model abstraction
  (the list is a flat `ListView` over `objectCount`). A faithful third level
  would either need a new ObjectDataViewModel-style C++ model or a notable
  QML delegate rewrite.

This is tracked as **deferred** below.

## Per-object settings entry

Already present as ObjectList's "在参数表中编辑" object-menu item and the volume-
menu "在参数表中编辑" item, both calling `requestSelectionSettings()` → opens
`SelectionSettingsDialog`. No new entry needed; this phase only adds the layer
editor entry alongside it.

## Known limitations / deferred

1. **Layer ranges as a true third tree level**: deferred. Only the editor entry
   + count badge are added. A full third level needs either an
   `ObjectDataViewModel`-style tree model in C++ or a delegate restructure.
2. **Volume rename** in the volume right-click menu: upstream `create_part_menu`
   (GUI_Factories.cpp:1375) calls `append_menu_item_rename` before delete. The
   Qt6 side has no `renameVolume` C++ API, so this is deferred (would need a new
   VM proxy + ProjectServiceMock method).
3. **Instance nodes (`itInstanceRoot`/`itInstance`)**: upstream shows instances
   as a subtree. Qt6 collapses instances into the object row (instance count is
   shown only to drive the "拆分为独立对象" item). Adding instance children is
   deferred.
4. **`itSettings` node**: upstream shows a "Settings" child node per object to
   indicate per-object overrides exist. Qt6 surfaces this only via the badge row
   and the per-object settings dialog; a dedicated settings tree node is
   deferred.
5. **Build verification**: could not be run in this environment — the VS 2022
   Developer Command Prompt (`VsDevCmd.bat`) errors out before `vcvars64.bat`
   completes, so the MSVC toolchain is unavailable here. Correctness was
   verified by header/cpp declaration/definition matching and a Python bracket
   balance check on every changed file. A normal developer-machine build should
   be run before merge.

## Upstream GUI_ObjectList parity checklist (object right-click)

| Upstream (GUI_Factories.cpp)         | Qt6 ObjectList.qml                  | Status |
|--------------------------------------|-------------------------------------|--------|
| Rename (1255)                        | 重命名 (inline)                     | have   |
| Delete (1259)                        | 删除对象 / 删除已选对象             | have   |
| Reload from disk (1264)              | (volume menu) 从磁盘重新加载        | have*  |
| Export STL (1265)                    | 导出为 STL...                       | have   |
| Fix through netfabb (1269)           | 修复网格                            | have   |
| Mirror (1270)                        | (volume menu) 镜像                  | have*  |
| Split To objects / To parts (1280+)  | 拆分为对象 (+ volume 拆分)          | have   |
| Clone (1301)                         | 克隆                                | have   |
| Simplify (1305)                      | (volume menu) 简化模型              | have*  |
| Center (1309)                        | 居中到热床                          | have   |
| Add volumes (1333)                   | 添加部件 submenu                    | have   |
| Printable (1337)                     | 设为可打印/不参与打印               | have   |
| per-object process (1338)            | 编辑工艺设置 (PreparePage)          | have*  |
| Add settings (1340)                  | 在参数表中编辑                      | have   |
| Layers (GUI_ObjectLayers)            | **层高范围... (new this phase)**    | have   |
| Change filament                      | 更换耗材 (PreparePage)              | have*  |

`have*` = present but only in the volume submenu or the PreparePage context
menu, not the ObjectList object menu. Acceptable for this incremental phase.

## Files changed

- `src/core/viewmodels/EditorViewModel.h` — selectedVolumeIndex Q_PROPERTY +
  getter, requestObjectLayerRanges Q_INVOKABLE, objectLayerRangeRequested signal.
- `src/core/viewmodels/EditorViewModel.cpp` — getter + requestObjectLayerRanges
  implementation.
- `src/qml_gui/pages/PreparePage.qml` — onObjectLayerRangeRequested handler.
- `src/qml_gui/panels/ObjectList.qml` — layer-range menu item + count badge +
  volume-count hint + disclosure-triangle glyph + print-toggle tooltip.

## Rules followed

- New/changed comments are English ASCII-only, 2-space indent.
- QML uses Theme tokens for new colors.
- No object/volume CRUD behavior changed; additions are strictly additive.
- No new C++ abstraction (no ObjectDataViewModel port) — only thin VM proxies.
