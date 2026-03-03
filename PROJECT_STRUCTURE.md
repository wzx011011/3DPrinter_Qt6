# 3DPrinter_Qt6 目录结构

> v3.0 更新：新增 libslic3r 迁移目录、GCodeRenderer、Phase J/K/L 预期产物。

当前工程采用“双入口”结构：

- `CREALITY_QML_GUI=ON`：走 QML 主路线（默认）
- `CREALITY_QML_GUI=OFF`：走现有 Widgets 原型路线

## 顶层

```
3DPrinter_Qt6/
├── CMakeLists.txt                               # 统一构建入口（QML/Widgets 可切换）
├── CrealityPrint_Qt_GUI_Rewrite_Architecture.md # 重构架构规划 v3.0
├── PROJECT_STRUCTURE.md                         # 本文件
├── TASKS.md                                     # 任务追踪
├── third_party/                                 # 第三方依赖（submodule 或拷贝）
│   ├── libslic3r/                               # Phase J/K：从上游迁移的切片核心库
│   │   ├── Format/
│   │   │   ├── 3mf.h / 3mf.cpp                 # 标准 3MF 加载（expat+miniz）
│   │   │   └── bbs_3mf.hpp / bbs_3mf.cpp        # BBS/Creality 扩展格式
│   │   ├── Print.hpp / Print.cpp                # 切片主流程
│   │   ├── GCode.hpp / GCode.cpp                # G-code 生成
│   │   └── GCode/GCodeProcessor.*               # GCodeProcessorResult 解析
│   ├── expat/                                   # XML 解析（3MF 依赖）
│   ├── miniz/                                   # ZIP 解压（3MF 依赖）
│   └── eigen/                                   # 线性代数（libslic3r 依赖）
└── src/
    ├── qml_gui/                                 # QML GUI 主路线
    └── core/                                    # 核心逻辑层
```

## src/qml_gui/

```
src/qml_gui/
├── main_qml.cpp                # QML 应用入口
├── BackendContext.h/.cpp       # 组合根：注入服务和 ViewModel
├── main.qml                    # 主窗口壳与页面导航
├── pages/
│   ├── PreparePage.qml
│   ├── PreviewPage.qml         # Phase F/L：含 GLViewport + LayerSlider + MoveSlider
│   ├── MonitorPage.qml
│   └── ConfigPage.qml
└── components/
    ├── LayerSlider.qml         # Phase F：垂直双端层范围滑块
    ├── MoveSlider.qml          # Phase F：水平进度滑块 + 播放/暂停
    ├── Legend.qml              # Phase L：G-code 颜色图例面板
    └── StatsPanel.qml          # Phase L：总时间 / 耗材用量 / 层数统计
```

## src/core/

```
src/core/
├── services/
│   ├── SliceServiceMock.*      # Mock 切片服务（Phase A 存量）
│   ├── SliceService.h/.cpp     # Phase K：去 wx 化真实切片服务（QThread + Qt 信号）
│   ├── ProjectServiceMock.*    # Mock 项目服务
│   ├── ProjectService.h/.cpp   # Phase J：真实 3MF 加载（QtConcurrent::run）
│   ├── PresetServiceMock.*
│   ├── DeviceServiceMock.*
│   └── NetworkServiceMock.*
├── viewmodels/
│   ├── EditorViewModel.*       # 对象列表 / 3D 视口刷新
│   ├── PreviewViewModel.*      # Phase F/L：层范围 / 移动进度 / 图例 / 统计
│   ├── MonitorViewModel.*
│   └── ConfigViewModel.*
└── rendering/
    ├── GCodeRenderer.h/.cpp    # Phase F（Mock）/ Phase L（VBO/IBO 真实渲染）
    └── GLViewportRenderer.*    # OpenGL 视口基类
```

## 依赖状态总览

| 依赖       | 用途                       | 当前状态   | 引入阶段 |
|------------|----------------------------|------------|----------|
| Qt 6.x     | 全部 UI / 信号槽 / 并发    | 已集成     | 基础     |
| OpenGL     | 3D 视口渲染                | 已集成     | Phase C  |
| expat      | XML 解析（3MF ZIP 内容）   | 待引入     | Phase J  |
| miniz      | ZIP 解压（.3mf/.cxprj）    | 待引入     | Phase J  |
| Boost      | 文件系统 / 字符串          | 待引入     | Phase J  |
| Eigen 3    | 矩阵 / 几何变换            | 待引入     | Phase J  |
| TBB 2021   | 并行切片（Print::process） | 待引入     | Phase K  |
| fast_float | 浮点解析加速               | 待引入     | Phase J  |

## 下一步建议

1. **Phase F**：GCodeRenderer Mock 骨架 + PreviewPage.qml UI 改造（Layer/Move 滑块）。
2. **Phase J**：引入 expat/miniz/Boost/Eigen，集成 libslic3r/Format/3mf.*，实现 ProjectService::loadFile()。
3. **Phase K**：引入 TBB，实现 SliceService（去 wx 化），打通切片->G-code 导出流程。
4. **Phase L**：GCodeRenderer::loadResult() VBO 真实渲染，补全 13 种视图着色模式 + 图例面板。
5. 新增 tests/ 对 ProjectService / SliceService / GCodeRenderer 建立回归测试。
