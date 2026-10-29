# UI 还原 + 功能补齐 收敛式战役总账（2026-10）

> 本文档是"修一轮 → 全量重审 → 直到零新发现"收敛式战役的唯一账本。
> 每轮结束后更新；条目只增不删，状态流转：`open → fixing → fixed(证据) / deferred(原因) / dup(id)`。

## 0. 口径

- **来源标注**：`R1A`=存量账清账（FB-*/RG-*/挂账复核）；`R1B`=全覆盖视觉审计（矩阵截图开放式目审）；`R1C`=功能面对照（/tmp/creality-print 源码对照）。
- **优先级**：P1=显眼破损/丑（日常必见）；P2=功能缺陷有绕过或入口缺失；P3=打磨项。
- **收敛判定**：一轮全量重审 零新 P1/P2 且零新 P3 → 确认轮仍为零 → 收敛。安全上限 5 轮。
- **类别**：`还原`（对照上游布局/样式）、`美观`（不丑但可更好）、`功能`（缺失/残缺）、`数据`（预设键集等）、`i18n`。
- **不计入收敛（deferred）**：硬件绑定（真机/摄像头/固件 OTA）、契约阻塞（MALL-01 商城审计锁）、需用户拍板项、Windows 侧回归。
- **验证规范**：修复须过 样式门禁 + 14 套件；行为类修复须反向验证；视觉修复须前后截图；验证一律重定向文件再读（管道 tail 吞退出码教训）。

## 1. Round 0 基线（2026-10-29）

- HEAD `d984566`（含 9ad9d96 侧栏像素还原、d984566 盘作用域收进对象 pill）。
- 工作区脏文件 = 并行会话在途渲染工作（PrepareSceneData/RhiViewport*/shaders/perf），本战役不碰不代提交。
- 构建：`audit/build-baseline.sh`（flock .build.lock + cgroup 26G J=8，j10 曾 OOM 实证）。
- 基线门禁：`audit/verify-baseline.log`（14 套件）。
- 基线矩阵：`audit/round0/`（audit-matrix.sh：7页×双主题×空/模型 + 12对话框浅/4深 + 向导 + 提示卡 + 右键×2）。

## 2. R1A 存量清账（复核后流转状态；初始=需复核）

### 2.1 P2 功能账（R1A 源码级复核完成 10-29，状态已流转）

| ID | 区域 | 描述 | 级 | 规模 | 类别 | 状态 |
|---|---|---|---|---|---|---|
| R1A-01 | 装配 | FB-R9-02 装配测量两点距离面板不出现 | P2 | M | 功能 | **已修(源码链闭合)**——测量面板 visible 绑 assemblyMeasureGizmoActive（AssemblePage.qml:514），待 GUI 抽验转终态 |
| R1A-02 | 装配 | FB-R9-01 进装配视图相机不取景对象 | P2 | S | 功能 | open——AssemblePage 无任何 fit/取景逻辑（唯一 requestFitView 在 PreparePage.qml:186） |
| R1A-03 | 校准 | FB-R10-01 校准历史对话框点击无反应 | P2 | M | 功能 | **已修(源码级)**——CaliHistoryDialog.qml:309-343 修正注释+reloadHistory 完整，待 GUI 抽验 |
| R1A-04 | 文件 | FB-R1-03 Ctrl+I zip 树为空（URL 未归一化） | P2 | S | 功能 | open——main.qml:163-165 selectedFile.toString() 直传，ProjectService.cpp:784 原样开路径，无 toLocalFile |
| R1A-05 | 文件 | FB-R1-04 Ctrl+I .gcode 静默无反应 | P2 | S | 功能 | open——main.qml:173-176 URL 直传；BackendContext.cpp:844 QFileInfo::exists(URL) 恒 false（对照 topbarImportModel:1643 已归一化，仅此两分支漏） |
| R1A-06 | 文件 | FB-R1-05 保存 3MF 机器预设串被拆坏 | P2 | M | 功能 | 需GUI——写链 ProjectService.cpp:11718 用 set_deserialize_strict 整串写，源码无拆分痕迹；需"存3MF→回读"实测定案 |
| R1A-07 | 撤销 | FB-R3-05 细分网格撤销无效 | P2 | M | 功能 | open——subdivideSelectedMesh（EditorViewModel.cpp:9373）不 push undo command（对照 convertSelectedObjectUnits:9398 有 ObjectSnapshotCommand） |
| R1A-08 | 涂色 | FB-R4-10 MMU 面板色块溢出 | P2 | S | 还原 | **已修**——Repeater model=mmuExtruderCount 配置驱动（PreparePage.qml:3591，11a6afe） |
| R1A-09 | 涂色 | FB-R4-11 绒毛皮肤无面板无笔刷 | P2 | M | 功能 | 部分修——按钮+per-object API 在（GLToolbars.qml:312、EditorViewModel.h:1313），facet 涂刷半边未移植（:291 自注），无笔刷面板 |
| R1A-10 | 多盘 | FB-R5-08 盘 rail ≥4 盘溢出/不可滚动 | P2 | M | 还原 | open——SelectPlateRail.qml:40-178 纯 Column，无 ScrollView/Flickable |
| R1A-11 | 设置 | FB-R6-04 床尺寸 QSettings 键大小写两套 | P2 | S | 功能 | 需拍板 |
| R1A-12 | 多盘 | FB-R6-07 无"移动对象到盘 N"入口 | P2 | S | 功能 | open——API 完整（EditorViewModel.cpp:10132）但 qml/ 0 调用点 |
| R1A-13 | 耗材 | FB-R6-09 耗材槽删/合并 stub | P2 | M | 功能 | open——EditorViewModel.cpp:9545/9553 "not yet implemented" 原样 |
| R1A-14 | 校准 | FB-P1-04 校准页鼠标死区 | P2 | ? | 功能 | 需GUI——源码无遮挡证据，疑自动化误判，待真机复认 |

### 2.2 还原账（R1A 源码级复核完成 10-29）

| ID | 区域 | 描述 | 级 | 规模 | 类别 | 状态 |
|---|---|---|---|---|---|---|
| R1A-20 | 预设 | RG-R6-02 耗材预设键集缺口 | P2 | L | 数据 | 需拍板范围 |
| R1A-21 | 预设 | RG-R6-03 机器预设键集缺口 | P2 | L | 数据 | 需拍板范围 |
| R1A-22 | 预设 | RG-R6-01 工艺缺键+Dependencies/Notes 页 | P3 | M | 数据 | 部分修——Notes 页已建（SettingsDialog.qml:110/116）；Dependencies 仍缺（:97 deferred 注释）；工艺键大幅扩充仍有零散缺（max_volumetric_speed 等） |
| R1A-23 | 预览 | RG-R8-01 缺 Tool 着色模式 | P3 | M | 功能 | **已修**——16 模式全量对齐（PreviewViewModel.cpp:938，上游 Tool 本就隐藏） |
| R1A-24 | 快捷键 | RG-R15-01 缺 A/I/O/L/C/Ctrl+Shift+E | P2 | S | 功能 | open——main.qml 快捷键全表（:640-782）无裸键/该组合 |
| R1A-25 | 菜单 | File 无 New Window；Publish 占位 | P2/P3 | M | 功能 | 需拍板 |
| R1A-26 | 菜单 | 导出缺 分别导出 STLs/Generic 3MF | P3 | M | 功能 | open——导出子菜单 6 项（BBLTopbar.qml:998-1046）仍无此两项 |
| R1A-27 | 菜单 | Edit 无 Deselect All | P3 | S | 还原 | **已修**——BBLTopbar.qml:1131"取消选择"→clearObjectSelection |
| R1A-28 | 导入 | RG-R3-01 无 StepMeshDialog | P3 | M | 功能 | open——全库无实现 |
| R1A-29 | 右键 | RG-R3-02 Auto Drop/Cone/Disc 占位 | P3 | S | 还原 | 部分修——盘级添加原语可用；Auto Drop（PrepareContextMenus.qml:526/923）与 Cone/Disc（:263）仍禁用 |
| R1A-30 | gizmo | RG-R4-01 旋转无 45° 吸附 | P2 | S | 还原 | open——deltaAngle 原样上抛（RhiViewport.cpp:2390，该文件含并行在途 diff）→直接累加无取整 |
| R1A-31 | gizmo | RG-R4-02 切割无 PlaceOnCut/CutToParts | P3 | M | 还原 | open——全库零命中 |
| R1A-32 | i18n | RG-R0-01/02 多设备/校准页英文 | P2 | S | i18n | 部分修——qsTr 化+QTranslator 已接；zh_CN.ts 仍缺 8 条（Device Status/Go/Flow 比率/HARDWARE CALIBRATION/Filament Flow Ratio/Fine Calibration (Pass 2)/两条校准 tooltip） |
| R1A-33 | i18n | schema tooltip 英文长文本 | P3 | L | i18n | 需拍板（机器翻译质量样张先行） |
| R1A-34 | 校准 | RG-P6-01 Cornering/Input Shaping 占位 | P3 | M | 功能 | deferred（上游同为硬件门） |

### 2.3 P3 瑕疵（批量 sweep 候选，逐条复核时并查）

FB-R0-03 首页提示条叠压 / FB-R1-08 Timer undefined / FB-R1-09 空工程对象列表"平板"行无字+无未保存标记 / FB-R2-03 多 --load-model 只第一个生效 / FB-R2-07 Shift+Tab / FB-R2-08 AMF 元数据名 / FB-R2-09 立方体侧视图文字标签 / FB-R3-08 已保存横幅布局抖动 / FB-R3-09 Split→To Parts 存盘 / FB-R4-01 旋转 ±180 表示 / FB-R4-07 禁用 gizmo 按钮穿透 / FB-R4-08 数值框回车显空 / FB-R4-09 提示文案泄露内部名 / FB-R4-12 接缝无默认 tool / FB-R4-13 信息条尺寸滞后 / FB-R4-14 面板挡对象 / FB-R4-15 测量两点 0.0 / FB-R5-02 重命名 Enter / FB-R5-09 盘右键英文+空盘无名 / FB-R5-10 盘缩略图 / FB-R6-05 新盘无名/设置框标题空 / FB-R6-06 平板设置白底不入主题 / FB-R6-10 移动 X 框回车 / FB-R7-03 Ctrl+G 默认目录/文件名 / FB-R7-06 切片后按钮禁用无法重切 / FB-R7-10 层高范围键入位移 / FB-R8-02 外部 gcode 总时间矛盾 / FB-R12-01 firmware/wipe-tower/print-host/lite 四框未复核 / FB-R1-06 退出偶发段错误（难复现，挂账）。

### 2.4 已确认修复（防重复入账）

FB-R7-08/FB-R10-02/FB-R0-05/FB-R2-06/FB-R3-06/FB-R5-07/RG-R5-01/FB-R7-09/FB-R8-01/FB-R11-01/FB-R11-02；FB-R1-07①、FB-R6-08 大概率已修待抽验；FB-R0-04=FB-P1-05 同一条。

## 3. R1B 视觉审计（round0 矩阵开放式目审 2026-10-29，33/39 张实审）

### 3.1 P1

| ID | 区域 | 描述 | 证据 |
|---|---|---|---|
| R1B-01 | 擦塔设置框 | 内容穿透对话框边界渲染到背景（from/to 冲洗矩阵、Flushing volume/Multiplier 区全部溢出框外）+ 成片英文 + 引用 "Orca Slicer" 品牌名 **→已修(待终验)**：根因=锚点覆写+implicitHeight 异步求值偏小；改 CxDialog 居中+childrenRect 高度+clip，四处英文文案中文化 | round0/dlgL_wipe_tower.png |
| R1B-02 | 首页提示横幅 | 左端文字叠压残影（"调**嘴温度""点击**即可""增加壁层"三处状态复现，浅/深同）=FB-R0-03 确认仍在 | L_home/D_home/L_home_hint.png | **首页提示横幅**

### 3.2 P2

| ID | 区域 | 描述 | 证据 |
|---|---|---|---|
| R1B-03 | 预览页×深色 | 视口清除色+右侧图例面板未随主题（D_prepare 视口已深色，预览页漏——10-26 第⑥条只修了准备页） | D_preview.png |
| R1B-04 | 打印机设置×深色 | 内容面板钉浅色、输入框却深色（半翻转割裂；材料设置深色正常反证为 printer tier 特有） | dlgD_settings_printer vs dlgD_settings_filament |
| R1B-05 | AMS 设置框 | 左上角定位不居中 + 定高裁底（"耗材槽位管理"切半行） | dlgL_ams_settings.png | **AMS 设置框**
| R1B-06 | 固件升级框 | 左上角定位 + 内容两行却大空白 + 关闭按钮悬在中下部不贴右下 | dlgL_firmware.png |
| R1B-07 | 打印主机框 | 左缘标签被裁半（"打印主机上传"→"印主机上传"）+ 顶部预设名裁半 + 窗口标题"物理打印机"与内容标题"打印主机上传"不一致 | dlgL_print_host.png |
| R1B-08 | 精简模式框 | 左上角定位 + "精简预览模式"标题裁半 | dlgL_lite_mode.png |
| R1B-09 | 右键菜单 | 画布/盘右键菜单整页英文（Select All/Arrange/Add Primitive/…/Move plate right 约 17 项；FB-R5-09 范围扩大） | ctx_on_object.png | **右键菜单**
| R1B-10 | 多设备页 | 整页英文：Device/Task Sending/Task Sent 标题+表头+No task/Printing Finish(语法亦误)/View/Edit Printers | L_multi-device.png | **多设备页**
| R1B-11 | 校准页 | 左列表 9 项+副标题+Blocked 说明全英文，与中央中文空态混搭 | L_calibration.png | **校准页**
| R1B-12 | 工艺设置框 | tab（Quality/Strength/…）+组头（Layer height/Seam/…）英文而字段中文——zhGroupLabel 未应用到 process tier（准备页侧栏同数据全中文） | dlgL_settings_process.png | **工艺设置框**
| R1B-13 | 预览页 | 左上视图工具条 "Line Type" 英文（右缘图例同概念已译"走线类型"，同屏不一致） | L_preview.png |
| R1B-14 | 偏好设置 | "通知设置"小节标题浅色模式白字浅底不可读（0ecf1da 判"可读不修"被推翻；深色正常） | dlgL_preferences vs dlgD_preferences |
| R1B-15 | 设备页 | 右上第二个视图切换按钮空白无图标（两主题，深色下白块刺眼） | L_device/D_device.png |
| R1B-16 | 准备页视口 | 盘名 "Untitled"/"01" 绿色斜体大字，上游为灰白小字风格 | L_prepare.png |
| R1B-17 | 配置向导 | 第一步"完成"即可点（门控疑缺失）+ 左下 logo 悬空 + 机型缩略图全为占位线稿（照相机占位挂账同族） | dlgL_config_wizard.png |

### 3.3 P3

| ID | 区域 | 描述 |
|---|---|---|
| R1B-18 | 准备页×浅色 | 视口右侧浮动工具条 6 钮发白对比度低（深色正常）；与顶栏功能重叠属既有挂账设计决策 |
| R1B-19 | 移动工具条 | 两主题底色不一（浅=黄底/深=紫底），与上游深玻璃风格不符 |
| R1B-20 | 预览页 | 迷你热床缩略图白网格浅底对比度低，深色下仍浅底 |
| R1B-21 | 床形状框 | 贴图/模型文件名红色显示语义似报错（文件实存）；预览网格纵向比例与 220 正方床不符待核 |
| R1B-22 | 设置框群 | 宽屏行拉伸 label↔值距离过大（10-26b 旧挂账，本批 printer/process 两框实锤） |
| R1B-23 | 杂项 | 设备页左上淡灰残影图标；校准列表 blocked 项深色下悬空小圆点 |
| R1B-24 | 侧栏变种格 | "hardened_steel" 英文材质名直出（schema 枚举值未译） |
| R1B-25 | 右键菜单 | 弹出位置疑似偏离光标（点击 1195,620 菜单现于 ~805,520，待复核） |
| R1B-26 | 首页 | 提示卡分页控件挤左下（可接受，随 R1B-02 顺看） |

### 3.4 工具问题（非产品）
- 矩阵 --load-model 相对路径不解析（cwd=build/），模型系列 6 张未加载，需绝对路径补拍；
- 首启向导无法经 CLI 复现（QSettings first-run 标志已写）；
- ctx 点击落空床（模型未加载连带），补拍时先载模型再点。

## 4. R1C 功能面对照（/tmp/creality-print 59ae8cb）

软件可补（拍板后施工）：
| ID | 上游功能 | 我方现状 | 级 | 规模 | 状态 |
|---|---|---|---|---|---|
| R1C-01 | Speed/Accel/Fan/Jitter/Limit/Tolerance/XY Offset 校准塔族 | 菜单标 Hardware calibration pending，实为纯 G-code 生成 | P2 | L | 需拍板 |
| R1C-02 | Import SVG 文件（过滤器） | 过滤器缺 *.svg（右键 Add SVG 体已有） | P3 | S | 需拍板 |
| R1C-03 | Scale selection to fit print volume（右键） | 无 | P3 | S | 需拍板 |
| R1C-04 | Generic 3MF 导出 | 无 | P3 | M | 需拍板 |
| R1C-05 | Newer3mfVersionDialog 新版本 3mf 警告 | 无 | P3 | M | 需拍板 |
| R1C-06 | 对象 Settings 作用域覆盖（TabPrintObject/Part/Layer 矩阵） | 部分（SelectionSettingsDialog 替代） | P2 | L | 需拍板 |
| R1C-07 | TextureImportDialog（Emboss 贴图导入） | 无 | P3 | M | 需拍板 |
| R1C-08 | View 菜单 Orthogonal/Perspective 切换确认 | 未确认 | P3 | S | 需复核 |
| R1C-09 | New Window 多窗口 | 无 | P3 | L | 需拍板（建议 deferred：单实例架构改动大） |

契约/硬件阻塞（只记账）：
云绑定（BindMachine/ConnectPrinter）、摄像头实时流、固件 OTA、AMS 设备命令（AMSSettingsDialog 仅本地映射）、多机真实推送（MQTT 契约待真机）、商城/Web 容器（MALL-01 审计锁）、Publish 按钮（依赖云契约）。

## 5. DEFERRED 决策清单（不阻塞收敛，轮报告时统一提请拍板）

1. R1A-11 床尺寸键名统一方案；2. R1A-20/21 预设键集补齐范围（建议高价值子集）；3. R1C 校准塔族是否本战役做；4. R1A-25 Publish 占位按钮处置（激活需云契约/删除需拍板）；5. R1A-33 tooltip 机器翻译质量样张；6. 顶栏与视口右侧浮动工具条功能重叠（10-26 挂账设计决策）；7. FB-R7-06 切片后重切语义。

## 6. 轮次统计

| 轮 | 新发现 P1/P2/P3 | 修复 | RESURFACED | 备注 |
|---|---|---|---|---|
| R1 | 待填 | | | |
