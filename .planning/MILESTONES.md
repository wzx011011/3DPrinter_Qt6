# Milestones History

## v1.x Series — CrealityPrint Era (2026-05 to 2026-06-11)

历史里程碑基于 **CrealityPrint v7.0.1** 上游推进，完成了核心切片管线、基础 GUI 框架、CLI 入口。所有历史 artifacts 已归档至 `.planning-v1-crealityprint-archive/`。

### v1.0 — Initial Migration Foundation ✓

**Goal:** 把 CrealityPrint C++/wxWidgets 项目初始化为 Qt6/QML 项目骨架。

**Shipped:**
- CMake + Qt6.10 + QML 构建链路
- BackendContext composition root + 基础 ViewModel 注入
- Theme 设计令牌 + 基础 Cx* 控件
- GLViewport 3D 视口骨架

### v1.1 — End-to-End Slicing Workflow ✓

**Goal:** STL/3MF 导入 → 机型/耗材/工艺参数 → 切片 → G-code 导出 → Preview 完整闭环。

**Shipped:**
- 真实 3MF/BBS 3MF 导入导出主链路
- 真实切片与 G-code 导出（SliceService → libslic3r）
- G-code 预览渲染（GCodeRenderer，13 色彩模式）
- PreparePage/PreviewPage 完整工作流
- 通知系统（9 级 severity + Hint + 队列）

### v1.2 — Project Workflow & Preset System ✓

**Goal:** 项目工作流（保存/加载）+ 预设三段式继承链。

**Shipped:**
- 项目 3MF 完整保存/加载（含多平板、配置、缩略图）
- 真实上游 preset 加载（PresetServiceMock::loadVendorPresets）
- ConfigOptionDef 完整 schema + 配置继承链
- 14 个对话框骨架

### v1.3 — CLI Port (Partial)

**Goal:** 把上游 CrealityPrint CLI 移植到 Qt6，支持 headless 切片。

**Shipped:**
- `creality-cli.exe` CMake 目标 + CliRunner 编排器
- `--load`/`--slice`/`--load-settings`/`--output-dir` 参数支持
- 真实 preset 配置注入（printer→filament→print 三 tier 合并）

**Incomplete:**
- Phase 3: libslic3r Print::apply() 在 CLI 上下文偶发崩溃（GUI 上下文无此问题）
- Phase 4: 3MF 导出 + transforms（--arrange/--orient/--rotate/--scale）

**Status:** Blocked on libslic3r headless crash root cause; CLI 流程已通但偶发崩溃。

---

## v2.0 Series — OrcaSlicer Era (2026-06-12 onwards)

### v2.0 — OrcaSlicer UI Full Restoration (In Progress)

**Started:** 2026-06-11 (upstream switch from CrealityPrint to OrcaSlicer)

**Goal:** 把 Qt6/QML 项目的 UI 全面对齐到 OrcaSlicer 上游——架构、布局、菜单、工具栏、Dialog、品牌。

**Trigger:** 上游切换为 OrcaSlicer，CrealityPrint 时代的 12 页 StackLayout 架构与上游 9 页 Notebook + Plater 共享实例设计严重错位。

**See:** `.planning/ROADMAP.md` (待生成)
