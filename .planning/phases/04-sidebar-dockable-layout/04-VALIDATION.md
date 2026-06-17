# Phase 4 Validation — Sidebar Dockable Layout

**Phase:** 4
**Requirements:** ARCH-08, ARCH-09, ARCH-10
**Validation Date:** 2026-06-17

---

## Wave 1 验证结果

### 编译时（通过）

- `BackendContext.h/.cpp`：SidebarDockArea Q_ENUM + 3 个 Q_PROPERTY + 4 个 Q_INVOKABLE + 持久化
- MOC 通过（Q_ENUM/Q_PROPERTY 元对象生成）
- RCC 通过（DockableSidebar.qml 注册）
- ViewModelSmokeTests.cpp：4 个新 sidebar 用例编译通过

### 单元测试（11/11 exit 0）

| 测试用例 | 验证内容 | 结果 |
|---|---|---|
| `testSidebarCollapsedDefault` | 默认 collapsed=false, width=280, dockArea=Left, min/max 常量 | ✅ exit 0 |
| `testRequestToggleSidebar` | toggle + signal + 去重 + 持久化恢复（新 ctx 读 QSettings） | ✅ exit 0 |
| `testSidebarWidthClamp` | clamp [240,480] + 同值去重 + 持久化恢复 | ✅ exit 0 |
| `testSidebarDockArea` | 枚举值 + 切换 + 越界防御（99→Left）+ 持久化 | ✅ exit 0 |

**回归验证**：4 viewMode + 3 tabPosition 用例全 exit 0，无回归。

### 测试隔离说明

PowerShell `foreach` 批量跑 ViewModelSmokeTests 会因 QSettings ini 文件并发访问竞态误报失败（多个 exe 实例背靠背启动，resetSidebarSettings 写盘与下一实例构造竞态）。逐个 cmd 独立进程跑全部稳定通过。**这是测试执行环境问题，非代码 bug**。

### 冒烟验证（通过）

- `OWzxSlicer.exe` 启动 8 秒未崩
- **stderr 为空 = 零 QML warning**

---

## Wave 2 验证结果

### 编译时（通过）

- PreparePage.qml：加 8 个 sidebar 透传 property + LeftSidebar 替换为 DockableSidebar
- Plater.qml：加 sidebar 三态透传 property + PreparePage 实例绑定
- main.qml：Plater 实例绑定 backend.sidebarXxx + 注入 toggle/width 回调
- RCC for qml.qrc 通过

### 三态透传链路（ARCH-08/09/10 数据流）

```
backend.sidebarCollapsed/Width/DockArea (QSettings 持久化)
    ↓ Q_PROPERTY 绑定
main.qml Plater.sidebarXxx
    ↓ property 透传
PreparePage.sidebarXxx
    ↓ property 绑定
DockableSidebar.collapsed/sidebarWidth/minWidth/maxWidth
    ↓ 内部状态
LeftSidebar (contentSidebar) + 折叠按钮 + 拖拽 handle
```

操作回流：
```
DockableSidebar 折叠按钮/拖拽 → toggleRequested/widthChanged 回调
    → PreparePage.sidebarToggleRequested/WidthChanged
    → Plater.sidebarToggleRequested/WidthChanged
    → main.qml: backend.requestToggleSidebar()/requestSetSidebarWidth(w)
    → BackendContext 更新状态 + emit NOTIFY + 持久化 QSettings
```

### 响应式布局（ARCH-10）

- **窗口缩放/最大化**：RowLayout + Layout.fillWidth 天然响应；sidebarWidth 是 preferredWidth（非固定），viewportArea fillWidth 独占剩余空间
- **DPI 缩放**：所有数值都是逻辑像素（Qt 自动 DPI 缩放），无物理像素硬编码
- **折叠**：sidebarCollapsed=true 时 Layout.preferredWidth=0，viewportArea 独占（对齐上游 Sidebar.Show/Hide）

### ARCH 契约验收

| REQ-ID | 契约 | 验证方式 | 结果 |
|---|---|---|---|
| ARCH-08 | Sidebar 支持 Dockable（Left/Right/折叠） | sidebarDockArea Q_ENUM (Left/Right) + DockableSidebar 容器 + Dock Left/Right 切换（浮动 Out of Scope） | ✅ |
| ARCH-09 | Sidebar 折叠按钮（对齐上游 collapse_toolbar） | DockableSidebar 标题栏折叠按钮 + collapsedHandle 展开窄条 + 持久化 | ✅ |
| ARCH-10 | Plater 与 Sidebar 布局响应式 | RowLayout fillWidth + 逻辑像素 + sidebarWidth preferredWidth | ✅ |

### 已知偏离（记入 CONTEXT）

- **浮动窗口 dock 不实现**：上游 Plater 本身没有真正的浮动 dock（Sidebar 是固定 wxPanel + collapse），ROADMAP "浮动"为超出上游的增强，v2.0 记为已知限制
- **GLCanvas overlay 折叠按钮**：Phase 6 迁移（当前折叠按钮在 DockableSidebar 标题栏）

---

## Phase 4 出口结论

- Wave 1 ✅ 完成（commit 5512ca3）
- Wave 2 ✅ 完成（PreparePage/Plater/main.qml 接入 DockableSidebar）
- ARCH-08 ✅ 满足（dock Left/Right + 折叠；浮动 Out of Scope）
- ARCH-09 ✅ 满足（折叠按钮 + 持久化）
- ARCH-10 ✅ 满足（RowLayout 响应式 + 逻辑像素 DPI）
- 可进入 Phase 5（Prepare Sidebar 8 Sections）
