# Phase 5 Context — Prepare Sidebar Eight Sections

**Phase:** 5
**Goal:** Sidebar 滚动区按上游八大区块顺序完整实现：Printer → Filament → Process 顶部条 → Search → ObjectList → ObjectSettings → ObjectLayers → ParamsPanel page_view。
**Depends on:** Phase 4（Dockable 布局完成后填充内容）
**Requirements:** SIDEBAR-01 ~ SIDEBAR-15
**Mode:** Standard / auto (yolo)

---

## Upstream Truth

上游 Sidebar（`third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp` Sidebar 类，~3000 行）八大区块：

1. **Printer**（SidebarPrinter）— preset combo + 喷嘴 + Bed type + extruders，标题栏可折叠
2. **Filament**（SidebarFilament）— 双列耗材列表（per extruder），标题栏可折叠
3. **Process 顶部条**（ParamsPanel.m_top_panel）— Process icon + SwitchButton(Global/Objects) + ModeIcon + ModeSwitchButton(Simple/Advanced) + Compare + Setting
4. **Search bar**（OG::SearchCtrl）— StaticBox + TextInput，跳转到参数
5. **ObjectList**（ObjectList.cpp）— 对象/部件/设置/层 树，拖拽/右键/重命名
6. **ObjectSettings**（选中对象的快速设置，无选中隐藏）
7. **ObjectLayers**（变量层高编辑器，仅打印对象显示）
8. **ParamsPanel page_view**（参数列表 + 左侧 7 子 Tab: Print/PrintPlate/PrintObject/PrintPart/PrintLayer/Filament/Printer）

上游关键文件：
- `Plater.hpp:128` Sidebar 类
- `Plater.cpp:4930+` Sidebar 构造
- `ParamsPanel.cpp/.hpp` 参数面板（Simple/Advanced + 7 Tab + page_view）
- `ObjectList.cpp/.hpp` 对象树
- `OG.cpp/.hpp` OptionsGroup（参数项渲染）

---

## Current State（Phase 4 之后）

`LeftSidebar.qml` (648 行) 当前是 **v1.x 五区块结构**（非上游八大）：
- Section 1: Printer（preset combo + 喷嘴 + bed type，已有折叠）✅
- Section 2: Filament（耗材 slot grid，已有折叠）✅
- Section 3: Objects（对象列表，部分实现）
- Section 4: Transform（v1.x 残留，属 Gizmo，Phase 6）
- Section 5: SliceProgress（v1.x 残留）

**缺失**：Process 顶部条、Search、ObjectSettings、ObjectLayers、ParamsPanel page_view。

ConfigViewModel 已有：settingsScope(Global/Objects)、preset 系列、printOptions/machineOptions/filamentOptions。
**缺**：Simple/Advanced 模式过滤、7 子 Tab、page_view 参数列表、search 跳转。

---

## Design Decision

**务实方案（采用）：骨架优先 + 已有数据源做实 + 重活留骨架**

Phase 5 不重写 LeftSidebar（避免回归），而是**插入缺失的区块**，保证 SIDEBAR-01 八大区块顺序。Transform/SliceProgress 等 v1.x 残留保留（放八大区块之后，不删除）。

### 能做实的（已有数据源）

- **Printer 折叠完善**（SIDEBAR-03）：已有 printerExpanded，完善标题栏图标/按钮
- **Filament 折叠完善**（SIDEBAR-05）：已有 filamentSection CollapsibleSection
- **Search bar**（SIDEBAR-11）：复用现有 SearchDialog（ConfigViewModel 已有 preset 搜索），TextInput + 跳转
- **ObjectSettings**（SIDEBAR-13）：选中对象时显示快速设置，绑定 editorVm.selectedObjectIndex
- **ObjectLayers**（SIDEBAR-14）：变量层高编辑器占位（仅打印对象显示），绑定 editorVm

### 留骨架的（需 VM 大改，记为部分完成）

- **Process 顶部条**（SIDEBAR-06）：UI 骨架（SwitchButton Global/Objects 绑 settingsScope + ModeSwitchButton Simple/Advanced 占位 + Compare/Setting 按钮）。Simple/Advanced 过滤（SIDEBAR-08）需 ConfigViewModel 加 configMode 属性 + 参数过滤，**v2.0 留占位**（标记 [-]）
- **ObjectList 树**（SIDEBAR-12）：现有 ObjectList 组件完善（对象/部件/设置/层树），拖拽/右键/重命名部分依赖 editorVm，**现有基础上补全**
- **ParamsPanel page_view**（SIDEBAR-15）：UI 骨架（7 子 Tab 按钮 + 参数列表占位）。完整参数列表需 ConfigViewModel 加 pageView 分组，**v2.0 留占位**（标记 [-]）
- **Compare**（SIDEBAR-09）：DiffPresetDialog v2.0 占位（需求明确说延后到 v2.2）

## Pitfalls

1. **不破坏现有 Printer/Filament 功能**：插入新区块用 CollapsibleSection 包裹，复用现有 configVm 数据源
2. **v1.x Transform/SliceProgress 残留**：放八大区块之后，Phase 6 迁移到 Gizmo 浮层时再清理
3. **Search 跳转**：复用 SearchDialog，避免重写搜索逻辑
4. **参数系统 (Simple/Advanced + 7 Tab + page_view)**：是 Phase 5 最重的部分，v2.0 只做 UI 骨架，完整参数列表延后

## Acceptance（来自 ROADMAP Success Criteria + 需求分级）

| REQ | 契约 | Phase 5 目标 | 完成度标记 |
|---|---|---|---|
| SIDEBAR-01 | 八大区块顺序布局 | 插入缺失区块，顺序对齐上游 | [x] |
| SIDEBAR-02 | Printer 标题栏 + 内容 | 已有，完善图标/按钮 | [x] |
| SIDEBAR-03 | Printer 标题栏可折叠 | 已有 printerExpanded | [x] |
| SIDEBAR-04 | Filament 标题栏 + 双列耗材 | 已有 filamentSection | [x] |
| SIDEBAR-05 | Filament 标题栏可折叠 | 已有 CollapsibleSection | [x] |
| SIDEBAR-06 | Process 顶部条 | UI 骨架 (SwitchButton/ModeSwitch/Compare/Setting) | [-] 骨架 |
| SIDEBAR-07 | Global/Objects 作用域切换 | 绑 settingsScope | [x] |
| SIDEBAR-08 | Simple/Advanced 模式过滤 | UI 占位，过滤需 VM 扩展 | [-] 骨架 |
| SIDEBAR-09 | Compare → DiffPresetDialog | 按钮 + 占位（需求延后 v2.2） | [-] 占位 |
| SIDEBAR-10 | Setting → ObjectTableDialog | 按钮 + 占位 | [-] 占位 |
| SIDEBAR-11 | Search bar | TextInput + 跳转 SearchDialog | [x] |
| SIDEBAR-12 | ObjectList 树 | 现有基础上补全（对象/部件/设置/层） | [-] 部分 |
| SIDEBAR-13 | ObjectSettings | 选中对象显示快速设置 | [x] |
| SIDEBAR-14 | ObjectLayers | 占位（仅打印对象显示） | [-] 占位 |
| SIDEBAR-15 | ParamsPanel page_view + 7 Tab | UI 骨架（7 Tab 按钮 + 参数列表占位） | [-] 骨架 |

**Phase 5 出口**：八大区块顺序对齐（SIDEBAR-01）+ 能做实的区块完成（02/03/04/05/07/11/13）+ 重活留骨架（06/08/09/10/12/14/15 标 [-]）。
