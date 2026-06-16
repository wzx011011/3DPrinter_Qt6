# Phase 2: 9-Page Notebook + BBLTopbar - Context

**Gathered:** 2026-06-16
**Status:** Ready for planning (autonomous mode — defaults applied)
**Source:** Autonomous discuss (gray areas pre-decided from ROADMAP.md)

<domain>
## Phase Boundary

重写 `main.qml` 顶层框架,从当前 4-tab `workflowTabs` 数组迁移到上游 OrcaSlicer 9-tab TabBar + StackLayout 架构,并完成 BBLTopbar 完整菜单系统(已有 CxMenu 基础设施,需补全 + 重排)。

**In scope:**
- 9 个 tab 按 OrcaSlicer 上游顺序:Home / Prepare / Preview / Device / MultiDevice / Project / Calibration + 2 占位
- TabBar + StackLayout 替代当前 `workflowTabs` 数组 + 手动切换
- Tab 切换通过 `request_select_tab(TabPosition)` 事件广播(对齐上游 `EVT_SELECT_TAB`)
- BBLTopbar 标题栏 + 菜单 + 工具按钮合体设计
- `[File ▾]` 下拉菜单完整化:New/Open/Recent/Save/Save As/Import 系列/Export 系列/Quit
- `[▾]` 二级下拉:Edit/View/Preferences/Calibration/Help 完整子菜单
- 工具按钮:Save/Undo/Redo/Calibration + 条件按钮(Account/ModelStore/Publish)
- CenteredTitle 显示项目名(居中)
- 窗口控制按钮:Min/Max/Close
- macOS 系统菜单栏 / Win+Linux BBLTopbar 条件编译分支

**Out of scope (defer to later phases):**
- Plater 共享实例(Phase 3)
- Sidebar Dockable(Phase 4)
- 八大区块内容(Phase 5)
- GLCanvas 工具栏(Phase 6)
- Calibration Dialog 实现(Phase 7)
</domain>

<decisions>
## Implementation Decisions

### 架构(ARCH-01 ~ ARCH-04)

- **TabBar 实现**:使用 Qt Quick Controls `TabBar` + `StackLayout` 组合(非自绘),保持与现有 Cx* 控件风格一致
- **9 个 Tab 顺序(锁定)**:`tpHome=0, tp3DEditor=1, tpPreview=2, tpDevice=3, tpMultiDevice=4, tpProject=5, tpCalibration=6, tpPlaceholder1=7, tpPlaceholder2=8`
- **TabPosition 常量语义(ARCH-02)**:对齐上游 `Notebook.cpp` 的 `TabPosition` 枚举值
- **Tab 切换事件(ARCH-03)**:实现 `request_select_tab(TabPosition)` 信号,通过 BackendContext 广播;所有需要响应 tab 切换的组件连接此信号(替代当前的 `pendingSwitchToken`/`pendingSwitchTargetPage` 局部状态)
- **side_tools 区(ARCH-04)**:TabBar 右侧附加 `side_tools` 容器,含 Slice 下拉按钮 + Print 下拉按钮 + FilamentGroupPopup(多耗材分组切片占位,v2.0 仅占位不实现完整逻辑)

### BBLTopbar 菜单系统(TOPBAR-01 ~ TOPBAR-07)

- **BBLTopbar 整合(锁定)**:标题栏 + 菜单 + 工具按钮合体,放在 ApplicationWindow 顶部,与现有 `flags: Qt.FramelessWindowHint` 配合
- **`[File ▾]` 菜单(锁定)**:New / Open / Recent(子菜单) / Save / Save As / Import(子菜单:Import 3MF/STL/OBJ/STEP/AMF) / Export(子菜单:Export G-code/Export 3MF/Export Model) / Quit
- **`[▾]` 二级菜单(锁定)**:
  - Edit: Undo / Redo / Cut / Copy / Paste / Delete / Select All / Invert Selection
  - View: Show/Hide Gizmo / Reset View / Show Layers / Hide Layers
  - Preferences: 打开 PreferencesDialog
  - Calibration: Calibration 子菜单(连接到 Phase 7 的 9 个校准入口,Phase 2 仅占位)
  - Help: Documentation / Check for Updates / About / Shortcut Overview
- **工具按钮(锁定)**:Save / Undo / Redo / Calibration(快捷按钮);条件按钮 Account / ModelStore / Publish 在 v2.0 仅占位
- **CenteredTitle**:ApplicationWindow 水平居中显示当前项目名(从 ProjectServiceMock 读取),无项目时显示 "OWzx Slicer"
- **窗口控制按钮**:Min / Max / Close,使用 CxIconButton 自绘风格(Linux 风格在 v2.0 不实现,统一走 Win 风格)
- **平台条件(TOPBAR-07)**:`#ifdef Q_OS_MACOS` 走系统菜单栏(MenuBar),Win/Linux 走自绘 BBLTopbar;当前 Windows-only 构建,MACOS 分支预留但不激活

### 兼容与现有代码

- **现有 4-tab workflowTabs 数组**:替换为 9-tab TabBar 模型,保留 `pendingSwitchToken`/`pendingSwitchTargetPage` 用于 latency 跟踪
- **现有 CxMenu/CxMenuItem 基础设施**:复用,不重写
- **现有 fileMenu / topMenu**:扩展为完整 [File ▾] 和 [▾] 菜单
- **现有 page indices(pagePrepare=1, pagePreview=2, pageDevice=7, pageOnline=9)**:重新映射到新 TabPosition 枚举
- **`backend.topbarImportModel/topbarOpenProject/topbarSaveProject/topbarSaveProjectAs`**:保留这些 Q_INVOKABLE,扩展新的(topbarNewProject/topbarExportGcode 等)

### 国际化

- 所有新增菜单项使用 `qsTr()` 包裹
- 翻译键命名:`menu.file.new`、`menu.file.open`、`menu.edit.undo` 等

### 设计令牌

- 复用现有 Theme tokens(`topbarHover/topbarPressed/topbarText/accentColor`)
- BBLTopbar 高度:对齐上游 `BBLTopbar.cpp` 默认值(研究阶段确认具体像素值)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### 上游 OrcaSlicer 源码(功能真值)
- `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp` — 顶层框架,9 页 Notebook + 菜单栏 + 工具栏组装
- `third_party/OrcaSlicer/src/slic3r/GUI/Notebook.cpp` — TabPosition 枚举、tab 切换事件、side_tools 区
- `third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp` — 自绘标题栏 + 菜单 + 工具按钮
- `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp` — MainFrame 类接口、菜单项 ID
- `third_party/OrcaSlicer/src/slic3r/GUI/wxMediaCtrl2.h` — Tab 容器实现参考
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp` (side_tools 区在 Sidebar 嵌入,@1597)

### 当前 Qt6 代码(迁移基线)
- `src/qml_gui/main.qml` (1147 行) — 当前 4-tab workflowTabs + 部分 CxMenu 基础设施
- `src/qml_gui/controls/CxMenu.qml` / `CxMenuItem.qml` — 菜单控件基础设施(复用)
- `src/qml_gui/controls/CxIconButton.qml` — 工具按钮基础(复用)
- `src/qml_gui/BackendContext.h` — `topbarImportModel/topbarOpenProject/...` Q_INVOKABLE 定义

### 项目规则
- `.claude/rules/source-truth-migration.md` — 源码真值迁移规则
- `.claude/rules/build-rules.md` — 唯一构建命令
- `.claude/rules/qml-boundaries.md` — QML 不承载业务逻辑
- `.claude/rules/debugging.md` — QML 调试入口

</canonical_refs>

<specifics>
## Specific Ideas

- TabPosition 枚举值要严格对齐上游 `Notebook.cpp`,以便后续 Phase 3 Plater 共享实例能正确判断当前 view mode
- `[File ▾]` 的 Recent 子菜单需要支持动态项目(从 ProjectServiceMock 读取最近项目列表)
- BBLTopbar 工具按钮中的 Undo/Redo 必须直接连接 UndoRedoManager(通过 BackendContext 暴露)
- Calibration 快捷按钮在 Phase 2 仅作为入口占位,实际行为在 Phase 7 实现
- 占位 tab (TabPosition 7, 8) 在 v2.0 显示为禁用状态,tooltip 提示 "v2.1 实现"
</specifics>

<deferred>
## Deferred Ideas

- FilamentGroupPopup 完整多耗材分组切片逻辑 → v2.1
- Account / ModelStore / Publish 工具按钮完整实现 → v2.1(依赖 QtWebEngine / Network)
- macOS 系统菜单栏完整测试 → 跨平台构建支持后
- Linux 自绘窗口控制按钮风格 → 跨平台构建支持后
- Tab 拖拽重排 → 上游不支持,不实现
</deferred>

---

*Phase: 02-9-page-notebook-bbltopbar*
*Context gathered: 2026-06-16 via autonomous mode (defaults from ROADMAP.md)*
