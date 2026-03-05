# 3DPrinter_Qt6

Qt6 重写版 3D 打印桌面端原型工程（QML 主路线）。

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
