# CrealityPrint Qt6/QML Migration

## What This Is

将创想三维（Creality）开源 3D 打印切片软件 CrealityPrint v7.0.1 从 C++/wxWidgets 迁移为 C++/Qt6/QML 架构。保留底层切片引擎（libslic3r）不变，重写整个 GUI 层，对齐上游全部用户可见行为和工作流。项目面向创想三维打印机的桌面端用户。

## Core Value

上游 CrealityPrint 源码为功能真值——Qt6 代码必须完整继承上游行为，不得自由设计新的产品行为。

## Requirements

### Validated

<!-- 从现有代码推断的已完成能力 -->

- ✓ CMake + Qt6 + QML 主入口与构建链路稳定可用 — P0
- ✓ BackendContext、基础 ViewModel 注入、页面导航框架已建立 — P1
- ✓ 共享设计令牌与基础控件体系（Theme/CxButton/CxComboBox 等） — P1
- ✓ GLViewport/GLViewportRenderer/CameraController 3D 视口桥接骨架 — P2
- ✓ 3MF/BBS 3MF 导入主链路 — P0.5
- ✓ 真实切片与 G-code 导出主链路 — SliceService
- ✓ G-code 预览渲染主链路 — GCodeRenderer
- ✓ 对象变换桥接（ModelInstance 真实 API） — P0.5.1
- ✓ 网格实例变换（完整 Transform3d 管线） — P0.5.2
- ✓ 对象 CRUD 真实实现 — P0.5.3
- ✓ Volume 管理真实实现（addVolume/deleteVolume/Emboss/SVG/Primitive） — P0.5.4
- ✓ Undo/Redo 真实系统（UndoRedoManager + 9 个 QUndoCommand 子类） — P1.4
- ✓ 后台任务系统（JobBase/JobManager） — P1.5
- ✓ 主窗口导航、Edit/View/Help/Settings 菜单 — P1.1-P1.2
- ✓ 通知管理器（9 级通知 + HintDatabase + 进度条 + 队列） — P1.2
- ✓ ConfigWizard/BedShapeDialog/EditGCodeDialog/AMS/Firmware/SpeedLimit 等 6 个对话框 — P8
- ✓ I18N 基础设施 + 6 语言包 — P9
- ✓ 插件管理 + 企业模式 — P10
- ✓ Bed grid/wipe tower/GLShaderUtil 3D 渲染基础 — P2.8
- ✓ 真实 Cut（cut_mesh）/ 镜像（set_mirror）/ 自动排列（arrange_objects）/ 自动朝向（orient）/ 拆分（split） — P2.5-P2.6

### Active

<!-- 当前里程碑：将所有可推进的 [-] 任务推到 [x] -->

- [ ] Prepare 工作区完整对齐：平板管理真实耗材分配、对象树/侧栏细节、工具栏/侧栏视觉打磨
- [ ] 右侧面板端到端验证：切片结果摘要、参数面板三段式、排列设置全部在有真实数据时正确展示
- [ ] Preview 工作区完整对照：13 种颜色映射端到端验证、StatsPanel 真实 G-code 数据、LayerSlider/MoveSlider 交互验证
- [ ] Settings/Preset 完整继承链：上游 Tab 真实数据加载、PresetBundle 完整继承、ConfigOptionDef 完整 schema
- [ ] 后台切片状态机对照：BackgroundSlicingProcess 状态机与上游对齐
- [ ] 项目工作流完善：项目保存/加载完整工作流、文件导入格式支持对齐
- [ ] Gizmo GL 渲染层补全：Text/Emboss/SVG/Simplify/Drill/MeshBoolean/AdvancedCut 的 GL 交互渲染
- [ ] Support Paint / Seam Paint GL 渲染和数据管理层（非 TriangleSelector 方案）
- [ ] Hollow Gizmo GL 渲染和空洞几何生成（非 OpenVDB 方案）
- [ ] MmuSegmentation per-triangle 绘制（非 TriangleSelectorPatch 方案）
- [ ] 设备交互层对齐：DeviceManager/MachineObject 交互层状态机
- [ ] 校准工作流深度功能：设备连接/预设选择/真实参数编辑/历史记录
- [ ] 商城 WebView 集成：QtWebEngine 替代 wxWebView
- [ ] 多机管理真实连接：MQTT/SSDP 协议层
- [ ] 发布就绪度：上游行为对照回归基线、全页面回归测试自动化、I18N 完整验证

### Out of Scope

<!-- 推迟到后续里程碑：全局阻塞项 -->

- Plate 管理真实 PartPlateList 集成 — 上游 PartPlateList 依赖 wxWidgets/OpenGL，需架构层面 Qt6 原生替代方案
- TriangleSelector 真实集成 — 上游 TriangleSelector 依赖 wxWidgets GL 交互，阻塞 SupportPaint/SeamPaint/MmuSegmentation 的 per-triangle 绘制
- OpenVDB 集成 — 链接失败，阻塞 Hollow Gizmo 真实空洞几何生成
- FFmpeg/RTSP 视频流 — 未找到，阻塞 Monitor 页面真实摄像头视频流
- bambu_networking 真实连接 — 闭源，阻塞真实设备连接/云服务/MQTT 协议
- Shell 渲染 — 需 libslic3r GCodeViewer shell 集成，依赖上游渲染管线重构
- SLA 模块完整迁移 — GLGizmoSlaSupports 等上游 SLA 专用功能
- FaceDetector 真实实现 — 上游已注释，功能不活跃
- BBL 扩展通知类型 — BBL 专有功能，非 Creality 范围

## Context

- **上游真值源**：`third_party/CrealityPrint` v7.0.1（commit 0d4ac73），基于 C++ + wxWidgets
- **迁移目标**：`src/` 下 Qt6 + QML 代码，libslic3r 引擎直接复用
- **迁移原则**：QML 仅负责呈现、组合和交互接线，不承载业务逻辑；业务逻辑在 core/services 和 core/viewmodels
- **构建验证**：`scripts/auto_verify_with_vcvars.ps1` 为权威完整验证路径，当前 269/269 编译、66/66 smoke test
- **已有文档**：`docs/源码对照迁移任务追踪.md`（P0-P7 任务追踪）、`docs/CrealityPrint_Qt_GUI重写架构.md`（架构文档）、`docs/项目结构.md`（模块边界）
- **19 个上游 Gizmo**：Text/Emboss/SVG/Simplify/Cut/AdvancedCut/MeshBoolean/Drill 已接入真实 API，但 GL 交互渲染层缺失
- **代码库规模**：~200+ 源文件，覆盖 15+ 页面、20+ 组件、10+ Service/ViewModel

## Constraints

- **Tech Stack**: C++17 / Qt 6.10 / QML / CMake / Ninja / MSVC — Windows 10 构建环境
- **Upstream Lock**: 上游锁定为 v7.0.1，不得自由设计新行为
- **Build Command**: 唯一构建命令 `scripts/auto_verify_with_vcvars.ps1`，唯一构建目录 `build/`
- **Architecture**: 业务逻辑在 core/，QML 仅做呈现，不得在 QML 内联脚本中承载业务逻辑
- **Dependency**: CGAL 可用（libslic3r_cgal），OpenVDB 不可用，FFmpeg 不可用
- **Platform**: 当前仅 Windows，上游同时支持 macOS/Linux

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| 以上游源码为真值 | 防止迁移过程中自由发挥偏离上游行为 | ✓ Good |
| 保留 libslic3r 不变 | 切片引擎成熟稳定，重写风险极高 | ✓ Good |
| Mock 平行数组过渡 | 逐步替换为真实 API，保持构建稳定 | ✓ Good |
| QML 不承载业务逻辑 | 遵循 Qt6 最佳实践，C++ ViewModel 驱动 | ✓ Good |
| TriangleSelector/OpenVDB/FFmpeg 推迟 | 依赖链阻塞，当前无法突破 | — Deferred |
| Gizmo 真实 API 优先于 GL 渲染 | 先打通后端能力，再补全视觉交互 | ⚠️ Revisit |

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
*Last updated: 2026-05-31 after initialization*
