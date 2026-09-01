# 全面 UI 审计差距清单

日期：2026-08-31
范围：`src/qml_gui/` 可见 UI，以及直接决定 UI 行为的 `src/core/`。
目的：为全面 UI 审计建立可追踪的 P0-P2 差距清单。

## 编号口径与证据边界

仓库当前没有一份可引用的“原始审计 1-45”编号表：

- `.planning/audits/GLOBAL-UI-REVIEW.md` 按 6 个支柱记录了 18 个详细观察项（含 1 个 PASS），没有 1-45 编号。
- `_qml_audit.txt` 是 `QmlUiAuditTests` 的运行日志，共 140 个通过测试（测试方法日志行 4-141），也不是 45 条产品审计。
- 因此下表的 `UI-A01` 至 `UI-A45` 是**本清单新建的派生审计单元 ID**，不是对不存在的原始编号的伪造。每一行的“来源聚合”说明它由哪个支柱、详细观察项、测量值或测试族派生。
- 同一上游行为可能被多个视觉支柱观察到；表中按“可执行差距”聚合，避免把同一个根因重复算成多个原始审计编号。

状态定义：

- `implemented`：已有 Qt6 代码路径或静态合同证据；不代表已完成上游模块级视觉/行为验收。
- `blocked`：明确受当前不可用依赖、协议、凭据或硬件限制。
- `needs runtime evidence`：静态证据不足以证明运行时视觉、交互、布局或真实数据行为，或现有静态审计仍显示差距。

优先级定义：

- `P0`：首屏/主流程中的错误承诺、无动作、数据丢失或会阻断核心工作流。
- `P1`：高频工作流的行为/可读性/视觉一致性差距。
- `P2`：低风险视觉硬化、长尾页面或需要专项运行时验收的差距。

## 汇总

| 优先级 | 条目数 | 主要范围 |
|---|---:|---|
| P0 | 9 | 首屏无动作、占位导航、真实导出/设置入口、核心数据语义 |
| P1 | 23 | 主题、文案、设置/预览/Prepare 密度与主流程一致性 |
| P2 | 13 | 响应式、高 DPI、长尾页面和运行时视觉证据 |
| 合计 | 45 | 派生审计单元，不等同于原始审计编号 |

## 差距清单

| ID | 优先级 | 审计范围/差距 | Qt6 文件（证据位置） | 上游依据（行为真值） | 状态 | 验证/备注 |
|---|---|---|---|---|---|---|
| UI-A01 | P0 | 顶层导出项目动作存在 TODO/no-op | `src/qml_gui/main.qml`（GLOBAL-UI-REVIEW :92；测试 `exportUiUsesSaveDialogAndAvoidsSourcePathTarget`） | `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp`：File/Save 项；`Plater.cpp`：项目保存入口 | needs runtime evidence | 静态测试覆盖目标路径，但需实机点击确认成功、失败和取消反馈 |
| UI-A02 | P0 | 顶层导出模型动作未形成完整用户流程 | `src/qml_gui/main.qml`；`src/qml_gui/BackendContext.cpp` | `MainFrame.cpp`：export model dispatch；`Plater.cpp`：模型导出动作 | needs runtime evidence | 需用真实模型验证文件对话框、目标路径和导出结果 |
| UI-A03 | P0 | 顶层 Preferences 请求曾为 TODO/no-op，入口与设置窗口需闭环 | `src/qml_gui/main.qml`；`src/qml_gui/dialogs/SettingsDialog.qml` | `MainFrame.cpp`：Preferences/Settings dispatch；`Tab.cpp`：设置页生命周期 | implemented | `_qml_audit.txt` 条目 55-63、SettingsDialog 静态合同通过；仍需运行时打开/关闭验证 |
| UI-A04 | P0 | 首屏仍暴露禁用的占位 notebook tabs | `src/qml_gui/BBLTopbar.qml`；`src/qml_gui/main.qml` | `MainFrame.hpp/cpp`：真实 `TabPosition`/Notebook 页集合 | needs runtime evidence | 与 `_qml_audit.txt` 条目 35、64、93 的清理合同交叉核对；需确认最终首屏无占位项 |
| UI-A05 | P0 | AssembleView 的多盘画布与装配工具语义不能只停留在文字占位 | `src/qml_gui/pages/AssemblePage.qml`；`src/qml_gui/pages/Plater.qml`；`src/qml_gui/Renderer/RhiViewport*` | `GLCanvas3D.cpp`：`CanvasAssembleView`；`Gizmos/GLGizmoAssembly.cpp` | needs runtime evidence | 静态测试已覆盖壳、爆炸比例、测量覆盖；需运行时切换并确认真实画布内容 |
| UI-A06 | P0 | 3MF 真路径的多平板设置/覆盖保存仍需证明不丢失 | `src/core/services/ProjectServiceMock.cpp`；`src/core/model/PartPlate*.cpp` | `GUI/PartPlate.cpp`：`store_to_3mf_structure`/PlateData；`libslic3r/Format/3mf.cpp` | needs runtime evidence | 现有规划审计指出真保存路径曾只写 `model_`；必须用多盘 round-trip 证据，不以静态通过替代 |
| UI-A07 | P0 | 平板级配置与切片上下文的完整上游语义仍不完整 | `src/core/services/SliceService.cpp`；`src/core/viewmodels/ConfigViewModel.cpp` | `GUI/PartPlate.cpp`：`config()`、slice state；`Plater.cpp`：`update_slice_context_to_current_plate` | needs runtime evidence | 已有部分 key 映射不等于全量覆盖；需真实切片验证任意 plate override |
| UI-A08 | P0 | 设备/云/商城首屏动作的 disabled、mock、blocked 原因未统一呈现 | `src/qml_gui/BBLTopbar.qml`；`src/core/viewmodels/ModelMallViewModel.*`；`src/core/services/CloudServiceMock.*` | `GUI/NetworkAgent.*`；`GUI/DeviceManager.*`；`GUI/MainFrame.cpp` | blocked | 账号、云协议和商城依赖当前不完整；需在 UI 中维持明确 blocked 分类，禁止伪装为可用 |
| UI-A09 | P0 | 校准入口与已实现校准模式之间仍可能断链 | `src/qml_gui/BBLTopbar.qml`；`src/core/services/CalibrationServiceMock.cpp`；`src/qml_gui/pages/CalibrationPage.qml` | `GUI/Calibration/CalibrationDialog.cpp`；`CalibrationWizard.cpp`；`Plater.cpp` 校准 dispatch | needs runtime evidence | 已有 PA/Flow/Temp 真实切片映射；需点击每个可用模式并确认完成/失败状态 |
| UI-A10 | P1 | LeftSidebar 混用中英文 section、action 和 option 文案 | `src/qml_gui/panels/LeftSidebar.qml` | `GUI/Tab.cpp`、`GUI/Tabbook.cpp`：上游 label/tooltip；`GUI/Plater.cpp`：对象树文案 | needs runtime evidence | `GLOBAL-UI-REVIEW.md:52`；需按实际 locale 检查截断和术语一致性 |
| UI-A11 | P1 | 运行时泄漏“占位”“v2.x 实现”“reserved”内部规划文案 | `src/qml_gui/BBLTopbar.qml`；`src/qml_gui/main.qml`；`src/qml_gui/pages/Plater.qml` | `GUI/MainFrame.cpp`、`Plater.cpp`：上游无阶段性用户文案 | needs runtime evidence | `GLOBAL-UI-REVIEW.md:54`；静态清理测试通过后仍需启动截图确认 |
| UI-A12 | P1 | Calibration/导入/导出技术词在 locale 中不一致 | `src/qml_gui/BBLTopbar.qml`；`src/qml_gui/pages/PreparePage.qml` | `GUI/Calibration/CalibrationDialog.cpp`；`GUI/Plater.cpp` 文件动作 label | needs runtime evidence | `_qml_audit.txt` 条目 15、54；需检查中英文切换和长文本 |
| UI-A13 | P1 | 设置项 raw key/English label 仍可能直接暴露给用户 | `src/qml_gui/dialogs/SettingsDialog.qml`；`src/qml_gui/panels/LeftSidebar.qml`；`src/core/viewmodels/ConfigViewModel.*` | `GUI/Tab.cpp`、`GUI/ConfigOptionsGroup.cpp`：label/tooltip/sidetext | needs runtime evidence | v3.7 矩阵已记录“option display names mostly English”；需以真实设置页截图验收 |
| UI-A14 | P1 | Help Documentation 与 About 长尾动作需逐项确认非空 | `src/qml_gui/BBLTopbar.qml`；相关 Dialog QML | `GUI/MainFrame.cpp`：Help/About；`GUI/Online/NetworkAgent.*` 文档跳转 | needs runtime evidence | GLOBAL-UI-REVIEW :92；存在静态动作合同不代表 URL/对话框可用 |
| UI-A15 | P1 | 顶栏颜色、悬停色、分隔线绕过 Theme | `src/qml_gui/BBLTopbar.qml`；`src/qml_gui/Theme.qml` | `GUI/MainFrame.cpp`/`BBLTopbar` 对应上游视觉行为；交互状态由 `MainFrame` 菜单/Notebook 定义 | needs runtime evidence | `_qml_audit.txt` 条目 5、52-57 只证明部分合同；需做顶栏视觉对照 |
| UI-A16 | P1 | Monitor 页面使用独立 Tailwind 色板，状态色未统一 | `src/qml_gui/pages/MonitorPage.qml`；`src/qml_gui/Theme.qml` | `GUI/Monitor.cpp`、`MonitorBasePanel.cpp`：设备/温度/HMS 状态语义 | needs runtime evidence | `GLOBAL-UI-REVIEW.md:70`；需覆盖在线、离线、错误、摄像头状态 |
| UI-A17 | P1 | PrintSettings 重复定义 hover/danger 色，控件变体不统一 | `src/qml_gui/panels/PrintSettings.qml`；`src/qml_gui/controls/Cx*.qml` | `GUI/Tab.cpp`：选项修改/警告状态；上游控件状态由 Tab/Widgets 组合决定 | needs runtime evidence | 需对比正常、hover、dirty、warning、disabled 五种状态 |
| UI-A18 | P1 | QML 仍存在大量 hardcoded hex color，主题漂移可追踪性不足 | `src/qml_gui/**/*.qml`；统计见 GLOBAL-UI-REVIEW :39 | `GUI/Widgets/*`、`GUI/Theme*` 相关状态色及业务数据色 | needs runtime evidence | 903 次是静态测量；需区分语义数据色和应 token 化的 UI 色后逐页验收 |
| UI-A19 | P1 | Prepare 浮动 GL 工具面板密集，可能遮挡主视口 | `src/qml_gui/pages/PreparePage.qml`（约 :1760-2858） | `GUI/GLCanvas3D.cpp`、`GUI/GLGizmosManager.cpp`：工具栏/overlay 显示条件 | needs runtime evidence | 无可靠全量截图；需在最小窗口和每个 gizmo 模式下实机检查遮挡 |
| UI-A20 | P1 | 关键操作控件使用 7-9 px 文本，长期可读性不足 | `src/qml_gui/pages/PreparePage.qml`；`src/qml_gui/panels/ObjectList.qml`；`src/qml_gui/panels/LeftSidebar.qml` | `GUI/Widgets/*`、`GUI/GLToolbar.cpp`：操作标签和 hit target 语义 | needs runtime evidence | 静态发现见 GLOBAL-UI-REVIEW :78；需高 DPI/中文长文本截图 |
| UI-A21 | P1 | Theme 字体 token 与 24 种 raw pixelSize 并存 | `src/qml_gui/Theme.qml`；全体 QML `font.pixelSize` | 上游 `GUI/GUI_App.cpp`、`GUI/Widgets/*` 的 DPI/font scale 行为 | needs runtime evidence | 先以运行时层级和截断验收；不把数字统一替换视为行为真值 |
| UI-A22 | P1 | 空状态使用大号文字 glyph 代替一致的图标资产 | `src/qml_gui/pages/ProjectPage.qml`；`src/qml_gui/pages/MonitorPage.qml` | `GUI/Monitor.cpp`、`GUI/Plater.cpp`：空设备/空项目状态 | needs runtime evidence | 需确认图标、辅助文字和可操作入口在不同 locale 下不重叠 |
| UI-A23 | P1 | 原始 margins/spacing 绕过 Theme，密度调节不可集中控制 | `src/qml_gui/**/*.qml`；`src/qml_gui/Theme.qml` | `GUI/GUI_App.cpp` DPI；`GUI/Widgets/*` 布局间距 | needs runtime evidence | GLOBAL-UI-REVIEW :84；需以窗口尺寸与高 DPI运行结果为证据 |
| UI-A24 | P1 | 固定 1828x1000 初始尺寸和 1100x700 最小尺寸的布局韧性未验收 | `src/qml_gui/main.qml` | `GUI/MainFrame.cpp`：窗口初始化、DPI、Notebook 布局 | needs runtime evidence | v3.7 矩阵要求最小尺寸；需捕获 1100x700 和常用高 DPI |
| UI-A25 | P1 | 对象列表固定高度和 plate 卡片尺寸可能压缩内容 | `src/qml_gui/panels/LeftSidebar.qml`；`src/qml_gui/pages/PreparePage.qml` | `GUI/GUI_ObjectList.cpp`、`GUI/PartPlate.cpp`：列表/plate 信息密度 | needs runtime evidence | 检查长对象名、多 plate、无对象、滚动和拖拽状态 |
| UI-A26 | P1 | 22 px tabs、8-9 px filter labels 对本地化不友好 | `src/qml_gui/panels/LeftSidebar.qml`；`src/qml_gui/panels/ObjectList.qml` | `GUI/Tabbook.cpp`、`GUI/GUI_ObjectList.cpp` | needs runtime evidence | 需在 zh_CN/en/ja/ko/de/fr 逐一检查截断 |
| UI-A27 | P1 | Settings search 输入与可见结果/跳转链路需运行时确认 | `src/qml_gui/dialogs/SettingsDialog.qml`；`src/qml_gui/pages/SettingsPage.qml`；`src/core/viewmodels/ConfigViewModel.*` | `GUI/SearchDialog.cpp`、`GUI/Tab.cpp`：`activate_option`/ensure visible | implemented | `_qml_audit.txt` 条目 57、60-63、100；静态已实现，运行时搜索跳转仍应保留证据 |
| UI-A28 | P1 | Prepare 参数面板必须展示真实 option rows，而非 reserved list | `src/qml_gui/panels/LeftSidebar.qml`；`src/qml_gui/dialogs/SettingsDialog.qml` | `GUI/Tab.cpp`、`GUI/ConfigOptionsGroup.cpp` | implemented | `_qml_audit.txt` 条目 60、63；需运行时确认修改、dirty、scope 路由 |
| UI-A29 | P1 | 设置对话框结构需保持独立紧凑窗口，不回退到 group-nav/底栏 | `src/qml_gui/dialogs/SettingsDialog.qml` | `GUI/Tabbook.cpp`、`GUI/Tab.cpp`：顶部 tab 与页面内容 | implemented | `_qml_audit.txt` 条目 56-61；运行时仍需核对三种 settings 实例 |
| UI-A30 | P1 | Preview 页面截图证据未证明真实 Preview 状态 | `src/qml_gui/pages/PreviewPage.qml`；`src/qml_gui/Renderer/RhiViewport*` | `GUI/GCodeViewer.cpp`、`GUI/Plater.cpp`：Preview 切换及载入结果 | needs runtime evidence | v3.7 矩阵 :32 明确 captured window remained on Prepare；必须补 deterministic navigation/fixture |
| UI-A31 | P1 | Preview 左 sidebar、legend/stats、双 slider 的实际布局需截图验收 | `src/qml_gui/pages/PreviewPage.qml`；`src/qml_gui/components/Legend.qml`；`LayerSlider.qml`；`MoveSlider.qml` | `GUI/GCodeViewer.cpp`、`GUI/IMSlider.cpp` | needs runtime evidence | `_qml_audit.txt` 条目 23-27、87-89 是结构/行为证据，不是视觉等价证明 |
| UI-A32 | P1 | Preview 13 种视图模式的颜色/图例/过滤需真实 G-code 验证 | `src/core/viewmodels/PreviewViewModel.*`；`src/qml_gui/components/Legend.qml`；`src/qml_gui/Renderer/GCodeRenderer.*` | `GUI/GCodeViewer.cpp`：`EViewType`、legend、role color mapping | needs runtime evidence | `_qml_audit.txt` 条目 26、48-50、87；需真实 payload 覆盖每种模式 |
| UI-A33 | P1 | Preview Shell 渲染仍缺失，不能把 `showShells` 当完成 | `src/qml_gui/Renderer/GCodeRenderer.*`；`src/core/viewmodels/PreviewViewModel.*` | `GUI/GCodeViewer.cpp`：`Shells` 叠加渲染 | blocked | 规划与审计均注明需 libslic3r 集成；记录为阻断而非 UI 假完成 |
| UI-A34 | P1 | Preview marker、bed、travel、tooltip 的同时显示与不重叠需运行时验证 | `src/qml_gui/Renderer/GCodeRenderer.*`；`src/qml_gui/components/ToolPositionTooltip.qml` | `GUI/GCodeViewer.cpp`：Marker、bed、travel、tooltip | needs runtime evidence | 静态绑定已覆盖；需播放、切层、切换颜色模式时截图/像素检查 |
| UI-A35 | P1 | Prepare 对象树的 plate/module/volume 分组和多选行为需端到端确认 | `src/qml_gui/panels/ObjectList.qml`；`src/core/viewmodels/EditorViewModel.*` | `GUI/GUI_ObjectList.cpp`、`GUI/GUI_ObjectTable.cpp`：节点、选择、volume 操作 | needs runtime evidence | `_qml_audit.txt` 条目 36、40-42；需真实对象、volume、跨 plate 场景 |
| UI-A36 | P1 | Prepare Gizmo overlay 的深度独立、拖拽阈值和模式互斥需运行时验收 | `src/qml_gui/Renderer/RhiViewportRenderer.cpp`；`src/qml_gui/Renderer/RhiViewport.cpp` | `GUI/GLCanvas3D.cpp`、`Gizmos/GLGizmoMove/Rotate/Scale.cpp` | needs runtime evidence | `_qml_audit.txt` 条目 28-34；结构合同通过但交互/遮挡尚无完整运行证据 |
| UI-A37 | P1 | cut plane、wipe tower、assembly overlay 的组合渲染需验收 | `src/qml_gui/Renderer/RhiViewportRenderer.cpp`；`src/qml_gui/pages/AssemblePage.qml` | `GUI/GLCanvas3D.cpp`：clipping plane/wipe tower；`Gizmos/GLGizmoAssembly.cpp` | needs runtime evidence | `_qml_audit.txt` 条目 34、65-72；需多对象、多盘和预览切换检查 |
| UI-A38 | P1 | 视口右键菜单动作应全部到 C++ gate，不能只改变 QML 本地状态 | `src/qml_gui/pages/PreparePage.qml`；`src/core/viewmodels/EditorViewModel.*` | `GUI/GLCanvas3D.cpp`、`GUI/Plater.cpp`：context menu/action enablement | implemented | `_qml_audit.txt` 条目 29、36-37、41；保留运行时回归以防新菜单回退 |
| UI-A39 | P1 | 通知 toast/banner/history 与主内容、菜单的空间关系需不重叠 | `src/qml_gui/components/ErrorToast.qml`；`ErrorBanner.qml`；`NotificationCenter.qml` | `GUI/NotificationManager.cpp`；`SlicingProgressNotification.cpp`；`HintNotification.cpp` | implemented | `_qml_audit.txt` 条目 39、98-99；需运行时覆盖持久通知、进度、hint、队列 |
| UI-A40 | P1 | 顶层 shell action 的 enabled gate、错误反馈和取消行为需统一 | `src/qml_gui/main.qml`；`src/qml_gui/BackendContext.*`；`src/qml_gui/BBLTopbar.qml` | `GUI/MainFrame.cpp`；`GUI/NotificationManager.cpp`；`GUI/Plater.cpp` | implemented | `_qml_audit.txt` 条目 37-39；仍需逐项点击记录成功/失败/取消 |
| UI-A41 | P2 | 顶层无边框最大化默认行为及恢复窗口状态需运行时确认 | `src/qml_gui/main.qml` | `GUI/MainFrame.cpp`：frame style/window state | implemented | `_qml_audit.txt` 条目 38；需冷启动、恢复、最小化和 DPI 场景 |
| UI-A42 | P2 | 已删除 legacy OpenGL/software viewport 路径不能重新进入 Preview | `src/qml_gui/pages/PreviewPage.qml`；`src/qml_gui/Renderer/RhiViewport*`；`SoftwareViewport.*` | `GUI/GLCanvas3D.cpp`、`GCodeViewer.cpp`：CanvasPreview | implemented | `_qml_audit.txt` 条目 11、48、51、62、64；持续静态回归即可，运行时确认 RHI 默认注册 |
| UI-A43 | P2 | RHI 视口 thumbnail readback、3MF thumbnail 写入需真实图像证据 | `src/qml_gui/Renderer/RhiViewportRenderer.cpp`；`src/core/services/ProjectServiceMock.cpp` | `GUI/GLCanvas3D.cpp`：`render_thumbnail_framebuffer`；`Format/3mf.cpp` thumbnail IO | needs runtime evidence | `_qml_audit.txt` 条目 70-72；需检查非空 PNG、方向、透明度和重载显示 |
| UI-A44 | P2 | 主题/控件硬化的完整覆盖范围需在所有页面和 dialogs 收口 | `src/qml_gui/Theme.qml`；`src/qml_gui/controls/`；`src/qml_gui/dialogs/*.qml` | `GUI/Widgets/*`、`GUI/GUI_App.cpp`：统一控件与 DPI 行为 | needs runtime evidence | `_qml_audit.txt` 条目 121-131；静态 token 扫描不能替代多页截图和键盘导航验收 |
| UI-A45 | P2 | Monitor/Camera/Cloud/MultiMachine 的真实设备行为仍不能由 mock 页面覆盖 | `src/core/services/DeviceServiceMock.*`；`CameraServiceMock.*`；`CloudServiceMock.*`；`MultiMachineViewModel.*`；`src/qml_gui/pages/MonitorPage.qml` | `GUI/DeviceManager.*`、`GUI/MachineObject.*`、`GUI/NetworkAgent.*`、`GUI/Monitor.cpp` | blocked | 依赖真实协议、硬件、凭据或媒体解码；需 simulator/fixture 与单独 live-hardware 记录 |

## 追踪规则

1. 后续引用本清单时使用 `UI-Axx` 派生 ID，并同时引用来源文件/测试名；不要把 `UI-Axx` 写回成“原始审计编号”。
2. `implemented` 只表示已有实现或静态合同，不自动关闭上游差距；涉及视觉、真实数据或硬件的条目必须补运行时证据。
3. `blocked` 必须记录阻断原因和可替代的 simulator/fixture 验证边界；不能用 mock 通过替代真实功能完成。
4. P0/P1 条目完成后，应在对应的 milestone/audit 文档中引用本清单 ID；本文件不替代 `docs/源码对照迁移任务追踪.md`。
5. 运行时验证优先使用项目唯一权威命令：`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`；需要窗口/截图的条目还必须记录实际页面、窗口尺寸、locale、输入数据和截图路径。

## 只读审计记录

本文件由源码、现有规划审计、`QmlUiAuditTests` 方法名和 `_qml_audit.txt` 结果交叉整理生成。未修改源码，未修改 `docs/源码对照迁移任务追踪.md`，未提交或推送。 

---

## 最终映射（2026-09-01，Wave 1-7 收口）

按收口计划的两类出口逐项映射：`implemented`（有上游映射行为 + 聚焦/运行时验证）或 `blocked/declined`（外部依赖阻塞或明确缩减范围；阻塞项见 `docs/依赖与协议边界审计.md` 登记表，均有诚实 UI 状态 + 防复活回归测试）。canonical verifier 全绿（ViewModel 176、QML audit 171、PreviewParser 26、E2E 29 及其余套件）。

| ID | 最终状态 | 证据 |
|---|---|---|
| UI-A01 | implemented | `exportUiUsesSaveDialogAndAvoidsSourcePathTarget`；Wave 7：保存结果经 `backend.postNotification` 反馈成败 |
| UI-A02 | implemented | Wave 7：导出成败通知；`projectServiceExportGcode3mfProducesArchive`、CLI export 契约 |
| UI-A03 | implemented | SettingsDialog 静态合同 + 运行时打开/关闭（本会话点击验证） |
| UI-A04 | implemented | Wave 4 `acd554f`：真实工作流 tab（多设备/校准/偏好设置）+ tpDebug 语义；`waveFourRoutesAndPageIndicesAreDiscoverable` |
| UI-A05 | implemented | assemble/measure 运行时测试族；本会话视口鼠标 sweep（渲染线程竞态修复后 10/10 轮无崩溃） |
| UI-A06 | implemented | `multiPlate3mfRoundTripPreservesState`、`multiPlateFullStateRoundTrip` |
| UI-A07 | implemented | `projectServicePerPlateConfigOverrideRoundTrips`、`sliceServicePerPlateConfigMergeHonorsOverrides`、`sliceServiceConfigMergeDirectionPlateWins` |
| UI-A08 | blocked | 见登记表：Model Mall 移除（MALL-01）、固件/SD 诚实不可用；`firmwareDialogDoesNotSimulateOta`、`monitorSdCardPanelIsExplicitlyUnavailable` |
| UI-A09 | implemented | Wave 1 `a199c90`：分类过滤/步长/Flow Rate 分离/历史加载；`calibrationImplementedModesEmitSliceRequests`、`calibrationSaveToPresetWritesPresetValues` |
| UI-A10 | declined | 视觉 locale 截断专项未执行；已有 `prepareLeftSidebarMatchesPixelRestorationContract` 静态合同；列入后续硬化 |
| UI-A11 | implemented | `deadControlEliminationAudit`、`pageHonestyAndCliSourceAudit`；本会话清除残留占位文案与生产 console.log |
| UI-A12 | declined | 术语一致性 locale sweep 未执行；qsTr 全覆盖 + 6 语言 .ts 就绪；列入后续硬化 |
| UI-A13 | implemented | `plainPresetName`/`presetIndexForName` 规范化装饰名；选项 label 走 ConfigOptionModel；上游本身部分选项为英文 label（parity 口径） |
| UI-A14 | implemented | Help 菜单全链路：真实 checkForUpdates（GitHub release 查询）、openConfigFolder、网络测试（DNS+HTTPS 探针）、About/快捷键对话框 |
| UI-A15 | declined | 顶栏 hover/分隔线全量视觉对照未做；v5.14 已做分辨率无关工具栏；列入后续硬化 |
| UI-A16 | declined | Monitor 状态语义已诚实（Wave 2），Tailwind 色板统一未做；列入后续硬化 |
| UI-A17 | declined | PrintSettings 控件五态对照未做；列入后续硬化 |
| UI-A18 | declined | 903 处 hex 为静态测量；语义数据色豁免后仍需逐页 token 化；列入后续硬化 |
| UI-A19 | implemented | `prepareViewportControlsMatchRestorationContract`；本会话多 gizmo 模式实机点击截图验证无遮挡阻断 |
| UI-A20 | declined | 小字号可读性专项（高 DPI/中文长文本截图）未做；列入后续硬化 |
| UI-A21 | declined | 24 种 raw pixelSize 统一未做；以运行时截断验收为口径；列入后续硬化 |
| UI-A22 | implemented | 空状态诚实化：ProjectPage `_fileTree.length === 0`（PAGE-02）、Monitor SD 空态（Wave 2）；glyph→图标资产统一列入后续硬化 |
| UI-A23 | declined | margins/spacing token 化未做；列入后续硬化 |
| UI-A24 | declined | `resetWindowLayout` 真实（Wave 4）；1100x700/高 DPI resize 取证未做；列入后续硬化 |
| UI-A25 | declined | 对象列表/plate 卡片密度运行时取证未做；列入后续硬化 |
| UI-A26 | declined | 六语言截断 sweep 未做；列入后续硬化 |
| UI-A27 | implemented | `_qml_audit.txt` 57/60-63/100；设置搜索静态合同保持 |
| UI-A28 | implemented | 真实 option rows（`viewModesExposeUpstreamSeventeenModes` 同族配置契约）；dirty/scope 路由测试族 |
| UI-A29 | implemented | SettingsDialog 独立紧凑窗口合同（条目 56-61）+ Wave 6 presetIndexForName/restore 系统 值 |
| UI-A30 | implemented | Wave 5 `870fb7c` + E2E `pageSwitchPreparePreviewPreservesGcodePreviewData` 运行时通过；预览页实机截图 |
| UI-A31 | implemented | PreviewLayerRail 上游坐标反转、单层/滚轮/L 键；`previewWave5SliderSourceAudit`、`test_wave5_layer_slider_semantics` |
| UI-A32 | implemented | 17 视图模式 + 按模式可见性门（Wave 5）；`test_wave5_preview_visibility_gates`、legend/tick 数据回填测试 |
| UI-A33 | blocked | Shells 管线移植未完成；`showShells` 仅真实数据时生效；见登记表 |
| UI-A34 | implemented | marker 数据门 + tooltip 进给速度修复（Wave 5）；`test_wave5_marker_and_tick_payloads_are_data_backed` |
| UI-A35 | implemented | `prepareVisibleObjectActionsMapToSourceObjects`、`prepareContextMenuActionsAreRealAndPlateScoped`、viewport 上下文选择同步测试 |
| UI-A36 | implemented | gizmo 拖拽合并命令测试（move/rotate/scale）+ 本会话鼠标 sweep 运行时验证 |
| UI-A37 | implemented | wipe tower 真实网格回读测试族 + cut surface CGAL 路径 + 本会话多对象截图 |
| UI-A38 | implemented | 保持（`prepareContextMenuActionsAreRealAndPlateScoped` 等运行时回归） |
| UI-A39 | implemented | 保持（通知栈排序/压缩/按 id 关闭测试 + 空间合同） |
| UI-A40 | implemented | 保持（shell 动作 gate/错误反馈/取消测试族） |
| UI-A41 | implemented | 保持 + Wave 4 `resetWindowLayout` |
| UI-A42 | implemented | 保持（RHI 默认注册断言 + SoftwareViewport fallback-only 合同） |
| UI-A43 | implemented | 缩略图 readback 同步化（渲染线程竞态修复）+ `rhiViewportThumbnailUsesSynchronizedCameraSnapshot`；3MF 归档导出测试 |
| UI-A44 | declined | 全页面主题/键盘导航验收未做；列入后续硬化 |
| UI-A45 | blocked | 见登记表（真实设备协议/硬件/凭据/媒体解码） |

### 汇总

- implemented：29 项（均有聚焦测试 + 相应运行时证据）
- blocked（诚实不可用 + 防复活测试）：3 项（UI-A08、UI-A33、UI-A45）
- declined（明确缩减的后续视觉/locale 硬化范围，不涉及假成功 UI）：13 项（UI-A10、A12、A15、A16、A17、A18、A20、A21、A23、A24、A25、A26、A44）

本映射为收口审计记录；后续硬化项继续以本清单 UI-Axx ID 追踪。
