# Requirements — Milestone v2.0: OrcaSlicer UI Full Restoration

**Scope:** 把 Qt6/QML 项目的 UI 全面对齐到 OrcaSlicer 上游。v2.0 聚焦 P0（架构对齐）+ P1（核心可见功能），P2-P3 推迟到 v2.1/v2.2。

**Reference:** 调研报告见会话历史；上游 GUI 源码 `third_party/OrcaSlicer/src/slic3r/GUI/`。

---

## Categories

| ID Prefix | Category | Phase Mapping |
|-----------|----------|---------------|
| `BRAND` | 品牌清理 | Phase 01 |
| `ARCH` | 顶层架构（Notebook + Plater 共享 + Sidebar Dockable） | Phase 02-04 |
| `SIDEBAR` | Prepare 左侧 Sidebar 八大区块 | Phase 05 |
| `GLUI` | GLCanvas 工具栏系统（Main/Gizmos/View） | Phase 06 |
| `CALIB` | Calibration 菜单 + 校准 Dialog | Phase 07 |
| `TOPBAR` | BBLTopbar 完整菜单系统 | Phase 02 |

---

## P0 — Architecture Alignment (Phase 01-04)

### BRAND — OWzx Brand Cleanup

- [x] **BRAND-01**: 应用窗口标题、关于对话框、shortcut 一览对话框中所有 "Creality Print" / "CrealityPrint" 字符串替换为 "OWzx"
- [x] **BRAND-02**: 移除 `third_party/CrealityPrint` submodule 引用与所有相关 CMake/config 路径
- [ ] **BRAND-03**: 内部命名空间 `Crality3D` / `creality` 等迁移到 `OWzx` / `owzx`（保持向后兼容的 alias 阶段可省略）
- [x] **BRAND-04**: 资源文件（图标、配置）中所有 Creality 品牌资源替换为 OWzx 版本
- [x] **BRAND-05**: 启动画面、关于对话框版本号对齐 OrcaSlicer main branch（不再用 v7.0.1）

### ARCH — Top-Level Architecture

- [ ] **ARCH-01**: `main.qml` 从 12 页 `StackLayout` 重构为 9 页 `TabBar + StackLayout`（Home/Prepare/Preview/Device/MultiDevice/Project/Calibration + 2 占位）
- [ ] **ARCH-02**: Notebook tab 支持上游 `tp3DEditor=1` / `tpPreview=2` 等位置常量语义
- [ ] **ARCH-03**: Tab 切换通过 `request_select_tab(TabPosition)` 事件广播（对齐上游 `EVT_SELECT_TAB`）
- [ ] **ARCH-04**: Notebook 右侧 `side_tools` 区支持 Slice/Print 下拉按钮 + FilamentGroupPopup（多耗材分组切片）
- [ ] **ARCH-05**: Prepare 与 Preview 共享同一 `Plater` QML 组件实例（通过 Loader + `viewMode` 属性切换 View3D↔Preview）
- [ ] **ARCH-06**: Plater 内部三选一显示 View3D/Preview/AssembleView（按 viewMode 切换）
- [ ] **ARCH-07**: 切换 Prepare→Preview 时保留所有 Plater 状态（对象/选择/切片结果/gizmo 状态），无需重建
- [ ] **ARCH-08**: Sidebar 支持 Dockable（可拖到 Left/Right/浮动/折叠），用 QDockWidget 或 QML 等价机制
- [ ] **ARCH-09**: Sidebar 折叠按钮（对齐上游 `collapse_toolbar`），点击隐藏 Sidebar 让 3D 区独占
- [ ] **ARCH-10**: Plater 与 Sidebar 之间布局响应式（窗口缩放/最大化正确处理）

### TOPBAR — BBLTopbar Menu System

- [ ] **TOPBAR-01**: BBLTopbar 重构为标题栏 + 菜单 + 工具按钮合体（对齐上游 `BBLTopbar.cpp`）
- [ ] **TOPBAR-02**: `[File ▾]` 下拉菜单（New/Open/Recent/Save/Save As/Import 系列/Export 系列/Quit）
- [ ] **TOPBAR-03**: `[▾]` 二级下拉（Edit/View/Preferences/Calibration/Help 完整子菜单）
- [ ] **TOPBAR-04**: 工具按钮（Save/Undo/Redo/Calibration）+ 条件按钮（Account/ModelStore/Publish）
- [ ] **TOPBAR-05**: CenteredTitle 显示项目名（居中）
- [ ] **TOPBAR-06**: 窗口控制按钮（Min/Max/Close），Linux 自绘风格可选
- [ ] **TOPBAR-07**: macOS 走系统菜单栏，Win/Linux 走 BBLTopbar（条件编译）

---

## P1 — Core Visible Features (Phase 05-07)

### SIDEBAR — Prepare Plater Sidebar

- [ ] **SIDEBAR-01**: Sidebar 滚动区按八大区块顺序布局：Printer → Filament → Process 顶部条 → Search → ObjectList → ObjectSettings → ObjectLayers → ParamsPanel page_view
- [ ] **SIDEBAR-02**: Printer 标题栏（icon + "Printer" + connect/sync/settings 按钮）+ 内容（preset combo + 喷嘴 diameter + Bed type + extruders 数）
- [ ] **SIDEBAR-03**: Printer 标题栏可折叠（点击切换内容显隐）
- [ ] **SIDEBAR-04**: Filament 标题栏 + 双列耗材列表（每个 extruder 一个 PlaterPresetComboBox + 颜色 + edit）
- [ ] **SIDEBAR-05**: Filament 标题栏可折叠
- [ ] **SIDEBAR-06**: Process 顶部条（ParamsPanel.m_top_panel）：Process icon + "Process" + SwitchButton(Global/Objects) + ModeIcon + ModeSwitchButton(Simple/Advanced) + Compare + Setting
- [ ] **SIDEBAR-07**: Global/Objects 作用域切换正确反映到参数列表显示
- [ ] **SIDEBAR-08**: Simple/Advanced 模式切换正确过滤参数可见性
- [ ] **SIDEBAR-09**: Compare 按钮打开 DiffPresetDialog（v2.0 占位，正式实现延后到 v2.2）
- [ ] **SIDEBAR-10**: Setting 按钮弹出 ObjectTableDialog（查看对象全部设置）
- [ ] **SIDEBAR-11**: Search bar（StaticBox + TextInput + search icon），输入跳转到对应参数
- [ ] **SIDEBAR-12**: ObjectList 树完整（对象/部件/设置/层），支持拖拽/右键菜单/重命名
- [ ] **SIDEBAR-13**: ObjectSettings 区块（选中对象的快速设置），无选中时隐藏
- [ ] **SIDEBAR-14**: ObjectLayers 区块（变量层高编辑器），仅打印对象时显示
- [ ] **SIDEBAR-15**: ParamsPanel page_view（参数列表）+ 左侧 7 个子 Tab 按钮（Print/PrintPlate/PrintObject/PrintPart/PrintLayer/Filament/Printer）

### GLUI — GLCanvas Toolbar System

- [ ] **GLUI-01**: MainToolbar（顶部 GL overlay）：[+Add][+Plate][⟳Orient][⇲Arrange] | [More/Fewer][SplitOptions][SplitParts][LayersEditing]
- [ ] **GLUI-02**: Gizmos 竖向条（左侧 GL overlay）：Move/Rotate/Scale/Flatten/Cut/MeshBoolean/FdmSupports/Seam/Emboss/Svg/Measure/Simplify
- [ ] **GLUI-03**: ViewToolbar（右侧 GL overlay）：[Top][Front][Right][Back][ISO][Reset] 视角预设
- [ ] **GLUI-04**: SeparatorToolbar（分隔条）
- [ ] **GLUI-05**: CollapseToolbar（折叠 Sidebar 按钮）
- [ ] **GLUI-06**: Preview 模式下显示 IMSlider/IMToolbar（层/G-code 滑块与工具条），用 QML 浮层实现
- [ ] **GLUI-07**: 工具栏 QML overlay 替代 ImGui GL 渲染（保持与 QML 主题一致）
- [ ] **GLUI-08**: PartPlateList（底部 GL overlay）：多板 plate 列表条 + [+Add] 按钮
- [ ] **GLUI-09**: NotificationManager 浮层通知（3D canvas 右上角），与全局通知系统集成

### CALIB — Calibration Menu & Dialogs

- [ ] **CALIB-01**: Calibration 顶级菜单（Temperature/Max flowrate/Pressure advance/Flow ratio/Retraction/Cornering/Input Shaping Freq+Damp/VFA/Calibration Guide）
- [ ] **CALIB-02**: PA_Calibration_Dlg（压力推进校准）
- [ ] **CALIB-03**: FlowRateCalibrationDialog（流量校准，含向导）
- [ ] **CALIB-04**: Temp_Calibration_Dlg（温度校准）
- [ ] **CALIB-05**: MaxVolumetricSpeed_Test_Dlg（最大流量测试）
- [ ] **CALIB-06**: VFA_Test_Dlg（VFA 测试）
- [ ] **CALIB-07**: Retraction_Test_Dlg（回抽测试）
- [ ] **CALIB-08**: Input_Shaping_Freq_Test_Dlg + Input_Shaping_Damp_Test_Dlg（输入整形）
- [ ] **CALIB-09**: Cornering_Test_Dlg（拐角测试）
- [ ] **CALIB-10**: 校准历史记录持久化（EditCalibrationHistoryDialog）
- [ ] **CALIB-11**: CalibrationWizard 系列（PA/Flow/MaxVS 嵌入 CalibrationPanel）

---

## Future Requirements (Deferred to v2.1 / v2.2)

### P2 — High-value Pages (v2.1 candidate)

- Home WebView（QtWebEngine 替代 wxWebView，Makezilla 模型商店）
- Device 双形态（MonitorPanel for BBL / PrinterWebView for 非 BBL）
- Multi-device 页（条件显示，多打印机批量管理）
- Project 页 5 子 Tab（Basic Info/Pictures/BOM/Assembly Guide/Others）
- ConfigWizard 完整版（Welcome→Printer→Filament→Done）
- PreferencesDialog 完整版（几十个偏好选项）

### P3 — Dialog & Polish (v2.2 candidate)

- AMS 系列 Dialog（~10 个：AmsMappingPopup 全家、AMSMaterialsSetting、SyncAmsInfoDialog 等）
- 预设管理 Dialog（SavePreset/DiffPreset/CreatePreset 系列）
- Print/PrintHost 系列（SelectMachine/SendToPrinter/PrintHostQueue）
- 绑定/网络 Dialog（Bind/UnBind/Bonjour/OAuth/PrinterCloudAuth）
- 更新/隐私 Dialog（ReleaseNote/UpdateVersion/PrivacyUpdate）
- i18n 21 种语言接入（迁移 OrcaSlicer .po 到 Qt .ts/.qm）
- 图标主题 `_dark` 后缀约定（Qt 等价机制）
- NotificationManager 浮层细节完善

---

## Out of Scope (v2.0)

- **TriangleSelector 真实集成** — 上游依赖 wxWidgets GL 交互，阻塞 SupportPaint/SeamPaint/MmuSegmentation
- **OpenVDB 集成** — 链接失败，阻塞 Hollow Gizmo
- **FFmpeg/RTSP 视频流** — 未找到，阻塞 Monitor 摄像头
- **bambu_networking 真实连接** — 闭源，阻塞真实设备连接/MQTT
- **Shell 渲染** — 依赖上游 GCodeViewer shell 集成
- **SLA 模块完整迁移** — 上游 SLA 专用功能
- **AssembleView 真实功能** — 多板装配视图（v2.0 仅保留入口，正式实现延后）

---

## Traceability

| REQ-ID | Phase | Verified By |
|--------|-------|-------------|
| BRAND-01 ~ BRAND-05 | Phase 1 | — |
| ARCH-01 ~ ARCH-04 | Phase 2 | — |
| ARCH-05 ~ ARCH-07 | Phase 3 | — |
| ARCH-08 ~ ARCH-10 | Phase 4 | — |
| TOPBAR-01 ~ TOPBAR-07 | Phase 2 | — |
| SIDEBAR-01 ~ SIDEBAR-15 | Phase 5 | — |
| GLUI-01 ~ GLUI-09 | Phase 6 | — |
| CALIB-01 ~ CALIB-11 | Phase 7 | — |

---
*Last updated: 2026-06-12 — Initial requirements for v2.0 milestone*
