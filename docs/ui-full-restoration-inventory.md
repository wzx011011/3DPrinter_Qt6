# UI 全量还原清单矩阵 (UI Full-Restoration Inventory)

> 建立日期：2026-09-13。本文档是 UI 全量还原里程碑的唯一总清单，沿用
> [v3.6-ui-inventory.md](v3.6-ui-inventory.md) 的冻结口径（7 态状态词汇 +
> 6 种验证方式），把覆盖范围从 v3.6 的 4 张截图扩展到全部用户可见表面。
> 真值定义不变：**上游运行截图 = 视觉真值；`third_party/OrcaSlicer`
> 源码 = 行为真值**。region 级（区域级）分解在各波次对该表面做差距分析时
> 产出（模板即 v3.6 的 9 列 schema），本文档维护 surface 级（文件级）状态。

## 0. 范围与验收口径

范围 = 主壳（顶栏/菜单/快捷键/通知）+ 10 页 + 34 对话框 + 画布组件与工具。

完成定义（"重复直到没有差距"的判据）：

1. 下表中除 `Blocked` 外不允许存在 `Mock/Placeholder/Missing`；
2. 每个表面有：双侧截图证据（`docs/visual-compare/<surface>/`）+
   对照报告 + QmlUiAuditTests 锁定断言；
3. 主流程 gate 12 套件 0 failed；
4. 已登记偏离（§5）不回退。

状态词汇（FROZEN）：`Real | Hybrid | Mock | Blocked | Placeholder | Superseded | Missing`
验证方式（FROZEN）：`automated-test | deterministic-harness | manual-visual | manual-uat-checklist | build-only | upstream-parity-audit`

## 0.1 波次划分

| 波次 | 范围 | 入口 |
|---|---|---|
| 1 | 主壳（SHELL-\*）+ PreparePage 剩余状态 + PreviewPage 剩余状态（PREV-LEFT 盘缩略图、R-P1.B 预览正确性） | §1/§2 |
| 2 | 预设/设置链：SettingsDialog、PrintDialog、SavePresetDialog、ConfigWizardDialog、UnsavedChangesDialog、BedShapeDialog（R-P1.J 假完成治理） | §3 |
| 3 | 其余对话框长尾（含 R-P1.E 假成功披露治理：设备/云/相机/多机相关对话框） | §3 |
| 4 | 画布微交互：右键菜单、gizmo 浮动面板（PREP-GIZMOFLOAT）、R-P1.H 陈旧绑定清扫、R-P1.F MMU HeightRange、R-P1.L 死代码清除、空白处点击取消选择 | §4 |

## 1. 主壳表面 (SHELL-\*)

| surface_id | surface_name | qt_target | upstream_source | status | verification | wave | notes |
|---|---|---|---|---|---|---|---|
| SHELL-TOPBAR | 顶栏导航/保存/撤销 (top bar) | src/qml_gui/BBLTopbar.qml | MainFrame.cpp + BBLTopbar.cpp | Hybrid | upstream-parity-audit | 1 | P1.1 基础闭环已登记；**未完成项**：标题栏 X / 退出绕过脏项目确认（R-P0.6，上游 MainFrame.cpp close_with_confirm 可否决）；Ctrl+S/菜单保存绕过 canSave 切片门禁（R-P1.I）；隐形 TabBar 死代码（R-P1.L） |
| SHELL-MENUS | 全局菜单/文件入口 (menus) | src/qml_gui/main.qml | MainFrame.cpp m_menubar | Hybrid | upstream-parity-audit | 1 | 已有 viewMenuShortcutsAndImportSourceAudit 锁定；缺：完整上游菜单项逐项对照（导出/导入子菜单、视图菜单项与快捷键绑定一致性） |
| SHELL-SHORTCUTS | 键盘快捷键表 (shortcuts) | src/qml_gui/main.qml + KBShortcutsDialog.qml | MainFrame.cpp + KBShortcutsDialog.cpp | Hybrid | automated-test | 1 | platerHotkeysMatchUpstreamTable / platerHotkeysFollowCanvasFocus 已锁定；长尾：全表逐项复核 |
| SHELL-NOTIFY | 通知系统 (notifications) | NotificationCenter.qml + ErrorToast.qml + ErrorBanner.qml + BackendContext notification queue | notification_manager.cpp + SlicingProgressNotification.cpp | Hybrid | upstream-parity-audit | 1 | 队列/进度/持久化/确认已承接（P1.1）；缺：通知历史列表 UI、通知偏好设置 |
| SHELL-FILEDLG | 文件对话框 (file dialogs) | main.qml FileDialog 流程 + ProjectViewModel | MainFrame.cpp FileOpen/Save + Project.cpp | Hybrid | manual-uat-checklist | 1 | 打开/保存/另存/最近文件已承接；缺：3MF 供应商检查反馈（Check3mfVendor）对照 |

## 2. 页面 (PAGE-\*)

| surface_id | surface_name | qt_target | upstream_source | status | verification | wave | notes |
|---|---|---|---|---|---|---|---|
| PAGE-HOME | 首页 (home) | src/qml_gui/pages/HomePage.qml | MainFrame.cpp 首页 tab + CrealityPrint Home 血统 | Hybrid | manual-visual | 3 | Mock 数据面较多；需逐区域定性（无直接 1:1 上游 Orca 页面，行为真值按 CrealityPrint 血统 + 已登记扩展处置） |
| PAGE-PREPARE | 准备页 (prepare) | src/qml_gui/pages/PreparePage.qml + panels/LeftSidebar.qml + components/GLToolbars.qml | Plater.cpp + Sidebar.cpp + GLCanvas3D.cpp | Hybrid | manual-visual | 1 | v3.6 §2 已逐区域登记：PREP-GIZMOFLOAT 为 Placeholder（波次 4）；**新增发现**：空白处点击不取消选择（上游 GLCanvas3D 左键空点=取消选择）——波次 4；单元区域以 v3.6 表为准逐项收口 |
| PAGE-PREVIEW | 预览页 (preview) | src/qml_gui/pages/PreviewPage.qml + LayerSlider/MoveSlider/Legend/StatsPanel | GUI_Preview.cpp + GCodeViewer.cpp + IMSlider.cpp | Hybrid | manual-visual | 1 | v3.6 §3 已逐区域登记：PREV-LEFT 盘缩略图 Placeholder；R-P1.B 正确性差距（avgSpeed 单位 60 倍错、G28 误匹配、播放结束 isPlaying 卡 true）随波次 1 修 |
| PAGE-MONITOR | 设备页 (monitor) | src/qml_gui/pages/MonitorPage.qml + MonitorViewModel | Monitor.cpp + MonitorPage.cpp + StatusPanel.cpp + HMSPanel.cpp | Hybrid | manual-uat-checklist | 3 | 设备状态/温度/控制已承接（Mock 设备）；摄像头流 Blocked（FFmpeg/WebRTC 缺失，已登记）；R-P1.E 假上线/假打印披露待补 |
| PAGE-MULTIMACHINE | 多设备页 (multi-machine) | src/qml_gui/pages/MultiMachinePage.qml + MultiMachineViewModel | MultiMachine.cpp + MultiMachinePage.cpp + SendMultiMachinePage.cpp | Placeholder | manual-uat-checklist | 3 | 功能矩阵口径 `[ ]`；R-P1.E 假发送披露待补 |
| PAGE-PROJECT | 项目页 (project) | src/qml_gui/pages/ProjectPage.qml + ProjectViewModel | Project.cpp + Preferences.cpp 项目级部分 | Hybrid | manual-uat-checklist | 3 | P6 口径 `[-]`；逐项对照未完成 |
| PAGE-CALIBRATION | 校准页 (calibration) | src/qml_gui/pages/CalibrationPage.qml + CalibrationViewModel + CalibrationDialog/CaliHistoryDialog | Calibration.cpp + CalibrationPanel.cpp + CalibrationWizard\*.cpp | Hybrid | manual-uat-checklist | 3 | 功能矩阵口径 `[ ]`→对话框已落地（P8.x）；向导流程与状态反馈逐项对照未完成 |
| PAGE-PREFERENCES | 偏好设置页 (preferences) | src/qml_gui/pages/PreferencesPage.qml + SettingsViewModel | Preferences.cpp | Hybrid | upstream-parity-audit | 2 | R-P1.D skipAmsBlacklistCheck 不落 saveToSettings；Wave 6 staged transaction 已提交（git log） |
| PAGE-ASSEMBLE | 装配视图页 (assemble) | src/qml_gui/pages/AssemblePage.qml + AssembleViewDataPool + AssemblyMeasureGeometry | none-mapped (Owzx-only, CrealityPrint 血统功能保留) | Hybrid | manual-uat-checklist | 4 | 已登记 OWzx 扩展（对照报告-准备页 处置表），保留不回退 |
| PAGE-PLATER | Plater 容器页 (plater shell) | src/qml_gui/pages/Plater.qml | Plater.cpp Plater::* | Hybrid | upstream-parity-audit | 1 | main.qml 已实例化；作为 Prepare/Preview 的宿主容器，随波次 1 一并对照 |

## 3. 对话框 (DLG-\*)

> 34 个 Qt 对话框全部已接线（无孤儿文件），全部有 QmlUiAuditTests 引用
> （1–91 处）。上游对话框约 60+，未列出的见 §3.1 缺失清单。每个对话框的
> region 级对照在波次 2/3 执行；此处为 surface 级基线状态。

| surface_id | qt_target | upstream_source | status | verification | wave | notes |
|---|---|---|---|---|---|---|
| DLG-SETTINGS | SettingsDialog.qml | Tab.cpp + ParamsDialog.cpp + Tabbook.cpp | Hybrid | upstream-parity-audit | 2 | 独立对话框已按 v3.6 SETTINGS-01 决策落地（替换旧嵌入 SettingsPage）；审计引用 91 处，是最大单件；缺：左侧组导航 Placeholder 区、搜索/进阶切换收口 |
| DLG-PRINT | PrintDialog.qml | PrintDialog.cpp + SendToPrinterDialog.cpp | Hybrid | manual-uat-checklist | 2 | R-P1.E：读不存在的 editorVm.lastGcodePath（PrintDialog.qml:155-159） |
| DLG-SAVEPRESET | SavePresetDialog.qml | SavePresetDialog.cpp | Hybrid | upstream-parity-audit | 2 | R-P1.J：覆盖保存当前预设不可达（SavePresetDialog.qml:62-69） |
| DLG-CONFIGWIZARD | ConfigWizardDialog.qml | ConfigWizard.cpp | Hybrid | upstream-parity-audit | 2 | R-P1.J：完成不应用预设（ConfigWizardDialog.qml:501-509）；QML 直绑 presetServiceMock 违反分层 |
| DLG-UNSAVED | UnsavedChangesDialog.qml | UnsavedChangesDialog.cpp | Hybrid | upstream-parity-audit | 2 | 与 R-P0.6/R-P1.I 保存门禁联动修 |
| DLG-BEDSHAPE | BedShapeDialog.qml | BedShapeDialog.cpp | Hybrid | upstream-parity-audit | 2 | R-P1.J：取消不回滚 |
| DLG-CREATEPRESETS | CreatePresetsDialog.qml | CreatePresetsDialog.cpp | Hybrid | upstream-parity-audit | 2 | 审计引用 24 处 |
| DLG-AMS | AMSSettingsDialog.qml + FilamentGroupPopup.qml | AMSMaterialsSetting.cpp + AMSSetting.cpp + AmsMappingPopup.cpp | Hybrid | manual-uat-checklist | 3 | P8.4 已落地；FilamentGroupPopup 审计 23 处 |
| DLG-ABOUT | AboutDialog.qml | AboutDialog.cpp | Real | manual-visual | 3 | 品牌信息按 OWzx 决策 |
| DLG-EDITGCODE | EditGCodeDialog.qml | EditGCodeDialog.cpp | Hybrid | manual-uat-checklist | 3 | P8.3 已落地 |
| DLG-KBSHORTCUTS | KBShortcutsDialog.qml | KBShortcutsDialog.cpp | Hybrid | manual-visual | 3 | 审计 23 处；随 SHELL-SHORTCUTS 全表复核 |
| DLG-FIRMWARE | FirmwareDialog.qml | FirmwareDialog.cpp | Hybrid | manual-uat-checklist | 3 | OTA 为 Mock，披露待补（R-P1.E 同类） |
| DLG-OBJECTLAYERS | ObjectLayersDialog.qml | GUI_ObjectLayers.cpp + ObjectLayersDialog 语义 | Hybrid | automated-test | 3 | 审计 15 处 |
| DLG-SELECTION | SelectionSettingsDialog.qml | GUI_ObjectSettings.cpp 对象设置面板 | Hybrid | automated-test | 3 | 审计 16 处 |
| DLG-OBJCOLOR | ObjColorDialog.qml | ObjColorDialog.cpp | Hybrid | manual-visual | 3 | |
| DLG-PRESETDIFF | PresetDiffDialog.qml | PresetDiffDialog.cpp（上游 PresetComboBoxes diff 视图） | Hybrid | automated-test | 3 | 审计 37 处 |
| DLG-EXPORTBUNDLE | ExportPresetBundleDialog.qml | PresetBundleDialog.cpp + PresetExporter.cpp | Hybrid | manual-uat-checklist | 3 | |
| DLG-CONFIRM | ConfirmDialog.qml | MsgDialog.cpp | Real | automated-test | 3 | 通用确认（上游 MsgDialog 等价物），审计 21 处 |
| DLG-SINGLECHOICE | SingleChoiceDialog.qml | MultiChoiceDialog.cpp / ChooserDialog 语义 | Real | automated-test | 3 | 通用单选 |
| DLG-FILESYS | FileArchiveDialog.qml | FileArchiveDialog.cpp（3MF 内文件浏览器） | Hybrid | manual-uat-checklist | 3 | 依赖 3MF 读取链路 |
| DLG-SELECTMACHINE | SelectMachineDialog.qml | SelectMachine.cpp + DeviceManager.cpp | Hybrid | manual-uat-checklist | 3 | R-P1.E 假发现披露待补 |
| DLG-NETWORKTEST | NetworkTestDialog.qml | NetworkTestDialog.cpp | Hybrid | manual-uat-checklist | 3 | Mock 网络栈，披露待补 |
| DLG-ACCESSCODE | AccessCodeInputDialog.qml | ConnectPrinter.cpp 配码流 | Hybrid | manual-uat-checklist | 3 | 依赖设备链路 |
| DLG-PLUGIN | PluginManagerDialog.qml | PluginsDialog.cpp + PluginPickerDialog.cpp + NetworkPluginDialog.cpp | Hybrid | manual-uat-checklist | 3 | 上游多对话框合并承接 |
| DLG-WIPETOWER | WipeTowerDialog.qml | WipeTowerDialog.cpp（P8.6） | Hybrid | manual-visual | 3 | QML 直绑 presetServiceMock 违反分层（R-P1.J 同类） |
| DLG-SPEEDLIMIT | SpeedLimitDialog.qml | SpeedLimitDialog.cpp（P8.6） | Hybrid | manual-visual | 3 | |
| DLG-CALI | CalibrationDialog.qml + CaliHistoryDialog.qml | Calibration.cpp + CalibrationWizard.cpp | Hybrid | manual-uat-checklist | 3 | 随 PAGE-CALIBRATION 波次 |
| DLG-RECENTER | RecenterDialog.qml | 上游 PartPlate 重定位语义 | Hybrid | manual-visual | 3 | |
| DLG-LITE | EnableLiteModeDialog.qml | 上游 lite 模式提示语义 | Hybrid | manual-visual | 3 | |
| DLG-SYSINFO | SysInfoDialog.qml | 上游 About/SysInfo 语义 | Hybrid | manual-visual | 3 | |
| DLG-TROUBLESHOOT | TroubleshootDialog.qml | 上游 HMS/故障排查语义 | Hybrid | manual-visual | 3 | |
| DLG-MALL | （已移除） | ModelMall.cpp + WebModelLibraryView.cpp + LoginDialog/OAuthDialog/WebUserLoginDialog.cpp | Blocked | upstream-parity-audit | n/a | MALL-01 审计锁定：无核准 Web host/远程契约/认证流；禁止静态目录与模拟下载 |

### 3.1 上游存在、Qt 侧缺失的对话框（波次 3 逐项定性）

BindDialog、BonjourDialog、CameraPopup（相机 Blocked）、CloneDialog、
ColorDecomposeDialog、CrealityDiscoveryDialog、DesktopIntegrationDialog、
DeviceErrorDialog、DownloadProgressDialog、FilamentMapDialog、
FilamentPickerDialog、MixedFilamentDialog、OAuthDialog（MALL Blocked）、
PartSkipDialog、PhysicalPrinterDialog、PlateSettingsDialog、
PrintOptionsDialog、SendToPrinterDialog、SplitDialog 语义、UpdateDialog。
处置原则：能映射到现有通用件（Confirm/SingleChoice/PrintDialog）的在对照
报告登记映射；真实缺失的按 Missing 进入波次 3 任务清单；设备/云依赖的按
Blocked 登记并补披露。

## 4. 画布组件与工具 (CVAS-\*)

| surface_id | qt_target | upstream_source | status | verification | wave | notes |
|---|---|---|---|---|---|---|
| CVAS-TOOLBAR | GLToolbars.qml | GLToolbar.cpp + Gizmos/GLGizmoBase.cpp | Hybrid | manual-visual | 4 | v3.6 PREP-VTOOLBAR |
| CVAS-CTXMENU | PrepareContextMenus.qml | Plater.cpp 右键菜单 + GLCanvas3D.cpp | Hybrid | upstream-parity-audit | 4 | R-P1.H：勾选/文案一次性绑定陈旧 |
| CVAS-GIZMOPANEL | PreparePage 内嵌 gizmo 浮动面板 | Gizmos/GLGizmoAdvancedCut.cpp on_render_input_window + GLGizmoEmboss.cpp + GLGizmoFdmSupports.cpp | Placeholder | manual-visual | 4 | v3.6 PREP-GIZMOFLOAT；P2.7 各 gizmo 控制面板逐个收口 |
| CVAS-PICKING | RhiViewport/SoftwareViewport 拾取链 | GLCanvas3D.cpp _update_picker + MeshRaycaster（MeshUtils.hpp:159） | Real | automated-test | n/a | 本轮 PICK-BVH 已收口（WORK_ITEMS PICK-BVH-RAYCASTER，1d5e79a） |
| CVAS-NAVIGATOR | NavigatorCube.qml + NavigatorLabels.qml | GLCanvas3D.cpp 导航立方语义 | Real | automated-test | n/a | NavigatorCubeTests 已锁 |
| CVAS-LAYERRAIL | PreviewLayerRail.qml | IMSlider.cpp | Hybrid | automated-test | 1 | R-P1.H：滑条一次性绑定陈旧 |
| CVAS-STATUSBAR | StatusBar.qml | Plater.cpp 状态条语义 | Hybrid | manual-visual | 4 | OWzx 调试特性（页面 N/9、延迟）按已登记决策保留 |
| CVAS-MANIPULATION | PreparePage 左侧对象操作面板（移动/旋转/缩放/测量） | Gizmos/GizmoObjectManipulation.cpp | Hybrid | automated-test | 1 | 已集成进 PreparePage；随波次 1 逐项对照 |
| CVAS-EMPTYCLICK | 空白处左键取消选择 | GLCanvas3D.cpp 左键空点=取消选择 | Missing | manual-visual | 4 | 2026-09-13 GUI 实机验收发现：RhiViewport release 路径 pick<0 时不清 selection（RhiViewport.cpp:1907） |

## 5. 已登记偏离（保留，不回退）

1. 顶栏激活态：绿色填充 pill vs 上游下划线+浅底（品牌样式决策，对照报告-准备页 #5）
2. 装配视图 / 移动 X/Y/Z 工具条 / 底部状态条调试区（OWzx 扩展，#6/#7 + PAGE-ASSEMBLE）
3. 左侧栏"盘"作用域第三 pill（P2.6 已登记功能扩展）
4. 顶部导航多 3 项（多设备/校准/偏好设置页化）
5. ModelMall/Web 容器移除（MALL-01 Blocked 锁定）

## 6. 已知差距登记（R 项 → 波次映射）

来自 docs/源码对照迁移任务追踪.md 第 10 节复查项中 UI 相关条目：

- 波次 1：R-P0.6（退出脏确认）、R-P1.I（保存门禁/删盘刷新/220×220 硬编码）、R-P1.B（预览正确性三件）、R-P1.H（PREV-LayerRail 部分）
- 波次 2：R-P1.J（预设/向导假完成四件）、R-P1.D（skipAmsBlacklistCheck、℃ 翻译打断）
- 波次 3：R-P1.E（四类假成功披露 + PrintDialog lastGcodePath）
- 波次 4：R-P1.H（陈旧绑定推广 Connections/NOTIFY）、R-P1.F（MMU HeightRange 三重缺陷）、R-P1.L（死代码：panels/ObjectList.qml ~1560 行未实例化、panels/SliceProgress.qml 未接线、BBLTopbar 隐形 TabBar、CxPillAction）
- 非目标（Blocked，随平台）：P17.8 冲突标记（切片期碰撞检测无数据通道）、摄像头流（FFmpeg/WebRTC 缺失）

## 7. 每表面执行循环（波次内固定流程）

1. `/analyzing-source-truth-gap <surface>` 只读差距分析（产出 region 级 9 列表）
2. 还原四层顺序：布局骨架 → 控件/间距/主题 → 文案（i18n 六语言同步）→ 行为（快捷键/焦点/禁用态）
3. QML 只做呈现；行为进 ViewModel；QML 直绑 service 即违规（qml-boundaries 规则）
4. 补/改 QmlUiAuditTests 断言锁定（防回退核心）
5. 主流程 gate（改 QML 也要跑；涉 C++ 头文件改动先清 obj 防陈旧布局混链）
6. 双侧截图 + 对照报告（模板 docs/visual-compare/对照报告-准备页.md）→ 提交 main → WORK_ITEMS 台账
