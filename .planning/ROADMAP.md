# Roadmap: Milestone v2.0 — OrcaSlicer UI Architecture & Core Restoration

## Overview

把 Qt6/QML 项目的 UI 从 CrealityPrint 时代的 12 页 StackLayout 架构,全面迁移对齐到 OrcaSlicer 上游的 9 页 Notebook + Plater 共享实例 + Sidebar Dockable 布局。v2.0 聚焦 **P0 架构对齐 + P1 核心可见功能**;P2 高级页面留给 v2.1,P3 Dialog 补全与 i18n 留给 v2.2。

**Upstream reference:**

- 主框架: `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp`
- Plater: `.../GUI/Plater.cpp` (18601 行)
- 顶部栏: `.../GUI/BBLTopbar.cpp`
- Notebook: `.../GUI/Notebook.cpp`
- Sidebar: 嵌入 `Plater.cpp` (class Sidebar @1597)
- 参数面板: `.../GUI/ParamsPanel.cpp`

**Phase numbering mode:** Reset (从 Phase 01 开始,与 v1.x 完全分离)

---

## Phases

### Phase 1: OWzx Brand Cleanup

**Mode:** Standard
**Goal:** 移除所有 CrealityPrint 残留品牌字符串/资源/路径,统一为 OWzx,为后续架构重构扫清干扰。
**Depends on:** Nothing (前置清扫任务)
**Requirements:** BRAND-01, BRAND-02, BRAND-03, BRAND-04, BRAND-05
**Plans:** 3/3 plans complete
Plans:

- [x] 01-01-PLAN.md — QML UI 品牌字符串替换 (BRAND-01, BRAND-04) — DONE 2026-06-15
- [x] 01-02-PLAN.md — CMake 目标/选项重命名 + Crality3D 命名空间/CrealityGL 模块迁移 (BRAND-02, BRAND-03)
- [x] 01-03-PLAN.md — C++ 源字符串 + 子模块移除 + 版本对齐 + 验证脚本 (BRAND-01, BRAND-02, BRAND-04, BRAND-05)

**Success Criteria:**

  1. `grep -ri "creality" src/` 仅保留历史注释和 third_party 引用,无品牌字符串
  2. 应用启动后窗口标题显示 "OWzx",关于对话框显示 OWzx 品牌
  3. shortcut 一览对话框、tooltip、error message 全部 OWzx 化
  4. `third_party/CrealityPrint` submodule 引用从 `.gitmodules` 和 CMake 中移除
  5. 构建仍然通过 (`scripts/auto_verify_with_vcvars.ps1`)

### Phase 2: 9-Page Notebook + BBLTopbar

**Mode:** Standard
**Goal:** 重写 `main.qml` 顶层框架,从 12 页 StackLayout 迁移到上游 9 页 TabBar + StackLayout 架构,并完成 BBLTopbar 完整菜单系统。
**Depends on:** Phase 1 (品牌清理后框架重构)
**Requirements:** ARCH-01, ARCH-02, ARCH-03, ARCH-04, TOPBAR-01, TOPBAR-02, TOPBAR-03, TOPBAR-04, TOPBAR-05, TOPBAR-06, TOPBAR-07
**Plans:** 2/2 plans complete
Plans:
**Wave 1**

- [x] 02-01-PLAN.md — BackendContext TabPosition Q_ENUM + requestSelectTab signal/slot + unit tests (ARCH-02, ARCH-03)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 02-02-PLAN.md — BBLTopbar.qml + main.qml 9-page rewrite + Import/Export menus + side_tools + i18n (ARCH-01, ARCH-04, TOPBAR-01..07)

**Success Criteria:**

  1. `main.qml` 顶层为 TabBar + StackLayout,9 个 tab 按 Home/Prepare/Preview/Device/MultiDevice/Project/Calibration + 2 占位 顺序
  2. Tab 切换通过 `request_select_tab(TabPosition)` 事件广播,跨组件响应一致
  3. BBLTopbar 含 [File ▾]、[▾]、Save/Undo/Redo/Calibration 工具按钮、CenteredTitle、Min/Max/Close
  4. [File ▾] 下拉菜单完整 (New/Open/Recent/Save/Save As/Import 系列/Export 系列/Quit)
  5. [▾] 二级下拉含 Edit/View/Preferences/Calibration/Help 完整子菜单
  6. Notebook 右侧 side_tools 区有 Slice/Print 下拉按钮占位
  7. macOS 系统菜单栏 / Win+Linux BBLTopbar 条件分支生效

### Phase 3: Plater Shared Instance

**Mode:** Standard
**Goal:** 把 PreparePage 和 PreviewPage 合并为单一 `Plater.qml` 组件,通过 viewMode 切换 View3D↔Preview 内部视图,确保切换 tab 时 Plater 状态完整保留。
**Depends on:** Phase 2 (新框架提供 tab 切换基础设施)
**Requirements:** ARCH-05, ARCH-06, ARCH-07
**Plans:** 2/2 plans complete
Plans:

- [x] 03-01-PLAN.md — BackendContext ViewMode Q_ENUM + Plater.qml skeleton + unit tests (ARCH-06, ARCH-07 准备) — DONE 2026-06-17
- [x] 03-02-PLAN.md — Plater.qml wrapper (PreparePage + PreviewPage 常驻) + main.qml 单实例 StackLayout 重构 (ARCH-05, ARCH-06, ARCH-07) — DONE 2026-06-17

**Success Criteria:**

  1. `Plater.qml` 单一组件实例,被 Prepare tab 和 Preview tab 共享
  2. viewMode 属性 (View3D/Preview/AssembleView) 切换内部 wxAui-like 三选一显示
  3. 切换 Prepare→Preview→Prepare 后:对象列表、选择状态、切片结果、gizmo 状态全部保留
  4. 切换不触发 Plater 重建 (QML Loader 共享 + 属性切换,非 Loader source 切换)
  5. 切片完成后切到 Preview 自动加载 G-code 数据,无需手动刷新
  6. 单元测试:QSignalSpy 验证 viewMode 切换不触发 modelChanged

### Phase 4: Sidebar Dockable Layout

**Plans:** 2/2 plans complete
Plans:

- [x] 04-01-PLAN.md — BackendContext sidebar 三态 (collapsed/width/dockArea) + 持久化 + DockableSidebar 容器 + 4 单测 (ARCH-08, ARCH-09, ARCH-10 准备) — DONE 2026-06-17
- [x] 04-02-PLAN.md — PreparePage/Plater/main.qml 接入 DockableSidebar 三态透传链路 + 响应式布局 (ARCH-08, ARCH-09, ARCH-10) — DONE 2026-06-17

**Mode:** Standard
**Goal:** Sidebar 从固定 LeftSidebar (280px 死宽) 升级为 Dockable,支持拖动到 Left/Right/浮动/折叠,对齐上游 wxAui Dockable 行为。
**Depends on:** Phase 3 (Plater 共享后布局基础稳定)
**Requirements:** ARCH-08, ARCH-09, ARCH-10
**Success Criteria:**

  1. Sidebar 可拖动到窗口左侧、右侧、浮出窗口
  2. Sidebar 折叠按钮 (对齐上游 `collapse_toolbar`),点击隐藏让 3D 区独占
  3. 折叠状态持久化 (app_config 中 `sidebar_collapsed` 字段)
  4. 窗口缩放/最大化时 Sidebar 与 3D 区比例正确
  5. Sidebar 宽度可拖拽调整 (|min_width, max_width| 区间)
  6. 多屏/DPI 缩放下布局不破

### Phase 5: Prepare Sidebar Eight Sections

**Mode:** Standard
**Goal:** Sidebar 滚动区按上游八大区块顺序完整实现:Printer → Filament → Process 顶部条 → Search → ObjectList → ObjectSettings → ObjectLayers → ParamsPanel page_view。
**Depends on:** Phase 4 (Dockable 布局完成后填充内容)
**Requirements:** SIDEBAR-01 ~ SIDEBAR-15
**Success Criteria:**

  1. 八大区块按固定顺序布局 (Printer → Filament → Process 顶条 → Search → ObjectList → ObjectSettings → ObjectLayers → ParamsPanel)
  2. Printer 标题栏可折叠,内容含 preset combo + 喷嘴 + Bed type + extruders
  3. Filament 标题栏可折叠,双列耗材列表 + 颜色 + edit
  4. Process 顶部条含 SwitchButton(Global/Objects) + ModeIcon + ModeSwitchButton + Compare + Setting
  5. Global/Objects 作用域切换正确过滤参数列表
  6. Simple/Advanced 模式切换正确控制参数可见性
  7. Search bar 输入跳转到对应参数 (复用现有 SearchDialog)
  8. ObjectList 树完整 (对象/部件/设置/层),支持拖拽/右键/重命名
  9. ObjectSettings 在选中对象时显示快速设置,无选中时隐藏
  10. ObjectLayers 仅打印对象时显示变量层高编辑器
  11. ParamsPanel page_view 含 7 个子 Tab (Print/PrintPlate/PrintObject/PrintPart/PrintLayer/Filament/Printer)

### Phase 6: GLCanvas Toolbar System

**Mode:** Standard
**Goal:** 把上游 ImGui 在 GL canvas 上绘制的工具栏 (MainToolbar/Gizmos/ViewToolbar/Separator/Collapse) 用 QML overlay 等价实现,并接入 Plater.qml。
**Depends on:** Phase 3 (Plater 共享后工具栏挂载点稳定)
**Requirements:** GLUI-01 ~ GLUI-09
**Success Criteria:**

  1. MainToolbar (顶部 overlay): [+Add][+Plate][⟳Orient][⇲Arrange] | [More/Fewer][SplitOptions][SplitParts][LayersEditing]
  2. Gizmos 竖向条 (左侧 overlay): Move/Rotate/Scale/Flatten/Cut/MeshBoolean/FdmSupports/Seam/Emboss/Svg/Measure/Simplify
  3. ViewToolbar (右侧 overlay): Top/Front/Right/Back/ISO/Reset 视角预设
  4. SeparatorToolbar 分隔条 + CollapseToolbar 折叠按钮 (与 Sidebar 联动)
  5. Preview 模式下显示 IMSlider/IMToolbar (QML 浮层)
  6. 工具栏使用 QML overlay (非 ImGui GL 渲染),与 Theme 令牌一致
  7. PartPlateList (底部 overlay): 多板列表条 + [+Add] 按钮
  8. NotificationManager 浮层 (3D canvas 右上角),与 BackendContext 通知系统集成
  9. 工具栏按钮悬停 tooltip + 禁用态正确显示

### Phase 7: Calibration Menu & Dialogs

**Mode:** Standard
**Goal:** 实现 Calibration 顶级菜单 + 11 个校准 Dialog,完整对齐上游校准工作流。
**Depends on:** Phase 2 (BBLTopbar 提供菜单入口)
**Requirements:** CALIB-01 ~ CALIB-11
**Success Criteria:**

  1. Calibration 顶级菜单含 9 个子项 (Temperature/Max flowrate/Pressure advance/Flow ratio/Retraction/Cornering/Input Shaping Freq+Damp/VFA/Calibration Guide)
  2. PA_Calibration_Dlg 压力推进校准参数输入 + 启动
  3. FlowRateCalibrationDialog 含完整向导 (多步骤)
  4. Temp_Calibration_Dlg 温度校准
  5. MaxVolumetricSpeed_Test_Dlg 最大流量测试
  6. VFA_Test_Dlg VFA 测试
  7. Retraction_Test_Dlg 回抽测试
  8. Input_Shaping_Freq/Damp 两个输入整形测试 Dialog
  9. Cornering_Test_Dlg 拐角测试
  10. 校准历史持久化 (EditCalibrationHistoryDialog)
  11. CalibrationWizard 系列 (PA/Flow/MaxVS) 嵌入 CalibrationPanel

---

## Execution Order

```
Phase 1 (Brand Cleanup)
    ↓
Phase 2 (9-Page Notebook + BBLTopbar)
    ↓
Phase 3 (Plater Shared Instance) ───┐
    ↓                                │
Phase 4 (Sidebar Dockable)          │
    ↓                                │
Phase 5 (Sidebar 8 Sections)         │
                                     │
Phase 6 (GLCanvas Toolbars) ←────────┘
    ↓
Phase 7 (Calibration Menu + Dialogs) ←─ Phase 2 (BBLTopbar)
```

Phase 3 完成后,Phase 4/5/6 可部分并行 (各自独立组件工作),但建议按依赖顺序推进避免合并冲突。

---

## Out of Scope (Defer to v2.1 / v2.2)

### v2.1 — High-value Pages

- Home WebView (QtWebEngine)
- Device 双形态 (MonitorPanel + PrinterWebView)
- Multi-device 页
- Project 页 5 子 Tab
- ConfigWizard 完整版
- PreferencesDialog 完整版

### v2.2 — Dialogs & i18n

- AMS 系列 Dialog (~10 个)
- 预设管理 Dialog (SavePreset/DiffPreset/CreatePreset)
- Print/PrintHost Dialog
- 绑定/网络 Dialog
- 更新/隐私 Dialog
- i18n 21 语言接入

---

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. OWzx Brand Cleanup | 3/3 | Complete   | 2026-06-15 |
| 2. 9-Page Notebook + BBLTopbar | 2/2 | Complete   | 2026-06-16 |
| 3. Plater Shared Instance | 2/2 | Complete   | 2026-06-17 |
| 4. Sidebar Dockable Layout | 2/2 | Complete   | 2026-06-17 |
| 5. Prepare Sidebar 8 Sections | 0/0 | Pending | — |
| 6. GLCanvas Toolbar System | 0/0 | Pending | — |
| 7. Calibration Menu & Dialogs | 0/0 | Pending | — |

---
*Last updated: 2026-06-17 — Phase 4 complete (2 plans, 2 waves; sidebar dockable 三态 + 持久化)*
