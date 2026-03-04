# Creality Print Qt6 — 开发任务追踪

> 更新规则：每完成一个任务立即将 `[ ]` 改为 `[x]`，并在行末补注完成日期。
> 排除范围：模型商城 / 登录云服务 / 局域网设备（不在本计划内）。
> 当前基线：CP7 风格 4 工作流顶栏 + 10 ViewModel（Q_INVOKABLE）+ 5 Mock Service + 测试 100% ✅
> v3.0 新增：Phase J（3MF 加载）/ Phase K（切片引擎）/ Phase L（真实 G-code 渲染）—— 对应架构文档第 12 章

---

## 已完成基线 ✅

- [x] CMake 三目标构建（FramelessDialogDemo / ViewModelSmokeTests / VisualRegressionTests）
- [x] main.qml — 11 标签页 + StackLayout + 无边框窗口 + 窗口缩放
- [x] 控件库 — CxButton / CxComboBox / CxTextField / CxSlider / CxCheckBox / CxSpinBox
- [x] Theme.qml 暗色主题单例
- [x] 组件 — StatusBar.qml / Toolbar.qml（骨架）
- [x] 11 个页面（Mock UI）— HomePage / PreparePage / PreviewPage / MonitorPage / ProjectPage / CalibrationPage / AuxiliaryPage / DeviceListPage / PreferencesPage / ModelMallPage / MultiMachinePage
- [x] 10 个 ViewModel（Mock）— Home / Editor / Preview / Monitor / Settings / Project / Calibration / ModelMall / MultiMachine / Config
- [x] ViewModel Q_INVOKABLE 逐项访问器（彻底修复 Qt 6.10 V4 VariantAssociationObject 崩溃）
- [x] 5 个 Mock Service — Slice / Preset / Device / Project / Network
- [x] BackendContext — 统一注册 ViewModel，默认 currentPage=1
- [x] Loader 懒加载（6 个有 QVariantList 的页面）
- [x] ViewModelSmokeTests + VisualRegressionTests 100% 通过

---

## 阶段 A — 服务层与 ViewModel 补全（目标 1 周）

- [x] A1 — 新建 `src/core/services/CalibrationServiceMock.h/.cpp`（接口：startCalibration / cancelCalibration / progress）2026-03-03
- [x] A2 — `CalibrationViewModel` 构造接收 `CalibrationServiceMock*`，转发 start / cancel 2026-03-03
- [x] A3 — `SettingsViewModel` 已有 `prefCategory` + `setPrefCategory(int)`（已确认）
- [x] A4 — `BackendContext` 实例化 `CalibrationServiceMock` 并注入 `CalibrationViewModel` 2026-03-03

---

## 阶段 B — 错误处理系统（目标 1 周）

- [x] B1 — 新建 `src/core/viewmodels/ServiceTypes.h`：`ServiceError { code / message / severity }` + `ServiceResult<T>` 模板 2026-03-03
- [x] B2 — `BackendContext` 增加 `lastErrorMessage` / `lastErrorSeverity` Q_PROPERTY + `postError(msg,severity)` / `clearError()` Q_INVOKABLE 2026-03-03
- [x] B3 — 新建 `src/qml_gui/components/ErrorToast.qml`（Info 级，底部弹出 3 秒消失）2026-03-03
- [x] B4 — 新建 `src/qml_gui/components/ErrorBanner.qml`（Warning 级，顶部 Banner + 关闭）2026-03-03
- [x] B5 — `main.qml` 叠加 ErrorToast + ErrorBanner，按 `backend.lastErrorSeverity` 驱动 2026-03-03

---

## 阶段 C — 面板层 panels/（目标 2 周）

- [x] C1 — 新建目录 `src/qml_gui/panels/`，注册到 qml.qrc 2026-03-03
- [x] C2 — `EditorViewModel` 补充：`objectCount` / `objectName(i)` / `objectVisible(i)` / `setObjectVisible(i,v)` / `deleteObject(i)` / `selectObject(i)`；`ConfigViewModel` 新增 9 个打印参数（层高/速度/支撑/填充/温度等）及 setter 2026-03-03
- [x] C3 — 新建 `panels/ObjectList.qml`（列表 + 选中高亮 + 删除按鈕，绑定 EditorViewModel Q_INVOKABLE）2026-03-03
- [x] C4 — 新建 `panels/PrintSettings.qml`（Top-10 参数 Slider：层高/速度/支撑/填充率/温度，绑定 ConfigViewModel）2026-03-03
- [x] C5 — 新建 `panels/SliceProgress.qml`（ProgressBar + 状态文字 + 切片/取消按鈕，绑定 EditorViewModel.sliceProgress/isSlicing）2026-03-03
- [x] C6 — 新建 `panels/Sidebar.qml`（右侧可折叠 340px，Tab：对象/打印/切片）2026-03-03
- [x] C7 — `PreparePage.qml` 右侧内联面板 → `Sidebar { editorVm / configVm }` 2026-03-03

---

## 阶段 D — 对话框层 dialogs/（目标 1 周）

- [x] D1 — 新建目录 `src/qml_gui/dialogs/`，注册到 qml.qrc 2026-03-03
- [x] D2 — 新建 `dialogs/PrintDialog.qml`（文件路径输入 + 打印/导出 G-code/取消，PreparePage "发送打印" 按鈕触发）2026-03-03
- [x] D3 — 新建 `dialogs/CalibrationDialog.qml`（ProgressBar + 状态文字 + 取消，CalibrationPage "开始校准" 按鈕触发）2026-03-03
- [x] D4 — 新建 `dialogs/AboutDialog.qml`（版本号 + Qt 版本 + 协议 + 确认关闭，PreferencesPage "关于" 分类自动弹出）2026-03-03

---

## 阶段 E — 3D 渲染桥接骨架（目标 3 周）

> 注：本阶段做骨架 + Mock 渲染（坐标轴/线框），不接真实 GLCanvas3D。

- [x] E1 — 新建目录 `src/qml_gui/Renderer/`，CMakeLists 添加源文件 2026-03-04
- [x] E2 — 新建 `GLViewport.h/.cpp`（QQuickFramebufferObject 子类，CanvasType 枚举，事件队列 QMutex 保护）2026-03-04
- [x] E3 — 新建 `GLViewportRenderer.h/.cpp`（createFramebufferObject MSAA / synchronize 消费事件 / render Mock 坐标轴）2026-03-04
- [x] E4 — 新建 `CameraController.h/.cpp`（轨道相机：左键旋转 / 中键平移 / 滚轮缩放）2026-03-04
- [x] E5 — `main_qml.cpp` 注册 `qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport")` 2026-03-04
- [x] E6 — `PreparePage.qml` 用真实 `GLViewport { canvasType: GLViewport.CanvasView3D }` 替换 Rectangle 占位 2026-03-04

---

## 阶段 F — G-code 预览渲染骨架（目标 2 周）

> 注：先用随机折线 Mock G-code 路径，完成 UI 骨架和组件接口，真实渲染数据由 Phase L 补全。

- [ ] F1 — 新建 `GCodeRenderer.h/.cpp`（GLViewportRenderer 子类，render() 绘制 Mock 层线，预留 `loadResult(GCodeProcessorResult*)` 接口）
- [ ] F2 — `PreviewViewModel` 补全：`layerCount` / `currentLayerMin` / `currentLayerMax` / `moveCount` / `currentMove` + `setLayerRange()` / `playAnimation()` / `pauseAnimation()`
- [ ] F3 — 新建 `components/LayerSlider.qml`（垂直双端 RangeSlider，绑定 previewVm 层范围）
- [ ] F4 — 新建 `components/MoveSlider.qml`（水平 Slider + 播放/暂停按钮）
- [ ] F5 — `PreviewPage.qml` 替换：占位图 → `GLViewport(CanvasPreview)` + LayerSlider + MoveSlider + 视图模式 CxComboBox（先显示 3 种）

---

## 阶段 J — 3MF 文件加载（目标 2 周）

> 前置：从上游仓库同步 `src/libslic3r/Format/3mf.*`、`bbs_3mf.*` 及依赖（expat / miniz / Boost / Eigen）。
> 完成后：能打开 .3mf / .cxprj 文件，3D 编辑器视口显示模型网格。

- [x] J1 — 将 `libslic3r/Format/3mf.cpp/.hpp` + `bbs_3mf.cpp/.hpp` 及依赖库（expat/miniz/Boost/Eigen/fast_float）加入 `CMakeLists.txt`，MSVC 补 `/bigobj /utf-8` 编译选项，确认编译通过 2026-03-04
  - ✅ 采用预编译 .lib 导入方案（`cmake/BuildLibslic3r.cmake`），无需重编译 ~480 个源文件
  - ✅ 上游 libslic3r 用 `Directory.Build.targets` 重编为 /MD CRT，与 Qt/TBB/Boost 完全兼容
  - ✅ 33 个静态库 + 27 个 OCCT 动态库 + bcrypt/assimp/qhullcpp/cr_tpms_library 全部链接成功
  - ✅ 程序启动正常，QML GUI 正常加载（内存 ~211MB）
- [ ] J2 — 实现 `ProjectService::loadFile(path)` 真实版本：`QtConcurrent::run` 异步调用 `Model::read_from_archive()`，`Import3mfProgressFn` 回调桥接为 `loadProgress(int, QString)` Qt 信号
- [ ] J3 — BBS/Creality 格式支持：`load_bbs_3mf()` 接入，解析 `PlateDataPtrs`，`emit plateDataLoaded(n)` 通知 PlateViewModel 更新平板栏
- [x] J4 — 对接 `EditorViewModel`：`loadFinished` 信号触发对象列表刷新 + `GLViewport(CanvasView3D)` 重绘，拖拽文件打开端到端联通 2026-03-05

---

## 阶段 K — 切片引擎接入（目标 3 周）

> 前置：Phase J 完成（能加载模型），新增依赖 TBB 2021.11（并行切片）。
> 完成后：点击「切片」按钮能生成 .gcode 文件，进度条实时更新。
> 关键：`BackgroundSlicingProcess` 中所有 `wxQueueEvent`/`wxGetApp()` 调用替换为 Qt 信号，不修改切片算法本身。

- [ ] K1 — 将 `libslic3r/Print.*`、`PrintObject.*`、`PrintObjectSlice.cpp`、`GCode.*`、`GCode/GCodeProcessor.*` 加入 CMake，补全 TBB/NLopt/CGAL 依赖，确认编译通过
- [ ] K2 — 实现 `SliceService`（`src/core/services/SliceService.h/.cpp`）：Qt Worker Thread 运行 `m_print.apply() + m_print.process()`，`Print::m_status_callback` 桥接为 `progressUpdated(int percent, QString label)` Qt 信号（替代 `wxQueueEvent` slicing 状态事件）
- [ ] K3 — 进度回调桥接：`Print::set_status()` 内部通过 `QMetaObject::invokeMethod(Qt::QueuedConnection)` 安全回调至主线程，UI 进度条随切片推进实时更新
- [ ] K4 — G-code 导出：切片完成后调用 `m_print.export_gcode(path, &m_gcodeResult)`，填充 `GCodeProcessorResult`，`emit slicingFinished(timestamp)` 通知 PreviewViewModel
- [ ] K5 — 已有 G-code 缓存复用：实现 `loadGCodeFromPrevious(gcodeFile)`，调用 `Print::export_gcode_from_previous_file()`，避免重复切片

---

## 阶段 L — 真实 G-code 预览渲染（目标 3 周）

> 前置：Phase K 完成（`GCodeProcessorResult` 已填充），Phase F 已完成（UI 骨架和组件已就位）。
> 完成后：预览页面渲染真实 G-code 路径，支持层浏览、动画播放、13 种视图着色模式。

- [ ] L1 — `GCodeRenderer::loadResult(const GCodeProcessorResult*)` 实现：解析路径数组，按 `layer_id` 分组构建 VBO/IBO（`QOpenGLBuffer`），存储 `m_layerOffsets[]` 用于范围裁剪
- [ ] L2 — 基础渲染通道：`render(layerMin, layerMax, moveEnd)` 按范围 `glDrawArrays`，实现 `FeatureType`（外壁/填充/支撑…）、`Height`、`Width`、`Tool` 四种 P0 优先着色模式
- [ ] L3 — 视图模式完整支持：补全剩余 9 种着色模式（Feedrate / FanSpeed / Temperature / ColorPrint / FilamentId / Chronology / VolumetricRate / LayerTime / Shells 叠加）
- [ ] L4 — `PreviewViewModel` 对接：`onSlicingFinished()` 读取 `GCodeProcessorResult`，更新 `layerCount` / `moveCount` / `legendItems` / `totalTime` / `filamentUsed` 属性，触发 PreviewPage 刷新
- [ ] L5 — 图例面板 + 统计信息：新建 `components/Legend.qml`（颜色块 + 标签，绑定 `previewVm.legendItems`）和 `components/StatsPanel.qml`（总时间 / 耗材用量 / 层数）

---

## 阶段 G — 动态配置 UI & 预设管理（目标 2 周）

> 注：ConfigOptionModel 用 QJsonArray 驱动，不依赖 DynamicPrintConfig。

- [x] G1 — 新建 `src/qml_gui/Models/ConfigOptionModel.h/.cpp`（QAbstractListModel，角色：key/label/type/value/min/max/enumLabels/category/readonly，Mock ~30 个参数）2026-03-04
- [x] G2 — 新建 `src/qml_gui/Models/PresetListModel.h/.cpp`（QAbstractListModel，Print/Filament/Printer 三类，Mock 13 条）2026-03-04
- [x] G3 — `ConfigViewModel` 增加 `Q_PROPERTY(QObject* printOptions)` + `presetList`，构造中初始化 Mock 数据 2026-03-04
- [x] G4 — `panels/PrintSettings.qml` 升级：静态 Slider → `ListView { model: configVm.printOptions }` + Loader 懒加载防 debug heap 崩溃 2026-03-04
- [x] G5 — `panels/Sidebar.qml` 顶部增加预设选择 `ComboBox { model: configVm.presetList }` + 底部「高级设置」按钮 2026-03-04

---

## 阶段 H — SettingsPage 全屏编辑（目标 1 周）

- [x] H1 — 新建 `src/qml_gui/pages/SettingsPage.qml`（三栏：左侧分类树 9 类 / 中间动态参数列表带搜索 / 参数类型控件 Slider/Switch/ComboBox）2026-03-04
- [x] H2 — `main.qml` StackLayout 为 SettingsPage 预留入口（第 12 页 index=11，Loader 懒加载）2026-03-04
- [x] H3 — `BackendContext` 增加 `openSettings()` Q_INVOKABLE，调用 `setCurrentPage(11)` 2026-03-04

---

## 阶段 I — 国际化（目标 1 周）

- [x] I1 — 所有 QML 文件硬编码中文改为 `qsTr()`，C++ 侧 `tr()` 同步 2026-03-04
- [x] I2 — CMakeLists 增加 `qt_add_translations()` 规则，生成 `i18n/zh_CN.ts` / `i18n/en.ts` 2026-03-04
- [x] I3 — 创建 `i18n/zh_CN.ts` / `i18n/en.ts` 翻译桩文件，qt5 TS 格式，生成 .qm 编译确认 2026-03-04

---

## 进度汇总

| 阶段     | 名称              | 任务数 | 完成   | 状态                  |
| -------- | ----------------- | ------ | ------ | --------------------- |
| 基线     | 已完成基线        | 14     | 14     | ✅ 完成               |
| A        | 服务层补全        | 4      | 4      | ✅ 完成               |
| B        | 错误处理系统      | 5      | 5      | ✅ 完成               |
| C        | 面板层 panels/    | 7      | 7      | ✅ 完成               |
| D        | 对话框层 dialogs/ | 4      | 4      | ✅ 完成               |
| E        | 3D 渲染桥接骨架   | 6      | 6      | ✅ 完成               |
| F        | G-code 预览渲染   | 5      | 0      | ⏳ 待开始             |
| G        | 动态配置 UI       | 5      | 5      | ✅ 完成               |
| H        | SettingsPage      | 3      | 3      | ✅ 完成               |
| I        | 国际化            | 3      | 3      | ✅ 完成               |
| J        | 3MF 文件加载      | 4      | 2      | 🔶 J1/J4完成，J2-J3待做 |
| K        | 切片引擎接入      | 5      | 0      | ⏳ 待开始（依赖 J）   |
| L        | 真实 G-code 渲染  | 5      | 0      | ⏳ 待开始（依赖 K+F） |
| **合计** |                   | **70** | **53** | 🔶 F/J2-3/K/L 待做    |

---

## 工作记录

| 日期       | 完成任务                       | 备注                                                                                                                                                                                                                          |
| ---------- | ------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 2026-03-03 | 基线 14 项 + A3 确认           | Qt 6.10 V4 崩溃根因修复（Q_INVOKABLE 逐项访问器方案）                                                                                                                                                                         |
| 2026-03-03 | A1/A2/A4 + B1-B5 共 8 项       | CalibrationServiceMock、ServiceTypes.h、BackendContext 错误系统、ErrorToast/Banner/main.qml；构建+测试 2/2 ✅                                                                                                                 |
| 2026-03-03 | C1-C7 共 7 项                  | EditorVM 对象列表 API、ConfigVM 9 个打印参数、ObjectList/PrintSettings/SliceProgress/Sidebar.qml、PreparePage 接入 Sidebar；构建+测试 2/2 ✅                                                                                  |
| 2026-03-03 | D1-D4 共 4 项                  | PrintDialog/CalibrationDialog/AboutDialog；三个页面接入触发逆辑；构建+测试 2/2 ✅                                                                                                                                             |
| 2026-03-04 | G1-G5 + H1-H3 + I2-I3 共 13 项 | ConfigOptionModel(30参)/PresetListModel(13预设)；PrintSettings动态ListView+Loader懒加载；Sidebar预设ComboBox+高级设置按钮；SettingsPage三栏全屏编辑；BackendContext.openSettings()；i18n翻译桩；构建+测试 2/2 ✅              |
| 2026-03-04 | E1-E6 共 6 项                  | GLViewport(QQuickFramebufferObject)+GLViewportRenderer(GLSL 330 core/MSAA×4/坐标轴+网格)+CameraController(orbit/pan/zoom)；PreparePage接入；VisualRegressionTests用GLViewportTestStub避Qt6.10析构堆损坏；构建+测试 2/2 ✅     |
| 2026-03-04 | I1 共 1 项                     | 25文件~250中文字符串qsTr()/tr()包裹；lupdate提取270条源文本→zh_CN.ts/en.ts；构建82/82+测试 2/2 ✅                                                                                                                             |
| 2026-03-04 | 架构文档 v3.0 更新             | 研究 CrealityOfficial/CrealityPrint 上游仓库；新增第12章（3MF加载/切片引擎/GCodeRenderer 完整方案）；新增 ADR-09/10/11；新增 Phase J/K/L 实施计划；PROJECT_STRUCTURE.md 同步更新目录树和依赖状态表                            |
| 2026-03-04 | J1 完成（libslic3r 链接成功）  | 预编译 .lib 导入方案：33 静态库+27 OCCT+assimp+qhullcpp+cr_tpms+bcrypt；CRT /MT→/MD 用 Directory.Build.targets 重编上游；101 个链接错误全部解决；nanosvg_impl.cpp 提供 stb 实现；程序启动正常（211MB 内存，QML GUI 正常加载） |
| 2026-03-05 | J4 完成（Editor-Viewport 端到端） | EditorViewModel 与 GLViewport 渲染链路联通：对象列表刷新、重绘触发、拖拽文件打开端到端可用；220×220 平台、fitView、多对象颜色与 Gizmo 交互合并验证通过 |
| 2026-03-05 | CP7 顶栏还原（计划外优化）      | `main.qml` 顶栏改为 Creality Print 7.0 风格（在线模型/准备/预览/设备）；补齐文件菜单/更多菜单、保存、窗口控制、标题截断提示；Prepare 页接入真实撤销/重做（拖拽变换命令栈） |
