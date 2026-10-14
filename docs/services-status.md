# 服务层 Real / Hybrid / Mock / Placeholder 状态注册表

> 依据：`docs/代码债与架构审计_2026-10-10.md` ④章 S5 第 (2) 步；分类学标准 = `.codex/rules/source-truth-migration.md:51-60`（Status Terms）。
> 建表日期：2026-10-14（S5 落地）。维护规则：每落地一个替换真实后端的工作项，更新对应行的状态与移除条件；新增服务必须带状态与移除条件入表。
> 用途：让"mock→真"的替换路径从"考古 12.5k 行具体类"变成"看表找缝"（S5 为什么不是补丁的验收口径）。

## 状态术语（source-truth-migration.md:54-60 原文口径）

| 术语 | 定义 |
| --- | --- |
| `Real` | source-truth 行为已实现且经验证。 |
| `Hybrid` | 真实路径存在，但仍有 fallback/mock 行为或验证不完整。 |
| `Mock` | 仅本地模拟。 |
| `Blocked` | 依赖/凭据/协议/产品决策不可用（暂无服务落此档，外部阻塞项记入各 Mock 行的移除条件）。 |
| `Placeholder` | 有可见 UI 或枚举但无有意义后端行为。 |

## 顶层服务注册表

| 服务 | 状态 | 真实部分（证据） | 模拟部分（证据） | 移除/转正条件 | UI『演示数据』披露 |
| --- | --- | --- | --- | --- | --- |
| `ProjectService`（S5 正名，原 ProjectServiceMock） | **Real** | 持真 `Slic3r::Model`；导入走 `Model::read_from_file`（ProjectService.cpp:1147）、项目走 `read_from_archive`/`store_bbs_3mf`（ProjectService.cpp:11861 区段、saveProjectAs）；undo/快照/排布/布尔/切镂等全部落真模型。名字中的 Mock 已由 S5 摘除 | `m_mockVolumes` 平行镜像仍作为非 lib 构建回退与部分写入路径的第二真值（S5 第 (3) 步开始消亡：`changeVolumeType` 已改读真实 ModelVolume，11 处镜像重建块已收敛为单一函数；后续条目继续逐消费者退位） | 镜像消亡后续刀：`m_mockVolumes` 降级为可失效缓存或删除（非 lib 回退分支一并退役）；`MockVolumeEntry/MockVolumeType` 命名随退位改真名 | 不适用（Real） |
| `PresetService`（S5 正名，原 PresetServiceMock） | **Real** | 预置枚举/比较/导入导出走上游 profile 真数据（`comparePresets`、`importBundle`；ConfigWizard 厂商/机型/材料枚举）；Q10 系列已把 extruderCount/床面等接真 preset | 插件类预置安装动作等个别入口仍为状态翻转（见 PluginService 行） | 个别入口随真实后端到位逐项转 Real；类名已正名，无整类移除条件 | 不适用（Real） |
| `SliceService` | **Real** | `HAS_LIBSLIC3R` 下走真引擎打印管线 + `export_gcode`（含 P1 直传 `GCodeProcessorResult`）；P4 逐步骤画像门禁 | 无引擎构建（`!HAS_LIBSLIC3R`）才有 `(mock)` 切片回退路径（SliceService.cpp:1406 区段）——该分支仅为无 lib 构建存活 | 无引擎构建分支退役时 mock 路径随之删除；引擎路径无移除条件 | 不适用（Real） |
| `DeviceServiceMock` | **Mock** | 无——设备清单/在线态/打印进度全部模拟（假设备 192.168.1.100-104，DeviceServiceMock.cpp:102-169）；发送链路的 FTP 上传 + MQTT publish 为真实协议客户端（U11：失败如实上报、真实进度） | 设备发现后的列表、连接态、任务进度、AMS 槽位为本地模拟；publish 成功后的打印进度为定时器模拟（DeviceServiceMock.cpp:27,940） | X1(a)(b)：真设备推送（MQTT 订阅遥测）落地 + 真实设备清单替换假 IP；`MqttClient`（明文 1883 改 TLS 8883）与 `FtpUploader` 两个真实协议组件届时直接被真服务复用 | MonitorPage 设备面板横幅（MonitorPage.qml:427-442，R-P1.E）；SelectMachineDialog.qml:815；MultiMachinePage.qml:76；PrintDialog.qml:884 |
| `CloudServiceMock` | **Mock** | 无——登录/绑定/云任务全模拟 | 登录态与云任务列表为本地模拟（HomePage.qml:378 已明示"依赖 bambu_networking，外部阻塞"） | 真云 SDK（bambu_networking 或替代）落地；此前为外部阻塞项 | HomePage.qml:378、HomePage.qml:615 |
| `PluginService` | **Mock** | 持久化启用/安装状态（稳定 id 作键，Q10）为真实本地行为 | `installPlugin()` 是状态翻转，无真实下载源（PluginService.h:24-25,67-70） | 真实插件下载/安装通道（HTTP 源 + 依赖安装）落地 | PluginManagerDialog（S5 第 (4) 步补披露横幅） |
| `CameraServiceMock` | **Mock**（附注：底层解码组件为真） | `CameraStream`（FFmpeg RTSP/RTMP 解码线程，CameraStream.h:1-40）在用户填入 rtsp:// URL 时真实拉流出帧（CameraServiceMock.cpp:223-260，v2.6 CAM-03） | 流状态机为 mock 时序（无条件翻 Streaming，X1(c) 待替换为真实拉流状态真相）；无 URL/无设备路径全模拟；截图无帧时回退 /tmp/ 假路径 | X1(c)：状态机改读真实拉流状态（连不上即错误态）+ 截图真实落盘；届时按 taxonomy 升 Hybrid→Real | MonitorPage 摄像头面板（S5 第 (4) 步加显式演示数据角标；面板横幅 MonitorPage.qml:436 已含"摄像头"） |
| `MonitorViewModel`（Monitor 域） | **Hybrid** | 真实发送链路（FTP 上传进度 + MQTT publish，U11；`MqttClient`/`FtpUploader` 真协议）+ 真实网络探测（`NetworkServiceMock::probe` 真 ICMP） | 消费的设备清单/遥测/AMS 槽位来自 DeviceServiceMock 模拟数据 | 随 DeviceServiceMock→真服务切换整体转 Real | MonitorPage.qml:427-442 横幅随 Device 行维护 |
| `AmsMaterialsViewModel`（Ams 域） | **Hybrid** | 槽位材质/映射规则为真实本地状态：QSettings 持久化（Phase 201，ams/materials/*）、映射规则增删/重置为真 API | 槽位数据源无真设备通道（AMSSettingsDialog.qml:41-45：capability bits 为 mock 默认；设置 toggle 会话级） | 真设备 AMS 遥测通道落地（依赖 X1 设备面）；届时本地持久化逻辑保留、数据源换真 | MonitorPage AMS 槽位条 + AMSSettingsDialog 底部披露（S5 第 (4) 步新增） |
| `CalibrationServiceMock` | **Hybrid** | 流量动学/压力前进（PA）校准有真计算回读：`hasRealReadback` 仅当 libslic3r 吐出机器可读结果标记才置真（CalibrationServiceMock.h 注释区段，Phase 125 CALIB-03） | 硬件类校准（热床调平等）无真设备通道，流程/历史条目部分为本地模拟 | 真设备校准指令通道落地（依赖 X1）；slice 驱动类校准可先行逐项转 Real | CalibrationPage 校准条目 `unavailableReason` 如实标注不可用项（既有） |
| `NetworkServiceMock` | **Real**（类名历史误名） | `probe()` 真 ICMP 连通性检测、`discoverDevices()` 真 SSDP 发现（NetworkServiceMock.h:7 "v2.6 真实化 NetworkService（替代 mock）"；`SsdpDiscovery` 真 UDP 多播，SsdpDiscovery.h:1-30） | `latencyMs` 语义为探测往返近似；类名 Mock 为历史误名（行为已真实化未正名） | 正名候选（后续低风险批次）：类名/文件名去 Mock；无行为移除条件 | 不适用（Real） |
| `AppSettingsService` | **Real** | QSettings 偏好持久化，无模拟面 | — | — | 不适用 |
| `UndoRedoManager` / `UndoCommands` | **Real** | 命令栈基础设施 + 真模型快照命令（U9 扩容后覆盖排布/摆向/拆分/导入/拖放） | — | — | 不适用 |

## 真实协议/算法组件（被上述服务消费，非顶层服务）

| 组件 | 状态 | 说明 |
| --- | --- | --- |
| `MqttClient` | Real | 真 MQTT 发布链路（U11）；X1(a) 需补 TLS 8883 + 订阅遥测 |
| `FtpUploader` | Real | 真 FTP 上传 + 真实进度回调（U11 接通 UI） |
| `SsdpDiscovery` | Real | SSDP/UDP 多播局域网发现（Bambu/Creality ST 应答解析） |
| `CameraStream` | Real | FFmpeg H.264 RTSP/RTMP 解码线程（v2.6 CAM-01/02） |

## 注册表与 UI 披露的耦合规则（S5 第 (4) 步成文）

- 状态为 `Mock` 的数据源，其每一个 UI 消费面必须挂显式『演示数据/演示模式』披露；`Hybrid` 的披露需写明"哪半边是模拟"。披露文案变更与状态变更同批提交，防止注册表与 UI 真话漂移。
- 现存披露锚点：MonitorPage.qml:427-442（设备/连接/摄像头/打印控制，R-P1.E）、MonitorPage.qml:214（添加设备 toast）、HomePage.qml:378/:615（云登录/绑定）、SelectMachineDialog.qml:815、MultiMachinePage.qml:76、PrintDialog.qml:884（发送仪表）；S5 新增：MonitorPage AMS 槽位条角标、AMSSettingsDialog 底部披露、MonitorPage 摄像头面板角标、PluginManagerDialog 横幅。
