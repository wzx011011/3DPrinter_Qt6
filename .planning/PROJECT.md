# OWzx Slicer — OrcaSlicer Qt6/QML Migration

## What This Is

将开源 3D 打印切片软件 **OrcaSlicer**（C++/wxWidgets）迁移为 **C++/Qt6/QML** 架构，品牌名 **OWzx**。保留底层切片引擎（libslic3r）不变，重写整个 GUI 层，对齐上游 OrcaSlicer 全部用户可见行为和工作流。

## Core Value

上游 OrcaSlicer 源码为功能真值——Qt6 代码必须完整继承上游行为，不得自由设计新的产品行为。

## Previous Milestones (v1.x — CrealityPrint Era)

历史里程碑基于 CrealityPrint v7.0.1 上游推进，已完成核心切片管线和基础 GUI 框架。2026-06-11 起上游切换为 OrcaSlicer，v1.x 资产作为基线继续推进 UI 全面还原。

详见 `.planning-v1-crealityprint-archive/` 与 `MILESTONES.md`。

## Current Milestone: v2.6 v2.5 Remaining Completion

**Goal:** 完成 v2.5 的三块遗留——SSDP 设备发现 + Calibration 完整实现 + 摄像头视频流。

**Target features:**
- SSDP/mDNS 局域网设备自动发现（替代手动输入 IP）
- Calibration 完整实现（CalibPressureAdvanceLine + GCode 生成 + 结果回写）
- 摄像头视频流（FFmpeg 解码 + 实时显示 + 延时摄影）

**工作方式：** Phase 模式（沿用）

**Target features:**

- **架构对齐（P0 阻断）**：重写 `main.qml` 框架为 9 页 Notebook；Prepare 与 Preview 共享同一 Plater 实例；Sidebar 支持 Dockable（可拖 Left/Right/折叠）
- **品牌清理（P0）**：移除所有 Creality Print 残留品牌字符串、图标、关于对话框，统一为 OWzx
- **Prepare 页面八大区块（P1）**：Printer/Filament 折叠标题栏、Process 顶部条、Search、ObjectList、ObjectSettings、ObjectLayers、ParamsPanel page_view
- **BBLTopbar 重写（P1）**：[File ▾] 下拉、[▾] 二级菜单（Edit/View/Preferences/Calibration/Help）、CenteredTitle、Calibration 快捷按钮
- **GLCanvas 工具栏系统（P1）**：MainToolbar（导入/排布/拆分/层编辑）+ Gizmos 竖向条 + ViewToolbar（视角预设）+ Separator/Collapse
- **Calibration 菜单与 11 个校准 Dialog（P1）**：PA/Flow/Temp/MaxVFA/Retraction/IS/VFA/Cornering 向导
- **90+ Dialog 补全（P2-P3）**：AMS 系列、预设管理、Print/PrintHost、绑定/网络、更新/隐私
- **Home WebView（P2）**：QtWebEngine 替代 wxWebView（Makezilla 模型商店）
- **i18n 接入（P3）**：迁移 OrcaSlicer 21 种语言 .po 到 Qt .ts/.qm 体系
- **图标主题（P3）**：上游 `_dark` 后缀 dark/light 双态约定

## Requirements

### Validated

<!-- v1.x 已完成的真实资产，作为 v2.0 UI 还原的基线 -->

- ✓ CMake + Qt6.10 + QML 主入口与构建链路稳定可用 — `scripts/auto_verify_with_vcvars.ps1`
- ✓ BackendContext 单一 composition root + 10 ViewModel + Service 注入体系
- ✓ 共享设计令牌（Theme 21 token）+ 16 个 Cx* 基础控件（Button/ComboBox/Dialog/Menu/Slider/ScrollView 等）
- ✓ GLViewport (QQuickFramebufferObject) + GLViewportRenderer + CameraController 3D 视口桥接
- ✓ 3MF / BBS 3MF 导入主链路（`Model::read_from_archive`）+ 导出（`store_bbs_3mf`）
- ✓ 真实切片与 G-code 导出主链路（SliceService → libslic3r Print::process + export_gcode）
- ✓ G-code 预览渲染（GCodeRenderer，13 种颜色模式）
- ✓ 对象 CRUD + Volume 管理（addVolume/deleteVolume/Emboss/SVG/Primitive）
- ✓ Undo/Redo 系统（UndoRedoManager + 9 个 QUndoCommand 子类）
- ✓ 后台任务系统（JobBase/JobManager）
- ✓ 通知系统（9 级 severity + HintDatabase + 队列 + ErrorBanner/ErrorToast/NotificationCenter）
- ✓ 14 个对话框骨架（ConfigWizard/BedShape/EditGCode/AMS/Firmware/SpeedLimit/WipeTower/PrintHost/PluginManager/EnableLiteMode/Print/Calibration/CaliHistory/About）
- ✓ I18N 基础设施 + 6 语言 .ts 包
- ✓ Cut/镜像/arrange/orient/split 真实 libslic3r API
- ✓ 13 个 QML 页面（Home/Prepare/Preview/Monitor/Project/Calibration/Auxiliary/DeviceList/Preferences/ModelMall/MultiMachine/Settings/Config）
- ✓ CLI 入口（creality-cli.exe，支持 --load/--slice/--load-settings/--output-dir）

### Active

<!-- v2.0 milestone 要推进的需求，由后续 REQUIREMENTS.md 详细定义 REQ-IDs -->

参见 `.planning/REQUIREMENTS.md`

### Out of Scope

<!-- 上游真实路径受依赖阻塞的功能（与上游 v1.x 时代相同） -->

- TriangleSelector 真实集成 — 上游依赖 wxWidgets GL 交互，阻塞 SupportPaint/SeamPaint/MmuSegmentation per-triangle 绘制
- OpenVDB 集成 — 链接失败，阻塞 Hollow Gizmo 真实空洞几何生成
- FFmpeg/RTSP 视频流 — 未找到，阻塞 Monitor 页面真实摄像头视频流
- bambu_networking 真实连接 — 闭源，阻塞真实设备连接/云服务/MQTT 协议
- Shell 渲染 — 依赖上游 GCodeViewer shell 集成
- SLA 模块完整迁移 — GLGizmoSlaSupports 等上游 SLA 专用功能
- 移动 OrcaSlicer 上游未实现的功能（v7.0.1 之后新增）

## Context

- **上游真值源**：`third_party/OrcaSlicer` (main branch)，基于 C++ + wxWidgets + ImGui
- **上游 GUI 规模**：197 cpp + 197 hpp = 394 源文件（`src/slic3r/GUI/`），90+ Dialog 类，1085 个图标资源
- **上游核心架构**：`MainFrame` + `Notebook`（9 页）+ `Plater`（wxAui Dockable，Prepare/Preview 共享）+ `BBLTopbar`
- **迁移目标**：`src/` 下 Qt6 + QML 代码，libslic3r 引擎直接复用
- **迁移原则**：QML 仅负责呈现、组合和交互接线，不承载业务逻辑；业务逻辑在 core/services 和 core/viewmodels
- **构建验证**：`scripts/auto_verify_with_vcvars.ps1` 为权威完整验证路径
- **v1.x 资产**：~200+ 源文件、52 QML 文件、整体完成度约 65-70%
- **品牌名**：OWzx（统一替换所有 "Creality Print" 残留）

## Constraints

- **Tech Stack**: C++17 / Qt 6.10 / QML / CMake / Ninja / MSVC — Windows 10 构建环境
- **Upstream Lock**: 上游为 OrcaSlicer main branch（不再锁定具体版本，跟随上游）
- **Build Command**: 唯一构建命令 `scripts/auto_verify_with_vcvars.ps1`，唯一构建目录 `build/`
- **Architecture**: 业务逻辑在 core/，QML 仅做呈现，不得在 QML 内联脚本中承载业务逻辑
- **Dependency**: CGAL 可用（libslic3r_cgal）；OpenVDB/FFmpeg/bambu_networking 不可用
- **Platform**: 当前仅 Windows，上游同时支持 macOS/Linux
- **上游架构关键事实**：Prepare ≠ 独立页面，是 `Plater` 实例的视图模式之一（与 Preview 共享实例）

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 上游切换为 OrcaSlicer | OrcaSlicer 是更活跃的开源社区版本，BBL 系生态完整 | ✓ Good (2026-06-11) |
| Plater 共享实例（Prepare/Preview 同一组件） | 对齐上游 wxAui 设计，避免状态丢失 | ⏳ Pending (v2.0 P0) |
| QML overlay 代替 ImGui GL 工具栏 | Qt6 用 QML 浮层比 ImGui 渲染简单，且与 QML 主题一致 | ⏳ Pending (v2.0 P1) |
| QDockWidget 替代 wxAui Dockable | Qt 原生支持，避免引入第三方 docking 库 | ⏳ Pending (v2.0 P0) |
| 以上游源码为真值 | 防止迁移过程中自由发挥偏离上游行为 | ✓ Good (沿用 v1.x) |
| 保留 libslic3r 不变 | 切片引擎成熟稳定，重写风险极高 | ✓ Good (沿用 v1.x) |
| TriangleSelector/OpenVDB/FFmpeg 推迟 | 依赖链阻塞，当前无法突破 | — Deferred (沿用 v1.x) |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-17 — Milestone v2.0 (OrcaSlicer UI Full Restoration) 核心架构完成（Prepare 页 G1-G5 + GLToolbars）；启动 v2.1 (切片/预览深化)*

---

## Milestone v2.0 — Shipped (2026-06-17, 架构层)

v2.0 的**架构层**已完成，作为 v2.1 的地基：
- ✅ 9-Page Notebook + BBLTopbar 框架（Phase 1-2）
- ✅ Plater 共享实例（Prepare/Preview 共享，viewMode 联动，Phase 3）
- ✅ Sidebar Dockable（折叠/宽度/dockArea + 持久化，Phase 4）
- ✅ 八大区块骨架 + 卡片化（Phase 5 + G1-G3）
- ✅ GLToolbars overlay（MainToolbar/Gizmos竖条/ViewToolbar，G4）
- ✅ v1.x 残留清理（RightParamsPanel/topTools/viewPresets/processBar）

**v2.0 未完成的部分**（转入 v2.1 或更后）：
- Calibration 菜单/Dialog（需 CalibrationService 真实化 → 未来 milestone）
- 90+ Dialog 补全（分散到各 milestone）
- Home WebView（需 QtWebEngine，未来 milestone）
- i18n 21 语言（未来 milestone）

---

## Milestone v2.1 — Slice & Preview Deep Dive（当前，2026-06-17 启动）

**Goal:** 深化切片预览体验 + 完善预设管理。集中在 Prepare/Preview/Settings 已有页面深化，不碰 Mock 服务。

**Target features:**

- **Preview TickCode/IMSlider 系统**（核心）：G-code 着色模式切换（Feature/Speed/Extruder/LayerHeight/Pressure/Pixel）+ 自定义刻度插入（change filament/pause/custom gcode）
- **Preset 管理 Dialog 套件**：SavePresetDialog + UnsavedChangesDialog + ExportPresetBundleDialog
- **Prepare 页打磨收尾**：G6 BBLTopbar 样式 + G8 配色对比度（v2.0 遗留）
- **Settings Search 集成**：SearchDialog 接入 SettingsPage tier

**Out of Scope (v2.1 不做)**：
- Device/Cloud/Network 真实化（v2.2+）
- Calibration 真实化（v2.2+）
- AssembleView（v2.2+）
- ModelMall WebView（v2.2+）
- i18n 翻译内容（v2.2+）
