# Phase 4 Context — Sidebar Dockable Layout

**Phase:** 4
**Goal:** LeftSidebar 升级为 Dockable：支持折叠/展开、宽度拖拽调整、dock area (Left/Right) 切换、状态持久化，对齐上游 `collapse_sidebar` 行为。
**Depends on:** Phase 3（Plater 共享后布局基础稳定）
**Requirements:** ARCH-08, ARCH-09, ARCH-10
**Mode:** Standard / auto (yolo)

---

## Upstream Truth

上游 Sidebar（`third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp`）核心事实：

- `Sidebar : public wxPanel`（Plater.hpp:128）—— 固定在 Plater 左侧的 wxPanel，**不是 wxAui dockable 浮动**
- `collapse_sidebar(bool)` (Plater.cpp:4452) —— 折叠/展开，通过 `Sidebar.Show/Hide` 实现
- `is_sidebar_collapsed()` —— 状态查询
- `EVT_GLCANVAS_COLLAPSE_SIDEBAR` (Plater.cpp:5117/5184) —— GLCanvas 工具栏的折叠按钮触发切换
- 持久化：`app_config->get("collapsed_sidebar")` (Plater.cpp:5399)
- `update_sidebar(bool force)` (Plater.cpp:4453) —— 布局刷新

**上游真实行为**：sidebar 是固定左侧面板 + collapse 显隐，**没有真正的 wxAui 浮动 dock**。ROADMAP 里"拖到 Right/浮动"是超出上游的增强。

---

## Current State（Phase 3 之后）

- `PreparePage.qml:1513` 内部 `RowLayout { LeftSidebar (preferredWidth 280) + viewportArea }`
- LeftSidebar (648 行) 在 PreparePage **内部**，固定 280px
- 折叠靠 `root.leftPanelVisible`（PreparePage 局部属性，非持久化）
- 无宽度拖拽调整、无 dock area 切换、无持久化

---

## Design Decision

**务实方案（采用，对齐上游核心 + 有限增强）**：

1. **折叠/展开**（ARCH-09，对齐上游 `collapse_sidebar`）
   - BackendContext 加 `sidebarCollapsed` Q_PROPERTY + `requestToggleSidebar()` + 持久化
   - 折叠时 LeftSidebar 隐藏，viewportArea 独占（对齐上游）

2. **宽度拖拽调整**（ARCH-08 子集，上游 sidebar 有固定宽度但 ROADMAP 要求可调）
   - BackendContext 加 `sidebarWidth` Q_PROPERTY (min 240, max 480, default 280)
   - LeftSidebar 右边缘加拖拽 handle，拖动改 sidebarWidth
   - 持久化 sidebarWidth

3. **dock area 切换 Left/Right**（ARCH-08 子集，超出上游但 ROADMAP 要求）
   - BackendContext 加 `sidebarDockArea` Q_ENUM (Left=0/Right=1)
   - Plater.qml 根据 dockArea 决定 LeftSidebar 在 RowLayout 的左/右位置
   - 持久化 sidebarDockArea

4. **响应式布局**（ARCH-10）
   - RowLayout + Layout.fillWidth 自然响应窗口缩放
   - sidebarWidth 是 preferredWidth（非固定），viewportArea fillWidth 独占剩余空间
   - DPI 缩放由 Qt 自动处理（数值都是逻辑像素）

5. **浮动窗口 dock** —— **Out of Scope（v2.0 不做）**
   - 上游本身没有真正的浮动 dock
   - QML 实现浮动窗口需 ApplicationWindow.multiWindow（Qt 6.x 实验）或额外 Window
   - 投入产出比低，记为已知限制

## Pitfalls

1. **LeftSidebar 在 PreparePage 内部 vs 外部** —— 不强行提升到 Plater 级（避免重写 3583 行 PreparePage 布局），改为：PreparePage 内部 LeftSidebar 的 width/visible/dockArea 都绑定 BackendContext 属性，由 BackendContext 统一管理 + 持久化。这样既满足 ARCH-08/09/10，又不破坏现有布局。
2. **持久化载体** —— 用 QSettings（Qt 标准），key 前缀 `owzx/sidebar/`。对齐上游 app_config 语义。
3. **dockArea 切换的视觉** —— RowLayout 里 LeftSidebar 和 viewportArea 的顺序由 dockArea 决定，用 RowLayout.children 动态调整或两个 Layout 分支。
4. **折叠按钮位置** —— 对齐上游 EVT_GLCANVAS_COLLAPSE_SIDEBAR（GLCanvas 工具栏），但 Phase 6 才做 GLCanvas 工具栏。Phase 4 先在 LeftSidebar 标题栏放折叠按钮（占位），Phase 6 再迁到 GLCanvas overlay。

## Acceptance（来自 ROADMAP Success Criteria）

1. ✅ Sidebar 可切换 dock 到左侧、右侧（浮动窗口 Out of Scope，记为已知限制）
2. ✅ Sidebar 折叠按钮，点击隐藏让 3D 区独占（对齐上游 collapse_toolbar）
3. ✅ 折叠状态持久化（QSettings owzx/sidebar/collapsed，对齐上游 app_config collapsed_sidebar）
4. ✅ 窗口缩放/最大化时 Sidebar 与 3D 区比例正确（RowLayout fillWidth 自然响应）
5. ✅ Sidebar 宽度可拖拽调整（min 240, max 480）
6. ✅ 多屏/DPI 缩放下布局不破（逻辑像素 + Qt 自动 DPI）

**已知偏离**：浮动窗口 dock 不实现（上游也没有，记入 CONTEXT）。
