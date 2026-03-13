# CrealityPrint Qt6 — 源码对照迁移任务追踪

> 更新日期：2026-03-10
> 新口径：以上游 `third_party/CrealityPrint` 源码为功能真值；当前 Qt6 工程是承接层，不再使用“阶段骨架已完成 = 全功能完成”的记账方式。
> 本文用于追踪“功能承接度”，不是追踪“文件是否创建过”。

---

## 0. 当前结论

- 当前工程已经具备可运行的 Qt6/QML 主壳、基础组件、3D 视口桥接、3MF 导入、切片链路、G-code 预览链路的基础承接能力。
- 但距离“基于 CrealityPrint 7.x 开源源码完整还原功能”仍有明显差距，主要差距在：
  - Prepare 工作区的对象树、平板、Gizmo、工具架、业务动作仍未完整对齐上游
  - Preview 页面虽然已打通链路，但交互、统计、告警与细节行为还未完成对照验收
  - Settings / Preset 体系目前仍偏向动态 Mock 模型，尚未完整承接上游 `Tab`、`PresetBundle`、多作用域覆盖
  - Monitor / Device / AMS / 视频 / 云账号 / Model Mall / MultiMachine 仍属于骨架或局部占位
- 因此，旧文档中的“70/70 全部完成”不再作为真实进度口径。

---

## 1. 进度记账规则

- `[x]`：已与上游功能真值完成对照，具备可用能力，并通过回归或端到端验证。
- `[-]`：已完成基础承接或局部闭环，但尚未完成上游对照和完整验收。
- `[ ]`：尚未开始，或仅有占位 UI / Mock 能力。

---

## 2. 已完成的基础承接

以下事项已经完成，且可作为后续全功能迁移的基础：

- [x] CMake + Qt6 + QML 主入口与构建链路稳定可用
- [x] `BackendContext`、基础 ViewModel 注入、页面导航框架已建立
- [x] 共享设计令牌与基础控件体系已建立：`Theme`、`CxButton`、`CxComboBox`、`CxSlider`、`CxIconButton`、`CxPillAction`、`CxPanel` 等
- [x] `GLViewport`、`GLViewportRenderer`、`CameraController` 3D 视口桥接骨架可运行
- [x] `ProjectService` 已打通 3MF / BBS 3MF 导入主链路
- [x] `SliceService` 已打通真实切片与 G-code 导出主链路
- [x] `GCodeRenderer` 已具备真实 G-code 预览渲染主链路
- [x] `PreviewViewModel` 已可消费切片结果并驱动预览页面基础数据
- [x] 程序当前可构建并成功启动，`scripts/auto_verify_with_vcvars.ps1` 仍是权威完整验证路径

---

## 3. 真实功能迁移状态

### P0 — 真值基线与迁移框架

- [x] P0.1 锁定上游功能真值版本（tag / commit）2026-03-10
  - 基线文档：[docs/SOURCE_TRUTH_BASELINE.md](SOURCE_TRUTH_BASELINE.md)
  - 锁定结果：`third_party/CrealityPrint` `v7.0.1` / `0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557`
- [x] P0.2 建立“上游模块 -> Qt6 模块 -> 当前状态 -> 验收方式”功能矩阵 2026-03-10
  - 功能矩阵文档：[docs/FEATURE_MATRIX.md](FEATURE_MATRIX.md)
  - 说明：已建立统一执行入口，后续按矩阵继续细化子任务和验收项
- [x] P0.3 梳理闭源依赖、动态库、云服务与协议边界 2026-03-10
  - 审计文档：[docs/DEPENDENCY_BOUNDARY_AUDIT.md](DEPENDENCY_BOUNDARY_AUDIT.md)
  - 说明：已识别 `bambu_networking`、WebView2、MQTT/SSDP/HTTP、登录/商城、媒体播放等关键边界
- [x] P0.4 将任务追踪切换为“功能承接度”口径

### P1 — 主框架与全局工作流

- [-] P1.1 顶栏导航、页面切换、文件入口、最近文件基础闭环已存在
- [-] P1.2 已新增 Edit 菜单（全选/取消选择/删除选中/清空全部）、扩展"更多"菜单加入"适应视图"和"线框模式"快捷入口、新增 Ctrl+I 导入模型和 Ctrl+O 打开项目全局快捷键；原有的 File 菜单已包含新建/打开/导入/保存/另存为/最近文件/退出，工具栏已含保存/撤销/重做按钮和页面 Tab 切换；线框模式已完整实现：GLViewport 新增 wireframeMode 属性，GLViewportRenderer 使用 glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) + DARK_GRAY 颜色渲染，匹配上游 Plater::toggle_show_wireframe 行为；但仍缺上游 View 菜单完整项（相机预设视图、标签显示）、缺 Edit 菜单完整项（剪切/复制/粘贴/克隆）、缺 Help 菜单（快捷键对话框/关于对话框/配置目录）、缺全局弹窗体系和通知管理器 2026-03-13
- [ ] P1.3 需要建立统一的上游 GUI 迁移适配层，禁止页面逻辑直接散落在 QML 内联脚本中

### P2 — Prepare 工作区

- [-] P2.1 已具备模型导入、视口显示、基础对象列表、平板切换、切片入口
- [-] P2.2 已完成准备页顶部工具条和左右侧栏的第一轮视觉重构
- [-] P2.3 左侧对象工作区已补入 plate 卡片切换、对象所属 plate 信息、基础右键动作，以及基于 ViewModel 的多选与批量操作；对象列表中的状态切换已从本地假显隐改为承接上游 `printable` 语义，并已支持按平板 / 按模块两种组织方式、分组头展开/收起，以及对象节点下的 volume 子层级展示；当前还新增了 volume 子项的同对象内多选、高亮、批量删除，并让删除快捷键与顶部工具栏删除动作跟随当前选中节点，同时加入了基于上游规则的基础删除约束；此外已补入与上游 `Edit in Parameter Table` 对应的单对象 / 单部件参数表入口，并让右侧打印参数面板开始对部分上游真实 key 承接对象 / 部件 scope 的读取与写回；本轮进一步把 `initial_layer_print_height`、`infill_wall_overlap`、`chamber_temperature` 等真实 key 纳入 Qt6 参数表承接，但仍未完成上游完整上下文动作集、完整对象 / volume 参数覆盖范围和全量树行为对照 2026-03-11
- [-] P2.4 右侧工作区已开始承接真实切片结果摘要：Qt6 右侧切片面板现在会基于真实切片结果显示预计打印时长、耗材重量、当前 plate 标签，并在项目对象发生变化后主动清空失效切片结果，避免继续展示旧摘要；本轮进一步把默认切片动作从“重新按源文件切整个项目”收敛为“按当前 plate 的内存对象子集切片”，使对象 printable 与对象 / 部件 scope 的内存修改能够进入当前 plate 切片结果，同时切片结果 plate 归属固定到切片启动时的目标 plate；此外，切换到其他 plate 时不再继续展示前一 plate 的切片摘要，当前 plate 已有有效切片结果时也会按上游 `MainFrame::get_enable_slice_status()` 的 `eSlicePlate` 规则禁用再次切片，同时切片入口会按当前 plate 是否存在可打印对象给出基础 enable/disable 提示；但切全部板动作、完整 plate 级切片状态、打印参数全量承接与完整右侧业务流仍未按上游 `Plater` / `SliceInfoPanel` 完整迁移 2026-03-11
- [-] P2.5 已实现 Move/Rotate/Scale 三模式 Gizmo 切换系统和线框模式：GLViewport 新增 gizmoMode 属性支持 Move/Rotate/Scale 三种模式切换，GLViewportRenderer 新增旋转环面几何体（torus rings）和缩放立方体手柄几何体，Mesh shader 升级为 mat4 模型矩阵支持完整的平移+旋转+缩放变换，对象拾取更新为变换后 AABB 检测，undo/redo 支持完整 TransformCmd 记录，PreparePage 工具栏新增移动/旋转/缩放模式按钮；线框模式通过"更多"菜单切换，使用 glPolygonMode + DARK_GRAY 颜色匹配上游 Plater::toggle_show_wireframe；但仍未完成上游完整 Gizmo 集合（Flatten/Cut/Hollow/Measure 等）、自动摆放（arrange）、自动朝向（orient）、裁切（cut）和视口告警显示 2026-03-13
- [-] P2.6 已新增上游对齐的键盘快捷键（W/E/R 切换 Gizmo 模式、Ctrl+Z/Ctrl+Y undo/redo、Delete 删除选中、Escape 取消选择、Ctrl+A 全选、F 适应视图）和对象右键上下文菜单（删除、全选、取消选择、Gizmo 模式切换），并通过 QML Shortcut 确保快捷键在焦点管理下可靠触发；已新增 Slice All（切片全部平板）和 Export G-code（导出 G-code 文件对话框）功能：SliceService 新增 startSlicePlate(int) 和 exportGCodeToPath(QString)，EditorViewModel 新增 requestSliceAll() 支持多平板顺序切片队列和 requestExportGCode(QString)，PreparePage 底部新增导出 G-code 和切片全部平板按钮；但仍未完成上游完整 Plater 工作流对照：缺全局菜单栏（文件/编辑/视图/设置）、缺项目保存/加载、缺自动排列（arrange）和自动朝向（orient）、缺镜像/拆分/复制粘贴等对象操作、缺完整切片状态指示器、缺 plate 级管理的完整上下文菜单 2026-03-13

### P3 — Preview 工作区

- [-] P3.1 已打通切片结果到预览页面的数据通路
- [-] P3.2 已具备 LayerSlider、MoveSlider、Legend、StatsPanel 等基础组件
- [-] P3.3 已完成全部 13 种 EViewType 的数据源和颜色映射：FeatureType(固定分类色)/Height(Z渐变)/Width(解析WIDTH注释,渐变)/Tool(挤出机色板)/Feedrate(渐变)/FanSpeed(渐变)/Temperature(渐变)/ColorPrint(挤出机色板)/FilamentId(挤出机色板)/VolumetricRate(feedrate*width渐变)/LayerTime(TIME_ELAPSED渐变)/LayerTimeLog(log渐变)/Acceleration(M204渐变)；G-code解析器已新增WIDTH注释、TIME_ELAPSED注释、T-指令(工具切换)、M204(加速度)的解析；PackedSegment已扩展width/layer_time/acceleration/extruder_id字段并同步GCodeRenderer二进制布局；recolorAndPackSegments()支持按模式动态着色并重建二进制数据；buildLegendItems()支持分类图例、挤出机图例和渐变范围图例三种图例类型；但仍需与上游GCodeViewer的渐变色阶(Range_Colors)和统计面板深度对照验收 2026-03-13
- [-] P3.4 已修复 GCodeRenderer 多项运行时问题：补入缺失的 `initializeOpenGLFunctions()` 调用防止 GL 函数未就绪、补入 shader link 错误日志输出、消除 render() 中每帧无条件调用 update() 导致的不必要重绘循环、将顶点过滤从双遍历（先计数再过滤）改为单遍历提升性能；PreviewPage.qml 本身无 QML 绑定警告；已新增 CameraController 集成支持预览视口的轨道/缩放/平移相机控制（左键拖拽轨道、中键拖拽平移、滚轮缩放），并自动计算数据包围盒适配初始视角；已新增相机预设视角按钮（顶/前/右/等轴），CameraController 新增 viewTop/viewFront/viewRight/viewIso 方法，GLViewport 新增 requestViewPreset() 接口和 ViewPreset 事件类型，GLViewportRenderer 和 GCodeRenderer 均已处理 ViewPreset 事件；但仍缺上游 GCodeViewer 的完整视口交互（如拾取点高亮、视口内层范围拖拽、鼠标悬浮提示等） 2026-03-13
- [-] P3.5 已实现视图模式切换时的动态颜色重计算：PreviewViewModel 将解析后的段存储为成员数据，setViewModeIndex() 触发 recolorAndPackSegments() 重新着色并重建二进制数据；图例已根据视图模式分为 FeatureType 分类图例、挤出机色板图例和渐变范围图例；LayerTime/LayerTimeLog/Width/Tool/Acceleration 数据源已在 P3.3 中补充；渐变色阶已从 4 段数学插值替换为上游 GCodeViewer 的 10 色 Range_Colors（#0b2c7a→#135985→#1c8891→#04d60f→#aaf200→#fcf903→#f5ce0a→#d16830→#c2523c→#942616），并采用相同的 lerp 插值逻辑（global_t 映射到 [0,9] 区间后对相邻色值线性插值）；图例端点颜色也更新为上游首尾色；统计面板 StatsPanel 已扩展为显示总时间、层数、总移动数、挤出移动数、空驶移动数、耗材长度（从 E 轴累计值计算）和耗材重量（从 volume × density 计算）；G-code 解析器已新增 filament_diameter 和 filament_density 注释解析；仍缺上游完整 PrintEstimatedStatistics（按角色时间分解、多挤出机体积分解、成本计算）和自定义时间模式的完整对照 2026-03-13

### P4 — Settings / Preset / 覆盖作用域

- [-] P4.1 `ConfigViewModel`、`ConfigOptionModel`、`PresetListModel`、`SettingsPage` 已有动态配置骨架
- [-] P4.2 `PrintSettings.qml` 已完成一轮参考图导向重构
- [-] P4.3 当前参数体系仍以 Qt 侧模型为主，尚未完整承接上游 `Tab`、`ConfigOptionsGroup`、`PresetBundle`；已新增 `ConfigOptionModel::loadFromUpstreamSchema()` 从上游 `print_config_def` 读取真实配置定义替代硬编码选项列表，支持自动提取 option key/label/type/category/min/max/enum/default 值，并通过上游 category 到 Qt6 显示类别的映射保持 UI 兼容；上游 schema 加载覆盖已从 30 个 key 扩展到约 60 个 key，覆盖层高/壳/填充/速度/温度/支撑/底座/冷却/回退/接缝/输出/挤出机/熨烫等主要分类；category 映射新增底座(Adhesion)、输出(Output/G-code)、质量(Ironing/Seam)、挤出机 等分类；但尚未完整迁移上游 `Tab` 的 Page/Group 层级结构、`PresetBundle` 的预设继承/兼容性/脏标记/搜索、以及多作用域覆盖的完整语义 2026-03-13
- [ ] P4.4 对象、部件、平板、层范围覆盖等上游作用域尚未完整迁移
- [ ] P4.5 预设继承、dirty 状态、搜索、重置、帮助文案仍需按上游做功能对照

### P5 — Device / Monitor / Network

- [-] P5.1 Monitor 页已有可运行页面与基础布局
- [ ] P5.2 设备发现、连接、实时状态、控制命令、AMS、多灯光/录像/AI 能力尚未完成对照迁移
- [ ] P5.3 视频流 / 摄像头方案尚未确定为最终架构
- [ ] P5.4 云账号、设备绑定、服务端同步仍未进入真实迁移阶段

### P6 — Calibration / Project / Auxiliary / Model Mall / MultiMachine

- [-] P6.1 Project、Calibration、Preferences 等页面具备基础入口
- [ ] P6.2 校准工作流目前仍未对照上游校准模块逐项承接
- [ ] P6.3 Model Mall / WebView / 登录体系仍未完成真实承接
- [ ] P6.4 MultiMachine / 批量打印 / 设备列表仍未完成真实承接

### P7 — 质量、稳定性与发布

- [-] P7.1 程序已可构建、可启动、可进行基础导入与切片验证
- [ ] P7.2 自动化测试仍偏向 smoke，缺少“与上游行为对照”的回归基线
- [-] P7.3 已审查全部关键页面 QML 绑定：修复 SettingsPage 分类计数字体色 parent 链（4 级 → 3 级）导致的运行时 undefined 警告、修复 main.qml compareReferenceSource 缩进异常；修复 verify 脚本 cmake 兼容性（添加 -DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=3.21 支持 CMake 3.21 + Qt 6.10 组合）；审查覆盖 PreparePage/PreviewPage/MonitorPage/SettingsPage/PrintSettings/ObjectList/Sidebar/SliceProgress/LayerSlider/MoveSlider/Legend/StatsPanel/StatusBar/ErrorBanner/ErrorToast/PrintDialog 全部 QML 文件，确认 required property + null guard 模式完整、无绑定循环、无类型失配；已通过运行时验证（QT_LOGGING_RULES 启用 qml.binding 和 qml.connections 日志）确认无 QML 运行时警告；但仍未做全量 QML runtime warning 自动化采集与回归对照 2026-03-13
- [ ] P7.4 尚未形成发布级功能冻结、缺陷清单、回滚策略

---

## 4. 模块对照矩阵（第一版）

| 上游模块         | 上游源码位置                                                                              | Qt6 承接位置                                                                                                                                    | 当前状态                         |
| ---------------- | ----------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------- |
| 主窗口与全局导航 | `third_party/CrealityPrint/src/slic3r/GUI/MainFrame.*`, `GUI_App.*`                       | `src/qml_gui/main.qml`, `src/qml_gui/BackendContext.*`                                                                                          | `[-]` 主壳已运行，未全量对照     |
| Prepare 主工作区 | `third_party/CrealityPrint/src/slic3r/GUI/Plater.*`                                       | `src/qml_gui/pages/PreparePage.qml`, `src/qml_gui/panels/*`, `src/core/viewmodels/EditorViewModel.*`                                            | `[-]` 基础闭环已通，功能未齐     |
| 3D 编辑视口      | `third_party/CrealityPrint/src/slic3r/GUI/GLCanvas3D.*`, `Gizmos/*`                       | `src/qml_gui/Renderer/*`, `src/core/rendering/*`                                                                                                | `[-]` 桥接已建，复杂交互未齐     |
| G-code 预览      | `third_party/CrealityPrint/src/slic3r/GUI/GCodeViewer.*`, `GUI_Preview.*`                 | `src/qml_gui/pages/PreviewPage.qml`, `src/qml_gui/components/*`, `src/core/viewmodels/PreviewViewModel.*`, `src/core/rendering/GCodeRenderer.*` | `[-]` 主链路已通，待对照         |
| 设置与预设       | `third_party/CrealityPrint/src/slic3r/GUI/Tab.*`, `PresetComboBoxes.*`                    | `src/qml_gui/pages/SettingsPage.qml`, `src/qml_gui/panels/PrintSettings.qml`, `src/qml_gui/Models/*`, `src/core/viewmodels/ConfigViewModel.*`   | `[-]` 动态骨架可用，未承接全语义 |
| 后台切片         | `third_party/CrealityPrint/src/slic3r/GUI/BackgroundSlicingProcess.*`                     | `src/core/services/SliceService.*`                                                                                                              | `[-]` 主流程已通，待状态机对照   |
| 项目与文件导入   | `third_party/CrealityPrint/src/slic3r/GUI` + `libslic3r/Format/*`                         | `src/core/services/ProjectService.*`, `src/core/viewmodels/EditorViewModel.*`                                                                   | `[-]` 导入可用，项目工作流未齐   |
| 设备与监控       | `third_party/CrealityPrint/src/slic3r/GUI/Monitor.*`, `DeviceManager.*`, `print_manage/*` | `src/qml_gui/pages/MonitorPage.qml`, `src/core/viewmodels/MonitorViewModel.*`, `src/core/services/DeviceService*`                               | `[ ]` 主要仍是骨架               |
| 校准             | `third_party/CrealityPrint/src/slic3r/GUI/CalibrationPanel.*`                             | `src/qml_gui/pages/CalibrationPage.qml`, `src/core/viewmodels/CalibrationViewModel.*`                                                           | `[ ]` 骨架为主                   |
| 商城与 WebView   | `third_party/CrealityPrint/src/slic3r/GUI/ModelMall.*`, `WebViewDialog.*`                 | `src/qml_gui/pages/ModelMallPage.qml` 及后续 Web 容器                                                                                           | `[ ]` 未真实承接                 |
| 多机管理         | `third_party/CrealityPrint/src/slic3r/GUI/MultiMachineManagerPage.*`                      | `src/qml_gui/pages/MultiMachinePage.qml`, 相关 ViewModel/Service                                                                                | `[ ]` 未真实承接                 |

---

## 5. 当前优先级

1. 收敛 Prepare：对象工作区、右侧工作区、Gizmo 与交互链路。
2. 收敛 Preview：清 warning、补对照验收。
3. 切换 Settings / Preset 到上游语义承接。
4. 再进入 Device / Monitor / Account / Web / MultiMachine。
5. 持续补齐“上游模块 -> Qt6 模块 -> 验收记录”的逐项对照闭环。

---

## 6. 里程碑口径

| 里程碑                      | 验收标准                                        | 当前状态 |
| --------------------------- | ----------------------------------------------- | -------- |
| M0 真值冻结                 | 上游 tag/commit + 功能矩阵冻结                  | 已完成   |
| M1 Prepare 可替代日常使用   | 导入、对象管理、Gizmo、切片入口、平板工作流可用 | 进行中   |
| M2 Preview 可稳定回归       | 切片到预览端到端，主要视图模式对照通过          | 进行中   |
| M3 Settings/Preset 完整承接 | 预设与多作用域参数体系对齐上游                  | 未开始   |
| M4 Device/Monitor 承接      | 设备连接、状态、控制、视频闭环                  | 未开始   |
| M5 全功能 RC                | 所有顶栏页面可用，关键回归通过                  | 未开始   |

---

## 7. 备注

- 旧任务文档中“合计 70/70 全部完成”的表述仅能代表“阶段性骨架任务已经做过”，不能再代表当前项目目标的真实完成度。
- 从 2026-03-10 起，所有新增任务和完成记录都必须挂到“上游功能真值对照”之下。
