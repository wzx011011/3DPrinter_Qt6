# CrealityPrint 7.0 — Qt/QML GUI 重写架构文档

> **版本**: 2.0  
> **日期**: 2026-03-01  
> **作者**: Architecture Team  
> **状态**: Final

| 版本 | 日期       | 变更摘要                                                                                                                     |
| ---- | ---------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 1.0  | 2026-02-28 | 初稿：11 章 + 3 附录                                                                                                         |
| 1.1  | 2026-03-01 | 逻辑审核修复：架构图/MVP/VM 注册/Service 补全                                                                                |
| 2.0  | 2026-03-01 | **最终版**：新增 ADR、线程模型、VM-Service 依赖矩阵、错误处理策略、CI/CD 管线、任务依赖图、性能预算、回滚/降级/安全/迁移方案 |

---

## 目录

1. [项目概述](#1-项目概述)
2. [现有系统分析](#2-现有系统分析)
3. [技术选型](#3-技术选型)
4. [系统架构设计](#4-系统架构设计)
5. [模块详细设计](#5-模块详细设计)
6. [3D 渲染桥接层](#6-3d-渲染桥接层)
7. [数据流与状态管理](#7-数据流与状态管理)
8. [构建系统与依赖管理](#8-构建系统与依赖管理)
9. [测试策略](#9-测试策略)
10. [开发计划与里程碑](#10-开发计划与里程碑)
11. [风险与缓解措施](#11-风险与缓解措施)

---

## 1. 项目概述

### 1.1 背景

CrealityPrint 是基于 OrcaSlicer/BambuStudio/PrusaSlicer 谱系的开源 FDM 3D 打印切片软件。当前版本使用 **wxWidgets 3.1 + 原生 OpenGL + ImGui** 构建 GUI，存在以下痛点：

| 痛点                           | 影响                             |
| ------------------------------ | -------------------------------- |
| wxWidgets 控件老旧，自定义困难 | UI/UX 现代化受限，开发效率低     |
| C++ 全量编译 UI 层             | 迭代周期长（全量构建 > 30 分钟） |
| 特效/动画能力弱                | 产品体验落后竞品                 |
| 3D 视口与 UI 框架耦合          | 渲染层维护困难                   |
| 跨平台一致性差                 | Win/macOS/Linux 外观不统一       |

### 1.2 目标

- **功能一比一复刻**：所有现有功能完整迁移，不丢失任何用户可见功能
- **技术架构升级**：采用 Qt 6.8 LTS + QML 替代 wxWidgets
- **保留切片引擎**：libslic3r 核心完全不动，仅替换 GUI 框架
- **渐进式迁移**：分阶段实施，每阶段可独立验收

### 1.3 范围

| 范围内                   | 范围外                                   |
| ------------------------ | ---------------------------------------- |
| src/slic3r/GUI/ 全部模块 | src/libslic3r/ 切片引擎                  |
| 3D 视口渲染桥接          | G-code 生成逻辑                          |
| 设备管理 / 网络通信 UI   | bambu_networking DLL 协议与 ABI 定义变更 |
| 打印配置面板             | 切片算法本身                             |
| 多语言系统               | 固件更新逻辑                             |

---

## 2. 现有系统分析

### 2.1 GUI 模块全景

```
src/slic3r/GUI/
├── GUI_App.cpp/h              // 应用入口 & 全局上下文
├── MainFrame.cpp/h            // 主窗口框架 (11 个 TabPosition)
├── Plater.cpp/h               // 工作区主视图 + Sidebar
├── GLCanvas3D.cpp/h           // 3D 视口 (1431 行头文件)
├── GCodeViewer.cpp/h          // G-code 预览 (13 种视图模式)
├── Tab.cpp/h                  // 设置面板 (Print/Filament/Printer)
├── DeviceManager.cpp/h        // 设备管理 (80+ 命令)
├── BackgroundSlicingProcess.*  // 后台切片状态机
├── Gizmos/                    // 3D 操作工具
├── Widgets/                   // 自定义 wxWidgets 控件
└── ...                        // 200+ 其他文件
```

### 2.2 核心类分析

#### 2.2.1 MainFrame — 主窗口

```cpp
// 11 个标签页位置
enum TabPosition {
    tpHome, tp3DEditor, tpPreview, tpMonitor,
    tpProject, tpCalibration, tpAuxiliary,
    tpDeviceList, tpPreferences, tpModelMall, tpMultiMachine
};

// 14 种打印选择类型
enum PrintSelectType {
    eLocalPrint, eSendGcode, eExportGcode, eExportSlicedFile,
    eUploadGcode, eExportAllSlicedFile, eSendToPrinterAll,
    eSendToPrinter, eExportGcodeAll, eUploadGcodeAll,
    ePrintPlate, ePrintAll, eCalibrationPrint, eCalibrationPrintAll
};
```

**关键组件**: BBLTopbar (自定义标题栏), Notebook (标签切换), PrinterWebView, SettingsDialog

#### 2.2.2 GLCanvas3D — 3D 视口

```cpp
// 3 种画布类型
enum ECanvasType { CanvasView3D, CanvasPreview, CanvasAssembleView };

// 核心子系统
class GLCanvas3D {
    Camera              m_camera;          // 相机控制
    Selection           m_selection;       // 对象选择
    GLGizmosManager     m_gizmos;          // Gizmo 工具管理
    GLVolumeCollection  m_volumes;         // 体积渲染数据
    SceneRaycaster      m_scene_raycaster; // 射线拾取
    LayersEditing       m_layers_editing;  // 层高编辑
    ClippingPlane       m_clipping_planes[2]; // 裁切面

    // 工具栏
    GLToolbar           m_main_toolbar;      // 主工具栏
    GLToolbar           m_separator_toolbar;  // 分隔工具栏
    GLToolbar           m_collapse_toolbar;   // 折叠工具栏
    IMToolbar           m_assembly_toolbar;   // 装配工具栏

    // 排列/朝向设置
    ArrangeSettings     m_arrange_settings_fff;
    OrientSettings      m_orient_settings;
};

// 7 种警告类型
enum WarningType {
    ObjectOutside, ToolpathOutside, SlasupportOutside,
    SomethingNotShown, ObjectClashed, MultiModeLimit, MeshError
};
```

#### 2.2.3 GCodeViewer — G-code 预览

```cpp
// 13 种视图模式
enum class EViewType : unsigned char {
    FeatureType, Height, Width, Feedrate, FanSpeed,
    Temperature, VolumetricRate, LayerTime, LayerTimeLog,
    Tool, ColorPrint, FilamentId, Chronology
};

// 多缓冲渲染系统
struct VBuffer { /* 顶点缓冲 */ };
struct IBuffer { /* 索引缓冲 */ };
struct TBuffer { /* 渲染批次 */ };

// 时间线导航
struct SequentialView {
    Marker marker;         // 当前位置标记
    GCodeWindow gcode_win; // G-code 行显示
    unsigned int first, current, last; // 帧范围
};

// 图层导航
struct Layers {
    std::vector<double> zs;
    std::array<unsigned int, 2> range; // [first, last]
};

// 辅助系统
IMSlider*  m_layers_slider;   // 层滑块
IMSlider*  m_moves_slider;    // 移动滑块
Shells     m_shells;          // 模型外壳叠加
```

#### 2.2.4 Tab 设置面板

```cpp
// 继承层次
class Tab                     // 基类：页面管理、搜索、dirty 状态
├── class TabPrint            // 打印参数 (质量/速度/支撑/...)
├── class TabFilament         // 耗材参数 (温度/冷却/高级)
├── class TabPrinter          // 打印机参数 (通用/挤出机/定制G-code)
└── class TabPrintModel       // 单模型覆盖参数
    ├── class TabPrintPart    // 部件级覆盖
    ├── class TabPrintPlate   // 平板级覆盖
    └── class TabPrintLayer   // 层范围覆盖

// 动态 UI 构建
class ConfigOptionsGroup {
    std::map<t_config_option_key, Option> m_options;
    // 根据 ConfigOptionDef 动态生成控件
};
```

#### 2.2.5 DeviceManager — 设备管理

```cpp
// 打印机架构类型
enum PrinterArch { ARCH_CORE_XY, ARCH_I3, ARCH_DELTA, ARCH_BELT };
enum PrinterSeries { SERIES_X1, SERIES_P1P, SERIES_PELICAN, ... };

class MachineObject {
    // AMS 多色系统
    std::map<int, Ams*> amsList;     // AMS 单元
    std::vector<AmsTray*> vt_tray;   // 虚拟托盘

    // 80+ 控制命令
    int command_set_printing_speed(PrintingSpeedLevel lvl);
    int command_ams_filament_setting(int id, ...);
    int command_set_chamber_light(LightEffect effect, int on_time, ...);
    int command_set_nozzle_light(LightEffect effect, int on_time, ...);
    int command_start_extrusion_cali(int tray_id, ...);
    int command_ipcam_record(bool on_off);
    int command_ipcam_timelapse(bool on_off);
    int command_xcam_control_ai_monitoring(bool on_off, ...);
    int command_set_printing_option(bool auto_recovery);
    // ... 等等

    // 状态属性
    PrinterArch printer_arch;
    float nozzle_temp, nozzle_temp_target;
    float bed_temp, bed_temp_target;
    float chamber_temp, chamber_temp_target;
    int mc_print_percent, mc_left_time;
    std::string subtask_name, print_status;
};
```

#### 2.2.6 BackgroundSlicingProcess — 后台切片

```cpp
// 7 状态切片状态机
enum State { INITIAL, IDLE, STARTED, RUNNING, FINISHED, CANCELED, EXIT };

class BackgroundSlicingProcess {
    State                m_state;
    Print               *m_fff_print;         // FFF 打印对象
    SL1Archive          *m_sla_archive;       // SLA 归档
    GCodeProcessorResult *m_gcode_result;     // G-code 处理结果

    // 回调
    std::function<void()> m_fff_print_invalidated_callback;
    std::function<void()> m_finished_callback;

    // UI 同步
    struct UITask {
        std::function<void()> task;
        std::atomic<bool>     done{false};
    };
    std::queue<UITask> m_ui_tasks;  // 线程安全 UI 任务队列
};
```

#### 2.2.7 Preset 系统

```cpp
// 10 种预设类型
enum Type {
    TYPE_INVALID, TYPE_PRINT, TYPE_SLA_PRINT, TYPE_FILAMENT,
    TYPE_SLA_MATERIAL, TYPE_PRINTER, TYPE_CONFIG_BUNDLE,
    TYPE_PHYSICAL_PRINTER, TYPE_SYSTEM, TYPE_PROJECT
};

struct Preset {
    Type           type;
    std::string    name, alias;
    DynamicPrintConfig config;   // 配置数据
    bool           is_default, is_system, is_visible;
    std::string    setting_id;   // 云同步 ID
    std::string    base_id;      // 继承父 ID
    std::string    filament_id, version;
};

class PresetBundle {
    PresetCollection  prints, sla_prints;
    PresetCollection  filaments, sla_materials;
    PresetCollection  printers, physical_printers;
    VendorMap         vendors;               // 厂商配置
    AppConfig*        m_appconfig;           // 应用配置
};
```

#### 2.2.8 NetworkAgent — 网络通信

```cpp
// 动态 DLL 加载模式
class NetworkAgent {
    static void* netdb;  // DLL handle

    // 通过函数指针调用 (12+ 回调)
    static func_on_ssdp_msg_fn         on_ssdp_msg_fn;
    static func_on_user_login_fn       on_user_login_fn;
    static func_on_printer_connected_fn on_printer_connected_fn;
    static func_on_server_connected_fn  on_server_connected_fn;
    static func_on_http_error_fn       on_http_error_fn;
    static func_on_message_fn          on_message_fn;

    // API 类别
    // - 用户认证: user_login, user_logout, is_user_login
    // - 打印任务: start_print, start_local_print_with_record
    // - 云服务: get_user_presets, get_setting_list, put_setting
    // - MQTT: connect_printer, disconnect_printer, send_message
    // - 模型商城: get_model_mall_home, get_model_mall_detail
    // - 设备绑定: bind_device, unbind_device, get_device_list
    // - 遥测: track_event, track_update_property
};
```

### 2.3 文件格式支持

| 格式        | 导入 | 导出 | 实现文件                        |
| ----------- | ---- | ---- | ------------------------------- |
| STL         | ✅   | ✅   | Format/STL.cpp                  |
| 3MF         | ✅   | ✅   | Format/3mf.cpp                  |
| BBS 3MF     | ✅   | ✅   | Format/bbs_3mf.cpp (带切片数据) |
| AMF         | ✅   | ✅   | Format/AMF.cpp                  |
| OBJ         | ✅   | ✅   | Format/OBJ.cpp                  |
| STEP/STP    | ✅   | ❌   | Format/STEP.cpp (via OCCT)      |
| SL1/SL1S    | ✅   | ✅   | Format/SL1.cpp                  |
| SVG         | ❌   | ✅   | Format/SVG.cpp                  |
| Assimp 通用 | ✅   | ❌   | Format/Assimp.cpp               |

### 2.4 耦合分析

```
┌──────────────────────────────────────────────────────────────┐
│                    wxWidgets GUI Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐   │
│  │MainFrame │ │ Plater   │ │ Tab      │ │ DeviceManager │   │
│  │(wxFrame) │ │(wxPanel) │ │(wxPanel) │ │ (wxPanel)     │   │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └───────┬───────┘   │
│       │             │            │                │           │
│  ┌────┴─────────────┴────────────┴────────────────┴────────┐ │
│  │              wxWidgets Event System                       │ │
│  └────────────────────────┬─────────────────────────────────┘ │
│                           │                                   │
│  ┌────────────────────────┴─────────────────────────────────┐ │
│  │         OpenGL Context (wxGLCanvas → GLCanvas3D)          │ │
│  └──────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬───────────────────────────────┘
                               │ (紧耦合约 200+ 文件)
┌──────────────────────────────┴───────────────────────────────┐
│                    libslic3r Core Engine                      │
│  Print / GCode / Layer / Fill / Support / TriangleMesh       │
│  Config / Preset / PresetBundle / Model                      │
└──────────────────────────────────────────────────────────────┘
```

**耦合热点分析**:

| 耦合区域       | 文件数 | 耦合类型           | 迁移难度        |
| -------------- | ------ | ------------------ | --------------- |
| wxWidgets 控件 | ~150   | wx 类继承          | 中 (QML 替代)   |
| OpenGL 视口    | ~20    | wxGLCanvas 上下文  | 低 (FBO 桥接)   |
| 事件系统       | ~100   | wxEvent 宏         | 中 (Qt 信号槽)  |
| Config 绑定    | ~30    | wxWidgets ↔ Config | 高 (需重新绑定) |
| ImGui 覆盖     | ~10    | GL 直接渲染        | 低 (保留或移除) |

---

## 3. 技术选型

### 3.1 对比分析

| 维度         | wxWidgets (现有) | Qt 6 QML  | Electron | Flutter  |
| ------------ | ---------------- | --------- | -------- | -------- |
| 语言         | C++              | C++ + QML | JS/TS    | Dart     |
| 3D 集成      | 原生 OpenGL      | QQuickFBO | WebGL    | 外部纹理 |
| 渲染性能     | ★★★★★            | ★★★★☆     | ★★☆      | ★★★      |
| 开发效率     | ★★☆              | ★★★★☆     | ★★★★★    | ★★★★     |
| 热重载       | ❌               | ✅ (QML)  | ✅       | ✅       |
| C++ 互操作   | 原生             | 原生      | FFI/NAPI | FFI      |
| 许可证       | wxWindows        | LGPL v3   | MIT      | BSD      |
| 包体积增量   | 0                | +40MB     | +120MB   | +15MB    |
| 跨平台一致性 | ★★★              | ★★★★★     | ★★★★★    | ★★★★★    |

### 3.2 选型决策

**选定方案: Qt 6.8 LTS + QML + Qt Quick Controls 2**

理由:

1. **C++ 原生互操作** — libslic3r 零成本调用，无 IPC/FFI 开销
2. **QML 声明式 UI** — 热重载 + 数据绑定，开发效率提升 3-5 倍
3. **QQuickFramebufferObject** — 完美桥接现有 OpenGL 渲染代码，90% 复用
4. **成熟 3D 生态** — Qt 3D、Quick3D 可选用
5. **LGPL v3** — 开源项目完全兼容，动态链接即可
6. **长期支持** — Qt 6.8 LTS 支持到 2029 年

### 3.3 Qt 6.8 模块清单

```
Qt Core             — 基础类型、事件循环、线程
Qt GUI              — 窗口系统、OpenGL 上下文
Qt Quick            — QML 引擎、场景图
Qt Quick Controls 2 — UI 控件库
Qt Quick Layouts    — 布局管理
Qt OpenGL           — GL 集成
Qt Network          — HTTP/WebSocket
Qt WebSockets       — 设备信令/实时通道
Qt Svg              — SVG 支持
Qt Multimedia       — 摄像头流 (设备监控)
Qt LinguistTools    — 国际化
Qt ShaderTools      — 着色器编译
```

### 3.4 架构原则与关键决策 (ADR)

以下记录本项目核心架构决策，供后续评审追溯：

| ID     | 决策点           | 备选方案                      | 最终选择           | 核心理由                                    |
| ------ | ---------------- | ----------------------------- | ------------------ | ------------------------------------------- |
| ADR-01 | UI 框架          | Qt QML / Electron / Flutter   | **Qt 6.8 QML**     | C++ 原生互操作零开销；QQuickFBO 桥接现有 GL |
| ADR-02 | 3D 集成方式      | Qt3D / QQuickFBO / 独立子窗口 | **QQuickFBO**      | 最大化复用现有 OpenGL 渲染代码（~85%）      |
| ADR-03 | 状态管理模式     | Redux-like / MVVM / MVC       | **MVVM**           | Qt 信号-属性绑定天然契合 MVVM               |
| ADR-04 | ViewModel 粒度   | 每页一个 VM / 多页共享 VM     | **9 VM : 11 Page** | 逻辑相近页面复用 VM，避免空壳类             |
| ADR-05 | Service 生命周期 | 单例 / 依赖注入容器           | **单例**           | 项目规模适中；Service 间无状态竞争          |
| ADR-06 | 双版本共存策略   | CMake 编译开关 / 独立仓库     | **CMake option**   | 同一代码库并行开发，Service 层共享          |
| ADR-07 | 网络层迁移       | 重写协议栈 / 桥接现有 DLL     | **桥接 DLL**       | ABI 不变，风险最低，工作量可控              |
| ADR-08 | ImGui 覆盖层     | 保留 / QML 替代               | **QML 替代**       | 统一渲染管线，消除 GL 直接绘制冲突          |

> **变更规则**: 任何 ADR 修改须经架构评审会议批准，并在本表追补"废弃日期"列。

---

## 4. 系统架构设计

### 4.1 四层 MVVM 架构

```
╔══════════════════════════════════════════════════════════════════════════════════╗
║  Layer 1: QML UI Layer (声明式) — 11 个页面                                      ║
║                                                                                  ║
║  ┌──────┐ ┌──────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌─────────┐          ║
║  │ Home │ │Editor│ │Preview│ │Monitor│ │Project│ │Calibr.│ │Auxiliary│          ║
║  └──┬───┘ └──┬───┘ └──┬────┘ └──┬────┘ └──┬────┘ └──┬────┘ └────┬────┘          ║
║     │        │        │        │        │        │             │                ║
║  ┌──────┐ ┌──────────┐ ┌───────────┐ ┌───────────┐                               ║
║  │DevLst│ │Preference│ │ ModelMall │ │MultiMach. │                               ║
║  └──┬───┘ └────┬─────┘ └─────┬─────┘ └─────┬─────┘                               ║
╠═════╪══════════╪═════════════╪═════════════╪═════════════════════════════════════╣
║  Layer 2: ViewModel Layer (C++ Q_OBJECT) — 多对一映射                             ║
║                                                                                  ║
║  ┌────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐ ┌────────────┐              ║
║  │ HomeVM │ │ EditorVM │ │PreviewVM │ │ MonitorVM  │ │ SettingsVM │              ║
║  └────┬───┘ └────┬─────┘ └────┬─────┘ └─────┬──────┘ └─────┬──────┘              ║
║       │          │            │             │              │                     ║
║  ┌──────────┐ ┌────────────┐ ┌──────────────┐ ┌──────────────┐                    ║
║  │ProjectVM │ │CalibrationVM│ │ ModelMallVM  │ │MultiMachineVM│                    ║
║  └────┬─────┘ └─────┬──────┘ └──────┬───────┘ └──────┬───────┘                    ║
║       │             │               │                │                           ║
║  说明: DeviceListPage 复用 MonitorVM; AuxiliaryPage 复用 ProjectVM;               ║
║        PreferencesPage 复用 SettingsVM — 轻量页面无需独立 VM                       ║
╠═══════╪═════════════╪═══════════════╪════════════════╪═══════════════════════════╣
║  Layer 3: Service Layer (C++ 业务逻辑)                                            ║
║  ┌─────────────┐ ┌──────────┐ ┌────────────┐ ┌──────────┐ ┌──────────────────┐   ║
║  │SliceService │ │PresetSvc │ │ DeviceSvc  │ │ProjectSvc│ │  NetworkService  │   ║
║  │  + BGSP     │ │ + Bundle │ │  + MQTT    │ │  + I/O   │ │  + DLL bridge    │   ║
║  └──────┬──────┘ └────┬─────┘ └─────┬──────┘ └────┬─────┘ └────────┬─────────┘   ║
║         │             │             │             │                │             ║
║  ┌──────────────┐ ┌──────────────┐ ┌─────────────────┐                            ║
║  │CalibrationSvc│ │ModelMallSvc  │ │MultiMachineSvc  │                            ║
║  └──────┬───────┘ └──────┬───────┘ └────────┬────────┘                            ║
╠═════════╪════════════════╪══════════════════╪════════════════════════════════════╣
║  Layer 4: Engine Layer (libslic3r — 不修改)                                       ║
║  ┌──────┴────────────────┴──────────────────┴──────────────────────────────────┐  ║
║  │  Print / GCode / Model / Config / Preset / Format / TriangleMesh           │  ║
║  └────────────────────────────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════════════════════════╝
```

### 4.2 目录结构

```
src/
├── libslic3r/                 # [不修改] 切片引擎
├── slic3r/
│   ├── GUI/                   # [废弃] wxWidgets GUI (渐进移除)
│   └── Service/               # [新建] 服务层 (从 GUI 抽取)
│       ├── SliceService.h/cpp
│       ├── PresetService.h/cpp
│       ├── DeviceService.h/cpp
│       ├── ProjectService.h/cpp
│       ├── NetworkService.h/cpp
│       ├── CalibrationService.h/cpp
│       ├── ModelMallService.h/cpp
│       └── MultiMachineService.h/cpp
├── qml_gui/                   # [新建] Qt/QML GUI
│   ├── main.cpp               # Qt 应用入口
│   ├── App.h/cpp              # QGuiApplication 子类
│   ├── ViewModels/            # ViewModel 层
│   │   ├── HomeViewModel.h/cpp
│   │   ├── EditorViewModel.h/cpp
│   │   ├── PreviewViewModel.h/cpp
│   │   ├── MonitorViewModel.h/cpp   # 同时服务 DeviceListPage
│   │   ├── SettingsViewModel.h/cpp  # 同时服务 PreferencesPage
│   │   ├── PlateViewModel.h/cpp
│   │   ├── ProjectViewModel.h/cpp   # 同时服务 AuxiliaryPage
│   │   ├── CalibrationViewModel.h/cpp
│   │   ├── ModelMallViewModel.h/cpp
│   │   └── MultiMachineViewModel.h/cpp
│   ├── Renderer/              # OpenGL 桥接
│   │   ├── GLViewport.h/cpp          # QQuickFramebufferObject
│   │   ├── GLViewportRenderer.h/cpp  # Renderer 实现
│   │   ├── GCodeRenderer.h/cpp       # G-code 预览渲染
│   │   └── CameraController.h/cpp    # 相机控制
│   ├── Models/                # Qt 数据模型
│   │   ├── ObjectListModel.h/cpp
│   │   ├── PresetListModel.h/cpp
│   │   ├── DeviceListModel.h/cpp
│   │   ├── FilamentModel.h/cpp
│   │   └── GCodeLayerModel.h/cpp
│   └── qml/                   # QML 文件
│       ├── main.qml
│       ├── Theme.qml
│       ├── pages/
│       │   ├── HomePage.qml
│       │   ├── EditorPage.qml
│       │   ├── PreviewPage.qml
│       │   ├── MonitorPage.qml
│       │   ├── ProjectPage.qml
│       │   ├── CalibrationPage.qml
│       │   ├── AuxiliaryPage.qml
│       │   ├── DeviceListPage.qml
│       │   ├── PreferencesPage.qml   # 应用偏好 (语言/主题/快捷键)
│       │   ├── ModelMallPage.qml
│       │   ├── MultiMachinePage.qml
│       │   └── SettingsPage.qml      # 切片参数独立全屏编辑 (Tab 等价)
│       ├── panels/
│       │   ├── Sidebar.qml
│       │   ├── ObjectList.qml
│       │   ├── PrintSettings.qml
│       │   ├── FilamentSettings.qml
│       │   ├── PrinterSettings.qml
│       │   └── SliceProgress.qml
│       ├── dialogs/
│       │   ├── PrintDialog.qml
│       │   ├── CalibrationDialog.qml
│       │   └── AboutDialog.qml
│       ├── components/
│       │   ├── Toolbar.qml
│       │   ├── GizmoBar.qml
│       │   ├── LayerSlider.qml
│       │   ├── MoveSlider.qml
│       │   ├── ColorPicker.qml
│       │   └── StatusBar.qml
│       └── controls/
│           ├── CxButton.qml
│           ├── CxComboBox.qml
│           ├── CxTextField.qml
│           ├── CxSlider.qml
│           ├── CxCheckBox.qml
│           └── CxSpinBox.qml
```

### 4.3 启动流程

```
main()
  │
  ├─ QGuiApplication app(argc, argv)
  ├─ QQmlApplicationEngine engine
  │
  ├─ // 初始化 Service 层
  ├─ PresetService::instance().init(resourceDir)
  ├─ SliceService::instance().init()
  ├─ DeviceService::instance().init()
  ├─ NetworkService::instance().init()   // 加载 bambu_networking DLL
  │
  ├─ // 注册 ViewModel 到 QML (9 个 VM 服务 11 个页面)
  ├─ qmlRegisterSingletonType<HomeViewModel>(...)
  ├─ qmlRegisterSingletonType<EditorViewModel>(...)
  ├─ qmlRegisterSingletonType<PreviewViewModel>(...)
  ├─ qmlRegisterSingletonType<MonitorViewModel>(...)     // → Monitor + DeviceList
  ├─ qmlRegisterSingletonType<SettingsViewModel>(...)    // → Settings + Preferences
  ├─ qmlRegisterSingletonType<ProjectViewModel>(...)     // → Project + Auxiliary
  ├─ qmlRegisterSingletonType<CalibrationViewModel>(...)
  ├─ qmlRegisterSingletonType<ModelMallViewModel>(...)
  ├─ qmlRegisterSingletonType<MultiMachineViewModel>(...)
  │
  ├─ // 注册 OpenGL 自定义项
  ├─ qmlRegisterType<GLViewport>("Creality.GL", 1, 0, "GLViewport")
  │
  ├─ engine.load("qrc:/qml/main.qml")
  └─ app.exec()
```

### 4.4 线程模型

```
┌─────────────────────────────────────────────────────────────────┐
│  Main Thread (QML Engine / GUI 消息循环)                         │
│   ├── QML Scene Graph 同步 (beforeSynchronize)                  │
│   ├── ViewModel Q_PROPERTY 读写 & 信号触发                      │
│   ├── Service 层轻量操作 (硬性要求 < 16 ms / 帧)               │
│   └── 所有 QML 对象的创建与销毁                                 │
├─────────────────────────────────────────────────────────────────┤
│  Render Thread (Qt Scene Graph 专用)                             │
│   ├── QQuickFramebufferObject::Renderer::render()              │
│   ├── GLCanvas3D / GCodeViewer 实际 OpenGL 调用                 │
│   └── synchronize() 是唯一安全与 QML 线程交换数据的时机         │
├─────────────────────────────────────────────────────────────────┤
│  Worker Thread Pool (Intel TBB taskgroup)                        │
│   ├── BackgroundSlicingProcess 切片 (最大线程由 TBB 调度)       │
│   ├── 模型导入/导出 (异步 I/O, QtConcurrent::run)              │
│   └── 大型 G-code 解析 (GCodeProcessor)                        │
├─────────────────────────────────────────────────────────────────┤
│  Network I/O Thread                                              │
│   ├── QNetworkAccessManager 异步 HTTP                           │
│   ├── bambu_networking DLL 回调 (MQTT/SSDP)                    │
│   └── WebSocket 信令通道                                        │
└─────────────────────────────────────────────────────────────────┘
```

**跨线程通信规则**：

| 方向           | 机制                                              | 说明                  |
| -------------- | ------------------------------------------------- | --------------------- |
| Worker → Main  | `QMetaObject::invokeMethod(Qt::QueuedConnection)` | 切片进度/完成回调     |
| Network → Main | Qt 信号/槽自动跨线程投递                          | MQTT 消息 → ViewModel |
| Main → Render  | `synchronize()` 同步点                            | Qt 框架保证时序安全   |
| Render → Main  | `QQuickItem::update()` 请求重绘                   | 不直接操作 QML 属性   |
| **禁止**       | 任何非 Main 线程直接读写 QML 对象                 | 违反则 UB / 崩溃      |

### 4.5 ViewModel ↔ Service 依赖矩阵

下表明确每个 ViewModel 对 Service 层的依赖关系，便于并行开发分工和接口契约定义。

| ViewModel          | SliceSvc | PresetSvc | DeviceSvc | ProjectSvc | NetworkSvc | CalibSvc | MallSvc | MultiMacSvc |
| ------------------ | :------: | :-------: | :-------: | :--------: | :--------: | :------: | :-----: | :---------: |
| **HomeVM**         |          |     R     |           |     R      |     R      |          |         |             |
| **EditorVM**       |    ●     |     R     |           |     ●      |            |          |         |             |
| **PreviewVM**      |    R     |           |           |            |            |          |         |             |
| **MonitorVM**      |          |           |     ●     |            |     ●      |          |         |             |
| **SettingsVM**     |          |     ●     |           |            |            |          |         |             |
| **ProjectVM**      |          |     R     |           |     ●      |            |          |         |             |
| **CalibrationVM**  |    ●     |     R     |     ●     |            |            |    ●     |         |             |
| **ModelMallVM**    |          |           |           |            |     ●      |          |    ●    |             |
| **MultiMachineVM** |          |           |     ●     |            |     ●      |          |         |      ●      |

> ● = 主依赖（核心功能调用） R = 辅助只读依赖 空 = 无依赖
>
> **开发规则**: 先完成 ● 标记的 Service 接口，再实现 R 依赖；无依赖的 VM-Service 对可并行开发。

### 4.6 统一错误处理与传播策略

```
Layer 4 (libslic3r)     → C++ 异常 / PlaceholderParser::runtime_error / 返回码
       │ try-catch + 包装
Layer 3 (Service)        → ServiceResult<T> { ok, value, error: {code, message, severity} }
       │ emit errorOccurred(ServiceError)
Layer 2 (ViewModel)      → Q_PROPERTY(QString lastError) + Q_PROPERTY(int errorSeverity)
       │ QML Binding
Layer 1 (QML)            → ErrorBanner / Toast / ConfirmDialog (根据 severity 自动选择)
```

**错误严重级别定义**：

| 级别    | 枚举值 | UI 表现                      | 自动恢复 | 示例                         |
| ------- | :----: | ---------------------------- | :------: | ---------------------------- |
| Info    |   0    | Toast 3 秒自动消失           |    —     | "切片完成，耗时 42s"         |
| Warning |   1    | 顶部 Banner 可手动关闭       |    —     | "模型 A 超出打印范围"        |
| Error   |   2    | 模态 Dialog 需用户确认       |    否    | "导入文件损坏：CRC 校验失败" |
| Fatal   |   3    | Dialog + 自动保存 + 建议重启 |    否    | "OpenGL 上下文丢失"          |

```cpp
// 统一错误类型 — 所有 Service 层均使用
struct ServiceError {
    int code;                  // 业务错误码 (模块前缀: 1xxx=Slice, 2xxx=Preset,
                               //   3xxx=Device, 4xxx=Project, 5xxx=Network, ...)
    QString message;           // 用户可读描述 (已国际化)
    int severity;              // 0=Info, 1=Warning, 2=Error, 3=Fatal
    QString technicalDetail;   // 开发调试信息 (Release 不显示给用户)
};

template<typename T>
struct ServiceResult {
    bool ok;
    T value;                   // ok==true 时有效
    ServiceError error;        // ok==false 时有效
};
```

---

## 5. 模块详细设计

### 5.1 主窗口 (MainFrame → main.qml)

**现有功能清单**:

| #   | 功能     | 现有实现           | QML 实现                  |
| --- | -------- | ------------------ | ------------------------- |
| 1   | 标题栏   | BBLTopbar (自定义) | TitleBar.qml (无边框窗口) |
| 2   | 标签切换 | Notebook (11 tabs) | TabBar + SwipeView        |
| 3   | 菜单栏   | wxMenuBar          | MenuBar (QML)             |
| 4   | 状态栏   | wxStatusBar        | StatusBar.qml             |
| 5   | 窗口管理 | wxFrame API        | ApplicationWindow flags   |
| 6   | 最近文件 | wxFileHistory      | RecentProjectsModel       |

**main.qml 骨架**:

```qml
ApplicationWindow {
    id: mainWindow
    visibility: Window.Windowed
    flags: Qt.FramelessWindowHint | Qt.Window

    // 自定义标题栏
    TitleBar {
        id: titleBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
    }

    // 主标签导航
    TabBar {
        id: mainTabBar
        anchors.top: titleBar.bottom

        CxTabButton { text: qsTr("Home");        icon: "qrc:/icons/home.svg" }
        CxTabButton { text: qsTr("3D Editor");   icon: "qrc:/icons/editor.svg" }
        CxTabButton { text: qsTr("Preview");     icon: "qrc:/icons/preview.svg" }
        CxTabButton { text: qsTr("Monitor");     icon: "qrc:/icons/monitor.svg" }
        CxTabButton { text: qsTr("Project");     icon: "qrc:/icons/project.svg" }
        CxTabButton { text: qsTr("Calibration"); icon: "qrc:/icons/calibrate.svg" }
        CxTabButton { text: qsTr("Auxiliary");   icon: "qrc:/icons/auxiliary.svg" }
        CxTabButton { text: qsTr("Device List"); icon: "qrc:/icons/device_list.svg" }
        CxTabButton { text: qsTr("Preferences"); icon: "qrc:/icons/preferences.svg" }
        CxTabButton { text: qsTr("Model Mall");  icon: "qrc:/icons/mall.svg" }
        CxTabButton { text: qsTr("Multi Machine"); icon: "qrc:/icons/multi.svg" }
    }

    // 页面堆栈
    StackLayout {
        currentIndex: mainTabBar.currentIndex

        HomePage     { }
        EditorPage   { }
        PreviewPage  { }
        MonitorPage  { }
        ProjectPage  { }
        CalibrationPage { }
        AuxiliaryPage { }
        DeviceListPage { }
        PreferencesPage { }
        ModelMallPage { }
        MultiMachinePage { }
    }

    // 状态栏
    StatusBar {
        anchors.bottom: parent.bottom
    }
}
```

### 5.2 3D 编辑器 (Plater → EditorPage.qml)

**功能映射**:

| #   | 功能       | 现有类/方法                 | QML/C++ 对应                     |
| --- | ---------- | --------------------------- | -------------------------------- |
| 1   | 3D 视口    | GLCanvas3D (CanvasView3D)   | GLViewport (QQuickFBO)           |
| 2   | 侧边栏     | Sidebar                     | Sidebar.qml + SidebarVM          |
| 3   | 对象列表   | ObjectList (wxDataViewCtrl) | ObjectList.qml + ObjectListModel |
| 4   | 主工具栏   | GLToolbar                   | Toolbar.qml                      |
| 5   | Gizmo 工具 | GLGizmosManager             | GizmoBar.qml + GizmoVM           |
| 6   | 右键菜单   | wxMenu                      | Menu { } (QML)                   |
| 7   | 排列       | ArrangeSettings + arrange() | ArrangeDialog.qml                |
| 8   | 朝向优化   | OrientSettings + orient()   | OrientDialog.qml                 |
| 9   | 层高编辑   | LayersEditing               | LayerEditor.qml                  |
| 10  | 多平板     | PartPlate / PartPlateList   | PlateBar.qml + PlateVM           |
| 11  | 拖拽导入   | wxDropTarget                | DropArea { } (QML)               |
| 12  | 裁切面     | ClippingPlane               | Slider 绑定 uniform              |
| 13  | 网格信息   | 顶点 / 三角面数显示         | 绑定 EditorVM 属性               |
| 14  | 模型操作   | add/delete/clone/mirror     | EditorVM Q_INVOKABLE             |

**EditorPage.qml 骨架**:

```qml
Item {
    // 3D 视口 (占据主区域)
    GLViewport {
        id: viewport3D
        anchors { top: toolbar.bottom; left: parent.left
                  right: sidebar.left; bottom: plateBar.top }
        canvasType: GLViewport.CanvasView3D

        // Gizmo 浮动工具栏
        GizmoBar {
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            model: EditorViewModel.gizmoModel
        }
    }

    // 顶部工具栏
    Toolbar {
        id: toolbar
        anchors { top: parent.top; left: parent.left; right: sidebar.left }
        model: EditorViewModel.toolbarModel
    }

    // 右侧边栏
    Sidebar {
        id: sidebar
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        width: 340

        ObjectList { model: EditorViewModel.objectListModel }
        PrintSettings { config: SettingsViewModel.printConfig }
        FilamentSettings { config: SettingsViewModel.filamentConfig }
        PrinterSettings { config: SettingsViewModel.printerConfig }
    }

    // 底部多平板栏
    PlateBar {
        id: plateBar
        anchors { left: parent.left; right: sidebar.left; bottom: parent.bottom }
        model: EditorViewModel.plateModel
    }
}
```

### 5.3 G-code 预览 (GCodeViewer → PreviewPage.qml)

**功能映射**:

| #   | 功能           | 现有实现                     | QML/C++ 对应                     |
| --- | -------------- | ---------------------------- | -------------------------------- |
| 1   | G-code 3D 视图 | GLCanvas3D (CanvasPreview)   | GLViewport (CanvasPreview)       |
| 2   | 层滑块         | IMSlider (m_layers_slider)   | LayerSlider.qml                  |
| 3   | 移动滑块       | IMSlider (m_moves_slider)    | MoveSlider.qml                   |
| 4   | 13 种着色模式  | EViewType 枚举               | ComboBox 绑定 PreviewVM.viewType |
| 5   | 图例           | GCodeViewer::render_legend() | Legend.qml                       |
| 6   | G-code 行窗口  | GCodeWindow                  | GCodeLineView.qml                |
| 7   | 统计信息       | 时间/耗材/重量               | StatsPanel.qml                   |
| 8   | 动画播放       | SequentialView               | PlaybackControls.qml             |
| 9   | 模型外壳叠加   | Shells                       | CheckBox 绑定 showShells         |

**PreviewViewModel 关键属性**:

```cpp
class PreviewViewModel : public QObject {
    Q_OBJECT

    // 视图控制
    Q_PROPERTY(int viewType READ viewType WRITE setViewType NOTIFY viewTypeChanged)
    Q_PROPERTY(bool showTravel READ showTravel WRITE setShowTravel NOTIFY showTravelChanged)
    Q_PROPERTY(bool showShells READ showShells WRITE setShowShells NOTIFY showShellsChanged)
    Q_PROPERTY(bool showRetractions READ showRetractions ...)

    // 层导航
    Q_PROPERTY(int layerCount READ layerCount NOTIFY layerCountChanged)
    Q_PROPERTY(int currentLayerMin READ currentLayerMin WRITE setCurrentLayerMin ...)
    Q_PROPERTY(int currentLayerMax READ currentLayerMax WRITE setCurrentLayerMax ...)

    // 移动导航
    Q_PROPERTY(int moveCount READ moveCount NOTIFY moveCountChanged)
    Q_PROPERTY(int currentMove READ currentMove WRITE setCurrentMove ...)

    // 统计
    Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY statsChanged)
    Q_PROPERTY(double filamentUsed READ filamentUsed NOTIFY statsChanged)
    Q_PROPERTY(double filamentCost READ filamentCost NOTIFY statsChanged)

    // 颜色映射
    Q_PROPERTY(QVariantList legendItems READ legendItems NOTIFY legendChanged)

public slots:
    void loadGCodeResult(const GCodeProcessorResult& result);
    void playAnimation();
    void pauseAnimation();
    void stepForward();
    void stepBackward();
};
```

### 5.4 设置面板 (Tab → SettingsPage.qml / Sidebar 内嵌)

**动态配置 UI 系统设计**:

现有系统使用 `ConfigOptionsGroup` 根据 `ConfigOptionDef` 动态生成 wxWidgets 控件。新系统需要等价机制：

```cpp
// 配置项元数据 → QML 模型
class ConfigOptionModel : public QAbstractListModel {
    Q_OBJECT

    enum Roles {
        KeyRole = Qt::UserRole + 1,
        LabelRole, TooltipRole, TypeRole,    // 基本信息
        ValueRole, DefaultRole, MinRole, MaxRole, // 值范围
        EnumLabelsRole, EnumValuesRole,      // 枚举选项
        CategoryRole, GroupRole, LineRole,    // 分组
        SideLabelRole, SideWidgetRole,       // 侧标签
        ReadonlyRole, VisibleRole            // 状态
    };

    // 从 ConfigOptionDef 列表构建
    void loadFromOptionKeys(const std::vector<std::string>& keys,
                            const DynamicPrintConfig& config);
};
```

```qml
// 动态配置渲染器
Component {
    id: configOptionDelegate

    Loader {
        sourceComponent: {
            switch (model.type) {
                case "bool":     return checkBoxOption
                case "int":      return spinBoxOption
                case "float":    return floatSpinOption
                case "percent":  return percentOption
                case "string":   return textFieldOption
                case "enum":     return comboBoxOption
                case "color":    return colorPickerOption
                case "point":    return pointOption
                case "slider":   return sliderOption
                default:         return textFieldOption
            }
        }
    }
}

// 设置页面使用
ListView {
    model: SettingsViewModel.printConfigModel
    delegate: configOptionDelegate
    section.property: "category"
    section.delegate: SectionHeader { text: section }
}
```

**4 级覆盖系统**:

```
基础 Print Preset (TabPrint)
    ├─ 对象级覆盖 (TabPrintModel)  — per ModelObject
    ├─ 部件级覆盖 (TabPrintPart)   — per ModelVolume
    ├─ 层范围覆盖 (TabPrintLayer)  — per layer range
    └─ 平板级覆盖 (TabPrintPlate)  — per PartPlate

冲突解析优先级（高 → 低）:
Layer > Part > Object > Plate > Global Preset
```

QML 实现：各覆盖维度使用独立 `ConfigOptionModel` 实例，通过合并视图 + `DynamicPrintConfig::diff()` 计算 dirty 标记。

### 5.5 设备监控 (DeviceManager → MonitorPage.qml)

**功能实体映射**:

| 功能     | 现有实现                | 新实现                          |
| -------- | ----------------------- | ------------------------------- |
| 设备列表 | DeviceListPanel (wx)    | DeviceListModel + ListView      |
| 打印状态 | MonitorPanel            | PrintStatusView.qml             |
| 温度面板 | 喷嘴/热床/仓温显示      | TemperaturePanel.qml + 实时绑定 |
| 摄像头   | CameraPopup + MJPEG     | VideoOutput (Qt Multimedia)     |
| AMS 面板 | AMSControl (wx)         | AmsPanel.qml (动画耗材流)       |
| 速度控制 | PrintingSpeedLevel      | SpeedSlider.qml                 |
| 灯光控制 | command*set*\*\_light   | LightControl.qml                |
| 延时摄影 | command_ipcam_timelapse | Toggle 绑定                     |
| AI 监控  | command_xcam_control    | Toggle 绑定                     |
| 固件更新 | FirmwareDialog          | FirmwareDialog.qml              |
| 打印历史 | wxListCtrl              | HistoryList.qml                 |

**MonitorViewModel**:

```cpp
class MonitorViewModel : public QObject {
    Q_OBJECT

    // 设备连接
    Q_PROPERTY(DeviceListModel* deviceList READ deviceList CONSTANT)
    Q_PROPERTY(MachineObject* currentDevice READ currentDevice NOTIFY deviceChanged)

    // 打印状态
    Q_PROPERTY(QString printStatus READ printStatus NOTIFY statusChanged)
    Q_PROPERTY(int printPercent READ printPercent NOTIFY statusChanged)
    Q_PROPERTY(int remainingTime READ remainingTime NOTIFY statusChanged)
    Q_PROPERTY(QString subtaskName READ subtaskName NOTIFY statusChanged)

    // 温度 (实时更新)
    Q_PROPERTY(double nozzleTemp READ nozzleTemp NOTIFY tempChanged)
    Q_PROPERTY(double nozzleTempTarget READ nozzleTempTarget NOTIFY tempChanged)
    Q_PROPERTY(double bedTemp READ bedTemp NOTIFY tempChanged)
    Q_PROPERTY(double bedTempTarget READ bedTempTarget NOTIFY tempChanged)
    Q_PROPERTY(double chamberTemp READ chamberTemp NOTIFY tempChanged)

    // AMS
    Q_PROPERTY(AmsModel* amsModel READ amsModel NOTIFY amsChanged)

    // 摄像头
    Q_PROPERTY(QUrl cameraUrl READ cameraUrl NOTIFY cameraChanged)

public slots:
    // 80+ 命令转发
    void setNozzleTemp(double temp);
    void setBedTemp(double temp);
    void setPrintSpeed(int level);
    void toggleChamberLight(bool on);
    void toggleNozzleLight(bool on);
    void startTimelapse(bool on);
    void pausePrint();
    void resumePrint();
    void stopPrint();
    void sendCustomGCode(const QString& gcode);
};
```

### 5.6 后台切片 (BackgroundSlicingProcess → SliceService)

```cpp
// 状态机保持不变，仅改变通知机制
class SliceService : public QObject {
    Q_OBJECT
    Q_PROPERTY(SliceState state READ state NOTIFY stateChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool canSlice READ canSlice NOTIFY canSliceChanged)

    enum SliceState { Idle, Preparing, Slicing, GeneratingGCode, Finished, Error, Canceled };
    Q_ENUM(SliceState)

signals:
    void stateChanged(SliceState state);
    void progressChanged(int percent);
    void statusMessageChanged(const QString& msg);
    void sliceFinished(bool success, const QString& gcodeFile);
    void sliceWarning(const QString& message);

public slots:
    void startSlice();
    void cancelSlice();
    void exportGCode(const QString& filePath);

private:
    BackgroundSlicingProcess m_bgsp;  // 复用现有状态机
    // 将 m_bgsp 的回调桥接为 Qt 信号
};
```

### 5.7 网络服务 (NetworkAgent → NetworkService)

```cpp
class NetworkService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY loginStateChanged)
    Q_PROPERTY(QString userAvatar READ userAvatar NOTIFY loginStateChanged)

signals:
    void loginStateChanged();
    void deviceDiscovered(const QString& devId, const QJsonObject& info);
    void printerConnected(const QString& devId);
    void printerDisconnected(const QString& devId);
    void messageReceived(const QString& devId, const QJsonObject& msg);
    void httpError(int statusCode, const QString& body);

public slots:
    // 用户认证
    void login(const QString& account, const QString& password);
    void loginWithToken(const QString& token);
    void logout();

    // 设备发现
    void startSSDPDiscovery();
    void connectPrinter(const QString& devId);
    void disconnectPrinter(const QString& devId);

    // 云服务
    QJsonArray getUserPresets();
    void uploadPreset(const QString& settingId, const QJsonObject& data);
    void downloadPreset(const QString& settingId);

    // 打印任务
    void startCloudPrint(const QString& devId, const QString& taskId);
    void startLocalPrint(const QString& devId, const QString& gcodeFile);
    void sendGCode(const QString& devId, const QString& gcodeFile);

    // 模型商城
    QJsonObject getModelMallHome();
    QJsonObject getModelMallDetail(const QString& modelId);

private:
    NetworkAgent* m_agent;  // 复用现有 DLL 加载机制
};
```

### 5.8 项目服务 (Plater 文件操作 → ProjectService)

```cpp
class ProjectService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY projectChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY dirtyChanged)
    Q_PROPERTY(QStringList recentProjects READ recentProjects NOTIFY recentChanged)

public slots:
    // 文件操作
    void newProject();
    void openProject(const QString& path);
    void saveProject();
    void saveProjectAs(const QString& path);

    // 模型导入
    void importModel(const QStringList& filePaths);  // STL/3MF/OBJ/STEP/AMF
    void importModelFromUrl(const QUrl& url);

    // 导出
    void exportSTL(const QString& path);
    void export3MF(const QString& path);
    void exportSlicedFile(const QString& path);
    void exportGCode(const QString& path);
    void exportAllPlates(const QString& dir);

    // 模型操作
    void addModel(const QString& filePath);
    void deleteSelected();
    void cloneSelected(int count);
    void mirrorSelected(int axis);  // 0=X, 1=Y, 2=Z
    void splitSelected();           // to objects
    void splitToPartsSelected();    // to parts
    void mergeSelected();
    void repairSelected();

signals:
    void projectChanged();
    void dirtyChanged();
    void recentChanged();
    void modelImported(bool success, const QString& error);
    void exportFinished(bool success, const QString& path);
};
```

---

## 6. 3D 渲染桥接层

### 6.1 架构

```
QML Scene Graph                      OpenGL Context
┌──────────────────┐                ┌──────────────────────────┐
│   EditorPage     │                │   GLViewportRenderer     │
│   ┌────────────┐ │   synchronize  │   ┌──────────────────┐   │
│   │ GLViewport │◄├───────────────►│   │ GLCanvas3D       │   │
│   │ (QQuickFBO)│ │                │   │ (现有渲染代码)     │   │
│   └────────────┘ │   render()     │   │ ├── GLVolumes     │   │
│                  │ ──────────────►│   │ ├── Selection     │   │
│   Mouse/Key     │                │   │ ├── Gizmos        │   │
│   events        │ ──────────────►│   │ ├── LayersEditing │   │
│                  │                │   │ └── Toolbars      │   │
└──────────────────┘                │   └──────────────────┘   │
                                    └──────────────────────────┘
```

### 6.2 GLViewport 实现

```cpp
// 桥接 QML 与 OpenGL 渲染
class GLViewport : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType)

public:
    enum CanvasType { CanvasView3D, CanvasPreview, CanvasAssembleView };
    Q_ENUM(CanvasType)

    Renderer* createRenderer() const override;

protected:
    // 输入事件转发到 GLCanvas3D
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;

private:
    CanvasType m_canvasType = CanvasView3D;
    // 输入事件队列 (QML thread → render thread)
    mutable QMutex m_eventMutex;
    mutable std::queue<std::shared_ptr<QEvent>> m_pendingEvents;
};

class GLViewportRenderer : public QQuickFramebufferObject::Renderer {
public:
    GLViewportRenderer(GLViewport::CanvasType type);

    void synchronize(QQuickFramebufferObject* item) override;
    void render() override;
    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;

private:
    std::unique_ptr<GLCanvas3D> m_canvas;    // 复用现有渲染逻辑
    std::unique_ptr<GCodeViewer> m_gcodeViewer; // Preview 模式
    QSize m_viewportSize;
};
```

### 6.3 渲染代码复用策略

| 现有模块             | 复用率 | 修改内容            |
| -------------------- | ------ | ------------------- |
| GLVolumeCollection   | 95%    | 移除 wxWidgets 依赖 |
| Selection            | 90%    | 事件接口适配        |
| GLGizmosManager      | 85%    | 输入事件适配        |
| SceneRaycaster       | 100%   | 纯 GL 代码无需改动  |
| GCodeViewer 渲染     | 90%    | FBO 适配            |
| Camera               | 95%    | 输入源切换          |
| LayersEditing        | 80%    | UI 部分改为 QML     |
| Toolbars (GLToolbar) | 30%    | 改为 QML 纯声明式   |
| ImGui overlays       | 0%     | 完全移除，QML 替代  |

**整体 3D 渲染代码复用率: ~85%**

### 6.4 输入事件桥接

```cpp
// QML 鼠标事件 → GLCanvas3D 事件
void GLViewport::mousePressEvent(QMouseEvent* event) {
    QMutexLocker lock(&m_eventMutex);
    m_pendingEvents.push(std::make_shared<QMouseEvent>(*event));
    update(); // 触发重绘
}

// Render 线程处理
void GLViewportRenderer::synchronize(QQuickFramebufferObject* item) {
    auto* viewport = static_cast<GLViewport*>(item);
    QMutexLocker lock(&viewport->m_eventMutex);
    while (!viewport->m_pendingEvents.empty()) {
        auto& event = viewport->m_pendingEvents.front();
        m_canvas->process_input_event(*event);
        viewport->m_pendingEvents.pop();
    }
}
```

---

## 7. 数据流与状态管理

### 7.1 切片数据流

```
用户操作 (QML)
    │
    ▼
EditorViewModel::requestSlice()
    │
    ▼
SliceService::startSlice()
    │ emit stateChanged(Preparing)
    │
    ▼
BackgroundSlicingProcess::start()   ◄── 线程池 (TBB)
    │
    ├── Print::process()
    │   ├── PrintObject::slice()           // 切片
    │   ├── PrintObject::make_perimeters() // 轮廓
    │   ├── PrintObject::infill()          // 填充
    │   └── PrintObject::generate_support()// 支撑
    │
    ├── [进度回调] ────► SliceService::onProgress(int)
    │                        │ emit progressChanged(percent)
    │                        ▼
    │                   QML ProgressBar.value = percent
    │
    ├── GCode::do_export()                 // G-code 生成
    │
    └── [完成回调] ────► SliceService::onFinished(success)
                             │ emit sliceFinished(true, path)
                             ▼
                        PreviewViewModel::loadGCodeResult()
                             │
                             ▼
                        GLViewport.update() → GCodeViewer::render()
```

### 7.2 设备通信数据流

```
bambu_networking.dll
    │
    ├── SSDP 发现 ────► NetworkService::onDeviceDiscovered()
    │                        │ emit deviceDiscovered(devId, info)
    │                        ▼
    │                   DeviceListModel::addDevice(info)
    │                        │
    │                        ▼
    │                   QML ListView 自动更新
    │
    ├── MQTT 消息 ────► NetworkService::onMessage(devId, json)
    │                        │
    │                        ▼
    │                   DeviceService::updateDeviceState(json)
    │                        │ 解析温度/进度/状态
    │                        ▼
    │                   MachineObject 属性更新
    │                        │ emit propertyChanged()
    │                        ▼
    │                   MonitorViewModel Q_PROPERTY 通知
    │                        │
    │                        ▼
    │                   QML 绑定自动更新 UI
    │
    └── HTTP API ────► NetworkService 直接返回 QJsonObject
```

### 7.3 配置数据流

```
QML ConfigOption 控件
    │ onValueChanged
    ▼
SettingsViewModel::setConfigValue(key, value)
    │
    ▼
PresetService::updateConfig(key, value)
    │
    ├── DynamicPrintConfig::set(key, value)  // 更新内存配置
    ├── dirty 状态计算 (diff vs saved preset)
    ├── 依赖项联动 (如 support_type 改变 → 相关选项 show/hide)
    │
    ├── emit configChanged(key)
    │       │
    │       ▼
    │   ConfigOptionModel::dataChanged()  // 通知 QML 刷新
    │
    └── 触发自动切片 (如果 auto-slice 开启)
            │
            ▼
        SliceService::scheduleSlice(debounce: 500ms)
```

---

## 8. 构建系统与依赖管理

### 8.1 CMake 结构

```cmake
# 顶级 CMakeLists.txt 新增
option(CREALITY_QML_GUI "Build new Qt/QML GUI" OFF)

if(CREALITY_QML_GUI)
    find_package(Qt6 6.8 REQUIRED COMPONENTS
        Core Gui Quick QuickControls2 OpenGL
        Network WebSockets Svg Multimedia LinguistTools ShaderTools)

    add_subdirectory(src/slic3r/Service)
    add_subdirectory(src/qml_gui)
else()
    # 现有 wxWidgets GUI 构建
    add_subdirectory(src/slic3r/GUI)
endif()
```

```cmake
# src/qml_gui/CMakeLists.txt
qt_add_executable(CrealityPrintQML
    main.cpp
    App.cpp
)

# QML 模块
qt_add_qml_module(CrealityPrintQML
    URI CrealityPrint
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/pages/HomePage.qml
        qml/pages/EditorPage.qml
        qml/pages/PreviewPage.qml
        qml/pages/MonitorPage.qml
        qml/pages/SettingsPage.qml
        # ... 所有 QML 文件
    RESOURCES
        resources/icons/
        resources/themes/
)

# 链接依赖
target_link_libraries(CrealityPrintQML PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    Qt6::OpenGL Qt6::Network Qt6::WebSockets Qt6::Svg Qt6::Multimedia
    libslic3r           # 切片引擎
    CrealityService      # 服务层
)
```

```cmake
# src/slic3r/Service/CMakeLists.txt
add_library(CrealityService STATIC
    SliceService.cpp
    PresetService.cpp
    DeviceService.cpp
    ProjectService.cpp
    NetworkService.cpp
    CalibrationService.cpp
    ModelMallService.cpp
    MultiMachineService.cpp
)

target_link_libraries(CrealityService PUBLIC
    libslic3r
    Qt6::Core
    Boost::filesystem
    TBB::tbb
)
```

### 8.2 依赖版本矩阵

| 依赖      | 版本    | 用途          | 来源         |
| --------- | ------- | ------------- | ------------ |
| Qt        | 6.8 LTS | GUI 框架      | qt.io 安装器 |
| Boost     | 1.84    | 文件系统/线程 | deps/ (已有) |
| TBB       | 2021.x  | 并行化        | deps/ (已有) |
| OpenSSL   | 1.1.1w  | 加密通信      | deps/ (已有) |
| CURL      | 7.75    | HTTP 客户端   | deps/ (已有) |
| CGAL      | 5.x     | 几何运算      | deps/ (已有) |
| OpenVDB   | 11.x    | 空间数据      | deps/ (已有) |
| OCCT      | 7.7     | STEP 导入     | deps/ (已有) |
| Assimp    | 5.x     | 通用格式导入  | deps/ (已有) |
| FFmpeg    | 5.x     | 视频录制      | deps/ (已有) |
| Paho MQTT | 1.3.x   | MQTT 通信     | deps/ (已有) |

### 8.3 并行构建支持

```
# 现有 GUI 和新 QML GUI 可并行开发
cmake -DCREALITY_QML_GUI=ON  ...  # 构建 QML 版本
cmake -DCREALITY_QML_GUI=OFF ...  # 构建 wxWidgets 版本 (保持兼容)
```

---

## 9. 测试策略

### 9.1 测试金字塔

```
          ╱╲
         ╱  ╲         E2E 测试
        ╱ E2E╲        - Squish for Qt (GUI 自动化)
       ╱──────╲       - 关键用户流程 10 个
      ╱        ╲
     ╱ 集成测试  ╲    集成测试
    ╱    (QML)   ╲    - Qt Quick Test (QML)
   ╱──────────────╲   - ViewModel ↔ Service 集成 30 个
  ╱                ╲
 ╱    单元测试      ╲  单元测试
╱   (C++ / QML)     ╲  - Catch2 (Service 层) 100+
╱────────────────────╲ - Qt Test (ViewModel) 50+
```

### 9.2 关键测试用例

| 模块               | 测试类型 | 用例                                   |
| ------------------ | -------- | -------------------------------------- |
| SliceService       | 单元     | 状态机转换、进度回调、取消             |
| PresetService      | 单元     | 预设加载/保存/继承/覆盖                |
| ConfigOptionModel  | 单元     | 动态 UI 模型数据完整性                 |
| GLViewportRenderer | 集成     | FBO 创建/渲染/事件传递                 |
| EditorViewModel    | 集成     | 添加模型 → 切片 → 预览完整流           |
| PreviewViewModel   | 集成     | G-code 加载 → 层导航 → 视图模式切换    |
| MonitorViewModel   | 集成     | 设备发现 → 连接 → 状态更新             |
| 导入导出           | E2E      | STL/3MF/STEP 导入 → 切片 → 导出 G-code |
| 打印流程           | E2E      | 选择设备 → 发送 → 监控 → 完成          |

### 9.3 视觉回归测试

每个 QML 页面生成快照，CI 管线对比：

```bash
# CI 流程
qt6_test --qml-snapshot EditorPage.qml --compare baseline/
```

### 9.4 CI/CD 管线设计

```
┌─ Push / PR 触发 ───────────────────────────────────────────────────┐
│                                                                     │
│  Stage 1: Build (三平台并行)                                        │
│  ├─ Windows x64  — MSVC 2022+ / Qt 6.8                             │
│  ├─ macOS arm64  — Clang 15+ / Qt 6.8                              │
│  └─ Linux x64    — GCC 13+ / Ubuntu 22.04 / Qt 6.8                │
│                                                                     │
│  Stage 2: Test (并行)                                               │
│  ├─ 单元测试 — Catch2 (Service) + Qt Test (ViewModel)              │
│  ├─ QML 测试 — Qt Quick Test (页面级交互)                           │
│  └─ 快照测试 — 视觉回归 (像素差异 < 0.5%)                          │
│                                                                     │
│  Stage 3: Quality Gate (任一失败阻断合并)                           │
│  ├─ 代码覆盖率 ≥ 70% (Service 层) / ≥ 50% (ViewModel)             │
│  ├─ clang-tidy 零新增 Warning                                      │
│  ├─ qmllint 零 Error                                               │
│  └─ 构建产物体积增量 < 5% (vs 上一次 Release)                      │
│                                                                     │
│  Stage 4: Package (仅 main / release 分支)                          │
│  ├─ NSIS 安装包 (Windows)                                           │
│  ├─ DMG + 公证 (macOS)                                              │
│  └─ AppImage + Flatpak (Linux)                                     │
│                                                                     │
│  Stage 5: Smoke Test (安装包级, 可选)                               │
│  └─ Squish 自动化: 启动 → 导入模型 → 切片 → 退出                   │
└─────────────────────────────────────────────────────────────────────┘
```

**分支策略**: `main`(稳定) ← `develop`(集成) ← `feature/*`(功能) / `phase/*`(阶段)

---

## 10. 开发计划与里程碑

### 10.1 总体时间线

```
Phase 0  [Month 1-2]     基础设施 & 服务层抽取
Phase 1  [Month 3-5]     3D 视口 & 编辑器
Phase 2  [Month 6-8]     预览 & 设置面板
Phase 3  [Month 9-10]    设备监控 & 网络
Phase 4  [Month 11-12]   集成 & 收尾
                          ──────────────────
                          总计: 12 个月
```

### 10.2 详细里程碑

#### Phase 0: 基础设施 (Month 1-2)

| 周   | 任务                | 交付物              | 验收标准                           |
| ---- | ------------------- | ------------------- | ---------------------------------- |
| W1-2 | Qt 6.8 集成到 CMake | 双模式构建成功      | `cmake -DCREALITY_QML_GUI=ON` 通过 |
| W3-4 | Service 层抽取      | 8 个 Service 类骨架 | SliceService 可独立调用 BGSP       |
| W5-6 | 基础 QML 框架       | main.qml + 空页面   | 应用启动显示标签页导航             |
| W7-8 | 控件库 & 主题       | CxButton/CxXxx 系列 | 控件风格与设计稿一致               |

#### Phase 1: 3D 视口 & 编辑器 (Month 3-5)

| 周     | 任务            | 交付物                 | 验收标准             |
| ------ | --------------- | ---------------------- | -------------------- |
| W9-10  | GLViewport 桥接 | QQuickFBO + GLCanvas3D | 3D 模型渲染正确      |
| W11-12 | 相机控制        | 旋转/平移/缩放         | 与现有操作一致       |
| W13-14 | 选择 & Gizmo    | 移动/旋转/缩放 Gizmo   | Gizmo 操作正确       |
| W15-16 | 侧边栏          | ObjectList + 基础设置  | 对象增删改           |
| W17-18 | 工具栏 & 排列   | 全部工具栏功能         | 排列/朝向/自动填充   |
| W19-20 | 多平板          | PlateBar + 平板管理    | 多平板切换/添加/删除 |

#### Phase 2: 预览 & 设置 (Month 6-8)

| 周     | 任务            | 交付物                     | 验收标准           |
| ------ | --------------- | -------------------------- | ------------------ |
| W21-22 | G-code 预览渲染 | GCodeViewer FBO 集成       | G-code 渲染正确    |
| W23-24 | 层/移动导航     | 双滑块 + 动画播放          | 层浏览 + 动画播放  |
| W25-26 | 13 种视图模式   | ViewType 完整实现          | 每种模式着色正确   |
| W27-28 | 动态配置 UI     | ConfigOptionModel + 渲染器 | 动态生成设置面板   |
| W29-30 | 4 级覆盖        | 对象/部件/层/平板覆盖      | 覆盖链正确生效     |
| W31-32 | 预设管理        | 预设列表/切换/保存         | 预设操作与原版一致 |

#### Phase 3: 设备 & 网络 (Month 9-10)

| 周     | 任务            | 交付物                      | 验收标准          |
| ------ | --------------- | --------------------------- | ----------------- |
| W33-34 | 设备发现 & 列表 | NetworkService + DeviceList | SSDP 发现并显示   |
| W35-36 | 打印监控面板    | 温度/进度/状态              | 实时数据刷新      |
| W37-38 | AMS & 摄像头    | AmsPanel + VideoOutput      | AMS 状态 + 视频流 |
| W39-40 | 云服务集成      | 登录/预设同步/商城          | 登录 → 同步完整流 |

#### Phase 4: 集成 & 收尾 (Month 11-12)

| 周     | 任务              | 交付物                | 验收标准                             |
| ------ | ----------------- | --------------------- | ------------------------------------ |
| W41-42 | 校准工具          | CalibrationPage       | 全部校准流程可用                     |
| W43-44 | 国际化            | Qt Linguist 翻译      | 中/英/日/韩 4 语言                   |
| W45-46 | 性能优化          | 大模型/大 G-code 测试 | 常见交互 p95 <100ms；切页首帧 <500ms |
| W47-48 | 全平台测试 & 发布 | RC 版本               | Win/macOS/Linux 通过                 |

### 10.3 团队资源建议

| 角色           | 人数  | 职责                         |
| -------------- | ----- | ---------------------------- |
| 架构师         | 1     | 技术方案、架构评审           |
| 3D 渲染工程师  | 1     | GLViewport 桥接、GCodeViewer |
| QML 前端工程师 | 2     | 页面开发、控件库、动画       |
| C++ 后端工程师 | 1     | Service 层、ViewModel        |
| QA 工程师      | 1     | 测试自动化、视觉回归         |
| **总计**       | **6** |                              |

### 10.4 任务依赖与并行化

```
W1─W2  CMake 集成 ──────────────────────────────────────────┐
  │                                                          │
W3─W4  Service 层抽取 ──────────────────────────┐            │
  │                                              │            │
W5─W6  QML 框架 + 空页面 ◄──────────────────────┘            │
  │                           并行 ↓                         │
W7─W8  控件库/主题 ─────┐   Service 单测编写                 │
  │                      │        │                          │
W9─W10 GLViewport 桥接 ◄┘────────┘                          │
  ├─ (3D 渲染工程师独立)                                     │
  │              并行 ↓                                      │
W11─W12 相机控制  │  QML 前端: Sidebar 骨架 (可并行)          │
  │               │                                          │
W13─W14 Gizmo ────┤                                          │
  │               │                                          │
W15─W16 侧边栏 ◄──┘  ObjectList + 基础设置                  │
  │                                                          │
W17─W20 工具栏/排列/多平板  ← Phase 1 后半可并行 2 人        │
  │                                                          │
W21─W26 G-code 预览 (依赖 SliceService+GLViewport)          │
  │              并行 ↓                                      │
W27─W32 设置面板 (依赖 PresetService, 可与预览并行)          │
  │                                                          │
W33─W40 设备/网络 (依赖 NetworkService, 与 Phase 2 串行)     │
  │                                                          │
W41─W48 集成/国际化/优化/发布 ◄──────────────────────────────┘
```

**关键路径**: W1→W3→W5→W9→W13→W21→W33→W41 (最长串行链 ~24 周)

**可并行窗口**:
| 时段 | 并行任务 A | 并行任务 B | 所需人力 |
| --- | --- | --- | --- |
| W7-8 | 控件库/主题 (QML×2) | Service 单测 (C++×1) | 3 人 |
| W11-14 | 相机+Gizmo (3D×1) | Sidebar 骨架 (QML×1) | 2 人 |
| W17-20 | 工具栏 (QML×1) | 排列/多平板 (QML×1) | 2 人 |
| W21-32 | G-code 预览 (3D×1+QML×1) | 设置面板 (C++×1+QML×1) | 4 人 |

### 10.5 性能预算 (DoD 硬性门槛)

每个 Phase 验收时须达到的性能指标——未达标则阻断进入下一阶段：

| 指标                  | Phase 0 | Phase 1 | Phase 2 | Phase 3 | Phase 4 |
| --------------------- | ------- | ------- | ------- | ------- | ------- |
| 冷启动到首屏          | < 3s    | < 3s    | < 3s    | < 4s    | < 3s    |
| 切页响应 (TabBar)     | < 200ms | < 200ms | < 200ms | < 200ms | < 100ms |
| 3D 视口 FPS (10 万面) | —       | ≥ 30fps | ≥ 30fps | ≥ 30fps | ≥ 45fps |
| G-code 加载 (50MB)    | —       | —       | < 10s   | < 10s   | < 8s    |
| 内存占用 (空项目)     | < 200MB | < 300MB | < 350MB | < 400MB | < 350MB |
| Service 主线程阻塞    | < 16ms  | < 16ms  | < 16ms  | < 16ms  | < 10ms  |
| 交互操作 p95 延迟     | —       | < 150ms | < 120ms | < 120ms | < 100ms |

> **测量方法**: 测试机配置 i5-12400 / 16GB / GTX 1650（CI 最低基准线）
>
> **失败处理**: 不达标项须创建 `perf-blocker` 标签 Issue，Phase 结束评审中降级或追加优化冲刺。

---

## 11. 风险与缓解措施

### 11.1 风险矩阵

| #   | 风险                      | 概率 | 影响 | 缓解措施                                       |
| --- | ------------------------- | ---- | ---- | ---------------------------------------------- |
| R1  | OpenGL 线程安全问题       | 高   | 高   | QQuickFBO 保证 render thread；严格隔离 GL 调用 |
| R2  | 配置动态 UI 复杂度        | 中   | 高   | 先实现 80% 常用控件类型，边角 case 逐步补齐    |
| R3  | bambu_networking DLL 兼容 | 中   | 中   | NetworkService 封装层隔离；函数指针模式不变    |
| R4  | 性能回归 (QML overhead)   | 低   | 高   | 早期 benchmark；大模型/大 G-code 性能测试      |
| R5  | 多平台 Qt 安装体积        | 低   | 低   | 按需裁剪 Qt 模块；静态链接可选                 |
| R6  | 现有 GUI 代码并行演进     | 高   | 中   | Service 层共享；定期 rebase                    |
| R7  | 人力不足/工期延期         | 中   | 高   | Phase 分离可独立发布；最小可用版本优先         |

### 11.2 最小可行产品 (MVP)

如遇工期压力，MVP 范围为 **Phase 0 + Phase 1 + Phase 2 前半**（W1-26，约 6.5 个月）：

- Qt/QML 基础框架 ✅ (Phase 0)
- 3D 编辑器完整功能 ✅ (Phase 1)
- 基础切片 + G-code 导出 ✅ (Phase 0 Service 层)
- G-code 预览 + 基础层滑块 + FeatureType 视图 ✅ (Phase 2 W21-26)
- 设置面板（Sidebar 内嵌 Print/Filament/Printer 基础参数集）✅ (Phase 1 W15-16)
- 13 种视图模式全量 ❌ (降级为 FeatureType + Height + Tool 三种)
- 4 级配置覆盖 ❌ (仅全局 Preset，后续迭代)
- 设备监控 ❌ (使用现有 GUI 或 Web)
- 云服务 / 模型商城 / 多机 ❌ (后续迭代)

MVP 可在 **6-7 个月**内交付。若需压缩到 5 个月，砍掉 Phase 2 预览部分，改用静态切片统计页。

### 11.3 逐 Phase 回滚策略

| Phase   | 回滚触发条件                      | 回滚方案                                | 最大回滚代价             |
| ------- | --------------------------------- | --------------------------------------- | ------------------------ |
| Phase 0 | CMake 双模式无法共存              | 放弃 QML 分支，wxWidgets 继续维护       | 2 个月人力沉没           |
| Phase 1 | QQuickFBO 性能不可接受 (< 15fps)  | 回退到 Qt3D 或独立 GL 子窗口方案        | 需重写渲染桥接 (~4 周)   |
| Phase 2 | 动态配置 UI 覆盖率 < 60% 常用选项 | 暂用静态 QML 硬编码常用选项             | 牺牲灵活性，后续迭代补齐 |
| Phase 3 | DLL 桥接无法覆盖全部回调          | 仅支持 USB 本地打印，网络功能保留旧 GUI | 功能降级                 |
| Phase 4 | 多平台兼容问题 > 20 个 blocker    | 仅发布 Windows 版本，macOS/Linux 延期   | 缩小发布范围             |

> **回滚决策权**: Phase Owner (架构师) + 产品负责人双签；回滚须在触发后 **3 个工作日** 内完成。

### 11.4 组件降级方案

当特定组件无法按期达到完整功能时，提供降级运行模式：

| 组件        | 完整模式               | 降级模式                         | 降级触发器                     |
| ----------- | ---------------------- | -------------------------------- | ------------------------------ |
| 3D 视口     | QQuickFBO 嵌入         | 独立 QWindow + 合成覆盖          | FBO FPS < 15 on 目标 GPU       |
| G-code 预览 | 13 种视图模式          | FeatureType + Height + Tool 三种 | Phase 2 W25-26 未完成          |
| 设置面板    | 动态 ConfigOptionModel | 静态 QML 硬编码 Top-50 选项      | ConfigOptionModel 覆盖率 < 60% |
| 设备监控    | 实时 MQTT + 摄像头     | 只读状态轮询 (HTTP 5s 间隔)      | MQTT 桥接不稳定                |
| 多色 AMS    | 动画耗材流 + 实时状态  | 静态色块 + 文字状态              | Qt Multimedia 兼容问题         |
| 国际化      | 4 语言 (中/英/日/韩)   | 中/英 双语                       | 翻译资源不足                   |

### 11.5 安全架构

| 关注点    | 威胁                        | 防护措施                                          |
| --------- | --------------------------- | ------------------------------------------------- |
| 用户凭据  | Token 明文泄露              | Qt Keychain 加密存储；内存中 SecureString 模式    |
| 网络通信  | MITM 攻击                   | 强制 TLS 1.2+；MQTT over SSL；证书固定 (可选)     |
| DLL 注入  | 恶意 bambu_networking 替换  | 加载前校验 DLL 文件哈希 (SHA-256)                 |
| 本地文件  | 恶意 3MF/STL 触发缓冲区溢出 | libslic3r 已有校验；新增 QML 层文件大小上限 (2GB) |
| 插件/脚本 | 未来扩展的 QML 插件安全     | QML 引擎禁用 `import` 外部未签名模块              |

### 11.6 用户数据迁移

从 CrealityPrint 6.x (wxWidgets) 升级到 7.0 (Qt/QML) 时，需自动迁移以下数据：

| 数据类型     | 6.x 存储位置                | 7.0 存储位置                       | 迁移方式                 |
| ------------ | --------------------------- | ---------------------------------- | ------------------------ |
| 用户预设     | `AppConfig` + JSON 文件     | 相同文件格式（Service 层兼容读取） | 零迁移（格式不变）       |
| 系统预设     | `resources/profiles/`       | 同路径                             | 零迁移                   |
| 登录 Token   | 本地配置文件                | Qt Keychain 加密                   | 首次启动自动导入         |
| 最近项目列表 | `AppConfig.recent_projects` | `QSettings`                        | 首次启动读取旧格式并转写 |
| 窗口布局     | wxWidgets 持久化            | `QSettings` (window geometry)      | 不迁移，使用新默认值     |
| 语言偏好     | `AppConfig.language`        | `QSettings` 或读取旧配置           | 首次启动读取旧值         |

**迁移流程**：

1. 首次启动检测旧版配置文件存在性
2. 弹出"检测到 6.x 配置，是否导入？"对话框
3. 用户确认后执行迁移，完成后标记 `migration_done=true`
4. 旧文件保留不删除（用户可手动回退旧版本）

---

## 附录 A: 现有 GUI 文件清单 (需迁移)

```
src/slic3r/GUI/
├── 核心框架 (优先级: P0)
│   ├── GUI_App.cpp/h
│   ├── MainFrame.cpp/h
│   └── Plater.cpp/h
│
├── 3D 渲染 (优先级: P0)
│   ├── GLCanvas3D.cpp/h
│   ├── 3DScene.cpp/h
│   ├── GLVolume.cpp/h
│   ├── Selection.cpp/h
│   ├── Camera.cpp/h
│   ├── SceneRaycaster.cpp/h
│   └── GCodeViewer.cpp/h
│
├── Gizmo 工具 (优先级: P1)
│   ├── Gizmos/GLGizmosManager.cpp/h
│   ├── Gizmos/GLGizmoBase.cpp/h
│   ├── Gizmos/GLGizmoMove.cpp/h
│   ├── Gizmos/GLGizmoRotate.cpp/h
│   ├── Gizmos/GLGizmoScale.cpp/h
│   ├── Gizmos/GLGizmoFlatten.cpp/h
│   ├── Gizmos/GLGizmoCut.cpp/h
│   ├── Gizmos/GLGizmoMeshBoolean.cpp/h
│   ├── Gizmos/GLGizmoFdmSupports.cpp/h
│   ├── Gizmos/GLGizmoSeam.cpp/h
│   ├── Gizmos/GLGizmoMmuSegmentation.cpp/h
│   └── Gizmos/GLGizmoText.cpp/h
│
├── 设置面板 (优先级: P1)
│   ├── Tab.cpp/h
│   ├── ConfigManipulation.cpp/h
│   ├── OG_CustomCtrl.cpp/h
│   └── OptionsGroup/Field.cpp/h
│
├── 设备管理 (优先级: P2)
│   ├── DeviceManager.cpp/h
│   ├── MonitorPanel.cpp/h
│   ├── StatusPanel.cpp/h
│   └── CameraPopup.cpp/h
│
├── 辅助功能 (优先级: P2)
│   ├── CalibrationWizard.cpp/h
│   ├── PrinterWebView.cpp/h
│   ├── Search.cpp/h
│   └── Preferences.cpp/h
│
└── 自定义控件 (全部 QML 替换)
    ├── Widgets/Label.cpp/h
    ├── Widgets/Button.cpp/h
    ├── Widgets/CheckBox.cpp/h
    ├── Widgets/ComboBox.cpp/h
    ├── Widgets/Slider.cpp/h
    ├── Widgets/SpinCtrl.cpp/h
    └── Widgets/TextInput.cpp/h
```

## 附录 B: QML ↔ C++ 类型映射

| C++ 类型               | QML 类型          | 桥接方式                       |
| ---------------------- | ----------------- | ------------------------------ |
| `std::string`          | `string`          | `QString::fromStdString()`     |
| `std::vector<T>`       | `list<T>`         | `QAbstractListModel` 子类      |
| `DynamicPrintConfig`   | `var` (JS object) | `ConfigOptionModel`            |
| `Vec3d` (Eigen)        | `vector3d`        | `Q_PROPERTY(QVector3D)`        |
| `TriangleMesh`         | 不暴露            | Service 层内部                 |
| `BoundingBoxf3`        | `rect3d`          | `Q_GADGET`                     |
| `Model`                | 不暴露            | `ObjectListModel` 包装         |
| `GCodeProcessorResult` | 不暴露            | `PreviewViewModel` 消化        |
| `MachineObject`        | QML 可见          | `Q_PROPERTY` 全属性暴露        |
| `Preset`               | QML 可见          | `Q_GADGET` + `PresetListModel` |

## 附录 C: 国际化迁移

```
现有: gettext (.po/.mo) → _L("string")
新系统: Qt Linguist (.ts/.qm) → qsTr("string") / tr("string")

迁移脚本:
1. 提取所有 _L("...") 字符串
2. 生成 .ts 模板
3. 从现有 .po 导入翻译
4. Qt Linguist 校验发布 .qm
```

---

> **文档结束 — v2.0 Final**  
> 本文档将随开发进展持续更新。每个 Phase 完成后进行架构评审和文档修订。  
> 任何结构性变更须更新版本号并追加修订记录。
