# Phase 4 Plan — Sidebar Dockable Layout

**Goal:** LeftSidebar 升级为 Dockable：折叠/展开 + 宽度拖拽 + dock area (Left/Right) 切换 + 持久化。

---

## Wave 1 — 04-01: BackendContext sidebar 状态 + DockableSidebar 骨架 + 单测

**Requirements:** ARCH-08 (dock area), ARCH-09 (collapse), ARCH-10 准备

### Tasks

1. **BackendContext sidebar 状态属性 + 持久化**
   - `Q_PROPERTY(bool sidebarCollapsed READ ... NOTIFY sidebarCollapsedChanged)`，默认 false
   - `Q_PROPERTY(int sidebarWidth READ ... NOTIFY sidebarWidthChanged)`，默认 280，clamp [240, 480]
   - `SidebarDockArea` Q_ENUM (Left=0/Right=1) + `Q_PROPERTY(int sidebarDockArea ...)`
   - `Q_INVOKABLE requestToggleSidebar()` / `requestSetSidebarWidth(int)` / `requestSetSidebarDockArea(int)`
   - 持久化：QSettings `owzx/sidebar/collapsed|width|dockArea`，构造时 load，setter 时 save
   - 常量 accessors: `sidebarMinWidth=240` / `sidebarMaxWidth=480` / `sdaLeft=0` / `sdaRight=1`

2. **DockableSidebar.qml 容器骨架**
   - 新建 `src/qml_gui/panels/DockableSidebar.qml`
   - 包装 LeftSidebar（contentItem）+ 折叠按钮（标题栏右侧）+ 右边缘拖拽 handle
   - required property: `editorVm`, `configVm`, `int dockArea`, `bool collapsed`, `int width`, `int minWidth`, `int maxWidth`
   - 折叠时高度/宽度收起（只剩一条 handle）
   - 拖拽 handle: MouseArea + drag.target，实时更新 width（clamp）
   - 注册 qml.qrc

3. **单元测试（ViewModelSmokeTests.cpp）**
   - `testSidebarCollapsedDefault` —— 默认 false
   - `testRequestToggleSidebar` —— toggle + signal + 持久化（QSettings 验证）
   - `testSidebarWidthClamp` —— setWidth(100)→240, setWidth(999)→480, setWidth(320)→320
   - `testSidebarDockArea` —— enum + requestSetSidebarDockArea + signal

4. **Wave 1 验证**
   - 增量构建通过（ninja OWzxSlicer.exe + ViewModelSmokeTests.exe）
   - ViewModelSmokeTests 新增 4 用例 + 原 7 用例全 exit 0

### Wave 1 出口

- BackendContext 暴露 sidebar 三态给 QML + 持久化
- DockableSidebar.qml 容器存在（包装 LeftSidebar + 折叠按钮 + 拖拽 handle）
- 单测覆盖三态 + 持久化

---

## Wave 2 — 04-02: PreparePage 接入 DockableSidebar + 响应式布局验证

**Requirements:** ARCH-08, ARCH-09, ARCH-10

**Depends on:** Wave 1 完成

### Tasks

1. **PreparePage 接入 DockableSidebar**
   - PreparePage.qml:1513 把 `LeftSidebar { ... }` 替换为 `DockableSidebar { ... }`
   - 绑定: collapsed←backend.sidebarCollapsed, width←backend.sidebarWidth, dockArea←backend.sidebarDockArea
   - 折叠时 DockableSidebar 宽度收为 0（或隐藏），viewportArea 独占
   - BBLTopbar 折叠按钮（Phase 2 已有 leftPanelVisible）接 backend.requestToggleSidebar()

2. **dock area Left/Right 切换布局**
   - PreparePage RowLayout 根据 dockArea 决定 DockableSidebar 在左/右
   - 用 RowLayout 的 Layout 排序或两个 Loader 分支
   - 切换 dockArea 时 sidebar 平滑移到另一侧（无重建）

3. **响应式布局验证（ARCH-10）**
   - 窗口缩放：RowLayout + Layout.fillWidth 自然响应
   - 最大化：同上
   - DPI 缩放：逻辑像素 + Qt 自动处理（数值不含物理像素）
   - sidebarWidth 是 preferredWidth，viewportArea fillWidth 独占剩余

4. **Wave 2 验证**
   - 完整构建通过
   - ViewModelSmokeTests 全过
   - QML 启动无 warning（冒烟 8s）
   - 手动 UAT 记录到 VERIFICATION（窗口缩放/折叠/拖宽/dock 切换）—— GUI 视觉部分留给用户

### Wave 2 出口

- PreparePage 用 DockableSidebar 替代固定 LeftSidebar
- sidebar 三态由 BackendContext 统一管理 + 持久化
- ARCH-08/09/10 全部满足（浮动 dock 记为已知限制）

---

## Risks

- **R1（中）**：dockArea 切换的 RowLayout 重排可能闪屏 → 用 Layout 属性绑定而非 Loader 重建
- **R2（低）**：拖拽 handle 实时更新 width 可能卡顿 → drag.filterChildren + drag.threshold
- **R3（低）**：QSettings 在测试环境的隔离 → 测试用临时 QSettings 或 reset

## Verification

- `ninja OWzxSlicer.exe` 增量构建
- ViewModelSmokeTests（新增 4 sidebar 用例）
- QML 启动无 warning（冒烟 8s）
- 手动 UAT（留给用户）：折叠/展开、拖宽、dock Left/Right、窗口缩放

## Out of Scope（Phase 4 不做）

- 浮动窗口 dock（上游也没有，记为已知限制）
- GLCanvas overlay 折叠按钮（Phase 6 做）
- Sidebar 八大区块内容（Phase 5 做）
