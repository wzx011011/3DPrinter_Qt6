# Phase 5 Plan — Prepare Sidebar Eight Sections

**Goal:** Sidebar 滚动区按上游八大区块顺序布局，能做实的区块完成，重活留骨架。

---

## Wave 1 — 05-01: 八大区块骨架 + Printer/Filament/Search/Process 骨架

**Requirements:** SIDEBAR-01, 02, 03, 04, 05, 06, 07, 08(骨架), 09(占位), 10(占位), 11

### Tasks

1. **LeftSidebar.qml 八大区块重构（SIDEBAR-01）**
   - 在现有 Printer(1)/Filament(2)/Objects(3) 之后，按上游顺序插入：
     - Process 顶部条（在 Objects 之前，对齐上游 ParamsPanel.m_top_panel 在 ObjectList 上方）
     - Search bar（在 Process 之后）
   - 调整顺序为：Printer → Filament → Process → Search → ObjectList → ObjectSettings → ObjectLayers → ParamsPanel
   - Transform/SliceProgress 移到八大区块之后（v1.x 残留保留）

2. **Process 顶部条（SIDEBAR-06/07/08 骨架）**
   - 新建 ProcessTopbar 组件（内联或独立 qml）
   - Process icon + "Process" 标签
   - SwitchButton(Global/Objects) → 绑 configVm.settingsScope（SIDEBAR-07 做实）
   - ModeSwitchButton(Simple/Advanced) → 占位（configMode 待 VM 扩展，SIDEBAR-08 骨架）
   - Compare 按钮（SIDEBAR-09 占位，v2.2）
   - Setting 按钮（SIDEBAR-10 占位）

3. **Search bar（SIDEBAR-11）**
   - StaticBox + TextInput + search icon
   - 输入回车跳转 SearchDialog（复用现有）

4. **Printer/Filament 折叠完善（SIDEBAR-02/03/04/05）**
   - 已有 printerExpanded/filamentSection，完善标题栏 icon/按钮样式对齐上游
   - 确保折叠状态正确（内容显隐）

5. **Wave 1 验证**
   - 增量构建通过
   - 冒烟 8s 未崩 + 零 QML warning
   - 八大区块顺序正确（肉眼/代码审查）

### Wave 1 出口

- 八大区块顺序对齐上游（SIDEBAR-01）
- Printer/Filament 折叠完整（SIDEBAR-02~05）
- Process 顶部条骨架（SIDEBAR-06/07 做实，08 骨架）
- Search bar（SIDEBAR-11）
- Compare/Setting 占位（SIDEBAR-09/10）

---

## Wave 2 — 05-02: ObjectList/ObjectSettings/ObjectLayers/ParamsPanel

**Requirements:** SIDEBAR-12, 13, 14, 15

**Depends on:** Wave 1 完成

### Tasks

1. **ObjectList 树完善（SIDEBAR-12）**
   - 现有 ObjectList 组件基础上补全：对象/部件/设置/层 树
   - 右键菜单（复用现有 CxMenu）
   - 重命名（复用现有）
   - 拖拽（v2.0 占位，依赖 editorVm 扩展）→ 标 [-]

2. **ObjectSettings（SIDEBAR-13）**
   - CollapsibleSection，绑定 editorVm.selectedObjectIndex
   - 选中对象时显示快速设置（层高/填充/速度等），无选中隐藏
   - 数据源：configVm + editorVm

3. **ObjectLayers（SIDEBAR-14）**
   - CollapsibleSection，仅打印对象时显示
   - 变量层高编辑器占位（v2.0 占位，完整编辑器延后）→ 标 [-]

4. **ParamsPanel page_view（SIDEBAR-15 骨架）**
   - 7 子 Tab 按钮（Print/PrintPlate/PrintObject/PrintPart/PrintLayer/Filament/Printer）
   - 参数列表占位（完整参数列表需 ConfigViewModel pageView 扩展）→ 标 [-]

5. **Wave 2 验证**
   - 增量构建通过
   - 冒烟 8s 未崩 + 零 QML warning
   - 八大区块全部存在（顺序 + 内容）

### Wave 2 出口

- ObjectList 树补全（SIDEBAR-12 部分）
- ObjectSettings 做实（SIDEBAR-13）
- ObjectLayers 占位（SIDEBAR-14）
- ParamsPanel 7 Tab 骨架（SIDEBAR-15）

---

## Risks

- **R1（中）**：重构 LeftSidebar 顺序可能破坏现有 Printer/Filament → 用插入而非移动，保留现有 Section 不动
- **R2（中）**：Process 顶部条的 Global/Objects 切换需 configVm.setSettingsScope → 确认 API 存在
- **R3（低）**：7 Tab 骨架无数据，冒烟可能 warning → 用空 model + visible 绑定

## Verification

- 增量构建（ninja OWzxSlicer.exe）
- 冒烟 8s + 零 QML warning
- 代码审查八大区块顺序

## Out of Scope（Phase 5 不做，标 [-]）

- Simple/Advanced 参数过滤的完整实现（需 ConfigViewModel configMode + 参数过滤）
- Compare DiffPresetDialog（v2.2）
- ObjectTableDialog（Setting 按钮）
- 变量层高编辑器完整实现
- ParamsPanel 完整参数列表（需 pageView 扩展）
