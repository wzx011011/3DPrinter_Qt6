# 3DPrinter_Qt6

Qt6 重写版 3D 打印桌面端原型工程（QML 主路线）。

## 协作入口

本仓库按“上游源码真值迁移”方式推进。日常继续开发时，优先使用固定入口：

`/continue-source-truth-migration`

该命令会基于当前仓库状态执行以下流程：

- 读取 `docs/TASKS.md`
- 对照 `third_party/CrealityPrint` 上游源码
- 选择当前最高优先级、尚未完成的任务
- 映射到对应 Qt6/QML 承接模块
- 实施修改并按规则更新任务状态

相关配置位置：

- 工作区规则：`.github/copilot-instructions.md`
- 专项迁移 agent：`.github/agents/source-truth-migration.agent.md`
- 可重复执行的 prompt：`.github/prompts/continue-source-truth-migration.prompt.md`

任务状态规则见 `docs/TASKS.md`：

- `[x]` 已完成上游对照并验证
- `[-]` 已完成基础承接或局部闭环，但未完成完整对照验收
- `[ ]` 未开始或仅占位

## 项目说明

- 默认构建目标：`FramelessDialogDemo`（QML GUI）
- 构建系统：CMake + Ninja
- 语言标准：C++17
- 默认入口：`src/qml_gui/main_qml.cpp`
- 必需后端：`BUILD_LIBSLIC3R=ON`（3MF/STL 导入与切片能力依赖）

## 快速开始（Windows）

### 1) 配置

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCREALITY_QML_GUI=ON
```

### 2) 编译

```powershell
cmake --build build --config Release --target FramelessDialogDemo
```

### 3) 运行

```powershell
./build/FramelessDialogDemo.exe
```

## 说明

- 已禁用“假加载/Mock 导入成功”路径。
- 若未启用 `BUILD_LIBSLIC3R`，配置阶段会直接失败，避免生成可启动但无法真实导入模型的包。

## CI / CD（Tag 触发）

仓库已配置 tag 触发工作流：

- 工作流文件：`.github/workflows/tag-build.yml`
- 触发方式：`git push origin <tag>`（例如 `v0.1.1`）
- 流程内容：Windows 构建 + 产物打包 + 自动创建 GitHub Release

示例：

```powershell
git tag v0.1.1
git push origin v0.1.1
```

详细说明见：`CI_TAG_RELEASE.md`

## 目录入口

- 构建配置：`CMakeLists.txt`
- QML 主入口：`src/qml_gui/main_qml.cpp`
- Backend 组装：`src/qml_gui/BackendContext.h/.cpp`
- 核心服务：`src/core/services/`
- 核心 ViewModel：`src/core/viewmodels/`
- 界面页面：`src/qml_gui/pages/`
- 测试：`tests/`

## 相关文档

- 架构说明：`CrealityPrint_Qt_GUI_Rewrite_Architecture.md`
- 目录结构：`PROJECT_STRUCTURE.md`
- 任务追踪：`TASKS.md`
- Tag 发布说明：`CI_TAG_RELEASE.md`
